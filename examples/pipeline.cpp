/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <vector>
#include <csdr/complex.hpp>
#include "pipeline.hpp"

constexpr int T_BUFSIZE = (1024 * 1024 / 4);

typedef Csdr::complex<short> CS16;
typedef Csdr::complex<float> CF32;


Pipeline::Pipeline(Csdr::UntypedSource* source, bool deleteUnusedModules):
    source(source),
    deleteUnusedModules(deleteUnusedModules),
    sourceWriter(nullptr),
    stages(std::vector<Stage*>())
{}

Pipeline::~Pipeline() {
    for (auto stage: stages) {
        delete stage->buffer;
        stage->buffer = nullptr;
        delete stage->runner;
        stage->runner = nullptr;
        if (deleteUnusedModules) {
            delete stage->module;
            stage->module = nullptr;
        }
    }
    if (!stages.empty()) {
        delete sourceWriter;
        sourceWriter = nullptr;
    }
    stages.clear();
    if (deleteUnusedModules) {
        delete source;
        source = nullptr;
    }
}

int Pipeline::addStage(Csdr::UntypedModule* module, int afterStage)
{
    Stage* previousStage = getStage(afterStage);
    connectStagesUntyped(previousStage, module);
    int distanceFromSource = previousStage == nullptr ? 1 : previousStage->distanceFromSource + 1;
    Stage* stage = new Stage(module, nullptr, nullptr, distanceFromSource);
    stages.push_back(stage);
    return stages.size();
}

int Pipeline::replaceStage(Csdr::UntypedModule* module, int stageNum)
{
    Stage* stage = getStage(stageNum);
    if (stage == nullptr)
        throw std::runtime_error("replacing the pipeline source is not implemented");

    // connect new module source
    untypedToTyped2<Csdr::Source, Csdr::UntypedModule,
                    Csdr::Source, Csdr::UntypedModule>(stage->module, module,
        [](auto oldSource, auto newSource){
            newSource->setWriter(oldSource->getWriter());
        });

    // connect new module sink
    untypedToTyped2<Csdr::Sink, Csdr::UntypedModule,
                    Csdr::Sink, Csdr::UntypedModule>(stage->module, module,
        [](auto oldSink, auto newSink){
            newSink->setReader(oldSink->getReader());
        });

    // stop old module and start new module
    if (stage->runner != nullptr) {
        stage->runner->stop();
        auto runner = new Csdr::AsyncRunner(module);
        delete stage->runner;
        stage->runner = runner;
    }

    // disconnect old module sink
    untypedToTyped1<Csdr::Sink, Csdr::UntypedModule>(stage->module,
        [](auto oldSink){
            oldSink->setReader(nullptr);
        });

    // disconnect old module source
    untypedToTyped1<Csdr::Source, Csdr::UntypedModule>(stage->module,
        [](auto oldSource){
            oldSource->setWriter(nullptr);
        });

    // delete the old module and replace the module in the stage
    if (deleteUnusedModules)
        delete stage->module;
    stage->module = module;

    return stageNum >= 0 ? stageNum : stages.size() + stageNum + 1;
}

void Pipeline::addWriter(Csdr::UntypedWriter* writer, int afterStage)
{
    Stage* previousStage = getStage(afterStage);
    if (previousStage == nullptr) {
        Csdr::UntypedWriter*& sw = sourceWriter;
        untypedToTyped2<Csdr::Source, Csdr::UntypedSource,
                        Csdr::Writer, Csdr::UntypedWriter>(source, writer,
            [&sw](auto s, auto w){
                // we'll set the writer later on when we start the pipeline
                sw = w;
            });
    } else {
        untypedToTyped2<Csdr::Source, Csdr::UntypedModule,
                        Csdr::Writer, Csdr::UntypedWriter>(previousStage->module, writer,
            [](auto s, auto w){
                s->setWriter(w);
            });
    }
    return;
}

Pipeline& Pipeline::operator|(Csdr::UntypedModule* module)
{
    addStage(module);
    return *this;
}

Pipeline& Pipeline::operator|(Csdr::UntypedWriter* writer)
{
    addWriter(writer);
    return *this;
}

