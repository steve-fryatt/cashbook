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

void column_init_window (int width[], int pos[], int columns, char *widths);
char *column_write_as_text (int width[], int columns, char *buffer);

void set_initial_window_area (wimp_window *window, int width, int height, int x, int y, int yoff);

void update_dragged_columns(char *mapping, char *widths, int heading, int width, int col_widths[], int col_pos[], int columns);

/* Column dragging */

void column_start_drag(wimp_pointer *ptr, file_data *file, int data, wimp_w w, char *mapping, char *widths, void (*callback)(file_data *, int, wimp_i, int));
void terminate_column_width_drag (wimp_dragged *drag);

/* Column grouping information. */

int column_get_leftmost_in_group (char *mapping, int heading);
int column_get_rightmost_in_group (char *mapping, int heading);
int column_get_group (char *mapping, int column);

#endif

