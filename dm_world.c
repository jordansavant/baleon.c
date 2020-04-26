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

#define WLD_COLOR_BASE 100
#define WLD_COLOR_COUNT 8
enum WLD_COLOR_INDEX {
	WCLR_BLACK,   // 0
	WCLR_GREEN,   // 1
	WCLR_WHITE,   // 2
	WCLR_RED,     // 3
	WCLR_BLUE,    // 4
	WCLR_CYAN,    // 5
	WCLR_YELLOW,  // 6
	WCLR_MAGENTA, // 7
};
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
#define MALLOC_MOB_SIZE 10
void wld_insert_item(struct wld_map* map, struct wld_item* item, int x, int y, int id)
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
	wld_insert_item(map, item, x, y, map->items_length);

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
			item->id = i - 1;
			map->items[item->id] = next_item;
			// his id is at his position in the map, we need to point that to his new id
			map->item_map[next_item->map_index] = item->id;
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
void wld_insert_mob(struct wld_map* map, struct wld_mob* mob, int x, int y, int id)
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
	wld_insert_mob(map, mob, x, y, map->mobs_length);

	// increment our list
	map->mobs_length++;
}
// This is called when we don't want to destroy a mob, just remove it from map ownership
void wld_map_remove_mob(struct wld_map* map, struct wld_mob* mob)
{
	bool found = false;
	for (int i=0; i<map->mobs_length; i++) {
		if (found) {
			// shift backwards a spot
			struct wld_mob *next_mob = map->mobs[i];
			mob->id = i - 1;
			map->mobs[mob->id] = next_mob;
			// his id is at his position in the map, we need to point that to his new id
			map->mob_map[next_mob->map_index] = mob->id;
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

///////////////////////////
// TILE EVENTS
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
int wld_distance_mob_tile(struct wld_map *map, struct wld_mob *mob, struct wld_tile *tile)
{
	int mx = mob->map_x;
	int my = mob->map_y;
	int tx = tile->map_x;
	int ty = tile->map_y;
	return dm_disti(mx, my, tx, ty);
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
	if (map->tiles[tile_id].type->is_block)
		return false;

	return true;
}
void wld_teleportmob(struct wld_mob *mob, int x, int y, bool trigger_events)
{
	if (wld_canmoveto(mob->map, x, y)) {
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
			struct wld_tile* tile = wld_gettileat(mob->map, mob->map_x, mob->map_y);
			if (tile->on_mob_enter)
				tile->on_mob_enter(mob->map, tile, mob);

			// if player, collect items he walks over unless he dropped it before
			struct wld_item *item = wld_getitemat(mob->map, mob->map_x, mob->map_y);
			if (mob->is_player && item != NULL && !item->has_dropped && wld_mob_has_inventory(mob)) {
				wld_mob_pickup_item(mob, item);
			}
		}
	}
}
void wld_movemob(struct wld_mob *mob, int relx, int rely, bool trigger_events)
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
		struct wld_tile *t1 = wld_gettileat(mob->map, newx, newy - ry);
		struct wld_tile *t2 = wld_gettileat(mob->map, newx - rx, newy);
		if ((t1 && t1->type->is_block) || (t2 && t2->type->is_block)) {
			return;
		}
	}

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

		if (trigger_events) {
			// run tile on enter event
			struct wld_tile* tile = wld_gettileat(mob->map, mob->map_x, mob->map_y);
			if (tile->on_mob_enter)
				tile->on_mob_enter(mob->map, tile, mob);

			// if player, collect items he walks over unless he dropped it before
			struct wld_item *item = wld_getitemat(mob->map, mob->map_x, mob->map_y);
			if (mob->is_player && item != NULL && !item->has_dropped && wld_mob_has_inventory(mob)) {
				wld_mob_pickup_item(mob, item);
			}
		}
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
		map->player->cursor_target_index = map->cursor->index;
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
		return t->type->is_block;
	}
	void wld_ss_onvisible(int x, int y, double radius)
	{
		on_see(mob, x, y, radius);
	}
	dm_shadowcast(mob->map_x, mob->map_y, map->cols, map->rows, 20, wld_ss_isblocked, wld_ss_onvisible, false); // no leakage allowed
}
struct draw_struct wld_get_drawstruct(struct wld_map *map, int x, int y)
{
	struct wld_tile *t = wld_gettileat(map, x, y);
	unsigned long cha = t->type->sprite;
	int mob_id = map->mob_map[t->map_index];
	int item_id = map->item_map[t->map_index];

