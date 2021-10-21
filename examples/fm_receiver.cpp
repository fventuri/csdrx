#include <csignal>
#include <iostream>
#include <csdr/converter.hpp>
#include <csdr/deemphasis.hpp>
#include <csdr/filter.hpp>
#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/fmdemod.hpp>
#include <csdr/fractionaldecimator.hpp>
#include <csdr/shift.hpp>
#include <csdrx/filesource.hpp>
#include <csdrx/pulseaudiowriter.hpp>
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
    auto prefilter = new LowPassFilter<float>(0.5 / (4.166666666666667 - 0.03), 0.03, hamming);

    Pipeline p(new FileSource<CF32>(), true);
    p | new ShiftAddfast(0.25)
      | new FirDecimate(10, 0.015, hamming)
      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.375, 0.375, 0.0016, hamming))
      | new FmDemod()
      | new FractionalDecimator<float>(4.166666666666667, 12, prefilter)
      | new WfmDeemphasis(48000, 7.5e-05)
      | new Converter<float, short>()
      | new PulseAudioWriter<short>(48000, 10240, "fm_receiver");

    auto tuner = dynamic_cast<ShiftAddfast*>(p.getModule(1));

    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    sleep(10);
    std::cerr << "changing station" << std::endl;
    tuner->setRate(-0.25);
    sleep(10);
    std::cerr << "changing station back" << std::endl;
    tuner->setRate(0.25);

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}
