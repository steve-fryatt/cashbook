/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: column.c
 *
 * Window column support code.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osbyte.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/string.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "column.h"

#include "filing.h"
#include "report.h"
#include "stringbuild.h"
#include "window.h"

/**
 * The horizontal space between columns in OS units.
 */

#define COLUMN_HEADING_MARGIN 2

/**
 * Placement offset for the sort column icon, in OS units.
 */

#define COLUMN_SORT_OFFSET 8

/**
 * A column definition block.
 */

struct column_block {
	size_t			columns;				/**< The number of columns defined in the block.							*/
	struct column_map	*map;					/**< The column icon map.										*/
	struct column_extra	*extra;					/**< The additional column list.									*/

	int			*position;				/**< The positions of the individual columns from the left hand edge of the window, in OS units.	*/
	int			*width;					/**< The widths of the individual columns, in OS units.							*/
	int			*minimum_width;				/**< The minimum widths of individual columns, in OS units.						*/

	wimp_i			sort_heading;				/**< The column heading icon which is currently obscured by the sort indicator.				*/
	wimp_i			sort_indicator;				/**< The icon which is used to display the sort indicator.						*/
};

static void		*column_drag_data;					/**< Client-specific data for the drag.				*/
static wimp_i		column_drag_icon;					/**< The handle of the icon being dragged.			*/
static void		(*column_drag_callback)(void *, wimp_i, int);		/**< The callback handler for the drag.				*/


static void		column_terminate_drag(wimp_dragged *drag, void *data);
static int		column_get_minimum_group_width(struct column_block *instance, int group);
static int		column_get_from_field(struct column_block *instance, wimp_i field);
static int		column_get_leftmost_from_sort_type(struct column_block *instance, enum sort_type sort);
static int		column_get_leftmost_in_heading_group(struct column_block *instance, wimp_i heading);
static int		column_get_rightmost_in_heading_group(struct column_block *instance, wimp_i heading);
static int		column_get_rightmost_in_footer_group(struct column_block *instance, wimp_i footer);

/**
 * Return TRUE if the given column is the right-most in the given instance.
 */

#define column_is_rightmost(instance, column) ((((column) + 1) >= (instance)->columns) ? TRUE : FALSE)

/**
 * Create a new column definition instance.
 *
 * \param columns		The number of columns to be defined.
 * \param *map			Pointer to the column icon map.
 * \param *extra		Pointer to the extra icon list, or NULL for none.
 * \param sort_indicator	The icon used to display the sort order in the column headings.
 * \return			Pointer to the instance, or NULL on failure.
 */

struct column_block *column_create_instance(size_t columns, struct column_map *map, struct column_extra *extra, wimp_i sort_indicator)
{
	void			*mem;
	struct column_block	*new;

	if (map == NULL)
		return NULL;

	mem = heap_alloc(sizeof(struct column_block) + (3 * columns * sizeof(int)));
	if (mem == NULL)
		return NULL;

	new = mem;

	new->columns = columns;
	new->map = map;
	new->extra = extra;

	mem += sizeof(struct column_block);

	new->position = mem;
	new->width = mem + (columns * sizeof(int));
	new->minimum_width = mem + (2 * columns * sizeof(int));

	new->sort_indicator = sort_indicator;
	new->sort_heading = wimp_ICON_WINDOW;

	return new;
}


/**
 * Clone a column definition instance.
 *
 * \param *instance		The instance to be cloned.
 * \return			The new, cloned instance, or NULL on failure.
 */

struct column_block *column_clone_instance(struct column_block *instance)
{
	struct column_block	*new;

	if (instance == NULL)
		return NULL;

	new = column_create_instance(instance->columns, instance->map, instance->extra, instance->sort_indicator);
	if (new == NULL)
		return NULL;

	new->columns = instance->columns;
	new->map = instance->map;

	column_copy_instance(instance, new);

	return new;
}


/**
 * Copy the column widths from one column definition instance to another.
 *
 * \param *from			The instance to copy the settings from.
 * \param *to			The instance to copy the settings to.
 */

void column_copy_instance(struct column_block *from, struct column_block *to)
{
	int	i;

	if (from == NULL || to == NULL || from->columns != to->columns)
		return;

	for (i = 0; i < from->columns; i++) {
		to->position[i] = from->position[i];
		to->width[i] = from->width[i];
		to->minimum_width[i] = from->minimum_width[i];
	}

}


