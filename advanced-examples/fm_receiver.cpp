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
#include <csdr/fractionaldecimator.hpp>
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
    auto prefilter = new LowPassFilter<float>(0.5 / (4.166666666666667 - 0.03), 0.03, hamming);

    // inizio controllo dei parametri
    double freq = 89300000;
    double s_rate = 2000000;
    double offset = 500000;
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

    //auto rspdx = new SDRplaySource<CF32>("", 2000000, 89800000);
    auto rspdx = new SDRplaySource<CF32>("", s_rate, freq);

    Pipeline p(rspdx, true );
    //p | new ShiftAddfast(0.25)
    p | new ShiftAddfast(shift)
      | new FirDecimate(10, 0.015, hamming)
      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.375, 0.375, 0.0016, hamming))
      | new FmDemod()
      | new FractionalDecimator<float>(4.166666666666667, 12, prefilter)
      | new WfmDeemphasis(48000, 7.5e-05)
      | new Converter<float, short>()
      | new PulseAudioWriter<short>(48000, 10240, "fm_receiver");

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
