// simple NAVTEX decoder that reads 48kHz audio (signed 16 bits) from stdin

#include <iostream>
#include <csdr/converter.hpp>
#include <csdrx/filesource.hpp>
#include <csdrx/navtexdecoderwriter.hpp>
#include <csdrx/pipeline.hpp>

constexpr int T_BUFSIZE = (1024 * 1024 / 4);


using namespace Csdr;
using namespace Csdrx;

int main()
{
    Pipeline p(new FileSource<short>("", 48000), true);
    p | new Converter<short, float>()
      | new NavtexDecoderWriter<float>(48000, stdout);
 
    // disable buffering on stdout
    setvbuf(stdout, nullptr, _IONBF, 0);

    struct timespec delay = { 0, 100000000 };   // 100ms delay

    p.run();

    while (p.isRunning())
        nanosleep(&delay, nullptr);
    p.stop();

    fflush(stdout);

    return 0;
}
