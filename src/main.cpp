
#include <Arduino.h>

#include "wc_time.h"
#include "wc_color.h"
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

    wordclock::wifi::connect_one_shot(wordclock::time::update_from_sntp);
}

void loop()
{
    wordclock::captive::process_next_request();

    if (wordclock::time::is_updated_from_sntp()) {
        const std::tm& localtime = wordclock::time::get();
        const char* t = asctime(&localtime);
        ESP_LOGV(main_log_tag, "local time: %s", t);
    }
    delay(1000);
}