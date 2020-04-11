#include <time.h>

#ifndef bool
#define false 0
#define true 1
typedef int bool; // or #define bool int
#endif

// Gets the difference in seconds between two timespecs (ts1 - ts2)
double timediff_s(struct timespec *ts1, struct timespec *ts2);

// Game Loop class
// will allow gametimer_done() to be infinitely run until it reaches the designated time
// maintains its own internal timers
struct gametimer
{
	double wait_s;
	struct timespec ts_start;
	struct timespec ts_end;
};

struct gametimer gametimer_new(double wait_s);
void gametimer_set(double wait_s, struct gametimer *gt);
bool gametimer_done(struct gametimer *gl);

// DeltaTimer class
// relies on an exteranal timer system to ssend in delta second snapshots
struct deltatimer
{
	double seconds_s;
	double counter_s;
	double wait_s;
};
struct deltatimer deltatimer_new(double wait_s);
bool deltatimer_update(struct deltatimer *gt, double delta_s);
