#include <stdio.h>
#include "dm_algorithm.h"

int countAliveNeighbours(int *map, int x, int y, int worldWidth, int worldHeight)
{
	int count = 0;
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			int nb_x = i+x;
			int nb_y = j+y;
			int nb_index = nb_y * worldWidth + nb_x;
			if (i == 0 && j == 0)
				continue;
			//If it's at the edges, consider it to be solid (you can try removing the count = count + 1)
			if (nb_x < 0 || nb_y < 0 || nb_x >= worldWidth || nb_y >= worldHeight)
				count = count + 1;
			else if (map[nb_index] == 1)
				count = count + 1;
		}
	}
	return count;
}


void doSimulationStep(int *map, double *life, int worldWidth, int worldHeight, int deathLimit, int birthLimit)
{
	for (int x = 0; x < worldWidth; x++) {
		for (int y = 0; y < worldHeight; y++) {
			int index = y * worldWidth + x;
			int nbs = countAliveNeighbours(map, x, y, worldWidth, worldHeight);
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

void initialiseMap(int *map, double *life, int worldWidth, int worldHeight, double chanceToStartAlive)
{
	for (int x=0; x < worldWidth; x++)
	{
		for (int y=0; y < worldHeight; y++)
		{
			int index = y * worldWidth + x;
			if (dm_randf() < chanceToStartAlive)
				//We're using numbers, not booleans, to decide if something is solid here. 0 = not solid
				map[index] = 1;
			else
				map[index] = 0;
		}
	}
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
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};
	double life[] = {
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

	double chanceToStartAlive = 0.4;
	int deathLimit = 3;
	int birthLimit = 4;
	int numberOfSteps = 2;


	initialiseMap(map, life, width, height, chanceToStartAlive);

	for(int i=0; i<numberOfSteps; i++){
		 doSimulationStep(map, life, width, height, deathLimit, birthLimit);
	}

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
}
