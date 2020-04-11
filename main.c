#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>

#include "gametime.h"
#include "draw.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define SCOLOR_NORMAL 1
#define TCOLOR_NORMAL 2
#define TCOLOR_OMINOUS 3
#define TCOLOR_BLACK 4

// GAME CODE
enum game_state
{
	GS_START,
	GS_INTRO,
	GS_TITLE,
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
		double time_in;
		double time_out;
	};
	struct reel reels[3] = {
		{3, "It was an age of light.", TCOLOR_NORMAL, 2, .25},
		{4, "It was an age of hope.", TCOLOR_NORMAL, 2.5, .25},
		{6, "And then he came...", TCOLOR_OMINOUS, 2, .25}
	};

	// roll in title but allow for escaping
	struct gametimer gt = gametimer_new(0);

	curs_set(0); // hide cursor
	nodelay(stdscr, true); // dont pause interrupt on getch

	for (int i=0; i < ARRAY_SIZE(reels); i++) {
		center(reels[i].row, reels[i].string, reels[i].colorpair, TCOLOR_NORMAL, true);
		refresh();
		gametimer_set(reels[i].time_in, &gt);
		while (!gametimer_done(&gt))
			if (getch() != ERR)
				goto defer; // i went there
	}
	// outroll
	for (int j=0; j < 2; j++) {
		for (int i=0; i < ARRAY_SIZE(reels); i++) {
			// first pass darken the text
			// second pass clear it
			if (j == 0)
				center(reels[i].row, reels[i].string, reels[i].colorpair, TCOLOR_NORMAL, false);
			else
				clear_row(reels[i].row);
			refresh();
			gametimer_set(reels[i].time_out, &gt);
			while (!gametimer_done(&gt))
				if (getch() != ERR)
					goto defer; // i went there
		}
	}
defer:
	nodelay(stdscr, false);
	curs_set(1);
}

void g_title()
{
	curs_set(0); // hide cursor
	center( 1, " ______   _______  _        _______  _______  _       ",    TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	center( 2, "(  ___ \\ (  ___  )( \\      (  ____ \\(  ___  )( (    /|", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	center( 3, "| (   ) )| (   ) || (      | (    \\/| (   ) ||  \\  ( |",  TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	center( 4, "| (__/ / | (___) || |      | (__    | |   | ||   \\ | |",   TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	center( 5, "|  __ (  |  ___  || |      |  __)   | |   | || (\\ \\) |",  TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	center( 6, "| (  \\ \\ | (   ) || |      | (      | |   | || | \\   |", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	center( 7, "| )___) )| )   ( || (____/\\| (____/\\| (___) || )  \\  |", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	center( 8, "|/ \\___/ |/     \\|(_______/(_______/(_______)|/    )_)",  TCOLOR_NORMAL, TCOLOR_NORMAL, 0);

	refresh();
	getch();
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
			state = GS_TITLE;
			break;
		case GS_TITLE:
			g_title();
			state = GS_EXIT;
			break;
		}
	}

	g_teardown();

	return 0;
}

