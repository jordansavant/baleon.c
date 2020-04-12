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
struct wld_mobtype wld_mobtypes[] = {
	{ MOB_VOID,	' ', 0 },
	{ MOB_BUGBEAR,	'b', 2 },
};
struct wld_mobtype* wld_get_mobtype(int id)
{
	return &wld_mobtypes[id];
}

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
