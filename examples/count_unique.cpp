#include <csignal>
#include <iostream>
#include <csdr/writer.hpp>
#include <csdrx/pipeline.hpp>
#include <csdrx/sdrplaysource.hpp>

constexpr int T_BUFSIZE = (1024 * 1024 / 4);


bool terminate = false;

void sigint_handler(int sig)
{
    terminate = true;
}


using namespace Csdr;
using namespace Csdrx;

int main(int argc, char **argv)
{
    double samp_rate;
    if (argc == 1) {
        fprintf(stderr, "usage: %s <sample rate>\n", argv[0]);
        exit(1);
    }
    if (sscanf(argv[1], "%lg", &samp_rate) != 1) {
        fprintf(stderr, "invalid sample rate: %s\n", argv[1]);
        exit(1);
    }
    fprintf(stderr, "sample rate: %lg\n", samp_rate);

    typedef complex<float> CF32;

    auto rspdx = new SDRplaySource<CF32>("", samp_rate, 100e6, "Antenna C", 2);

    Pipeline p(rspdx, true);
    p | new VoidWriter<CF32>();

    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    // handle Ctrl-C
    signal(SIGINT, sigint_handler);
    while (!terminate && p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    return 0;
}
