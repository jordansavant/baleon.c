#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include "dm_defines.h"
#include "mt_rand.h"
#include "dm_algorithm.h"
#include "dm_debug.h"

// #include <stdio.h> // debugging only



///////////////////////////
// SHADOWCASTING START

unsigned int rand_seed = 0;
unsigned int shadowcast_id = 0;
int shadow_multiples[4][8] = {
    {1,  0,  0, -1, -1,  0,  0,  1},
    {0,  1, -1,  0,  0, -1,  1,  0},
    {0,  1,  1,  0,  0, -1, -1,  0},
    {1,  0,  0,  1, -1,  0,  0, -1},
};

bool dm_sc_is_leakageblocked(int x, int y, int ax, int ay, bool (*is_blocked)(int, int))
{
	double dirf_x, dirf_y;
	dm_direction((double)x, (double)y, (double)ax, (double)ay, &dirf_x, &dirf_y);
	int rx = (int)dm_ceil_out(dirf_x);
	int ry = (int)dm_ceil_out(dirf_y);
	//if (!is_blocked(ax, ay)) { // uncomment this if you want to show blocked cells, but it hints at things on the map if you do
	if (is_blocked(ax - rx, ay) && is_blocked(ax, ay - ry)) {
		return true;
	}
	//}
	return false;
}

void dm_shadowcast_r(
	int x, int y,
	int xmax, int ymax,
	unsigned int radius,
	bool (*is_blocked)(int, int),
	void (*on_visible)(int, int, double, unsigned int),
	bool allow_leakage,
	int octant, int row, double start_slope, double end_slope,
	int xx, int xy, int yx, int yy
)
{
	// If light start is less than light end, return
	if (start_slope < end_slope)
		return;

	double next_start_slope = start_slope;

	// Loop moves outward from current row (or column if octant) to final radius in rows
	// row is passed in since this is recursive
	for (int i = row; i <= radius; i++) {
		bool blocked = false;

		// Loop moves across columns (or row if octant) until the right most
		// border is reached
		for (int dx = -i, dy = -i; dx <= 0; dx++) {
			// l_slope and r_slope store the slopes of the left and right extremities of the square we're considering:
			double l_slope = (dx - 0.5f) / (dy + 0.5f);
			double r_slope = (dx + 0.5f) / (dy - 0.5f);

			if (start_slope < r_slope)
				continue;
			else if (end_slope > l_slope)
				break;

			int sax = dx * xx + dy * xy;
			int say = dx * yx + dy * yy;
			if ((sax < 0 && abs(sax) > x) || (say < 0 && abs(say) > y))
				continue;

			int ax = x + sax;
			int ay = y + say;

			// Dont look out of bounds
			if (ax >= xmax || ay >= ymax)
				continue;

			// Todo, check to see if we are leaking through diagonal walls?
			bool leakage_blocked = false;
			if (!allow_leakage) {
				leakage_blocked = dm_sc_is_leakageblocked(x, y, ax, ay, is_blocked);
			}

			// Our light beam is touching this square; light it
			int radius2 = radius * radius;
			if ((int)(dx * dx + dy * dy) < radius2 && !leakage_blocked)
				on_visible(ax, ay, (double)i / (double)radius, shadowcast_id);

			if (blocked) {
				// We're scanning a row of blocked squares
				if (is_blocked(ax, ay) || leakage_blocked) {
					next_start_slope = r_slope;
					continue;
				} else {
					blocked = false;
					start_slope = next_start_slope;
				}
			} else if (is_blocked(ax, ay) || leakage_blocked) {
				blocked = true;
				next_start_slope = r_slope;
				dm_shadowcast_r(x, y, xmax, ymax, radius, is_blocked, on_visible, allow_leakage, octant, i + 1, start_slope, l_slope, xx, xy, yx, yy);
			}
		}

		if (blocked)
			break;
	}
}

