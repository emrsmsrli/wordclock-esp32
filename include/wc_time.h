// Copyright (C) 2023 Emre Simsirli

#ifndef WORDCLOCK_WC_TIME_H
#define WORDCLOCK_WC_TIME_H

#include <ctime>

#include <Arduino.h>

#include "wc_span.h"

namespace wordclock { namespace time {

void setup();

span<const String> all_timezones();
const String& timezone();
void set_timezone(const String& timezone);

void update_from_sntp();
bool is_updated_from_sntp();
const std::tm& get(bool cache = true);
bool is_night();

}} //namespace wordclock::time

#endif //WORDCLOCK_WC_TIME_H
