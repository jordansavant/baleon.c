// Contains all definitions for the world
#include "mt_rand.h"
#include "dm_algorithm.h"
#include "dm_debug.h"
#include <stdlib.h>
#include "dm_world.h"
#include <ncurses.h>

///////////////////////////
// RAW DATA

enum COLOR_INDEX {
	CLX_BLACK,  // 0 void
	CLX_GREEN,  // 1 grass
	CLX_WHITE,  // 2 snow
	CLX_RED,    // 3 fire
	CLX_BLUE,   // 4 water
	CLX_CYAN,   // 5
	CLX_YELLOW, // 6
	CLX_MAGENTA, // 7
};

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
struct wld_tiletype *wld_tiletypes;


///////////////////////////
// MOB STRUCTS
struct wld_mobtype *wld_mobtypes;


///////////////////////////
// ITEM STRUCTS
struct wld_itemtype *wld_itemtypes;


///////////////////////////
// WORLD STRUCTS
#define MALLOC_ITEM_SIZE 10
void wld_insert_item(struct wld_map* map, struct wld_item* item, int x, int y, int id)
{
	int index = wld_calcindex(x, y, map->cols);
	map->items[id] = item;
	map->item_map[index] = id;
}
void wld_new_item(struct wld_map* map, struct wld_item* item, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);

	// see if I have a null spot
	for (int i=0; i<map->items_length; i++) {
		if (map->items[i] == NULL) {
			wld_insert_item(map, item, i, x, y);
			return;
		}
	}

	// nope, time to expand the list
	struct wld_item **new_items = (struct wld_item**)malloc(map->items_length + MALLOC_ITEM_SIZE * sizeof(struct wld_item*));
	for (int i=0; i<map->items_length + MALLOC_ITEM_SIZE; i++) {
		if (i < map->items_length) {
			new_items[i] = map->items[i]; // copied item
		} else if (i ==	map->items_length) {
			wld_insert_item(map, item, i, x, y);
		}
		// else we leav it unitialized garbOGE
	}
	free(map->items);
	map->items = new_items;
	map->items_length += MALLOC_ITEM_SIZE;
}
//void wld_new_mob_at(struct wld_map* map, struct wld_mob* mob, int x, int y, int id)
//{
//	int index = wld_calcindex(x, y, map->cols);
//	maps->mobs[id] = mob;
//	maps->mob_map[index] = id;
//}
//void wld_new_mob(struct wld_map* map, struct wld_mob* mob, int x, int y)
//{
//	// TODO NEEDS TO BUILD MOBHERE
//	// add a mob instance to this world
//	int index = wld_calcindex(x, y, map->cols);
//
//	// see if I have a null spot
//	for (int i=0; i<map->mobs_length; i++) {
//		if (map->mobs[i] == NULL) {
//			wld_new_mob_at(map, mob, i);
//			return;
//		}
//	}
//
//	// nope, time to expand the list
//	struct wld_mob* new_mobs = (struct wld_mob*)malloc(map->mobs_length + MALLOC_MOB_SIZE * sizeof(struct wld_mob));
//	for (int i=0; i<map->mobs_length + MALLOC_MOB_SIZE; i++) {
//		if (i < map->mobs_length) {
//			new_mobs[i] = map->mobs[i]; // copied mob TODO MAKE THESE MALLOCD INSTANCES?
//		} else if (i ==	map->mobs_length) {
//			wld_new_mob_at(map, mob, i);
//		}
//		// else we leav it unitialized garbOGE
//	}
//	free(map->mobs);
//	map->mobs = new_mobs;
//	map->mobs_length += MALLOC_MOB_SIZE;
//	// done?
//	//
//}


