// Contains all definitions for the world
#include "mt_rand.h"
#include "dm_algorithm.h"
#include "dm_debug.h"
#include <stdlib.h>
#include <ncurses.h>
#include <math.h>
#include "dm_world.h"
#include "dm_dungeon.h"


///////////////////////////
// RAW DATA

#define MALLOC_ITEM_SIZE 10
#define MALLOC_MOB_SIZE 10
#define WLD_COLOR_BASE 100
#define WLD_COLOR_COUNT 8
int curses_colors[] = {
	COLOR_BLACK,   // 0
	COLOR_GREEN,   // 1
	COLOR_WHITE,   // 2
	COLOR_RED,     // 3
	COLOR_BLUE,    // 4
	COLOR_CYAN,    // 5
	COLOR_YELLOW,  // 6
	COLOR_MAGENTA, // 7
};

struct wld_tiletype *wld_tiletypes;
struct wld_mobtype *wld_mobtypes;
struct wld_itemtype *wld_itemtypes;



///////////////////////////
// WORLD METHODS START

struct wld_world* wld_new_world(int seed, int count)
{
	dm_seed(seed);
	struct wld_world* world = (struct wld_world*)malloc(sizeof(struct wld_world));
	world->seed;
	world->maps_length = count;
	world->maps = (struct wld_map**)malloc(world->maps_length * sizeof(struct wld_map*));

	struct dng_dungeon* dungeon = dng_gendungeon(seed, world->maps_length);

	for (int i=0; i < dungeon->maps_length; i++) {
		// convert dungeon maps to game maps
		// iterate the cellmap
		struct dng_cellmap* cellmap = dungeon->maps[i];
		struct wld_map* map = wld_new_map(i, cellmap->difficulty, cellmap->width, cellmap->height);
		map->world = world;
		map->is_first_map = (i == 0);

		// populate
		wld_generate_tiles(map, cellmap);
		wld_generate_mobs(map, cellmap);
		wld_generate_items(map, cellmap);

		world->maps[i] = map;
	}

	dng_deldungeon(dungeon);

	return world;
}

void wld_delete_world(struct wld_world* world)
{
	for (int i=0; i < world->maps_length; i++) {
		wld_delete_map(world->maps[i]);
	}
	free(world->maps);
	free(world);
}

void wld_transition_player(struct wld_world* world, struct wld_map* from_map, struct wld_map* to_map, bool at_entrance)
{
	// remove player instance from current map and move him to the next map
	struct wld_mob* mob = from_map->player;
	wld_map_remove_mob(from_map, from_map->player);
	if (at_entrance)
		wld_map_add_mob_at_entrance(to_map, mob);
	else
		wld_map_add_mob_at_exit(to_map, mob);
	to_map->player = mob;
	to_map->cursor->x = mob->map_x + 1;
	to_map->cursor->y = mob->map_y + 1;
}

void wld_setup()
{

	// build color maps of all bg/fg color pairs
	for (int i=0; i < WLD_COLOR_COUNT; i++) {
		for (int j=0; j < WLD_COLOR_COUNT; j++) {
			int index = j * WLD_COLOR_COUNT + i;
			int colorpair_id = WLD_COLOR_BASE + index;
			init_pair(colorpair_id, curses_colors[j], curses_colors[i]);
		}
	}

	// copy tiletypes into malloc
	struct wld_tiletype tts[] = { //     bg		 fg		  membg       memfg
		{ TILE_VOID,            ' ', WCLR_BLACK, WCLR_BLACK, ' ', WCLR_BLACK, WCLR_BLACK, false, "" },
		{ TILE_GRASS,           '"', WCLR_BLACK, WCLR_GREEN, '"', WCLR_BLACK, WCLR_BLUE,  false, "a small tuft of grass" },
		{ TILE_WATER,           '~', WCLR_BLUE,  WCLR_BLUE,  '~', WCLR_BLACK, WCLR_BLUE,  false, "a pool of water glistens" },
		{ TILE_TREE,            'T', WCLR_BLACK, WCLR_GREEN, 'T', WCLR_BLACK, WCLR_BLUE,  false, "a large tree" },
		{ TILE_STONEWALL,       '#', WCLR_WHITE, WCLR_WHITE, '#', WCLR_BLACK, WCLR_BLUE,  true,  "a rough stone wall" },
		{ TILE_STONEFLOOR,	'.', WCLR_BLACK, WCLR_WHITE, '.', WCLR_BLACK, WCLR_BLUE,  false, "a rough stone floor" },
		{ TILE_ENTRANCE,	'>', WCLR_BLACK, WCLR_CYAN,  '>', WCLR_BLACK, WCLR_CYAN,  false, "the entrance back up" },
		{ TILE_EXIT,		'<', WCLR_BLACK, WCLR_CYAN,  '<', WCLR_BLACK, WCLR_CYAN,  false, "an exit further down" },
		{ TILE_STONEDOOR,       '+', WCLR_BLACK, WCLR_WHITE, '+', WCLR_BLACK, WCLR_BLUE,  false, "an old stone door" },
		{ TILE_DEEPWATER,       ' ', WCLR_BLACK, WCLR_BLACK, '~', WCLR_BLACK, WCLR_BLUE,  false, "an old stone door" },
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
		// hp, sprite, color, desc, title
		{ MOB_VOID,	  0, ' ', WCLR_BLACK,   "" },
		{ MOB_PLAYER,	100, '@', WCLR_MAGENTA, "yourself", "You"},
		{ MOB_JACKAL,	 35, 'j', WCLR_RED,     "a small jackal", "jackal" },
		{ MOB_RAT,	 5, 'r', WCLR_RED,     "a hideous rat", "rat" },
	};
	wld_mobtypes = (struct wld_mobtype*)malloc(ARRAY_SIZE(mts) * sizeof(struct wld_mobtype));
	for (int i=0; i<ARRAY_SIZE(mts); i++) {
		wld_mobtypes[i].type = mts[i].type;
		wld_mobtypes[i].base_health = mts[i].base_health;
		wld_mobtypes[i].sprite = mts[i].sprite;
		wld_mobtypes[i].fg_color = mts[i].fg_color;
		wld_mobtypes[i].short_desc = mts[i].short_desc;
		wld_mobtypes[i].title = mts[i].title;
	}

	// copy item types into malloc
	struct wld_itemtype its [] = {
		{
			ITEM_VOID,
			' ',
			WCLR_BLACK,
			false,false,false, // weapon, armor, key
			"",
			"",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			false, 0, // uses
			"",
			"",
			"",
			""
		},
		{
			ITEM_POTION_MINOR_HEAL,
			';',
			WCLR_YELLOW,
			false,false,false, // weapon, armor, key
			"a potion of minor healing",
			"minor healing potion",
			itm_drink_minorhealth,
			itm_target_ranged_los,
			itm_can_use_ranged_los,
			itm_use_ranged_los,
			itm_hit_minorhealth,
			5,
			true,
			1,
			"quaff",
			"throw",
			/////////////////////////////////////////////////////////
			"The glass of the potion is warm to the touch, its",
			"properties should heal a small amount."
		},
		{
			ITEM_WEAPON_SHORTSWORD,
			'/',
			WCLR_YELLOW,
			true,false,false, // weapon, armor, key
			"a shortsword",
			"shortsword",
			NULL,
			itm_target_melee,
			itm_can_use_melee,
			itm_use_melee,
			itm_hit_melee_swordstyle,
			1,
			false, 0, // uses
			"",
			"strike",
			/////////////////////////////////////////////////////////
			"Though short, its sharp point could plunge deeply into",
			"a soft skinned enemy."
		},
		{
			ITEM_WEAPON_SHORTBOW,
			')',
			WCLR_YELLOW,
			true,false,false, // weapon, armor, key
			"a shortbow",
			"shortbow",
			NULL,
			itm_target_ranged_los,
			itm_can_use_ranged_los,
			itm_use_ranged_los,
			itm_hit_ranged_los_bowstyle,
			10,
			false, 0, // uses
			"",
			"shoot",
			/////////////////////////////////////////////////////////
			"Its string has been worn but the wood is strong, this",
			"small bow could fell small creatures"
		},
		{
			ITEM_SCROLL_FIREBOMB,
			'=',
			WCLR_YELLOW,
			false,false,false, // weapon, armor, key
			"a scroll of firebomb",
			"scroll of firebomb",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			true,1,//uses
			"",
			"",
			/////////////////////////////////////////////////////////
			"Runic art covers the parchment surface showing a",
			"large swathe of fire."
		},
		{
			ITEM_ARMOR_LEATHER,
			'M',
			WCLR_YELLOW,
			false,true,false, // weapon, armor, key
			"a set of leather armor",
			"leather armor",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			false,0, // uses
			"",
			"",
			/////////////////////////////////////////////////////////
			"Humble but sturdy this set of leather armor is a rogue's",
			"favorite friend."
		},
		{
			ITEM_KEY_BASIC,
			'*',
			WCLR_YELLOW,
			false,false,true, // weapon, armor, key
			"a small bronze key",
			"bronze key",
			NULL,
			itm_target_key,
			itm_can_use_key,
			itm_use_key,
			itm_hit_key,
			1,
			true,1, // uses (key uses increment on failure)
			"",
			"use",
			/////////////////////////////////////////////////////////
			"This lost, tarnished bronze key may fit a lock to a",
			"nearby door or chest."
		},
	};
	wld_itemtypes = (struct wld_itemtype*)malloc(ARRAY_SIZE(its) * sizeof(struct wld_itemtype));
	for (int i=0; i<ARRAY_SIZE(its); i++) {
		wld_itemtypes[i].type = its[i].type;
		wld_itemtypes[i].sprite = its[i].sprite;
		wld_itemtypes[i].fg_color = its[i].fg_color;
		wld_itemtypes[i].is_weq = its[i].is_weq;
		wld_itemtypes[i].is_aeq = its[i].is_aeq;
		wld_itemtypes[i].is_key = its[i].is_key;
		wld_itemtypes[i].short_desc = its[i].short_desc;
		wld_itemtypes[i].title = its[i].title;
		wld_itemtypes[i].fn_drink = its[i].fn_drink;
		wld_itemtypes[i].fn_target = its[i].fn_target;
		wld_itemtypes[i].fn_can_use = its[i].fn_can_use;
		wld_itemtypes[i].fn_use = its[i].fn_use;
		wld_itemtypes[i].fn_hit = its[i].fn_hit;
		wld_itemtypes[i].base_range = its[i].base_range;
		wld_itemtypes[i].has_uses = its[i].has_uses;
		wld_itemtypes[i].base_uses = its[i].base_uses;
		wld_itemtypes[i].drink_label = its[i].drink_label;
		wld_itemtypes[i].use_label = its[i].use_label;
		wld_itemtypes[i].use_text_1 = its[i].use_text_1;
		wld_itemtypes[i].use_text_2 = its[i].use_text_2;
	}
}

