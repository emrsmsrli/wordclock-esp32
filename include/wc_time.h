// Copyright (C) 2023 Emre Simsirli

#ifndef WORDCLOCK_WC_TIME_H
#define WORDCLOCK_WC_TIME_H

#include <ctime>

namespace wordclock { namespace time {

void update_from_sntp();
bool is_updated_from_sntp();
const std::tm& get(bool cache = true);
bool is_night();

}} //namespace wordclock::time

#endif //WORDCLOCK_WC_TIME_H
