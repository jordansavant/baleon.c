#ifndef DM_DEFINES
#define DM_DEFINES

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define STR_W_SIZE(str) str, sizeof(str) - 1

#include <stdbool.h>
// #ifndef bool
// #define false 0
// #define true 1
// typedef int bool; // or #define bool int
// #endif

#endif
