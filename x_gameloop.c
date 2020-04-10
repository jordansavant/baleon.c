#include <stdio.h>
#include <time.h>

void draw(double delta_s)
{
	printf("draw %fs - ", delta_s);
}

void update(double delta_s)
{
	printf("\nupdate %fs\n", delta_s);
}

int main(void)
{
	struct timespec new_time, actual_time;
	clock_gettime(CLOCK_REALTIME, &actual_time);
	double timer = 0;
	double dt = .0166666667; // 60fps in seconds

	while (1) {
		clock_gettime(CLOCK_REALTIME, &new_time);
		double frame_s = new_time.tv_sec - actual_time.tv_sec;
		double frame_ns = new_time.tv_nsec - actual_time.tv_nsec;
		if (actual_time.tv_nsec > new_time.tv_nsec) { // clock underflow
			frame_s--;
			frame_ns += 1000000000;
		}
		frame_s = (double)frame_s + (double)frame_ns / (double)1000000000;

		// avoid spiral of death
		if (frame_s > .250) {
			frame_s = .250;
		}

		// update in dt sized chunks
		timer += frame_s;
		while (timer >= dt) {
			// TODO
			// sf::Time gtu = sf::seconds(dt);
			// update(gtu);
			update(dt);
			timer -= dt;
		}

		// draw in every frame
		// TODO
		// sf::Time gtd = sf::seconds(newTime - actualTime);
		// draw(*renderWindow, gtd);
		double draw_s = new_time.tv_sec - actual_time.tv_sec;
		double draw_ns = new_time.tv_nsec - actual_time.tv_nsec;
		if (actual_time.tv_nsec > new_time.tv_nsec) { // clock underflow
			draw_s--;
			draw_ns += 1000000000;
		}
		draw_s = (double)frame_s + (double)frame_ns / (double)1000000000;

		draw(draw_s);

		actual_time = new_time;
	}

	return 0;
}