void wld_teardown()
{
	free(wld_itemtypes);
	free(wld_mobtypes);
	free(wld_tiletypes);
}

// WORLD METHODS END
///////////////////////////




///////////////////////////
// MAP INITIALIZATION

void wld_generate_tiles(struct wld_map *map, struct dng_cellmap* cellmap)
{
	int *tile_map_array = (int*)malloc(map->length * sizeof(int));
	int *mob_map_array = (int*)malloc(map->length * sizeof(int));
	int *item_map_array = (int*)malloc(map->length * sizeof(int));

	map->tiles = (struct wld_tile*)malloc(map->length * sizeof(struct wld_tile));
	map->tiles_length = map->length;

	for (int r = 0; r < cellmap->height; r++) { // rows
		for (int c=0; c < cellmap->width; c++){ // cols
			// get cell from map
			int index = r * cellmap->width + c;
			struct dng_cell *cell = cellmap->cells[index];

			// get tile
			struct wld_tile *tile = &map->tiles[index];
			tile->id = index;
			tile->map = map;
			tile->map_x = wld_calcx(index, map->cols);
			tile->map_y = wld_calcy(index, map->cols);
			tile->map_index = index;
			tile->map = map;
			tile->is_visible = false;
			tile->was_visible = false;
			tile->is_blocked = false;
			tile->is_door = false;
			tile->is_door_open = false;
			tile->is_door_locked = false;
			tile->door_lock_id = -1;
			tile->dead_mob_type = NULL;
			tile->on_mob_enter = NULL;
			tile->dm_ss_id = 0;

			// TODO more
			if (cell->is_wall) {
				tile->type_id = TILE_STONEWALL;
				tile->type = &wld_tiletypes[TILE_STONEWALL];
			} else if (cell->is_door) {
				tile->type_id = TILE_STONEDOOR;
				tile->type = &wld_tiletypes[TILE_STONEDOOR];
				tile->is_blocked = true; // cell->is_door_locked; // locked based on cell door quality
				tile->is_door = true;
				tile->is_door_open = false;
				tile->is_door_locked = cell->is_door_locked;
				tile->door_lock_id = cell->door_lock_id;
			} else if (cell->is_exit_transition || cell->is_entrance_transition) {
				if (cell->is_entrance_transition) {
					tile->type_id = TILE_ENTRANCE;
					tile->type = &wld_tiletypes[TILE_ENTRANCE];
					tile->on_mob_enter = wld_tile_on_mob_enter_entrance;
					map->entrance_tile = tile;
				}
				if (cell->is_exit_transition) {
					tile->type_id = TILE_EXIT;
					tile->type = &wld_tiletypes[TILE_EXIT];
					tile->on_mob_enter = wld_tile_on_mob_enter_exit;
					map->exit_tile = tile;
				}
			} else {
				if (cell->floor_style == DNG_STYLE_GRASS) {
					tile->type_id = TILE_GRASS;
					tile->type = &wld_tiletypes[TILE_GRASS];
				} else if(cell->floor_style == DNG_STYLE_WATER) {
					tile->type_id = TILE_WATER;
					tile->type = &wld_tiletypes[TILE_WATER];
				} else if(cell->floor_style == DNG_STYLE_DEEPWATER) {
					tile->type_id = TILE_DEEPWATER;
					tile->type = &wld_tiletypes[TILE_DEEPWATER];
				} else {
					tile->type_id = TILE_STONEFLOOR;
					tile->type = &wld_tiletypes[TILE_STONEFLOOR];
				}
			}

			tile_map_array[index] = tile->id; // set tile map to this tile id
			mob_map_array[index] = -1; // initialize to -1 for "no mob"
			item_map_array[index] = -1; // no item
		}
	}

	map->tile_map = tile_map_array;
	map->mob_map = mob_map_array;
	map->item_map = item_map_array;
}

void wld_init_mob(struct wld_mob *mob, enum WLD_MOBTYPE type)
{
	mob->state = MS_START;
	mob->queue_x = 0;
	mob->queue_y = 0;
	mob->basevision = 20; // TODO define based on things
	mob->vision = mob->basevision;
	mob->ai_wander = NULL;
	mob->ai_is_hostile = NULL;
	mob->ai_detect_combat = NULL;
	mob->ai_decide_combat = NULL;
	mob->ai_player_input = NULL;
	mob->cursor_target_index = -1;
	mob->mode = MODE_PLAY;
	mob->target_mode = TMODE_NONE;
	mob->is_dead = false;
	mob->target_x = 0;
	mob->target_y = 0;
	mob->active_item = NULL;
	mob->is_destroy_queued = false;
	mob->type_id = type;
	mob->type = &wld_mobtypes[type];

	// create inventory (pointers to malloc items)
	mob->inventory = (struct wld_item**)malloc(INVENTORY_SIZE * sizeof(struct wld_item*));
	for (int j=0; j < INVENTORY_SIZE; j++) {
		mob->inventory[j] = NULL;
	}

	// stats TODO roll these in some sort of character rolling menu?
	switch (type) {
	case MOB_PLAYER:
		mob->stat_strength = dm_randii(5, 16);
		mob->stat_dexterity = dm_randii(5, 16);
		mob->stat_constitution = dm_randii(5, 16);

		// lets give him some items for playtesting
		mob->inventory[0] = (struct wld_item*)malloc(sizeof(struct wld_item));
		wld_init_item(mob->inventory[0], ITEM_WEAPON_SHORTSWORD);
		break;
	default:
		mob->stat_strength = dm_randii(3, 16);
		mob->stat_dexterity = dm_randii(3, 16);
		mob->stat_constitution = dm_randii(3, 16);
		break;
	}
	double conf = (double)mob->stat_constitution / (double)STAT_CON_BASE;
	mob->health = conf * mob->type->base_health;
	mob->maxhealth = conf * mob->type->base_health;

}

