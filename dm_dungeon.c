#include <stdlib.h>
#include <math.h>
#include <stdio.h> // TODO REMOVE
#include "dm_algorithm.h"
#include "dm_dungeon.h"

///////////////////////////
// CELLMAP BUILDERS

struct tunnel_dir tunnel_dirs[] = {
	// right, down, left, up
	{1,0}, {0,1}, {-1,0}, {0,-1}
};

///////////////////////////
// GROUND START

void dng_cellmap_buildground(struct dng_cellmap *cellmap)
{
	cellmap->cells = (struct dng_cell**)malloc(cellmap->size * sizeof(struct dng_cell*));
	cellmap->cells_length = cellmap->size; // redundant but consistent

	for (int r=0; r < cellmap->height; r++) {
		for (int c=0; c < cellmap->width; c++) {
			struct dng_cell* cell = (struct dng_cell*)malloc(sizeof(struct dng_cell));
			dng_cell_init(cell);

			// TODO is this why it blows up with non-square maps?
			int index = r * cellmap->width + c;
			cell->index = index;
			cell->x = c;
			cell->y = r;

			cellmap->cells[index] = cell;
		}
	}
}

void dng_cell_init(struct dng_cell *cell)
{
	cell->index = 0;
	cell->x = 0;
	cell->y = 0;
	cell->is_wall = false;

	cell->room = NULL;
	cell->is_room_edge = false;
	cell->is_room_perimeter = false;

        cell->is_tunnel = false;
        cell->was_corridor_tunnel = false;
        cell->was_door_tunnel = false;
        cell->was_room_fix_tunnel = false;

	cell->sill_data.door_x = -1;
	cell->sill_data.door_y = -1;
	cell->is_sill = false;
        cell->is_door = false;
        cell->was_door = false;
	cell->door; // TODO? is this ok?

	cell->is_entrance = false;
	cell->entrance_id = -1;
	cell->entrance_priority = 0;
	cell->is_entrance_transition = false;
	cell->entrance_transition = NULL;
	cell->is_exit_transition = false;
	//cell->exit_transition = NULL; // TODO
	cell->transition_dir_x = 0;
	cell->transition_dir_y = 0;
	cell->metadata_flood_id = -1;

	cell->astar_node = (struct dm_astarnode*)malloc(sizeof(struct dm_astarnode));
	cell->astar_node->owner = (void*)cell;
	cell->astar_node->get_x = dng_cell_get_x;
	cell->astar_node->get_y = dng_cell_get_y;
}

int dng_cell_get_x(struct dm_astarnode *astar_node)
{
	struct dng_cell *cell = (struct dng_cell*)astar_node->owner;
	return cell->x;
}

