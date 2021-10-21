/* -*- c++ -*- */
/*
 * Copyright 2021 Franco Venturi.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <algorithm>
#include <vector>
#include <csdr/async.hpp>
#include <csdr/module.hpp>
#include <csdr/ringbuffer.hpp>
#include <csdr/source.hpp>
#include <csdr/writer.hpp>
#include <csdrx/filesource.hpp>

class Pipeline {

    class Stage;

    public:
        Pipeline(Csdr::UntypedSource* source, bool deleteUnusedModules=false);
        ~Pipeline();
        int addStage(Csdr::UntypedModule* module, int afterStage=-1);
        int replaceStage(Csdr::UntypedModule* module, int stageNum);
        void addWriter(Csdr::UntypedWriter* writer, int afterStage=-1);
        Pipeline& operator|(Csdr::UntypedModule* module);
        Pipeline& operator|(Csdr::UntypedWriter* writer);
        void run();
        void stop(double delay=0);
        bool isRunning();
        Csdr::UntypedSource* getSource();
        Csdr::UntypedModule* getModule(int stagenum);
    private:
        Csdr::UntypedSource* source;
        bool deleteUnusedModules;
        Csdr::UntypedWriter* sourceWriter;
        std::vector<Stage*> stages;

        // internal functions
        Stage* getStage(int stageNum) const;
        void connectStagesUntyped(Stage* previousStage, Csdr::UntypedModule* module);
        template <typename T>
        void connectStagesTyped(Csdr::Source<T>* source, Csdr::Sink<T>* sink, Stage* previousStage);

        // meta functions
        template <template<typename> typename T, typename U, typename P>
        bool untypedToTyped1(U* untyped, P pred);
        template <template<typename> typename T1, typename U1,
                  template<typename> typename T2, typename U2,
                  typename P>
        bool untypedToTyped2(U1* untyped1, U2* untyped2, P pred);

    class Stage {
        public:
            Stage(Csdr::UntypedModule* module,
                  Csdr::UntypedWriter* buffer,
                  Csdr::AsyncRunner* runner, 
                  int distanceFromSource);
            ~Stage();

            Csdr::UntypedModule* module;
            Csdr::UntypedWriter* buffer;
            Csdr::AsyncRunner* runner;
            int distanceFromSource;
    };
};