void wld_generate_mobs(struct wld_map *map, struct dng_cellmap* cellmap)
{
	map->mobs = NULL;
	map->mobs_length = 0;

	void gen_jackal(struct wld_map* map, int c, int r) {
		struct wld_mob *mob = (struct wld_mob*)malloc(sizeof(struct wld_mob));
		wld_init_mob(mob, MOB_JACKAL);
		wld_map_new_mob(map, mob, c, r);
		mob->is_player = false;
		mob->ai_wander = ai_default_wander;
		mob->ai_is_hostile = ai_default_is_hostile;
		mob->ai_detect_combat = ai_default_detect_combat;
		mob->ai_decide_combat = ai_default_decide_combat;
	}
	void gen_rat(struct wld_map* map, int c, int r) {
		struct wld_mob *mob = (struct wld_mob*)malloc(sizeof(struct wld_mob));
		wld_init_mob(mob, MOB_RAT);
		wld_map_new_mob(map, mob, c, r);
		mob->is_player = false;
		mob->ai_wander = ai_default_wander;
		mob->ai_is_hostile = ai_default_is_hostile;
		mob->ai_detect_combat = ai_default_detect_combat;
		mob->ai_decide_combat = ai_default_decide_combat;
	}

	for (int r = 0; r < cellmap->height; r++) { // rows
		for (int c=0; c < cellmap->width; c++){ // cols
			// get cell from map
			int index = r * cellmap->width + c;
			struct dng_cell *cell = cellmap->cells[index];

			// Spawn player and first dungeon entrance
			if (cell->is_entrance_transition && map->is_first_map) {
				struct wld_mob *mob = (struct wld_mob*)malloc(sizeof(struct wld_mob));
				wld_init_mob(mob, MOB_PLAYER);
				wld_map_new_mob(map, mob, c, r);
				mob->is_player = true;
				map->player = mob; // assign to map specifically

				// set cursor nearby
				map->cursor->x = mob->map_x + 2;
				map->cursor->y = mob->map_y;
				map->cursor->index = wld_calcindex(map->cursor->x, map->cursor->y, map->cols);

			} else if (cell->has_mob) {
				switch (cell->mob_style) {
					case DNG_MOB_STYLE_HOARD:
						gen_rat(map, c, r);
						break;
					default:
						if (dm_chance(1,4)) {
							gen_jackal(map, c, r);
						} else {
							gen_rat(map, c, r);
						}
						break;
				}
			}
		}
	}
}

void wld_init_item(struct wld_item* item, enum WLD_ITEMTYPE type)
{
	item->type_id = type;
	item->type = &wld_itemtypes[type];
	item->has_dropped = false;
	item->uses = wld_itemtypes[type].base_uses;
	item->id = -1;
	item->map_index = -1;
	item->map_x = 0;
	item->map_y = 0;
	item->key_id = -1;
	item->map_found -1;
}

void wld_generate_items(struct wld_map *map, struct dng_cellmap* cellmap)
{
	map->items = NULL;
	map->items_length = 0;

	// TODO we need to work on assigning items to players, mobs etc
	for (int r = 0; r < cellmap->height; r++) { // rows
		for (int c=0; c < cellmap->width; c++){ // cols
			// get cell from map
			int index = r * cellmap->width + c;
			struct dng_cell *cell = cellmap->cells[index];

			if (cell->has_item) {
				struct wld_item* item = (struct wld_item*)malloc(sizeof(struct wld_item));
				switch (cell->item_type) {
				case DNG_ITEM_LOOT:
					switch (dm_randii(0, 4)) {
					case 0:
						wld_init_item(item, ITEM_WEAPON_SHORTSWORD);
						wld_map_new_item(map, item, c, r);
						break;
					case 1:
						wld_init_item(item, ITEM_WEAPON_SHORTBOW);
						wld_map_new_item(map, item, c, r);
						break;
					case 2:
						wld_init_item(item, ITEM_POTION_MINOR_HEAL);
						wld_map_new_item(map, item, c, r);
						break;
					case 3:
						wld_init_item(item, ITEM_ARMOR_LEATHER);
						wld_map_new_item(map, item, c, r);
						break;
					}
					break;
				case DNG_ITEM_KEY:
					wld_init_item(item, ITEM_KEY_BASIC);
					wld_map_new_item(map, item, c, r);
					item->key_id = cell->key_id; // matches a door somewhere spooky
					break;
				}
			}
		}
	}
}

struct wld_map* wld_new_map(int id, int difficulty, int width, int height)
{
	struct wld_map *map = (struct wld_map*)malloc(sizeof(struct wld_map));

	// data
	map->id = id;
	map->rows = height;
	map->cols = width;
	map->length = width * height;
	map->difficulty = difficulty;

	map->tile_map = NULL;
	map->mob_map = NULL;
	map->tiles = NULL;
	map->mobs = NULL;
	map->mobs_length = 0;
	map->mobs_capacity = 0;
	map->items = NULL;
	map->items_length = 0;
	map->items_capacity = 0;
	map->player = NULL;
	map->cursor = (struct wld_cursor*)malloc(sizeof(struct wld_cursor));
	map->cursor->x = 0;
	map->cursor->y = 0;
	map->cursor->index = 0;

	// function events
	map->on_player_map_transition = NULL;
	map->on_cursormove = NULL;
	map->on_playermove = NULL;

	map->on_mob_heal = NULL;
	map->on_mob_attack_mob = NULL;
	map->on_mob_attack_player = NULL;
	map->on_mob_whiff = NULL;
	map->on_mob_whiff_mob = NULL;
	map->on_mob_whiff_player = NULL;
	map->on_mob_kill_mob = NULL;
	map->on_mob_kill_player = NULL;

	map->on_player_heal = NULL;
	map->on_player_attack_mob = NULL;
	map->on_player_whiff = NULL;
	map->on_player_whiff_mob = NULL;
	map->on_player_kill_mob = NULL;
	map->on_player_pickup_item = NULL;
	map->on_player_pickup_item_fail = NULL;
	map->on_player_drop_item = NULL;
	map->on_player_drop_item_fail = NULL;

	return map;
}

// frees mob memory
void wld_delete_mob(struct wld_mob *mob)
{
	for (int j=0; j<INVENTORY_SIZE; j++)
		if (mob->inventory[j] != NULL)
			free(mob->inventory[j]);
	free(mob->inventory);
	free(mob);
}

void wld_delete_map(struct wld_map *map)
{
	free(map->cursor);

	for (int i=0; i<map->items_length; i++)
		if (map->items[i] != NULL)
			free(map->items[i]);
	free(map->items);
	free(map->item_map);


	for (int i=0; i<map->mobs_length; i++)
		wld_delete_mob(map->mobs[i]);
	free(map->mobs);
	free(map->mob_map);

	free(map->tiles);
	free(map->tile_map);
	free(map);
}

// MAP INITALIZATION END
///////////////////////////




///////////////////////////
// MAP METHODS START

int wld_calcindex(int x, int y, int cols)
{
	return y * cols + x;
}

// TODO I think these are wrong!
int wld_calcx(int index, int cols)
{
	return index % cols;
}

// TODO I think these are wrong!
int wld_calcy(int index, int rows)
{
	return index / rows;
}

void wld_map_insert_item(struct wld_map* map, struct wld_item* item, int x, int y, int id)
{
	int index = wld_calcindex(x, y, map->cols);
	// assign items properties for the map
	item->id = id;
	item->map_x = x;
	item->map_y = y;
	item->map_index = index;
	// assign map the items id and stuff
	map->items[id] = item;
	map->item_map[index] = id;
}

void wld_map_new_item(struct wld_map* map, struct wld_item* item, int x, int y)
{
	// if out of room expand
	if (map->items_length == map->items_capacity) {
		map->items = (struct wld_item**)realloc(map->items, (map->items_capacity + MALLOC_MOB_SIZE) * sizeof(struct wld_item*));
		map->items_capacity += MALLOC_MOB_SIZE;
	}
	// copy to the next spot
	map->items[map->items_length] = item;

	// update the map to contain the item
	wld_map_insert_item(map, item, x, y, map->items_length);

	// increment our list
	map->items_length++;
}

void wld_map_remove_item(struct wld_map* map, struct wld_item* item)
{
	bool found = false;
	for (int i=0; i<map->items_length; i++) {
		if (found) {
			// shift backwards a spot
			struct wld_item *next_item = map->items[i];
			next_item->id = i - 1;
			map->items[next_item->id] = next_item;
			// his id is at his position in the map, we need to point that to his new id
			map->item_map[next_item->map_index] = next_item->id;
		} else if(map->items[i] == item) {
			// if we find the item, null his instance and -1 his item map
			int index = item->map_index;
			map->items[i] = NULL;
			map->item_map[index] = -1;
			found = true;

			item->id = -1;
			item->map_index = -1;
			item->map_x = 0;
			item->map_y = 0;
		}
	}

	if (found) {
		// turn the last index into a null
		map->items[map->items_length - 1] = NULL;
		map->items_length--;
	}
}

// take a mob instance and add him to the mob list
void wld_map_insert_mob(struct wld_map* map, struct wld_mob* mob, int x, int y, int id)
{
	int index = wld_calcindex(x, y, map->cols);
	// assign mobs properties for the map
	mob->id = id;
	mob->map = map;
	mob->map_x = x;
	mob->map_y = y;
	mob->map_index = index;
	// assign map the mobs id and stuff
	map->mobs[id] = mob;
	map->mob_map[index] = id;
}

