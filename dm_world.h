#ifndef DM_WORLD
#define DM_WORLD

#include "dm_defines.h"
#include "dm_dungeon.h"

#define INVENTORY_SIZE 12

#define STAT_STR_BASE 10
#define STAT_DEX_BASE 10
#define STAT_CON_BASE 10

#define MAX_ACTIVE_EFFECTS 10
#define MAX_MUTATIONS 5
#define MAX_MUTATION_DESC_LEN 35
#define MAX_ABERRATIONS 20

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

struct wld_item;
struct wld_mob;
struct wld_mob_type;
struct wld_effect;

enum WLD_VISUAL_EFFECT {
	VFX_HEAL,
	VFX_DMG_HIT,
	VFX_SUMMON,
};

struct wld_vfx {
	enum WLD_VISUAL_EFFECT type;
	int x, y;
	int radius;
};


///////////////////////////
// ABERRATIONS
struct wld_mutation {
	char desc[MAX_MUTATION_DESC_LEN];
	void(*on_apply)(struct wld_mob*);
	void(*on_update)(struct wld_mob*);
};
struct wld_aberration {
	int id; // list index
	struct wld_mutation mutations[MAX_MUTATIONS];
	int mutations_length;
};


///////////////////////////
// EFFECTS LIKE FIRE

enum WLD_EFFECT {
	EFFECT_FIRE,
};

struct wld_effecttype {
	enum WLD_EFFECT type;
	int iterations;
	unsigned long sprite;
	int fg_color, bg_color;
	void (*on_update_mob)(struct wld_effect *e, struct wld_mob *);
	char *title;
};

struct wld_effect {
	struct wld_effecttype *type;
	bool is_active;
	int current_iterations;
};


///////////////////////////
// TILE STRUCTS

enum WLD_TILETYPE {
	TILE_VOID = 0,
	TILE_GRASS = 1,
	TILE_WATER = 2,
	TILE_TREE = 3,
	TILE_STONEWALL = 4,
	TILE_STONEFLOOR = 5,
	TILE_ENTRANCE,
	TILE_EXIT,
	TILE_STONEDOOR,
	TILE_DEEPWATER,
	TILE_SUMMONCIRCLE_SF_INERT,
	TILE_SUMMONCIRCLE_ACTIVE,
	TILE_SUMMONCIRCLE_NODE,
};
struct wld_tiletype {
	enum WLD_TILETYPE type;
	unsigned long sprite;
	int bg_color;
	int fg_color;
	unsigned long memory_sprite;
	int memory_bg_color;
	int memory_fg_color;
	bool is_block;
	bool is_transformable;
	char *short_desc;
};
struct wld_tile {
	int id; // position in tile list
	int map_x, map_y, map_index;
	struct wld_tiletype *type;
	struct wld_map *map;
	bool is_visible;
	bool was_visible;
	bool is_blocked;
	bool is_door;
	bool is_door_open;
	bool is_door_locked;
	int door_lock_id;
	struct wld_mobtype* dead_mob_type;
	// on_enter, on_leave events
	void(*on_mob_enter)(struct wld_map*, struct wld_tile*, struct wld_mob*); // TODO left off here
	unsigned int dm_ss_id; // shadowcast id
};


///////////////////////////
// MOB STRUCTS

enum WLD_MOBTYPE {
	MOB_VOID = 0,
	MOB_PLAYER = 1,
	MOB_JACKAL = 2,
	MOB_RAT = 3,
};
enum WLD_MOB_STATE {
	MS_START,
	MS_DEAD,
	MS_WANDER,
	MS_COMBAT,
	MS_SLEEP,
	MS_HUNT,
};
enum MODE {
	MODE_PLAY,
	MODE_INVENTORY,
	MODE_USE,
	MODE_ABERRATE,
};
enum TARGET_MODE {
	TMODE_NONE,
	TMODE_ACTIVE,

