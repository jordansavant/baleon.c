#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "dm_defines.h"
#include "mt_rand.h"
#include "dm_algorithm.h"
#include "dm_debug.h"


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
// SPIRAL
struct dm_spiral dm_spiral(int maxlayers)
{
	struct dm_spiral s = {
		0, 0, 0, 1, maxlayers
	};
	return s;
}
// this process steps right, down, left, up (and then right again)
// to form a spiral to the layer specified
// "leg" is what direction it is going
// it follows its leg until it hits the next layer edge
// "layer" is how expanded the legs will run
bool dm_spiralnext(struct dm_spiral *s)
{
	switch (s->leg) {
	// Step right
	case 0:
		++s->x;
		// since stepping right finishes a layers circle we break if it has finished
		if (s->maxlayers > -1 && s->x > s->maxlayers)
			return false;
		if (s->x == s->layer)
			++s->leg;
		break;
	// Step down
	case 1:
		++s->y;
		if (s->y == s->layer)
			++s->leg;
		break;
	// Step left
	case 2:
		--s->x;
		if (-s->x == s->layer)
			++s->leg;
		break;
	// Step up
	case 3:
		--s->y;
		if (-s->y == s->layer) {
			s->leg = 0;
			++s->layer;
		}
		break;
	}

	return true;
}


///////////////////////////
// BRESENHAM LINE
// http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm - Algorithm
// http://roguebasin.roguelikedevelopment.org/index.php?title=Bresenham%27s_Line_Algorithm - Rogue Code
void dm_bresenham(int x1, int y1, int x2, int y2, bool (*is_blocked)(int, int), void (*on_visible)(int, int))
{
    if (is_blocked != NULL && is_blocked(x1, y1))
	    return;

    int delta_x = x2 - x1;
    // if x1 == x2, then it does not matter what we set here
    signed char const ix = (delta_x > 0) - (delta_x < 0);
    delta_x = abs(delta_x) << 1;

    int delta_y = y2 - y1;
    // if y1 == y2, then it does not matter what we set here
    signed char const iy = (delta_y > 0) - (delta_y < 0);
    delta_y = abs(delta_y) << 1;

    if (on_visible != NULL)
	    on_visible(x1, y1);

    if (delta_x >= delta_y) {
        // error may go below zero
        int error = delta_y - (delta_x >> 1);
        while (x1 != x2) {
            if ((error >= 0) && (error || (ix > 0))) {
                error -= delta_x;
                y1 += iy;
            }
            // else do nothing

            error += delta_y;
            x1 += ix;

	    if (is_blocked != NULL && is_blocked(x1, y1))
		    return;

	    if (on_visible != NULL)
		    on_visible(x1, y1);
        }
    } else {
        // error may go below zero
        int error = delta_x - (delta_y >> 1);
        while (y1 != y2) {
            if ((error >= 0) && (error || (iy > 0))) {
                error -= delta_y;
                x1 += ix;
            }
            // else do nothing

            error += delta_x;
            y1 += iy;

	    if (is_blocked != NULL && is_blocked(x1, y1))
		    return;

	    if (on_visible != NULL)
		    on_visible(x1, y1);
        }
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

int dm_randi()
{
	return (int)(dm_randf() * INT_MAX);
}

int dm_randii(int a, int b)
{
	int len = b - a;
	return (int)(dm_randf() * len + a);
}



///////////////////////////
// MATH
int dm_disti(int x1, int y1, int x2, int y2)
{
    double x = (double)(x1 - x2);
    double y = (double)(y1 - y2);
    return (int)sqrt(x*x + y*y);
}

double dm_distf(double x1, double y1, double x2, double y2)
{
    double x = x1 - x2;
    double y = y1 - y2;
    return sqrt(x*x + y*y);
}

