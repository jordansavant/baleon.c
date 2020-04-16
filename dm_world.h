// Contains all definitions for the world
#include "mt_rand.h"
#include "dm_algorithm.h"
#include "dm_debug.h"
#include <stdlib.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef bool
#define false 0
#define true 1
typedef int bool; // or #define bool int
#endif


///////////////////////////
// RAW DATA

int bg_colors[] = {
	COLOR_BLACK,  // 0 void
	COLOR_GREEN,  // 1 grass
	COLOR_WHITE,  // 2 snow
	COLOR_RED,    // 3 fire
	COLOR_BLUE,   // 4 water
	COLOR_CYAN,   // 5
	COLOR_YELLOW, // 6
	COLOR_MAGENTA, // 7
};
int fg_colors[] = {
	COLOR_BLACK,  // 0
	COLOR_GREEN,  // 1
	COLOR_WHITE,  // 2
	COLOR_RED,    // 3
	COLOR_BLUE,   // 4
	COLOR_CYAN,   // 5
	COLOR_YELLOW, // 6
	COLOR_MAGENTA, // 7
};
int color_base = 100;
int fg_size = 8;
int bg_size = 8;


///////////////////////////
// TILE STRUCTS

enum WLD_TILETYPE {
	TILE_VOID = 0,
	TILE_GRASS = 1,
	TILE_WATER = 2,
	TILE_TREE = 3,
	TILE_STONEWALL = 4,
	TILE_STONEFLOOR = 5,
};
struct wld_tiletype {
	int type;
	unsigned long sprite;
	int bg_color;
	int fg_color;
	unsigned long memory_sprite;
	int memory_bg_color;
	int memory_fg_color;
	bool is_block;
	char *short_desc;
};
struct wld_tile {
	int id; // position in tile list
	int map_x, map_y, map_index;
	enum WLD_TILETYPE type;
	struct wld_map *map;
	bool is_visible;
	bool was_visible;
	// on_enter, on_leave events
};
struct wld_tiletype *wld_tiletypes;


///////////////////////////
// MOB STRUCTS

enum WLD_MOBTYPE {
	MOB_VOID = 0,
	MOB_PLAYER = 1,
	MOB_BUGBEAR = 2,
};
enum WLD_MOB_STATE {
	MS_START,
	MS_DEAD,
	MS_WANDER,
	MS_COMBAT,
	MS_SLEEP,
	MS_HUNT,
};
struct wld_mobtype {
	int type;
	unsigned long sprite;
	int fg_color;
	unsigned long memory_sprite;
	int memory_fg_color;
	char *short_desc;
};
struct wld_mob
{
	int id; // positin in maps mob list
	int map_x, map_y, map_index; // position in map geo and index
	enum WLD_MOBTYPE type; // wld_mobtypes struct index
	struct wld_map *map;
	enum WLD_MOB_STATE state;
	void (*ai_wander)(struct wld_mob*);
	bool (*ai_detect_combat)(struct wld_mob*);
	void (*ai_decide_combat)(struct wld_mob*);
	int queuex, queuey, queuetarget;
	int health, maxhealth;
	bool is_player, is_dead;
};
struct wld_mobtype *wld_mobtypes;


///////////////////////////
// WORLD STRUCTS

struct wld_cursor {
	int x;
	int y;
	int index;
};
struct wld_map {
	int rows;
	int cols;
	int length;
	int depth;
	int *tile_map; // array of tile types
	struct wld_tile *tiles;
	unsigned int tiles_length;
	int *mob_map; // array of mob ids in mob listing
	struct wld_mob *mobs;
	unsigned int mobs_length;
	struct wld_mob *player;
	struct wld_cursor *cursor;

	// function pointers game subscribes to for events
	void (*on_cursormove)(struct wld_map*, int x, int y, int index);
	void (*on_mob_attack_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_mob_attack_player)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_mob_kill_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_mob_kill_player)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
};


///////////////////////////
// UTILITY METHODS

struct wld_tiletype* wld_get_tiletype(int id)
{
	return &wld_tiletypes[id];
}
struct wld_mobtype* wld_get_mobtype(int id)
{
	return &wld_mobtypes[id];
};
int wld_cpair(int tiletype, int mobtype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	if (mobtype == 0)
		return color_base + (tt->fg_color * fg_size + tt->bg_color);
	struct wld_mobtype *mt = wld_get_mobtype(mobtype);
	return color_base + (mt->fg_color * fg_size + tt->bg_color);
}
int wld_cpairmem(int tiletype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	return color_base + (tt->memory_fg_color * fg_size + tt->memory_bg_color);
}

