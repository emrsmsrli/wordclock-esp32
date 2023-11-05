
#ifndef WORDCLOCK_WC_WIFI_H
#define WORDCLOCK_WC_WIFI_H

#include <Arduino.h>

namespace wordclock { namespace wifi {

void setup();

const String& ssid();
const String& pass();

void set_ssid(const String& ssid);
void set_pass(const String& pass);

using on_connect_fun = void();
void connect_one_shot(on_connect_fun* on_wifi_connect);

}}

#endif //WORDCLOCK_WC_WIFI_H
