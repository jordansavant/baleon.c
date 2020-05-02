#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <menu.h>
#include <signal.h>
#include <time.h>

#include "dm_defines.h"
#include "dm_debug.h"
#include "dm_algorithm.h"
#include "dm_gametime.h"
#include "dm_draw.h"
#include "dm_world.h"
#include "game.h"

#define SCOLOR_NORMAL	1
#define TCOLOR_NORMAL	2
#define TCOLOR_RED	3
#define TCOLOR_BLACK	4
#define TCOLOR_CYAN	5
#define TCOLOR_YELLOW	6
#define SCOLOR_ALLWHITE 7
#define SCOLOR_CURSOR   8
#define TCOLOR_PURPLE   9
#define SCOLOR_BLOOD    10
#define SCOLOR_ALLBLACK 11
#define SCOLOR_TARGET   12
#define ECOLOR_HEAL_A	13
#define ECOLOR_HEAL_B	14
#define ECOLOR_DMG_A	15
#define ECOLOR_DMG_B	16
#define ECOLOR_SUMMON_A	17
#define ECOLOR_SUMMON_B	18



///////////////////////////
// UTILITIES START

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
// term resized os event
bool term_resized = false;
void on_term_resize(int dummy)
{
	term_resized = true;
}

// UTILITIES END
///////////////////////////




///////////////////////////
// GAME SETUP START

// game variables
enum GAME_STATE game_state = GS_START;
bool trigger_world = false;

// Play Variables
enum PLAY_STATE play_state = PS_START;

// MAP SETTINGS / VARS
int map_rows_scale = 1;
int map_cols_scale = 2;
WINDOW *map_pad = NULL;
struct wld_world *world;
struct wld_map *current_map = NULL;
struct wld_map *next_map = NULL;
int ui_map_cols;
int ui_map_rows;
int ui_map_padding = 15;
int ui_map_border = 1;

// PLAY UI PANELS
WINDOW* cmdpanel;
WINDOW* cursorpanel;
WINDOW* logpanel;
WINDOW* mobpanel;
WINDOW* inventorypanel;
WINDOW* usepanel;
WINDOW* aberratepanel;
enum USE_TYPE use_type = USE_NONE;
struct wld_item *use_item = NULL;
int use_item_slot = -1;

#define CURSOR_INFO_LENGTH 45
#define LOG_COUNT 7
#define LOG_LENGTH 60
#define INV_LENGTH 48
#define INV_ITEM_LENGTH 46
#define USE_LENGTH 58
#define ABE_LENGTH 58
#define VIS_LENGTH 30
#define CMD_LENGTH 100
char logs[LOG_COUNT][LOG_LENGTH];

// visible mob list
#define VISIBLE_MOB_MAX 100
struct wld_mob* visible_mobs[VISIBLE_MOB_MAX];
int visible_mobs_length = 0;
int visible_mob_focus = 0;

bool g_setup()
{
	// setup debug file pointer
	dmlogopen("log/log.txt", "w");

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
	init_pair(TCOLOR_NORMAL,	COLOR_WHITE,	COLOR_BLACK);
	init_pair(TCOLOR_RED,		COLOR_RED,	COLOR_BLACK);
	init_pair(TCOLOR_BLACK,		COLOR_BLACK,	COLOR_BLACK);
	init_pair(TCOLOR_CYAN,		COLOR_CYAN,	COLOR_BLACK);
	init_pair(TCOLOR_YELLOW,	COLOR_YELLOW,	COLOR_BLACK);
	init_pair(TCOLOR_PURPLE,	COLOR_MAGENTA,	COLOR_BLACK);

	init_pair(SCOLOR_NORMAL,	COLOR_WHITE,	COLOR_BLACK);
	init_pair(SCOLOR_ALLWHITE,	COLOR_WHITE,	COLOR_WHITE);
	init_pair(SCOLOR_CURSOR,	COLOR_BLACK,	COLOR_MAGENTA);
	init_pair(SCOLOR_TARGET,	COLOR_BLACK,	COLOR_YELLOW);
	init_pair(SCOLOR_BLOOD,		COLOR_BLACK,	COLOR_RED);
	init_pair(SCOLOR_ALLBLACK,	COLOR_BLACK,	COLOR_BLACK);
	init_pair(ECOLOR_HEAL_A,	COLOR_WHITE,	COLOR_GREEN);
	init_pair(ECOLOR_HEAL_B,	COLOR_GREEN,	COLOR_WHITE);
	init_pair(ECOLOR_DMG_A,		COLOR_WHITE,	COLOR_RED);
	init_pair(ECOLOR_DMG_B,		COLOR_RED,	COLOR_WHITE);
	init_pair(ECOLOR_SUMMON_A,	COLOR_WHITE,	COLOR_CYAN);
	init_pair(ECOLOR_SUMMON_B,	COLOR_BLACK,	COLOR_CYAN);

	// setup world colors
	wld_setup();

	// set primary color
	bkgd(COLOR_PAIR(SCOLOR_NORMAL));

	// listen to os signal
	signal(SIGWINCH, on_term_resize);

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

	// unload dm log
	dmlog("game end...");
	dmlogclose();
}

void g_newgame()
{
	play_state = PS_START;
	while (play_state != PS_EXIT) {
		switch (play_state) {
		case PS_START:
			ps_build_ui();
			ps_build_world();
			play_state = PS_PLAY;
			break;
		case PS_PLAY:
			// draw world
			ps_play_draw();

			// update world
			ps_play_update();

			// not sure if i am going to keep this
			// fix ui that may have changed
			if (term_resized) {
				ps_reset_ui();
				term_resized = false;
			}

			break;
		case PS_MENU:
			ps_menu_draw();
			ps_menu_input();
			break;
		case PS_MAPCHANGE:
			current_map = next_map;
			next_map = NULL;
			ps_on_mapchange();
			play_state = PS_PLAY;

			break;
		case PS_GAMEOVER:
			ui_loginfo("After many weary battles you succumb to the darkness.");
			ui_loginfo("Death overtakes you.");
			ps_play_draw();

			getch();

			play_state = PS_END;
			break;
		case PS_WIN:
			ui_loginfo("Congratulations!");
			ui_loginfo("After many weary battles you have overcome Baleon.");
			ps_play_draw();

			getch();

			play_state = PS_END;
			break;
		case PS_END:
			dmlog("GAME END");
			ps_destroy_ui();
			ps_destroy_world();
			play_state = PS_EXIT;
			game_state = GS_TITLE;
			break;
		}
	}
}


int game_main()
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

// GAME SETUP END
///////////////////////////




///////////////////////////
// INTRO START

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
		{6, "And then he came...", TCOLOR_RED, 2, .25}
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

// INTRO END
///////////////////////////



