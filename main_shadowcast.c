#include <stdlib.h>
#include <stdio.h>
#include "dm_algorithm.h"

int get_index(int x, int y, int width)
{
	return y * width + x;
}

void print_cast(int *cast, int width, int height)
{
	for (int r=0; r < height; r++) {
		for (int c=0; c < width; c++) {
			int index = get_index(c, r, width);
			if (cast[index] == 0)
				printf(". ");
			else
				printf("%d ", cast[index]);
		}
		printf("\n");
	}
	printf("\n");
	getchar();
}

int main(void)
{
	int map[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};
	int cast[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};
	int width = 12;
	int height = 12;

	// print base map
	for (int r=0; r < height; r++) {
		for (int c=0; c < width; c++) {
			int index = r * width + c;
			if (map[index] == 0)
				printf(". ");
			else
				printf("# ");
		}
		printf("\n");
	}
	printf("\n");

	getchar();

	int my_ss_id = 0;
	// step through a shadowcast
	bool sc_isblocked(int x, int y)
	{
		int index = get_index(x, y, width);
		return map[index] == 1;
	}
	void sc_onvisible(int x, int y, double radius, unsigned int ss_id)
	{
		int index = get_index(x, y, width);
		cast[index]++;
		print_cast(cast, width, height);
	}
	dm_shadowcast(5, 5, width, height, 5, sc_isblocked, sc_onvisible, false); // no leakage allowed

	print_cast(cast, width, height);
}

