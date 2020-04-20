# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS=-I.
BUILD = main.c dm_gametime.c dm_draw.c dm_world.c dm_debug.c dm_algorithm.c mt_rand.c
LIBS = -lncurses -lmenu -lm
DEPS = dm_gametime.h dm_draw.h dm_world.h dm_debug.h dm_algorithm.h mt_rand.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

make: $(BUILD) $(DEPS)
	gcc $(BUILD) $(LIBS) $(CFLAGS)

run: $(BUILD) $(DEPS)
	gcc $(BUILD) $(LIBS) $(CFLAGS) -o main.out && ./main.out

valgrind: $(BUILD) $(DEPS)
	gcc -g -O0 $(BUILD) $(LIBS) $(CFLAGS) -o main.out && /usr/bin/valgrind --leak-check=full --show-leak-kinds=all --suppressions=ref/valgrind.suppression ./main.out

valgrind-full: $(BUILD) $(DEPS)
	gcc -g -O0 $(BUILD) $(LIBS) $(CFLAGS) -o main.out && /usr/bin/valgrind --leak-check=full --show-leak-kinds=all ./main.out
