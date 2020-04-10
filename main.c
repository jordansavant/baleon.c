#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>

#define SCOLOR_NORMAL 1
#define TCOLOR_NORMAL 2
#define TCOLOR_OMINOUS 3

void msleep(int milliseconds)
{
	usleep(milliseconds * 1000);
}

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

	// set primary color
	bkgd(COLOR_PAIR(SCOLOR_NORMAL));
	return true;
}

void g_teardown()
{
	// teardown ncurses
	endwin();
}

void center(int row, char *title, int colorpair, bool bold)
{
	if (bold)
		attrset(COLOR_PAIR(colorpair) | A_BOLD);
	else
		attrset(COLOR_PAIR(colorpair));
	int len, indent, y, width;
	getmaxyx(stdscr, y, width);	/* get screen width */
	len = strlen(title);		/* get title's length */
	indent = width - len;		/* subtract it from screen width */
	indent /= 2;			/* divide result into two */
	mvaddstr(row, indent, title);
	attrset(COLOR_PAIR(SCOLOR_NORMAL));
}

enum intro_state
{
	INTRO_1,
	INTRO_2,
	INTRO_3,
	INTRO_4,
	INTRO_EXIT
};
void g_intro()
{
	curs_set(0); // hide cursor
	enum intro_state state = INTRO_1;
	struct reel {
		int row;
		char *string;
		int colorpair;
		bool bold;
	};
	struct reel reels[2] = {
		{3, "It was an age of light.", TCOLOR_NORMAL, 1},
		{3, "It was an age of light.", TCOLOR_NORMAL, 1}
	};

	center(3, "It was an age of light.", TCOLOR_NORMAL, 1);
	refresh();
	sleep(2);

	clear();
	center(3, "It was an age of light.", TCOLOR_NORMAL, 1);
	center(4, "It was an age of hope.", TCOLOR_NORMAL, 1);
	refresh();
	sleep(2);

	clear();
	center(3, "It was an age of light.", TCOLOR_NORMAL, 1);
	center(4, "It was an age of hope.", TCOLOR_NORMAL, 1);
	center(6, "And then he came...", TCOLOR_OMINOUS, 1);
	refresh();
	sleep(2);

	clear();
	center(3, "It was an age of light.", TCOLOR_NORMAL, 0);
	center(4, "It was an age of hope.", TCOLOR_NORMAL, 1);
	center(6, "And then he came...", TCOLOR_OMINOUS, 1);
	refresh();
	msleep(500);

	clear();
	center(4, "It was an age of hope.", TCOLOR_NORMAL, 0);
	center(6, "And then he came...", TCOLOR_OMINOUS, 1);
	refresh();
	msleep(500);

	clear();
	center(6, "And then he came...", TCOLOR_OMINOUS, 1);
	refresh();
	msleep(500);

	clear();
	center(6, "And then he came...", TCOLOR_OMINOUS, 0);
	refresh();
	msleep(500);

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

