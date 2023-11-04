
#include "wc_neopixel.h"
#include "wc_globals.h"

namespace wordclock {

namespace color {

namespace {

const char pref_current_color[] = "pref_current_color";
RgbColor current_color{255, 255, 0};

}

const RgbColor black{0};
const RgbColor red{255, 0, 0};

void setup()
{
    wordclock::preferences.getBytes(pref_current_color, &current_color, sizeof(current_color));
}

RgbColor current()
{
    return current_color;
}

void set_current(RgbColor new_color)
{
    current_color = new_color;
    wordclock::preferences.putBytes(pref_current_color, &current_color, sizeof(current_color));
    // todo update all leds
}

RgbColor from_string(const String& str)
{
    RgbColor color;
    sscanf(str.c_str(), "#%02" SCNx8 "02%" SCNx8 "02%" SCNx8, &color.R, &color.G, &color.B);
    return color;
}

String to_string(RgbColor color)
{
    char rgb_buf[8];
    snprintf(rgb_buf, 8, "#%02" PRIx8 "02%" PRIx8 "02%" PRIx8, color.R, color.G, color.B);
    return String{rgb_buf};
}

}

namespace neopixel {

namespace {

bus pixel_bus{num_pixels, gpio_pin};

}

void setup()
{
    pinMode(gpio_pin, OUTPUT);
    pixel_bus.Begin();
}

bus& get_bus()
{
    return pixel_bus;
}

}

}
