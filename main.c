#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <menu.h>
#include <signal.h>

#include "dm_defines.h"
#include "dm_debug.h"
#include "dm_gametime.h"
#include "dm_draw.h"
#include "dm_world.h"

#define SCOLOR_NORMAL	1
#define TCOLOR_NORMAL	2
#define TCOLOR_OMINOUS	3
#define TCOLOR_BLACK	4
#define TCOLOR_SKY	5
#define TCOLOR_DAWN	6
#define SCOLOR_ALLWHITE 7
#define SCOLOR_CURSOR   8
#define TCOLOR_PURPLE   9
#define SCOLOR_BLOOD    10
#define SCOLOR_ALLBLACK 11

#define KEY_RETURN	10
#define KEY_ESC		27
#define KEY_SPACE	32

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


///////////////////////////
// UTILITIES

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


///////////////////////////
// GAME BUILD

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
bool trigger_world = false;

bool term_resized = false;
void on_term_resize(int dummy)
{
	term_resized = true;
}
bool g_setup()
{
	// setup debug file pointer
	dmlogopen("log.txt", "w");

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
	init_pair(SCOLOR_CURSOR,	COLOR_BLACK,	COLOR_MAGENTA);
	init_pair(TCOLOR_PURPLE,	COLOR_MAGENTA,	COLOR_BLACK);
	init_pair(SCOLOR_BLOOD,		COLOR_BLACK,	COLOR_RED);
	init_pair(SCOLOR_ALLBLACK,	COLOR_BLACK,	COLOR_BLACK);

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


///////////////////////////
// INTRODUCTION STATE

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


///////////////////////////
// TITLE SCREEN STATE

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


///////////////////////////
// PLAY STATE

enum PLAY_STATE
{
	PS_START,
	PS_PLAY,
	PS_MENU,
	PS_MAPCHANGE,
	PS_GAMEOVER,
	PS_END, // do not call exit directly
	PS_EXIT,
};
enum PLAY_STATE play_state = PS_START;

// MAP SETTINGS / VARS
int map_rows_scale = 1;
int map_cols_scale = 2;
WINDOW *map_pad;
struct wld_map *current_map;
int ui_map_cols;
int ui_map_rows;
int ui_map_padding = 15;

// PLAY UI PANELS
WINDOW* cursorpanel;
WINDOW* logpanel;
WINDOW* mobpanel;
#define CURSOR_INFO_LENGTH 45
#define LOG_COUNT 7
#define LOG_LENGTH 60
char logs[LOG_COUNT][LOG_LENGTH];

void ui_box(WINDOW* win)
{
	//box(win, ACS_VLINE, ACS_HLINE);
	//box(win, ':', '-');
	//wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
	wattrset(win, COLOR_PAIR(TCOLOR_PURPLE));
	wborder(win, '|', '|', '=', '=', '@', '@', '@', '@');
	wattrset(win, COLOR_PAIR(TCOLOR_NORMAL));
}

// helper to write to a row in a panel we boxed
void ui_write(WINDOW *win, int row, char *msg)
{
	dm_clear_row_in_win(win, row + 1);
	wmove(win, row + 1, 2);
	waddstr(win, msg);
}

//void ui_anchor_ur(WINDOW* win, float height, float width)
void ui_anchor_ur(WINDOW* win, int rows, int cols)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	wresize(win, rows, cols);
	mvwin(win, 0, x - cols);
}
void ui_anchor_br(WINDOW *win, int rows, int cols)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	wresize(win, rows, cols);
	mvwin(win, y - rows, x - cols);
}
void ui_anchor_bl(WINDOW *win, int rows, int cols)
{
	int y, x;
	getmaxyx(stdscr, y, x);
	wresize(win, rows, cols);
	mvwin(win, y - rows, 0);
}

void ui_cursorinfo(char *msg)
{
	if (msg[0] != '\0') {
		char full[CURSOR_INFO_LENGTH] = "ghost: ";
		ui_write(cursorpanel, 0, strncat(full, msg, CURSOR_INFO_LENGTH - 7));
	} else {
		ui_write(cursorpanel, 0, "--");
	}
	ui_box(cursorpanel);
	wrefresh(cursorpanel);
}
void ui_positioninfo(char *msg)
{
	if (msg[0] != '\0') {
		char full[CURSOR_INFO_LENGTH] = "@: ";
		ui_write(cursorpanel, 1, strncat(full, msg, CURSOR_INFO_LENGTH - 7));
	} else {
		ui_write(cursorpanel, 1, "--");
	}
	ui_box(cursorpanel);
	wrefresh(cursorpanel);
}
void ui_modeinfo(char *msg)
{
	if (msg[0] != '\0') {
		char full[CURSOR_INFO_LENGTH] = "y: ";
		ui_write(cursorpanel, 2, strncat(full, msg, CURSOR_INFO_LENGTH - 6));
	} else {
		ui_write(cursorpanel, 2, "");
	}
	ui_box(cursorpanel);
	wrefresh(cursorpanel);
}

