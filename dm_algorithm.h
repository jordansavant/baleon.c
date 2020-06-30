#ifndef DM_ALGORITHM
#define DM_ALGORITHM

#include "dm_defines.h"

// SHADOWCASTING
bool dm_sc_is_leakageblocked(int x, int y, int ax, int ay, bool (*is_blocked)(int, int));
void dm_shadowcast_r(
	int x, int y,
	int xmax, int ymax,
	unsigned int radius,
	bool (*is_blocked)(int, int),
	void (*on_visible)(int, int, double, unsigned int),
	bool allow_leakage,
	int octant, int row, double start_slope, double end_slope,
	int xx, int xy, int yx, int yy
);
void dm_shadowcast(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double, unsigned int), bool allow_leakage);

// SPIRAL
struct dm_spiral {
	int x, y;
	int leg, layer, maxlayers;
};
struct dm_spiral dm_spiral(int maxlayers);
bool dm_spiralnext(struct dm_spiral*);

// BRESENHAM LINE
void dm_bresenham(int x1, int y1, int x2, int y2, bool (*is_blocked)(int, int), void (*on_visible)(int, int));

// FLOODFILL
void dm_floodfill(int x, int y, bool (*is_blocked)(int, int, int), void (*on_flood)(int, int, int));
void dm_floodfill_r(int x, int y, bool (*is_blocked)(int, int, int), void (*on_flood)(int, int, int), int depth);
int dm_floodfill_id();

// RANDOM
void dm_seed(unsigned long seed);
double dm_randf();
int dm_randi();
int dm_randii(int a, int b);
bool dm_chance(int in, int outof);

// MATH
int dm_disti(int x1, int y1, int x2, int y2);
double dm_distf(double x1, double y1, double x2, double y2);
void dm_direction(double x1, double y1, double x2, double y2, double *xref, double *yref);
void dm_normalize(double x, double y, double *xref, double *yref);
double dm_round(double val);
double dm_ceil_out(double val);

// STRING
void dm_splitstr(char *text, char splitter, int m, int n, char words[m][n], int *wordlen);
void dm_lines(int m, int n, char words[m][n], int sentence_size, int o, int p, char lines[o][p], int *linelen);

// ASTAR
struct dm_astarnode {
	unsigned int astar_id;
	int astar_x;
	int astar_y;
	bool astar_closed;
	bool astar_opened;
	int astar_fcost;
	int astar_gcost;
	int astar_hcost;
	struct dm_astarnode* astar_parent;
	struct dm_astarnode* astar_child;
	// helper pointer to something we can reference in the parent world in the getters below
	void *owner;
	// getters for parent world to let me update my astar x and y
	int (*get_x)(struct dm_astarnode*);
	int (*get_y)(struct dm_astarnode*);
};
struct dm_astarlist {
	unsigned int index;
	unsigned int length;
	unsigned int capacity;
	struct dm_astarnode **list;
};
struct dm_astarnode* dm_astar_newnode();
void dm_astar_reset(struct dm_astarnode* node);
void dm_astar_clean(struct dm_astarnode* node, unsigned int astar_id);
bool dm_astar_equals(struct dm_astarnode* node_a, struct dm_astarnode* node_b);
void dm_astarlist_push(struct dm_astarlist *list, struct dm_astarnode *node);
void dm_astarlist_remove(struct dm_astarlist *list, struct dm_astarnode *node);
void dm_astarlist_free(struct dm_astarlist *list);

void dm_astar(
	struct dm_astarnode *start_node,
	struct dm_astarnode *end_node,
	bool (*is_blocked)(struct dm_astarnode*, struct dm_astarnode*),
	struct dm_astarnode* (*get_node)(int, int),
	void (*on_path)(struct dm_astarnode*),
	bool is_cardinal_only,
	bool is_manhattan
);
void dm_astar_check(
	struct dm_astarnode *current_node,
	struct dm_astarnode *end_node,
	int neighbor_x,
	int neighbor_y,
	struct dm_astarlist *open_list,
	bool (*is_blocked)(struct dm_astarnode*, struct dm_astarnode*),
	struct dm_astarnode* (*get_node)(int, int),
	bool is_manhattan
);

// CELLULAR AUTOMATA
int dm_cella_count_alive_neighbors(int *map, int x, int y, int width, int height);
void dm_cella_simulate(int *map, int width, int height, int deathLimit, int birthLimit);
void dm_cella_init(int *map, int width, int height, double chanceToStartAlive);
void dm_cellular_automata(int width, int height, void (*on_solid)(int, int), void (*on_open)(int, int));
void dm_cellular_automata_detail(int width, int height, void (*on_solid)(int, int), void (*on_open)(int, int), double alive_chance, int death_max, int birth_max, int steps);

#endif
