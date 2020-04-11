// Contains all definitions for the world
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

enum TILETYPE
{
	TILE_VOID = 0,
	TILE_GRASS = 1,
	TILE_WATER = 2,
	TILE_TREE = 3,
};

// TILE COLORS

// build a map of all bg colors for tiles
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

void setup_world()
{
	for (int i=0; i < bg_size; i++) {
		for (int j=0; j < fg_size; j++) {
			int index = j * fg_size + i;
			int colorpair_id = color_base + index;
			init_pair(colorpair_id, fg_colors[j], bg_colors[i]);
		}
	}
}
int get_tile_bgcolor(int type)
{
	switch (type) {
	case TILE_TREE:
	case TILE_GRASS:
		return 1;
	case TILE_WATER:
		return 4;
	}
	return 0;
}
int get_tile_fgcolor(int type)
{
	switch (type) {
	case TILE_GRASS:
	case TILE_TREE:
		return 0;
	}
	return 0; // default black
}
char get_tile_char(int type)
{
	switch (type) {
	case TILE_GRASS:
		return 'w';
	case TILE_TREE:
		return 'T';
	}
	return ' ';
}
int get_mob_color(int type)
{
	return 0; // hard coded
}
char get_mob_char(int type)
{
	return ' '; // hard coded
}

int get_color_pair(int tiletype, int mobtype)
{
	int tile_bg = get_tile_bgcolor(tiletype);
	if (mobtype == 0) {
		int tile_fg = get_tile_fgcolor(tiletype);
		return color_base + (tile_fg * fg_size + tile_bg);
	}
	int mob_fg = get_mob_color(mobtype);
	return color_base + (mob_fg * fg_size + tile_bg);
}

int get_color_pair_tile_bg(int tiletype)
{
	int tile_bg = get_tile_bgcolor(tiletype);
	return color_base + tile_bg; // no addition of foreground so it is black
}