void ui_loginfo(char *msg)
{
	// rotate strings onto log
	for (int i=0; i < LOG_COUNT - 1; i++) {
		strcpy(logs[i], logs[i+1]);
	}
	strcpy(logs[LOG_COUNT - 1], msg);
	dmlog(msg);
}

void ui_update_cursorinfo(struct wld_map *map)
{
	int x = map->cursor->x;
	int y = map->cursor->y;

	// Get details about the tile they are on
	struct wld_tile *t = wld_gettileat(map, x, y);
	struct wld_tiletype *tt = wld_get_tiletype(t->type);
	if (t->is_visible) {
		struct wld_mob *m = wld_getmobat(map, x, y);
		if (m != NULL) {
			// TODO get "what" the mob is doing
			struct wld_mobtype *mt = wld_get_mobtype(m->type);
			ui_cursorinfo(mt->short_desc);
		} else {
			ui_cursorinfo(tt->short_desc);
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
	struct wld_tile *t = wld_gettileat(map, x, y);
	struct wld_tiletype *tt = wld_get_tiletype(t->type);
	if (t->is_visible)
		ui_positioninfo(tt->short_desc);
	else
		ui_positioninfo("");
}
void ui_update_logpanel(struct wld_map *map)
{
	for (int i=0; i < LOG_COUNT; i++) {
		ui_write(logpanel, i, logs[i]);
	}

	ui_box(logpanel);
	wrefresh(logpanel);
}
void ui_update_mobpanel(struct wld_map *map)
{
	// probably just need to do a shadowcast event each turn to get mobs in vision
	ui_write(mobpanel, 0, "MOBS:");
	ui_box(mobpanel);
	wrefresh(mobpanel);
}


///////////////////////////
// MAP EVENT SUBSCRIPTIONS

void map_on_cursormove(struct wld_map *map, int x, int y, int index)
{
	ui_update_cursorinfo(map);
}

void map_on_playermove(struct wld_map *map, struct wld_mob *player, int x, int y, int index)
{
	ui_update_positioninfo(map);
}

void map_on_mob_attack_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player)
{
	ui_loginfo("mob attacked player");
}

void map_on_mob_kill_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player)
{
	ui_loginfo("mob killed player");
	play_state = PS_GAMEOVER;
}

void map_on_player_attack_mob(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player)
{
	ui_loginfo("player attacked mob");
}

