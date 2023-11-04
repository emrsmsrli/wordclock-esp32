
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

constexpr uint8_t num_pixels = 116;
constexpr uint8_t unused_pixel_idx = 89;
constexpr uint8_t gpio_pin = GPIO_NUM_32;

using bus = NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod>;

void setup();
bus& get_bus();

} // namespace neopixel

}

#endif //WORDCLOCK_WC_COLOR_H
