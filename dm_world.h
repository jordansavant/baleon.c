#ifndef DM_WORLD
#define DM_WORLD

#include "dm_defines.h"

#define INVENTORY_SIZE 12

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
};
enum TARGET_MODE {
	TMODE_NONE,
	TMODE_SELF,
	TMODE_MELEE,
	TMODE_RANGED_LOS,
	TMODE_RANGED_TEL,
	TMODE_RANGED_LOS_AOE,
	TMODE_RANGED_TEL_AOE,
};
struct wld_mobtype {
	int type;
	unsigned long sprite;
	int fg_color;
	char *short_desc;
	char *title;
};
struct wld_item;
struct wld_mob {
	int id; // positin in maps mob list
	int map_x, map_y, map_index; // position in map geo and index
	enum WLD_MOBTYPE type; // wld_mobtypes struct index
	struct wld_map *map;
	enum WLD_MOB_STATE state;
	void (*ai_wander)(struct wld_mob*);
	bool (*ai_detect_combat)(struct wld_mob*);
	void (*ai_decide_combat)(struct wld_mob*);
	void (*ai_player_input)(struct wld_mob*);
	int queue_x, queue_y, queue_target;
	int health, maxhealth;
	bool is_player, is_dead;
	int cursor_target; // map index
	enum MODE mode;
	enum TARGET_MODE target_mode;
	struct wld_item **inventory;
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
enum WLD_TARGETTYPE {
	TARGET_PASSIVE,
	TARGET_SELF,
	TARGET_MELEE,
	TARGET_RANGED_LOS,
	TARGET_RANGED_TEL,
	TARGET_RANGED_LOS_AOE,
	TARGET_RANGED_TEL_AOE,
};
struct wld_item {
	int id;
	int map_x, map_y, map_index;
	enum WLD_ITEMTYPE type;
};
struct wld_itemtype {
	enum WLD_ITEMTYPE type;
	unsigned long sprite;
	int fg_color;
	enum WLD_TARGETTYPE target_type;
	char *short_desc;
	char *title;
	void (*on_use)(struct wld_item*, struct wld_mob*);
	void (*on_fire)(struct wld_item*, struct wld_mob*, int, int);
};


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
	int *item_map;
	struct wld_item **items;
	unsigned int items_length;
	struct wld_mob *player;
	struct wld_cursor *cursor;

	// function pointers game subscribes to for events
	void (*on_cursormove)(struct wld_map*, int x, int y, int index);
	void (*on_playermove)(struct wld_map*, struct wld_mob *, int x, int y, int index);
	void (*on_mob_attack_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_mob_attack_player)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_mob_kill_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_mob_kill_player)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_player_attack_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_player_kill_mob)(struct wld_map*, struct wld_mob *agg, struct wld_mob *def);
	void (*on_player_pickup_item)(struct wld_map*, struct wld_mob *player, struct wld_item *item);
	void (*on_player_pickup_item_fail)(struct wld_map*, struct wld_mob *player, struct wld_item *item);
};
struct draw_struct {
	int colorpair;
	unsigned long sprite;
};


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
bool wld_canmoveto(struct wld_map *map, int x, int y);
void wld_movemob(struct wld_mob *mob, int relx, int rely);
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
bool wld_is_mob_nextto_mob(struct wld_mob* ma, struct wld_mob* mb);
int wld_get_open_inventory_slot(struct wld_mob *mob);
bool wld_has_inventory(struct wld_mob*);
bool wld_pickup_item(struct wld_mob*, struct wld_item*);


///////////////////////////
// MOB AI
void ai_default_wander(struct wld_mob *mob);
bool ai_default_detect_combat(struct wld_mob *mob);
void ai_default_decide_combat(struct wld_mob *mob);
void ai_mob_kill_mob(struct wld_mob *aggressor, struct wld_mob *defender);
void ai_mob_attack_mob(struct wld_mob *aggressor, struct wld_mob *defender, int amt);
bool ai_can_melee(struct wld_mob *aggressor, struct wld_mob *defender);
void ai_mob_melee_mob(struct wld_mob *aggressor, struct wld_mob *defender);
bool ai_player_attack_melee(struct wld_mob* player);
bool ai_queuemobmove(struct wld_mob *mob, int relx, int rely);
bool ai_act_upon(struct wld_mob *mob, int relx, int rely);
bool ai_rest(struct wld_mob *mob);
bool ai_get(struct wld_mob *mob, int relx, int rely);
void wld_update_mob(struct wld_mob *mob);


///////////////////////////
// ITEM ACTIONS
void itm_on_user_melee(struct wld_item *item, struct wld_mob *user);
void itm_on_fire_melee(struct wld_item *item, struct wld_mob *user, int mapx, int mapy);


///////////////////////////
// MAP INITIALIZATION
void wld_gentiles(struct wld_map *map);
void wld_genmobs(struct wld_map *map);
void wld_genitems(struct wld_map *map);
struct wld_map* wld_newmap(int depth);
void wld_delmap(struct wld_map *map);


///////////////////////////
// WORLD INITALIZATION
void wld_setup();
void wld_teardown();

#endif
