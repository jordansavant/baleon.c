#ifndef bool
#define false 0
#define true 1
typedef int bool; // or #define bool int
#endif

// Print text on center of std screen
void center(int row, char *title, int colorpair, int colorpair_reset, bool bold);
void clear_row(int row);

