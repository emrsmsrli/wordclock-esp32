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
#include "wc_span.h"

namespace wordclock { namespace captive {

namespace {

[[maybe_unused]] const char captive_log_tag[] = "captive";

DNSServer dns_server;
AsyncWebServer web_server(80);

String index_html = R"html(<!doctypehtml><html><head><style>:root{--active-brightness:0.85;--border-radius:5px;--box-shadow:2px 2px 10px;--color-accent:#118bee15;--color-bg:#fff;--color-bg-secondary:#e9e9e9;--color-link:#118bee;--color-secondary:#920de9;--color-secondary-accent:#920de90b;--color-shadow:#f4f4f4;--color-table:#118bee;--color-text:#000;--color-text-secondary:#999;--font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Oxygen-Sans,Ubuntu,Cantarell,"Helvetica Neue",sans-serif;--hover-brightness:1.2;--justify-important:center;--justify-normal:left;--line-height:1.5;--width-card:285px;--width-card-medium:460px;--width-card-wide:800px;--width-content:1080px}@media (prefers-color-scheme:dark){:root[color-mode=user]{--color-accent:#0097fc4f;--color-bg:#333;--color-bg-secondary:#555;--color-link:#0097fc;--color-secondary:#e20de9;--color-secondary-accent:#e20de94f;--color-shadow:#bbbbbb20;--color-table:#0097fc;--color-text:#f7f7f7;--color-text-secondary:#aaa}}html{scroll-behavior:smooth}@media (prefers-reduced-motion:reduce){html{scroll-behavior:auto}}body{background:var(--color-bg);color:var(--color-text);font-family:var(--font-family);line-height:var(--line-height);margin:0;overflow-x:hidden;padding:0}header,main{margin:0 auto;max-width:var(--width-content);padding:3rem 1rem}section{display:flex;flex-wrap:wrap;justify-content:var(--justify-important)}[hidden]{display:none}main header{padding-top:0}header{text-align:var(--justify-important)}section header{padding-top:0;width:100%}h1,h2,h3,h4,h5,h6{line-height:var(--line-height);text-wrap:balance}input[type=submit]{border-radius:var(--border-radius);display:inline-block;font-size:medium;font-weight:700;line-height:var(--line-height);margin:.5rem 0;padding:1rem 2rem}input[type=submit]{font-family:var(--font-family)}input[type=submit]:active{filter:brightness(var(--active-brightness))}input[type=submit]:hover{cursor:pointer;filter:brightness(var(--hover-brightness))}input[type=submit]{background-color:var(--color-link);border:2px solid var(--color-link);color:var(--color-bg)}input:disabled{background:var(--color-bg-secondary);border-color:var(--color-bg-secondary);color:var(--color-text-secondary);cursor:not-allowed}input[type=submit][disabled]:hover{filter:none}form{border:1px solid var(--color-bg-secondary);border-radius:var(--border-radius);box-shadow:var(--box-shadow) var(--color-shadow);display:block;max-width:var(--width-card-wide);min-width:var(--width-card);padding:1.5rem;text-align:var(--justify-normal)}form header{margin:1.5rem 0;padding:1.5rem 0}input,label,select{display:block;font-size:inherit;max-width:var(--width-card-wide)}input[type=checkbox],input[type=radio]{display:inline-block}input[type=checkbox]+label,input[type=radio]+label{display:inline-block;font-weight:400;position:relative;top:1px}input[type=range]{padding:.4rem 0}input,select{border:1px solid var(--color-bg-secondary);border-radius:var(--border-radius);margin-bottom:1rem;padding:.4rem .8rem}input[type=text]{width:calc(100% - 1.6rem)}input[readonly]{background-color:var(--color-bg-secondary)}label{font-weight:700;margin-bottom:.2rem}</style><meta name="viewport" content="width=device-width,initial-scale=1"><title>WordClock</title></head><body><main><section><form action="/update"><header><h2>WordClock</h2></header><label for="wifi_ssid">SSID:</label><input value="{wifi_ssid}" name="wifi_ssid" id="wifi_ssid"><label for="wifi_pass">Password:</label><input type="password" value="{wifi_pass}" name="wifi_pass" id="wifi_pass"><label for="tz">Timezone:</label><select name="tz" id="tz">{tz}</select><label for="color">Color:</label><div style="width:var(--width-card);height:150px;border-radius:var(--border-radius);overflow:hidden"><input type="color" value="{current_color_hex}" name="color" id="color" style="border:0;padding:0;width:200%;height:200%;cursor:pointer;transform:translate(-25%,-25%)"></div><input value="Update" type="submit"></form></section></main></body></html>)html";

String fill_page_details()
{
    String page = index_html;
    page.replace("{wifi_ssid}", wordclock::wifi::ssid());
    page.replace("{wifi_pass}", wordclock::wifi::pass());

    const String tz_str = []() {
        const time::tz_info& current_timezone = wordclock::time::timezone();
        const span<const time::tz_info> timezones = wordclock::time::all_timezones();
        String str;
        for (const time::tz_info& timezone : timezones) {
            str.concat("<option value=\"");
            str.concat(timezone.tz);
            if (current_timezone.tz == timezone.tz) {
                str.concat(R"(" selected>)");
            } else {
                str.concat(R"(">)");
            }
            str.concat(timezone.pretty_name);
            str.concat("</option>");
        }
        return str;
    }();

    page.replace("{tz}", tz_str);
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

    web_server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("wifi_ssid") || !request->hasParam("wifi_pass") ||
          !request->hasParam("tz") || !request->hasParam("color")) {
            ESP_LOGW(captive_log_tag, "update params not found");
            request->send(200, "text/html",
              "Failed to acquire parameters <br>"
              "<a href=\"/\">Return to main page</a>");
            return;
        }

        // TZ
        {
            const String& new_tz = request->getParam("tz")->value();
            if (new_tz != wordclock::time::timezone().tz) {
                ESP_LOGI(captive_log_tag, "timezone changed: %s", new_tz.c_str());
                wordclock::time::set_timezone(new_tz);
            }
        }

        // WIFI
        {
            bool creds_changed = false;

            const String& new_wifi_ssid = request->getParam("wifi_ssid")->value();
            if (new_wifi_ssid != wordclock::wifi::ssid()) {
                ESP_LOGI(captive_log_tag, "WiFi SSID changed: %s", new_wifi_ssid.c_str());
                wordclock::wifi::set_ssid(new_wifi_ssid);
                creds_changed = true;
            }

            const String& new_wifi_pass = request->getParam("wifi_pass")->value();
            if (new_wifi_pass != wordclock::wifi::pass()) {
                ESP_LOGI(captive_log_tag, "WiFi pass changed: %s", new_wifi_pass.c_str());
                wordclock::wifi::set_pass(new_wifi_pass);
                creds_changed = true;
            }

            if (creds_changed && !wordclock::time::is_updated_from_sntp()) {
                wordclock::wifi::connect_one_shot(wordclock::time::update_from_sntp);
            }
        }

        // COLOR
        {
            const String& new_color_str = request->getParam("color")->value();
            const RgbColor new_color = wordclock::color::from_string(new_color_str);
            if (new_color != wordclock::color::current()) {
                ESP_LOGI(captive_log_tag, "color changed: %s", new_color_str.c_str());
                wordclock::color::set_current(new_color);
            }
        }

        request->send(200, "text/html",
          "Settings successfully updated <br>"
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
