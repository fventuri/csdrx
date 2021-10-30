#include <iostream>
#include <csdr/converter.hpp>
#include <csdr/deemphasis.hpp>
#include <csdr/filter.hpp>
#include <csdr/fir.hpp>
#include <csdr/firdecimate.hpp>
#include <csdr/fmdemod.hpp>
#include <csdr/fractionaldecimator.hpp>
#include <csdr/shift.hpp>
#include <csdrx/pipeline.hpp>
#include <csdrx/pulseaudiowriter.hpp>
#include <csdrx/sdrplaysource.hpp>

constexpr int T_BUFSIZE = (1024 * 1024 / 4);


using namespace Csdr;
using namespace Csdrx;

int main()
{
    typedef complex<float> CF32;

    auto hamming = new HammingWindow();
    auto prefilter = new LowPassFilter<float>(0.5 / (4.166666666666667 - 0.03), 0.03, hamming);

    auto rspdx = new SDRplaySource<CF32>("", 2000000, 90.4e6, "Antenna C");

    Pipeline p(rspdx, true);
    p | new ShiftAddfast(0.25)
      | new FirDecimate(10, 0.015, hamming)
      | new FilterModule<CF32>(new BandPassFilter<CF32>(-0.375, 0.375, 0.0016, hamming))
      | new FmDemod()
      | new FractionalDecimator<float>(4.166666666666667, 12, prefilter)
      | new WfmDeemphasis(48000, 7.5e-05)
      | new Converter<float, short>()
      | new PulseAudioWriter<short>(48000, 10240, "fm_receiver_user_input");

    auto tuner = dynamic_cast<ShiftAddfast*>(p.getModule(1));

    p.run();

    while (true) {
        std::string command;

        std::cout << "RX> ";
        bool eof = !(bool) (std::getline(std::cin, command));

        // process command
        double d_value;
        if (command == "quit" || command == "q"  || eof) {
            break;
        } else if (command == "") {    // ignore empty lines
            continue;
        } else if (sscanf(command.c_str(), "F%lf", &d_value) == 1) {
            rspdx->setFrequency(d_value);
        } else if (sscanf(command.c_str(), "X%lf", &d_value) == 1) {
            tuner->setRate(d_value);
        } else {
            std::cerr << "*** invalid command: " << command << std::endl;
        }
    }

    p.stop();

    return 0;
}