int dng_cell_get_y(struct dm_astarnode *astar_node)
{
	struct dng_cell *cell = (struct dng_cell*)astar_node->owner;
	return cell->y;
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
	struct dng_room *room = NULL;
	int room_width = dm_randii(cellmap->min_room_width, cellmap->max_room_width);
	int room_height = dm_randii(cellmap->min_room_height, cellmap->max_room_height);
	int i=0;

	// sample cells spread out by room scatter factor
	bool inspect_sp(struct dng_cell* cell) {
		if (i % cellmap->room_scatter == 0) {
			// if the map can house this room then lets pick it for a room spot
			if (dng_cellmap_can_house_dimension(cellmap, cell->x, cell->y, room_width, room_height)) {

				// can be a room if not already a room or room perimeter, if any cell then no
				bool can_be_placed = true;
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
					dng_room_init(room, cell->x, cell->y, room_width, room_height);
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

void dng_room_init(struct dng_room *room, int x, int y, int w, int h)
{
	//: x(0), y(0), width(0), height(0), entranceWeight(0), isMachineRoom(false)
	room->x = x;
	room->y = y;
	room->width = w;
	room->height = h;
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
			cell->y == room->y + room->height - 1
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

	dng_cellmap_inspect_cells_in_dimension(cellmap, room->x, room->y, room->width, room->height, inspect_dim);
}

// ROOMS END
///////////////////////////


///////////////////////////
// TUNNELS START
void dng_cellmap_buildtunnels(struct dng_cellmap *cellmap)
{
	// Iterate all cells within the map
	for (int i = cellmap->map_padding; i < cellmap->width - cellmap->map_padding; i++) { // cols
		for(int j = cellmap->map_padding; j < cellmap->height - cellmap->map_padding; j++) { // rows
			// Build recursive tunnels from cell
			struct dng_cell *cell = dng_cellmap_get_cell_at_position(cellmap, i, j);
			if (cell->room == NULL && !cell->is_room_perimeter && !cell->is_tunnel) {
				//tunnel2(cell);
				dng_cellmap_tunnel(cellmap, cell, tunnel_dirs[0]);
			}
		}
	}
}

// recursive dig
void dng_cellmap_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir last_dir)
{
	// ORIGINAL
	if (last_dir.x == 0 && last_dir.y == 0) {
		// first cell
		last_dir = tunnel_dirs[0];
	}

	bool attempt_straight = dm_randf() > cellmap->tunnel_turn_ratio; // % chance to try to go straight

	// create new tunnel dirs to randomize
	struct tunnel_dir rand_dirs[4];
	for (int i=0; i<4; i++)
		rand_dirs[i] = tunnel_dirs[i];
	dng_get_shuffled_directions(rand_dirs);

	// Try to dig straight
	if (attempt_straight) {
		if (dng_cellmap_open_tunnel(cellmap, cell, last_dir)) {
			struct dng_cell *next_cell = dng_cellmap_get_cell_at_position(cellmap, cell->x + last_dir.x, cell->y + last_dir.y);
			dng_cellmap_tunnel(cellmap, next_cell, last_dir);
		}
	}

	// Try to dig in any direction
	for (unsigned int i=0; i < 4; i++) {
		struct tunnel_dir dir = rand_dirs[i];

		if (attempt_straight && dir.x == last_dir.x && dir.y == last_dir.y)
			continue;

		if (dng_cellmap_open_tunnel(cellmap, cell, dir)) {
			struct dng_cell *next_cell = dng_cellmap_get_cell_at_position(cellmap, cell->x + dir.x, cell->y + dir.y);
			dng_cellmap_tunnel(cellmap, next_cell, dir);
		}
	}
}

bool dng_cellmap_open_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir dir)
{
    if (dng_cellmap_can_tunnel(cellmap, cell, dir)) {
        dng_cellmap_emplace_tunnel(cellmap, cell, dir);
        return true;
    }

    return false;
}

bool dng_cellmap_can_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir dir)
{
	int next_x = cell->x + dir.x;
	int next_y = cell->y + dir.y;
	int next_right_x = next_x + -dir.y;
	int next_right_y = next_y + dir.x;
	int next_left_x = next_x + dir.y;
	int next_left_y = next_y + -dir.x;

	int next_up_x = next_x + dir.x;
	int next_up_y = next_y + dir.y;
	int next_up_right_x = next_up_x + -dir.y;
	int next_up_right_y = next_up_y + dir.x;
	int next_up_left_x = next_up_x + dir.y;
	int next_up_left_y = next_up_y + -dir.x;

	if (cell->room != NULL)
		return false; // I added this in baleon code, not xogeni

	// Cell two positions over cannot be: outside of margin, room perimeter, another corridor
	if (next_x >= cellmap->map_padding && next_y >= cellmap->map_padding && next_x < cellmap->width - cellmap->map_padding && next_y < cellmap->height - cellmap->map_padding) {
		struct dng_cell *next_cell = dng_cellmap_get_cell_at_position(cellmap, next_x, next_y);
		struct dng_cell *next_left_cell = dng_cellmap_get_cell_at_position(cellmap, next_left_x, next_left_y);
		struct dng_cell *next_right_cell = dng_cellmap_get_cell_at_position(cellmap, next_right_x, next_right_y);
		struct dng_cell *next_up_cell = dng_cellmap_get_cell_at_position(cellmap, next_up_x, next_up_y);
		struct dng_cell *next_up_left_cell = dng_cellmap_get_cell_at_position(cellmap, next_up_left_x, next_up_left_y);
		struct dng_cell *next_up_right_cell = dng_cellmap_get_cell_at_position(cellmap, next_up_right_x, next_up_right_y);

		bool is_heading_into_room = next_cell->is_room_perimeter || next_up_cell->is_room_perimeter;
		bool is_heading_into_tunnel = next_cell->is_tunnel || next_up_cell->is_tunnel;
		bool is_running_adjacent_to_tunnel = (
			next_left_cell->is_tunnel ||
			next_right_cell->is_tunnel ||
			next_up_left_cell->is_tunnel ||
			next_up_right_cell->is_tunnel
		);

		if (!is_heading_into_room && !is_heading_into_tunnel && !is_running_adjacent_to_tunnel)
			return true;
	}

	return false;
}

void dng_cellmap_emplace_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir dir)
{
	struct dng_cell *next_cell = dng_cellmap_get_cell_at_position(cellmap, cell->x + dir.x, cell->y + dir.y);
	dng_cellmap_mark_as_tunnel(cellmap, cell);
}

void dng_cellmap_mark_as_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell)
{
    cell->is_tunnel = true;
    cell->was_corridor_tunnel = true;
}

void dng_get_shuffled_directions(struct tunnel_dir *dirs)
{
    int n = 4; // 4 directions
    for (int i = n - 1; i > 0; --i) {
	    dirs[i] = dirs[dm_randii(0, i + 1)];
    }
}
// TUNNELS END
///////////////////////////




///////////////////////////
// DOORS START

void dng_cellmap_builddoors(struct dng_cellmap *cellmap)
{
	for (int i=0; i < cellmap->rooms_length; i++)
		dng_cellmap_open_room(cellmap, cellmap->rooms[i]);
}