	TMODE_SELF,
	TMODE_MELEE,
	TMODE_RANGED_LOS,
	TMODE_RANGED_TEL,
	TMODE_RANGED_LOS_AOE,
	TMODE_RANGED_TEL_AOE,
};
struct wld_mobtype {
	enum WLD_MOBTYPE type;
	int base_health;
	unsigned long sprite;
	int fg_color;
	char *short_desc;
	char *title;
};
struct wld_mob {
	int id; // positin in maps mob list
	int map_x, map_y, map_index; // position in map geo and index
	struct wld_mobtype *type;
	struct wld_map *map;
	enum WLD_MOB_STATE state;
	void (*ai_wander)(struct wld_mob*);
	bool (*ai_is_hostile)(struct wld_mob*, struct wld_mob*);
	bool (*ai_detect_combat)(struct wld_mob*);
	void (*ai_decide_combat)(struct wld_mob*);
	void (*ai_player_input)(struct wld_mob*);
	int queue_x, queue_y;
	int health, maxhealth;
	int vision, basevision;
	bool is_player, is_dead;
	int cursor_target_index; // map index
	enum MODE mode;
	enum TARGET_MODE target_mode;
	struct wld_item **inventory;
	int target_x, target_y;
	struct wld_item *active_item;
	bool is_destroy_queued;

	int stat_strength;
	int stat_dexterity;
	int stat_constitution;

	struct wld_effect active_effects[MAX_ACTIVE_EFFECTS];
	int active_effects_length;

	bool can_aberrate;
	struct wld_aberration **aberrations;
	int aberrations_length;
	struct wld_aberration *current_aberration;
};


///////////////////////////
// ITEM STRUCTS
enum WLD_ITEMTYPE {
	ITEM_VOID,
	ITEM_POTION_MINOR_HEAL,
	ITEM_WEAPON_SHORTSWORD,
	ITEM_WEAPON_SHORTBOW,
	ITEM_SCROLL_FIREBOMB,
	ITEM_ARMOR_LEATHER,
	ITEM_KEY_BASIC,
};
struct wld_itemtype {
	enum WLD_ITEMTYPE type;
	unsigned long sprite;
	int fg_color;
	bool is_weq, is_aeq, is_key;
	char *short_desc;
	char *title;
	void (*fn_drink)(struct wld_item*, struct wld_mob*);
	void (*fn_target)(struct wld_item*, struct wld_mob*, void (*inspect)(int,int));
	bool (*fn_can_use)(struct wld_item*, struct wld_mob*, struct wld_tile*);
	void (*fn_use)(struct wld_item*, struct wld_mob*, struct wld_tile*);
	void (*fn_hit)(struct wld_item*, struct wld_mob*, struct wld_tile*);
	int base_range;
	int base_radius;
	bool has_uses;
	int base_uses;
	int min_val, max_val;
	char *drink_label;
	char *use_label;
	char *use_text_1;
	char *use_text_2;
};
struct wld_item {
	int id;
	int map_x, map_y, map_index;
	struct wld_itemtype *type;
	bool has_dropped;
	int uses;
	int key_id;
	int map_found;
};


///////////////////////////
// WORLD STRUCTS

struct wld_world;
struct wld_cursor {
	int x;
	int y;
	int index;
};
struct wld_map {
	int id;
	int rows;
	int cols;
	int length;
	int difficulty;
	struct wld_world* world;
	bool is_first_map;
	int *tile_map; // array of tile types
	struct wld_tile *tiles;
	unsigned int tiles_length;
	int *mob_map; // array of mob ids in mob listing
	struct wld_mob **mobs;
	unsigned int mobs_length;
	unsigned int mobs_capacity; // to manage list mallocs
	int *item_map;
	struct wld_item **items;
	unsigned int items_length;
	unsigned int items_capacity; // to manage list mallocs
	struct wld_mob *player;
	struct wld_cursor *cursor;
	struct wld_tile* entrance_tile;
	struct wld_tile* exit_tile;

	// function pointers game subscribes to for events
	void (*on_effect)(struct wld_map *map, struct wld_vfx *effect);

	void (*on_player_map_transition)(struct wld_map*, struct wld_mob *mob, bool forward);
	void (*on_cursormove)(struct wld_map*, int x, int y, int index);
	void (*on_playermove)(struct wld_map*, struct wld_mob *, int x, int y, int index);

