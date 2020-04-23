#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "dm_defines.h"
#include "mt_rand.h"
#include "dm_algorithm.h"
#include "dm_debug.h"

#include <stdio.h> // TODO REMOVE

///////////////////////////
// SHADOWCASTING

unsigned int rand_seed = 0;
unsigned int shadowcast_id = 0;
int shadow_multiples[4][8] = {
    {1,  0,  0, -1, -1,  0,  0,  1},
    {0,  1, -1,  0,  0, -1,  1,  0},
    {0,  1,  1,  0,  0, -1, -1,  0},
    {1,  0,  0,  1, -1,  0,  0, -1},
};

void dm_shadowcast_r(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double), int octant, int row, double start_slope, double end_slope, int xx, int xy, int yx, int yy)
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
			// Commenting this out to remove dependency on Map
			// looks like bounds checking for perf boost in edget of map
			//if (ax >= map.shadowcastGetWidth() || ay >= map.shadowcastGetHeight())
			if (ax >= xmax || ay >= ymax)
				continue;

			// Our light beam is touching this square; light it
			int radius2 = radius * radius;
			if ((int)(dx * dx + dy * dy) < radius2)
				on_visible(ax, ay, (double)i / (double)radius);

			if (blocked) {
				// We're scanning a row of blocked squares
				if (is_blocked(ax, ay)) {
					next_start_slope = r_slope;
					continue;
				} else {
					blocked = false;
					start_slope = next_start_slope;
				}
			} else if (is_blocked(ax, ay)) {
				blocked = true;
				next_start_slope = r_slope;
				dm_shadowcast_r(x, y, xmax, ymax, radius, is_blocked, on_visible, octant, i + 1, start_slope, l_slope, xx, xy, yx, yy);
			}
		}

		if (blocked)
			break;
	}
}

void dm_shadowcast(int x, int y, int xmax, int ymax, unsigned int radius, bool (*is_blocked)(int, int), void (*on_visible)(int, int, double))
{
	shadowcast_id++;
	on_visible(x, y, 0);

	for (int i = 0; i < 8; i++) {
		dm_shadowcast_r(x, y, xmax, ymax, radius, is_blocked, on_visible, i, 1, 1.0, 0.0, shadow_multiples[0][i], shadow_multiples[1][i], shadow_multiples[2][i], shadow_multiples[3][i]);
	}
}



///////////////////////////
// SPIRAL
struct dm_spiral dm_spiral(int maxlayers)
{
	struct dm_spiral s = {
		0, 0, 0, 1, maxlayers
	};
	return s;
}
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


///////////////////////////
// BRESENHAM LINE
// http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm - Algorithm
// http://roguebasin.roguelikedevelopment.org/index.php?title=Bresenham%27s_Line_Algorithm - Rogue Code
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


///////////////////////////
// FLOODFILL
int __dm_floodfill_id = 0;
void dm_floodfill(int x, int y, bool (*is_blocked)(int, int, int), void (*on_flood)(int, int, int))
{
	dm_floodfill_r(x, y, is_blocked, on_flood, 0);
	__dm_floodfill_id++;
}
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
int dm_floodfill_id()
{
	return __dm_floodfill_id;
}


///////////////////////////
// RANDOM

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

int dm_randii(int a, int b)
{
	int len = b - a;
	return (int)(dm_randf() * len + a);
}



///////////////////////////
// MATH
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




