#include <stdio.h>
#include "dm_dungeon.h"

int main(void)
{
	struct dng_cellmap *cellmap = dng_genmap(1, 32, 32);

	for (int r=0; r < cellmap->height; r++) {
		for (int c=0; c < cellmap->width; c++) {
			int index = r * cellmap->width + c;
			struct dng_cell *cell = cellmap->cells[index];
			if (cell->room != NULL)
				printf("# ");
			else
				printf("  ");
		}
		printf("\n");
	}

	dng_delmap(cellmap);

	return 0;
}
