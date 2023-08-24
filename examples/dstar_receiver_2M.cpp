#include <csignal>
#include <iostream>
#include <csdr/agc.hpp>
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
#include <digiham/dc_block.hpp>
#include <digiham/digitalvoice_filter.hpp>
#include <digiham/dstar_decoder.hpp>
#include <digiham/fsk_demodulator.hpp>
#include <digiham/mbe_synthesizer.hpp>
#include <digiham/meta.hpp>

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

    auto dstar_decoder = new DStar::Decoder();
    dstar_decoder->setMetaWriter(new FileMetaWriter(stdout));

    auto dstar_mbe_synthesizer = new Mbe::MbeSynthesizer();
    short dstar_mode_control_words[] = { 0x3001, 0x6307, 0x0040, 0x0000, 0x0000, 0x4800 };
    dstar_mbe_synthesizer->setMode(new Mbe::ControlWordMode(dstar_mode_control_words));

    auto agc = new Agc<short>();
    agc->setAttack(0.01);
    agc->setDecay(0.0001);
    agc->setMaxGain(30);
    agc->setInitialGain(3);
    agc->setHangTime(600);

    Pipeline p(new FileSource<CF32>("", 2000000), true);
    p | new ShiftAddfast(-0.1895)
      | new FirDecimate(41, 0.0036, hamming, 0.492)
      | new FractionalDecimator<CF32>(1.01626, 12, nullptr)
//      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.0677083, 0.0677083, 0.00666667, hamming))
      | new FilterModule<CF32>(new FftBandPassFilter(-0.0677083, 0.0677083, 0.00666667, hamming))
      | new FmDemod()
      | new DcBlock::DcBlock()
      | new Fsk::FskDemodulator(10, false)
      | dstar_decoder
      | dstar_mbe_synthesizer
      | new DigitalVoice::DigitalVoiceFilter()
      | agc
      | new PulseAudioWriter<short>(8000, 10240, "dstar_receiver");

    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}