void dm_shadowcast(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double, unsigned int), bool allow_leakage)
{
	shadowcast_id++;
	on_visible(x, y, 0, shadowcast_id);

	for (int i = 0; i < 8; i++) {
		dm_shadowcast_r(x, y, xmax, ymax, radius, is_blocked, on_visible, allow_leakage, i, 1, 1.0, 0.0, shadow_multiples[0][i], shadow_multiples[1][i], shadow_multiples[2][i], shadow_multiples[3][i]);
	}
}

// SHADOWCASTING END
///////////////////////////



///////////////////////////
// SPIRAL START

// this process steps right, down, left, up (and then right again)
// to form a spiral to the layer specified
// "leg" is what direction it is going
// it follows its leg until it hits the next layer edge
// "layer" is how expanded the legs will run
bool dm_spiralnext(struct dm_spiral *s)
{
	switch (s->leg) {
	// Step right
	case 0:
		++s->x;
		// since stepping right finishes a layers circle we break if it has finished
		if (s->maxlayers > -1 && s->x > s->maxlayers)
			return false;
		if (s->x == s->layer)
			++s->leg;
		break;
	// Step down
	case 1:
		++s->y;
		if (s->y == s->layer)
			++s->leg;
		break;
	// Step left
	case 2:
		--s->x;
		if (-s->x == s->layer)
			++s->leg;
		break;
	// Step up
	case 3:
		--s->y;
		if (-s->y == s->layer) {
			s->leg = 0;
			++s->layer;
		}
		break;
	}

	return true;
}

struct dm_spiral dm_spiral(int maxlayers)
{
	struct dm_spiral s = {
		0, 0, 0, 1, maxlayers
	};
	return s;
}

// SPIRAL END
///////////////////////////



///////////////////////////
// BRESENHAM LINE START
// http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm - Algorithm
// http://roguebasin.roguelikedevelopment.org/index.php?title=Bresenham%27s_Line_Algorithm - Rogue Code

// ------------------
// BR PUBLIC

void dm_bresenham(int x1, int y1, int x2, int y2, bool (*is_blocked)(int, int), void (*on_visible)(int, int))
{
    if (is_blocked != NULL && is_blocked(x1, y1))
	    return;

    int delta_x = x2 - x1;
    // if x1 == x2, then it does not matter what we set here
    signed char const ix = (delta_x > 0) - (delta_x < 0);
    delta_x = abs(delta_x) << 1;

    int delta_y = y2 - y1;
    // if y1 == y2, then it does not matter what we set here
    signed char const iy = (delta_y > 0) - (delta_y < 0);
    delta_y = abs(delta_y) << 1;

    if (on_visible != NULL)
	    on_visible(x1, y1);

    if (delta_x >= delta_y) {
        // error may go below zero
        int error = delta_y - (delta_x >> 1);
        while (x1 != x2) {
            if ((error >= 0) && (error || (ix > 0))) {
                error -= delta_x;
                y1 += iy;
            }
            // else do nothing

            error += delta_y;
            x1 += ix;

	    if (is_blocked != NULL && is_blocked(x1, y1))
		    return;

	    if (on_visible != NULL)
		    on_visible(x1, y1);
        }
    } else {
        // error may go below zero
        int error = delta_x - (delta_y >> 1);
        while (y1 != y2) {
            if ((error >= 0) && (error || (iy > 0))) {
                error -= delta_y;
                x1 += ix;
            }
            // else do nothing

            error += delta_x;
            y1 += iy;

	    if (is_blocked != NULL && is_blocked(x1, y1))
		    return;

	    if (on_visible != NULL)
		    on_visible(x1, y1);
        }
    }
}

// BRESENHAM END
///////////////////////////



///////////////////////////
// FLOODFILL START

// ------------------
// FF DATA

int __dm_floodfill_id = 0;


// ------------------
// FF PRIVATE

