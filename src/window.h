/* CashBook - window.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_WINDOW
#define CASHBOOK_WINDOW

/* ==================================================================================================================
 * Static constants
 */

#define X_WINDOW_PERCENT_LIMIT 98
#define Y_WINDOW_PERCENT_LIMIT 40
#define Y_WINDOW_PERCENT_ORIGIN 75

void set_initial_window_area (wimp_window *window, int width, int height, int x, int y, int yoff);

#endif