///////////////////////////
// TITLE START

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

	dm_center( 1, "  ...........      ...      ....      ..........  ........ .....    .....", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 2, "   +:+:    :+:   :+: :+:   :+:        :+:      : :+:    :+: :+:+:   :+:+ ", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 3, "    +:+    +:+  +:+   +:+  +:+        +:+        +:+    +:+ :+:+:+  +:+  ", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 4, "    +#++:++#+  +#++:++#++: +#+        +#++:+#    +#+    +:+ +#+ +:+ +#+  ", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 5, "    +#+    +#+ +#+     +#+ +#+     +# +#+     +# +#+    +#+ +#+  +#+#+#  ", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 6, "    #+#    #+# #+#     #+# ########## ##########  +#+  +#+  #+#   #+#+#  ", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 7, "    ###+  +###  ##     ##                           ###     ###    ####  ", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 8, "   ##########                                               ##       ##  ", TCOLOR_RED, TCOLOR_NORMAL, 0);
	dm_center( 9, "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #         #  ", TCOLOR_RED, TCOLOR_NORMAL, -1);
	dm_center(10, "    ~~ Aberrati@ns in The Dark ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   ", TCOLOR_YELLOW, TCOLOR_NORMAL, 0);

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

// TITLE END
///////////////////////////




///////////////////////////
// UI START

void ui_clear_visible_mobs(bool full)
{
	int total = VISIBLE_MOB_MAX;
	if (!full)
		total = visible_mobs_length;
	for (int i=0; i < VISIBLE_MOB_MAX; i++) {
		visible_mobs[i] = NULL;
	}
	visible_mobs_length = 0;
}

void ui_next_visible_mob()
{
	if (visible_mobs_length > 0)
		visible_mob_focus = (visible_mob_focus + 1) % visible_mobs_length;
	else
		visible_mob_focus = 0;
	for (int i=0; i < visible_mobs_length; i++) {
		struct wld_mob *mob = visible_mobs[i];
		if (mob && i == visible_mob_focus) {
			wld_map_set_cursor_pos(mob->map, mob->map_x, mob->map_y);
		}
	}
}


void ui_box_color(WINDOW* win, int colorpair)
{
	//box(win, ACS_VLINE, ACS_HLINE);
	//box(win, ':', '-');
	//wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
	wattrset(win, COLOR_PAIR(colorpair));
	wborder(win, '|', '|', '=', '=', '@', '@', '@', '@');
	wattrset(win, COLOR_PAIR(TCOLOR_NORMAL));
}

void ui_box(WINDOW* win)
{
	ui_box_color(win, TCOLOR_PURPLE);
}

void ui_clear(WINDOW *win, int row)
{
	wmove(win, row + 1, 0);
	wclrtoeol(win);
}

// helper to write to a row in a panel we boxed
void ui_write_rc_len(WINDOW *win, int row, int col, char *msg, int length)
{
	char buffer[length];
	memcpy(buffer, msg, length);

	dm_clear_row_in_win(win, row + 1);
	wmove(win, row + 1, col + 2);
	waddstr(win, buffer);
}
void ui_write_char(WINDOW *win, int row, int col, unsigned long ch)
{
	wmove(win, row + 1, col + 2);
	waddch(win, ch);
}
void ui_write_rc(WINDOW *win, int row, int col, char *msg)
{
	dm_clear_row_in_win(win, row + 1);
	wmove(win, row + 1, col + 2);
	waddstr(win, msg);
}
void ui_write(WINDOW *win, int row, char *msg)
{
	ui_write_rc(win, row, 0, msg);
}
void ui_write_len(WINDOW *win, int row, char *msg, int length)
{
	ui_write_rc_len(win, row, 0, msg, length);
}

//void ui_anchor_ur(WINDOW* win, float height, float width)
void ui_anchor_ur(WINDOW* win, int rows, int cols)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	wresize(win, rows, cols);
	mvwin(win, 0, x - cols);
}
void ui_anchor_ul(WINDOW *win, int rows, int cols)
{
	wresize(win, rows, cols);
	mvwin(win, 0, 0);
}
void ui_anchor_br(WINDOW *win, int rows, int cols)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	wresize(win, rows, cols);
	mvwin(win, y - rows, x - cols);
}
void ui_anchor_bl(WINDOW *win, int rows, int cols, int yoff, int xoff)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	wresize(win, rows, cols);
	mvwin(win, y - rows + yoff, xoff);
}
void ui_anchor_center(WINDOW *win, int rows, int cols, int yoff, int xoff)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	int oy = (y - rows) / 2 + yoff;
	int ox = (x - cols) / 2 + xoff;
	wresize(win, rows, cols);
	mvwin(win, oy, ox);
}

void ui_clear_win(WINDOW *win)
{
	for (int i=0; i<getmaxy(win); i++) {
		wmove(win, i,0);
		wclrtoeol(win);
	}
	wrefresh(win);
}

void ui_cursorinfo(char *msg)
{
	if (msg[0] != '\0') {
		char buffer[CURSOR_INFO_LENGTH];
		snprintf(buffer, CURSOR_INFO_LENGTH, "ghost :%s", msg);
		ui_write(cursorpanel, 0, buffer);
	} else {
		ui_write(cursorpanel, 0, "--");
	}
	ui_box(cursorpanel);
	wrefresh(cursorpanel);
}

void ui_positioninfo(char *msg)
{
	if (msg[0] != '\0') {
		char buffer[CURSOR_INFO_LENGTH];
		snprintf(buffer, CURSOR_INFO_LENGTH, "@ :%s", msg);
		ui_write(cursorpanel, 1, buffer);
	} else {
		ui_write(cursorpanel, 1, "--");
	}
	ui_box(cursorpanel);
	wrefresh(cursorpanel);
}

void ui_modeinfo(char *msg)
{
	if (msg[0] != '\0') {
		char buffer[CURSOR_INFO_LENGTH];
		snprintf(buffer, CURSOR_INFO_LENGTH, "y :%s", msg);
		ui_write(cursorpanel, 2, buffer);
	} else {
		ui_write(cursorpanel, 2, "");
	}
	ui_box(cursorpanel);
	wrefresh(cursorpanel);
}

void ui_loginfo(char *msg)
{
	dmlog(msg);
	char buffer [LOG_LENGTH];
	strncpy(buffer, msg, LOG_LENGTH);
	// rotate strings onto log
	for (int i=0; i < LOG_COUNT - 1; i++) {
		strcpy(logs[i], logs[i+1]);
	}
	strcpy(logs[LOG_COUNT - 1], buffer);
}

void ui_loginfo_s(char *msg, char *msg2)
{
	char buffer [LOG_LENGTH];
	snprintf(buffer, LOG_LENGTH, msg, msg2);
	ui_loginfo(buffer);
}
void ui_loginfo_i(char *msg, int i)
{
	char buffer [LOG_LENGTH];
	snprintf(buffer, LOG_LENGTH, msg, i);
	ui_loginfo(buffer);
}
void ui_loginfo_is(char *msg, int i, char *msg2)
{
	char buffer [LOG_LENGTH];
	snprintf(buffer, LOG_LENGTH, msg, i, msg2);
	ui_loginfo(buffer);
}
void ui_loginfo_si(char *msg, char *msg2, int i)
{
	char buffer [LOG_LENGTH];
	snprintf(buffer, LOG_LENGTH, msg, msg2, i);
	ui_loginfo(buffer);
}
void ui_loginfo_ss(char *msg, char *msg2, char *msg3)
{
	char buffer [LOG_LENGTH];
	snprintf(buffer, LOG_LENGTH, msg, msg2, msg3);
	ui_loginfo(buffer);
}
void ui_loginfo_ssi(char *msg, char *msg2, char *msg3, int i)
{
	char buffer [LOG_LENGTH];
	snprintf(buffer, LOG_LENGTH, msg, msg2, msg3, i);
	ui_loginfo(buffer);
}

