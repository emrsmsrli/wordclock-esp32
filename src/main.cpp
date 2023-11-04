
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
    Serial.begin(9600);

    wordclock::setup_globals();
    wordclock::color::setup();
    wordclock::wifi::setup();
    wordclock::captive::setup();
    wordclock::neopixel::setup();

    wordclock::wifi::connect_one_shot(wordclock::time::update_from_sntp);
}

void loop()
{
    wordclock::captive::process_next_request();
    wordclock::neopixel::loop();
}