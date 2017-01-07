/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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

#define COLUMN_GUTTER 4


struct column_map {
	wimp_i			field;					/**< The icon relating to the column in the main data table window.					*/
	wimp_i			heading;				/**< The icon relating to the column heading in the table heading pane.					*/
	wimp_i			footer;					/**< The icon relating to the column footer in the table footer pane.					*/
};

/**
 * A column definition block.
 */

struct column_block {
	size_t			columns;				/**< The number of columns defined in the block.							*/
	struct column_map	*map;					/**< The column icon map.										*/

	int			*position;				/**< The positions of the individual columns from the left hand edge of the window, in OS units.	*/
	int			*width;					/**< The widths of the individual columns, in OS units.							*/
	int			*minimum_width;				/**< The minimum widths of individual columns, in OS units.						*/
};


/**
 * Create a new column definition instance.
 * 
 * \param columns		The number of columns to be defined.
 * \param *map			Pointer to the column icon map.
 * \return			Pointer to the instance, or NULL on failure.
 */

struct column_block *column_create_instance(size_t columns, struct column_map *map);


/**
 * Clone a column definition instance.
 *
 * \param *instance		The instance to be cloned.
 * \return			The new, cloned instance, or NULL on failure.
 */

struct column_block *column_clone_instance(struct column_block *instance);


/**
 * Copy the column widths from one column definition instance to another.
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
 * Set, or reset, the minimum column widths for an instance from a
 * configuration string. The string is a comma-separated list of decimal
 * integers giving the widths, in OS units, of each column.
 *
 * \param *instance		Pointer to the instance to be updated.
 * \param *widths		The width configuration string to be used.
 */

void column_set_minimum_widths(struct column_block *instance, char *widths);


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
 * Adjust the positions of the column heading icons in the toolbar window
 * template, according to the current column positions, ready for the
 * window to be created.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *heading		Pointer to the heading window template definition.
 */

void columns_place_heading_icons(struct column_block *instance, wimp_window *definition);


/**
 * Adjust the positions of the column footer icons in the footer window
 * template, according to the current column positions, ready for the
 * window to be created. Vertically, the icons are set to Y1=0 and
 * Y0 to the negative window height.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *heading		Pointer to the heading window template definition.
 */

void columns_place_footer_icons(struct column_block *instance, wimp_window *definition, int height);


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
 * \param *instance		The column instance to be processed.
 * \param *ptr			The Wimp pointer data starting the drag.
 * \param *data			Client-specific data pointer.
 * \param w			The parent window the dragged toolbar belongs to.
 * \param *callback		The function to be called at the end of the drag.
 */

void column_start_drag(struct column_block *instance, wimp_pointer *ptr, void *data, wimp_w w, void (*callback)(void *, wimp_i, int));


/**
 * Reallocate the new group width across all the columns in the group, updating
 * the column width and position arrays.  Most columns just take their minimum
 * width, while the right-hand column takes up the slack.
 *
 * \param *instance		The column instance to be processed.
 * \param header		Handle of the heading window, or NULL.
 * \param footer		Handle of the footer window, or NULL.
 * \param group			The heading icon of the column group that has been resized.
 * \param new_width		The new width of the dragged group.
 */

void columns_update_dragged(struct column_block *instance, wimp_w header, wimp_w footer, wimp_i group, int new_width);


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
 * Return the column group icon handle for the column containing a given
 * field icon.
 * 
 * \param *instance		The column set instance to search.
 * \param field			The field icon handle to look up.
 * \return			The column heading icon handle.
 */

wimp_i column_get_group_icon(struct column_block *instance, wimp_i field);

#endif