	void (*on_mob_heal)(struct wld_map*, struct wld_mob *mob, int amt, struct wld_item* item);
	void (*on_mob_attack_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, int dmg, struct wld_item* item);
	void (*on_mob_attack_player)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, int dmg, struct wld_item* item);
	void (*on_mob_whiff)(struct wld_map*, struct wld_mob *agg, struct wld_item* item);
	void (*on_mob_whiff_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, struct wld_item* item);
	void (*on_mob_whiff_player)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, struct wld_item* item);
	void (*on_mob_kill_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, struct wld_item* item);
	void (*on_mob_kill_player)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, struct wld_item* item);

	void (*on_player_heal)(struct wld_map*, struct wld_mob *mob, int amt, struct wld_item* item);
	void (*on_player_attack_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, int dmg, struct wld_item* item);
	void (*on_player_whiff)(struct wld_map*, struct wld_mob *agg, struct wld_item* item);
	void (*on_player_whiff_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, struct wld_item* item);
	void (*on_player_kill_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def, struct wld_item* item);
	void (*on_player_pickup_item)(struct wld_map*, struct wld_mob *player, struct wld_item *item);
	void (*on_player_pickup_item_fail)(struct wld_map*, struct wld_mob *player, struct wld_item *item);
	void (*on_player_drop_item)(struct wld_map*, struct wld_mob *player, struct wld_item *item);
	void (*on_player_drop_item_fail)(struct wld_map*, struct wld_mob *player, struct wld_item *item);
};
struct draw_struct {
	int colorpair;
	unsigned long sprite;
	unsigned long sprite_2;
	int colorpair_2;
};

struct wld_world {
	int seed;
	struct wld_map **maps;
	int maps_length;
};

// WORLD INITALIZATION
void wld_setup();
void wld_teardown();
struct wld_world* wld_new_world(int seed, int count);
void wld_delete_world(struct wld_world*);
void wld_transition_player(struct wld_world*, struct wld_map *from, struct wld_map *to, bool at_entrance);

// GENERATORS
void gen_mob_jackal(struct wld_map* map, int c, int r);
void gen_mob_rat(struct wld_map* map, int c, int r);

// MAP INITIALIZATION
void wld_generate_tiles(struct wld_map *map, struct dng_cellmap* cellmap);
void wld_init_mob(struct wld_mob *mob, enum WLD_MOBTYPE type);
void wld_generate_mobs(struct wld_map *map, struct dng_cellmap* cellmap);
void wld_init_item(struct wld_item* item, enum WLD_ITEMTYPE type);
void wld_generate_items(struct wld_map *map, struct dng_cellmap* cellmap);
struct wld_map* wld_new_map(int id, int difficulty, int width, int height);
void wld_delete_mob(struct wld_mob* mob);
void wld_delete_map(struct wld_map *map);

// MAP METHODS
int wld_calcindex(int x, int y, int cols);
int wld_calcx(int index, int cols);
int wld_calcy(int index, int cols);
void wld_map_insert_item(struct wld_map* map, struct wld_item* item, int x, int y, int id);
void wld_map_new_item(struct wld_map* map, struct wld_item* item, int x, int y);
void wld_map_remove_item(struct wld_map* map, struct wld_item* item);
void wld_map_insert_mob(struct wld_map* map, struct wld_mob* mob, int x, int y, int id);
void wld_map_new_mob(struct wld_map* map, struct wld_mob* mob, int x , int y);
void wld_map_queue_destroy_mob(struct wld_map* map, struct wld_mob* mob);
void wld_map_destroy_mob(struct wld_map* map, struct wld_mob* mob);
void wld_map_remove_mob(struct wld_map* map, struct wld_mob* mob);
void wld_map_add_mob_at_entrance(struct wld_map* map, struct wld_mob* mob);
void wld_map_add_mob_at_exit(struct wld_map* map, struct wld_mob* mob);
bool wld_map_can_move_to(struct wld_map *map, int x, int y);
void wld_map_move_cursor(struct wld_map *map, int relx, int rely);
void wld_map_set_cursor_pos(struct wld_map *map, int newx, int newy);
struct wld_tile* wld_map_get_tile_at(struct wld_map *map, int x, int y);
struct wld_tile* wld_map_get_tile_at_index(struct wld_map *map, int index);
struct wld_mob* wld_map_get_mob_at(struct wld_map *map, int x, int y);
struct wld_mob* wld_map_get_mob_at_index(struct wld_map *map, int index);
struct wld_item* wld_map_get_item_at(struct wld_map *map, int x, int y);
struct wld_item* wld_map_get_item_at_index(struct wld_map *map, int index);
struct draw_struct wld_map_get_drawstruct(struct wld_map *map, int x, int y);
struct draw_struct wld_map_get_drawstruct_memory(struct wld_map *map, int x, int y);
void wld_map_vfx_heal(struct wld_map *map, int x, int y);
void wld_map_vfx_dmg(struct wld_map *map, int x, int y);
void wld_map_add_effect(struct wld_map *map, enum WLD_EFFECT, int x, int y);