// This is called when we want the map to take ownership of the mob
void wld_map_new_mob(struct wld_map* map, struct wld_mob* mob, int x , int y)
{
	// if out of room expand
	if (map->mobs_length == map->mobs_capacity) {
		map->mobs = (struct wld_mob**)realloc(map->mobs, (map->mobs_capacity + MALLOC_MOB_SIZE) * sizeof(struct wld_mob*));
		map->mobs_capacity += MALLOC_MOB_SIZE;
	}
	// copy to the next spot
	map->mobs[map->mobs_length] = mob;

	// update the map to contain the mob
	wld_map_insert_mob(map, mob, x, y, map->mobs_length);

	// increment our list
	map->mobs_length++;
}

// This is called by other game code and is very specifically not called
// during any function that runs in the update loop
void wld_map_queue_destroy_mob(struct wld_map* map, struct wld_mob* mob)
{
	mob->is_destroy_queued = true;
}

// This is called when we want to remove the map and free it from memory
// such as when a mob is killed and permanently removed from the game
void wld_map_destroy_mob(struct wld_map* map, struct wld_mob* mob)
{
	wld_map_remove_mob(map, mob);
	wld_delete_mob(mob); // frees memory
}

// This is called when we don't want to destroy a mob, just remove it from map ownership
void wld_map_remove_mob(struct wld_map* map, struct wld_mob* mob)
{
	bool found = false;
	for (int i=0; i<map->mobs_length; i++) {
		if (found) {
			// shift backwards a spot
			struct wld_mob *next_mob = map->mobs[i];
			next_mob->id = i - 1;
			map->mobs[next_mob->id] = next_mob;
			// his id is at his position in the map, we need to point that to his new id
			map->mob_map[next_mob->map_index] = next_mob->id;
		} else if(map->mobs[i] == mob) {
			// if we find the mob, null his instance and -1 his mob map
			int index = mob->map_index;
			map->mobs[i] = NULL;
			map->mob_map[index] = -1;
			found = true;

			mob->id = -1;
			mob->map_index = -1;
			mob->map_x = 0;
			mob->map_y = 0;
			mob->map = NULL;
		}
	}

	if (found) {
		// turn the last index into a null
		map->mobs[map->mobs_length - 1] = NULL;
		map->mobs_length--;
	}
}

void wld_map_add_mob_at_entrance(struct wld_map* map, struct wld_mob* mob)
{
	wld_map_new_mob(map, mob, map->entrance_tile->map_x, map->entrance_tile->map_y);
}

void wld_map_add_mob_at_exit(struct wld_map* map, struct wld_mob* mob)
{
	wld_map_new_mob(map, mob, map->exit_tile->map_x, map->exit_tile->map_y);
}

bool wld_map_can_move_to(struct wld_map *map, int x, int y)
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

	// if this is a door it is blocked if its
	// door is not open and it is locked
	// TODO STOPPED HERE

	int tile_id = map->tile_map[map_index];
	if (wld_tile_is_blocked_movement(&map->tiles[tile_id]))
		return false;

	return true;
}

void wld_map_move_cursor(struct wld_map *map, int relx, int rely)
{
	int newx = map->cursor->x + relx;
	int newy = map->cursor->y + rely;
	wld_map_set_cursor_pos(map, newx, newy);
}

void wld_map_set_cursor_pos(struct wld_map *map, int newx, int newy)
{
	if (newx >= 0 && newx < map->cols && newy >= 0 && newy < map->rows)
	{
		map->cursor->x = newx;
		map->cursor->y = newy;
		map->cursor->index = wld_calcindex(map->cursor->x, map->cursor->y, map->cols);
		// notify subscriber
		map->on_cursormove(map, map->cursor->x, map->cursor->y, map->cursor->index);
		// if I am on top of a mob set them as the target
		map->player->cursor_target_index = map->cursor->index;
	}
}

struct wld_tile* wld_map_get_tile_at(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	return &map->tiles[map->tile_map[index]];
}

struct wld_tile* wld_map_get_tile_at_index(struct wld_map *map, int index)
{
	return &map->tiles[map->tile_map[index]];
}

struct wld_mob* wld_map_get_mob_at(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	int id = map->mob_map[index];
	if (id > -1)
		return map->mobs[id];
	return NULL;
}

struct wld_mob* wld_map_get_mob_at_index(struct wld_map *map, int index)
{
	int id = map->mob_map[index];
	if (id > -1)
		return map->mobs[id];
	return NULL;
}

struct wld_item* wld_map_get_item_at(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	int id = map->item_map[index];
	if (id > -1)
		return map->items[id]; // pointers
	return NULL;
}

struct wld_item* wld_map_get_item_at_index(struct wld_map *map, int index)
{
	int id = map->item_map[index];
	if (id > -1)
		return map->items[id]; // pointers
	return NULL;
}

struct draw_struct wld_map_get_drawstruct(struct wld_map *map, int x, int y)
{
	struct wld_tile *t = wld_map_get_tile_at(map, x, y);
	unsigned long cha = t->type->sprite;
	int mob_id = map->mob_map[t->map_index];
	int item_id = map->item_map[t->map_index];

	int colorpair;
	struct wld_mob *m = wld_map_get_mob_at(map, x, y);
	if (m && !m->is_dead) { // player is dead and not removed
		// if mob use its fg sprite and fg color
		if (m->type->sprite != ' ')
			cha = m->type->sprite;
		colorpair = wld_cpair_tm(t->type_id, m->type_id);
	} else if(item_id > -1) {
		// if item  use its fg sprite and fg color
		struct wld_item *i = wld_map_get_item_at(map, x, y);
		if (i->type->sprite != ' ')
			cha = i->type->sprite;
		colorpair = wld_cpair_ti(t->type_id, i->type_id);
	} else if(t->dead_mob_type != NULL) {
		// if we have dead mob on this tile use its display
		// if there is a dead mob on this then color it bloody
		colorpair = wld_cpair(WCLR_BLACK, WCLR_RED);
		cha = t->dead_mob_type->sprite;
	} else {
		colorpair = wld_cpair_tm(t->type_id, 0);
	}

	struct draw_struct ds = { colorpair, cha };
	return ds;
}

struct draw_struct wld_map_get_drawstruct_memory(struct wld_map *map, int x, int y)
{
	// memory we do not look at mob data
	struct wld_tile *t = wld_map_get_tile_at(map, x, y);
	unsigned long cha = t->type->memory_sprite;
	int colorpair = wld_cpairmem(t->type_id);

	struct draw_struct ds = { colorpair, cha };
	return ds;
}

// MAP METHODS END
///////////////////////////




///////////////////////////
// TILE EVENTS START

void wld_tile_on_mob_enter_entrance(struct wld_map* map, struct wld_tile* tile, struct wld_mob* mob)
{
	if (mob->is_player) {
		if (map->on_player_map_transition)
			map->on_player_map_transition(map, mob, false);
	}
}

void wld_tile_on_mob_enter_exit(struct wld_map* map, struct wld_tile* tile, struct wld_mob* mob)
{
	if (mob->is_player) {
		if (map->on_player_map_transition)
			map->on_player_map_transition(map, mob, true);
	}
}

bool wld_tile_is_blocked_vision(struct wld_tile* tile)
{
	// door is visually blocked if not open
	if (tile->is_door)
		return !tile->is_door_open;
	// otherwise inherit settings from tile and parent
	return tile->type->is_block || tile->is_blocked;
}

bool wld_tile_is_blocked_movement(struct wld_tile* tile)
{
	// a door is blocked only if it is clsed and locked
	if (tile->is_door)
		return tile->is_door_locked && !tile->is_door_open;
	// otherwise inherit settings from tile and parent
	return tile->type->is_block || tile->is_blocked;
}

// TILE EVENTS END
///////////////////////////



///////////////////////////
// TYPE METHODS START

struct wld_tiletype* wld_get_tiletype(int id)
{
	return &wld_tiletypes[id];
}
struct wld_mobtype* wld_get_mobtype(int id)
{
	return &wld_mobtypes[id];
}
struct wld_itemtype* wld_get_itemtype(int id)
{
	return &wld_itemtypes[id];
}
int wld_cpair(enum WLD_COLOR_INDEX a, enum WLD_COLOR_INDEX b)
{
	return WLD_COLOR_BASE + (a * WLD_COLOR_COUNT + b);
}
int wld_cpair_tm(int tiletype, int mobtype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	if (mobtype == 0)
		return WLD_COLOR_BASE + (tt->fg_color * WLD_COLOR_COUNT + tt->bg_color);
	struct wld_mobtype *mt = wld_get_mobtype(mobtype);
	return WLD_COLOR_BASE + (mt->fg_color * WLD_COLOR_COUNT + tt->bg_color);
}
int wld_cpair_ti(int tiletype, int itemtype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	if (itemtype == 0)
		return WLD_COLOR_BASE + (tt->fg_color * WLD_COLOR_COUNT + tt->bg_color);
	struct wld_itemtype *it = wld_get_itemtype(itemtype);
	return WLD_COLOR_BASE + (it->fg_color * WLD_COLOR_COUNT + tt->bg_color);
}
int wld_cpairmem(int tiletype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	return WLD_COLOR_BASE + (tt->memory_fg_color * WLD_COLOR_COUNT + tt->memory_bg_color);
}