/**
 * Delete a column instance.
 *
 * \param *instance		Pointer to the instance to be deleted.
 */

void column_delete_instance(struct column_block *instance)
{
	if (instance == NULL)
		return;

	heap_free(instance);
}


/**
 * Set, or reset, the minimum column widths for an instance from a
 * configuration string. The string is a comma-separated list of decimal
 * integers giving the widths, in OS units, of each column.
 *
 * \param *instance		Pointer to the instance to be updated.
 * \param *widths		The width configuration string to be used.
 */

void column_set_minimum_widths(struct column_block *instance, char *widths)
{
	int	column;
	char	*token, *copy;


	if (instance == NULL || widths == NULL)
		return;

	/* Take a copy of the configuration string. */

	copy = strdup(widths);

	if (copy == NULL)
		return;

	/* Extract the minimum widths from the string. */

	token = strtok(copy, ",");

	for (column = 0; column < instance->columns; column++) {
		instance->minimum_width[column] = (token != NULL) ? atoi(token) : COLUMN_DRAG_MIN;

		token = strtok(NULL, ",");
	}

	/* Free the copied string. */

	free(copy);
}


/**
 * Set a window's column data up, based on the supplied values in a column
 * width configuration string.
 *
 * \param *instance		The columns instance to take the data.
 * \param start			The first column to read in from the string.
 * \param skip			TRUE to ignore missing entries; FALSE to set to default.
 * \param *widths		The width configuration string to process.
 */

void column_init_window(struct column_block *instance, int start, osbool skip, char *widths)
{
	int	i;
	char	*copy, *str;

	if (instance == NULL)
		return;

	copy = strdup(widths);

	/* Read the column widths and set up an array. */

	str = strtok(copy, ",");

	for (i = start; i < instance->columns; i++) {
		if (str != NULL)
			instance->width[i] = atoi(str);
		else if (skip == FALSE)
			instance->width[i] = COLUMN_WIDTH_DEFAULT; /* Stick a default value in if the config data is missing. */

		str = strtok(NULL, ",");
	}

	free(copy);

	/* Now set the positions, based on the widths that were read in. */

	instance->position[0] = 0;

	for (i = 1; i < instance->columns; i++)
		instance->position[i] = instance->position[i - 1] + instance->width[i - 1] + COLUMN_GUTTER;
}


/**
 * Set an icon definitions X0 and X1 coordinates to suit a column position.
 *
 * \param *instance		The instance to set the icon from.
 * \param field			The icon belonging to the column.
 * \param *icon			Pointer to the icon definition.
 */

void column_place_icon_horizontally(struct column_block *instance, wimp_i field, wimp_icon_create *icon)
{
	int	column;

	if (instance == NULL || icon == NULL)
		return;

	column = column_get_from_field(instance, field);
	if (column == -1)
		return;

	icon->icon.extent.x0 = instance->position[column];
	icon->icon.extent.x1 = instance->position[column] + instance->width[column];
}


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

void columns_place_table_icons_horizontally(struct column_block *instance, wimp_window *definition, char *buffer, size_t length)
{
	int	column, extra, left, right;
	wimp_i	icon;

	if (instance == NULL || definition == NULL)
		return;

	/* Position the main column icons. */

	for (column = 0; column < instance->columns; column++) {
		icon = instance->map[column].field;
		if (icon == wimp_ICON_WINDOW)
			continue;

		definition->icons[icon].extent.x0 = instance->position[column];
		definition->icons[icon].extent.x1 = instance->position[column] + instance->width[column];
		if (buffer != NULL) {
			definition->icons[icon].data.indirected_text.text = buffer;
			definition->icons[icon].data.indirected_text.size = length;
		}
	}

	/* If there are no extra icons to position, there's nothing more to do. */

	if (instance->extra == NULL)
		return;

	/* Position the extra icons in the list. */

	for (extra = 0; instance->extra[extra].icon != wimp_ICON_WINDOW; extra++) {
		icon = instance->extra[extra].icon;
		left = instance->extra[extra].left;
		right = instance->extra[extra].right;

		if (left < 0 || right < left || right >= instance->columns)
			continue;

		definition->icons[icon].extent.x0 = instance->position[left];
		definition->icons[icon].extent.x1 = instance->position[right] + instance->width[right];
		if (buffer != NULL) {
			definition->icons[icon].data.indirected_text.text = buffer;
			definition->icons[icon].data.indirected_text.size = length;
		}
	}
}


