#ifndef DM_DUNGEON
#define DM_DUNGEON

#include "dm_defines.h"

struct dng_cell {
        int index;
        int x, y;

	bool is_wall;
	bool is_entrance;
};

struct dng_cellmap {
	int id;
	int difficulty;
	int width, height, size;
	struct dng_cell **cells;
	int cells_length;

	// room details
	int map_padding;
	int min_room_width, max_room_width, min_room_height, max_room_height;
	int room_count, room_attempt_count, room_scatter;

	// tunnel details
	int min_hall_width;
	double tunnel_turn_ratio, deadend_ratio;
	// TODO tunneldirs

	// entrance details
	int entrance_count, exit_count;
};


///////////////////////////
// CELLMAP BUILDERS
void dng_cellmap_buildground(struct dng_cellmap*);
void dng_cellmap_buildrooms(struct dng_cellmap*);


///////////////////////////
// OVERALL DUNGEON

struct dng_cellmap* dng_genmap(int difficulty, int width, int height);
void dng_delmap(struct dng_cellmap*);

#endif
