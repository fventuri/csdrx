/* Copyright 2021 Franco Spinelli IW2DHW */

#include <csignal>
#include <iostream>
#include <unistd.h>
#include <csdr/converter.hpp>
#include <csdr/deemphasis.hpp>
#include <csdr/filter.hpp>
#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/fmdemod.hpp>
#include <csdr/agc.hpp>
#include <csdr/limit.hpp>
#include <csdr/power.hpp>
#include <csdr/source.hpp>
#include <csdr/shift.hpp>
#include <csdrx/sdrplaysource.hpp>
#include <csdrx/pulseaudiowriter.hpp>
#include <csdrx/pipeline.hpp>


constexpr int T_BUFSIZE = (1024 * 1024 / 4);


bool terminate = false;

void sigint_handler(int sig)
{
    terminate = true;
}


using namespace Csdr;
using namespace Csdrx;

int main(int argc, char *argv[])
{
    typedef complex<float> CF32;

    auto hamming = new HammingWindow();
    

    auto rspdx = new SDRplaySource<CF32>("", 2016000, 144810080);

    Pipeline p(rspdx, true );
    p | new ShiftAddfast(0.005)
      | new FirDecimate(42, 0.005, hamming)
      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.26, 0.26, 0.0016, hamming))
      | new FmDemod()
      | new NfmDeephasis(48000)
      | new Agc<float>()
      | new Limit(1.0)
      | new Converter<float, short>()
      | new PulseAudioWriter<short>(48000, 10240, "nbfm_receiver");

    p.run();
    
    struct timespec delay = { 0, 100000000 };   // 100ms delay

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}