/**
 * Vertically position the table icons in a window defintion, so that
 * they are ready to be used in a redraw operation.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *definition		Pointer to the table window template definition.
 * \param ymin			The minimum Y0 window coordinate for the table row.
 * \param ymax			The maximum Y1 window coordinate for the table row.
 */

void columns_place_table_icons_vertically(struct column_block *instance, wimp_window *definition, int ymin, int ymax)
{
	int	column, extra;
	wimp_i	icon;

	if (instance == NULL || definition == NULL)
		return;

	/* Position the main column icons. */

	for (column = 0; column < instance->columns; column++) {
		icon = instance->map[column].field;
		if (icon == wimp_ICON_WINDOW)
			continue;

		definition->icons[icon].extent.y0 = ymin;
		definition->icons[icon].extent.y1 = ymax;
	}

	/* If there are no extra icons to position, there's nothing more to do. */

	if (instance->extra == NULL)
		return;

	/* Position the extra icons in the list. */

	for (extra = 0; instance->extra[extra].icon != wimp_ICON_WINDOW; extra++) {
		icon = instance->extra[extra].icon;

		definition->icons[icon].extent.y0 = ymin;
		definition->icons[icon].extent.y1 = ymax;
	}
}


/**
 * Plot all of the table icons in a window definition as empty fields, via
 * the window template iocn plotting mechanism. This can be used to plot a
 * blank line in a window. It is assumed that the window template has been
 * set up.
 *
 * \param *instance		The columns instance to be plotted.
 */

void columns_plot_empty_table_icons(struct column_block *instance)
{
	int	column;
	wimp_i	icon;

	if (instance == NULL)
		return;

	/* Position the main column icons. */

	for (column = 0; column < instance->columns; column++) {
		icon = instance->map[column].field;
		if (icon == wimp_ICON_WINDOW)
			continue;

		window_plot_empty_field(icon);
	}
}


/**
 * Adjust the positions of the column heading icons in the toolbar window
 * template, according to the current column positions, ready for the
 * window to be created.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *heading		Pointer to the heading window template definition.
 */

void columns_place_heading_icons(struct column_block *instance, wimp_window *definition)
{
	int	column;
	wimp_i	icon;

	if (instance == NULL || definition == NULL)
		return;

	/* Position the heading icons. */

	for (column = 0; column < instance->columns; column++) {
		icon = instance->map[column].heading;
		if (icon == wimp_ICON_WINDOW)
			continue;

		definition->icons[icon].extent.x0 = instance->position[column];
		column = column_get_rightmost_in_heading_group(instance, icon);
		if (column == -1)
			break;

		definition->icons[icon].extent.x1 = instance->position[column] + instance->width[column] + COLUMN_HEADING_MARGIN;
	}
}


/**
 * Adjust the positions of the column footer icons in the footer window
 * template, according to the current column positions, ready for the
 * window to be created. Vertically, the icons are set to Y1=0 and
 * Y0 to the negative window height.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *heading		Pointer to the heading window template definition.
 */

void columns_place_footer_icons(struct column_block *instance, wimp_window *definition, int height)
{
	int	column;
	wimp_i	icon;

	if (instance == NULL)
		return;

	/* Position the footer icons. */


	for (column = 0; column < instance->columns; column++) {
		icon = instance->map[column].footer;
		if (icon == wimp_ICON_WINDOW)
			continue;

		definition->icons[icon].extent.y0 = -height;
		definition->icons[icon].extent.y1 = 0;

		definition->icons[icon].extent.x0 = instance->position[column];
		column = column_get_rightmost_in_footer_group(instance, icon);
		if (column == -1)
			break;

		definition->icons[icon].extent.x1 = instance->position[column] + instance->width[column] + COLUMN_HEADING_MARGIN;
	}
}


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
		char *buffer, size_t length)
{
	int	column;
	wimp_i	icon;

	if (instance == NULL || window == NULL)
		return;

	/* Write out the column heading names, using the icon text. */

	for (column = 0; column < instance->columns; column++) {
		icon = instance->map[column].heading;
		if (icon == wimp_ICON_WINDOW)
			continue;

		column = column_get_rightmost_in_heading_group(instance, icon);
		if (column == -1)
			break;

		icons_copy_text(window, icon, buffer, length);
		filing_output_delimited_field(out, buffer, format, column_is_rightmost(instance, column) ? DELIMIT_LAST : DELIMIT_NONE);
	}
}