	int colorpair;
	if (mob_id > -1) {
		// if mob use its fg sprite and fg color
		struct wld_mob *m = wld_getmobat(map, x, y);
		if (m->type->sprite != ' ')
			cha = m->type->sprite;
		colorpair = wld_cpair_tm(t->type_id, m->type_id);
	} else if(item_id > -1) {
		// if item  use its fg sprite and fg color
		struct wld_item *i = wld_getitemat(map, x, y);
		if (i->type->sprite != ' ')
			cha = i->type->sprite;
		colorpair = wld_cpair_ti(t->type_id, i->type_id);
	} else {
		colorpair = wld_cpair_tm(t->type_id, 0);
	}

	struct draw_struct ds = { colorpair, cha };
	return ds;
}
struct draw_struct wld_get_memory_drawstruct(struct wld_map *map, int x, int y)
{
	// memory we do not look at mob data
	struct wld_tile *t = wld_gettileat(map, x, y);
	unsigned long cha = t->type->memory_sprite;
	int colorpair = wld_cpairmem(t->type_id);

	struct draw_struct ds = { colorpair, cha };
	return ds;
}
bool wld_mob_nextto_mob(struct wld_mob *ma, struct wld_mob *mb)
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
		wld_map_remove_item(m->map, i);

		// copy item to mob inventory
		m->inventory[slot] = i;

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
	if (item != NULL && wld_getitemat(mob->map, mob->map_x, mob->map_y) == NULL) {
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
void wld_swap_item(struct wld_mob* mob, int slot_a, int slot_b)
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
			wld_swap_item(mob, itemslot, 0);
			return true;
		}
		// armor
		if (i->type->is_aeq) {
			wld_swap_item(mob, itemslot, 1);
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
			wld_swap_item(mob, itemslot, open_slot);
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
		struct wld_tile *t = wld_gettileat(mob->map, spx, spy);
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


///////////////////////////
// CHEATS
void wld_cheat_teleport_exit(struct wld_map *map, struct wld_mob* mob)
{
	struct wld_tile* exit_tile = map->exit_tile;
	wld_teleportmob(mob, exit_tile->map_x, exit_tile->map_y, false);
}



///////////////////////////
// RPG CALCULATIONS
int rpg_calc_melee_dmg(struct wld_mob *aggressor, struct wld_mob *defender)
{
	return 34;
}
double rpg_calc_melee_coh(struct wld_mob *aggressor, struct wld_mob *defender)
{
	return .66;
}
int rpg_calc_ranged_dmg(struct wld_mob *aggressor, struct wld_mob *defender)
{
	return 34;
}
double rpg_calc_ranged_coh(struct wld_mob *aggressor, struct wld_mob *defender)
{
	return .66;
}
int rpg_calc_range_dist(struct wld_mob *aggressor, int base_range)
{
	// TODO will calculate strength and dex etc
	// TODO break into options for throwing range vs shooting range?
	return base_range;
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
	if (wld_mob_nextto_mob(mob, mob->map->player)) {
		// attack target
		ai_mob_melee_mob(mob, mob->map->player);
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
void ai_mob_heal(struct wld_mob *mob, int amt, struct wld_item* item) // item can be NULL if it was not an item
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
	return !defender->is_dead && wld_mob_nextto_mob(aggressor, defender);
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
		struct wld_tile *tile = wld_gettileat_index(defender->map, defender->map_index);
		if (weapon->type->fn_can_use(weapon, aggressor, tile)) {
			weapon->type->fn_use(weapon, aggressor, tile);
			// TODO item uses?
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

	struct wld_tile *tile = wld_gettileat_index(player->map, player->cursor_target_index);
	return ai_mob_use_item(player, player->active_item, tile);
}
bool ai_player_trigger_target(struct wld_mob* player)
{
	// this is run whenever in a target mode
	// Melee can be here without an active item
	if (player->active_item != NULL)
		return ai_player_use_active_item(player);

	// else do a fist attack
	struct wld_mob *target = wld_getmobat_index(player->map, player->cursor_target_index);
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
			ai_mob_melee_mob(mob, mob2);
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
	wld_movemob(mob, mob->queue_x, mob->queue_y, true);
	mob->queue_x = 0;
	mob->queue_y = 0;
}





///////////////////////////
// ITEM ACTIONS

/////////////
// MELEE
void itm_target_melee(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int))
{
	// spiral
	wld_mob_inspect_melee(user, inspect);
}
bool itm_can_use_melee(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	struct wld_mob *target = wld_getmobat_index(user->map, cursor_tile->map_index);
	if (target != NULL && ai_can_melee(user, target))
		return true;
	return false;
}
void itm_use_melee(struct wld_item *weapon, struct wld_mob *user, struct wld_tile* cursor_tile)
{
	// Using a standard melee item just fires on the targeted tile without any calculation
	struct wld_mob *target = wld_getmobat_index(user->map, cursor_tile->map_index);
	if (target != NULL)
		weapon->type->fn_hit(weapon, user, cursor_tile);
}
void itm_hit_melee_swordstyle(struct wld_item *weapon, struct wld_mob *user, struct wld_tile* tile)
{
	// TODO make this do a melee attack with the item's damage etc
	struct wld_mob *target = wld_getmobat_index(user->map, tile->map_index);
	double chance = rpg_calc_melee_coh(user, target);
	if (dm_randf() < chance) {
		int dmg = rpg_calc_melee_dmg(user, target);
		ai_mob_attack_mob(user, target, dmg, weapon);
	} else {
		// whiff event
		ai_mob_whiff_mob(user, target, weapon);
	}
}

/////////////
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
		struct wld_tile *t = wld_gettileat(user->map, x, y);

		// stop at tiles outside of the range of the item
		int dist = dm_disti(user->map_x, user->map_y, x, y);
		if (dist > allowed_range)
			return true;

		return t->type->is_block || !t->is_visible;
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
		struct wld_mob *mob = wld_getmobat(user->map, x, y);
		if (mob != NULL && !mob->is_dead) {
			item->type->fn_hit(item, user, wld_gettileat(user->map, x, y));
			hit_target = true;
			return true; // stop inspection
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
	struct wld_mob *target = wld_getmobat_index(user->map, tile->map_index);
	double chance = rpg_calc_ranged_coh(user, target);
	if (dm_randf() < chance) {
		int dmg = rpg_calc_ranged_dmg(user, target);
		ai_mob_attack_mob(user, target, dmg, item);
	} else {
		// whiff event
		ai_mob_whiff_mob(user, target, item);
	}
}


/////////////
// POTIONS
void itm_drink_minorhealth(struct wld_item *item, struct wld_mob *user)
{
	// TODO minor vs major healing levels
	int hp = dm_randf() * 10;
	ai_mob_heal(user, hp, item);
}
void itm_hit_minorhealth(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile)
{
	struct wld_mob *target = wld_getmobat_index(user->map, tile->map_index);
	int hp = dm_randf() * 10;
	ai_mob_heal(target, hp, item);
}







///////////////////////////
// MAP INITIALIZATION

void wld_gentiles(struct wld_map *map, struct dng_cellmap* cellmap)
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
			tile->on_mob_enter = NULL;

			// TODO more
			if (cell->is_wall) {
				tile->type_id = TILE_STONEWALL;
				tile->type = &wld_tiletypes[TILE_STONEWALL];
			} else if (cell->is_door) {
				tile->type_id = TILE_STONEDOOR;
				tile->type = &wld_tiletypes[TILE_STONEDOOR];
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
				tile->type_id = TILE_STONEFLOOR;
				tile->type = &wld_tiletypes[TILE_STONEFLOOR];
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
void wld_initmob(struct wld_mob *mob, enum WLD_MOBTYPE type)
{
	mob->state = MS_START;
	mob->queue_x = 0;
	mob->queue_y = 0;
	mob->health = 100;
	mob->maxhealth = 100; // TODO define based on things
	mob->ai_wander = NULL;
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
	mob->type_id = type;
	mob->type = &wld_mobtypes[type];

	// create inventory (pointers to malloc items)
	mob->inventory = (struct wld_item**)malloc(INVENTORY_SIZE * sizeof(struct wld_item*));
	for (int j=0; j < INVENTORY_SIZE; j++) {
		mob->inventory[j] = NULL;
	}
}
void wld_genmobs(struct wld_map *map, struct dng_cellmap* cellmap)
{
	// TODO the cellmap should keep alist of mob counts so we can allocate for it
	// only the player for now
	map->mobs = NULL;
	map->mobs_length = 0;

	for (int r = 0; r < cellmap->height; r++) { // rows
		for (int c=0; c < cellmap->width; c++){ // cols
			// get cell from map
			int index = r * cellmap->width + c;
			struct dng_cell *cell = cellmap->cells[index];

			// Spawn player and first dungeon entrance
			if (cell->is_entrance_transition && map->is_first_map) {
				struct wld_mob *mob = (struct wld_mob*)malloc(sizeof(struct wld_mob));
				wld_initmob(mob, MOB_PLAYER);
				wld_map_new_mob(map, mob, c, r);
				mob->is_player = true;
				map->player = mob; // assign to map specifically

				// set cursor nearby
				map->cursor->x = mob->map_x + 2;
				map->cursor->y = mob->map_y;
				map->cursor->index = wld_calcindex(map->cursor->x, map->cursor->y, map->cols);

			} else if (cell->has_mob) {
				struct wld_mob *mob = (struct wld_mob*)malloc(sizeof(struct wld_mob));
				wld_initmob(mob, MOB_BUGBEAR); // TODO make this randomized and tuned for difficulty
				wld_map_new_mob(map, mob, c, r);
				mob->is_player = false;
				mob->ai_wander = ai_default_wander;
				mob->ai_detect_combat = ai_default_detect_combat;
				mob->ai_decide_combat = ai_default_decide_combat;
			}
		}
	}
}
void wld_inititem(struct wld_item* item, enum WLD_ITEMTYPE type)
{
	item->type_id = type;
	item->type = &wld_itemtypes[type];
	item->has_dropped = false;
	item->uses = wld_itemtypes[type].base_uses;
}
void wld_genitems(struct wld_map *map, struct dng_cellmap* cellmap)
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
				switch (dm_randii(0, 4)) {
				case 0:
					wld_inititem(item, ITEM_WEAPON_SHORTSWORD);
					wld_map_new_item(map, item, c, r);
					break;
				case 1:
					wld_inititem(item, ITEM_WEAPON_SHORTBOW);
					wld_map_new_item(map, item, c, r);
					break;
				case 2:
					wld_inititem(item, ITEM_POTION_MINOR_HEAL);
					wld_map_new_item(map, item, c, r);
					break;
				case 3:
					wld_inititem(item, ITEM_ARMOR_LEATHER);
					wld_map_new_item(map, item, c, r);
					break;
				}
			}
		}
	}
}
struct wld_map* wld_newmap(int id, int difficulty, int width, int height)
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

