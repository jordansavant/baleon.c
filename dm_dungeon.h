#ifndef DM_DUNGEON
#define DM_DUNGEON

#include "dm_defines.h"

struct dng_room {
	int x, y;
	int width, height;
	int entrance_weight;
	bool is_machine_room;

	//unsigned int cellCount();
};

struct dng_cell {
        int index;
        int x, y;

	bool is_wall;
	bool is_entrance;

	struct dng_room* room;
        bool is_room_edge;
        bool is_room_perimeter;
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
	struct dng_room **rooms;
	int rooms_length;

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
void dng_cell_init(struct dng_cell*);

// ROOMS
void dng_cellmap_buildrooms(struct dng_cellmap*);
struct dng_room* dng_cellmap_buildroom(struct dng_cellmap *cellmap);
void dng_room_init(struct dng_room *room, int x, int y, int w, int h);
void dng_cellmap_emplace_room(struct dng_cellmap *cellmap, struct dng_room *room);

// INSPECTORS
void dng_cellmap_inspect_spiral_cells(struct dng_cellmap *cellmap, bool (*inspect)(struct dng_cell*));
void dng_cellmap_inspect_cells_in_dimension(struct dng_cellmap *cellmap, int x, int y, int w, int h, bool (*inspect)(struct dng_cell*));
bool dng_cellmap_can_house_dimension(struct dng_cellmap *cellmap, int x, int y, int w, int h);
struct dng_cell* dng_cellmap_get_cell_at_position(struct dng_cellmap *cellmap, int x, int y);
struct dng_cell* dng_cellmap_get_cell_at_position_nullable(struct dng_cellmap *cellmap, int x, int y);


///////////////////////////
// OVERALL DUNGEON

struct dng_cellmap* dng_genmap(int difficulty, int width, int height);
void dng_delmap(struct dng_cellmap*);

#endif
