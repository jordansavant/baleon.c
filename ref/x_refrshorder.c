#include <ncurses.h>

int main(void) {
    initscr();
    keypad(stdscr, true);
	noecho();

	WINDOW *subwin = newwin(1, 20, 1, 1);
	WINDOW *subpad = newpad(10, 10);

	// render to main screen
	move(0, 0);
	addstr("std screen");

	// render to subwindow
	wmove(subwin, 0, 0);
	waddstr(subwin, "sub win");

	// render to pad
	wmove(subpad, 0, 0);
	waddstr(subpad, "sub pad");

	// render to screen
	do {
		refresh();
		wrefresh(subwin);
		prefresh(subpad, 0, 0, 2, 2, 20, 20);
	} while(getch() != 'x');

	delwin(subwin);
	delwin(subpad);
    endwin();

	return 0;
}

