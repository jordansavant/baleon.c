#include <stdio.h>
#include "dm_algorithm.h"
#include "dm_dungeon.h"

int main(void)
{
	int seed = 1501131732;
	do {
		char *red = "\033[0;31m";
		char *green = "\033[0;32m";
		char *blue = "\033[0;34m";
		char *def = "\033[0m";
		printf("SEED %d\n", seed);
		struct dng_dungeon *dungeon = dng_gendungeon(seed, 16, true);

		for (int i=0; i < dungeon->maps_length; i++) {
			printf("MAP %d\n", i);

			struct dng_cellmap* cellmap = dungeon->maps[i];

			// top ruler
			printf("    ");
			for (int c=0; c < cellmap->width; c++) {
				if (c % 10 == 0)
					if (c == 0)
						printf("%d ", c / 10);
					else
						printf("%19d ", c / 10);
			}
			printf("\n");
			printf("    ");
			for (int c=0; c < cellmap->width; c++) {
				printf("%d ", c %10);
			}
			printf("\n");

			for (int r=0; r < cellmap->height; r++) {
				printf("%3d-", r);
				for (int c=0; c < cellmap->width; c++) {
					int index = r * cellmap->width + c;
					struct dng_cell *cell = cellmap->cells[index];
					//if (cell->is_room_perimeter)
					//	printf("P ");
					if (false)
						printf("skip");
					else if (cell->tile_style == DNG_TILE_STYLE_SUMMONCIRCLE)
						printf("S ");
				//	else if (cell->temp_wall)
				//		printf("w ");
					else if (cell->has_mob)
						printf("m ");
					else if (cell->has_item) {
						if (cell->item_style == DNG_ITEM_KEY)
							printf("k ");
						else
							printf("i ");
					}
					else if (cell->is_tag_unreachable)
						printf("U ");
					else if (cell->is_entrance_transition)
						printf("E ");
					else if (cell->is_tutorial)
						printf("* ");
					else if (cell->is_entrance)
						printf("e ");
					else if (cell->is_exit_transition)
						printf("X ");
					else if (cell->is_door)
						printf("+ ");
					//else if (cell->was_room_fix_tunnel)
					//	printf("F ");
					else if (cell->is_tunnel)
						printf("  ");
					//	printf("T ");
					//else if (cell->was_corridor_tunnel)
					//	printf("t ");
					//else if (cell->was_door)
					//	printf("d ");
					//else if (cell->is_sill)
					//	printf("S ");
					else if (cell->is_wall)
						printf(". ");
					else if (cell->room != NULL && cell->room->is_room_isolated)
						printf("%s. %s", blue, def);
					else {
						// cell rooms, cellular openings, default
						if (cell->tile_style == DNG_TILE_STYLE_GRASS)
							printf("%s\" %s", green, def);
						else if (cell->tile_style == DNG_TILE_STYLE_WATER)
							printf("%s~ %s", blue, def);
						else if (cell->tile_style == DNG_TILE_STYLE_DEEPWATER)
							printf("%sW %s", blue, def);
						else
							printf("  ");
					}
				}
				printf("\n");
			}

			getchar();
		}

		dng_deldungeon(dungeon);

		seed++;
	} while(true);

	return 0;
}
