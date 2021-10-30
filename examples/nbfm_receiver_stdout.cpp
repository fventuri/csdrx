/* Copyright 2021 Franco Spinelli IW2DHW */

#include <csignal>
#include <iostream>
#include <csdr/converter.hpp>
#include <csdr/deemphasis.hpp>
#include <csdr/filter.hpp>
#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/fmdemod.hpp>
#include <csdr/agc.hpp>
#include <csdr/limit.hpp>
#include <csdr/power.hpp>
#include <csdr/shift.hpp>
#include <csdrx/filesource.hpp>
#include <csdrx/pipeline.hpp>

constexpr int T_BUFSIZE = (1024 * 1024 / 4);


bool terminate = false;

void sigint_handler(int sig)
{
    terminate = true;
}


using namespace Csdr;
using namespace Csdrx;

int main()
{
    typedef complex<float> CF32;

    auto hamming = new HammingWindow();

    Pipeline p(new FileSource<CF32>(), true);
    p | new ShiftAddfast(0.005)
      | new FirDecimate(42, 0.005, hamming)
      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.26, 0.26, 0.0016, hamming))
      | new FmDemod()
      | new NfmDeephasis(48000)
      | new Agc<float>()
      | new Limit(1.0)
      | new Converter<float, short>()
      | new StdoutWriter<short>();


    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}

