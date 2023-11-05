
#include "wc_neopixel.h"

#include <NeoPixelAnimator.h>
#include <Ticker.h>

#include "wc_globals.h"
#include "wc_time.h"

namespace wordclock {

namespace neopixel {

void animate_all_lit(RgbColor start, RgbColor end);

}

namespace color {

const RgbColor black{0};
const RgbColor red{255, 0, 0};

namespace {

const char pref_current_color[] = "pref_color";
RgbColor current_color = red;

}

void setup()
{
    wordclock::preferences.getBytes(pref_current_color, &current_color, sizeof(current_color));
}

RgbColor current()
{
    return current_color;
}

void set_current(RgbColor new_color)
{
    neopixel::animate_all_lit(current_color, new_color);
    current_color = new_color;
    wordclock::preferences.putBytes(pref_current_color, &current_color, sizeof(current_color));
}

RgbColor from_string(const String& str)
{
    RgbColor color;
    sscanf(str.c_str(), "#%02" SCNx8 "%02" SCNx8 "%02" SCNx8, &color.R, &color.G, &color.B);
    return color;
}

String to_string(RgbColor color)
{
    char rgb_buf[8];
    snprintf(rgb_buf, 8, "#%02" PRIx8 "%02" PRIx8 "%02" PRIx8, color.R, color.G, color.B);
    return String{rgb_buf};
}

} // namespace color

namespace neopixel {

namespace {

[[maybe_unused]] const char neo_log_tag[] = "neo";

constexpr uint8_t num_pixels = 116;
constexpr uint8_t num_pixels_for_secs = 5;
constexpr uint8_t unused_pixel_idx = 89;
constexpr uint8_t gpio_pin = GPIO_NUM_12;

class led_array {
    uint8_t anim_idx;
    uint8_t start;
    uint8_t end;

public:
    led_array(uint8_t a, uint8_t s, uint8_t e) : anim_idx(a), start(s), end(e) {}

    void paint(RgbColor color) const;

    bool operator==(const led_array& other) const
    {
        return start == other.start && end == other.end;
    }

    bool operator!=(const led_array& other) const
    {
        return start != other.start || end != other.end;
    }

