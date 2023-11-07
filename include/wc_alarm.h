
#ifndef WORDCLOCK_WC_ALARM_H
#define WORDCLOCK_WC_ALARM_H

#include <ctime>
#include <functional>

namespace wordclock { namespace alarm {

static constexpr uint8_t invalid_val = static_cast<uint8_t>(-1);

using alarm_on_fire_func = void();
struct alarm {
    alarm_on_fire_func* on_fire = nullptr;
    uint8_t min = 0;
    uint8_t hour = 0;
    uint8_t day = invalid_val;
    uint8_t month = invalid_val;

    static alarm from_hours_mins(uint8_t hour, uint8_t min, alarm_on_fire_func* on_fire);
    static alarm from_date(uint8_t hour, uint8_t min, uint8_t day, uint8_t month, alarm_on_fire_func* on_fire);
};

void setup();
void register_alarm(const alarm& a);

}} // namespace wordclock::alarm

#endif //WORDCLOCK_WC_ALARM_H