void Pipeline::run()
{
    // start the stages in reverse order
    std::vector<Stage*> sortedStages = stages;
    std::sort(sortedStages.begin(), sortedStages.end(), [](Stage* a, Stage* b) {
        return a->distanceFromSource > b->distanceFromSource;
    });
    for (auto stage: sortedStages)
        stage->runner = new Csdr::AsyncRunner(stage->module);

    // finally start the source
    untypedToTyped2<Csdr::Source, Csdr::UntypedSource,
                    Csdr::Writer, Csdr::UntypedWriter>(source, sourceWriter,
        [](auto s, auto w){
            s->setWriter(w);
        });

    return;
}

void Pipeline::stop(double delay)
{
    // stop the source first
    untypedToTyped1<Csdr::TcpSource, Csdr::UntypedSource>(source,
        [](auto s){
            s->stop();
        }) ||
    untypedToTyped1<Csdrx::FileSource, Csdr::UntypedSource>(source,
        [](auto s){
            s->stop();
        });

    // sleep for some time to let all the downstreamm stages drain
    if (delay > 0) {
        double delay_s;
        double delay_frac = modf(delay, &delay_s);
        struct timespec request_time = { long(delay_s), long(delay_frac * 1e9) };
        nanosleep(&request_time, nullptr);
    }

    // stop the stages in forward order
    std::vector<Stage*> sortedStages = stages;
    std::sort(sortedStages.begin(), sortedStages.end(), [](Stage* a, Stage* b) {
        return b->distanceFromSource > a->distanceFromSource;
    });
    for (auto stage: sortedStages) {
        stage->runner->stop();
        delete stage->runner;
        stage->runner = nullptr;
    }

    return;
}

bool Pipeline::isRunning()
{
    bool isRunning = true;
    untypedToTyped1<Csdrx::FileSource, Csdr::UntypedSource>(source,
        [&isRunning](auto s){
            isRunning = s->isRunning();
        });
    return isRunning;
}

Csdr::UntypedSource* Pipeline::getSource()
{
    return source;
}

Csdr::UntypedModule* Pipeline::getModule(int stageNum)
{
    Stage* stage = getStage(stageNum);
    if (stage == nullptr)
        throw std::runtime_error("pipeline source is not a module");
    return stage->module;
}

Pipeline::Stage::Stage(Csdr::UntypedModule* module,
                       Csdr::UntypedWriter* buffer,
                       Csdr::AsyncRunner* runner,
                       int distanceFromSource):
    module(module),
    buffer(buffer),
    runner(runner),
    distanceFromSource(distanceFromSource)
{}

Pipeline::Stage::~Stage() {}

// internal functions
Pipeline::Stage* Pipeline::getStage(int stageNum) const
{
    int stgnum = stageNum >= 0 ? stageNum : stages.size() + stageNum + 1;
    // stage 0 is the pipeline source
    return stgnum == 0 ? nullptr : stages.at(stgnum - 1);
}

void Pipeline::connectStagesUntyped(Stage* previousStage, Csdr::UntypedModule* module)
{
    bool ok = false;
    if (auto from = previousStage == nullptr ?
                    dynamic_cast<Csdr::Source<CF32>*>(source) :
                    dynamic_cast<Csdr::Source<CF32>*>(previousStage->module)) {
        if (auto to = dynamic_cast<Csdr::Sink<CF32>*>(module)) {
            ok = true;
            connectStagesTyped<CF32>(from, to, previousStage);
        }
    } else if (auto from = previousStage == nullptr ?
                    dynamic_cast<Csdr::Source<CS16>*>(source) :
                    dynamic_cast<Csdr::Source<CS16>*>(previousStage->module)) {
        if (auto to = dynamic_cast<Csdr::Sink<CS16>*>(module)) {
            ok = true;
            connectStagesTyped<CS16>(from, to, previousStage);
        }
    } else if (auto from = previousStage == nullptr ?
                    dynamic_cast<Csdr::Source<float>*>(source) :
                    dynamic_cast<Csdr::Source<float>*>(previousStage->module)) {
        if (auto to = dynamic_cast<Csdr::Sink<float>*>(module)) {
            ok = true;
            connectStagesTyped<float>(from, to, previousStage);
        }
    } else if (auto from = previousStage == nullptr ?
                    dynamic_cast<Csdr::Source<short>*>(source) :
                    dynamic_cast<Csdr::Source<short>*>(previousStage->module)) {
        if (auto to = dynamic_cast<Csdr::Sink<short>*>(module)) {
            ok = true;
            connectStagesTyped<short>(from, to, previousStage);
        }
    } else if (auto from = previousStage == nullptr ?
                    dynamic_cast<Csdr::Source<unsigned char>*>(source) :
                    dynamic_cast<Csdr::Source<unsigned char>*>(previousStage->module)) {
        if (auto to = dynamic_cast<Csdr::Sink<unsigned char>*>(module)) {
            ok = true;
            connectStagesTyped<unsigned char>(from, to, previousStage);
        }
    }

    if (!ok)
        throw std::runtime_error(std::string("type does not match from ") + (previousStage == nullptr ? typeid(*source).name() : typeid(*previousStage->module).name()) + " to " + typeid(*module).name());

    return;
}