void dng_cellmap_open_room(struct dng_cellmap *cellmap, struct dng_room *room)
{
	// THIS function was pretty significantly refactored from XoGeni so
	// I could avoid these vectors beloe
	//std::vector<Cell*> sills;
	//std::vector<Cell*> doorSills;

	// Calculate number of openings this rooms could/should have
	int hyp_size = (int)sqrt(room->width * room->height);
	int num_openings = hyp_size + dm_randii(0, hyp_size);

	// for small rooms this is not running
	// assign room sills and prep a list of door sills
	int sill_length = 0;
	int starter_cell_index = -1;
	void on_sill_set(struct dng_cell *cell_sill) {
		// ensure we pick at least one cell to be a door
		// pick first, then potentially overwrite it with another random one
		if (starter_cell_index == -1 || dm_randii(0, sill_length) == sill_length) {
			starter_cell_index = cell_sill->index;
		// else pick them at random to be a door cell
		} else if (dm_randii(0, num_openings / 2) == 0)
			dng_cellmap_emplace_door(cellmap, room, cell_sill);

		sill_length++;
	};
	dng_cellmap_set_room_sills(cellmap, room, on_sill_set);

	// ensure we set our door for the first cell we picked
	struct dng_cell *cell_sill = cellmap->cells[starter_cell_index];
	dng_cellmap_emplace_door(cellmap, room, cell_sill);
}

void dng_cellmap_emplace_door(struct dng_cellmap* cellmap, struct dng_room *room, struct dng_cell *cell_sill)
{
	struct dng_cell *cell_door = dng_cellmap_get_cell_at_position(cellmap, cell_sill->sill_data.door_x, cell_sill->sill_data.door_y);
	if (!cell_door->is_door) {
		// TODO door types were here
		//RoomDoor::DoorType doorType;
		//switch(LevelGenerator::random.next(2))
		//{
		//	case 0:
		//		doorType = RoomDoor::DoorType::Arch;
		//		break;
		//	case 1:
		//		doorType = RoomDoor::DoorType::Door;
		//		break;
		//}

		// Create door
		int dir_x = cell_door->x - cell_sill->x;
		int dir_y = cell_door->y - cell_sill->y;
		struct dng_roomdoor roomdoor = {
			cell_door->x, cell_door->y,
			// TODO DOOR TYPE
			cell_door->room,
			dir_x, dir_y
		};

		cell_door->is_door = true;
		cell_door->was_door = true;
		cell_door->door = roomdoor;

		// Help connections
		dng_cellmap_connect_door(cellmap, &roomdoor);
	}
}

// TODO this is hardcoded for square rooms, not round or cavelikes
void dng_cellmap_set_room_sills(struct dng_cellmap* cellmap, struct dng_room *room, void (*on_set)(struct dng_cell*))
{
	// identify and mark sills of a room
	int topMargin = cellmap->min_hall_width + 2;
	int bottomMargin = cellmap->height - (cellmap->min_hall_width + 2);
	int leftMargin = cellmap->min_hall_width + 2;
	int rightMargin = cellmap->width - (cellmap->min_hall_width + 2);
	int cornerSpacing = 2; // do not let sills be valid within 2 spaces of corners

	// North wall
	if (room->y > topMargin) {
		for (int i = room->x + cornerSpacing; i < room->x + room->width - cornerSpacing; i++) {
			if(i % 2 == 0) {
				struct dng_cell* cell = dng_cellmap_get_cell_at_position(cellmap, i, room->y);
				cell->is_sill = true;
				cell->sill_data.door_x = cell->x;
				cell->sill_data.door_y = cell->y - 1;
				on_set(cell);
				//fill.push_back(cell);
			}
		}
	}
	// South wall
	if (room->y + room->height < bottomMargin) {
		for (int i = room->x + cornerSpacing; i < room->x + room->width - cornerSpacing; i++) {
			if (i % 2 == 0) {
				struct dng_cell* cell = dng_cellmap_get_cell_at_position(cellmap, i, room->y + room->height - 1);
				cell->is_sill = true;
				cell->sill_data.door_x = cell->x;
				cell->sill_data.door_y = cell->y + 1;
				on_set(cell);
				//fill.push_back(cell);
			}
		}
	}
	// East wall
	if (room->x > leftMargin) {
		for (int i = room->y + cornerSpacing; i < room->y + room->height - cornerSpacing; i++) {
			if (i % 2 == 0) {
				struct dng_cell* cell = dng_cellmap_get_cell_at_position(cellmap, room->x, i);
				cell->is_sill = true;
				cell->sill_data.door_x = cell->x - 1;
				cell->sill_data.door_y = cell->y;
				on_set(cell);
				//fill.push_back(cell);
			}
		}
	}
	// West wall
	if (room->x + room->width < rightMargin) {
		for (int i = room->y + cornerSpacing; i < room->y + room->height - cornerSpacing; i++) {
			if (i % 2 == 0) {
				struct dng_cell* cell = dng_cellmap_get_cell_at_position(cellmap, room->x + room->width - 1, i);
				cell->is_sill = true;
				cell->sill_data.door_x = cell->x + 1;
				cell->sill_data.door_y = cell->y;
				on_set(cell);
				//fill.push_back(cell);
			}
		}
	}
}