int wld_cpair_bg(int tiletype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	return color_base + tt->bg_color; // no addition of foreground so it is black
}
int wld_calcindex(int x, int y, int cols)
{
	return y * cols + x;
}
int wld_calcx(int index, int cols)
{
	return index % cols;
}
int wld_calcy(int index, int cols)
{
	return index / cols;
}
bool wld_canmoveto(struct wld_map *map, int x, int y)
{
	// make sure its in bounds
	if (x >= map->cols || y >= map->rows || x < 0 || y < 0) {
		return false;
	}

	// current rules just check if a mob is ther
	int map_index = wld_calcindex(x, y, map->cols);
	int mob_id = map->mob_map[map_index];
	if (mob_id > -1)
		return false;

	int tile_id = map->tile_map[map_index];
	int tiletype = map->tiles[tile_id].type;
	struct wld_tiletype *tt = &wld_tiletypes[tiletype];
	if (tt->is_block)
		return false;

	return true;
}
void wld_queuemobmove(struct wld_mob *mob, int relx, int rely)
{
	mob->queuex += relx;
	mob->queuey += rely;
}
void wld_movemob(struct wld_mob *mob, int relx, int rely)
{
	// TODO speeds and things could come into play here
	// TODO need to make sure they do not diagonally move around corners
	int newx = mob->map_x + relx;
	int newy = mob->map_y + rely;

	// test if we can do this move
	if (wld_canmoveto(mob->map, newx, newy)) {
		int old_index = mob->map_index;
		int new_index = wld_calcindex(newx, newy, mob->map->cols);

		// update indexes
		mob->map->mob_map[new_index] = mob->id;
		mob->map->mob_map[old_index] = -1; // vacated

		// update position
		mob->map_index = new_index;
		mob->map_x = newx;
		mob->map_y = newy;
	}
}
void wld_movecursor(struct wld_map *map, int relx, int rely)
{
	int newx = map->cursor->x + relx;
	int newy = map->cursor->y + rely;
	if (newx >= 0 && newx < map->cols && newy >= 0 && newy < map->rows)
	{
		map->cursor->x = newx;
		map->cursor->y = newy;
		map->cursor->index = wld_calcindex(map->cursor->x, map->cursor->y, map->cols);
		map->on_cursormove(map, map->cursor->x, map->cursor->y, map->cursor->index);
	}
}
struct wld_tile* wld_gettileat(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	return &map->tiles[map->tile_map[index]];
}
struct wld_mob* wld_getmobat(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	int id = map->mob_map[index];
	if (id > -1)
		return &map->mobs[id];
	return NULL;
}
struct wld_mob* wld_getmobat_index(struct wld_map *map, int index)
{
	int id = map->mob_map[index];
	if (id > -1)
		return &map->mobs[id];
	return NULL;
}
void wld_mobvision(struct wld_mob *mob, void (*on_see)(struct wld_mob*, int, int, double))
{
	// todo get radius of mobs vision?
	struct wld_map* map = mob->map;
	bool wld_ss_isblocked(int x, int y)
	{
		struct wld_tile *t = wld_gettileat(map, x, y);
		struct wld_tiletype *tt = wld_get_tiletype(t->type);
		return tt->is_block;
	}
	void wld_ss_onvisible(int x, int y, double radius)
	{
		on_see(mob, x, y, radius);
	}
	dm_shadowcast(mob->map_x, mob->map_y, map->cols, map->rows, 20, wld_ss_isblocked, wld_ss_onvisible);
}
struct draw_struct {
	int colorpair;
	unsigned long sprite;
};
struct draw_struct wld_get_drawstruct(struct wld_map *map, int x, int y)
{
	struct wld_tile *t = wld_gettileat(map, x, y);
	struct wld_tiletype *tt = wld_get_tiletype(t->type);
	unsigned long cha = tt->sprite;
	int mob_id = map->mob_map[t->map_index];
	int colorpair;
	if (mob_id > -1) {
		struct wld_mob *m = wld_getmobat(map, x, y);
		struct wld_mobtype *mt = wld_get_mobtype(m->type);
		if (mt->sprite != ' ')
			cha = mt->sprite;
		colorpair = wld_cpair(t->type, m->type);
	} else {
		colorpair = wld_cpair(t->type, 0);
	}

