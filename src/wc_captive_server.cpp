
#include "wc_captive_server.h"

#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "wc_color.h"
#include "wc_wifi.h"
#include "wc_time.h"

namespace wordclock { namespace captive {

namespace {

[[maybe_unused]] const char captive_log_tag[] = "captive";

DNSServer dns_server;
AsyncWebServer web_server(80);

String index_html = R"html(
    <!DOCTYPE HTML>
    <html>
    <head>
      <title>WordClock</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body, html {
          height: 100%;
          width: 100%;
          margin: 0px;
          display: flex;
          flex-direction: column;
        }
        .color_form {
          display: flex;
          flex-direction: column;
          flex: 1;
        }
        .color_picker {
          flex: 1;
        }
        label { display: block; }
        input[type='submit'],
        input[type='color'],
        label { margin-top: 1rem; }
      </style>
    </head>
    <body>
      <h1>WordClock</h1>
      <form class="wifi_form" action="/update_wifi">
        <div>
          <label for="wifi_ssid">SSID:</label>
          <input type="text" id="wifi_ssid" name="wifi_ssid" value="{wifi_ssid}" />
        </div>
        <div>
          <label for="wifi_pass">Password:</label>
          <input type="text" id="wifi_pass" name="wifi_pass" value="{wifi_pass}" />
        </div>
        <input type="submit" value="Update WiFi credentials">
      </form>

      <form class="color_form" action="/update_color">
        <div class="color_picker">
          <input type="color" name="color" value="{current_color_hex}" style="width:100%;height:100%;">
        </div>
        <input type="submit" value="Update color">
      </form>
    </body>
    </html>)html";

String fill_page_details()
{
    String page = index_html;
    page.replace("{wifi_ssid}", wordclock::wifi::ssid());
    page.replace("{wifi_pass}", wordclock::wifi::pass());

    const RgbColor& color_hex = wordclock::color::current;
    char rgb_buf[8];
    snprintf(rgb_buf, 8, "#%02X%02X%02x", color_hex.R, color_hex.G, color_hex.B);
    page.replace("{current_color_hex}", rgb_buf);
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
            ESP_LOGV(captive_log_tag, "WiFi SSID changed: %s", new_wifi_ssid.c_str());
            wordclock::wifi::set_ssid(new_wifi_ssid);
            creds_changed = true;
        }

        if (new_wifi_pass != wordclock::wifi::pass()) {
            ESP_LOGV(captive_log_tag, "WiFi pass changed: %s", new_wifi_pass.c_str());
            wordclock::wifi::set_pass(new_wifi_pass);
            creds_changed = true;
        }

        if (creds_changed) {
            wordclock::wifi::connect_one_shot(wordclock::time::update_from_sntp);
        }

        request->send(200, "text/html",
                      "WiFi credentials successfully updated <br>"
                      "<a href=\"/\">Return to main page</a>");
    });

    web_server.on("/color", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("color")) {
            ESP_LOGW(captive_log_tag, "update color params not found");
            return;
        }
        const String& new_color_str = request->getParam("color")->value();
        RgbColor new_color;
        sscanf(new_color_str.c_str(), "#%02hhX%02hhX%02hhX", &new_color.R, &new_color.G, &new_color.B);

        if (new_color != wordclock::color::current()) {
            wordclock::color::set_current(new_color);
        }

        request->send(200, "text/html",
                      "Color successfully updated <br>"
                      "<a href=\"/\">Return to main page</a>");
    });

    dns_server.start(53, "*", WiFi.softAPIP());
    web_server.addHandler(new captive_req_handler()).setFilter(ON_AP_FILTER);
    web_server.begin();
}

void process_next_request()
{
    dns_server.processNextRequest();
}

}}
