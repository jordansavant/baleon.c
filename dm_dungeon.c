#include <stdlib.h>
#include <math.h>
#include "dm_dungeon.h"

///////////////////////////
// CELLMAP BUILDERS

///////////////////////////
// GROUND START

void dng_cellmap_buildground(struct dng_cellmap *cellmap)
{
	cellmap->cells = (struct dng_cell**)malloc(cellmap->size * sizeof(struct dng_cell*));
	cellmap->cells_length = cellmap->size; // redundant but consistent

	for (int i=0; i < cellmap->size; i++) {
		struct dng_cell* cell = (struct dng_cell*)malloc(sizeof(struct dng_cell));

		// TODO is this why it blows up with non-square maps?
		cell->index = i;
		cell->x = i % cellmap->width;
		cell->y = i / cellmap->height;

		cellmap->cells[i] = cell;
	}
}

// GROUND END
///////////////////////////

///////////////////////////
// ROOMS START

void dng_cellmap_buildrooms(struct dng_cellmap *cellmap)
{
	//for(unsigned int a = 0; a < roomAttemptCount; a++)
	//{
	//	Room* room = buildRoom();
	//	if(room)
	//	{
	//		rooms.push_back(room);
	//	}

	//	if(rooms.size() > roomCount)
	//	{
	//		return;
	//	}
	//}
}

// ROOMS END
///////////////////////////






struct dng_cellmap* dng_genmap(int difficulty, int width, int height)
{
	struct dng_cellmap *cellmap = (struct dng_cellmap*)malloc(sizeof(struct dng_cellmap));
	cellmap->id = 0; // TODO could be index in a list of generated dungeons
	cellmap->difficulty = difficulty;
	cellmap->width = width;
	cellmap->height = height;
	cellmap->size = width * height;

	// Room details
	cellmap->map_padding = 1;
	cellmap->min_room_width = 6;
	cellmap->max_room_width = 16;
	cellmap->min_room_height = 6;
	cellmap->max_room_height = 16;

	double map_hyp_size = sqrt(width * height);
	double hyp_min_room_size = sqrt(cellmap->min_room_width * cellmap->min_room_height);
	double hyp_max_room_size = sqrt(cellmap->max_room_width * cellmap->max_room_height);
	double hyp_size = (hyp_min_room_size + hyp_max_room_size) / 2;

	double room_density = .8;
	double max_rooms_per_map = (map_hyp_size / hyp_size) * (map_hyp_size / hyp_size);

	cellmap->room_count = max_rooms_per_map * room_density;
	cellmap->room_attempt_count = cellmap->room_count * 2;
	cellmap->room_scatter = hyp_size * (1 - room_density) + hyp_size / 2;
	cellmap->room_scatter = hyp_size / 2 + hyp_size * 10 * (1 - room_density); // TODO why is this overwritten?

	// Tunnel details
	cellmap->min_hall_width = 1;
	cellmap->tunnel_turn_ratio = 0;
	cellmap->deadend_ratio = 0;

	// TODO tunnel directions list

	// Entrance details
	cellmap->entrance_count = 1;
	cellmap->exit_count = 1;

	//mapPadding = 1;
	//float mapHypSize = sqrtf(width * height);

	//// Room Details
	//minRoomWidth = 6;
	//maxRoomWidth = 16;
	//minRoomHeight = 6;
	//maxRoomHeight = 16;

	//float hypMinRoomSize = sqrtf(minRoomWidth * minRoomHeight);
	//float hypMaxRoomSize = sqrtf(maxRoomWidth * maxRoomHeight);
	//float hypSize = (hypMinRoomSize + hypMaxRoomSize) / 2;

	//float roomDensity = .8;
	//float maxRoomsPerMap = (mapHypSize / hypSize) * (mapHypSize / hypSize);

	////roomDensity = .5f + (roomDensity / 2);
	//roomCount = maxRoomsPerMap * roomDensity;
	//roomAttemptCount = roomCount * 2;
	//roomScatter = (hypSize * (1 - roomDensity)) + hypSize / 2;
	//roomScatter = (hypSize / 2) + (hypSize * 10 * (1 - roomDensity)) ;

	//// Tunnel details
	//minHallWidth = 1;
	//tunnelTurnRatio = 0;
	//deadEndRatio = 0;

	//tunnelDirs.push_back(sf::Vector2i(1, 0)); // right
	//tunnelDirs.push_back(sf::Vector2i(0, 1)); // down
	//tunnelDirs.push_back(sf::Vector2i(-1, 0)); // left
	//tunnelDirs.push_back(sf::Vector2i(0, -1)); // up

	//// Exit details
	//entranceCount = 1;
	//exitCount = 1;

	dng_cellmap_buildground(cellmap);
	dng_cellmap_buildrooms(cellmap);

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