void dm_floodfill_r(int x, int y, bool (*is_blocked)(int, int, int), void (*on_flood)(int, int, int), int depth)
{
	if (is_blocked(x, y, depth))
		return;

	on_flood(x, y, depth);

	depth++;
	dm_floodfill_r(x + 1, y, is_blocked, on_flood, depth);
	dm_floodfill_r(x - 1, y, is_blocked, on_flood, depth);
	dm_floodfill_r(x, y + 1, is_blocked, on_flood, depth);
	dm_floodfill_r(x, y - 1, is_blocked, on_flood, depth);
}


// ------------------
// FF PUBLIC

int dm_floodfill_id()
{
	return __dm_floodfill_id;
}

void dm_floodfill(int x, int y, bool (*is_blocked)(int, int, int), void (*on_flood)(int, int, int))
{
	dm_floodfill_r(x, y, is_blocked, on_flood, 0);
	__dm_floodfill_id++;
}

// FLOODFILL END
///////////////////////////



///////////////////////////
// RANDOM START

void dm_seed(unsigned long seed)
{
	sgenrand(seed);
}

double dm_randf()
{
	return genrandf();
}

int dm_randi()
{
	return (int)(dm_randf() * INT_MAX);
}

// from a up to but not including b
int dm_randii(int a, int b)
{
	int len = b - a;
	return (int)(dm_randf() * len + a);
}

bool dm_chance(int in, int outof)
{
	return ((double)in / (double)outof) > dm_randf();
}

// RANDOM END
///////////////////////////



///////////////////////////
// MATH START

int dm_disti(int x1, int y1, int x2, int y2)
{
	double x = (double)(x1 - x2);
	double y = (double)(y1 - y2);
	return (int)sqrt(x*x + y*y);
}

double dm_distf(double x1, double y1, double x2, double y2)
{
	double x = x1 - x2;
	double y = y1 - y2;
	return sqrt(x*x + y*y);
}

// get the vector direction from point a to point b
void dm_direction(double x1, double y1, double x2, double y2, double *xref, double *yref)
{
	dm_normalize(x2 - x1, y2 - y1, xref, yref);
}

void dm_normalize(double x, double y, double *xref, double *yref)
{
	if (x != 0 || y != 0) {
		double length = sqrt(x*x + y*y);
		(*xref) = x / length;
		(*yref) = y / length;
	} else {
		(*xref) = 0;
		(*yref) = 0;
	}
}

double dm_round(double val)
{
	return floor(val + 0.5);
}

// ceil positive numbers up, negative down
double dm_ceil_out(double val)
{
	if (val >= 0)
		return ceil(val);
	else
		return floor(val);
}

// MATH END
///////////////////////////



///////////////////////////
// STRING START
void dm_splitstr(char *text, char splitter, int m, int n, char words[m][n], int *wordlen)
{
	//this is the mother who likes to speak and use the language of the underground to be able to tell players what they can and cannot do
	// split the text int a bunch of sub strings that represent the words
	int wordcount = 0;
	int spacepos = 0;
	for (int i=0; i < strlen(text) + 1; i++) {
		if (text[i] == splitter || text[i] == '\n' || text[i] == '\0') {
			// copy characters from last space pos to current position
			int wordsize = i - spacepos;
			memcpy(&words[wordcount], &text[spacepos], wordsize);
			words[wordcount][wordsize] = '\0';
			//dmlogf("wordcount %d wordsize %d spacepos %d i %d [%s]", wordcount, wordsize, spacepos, i, words[wordcount]);
			spacepos = i + 1;
			wordcount++;
		}
		if (text[i] == '\n') {
			// we ran into a newline, we want to split and make the prior contents a word, then also inject another word for newline after it
			// special newline word
			words[wordcount][0] = '\n';
			words[wordcount][1] = '\0';
			wordcount++;
		}
	}
	*wordlen = wordcount;
}
void dm_lines(int m, int n, char words[m][n], int sentence_size, int o, int p, char lines[o][p], int *linelen)
{
	// take a collection of words that are null terminated and a sentence size
	// and copy them into the lines buffer
	int cursize = 0;
	int linepos = 0;
	for (int i=0; i < m; i++) {
		char *word = words[i];
		int wordsize = strlen(word);
		if (word[0] == '\n') {
			// its a newline word, move to next line, reset space pos
			cursize = 0;
			linepos++;
		}
		//dmlogf("compare %d + %d < %d", cursize, wordsize, sentence_size);
		else if (cursize + wordsize < sentence_size) {
			// append word to line
			memcpy(&lines[linepos][cursize], word, wordsize);
			lines[linepos][cursize + wordsize] = ' ';
			lines[linepos][cursize + wordsize + 1] = '\0';
			cursize += wordsize + 1;
		} else {
			// move to next line, copy the word there
			linepos++;
			memcpy(&lines[linepos], word, wordsize);
			lines[linepos][wordsize] = ' ';
			lines[linepos][wordsize + 1] = '\0';
			cursize = wordsize + 1;
		}
	}
	*linelen = linepos + 1;
}
void dm_wordwrap(char *text, int size, void (*on_line)(char *line))
{
	int wordlen = 0;
	char words[1056][32];
	dm_splitstr(text, ' ', 1056, 32, words, &wordlen);

	int linelen = 0;
	char lines[64][1056];
	dm_lines(wordlen, 32, words, size, 64, 1056, lines, &linelen);

	for (int i=0; i<linelen; i++) {
		char *line = lines[i]; // null terminated
		on_line(line);
	}
}
// STRING END
///////////////////////////


