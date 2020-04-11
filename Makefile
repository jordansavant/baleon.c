# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS=-I.
BUILD = main.c gametime.c draw.c
LIBS = -lncurses
DEPS = gametime.h draw.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

build: main.c
	gcc $(BUILD) $(LIBS) $(CFLAGS)

build-run: main.c
	gcc $(BUILD) $(LIBS) $(CFLAGS) -o main.out && ./main.out
