#ifndef DM_ALGORITHM
#define DM_ALGORITHM

#include "dm_defines.h"

///////////////////////////
// SHADOWCASTING
void dm_shadowcast(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double));
void dm_shadowcast_r(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double), int octant, int row, double start_slope, double end_slope, int xx, int xy, int yx, int yy);

///////////////////////////
// RANDOM NUMBER GOD
void dm_seed(unsigned long seed);
double dm_randf();

#endif