void ui_update_cmdpanel(struct wld_map *map)
{
	// deduce what our context is and provide options
	wmove(cmdpanel, 0, 0);
	wclrtoeol(cmdpanel);

	char buffer[CMD_LENGTH]; // length of cmd bar
	buffer[0] = '\0';
	if (current_map->player->mode == MODE_PLAY) {
		struct wld_tile *t = wld_map_get_tile_at_index(current_map, current_map->player->map_index);
		if (current_map->player->target_mode == TMODE_ACTIVE) {
			strncat(buffer, "  x: exit", 9);
		} else {
			strncat(buffer, "  y: wield  i: inventory  p: rest", 21);
		}
		if (ai_can_get(current_map->player, 0, 0)) {
			strncat(buffer, "  g: get", 8);
		}
		if (t->dead_mob_type && current_map->player->can_aberrate) {
			strncat(buffer, "  b: aberrate", 13);
		}
	}

	if (buffer[0] != '\0')
		ui_write_rc(cmdpanel, 0, 0, buffer);
	wrefresh(cmdpanel);
}

void ui_update_cursorinfo(struct wld_map *map)
{
	int x = map->cursor->x;
	int y = map->cursor->y;

	// Get details about the tile they are on
	struct wld_tile *t = wld_map_get_tile_at(map, x, y);
	if (t->is_visible) {
		struct wld_mob *m = wld_map_get_mob_at(map, x, y);
		struct wld_item *i = wld_map_get_item_at(map, x, y);
		if (m != NULL) {
			// TODO get "what" the mob is doing
			ui_cursorinfo(m->type->short_desc);
		} else if (i != NULL) {
			ui_cursorinfo(i->type->short_desc);
		} else if(t->dead_mob_type) {
			char buffer[CURSOR_INFO_LENGTH];
			sprintf(buffer, "%s (dead)", t->dead_mob_type->short_desc);
			ui_cursorinfo(buffer);
		} else {
			ui_cursorinfo(t->type->short_desc);
		}
	} else {
		ui_cursorinfo("");
	}
}

void ui_update_positioninfo(struct wld_map *map)
{
	int x = map->player->map_x;
	int y = map->player->map_y;
	// Get details about the tile they are on
	struct wld_tile *t = wld_map_get_tile_at(map, x, y);

	if (t->is_visible) {
		struct wld_item *i = wld_map_get_item_at(map, x, y);
		if (i != NULL) {
			ui_positioninfo(i->type->short_desc);
		} else if(t->dead_mob_type) {
			char buffer[CURSOR_INFO_LENGTH];
			sprintf(buffer, "%s (dead)", t->dead_mob_type->short_desc);
			ui_positioninfo(buffer);
		} else {
			ui_positioninfo(t->type->short_desc);
		}
	} else {
		ui_positioninfo("");
	}
}

void ui_update_logpanel(struct wld_map *map)
{
	for (int i=0; i < LOG_COUNT; i++) {
		ui_write(logpanel, i, logs[i]);
	}

	ui_box(logpanel);
	wrefresh(logpanel);
}

void ui_meter_effect(struct wld_effect *e, int len, int row, int col, int fg, int bg)
{
	int fill = (1 - ((double)e->current_iterations / (double)e->type->iterations)) * len;
	char c = ' ';
	for (int i=0; i < len; i++) {
		if (c != '\0')
			c = e->type->title[i];
		if (i <= fill)
			wattrset(mobpanel, COLOR_PAIR(wld_cpair(WCLR_RED, WCLR_YELLOW)));
		else
			wattrset(mobpanel, COLOR_PAIR(wld_cpair(WCLR_RED, WCLR_BLACK)));
		if (c != '\0')
			ui_write_char(mobpanel, row, col + i, c);
		else
			ui_write_char(mobpanel, row, col + i, ' ');
	}
	wattrset(mobpanel, COLOR_PAIR(SCOLOR_NORMAL));
}

int last_mob_list_length = 0;
void ui_update_mobpanel(struct wld_map *map)
{
	// probably just need to do a shadowcast event each turn to get mobs in vision
	if (map->player->mode == MODE_PLAY) {
		ui_write(mobpanel, 0, "-------- Visible ---------");
		int offx = 1;
		int offy = 2;
		for (int i =0; i < last_mob_list_length; i++) {
			ui_clear(mobpanel, i + offy);
		}
		// brute force method is another mobvision call
		// better version is to hook into the primary mobvision call in main draw
		int i=0;
		void onvisible(struct wld_mob* player, int x, int y, double radius) {
			struct wld_tile *tile = wld_map_get_tile_at(map, x, y);
			struct wld_mob *mob = wld_map_get_mob_at(map, x, y);
			struct wld_item *item = wld_map_get_item_at(map, x, y);

			if (mob && !mob->is_dead) { // players are not destroyed
				// mob name
				char buffer[VIS_LENGTH];
				snprintf(buffer, VIS_LENGTH, "- %s", mob->type->title);
				ui_write_rc(mobpanel, i + offy, offx, buffer);
				// icon
				wattrset(mobpanel, COLOR_PAIR(wld_cpair(mob->type->fg_color, WCLR_BLACK)));
				ui_write_char(mobpanel, i + offy, offx, mob->type->sprite);
				wattrset(mobpanel, COLOR_PAIR(SCOLOR_NORMAL));
				i++;
				// health bar
				char hmsg[VIS_LENGTH - 4];
				snprintf(hmsg, VIS_LENGTH, "%3d/%3d", mob->health, mob->maxhealth);
				int hpbarsize = VIS_LENGTH - 5;
				int hpbar = ((double)mob->health / (double)mob->maxhealth) * hpbarsize;
				int cj = 0;
				for (int j=0; j < hpbarsize; j++) {
					// get the character number total to render over the bar
					char c = ' ';
					while (c == ' ' && cj < 7) { // loops until next non blank character found (and less than strlen max)
						c = hmsg[cj];
						cj++;
					}
					// render green front and red bg
					if (hpbar >= j)
						wattrset(mobpanel, COLOR_PAIR(wld_cpair(WCLR_BLACK, WCLR_GREEN)));
					else
						wattrset(mobpanel, COLOR_PAIR(wld_cpair(WCLR_BLACK, WCLR_RED)));
					// render character
					ui_write_char(mobpanel, i + offy, j + offx + 2, c);
					wattrset(mobpanel, COLOR_PAIR(SCOLOR_NORMAL));
				}
				i++;
				// active effects
				for (int j=0; j < mob->active_effects_length; j++) {
					struct wld_effect *e = &mob->active_effects[j];
					if (e->is_active) {
						ui_meter_effect(e, VIS_LENGTH - 5, i + offy, offx + 2, 1, 1);
						//ui_write_rc(mobpanel, i + offy, offx + 2, e->type->title);
						i++;
					}
				}
			}
			if (item) {
				// item name
				char buffer[VIS_LENGTH];
				snprintf(buffer, VIS_LENGTH, "- %s", item->type->title);
				ui_write_rc(mobpanel, i + offy, offx, buffer);
				// icon
				wattrset(mobpanel, COLOR_PAIR(wld_cpair(item->type->fg_color, WCLR_BLACK)));
				ui_write_char(mobpanel, i + offy, offx, item->type->sprite);
				wattrset(mobpanel, COLOR_PAIR(SCOLOR_NORMAL));
				i++;
			}
			if(tile->dead_mob_type) {
				// dead mob
				char buffer[VIS_LENGTH];
				snprintf(buffer, VIS_LENGTH, "- %s (dead)", tile->dead_mob_type->title);
				ui_write_rc(mobpanel, i + offy, offx, buffer);
				// icon
				wattrset(mobpanel, COLOR_PAIR(wld_cpair(WCLR_BLACK, WCLR_RED)));
				ui_write_char(mobpanel, i + offy, offx, tile->dead_mob_type->sprite);
				wattrset(mobpanel, COLOR_PAIR(SCOLOR_NORMAL));
				i++;
			}
		}
		wld_mob_vision(current_map->player, onvisible);
		last_mob_list_length = i;

		// render frame
		ui_box(mobpanel);
		wrefresh(mobpanel);
	}
}

