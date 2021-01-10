/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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

#include <stddef.h>
#include "filing.h"
#include "sort.h"

#define COLUMN_SORT_SPRITE_LEN 12

#define COLUMN_WIDTH_DEFAULT 100

#define COLUMN_DRAG_HOTSPOT 40
#define COLUMN_DRAG_MIN 140

#define COLUMN_GUTTER 4


/**
 * A column map entry, detailing the field, header and footer icons associated
 * with the column.
 * 
 * It is assumed that heading icons will be contiguous, so while adjacent fields
 * may share the same heading, a heading icon can not be re-used once another
 * icon has been introduced. Sort orders should go with heading icons, on a
 * one-to-one correlation.
 */

struct column_map {
	wimp_i			field;		/**< The icon relating to the column in the main data table window.	*/
	wimp_i			heading;	/**< The icon relating to the column heading in the table heading pane.	*/
	wimp_i			footer;		/**< The icon relating to the column footer in the table footer pane.	*/
	enum sort_type		sort;		/**< The sort order relating to the column.				*/
};


/**
 * An extra icon entry, detailing the icon handle and the columns to which it
 * is associated.
 */

struct column_extra {
	wimp_i			icon;		/**< The additional icon in the main data table window.			*/
	int			left;		/**< The column from which the icon spans on the left.			*/
	int			right;		/**< The column to which the icon spans on the right.			*/
};


/**
 * A column definition block.
 */

struct column_block;


/**
 * Create a new column definition instance.
 *
 * \param columns		The number of columns to be defined.
 * \param *map			Pointer to the column icon map.
 * \param *extra		Pointer to the extra icon list, or NULL for none.
 * \param sort_indicator	The icon used to display the sort order in the column headings.
 * \return			Pointer to the instance, or NULL on failure.
 */

struct column_block *column_create_instance(size_t columns, struct column_map *map, struct column_extra *extra, wimp_i sort_indicator);


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
 * Set an icon definitions X0 and X1 coordinates to suit a column position.
 *
 * \param *instance		The instance to set the icon from.
 * \param field			The icon belonging to the column.
 * \param *icon			Pointer to the icon definition.
 */

void column_place_icon_horizontally(struct column_block *instance, wimp_i field, wimp_icon_create *icon);


/**
 * Horizontally position the table icons in a window defintion, so that
 * they are ready to be used in a redraw operation. If a buffer is supplied,
 * the icons' indirected data is set up to point to it.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *definition		Pointer to the table window template definition.
 * \param *buffer		Pointer to a buffer to use for indirection, or NULL for none.
 * \param length		The length of the indirection buffer.
 */

void columns_place_table_icons_horizontally(struct column_block *instance, wimp_window *definition, char *buffer, size_t length);


/**
 * Vertically position the table icons in a window defintion, so that
 * they are ready to be used in a redraw operation.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *definition		Pointer to the table window template definition.
 * \param ymin			The minimum Y0 window coordinate for the table row.
 * \param ymax			The maximum Y1 window coordinate for the table row.
 */

void columns_place_table_icons_vertically(struct column_block *instance, wimp_window *definition, int ymin, int ymax);


/**
 * Plot all of the table icons in a window definition as empty fields, via
 * the window template iocn plotting mechanism. This can be used to plot a
 * blank line in a window. It is assumed that the window template has been
 * set up.
 *
 * \param *instance		The columns instance to be plotted.
 */

void columns_plot_empty_table_icons(struct column_block *instance);


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
 * Export the column heading names to a delimited file.
 *
 * \param *instance		The column instance to be processed.
 * \param window		The handle of the window holding the heading icons.
 * \param *out			The handle of the file to export to.
 * \param format		The format of the file to export.
 * \param *buffer		Pointer to a buffer to use to build the data.
 * \param length		The length of the supplied buffer.
 */

void columns_export_heading_names(struct column_block *instance, wimp_w window, FILE *out, enum filing_delimit_type format,
		char *buffer, size_t length);


