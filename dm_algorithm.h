#ifndef DM_ALGORITHM
#define DM_ALGORITHM

#include "dm_defines.h"

///////////////////////////
// SHADOWCASTING
void dm_shadowcast(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double));
void dm_shadowcast_r(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double), int octant, int row, double start_slope, double end_slope, int xx, int xy, int yx, int yy);

///////////////////////////
// SPIRAL
struct dm_spiral {
	int x, y;
	int leg, layer, maxlayers;
};
struct dm_spiral dm_spiral(int maxlayers);
bool dm_spiralnext(struct dm_spiral*);

///////////////////////////
// BRESENHAM LINE
void dm_bresenham(int x1, int y1, int x2, int y2, bool (*is_blocked)(int, int), void (*on_visible)(int, int));

///////////////////////////
// RANDOM NUMBER GOD
void dm_seed(unsigned long seed);
double dm_randf();

///////////////////////////
// MATH
int dm_disti(int x1, int y1, int x2, int y2);
double dm_distf(double x1, double y1, double x2, double y2);


#endif