void ui_update_inventorypanel(struct wld_map *map)
{
	if (map->player->mode == MODE_INVENTORY) {
		// probably just need to do a shadowcast event each turn to get mobs in vision
		ui_write(inventorypanel, 0, "----------------- Inventory ------------------");

		// list the items in player possession
		ui_write(inventorypanel, 1, "weapon:");
		struct wld_item *w = map->player->inventory[0];
		if (w != NULL) {
			char buffer[INV_ITEM_LENGTH];
			snprintf(buffer, INV_ITEM_LENGTH, "w: - %s", w->type->title);
			ui_write_rc(inventorypanel, 2, 1, buffer);
			ui_write_char(inventorypanel, 2, 4, w->type->sprite);
		}
		else
			ui_write_rc(inventorypanel, 2, 1, "-- none --");

		ui_write(inventorypanel, 3, "armor:");
		struct wld_item *a = map->player->inventory[1];
		if (a != NULL) {
			char buffer[INV_ITEM_LENGTH];
			snprintf(buffer, INV_ITEM_LENGTH, "a: - %s", a->type->title);
			ui_write_rc(inventorypanel, 4, 1, buffer);
			ui_write_char(inventorypanel, 4, 4, a->type->sprite);
		}
		else
			ui_write_rc(inventorypanel, 4, 1, "-- none --");

		ui_write(inventorypanel, 5, "pack:");
		int row = 6;
		for (int i=2; i<INVENTORY_SIZE; i++) {
			// depending on position of item designate interation key
			char key = ' ';
			switch (i) {
				case  2: key = '1'; break;
				case  3: key = '2'; break;
				case  4: key = '3'; break;
				case  5: key = '4'; break;
				case  6: key = '5'; break;
				case  7: key = '6'; break;
				case  8: key = '7'; break;
				case  9: key = '8'; break;
				case 10: key = '9'; break;
				case 11: key = '0'; break;
			}
			char buffer[INV_ITEM_LENGTH];
			char *desc;
			unsigned long sprite;
			struct wld_item *item = map->player->inventory[i];
			if (item != NULL) {
				desc = item->type->title;
				sprite = item->type->sprite;
			} else {
				desc = "--";
				sprite = '-';
			}
			snprintf(buffer, INV_ITEM_LENGTH, "%c: - %s", key, desc);
			ui_write_rc(inventorypanel, row, 1, buffer);
			ui_write_char(inventorypanel, row, 4, sprite); // stick sprite over '-'
			row++;
		}
		ui_write_rc(inventorypanel, row + 1, 1, "x: close");


		ui_box_color(inventorypanel, TCOLOR_YELLOW);
		wrefresh(inventorypanel);
	}
}

void ui_update_aberratepanel(struct wld_map *map)
{
	ui_write(aberratepanel, 0, "----------------------- Aberration -----------------------");
	ui_write(aberratepanel, 2, "Looking at the dead beneath your feet the lust for");
	ui_write(aberratepanel, 3, "evolution and mutation grows. Do you wish to consume?");
	ui_write(aberratepanel, 5, "y: yes  n: no");
	ui_box(aberratepanel);
	wrefresh(aberratepanel);
}

void ui_update_usepanel(struct wld_map *map)
{
	// probably just need to do a shadowcast event each turn to get mobs in vision
	if (map->player->mode == MODE_USE) {
		switch (use_type) {
		case USE_ITEM: {
				// intro item
				char buffer[USE_LENGTH];
				snprintf(buffer, USE_LENGTH, "You prepare to use %s.", use_item->type->short_desc);
				ui_write(usepanel, 0, buffer);
				ui_write_len(usepanel, 2, use_item->type->use_text_1, USE_LENGTH);
				ui_write_len(usepanel, 3, use_item->type->use_text_2, USE_LENGTH);
				if (use_item->map_found > -1) {
					char b[USE_LENGTH];
					snprintf(b, USE_LENGTH, "Found on level %d.", use_item->map_found + 1);
					ui_write_len(usepanel, 4, b, USE_LENGTH);
				}

				// give options
				int offset = 6;
				bool equippable = use_item->type->is_weq || use_item->type->is_aeq;
				if (use_item_slot == 0 || use_item_slot == 1) {
					ui_write(usepanel, offset, "e: unequip");
					offset++;
				} else if (equippable) {
					ui_write(usepanel, offset, "e: equip");
					offset++;
				}
				if (use_item->type->fn_use != NULL) {
					char label[USE_LENGTH];
					snprintf(label, USE_LENGTH, "u: %s", use_item->type->use_label);
					ui_write(usepanel, offset++, label);
				}
				if (use_item->type->fn_drink != NULL) {
					char label[USE_LENGTH];
					snprintf(label, USE_LENGTH, "q: %s", use_item->type->drink_label);
					ui_write(usepanel, offset++, label);
				}
				ui_write(usepanel, offset++, "d: drop");
				ui_write(usepanel, ++offset, "x: close");
			}
			break;
		}
		ui_box(usepanel);
		wrefresh(usepanel);
	}
}

void ui_unset_use()
{
	use_type = USE_NONE;
	use_item = NULL;
	use_item_slot = -1;
}

void ui_set_use_item(struct wld_item* item, int item_slot)
{
	use_type = USE_ITEM;
	use_item = item;
	use_item_slot = item_slot;
}

void ui_use_item_select(struct wld_mob* player, int item_slot)
{
	ui_clear_win(inventorypanel);
	struct wld_item *item = wld_mob_get_item_in_slot(player, item_slot);
	if (item != NULL) {
		ui_set_use_item(item, item_slot);
		player->mode = MODE_USE;
	}
}

// UI END
///////////////////////////



///////////////////////////
// MAP EVENTS START

