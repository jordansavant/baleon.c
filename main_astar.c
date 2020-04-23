#include <stdlib.h>
#include <stdio.h>
#include "dm_algorithm.h"

int main(void)
{
	//int map[] = {
	//	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,
	//	1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1,
	//	1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1,
	//	1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
	//	1, 1, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1,
	//	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1,
	//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
	//	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1,
	//	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1,
	//	1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	//};
	//int path[] = {
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//};
	//int width = 12;
	//int height = 12;
	int map[] = {
		1, 2, 1, 1,
		1, 2, 1, 1,
		1, 1, 1, 1,
		1, 1, 2, 1
	};
	int path[] = {
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
	};
	int width = 4, height = 4;

	struct cell {
		int x, y;
		struct dm_astarnode *astar_node;
	};
	int cell_get_x(struct dm_astarnode *astar_node)
	{
		struct cell *cell = (struct cell*)astar_node->owner;
		return cell->x;
	}
	int cell_get_y(struct dm_astarnode *astar_node)
	{
		struct cell *cell = (struct cell*)astar_node->owner;
		return cell->y;
	}

	struct cell *startCell;
	struct cell *endCell;

	// build astar lists
	struct cell **celllist = (struct cell**)malloc(ARRAY_SIZE(map) * sizeof(struct cell*));
	for (int r=0; r<height; r++) {
		for (int c=0; c<width; c++) {
			struct cell *cell = (struct cell*)malloc(sizeof(struct cell));
			struct dm_astarnode *node = (struct dm_astarnode*)malloc(sizeof(struct dm_astarnode));

			cell->astar_node = node;
			node->owner = (void*)cell;
			node->get_x = cell_get_x;
			node->get_y = cell_get_y;

			int index = r * width + c;
			cell->x = c;
			cell->y = r;
			celllist[index] = cell;

			// assign top left to start and bottom right to end
			if (index == 0)
				startCell = cell;
			if (index == ARRAY_SIZE(map) - 1)
				endCell = cell;
		}
	}

	bool is_blocked(struct dm_astarnode* node) {
		struct cell *cell = (struct cell*)node->owner;
		int index = cell->y * width + cell->x;
		bool blocked = map[index] != 1;
		if (blocked)
			printf("   is_blocked %d %d,%d\n", index, node->astar_x, node->astar_y);
		else
			printf("   isnt_blocked %d %d,%d\n", index, node->astar_x, node->astar_y);
		return blocked;

	}
	struct dm_astarnode* get_node(int x, int y) {
		int index = y * width + x;
		//printf("   get_node %d %d,%d\n", index, x, y);
		if (x >= 0 && x < width && y >= 0 && y < height)
			return celllist[index]->astar_node;
		return NULL;
	}
	void on_path(struct dm_astarnode* node) {
		struct cell *cell = (struct cell*)node->owner;
		int index = cell->y * width + cell->x;
		printf("on_path %d %d,%d\n", index, cell->x, cell->y);
		path[index] = 1; // todo show path being made?
	}

	printf("start astar\n");
	dm_astar(startCell->astar_node, endCell->astar_node, is_blocked, get_node, on_path, true, true);

	for (int r=0; r < height; r++) {
		for (int c=0; c < width; c++) {
			int index = r * width + c;
			if (map[index] == 1)
				printf(". ");
			else
				printf("# ");
		}
		printf("\n");
	}
	printf("\n");

	for (int r=0; r < height; r++) {
		for (int c=0; c < width; c++) {
			int index = r * width + c;
			if (path[index] == 0)
				printf(". ");
			else
				printf("p ");
		}
		printf("\n");
	}

	// free mem
	for (int i=0; i<ARRAY_SIZE(map); i++) {
		free(celllist[i]);
	}
	free(celllist);
}