///////////////////////////
// UTILITY METHODS

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
int wld_cpair_tm(int tiletype, int mobtype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	if (mobtype == 0)
		return color_base + (tt->fg_color * fg_size + tt->bg_color);
	struct wld_mobtype *mt = wld_get_mobtype(mobtype);
	return color_base + (mt->fg_color * fg_size + tt->bg_color);
}
int wld_cpair_ti(int tiletype, int itemtype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	if (itemtype == 0)
		return color_base + (tt->fg_color * fg_size + tt->bg_color);
	struct wld_itemtype *it = wld_get_itemtype(itemtype);
	return color_base + (it->fg_color * fg_size + tt->bg_color);
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
		// notify subscriber
		map->on_cursormove(map, map->cursor->x, map->cursor->y, map->cursor->index);
		// if I am on top of a mob set them as the target
		map->player->cursor_target = map->cursor->index;
	}
}
struct wld_tile* wld_gettileat(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	return &map->tiles[map->tile_map[index]];
}
struct wld_tile* wld_gettileat_index(struct wld_map *map, int index)
{
	return &map->tiles[map->tile_map[index]];
}
struct wld_mob* wld_getmobat(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	int id = map->mob_map[index];
	if (id > -1)
		return map->mobs[id];
	return NULL;
}
struct wld_mob* wld_getmobat_index(struct wld_map *map, int index)
{
	int id = map->mob_map[index];
	if (id > -1)
		return map->mobs[id];
	return NULL;
}
struct wld_item* wld_getitemat(struct wld_map *map, int x, int y)
{
	int index = wld_calcindex(x, y, map->cols);
	int id = map->item_map[index];
	if (id > -1)
		return map->items[id]; // pointers
	return NULL;
}
struct wld_item* wld_getitemat_index(struct wld_map *map, int index)
{
	int id = map->item_map[index];
	if (id > -1)
		return map->items[id]; // pointers
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
struct draw_struct wld_get_drawstruct(struct wld_map *map, int x, int y)
{
	struct wld_tile *t = wld_gettileat(map, x, y);
	struct wld_tiletype *tt = wld_get_tiletype(t->type);
	unsigned long cha = tt->sprite;
	int mob_id = map->mob_map[t->map_index];
	int item_id = map->item_map[t->map_index];