void map_on_effect(struct wld_map *map, struct wld_vfx *effect)
{
	struct draw_struct ds = wld_map_get_drawstruct(map, effect->x, effect->y);
	ui_clear_win(usepanel);
	switch (effect->type) {
	case VFX_HEAL: {
			for (int i=0; i < 2; i++) {
				if (i % 2 == 0)
					ps_draw_tile(effect->y, effect->x, ds.sprite, ECOLOR_HEAL_A, ' ', -1, false);
				else
					ps_draw_tile(effect->y, effect->x, ds.sprite, ECOLOR_HEAL_B, ' ', -1, false);
				ps_refresh_map_pad();
				napms(100);
			}
		}
		break;
	case VFX_DMG_HIT: {
			for (int i=0; i < 2; i++) {
				if (i % 2 == 0)
					ps_draw_tile(effect->y, effect->x, ds.sprite, ECOLOR_DMG_A, ' ', -1, false);
				else
					ps_draw_tile(effect->y, effect->x, ds.sprite, ECOLOR_DMG_B, ' ', -1, false);
				ps_refresh_map_pad();
				napms(100);
			}
		}
		break;
	case VFX_SUMMON: {
			dmlog("SUMMON");
			for (int i=0; i < 2; i++) {
				if (i % 2 == 0)
					ps_draw_tile(effect->y, effect->x, ds.sprite, ECOLOR_SUMMON_A, ' ', -1, false);
				else
					ps_draw_tile(effect->y, effect->x, ds.sprite, ECOLOR_SUMMON_B, ' ', -1, false);
				ps_refresh_map_pad();
				napms(100);
			}
		}
		break;
	}
}

void map_on_player_transition(struct wld_map *map, struct wld_mob *player, bool forward)
{
	if (forward) {
		int map_id = map->id;
		if (map_id + 1 == world->maps_length) {
			// end of the maps
			play_state = PS_WIN;
		} else {
			// transition player to next map
			wld_transition_player(world, map, world->maps[map_id + 1], forward);
			next_map = world->maps[map_id + 1];
			play_state = PS_MAPCHANGE;
		}
	} else {
		int map_id = map->id;
		if (map_id == 0) {
			// no where to go back too
			ui_loginfo("Though aflame in fear, you cannot retreat.");
		} else {
			// transition player to prior map
			wld_transition_player(world, map, world->maps[map_id - 1], forward);
			next_map = world->maps[map_id - 1];
			play_state = PS_MAPCHANGE;
		}
	}
}

void map_on_cursormove(struct wld_map *map, int x, int y, int index)
{
	ui_update_cursorinfo(map);
}

void map_on_playermove(struct wld_map *map, struct wld_mob *player, int x, int y, int index)
{
	ui_update_positioninfo(map);
}


void map_on_mob_heal(struct wld_map *map, struct wld_mob* mob, int amt, struct wld_item* item)
{
	if (item != NULL) {
		ui_loginfo_ssi("The %s healed %s for %d.", item->type->title, mob->type->short_desc, amt);
	} else
		ui_loginfo_si("%s healed for %d.", mob->type->short_desc, amt);
}

void map_on_mob_attack_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player, int dmg, struct wld_item* item)
{
	ui_loginfo_is("You were attacked for %d by %s.", dmg, aggressor->type->short_desc);
}

void map_on_mob_whiff_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player, struct wld_item* item)
{
	ui_loginfo_s("Attack from %s misses.", aggressor->type->short_desc);
}

void map_on_mob_kill_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player, struct wld_item* item)
{
	ui_loginfo_s("You were killed by %s.", aggressor->type->short_desc);
	play_state = PS_GAMEOVER;
}

void map_on_player_heal(struct wld_map *map, struct wld_mob* player, int amt, struct wld_item* item)
{
	if (item != NULL) {
		ui_loginfo_si("The %s healed you for %d.", item->type->title, amt);
	} else
		ui_loginfo_i("You healed for %d.", amt);
}

void map_on_player_attack_mob(struct wld_map *map, struct wld_mob* player, struct wld_mob* defender, int dmg, struct wld_item* item)
{
	if (item != NULL) {
		ui_loginfo_ssi("With %s you attacked %s for %d.", item->type->title, defender->type->short_desc, dmg);
	} else
		ui_loginfo_si("You attacked %s for %d.", defender->type->short_desc, dmg);
}

void map_on_player_whiff(struct wld_map *map, struct wld_mob* player, struct wld_item* item)
{
	ui_loginfo("You attacked and missed.");
}

void map_on_player_whiff_mob(struct wld_map *map, struct wld_mob* player, struct wld_mob* defender, struct wld_item* item)
{
	ui_loginfo_s("You attacked and missed %s.", defender->type->short_desc);
}

void map_on_player_kill_mob(struct wld_map *map, struct wld_mob* player, struct wld_mob* defender, struct wld_item* item)
{
	if (item != NULL) {
		ui_loginfo_ss("You killed %s with your %s.", defender->type->short_desc, item->type->title);
	} else
		ui_loginfo_s("You killed %s.", defender->type->short_desc);
}

void map_on_player_pickup_item(struct wld_map *map, struct wld_mob* player, struct wld_item* item)
{
	ui_loginfo_s("You picked up %s.", item->type->short_desc);
}

void map_on_player_pickup_item_fail(struct wld_map *map, struct wld_mob* player, struct wld_item* item)
{
	ui_loginfo("Inventory is full.");
}

void map_on_player_drop_item(struct wld_map *map, struct wld_mob* player, struct wld_item* item)
{
	ui_loginfo_s("You dropped %s to the floor.", item->type->short_desc);
}

void map_on_player_drop_item_fail(struct wld_map *map, struct wld_mob* player, struct wld_item* item)
{
	ui_loginfo("Unable to drop; floor is occupied by another item.");
}


// MAP EVENTS END
///////////////////////////



///////////////////////////
//PLAY MODE START

