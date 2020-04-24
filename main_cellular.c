#include "dm_algorith.h"

void countAliveNeighbours(int *map, int x, int y, int worldWith, int worldHeight)
{
	int count = 0;
	for(int i = -1; i < 2; i++){
		for(int j = -1; j < 2; j++){
			int nb_x = i+x;
			int nb_y = j+y;
			if(i == 0 && j == 0){
			}
			//If it's at the edges, consider it to be solid (you can try removing the count = count + 1)
			else if(nb_x < 0 || nb_y < 0 ||
					nb_x >= worldWidth ||
					nb_y >= worldHeight) {
				count = count + 1;
			}
			else if(map[nb_x][nb_y] == 1) {
				count = count + 1;
			}
		}
	}
	return count;
}


void doSimulationStep(int *map, int worldWidth, int worldHeight, int deathLimit, int birthLimit)
{
	//Here's the new map we're going to copy our data into
	//int newmap = [[]];
	for(int x = 0; x < worldWidth; x++){
		//newmap[x] = [];
		for(int y = 0; y < worldHeight; y++)
		{
			//Count up the neighbours
			int nbs = countAliveNeighbours(map, x, y);
			//If the tile is currently solid
			if(map[x][y] > 0){
				//See if it should die
				if(nbs < deathLimit){
					//newmap[x][y] = 0;
					map[x][y] = 0;
				}
				//Otherwise keep it solid
				else{
					//newmap[x][y] = 1;
					map[x][y] = 1;
				}
			}
			//If the tile is currently empty
			else{
				//See if it should become solid
				if(nbs > birthLimit){
					//newmap[x][y] = 1;
					map[x][y] = 1;
				}
				else{
					//newmap[x][y] = 0;
					map[x][y] = 0;
				}
			}
		}
	}

	//return newmap;
}

void initialiseMap(int *map, int worldWidth, int worldHeight, double chanceToStartAlive)
{
	for (var x=0; x < worldWidth; x++)
	{
		for (var y=0; y < worldHeight; y++)
		{
			map[x][y] = 0;
		}
	}

	for (var x=0; x < worldWidth; x++)
	{
		for (var y=0; y < worldHeight; y++)
		{
			//Here we use our chanceToStartAlive variable
			if (dm_randf() < chanceToStartAlive)
				//We're using numbers, not booleans, to decide if something is solid here. 0 = not solid
				map[x][y] = 1;
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
	int width = 12;
	int height = 12;

	double chanceToStartAlive = 0.4;
	int deathLimit = 3;
	int birthLimit = 4;
	int numberOfSteps = 2;


	initialiseMap(&map, width, height, chanceToStartAlive);

	for(var i=0; i<numberOfSteps; i++){
		 map = doSimulationStep(map);
	}

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
}