int wld_cpair_bg(int tiletype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	return WLD_COLOR_BASE + tt->bg_color; // no addition of foreground so it is black
}

// TYPE METHODS END
///////////////////////////




///////////////////////////
// MOB METHODS START


int wld_mob_dist_tile(struct wld_mob *mob, struct wld_tile *tile)
{
	int mx = mob->map_x;
	int my = mob->map_y;
	int tx = tile->map_x;
	int ty = tile->map_y;
	return dm_disti(mx, my, tx, ty);
}

// TODO code duplicate with wld_mob_move
void wld_mob_teleport(struct wld_mob *mob, int x, int y, bool trigger_events)
{
	if (wld_map_can_move_to(mob->map, x, y)) {
		int old_index = mob->map_index;
		int new_index = wld_calcindex(x, y, mob->map->cols);

		// update indexes
		mob->map->mob_map[new_index] = mob->id;
		mob->map->mob_map[old_index] = -1; // vacated

		// update position
		mob->map_index = new_index;
		mob->map_x = x;
		mob->map_y = y;

		if (trigger_events) {
			// run tile on enter event
			struct wld_tile* tile = wld_map_get_tile_at(mob->map, mob->map_x, mob->map_y);
			if (tile->on_mob_enter)
				tile->on_mob_enter(mob->map, tile, mob);

			// if player, collect items he walks over unless he dropped it before
			struct wld_item *item = wld_map_get_item_at(mob->map, mob->map_x, mob->map_y);
			if (mob->is_player && item != NULL && !item->has_dropped && wld_mob_has_inventory(mob)) {
				wld_mob_pickup_item(mob, item);
			}
		}
	}
}
void wld_mob_move(struct wld_mob *mob, int relx, int rely, bool trigger_events)
{
	// TODO speeds and things could come into play here
	// TODO need to make sure they do not diagonally move around corners
	int newx = mob->map_x + relx;
	int newy = mob->map_y + rely;

	// we cannot physically move around corners
	if (relx != 0 && rely != 0) {
		double dirf_x, dirf_y;
		dm_direction((double)mob->map_x, (double)mob->map_y, (double)newx, (double)newy, &dirf_x, &dirf_y);
		int rx = (int)dm_ceil_out(dirf_x);
		int ry = (int)dm_ceil_out(dirf_y);
		struct wld_tile *t1 = wld_map_get_tile_at(mob->map, newx, newy - ry);
		struct wld_tile *t2 = wld_map_get_tile_at(mob->map, newx - rx, newy);
		if ((t1 && wld_tile_is_blocked_movement(t1)) || (t2 && wld_tile_is_blocked_movement(t2))) {
			return;
		}
	}

	// test if we can do this move
	if (wld_map_can_move_to(mob->map, newx, newy)) {
		int old_index = mob->map_index;
		int new_index = wld_calcindex(newx, newy, mob->map->cols);

		// update indexes
		mob->map->mob_map[new_index] = mob->id;
		mob->map->mob_map[old_index] = -1; // vacated

		// update position
		mob->map_index = new_index;
		mob->map_x = newx;
		mob->map_y = newy;

		if (trigger_events) {
			// run tile on enter event
			struct wld_tile* tile = wld_map_get_tile_at(mob->map, mob->map_x, mob->map_y);
			if (tile->on_mob_enter)
				tile->on_mob_enter(mob->map, tile, mob);

			// if player, collect items he walks over unless he dropped it before
			struct wld_item *item = wld_map_get_item_at(mob->map, mob->map_x, mob->map_y);
			if (mob->is_player && item != NULL && !item->has_dropped && wld_mob_has_inventory(mob)) {
				wld_mob_pickup_item(mob, item);
			}
		}
	}
}
void wld_mob_vision(struct wld_mob *mob, void (*on_see)(struct wld_mob*, int, int, double))
{
	// todo get radius of mobs vision?
	struct wld_map* map = mob->map;
	bool wld_ss_isblocked(int x, int y)
	{
		struct wld_tile *t = wld_map_get_tile_at(map, x, y);
		return wld_tile_is_blocked_vision(t);
	}
	void wld_ss_onvisible(int x, int y, double radius, unsigned int ss_id)
	{
		struct wld_tile *t = wld_map_get_tile_at(map, x, y);
		if (t->dm_ss_id != ss_id) {
			on_see(mob, x, y, radius);
			t->dm_ss_id = ss_id;
		}
	}
	dm_shadowcast(mob->map_x, mob->map_y, map->cols, map->rows, mob->vision, wld_ss_isblocked, wld_ss_onvisible, false); // no leakage allowed
}
bool wld_mob_is_next_to_tile(struct wld_mob *mob, struct wld_tile* tile)
{
	int diffx = abs(mob->map_x - tile->map_x);
	int diffy = abs(mob->map_y - tile->map_y);
	return diffx <= 1 && diffy <= 1;
}
bool wld_mob_is_next_to_mob(struct wld_mob *ma, struct wld_mob *mb)
{
	int diffx = abs(ma->map_x - mb->map_x);
	int diffy = abs(ma->map_y - mb->map_y);
	return diffx <= 1 && diffy <= 1;
}
struct wld_item* wld_mob_get_item_in_slot(struct wld_mob *mob, int slot)
{
	struct wld_item *item = mob->inventory[slot];
	return item;
}
int wld_mob_get_open_inventory_slot(struct wld_mob *mob)
{
	// slot 0 reserved for weapon
	// slot 1 reserved for armor/robes
	for (int i=2; i<INVENTORY_SIZE; i++)
		if (mob->inventory[i] == NULL)
			return i;
	return -1;
}
bool wld_mob_has_inventory(struct wld_mob *mob)
{
	int slot = wld_mob_get_open_inventory_slot(mob);
	if (slot != -1)
		return true;
	return false;
}
bool wld_mob_pickup_item(struct wld_mob *m, struct wld_item *i)
{
	int slot = wld_mob_get_open_inventory_slot(m);
	if (slot != -1) {
		// set item copy in world to nonexist
		// move item from world to mob inventory
		if (i->id != -1) // if in the map remove it
			wld_map_remove_item(m->map, i);

		// copy item to mob inventory
		m->inventory[slot] = i;
		i->map_found = m->map->id;

		if (m->is_player && m->map->on_player_pickup_item)
			m->map->on_player_pickup_item(m->map, m, i);

		return true;
	}

	if (m->is_player && m->map->on_player_pickup_item_fail)
		m->map->on_player_pickup_item_fail(m->map, m, i);

	return false;
}
bool wld_mob_drink_item(struct wld_mob *mob, int itemslot)
{
	struct wld_item* item = wld_mob_get_item_in_slot(mob, itemslot);
	if (item != NULL) {
		if (item->type->fn_drink) {
			item->type->fn_drink(item, mob);
			wld_mob_resolve_item_uses(mob, item);
			return true;
		}
	}
	return false;
}

void wld_mob_destroy_item_in_slot(struct wld_mob* mob, int itemslot)
{
	struct wld_item *item = mob->inventory[itemslot];
	if (mob->is_player)
		wld_log_it("Your %s was destroyed.", item);

	if (item == mob->active_item)
		mob->active_item = NULL;
	free(item);
	mob->inventory[itemslot] = NULL;
}

void wld_mob_destroy_item(struct wld_mob* mob, struct wld_item* item)
{
	// need to find the item in the slot
	for (int i=0; i<INVENTORY_SIZE; i++) {
		struct wld_item *check = mob->inventory[i];
		if (check == item) {
			wld_mob_destroy_item_in_slot(mob, i);
			break;
		}
	}
}

void wld_mob_resolve_item_uses(struct wld_mob* mob, struct wld_item* item)
{
	if (item->type->has_uses && --item->uses <= 0)
		wld_mob_destroy_item(mob, item);
}