/**
 * Send the column heading names to a stringbuild line. Note that this function
 * expects a stringbuild instance to be set up and ready to use.
 *
 * \param *instance		The column instance to be processed.
 * \param window		The handle of the window holding the heading icons.
 */

void columns_print_heading_names(struct column_block *instance, wimp_w window)
{
	wimp_icon_state		state;
	int			column;
	wimp_i			icon = wimp_ICON_WINDOW;
	osbool			first = TRUE;

	if (instance == NULL || window == NULL)
		return;

	/* Write out the column heading names, using the icon text. */

	for (column = 0; column < instance->columns; column++) {
		/* If the column has the same heading as the previous
		 * one, just output an overflow field.
		 */

		if (instance->map[column].heading == icon && icon != wimp_ICON_WINDOW) {
			stringbuild_add_string("\\t\\s");
			continue;
		}

		/* Find the next heading icon. */

		icon = instance->map[column].heading;
		if (icon == wimp_ICON_WINDOW)
			continue;

		/* The first field starts with a "keep together" flag, all
		 * the rest start with a tab.
		 */

		if (first == TRUE)
			stringbuild_add_string("\\k");
		else
			stringbuild_add_string("\\v\\t");

		first = FALSE;

		/* Headings are Bold and Underlined. */

		stringbuild_add_string("\\b\\o");

		/* If the icon is right-aligned, so is the heading. */

		state.w = window;
		state.i = icon;
		wimp_get_icon_state(&state);

		if (state.icon.flags & wimp_ICON_RJUSTIFIED)
			stringbuild_add_string("\\r");
		else if (state.icon.flags & wimp_ICON_HCENTRED)
			stringbuild_add_string("\\c");

		/* Copy the icon text for the heading. */

		stringbuild_add_icon(window, icon);
	}

	stringbuild_add_string("\\v");
}


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

osbool column_get_icons(struct column_block *instance, wimp_i icons[], size_t length, osbool headings)
{
	int	column, i;
	wimp_i	icon;

	/* If no buffer is supplied, there's nothing to do. */

	if (icons == NULL || length == 0)
		return FALSE;

	/* If no instance is supplied, blank the buffer and return. */

	if (instance == NULL) {
		for (i = 0; i < length; i++)
			icons[i] = wimp_ICON_WINDOW;
		return FALSE;
	}

	/* Fill the buffer with details of the column icons. */

	i = 0;

	for (column = 0; (column < instance->columns) && (i < length); column++) {
		if (headings == FALSE) {
			icons[i++] = instance->map[column].field;
		} else {
			icon = instance->map[column].heading;
			icons[i++] = icon;

			if (icon == wimp_ICON_WINDOW)
				continue;

			column = column_get_rightmost_in_heading_group(instance, icon);
			if (column == -1)
				break;
		}
	}

	/* Pad the rest of the buffer with blank fields. */

	for (; i < length; i++)
		icons[i] = wimp_ICON_WINDOW;

	return TRUE;
}


/**
 * Return details of the field icons associated with a heading icon in
 * a column instance.
 *
 * \param *instance		The column instance to query.
 * \param heading		The heading to query.
 * \param icons[]		Pointer to an array to hold the column icon handles.
 * \param length		The size of the supplied array.
 * \int				The number of columns found; otherwise zero.
 */

osbool column_get_heading_icons(struct column_block *instance, wimp_i heading, wimp_i icons[], size_t length)
{
	int	column = -1, i, found = 0;

	/* If no buffer is supplied, there's nothing to do. */

	if (icons == NULL || length == 0)
		return found;

	/* Locate the first column in the heading group. */

	column = column_get_leftmost_in_heading_group(instance, heading);

	/* If no instance is supplied, or the located values don't make
	 * sense, blank the buffer and return.
	 */

	if (instance == NULL) {
		for (i = 0; i < length; i++)
			icons[i] = wimp_ICON_WINDOW;
		return found;
	}

	/* Fill the buffer with details of the column icons. */

	i = 0;

	for (; (column < instance->columns) && (i < length) && (instance->map[column].heading == heading); column++)
		icons[i++] = instance->map[column].field;

	found = i;

	/* Pad the rest of the buffer with blank fields. */

	for (; i < length; i++)
		icons[i] = wimp_ICON_WINDOW;

	return found;
}


