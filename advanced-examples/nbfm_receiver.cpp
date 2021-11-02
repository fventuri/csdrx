/* Copyright 2021 Franco Spinelli IW2DHW */

// ricevitore NBFM con CSDR++ e la pipeline di Franco Venturi
//
//  riceve i samples da SDRPlay come I&Q complessi
//  esegue uno shift 0.005 pari a 0.005x2016000=10080
//  decima per 42 con una transition bw di 10080 (0.005) 
//  filtra con un bandpass da -0.26 a 0.26 e transition 0.0016 
//  quindi, operando a 48000, da -12480 a 12480 e transition 78
//  decodifica NBFM e applica la relativa de-enfasi
//  poi AGC slow, limit a 1.0, conversione a interi 16 bit e audio
//
//

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
    
    // inizio controllo dei parametri
    double freq = 144800000;
    double s_rate = 2016000;
    double offset = 10080;
    int opt;
    
    while ((opt = getopt(argc, argv, "f:s:o:")) != -1) {
        switch (opt) {
        case 'f':
             if(sscanf(optarg, "%lf", &freq) != 1) { 
                std::cerr << "Frequenza non valida: " << optarg << std::endl;
                return 1;
             }
             std::cerr << "Frequeza: " << freq << std::endl;
             break;           
        case 's':
             if(sscanf(optarg, "%lf", &s_rate) != 1) { 
                std::cerr << "Sample Rate non valido: " << optarg << std::endl;
                return 1;
             }
             std::cerr << "Sample rate: " << s_rate << std::endl;            
             break;           
        case 'o':
             if(sscanf(optarg, "%lf", &offset) != 1) {
                std::cerr << "Offset non valido: " << optarg << std::endl;
                return 1;
             }
             std::cerr << "Offset: " << offset << std::endl;            
             break;           
        default:
             std::cerr << "Usage: %s [-f fred] [-s samplerate [-o offset]\n" << argv[0] << std::endl;
             return 1;
        }
    }

    // a questo punto devo calcolare il dato da mettere come ShiftAddition sulla base
    // del samplerate e dell'offset
    double shift = offset / s_rate;
    //std::cerr << "Shift: " << shift << std::endl;
    freq = freq + offset;
            
    // AGC slow (parametri da slow agc di pycsdr a parte i Gain)
    auto agc = new Agc<float>();
    agc->setAttack(0.01);
    agc->setDecay(0.0001);
    agc->setMaxGain(30);
    agc->setInitialGain(1);
    agc->setHangTime(600);

    //auto rspdx = new SDRplaySource<CF32>("", 2016000, 144810080);
    auto rspdx = new SDRplaySource<CF32>("", s_rate, freq);

    Pipeline p(rspdx, true );
    p | new ShiftAddfast(shift)
      | new FirDecimate(42, 0.005, hamming)
      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.26, 0.26, 0.0016, hamming))
      | new FmDemod()
      | new Limit(1.0)
      | new NfmDeephasis(48000) // default NBFM filter for sample rate
      | agc
      | new Converter<float, short>()
      | new PulseAudioWriter<short>(48000, 10240, "nbfm_receiver");

    p.run();

    // set IF AGC
    rspdx->setIFGainReduction(0);

    while (true) {
        std::string command;

        std::cout << "RX> ";
        std::getline(std::cin, command);

        //std::cerr << "ricevuto command: " << command.c_str() << std::endl;

        // process command - definiti F[freq], R[RFGain] r i get relativi
        double d_value;
        //if (command == "quit" || command == "q"  || eof) {
        if (command == "quit" || command == "q" ) {
            break;
        } else if (command == "") {    // ignore empty lines
            continue;
        } else if (sscanf(command.c_str(), "F%lf", &d_value) == 1) {
            rspdx->setFrequency(d_value + offset);
        } else if (sscanf(command.c_str(), "R%lf", &d_value) == 1) {
            rspdx->setRFLnaState(d_value);
        } else if (command == "r") {
            std::cout << std::to_string(rspdx->getRFLnaState()) << std::endl;
        } else if (command == "f") {
            std::cout << std::to_string(rspdx->getFrequency() - offset) << std::endl;
        } else {
            std::cerr << "*** invalid command: " << command << std::endl;
        }
    }
    p.stop();

    return 0;
}

