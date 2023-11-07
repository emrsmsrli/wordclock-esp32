
#include "wc_globals.h"

namespace wordclock {

Preferences preferences;

void setup_globals()
{
    ASSERT(preferences.begin("wordclock"));
}

void halt()
{
    while (true) {}
}

} // namespace wordclock
