#include <csignal>
#include <iostream>
#include <csdr/agc.hpp>
#include <csdr/dcblock.hpp>
#include <csdr/fftfilter.hpp>
#include <csdr/filter.hpp>
//#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/fmdemod.hpp>
#include <csdr/fractionaldecimator.hpp>
#include <csdr/shift.hpp>
#include <csdrx/filesource.hpp>
#include <csdrx/pipeline.hpp>
#include <csdrx/pulseaudiowriter.hpp>
#include <digiham/digitalvoice_filter.hpp>
#include <digiham/gfsk_demodulator.hpp>
#include <digiham/mbe_synthesizer.hpp>
#include <digiham/meta.hpp>
#include <digiham/rrc_filter.hpp>
#include <digiham/ysf_decoder.hpp>

constexpr int T_BUFSIZE = (1024 * 1024 / 4);


bool terminate = false;

void sigint_handler(int sig)
{
    terminate = true;
}


using namespace Csdr;
using namespace Csdrx;
using namespace Digiham;

int main()
{
    typedef complex<float> CF32;

    auto hamming = new HammingWindow();

    auto ysf_decoder = new Ysf::Decoder();
    ysf_decoder->setMetaWriter(new FileMetaWriter(stdout));

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

    Pipeline p(new FileSource<CF32>(), true);
    p | new ShiftAddfast(-0.0125)
      | new FirDecimate(41, 0.0036, hamming, 0.492)
      | new FractionalDecimator<CF32>(1.01626, 12, nullptr)
//      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FilterModule<CF32>(new FftBandPassFilter(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FmDemod()
      | new DcBlock()
      | new RrcFilter::WideRrcFilter()
      | new Fsk::GfskDemodulator(10)
      | ysf_decoder
      | ysf_mbe_synthesizer
      | new DigitalVoice::DigitalVoiceFilter()
      | agc
      | new PulseAudioWriter<short>(8000, 10240, "ysf_receiver");

    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}
