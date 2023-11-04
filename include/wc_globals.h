
#ifndef WORDCLOCK_WC_GLOBALS_H
#define WORDCLOCK_WC_GLOBALS_H

#include "Preferences.h"

#define ASSERT(cond) do { if (!(cond)) { ESP_LOGE(::wordclock::assert_log_tag, #cond " failed"); ::wordclock::halt(); } } while(false)

namespace wordclock {

extern Preferences preferences;

constexpr const char* assert_log_tag = "assert";

void setup_globals();
[[noreturn]] void halt();

}

#endif //WORDCLOCK_WC_GLOBALS_H