/**
 * Create a column width configuration string from an array of column widths.
 *
 * \param *instance		The column instance to be processed.
 * \param *buffer		A buffer to hold the configuration string.
 * \param length		The size of the buffer.
 */

char *column_write_as_text(struct column_block *instance, char *buffer, size_t length)
{
	int	i, written;
	char	*end;
	size_t	remaining;

	if (buffer == NULL || length == 0)
		return buffer;

	/* Start the buffer off as a NULL string that will be appended to. */

	*buffer = '\0';
	end = buffer;

	if (instance == NULL)
		return buffer;

	/* Write the column widths to the buffer. */


	for (i = 0; i < instance->columns; i++) {
		remaining = (buffer + length) - end;

		written = snprintf(end, remaining, "%d,", instance->width[i]);
		if (written > remaining)
			written = remaining;

		end += written;
	}

	/* Remove the terminating ','. */

	*(end - 1) = '\0';

	return buffer;
}


/**
 * Test an icon from the column headings window to see if it is a draggable
 * column heading.
 *
 * \param *instance		The column instance to be tested.
 * \param icon			The icon to be tested.
 * \return			TRUE if the icon is a draggable column; FALSE if not.
 */

osbool column_is_heading_draggable(struct column_block *instance, wimp_i icon)
{
	int	column;

	if (instance == NULL)
		return FALSE;

	for (column = 0; column < instance->columns; column++) {
		if (icon == instance->map[column].heading)
			return TRUE;
	}

	return FALSE;
}


/**
 * Start a column width drag operation.
 *
 * \param *instance		The column instance to be processed.
 * \param *ptr			The Wimp pointer data starting the drag.
 * \param *data			Client-specific data pointer.
 * \param w			The parent window the dragged toolbar belongs to.
 * \param *callback		The function to be called at the end of the drag.
 */

void column_start_drag(struct column_block *instance, wimp_pointer *ptr, void *data, wimp_w w, void (*callback)(void *, wimp_i, int))
{
	wimp_window_state	window;
	wimp_window_info	parent;
	wimp_icon_state		icon;
	wimp_drag		drag;
	int			ox, oy;

	window.w = ptr->w;
	wimp_get_window_state(&window);

	ox = window.visible.x0 - window.xscroll;
	oy = window.visible.y1 - window.yscroll;

	icon.w = ptr->w;
	icon.i = ptr->i;
	wimp_get_icon_state(&icon);

	parent.w = w;
	wimp_get_window_info_header_only(&parent);

	column_drag_icon = ptr->i;
	column_drag_data = data;
	column_drag_callback = callback;

	/* If the window exists and the hot-spot was hit, set up the drag parameters and start the drag. */

	if (column_drag_data != NULL && ptr->pos.x >= (ox + icon.icon.extent.x1 - COLUMN_DRAG_HOTSPOT)) {
		drag.w = ptr->w;
		drag.type = wimp_DRAG_USER_RUBBER;

		drag.initial.x0 = ox + icon.icon.extent.x0;
		drag.initial.y0 = parent.visible.y0;
		drag.initial.x1 = ox + icon.icon.extent.x1;
		drag.initial.y1 = oy + icon.icon.extent.y1;

		drag.bbox.x0 = ox + icon.icon.extent.x0 -
				(icon.icon.extent.x1 - icon.icon.extent.x0 - column_get_minimum_group_width(instance, ptr->i));
		drag.bbox.y0 = parent.visible.y0;
		drag.bbox.x1 = 0x7fffffff;
		drag.bbox.y1 = oy + icon.icon.extent.y1;

		wimp_drag_box(&drag);

		event_set_drag_handler(column_terminate_drag, NULL, NULL);
	}
}


/**
 * Handle drag-end events relating to column dragging.
 *
 * \param *drag			The Wimp drag end data.
 * \param *data			Unused client data sent via Event Lib.
 */

static void column_terminate_drag(wimp_dragged *drag, void *data)
{
	int		width;

	width = drag->final.x1 - drag->final.x0;

	if (column_drag_callback != NULL)
		column_drag_callback(column_drag_data, column_drag_icon, width);
}


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

