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
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "column.h"

#include "account.h"
#include "accview.h"
#include "edit.h"
#include "file.h"
#include "presets.h"
#include "sorder.h"
#include "transact.h"


static void		*column_drag_data;					/**< Client-specific data for the drag.				*/
static wimp_i		column_drag_icon;					/**< The handle of the icon being dragged.			*/
static void		(*column_drag_callback)(void *, wimp_i, int);		/**< The callback handler for the drag.				*/


static void		column_terminate_drag(wimp_dragged *drag, void *data);
static int		column_get_minimum_group_width(struct column_block *instance, int group);
static int		column_get_from_field(struct column_block *instance, wimp_i field);
static int		column_get_leftmost_in_heading_group(struct column_block *instance, wimp_i heading);
static int		column_get_rightmost_in_heading_group(struct column_block *instance, wimp_i heading);
static int		column_get_rightmost_in_footer_group(struct column_block *instance, wimp_i footer);


/**
 * Create a new column definition instance.
 * 
 * \param columns		The number of columns to be defined.
 * \param *map			Pointer to the column icon map.
 * \param *extra		Pointer to the extra icon list, or NULL for none.
 * \return			Pointer to the instance, or NULL on failure.
 */

struct column_block *column_create_instance(size_t columns, struct column_map *map, struct column_extra *extra)
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

	new = column_create_instance(instance->columns, instance->map, instance->extra);
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
 * \param skip			TRUE to ignore missing entyries; FALSE to set to default.
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
 * Horizontally position the table icons in a window defintion, so that
 * they are ready to be used in a redraw operation. If a buffer is supplied,
 * the icons' indirected data is set up to point to it.
 *
 * \param *instance		Pointer to the column instance to use.
 * \param *definition		Pointer to the table window template definition.
 * \param *buffer		Pointer to a buffer to use for indirection, or NULL for none.
 * \param length		The length of the indirection buffer.
 */

void columns_place_table_icons(struct column_block *instance, wimp_window *definition, char *buffer, size_t length)
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
 * \param heading		The column heading icon handle to take the sort indicator.
 * \param sort_order		The sort details for the table.
 */

void column_update_search_indicator(struct column_block *instance, wimp_icon *indicator, wimp_window *window, wimp_i heading, enum sort_type sort_order)
{
	int		column, anchor, width;
	char		*sprite;
	size_t		length;

	if (instance == NULL || indicator == NULL || window == NULL)
		return;

	/* Update the sort icon sprite name. As this is a sprite name, there's
	 * no need to forcibly terminate the string on a buffer overrun, as the
	 * system stops at 12 characters anyway.
	 */

	sprite = (char *) indicator->data.indirected_sprite.id;
	length = indicator->data.indirected_sprite.size;

	if (sort_order & SORT_ASCENDING)
		strncpy(sprite, "sortarrd", length);
	else if (sort_order & SORT_DESCENDING)
		strncpy(sprite, "sortarru", length);

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
 * \return			The iocn of the column into which the location falls.
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
