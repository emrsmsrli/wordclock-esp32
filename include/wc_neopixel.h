// Copyright (C) 2023 Emre Simsirli

#ifndef WORDCLOCK_WC_COLOR_H
#define WORDCLOCK_WC_COLOR_H

#include <NeoPixelBus.h>

namespace wordclock {

namespace color {

extern const RgbColor black;
extern const RgbColor red;

RgbColor current();
void set_current(RgbColor new_color);

RgbColor from_string(const String& str);
String to_string(RgbColor color);

} // namespace color

namespace neopixel {

void setup();

void show_loading_led();
void hide_loading_led();

} // namespace neopixel

}

#endif //WORDCLOCK_WC_COLOR_H