template <typename T>
void Pipeline::connectStagesTyped(Csdr::Source<T>* source, Csdr::Sink<T>* sink, Stage* previousStage)
{
    Csdr::Ringbuffer<T>* buffer;
    if (previousStage == nullptr) {
        if (sourceWriter == nullptr) {
            buffer = new Csdr::Ringbuffer<T>(T_BUFSIZE);
            sourceWriter = buffer;
        } else {
            buffer = dynamic_cast<Csdr::Ringbuffer<T>*>(sourceWriter);
        }
    } else {
        if (previousStage->buffer == nullptr) {
            buffer = new Csdr::Ringbuffer<T>(T_BUFSIZE);
            previousStage->buffer = buffer;
        } else {
            buffer = dynamic_cast<Csdr::Ringbuffer<T>*>(previousStage->buffer);
        }
    }
    sink->setReader(new Csdr::RingbufferReader<T>(buffer));
    // for the actual source we'll set the writer later on when we start the pipeline
    if (previousStage != nullptr)
        source->setWriter(buffer);
    return;
}

// meta functions
template <template<typename> typename T, typename U, typename P>
bool Pipeline::untypedToTyped1(U* untyped, P pred)
{
    if (auto typed = dynamic_cast<T<CF32>*>(untyped)) {
        pred(typed);
    } else if (auto typed = dynamic_cast<T<CS16>*>(untyped)) {
        pred(typed);
    } else if (auto typed = dynamic_cast<T<float>*>(untyped)) {
        pred(typed);
    } else if (auto typed = dynamic_cast<T<short>*>(untyped)) {
        pred(typed);
    } else if (auto typed = dynamic_cast<T<unsigned char>*>(untyped)) {
        pred(typed);
    } else {
        return false;
    }
    return true;
}

template <template<typename> typename T1, typename U1,
          template<typename> typename T2, typename U2,
          typename P>
bool Pipeline::untypedToTyped2(U1* untyped1, U2* untyped2, P pred)
{
    bool ok = false;
    if (auto typed1 = dynamic_cast<T1<CF32>*>(untyped1)) {
        if (auto typed2 = dynamic_cast<T2<CF32>*>(untyped2)) {
            ok = true;
            pred(typed1, typed2);
        }
    } else if (auto typed1 = dynamic_cast<T1<CS16>*>(untyped1)) {
        if (auto typed2 = dynamic_cast<T2<CS16>*>(untyped2)) {
            ok = true;
            pred(typed1, typed2);
        }
    } else if (auto typed1 = dynamic_cast<T1<float>*>(untyped1)) {
        if (auto typed2 = dynamic_cast<T2<float>*>(untyped2)) {
            ok = true;
            pred(typed1, typed2);
        }
    } else if (auto typed1 = dynamic_cast<T1<short>*>(untyped1)) {
        if (auto typed2 = dynamic_cast<T2<short>*>(untyped2)) {
            ok = true;
            pred(typed1, typed2);
        }
    } else if (auto typed1 = dynamic_cast<T1<unsigned char>*>(untyped1)) {
        if (auto typed2 = dynamic_cast<T2<unsigned char>*>(untyped2)) {
            ok = true;
            pred(typed1, typed2);
        }
    }

    if (!ok)
        throw std::runtime_error(std::string("type does not match from ") + typeid(*untyped1).name() + " to " + typeid(*untyped2).name());
    return ok;
}
