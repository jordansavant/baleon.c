#include <stdio.h>
#include <time.h>

void fun()
{
	printf("fun() starts \n");
	printf("Press enter to stop fun \n");
	while(1) {
		if (getchar())
			break;
	}
	printf("fun() ends \n");
}

int main(void) {

	struct timespec start, end;
	clock_gettime(CLOCK_REALTIME, &start);
	fun();
	clock_gettime(CLOCK_REALTIME, &end);

	long seconds = end.tv_sec - start.tv_sec;
	long nanos = end.tv_nsec - start.tv_nsec;

	// clock underflow?
	if (start.tv_nsec > end.tv_nsec) {
		seconds--;
		nanos += 1000000000;
	}

	printf("seconds %ld nanos %ld total %e \n", seconds, nanos, (double)seconds + (double)nanos/(double)1000000000);

	return 0;
}