	int colorpair;
	if (mob_id > -1) {
		// if mob use its fg sprite and fg color
		struct wld_mob *m = wld_getmobat(map, x, y);
		struct wld_mobtype *mt = wld_get_mobtype(m->type);
		if (mt->sprite != ' ')
			cha = mt->sprite;
		colorpair = wld_cpair_tm(t->type, m->type);
	} else if(item_id > -1) {
		// if item  use its fg sprite and fg color
		struct wld_item *i = wld_getitemat(map, x, y);
		struct wld_itemtype *it = wld_get_itemtype(i->type);
		if (it->sprite != ' ')
			cha = it->sprite;
		colorpair = wld_cpair_ti(t->type, i->type);
	} else {
		colorpair = wld_cpair_tm(t->type, 0);
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
bool wld_is_mob_nextto_mob(struct wld_mob *ma, struct wld_mob *mb)
{
	int diffx = abs(ma->map_x - mb->map_x);
	int diffy = abs(ma->map_y - mb->map_y);
	return diffx <= 1 && diffy <= 1;
}
struct wld_item* wld_get_item_in_slot(struct wld_mob *mob, int slot)
{
	struct wld_item *item = mob->inventory[slot];
	return item;
}
int wld_get_open_inventory_slot(struct wld_mob *mob)
{
	// slot 0 reserved for weapon
	// slot 1 reserved for armor/robes
	for (int i=2; i<INVENTORY_SIZE; i++)
		if (mob->inventory[i] == NULL)
			return i;
	return -1;
}
bool wld_has_inventory(struct wld_mob *mob)
{
	int slot = wld_get_open_inventory_slot(mob);
	if (slot != -1)
		return true;
	return false;
}
bool wld_pickup_item(struct wld_mob *m, struct wld_item *i)
{
	int slot = wld_get_open_inventory_slot(m);
	if (slot != -1) {
		// set item copy in world to nonexist
		// move item from world to mob inventory
		m->map->item_map[i->map_index] = -1;
		m->map->items[i->id] = NULL;

		// copy item to mob inventory
		i->id = -1;
		i->map_index = -1;
		i->map_x = -1;
		i->map_y = -1;
		m->inventory[slot] = i;

		if (m->is_player && m->map->on_player_pickup_item)
			m->map->on_player_pickup_item(m->map, m, i);
	} else {
		if (m->is_player && m->map->on_player_pickup_item_fail)
			m->map->on_player_pickup_item_fail(m->map, m, i);
	}
}
void wld_swap_item(struct wld_mob* mob, int slot_a, int slot_b)
{
	struct wld_item *a = wld_get_item_in_slot(mob, slot_a);
	struct wld_item *b = wld_get_item_in_slot(mob, slot_b);
	mob->inventory[slot_b] = a;
	mob->inventory[slot_a] = b;
}
bool wld_mob_equip(struct wld_mob* mob, int itemslot)
{
	struct wld_item *i = wld_get_item_in_slot(mob, itemslot);
	if (i != NULL) {
		struct wld_itemtype *it = wld_get_itemtype(i->type);
		// weapon
		if (it->is_weq) {
			wld_swap_item(mob, itemslot, 0);
			return true;
		}
		// armor
		if (it->is_aeq) {
			wld_swap_item(mob, itemslot, 1);
			return true;
		}
	}
	return false;
}
bool wld_mob_unequip(struct wld_mob* mob, int itemslot)
{
	struct wld_item *i = wld_get_item_in_slot(mob, itemslot);
	if (i != NULL) {
		int open_slot = wld_get_open_inventory_slot(mob);
		if (open_slot != -1) {
			wld_swap_item(mob, itemslot, open_slot);
			return true;
		}
	}
	return false;
}
bool wld_mob_drop_item(struct wld_mob *mob, int itemslot)
{
	// TODO UH OH, HOW DO I RETURN AN ITEM TO THE MAP? LOOK FOR A NULL SLOT AND IF NOT REMALLOC?
}


///////////////////////////
// MOB AI

void ai_default_wander(struct wld_mob *mob)
{
	mob->queue_x += 1;
}
bool ai_default_detect_combat(struct wld_mob *mob)
{
	// TODO
	return false && !mob->map->player->is_dead;
}
void ai_default_decide_combat(struct wld_mob *mob) // melee approach, melee attack
{
	// GET PLAYER (TODO in visible range, if can see, etc)
	if (wld_is_mob_nextto_mob(mob, mob->map->player)) {
		// attack target
		ai_mob_melee_mob(mob, mob->map->player);
		//mob->queue_target = mob->map->player->map_index;
	} else {
		int x = mob->map->player->map_x;
		int y = mob->map->player->map_y;
		// move to target
		if (x < mob->map_x)
			mob->queue_x += -1;
		else if (x > mob->map_x)
			mob->queue_x += 1;
		if (y < mob->map_y)
			mob->queue_y += -1;
		else if (y > mob->map_y)
			mob->queue_y += 1;
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
	if (aggressor->is_player && aggressor->map->on_player_kill_mob)
		aggressor->map->on_player_kill_mob(aggressor->map, aggressor, defender);
}
void ai_mob_attack_mob(struct wld_mob *aggressor, struct wld_mob *defender, int amt)
{
	defender->health -= amt;

	// TODO pass damage amount, attack type, etc
	if (!defender->is_player && aggressor->map->on_mob_kill_mob)
		aggressor->map->on_mob_attack_mob(aggressor->map, aggressor, defender);
	if (defender->is_player && aggressor->map->on_mob_attack_player)
		aggressor->map->on_mob_attack_player(aggressor->map, aggressor, defender);
	if (aggressor->is_player && aggressor->map->on_player_attack_mob)
		aggressor->map->on_player_attack_mob(aggressor->map, aggressor, defender);

	if (defender->health <= 0)
		ai_mob_kill_mob(aggressor, defender);

}
// triggered from inputs that request attack
// Bool returns if it was a valid option?
bool ai_can_melee(struct wld_mob *aggressor, struct wld_mob *defender)
{
	return !defender->is_dead && wld_is_mob_nextto_mob(aggressor, defender);
}
void ai_mob_melee_mob(struct wld_mob *aggressor, struct wld_mob *defender)
{
	ai_mob_attack_mob(aggressor, defender, 34);
}
bool ai_player_attack_melee(struct wld_mob* player)
{
	struct wld_tile *tile = wld_gettileat_index(player->map, player->cursor_target);
	if (tile->is_visible) {
		struct wld_mob *target = wld_getmobat_index(player->map, player->cursor_target);
		// make sure its another valid mob
		if (target != NULL && ai_can_melee(player, target)) {
			ai_mob_melee_mob(player, target);
			return true;
		}
	}
	return false;
}
bool ai_queuemobmove(struct wld_mob *mob, int relx, int rely)
{
	int newx = mob->map_x + relx;
	int newy = mob->map_y + rely;
	if (wld_canmoveto(mob->map, newx, newy)) {
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

	struct wld_tile *tile = wld_gettileat(mob->map, newx, newy);
	struct wld_mob *mob2 = wld_getmobat(mob->map, newx, newy);
	if (mob2 != NULL) {
		if (!mob2->is_dead) {
			ai_mob_attack_mob(mob, mob2, 34);
			return true;
		} else {
			// ai_search_mob TODO
			// ai_abberation_mob
			return false;
		}
	} else {
		// TODO is transition
		if (wld_canmoveto(mob->map, newx, newy)) {
			return ai_queuemobmove(mob, relx, rely);
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
	struct wld_item *i = wld_getitemat(mob->map, posx, posy);

	if (i != NULL)
		if (wld_pickup_item(mob, i))
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
	wld_movemob(mob, mob->queue_x, mob->queue_y);
	mob->queue_x = 0;
	mob->queue_y = 0;

	if (mob->queue_target > -1) {
		// attack enemy
		struct wld_mob* target = wld_getmobat_index(mob->map, mob->queue_target);
		if (target != NULL) {
			//ai_mob_attack_mob(mob, target);
		}
	}
	mob->queue_target = -1;
}


///////////////////////////
// ITEM ACTIONS
void itm_on_use_melee(struct wld_item *item, struct wld_mob *user)
{
}
void itm_on_fire_melee(struct wld_item *item, struct wld_mob *user, int mapx, int mapy)
{
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
	int *item_map_array = (int*)malloc(map->length * sizeof(int));

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
			tile->type = TILE_GRASS;
			break;
		}

		tile_map_array[i] = i; // set tile map to this tile id
		mob_map_array[i] = -1; // initialize to -1 for "no mob"
		item_map_array[i] = -1; // no item
	}
	map->tile_map = tile_map_array;
	map->mob_map = mob_map_array;
	map->item_map = item_map_array;
}
void wld_genmobs(struct wld_map *map)
{
	// hardcoded to 3 mobs for now
	int mob_count = 2;
	map->mobs = (struct wld_mob**)malloc(mob_count * sizeof(struct wld_mob));
	map->mobs_length = mob_count;

	// setup mobs in mob map
	for (int i=0; i < mob_count; i++) {
		// create mob
		struct wld_mob *mob = (struct wld_mob*)malloc(sizeof(struct wld_mob));
		// create reference to parent map
		mob->id = i;
		mob->map = map;
		mob->state = MS_START;
		mob->queue_x = 0;
		mob->queue_y = 0;
		mob->queue_target = -1;
		mob->health = 100;
		mob->maxhealth = 100; // TODO define based on things
		mob->ai_wander = NULL;
		mob->ai_detect_combat = NULL;
		mob->ai_decide_combat = NULL;
		mob->ai_player_input = NULL;
		mob->cursor_target = -1;
		mob->mode = MODE_PLAY;
		mob->target_mode = TMODE_NONE;
		mob->is_dead = false;

		// create inventory (pointers to malloc items)
		mob->inventory = (struct wld_item**)malloc(INVENTORY_SIZE * sizeof(struct wld_item*));
		for (int j=0; j < INVENTORY_SIZE; j++) {
			mob->inventory[j] = NULL;
		}

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
		} else {
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
		map->mobs[i] = mob;
	}
}
void wld_genitems(struct wld_map *map)
{
	// items are malloc'd pointers, not structs in map memory because they need to be moved
	int item_count = 3;
	map->items = (struct wld_item**)malloc(MALLOC_ITEM_SIZE * sizeof(struct wld_item*));
	map->items_length = item_count;

	// TODO we need to work on assigning items to players, mobs etc
	// setup items in item map
	for (int i=0; i < MALLOC_ITEM_SIZE; i++) {
		// create item
		struct wld_item *item;
		// create reference to parent map
		if (i == 0) {
			// put item near player
			item = (struct wld_item*)malloc(sizeof(struct wld_item));
			item->id = i;
			item->map_x = map->cols / 2 + 4;
			item->map_y = map->rows / 2 + 2;
			item->map_index = wld_calcindex(item->map_x, item->map_y, map->cols);
			item->type = ITEM_WEAPON_SHORTSWORD;
			wld_insert_item(map, item, item->map_x, item->map_y, item->id);
		} else if (i == 1) {
			item = (struct wld_item*)malloc(sizeof(struct wld_item));
			item->id = i;
			item->map_x = map->cols / 2 + 6;
			item->map_y = map->rows / 2 + 2;
			item->map_index = wld_calcindex(item->map_x, item->map_y, map->cols);
			item->type = ITEM_WEAPON_SHORTBOW;
			wld_insert_item(map, item, item->map_x, item->map_y, item->id);
		} else if (i == 2) {
			item = (struct wld_item*)malloc(sizeof(struct wld_item));
			item->id = i;
			item->map_x = map->cols / 2 + 7;
			item->map_y = map->rows / 2 - 1;
			item->map_index = wld_calcindex(item->map_x, item->map_y, map->cols);
			item->type = ITEM_ARMOR_LEATHER;
			wld_insert_item(map, item, item->map_x, item->map_y, item->id);
		} else {
			map->items[i] = NULL;
		}

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
	map->on_player_attack_mob = NULL;
	map->on_player_kill_mob = NULL;
	map->on_player_pickup_item = NULL;
	map->on_player_pickup_item_fail = NULL;

	// populate tiles
	wld_gentiles(map);

	// build an populate map with mobs
	wld_genmobs(map);

	// build items
	wld_genitems(map);

	return map;
}
void wld_delmap(struct wld_map *map)
{
	free(map->cursor);

	for (int i=0; i<map->items_length; i++)
		if (map->items[i] != NULL)
			free(map->items[i]);
	free(map->items);
	free(map->item_map);

	for (int i=0; i<map->mobs_length; i++) {
		for (int j=0; j<INVENTORY_SIZE; j++)
			if (map->mobs[i]->inventory[j] != NULL)
				free(map->mobs[i]->inventory[j]);
		free(map->mobs[i]->inventory);
	}
	for (int i=0; i<map->mobs_length; i++)
		if (map->mobs[i] != NULL)
			free(map->mobs[i]);
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
		{ TILE_GRASS,           '"', 0, 1, '"', 0, 4, false, "a small tuft of grass" }, // 1
		{ TILE_WATER,           ' ', 4, 2, ' ', 0, 0, false, "a pool of water glistens" }, // 2
		{ TILE_TREE,            'T', 0, 1, 'T', 0, 4, true,  "a large tree" }, // 3
		{ TILE_STONEWALL,       '#', 2, 2, '#', 0, 4, true,  "rough stone wall" }, // 4
		{ TILE_STONEFLOOR,      '.', 0, 2, '.', 0, 4, false, "rough stone floor" }, // 5
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
		{ MOB_VOID,	' ', 0, "" },
		{ MOB_PLAYER,	'@', 7, "yourself", "You"},
		{ MOB_BUGBEAR,	'b', 3, "a small bugbear", "small bugbear" },
	};
	wld_mobtypes = (struct wld_mobtype*)malloc(ARRAY_SIZE(mts) * sizeof(struct wld_mobtype));
	for (int i=0; i<ARRAY_SIZE(mts); i++) {
		wld_mobtypes[i].type = mts[i].type;
		wld_mobtypes[i].sprite = mts[i].sprite;
		wld_mobtypes[i].fg_color = mts[i].fg_color;
		wld_mobtypes[i].short_desc = mts[i].short_desc;
		wld_mobtypes[i].title = mts[i].title;
	}

	// copy item types into malloc
	struct wld_itemtype its [] = {																	/////////////////////////////////////////////////////////	/////////////////////////////////////////////////////////
		{ ITEM_VOID,			' ', CLX_BLACK,		TARGET_PASSIVE, false, false, "", "", NULL, NULL, ""},
		{ ITEM_POTION_MINOR_HEAL,	'i', CLX_YELLOW,	TARGET_SELF, false, false, "a potion of minor healing", "minor healing potion", NULL, NULL,	"The glass of the potion is warm to the touch, its",		"properties should heal a small amount." },
	 	{ ITEM_WEAPON_SHORTSWORD,	'/', CLX_YELLOW,	TARGET_MELEE, true, false, "a shortsword", "shortsword", itm_on_use_melee, itm_on_fire_melee,	"Though short, its sharp point could plunge deeply into",	"a soft skinned enemy." },
		{ ITEM_WEAPON_SHORTBOW,		')', CLX_YELLOW,	TARGET_RANGED_LOS, true, false, "a shortbow", "shortbow", NULL, NULL,				"Its string has been worn but the wood is strong, this",	"small bow could fell small creatures" },
		{ ITEM_SCROLL_FIREBOMB,		'=', CLX_YELLOW,	TARGET_RANGED_LOS_AOE, false, false, "a scroll of firebomb", "scroll of firebomb", NULL, NULL,	"Runic art covers the parchment surface showing a",		"large swathe of fire." },
		{ ITEM_ARMOR_LEATHER,		'M', CLX_YELLOW,	TARGET_PASSIVE, false, true, "a set of leather armor", "leather armor", NULL, NULL,		"Humble but sturdy this set of leather armor is a rogue's",	"favorite friend." },
	};
	wld_itemtypes = (struct wld_itemtype*)malloc(ARRAY_SIZE(its) * sizeof(struct wld_itemtype));
	for (int i=0; i<ARRAY_SIZE(its); i++) {
		wld_itemtypes[i].type = its[i].type;
		wld_itemtypes[i].sprite = its[i].sprite;
		wld_itemtypes[i].fg_color = its[i].fg_color;
		wld_itemtypes[i].target_type = its[i].target_type;
		wld_itemtypes[i].is_weq = its[i].is_weq;
		wld_itemtypes[i].is_aeq = its[i].is_aeq;
		wld_itemtypes[i].short_desc = its[i].short_desc;
		wld_itemtypes[i].title = its[i].title;
		wld_itemtypes[i].on_use = its[i].on_use;
		wld_itemtypes[i].on_fire = its[i].on_fire;
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
