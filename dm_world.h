#ifndef DM_WORLD
#define DM_WORLD

#include "dm_defines.h"
#include "dm_dungeon.h"

#define INVENTORY_SIZE 12

struct wld_item;
struct wld_mob;

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
	char *short_desc;
};
struct wld_tile {
	int id; // position in tile list
	int map_x, map_y, map_index;
	enum WLD_TILETYPE type_id;
	struct wld_tiletype *type;
	struct wld_map *map;
	bool is_visible;
	bool was_visible;
	// on_enter, on_leave events
	void(*on_mob_enter)(struct wld_map*, struct wld_tile*, struct wld_mob*); // TODO left off here
};


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
enum MODE {
	MODE_PLAY,
	MODE_INVENTORY,
	MODE_USE,
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
	unsigned long sprite;
	int fg_color;
	char *short_desc;
	char *title;
};
struct wld_mob {
	int id; // positin in maps mob list
	int map_x, map_y, map_index; // position in map geo and index
	enum WLD_MOBTYPE type_id; // wld_mobtypes struct index
	struct wld_mobtype *type;
	struct wld_map *map;
	enum WLD_MOB_STATE state;
	void (*ai_wander)(struct wld_mob*);
	bool (*ai_detect_combat)(struct wld_mob*);
	void (*ai_decide_combat)(struct wld_mob*);
	void (*ai_player_input)(struct wld_mob*);
	int queue_x, queue_y;
	int health, maxhealth;
	bool is_player, is_dead;
	int cursor_target_index; // map index
	enum MODE mode;
	enum TARGET_MODE target_mode;
	struct wld_item **inventory;
	int target_x, target_y;
	struct wld_item *active_item;
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
};
struct wld_itemtype {
	enum WLD_ITEMTYPE type;
	unsigned long sprite;
	int fg_color;
	bool is_weq, is_aeq;
	char *short_desc;
	char *title;
	void (*fn_drink)(struct wld_item*, struct wld_mob*);
	void (*fn_target)(struct wld_item*, struct wld_mob*, void (*inspect)(int,int));
	bool (*fn_can_use)(struct wld_item*, struct wld_mob*, struct wld_tile*);
	void (*fn_use)(struct wld_item*, struct wld_mob*, struct wld_tile*);
	void (*fn_hit)(struct wld_item*, struct wld_mob*, struct wld_tile*);
	int base_range;
	bool has_uses;
	int base_uses;
	char *drink_label;
	char *use_label;
	char *use_text_1;
	char *use_text_2;
};
struct wld_item {
	int id;
	int map_x, map_y, map_index;
	enum WLD_ITEMTYPE type_id;
	struct wld_itemtype *type;
	bool has_dropped;
	int uses;
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
};
void wld_new_mob(struct wld_map* map, struct wld_mob* mob, int x, int y);

struct wld_world {
	int seed;
	struct wld_map **maps;
	int maps_length;
};

void wld_insert_item(struct wld_map* map, struct wld_item* item, int x, int y, int id);
void wld_map_new_item(struct wld_map* map, struct wld_item* item, int x, int y);
void wld_map_remove_mob(struct wld_map* map, struct wld_mob* mob);

///////////////////////////
// UTILITY METHODS
struct wld_tiletype* wld_get_tiletype(int id);
struct wld_mobtype* wld_get_mobtype(int id);
struct wld_itemtype* wld_get_itemtype(int id);

int wld_cpair_tm(int tiletype, int mobtype);
int wld_cpair_ti(int tiletype, int itemtype);
int wld_cpairmem(int tiletype);
int wld_cpair_bg(int tiletype);

int wld_calcindex(int x, int y, int cols);
int wld_calcx(int index, int cols);
int wld_calcy(int index, int cols);

int wld_distance_mob_tile(struct wld_map *map, struct wld_mob *mob, struct wld_tile *tile);
bool wld_canmoveto(struct wld_map *map, int x, int y);
void wld_teleportmob(struct wld_mob *mob, int relx, int rely, bool trigger_events); // TODO duplicate code in here from movemob
void wld_movemob(struct wld_mob *mob, int relx, int rely, bool trigger_events);
void wld_movecursor(struct wld_map *map, int relx, int rely);

