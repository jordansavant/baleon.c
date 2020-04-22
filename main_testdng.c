#include <stdio.h>
#include "dm_algorithm.h"
#include "dm_dungeon.h"

int main(void)
{
	dm_seed(100);
	struct dng_cellmap *cellmap = dng_genmap(1, 72, 48);

	for (int r=0; r < cellmap->height; r++) {
		for (int c=0; c < cellmap->width; c++) {
			int index = r * cellmap->width + c;
			struct dng_cell *cell = cellmap->cells[index];
			//if (cell->is_room_perimeter)
			//	printf("P ");
			if (cell->is_entrance_transition)
				printf("E ");
			else if (cell->is_entrance)
				printf("e ");
			else if (cell->is_tunnel)
				printf("T ");
			else if (cell->is_door)
				printf("D ");
			else if (cell->is_sill)
				printf("S ");
			else if (cell->room != NULL)
				printf("- ");
			else
				printf("  ");
		}
		printf("\n");
	}

	dng_delmap(cellmap);

	return 0;
}