// this runs in the player update loop and should take an input that triggers world updates
void ai_player_input(struct wld_mob* player)
{
	//nodelay(stdscr, true);
	escapedelay(false);
	bool listen = true;
	trigger_world = false;
	while (listen) {
		int key = getch();
		switch (player->mode) {
			// while in play mode listen to targeting method commands if active
			case MODE_PLAY:
				switch (player->target_mode) {
				// If not in a targeting mode then listen for interaction inputs
				case TMODE_NONE:
					switch (key) {
					// CHEATS:
					case KEY_F(1):
						// teleport to exit and don't trigger exit
						wld_cheat_teleport_exit(player->map, player);
						listen = false;
						break;
					case KEY_F(2):
						// enable aberate
						current_map->player->can_aberrate = true;
						listen = false;
						break;
					// Player movement
					case KEY_BACKSPACE:
						play_state = PS_MENU;
						listen = false;
						break;
					case KEY_w:
						// attempt move up, if blocked determine what is blocking me and attempt
						// to "interact" with it, if its an enemy mob, attack with melee?
						// if its a tile attempt to search it or activate it, "on touch" it?
						// if its empty tile move into it
						// if its a transitional tile interrupt confirm? call back into transition?
						trigger_world = ai_act_upon(player, 0, -1);
						listen = false;
						break;
					case KEY_s:
						trigger_world = ai_act_upon(player, 0, 1);
						listen = false;
						break;
					case KEY_a:
						trigger_world = ai_act_upon(player, -1, 0);
						listen = false;
						break;
					case KEY_d:
						trigger_world = ai_act_upon(player, 1, 0);
						listen = false;
						break;
					case KEY_q:
						trigger_world = ai_act_upon(player, -1, -1);
						listen = false;
						break;
					case KEY_e:
						trigger_world = ai_act_upon(player, 1, -1);
						listen = false;
						break;
					case KEY_z:
						trigger_world = ai_act_upon(player, -1, 1);
						listen = false;
						break;
					case KEY_x:
						trigger_world = ai_act_upon(player, 1, 1);
						listen = false;
						break;
					case KEY_p:
						trigger_world = ai_rest(player);
						listen = false;
						ui_loginfo("You rested.");
						break;
					case KEY_g:
						trigger_world = ai_get(player, 0, 0);
						listen = false;
						break;
					// Player attack
					case KEY_y:
						// enter targeting mode for active weapon
						if (ai_player_draw_weapon(player)) {
							// trigger targeting mode
							ui_loginfo("You draw your weapon.");
							//switch (player->target_mode) {
							//	//case TARGET_PASSIVE: ui_modeinfo("passive"); break; // not a thing for weapons
							//	//case TARGET_SELF: ui_modeinfo("self"); break; // not a thing for weapons
							//	case TARGET_MELEE: ui_modeinfo("melee"); break;
							//	case TARGET_RANGED_LOS: ui_modeinfo("ranged los"); break;
							//	case TARGET_RANGED_LOS_AOE: ui_modeinfo("los aoe"); break;
							//	case TARGET_RANGED_TEL: ui_modeinfo("ranged instant"); break;
							//	case TARGET_RANGED_TEL_AOE: ui_modeinfo("tele aoe"); break;
							//}
						} else {
							ui_loginfo("You are unequipped and grasp at nothing.");
						}
						listen = false;
						break;
					case KEY_i:
						// enter inventory management mode
						player->mode = MODE_INVENTORY;
						ui_clear_win(mobpanel);
						listen = false;
						break;
					case KEY_b:
						// enter aberration mode
						if (player->can_aberrate) {
							struct wld_tile *t = wld_map_get_tile_at_index(player->map, player->map_index);
							if (t->dead_mob_type) {
								player->mode = MODE_ABERRATE;
							}
						}
						listen = false;
						break;
					}
					break;
				case TMODE_ACTIVE:
					// we are in targeting mode, be it an item, spell or whatever set us into this mode
					switch (key) {
					case KEY_SPACE:
						if (player->active_item) {
							trigger_world = ai_player_trigger_target(player);
							if (trigger_world) {
								if (player->active_item == NULL) {
									ai_player_leave_targeting(player);
								}
							}
						} else {
							trigger_world = ai_player_trigger_target(player);
						}
						if (!trigger_world)
							ui_loginfo("Incapable of such attempt.");
						listen = false;
						break;
					// exiting target mode
					case KEY_ESC:
					case KEY_x:
						ai_player_leave_targeting(player);
						listen = false;
						break;
					case KEY_y:
						// escape targeting mode
						if (ai_player_sheath_weapon(player))
						ui_loginfo("You sheath your weapon.");
						listen = false;
						break;
					}
					break;
				} // eo target mode switch

				// always active should we move this to play mode?
				switch (key) {
				// Cursor movement
				case KEY_UP:
				case KEY_8: // up
					wld_map_move_cursor(current_map, 0, -1);
					listen = false;
					break;
				case KEY_DOWN:
				case KEY_2: // down
					wld_map_move_cursor(current_map, 0, 1);
					listen = false;
					break;
				case KEY_LEFT:
				case KEY_4: // left
					wld_map_move_cursor(current_map, -1, 0);
					listen = false;
					break;
				case KEY_RIGHT:
				case KEY_6: // right
					wld_map_move_cursor(current_map, 1, 0);
					listen = false;
					break;
				case KEY_7: // upleft
					wld_map_move_cursor(current_map, -1, -1);
					listen = false;
					break;
				case KEY_9: // upright
					wld_map_move_cursor(current_map, 1, -1);
					listen = false;
					break;
				case KEY_1: // downleft
					wld_map_move_cursor(current_map, -1, 1);
					listen = false;
					break;
				case KEY_3: // downright
					wld_map_move_cursor(current_map, 1, 1);
					listen = false;
					break;
				case KEY_TAB: // focus on next visible mob
					// search the mob list for the next visible mob
					ui_next_visible_mob();
					listen = false;
					break;
				} // eo switch always

				break; // eo MODE PLAY

			// If in Inventory Mode show inventory and listen to inventory commands
			case MODE_INVENTORY:
				switch (key) {
				case KEY_ESC:
				case KEY_x:
				case KEY_i:
					ui_clear_win(inventorypanel);
					player->mode = MODE_PLAY;
					listen = false;
					break;
				case KEY_w: ui_use_item_select(player,  0); listen = false; break;
				case KEY_a: ui_use_item_select(player,  1); listen = false; break;
				case KEY_1: ui_use_item_select(player,  2); listen = false; break;
				case KEY_2: ui_use_item_select(player,  3); listen = false; break;
				case KEY_3: ui_use_item_select(player,  4); listen = false; break;
				case KEY_4: ui_use_item_select(player,  5); listen = false; break;
				case KEY_5: ui_use_item_select(player,  6); listen = false; break;
				case KEY_6: ui_use_item_select(player,  7); listen = false; break;
				case KEY_7: ui_use_item_select(player,  8); listen = false; break;
				case KEY_8: ui_use_item_select(player,  9); listen = false; break;
				case KEY_9: ui_use_item_select(player, 10); listen = false; break;
				case KEY_0: ui_use_item_select(player, 11); listen = false; break;
				}
				break; // eo MODE INVENTORY
			case MODE_ABERRATE:
				switch (key) {
				case KEY_ESC:
				case KEY_x:
					ui_unset_use();
					ui_clear_win(aberratepanel);
					player->mode = MODE_PLAY;
					listen = false;
					break;
				}
				break;
			case MODE_USE:
				// EXIT USE MODE
				switch (key) {
				case KEY_ESC:
				case KEY_x:
					ui_unset_use();
					ui_clear_win(usepanel);
					player->mode = MODE_INVENTORY;
					listen = false;
					break;
				// EQUIP ITEM
				case KEY_e:
					if (use_item_slot == 0 || use_item_slot == 1) {
						if (!wld_mob_unequip(player, use_item_slot))
							// switch to new slot?
							// TODO make callback events?
							ui_loginfo("Cannot unequip; inventory is full.");
					} else {
						if (wld_mob_equip(player, use_item_slot))
							ui_loginfo("Item equipped.");
					}
					ui_unset_use();
					ui_clear_win(usepanel);
					player->mode = MODE_INVENTORY;
					listen = false;
					break;
				// DRINK ITEM directly
				case KEY_q:
					if (wld_mob_drink_item(player, use_item_slot)) {
						ui_unset_use();
						ui_clear_win(usepanel);
						ui_clear_win(inventorypanel);
						player->mode = MODE_PLAY;
					}
					listen = false;
					break;
				// USE ITEM directly
				case KEY_u:
					if (ai_player_set_use_item(player, use_item_slot)) {
						ui_unset_use();
						ui_clear_win(usepanel);
						ui_clear_win(inventorypanel);
						player->mode = MODE_PLAY;
						ai_player_enter_targeting(player);
					}
					listen = false;
					break;
				// DROP ITEM
				case KEY_d:
					if (wld_mob_drop_item(player, use_item_slot)) {
						ui_unset_use();
						ui_clear_win(usepanel);
						ui_clear_win(inventorypanel);
						player->mode = MODE_PLAY;
					}
					listen = false;
					break;
				}
				break; // eo MODE USE
		}
	} // eo while listen
	escapedelay(true);
	//nodelay(stdscr, false);
}

