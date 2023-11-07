
#include "wc_alarm.h"

#include <vector>
#include <Ticker.h>

#include "wc_time.h"

namespace wordclock { namespace alarm {

namespace {

std::vector<alarm> alarms;
Ticker ticker;

void loop()
{
    const std::tm& time = time::get();
    for (const alarm& alarm : alarms) {
        if (alarm.min == time.tm_min && alarm.hour == time.tm_hour &&
            alarm.day != wordclock::alarm::invalid_val && alarm.day == time.tm_mday &&
            alarm.month != wordclock::alarm::invalid_val && alarm.month == time.tm_mon)
        {
            if (alarm.on_fire) {
                alarm.on_fire();
            }
        }
    }
}

}

alarm alarm::from_hours_mins(uint8_t hour, uint8_t min, alarm_on_fire_func* on_fire)
{
    alarm a;
    a.on_fire = on_fire;
    a.min = min;
    a.hour = hour;
    return a;
}

alarm alarm::from_date(uint8_t hour, uint8_t min, uint8_t day, uint8_t month, alarm_on_fire_func* on_fire)
{
    alarm a = from_hours_mins(hour, min, on_fire);
    a.day = day;
    a.month = month;
    return a;
}

void setup()
{
    ticker.attach(/*seconds=*/60, loop);
}

void register_alarm(const alarm& a)
{
    alarms.push_back(a);
}

}} // namespace wordclock::alarm

