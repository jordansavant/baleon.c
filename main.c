#include <stdlib.h>
#include <ncurses.h>
#include <menu.h>

#include "dmg_gametime.h"
#include "dmg_draw.h"
#include "dmg_world.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define SCOLOR_NORMAL	1
#define TCOLOR_NORMAL	2
#define TCOLOR_OMINOUS	3
#define TCOLOR_BLACK	4
#define TCOLOR_SKY	5
#define TCOLOR_DAWN	6

#define KEY_RETURN	10
#define KEY_Y		121
#define KEY_N		110
#define KEY_ESC		27

// UTILS
void skip_delay(double wait_s)
{
	struct gametimer gt = gametimer_new(wait_s);
	nodelay(stdscr, true);
	while (!gametimer_done(&gt))
		if (getch() != ERR)
			break;
	nodelay(stdscr, false);
}
void escapedelay(bool on)
{
	if (on)
		ESCDELAY = 1000;
	else
		ESCDELAY = 0;
}
void debug(char *msg)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	center(y - 1, msg, TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	refresh();
}


// GAME CODE
enum GAME_STATE
{
	GS_START,
	GS_INTRO,
	GS_TITLE,
	GS_NEWGAME,
	GS_EXIT
};
enum GAME_STATE state = GS_START;

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

	curs_set(0); // hide cursor
	noecho();
	keypad(stdscr, true); // turn on F key listening

	// colors
	init_pair(SCOLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(TCOLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(TCOLOR_OMINOUS, COLOR_RED, COLOR_BLACK);
	init_pair(TCOLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
	init_pair(TCOLOR_SKY, COLOR_CYAN, COLOR_BLACK);
	init_pair(TCOLOR_DAWN, COLOR_YELLOW, COLOR_BLACK);

	// setup world colors
	wld_setup();

	// set primary color
	bkgd(COLOR_PAIR(SCOLOR_NORMAL));


	return true;
}

void g_teardown()
{
	// teardown ncurses
	keypad(stdscr,FALSE);
	curs_set(1);
	echo();
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

	nodelay(stdscr, true); // dont pause interrupt on getch

	for (int i=0; i < ARRAY_SIZE(reels); i++) {
		center(reels[i].row, reels[i].string, reels[i].colorpair, TCOLOR_NORMAL, true);
		refresh();
		// wait a second before loading the menu
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
	clear();
	nodelay(stdscr, false);
	state = GS_TITLE; // to to title screen
}


bool g_title_done = false;
int g_title_prompt_row = 18;
void g_title_onquit(char *label)
{
	clear_row(g_title_prompt_row);
	center(g_title_prompt_row, "Are you sure? [y/n]", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	refresh();

	bool listen = true;
	while (listen) {
		switch (getch()) {
		case KEY_Y:
			state = GS_EXIT;
			g_title_done = true;
			listen = false;
			break;
		case ERR:
			break;
		default:
		case KEY_N:
			clear_row(g_title_prompt_row);
			refresh();
			listen = false;
			break;
		}
	}
}
void g_title_onintro(char *label)
{
	state = GS_INTRO;
	g_title_done = true;
}
void g_title_onnewgame(char *label)
{
	state = GS_NEWGAME;
	g_title_done = true;
}
void g_title_onundefined(char *label)
{
	clear_row(g_title_prompt_row);
	center(g_title_prompt_row, "Unimplemented", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	refresh();
}
void g_title()
{
	g_title_done = false;

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

	// TITLE MENU
	// create menu items
	struct choice {
		char *label;
		void (*func)(char *);
	};
	struct choice choices[] = {
		{"Intro", g_title_onintro},
		{"New Game", g_title_onnewgame},
		{"Load Game", g_title_onundefined},
		{"Options", g_title_onundefined},
		{"Quit", g_title_onquit},
	};
        int n_choices = ARRAY_SIZE(choices);

	WINDOW *title_menu_win = new_center_win(12, 13, 5, 0); // max choice length + cursor offset by cursor
	ITEM **title_items;
	MENU *title_menu;
	keypad(title_menu_win, TRUE);

	// allocate items
        title_items = (ITEM **)malloc((n_choices + 1) * sizeof(ITEM *));
        for(int i = 0; i < n_choices; i++) {
		title_items[i] = new_item(choices[i].label, "");
		set_item_userptr(title_items[i], choices[i].func);
	}
	title_items[n_choices] = (ITEM *)NULL; // last item must be null terminated
	title_menu = new_menu((ITEM **)title_items);
	set_menu_win(title_menu, title_menu_win);
	set_menu_mark(title_menu, "~ ");
	post_menu(title_menu);

	wrefresh(title_menu_win);

	// loop and listen
	while (!g_title_done) {
		switch (wgetch(title_menu_win)) {
		case KEY_DOWN:
			menu_driver(title_menu, REQ_NEXT_ITEM);
			break;
		case KEY_UP:
			menu_driver(title_menu, REQ_PREV_ITEM);
			break;
		// Enter
		case KEY_RETURN:
		case KEY_ENTER:
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

	clear();
}


// GAME LOOP
enum PLAY_STATE
{
	PS_START,
	PS_PLAY,
	PS_MENU,
	PS_EXIT,
};
enum PLAY_STATE play_state = PS_START;

int map[] = {
	1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1,
	1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1,
	1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
int map_rows = 12;
int map_cols = 12;
int map_offset_x = 0;
int map_offset_y = 0;
void ps_build_world()
{
	// World is large indexed map to start
	//
}
void ps_play_draw()
{
	clear();
	for (int r=0; r < map_rows; r++) {
		for (int c=0; c < map_cols; c++) {
			int index = r * map_cols + c;
			int tiletype = map[index];
			int mobtype = 0;
			if (r == 2 && c == 2)
				mobtype = MOB_BUGBEAR;

			struct wld_tiletype *tt = wld_get_tiletype(tiletype);
			struct wld_mobtype *mt = wld_get_mobtype(mobtype);
			int colorpair = wld_cpair(tiletype, mobtype);
			char tch = tt->sprite;
			char mch = mt->sprite;
			char cha = mch;
			if (mch == ' ')
				cha = tch; // render tile sprite if no mob sprite

			// render
			move(r + map_offset_y, c*2 + map_offset_x);
			attrset(COLOR_PAIR(colorpair));
			addch(cha);

			// extra padding
			int bg_only = wld_cpair_bg(tiletype);
			move(r + map_offset_y, c*2+1 + map_offset_x);
			attrset(COLOR_PAIR(bg_only));
			addch(' ');
		}
	}
	refresh();
}
void ps_play_input()
{
	nodelay(stdscr, true);
	escapedelay(false);
	bool listen = true;
	while (listen) {
		switch (getch()) {
		case KEY_ESC:
			play_state = PS_MENU;
			listen = false;
			break;
		}
	}
	escapedelay(true);
	nodelay(stdscr, false);
}
void ps_play_update()
{
	// depending on input change and trigger various updates
}

void ps_menu_draw()
{
	center(20, "GAME MENU", TCOLOR_NORMAL, TCOLOR_NORMAL, false);
	refresh();
}
void ps_menu_input()
{
	nodelay(stdscr, true);
	escapedelay(false);
	bool listen = true;
	while (listen) {
		switch (getch()) {
		case KEY_ESC:
			play_state = PS_PLAY;
			listen = false;
			break;
		}
	}
	escapedelay(true);
	nodelay(stdscr, false);
}

void g_newgame()
{
	while (play_state != PS_EXIT) {
		switch (play_state) {
		case PS_START:
			ps_build_world();
			play_state = PS_PLAY;
			break;
		case PS_PLAY:
			// draw world
			ps_play_draw();

			// capture play input
			ps_play_input();

			// update world
			ps_play_update();

			break;
		case PS_MENU:
			ps_menu_draw();
			ps_menu_input();
			break;
		}
	}
}

int main(void)
{
	if (!g_setup()) {
		return 1;
	}

	while (state != GS_EXIT) {
		switch (state) {
		case GS_START:
			state = GS_INTRO;
			break;
		case GS_INTRO:
			g_intro();
			break;
		case GS_TITLE:
			g_title();
			break;
		case GS_NEWGAME:
			g_newgame();
			break;
		}
	}

	g_teardown();

	return 0;
}