void columns_update_dragged(struct column_block *instance, wimp_w header, wimp_w footer, wimp_i group, int new_width)
{
	int			sum = 0;
	int			column, left, right;
	wimp_icon_state		icon;

	if (instance == NULL)
		return;

	left = column_get_leftmost_in_heading_group(instance, group);
	right = column_get_rightmost_in_heading_group(instance, group);

	if (left == -1 || right == -1 || left < 0 || right >= instance->columns || left > right)
		return;

	for (column = left; column <= right; column++) {
		if (column == right) {
			instance->width[column] = new_width - (sum + COLUMN_HEADING_MARGIN);
		} else {
			instance->width[column] = instance->minimum_width[column];
			sum += (instance->minimum_width[column] + COLUMN_GUTTER);
		}
	}

	for (column = left + 1; column < instance->columns; column++)
		instance->position[column] = instance->position[column - 1] + instance->width[column - 1] + COLUMN_GUTTER;

	/* Adjust the icons in the header pane. */

	if (header != NULL) {
		icon.w = header;

		for (column = 0; column < instance->columns; column++) {
			icon.i = instance->map[column].heading;
			if (icon.i == wimp_ICON_WINDOW)
				continue;

			wimp_get_icon_state(&icon);

			icon.icon.extent.x0 = instance->position[column];

			column = column_get_rightmost_in_heading_group(instance, icon.i);
			if (column == -1)
				break;

			icon.icon.extent.x1 = instance->position[column] + instance->width[column] + COLUMN_HEADING_MARGIN;

			wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
					icon.icon.extent.x1, icon.icon.extent.y1);
		}
	}

	/* Adjust the icons in the footer pane. */

	if (footer != NULL) {
		icon.w = footer;

		for (column = 0; column < instance->columns; column++) {
			icon.i = instance->map[column].footer;
			if (icon.i == wimp_ICON_WINDOW)
				continue;

			wimp_get_icon_state(&icon);

			icon.icon.extent.x0 = instance->position[column];

			column = column_get_rightmost_in_footer_group(instance, icon.i);
			if (column == -1)
				break;

			icon.icon.extent.x1 = instance->position[column] + instance->width[column] + COLUMN_HEADING_MARGIN;

			wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
					icon.icon.extent.x1, icon.icon.extent.y1);
		}
	}
}


/**
 * Position the column sort indicator icon in a table header pane.
 *
 * \param *instance		The column instance to use for the positioning.
 * \param *indicator		Pointer to the icon definiton data for the sort indicator.
 * \param *window		Pointer to the header pane window definition template.
 * \param sort_order		The sort details for the table.
 */

void column_update_sort_indicator(struct column_block *instance, wimp_icon *indicator, wimp_window *window, enum sort_type sort_order)
{
	int		column, anchor, width;
	char		*sprite;
	size_t		length;
	wimp_i		heading;

	if (instance == NULL || indicator == NULL || window == NULL || sort_order == SORT_NONE)
		return;

	heading = column_get_heading_from_sort_type(instance, sort_order);
	if (heading == wimp_ICON_WINDOW)
		return;

	instance->sort_heading = heading;

	/* Update the sort icon sprite name. As this is a sprite name, there's
	 * no need to forcibly terminate the string on a buffer overrun, as the
	 * system stops at 12 characters anyway.
	 */

	sprite = (char *) indicator->data.indirected_sprite.id;
	length = indicator->data.indirected_sprite.size;

	if (sort_order & SORT_ASCENDING)
		string_copy(sprite, "sortarrd", length);
	else if (sort_order & SORT_DESCENDING)
		string_copy(sprite, "sortarru", length);

	/* Place the icon in the correct location. */

	width = indicator->extent.x1 - indicator->extent.x0;

	if ((window->icons[heading].flags & wimp_ICON_HCENTRED) || (window->icons[heading].flags & wimp_ICON_RJUSTIFIED)) {
		column = column_get_leftmost_in_heading_group(instance, heading);
		if (column == -1)
			return;

		anchor = instance->position[column] + COLUMN_HEADING_MARGIN;
		indicator->extent.x0 = anchor + COLUMN_SORT_OFFSET;
		indicator->extent.x1 = indicator->extent.x0 + width;
	} else {
		column = column_get_rightmost_in_heading_group(instance, heading);
		if (column == -1)
			return;

		anchor = instance->position[column] + instance->width[column] + COLUMN_HEADING_MARGIN;
		indicator->extent.x1 = anchor - COLUMN_SORT_OFFSET;
		indicator->extent.x0 = indicator->extent.x1 - width;
	}
}


/**
 * Process clicks on the window containing the column headings, so that
 * if the icon under the pointer is the sort indicator, it reflects the
 * icon beneath this.
 *
 * \param *instance		The column instance to use.
 * \param *pointer		Pointer to the pointer data returned by the Wimp.
 */