///////////////////////////
// ASTAR START

#define ASTAR_LIST_LENGTH 25

struct dm_astarnode* dm_astar_newnode()
{
	struct dm_astarnode* node = (struct dm_astarnode*)malloc(sizeof(struct dm_astarnode));
	node->astar_id = 0;
	node->get_x = NULL;
	node->get_y = NULL;
	dm_astar_reset(node);
	return node;
}

void dm_astar_reset(struct dm_astarnode* node)
{
	node->astar_gcost = 0;
	node->astar_hcost = 0;
	node->astar_fcost = 0;
	node->astar_opened = false;
	node->astar_closed = false;
	node->astar_parent = NULL;
	node->astar_child = NULL;
	// update the x and y coords of this astar node
	if (node->get_x)
		node->astar_x = node->get_x(node);
	if (node->get_y)
		node->astar_y = node->get_y(node);
}

void dm_astar_clean(struct dm_astarnode* node, unsigned int astar_id)
{
    if (node->astar_id != astar_id) {
        node->astar_id = astar_id;
        dm_astar_reset(node);
    }
}

bool dm_astar_equals(struct dm_astarnode* node_a, struct dm_astarnode* node_b)
{
	return node_a->astar_x == node_b->astar_x && node_a->astar_y == node_b->astar_y;
}

void dm_astarlist_push(struct dm_astarlist *list, struct dm_astarnode *node)
{
	// if out of room expand
	if (list->length == list->capacity) {
		list->list = (struct dm_astarnode**)realloc(list->list, (list->capacity + ASTAR_LIST_LENGTH) * sizeof(struct dm_astarnode*));
		list->capacity += ASTAR_LIST_LENGTH;
	}

	// copy to the next spot
	list->list[list->length] = node;
	list->length++;
}

void dm_astarlist_remove(struct dm_astarlist *list, struct dm_astarnode *node)
{
	// this does not realloc down, we just shrink the list so new pushes can replace old pushes
	bool found = false;
	// find the node and null it
	// move everything after it one slot back
	// null the last one and shrink the counter
	for (unsigned int i=0; i < list->length; i++) {
		if (found) {
			list->list[i-1] = list->list[i]; // shift them back
		} else if(list->list[i] == node) {
			list->list[i] = NULL;
			found = true;
		}
	}
	if (found) {
		list->list[list->length - 1] = NULL;
		list->length--;
	}
}

void dm_astarlist_free(struct dm_astarlist *list)
{
	free(list->list);
}

static unsigned int dm_astar_id = 0;

struct neighbor {
	int x, y;
};

