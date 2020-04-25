#include <stdio.h>
#include "dm_algorithm.h"

int main(void)
{
	int seed = 123;
	do {
		printf("SEED %d\n", seed);
		dm_seed(seed);
		seed++;
		int width = 24;
		int height = 24;
		int map[width * height];
		void on_solid(int x, int y) {
			map[y * width + x] = 1;
		}
		void on_open(int x, int y) {
			map[y * width + x] = 0;
		}
		dm_cellular_automata(width, height, on_solid, on_open);

		for (int r=0; r < height; r++) {
			for (int c=0; c < width; c++) {
				int index = r * width + c;
				if (map[index] == 0)
					printf("  ");
				else
					printf("# ");
			}
			printf("\n");
		}

		getchar();
	} while(true);

	return 0;
}
