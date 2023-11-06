
#include <Arduino.h>

#include "wc_time.h"
#include "wc_neopixel.h"
#include "wc_wifi.h"
#include "wc_captive_server.h"
#include "wc_globals.h"

namespace {

[[maybe_unused]] const char main_log_tag[] = "main";

}

void setup()
{
    Serial.begin(115200);

    wordclock::setup_globals();
    wordclock::color::setup();
    wordclock::wifi::setup();
    wordclock::captive::setup();
    wordclock::neopixel::setup();

    wordclock::neopixel::show_loading_led();
    wordclock::wifi::connect_one_shot(wordclock::time::update_from_sntp);
    if (wordclock::time::is_updated_from_sntp()) {
        wordclock::neopixel::hide_loading_led();
    }
}

void loop()
{
    wordclock::captive::process_next_request();
    wordclock::neopixel::loop();
}