bool wld_mob_drop_item(struct wld_mob *mob, int itemslot)
{
	struct wld_item* item = wld_mob_get_item_in_slot(mob, itemslot);
	// if this is a real item and there is not one on this slot
	if (item != NULL && wld_map_get_item_at(mob->map, mob->map_x, mob->map_y) == NULL) {
		wld_map_new_item(mob->map, item, mob->map_x, mob->map_y);

		// remove from inventory
		mob->inventory[itemslot] = NULL;
		item->has_dropped = true; // mark as having been dropped before

		if (mob->is_player && mob->map->on_player_pickup_item)
			mob->map->on_player_drop_item(mob->map, mob, item);

		return true;
	}

	if (mob->is_player && mob->map->on_player_pickup_item)
		mob->map->on_player_drop_item_fail(mob->map, mob, item);

	return false;
}

void wld_mob_swap_item(struct wld_mob* mob, int slot_a, int slot_b)
{
	struct wld_item *a = wld_mob_get_item_in_slot(mob, slot_a);
	struct wld_item *b = wld_mob_get_item_in_slot(mob, slot_b);
	mob->inventory[slot_b] = a;
	mob->inventory[slot_a] = b;
}

bool wld_mob_equip(struct wld_mob* mob, int itemslot)
{
	struct wld_item *i = wld_mob_get_item_in_slot(mob, itemslot);
	if (i != NULL) {
		// weapon
		if (i->type->is_weq) {
			wld_mob_swap_item(mob, itemslot, 0);
			return true;
		}
		// armor
		if (i->type->is_aeq) {
			wld_mob_swap_item(mob, itemslot, 1);
			return true;
		}
	}
	return false;
}

bool wld_mob_unequip(struct wld_mob* mob, int itemslot)
{
	struct wld_item *i = wld_mob_get_item_in_slot(mob, itemslot);
	if (i != NULL) {
		int open_slot = wld_mob_get_open_inventory_slot(mob);
		if (open_slot != -1) {
			wld_mob_swap_item(mob, itemslot, open_slot);
			return true;
		}
	}
	return false;
}

void wld_mob_inspect_melee(struct wld_mob* mob, void (*inspect)(int,int))
{
	struct dm_spiral sp = dm_spiral(1);
	while (dm_spiralnext(&sp)) {
		int spx = mob->map_x + sp.x;
		int spy = mob->map_y + sp.y;
		struct wld_tile *t = wld_map_get_tile_at(mob->map, spx, spy);
		if (t->is_visible) {
			inspect(spx, spy);
		}
	}
}

void wld_mob_inspect_targetables(struct wld_mob* mob, void (*inspect)(int,int))
{
	// if we have an item we are using, target with it
	if (mob->active_item) {
		if (mob->active_item->type->fn_target != NULL) {
			mob->active_item->type->fn_target(mob->active_item, mob, inspect);
			return;
		}

	}

	// else target as melee
	wld_mob_inspect_melee(mob, inspect);
}

void wld_mob_inspect_inventory(struct wld_mob *mob, void (*inspect)(struct wld_item*))
{
	for (int i=0; i < INVENTORY_SIZE; i++) {
		struct wld_item *item = mob->inventory[i];
		if (item)
			inspect(item);
	}
}

// MOB METHODS END
///////////////////////////



///////////////////////////
// MOB AI START

struct wld_mob* ai_get_closest_visible_enemy(struct wld_mob* self)
{
	struct wld_mob* closest_enemy = NULL;
	int closest_dist = 0;
	void invision(struct wld_mob *myself, int x, int y, double radius) {
		struct wld_mob *visible_mob = wld_map_get_mob_at(myself->map, x, y);
		if (visible_mob) {
			int dist = dm_disti(myself->map_x, myself->map_y, visible_mob->map_x, visible_mob->map_y);
			if(myself->ai_is_hostile && myself->ai_is_hostile(myself, visible_mob) && (closest_enemy == NULL || dist < closest_dist)) {
				closest_enemy = visible_mob;
				closest_dist = dist;
			}
		}
	}
	wld_mob_vision(self, invision);
	return closest_enemy;
}

void ai_flee_enemy(struct wld_mob* self, struct wld_mob *enemy)
{
	double dirf_x, dirf_y;
	dm_direction((double)self->map_x, (double)self->map_y, (double)enemy->map_x, (double)enemy->map_y, &dirf_x, &dirf_y);
	int diri_x = -dm_round(dirf_x);
	int diri_y = -dm_round(dirf_y);
	// move away from closest enemy
	if (wld_map_can_move_to(self->map, self->map_x + diri_x, self->map_y + diri_y)) {
		self->queue_x += diri_x;
		self->queue_y += diri_y;
	} else if(wld_map_can_move_to(self->map, self->map_x + diri_x, self->map_y)) {
		self->queue_x += diri_x;
		self->queue_y;
	} else if(wld_map_can_move_to(self->map, self->map_x, self->map_y + diri_y)) {
		self->queue_x;
		self->queue_y += diri_y;
	} else {
		// trapped run randomly
		int dirx = dm_randii(0, 3) - 1;
		int diry = dm_randii(0, 3) - 1;
		self->queue_x += dirx;
		self->queue_y += diry;
	}
}

void ai_default_wander(struct wld_mob *mob)
{
	// todo pick random destinations to walk to?
	if (dm_chance(1, 4)) {
		// 1/4 chance to wander in a random spot
		int dirx = dm_randii(0, 3) - 1;
		int diry = dm_randii(0, 3) - 1;
		mob->queue_x += dirx;
		mob->queue_y += diry;
	}
}

bool ai_default_is_hostile(struct wld_mob *self, struct wld_mob *target)
{
	return target->is_player && !target->is_dead && self != target;
}

bool ai_default_detect_combat(struct wld_mob *self)
{
	// I need to see if I can see an enemy that is a threat to me
	// This is expensive to run on a bunch of mobs
	// So it should maybe be optimized in som manner?
	if (!self->ai_is_hostile)
		return false;

	bool detect_enemy = false;
	void on_visible(struct wld_mob *myself, int x, int y, double radius) {
		struct wld_mob *visible_mob = wld_map_get_mob_at(myself->map, x, y);
		if (visible_mob && myself->ai_is_hostile && myself->ai_is_hostile(myself, visible_mob)) {
			detect_enemy = true;
		}
	}
	wld_mob_vision(self, on_visible);
	return detect_enemy;
}

void ai_default_decide_combat(struct wld_mob *self) // melee approach, melee attack
{
	// Basic melee combat to attack hostile neighbors
	// if health is above 50%, otherwise it flees
	double hp = (double)self->health / (double)self->maxhealth;
	if (hp > .5) { // healthy enough to fight
		struct wld_mob* hostile_neighbor = NULL;
		void inspect(int x, int y) {
			struct wld_mob *neighbor = wld_map_get_mob_at(self->map, x, y);
			if(neighbor && self->ai_is_hostile(self, neighbor))
				hostile_neighbor = neighbor;
		}
		wld_mob_inspect_melee(self, inspect);

		if (hostile_neighbor) {
			// attack target
			ai_mob_melee_mob(self, hostile_neighbor);
		} else {
			int x = self->map->player->map_x;
			int y = self->map->player->map_y;
			// move to target
			if (dm_chance(1,6)) {
				// random movement
				self->queue_x = dm_randii(0, 3) - 1;
				self->queue_y = dm_randii(0, 3) - 1;
			} else {
				if (x < self->map_x)
					self->queue_x += -1;
				else if (x > self->map_x)
					self->queue_x += 1;
				if (y < self->map_y)
					self->queue_y += -1;
				else if (y > self->map_y)
					self->queue_y += 1;
			}
		}
	} else { // badly hurt, gonna go lick my wounds
		// find closest
		struct wld_mob* closest_enemy = ai_get_closest_visible_enemy(self);
		if (closest_enemy) {
			ai_flee_enemy(self, closest_enemy);
		}
	}
}

// item can be NULL if it was not an item
void ai_mob_heal(struct wld_mob *mob, int amt, struct wld_item* item)
{
	mob->health += amt;
	if (mob->health > mob->maxhealth)
		mob->health = mob->maxhealth;
	if (mob->is_player && mob->map->on_player_heal)
		mob->map->on_player_heal(mob->map, mob, amt, item);
	else if(mob->map->on_mob_heal)
		mob->map->on_mob_heal(mob->map, mob, amt, item);
}