void dng_cellmap_connect_door(struct dng_cellmap *cellmap, struct dng_roomdoor *door)
{
	// Tunnel away from door until we find
	// - A tunnel
	// - Another door
	// - Another room

	bool complete = false;
	struct dng_cell* cell = dng_cellmap_get_cell_at_position(cellmap, door->x, door->y);
	while (!complete) {
		struct dng_cell* right_cell = dng_cellmap_get_cell_at_position(cellmap, cell->x - door->dir_y, cell->y + door->dir_x);
		struct dng_cell* left_cell = dng_cellmap_get_cell_at_position(cellmap, cell->x + door->dir_y, cell->y - door->dir_x);


		cell = dng_cellmap_get_cell_at_position_nullable(cellmap, cell->x + door->dir_x, cell->y + door->dir_y);
		if (cell != NULL) {
			bool stop = (
				cell->is_tunnel || cell->is_door || cell->is_room_edge || // next cell
				right_cell->is_tunnel || left_cell->is_tunnel || // side is a tunnel
				right_cell->is_room_edge || left_cell->is_room_edge || // side is a room
				right_cell->is_door || left_cell->is_door // side is a door
			);
			if (stop) {
				complete = true;
			} else {
				cell->is_tunnel = true;
				cell->was_door_tunnel = true;
			}
		} else {
			// TODO: Failure condition
			complete = true;
		}
	}
}

// DOORS END
///////////////////////////




///////////////////////////
// ENTRANCE START
//
// Exit and entrances depend on connecting parent maps
// If there is no parent map we can create an entrance and exit
// If there is a parent we should tie the entrance to their exit and create an exit
// In either scenario we are the master of exits so generate them

// This is run before rooms are guaranteed connected because it is used as the primary connection point
void dng_cellmap_buildentrance(struct dng_cellmap *cellmap)
{
	int entrance_id = 1;

	// Pick a random room (in future give better heuristic)
	// TODO in future make better heuristic, maybe?
	struct dng_room *room = cellmap->rooms[dm_randii(0, cellmap->rooms_length)];

	// Pick a good entrance cell
	struct dng_cell *entrance_cell = dng_cellmap_pick_transition_cell_for_room(cellmap, room);

	// Build entrance
	cellmap->entrance = (struct dng_entrance*)malloc(sizeof(struct dng_entrance));
	dng_entrance_init(cellmap->entrance, entrance_id, entrance_cell->x, entrance_cell->y);
	entrance_cell->is_entrance_transition = true;
	entrance_cell->entrance_transition = cellmap->entrance;

	// Build  landing pad
	dng_cellmap_build_landing_pad(cellmap, entrance_cell, entrance_id);

	// Set our maps entrance room
	cellmap->entrance_room = room;
}

void dng_entrance_init(struct dng_entrance *entrance, int id, int x, int y)
{
	entrance->id = id;
	entrance->x = x;
	entrance->y = y;
	entrance->parent_map_id = 0;
	entrance->parent_exit_id = 0;
}

struct dng_cell* dng_cellmap_pick_transition_cell_for_room(struct dng_cellmap *cellmap, struct dng_room *room)
{
	// Default to the center of the room because its guaranteed, though not amazing
	int center_x = room->x + room->width / 2;
	int center_y = room->y + room->height / 2;
	struct dng_cell *pick = dng_cellmap_get_cell_at_position(cellmap, center_x, center_y);

