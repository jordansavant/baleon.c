#include <time.h>

#ifndef bool
#define false 0
#define true 1
typedef int bool; // or #define bool int
#endif

// Gets the difference in seconds between two timespecs (ts1 - ts2)
double dm_timediff_s(struct timespec *ts1, struct timespec *ts2);

// Game Loop class
// will allow gametimer_done() to be infinitely run until it reaches the designated time
// maintains its own internal timers
struct dm_gametimer
{
	double wait_s;
	struct timespec ts_start;
	struct timespec ts_end;
};

struct dm_gametimer dm_gametimer_new(double wait_s);
void dm_gametimer_set(double wait_s, struct dm_gametimer *gt);
bool dm_gametimer_done(struct dm_gametimer *gl);

// DeltaTimer class
// relies on an exteranal timer system to ssend in delta second snapshots
struct dm_deltatimer
{
	double seconds_s;
	double counter_s;
	double wait_s;
};
struct dm_deltatimer dm_deltatimer_new(double wait_s);
bool dm_deltatimer_update(struct dm_deltatimer *gt, double delta_s);