	struct draw_struct ds = { colorpair, cha };
	return ds;
}
struct draw_struct wld_get_memory_drawstruct(struct wld_map *map, int x, int y)
{
	// memory we do not look at mob data
	struct wld_tile *t = wld_gettileat(map, x, y);
	struct wld_tiletype *tt = wld_get_tiletype(t->type);
	unsigned long cha = tt->memory_sprite;
	int colorpair = wld_cpairmem(t->type);

	struct draw_struct ds = { colorpair, cha };
	return ds;
}


///////////////////////////
// MOB AI

void ai_default_wander(struct wld_mob *mob)
{
	mob->queuex += 1;
}
bool ai_default_detect_combat(struct wld_mob *mob)
{
	// TODO
	return false && !mob->map->player->is_dead;
}
void ai_default_decide_combat(struct wld_mob *mob)
{
	// GET PLAYER (TODO in visible range, if can see, etc)
	int x = mob->map->player->map_x;
	int y = mob->map->player->map_y;
	int diffx = abs(x - mob->map_x);
	int diffy = abs(y - mob->map_y);
	// todo range and attack types?
	if (diffx > 1 || diffy > 1) {
		// move to target
		if (x < mob->map_x)
			mob->queuex += -1;
		else if (x > mob->map_x)
			mob->queuex += 1;
		if (y < mob->map_y)
			mob->queuey += -1;
		else if (y > mob->map_y)
			mob->queuey += 1;
	} else {
		// attack target
		mob->queuetarget = mob->map->player->map_index;
	}
}
void ai_mob_kill_mob(struct wld_mob *aggressor, struct wld_mob *defender)
{
	defender->state = MS_DEAD;
	defender->health = 0;
	defender->is_dead = true;
	// notify event
	if (!defender->is_player && aggressor->map->on_mob_kill_mob)
		aggressor->map->on_mob_kill_mob(aggressor->map, aggressor, defender);
	if (defender->is_player && aggressor->map->on_mob_kill_player)
		aggressor->map->on_mob_kill_player(aggressor->map, aggressor, defender);
}
void ai_mob_attack_mob(struct wld_mob *aggressor, struct wld_mob *defender)
{
	defender->health -= 34;

	// TODO pass damage amount, attack type, etc
	if (!defender->is_player && aggressor->map->on_mob_kill_mob)
		aggressor->map->on_mob_attack_mob(aggressor->map, aggressor, defender);
	if (defender->is_player && aggressor->map->on_mob_attack_player)
		aggressor->map->on_mob_attack_player(aggressor->map, aggressor, defender);

	if (defender->health <= 0)
		ai_mob_kill_mob(aggressor, defender);

}
void wld_update_mob(struct wld_mob *mob)
{
	// setup
	if (mob->state == MS_START) {
		mob->health = mob->maxhealth;
		mob->state = MS_WANDER;
		mob->is_dead = false;
	}

	// if player
	if (!mob->is_player) {
ai_rerun:
		switch (mob->state) {
		case MS_WANDER:
			if (mob->ai_detect_combat != NULL && mob->ai_detect_combat(mob)) {
				// enter combat
				mob->state = MS_COMBAT;
				goto ai_rerun;
			} else if (mob->ai_wander != NULL) {
				// wander
				mob->ai_wander(mob);
			}
			break;
		case MS_COMBAT:
			if (mob->ai_detect_combat != NULL && !mob->ai_detect_combat(mob)) {
				// exit combat
				mob->state = MS_WANDER;
			} else if(mob->ai_decide_combat != NULL) {
				// decide on what to do during combat
				// could be to move, could be to attack
				// calls other mob combat routines or movement routines
				mob->ai_decide_combat(mob);
			}
			break;
		}
	}

	// apply changes
	// TODO maybe this should be a dynamic array?
	wld_movemob(mob, mob->queuex, mob->queuey);
	mob->queuex = 0;
	mob->queuey = 0;

	if (mob->queuetarget > -1) {
		// attack enemy
		struct wld_mob* target = wld_getmobat_index(mob->map, mob->queuetarget);
		if (target != NULL) {
			ai_mob_attack_mob(mob, target);
		}
	}
}

///////////////////////////
// MAP INITIALIZATION