void map_on_player_kill_mob(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player)
{
	ui_loginfo("player killed mob");
}
// this runs in the player update loop and should take an input that triggers world updates
void ai_player_input(struct wld_mob* player)
{
	//nodelay(stdscr, true);
	escapedelay(false);
	bool listen = true;
	trigger_world = false;
	while (listen) {
		int key = getch();
		switch (player->target_mode) {
		case TMODE_NONE:
			switch (key) {
			// Player movement
			case KEY_ESC:
				play_state = PS_MENU;
				listen = false;
				break;
			case KEY_w:
				wld_queuemobmove(player, 0, -1);
				trigger_world = true;
				listen = false;
				break;
			case KEY_s:
				wld_queuemobmove(player, 0, 1);
				trigger_world = true;
				listen = false;
				break;
			case KEY_a:
				wld_queuemobmove(player, -1, 0);
				trigger_world = true;
				listen = false;
				break;
			case KEY_d:
				wld_queuemobmove(player, 1, 0);
				trigger_world = true;
				listen = false;
				break;
			case KEY_q:
				wld_queuemobmove(player, -1, -1);
				trigger_world = true;
				listen = false;
				break;
			case KEY_e:
				wld_queuemobmove(player, 1, -1);
				trigger_world = true;
				listen = false;
				break;
			case KEY_z:
				wld_queuemobmove(player, -1, 1);
				trigger_world = true;
				listen = false;
				break;
			case KEY_x:
				wld_queuemobmove(player, 1, 1);
				trigger_world = true;
				listen = false;
				break;
			// Player attack
			case KEY_y:
				// enter targeting mode for active weapon
				// TODO this is hardcoded to melee but needs to use equipped weapon's activation target
				player->target_mode = TMODE_MELEE;
				ui_loginfo("You draw your weapon for melee, range of 1.");
				ui_modeinfo("melee range 1");
				listen = false;
				break;
			}
			break;
		case TMODE_MELEE:
			switch (key) {
			case KEY_SPACE:
				trigger_world = ai_player_attack_melee(player);
				listen = false;
				break;
			case KEY_y:
			case KEY_ESC:
				player->target_mode = TMODE_NONE;
				ui_loginfo("You sheath your weapon.");
				ui_modeinfo("");
				listen = false;
				break;
			}
			break;
		} // eo target mode switch
		// always active
		switch (key) {
		// Cursor movement
		case KEY_8: // up
			wld_movecursor(current_map, 0, -1);
			listen = false;
			break;
		case KEY_2: // down
			wld_movecursor(current_map, 0, 1);
			listen = false;
			break;
		case KEY_4: // left
			wld_movecursor(current_map, -1, 0);
			listen = false;
			break;
		case KEY_6: // right
			wld_movecursor(current_map, 1, 0);
			listen = false;
			break;
		case KEY_7: // upleft
			wld_movecursor(current_map, -1, -1);
			listen = false;
			break;
		case KEY_9: // upright
			wld_movecursor(current_map, 1, -1);
			listen = false;
			break;
		case KEY_1: // downleft
			wld_movecursor(current_map, -1, 1);
			listen = false;
			break;
		case KEY_3: // downright
			wld_movecursor(current_map, 1, 1);
			listen = false;
			break;
		} // eo switch always
	} // eo while listen
	escapedelay(true);
	//nodelay(stdscr, false);
}


///////////////////////////
// SETUP PLAY