void column_update_heading_icon_click(struct column_block *instance, wimp_pointer *pointer)
{
	if (instance == NULL || pointer == NULL)
		return;

	if (pointer->i == instance->sort_indicator)
		pointer->i = instance->sort_heading;
}


/**
 * Find the minimum and maximum horizontal positions of a field's icon, reporting
 * back in OS units relative to the parent window origin.
 *
 * \param *instance		The column instance to report on.
 * \param field			The icon used by the field.
 * \param *xmin			Pointer to variable to take the minimum X coordinate.
 * \param *xmax			Pointer to variable to take the maximum X coordinate.
 */

void column_get_xpos(struct column_block *instance, wimp_i field, int *xmin, int *xmax)
{
	int	column;

	if (instance == NULL || (xmin == NULL && xmax == NULL))
		return;

	column = column_get_from_field(instance, field);
	if (column == -1)
		return;

	if (xmin != NULL && instance->position[column] < *xmin)
		*xmin = instance->position[column];

	if (xmax != NULL && instance->position[column] + instance->width[column] > *xmax)
		*xmax = instance->position[column] + instance->width[column];
}


/**
 * Find the minimum and maximum horizontal positions of a heading's icon
 * group, reporting back in OS units relative to the parent window origin.
 *
 * \param *instance		The column instance to report on.
 * \param heading		The icon used by the heading.
 * \param *xmin			Pointer to variable to take the minimum X coordinate.
 * \param *xmax			Pointer to variable to take the maximum X coordinate.
 */

void column_get_heading_xpos(struct column_block *instance, wimp_i heading, int *xmin, int *xmax)
{
	int	column;

	if (instance == NULL || (xmin == NULL && xmax == NULL))
		return;

	if (xmin != NULL) {
		column = column_get_leftmost_in_heading_group(instance, heading);

		if (column != -1 && instance->position[column] < *xmin)
			*xmin = instance->position[column];
	}

	if (xmax != NULL) {
		column = column_get_rightmost_in_heading_group(instance, heading);

		if (instance->position[column] + instance->width[column] > *xmax)
			*xmax = instance->position[column] + instance->width[column];
	}
}


/**
 * Get the total width of the columns represented by an instance.
 *
 * \param *instance		The column instance to report on.
 * \return			The total width, or 0 on error.
 */

int column_get_window_width(struct column_block *instance)
{
	if (instance == NULL)
		return 0;

	return instance->position[instance->columns - 1] + instance->width[instance->columns - 1];
}


/**
 * Given an X position in OS units, locate the column into which it falls.
 *
 * \param *instance		The column instance to test the position against.
 * \param xpos			The X position from the left margin, in OS units.
 * \return			The icon of the column into which the location falls.
 */

wimp_i column_find_icon_from_xpos(struct column_block *instance, int xpos)
{
	int column;

	if (instance == NULL)
		return wimp_ICON_WINDOW;

	for (column = 0; column < instance->columns && xpos > (instance->position[column] + instance->width[column]);
		column++);

	if (column >= instance->columns)
		return wimp_ICON_WINDOW;

	return instance->map[column].field;
}


/**
 * Given a heading icon, return the sort type associated with it.
 *
 * \param *instance		The column set instance to search.
 * \param heading		The column heading icon to look up.
 * \return			The associated sort order, or SORT_NONE.
 */

enum sort_type column_get_sort_type_from_heading(struct column_block *instance, wimp_i heading)
{
	int	left;

	if (instance == NULL)
		return SORT_NONE;

	left = column_get_leftmost_in_heading_group(instance, heading);
	if (left == -1)
		return SORT_NONE;

	return instance->map[left].sort;
}


/**
 * Given a sort order, return the heading icon associated with it.
 *
 * \param *instance		The column set instance to search.
 * \param sort			The sort type to look up.
 * \return			The associated column heading icon, or wimp_ICON_WINDOW.
 */

wimp_i column_get_heading_from_sort_type(struct column_block *instance, enum sort_type sort)
{
	int	left;

	if (instance == NULL)
		return wimp_ICON_WINDOW;

	left = column_get_leftmost_from_sort_type(instance, sort & SORT_MASK);
	if (left == -1)
		return wimp_ICON_WINDOW;

	return instance->map[left].heading;
}