void dm_astar(
	struct dm_astarnode *start_node,
	struct dm_astarnode *end_node,
	bool (*is_blocked)(struct dm_astarnode*, struct dm_astarnode*),
	struct dm_astarnode* (*get_node)(int, int),
	void (*on_path)(struct dm_astarnode*),
	bool is_cardinal_only,
	bool is_manhattan
)
{
	dm_astar_id++;
	dm_astar_clean(start_node, dm_astar_id);
	dm_astar_clean(end_node, dm_astar_id);

	struct dm_astarlist open_list = {0, 0, 0, NULL};
	struct dm_astarlist closed_list = {0, 0, 0, NULL};

	// add start node to open list
	struct dm_astarnode* current_node = start_node;
	dm_astarlist_push(&closed_list, current_node);
	current_node->astar_closed = true;

	// This was gone and made a massive memory leak before, it is vital!
	dm_astarlist_push(&open_list, current_node);
	current_node->astar_opened = true;

	//printf("astar while not there begin\n");
	// Perform the path search
	while (dm_astar_equals(current_node, end_node) == false) {
		// Mechanism for comparing neighbors
		// This was originally a list you could dynamically define
		// but I changed it to just look at cardinal and diagonal neighbors to avoid the list
		//printf(" begin %d,%d\n", current_node->astar_x, current_node->astar_y);
		if (is_cardinal_only) {
			// cardinal neighbors only
			struct neighbor neighbors[] = {
				{current_node->astar_x, current_node->astar_y - 1}, // top
				{current_node->astar_x, current_node->astar_y + 1}, // bottom
				{current_node->astar_x - 1, current_node->astar_y}, // left
				{current_node->astar_x + 1, current_node->astar_y }, // right
			};
			// Loop to look for best candidate via A*
			for (int i=0; i < 4; i++)
				dm_astar_check(current_node, end_node, neighbors[i].x, neighbors[i].y, &open_list, is_blocked, get_node, is_manhattan);
		} else {
			// all neighbords
			struct neighbor neighbors[] = {
				{current_node->astar_x, current_node->astar_y - 1}, // top
				{current_node->astar_x, current_node->astar_y + 1}, // bottom
				{current_node->astar_x - 1, current_node->astar_y}, // left
				{current_node->astar_x + 1, current_node->astar_y }, // right
				{current_node->astar_x - 1, current_node->astar_y - 1}, // top left
				{current_node->astar_x - 1, current_node->astar_y + 1}, // bottom left
				{current_node->astar_x + 1, current_node->astar_y - 1}, // top right
				{current_node->astar_x + 1, current_node->astar_y + 1}, // bottom right
			};
			for (int i=0; i < 8; i++)
				dm_astar_check(current_node, end_node, neighbors[i].x, neighbors[i].y, &open_list, is_blocked, get_node, is_manhattan);
		}
		//printf(" end neighbor loop\n");

		// At this point the open list has been updated to reflect new parents and costs

		// Find the node in the open list with the lowest F cost,
		// (the total cost from the current active node to the open node
		// and the guesstimated cost from the open node to the destination node)
		struct dm_astarnode *cheap_open_node = NULL;
		//printf("openlist length %d %d\n", open_list.length, cheap_open_node);
		for (int i=0; i < open_list.length; i++) {
			// Compare the open_list nodes for the lowest F Cost
			if (cheap_open_node == NULL) {
				// initialize our cheapest open node
				cheap_open_node = open_list.list[i];
				continue;
			}

			if (open_list.list[i]->astar_fcost < cheap_open_node->astar_fcost) {
				// we found a cheaper open list node
				cheap_open_node = open_list.list[i];
			}
		}

		//printf("openlist length %d %d\n", open_list.length, cheap_open_node);
		// We have run out of options, no shortest path, circumvent and leave
		if (cheap_open_node == NULL) {
			//printf("free lists\n");
			dm_astarlist_free(&open_list);
			//printf("freed list open\n");
			dm_astarlist_free(&closed_list);
			//printf("freed list closed\n");
			//printf("astar abort\n");
			return;
		}

		// Now we have the node from the open list that has the cheapest F cost
		// move it to the closed list and set it as the current node
		dm_astarlist_remove(&open_list, cheap_open_node);
		cheap_open_node->astar_opened = false;

		dm_astarlist_push(&closed_list, cheap_open_node);
		cheap_open_node->astar_closed = true;

		current_node = cheap_open_node;
	} // A* Complete
	//printf("astar complete\n");

	// we have found the end node
	// Loop from the current/end node moving back through the parents until we reach the start node
	// add those to the list and we have our path
	//
	// This used to traverse as working node from end back to beginning
	// I switched it to traverse as working node from beginning to end
	struct dm_astarnode *working_node = current_node; // this is the end node
	struct dm_astarnode *first_node = NULL;
	while (true) {
		// If I have traversed back to the beginning of the linked path
		if (working_node->astar_parent == NULL) {
			//on_path(working_node);
			first_node = working_node;
			break;
		} else {
			// If I have more traversal to do
			// Update my working object to my next parent
			working_node->astar_parent->astar_child = working_node;
			working_node = working_node->astar_parent;
		}
	}
	// Now iterate path from start to end, instead of end to start and report it out
	working_node = first_node;
	while (true) {
		if (working_node->astar_child == NULL) {
			// last node
			on_path(working_node);
			break;
		} else {
			on_path(working_node);
			working_node = working_node->astar_child;
		}
	}

	dm_astarlist_free(&open_list);
	dm_astarlist_free(&closed_list);
}

