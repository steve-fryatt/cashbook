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
static int		column_get_minimum_group_width(char *mapping, char *widths, int group);
static int		column_get_minimum_width(char *widths, int column);


/**
 * Create a new column definition instance.
 * 
 * \param columns		The number of columns to be defined.
 * \return			Pointer to the instance, or NULL on failure.
 */

struct column_block *column_create_instance(size_t columns)
{
	void			*mem;
	struct column_block	*new;

	mem = heap_alloc(sizeof(struct column_block) + (2 * columns * sizeof(int)));
	if (mem == NULL)
		return NULL;

	new = mem;

	new->columns = columns;

	mem += sizeof(struct column_block);

	new->position = mem;
	new->width = mem + (columns * sizeof(int));

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

	new = column_create_instance(instance->columns);
	if (new == NULL)
		return NULL;

	new->columns = instance->columns;

	column_copy_instance(instance, new);

	return new;
}


/**
 * Copy the column settings from one column definition instance to another.
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
 * Start a column width drag operation.
 *
 * \param *ptr			The Wimp pointer data starting the drag.
 * \param *data			Client-specific data pointer.
 * \param w			The parent window the dragged toolbar belongs to.
 * \param *mapping		The column group mapping for the window.
 * \param *widths		The minimum column width configuration string.
 * \param *callback		The function to be called at the end of the drag.
 */

void column_start_drag(wimp_pointer *ptr, void *data, wimp_w w, char *mapping, char *widths, void (*callback)(void *, wimp_i, int))
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
				(icon.icon.extent.x1 - icon.icon.extent.x0 - column_get_minimum_group_width(mapping, widths, ptr->i));
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
 * \param *mapping		The column group mapping for the window.
 * \param *widths		The minimum column width configuration string.
 * \param group			The column group that has been resized.
 * \param new_width		The new width of the dragged group.
 * \param *instance		The column instance to be updated.
 */

void update_dragged_columns(char *mapping, char *widths, int group, int new_width, struct column_block *instance)
{
	int	sum = 0, i, left, right;

	if (instance == NULL)
		return;

	left = column_get_leftmost_in_group(mapping, group);
	right = column_get_rightmost_in_group(mapping, group);

	if (left < 0 || right >= instance->columns || left > right)
		return;

	for (i = left; i <= right; i++) {
		if (i == right) {
			instance->width[i] = new_width - (sum + COLUMN_HEADING_MARGIN);
		} else {
			instance->width[i] = column_get_minimum_width(widths, i);
			sum +=  (column_get_minimum_width(widths, i) + COLUMN_GUTTER);
		}
	}

	for (i = left + 1; i < instance->columns; i++)
		instance->position[i] = instance->position[i - 1] + instance->width[i - 1] + COLUMN_GUTTER;
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
 * \return			The column into which the location falls.
 */

int column_get_position(struct column_block *instance, int xpos)
{
	int column;

	if (instance == NULL)
		return 0;

	for (column = 0; column < instance->columns && xpos > (instance->position[column] + instance->width[column]);
		column++);

	return column;
}


/**
 * Return the number of the column group containing the given column.
 *
 * \param *mapping		The column group mapping string.
 * \param column		The column to investigate.
 * \return			The column group number.
 */

int column_get_group(char *mapping, int column)
{
	int	group = 0;

	while (column_get_rightmost_in_group(mapping, group) < column)
		group++;

	return group;
}


/**
 * Return the number of the left-hand column in a given group.
 *
 * \param *mapping		The column group mapping string.
 * \param group			The column group.
 * \return			The left-most column in the group.
 */

int column_get_leftmost_in_group(char *mapping, int group)
{
	char	*copy, *token, *value;
	int	column = 0;

	copy = strdup(mapping);

	if (copy == NULL)
		return column;

	/* Find the mapping block for the required group. */

	token = strtok(copy, ";");

	while (group-- > 0)
		token = strtok(NULL, ";");

	/* Find the left-most column in the block. */

	value = strtok(token, ",");
	column = atoi(value);

	free(copy);

	return column;
}


/**
 * Return the number of the right-hand column in a given group.
 *
 * \param *mapping		The column group mapping string.
 * \param group			The column group.
 * \return			The right-most column in the group.
 */

int column_get_rightmost_in_group(char *mapping, int group)
{
	char	*copy, *token, *value;
	int	column = 0;

	copy = strdup(mapping);

	if (copy == NULL)
		return column;

	/* Find the mapping block for the required group. */

	token = strtok(copy, ";");

	while (group-- > 0)
		token = strtok(NULL, ";");

	/* Find the right-most column in the block. */

	value = strtok(token, ",");

	while (value != NULL) {
		column = atoi(value);
		value = strtok(NULL, ",");
	}

	free(copy);

	return column;
}


/**
 * Return the minimum width that a group of columns can be dragged to.  This is
 * a simple sum of the minimum widths of all the columns in that group.
 *
 * \param *mapping		The column group mapping to apply.
 * \param *widths		The minimum column width configuration string.
 * \param group			The group to return data for.
 * \return			The minimum width of the group.
 */

static int column_get_minimum_group_width(char *mapping, char *widths, int group)
{
	int	left, right, width, i;

	width = 0;

	left = column_get_leftmost_in_group(mapping, group);
	right = column_get_rightmost_in_group(mapping, group);

	for (i = left; i <= right; i++)
		width += column_get_minimum_width (widths, i);

	return(width);
}


/**
 * Return the minimum width that a column can be dragged to.
 *
 * \param *mapping		The column group mapping to apply.
 * \param *widths		The minimum column width configuration string.
 * \param group			The group to return data for.
 * \return			The minimum width of the group.
 */

static int column_get_minimum_width(char *widths, int column)
{
	int	width, i;
	char	*token, *copy;


	width = COLUMN_DRAG_MIN;
	copy = strdup(widths);

	if (copy == NULL)
		return width;

	token = strtok(copy, ",");
	i = 0;

	while (i < column) {
		token = strtok(NULL, ",");
		i++;
	}

	if (token != NULL)
		width = atoi(token);

	free(copy);

	return width;
}

