#include <stdlib.h>
#include <math.h>
#include "dm_algorithm.h"
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
	cellmap->rooms = (struct dng_room**)malloc(cellmap->room_count * sizeof(struct dng_room*));
	int i=0;
	cellmap->rooms_length = 0;
	for (int a = 0; a < cellmap->room_attempt_count; a++) {
		struct dng_room* room = dng_cellmap_buildroom(cellmap);
		if (room) {
			cellmap->rooms[i] = room;
			cellmap->rooms_length++;
			i++;
		}
		if (cellmap->rooms_length > cellmap->room_count)
			return;
	}
}

struct dng_room* dng_cellmap_buildroom(struct dng_cellmap *cellmap)
{
	int room_width = dm_randii(cellmap->min_room_width, cellmap->max_room_width);
	int room_height = dm_randii(cellmap->min_room_height, cellmap->max_room_height);
	struct dng_room *room = NULL;
	int i=0;

	// sample cells spread out by room scatter factor
	bool inspect_sp(struct dng_cell* cell) {
		if (i % cellmap->room_scatter == 0) {
			// if the map can house this room then lets pick it for a room spot
			if (dng_cellmap_can_house_dimension(cellmap, cell->x, cell->y, room_width, room_height)) {
				bool can_be_placed = true;
				// can be a room if not already a room or room perimeter, if any cell then no
				bool inspect_dim(struct dng_cell* cell) {
					if (cell->room != NULL || cell->is_room_perimeter) {
						can_be_placed = false;
						return true;
					}
					return false;
				}

				dng_cellmap_inspect_cells_in_dimension(cellmap, cell->x, cell->y, room_width, room_height, inspect_dim);

				if (can_be_placed) {
					room = (struct dng_room*)malloc(sizeof(struct dng_room));
					dng_room_init(room);
					dng_cellmap_emplace_room(cellmap, room);
					return true;
				}
			}
		}
		i++;
		return false;
	}

	dng_cellmap_inspect_spiral_cells(cellmap, inspect_sp);

	return room;
}

void dng_room_init(struct dng_room *room)
{
	//: x(0), y(0), width(0), height(0), entranceWeight(0), isMachineRoom(false)
	room->x = 0;
	room->y = 0;
	room->entrance_weight = 0;
	room->is_machine_room = false;
}

void dng_cellmap_emplace_room(struct dng_cellmap *cellmap, struct dng_room *room)
{
	bool inspect_dim(struct dng_cell* cell) {
		cell->room = room;
		// set room edge
		cell->is_room_edge = (
			cell->x == room->x ||
			cell->y == room->y ||
			cell->x == room->x + room->width - 1 ||
			cell->y == room->y + room->height - 0
		);
		// set exterior walls
		if (cell->is_room_edge) {
			// top left corner
			if (cell->x == room->x && cell->y == room->y) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x - 1, cell->y - 1);
				neighbor->is_room_perimeter = true;
			}
			// top right corner
			if (cell->x == room->x + room->width - 1 && cell->y == room->y) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x + 1, cell->y - 1);
				neighbor->is_room_perimeter = true;
			}
			// bottom left corner
			if (cell->x == room->x && cell->y == room->y + room->height - 1) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x - 1, cell->y + 1);
				neighbor->is_room_perimeter = true;
			}
			// bottom right corner
			if (cell->x == room->x && cell->y == room->y + room->height - 1) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x + 1, cell->y + 1);
				neighbor->is_room_perimeter = true;
			}
			// Left side
			if (cell->x == room->x) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x - 1, cell->y);
				neighbor->is_room_perimeter = true;
			}
			// top side
			if (cell->y == room->y) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x, cell->y - 1);
				neighbor->is_room_perimeter = true;
			}
			// right side
			if(cell->x == room->x + room->width - 1) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x + 1, cell->y);
				neighbor->is_room_perimeter = true;
			}
			// bottom side
			if (cell->y == room->y + room->height - 1) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x, cell->y + 1);
				neighbor->is_room_perimeter = true;
			}
		}
		return false;
	}

	dng_cellmap_inspect_spiral_cells(cellmap, inspect_dim);
}

// ROOMS END
///////////////////////////



///////////////////////////
// CELLMAP INSPECTORS START
void dng_cellmap_inspect_spiral_cells(struct dng_cellmap *cellmap, bool (*inspect)(struct dng_cell*))
{
	int center_x = cellmap->width / 2;
	int center_y = cellmap->height / 2;
	struct dm_spiral sp = dm_spiral(-1); // TODO infinie untested
	do {
		int current_x = center_x + sp.x;
		int current_y = center_y + sp.y;
		struct dng_cell *current = dng_cellmap_get_cell_at_position(cellmap, current_x, current_y);
		if (inspect(current)) {
			return; // break
		}
	} while(dm_spiralnext(&sp));
}

void dng_cellmap_inspect_cells_in_dimension(struct dng_cellmap *cellmap, int x, int y, int w, int h, bool (*inspect)(struct dng_cell*))
{
	for (int i = x; i < x + w; i++) { // cols
		for (int j = y; j < y + h; j++) { // rows
			struct dng_cell *cell = dng_cellmap_get_cell_at_position(cellmap, i, j);
			if (inspect(cell)) {
				// break; // TODO should this be return?
				return;
			}
		}
	}
}

bool dng_cellmap_can_house_dimension(struct dng_cellmap *cellmap, int x, int y, int w, int h)
{
	if (x > cellmap->map_padding && y > cellmap->map_padding) {
		if (x + w + cellmap->map_padding < cellmap->width && y + h + cellmap->map_padding < cellmap->height) {
			return true;
		}
	}
	return false;
}

struct dng_cell* dng_cellmap_get_cell_at_position(struct dng_cellmap *cellmap, int x, int y)
{
	return cellmap->cells[y * cellmap->width + x];
}

struct dng_cell* dng_cellmap_get_cell_at_position_nullable(struct dng_cellmap *cellmap, int x, int y)
{
	if (x > 0 && y > 0 && x < cellmap->width && y < cellmap->height) {
		return cellmap->cells[y * cellmap->width + x];
	}
	return NULL;
}

// CELLMAP INSPECTORS END
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
