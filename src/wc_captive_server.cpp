// Copyright (C) 2023 Emre Simsirli

#include "wc_captive_server.h"

#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "wc_globals.h"
#include "wc_neopixel.h"
#include "wc_wifi.h"
#include "wc_time.h"

namespace wordclock { namespace captive {

namespace {

[[maybe_unused]] const char captive_log_tag[] = "captive";

DNSServer dns_server;
AsyncWebServer web_server(80);

String index_html = R"html(<!doctypehtml><html><head><title>WordClock</title><meta content="width=device-width,initial-scale=1" name="viewport"><style>body,html{height:100%;width:100%;margin:0;display:flex;flex-direction:column}.color_form{display:flex;flex-direction:column;flex:1}.color_picker{flex:1}label{display:block}input[type=color],input[type=submit],label{margin-top:1rem}</style></head><body><h1>WordClock</h1><form action="/update_wifi" class="wifi_form"><div><label for="wifi_ssid">SSID:</label><input value="{wifi_ssid}" name="wifi_ssid" id="wifi_ssid"></div><div><label for="wifi_pass">Password:</label> <input value="{wifi_pass}" name="wifi_pass" id="wifi_pass"></div><input value="Update WiFi credentials" type="submit"></form><form action="/update_color" class="color_form"><div class="color_picker"><input name="color" value="{current_color_hex}" type="color" style="width:100%;height:100%"></div><input value="Update color" type="submit"></form></body></html>)html";

String fill_page_details()
{
    String page = index_html;
    page.replace("{wifi_ssid}", wordclock::wifi::ssid());
    page.replace("{wifi_pass}", wordclock::wifi::pass());
    page.replace("{current_color_hex}", wordclock::color::to_string(wordclock::color::current()));
    return page;
}

class captive_req_handler : public AsyncWebHandler {
public:
    bool canHandle(AsyncWebServerRequest* request) override
    {
        return true;
    }

    void handleRequest(AsyncWebServerRequest* request) override
    {
        String html = fill_page_details();
        request->send_P(200, "text/html", html.c_str());
    }
};

TaskHandle_t task_captive_handle = nullptr;

[[noreturn]] void task_captive(void*)
{
    while (true) {
        dns_server.processNextRequest();
        delay(100);
    }
}

}

void setup()
{
    ESP_LOGI(captive_log_tag, "starting captive portal servers");

    web_server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        String html = fill_page_details();
        request->send_P(200, "text/html", html.c_str());
    });

    web_server.on("/update_wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("wifi_ssid") || !request->hasParam("wifi_pass")) {
            ESP_LOGW(captive_log_tag, "update wifi params not found");
            return;
        }
        const String& new_wifi_ssid = request->getParam("wifi_ssid")->value();
        const String& new_wifi_pass = request->getParam("wifi_pass")->value();

        bool creds_changed = false;
        if (new_wifi_ssid != wordclock::wifi::ssid()) {
            ESP_LOGI(captive_log_tag, "WiFi SSID changed: %s", new_wifi_ssid.c_str());
            wordclock::wifi::set_ssid(new_wifi_ssid);
            creds_changed = true;
        }

        if (new_wifi_pass != wordclock::wifi::pass()) {
            ESP_LOGI(captive_log_tag, "WiFi pass changed: %s", new_wifi_pass.c_str());
            wordclock::wifi::set_pass(new_wifi_pass);
            creds_changed = true;
        }

        if (creds_changed && !wordclock::time::is_updated_from_sntp()) {
            wordclock::wifi::connect_one_shot(wordclock::time::update_from_sntp);
        }

        request->send(200, "text/html",
          "WiFi credentials successfully updated <br>"
          "<a href=\"/\">Return to main page</a>");
    });

    web_server.on("/update_color", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("color")) {
            ESP_LOGW(captive_log_tag, "update color params not found");
            return;
        }

        const String& new_color_str = request->getParam("color")->value();
        const RgbColor new_color = wordclock::color::from_string(new_color_str);
        if (new_color != wordclock::color::current()) {
            ESP_LOGI(captive_log_tag, "color changed: %s", new_color_str.c_str());
            wordclock::color::set_current(new_color);
        }

        request->send(200, "text/html",
          "Color successfully updated <br>"
          "<a href=\"/\">Return to main page</a>");
    });

    dns_server.start(53, "*", WiFi.softAPIP());
    web_server.addHandler(new captive_req_handler()).setFilter(ON_AP_FILTER);
    web_server.begin();

    xTaskCreate(
      task_captive,
      "task_captive",
      default_task_stack_size / 2,
      /*pvParameters=*/nullptr,
      /*uxPriority=*/1,
      &task_captive_handle);
}

}} // namespace wordclock::captive
