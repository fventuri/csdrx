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
#include <digiham/dmr_decoder.hpp>
#include <digiham/gfsk_demodulator.hpp>
#include <digiham/mbe_synthesizer.hpp>
#include <digiham/meta.hpp>
#include <digiham/rrc_filter.hpp>

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

    auto dmr_decoder = new Dmr::Decoder();
    dmr_decoder->setMetaWriter(new FileMetaWriter(stdout));

    auto dmr_mbe_synthesizer = new Mbe::MbeSynthesizer();
    dmr_mbe_synthesizer->setMode(new Mbe::TableMode(33));

    auto agc = new Agc<short>();
    agc->setAttack(0.01);
    agc->setDecay(0.0001);
    agc->setMaxGain(30);
    agc->setInitialGain(3);
    agc->setHangTime(600);

    Pipeline p(new FileSource<CF32>(), true);
    p | new ShiftAddfast(-0.1)
      | new FirDecimate(20, 0.0072, hamming, 0.48)
      | new FractionalDecimator<CF32>(1.04167, 12, nullptr)
//      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FilterModule<CF32>(new FftBandPassFilter(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FmDemod()
      | new DcBlock()
      | new RrcFilter::WideRrcFilter()
      | new Fsk::GfskDemodulator(10)
      | dmr_decoder
      | dmr_mbe_synthesizer
      | new DigitalVoice::DigitalVoiceFilter()
      | agc
      | new PulseAudioWriter<short>(8000, 10240, "dmr_receiver");

    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}
