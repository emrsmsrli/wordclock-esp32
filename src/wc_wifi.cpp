// Copyright (C) 2023 Emre Simsirli

#include "wc_wifi.h"

#include <Arduino.h>
#include <WiFi.h>

#include "wc_globals.h"

#ifndef WC_WIFI_AP_PASSWORD
  #define WC_WIFI_AP_PASSWORD "wordclock_default_ap_password"
#endif // WC_WIFI_AP_PASSWORD

namespace wordclock { namespace wifi {

namespace {

[[maybe_unused]] const char wifi_log_tag[] = "wifi";
const char pref_wifi_ssid[] = "pref_wifi_ssid";
const char pref_wifi_pass[] = "pref_wifi_pass";

String wifi_ssid;
String wifi_pass;

TaskHandle_t task_wifi_handle = nullptr;

void task_wifi(void* param)
{
    if (!wifi_ssid.isEmpty()) {
        ESP_LOGI(wifi_log_tag, "connecting WiFi to %s:%s ...", wifi_ssid.c_str(), wifi_pass.c_str());
        ASSERT(WiFi.begin(wifi_ssid, wifi_pass));

        constexpr uint32_t wait_delay = 100;
        uint32_t n_tries = 100;

        while (true) {
            if (WiFi.isConnected()) {
                ESP_LOGI(wifi_log_tag, "connected to WiFi on %s! updating time from SNTP...", WiFi.localIP().toString().c_str());
                if (auto* on_connect = reinterpret_cast<on_connect_fun*>(param)) {
                    on_connect();
                }
                break;
            } else {
                ESP_LOGI(wifi_log_tag, "waiting for WiFi connection (%d)...", n_tries);
            }

            delay(wait_delay);
            if (n_tries == 0) {
                ESP_LOGE(wifi_log_tag, "failed to connect to WiFi");
                break;
            } else {
                n_tries--;
            }
        }

        WiFi.disconnect();
    }

    vTaskDelete(task_wifi_handle);
    task_wifi_handle = nullptr;
}

}

void setup()
{
    wifi_ssid = preferences.getString(pref_wifi_ssid);
    wifi_pass = preferences.getString(pref_wifi_pass);

    ESP_LOGI(wifi_log_tag, "starting WiFi AP");

    ASSERT(WiFi.mode(WIFI_AP_STA));
    ASSERT(WiFi.softAP("WordClock-AP",  WC_WIFI_AP_PASSWORD));
}

const String& ssid()
{
    return wifi_ssid;
}

const String& pass()
{
    return wifi_pass;
}

void set_ssid(const String& new_ssid)
{
    wifi_ssid = new_ssid;
    wordclock::preferences.putString(pref_wifi_ssid, wifi_ssid);
}

void set_pass(const String& new_pass)
{
    wifi_pass = new_pass;
    wordclock::preferences.putString(pref_wifi_pass, wifi_pass);
}

void connect_one_shot(on_connect_fun* on_connect)
{
    xTaskCreate(
      task_wifi,
      "task_wifi",
      default_task_stack_size,
      reinterpret_cast<void*>(on_connect),
      /*uxPriority=*/2,
      &task_wifi_handle);
}

}} // namespace wordclock::wifi
