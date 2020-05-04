#ifndef GAME
#define GAME

#include <ncurses.h>
#include "dm_world.h"

#define KEY_RETURN	10
#define KEY_ESC		27
#define KEY_SPACE	32
#define KEY_TAB		9

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

enum GAME_STATE
{
	GS_START,
	GS_INTRO,
	GS_TITLE,
	GS_NEWGAME,
	GS_EXIT
};
enum PLAY_STATE {
	PS_START,
	PS_PLAY,
	PS_MENU,
	PS_MAPCHANGE,
	PS_GAMEOVER,
	PS_WIN,
	PS_END, // do not call exit directly
	PS_EXIT,
};
enum USE_TYPE {
	USE_NONE,
	USE_ITEM,
	USE_SPELL, // TBD
};

// UTILS
void skip_delay(double wait_s);
void escapedelay(bool on);
void on_term_resize(int dummy);

// GAME SETUP
bool g_setup();
void g_teardown();
void g_newgame();
int game_main();

// INTRO
void g_intro();

// TITLE
void g_title_onquit(char *label);
void g_title_onintro(char *label);
void g_title_onnewgame(char *label);
void g_title_onundefined(char *label);
void g_title();

// UI
void ui_clear_visible_mobs(bool full);
void ui_next_visible_mob();
void ui_box_color(WINDOW* win, int colorpair);
void ui_box(WINDOW* win);
void ui_clear(WINDOW *win, int row);
void ui_printchar(WINDOW *win, int row, int col, unsigned long ch);
void ui_write_rc(WINDOW *win, int row, int col, char *msg);
void ui_write(WINDOW *win, int row, char *msg);
void ui_print(WINDOW *win, int buffer_size, int row, int col, char *msg);
void ui_printf(WINDOW *win, int buffer_size, int row, int col, char *format, ...);

void ui_anchor_ur(WINDOW* win, int rows, int cols);
void ui_anchor_ul(WINDOW *win, int rows, int cols);
void ui_anchor_br(WINDOW *win, int rows, int cols);
void ui_anchor_bl(WINDOW *win, int rows, int cols, int yoff, int xoff);
void ui_anchor_center(WINDOW *win, int rows, int cols, int yoff, int xoff);
void ui_clear_win(WINDOW *win);
void ui_cursorinfo(char *msg);
void ui_positioninfo(char *msg);
void ui_modeinfo(char *msg);
void ui_loginfo(char *msg);
void ui_loginfo_s(char *msg, char *msg2);
void ui_loginfo_i(char *msg, int i);
void ui_loginfo_is(char *msg, int i, char *msg2);
void ui_loginfo_si(char *msg, char *msg2, int i);
void ui_loginfo_ss(char *msg, char *msg2, char *msg3);
void ui_loginfo_ssi(char *msg, char *msg2, char *msg3, int i);
void ui_update_cmdpanel(struct wld_map *map);
void ui_update_cursorinfo(struct wld_map *map);
void ui_update_positioninfo(struct wld_map *map);
void ui_update_logpanel(struct wld_map *map);
void ui_update_mobpanel(struct wld_map *map);
void ui_update_inventorypanel(struct wld_map *map);
void ui_update_usepanel(struct wld_map *map);
void ui_unset_use();
void ui_set_use_item(struct wld_item* item, int item_slot);
void ui_use_item_select(struct wld_mob* player, int item_slot);

// MAP EVENTS
void map_on_effect(struct wld_map *map, struct wld_vfx *effect);
void map_on_player_transition(struct wld_map *map, struct wld_mob *player, bool forward);
void map_on_cursormove(struct wld_map *map, int x, int y, int index);
void map_on_playermove(struct wld_map *map, struct wld_mob *player, int x, int y, int index);
void map_on_mob_heal(struct wld_map *map, struct wld_mob* mob, int amt, struct wld_item* item);
void map_on_mob_attack_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player, int dmg, struct wld_item* item);
void map_on_mob_whiff_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player, struct wld_item* item);
void map_on_mob_kill_player(struct wld_map *map, struct wld_mob* aggressor, struct wld_mob* player, struct wld_item* item);
void map_on_player_heal(struct wld_map *map, struct wld_mob* player, int amt, struct wld_item* item);
void map_on_player_attack_mob(struct wld_map *map, struct wld_mob* player, struct wld_mob* defender, int dmg, struct wld_item* item);
void map_on_player_whiff(struct wld_map *map, struct wld_mob* player, struct wld_item* item);
void map_on_player_whiff_mob(struct wld_map *map, struct wld_mob* player, struct wld_mob* defender, struct wld_item* item);
void map_on_player_kill_mob(struct wld_map *map, struct wld_mob* player, struct wld_mob* defender, struct wld_item* item);
void map_on_player_pickup_item(struct wld_map *map, struct wld_mob* player, struct wld_item* item);
void map_on_player_pickup_item_fail(struct wld_map *map, struct wld_mob* player, struct wld_item* item);
void map_on_player_drop_item(struct wld_map *map, struct wld_mob* player, struct wld_item* item);
void map_on_player_drop_item_fail(struct wld_map *map, struct wld_mob* player, struct wld_item* item);

// PLAY MODE
void ai_player_input(struct wld_mob* player);
void ps_on_mapchange();
void ps_build_world();
void ps_destroy_world();
void ps_layout_ui();
void ps_reset_ui();
void ps_build_ui();
void ps_destroy_ui();
void ps_play_draw_onvisible(struct wld_mob* mob, int x, int y, double radius);
void ps_draw_tile(int r, int c, unsigned long cha, int colorpair, unsigned long cha2, int cpair2, bool bold);
void ps_refresh_map_pad();
void ps_play_draw();
void ps_play_update();

// PLAY MENU
void ps_menu_draw();
void ps_menu_input();

// LOGGER LINKS
void wld_log(char *msg);
void wld_log_s(char *msg, char *s);
void wld_log_ss(char *msg, char *s, char *s2);
void wld_log_ms(char* msg, struct wld_mob* mob);
void wld_log_it(char* msg, struct wld_item* item);
void wld_log_ts(char* msg, struct wld_tile* tile);

#endif