void ai_mob_kill_mob(struct wld_mob *aggressor, struct wld_mob *defender, struct wld_item* item)
{
	defender->state = MS_DEAD;
	defender->health = 0;
	defender->is_dead = true;
	// notify event
	if (!defender->is_player && aggressor->map->on_mob_kill_mob)
		aggressor->map->on_mob_kill_mob(aggressor->map, aggressor, defender, item);
	if (defender->is_player && aggressor->map->on_mob_kill_player)
		aggressor->map->on_mob_kill_player(aggressor->map, aggressor, defender, item);
	if (aggressor->is_player && aggressor->map->on_player_kill_mob)
		aggressor->map->on_player_kill_mob(aggressor->map, aggressor, defender, item);
	// after event executed lets remove the mob and add the dead mob type to the tile floor
	// TODO drop loot?
	struct wld_tile* tile = wld_map_get_tile_at_index(defender->map, defender->map_index);
	tile->dead_mob_type = defender->type; // only most recent

	// set this mob up to be destroyed if its not the player
	if (!defender->is_player)
		wld_map_queue_destroy_mob(defender->map, defender);
}

void ai_mob_attack_mob(struct wld_mob *aggressor, struct wld_mob *defender, int amt, struct wld_item *item)
{
	defender->health -= amt;

	// TODO pass damage amount, attack type, etc
	if (!defender->is_player && aggressor->map->on_mob_attack_mob)
		aggressor->map->on_mob_attack_mob(aggressor->map, aggressor, defender, amt, item);
	if (defender->is_player && aggressor->map->on_mob_attack_player)
		aggressor->map->on_mob_attack_player(aggressor->map, aggressor, defender, amt, item);
	if (aggressor->is_player && aggressor->map->on_player_attack_mob)
		aggressor->map->on_player_attack_mob(aggressor->map, aggressor, defender, amt, item);

	if (defender->health <= 0)
		ai_mob_kill_mob(aggressor, defender, item);

}

// triggered from inputs that request attack
// Bool returns if it was a valid option?
bool ai_can_melee(struct wld_mob *aggressor, struct wld_mob *defender)
{
	return !defender->is_dead && wld_mob_is_next_to_mob(aggressor, defender);
}

void ai_mob_whiff(struct wld_mob *aggressor, struct wld_item* item)
{
	if (!aggressor->is_player && aggressor->map->on_mob_whiff)
		aggressor->map->on_mob_whiff(aggressor->map, aggressor, item);

	if (aggressor->is_player && aggressor->map->on_player_whiff)
		aggressor->map->on_player_whiff(aggressor->map, aggressor, item);
}

void ai_mob_whiff_mob(struct wld_mob *aggressor, struct wld_mob *defender, struct wld_item* item)
{
	if (!defender->is_player && aggressor->map->on_mob_whiff_mob)
		aggressor->map->on_mob_whiff_mob(aggressor->map, aggressor, defender, item);
	if (defender->is_player && aggressor->map->on_mob_whiff_player)
		aggressor->map->on_mob_whiff_player(aggressor->map, aggressor, defender, item);
	if (aggressor->is_player && aggressor->map->on_player_whiff_mob)
		aggressor->map->on_player_whiff_mob(aggressor->map, aggressor, defender, item);
}

void ai_mob_melee_mob(struct wld_mob *aggressor, struct wld_mob *defender)
{
	// determine melee damage from weapon (or unarmed?)
	struct wld_item* weapon = wld_mob_get_item_in_slot(aggressor, 0);
	if (weapon != NULL) {
		struct wld_tile *tile = wld_map_get_tile_at_index(defender->map, defender->map_index);
		if (weapon->type->fn_can_use(weapon, aggressor, tile)) {
			weapon->type->fn_use(weapon, aggressor, tile);
			wld_mob_resolve_item_uses(aggressor, weapon);
			return;
		}
	} else {
		// not a melee weapon (use fists)
		double chance = rpg_calc_melee_coh(aggressor, defender);
		if (dm_randf() < chance) {
			int dmg = rpg_calc_melee_dmg(aggressor, defender);
			ai_mob_attack_mob(aggressor, defender, dmg, NULL);
			return;
		}
	}
	// whiff event
	ai_mob_whiff_mob(aggressor, defender, weapon);
}

bool ai_mob_use_item(struct wld_mob* mob, struct wld_item* item, struct wld_tile* cursor_tile)
{
	if (item->type->fn_can_use(item, mob, cursor_tile)) {
		item->type->fn_use(item, mob, cursor_tile);
		// if this item can be used up then do so
		wld_mob_resolve_item_uses(mob, item);
		return true;
	}
	return false;
}

bool ai_player_use_active_item(struct wld_mob* player)
{
	// this is run with the active item (which is drawn from sheath or used from inventory)
	if (player->active_item == NULL)
		return false;

	struct wld_tile *tile = wld_map_get_tile_at_index(player->map, player->cursor_target_index);
	return ai_mob_use_item(player, player->active_item, tile);
}

bool ai_player_trigger_target(struct wld_mob* player)
{
	// this is run whenever in a target mode
	// Melee can be here without an active item
	if (player->active_item != NULL)
		return ai_player_use_active_item(player);

	// else do a fist attack
	struct wld_mob *target = wld_map_get_mob_at_index(player->map, player->cursor_target_index);
	if (target != NULL && ai_can_melee(player, target)) {
		ai_mob_melee_mob(player, target);
		return true;
	}

	return false;
}

// this is synomous with drawing your active weapon
// but it is called after something else has set the active item (or spell etc)
bool ai_player_set_use_item(struct wld_mob* mob, int itemslot)
{
	struct wld_item* item = wld_mob_get_item_in_slot(mob, itemslot);
	if (item != NULL && item->type->fn_use != NULL) {
		mob->active_item = item;
		return true;
	}
	// TODO Events to say the user has prepped to use an item
	return false;
}

bool ai_player_enter_targeting(struct wld_mob* player)
{
	player->target_mode = TMODE_ACTIVE;
	return true;
}

bool ai_player_draw_weapon(struct wld_mob* player)
{
	struct wld_item *weapon = wld_mob_get_item_in_slot(player, 0);
	if (weapon) {
		player->target_mode = TMODE_ACTIVE;
		player->active_item = weapon;
		return true;
	}
	// unarmed
	player->target_mode = TMODE_ACTIVE;
	player->active_item = NULL;
	return false;
}

// this is synomous with sheathing your active weapon
// but it also will nullify active item if it was not primary weapon
bool ai_player_leave_targeting(struct wld_mob* player)
{
	// unarmed
	player->target_mode = TMODE_NONE;
	player->active_item = NULL;
	return true;
}

bool ai_player_sheath_weapon(struct wld_mob* player)
{
	// unarmed
	player->target_mode = TMODE_NONE;
	player->active_item = NULL;
	return true;
}

bool ai_queuemobmove(struct wld_mob *mob, int relx, int rely)
{
	int newx = mob->map_x + relx;
	int newy = mob->map_y + rely;
	if (wld_map_can_move_to(mob->map, newx, newy)) {
		mob->queue_x += relx;
		mob->queue_y += rely;
		return true;
	}
	return false;
}
bool ai_act_upon(struct wld_mob *mob, int relx, int rely)
{
	// inspect objects at position, if tile thats traversable queue walking on to it
	// if it has a mob then and they are living, melee it (or use active weapon on it)
	// if it has a mob that is dead, search it
	// if it is a tile that is a transition (exit) confirm then descend
	int newx = mob->map_x + relx;
	int newy = mob->map_y + rely;

	struct wld_tile *tile = wld_map_get_tile_at(mob->map, newx, newy);
	struct wld_mob *mob2 = wld_map_get_mob_at(mob->map, newx, newy);
	if (mob2 != NULL) {
		if (!mob2->is_dead) {
			ai_mob_melee_mob(mob, mob2);
			return true;
		} else {
			// ai_search_mob TODO
			// ai_abberation_mob
			return false;
		}
	} else {
		if (tile->is_door && tile->is_door_locked && tile->door_lock_id > -1) {
			// search inventory for key
			struct wld_item *key = NULL;
			void inspect(struct wld_item *item) {
				if (!item->type->is_key || !item->type->fn_can_use || !item->type->fn_use)
					return;
				if (item->type->fn_can_use(item, mob, tile))
					item->type->fn_use(item, mob, tile);
			}
			wld_mob_inspect_inventory(mob, inspect);
		}
		if (wld_map_can_move_to(mob->map, newx, newy)) {
			return ai_queuemobmove(mob, relx, rely);
		} else {
			// I may not be able to move here because the tile
			// is a locked door, if so search inventory for key
			// and unlock, then attempt to move again
		}
		return false;
	}
	return false;
}

bool ai_get(struct wld_mob *mob, int relx, int rely)
{
	int posx = mob->map_x + relx;
	int posy = mob->map_y + rely;
	int posindex = wld_calcindex(posx, posy, mob->map->cols);
	struct wld_item *i = wld_map_get_item_at(mob->map, posx, posy);

	if (i != NULL)
		if (wld_mob_pickup_item(mob, i))
			return true;
	return false;
}