	// First pick is the northwest corner
	// It must have a wall to north and to west (TODO, this was because of Isometric map display, does not have to be this)
	switch (dm_randii(0, 4)) {
	case 0: { // northwest
			int nw_x = room->x;
			int nw_y = room->y;
			struct dng_cell *nw_cell = dng_cellmap_get_cell_at_position(cellmap, nw_x, nw_y);
			struct dng_cell *nwn_cell = dng_cellmap_get_cell_at_position(cellmap, nw_x, nw_y - 1);
			struct dng_cell *nww_cell = dng_cellmap_get_cell_at_position(cellmap, nw_x - 1, nw_y);
			if (!nwn_cell->is_door && !nwn_cell->is_tunnel && !nww_cell->is_door && !nww_cell->is_tunnel) {
				pick = nw_cell;
				pick->transition_dir_x = 0;
				pick->transition_dir_y = 1;
				return pick;
			}
		}
		break;
	case 1: { // southeast
			int se_x = room->x + room->width - 1;
			int se_y = room->y + room->height - 1;
			struct dng_cell *se_cell = dng_cellmap_get_cell_at_position(cellmap, se_x, se_y);
			struct dng_cell *ses_cell = dng_cellmap_get_cell_at_position(cellmap, se_x, se_y + 1);
			struct dng_cell *see_cell = dng_cellmap_get_cell_at_position(cellmap, se_x + 1, se_y);
			if (!ses_cell->is_door && !ses_cell->is_tunnel && !see_cell->is_door && !see_cell->is_tunnel) {
				pick = se_cell;
				pick->transition_dir_x = 0;
				pick->transition_dir_y = -1;
				return pick;
			}
		break;
		}
	case 2: { // northeast
			int ne_x = room->x + room->width - 1;
			int ne_y = room->y;
			struct dng_cell *ne_cell = dng_cellmap_get_cell_at_position(cellmap, ne_x, ne_y);
			struct dng_cell *nen_cell = dng_cellmap_get_cell_at_position(cellmap, ne_x, ne_y - 1);
			struct dng_cell *nee_cell = dng_cellmap_get_cell_at_position(cellmap, ne_x + 1, ne_y);
			if (!nen_cell->is_door && !nen_cell->is_tunnel && !nee_cell->is_door && !nee_cell->is_tunnel) {
				pick = ne_cell;
				pick->transition_dir_x = 0;
				pick->transition_dir_y = 1;
				return pick;
			}
		}
		break;
	case 3: { // southwest
			int sw_x = room->x;
			int sw_y = room->y + room->height - 1;
			struct dng_cell *sw_cell = dng_cellmap_get_cell_at_position(cellmap, sw_x, sw_y);
			struct dng_cell *sws_cell = dng_cellmap_get_cell_at_position(cellmap, sw_x, sw_y - 1);
			struct dng_cell *sww_cell = dng_cellmap_get_cell_at_position(cellmap, sw_x + 1, sw_y);
			if (!sws_cell->is_door && !sws_cell->is_tunnel && !sww_cell->is_door && !sww_cell->is_tunnel) {
				pick = sw_cell;
				pick->transition_dir_x = 0;
				pick->transition_dir_y = 1;
				return pick;
			}
		}
		break;
	}

	return pick;
}

void dng_cellmap_build_landing_pad(struct dng_cellmap *cellmap, struct dng_cell* start_cell, int entrance_id)
{
	// flood fill outwards picking a landing pad (so we can transition into the area even if entrance is blocked)
	// this was for multiplayer logic back in the day in RogueZombie but I like the idea still
	int max_radius = 3;
	bool is_blocked(int x, int y, int depth) {
		struct dng_cell *cell = dng_cellmap_get_cell_at_position(cellmap, x, y);
		int radius = dm_disti(cell->x, cell->y, start_cell->x, start_cell->y);
		return radius > max_radius || cell->metadata_flood_id == dm_floodfill_id() || cell->is_room_perimeter;
	}
	void on_fill(int x, int y, int depth) {
		struct dng_cell *cell = dng_cellmap_get_cell_at_position(cellmap, x, y);
		if (cell->metadata_flood_id != dm_floodfill_id()) {
			// Entrance transition is not blocked so we can flodd from it outward, so dont assign it as a pad
			if (!cell->is_entrance_transition && !cell->is_exit_transition) {
				int radius = dm_disti(cell->x, cell->y, start_cell->x, start_cell->y);
				cell->is_entrance = true;
				cell->entrance_id = entrance_id;
				cell->entrance_priority = radius;
				cell->metadata_flood_id = dm_floodfill_id();
			}
		}
	}
	dm_floodfill(start_cell->x, start_cell->y, is_blocked, on_fill);
}
// ENTRANCE END
///////////////////////////



///////////////////////////
// CLEANUP CONNECTION START

void dng_cellmap_cleanup_connections(struct dng_cellmap *cellmap)
{
    // Remove dead ends
    dng_cellmap_collapse_tunnels(cellmap);
    // TODO i have found tunnels that form a loop and live in isolation
    // others that dangle out in a 2x2 and therefore do not collapse

    // Fix doors
    dng_cellmap_fix_doors(cellmap);

    // Fix rooms
    dng_cellmap_fix_rooms(cellmap);
}

void dng_cellmap_collapse_tunnels(struct dng_cellmap *cellmap)
{
    // scan all cells
    for (int i = cellmap->map_padding; i < cellmap->width - cellmap->map_padding; i++) { // cols
        for(unsigned int j = cellmap->map_padding; j < cellmap->height - cellmap->map_padding; j++) { // rows
	    struct dng_cell *cell = dng_cellmap_get_cell_at_position(cellmap, i, j);
            // if tunnel and connected tunnels < 2 its a dead end
            // if dead end recursively collapse
	    bool p = dm_randf() > cellmap->deadend_ratio;
            if (p && cell->is_tunnel && dng_cellmap_count_tunnel_connections(cellmap, cell) < 2) {
                dng_cellmap_collapse(cellmap, cell);
            }
        }
    }
}