/**
 * Return the column group icon handle for the column containing a given
 * field icon.
 *
 * \param *instance		The column set instance to search.
 * \param field			The field icon handle to look up.
 * \return			The column heading icon handle.
 */

wimp_i column_get_group_icon(struct column_block *instance, wimp_i field)
{
	int	column;

	if (instance == NULL)
		return wimp_ICON_WINDOW;

	column = column_get_from_field(instance, field);
	if (column == -1)
		return wimp_ICON_WINDOW;

	return instance->map[column].heading;
}


/**
 * Return the parent field column icon handle for the column containing a given
 * field icon.
 *
 * \param *instance		The column set instance to search.
 * \param field			The field icon handle to look up.
 * \return			The parent field icon handle, or wimp_ICON_WINDOW.
 */

wimp_i column_get_parent_field_icon(struct column_block *instance, wimp_i field)
{
	int	column;

	if (instance == NULL)
		return wimp_ICON_WINDOW;

	column = column_get_from_field(instance, field);
	if (column == -1)
		return wimp_ICON_WINDOW;

	return instance->map[column].parent;
}



/**
 * Return the minimum width that a group of columns can be dragged to.  This is
 * a simple sum of the minimum widths of all the columns in that group.
 *
 * \param *instance		The instance colntaining the column group.
 * \param heading		The icon heading the group.
 * \return			The minimum width of the group.
 */

static int column_get_minimum_group_width(struct column_block *instance, wimp_i heading)
{
	int	width = 0;
	int	left, right, column;

	if (instance == NULL)
		return 0;

	left = column_get_leftmost_in_heading_group(instance, heading);
	right = column_get_rightmost_in_heading_group(instance, heading);

	if (left == -1 || right == -1)
		return 0;

	for (column = left; column <= right; column++)
		width += instance->minimum_width[column];

	return(width);
}


/**
 * Return the number of the column using the given field icon.
 *
 * \param *instance		The instance to search.
 * \param field			The field icon to be loctaed.
 * \return			The associated column number, or -1 on failure.
 */

static int column_get_from_field(struct column_block *instance, wimp_i field)
{
	int column;

	if (instance == NULL)
		return -1;

	for (column = 0; column < instance->columns; column++) {
		if (instance->map[column].field == field)
			return column;
	}

	return -1;
}


/**
 * Return the number of the left-most column using the given sort type.
 *
 * \param *instance		The instance to search.
 * \param sort			The sort type to be located.
 * \return			The left-most column number, or -1 on failure.
 */

static int column_get_leftmost_from_sort_type(struct column_block *instance, enum sort_type sort)
{
	int column;

	if (instance == NULL)
		return -1;

	for (column = 0; column < instance->columns; column++) {
		if (instance->map[column].sort == sort)
			return column;
	}

	return -1;
}


/**
 * Return the number of the left-hand column in a given group.
 *
 * \param *mapping		The column group mapping string.
 * \param group			The column group.
 * \return			The left-most column in the group, or -1 on failure.
 */

static int column_get_leftmost_in_heading_group(struct column_block *instance, wimp_i heading)
{
	int	column, match = -1;

	if (instance == NULL)
		return -1;

	for (column = 0; column < instance->columns && instance->map[column].heading <= heading; column++) {
		if (heading == instance->map[column].heading) {
			match = column;
			break;
		}
	}

	return match;
}


/**
 * Return the number of the right-hand column in a given group.
 *
 * \param *mapping		The column group mapping string.
 * \param group			The column group.
 * \return			The right-most column in the group, or -1 on failure.
 */

static int column_get_rightmost_in_heading_group(struct column_block *instance, wimp_i heading)
{
	int	column, match = -1;

	if (instance == NULL)
		return -1;

	for (column = 0; column < instance->columns && instance->map[column].heading <= heading; column++) {
		if (heading == instance->map[column].heading)
			match = column;
	}

	return match;
}


/**
 * Return the number of the right-hand column in a given group.
 *
 * \param *mapping		The column group mapping string.
 * \param group			The column group.
 * \return			The right-most column in the group, or -1 on failure.
 */

static int column_get_rightmost_in_footer_group(struct column_block *instance, wimp_i footer)
{
	int	column, match = -1;

	if (instance == NULL)
		return -1;

	for (column = 0; column < instance->columns && instance->map[column].footer <= footer; column++) {
		if (footer == instance->map[column].footer)
			match = column;
	}

	return match;
}