struct wld_tile* wld_gettileat(struct wld_map *map, int x, int y);
struct wld_tile* wld_gettileat_index(struct wld_map *map, int index);
struct wld_mob* wld_getmobat(struct wld_map *map, int x, int y);
struct wld_mob* wld_getmobat_index(struct wld_map *map, int index);
struct wld_item* wld_getitemat(struct wld_map *map, int x, int y);
struct wld_item* wld_getitemat_index(struct wld_map *map, int index);

void wld_mobvision(struct wld_mob *mob, void (*on_see)(struct wld_mob*, int, int, double));
struct draw_struct wld_get_drawstruct(struct wld_map *map, int x, int y);
struct draw_struct wld_get_memory_drawstruct(struct wld_map *map, int x, int y);
bool wld_mob_nextto_mob(struct wld_mob* ma, struct wld_mob* mb);
struct wld_item* wld_mob_get_item_in_slot(struct wld_mob *mob, int slot);
int wld_mob_get_open_inventory_slot(struct wld_mob *mob);
bool wld_mob_has_inventory(struct wld_mob*);
bool wld_mob_pickup_item(struct wld_mob*, struct wld_item*);
bool wld_mob_equip(struct wld_mob*, int);
bool wld_mob_unequip(struct wld_mob*, int);
bool wld_mob_drink_item(struct wld_mob *mob, int itemslot);
void wld_mob_destroy_item_in_slot(struct wld_mob* mob, int itemslot);
void wld_mob_destroy_item(struct wld_mob* mob, struct wld_item* item);
void wld_mob_resolve_item_uses(struct wld_mob* mob, struct wld_item* item);
bool wld_mob_drop_item(struct wld_mob*, int);
void wld_mob_inspect_melee(struct wld_mob*, void (*inspect)(int,int));
void wld_mob_inspect_targetables(struct wld_mob*, void (*inspect)(int,int));


///////////////////////////
// CHEATS
void wld_cheat_teleport_exit(struct wld_map *map, struct wld_mob*);



///////////////////////////
// RPG CALCULATIONS
int rpg_calc_melee_dmg(struct wld_mob *aggressor, struct wld_mob *defender);
int rpg_calc_ranged_dmg(struct wld_mob *aggressor, struct wld_mob *defender);
double rpg_calc_melee_coh(struct wld_mob *aggressor, struct wld_mob *defender);
double rpg_calc_ranged_coh(struct wld_mob *aggressor, struct wld_mob *defender);
int rpg_calc_range_dist(struct wld_mob *aggressor, int base_range);


///////////////////////////
// MOB AI
void ai_default_wander(struct wld_mob *mob);
bool ai_default_detect_combat(struct wld_mob *mob);
void ai_default_decide_combat(struct wld_mob *mob);
void ai_mob_heal(struct wld_mob *mob, int amt, struct wld_item* item);
void ai_mob_kill_mob(struct wld_mob *aggressor, struct wld_mob *defender, struct wld_item* item);
void ai_mob_attack_mob(struct wld_mob *aggressor, struct wld_mob *defender, int amt, struct wld_item* item);
bool ai_can_melee(struct wld_mob *aggressor, struct wld_mob *defender);
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
void wld_update_mob(struct wld_mob *mob);


///////////////////////////
// ITEM ACTIONS
bool itm_can_use_melee(struct wld_item *item, struct wld_mob *user, struct wld_tile* curstile);
void itm_use_melee(struct wld_item *item, struct wld_mob *user, struct wld_tile* curstile);


///////////////////////////
// MAP INITIALIZATION
void wld_gentiles(struct wld_map *map, struct dng_cellmap* cellmap);
void wld_genmobs(struct wld_map *map, struct dng_cellmap* cellmap);
void wld_genitems(struct wld_map *map, struct dng_cellmap* cellmap);
struct wld_map* wld_newmap(int id, int difficulty, int width, int height);
void wld_delmap(struct wld_map *map);


///////////////////////////
// WORLD INITALIZATION
void wld_log(char* msg);
void wld_setup();
void wld_teardown();
struct wld_world* wld_newworld(int seed, int count);
void wld_delworld(struct wld_world*);
void wld_transition_player(struct wld_world*, struct wld_map *from, struct wld_map *to, bool at_entrance);


///////////////////////////
// LOGGERS
// These are built in main
void wld_log(char* msg);
void wld_log_s(char* msg, char* s2);
void wld_log_ms(char* msg, struct wld_mob* mob);
void wld_log_it(char* msg, struct wld_item* item);

#endif
