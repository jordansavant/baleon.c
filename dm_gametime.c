#include "dm_gametime.h"

// Gets the difference in seconds between two timespecs (ts1 - ts2)
double dm_timediff_s(struct timespec *ts1, struct timespec *ts2)
{
	double diff_s = ts2->tv_sec - ts1->tv_sec;
	double diff_ns = ts2->tv_nsec - ts1->tv_nsec;
	if (ts1->tv_nsec > ts2->tv_nsec) { // clock underflow
		diff_s--;
		diff_ns += 1000000000;
	}
	return (double)diff_s + (double)diff_ns / (double)1000000000;
}

// Game Loop class
// will allow gametimer_done() to be infinitely run until it reaches the designated time
// maintains its own internal timers
struct dm_gametimer dm_gametimer_new(double wait_s)
{
	struct timespec ts_start, ts_end;
	clock_gettime(CLOCK_REALTIME, &ts_start);
	clock_gettime(CLOCK_REALTIME, &ts_end);
	struct dm_gametimer gl = {
		wait_s,
		ts_start,
		ts_end,
	};
	return gl;
}
void dm_gametimer_set(double wait_s, struct dm_gametimer *gt)
{
	gt->wait_s = wait_s;
	clock_gettime(CLOCK_REALTIME, &gt->ts_start);
}
bool dm_gametimer_done(struct dm_gametimer *gl)
{
	clock_gettime(CLOCK_REALTIME, &gl->ts_end);
	double diff_s = dm_timediff_s(&gl->ts_start, &gl->ts_end);
	return (diff_s > gl->wait_s);
}

// DeltaTimer class
// relies on an exteranal timer system to ssend in delta second snapshots
struct dm_deltatimer dm_deltatimer_new(double wait_s)
{
	struct dm_deltatimer gt = {0, 0, wait_s};
	return gt;
}
bool dm_deltatimer_update(struct dm_deltatimer *gt, double delta_s)
{
	gt->counter_s += delta_s;
	if (gt->counter_s >= gt->wait_s) {
		gt->counter_s = 0.0;
		return 1;
	}
	return 0;
}