int dng_cellmap_count_tunnel_connections(struct dng_cellmap *cellmap, struct dng_cell *tunnel_cell)
{
	int count = 0;

	// Look at cardinal tiles
	struct dng_cell *top_cell = dng_cellmap_get_cell_at_position_nullable(cellmap, tunnel_cell->x, tunnel_cell->y - 1);
	struct dng_cell *bottom_cell = dng_cellmap_get_cell_at_position_nullable(cellmap, tunnel_cell->x, tunnel_cell->y + 1);
	struct dng_cell *right_cell = dng_cellmap_get_cell_at_position_nullable(cellmap, tunnel_cell->x + 1, tunnel_cell->y);
	struct dng_cell *left_cell = dng_cellmap_get_cell_at_position_nullable(cellmap, tunnel_cell->x - 1, tunnel_cell->y);

	if (top_cell != NULL && (top_cell->is_tunnel || top_cell->is_door || top_cell->is_room_edge))
		count++;
	if (bottom_cell != NULL && (bottom_cell->is_tunnel || bottom_cell->is_door || bottom_cell->is_room_edge))
		count++;
	if (right_cell != NULL && (right_cell->is_tunnel || right_cell->is_door || right_cell->is_room_edge))
		count++;
	if (left_cell != NULL && (left_cell->is_tunnel || left_cell->is_door || left_cell->is_room_edge))
		count++;


	return count;
}

void dng_cellmap_collapse(struct dng_cellmap *cellmap, struct dng_cell *tunnel_cell)
{
	// collapse this tunnel
	tunnel_cell->is_tunnel = false;

	// Find direction of adjacent tunnel(s) // should be one unless used outside of collapsing dead ends
	for (int i=0; i < 4; i++) {
		// Collapse them if they are dead ends
		struct dng_cell *neighbor = dng_cellmap_get_cell_at_position_nullable(cellmap, tunnel_cell->x + tunnel_dirs[i].x, tunnel_cell->y + tunnel_dirs[i].y);
		if (neighbor && neighbor->is_tunnel && dng_cellmap_count_tunnel_connections(cellmap, neighbor) < 2) {
			dng_cellmap_collapse(cellmap, neighbor);
		}
	}
}

void dng_cellmap_fix_doors(struct dng_cellmap *cellmap)
{
	//Door Cleanup
	//- Iterate all doors
	//    - If door direction has no tunnel, door or other room
	//        - delete door
	//  - If door direction is another door
	//      - delete door

	// Find all doors, I opted to not keep a list of doors so we have to rescan the map
	// Maybe go back to a listing of doors?
	// although the list of doors had to be deleted in place
	for (int i = cellmap->map_padding; i < cellmap->width - cellmap->map_padding; i++) { // cols
		for(unsigned int j = cellmap->map_padding; j < cellmap->height - cellmap->map_padding; j++) { // rows

			// Fix dangling doors
			struct dng_cell *cell = dng_cellmap_get_cell_at_position(cellmap, i, j);
			if (cell->is_door) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x + cell->door.dir_x, cell->y + cell->door.dir_y);
				if (neighbor->room == NULL && !neighbor->is_door && !neighbor->is_tunnel) {
					cell->is_door = false;
				}
			}

			// Fix doubled doors
			if (cell->is_door) {
				struct dng_cell *neighbor = dng_cellmap_get_cell_at_position(cellmap, cell->x + cell->door.dir_x, cell->y + cell->door.dir_y);
				if (neighbor->is_door) {
					cell->is_door = false;
					dng_cellmap_mark_as_tunnel(cellmap, cell);
				}
			}

			// Else this passes the door tests so give it a door type
			// TODO door types
			if (cell->is_door) {
				// TODO
			}
		}
	}
}

void dng_cellmap_fix_rooms(struct dng_cellmap *cellmap)
{
	//- If more than one room
	//    - Iterate rooms
	//        - Iterate all doors connected to room
	//            - Floodfill tunnel until a room is located
	//                - If room is not origin room
	//                    - Increment origin room connections counter
	//        - If connections is 0
	//            - OPTION A: delete room
	//            - OPTION B: find closest room and tunnel toward room

	// Iterate rooms
	// Astar from their center to the center of every room
	// Count every other room connected
	// If any room has no connections it must be fixed
	if (cellmap->rooms_length > 1) {
		for (int i=0; i < cellmap->rooms_length; i++) {
			struct dng_room *room = cellmap->rooms[i];
			if (!dng_cellmap_are_rooms_connected(cellmap, room, cellmap->entrance_room)) {
				dng_cellmap_tunnel_rooms(cellmap, room, cellmap->entrance_room, true);
			}
		}
	}
}

bool dng_cellmap_are_rooms_connected(struct dng_cellmap *cellmap, struct dng_room *room_a, struct dng_room *room_b)
{
	if (room_a == room_b)
		return true;

	bool connected = false;

	void inspect(struct dng_cell* cell) {
		// if inspect runs it means there was a path
		connected = true;
	}
	dng_cellmap_get_room_connection_path(cellmap, room_a, room_b, inspect);

	return connected;
}

