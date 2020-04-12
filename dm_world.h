// Contains all definitions for the world
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// TILE TYPES
enum WLD_TILETYPE
{
	TILE_VOID = 0,
	TILE_GRASS = 1,
	TILE_WATER = 2,
	TILE_TREE = 3,
};
struct wld_tiletype
{
	int type;
	char sprite;
	int bg_color;
	int fg_color;
};
// needs to correspond to tile type enum
struct wld_tiletype wld_tiletypes[] = {
	//                  bg fg
	{ TILE_VOID,	' ', 0, 0 },
	{ TILE_GRASS,	'w', 1, 0 },
	{ TILE_WATER,	' ', 4, 0 },
	{ TILE_TREE,	'T', 1, 0 },
};
struct wld_tiletype* wld_get_tiletype(int id)
{
	return &wld_tiletypes[id];
}

// MOB TYPES
enum WLD_MOBTYPE
{
	MOB_VOID = 0,
	MOB_BUGBEAR = 1,
};
struct wld_mobtype
{
	int type;
	char sprite;
	int fg_color;
};
// needs to correspond to mobtype enum
struct wld_mobtype wld_mobtypes[] = {
	{ MOB_VOID,	' ', 0 },
	{ MOB_BUGBEAR,	'b', 2 },
};
struct wld_mobtype* wld_get_mobtype(int id)
{
	return &wld_mobtypes[id];
};
struct wld_mob
{
	int map_x, map_y, map_index; // position in map geo and index
	enum WLD_MOBTYPE type; // wld_mobtypes struct index
};

// COLORS
int bg_colors[] = {
	COLOR_BLACK,  // 0 void
	COLOR_GREEN,  // 1 grass
	COLOR_WHITE,  // 2 snow
	COLOR_RED,    // 3 fire
	COLOR_BLUE,   // 4 water
	COLOR_CYAN,   // 5
	COLOR_YELLOW, // 6
};
int fg_colors[] = {
	COLOR_BLACK,  // 0
	COLOR_GREEN,  // 1
	COLOR_WHITE,  // 2
	COLOR_RED,    // 3
	COLOR_BLUE,   // 4
	COLOR_CYAN,   // 5
	COLOR_YELLOW, // 6
};

int color_base = 100;
int fg_size = 8;
int bg_size = 8;

void wld_setup()
{
	for (int i=0; i < bg_size; i++) {
		for (int j=0; j < fg_size; j++) {
			int index = j * fg_size + i;
			int colorpair_id = color_base + index;
			init_pair(colorpair_id, fg_colors[j], bg_colors[i]);
		}
	}
}

int wld_cpair(int tiletype, int mobtype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	if (mobtype == 0)
		return color_base + (tt->fg_color * fg_size + tt->bg_color);
	struct wld_mobtype *mt = wld_get_mobtype(mobtype);
	return color_base + (mt->fg_color * fg_size + tt->bg_color);
}

int wld_cpair_bg(int tiletype)
{
	struct wld_tiletype *tt = wld_get_tiletype(tiletype);
	return color_base + tt->bg_color; // no addition of foreground so it is black
}

// WORLD MAP

struct wld_map {
	int rows;
	int cols;
	int length;
	int depth;
	int *tile_map; // array of tile types
	int *mob_map; // array of mob ids in mob listing
	struct wld_mob *mobs;
};

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

void wld_genmap(struct wld_map *map)
{
	int *tile_map_array = (int*)malloc(map->length * sizeof(int));
	int *mob_map_array = (int*)malloc(map->length * sizeof(int));

	// randomly fill map with grass, water, and trees
	for (int i=0; i < map->length; i++) {
		tile_map_array[i] = 1; // hard coded grass fow now;
		mob_map_array[i] = -1; // initialize to -1 for "no mob"
	}
	map->tile_map = tile_map_array;
	map->mob_map = mob_map_array;
}
int wld_calcindex(int x, int y, int cols)
{
	return y * cols + x;
}
void wld_genmobs(struct wld_map *map)
{
	// hardcoded to 3 mobs for now
	int mob_count = 3;
	map->mobs = (struct wld_mob*)malloc(mob_count * sizeof(struct wld_mob));

	// setup mobs in mob map
	for (int i=0; i < mob_count; i++) {
		// create mob
		struct wld_mob *mob = &map->mobs[i];
		mob->map_x = 2 + i; // hardcoded positions
		mob->map_y = 2 + i;
		mob->map_index = wld_calcindex(mob->map_x, mob->map_y, map->cols);
		mob->type = MOB_BUGBEAR;
		// set mob's id into the mob map
		map->mob_map[mob->map_index] = i;
	}
}
struct wld_map* wld_newmap(int depth)
{
	struct wld_map *map = (struct wld_map*)malloc(sizeof(struct wld_map));
	map->rows = 12;
	map->cols = 12;
	map->length = 12 * 12;
	map->depth = depth;
	map->tile_map = NULL;
	map->mob_map = NULL;
	map->mobs = NULL;

	// populate tiles
	wld_genmap(map);

	// build an populate map with mobs
	wld_genmobs(map);

	return map;
}

void wld_delmap(struct wld_map *map)
{
	free(map->mobs);
	free(map->mob_map);
	free(map->tile_map);
	free(map);
}