//int map[] = {
//	1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,
//	1, 2, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1,
//	1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 2, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1,
//	1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//};
int map_data[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 6, 1, 1, 1, 1, 3, 3, 3, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 3, 3, 3, 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2,
	2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 2, 2, 4, 2, 2, 2, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 3, 1, 1, 2, 2, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 3, 1, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 3, 3, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 3, 1, 3, 2, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 3, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 3, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 3, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 1, 1, 3, 3, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 3, 1, 1, 2, 1, 1, 1, 3, 3, 1, 1, 1, 1, 1, 3, 3, 1, 3, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 3, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 3, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 2, 1, 2,
	2, 1, 2, 1, 3, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 4, 2, 2, 2, 2, 1, 1, 1, 3, 1, 1, 3, 1, 1, 1, 2, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 3, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 3, 2, 3, 1, 1, 1, 1, 3, 1, 1, 1, 1, 3, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 3, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 5, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 1, 1, 1, 3, 1, 1, 3, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 3, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 3, 2, 3, 1, 1, 1, 1, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 1, 2, 2, 2, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 1, 2, 4, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 3, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 3, 1, 3, 1, 1, 3, 3, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2,
	2, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 4, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 3, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 3, 1, 3, 1, 1, 1, 1, 1, 2, 1, 2,
	2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 2, 2, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2,
	2, 1, 2, 2, 1, 3, 1, 1, 1, 1, 3, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 3, 1, 3, 1, 1, 2, 1, 2,
	2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 3, 2, 1, 2, 1, 2, 2, 2, 4, 2, 2, 2, 4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 3, 2, 2, 2, 1, 2, 3, 1, 1, 1, 3, 3, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 3, 3, 3, 1, 3, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 1, 2,
	2, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 2, 1, 1, 1, 1, 3, 1, 1, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 2,
	2, 1, 2, 2, 3, 3, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1, 3, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 3, 1, 1, 1, 1, 3, 2, 2, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 4, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 3, 1, 2, 2, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 3, 3, 3, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};

void wld_gentiles(struct wld_map *map)
{
	int *tile_map_array = (int*)malloc(map->length * sizeof(int));
	int *mob_map_array = (int*)malloc(map->length * sizeof(int));

	map->tiles = (struct wld_tile*)malloc(map->length * sizeof(struct wld_tile));
	map->tiles_length = map->length;

	// randomly fill map with grass, water, and trees
	for (int i=0; i < map->length; i++) {
		int map_data_type = map_data[i]; // global

		struct wld_tile *tile = &map->tiles[i];
		tile->id = i;
		tile->map = map;
		tile->map_x = wld_calcx(i, map->cols);
		tile->map_y = wld_calcy(i, map->cols);
		tile->map_index = i;
		tile->is_visible = false;
		tile->was_visible = false;

		switch (map_data_type) {
		default:
		case 1:
			tile->type = TILE_STONEFLOOR;
			break;
		case 2:
			tile->type = TILE_STONEWALL;
			break;
		case 3:
			tile->type = TILE_WATER;
			break;
		}

		tile_map_array[i] = i; // set tile map to this tile id
		mob_map_array[i] = -1; // initialize to -1 for "no mob"
	}
	map->tile_map = tile_map_array;
	map->mob_map = mob_map_array;
}
void wld_genmobs(struct wld_map *map)
{
	// hardcoded to 3 mobs for now
	int mob_count = 2;
	map->mobs = (struct wld_mob*)malloc(mob_count * sizeof(struct wld_mob));
	map->mobs_length = mob_count;

	// setup mobs in mob map
	for (int i=0; i < mob_count; i++) {
		// create mob
		struct wld_mob *mob = &map->mobs[i];
		// create reference to parent map
		mob->id = i;
		mob->map = map;
		mob->state = MS_START;
		mob->queuex = 0;
		mob->queuey = 0;
		mob->queuetarget = -1;
		mob->maxhealth = 100; // TODO define based on things
		mob->ai_wander = NULL;

		// first mob is player
		if (i == 0) {
			// hardcoded to center of map until we get a heuristic for map entrance
			// TBD xogeni!
			mob->map_x = map->cols / 2;
			mob->map_y = map->rows / 2;
			mob->map_index = wld_calcindex(mob->map_x, mob->map_y, map->cols);
			mob->type = MOB_PLAYER;
			mob->is_player = true;
			map->player = mob; // assign to map specifically

			// set cursor nearby
			map->cursor->x = mob->map_x + 2;
			map->cursor->y = mob->map_y;
			map->cursor->index = wld_calcindex(map->cursor->x, map->cursor->y, map->cols);
		}else {
			// TODO this can conflict with the player if we are not careful
			// hardcoded diagonal positions
			mob->map_x = 38;
			mob->map_y = 36;
			mob->map_index = wld_calcindex(mob->map_x, mob->map_y, map->cols);
			mob->type = MOB_BUGBEAR;
			mob->is_player = false;
			mob->ai_wander = ai_default_wander;
			mob->ai_detect_combat = ai_default_detect_combat;
			mob->ai_decide_combat = ai_default_decide_combat;
		}

		// set mob's id into the mob map
		map->mob_map[mob->map_index] = i;
	}
}
struct wld_map* wld_newmap(int depth)
{
	struct wld_map *map = (struct wld_map*)malloc(sizeof(struct wld_map));

