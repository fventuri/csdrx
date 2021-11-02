/* Copyright 2021 Franco Spinelli IW2DHW */

#include <csignal>
#include <iostream>
#include <unistd.h>
#include <csdr/agc.hpp>
#include <csdr/fftfilter.hpp>
#include <csdr/filter.hpp>
//#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/fmdemod.hpp>
#include <csdr/fractionaldecimator.hpp>
//#include <csdr/source.hpp>
#include <csdr/shift.hpp>
#include <digiham/dc_block.hpp>
#include <digiham/digitalvoice_filter.hpp>
#include <digiham/gfsk_demodulator.hpp>
#include <digiham/mbe_synthesizer.hpp>
#include <digiham/meta.hpp>
#include <digiham/rrc_filter.hpp>
#include <digiham/ysf_decoder.hpp>
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
using namespace Digiham;

int main(int argc, char *argv[])
{
    typedef complex<float> CF32;

    auto hamming = new HammingWindow();

    auto ysf_decoder = new Ysf::Decoder();
    ysf_decoder->setMetaWriter(new FileMetaWriter(stderr));

    auto ysf_mbe_synthesizer = new Mbe::MbeSynthesizer();
    auto ysf_mode = new Mbe::DynamicMode([](auto code) -> Mbe::Mode* {
        unsigned short ysf_vw_control_words[] = { 0x5805, 0x6b08, 0x3010, 0x0000, 0x0000, 0x9001 };
        switch (code) {
            case 0: return new Mbe::TableMode(33);
            case 2: return new Mbe::TableMode(34);
            case 3:
                return new Mbe::ControlWordMode(reinterpret_cast<short*>(ysf_vw_control_words));
            default: return nullptr;
        }
    });
    ysf_mbe_synthesizer->setMode(ysf_mode);

    auto agc = new Agc<short>();
    agc->setAttack(0.01);
    agc->setDecay(0.0001);
    agc->setMaxGain(30);
    agc->setInitialGain(3);
    agc->setHangTime(600);
    
    // inizio controllo dei parametri
    double freq = 430750000;
    double s_rate = 2000000;
    double offset = -25000;
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

    //auto rspdx = new SDRplaySource<CF32>("", 2016000, 430725000);
    auto rspdx = new SDRplaySource<CF32>("", s_rate, freq);
    
    Pipeline p(rspdx, true );
    p | new ShiftAddfast(shift)
      | new FirDecimate(41, 0.0036, hamming, 0.492)
      | new FractionalDecimator<CF32>(1.01626, 12, nullptr)
//      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FilterModule<CF32>(new FftBandPassFilter(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FmDemod()
      | new DcBlock::DcBlock()
      | new RrcFilter::WideRrcFilter()
      | new Fsk::GfskDemodulator(10)
      | ysf_decoder
      | ysf_mbe_synthesizer
      | new DigitalVoice::DigitalVoiceFilter()
      | agc
      | new PulseAudioWriter<short>(8000, 10240, "ysf_receiver");

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
