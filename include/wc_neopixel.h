
#ifndef WORDCLOCK_WC_COLOR_H
#define WORDCLOCK_WC_COLOR_H

#include <NeoPixelBus.h>

namespace wordclock {

namespace color {

extern const RgbColor black;
extern const RgbColor red;

void setup();
RgbColor current();
void set_current(RgbColor new_color);

RgbColor from_string(const String& str);
String to_string(RgbColor color);

} // namespace color

namespace neopixel {

void setup();
void loop();

} // namespace neopixel

}

#endif //WORDCLOCK_WC_COLOR_H
