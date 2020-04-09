#include <ncurses.h>

enum GameState
{
	gs_start,
	gs_intro,
	gs_main,
	gs_exit
};

void g_setup() {
	// setup ncurses
	initscr();
}

void g_teardown() {
	// teardown ncurses
	endwin();
}

int main(void) {
	// Game Loop
	// Intro
	// Main Loop
	// Exit
	enum GameState state = gs_start;

	g_setup();

	while (state != gs_exit) {
	}

	g_teardown();

	return 0;
}