void dm_astar_reset(struct dm_astarnode* node)
{
	node->aStarGCost = 0;
	node->aStarHCost = 0;
	node->aStarFCost = 0;
	node->aStarOpened = false;
	node->aStarClosed = false;
	node->aStarParent = NULL;
	// update the x and y coords of this astar node
	node->aStarX = node->get_x(node);
	node->aStarY = node->get_y(node);
}
void dm_astar_clean(struct dm_astarnode* node, unsigned int aStarID)
{
    if (node->aStarID != aStarID) {
        node->aStarID = aStarID;
        dm_astar_reset(node);
    }
}
bool dm_astar_equals(struct dm_astarnode* node_a, struct dm_astarnode* node_b)
{
	return node_a->aStarX == node_b->aStarX && node_a->aStarY == node_b->aStarY;
}
void dm_astarlist_push(struct dm_astarlist *list, struct dm_astarnode *node)
{
	//printf("astar push %d %d\n", list->length, list->capacity);
	// if out of room expand
	if (list->length == list->capacity) {
		struct dm_astarnode **new_list = (struct dm_astarnode**)malloc(ASTAR_LIST_LENGTH * sizeof(struct dm_astarnode*));
		for (unsigned int i=0; i<list->length; i++) {
			printf("astar copy %d\n", i);
			new_list[i] = list->list[i];
		}
		free(list->list);
		list->list = new_list;
		list->capacity += ASTAR_LIST_LENGTH;
	}
	//printf("astar mallocd %d %d\n", list->length, list->capacity);

	// copy to the next spot
	list->list[list->length] = node;
	list->length++;
}
void dm_astarlist_remove(struct dm_astarlist *list, struct dm_astarnode *node)
{
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

static unsigned int aStarID;
struct neighbor {
	int x, y;
};

void dm_astar(
	struct dm_astarnode *startNode,
	struct dm_astarnode *endNode,
	bool (*is_blocked)(struct dm_astarnode*),
	struct dm_astarnode* (*get_node)(int, int),
	void (*on_path)(struct dm_astarnode*),
	bool is_cardinal_only,
	bool is_manhattan
)
{
	aStarID++;
	dm_astar_clean(startNode, aStarID);
	dm_astar_clean(endNode, aStarID);

	struct dm_astarlist openList = {0, 0, 0, NULL};
	struct dm_astarlist closedList = {0, 0, 0, NULL};

	// add start node to open list
	struct dm_astarnode* currentNode = startNode;
	dm_astarlist_push(&closedList, currentNode);
	currentNode->aStarClosed = true;

	// This was gone and made a massive memory leak before, it is vital!
	dm_astarlist_push(&openList, currentNode);
	currentNode->aStarOpened = true;

	printf("astar while not there begin\n");
	// Perform the path search

	while (dm_astar_equals(currentNode, endNode) == false) {
		// Mechanism for comparing neighbors
		// TODO this was originally a list you could dynamically define
		// but I changed it to just look at cardinal and diagonal neighbors to avoid the list
		printf(" begin %d,%d\n", currentNode->aStarX, currentNode->aStarY);
		if (is_cardinal_only) {
			struct neighbor neighbors[] = {
				{currentNode->aStarX, currentNode->aStarY - 1}, // top
				{currentNode->aStarX, currentNode->aStarY + 1}, // bottom
				{currentNode->aStarX - 1, currentNode->aStarY}, // left
				{currentNode->aStarX + 1, currentNode->aStarY }, // right
			};
			// Loop to look for best candidate via A*
			for (int i=0; i < 4; i++) {
				printf("  starting dir %d %d,%d\n", i, neighbors[i].x, neighbors[i].y);
				struct dm_astarnode *checkNode = get_node(neighbors[i].x, neighbors[i].y);
				if (!checkNode)
					continue;

				printf("  checking neighbor %d %d,%d\n", i, checkNode->aStarX, checkNode->aStarY);
				dm_astar_clean(checkNode, aStarID);

				int xdiff;
				int ydiff;
				int gcost;
				int hcost;

				// G cost for this node
				if (checkNode->aStarGCost == 0) {
					// G = Cost to move from current active node to this node
					//     If this is a locked down grid, you can treat horizontal neighbors
					//     as a straight 10 and diagonal neighbors as a 14
					//     If this is node mapping is not a locked down grid, perhaps a web
					//     or something else, then calculate the distance realistically.
					xdiff = abs(checkNode->aStarX - currentNode->aStarX);
					ydiff = abs(checkNode->aStarY - currentNode->aStarY);
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

					checkNode->aStarGCost = gcost;
				}

				// H cost for this node
				if (checkNode->aStarHCost == 0) {
					// H = Cost to move from this node to the destination node.
					//     Use manhattan distance (total x distance + total y distance)
					//     Or use real distance squareRoot( x distance ^ 2, y distance ^ 2)
					//     Or some other heuristic if you are brave
					xdiff = checkNode->aStarX - endNode->aStarX;
					ydiff = checkNode->aStarY - endNode->aStarY;
					if(is_manhattan)
						hcost = xdiff + ydiff;
					else
						hcost = (int)sqrt((double)(xdiff * xdiff) + (double)(ydiff * ydiff));
					checkNode->aStarHCost = hcost;
				}

				// F cost for this node
				if (checkNode->aStarFCost == 0) {
					// F = G + H
					// F = Cost to move from current active node to this node
					//     plus the cost for this mode to travel to the final node
					//     (calculated by manhattan distance or real distance etc.)
					checkNode->aStarFCost = checkNode->aStarGCost + checkNode->aStarHCost;
				}

				// Skip nodes that are blocked or already closed
				if (!is_blocked(checkNode) && !checkNode->aStarClosed) {
					//printf("--- in\n");
					if (!checkNode->aStarOpened) {
						printf("    astarpush open\n");
						// If the connected node is not in the open list, add it to the open list
						// and set its parent to our current active node
						checkNode->aStarParent = currentNode;
						dm_astarlist_push(&openList, checkNode);
						checkNode->aStarOpened = true;
					} else {
						//printf("--- astarclosed\n");
						// If the connected node is already in the open list, check to see if
						// the path from our current active node to this node is better
						// than are current selection
						// Check to see if its current G cost is less than the new G cost of the parent and the old G cost
						gcost = checkNode->aStarGCost + currentNode->aStarGCost;
						if (gcost < checkNode->aStarGCost) {
							// If so, make the current node its new parent and recalculate the gcost, and fcost
							checkNode->aStarParent = currentNode;
							checkNode->aStarGCost = gcost;
							checkNode->aStarFCost = checkNode->aStarGCost + checkNode->aStarHCost;
						}
					}
				}
				//printf("  end dir\n");
			} // End neighbor loop
		} // end cardinal version TODO
		printf(" end neighbor loop\n");

		// At this point the open list has been updated to reflect new parents and costs

		// Find the node in the open list with the lowest F cost,
		// (the total cost from the current active node to the open node
		// and the guesstimated cost from the open node to the destination node)
		struct dm_astarnode *cheapOpenNode = NULL;
		for (int i=0; i < openList.length; i++) {
			// Compare the openList nodes for the lowest F Cost
			if (cheapOpenNode == NULL) {
				// initialize our cheapest open node
				cheapOpenNode = openList.list[i];
				continue;
			}

			if (openList.list[i]->aStarFCost < cheapOpenNode->aStarFCost) {
				// we found a cheaper open list node
				cheapOpenNode = openList.list[i];
			}
		}

		// We have run out of options, no shortest path, circumvent and leave
		if (cheapOpenNode == NULL)
			return;

		// Now we have the node from the open list that has the cheapest F cost
		// move it to the closed list and set it as the current node
		dm_astarlist_remove(&openList, cheapOpenNode);
		cheapOpenNode->aStarOpened = false;

		dm_astarlist_push(&closedList, cheapOpenNode);
		cheapOpenNode->aStarClosed = true;

		currentNode = cheapOpenNode;
	} // A* Complete
	printf("astar complete\n");

	// we have found the end node
	// Loop from the current/end node moving back through the parents until we reach the start node
	// add those to the list and we have our path
	struct dm_astarnode *workingNode = currentNode;
	while (true) {
		// If I have traversed back to the beginning of the linked path
		if (workingNode->aStarParent == NULL) {
			// Push the final game object
			on_path(workingNode);
			//fill.push_back(workingNode);
			break;
		} else {
			// If I have more traversal to do
			// Push the current game object
			on_path(workingNode);
			//fill.push_back(workingNode);
			// Update my working object to my next parent
			workingNode = workingNode->aStarParent;
		}
	}

	//return fill;
}