// call this when the current map changes
void ps_on_mapchange()
{
	if (map_pad)
		delwin(map_pad);

	// World is large indexed map to start
	map_pad = newpad(current_map->rows * map_rows_scale + ui_map_border * 2, current_map->cols * map_cols_scale + ui_map_border * 2); // pad extra two for us two draw black around the edges

	current_map->on_effect = map_on_effect;

	current_map->on_player_map_transition = map_on_player_transition;
	current_map->on_cursormove = map_on_cursormove;
	current_map->on_playermove = map_on_playermove;

	current_map->on_mob_heal = map_on_mob_heal;
	current_map->on_mob_attack_player = map_on_mob_attack_player;
	current_map->on_mob_whiff_player = map_on_mob_whiff_player;
	current_map->on_mob_kill_player = map_on_mob_kill_player;

	current_map->on_player_heal = map_on_player_heal;
	current_map->on_player_attack_mob = map_on_player_attack_mob;
	current_map->on_player_whiff = map_on_player_whiff;
	current_map->on_player_whiff_mob = map_on_player_whiff_mob;
	current_map->on_player_kill_mob = map_on_player_kill_mob;
	current_map->on_player_pickup_item = map_on_player_pickup_item;
	current_map->on_player_pickup_item_fail = map_on_player_pickup_item_fail;
	current_map->on_player_drop_item = map_on_player_drop_item;
	current_map->on_player_drop_item_fail = map_on_player_drop_item_fail;

	current_map->player->ai_player_input = ai_player_input;

}

void ps_build_world()
{
	dm_seed(time(NULL));
	int seed = dm_randi();
	seed = 146;
	dmlogi("SEED", seed);
	world = wld_new_world(seed, 2);
	current_map = world->maps[0];

	// clear visible mob list
	ui_clear_visible_mobs(true);

	ps_on_mapchange();
}

void ps_destroy_world()
{
	// THIS HAS NOT BEEN TESTED I CANT FIND DOCS ON HOW TO CLEAN UP PAD MEMORY
	delwin(map_pad);
	//wld_delete_map(current_map);

	wld_delete_world(world);

	clear();
}

void ps_layout_ui()
{
	int sx, sy;
	getmaxyx(stdscr,sy,sx);

	// info panel is at bottom
	int logpanel_cols = LOG_LENGTH + 2;
	int logpanel_rows = LOG_COUNT + 2;
	ui_anchor_br(logpanel, logpanel_rows, logpanel_cols);
	ui_box(logpanel);

	// anchor mobpanel to right side
	// TODO make this fit the right side
	// TODO scrollable?
	int mobpanel_cols = VIS_LENGTH + 2;
	int mobpanel_rows = sy - logpanel_rows;
	ui_anchor_ur(mobpanel, mobpanel_rows, mobpanel_cols);
	ui_box(mobpanel);

	// make map the leftover width up to the mob panel
	ui_map_cols = sx - mobpanel_cols - 1;
	ui_map_rows = sy - logpanel_rows - 1;

	// mode bar on the bottom
	ui_anchor_bl(cursorpanel, logpanel_rows, sx - logpanel_cols - 1, 0, 0);
	ui_box(cursorpanel);

	// inventory panel
	int invpanel_cols = INV_LENGTH + 2;
	int invpanel_rows = sy - logpanel_rows;
	ui_anchor_ur(inventorypanel, invpanel_rows, invpanel_cols);
	ui_box(inventorypanel);

	// use panel
	int usepanel_cols = USE_LENGTH + 4;
	int usepanel_rows = 16;
	ui_anchor_center(usepanel, usepanel_rows, usepanel_cols, -(logpanel_rows / 2), 0);
	ui_box(usepanel);

	// cmd panel is at the bottome of the map
	int cmdpanel_cols = CMD_LENGTH;
	int cmdpanel_rows = 1;
	ui_anchor_bl(cmdpanel, cmdpanel_rows, cmdpanel_cols, - logpanel_rows, 0);

	// aberrate panel
	int aberratepanel_cols = ABE_LENGTH + 4;
	int aberratepanel_rows = 16;
	ui_anchor_center(aberratepanel, aberratepanel_rows, aberratepanel_cols, -(logpanel_rows / 2), 0);
	ui_box(aberratepanel);
}

// this is called when window resize event happens
void ps_reset_ui()
{
	clear();
	endwin();
	refresh();

	ps_layout_ui();

	refresh();
}

void ps_build_ui()
{
	cmdpanel = newwin(0, 0, 0, 0);
	cursorpanel = newwin(0, 0, 0, 0);
	logpanel = newwin(0, 0, 0, 0);
	mobpanel = newwin(0, 0, 0, 0);
	inventorypanel = newwin(0, 0, 0, 0);
	usepanel = newwin(0, 0, 0, 0);
	aberratepanel = newwin(0, 0, 0, 0);

	ps_layout_ui();

	ui_loginfo("You awake in darkness, then a light appears...");
}

void ps_destroy_ui()
{
	delwin(aberratepanel);
	delwin(usepanel);
	delwin(inventorypanel);
	delwin(mobpanel);
	delwin(logpanel);
	delwin(cursorpanel);
	delwin(cmdpanel);
}

void ps_play_draw_onvisible(struct wld_mob* mob, int x, int y, double radius)
{
	// get the map tile at this position and draw it
	struct wld_tile *t = wld_map_get_tile_at(mob->map, x, y);
	t->is_visible = true;
	t->was_visible = true;
	struct wld_mob *visible_mob = wld_map_get_mob_at_index(mob->map, t->map_index);
	if (visible_mob && visible_mob != mob) { //player
		visible_mobs[visible_mobs_length] = visible_mob;
		visible_mobs_length++;
	}
}

// r and c are world coords, translate them to map pad with border in mind
void ps_draw_tile(int r, int c, unsigned long cha, int colorpair, unsigned long cha2, int colorpair2, bool bold)
{
	int yborder = ui_map_border * map_rows_scale;
	int xborder = ui_map_border * map_cols_scale;
	wmove(map_pad, r * map_rows_scale + yborder, c * map_cols_scale + xborder); // pads by scaling out
	if (bold)
		wattrset(map_pad, COLOR_PAIR(colorpair) | A_BOLD);
	else
		wattrset(map_pad, COLOR_PAIR(colorpair));

	waddch(map_pad, cha); // pad
	// extra padding if we are scaling the columns to make it appear at a better ratio in the terminal
	if (map_cols_scale > 1 || map_rows_scale > 1) {
		for (int i=1; i < map_cols_scale; i++) {
			wmove(map_pad, r * map_rows_scale + yborder, c * map_cols_scale + i + xborder);
			int second_cpair = colorpair;
			if (colorpair2 != -1)
				second_cpair = colorpair2;
			wattrset(map_pad, COLOR_PAIR(second_cpair));
			waddch(map_pad, cha2); // pad
			// TODO, stopped working correctly after shadowcaster
			//for (int j=1; j < map_rows_scale; j++) {
			//	wmove(map_pad, r * map_rows_scale + j, c * map_cols_scale + i);
			//	wattrset(map_pad, COLOR_PAIR(bg_only));
			//	waddch(map_pad, ' '); // pad
			//}
		}
	}
}