bool ai_rest(struct wld_mob *mob)
{
	// TODO heal or let heal be natural at beginning of turn?
	return true;
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
//ai_rerun:
		// performance helper to see if we are within a certain range
		// of the player before we run heavy computes
		int player_dist = 0;
		if (mob->map->player)
			player_dist = dm_disti(mob->map_x, mob->map_y, mob->map->player->map_x, mob->map->player->map_y);
		bool worth_it = player_dist < (mob->vision * 2);

		switch (mob->state) {
		case MS_WANDER:
			if (mob->ai_detect_combat != NULL && worth_it && mob->ai_detect_combat(mob)) {
				// enter combat
				mob->state = MS_COMBAT;
				//goto ai_rerun;
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
	} else {
		// listen for input commands
		switch (mob->state) {
		case MS_WANDER:
			if (mob->ai_player_input != NULL) {
				mob->ai_player_input(mob);
			}
			break;
		}
	}

	// apply changes
	// TODO maybe this should be a dynamic array?
	wld_mob_move(mob, mob->queue_x, mob->queue_y, true);
	mob->queue_x = 0;
	mob->queue_y = 0;
}

// MOB AI END
///////////////////////////




///////////////////////////
// CHEATS START

void wld_cheat_teleport_exit(struct wld_map *map, struct wld_mob* mob)
{
	struct wld_tile* exit_tile = map->exit_tile;
	wld_mob_teleport(mob, exit_tile->map_x, exit_tile->map_y, false);
}

// CHEATS END
///////////////////////////



///////////////////////////
// RPG CALCULATIONS START

// fist melee
int rpg_calc_melee_dmg(struct wld_mob *aggressor, struct wld_mob *defender)
{
	// a factor of your strength out of 10
	double strf = (double)aggressor->stat_strength / STAT_STR_BASE;
	return 6 * strf;
}

double rpg_calc_melee_coh(struct wld_mob *aggressor, struct wld_mob *defender)
{
	return .5;
}

// melee weapons
int rpg_calc_melee_weapon_dmg(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender)
{
	// TODO strngth * weapon dmg?
	double strf = (double)aggressor->stat_strength / STAT_STR_BASE;
	return 12 * strf;
}

double rpg_calc_melee_weapon_coh(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender)
{
	return .76;
}

// ranged weapons
int rpg_calc_ranged_weapon_dmg(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender)
{
	// TODO flat weapon dmg range?
	return 12;
}

double rpg_calc_ranged_weapon_coh(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender)
{
	return .66;
}

int rpg_calc_range_dist(struct wld_mob *aggressor, int base_range)
{
	// TODO will calculate strength and dex etc
	// TODO break into options for throwing range vs shooting range?
	return base_range;
}

// RPG CALCULATIONS END
///////////////////////////







///////////////////////////
// ITEM ACTIONS START

// MELEE
void itm_target_melee(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int))
{
	// spiral
	wld_mob_inspect_melee(user, inspect);
}

bool itm_can_use_melee(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	struct wld_mob *target = wld_map_get_mob_at_index(user->map, cursor_tile->map_index);
	if (target != NULL && ai_can_melee(user, target))
		return true;
	return false;
}

void itm_use_melee(struct wld_item *weapon, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	// Using a standard melee item just fires on the targeted tile without any calculation
	struct wld_mob *target = wld_map_get_mob_at_index(user->map, cursor_tile->map_index);
	if (target != NULL)
		weapon->type->fn_hit(weapon, user, cursor_tile);
}

void itm_hit_melee_swordstyle(struct wld_item *weapon, struct wld_mob *user, struct wld_tile* tile)
{
	// TODO make this do a melee attack with the item's damage etc
	struct wld_mob *target = wld_map_get_mob_at_index(user->map, tile->map_index);
	double chance = rpg_calc_melee_weapon_coh(user, weapon, target);
	if (dm_randf() < chance) {
		int dmg = rpg_calc_melee_weapon_dmg(user, weapon, target);
		ai_mob_attack_mob(user, target, dmg, weapon);
	} else {
		// whiff event
		ai_mob_whiff_mob(user, target, weapon);
	}
}

// RANGED LOS
void itm_target_ranged_los(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int))
{
	// highlight a line from player to cursor, if something blocks the path then kill the line
	int start_x = user->map->player->map_x;
	int start_y = user->map->player->map_y;
	int end_x = user->map->cursor->x;
	int end_y = user->map->cursor->y;
	int allowed_range = rpg_calc_range_dist(user, item->type->base_range);
	bool is_blocked(int x, int y) {
		struct wld_tile *t = wld_map_get_tile_at(user->map, x, y);

		// stop at tiles outside of the range of the item
		int dist = dm_disti(user->map_x, user->map_y, x, y);
		if (dist > allowed_range)
			return true;

		return wld_tile_is_blocked_movement(t) || wld_tile_is_blocked_vision(t) || !t->is_visible;
	}
	void on_visible(int x, int y) {
		if (x == start_x && y == start_y) // ignore origin position
			return;
		inspect(x, y);
	}
	dm_bresenham(start_x, start_y, end_x, end_y, is_blocked, on_visible);
}

// I can fire a ranged los weapon at any position I want
// the projectile should "hit" any mob or blocked wall in between
// valid positions are the same as targeting los
bool itm_can_use_ranged_los(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	// I would say we CAN use the item at any range despite its limit
	// given that we will allow them to target any tile that is visible without regard for
	// blocked obstacles or mobs in between
	return true; // we can shoot it anywhere, even into the darkness
	//return cursor_tile->is_visible;
}

void itm_use_ranged_los(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	// draw a bresenham line if I run into a living mob, then hit it
	int start_x = user->map->player->map_x;
	int start_y = user->map->player->map_y;
	int end_x = cursor_tile->map_x;
	int end_y = cursor_tile->map_y;
	int allowed_range = rpg_calc_range_dist(user, item->type->base_range);
	bool hit_target = false;
	bool is_blocked(int x, int y) {
		// check and make sure we do not shoot ourself
		if (x == start_x && y == start_y)
			return false;

		// check to see if we have traveled further than our max range
		int dist = dm_disti(user->map_x, user->map_y, x, y);
		if (dist > allowed_range)
			return true;

		// if we have come in contact with a mob, hit it and stop raycast
		struct wld_mob *mob = wld_map_get_mob_at(user->map, x, y);
		if (mob != NULL && !mob->is_dead) {
			item->type->fn_hit(item, user, wld_map_get_tile_at(user->map, x, y));
			hit_target = true;
			return true; // stop inspection
		}

		// if we hit something that blocks movement stop the attack
		struct wld_tile* tile = wld_map_get_tile_at(user->map, x, y);
		if (wld_tile_is_blocked_movement(tile)) {
			if (user->is_player)
				wld_log_ts("It strikes %s.", tile);
			return true;
		}

		return false;
	}
	dm_bresenham(start_x, start_y, end_x, end_y, is_blocked, NULL);

	if (!hit_target)
		ai_mob_whiff(user, item);
}

void itm_hit_ranged_los_bowstyle(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile)
{
	// when this standard ranged attack hits its first target it will do ranged weapon damage
	struct wld_mob *target = wld_map_get_mob_at_index(user->map, tile->map_index);
	double chance = rpg_calc_ranged_weapon_coh(user, item, target);
	if (dm_randf() < chance) {
		int dmg = rpg_calc_ranged_weapon_dmg(user, item, target);
		ai_mob_attack_mob(user, target, dmg, item);
	} else {
		// whiff event
		ai_mob_whiff_mob(user, target, item);
	}
}

// POTIONS
void itm_drink_minorhealth(struct wld_item *item, struct wld_mob *user)
{
	// TODO minor vs major healing levels
	int hp = dm_randf() * 10;
	ai_mob_heal(user, hp, item);
}

void itm_hit_minorhealth(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile)
{
	struct wld_mob *target = wld_map_get_mob_at_index(user->map, tile->map_index);
	int hp = dm_randf() * 10;
	ai_mob_heal(target, hp, item);
}


// KEYS
void itm_target_key(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int))
{
	// spiral
	wld_mob_inspect_melee(user, inspect);
}

bool itm_can_use_key(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	return wld_mob_is_next_to_tile(user, cursor_tile);
}

void itm_use_key(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	if (cursor_tile->door_lock_id == item->key_id) {
		item->type->fn_hit(item, user, cursor_tile);
		// do not increment uses so it is "used" when it opens the door
	} else {
		item->uses++; // increment uses so the key remains in inventory
		wld_log_ss("The %s failed to open %s.", item->type->title, cursor_tile->type->short_desc);
	}
}

void itm_hit_key(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile)
{
	tile->is_door_locked = false;
	wld_log_ss("You used %s to open %s.", item->type->short_desc, tile->type->short_desc);
}

// ITEM ACTIONS END
///////////////////////////






