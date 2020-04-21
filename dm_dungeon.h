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
};


///////////////////////////
// CELLMAP BUILDERS
void dng_cellmap_buildground(struct dng_cellmap*);


///////////////////////////
// OVERALL DUNGEON

struct dng_cellmap* dng_genmap(int difficulty, int width, int height);
void dng_delmap(struct dng_cellmap*);

#endif