	// data
	map->rows = 56;
	map->cols = 56;
	map->length = 56 * 56;
	map->depth = depth;
	map->tile_map = NULL;
	map->mob_map = NULL;
	map->tiles = NULL;
	map->mobs = NULL;
	map->player = NULL;
	map->cursor = (struct wld_cursor*)malloc(sizeof(struct wld_cursor));
	map->cursor->x = 0;
	map->cursor->y = 0;
	map->cursor->index = 0;

	// function events
	map->on_cursormove = NULL;
	map->on_mob_attack_mob = NULL;
	map->on_mob_attack_player = NULL;
	map->on_mob_kill_mob = NULL;
	map->on_mob_kill_player = NULL;

	// populate tiles
	wld_gentiles(map);

	// build an populate map with mobs
	wld_genmobs(map);

	return map;
}
void wld_delmap(struct wld_map *map)
{
	free(map->cursor);
	free(map->mobs);
	free(map->mob_map);
	free(map->tiles);
	free(map->tile_map);
	free(map);
}


///////////////////////////
// WORLD INITALIZATION

void wld_setup()
{
	// build color maps
	for (int i=0; i < bg_size; i++) {
		for (int j=0; j < fg_size; j++) {
			int index = j * fg_size + i;
			int colorpair_id = color_base + index;
			init_pair(colorpair_id, fg_colors[j], bg_colors[i]);
		}
	}

	// copy tiletypes into malloc
	struct wld_tiletype tts[] = {
		{ TILE_VOID,            ' ', 0, 0, ' ', 0, 0, false, "" }, // 0
		{ TILE_GRASS,           '"', 0, 1, '"', 0, 0, false, "A small tuft of grass" }, // 1
		{ TILE_WATER,           ' ', 4, 2, ' ', 0, 0, false, "A pool of water glistens" }, // 2
		{ TILE_TREE,            'T', 0, 1, 'T', 0, 0, true,  "A large tree" }, // 3
		{ TILE_STONEWALL,       '#', 2, 2, '#', 5, 5, true,  "A stone wall" }, // 4
		{ TILE_STONEFLOOR,      '.', 0, 2, '.', 0, 5, false, "Stone floor" }, // 5
	};
	wld_tiletypes = (struct wld_tiletype*)malloc(ARRAY_SIZE(tts) * sizeof(struct wld_tiletype));
	for (int i=0; i<ARRAY_SIZE(tts); i++) {
		wld_tiletypes[i].type = tts[i].type;
		wld_tiletypes[i].sprite = tts[i].sprite;
		wld_tiletypes[i].fg_color = tts[i].fg_color;
		wld_tiletypes[i].bg_color = tts[i].bg_color;
		wld_tiletypes[i].memory_sprite = tts[i].memory_sprite;
		wld_tiletypes[i].memory_fg_color = tts[i].memory_fg_color;
		wld_tiletypes[i].memory_bg_color = tts[i].memory_bg_color;
		wld_tiletypes[i].is_block = tts[i].is_block;
		wld_tiletypes[i].short_desc = tts[i].short_desc;
	}

	// copy mob types into malloc
	struct wld_mobtype mts [] = {
		{ MOB_VOID,	' ', 0, ' ', 0, "" },
		{ MOB_PLAYER,	'@', 7, '@', 2, "You" },
		{ MOB_BUGBEAR,	'b', 3, 'b', 2, "A bugbear" },
	};
	wld_mobtypes = (struct wld_mobtype*)malloc(ARRAY_SIZE(mts) * sizeof(struct wld_mobtype));
	for (int i=0; i<ARRAY_SIZE(mts); i++) {
		wld_mobtypes[i].type = mts[i].type;
		wld_mobtypes[i].sprite = mts[i].sprite;
		wld_mobtypes[i].fg_color = mts[i].fg_color;
		wld_mobtypes[i].short_desc = mts[i].short_desc;
	}
}
void wld_teardown()
{
	free(wld_mobtypes);
	free(wld_tiletypes);
}