void dm_astar_check(
	struct dm_astarnode *current_node,
	struct dm_astarnode *end_node,
	int neighbor_x,
	int neighbor_y,
	struct dm_astarlist *open_list,
	bool (*is_blocked)(struct dm_astarnode*, struct dm_astarnode*),
	struct dm_astarnode* (*get_node)(int, int),
	bool is_manhattan
)
{
	//printf("  starting dir %d,%d\n", neighbor_x, neighbor_y);
	struct dm_astarnode *check_node = get_node(neighbor_x, neighbor_y);
	if (!check_node)
		return;

	//printf("  checking neighbor %d,%d\n", check_node->astar_x, check_node->astar_y);
	dm_astar_clean(check_node, dm_astar_id);

	int xdiff;
	int ydiff;
	int gcost;
	int hcost;

	// G cost for this node
	if (check_node->astar_gcost == 0) {
		// G = Cost to move from current active node to this node
		//     If this is a locked down grid, you can treat horizontal neighbors
		//     as a straight 10 and diagonal neighbors as a 14
		//     If this is node mapping is not a locked down grid, perhaps a web
		//     or something else, then calculate the distance realistically.
		xdiff = abs(check_node->astar_x - current_node->astar_x);
		ydiff = abs(check_node->astar_y - current_node->astar_y);
		gcost = 0;
		if (ydiff > 0 && xdiff > 0) {
			// If diagonal
			if(is_manhattan)
				gcost = (int)((double)(xdiff + ydiff) / 1.4); // 1.4 is rough diagonal length of a square
			else
				gcost = (int)sqrt((double)(xdiff * xdiff) + (double)(ydiff * ydiff));
		} else {
			// If straight
			gcost = xdiff + ydiff; // one has to be zero so it is the length of one side
		}

		check_node->astar_gcost = gcost;
	}

	// H cost for this node
	if (check_node->astar_hcost == 0) {
		// H = Cost to move from this node to the destination node.
		//     Use manhattan distance (total x distance + total y distance)
		//     Or use real distance squareRoot( x distance ^ 2, y distance ^ 2)
		//     Or some other heuristic if you are brave
		xdiff = check_node->astar_x - end_node->astar_x;
		ydiff = check_node->astar_y - end_node->astar_y;
		if(is_manhattan)
			hcost = xdiff + ydiff;
		else
			hcost = (int)sqrt((double)(xdiff * xdiff) + (double)(ydiff * ydiff));
		check_node->astar_hcost = hcost;
	}

	// F cost for this node
	if (check_node->astar_fcost == 0) {
		// F = G + H
		// F = Cost to move from current active node to this node
		//     plus the cost for this mode to travel to the final node
		//     (calculated by manhattan distance or real distance etc.)
		check_node->astar_fcost = check_node->astar_gcost + check_node->astar_hcost;
	}

	// Skip nodes that are blocked or already closed
	if (!is_blocked(current_node, check_node) && !check_node->astar_closed) {
		//printf("--- in\n");
		if (!check_node->astar_opened) {
			//printf("    astarpush open\n");
			// If the connected node is not in the open list, add it to the open list
			// and set its parent to our current active node
			check_node->astar_parent = current_node;
			dm_astarlist_push(open_list, check_node);
			check_node->astar_opened = true;
		} else {
			//printf("--- astarclosed\n");
			// If the connected node is already in the open list, check to see if
			// the path from our current active node to this node is better
			// than are current selection
			// Check to see if its current G cost is less than the new G cost of the parent and the old G cost
			gcost = check_node->astar_gcost + current_node->astar_gcost;
			if (gcost < check_node->astar_gcost) {
				// If so, make the current node its new parent and recalculate the gcost, and fcost
				check_node->astar_parent = current_node;
				check_node->astar_gcost = gcost;
				check_node->astar_fcost = check_node->astar_gcost + check_node->astar_hcost;
			}
		}
	}
	//printf("  end dir\n");
}

