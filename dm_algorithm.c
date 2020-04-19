#include <stdlib.h>
#include "dm_defines.h"
#include "mt_rand.h"
#include "dm_algorithm.h"


///////////////////////////
// SHADOWCASTING

unsigned int rand_seed = 0;
unsigned int shadowcast_id = 0;
int shadow_multiples[4][8] = {
    {1,  0,  0, -1, -1,  0,  0,  1},
    {0,  1, -1,  0,  0, -1,  1,  0},
    {0,  1,  1,  0,  0, -1, -1,  0},
    {1,  0,  0,  1, -1,  0,  0, -1},
};

void dm_shadowcast_r(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double), int octant, int row, double start_slope, double end_slope, int xx, int xy, int yx, int yy)
{
	// If light start is less than light end, return
	if (start_slope < end_slope)
		return;

	double next_start_slope = start_slope;

	// Loop moves outward from current row (or column if octant) to final radius in rows
	// row is passed in since this is recursive
	for (int i = row; i <= radius; i++) {
		bool blocked = false;

		// Loop moves across columns (or row if octant) until the right most
		// border is reached
		for (int dx = -i, dy = -i; dx <= 0; dx++) {
			// l_slope and r_slope store the slopes of the left and right extremities of the square we're considering:
			double l_slope = (dx - 0.5f) / (dy + 0.5f);
			double r_slope = (dx + 0.5f) / (dy - 0.5f);

			if (start_slope < r_slope)
				continue;
			else if (end_slope > l_slope)
				break;

			int sax = dx * xx + dy * xy;
			int say = dx * yx + dy * yy;
			if ((sax < 0 && abs(sax) > x) || (say < 0 && abs(say) > y))
				continue;

			int ax = x + sax;
			int ay = y + say;
			// Commenting this out to remove dependency on Map
			// looks like bounds checking for perf boost in edget of map
			//if (ax >= map.shadowcastGetWidth() || ay >= map.shadowcastGetHeight())
			if (ax >= xmax || ay >= ymax)
				continue;

			// Our light beam is touching this square; light it
			int radius2 = radius * radius;
			if ((int)(dx * dx + dy * dy) < radius2)
				on_visible(ax, ay, (double)i / (double)radius);

			if (blocked) {
				// We're scanning a row of blocked squares
				if (is_blocked(ax, ay)) {
					next_start_slope = r_slope;
					continue;
				} else {
					blocked = false;
					start_slope = next_start_slope;
				}
			} else if (is_blocked(ax, ay)) {
				blocked = true;
				next_start_slope = r_slope;
				dm_shadowcast_r(x, y, xmax, ymax, radius, is_blocked, on_visible, octant, i + 1, start_slope, l_slope, xx, xy, yx, yy);
			}
		}

		if (blocked)
			break;
	}
}

void dm_shadowcast(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double))
{
	shadowcast_id++;
	on_visible(x, y, 0);

	for (int i = 0; i < 8; i++) {
		dm_shadowcast_r(x, y, xmax, ymax, radius, is_blocked, on_visible, i, 1, 1.0, 0.0, shadow_multiples[0][i], shadow_multiples[1][i], shadow_multiples[2][i], shadow_multiples[3][i]);
	}
}



///////////////////////////
// RANDOM NUMBER GOD

void dm_seed(unsigned long seed)
{
	sgenrand(seed);
}

double dm_randf()
{
	return genrandf();
}