// EFFECTS
void wld_effect_on_fire(struct wld_effect *effect, struct wld_mob *mob);

// TILE EVENTS
void wld_tile_on_mob_enter_entrance(struct wld_map* map, struct wld_tile* tile, struct wld_mob* mob);
void wld_tile_on_mob_enter_exit(struct wld_map* map, struct wld_tile* tile, struct wld_mob* mob);
void wld_tile_on_mob_enter_summoncircle(struct wld_map* map, struct wld_tile* tile, struct wld_mob* mob);
bool wld_tile_is_blocked_vision(struct wld_tile* tile);
bool wld_tile_is_blocked_movement(struct wld_tile* tile);

// TYPE METHODS
struct wld_tiletype* wld_get_tiletype(int id);
struct wld_mobtype* wld_get_mobtype(int id);
struct wld_itemtype* wld_get_itemtype(int id);
struct wld_effecttype* wld_get_effectype(int id);
int wld_cpair(enum WLD_COLOR_INDEX a, enum WLD_COLOR_INDEX b);
int wld_cpair_tm(int tiletype, int mobtype);
int wld_cpair_ti(int tiletype, int itemtype);
int wld_cpairmem(int tiletype);
int wld_cpair_bg(int tiletype);

// MOB METHODS
int wld_mob_dist_tile(struct wld_mob *mob, struct wld_tile *tile);
void wld_mob_teleport(struct wld_mob *mob, int relx, int rely, bool trigger_events);
void wld_mob_move(struct wld_mob *mob, int relx, int rely, bool trigger_events);
void wld_mob_vision(struct wld_mob *mob, void (*on_see)(struct wld_mob*, int, int, double));
bool wld_mob_is_next_to_tile(struct wld_mob *mob, struct wld_tile* tile);
bool wld_mob_is_next_to_mob(struct wld_mob* ma, struct wld_mob* mb);
struct wld_item* wld_mob_get_item_in_slot(struct wld_mob *mob, int slot);
int wld_mob_get_open_inventory_slot(struct wld_mob *mob);
bool wld_mob_has_inventory(struct wld_mob*);
bool wld_mob_pickup_item(struct wld_mob*, struct wld_item*);
void wld_mob_swap_item(struct wld_mob* mob, int slot_a, int slot_b);
bool wld_mob_equip(struct wld_mob*, int);
bool wld_mob_unequip(struct wld_mob*, int);
bool wld_mob_drink_item(struct wld_mob *mob, int itemslot);
void wld_mob_destroy_item_in_slot(struct wld_mob* mob, int itemslot);
void wld_mob_destroy_item(struct wld_mob* mob, struct wld_item* item);
void wld_mob_resolve_item_uses(struct wld_mob* mob, struct wld_item* item);
bool wld_mob_drop_item(struct wld_mob*, int);
void wld_mob_inspect_melee(struct wld_mob*, void (*inspect)(int,int));
void wld_mob_inspect_targetables(struct wld_mob*, void (*inspect)(int,int));
void wld_mob_inspect_inventory(struct wld_mob*, void (*inspect)(struct wld_item*));
void wld_mob_new_aberration(struct wld_mob *mob);

