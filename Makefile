# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS=-I.
BUILD = main.c game.c dm_gametime.c dm_draw.c dm_world.c dm_debug.c dm_algorithm.c mt_rand.c dm_dungeon.c
LIBS = -lncurses -lmenu -lm
DEPS = game.h dm_gametime.h dm_draw.h dm_world.h dm_debug.h dm_algorithm.h mt_rand.h dm_dungeon.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

make: $(BUILD) $(DEPS)
	gcc $(BUILD) $(LIBS) $(CFLAGS)

run: $(BUILD) $(DEPS)
	gcc $(BUILD) $(LIBS) $(CFLAGS) -o build/main.out && ./build/main.out

valgrind: $(BUILD) $(DEPS)
	#gcc -g -O0 $(BUILD) $(LIBS) $(CFLAGS) -o build/main.out && /usr/bin/valgrind --leak-check=full --show-leak-kinds=all --suppressions=ref/valgrind.suppression --log-file="log/valgrind-log.txt" ./main.out
	gcc -g -O0 $(BUILD) $(LIBS) $(CFLAGS) -o build/main.out && /usr/bin/valgrind --leak-check=full --show-leak-kinds=all --suppressions=ref/valgrind.suppression ./build/main.out

valgrind-full: $(BUILD) $(DEPS)
	gcc -g -O0 $(BUILD) $(LIBS) $(CFLAGS) -o build/main.out && /usr/bin/valgrind --leak-check=full --show-leak-kinds=all ./build/main.out

dng: main_dng.c dm_dungeon.c dm_dungeon.h dm_algorithm.h dm_algorithm.c
	gcc main_dng.c dm_dungeon.c dm_algorithm.c mt_rand.c -o build/main_dng.out -lm && ./build/main_dng.out
valgrind-dng: main_dng.c dm_algorithm.c dm_algorithm.h
	gcc main_dng.c dm_dungeon.c dm_algorithm.c mt_rand.c -o build/main_dng.out -lm && /usr/bin/valgrind --leak-check=full --show-leak-kinds=all ./build/main_dng.out

astar: main_astar.c dm_algorithm.c dm_algorithm.h
	gcc main_astar.c dm_algorithm.c mt_rand.c -o build/main_astar.out -lm && ./build/main_astar.out
valgrind-astar: main_astar.c dm_algorithm.c dm_algorithm.h
	gcc main_astar.c dm_algorithm.c mt_rand.c -o build/main_astar.out -lm && /usr/bin/valgrind --leak-check=full --show-leak-kinds=all ./build/main_astar.out

cell: main_cellular.c dm_algorithm.c dm_algorithm.h
	gcc main_cellular.c dm_algorithm.c mt_rand.c -o build/main_cellular.out -lm && ./build/main_cellular.out
