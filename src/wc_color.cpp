
#include "wc_color.h"
#include "wc_globals.h"

namespace wordclock { namespace color {

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

}}
