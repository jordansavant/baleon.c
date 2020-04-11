#include <stdlib.h>
#include <ncurses.h>
#include <menu.h>

#include "gametime.h"
#include "draw.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define SCOLOR_NORMAL	1
#define TCOLOR_NORMAL	2
#define TCOLOR_OMINOUS	3
#define TCOLOR_BLACK	4
#define TCOLOR_SKY	5
#define TCOLOR_DAWN	6

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

	keypad(stdscr,TRUE); // turn on F key listening

	// colors
	init_pair(SCOLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(TCOLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(TCOLOR_OMINOUS, COLOR_RED, COLOR_BLACK);
	init_pair(TCOLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
	init_pair(TCOLOR_SKY, COLOR_CYAN, COLOR_BLACK);
	init_pair(TCOLOR_DAWN, COLOR_YELLOW, COLOR_BLACK);

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
	struct reel reels[] = {
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


void g_title_onquit(char *label)
{
	center(32, "QUIT SELECTED", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	refresh();
}
void g_title()
{
	curs_set(0); // hide cursor
	noecho();

	center( 1, "  ...........      ...      ....      ..........  ........ .....    .....", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 2, "   +:+:    :+:   :+: :+:   :+:        :+:      : :+:    :+: :+:+:   :+:+ ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 3, "    +:+    +:+  +:+   +:+  +:+        +:+        +:+    +:+ :+:+:+  +:+  ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 4, "    +#++:++#+  +#++:++#++: +#+        +#++:+#    +#+    +:+ +#+ +:+ +#+  ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 5, "    +#+    +#+ +#+     +#+ +#+     +# +#+     +# +#+    +#+ +#+  +#+#+#  ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 6, "    #+#    #+# #+#     #+# ########## ##########  +#+  +#+  #+#   #+#+#  ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 7, "    ###+  +###  ##     ##                           ###     ###    ####  ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 8, "   ##########                                               ##       ##  ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center( 9, "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #         #  ", TCOLOR_SKY, TCOLOR_NORMAL, 0);
	center(10, "    ~~ RISING AGAINST THE DARK ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   ", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);

	refresh();
	getch();


	// TITLE MENU
	// create menu items
	char *choices[] = {
		"Intro",
		"New Game",
		"Load Game",
		"Options",
		"Quit",
	};
        int n_choices = ARRAY_SIZE(choices);

	WINDOW *title_menu_win = new_center_win(12, 13, 5, -3); // max choice length + cursor offset by cursor
	ITEM **title_items;
	MENU *title_menu;
	keypad(title_menu_win, TRUE);

	// allocate items
        title_items = (ITEM **)malloc((n_choices + 1) * sizeof(ITEM *));
        for(int i = 0; i < n_choices; i++) {
		title_items[i] = new_item(choices[i], "");
		/* Set the user pointer */
		set_item_userptr(title_items[i], g_title_onquit);
	}
	title_items[n_choices] = (ITEM *)NULL; // last item must be null terminated
	title_menu = new_menu((ITEM **)title_items);
	set_menu_win(title_menu, title_menu_win);
	set_menu_mark(title_menu, " ~ ");
	post_menu(title_menu);

	wrefresh(title_menu_win);

	// loop and listen
	int key;
	while ((key = wgetch(title_menu_win)) != KEY_F(1)) {
		switch (key) {
		case KEY_DOWN:
			menu_driver(title_menu, REQ_DOWN_ITEM);
			break;
		case KEY_UP:
			menu_driver(title_menu, REQ_UP_ITEM);
			break;
		// Enter
		case 10:
			{
				void (*func)(char *); // wtf is this? this is the initalization of the function pointer
				ITEM *cur_item = current_item(title_menu);

				func = item_userptr(cur_item);
				func((char *)item_name(cur_item));
				pos_menu_cursor(title_menu);
			}
			break;
		}
		wrefresh(title_menu_win);
	}
	// free
	unpost_menu(title_menu);
	for(int i = 0; i < n_choices; i++)
		free_item(title_items[i]);
	free_menu(title_menu);
	delwin(title_menu_win);

	echo();
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

