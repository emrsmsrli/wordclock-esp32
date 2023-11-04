
#include "wc_time.h"

#include <Arduino.h>
#include <esp_log.h>
#include <esp_sntp.h>

namespace wordclock { namespace time {

namespace {

[[maybe_unused]] const char sntp_log_tag[] = "sntp";

bool is_time_fetched = false;
std::tm local_time{};

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
    constexpr uint32_t retry_count = 40;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGV(sntp_log_tag, "waiting for system time to be set... (%d/%d)", retry, retry_count);
        delay(250);
    }

    if (retry == retry_count) {
        ESP_LOGE(sntp_log_tag, "failed to fetch time from SNTP");
        return;
    }

    // Europe/Sweden
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", /*overwrite*/1);
    tzset();

    is_time_fetched = true;
    ESP_LOGV(sntp_log_tag, "time fetched successfully");
}

bool is_updated_from_sntp()
{
    return is_time_fetched;
}

const std::tm& get()
{
    std::time_t now = 0;
    std::time(&now);
    localtime_r(&now, &local_time);
    return local_time;
}

}}
