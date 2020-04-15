#include <stdlib.h>
#include <ncurses.h>
#include <menu.h>

#include "dm_gametime.h"
#include "dm_draw.h"
#include "dm_world.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef bool
#define false 0
#define true 1
typedef int bool; // or #define bool int
#endif

#define SCOLOR_NORMAL	1
#define TCOLOR_NORMAL	2
#define TCOLOR_OMINOUS	3
#define TCOLOR_BLACK	4
#define TCOLOR_SKY	5
#define TCOLOR_DAWN	6
#define SCOLOR_ALLWHITE 7
#define SCOLOR_CURSOR   8
#define TCOLOR_PURPLE   9

#define KEY_RETURN	10
#define KEY_ESC		27

#define KEY_a		97
#define KEY_b		98
#define KEY_c		99
#define KEY_d		100
#define KEY_e		101
#define KEY_f		102
#define KEY_g		103
#define KEY_h		104
#define KEY_i		105
#define KEY_j		106
#define KEY_k		107
#define KEY_l		108
#define KEY_m		109
#define KEY_n		110
#define KEY_o		111
#define KEY_p		112
#define KEY_q		113
#define KEY_r		114
#define KEY_s		115
#define KEY_t		116
#define KEY_u		117
#define KEY_v		118
#define KEY_w		119
#define KEY_x		120
#define KEY_y		121
#define KEY_z		122

#define KEY_0		48
#define KEY_1		49
#define KEY_2		50
#define KEY_3		51
#define KEY_4		52
#define KEY_5		53
#define KEY_6		54
#define KEY_7		55
#define KEY_8		56
#define KEY_9		57

