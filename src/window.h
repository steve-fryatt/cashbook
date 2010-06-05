/* CashBook - window.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_WINDOW
#define _ACCOUNTS_WINDOW

/* ==================================================================================================================
 * Static constants
 */

#define X_WINDOW_PERCENT_LIMIT 98
#define Y_WINDOW_PERCENT_LIMIT 40
#define Y_WINDOW_PERCENT_ORIGIN 75

#define COLUMN_WIDTH_DEFAULT 100

#define COLUMN_DRAG_HOTSPOT 40
#define COLUMN_DRAG_MIN 140

#define COLUMN_DRAG_TRANSACT 1
#define COLUMN_DRAG_ACCOUNT 2
#define COLUMN_DRAG_SORDER 3
#define COLUMN_DRAG_ACCVIEW 4
#define COLUMN_DRAG_PRESET 5

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* Setting up columned windows. */

void init_window_columns (int width[], int pos[], int columns, char *widths);
char *write_window_columns_text (int width[], int columns, char *buffer);

void set_initial_window_area (wimp_window *window, int width, int height, int x, int y, int yoff);

/* Column dragging */

void start_column_width_drag (wimp_pointer *ptr);
void terminate_column_width_drag (wimp_dragged *drag);
void update_dragged_columns (char *mapping, char *widths, int heading, int width,
                             int col_widths[], int col_pos[], int columns);

/* Column grouping information. */

int leftmost_group_column (char *mapping, int heading);
int rightmost_group_column (char *mapping, int heading);
int column_group (char *mapping, int column);
int minimum_group_width (char *mapping, char *widths, int heading);

int minimum_column_width (char *widths, int column);

#endif
