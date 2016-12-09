/* Copyright 2003-2012, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: column.h
 *
 * Window column support code.
 */

#ifndef CASHBOOK_COLUMN
#define CASHBOOK_COLUMN


#define COLUMN_WIDTH_DEFAULT 100

#define COLUMN_DRAG_HOTSPOT 40
#define COLUMN_DRAG_MIN 140

/**
 * A column definition block.
 */

struct column_block {
	size_t	columns;		/**< The number of columns defined in the block.							*/

	int	*position;		/**< The positions of the individual columns from the left hand edge of the window, in OS units.	*/
	int	*width;			/**< The widths of the individual columns, in OS units.							*/
};


/**
 * Create a new column definition instance.
 * 
 * \param columns		The number of columns to be defined.
 * \return			Pointer to the instance, or NULL on failure.
 */

struct column_block *column_create_instance(size_t columns);


/**
 * Clone a column definition instance.
 *
 * \param *instance		The instance to be cloned.
 * \return			The new, cloned instance, or NULL on failure.
 */

struct column_block *column_clone_instance(struct column_block *instance);


/**
 * Copy the column settings from one column definition instance to another.
 *
 * \param *from			The instance to copy the settings from.
 * \param *to			The instance to copy the settings to.
 */

void column_copy_instance(struct column_block *from, struct column_block *to);


/**
 * Delete a column instance.
 * 
 * \param *instance		Pointer to the instance to be deleted.
 */

void column_delete_instance(struct column_block *instance);


/**
 * Set a window's column data up, based on the supplied values in a column
 * width configuration string.
 *
 * \param *instance		The columns instance to take the data.
 * \param start			The first column to read in from the string.
 * \param skip			TRUE to ignore missing entries; FALSE to set to default.
 * \param *widths		The width configuration string to process.
 */

void column_init_window(struct column_block *instance, int start, osbool skip, char *widths);


/**
 * Create a column width configuration string from an array of column widths.
 *
 * \param *instance		The column instance to be processed.
 * \param *buffer		A buffer to hold the configuration string.
 * \param length		The size of the buffer.
 */

char *column_write_as_text(struct column_block *instance, char *buffer, size_t length);


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
 * \param *instance		The column instance to be updated.
 */

void update_dragged_columns(char *mapping, char *widths, int group, int new_width, struct column_block *instance);


/**
 * Get the total width of the columns represented by an instance.
 * 
 * \param *instance		The column instance to report on.
 * \return			The total width, or 0 on error.
 */

int column_get_window_width(struct column_block *instance);


/**
 * Given an X position in OS units, locate the column into which it falls.
 *
 * \param *instance		The column instance to test the position against.
 * \param xpos			The X position from the left margin, in OS units.
 * \return			The column into which the location falls.
 */

int column_get_position(struct column_block *instance, int xpos);


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

