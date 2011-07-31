/* CashBook - column.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_COLUMN
#define CASHBOOK_COLUMN


#define COLUMN_WIDTH_DEFAULT 100

#define COLUMN_DRAG_HOTSPOT 40
#define COLUMN_DRAG_MIN 140


/**
 * Set a window's column data up, based on the supplied values.
 *
 * \param width[]		Array to take column width details.
 * \param position[]		Array to take column position details.
 * \param columns		The number of columns to be processed.
 * \param *widths		The width configuration string to process.
 */

void column_init_window(int width[], int position[], int columns, char *widths);


/**
 * Create a column width configuration string from an array of column widths.
 *
 * \param width[]		An array of column widths.
 * \param columns		The number of columns to process.
 * \param *buffer		A buffer to hold the configuration string.
 */

char *column_write_as_text(int width[], int columns, char *buffer);


/**
 * Start a column width drag operation.
 *
 * \param *ptr			The Wimp pointer data starting the drag.
 * \param *data			Client-specific data pointer.
 * \param w			The parent window the dragged toolbar belongs to.
 * \param *mapping		The column group mapping for the window.
 * \param *widths		The minimum column width configuration string.
 * \param *callback		The function to be called at the end of the drag.
 */

void column_start_drag(wimp_pointer *ptr, void *data, wimp_w w, char *mapping, char *widths, void (*callback)(void *, wimp_i, int));


/**
 * Reallocate the new group width across all the columns in the group, updating
 * the column width and position arrays.  Most columns just take their minimum
 * width, while the right-hand column takes up the slack.
 *
 * \param *mapping		The column group mapping for the window.
 * \param *widths		The minimum column width configuration string.
 * \param group			The column group that has been resized.
 * \param new_width		The new width of the dragged group.
 * \param width[]		The column width array to be updated.
 * \param position[]		The column position array to be updated.
 * \param columns		The number of columns to be processed.
 */

void update_dragged_columns(char *mapping, char *widths, int group, int new_width, int width[], int position[], int columns);


/**
 * Return the number of the column group containing the given column.
 *
 * \param *mapping		The column group mapping string.
 * \param column		The column to investigate.
 * \return			The column group number.
 */

int column_get_group(char *mapping, int column);


/**
 * Return the number of the left-hand column in a given group.
 *
 * \param *mapping		The column group mapping string.
 * \param group			The column group.
 * \return			The left-most column in the group.
 */

int column_get_leftmost_in_group(char *mapping, int group);


/**
 * Return the number of the right-hand column in a given group.
 *
 * \param *mapping		The column group mapping string.
 * \param group			The column group.
 * \return			The right-most column in the group.
 */

int column_get_rightmost_in_group(char *mapping, int group);

#endif