/**
 * Send the column heading names to a stringbuild line. Note that this function
 * expects a stringbuild instance to be set up and ready to use.
 *
 * \param *instance		The column instance to be processed.
 * \param window		The handle of the window holding the heading icons.
 */

void columns_print_heading_names(struct column_block *instance, wimp_w window);


/**
 * Return details of the field or heading icons associated with a column
 * instance.
 *
 * \param *instance		The column instance to query.
 * \param icons[]		Pointer to an array to hold the column icon handles.
 * \param length		The size of the supplied array.
 * \param headings		TRUE to return heading icons; FALSE to return field icons.
 * \return			TRUE if successful; otherwise false.
 */

osbool column_get_icons(struct column_block *instance, wimp_i icons[], size_t length, osbool headings);


/**
 * Create a column width configuration string from an array of column widths.
 *
 * \param *instance		The column instance to be processed.
 * \param *buffer		A buffer to hold the configuration string.
 * \param length		The size of the buffer.
 */

char *column_write_as_text(struct column_block *instance, char *buffer, size_t length);


/**
 * Test an icon from the column headings window to see if it is a draggable
 * column heading.
 *
 * \param *instance		The column instance to be tested.
 * \param icon			The icon to be tested.
 * \return			TRUE if the icon is a draggable column; FALSE if not.
 */

osbool column_is_heading_draggable(struct column_block *instance, wimp_i icon);


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
 * Position the column sort indicator icon in a table header pane.
 *
 * \param *instance		The column instance to use for the positioning.
 * \param *indicator		Pointer to the icon definiton data for the sort indicator.
 * \param *window		Pointer to the header pane window definition template.
 * \param sort_order		The sort details for the table.
 */

void column_update_sort_indicator(struct column_block *instance, wimp_icon *indicator, wimp_window *window,enum sort_type sort_order);


/**
 * Process clicks on the window containing the column headings, so that
 * if the icon under the pointer is the sort indicator, it reflects the
 * icon beneath this.
 *
 * \param *instance		The column instance to use.
 * \param *pointer		Pointer to the pointer data returned by the Wimp.
 */

void column_update_heading_icon_click(struct column_block *instance, wimp_pointer *pointer);

/**
 * Find the minimum and maximum horizontal positions of a field's icon, reporting
 * back in OS units relative to the parent window origin.
 *
 * \param *instance		The column instance to report on.
 * \param field			The icon used by the field.
 * \param *xmin			Pointer to variable to take the minimum X coordinate.
 * \param *xmax			Pointer to variable to take the maximum X coordinate.
 */

void column_get_xpos(struct column_block *instance, wimp_i field, int *xmin, int *xmax);


/**
 * Find the minimum and maximum horizontal positions of a heading's icon
 * group, reporting back in OS units relative to the parent window origin.
 *
 * \param *instance		The column instance to report on.
 * \param heading		The icon used by the heading.
 * \param *xmin			Pointer to variable to take the minimum X coordinate.
 * \param *xmax			Pointer to variable to take the maximum X coordinate.
 */

void column_get_heading_xpos(struct column_block *instance, wimp_i heading, int *xmin, int *xmax);


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
 * \return			The icon of the column into which the location falls.
 */

wimp_i column_find_icon_from_xpos(struct column_block *instance, int xpos);


/**
 * Given a heading icon, return the sort type associated with it.
 *
 * \param *instance		The column set instance to search.
 * \param heading		The column heading icon to look up.
 * \return			The associated sort order, or SORT_NONE.
 */

enum sort_type column_get_sort_type_from_heading(struct column_block *instance, wimp_i heading);


/**
 * Given a sort order, return the heading icon associated with it.
 *
 * \param *instance		The column set instance to search.
 * \param sort			The sort type to look up.
 * \return			The associated column heading icon, or wimp_ICON_WINDOW.
 */

wimp_i column_get_heading_from_sort_type(struct column_block *instance, enum sort_type sort);


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