// MOB AI
struct wld_mob* ai_get_closest_visible_enemy(struct wld_mob* self);
void ai_flee_enemy(struct wld_mob* self, struct wld_mob *enemy);
void ai_default_wander(struct wld_mob *mob);
bool ai_default_is_hostile(struct wld_mob *self, struct wld_mob *target);
bool ai_default_detect_combat(struct wld_mob *mob);
void ai_default_decide_combat(struct wld_mob *self);
void ai_mob_heal(struct wld_mob *mob, int amt, struct wld_item* item);
void ai_mob_die(struct wld_mob *mob);
void ai_effect_attack_mob(struct wld_effect *effect, struct wld_mob *defender, int amt);
void ai_mob_kill_mob(struct wld_mob *aggressor, struct wld_mob *defender, struct wld_item* item);
void ai_mob_attack_mob(struct wld_mob *aggressor, struct wld_mob *defender, int amt, struct wld_item* item);
bool ai_can_melee(struct wld_mob *aggressor, struct wld_mob *defender);
void ai_mob_whiff(struct wld_mob *aggressor, struct wld_item* item);
void ai_mob_whiff_mob(struct wld_mob *aggressor, struct wld_mob *defender, struct wld_item* item);
void ai_mob_melee_mob(struct wld_mob *aggressor, struct wld_mob *defender);
bool ai_mob_use_item(struct wld_mob* mob, struct wld_item* item, struct wld_tile* cursor_tile);
bool ai_player_use_active_item(struct wld_mob* player);
bool ai_player_trigger_target(struct wld_mob* player);
bool ai_player_set_use_item(struct wld_mob*, int);
bool ai_player_enter_targeting(struct wld_mob* player);
bool ai_player_draw_weapon(struct wld_mob* player);
bool ai_player_leave_targeting(struct wld_mob* player);
bool ai_player_sheath_weapon(struct wld_mob* player);
bool ai_queuemobmove(struct wld_mob *mob, int relx, int rely);
bool ai_act_upon(struct wld_mob *mob, int relx, int rely);
bool ai_rest(struct wld_mob *mob);
bool ai_get(struct wld_mob *mob, int relx, int rely);
bool ai_can_get(struct wld_mob *mob, int relx, int rely);
void wld_update_mob(struct wld_mob *mob);

// CHEATS
void wld_cheat_teleport_exit(struct wld_map *map, struct wld_mob*);

// RPG CALCULATIONS
int rpg_calc_melee_dmg(struct wld_mob *aggressor, struct wld_mob *defender);
double rpg_calc_melee_coh(struct wld_mob *aggressor, struct wld_mob *defender);
int rpg_calc_melee_weapon_dmg(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender);
double rpg_calc_melee_weapon_coh(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender);
int rpg_calc_ranged_weapon_dmg(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender);
double rpg_calc_ranged_weapon_coh(struct wld_mob *aggressor, struct wld_item *weapon, struct wld_mob *defender);
int rpg_calc_range_dist(struct wld_mob *aggressor, int base_range);
int rpg_calc_alchemy_boost(struct wld_mob *user, struct wld_item *item);

// ITEM ACTIONS
void itm_target_melee(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int));
bool itm_can_use_melee(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_use_melee(struct wld_item *weapon, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_hit_melee_swordstyle(struct wld_item *weapon, struct wld_mob *user, struct wld_tile* tile);

void itm_target_ranged_los(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int));
bool itm_can_use_ranged_los(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_use_ranged_los(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_hit_ranged_los_bowstyle(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile);

void itm_drink_minorhealth(struct wld_item *item, struct wld_mob *user);
void itm_hit_minorhealth(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile);

void itm_target_key(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int));
bool itm_can_use_key(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_use_key(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_hit_key(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile);

void itm_target_ranged_aoe(struct wld_item *item, struct wld_mob *user, void(*inspect)(int, int));
bool itm_can_use_ranged_aoe(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_use_ranged_aoe(struct wld_item *item, struct wld_mob *user, struct wld_tile* cursor_tile);
void itm_hit_ranged_aoe_firebomb(struct wld_item *item, struct wld_mob *user, struct wld_tile* tile);

// LOGGERS
// These are built in main
void wld_log(char* msg);
void wld_log_s(char* msg, char* s2);
void wld_log_ss(char* msg, char* s2, char *s3);
void wld_log_ms(char* msg, struct wld_mob* mob);
void wld_log_it(char* msg, struct wld_item* item);
void wld_log_ts(char* msg, struct wld_tile* tile);

#endif