struct wld_world* wld_newworld(int seed, int count)
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
		struct wld_map* map = wld_newmap(i, cellmap->difficulty, cellmap->width, cellmap->height);
		map->world = world;
		map->is_first_map = (i == 0);

		// populate
		wld_gentiles(map, cellmap);
		wld_genmobs(map, cellmap);
		wld_genitems(map, cellmap);

		world->maps[i] = map;
	}

	dng_deldungeon(dungeon);

	return world;
}

void wld_delworld(struct wld_world* world)
{
	for (int i=0; i < world->maps_length; i++) {
		wld_delmap(world->maps[i]);
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


///////////////////////////
// WORLD INITALIZATION

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
		{ TILE_WATER,           ' ', WCLR_BLUE,  WCLR_WHITE, ' ', WCLR_BLACK, WCLR_BLACK, false, "a pool of water glistens" },
		{ TILE_TREE,            'T', WCLR_BLACK, WCLR_GREEN, 'T', WCLR_BLACK, WCLR_BLUE,  true,  "a large tree" },
		{ TILE_STONEWALL,       '#', WCLR_WHITE, WCLR_WHITE, '#', WCLR_BLACK, WCLR_BLUE,  true,  "rough stone wall" },
		{ TILE_STONEFLOOR,	'.', WCLR_BLACK, WCLR_WHITE, '.', WCLR_BLACK, WCLR_BLUE,  false, "rough stone floor" },
		{ TILE_ENTRANCE,	'>', WCLR_BLACK, WCLR_CYAN,  '>', WCLR_BLACK, WCLR_CYAN,  false, "the entrance back up" },
		{ TILE_EXIT,		'<', WCLR_BLACK, WCLR_CYAN,  '<', WCLR_BLACK, WCLR_CYAN,  false, "an exit further down" },
		{ TILE_STONEDOOR,       '+', WCLR_BLACK, WCLR_WHITE, '+', WCLR_BLACK, WCLR_BLUE,  false,  "an old stone door" },
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
		{ MOB_VOID,	' ', WCLR_BLACK,   "" },
		{ MOB_PLAYER,	'@', WCLR_MAGENTA, "yourself", "You"},
		{ MOB_BUGBEAR,	'b', WCLR_RED,     "a small bugbear", "small bugbear" },
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
	struct wld_itemtype its [] = {
		{
			ITEM_VOID,
			' ',
			WCLR_BLACK,
			false,
			false,
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
			false,
			false,
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
			true,
			false,
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
			true,
			false,
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
			"Its string has been worn but the wood is strong,this",
			"small bow could fell small creatures"
		},
		{
			ITEM_SCROLL_FIREBOMB,
			'=',
			WCLR_YELLOW,
			false,
			false,
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
			false,
			true,
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
	};
	wld_itemtypes = (struct wld_itemtype*)malloc(ARRAY_SIZE(its) * sizeof(struct wld_itemtype));
	for (int i=0; i<ARRAY_SIZE(its); i++) {
		wld_itemtypes[i].type = its[i].type;
		wld_itemtypes[i].sprite = its[i].sprite;
		wld_itemtypes[i].fg_color = its[i].fg_color;
		wld_itemtypes[i].is_weq = its[i].is_weq;
		wld_itemtypes[i].is_aeq = its[i].is_aeq;
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



