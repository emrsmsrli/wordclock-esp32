// Copyright (C) 2023 Emre Simsirli

#include "wc_time.h"

#include <Arduino.h>
#include <esp_log.h>
#include <esp_sntp.h>

#include "wc_globals.h"

namespace wordclock { namespace time {

namespace {

[[maybe_unused]] const char sntp_log_tag[] = "sntp";
const char pref_timezone[] = "pref_timezone";

volatile bool is_time_fetched = false;
std::tm local_time{};
uint32_t timezone_idx;

void cache_time()
{
    std::time_t now = 0;
    std::time(&now);
    localtime_r(&now, &local_time);
}

void setenv_tz()
{
    setenv("TZ", timezone().tz.c_str(), /*overwrite*/1);
    tzset();
    cache_time();
}

}

void setup()
{
    timezone_idx = preferences.getUInt(pref_timezone);
}

span<const tz_info> all_timezones()
{
    static const tz_info tz[]{
      tz_info{"<GMT+3>-3", "Europe/Istanbul"},
      tz_info{"GMT+0BST-1,M3.5.0/1,M10.5.0/2", "Europe/London"},
      tz_info{"CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Stockholm"},
    };
    return span<const tz_info>{tz, 3};
}

const tz_info& timezone()
{
    return all_timezones()[timezone_idx];
}

void set_timezone(const String& timezone)
{
    uint32_t idx = 0;
    for (const tz_info& tz : all_timezones()) {
        if (tz.tz == timezone) {
            if (timezone_idx != idx) {
                timezone_idx = idx;
                preferences.putUInt(pref_timezone, timezone_idx);
                setenv_tz();
            }
            return;
        }
        ++idx;
    }
}

void update_from_sntp()
{
    is_time_fetched = false;
    local_time = {};

    ESP_LOGI(sntp_log_tag, "initializing SNTP");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    uint32_t retry = 0;
    constexpr uint32_t retry_count = 100;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(sntp_log_tag, "waiting for system time to be set... (%d/%d)", retry, retry_count);
        delay(100);
    }

    if (retry == retry_count) {
        ESP_LOGE(sntp_log_tag, "failed to fetch time from SNTP");
        return;
    }

    setenv_tz();

    is_time_fetched = true;
    ESP_LOGI(sntp_log_tag, "time fetched successfully");
}

bool is_updated_from_sntp()
{
    return is_time_fetched;
}

const std::tm& get(bool cache)
{
    if (cache) {
        cache_time();
    }
    return local_time;
}

bool is_night()
{
    const int32_t h = local_time.tm_hour;
    return h >= 21 || h < 7;
}

}} // namespace wordclock::time
