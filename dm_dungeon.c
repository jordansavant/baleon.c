#include <stdlib.h>
#include "dm_dungeon.h"

void dng_cellmap_buildground(struct dng_cellmap *cellmap)
{
	cellmap->cells = (struct dng_cell**)malloc(cellmap->size * sizeof(struct dng_cell*));
	cellmap->cells_length = cellmap->size; // redundant but consistent

	for (int i=0; i < cellmap->size; i++) {
		struct dng_cell* cell = (struct dng_cell*)malloc(sizeof(struct dng_cell));
		cell->index = i;
		// TODO is this why it blows up with non-square maps?
		cell->x = i % cellmap->width;
		cell->y = i / cellmap->height;

		cell->is_wall = (i % 3 == 0);

		cellmap->cells[i] = cell;
	}
}

struct dng_cellmap* dng_genmap(int difficulty, int width, int height)
{
	struct dng_cellmap *cellmap = (struct dng_cellmap*)malloc(sizeof(struct dng_cellmap));
	cellmap->id = 0; // TODO could be index in a list of generated dungeons
	cellmap->difficulty = difficulty;
	cellmap->width = width;
	cellmap->height = height;
	cellmap->size = width * height;

	dng_cellmap_buildground(cellmap);

	//cellMap->buildGround();
	//cellMap->buildRooms();
	//cellMap->buildTunnels();
	//cellMap->buildDoors();
	//cellMap->buildEntrance();
	//cellMap->cleanupConnections();
	//cellMap->calculateEntranceWeights();
	//cellMap->buildExit();
	//cellMap->buildWalls();
	//cellMap->buildLights();
	//cellMap->buildTags();
	//cellMap->machinate();

	return cellmap;
}

void dng_delmap(struct dng_cellmap *cellmap)
{
	for (int i=0; i < cellmap->cells_length; i++) {
		free(cellmap->cells[i]); // free cell
	}
	free(cellmap->cells); // free cell list
	free(cellmap);
}
