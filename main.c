#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>

#include "gametime.h"
#include "draw.h"

// TOOLS
void msleep(int milliseconds)
{
	usleep(milliseconds * 1000);
}

// GAME CODE
enum game_state
{
	GS_START,
	GS_INTRO,
	GS_MAIN,
	GS_EXIT
};

bool g_setup()
{
	// setup ncurses
	initscr();
	if (has_colors()) {
		start_color();
	} else {
		printf("%s\n", "Colors not supported on this terminal");
		endwin(); // cleanup
		return false;
	}
	// colors
	init_pair(SCOLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(TCOLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(TCOLOR_OMINOUS, COLOR_RED, COLOR_BLACK);
	init_pair(TCOLOR_BLACK, COLOR_BLACK, COLOR_BLACK);

	// set primary color
	bkgd(COLOR_PAIR(SCOLOR_NORMAL));
	return true;
}

void g_teardown()
{
	// teardown ncurses
	endwin();
}

void g_intro()
{
	struct reel {
		int row;
		char *string;
		int colorpair;
		bool bold;
		double time_in;
		double time_out;
	};
	struct reel reels[3] = {
		{3, "It was an age of light.", TCOLOR_NORMAL, 1, 2, .25},
		{4, "It was an age of hope.", TCOLOR_NORMAL, 1, 2.5, .25},
		{6, "And then he came...", TCOLOR_OMINOUS, 1, 2, .25}
	};

	// roll in title but allow for escaping
	struct gametimer gt = gametimer_new(0);

	curs_set(0); // hide cursor
	nodelay(stdscr, true); // dont pause interrupt on getch

	for (int i=0; i<3; i++) {
		center(reels[i].row, reels[i].string, reels[i].colorpair, reels[i].bold);
		refresh();
		gametimer_set(reels[i].time_in, &gt);
		while (!gametimer_done(&gt))
			if (getch() != ERR)
				goto defer; // i went there
	}
	// outroll
	for (int i=0; i<3; i++) {
		clear_row(reels[i].row);
		refresh();
		gametimer_set(reels[i].time_out, &gt);
		while (!gametimer_done(&gt))
			if (getch() != ERR)
				goto defer; // i went there
	}
defer:
	nodelay(stdscr, false);
	curs_set(1);
}


int main(void)
{
	if (!g_setup()) {
		return 1;
	}

	enum game_state state = GS_START;
	while (state != GS_EXIT) {
		switch (state) {
		case GS_START:
			state = GS_INTRO;
			break;
		case GS_INTRO:
			g_intro();
			state = GS_EXIT;
			break;
		}
	}

	g_teardown();

	return 0;
}