void ps_refresh_map_pad()
{
	// lets calculate where to offset the pad
	// we need to shift the pad if the player is within X range of a map edge
	int plyrpad_x = current_map->player->map_x;
	int plyrpad_y = current_map->player->map_y;

	int shiftpad_x = 0, shiftpad_y = 0, shiftwin_x = 0, shiftwin_y = 0;
	int paddingx = ui_map_padding * map_cols_scale;
	int paddingy = ui_map_padding * map_rows_scale;
	if (plyrpad_x < paddingx)
		shiftwin_x += paddingx - plyrpad_x * map_cols_scale;
	if (plyrpad_x * map_cols_scale > ui_map_cols - paddingx)
		shiftpad_x += paddingx + (plyrpad_x * map_cols_scale - ui_map_cols);
	if (plyrpad_y < paddingy)
		shiftwin_y += paddingy - plyrpad_y * map_rows_scale;
	if (plyrpad_y * map_rows_scale > ui_map_rows - paddingy)
		shiftpad_y += paddingy + (plyrpad_y * map_rows_scale - ui_map_rows);

	refresh(); // has to be called before prefresh for some reason?
	prefresh(map_pad, 0 + shiftpad_y, 0 + shiftpad_x, 0 + shiftwin_y, 0 + shiftwin_x, ui_map_rows, ui_map_cols);
}

void ps_play_draw()
{
	// do not clear, it causes awful redraw
	//clear();
	//wclear(map_pad);

	// Clear our list of last seen mobs
	ui_clear_visible_mobs(false);
	// Draw tiles within the vision of the player
	wld_mob_vision(current_map->player, ps_play_draw_onvisible);

	// Clear map without flutter
	for (int padr=0; padr < current_map->rows + ui_map_border*2; padr++) {
		wmove(map_pad, padr, 0);
		wclrtoeol(map_pad);
		// skip borders skip top and bottom rows so they do not screen tear
		if (padr == 0 || padr == current_map->rows + 1)
			continue;
		int r = padr - 1;
		for (int padc=0; padc < current_map->cols + ui_map_border*2; padc++) {
			// skip borders
			if (padc == 0 || padc == current_map->cols + ui_map_border)
				continue;
			int c = padc - 1;

			struct wld_tile *t = wld_map_get_tile_at(current_map, c, r);

			if (t->is_visible) {
				struct draw_struct ds = wld_map_get_drawstruct(current_map, c, r);
				ps_draw_tile(r, c, ds.sprite, ds.colorpair, ds.sprite_2, ds.colorpair_2, false);

			} else if (t->was_visible) {
				struct draw_struct ds = wld_map_get_drawstruct_memory(current_map, c, r);
				ps_draw_tile(r, c, ds.sprite, ds.colorpair, ds.sprite_2, ds.colorpair_2, false);
			}
		}
	}

	// Draw targeting mode
	if (current_map->player->target_mode == TMODE_ACTIVE) {
		// highlight the tiles in melee range of the player
		int map_x = current_map->player->map_x;
		int map_y = current_map->player->map_y;
		void inspect(int x, int y) {
			struct wld_tile *t = wld_map_get_tile_at(current_map, x, y);
			if (t->is_visible) {
				struct draw_struct ds = wld_map_get_drawstruct(current_map, x, y);
				if (t->dead_mob_type != NULL)
					ps_draw_tile(t->map_y, t->map_x, ds.sprite, ds.colorpair, ' ', -1, false);
				else
					ps_draw_tile(t->map_y, t->map_x, ds.sprite, SCOLOR_TARGET, ' ', -1, false);
			}
		}
		wld_mob_inspect_targetables(current_map->player, inspect);
	}

	// Draw cursor
	int yborder = ui_map_border * map_rows_scale;
	int xborder = ui_map_border * map_cols_scale;
	wmove(map_pad, current_map->cursor->y * map_rows_scale + yborder, current_map->cursor->x * map_cols_scale + xborder);
	wattrset(map_pad, COLOR_PAIR(SCOLOR_CURSOR));
	unsigned long ch = mvwinch(map_pad, current_map->cursor->y * map_rows_scale + yborder, current_map->cursor->x * map_cols_scale + xborder) & A_CHARTEXT;
	waddch(map_pad, ch);

	ps_refresh_map_pad();

	// UI constants (needs to be done in an event?)
	ui_update_logpanel(current_map);
	ui_update_cursorinfo(current_map);
	ui_update_positioninfo(current_map);

	wrefresh(cursorpanel);
	ui_update_cmdpanel(current_map);
	if (current_map->player->mode == MODE_PLAY)
		ui_update_mobpanel(current_map);
	if (current_map->player->mode == MODE_INVENTORY)
		ui_update_inventorypanel(current_map);
	if (current_map->player->mode == MODE_USE)
		ui_update_usepanel(current_map);
	if (current_map->player->mode == MODE_ABERRATE)
		ui_update_aberratepanel(current_map);
}

void ps_play_update()
{
	// depending on input change and trigger various updates

	// loop over map mobs and run their update routines
	// update the player first, then others
	// why? because we do not draw until after
	// everything has updated, so we if pause
	// for player input in the middle of the list
	// mobs before us can trigger_world process
	// then not be drawn, while others after are
	// not triggered, and the result is that mobs
	// before do not visually update in real time
	if (current_map->player) {
		wld_update_mob(current_map->player);
	}
	for (int i=0; i < current_map->mobs_length; i++) {
		struct wld_mob *m = current_map->mobs[i];
		if (trigger_world && !m->is_player) {
			wld_update_mob(m);
		}
	}

	// reset map elements
	// Clear map without flutter
	for (int r=0; r < current_map->rows; r++) {
		for (int c=0; c < current_map->cols; c++) {
			struct wld_tile *t = wld_map_get_tile_at(current_map, c, r);
			t->is_visible = false;
		}
	}

	// clean up mob graveyard
	for (int i=0; i < current_map->mobs_length; i++) {
		struct wld_mob *m = current_map->mobs[i];
		if (m->is_destroy_queued) {
			wld_map_destroy_mob(current_map, m);
			i--; // move iterator back because mob list is now shorter and shifted back
		}
	}
}

// PLAY MODE END
///////////////////////////



///////////////////////////
// PLAY MENU START

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

// PLAY MENU END
///////////////////////////




///////////////////////////
// LOGGER LINKS START

void wld_log(char *msg)
{
	ui_loginfo(msg);
}
void wld_log_s(char *msg, char *s)
{
	ui_loginfo_s(msg, s);
}
void wld_log_ss(char *msg, char *s, char *s2)
{
	ui_loginfo_ss(msg, s, s2);
}
void wld_log_ms(char* msg, struct wld_mob* mob)
{
	ui_loginfo_s(msg, mob->type->short_desc);
}
void wld_log_it(char* msg, struct wld_item* item)
{
	ui_loginfo_s(msg, item->type->title);
}
void wld_log_ts(char* msg, struct wld_tile* tile)
{
	ui_loginfo_s(msg, tile->type->short_desc);
}

// LOGGER LINKS END
///////////////////////////

