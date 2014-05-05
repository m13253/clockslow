#ifndef _CLOCK_SLOW_CONFIG_H
#define _CLOCK_SLOW_CONFIG_H

// Application name, used when printing error messages.
#define APP_NAME "clockslow"

// Environment variable prefix
#define APP_ENV_PREFIX "CLOCKSLOW"
// clockslow accepts two variables:
// CLOCKSLOW_START: The epoch time when modified time coincide.
// CLOCKSLOW_FACTOR: The speed factor of time modification.
// CLOCKSLOW_VERBOSE: If not empty, print debugging information.

// Uncomment the following line to completely disable debugging information to speed up execution.
//#define DISABLE_VERBOSE

#endif