void dng_cellmap_get_room_connection_path(struct dng_cellmap *cellmap, struct dng_room *room_a, struct dng_room *room_b, void (*inspect)(struct dng_cell *cell))
{
	if (room_a == room_b)
		return;

	int center_x = room_a->x + room_a->width / 2;
	int center_y = room_a->y + room_a->height / 2;
	struct dng_cell *room_a_center_cell = dng_cellmap_get_cell_at_position(cellmap, center_x, center_y);

	unsigned int other_center_x = room_b->x + room_b->width / 2;
	unsigned int other_center_y = room_b->y + room_b->height / 2;
	struct dng_cell *room_b_center_cell = dng_cellmap_get_cell_at_position(cellmap, other_center_x, other_center_y);

	bool is_blocked(struct dm_astarnode* node) {
		struct dng_cell *cell = (struct dng_cell*)node->owner;
		return cell->room == NULL && !cell->is_tunnel && !cell->is_door;
	}
	struct dm_astarnode* get_node(int x, int y) {
		int index = y * cellmap->width + x;
		if (x >= 0 && x < cellmap->width && y >= 0 && y < cellmap->height)
			return cellmap->cells[index]->astar_node;
		return NULL;
	}
	void on_path(struct dm_astarnode* node) {
		struct dng_cell *cell = (struct dng_cell*)node->owner;
		int index = cell->y * cellmap->width + cell->x;
		inspect(cell); // pass this cell to inspector
	}

	dm_astar(room_a_center_cell->astar_node, room_b_center_cell->astar_node, is_blocked, get_node, on_path, true, true); // cardinals only, manhattan distance
}

void dng_cellmap_tunnel_rooms(struct dng_cellmap *cellmap, struct dng_room *room_a, struct dng_room *room_b, bool stop_on_room)
{
	struct dng_cell *room_a_center_cell = dng_cellmap_get_cell_at_position(cellmap, room_a->x + room_a->width / 2, room_a->y + room_a->height / 2);
	struct dng_cell *room_b_center_cell = dng_cellmap_get_cell_at_position(cellmap, room_b->x + room_b->width / 2, room_b->y + room_b->height / 2);

	double dirf_x, dirf_y;
	dm_direction(room_a_center_cell->x, room_a_center_cell->y, room_b_center_cell->x, room_b_center_cell->y, &dirf_x, &dirf_y);

	double distance = dm_distf(room_a_center_cell->x, room_a_center_cell->y, room_b_center_cell->x, room_b_center_cell->y);

	int dir_x = (int)dm_round(dirf_x * distance);
	int dir_y = (int)dm_round(dirf_y * distance);

	int ortho_x = (room_b_center_cell->x - room_a_center_cell->x);
	int ortho_y = (room_b_center_cell->y - room_a_center_cell->y);
	int orthodist_x = abs(ortho_x);
	int orthodist_y = abs(ortho_y);

	// fukn dig a big orthogonal tunnel until we hit another room or something
	for (int r = 0; r < orthodist_y; r++) {
		for (int c = 0; c < orthodist_x; c++) {
			int pos_x;
			int pos_y;
			if (ortho_x > 0)
				pos_x = room_a_center_cell->x + c;
			else
				pos_x = room_a_center_cell->x - c;
			if (ortho_y > 0)
				pos_y = room_a_center_cell->y + r;
			else
				pos_y = room_a_center_cell->y - r;

			struct dng_cell *cell = dng_cellmap_get_cell_at_position(cellmap, pos_x, pos_y);
			if (stop_on_room) {
				// stop if we reach another room
				if (cell->room && cell->room != room_a)
					return;
				// check to see if we are adjacent to a room
				// using cardinal directions to check
				struct dng_cell *north = dng_cellmap_get_cell_at_position_nullable(cellmap, pos_x, pos_y - 1);
				struct dng_cell *east = dng_cellmap_get_cell_at_position_nullable(cellmap, pos_x + 1, pos_y);
				struct dng_cell *south = dng_cellmap_get_cell_at_position_nullable(cellmap, pos_x, pos_y + 1);
				struct dng_cell *west = dng_cellmap_get_cell_at_position_nullable(cellmap, pos_x - 1, pos_y);
				if ((north && north->room && north->room != room_a) ||
					(east && east->room && east->room != room_a) ||
					(south && south->room && south->room != room_a) ||
					(west && west->room && west->room != room_a)) {
					// dig this cell and return
					dng_cellmap_emplace_room_fix(cellmap, cell);
					return;
				}
			}

			if(cell->room && cell->room != room_a && stop_on_room)
				return;

			dng_cellmap_emplace_room_fix(cellmap, cell);
		}
	}
}

void dng_cellmap_emplace_room_fix(struct dng_cellmap *cellmap, struct dng_cell *cell)
{
	// Make door
	//if(cell->isRoomPermiter && !cell->isDoor && !cell->isTunnel)
	//{
	//    cell->isDoor = true;
	//}
	//// Make tunnel
	//else
	if (cell->room == NULL && !cell->is_door && !cell->is_tunnel) {
		cell->is_tunnel = true;
		cell->was_room_fix_tunnel = true;
	}
}
// CLEANUP CONNECTION END
///////////////////////////