    uint8_t animator_idx() const { return anim_idx; };
};

led_array IT(0, 105, 107);
led_array IS(1, 108, 110);
led_array O_OCLOCK(2, 6, 12);
led_array O_PAST(3, 61, 65);
led_array O_TO(4, 72, 74);

led_array S[] = {
  led_array(5, 1, 2),      //S_1
  led_array(6, 2, 3),      //S_2
  led_array(7, 3, 4),      //S_3
  led_array(8, 4, 5),      //S_4
  led_array(9, 5, 6),      //S_5
};

led_array M_NONE(10, 0, 0);
led_array M_5(11, 90, 94);
led_array M_10(12, 75, 78);
led_array M_15(13, 97, 104);
led_array M_20(14, 83, 89);
led_array M_25(15, 83, 94);
led_array M_30(16, 79, 83);

led_array H[] = {
  led_array(17, 58, 61),    // H_1
  led_array(18, 55, 58),    // H_2
  led_array(19, 50, 55),    // H_3
  led_array(20, 39, 43),    // H_4
  led_array(21, 43, 47),    // H_5
  led_array(22, 47, 50),    // H_6
  led_array(23, 66, 71),    // H_7
  led_array(24, 17, 22),    // H_8
  led_array(25, 35, 39),    // H_9
  led_array(26, 14, 17),    // H_10
  led_array(27, 22, 28),    // H_11
  led_array(28, 28, 34)     // H_12
};

NeoPixelAnimator animator{29 /*n_led_array_count*/};

struct led_state {
    led_array seconds = S[0];
    led_array minute = M_NONE;
    led_array hour = H[0];
    led_array oclock = O_OCLOCK;
};

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> pixel_bus{num_pixels, gpio_pin};
Ticker ticker;

led_state current_leds;
led_state last_leds;

bool do_leds_needs_calculation = false;

// timer task callback
void on_ticker_tick()
{
    ESP_LOGV(neo_log_tag, "ticker");
    do_leds_needs_calculation = true;
}

bool is_night()
{
    const int32_t h = time::get(/*cache=*/false).tm_hour;
    return h >= 21 || h < 7;
}

void calculate_next_leds()
{
    const std::tm& localtime = wordclock::time::get();
    const uint8_t sec = localtime.tm_sec;
    const uint8_t min = localtime.tm_min;
    uint8_t hour = localtime.tm_hour;

    current_leds.seconds = S[sec % num_pixels_for_secs];

    if(min < 5) {                                   /// 0 - 5
        current_leds.oclock = O_OCLOCK;
        current_leds.minute = M_NONE;
    } else if(min < 35) {                           /// 5 - 35
        current_leds.oclock = O_PAST;
        if (min < 10) {                             // 5 - 10
            current_leds.minute = M_5;
        } else if (min < 15) {                      // 10 - 15
            current_leds.minute = M_10;
        } else if (min < 20) {                      // 15 - 20
            current_leds.minute = M_15;
        } else if (min < 25) {                      // 20 - 25
            current_leds.minute = M_20;
        } else if (min < 30) {                      // 25 - 30
            current_leds.minute = M_25;
        } else {                                    // 30 - 35
            current_leds.minute = M_30;
        }
    } else {                                        /// 35 - 60
        hour = static_cast<uint8_t>(hour + 1);
        current_leds.oclock = O_TO;
        if (min < 40) {                             // 35 - 40
            current_leds.minute = M_25;
        } else if (min < 45) {                      // 40 - 45
            current_leds.minute = M_20;
        } else if (min < 50) {                      // 45 - 50
            current_leds.minute = M_15;
        } else if (min < 55) {                      // 50 - 55
            current_leds.minute = M_10;
        } else {                                    // 55 - 60
            current_leds.minute = M_5;
        }
    }

    while (hour >= 12) {
        hour -= 12;
    }
    current_leds.hour = H[hour];
}

void start_animation(led_array arr, RgbColor start, RgbColor end, uint16_t animation_duration_ms = 250)
{
    animator.StartAnimation(arr.animator_idx(), animation_duration_ms, [=](const AnimationParam& param) {
        const float progress = NeoEase::CubicOut(param.progress);
        RgbColor updatedColor = RgbColor::LinearBlend(start, end, progress);
        if (is_night()) {
            updatedColor = updatedColor.Dim(0x20);
        }
        arr.paint(updatedColor);
    });
}

void start_new_animations()
{
    if (last_leds.seconds != current_leds.seconds) {
        start_animation(last_leds.seconds, color::current_color, color::black);
        start_animation(current_leds.seconds, color::black, color::current_color);
    }

    if (last_leds.minute != current_leds.minute) {
        if (last_leds.minute == M_20 && current_leds.minute == M_25) {
            start_animation(M_5, color::black, color::current_color);
        } else if (last_leds.minute == M_25 && current_leds.minute == M_20) {
            start_animation(M_5, color::current_color, color::black);
        } else {
            start_animation(last_leds.minute, color::current_color, color::black);
            start_animation(current_leds.minute, color::black, color::current_color);
        }
    }

    if (last_leds.hour != current_leds.hour) {
        start_animation(last_leds.hour, color::current_color, color::black);
        start_animation(current_leds.hour, color::black, color::current_color);
    }

    if (last_leds.oclock != current_leds.oclock) {
        start_animation(last_leds.oclock, color::current_color, color::black);
        start_animation(current_leds.oclock, color::black, color::current_color);
    }
}

void led_array::paint(RgbColor color) const
{
    for (uint32_t i = start; i < end; ++i) {
        if (i == unused_pixel_idx) {
            continue;
        }
        pixel_bus.SetPixelColor(i, color);
    }
}

}

void animate_all_lit(RgbColor start, RgbColor end)
{
    constexpr uint16_t animation_duration_ms = 100;
    start_animation(IT, start, end, animation_duration_ms);
    start_animation(IS, start, end, animation_duration_ms);
    start_animation(current_leds.seconds, start, end, animation_duration_ms);
    start_animation(current_leds.minute, start, end, animation_duration_ms);
    start_animation(current_leds.hour, start, end, animation_duration_ms);
    start_animation(current_leds.oclock, start, end, animation_duration_ms);
}

void setup()
{
    ESP_LOGI(neo_log_tag, "setting up NeoPixelBus");

    pixel_bus.Begin();
    pixel_bus.Show();

    ticker.attach(1.f, on_ticker_tick);

    start_animation(IT, color::black, color::current_color);
    start_animation(IS, color::black, color::current_color);
}

void loop()
{
    if (!wordclock::time::is_updated_from_sntp()) {
        delay(100);
        return;
    }

    if (do_leds_needs_calculation) {
        last_leds = current_leds;
        calculate_next_leds();
        start_new_animations();

        do_leds_needs_calculation = false;
    }

    if (animator.IsAnimating()) {
        animator.UpdateAnimations();
        pixel_bus.Show();
    }
}

} // namespace neopixel

}