// ASTAR END
///////////////////////////



///////////////////////////
// CELLULAR AUTOMATA START

void dm_cellular_automata(int width, int height, void (*on_solid)(int, int), void (*on_open)(int, int))
{
	double alive_chance = 0.4;
	int death_max = 3;
	int birth_max = 4;
	int steps = 2;

	dm_cellular_automata_detail(width, height, on_solid, on_open, alive_chance, death_max, birth_max, steps);
}

void dm_cellular_automata_detail(int width, int height, void (*on_solid)(int, int), void (*on_open)(int, int), double alive_chance, int death_max, int birth_max, int steps)
{
	int map[width * height];
	dm_cella_init(map, width, height, alive_chance);

	for (int i=0; i<steps; i++) {
		dm_cella_simulate(map, width, height, death_max, birth_max);
	}

	for (int x=0; x < width; x++) {
		for (int y=0; y < height; y++) {
			if (map[y * width + x] == 0)
				on_open(x, y);
			else
				on_solid(x, y);
		}
	}
}

void dm_cella_init(int *map, int width, int height, double chanceToStartAlive)
{
	for (int x=0; x < width; x++) {
		for (int y=0; y < height; y++) {
			int index = y * width + x;
			if (dm_randf() < chanceToStartAlive)
				//We're using numbers, not booleans, to decide if something is solid here. 0 = not solid
				map[index] = 1;
			else
				map[index] = 0;
		}
	}
}


void dm_cella_simulate(int *map, int width, int height, int deathLimit, int birthLimit)
{
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int index = y * width + x;
			int nbs = dm_cella_count_alive_neighbors(map, x, y, width, height);
			if(map[index] > 0){
				//See if it should die or stay solid
				if (nbs < deathLimit)
					map[index] = 0;
				else
					map[index] = 1;
			} else {
				//See if it should become solid
				if (nbs > birthLimit)
					map[index] = 1;
				else
					map[index] = 0;
			}
		}
	}
}

int dm_cella_count_alive_neighbors(int *map, int x, int y, int width, int height)
{
	int count = 0;
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			int nb_x = i+x;
			int nb_y = j+y;
			int nb_index = nb_y * width + nb_x;
			if (i == 0 && j == 0)
				continue;
			//If it's at the edges, consider it to be solid (you can try removing the count = count + 1)
			if (nb_x < 0 || nb_y < 0 || nb_x >= width || nb_y >= height)
				count = count + 1;
			else if (map[nb_index] == 1)
				count = count + 1;
		}
	}
	return count;
}

// CELLULAR AUTOMATA END
///////////////////////////