void ps_build_world()
{
	clear();

	// World is large indexed map to start
	current_map = wld_newmap(1);
	map_pad = newpad(current_map->rows * map_rows_scale, current_map->cols * map_cols_scale);
	current_map->on_cursormove = map_on_cursormove;
	current_map->on_playermove = map_on_playermove;
	current_map->on_mob_attack_player = map_on_mob_attack_player;
	current_map->on_mob_kill_player = map_on_mob_kill_player;
	current_map->player->ai_player_input = ai_player_input;
	current_map->on_player_attack_mob = map_on_player_attack_mob;
	current_map->on_player_kill_mob = map_on_player_kill_mob;
}
void ps_destroy_world()
{
	// THIS HAS NOT BEEN TESTED I CANT FIND DOCS ON HOW TO CLEAN UP PAD MEMORY
	delwin(map_pad);
	wld_delmap(current_map);

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
	int mobpanel_cols = 30;
	int mobpanel_rows = sy - logpanel_rows;
	ui_anchor_ur(mobpanel, mobpanel_rows, mobpanel_cols);
	ui_box(mobpanel);

	// make map the leftover width up to the mob panel
	ui_map_cols = sx - mobpanel_cols - 1;
	ui_map_rows = sy - logpanel_rows - 1;

	// mode bar on the bottom
	ui_anchor_bl(cursorpanel, logpanel_rows, sx - logpanel_cols - 1);
	ui_box(cursorpanel);
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
	cursorpanel = newwin(0, 0, 0, 0);
	logpanel = newwin(0, 0, 0, 0);
	mobpanel = newwin(0, 0, 0, 0);

	ps_layout_ui();

	ui_loginfo("You awake in darkness, then a light appears...");
}
void ps_destroy_ui()
{
	delwin(mobpanel);
	delwin(logpanel);
	delwin(cursorpanel);
}
// TODO function on translating a screen item to the pad item
//void translate_screenyx_mapyx(int sy, int, sx)
//{
//	// get difference between pad top,left and screen top,left
//	// subtract that from screen y,x to get pad y,x
//}
void ps_play_draw_onvisible(struct wld_mob* mob, int x, int y, double radius)
{
	// get the map tile at this position and draw it
	struct wld_tile *t = wld_gettileat(mob->map, x, y);
	t->is_visible = true;
	t->was_visible = true;
}
void ps_draw_tile(int r, int c, unsigned long cha, int colorpair, bool bold)
{
	wmove(map_pad, r * map_rows_scale, c * map_cols_scale); // pads by scaling out
	if (bold)
		wattrset(map_pad, COLOR_PAIR(colorpair) | A_BOLD);
	else
		wattrset(map_pad, COLOR_PAIR(colorpair));

	waddch(map_pad, cha); // pad
	// extra padding if we are scaling the columns to make it appear at a better ratio in the terminal
	if (map_cols_scale > 1 || map_rows_scale > 1) {
		for (int i=1; i < map_cols_scale; i++) {
			wmove(map_pad, r * map_rows_scale, c * map_cols_scale + i);
			wattrset(map_pad, COLOR_PAIR(colorpair));
			waddch(map_pad, ' '); // pad
			// TODO, stopped working correctly after shadowcaster
			//for (int j=1; j < map_rows_scale; j++) {
			//	wmove(map_pad, r * map_rows_scale + j, c * map_cols_scale + i);
			//	wattrset(map_pad, COLOR_PAIR(bg_only));
			//	waddch(map_pad, ' '); // pad
			//}
		}
	}
}
void ps_play_draw()
{
	// do not clear, it causes awful redraw
	//clear();
	//wclear(map_pad);

	// Draw tiles within the vision of the player
	wld_mobvision(current_map->player, ps_play_draw_onvisible);

	// Clear map without flutter
	for (int r=0; r < current_map->rows; r++) {
		wmove(map_pad, r, 0);
		wclrtoeol(map_pad);
		for (int c=0; c < current_map->cols; c++) {
			struct wld_tile *t = wld_gettileat(current_map, c, r);
			struct wld_tiletype *tt = wld_get_tiletype(t->type);

			if (t->is_visible) {
				struct draw_struct ds = wld_get_drawstruct(current_map, c, r);

				// if there is a dead mob on this then color it bloody
				struct wld_mob *m = wld_getmobat_index(current_map, t->map_index);
				if (m != NULL && m->is_dead)
					ps_draw_tile(r, c, ds.sprite, SCOLOR_BLOOD, false);
				else
					ps_draw_tile(r, c, ds.sprite, ds.colorpair, false);

			} else if (t->was_visible) {
				struct draw_struct ds = wld_get_memory_drawstruct(current_map, c, r);

				ps_draw_tile(r, c, ds.sprite, ds.colorpair, false);
			}
		}
	}

	// Draw cursor
	wmove(map_pad, current_map->cursor->y * map_rows_scale, current_map->cursor->x * map_cols_scale);
	wattrset(map_pad, COLOR_PAIR(SCOLOR_CURSOR));
	unsigned long ch = mvwinch(map_pad, current_map->cursor->y * map_rows_scale, current_map->cursor->x * map_cols_scale) & A_CHARTEXT;
	waddch(map_pad, ch);

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

	// UI constants (needs to be done in an event?)
	ui_update_logpanel(current_map);
	ui_update_mobpanel(current_map);
	ui_update_cursorinfo(current_map);
	ui_update_positioninfo(current_map);

	// ascii tools
	int y, x;
	getmaxyx(stdscr, y, x);
	move(ui_map_rows, 2);
	addstr("y: draw/sheath   i: inventory   z: rest   p: pickup");

	wrefresh(cursorpanel);
	wrefresh(mobpanel);
}
void ps_play_update()
{
	// depending on input change and trigger various updates

	// loop over map mobs and run their update routines
	for (int i=0; i < current_map->mobs_length; i++) {
		struct wld_mob *m = &current_map->mobs[i];
		if (m->is_player)
			wld_update_mob(m);
		else if (trigger_world)
			wld_update_mob(m);
	}

	// reset map elements
	// Clear map without flutter
	for (int r=0; r < current_map->rows; r++) {
		for (int c=0; c < current_map->cols; c++) {
			struct wld_tile *t = wld_gettileat(current_map, c, r);
			struct wld_tiletype *tt = wld_get_tiletype(t->type);
			t->is_visible = false;
		}
	}
}


///////////////////////////
// SETUP PLAY MENU

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


///////////////////////////
// PLAY MAIN LOOP

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

			// capture play input
			//ps_play_input();

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
			break;
		case PS_GAMEOVER:
			// draw world
			ps_play_draw();

			getch(); // wait until player does something then quit

			play_state = PS_END;
		case PS_END:
			ps_destroy_ui();
			ps_destroy_world();
			play_state = PS_EXIT;
			game_state = GS_TITLE;
			break;
		}
	}
}


///////////////////////////
// MAIN

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

