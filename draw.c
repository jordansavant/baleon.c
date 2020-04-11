#include <ncurses.h>
#include <string.h>
#include "draw.h"

// Print text on center of std screen
void center(int row, char *title, int colorpair, int colorpair_reset, bool bold)
{
	center_in_win(stdscr, row, title, colorpair, colorpair_reset, bold);
}
void center_in_win(WINDOW *win, int row, char *title, int colorpair, int colorpair_reset, bool bold)
{
	if (win == NULL)
		win = stdscr;

	if (bold)
		attrset(COLOR_PAIR(colorpair) | A_BOLD);
	else
		attrset(COLOR_PAIR(colorpair));
	int len, indent, y, width;
	getmaxyx(stdscr, y, width);	/* get screen width */
	len = strlen(title);		/* get title's length */
	indent = width - len;		/* subtract it from screen width */
	indent /= 2;			/* divide result into two */
	mvaddstr(row, indent, title);
	attrset(COLOR_PAIR(colorpair_reset));
}

void clear_row(int row)
{
	move(row, 0);
	clrtoeol();
}