///////////////////////////
// CALCAULTE WEIGHTS START

void dng_cellmap_calc_entrance_weights(struct dng_cellmap *cellmap)
{
	for (int i=0; i < cellmap->rooms_length; i++) {
		// get room weight to entrance room
		struct dng_room* room = cellmap->rooms[i];
		int weight = 0;
		struct dng_room* last_room = NULL;
		if (room != cellmap->entrance_room) {
			// get the path
			// iterate path between rooms and count how many different rooms there are
			weight++;
			void inspect(struct dng_cell* cell) {
				if (cell->room && cell->room != room && cell->room != cellmap->entrance_room && cell->room != last_room) {
					last_room = cell->room;
					weight++;
				}
			}
			dng_cellmap_get_room_connection_path(cellmap, room, cellmap->entrance_room, inspect);
		}

		// save it to the room
		room->entrance_weight = weight;
		// dropped the list here i had built in xogeni
	}
}

// CALCULATE WEIGHTS END
///////////////////////////



///////////////////////////
// BUILD EXIT START

void dng_cellmap_buildexit(struct dng_cellmap *cellmap)
{
	int exit_id = 2; // must not be same as entrance id

	// Pick a random room that is farthest from our entrance
	// In Baleon I switched this to just pick the furthest room to simplify
	// TODO make it pick a better random room? maybe put treasure in far rooms too?
	int weight = 0;
	struct dng_room* exit_room;
	for (int i=0; i < cellmap->rooms_length; i++) {
		struct dng_room* room = cellmap->rooms[i];
		if (exit_room == NULL || room->entrance_weight > weight) {
			exit_room = room;
			weight = room->entrance_weight;
		}
	}

	// Pick a good entrance cell
	struct dng_cell *exit_cell = dng_cellmap_pick_transition_cell_for_room(cellmap, exit_room);

	// Build exit
	struct dng_exit *exit = (struct dng_exit*)malloc(sizeof(struct dng_exit));
	dng_exit_init(exit, exit_id, exit_cell->x, exit_cell->y);
	exit_cell->is_exit_transition = true;
	exit_cell->exit_transition = exit;

	// build a landing pad here for a reverse exit entrance from a lower floor
	dng_cellmap_build_landing_pad(cellmap, exit_cell, exit_id);

	// Set our maps exit room
	cellmap->exit_room = exit_room;
}

void dng_exit_init(struct dng_exit *exit, int id, int x, int y)
{
	exit->id = id;
	exit->x = x;
	exit->y = y;
	exit->child_map_id = 0;
	exit->child_entrance_id = 0;
}

// BUILD EXIT END
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
		if (current_x >= cellmap->width || current_y >= cellmap->height)
			return;
		struct dng_cell *current = dng_cellmap_get_cell_at_position(cellmap, current_x, current_y);
		if (inspect(current))
			return; // break
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
	// mathematically anything smaller than a 6 by 6 can't have sills placed reliably
	// we required certain spacing between sills and room corners to not have
	// strange tunneling adjacent to doors
	cellmap->min_room_width = 6;
	cellmap->min_room_height = 6;
	cellmap->max_room_width = 16;
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

	// Entrance details
	cellmap->entrance_count = 1;
	cellmap->exit_count = 1;
	cellmap->entrance = NULL;
	//cellmap->exit = NULL; TODO
	cellmap->entrance_room = NULL;
	cellmap->exit_room = NULL;

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

	printf("build ground\n");
	dng_cellmap_buildground(cellmap);
	printf("build rooms\n");
	dng_cellmap_buildrooms(cellmap);
	printf("build tunnels\n");
	dng_cellmap_buildtunnels(cellmap);
	printf("build doors\n");
	dng_cellmap_builddoors(cellmap);
	printf("build entrance\n");
	// TODO throw in big dungeon aberration rooms here
	dng_cellmap_buildentrance(cellmap);
	printf("clean\n");
	dng_cellmap_cleanup_connections(cellmap);
	printf("calc entrance weights\n");
	dng_cellmap_calc_entrance_weights(cellmap);
	printf("build exit\n");
	dng_cellmap_buildexit(cellmap);

	//cellMap->buildWalls();
	//cellMap->buildLights();
	//cellMap->buildTags();
	//cellMap->machinate();

	return cellmap;
}

void dng_delmap(struct dng_cellmap *cellmap)
{
	free(cellmap->entrance);
	for (int i=0; i < cellmap->rooms_length; i++) {
		free(cellmap->rooms[i]); // free cell
	}
	free(cellmap->rooms); // free cell list
	for (int i=0; i < cellmap->cells_length; i++) {
		free(cellmap->cells[i]->astar_node); // astar node
		free(cellmap->cells[i]); // free cell
	}
	free(cellmap->cells); // free cell list
	free(cellmap);
}
