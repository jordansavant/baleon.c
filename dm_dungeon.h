#ifndef DM_DUNGEON
#define DM_DUNGEON

#include "dm_defines.h"
#include "dm_algorithm.h"


struct dng_exit {
        // Exit(unsigned int id, unsigned int x, unsigned int y, unsigned int childMapId = 0, unsigned int childEntranceId = 0);
        int id;
        int x, y;
        bool is_connected_to_child;
        int child_map_id;
        int child_entrance_id;
};

struct dng_entrance {
	// Entrance(unsigned int id, unsigned int x, unsigned int y, unsigned int parentMapId = 0, unsigned int parentExitId = 0);
	int id;
	int x, y;
	bool is_connected_to_parent;
	int parent_map_id;
	int parent_exit_id;
};

struct dng_sill_data {
	int door_x;
	int door_y;
};

struct dng_room {
	int x, y;
	int width, height;
	int entrance_weight;
	bool is_machine_room;

	//unsigned int cellCount();
};

struct dng_roomdoor {
        int x, y;
	//  TODO door types
        // DoorType doorType;
        struct dng_room* parent_room;
	int dir_x, dir_y;
};

struct dng_cell {
        int index;
        int x, y;

	bool is_wall;

	struct dng_room* room;
        bool is_room_edge;
        bool is_room_perimeter;

        bool is_tunnel;
        bool was_corridor_tunnel;
        bool was_door_tunnel;
        bool was_room_fix_tunnel;

	struct dng_sill_data sill_data;
        bool is_sill;
        bool is_door;
        bool was_door;
	struct dng_roomdoor door; // TODO was a pointer in xogeni (seems to work as stack based)

        bool is_entrance;
        int entrance_id;
        int entrance_priority;
        bool is_entrance_transition;
	struct dng_entrance *entrance_transition;
        bool is_exit_transition;
	struct dng_exit *exit_transition;
	int transition_dir_x, transition_dir_y;
        int metadata_flood_id;

	struct dm_astarnode *astar_node;
};

struct tunnel_dir {
	int x, y;
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

	// entrance details
	int entrance_count, exit_count;
	struct dng_entrance *entrance;
	//struct dng_exit *exit; TODO
	struct dng_room *entrance_room;
	struct dng_room *exit_room;
};


///////////////////////////
// CELLMAP BUILDERS
void dng_cellmap_buildground(struct dng_cellmap*);
void dng_cell_init(struct dng_cell*);
int dng_cell_get_x(struct dm_astarnode *astar_node);
int dng_cell_get_y(struct dm_astarnode *astar_node);

// ROOMS
void dng_cellmap_buildrooms(struct dng_cellmap*);
struct dng_room* dng_cellmap_buildroom(struct dng_cellmap *cellmap);
void dng_room_init(struct dng_room *room, int x, int y, int w, int h);
void dng_cellmap_emplace_room(struct dng_cellmap *cellmap, struct dng_room *room);

// TUNNELS
void dng_cellmap_buildtunnels(struct dng_cellmap*);
void dng_cellmap_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir last_dir);
bool dng_cellmap_open_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir dir);
bool dng_cellmap_can_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir dir);
void dng_cellmap_emplace_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell, struct tunnel_dir dir);
void dng_cellmap_mark_as_tunnel(struct dng_cellmap *cellmap, struct dng_cell *cell);
void dng_get_shuffled_directions(struct tunnel_dir *dirs);

// DOORS
void dng_cellmap_builddoors(struct dng_cellmap *cellmap);
void dng_cellmap_open_room(struct dng_cellmap *cellmap, struct dng_room *room);
void dng_cellmap_emplace_door(struct dng_cellmap* cellmap, struct dng_room *room, struct dng_cell *cell_sill);
void dng_cellmap_set_room_sills(struct dng_cellmap* cellmap, struct dng_room *room, void (*on_set)(struct dng_cell*));
void dng_cellmap_connect_door(struct dng_cellmap *cellmap, struct dng_roomdoor *door);

// ENTRANCE
void dng_cellmap_buildentrance(struct dng_cellmap *cellmap);
void dng_entrance_init(struct dng_entrance *entrance, int id, int x, int y);
struct dng_cell* dng_cellmap_pick_transition_cell_for_room(struct dng_cellmap *cellmap, struct dng_room *room);
void dng_cellmap_build_landing_pad(struct dng_cellmap *cellmap, struct dng_cell* start_cell, int entrance_id);

// CLEANERS
void dng_cellmap_cleanup_connections(struct dng_cellmap *cellmap);
void dng_cellmap_collapse_tunnels(struct dng_cellmap *cellmap);
int dng_cellmap_count_tunnel_connections(struct dng_cellmap *cellmap, struct dng_cell *tunnel_cell);
void dng_cellmap_collapse(struct dng_cellmap *cellmap, struct dng_cell *tunnel_cell);
void dng_cellmap_fix_doors(struct dng_cellmap *cellmap);
void dng_cellmap_fix_rooms(struct dng_cellmap *cellmap);
bool dng_cellmap_are_rooms_connected(struct dng_cellmap *cellmap, struct dng_room *room_a, struct dng_room *room_b);
void dng_cellmap_get_room_connection_path(struct dng_cellmap *cellmap, struct dng_room *room_a, struct dng_room *room_b, void (*inspect)(struct dng_cell *cell));
void dng_cellmap_tunnel_rooms(struct dng_cellmap *cellmap, struct dng_room *room_a, struct dng_room *room_b, bool stop_on_room);
void dng_cellmap_emplace_room_fix(struct dng_cellmap *cellmap, struct dng_cell *cell);

// CALC WEIGHTS
void dng_cellmap_calc_entrance_weights(struct dng_cellmap *cellmap);

// EXIT
void dng_cellmap_buildexit(struct dng_cellmap *cellmap);
void dng_exit_init(struct dng_exit *exit, int id, int x, int y);

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
