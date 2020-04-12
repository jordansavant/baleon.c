#include <string.h>
#include "dm_draw.h"

// Print text on center of std screen
void dm_center(int row, char *title, int colorpair, int colorpair_reset, bool bold)
{
	dm_center_in_win(stdscr, row, title, colorpair, colorpair_reset, bold);
}
void dm_center_in_win(WINDOW *win, int row, char *title, int colorpair, int colorpair_reset, bool bold)
{
	if (win == NULL)
		win = stdscr;

	if (bold)
		attrset(COLOR_PAIR(colorpair) | A_BOLD);
	else
		attrset(COLOR_PAIR(colorpair));
	int len, indent, y, width;
	getmaxyx(win, y, width);	/* get screen width */
	len = strlen(title);		/* get title's length */
	indent = width - len;		/* subtract it from screen width */
	indent /= 2;			/* divide result into two */
	mvaddstr(row, indent, title);
	attrset(COLOR_PAIR(colorpair_reset));
}

void dm_clear_row(int row)
{
	move(row, 0);
	clrtoeol();
}

// get a center a window in std
WINDOW* dm_new_center_win(int row, int width, int height, int offset_x)
{
	int y, x, indent;
	getmaxyx(stdscr, y, x);
	indent = (x - width) / 2 + offset_x;
	return newwin(height, width, row, indent);
}

int dm_calc_center_top(WINDOW *win, int rows)
{
	int y, x;
	getmaxyx(win, y, x);
	return y/2 - rows/2;
}
int dm_calc_center_left(WINDOW *win, int cols)
{
	int y, x;
	getmaxyx(win, y, x);
	return x/2 - cols/2;
}
