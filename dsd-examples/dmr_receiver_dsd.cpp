#include <csignal>
#include <iostream>
#include <csdr/agc.hpp>
#include <csdr/converter.hpp>
#include <csdr/fftfilter.hpp>
#include <csdr/filter.hpp>
//#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/fmdemod.hpp>
#include <csdr/fractionaldecimator.hpp>
#include <csdr/shift.hpp>
#include <csdrx/dsddecoder.hpp>
#include <csdrx/pipeline.hpp>
#include <csdrx/pulseaudiowriter.hpp>
#include <csdrx/sdrplaysource.hpp>

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

    auto dmr_decoder = new DsdDecoder("dmr", stdout);
    dmr_decoder->setSlots(3);
    dmr_decoder->setQuiet();
    dmr_decoder->setFormatTextRefresh(1.0);         // refresh text every 1s

    auto agc = new Agc<short>();
    agc->setAttack(0.01);
    agc->setDecay(0.0001);
    agc->setMaxGain(30);
    agc->setInitialGain(3);
    agc->setHangTime(600);

    auto rspdx = new SDRplaySource<CF32>("", 1000000, 441.9e6, "Antenna C");

    Pipeline p(rspdx, true);
    p | new ShiftAddfast(-0.1)
      | new FirDecimate(20, 0.0072, hamming, 0.48)
      | new FractionalDecimator<CF32>(1.04167, 12, nullptr)
//      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FilterModule<CF32>(new FftBandPassFilter(-0.0833333, 0.0833333, 0.00666667, hamming))
      | new FmDemod()
      | new Converter<float, short>()
      | dmr_decoder
      | agc
      | new PulseAudioWriter<short>(8000, 10240, "dmr_receiver_dsd");

    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}
