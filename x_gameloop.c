#include <stdio.h>
#include <time.h>

#define false 0
#define true 1
typedef int bool; // or #define bool int

// Gametimer class
struct gametimer
{
	double seconds_s;
	double counter_s;
	double wait_s;
};
struct gametimer gametimer_new(double wait_s)
{
	struct gametimer gt = {0, 0, wait_s};
	return gt;
}
bool gametimer_update(struct gametimer *gt, double delta_s)
{
	gt->counter_s += delta_s;
	if (gt->counter_s >= gt->wait_s) {
		gt->counter_s = 0.0;
		return true;
	}
	return false;
}

void draw(double delta_s)
{
	//printf("draw %fs - ", delta_s);
}

void update(double delta_s, struct gametimer *gttest)
{
	printf("update %fs\n", delta_s);
	if (gametimer_update(gttest, delta_s)) {
		printf("GAMETIMER HIT %fs\n", gttest->wait_s);
	}
}

// Gets the difference in seconds between two timespecs (ts1 - ts2)
double timediff_s(struct timespec *ts1, struct timespec *ts2)
{
	double diff_s = ts2->tv_sec - ts1->tv_sec;
	double diff_ns = ts2->tv_nsec - ts1->tv_nsec;
	if (ts1->tv_nsec > ts2->tv_nsec) { // clock underflow
		diff_s--;
		diff_ns += 1000000000;
	}
	return (double)diff_s + (double)diff_ns / (double)1000000000;
}

int main(void)
{
	struct timespec new_time, current_time;
	double timer = 0;
	double dt = .0166666667; // 60fps in seconds

	// gametimer tests
	struct gametimer gt = gametimer_new(1.5);

	// get starting time of game
	clock_gettime(CLOCK_REALTIME, &current_time);
	while (1) {
		// time since last loop
		clock_gettime(CLOCK_REALTIME, &new_time);
		double frame_s = timediff_s(&current_time, &new_time);

		// avoid spiral of death
		if (frame_s > .250)
			frame_s = .250;

		// only update in dt sized chunks
		timer += frame_s;
		while (timer >= dt) {
			update(dt, &gt);
			timer -= dt;
		}

		// draw in every frame
		double draw_s = timediff_s(&current_time, &new_time);
		draw(draw_s);

		// update current loop time to the new time
		current_time = new_time;
	}

	return 0;
}
