/* Copyright 2021 Franco Spinelli IW2DHW */

// ricevitore solo USB con CSDR++ e la pipeline di Franco Venturi
//
//  riceve i samples su stdin come I&Q complessi
//  esegue uno shift 0.005 pari a 0.005x2016000=10080
//  decima per 42 con una transition bw di 0.005 ???
//  filtra con un bandpass da 0.0 a 0.06 e transition 0.05
//  quindi, operando a 48000, da 0 a 2880 e transition 2400
//  decodifica SSB (realpart) e quindi USB per come filtro
//  poi AGC, limit a 1.0, conversione a interi 16 bit e audio
//
//


#include <csignal>
#include <iostream>
#include <csdr/converter.hpp>
#include <csdr/filter.hpp>
#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/realpart.hpp>
#include <csdr/agc.hpp>
#include <csdr/limit.hpp>
#include <csdr/shift.hpp>
#include <csdrx/filesourcemeasuredelay.hpp>
#include <csdrx/pulseaudiowritermeasuredelay.hpp>
#include "pipeline.hpp"

constexpr int T_BUFSIZE = (1024 * 1024 / 4);

constexpr int NUM_DELAY_SLOTS = 100;
int num_delay_slots = NUM_DELAY_SLOTS;
struct timespec delay_slots[NUM_DELAY_SLOTS];
double delay_interval = 10.0;    // print results every 10 seconds


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

    Pipeline p(new FileSourceMeasureDelay<CF32>(2000000), true);
    p | new ShiftAddfast(0.005)
      | new FirDecimate(42, 0.005, hamming)
      | new FilterModule<CF32>(new BandPassFilter<CF32>(0.0, 0.06, 0.05, hamming))
      | new Realpart()
      | new Agc<float>()
      | new Limit(1.0)
      | new Converter<float, short>()
      | new PulseAudioWriterMeasureDelay<short>(48000, 10240, "usb_receiver");


    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}