// UTILS
void skip_delay(double wait_s)
{
	struct dm_gametimer gt = dm_gametimer_new(wait_s);
	nodelay(stdscr, true);
	while (!dm_gametimer_done(&gt))
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
	int y = getmaxy(stdscr);
	dm_center(y - 1, msg, TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
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
enum GAME_STATE game_state = GS_START;

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

	// colors ID			FG		BG
	init_pair(SCOLOR_NORMAL,	COLOR_WHITE,	COLOR_BLACK);
	init_pair(TCOLOR_NORMAL,	COLOR_WHITE,	COLOR_BLACK);
	init_pair(TCOLOR_OMINOUS,	COLOR_RED,	COLOR_BLACK);
	init_pair(TCOLOR_BLACK,		COLOR_BLACK,	COLOR_BLACK);
	init_pair(TCOLOR_SKY,		COLOR_CYAN,	COLOR_BLACK);
	init_pair(TCOLOR_DAWN,		COLOR_YELLOW,	COLOR_BLACK);
	init_pair(SCOLOR_ALLWHITE,	COLOR_WHITE,	COLOR_WHITE);
	init_pair(SCOLOR_CURSOR,	COLOR_BLACK,	COLOR_CYAN);
	init_pair(TCOLOR_PURPLE,	COLOR_MAGENTA,	COLOR_BLACK);

	// setup world colors
	wld_setup();

	// set primary color
	bkgd(COLOR_PAIR(SCOLOR_NORMAL));


	return true;
}

void g_teardown()
{
	// teardown world vars
	wld_teardown();

	// teardown ncurses
	keypad(stdscr,FALSE);
	curs_set(1);
	echo();
	endwin();
}

void g_intro()
{
	clear();

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
	struct dm_gametimer gt = dm_gametimer_new(0);

	nodelay(stdscr, true); // dont pause interrupt on getch

	for (int i=0; i < ARRAY_SIZE(reels); i++) {
		dm_center(reels[i].row, reels[i].string, reels[i].colorpair, TCOLOR_NORMAL, true);
		refresh();
		// wait a second before loading the menu
		dm_gametimer_set(reels[i].time_in, &gt);
		while (!dm_gametimer_done(&gt))
			if (getch() != ERR)
				goto defer; // i went there
	}
	// outroll
	for (int j=0; j < 2; j++) {
		for (int i=0; i < ARRAY_SIZE(reels); i++) {
			// first pass darken the text
			// second pass clear it
			if (j == 0)
				dm_center(reels[i].row, reels[i].string, reels[i].colorpair, TCOLOR_NORMAL, false);
			else
				dm_clear_row(reels[i].row);
			refresh();
			dm_gametimer_set(reels[i].time_out, &gt);
			while (!dm_gametimer_done(&gt))
				if (getch() != ERR)
					goto defer; // i went there
		}
	}

defer:
	clear();
	nodelay(stdscr, false);
	game_state = GS_TITLE; // to to title screen
}


bool g_title_done = false;
int g_title_prompt_row = 18;
void g_title_onquit(char *label)
{
	dm_clear_row(g_title_prompt_row);
	dm_center(g_title_prompt_row, "Are you sure? [y/n]", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	refresh();

	bool listen = true;
	while (listen) {
		switch (getch()) {
		case KEY_y:
			game_state = GS_EXIT;
			g_title_done = true;
			listen = false;
			break;
		case ERR:
			break;
		default:
		case KEY_n:
			dm_clear_row(g_title_prompt_row);
			refresh();
			listen = false;
			break;
		}
	}
}
void g_title_onintro(char *label)
{
	game_state = GS_INTRO;
	g_title_done = true;
}
void g_title_onnewgame(char *label)
{
	game_state = GS_NEWGAME;
	g_title_done = true;
}
void g_title_onundefined(char *label)
{
	dm_clear_row(g_title_prompt_row);
	dm_center(g_title_prompt_row, "Unimplemented", TCOLOR_NORMAL, TCOLOR_NORMAL, 0);
	refresh();
}
void g_title()
{
	clear();

	g_title_done = false;

	dm_center( 1, "  ...........      ...      ....      ..........  ........ .....    .....", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 2, "   +:+:    :+:   :+: :+:   :+:        :+:      : :+:    :+: :+:+:   :+:+ ", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 3, "    +:+    +:+  +:+   +:+  +:+        +:+        +:+    +:+ :+:+:+  +:+  ", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 4, "    +#++:++#+  +#++:++#++: +#+        +#++:+#    +#+    +:+ +#+ +:+ +#+  ", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 5, "    +#+    +#+ +#+     +#+ +#+     +# +#+     +# +#+    +#+ +#+  +#+#+#  ", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 6, "    #+#    #+# #+#     #+# ########## ##########  +#+  +#+  #+#   #+#+#  ", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 7, "    ###+  +###  ##     ##                           ###     ###    ####  ", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 8, "   ##########                                               ##       ##  ", TCOLOR_OMINOUS, TCOLOR_NORMAL, 0);
	dm_center( 9, "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #         #  ", TCOLOR_OMINOUS, TCOLOR_NORMAL, -1);
	dm_center(10, "    ~~ Aberrati@ns in The Dark ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   ", TCOLOR_DAWN, TCOLOR_NORMAL, 0);

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

	WINDOW *title_menu_win = dm_new_center_win(12, 13, 5, 0); // max choice length + cursor offset by cursor
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
	set_menu_mark(title_menu, "@ ");
	post_menu(title_menu);

	wrefresh(title_menu_win);

	// loop and listen
	while (!g_title_done) {
		switch (wgetch(title_menu_win)) {
		case KEY_DOWN:
		case KEY_s:
			menu_driver(title_menu, REQ_NEXT_ITEM);
			break;
		case KEY_UP:
		case KEY_w:
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
	PS_MAPCHANGE,
	PS_END, // do not call exit directly
	PS_EXIT,
};
enum PLAY_STATE play_state = PS_START;


// MAP SETTINGS / VARS
int map_rows_scale = 1;
int map_cols_scale = 2;
WINDOW *map_pad;
struct wld_map *current_map;

WINDOW* cursorpanel;
WINDOW* mobpanel;

void ui_box(WINDOW* win)
{
	box(win, ACS_VLINE, '=');
}
// helper to write to a row in a panel we boxed
void ui_write(WINDOW *win, int row, char *msg)
{
	dm_clear_row_in_win(win, row + 1);
	wmove(win, row + 1, 2);
	waddstr(win, msg);
}

//void ui_anchor_ur(WINDOW* win, float height, float width)
void ui_anchor_ur(WINDOW* win, float height, float width, int minrows, int mincols)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	int rows = y * height;
	int cols = x * width;
	if (minrows > 0 && rows < minrows)
		rows = minrows;
	if (mincols > 0 && cols < mincols)
		cols = mincols;
	wresize(win, rows, cols);
	mvwin(win, 0, x - cols);
}

void ui_cursorinfo(char *msg)
{
	ui_write(cursorpanel, 0, msg);
	ui_box(cursorpanel);
	wrefresh(cursorpanel);
}

void ui_update_cursorinfo(struct wld_map *map)
{
	int x = map->cursor->x;
	int y = map->cursor->y;

	// Get details about the tile they are on
	struct wld_tile *t = wld_gettileat(map, x, y);
	struct wld_tiletype *tt = &wld_tiletypes[t->type];

	struct wld_mob *m = wld_getmobat(map, x, y);
	if (m != NULL) {
		// TODO get "what" the mob is doing
		struct wld_mobtype *mt = &wld_mobtypes[m->type];
		ui_cursorinfo(mt->short_desc);
	} else {
		ui_cursorinfo(tt->short_desc);
	}
}
void ui_update_mobpanel(struct wld_map *map)
{
	// probably just need to do a shadowcast event each turn to get mobs in vision
	ui_write(mobpanel, 0, "MOBS:");
	ui_box(mobpanel);
	wrefresh(mobpanel);
}

void map_on_cursormove(struct wld_map *map, int x, int y, int index)
{
	ui_update_cursorinfo(map);
}

void ps_build_world()
{
	clear();

	// World is large indexed map to start
	current_map = wld_newmap(1);
	map_pad = newpad(current_map->rows * map_rows_scale, current_map->cols * map_cols_scale);
	current_map->on_cursormove = map_on_cursormove;
}
void ps_destroy_world()
{
	// THIS HAS NOT BEEN TESTED I CANT FIND DOCS ON HOW TO CLEAN UP PAD MEMORY
	delwin(map_pad);
	wld_delmap(current_map);

	clear();
}

void ps_build_ui()
{
	cursorpanel = newwin(3, 50, 0, 0);
	ui_box(cursorpanel);

	// anchor to right side
	mobpanel = newwin(0, 0, 0, 0);
	ui_anchor_ur(mobpanel, 1, .2, 0, 30);
	ui_box(mobpanel);
}
void ps_destroy_ui()
{
	delwin(mobpanel);
	delwin(cursorpanel);
}
// TODO function on translating a screen item to the pad item
//void translate_screenyx_mapyx(int sy, int, sx)
//{
//	// get difference between pad top,left and screen top,left
//	// subtract that from screen y,x to get pad y,x
//}
void ps_play_draw()
{
	// do not clear, it causes awful redraw
	//clear();
	//wclear(map_pad);

	// Draw map
	for (int r=0; r < current_map->rows; r++) {
		for (int c=0; c < current_map->cols; c++) {
			int index = r * current_map->cols + c;

			// get tile from tile map
			int tile_id = current_map->tile_map[index];
			int tiletype = current_map->tiles[tile_id].type;

			// get mob from mob map index
			int mob_id = current_map->mob_map[index];
			int mobtype = 0;
			if (mob_id > -1) {
				mobtype = current_map->mobs[mob_id].type;
			}

			// get type structs for rendering
			struct wld_tiletype *tt = wld_get_tiletype(tiletype);
			struct wld_mobtype *mt = wld_get_mobtype(mobtype);
			int colorpair = wld_cpair(tiletype, mobtype);

			// default symbol to mob, if no mob symbol then tile symbol if there is one
			unsigned long cha = mt->sprite;
			if (mt->sprite == ' ')
				cha = tt->sprite;

			// render to pad
			wmove(map_pad, r * map_rows_scale, c * map_cols_scale); // pads by scaling out
			wattrset(map_pad, COLOR_PAIR(colorpair));
			waddch(map_pad, cha); // pad

			// extra padding if we are scaling the columns to make it appear at a better ratio in the terminal
			if (map_cols_scale > 1 || map_rows_scale > 1) {
				int bg_only = wld_cpair_bg(tiletype);
				for (int i=0; i < map_cols_scale; i++) {
					wmove(map_pad, r * map_rows_scale, c * map_cols_scale + i+1);
					wattrset(map_pad, COLOR_PAIR(bg_only));
					waddch(map_pad, ' '); // pad

					for (int j=0; j < map_rows_scale; j++) {
						wmove(map_pad, r * map_rows_scale + j+1, c * map_cols_scale + i);
						wattrset(map_pad, COLOR_PAIR(bg_only));
						waddch(map_pad, ' '); // pad
					}
				}
			}
		}
	}

	// Draw cursor
	wmove(map_pad, current_map->cursor->y * map_rows_scale, current_map->cursor->x * map_cols_scale);
	wattrset(map_pad, COLOR_PAIR(SCOLOR_CURSOR));
	char ch = mvwinch(map_pad, current_map->cursor->y * map_rows_scale, current_map->cursor->x * map_cols_scale) & A_CHARTEXT;
	waddch(map_pad, ch);

	// lets calculate where to offset the pad
	int top, left;
	DM_CALC_CENTER_TOPLEFT(stdscr, current_map->rows * map_rows_scale, current_map->cols * map_cols_scale, top, left);

	// render pad
	refresh(); // has to be called before prefresh for some reason?
	// prefresh(pad,pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol)
	prefresh(map_pad, 0, 0, top, left, top + current_map->rows * map_rows_scale, left + current_map->cols * map_cols_scale);

	// UI constants (needs to be done in an event?)
	ui_update_mobpanel(current_map);

	wrefresh(cursorpanel);
	wrefresh(mobpanel);
}
bool trigger_world = false;
void ps_play_input()
{
	nodelay(stdscr, true);
	escapedelay(false);
	bool listen = true;
	trigger_world = false;
	while (listen) {
		listen = false;
		switch (getch()) {
		// Cursor movement
		case KEY_8: // up
			wld_movecursor(current_map, 0, -1);
			break;
		case KEY_2: // down
			wld_movecursor(current_map, 0, 1);
			break;
		case KEY_4: // left
			wld_movecursor(current_map, -1, 0);
			break;
		case KEY_6: // right
			wld_movecursor(current_map, 1, 0);
			break;
		case KEY_7: // upleft
			wld_movecursor(current_map, -1, -1);
			break;
		case KEY_9: // upright
			wld_movecursor(current_map, 1, -1);
			break;
		case KEY_1: // downleft
			wld_movecursor(current_map, -1, 1);
			break;
		case KEY_3: // downright
			wld_movecursor(current_map, 1, 1);
			break;
		// Player movement
		case KEY_ESC:
			play_state = PS_MENU;
			break;
		case KEY_w:
			wld_queuemobmove(current_map->player, 0, -1);
			trigger_world = true;
			break;
		case KEY_s:
			wld_queuemobmove(current_map->player, 0, 1);
			trigger_world = true;
			break;
		case KEY_a:
			wld_queuemobmove(current_map->player, -1, 0);
			trigger_world = true;
			break;
		case KEY_d:
			wld_queuemobmove(current_map->player, 1, 0);
			trigger_world = true;
			break;
		case KEY_q:
			wld_queuemobmove(current_map->player, -1, -1);
			trigger_world = true;
			break;
		case KEY_e:
			wld_queuemobmove(current_map->player, 1, -1);
			trigger_world = true;
			break;
		case KEY_z:
			wld_queuemobmove(current_map->player, -1, 1);
			trigger_world = true;
			break;
		case KEY_x:
			wld_queuemobmove(current_map->player, 1, 1);
			trigger_world = true;
			break;
		default:
			listen = true;
			break;
		}
	}
	escapedelay(true);
	nodelay(stdscr, false);
}
void ps_play_update()
{
	// depending on input change and trigger various updates

	if (trigger_world) {
		// loop over map mobs and run their update routines
		for (int i=0; i < current_map->mobs_length; i++) {
			wld_mob_update(&current_map->mobs[i]);
		}
	}
}

void ps_menu_draw()
{
	dm_center(20, "GAME MENU", TCOLOR_NORMAL, TCOLOR_NORMAL, false);
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
		case KEY_q:
			play_state = PS_END;
			listen = false;
			break;
		}
	}
	escapedelay(true);
	nodelay(stdscr, false);
}

void g_newgame()
{
	play_state = PS_START;
	while (play_state != PS_EXIT) {
		switch (play_state) {
		case PS_START:
			ps_build_world();
			ps_build_ui();
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
		case PS_MAPCHANGE:
			break;
		case PS_END:
			ps_destroy_ui();
			ps_destroy_world();
			play_state = PS_EXIT;
			game_state = GS_TITLE;
			break;
		}
	}
}

int main(void)
{
	if (!g_setup()) {
		return 1;
	}

	game_state = GS_START;
	while (game_state != GS_EXIT) {
		switch (game_state) {
		case GS_START:
			game_state = GS_INTRO;
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

