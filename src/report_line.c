/* Copyright 2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: report_line.c
 *
 * Track the lines of a report.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"

/* OSLib Header files. */

#include "oslib/types.h"

/* Application header files. */

#include "report_line.h"

#include "flexutils.h"

/**
 * The default allocation block size.
 */

#define REPORT_LINE_ALLOCATION 250


/**
 * A Report Line instance data block.
 */

struct report_line_block {
	struct report_line_data	*lines;
	size_t			size;
	size_t			line_count;
	size_t			allocation;
};


/**
 * Initialise a report line data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_line_block *report_line_create(size_t allocation)
{
	struct report_line_block	*new;

	new = heap_alloc(sizeof(struct report_line_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? REPORT_LINE_ALLOCATION : allocation;

	new->lines = NULL;
	new->line_count = 0;
	new->size = new->allocation;

	/* Claim the memory for the dump itself. */

	if (!flexutils_allocate((void **) &(new->lines), sizeof(struct report_line_data), new->allocation)) {
		heap_free(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a report line data block, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_line_destroy(struct report_line_block *handle)
{
	if (handle == NULL)
		return;

	flexutils_free((void **) &(handle->lines));

	heap_free(handle);
}


/**
 * Clear the contents of a report line data block, so that it will behave
 * as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_line_clear(struct report_line_block *handle)
{
	if (handle == NULL || handle->lines == NULL)
		return;

	handle->line_count = 0;

	if (flexutils_resize((void **) &(handle->lines), sizeof(struct report_line_data), handle->allocation))
		handle->size = handle->allocation;
}


/**
 * Close a report line data block, so that its allocation shrinks to
 * occupy only the space used by data.
 *
 * \param *handle		The block to be closed.
 */

void report_line_close(struct report_line_block *handle)
{
	if (handle == NULL || handle->lines == NULL)
		return;

	if (flexutils_resize((void **) &(handle->lines), sizeof(struct report_line_data), handle->line_count))
		handle->size = handle->line_count;

	debug_printf("Line data: %d records, using %dKb", handle->line_count, handle->line_count * sizeof (struct report_line_data) / 1024);
}


/**
 * Add a line to a report line data block.
 *
 * \param *handle		The block to add to.
 * \param first_cell		The offset of the first cell's data in the cell store.
 * \param cell_count		The number of cells in the line.
 * \param tab_bar		The tab bar which applies to the line.
 * \param flags			The flags associated with the line.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_line_add(struct report_line_block *handle, unsigned first_cell, size_t cell_count, int tab_bar, enum report_line_flags flags)
{
	unsigned new;

	if (handle == NULL || handle->lines == NULL)
		return FALSE;

	if (handle->line_count >= handle->size) {
		if (!flexutils_resize((void **) &(handle->lines), sizeof(struct report_line_data), handle->size + handle->allocation))
			return FALSE;

		handle->size += handle->allocation;
	}

	new = handle->line_count++;

	handle->lines[new].flags = flags;
	handle->lines[new].first_cell = first_cell;
	handle->lines[new].cell_count = cell_count;
	handle->lines[new].tab_bar = tab_bar;
	handle->lines[new].ypos = 0;

	return TRUE;
}


/**
 * Return the number of lines held in a report line data block.
 *
 * \param *handle		The block to query.
 * \return			The number of lines in the block, or 0.
 */

size_t report_line_get_count(struct report_line_block *handle)
{
	if (handle == NULL || handle->lines == NULL)
		return 0;

	return handle->line_count;
}


/**
 * Return details about a line held in a report line data block. The data
 * returned is transient, and not guaracteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param line			The line to query.
 * \return			Pointer to the line data block, or NULL.
 */

struct report_line_data *report_line_get_info(struct report_line_block *handle, unsigned line)
{
	if (handle == NULL || handle->lines == NULL)
		return NULL;

	if (line >= handle->line_count)
		return NULL;

	return handle->lines + line;
}


/**
 * Find a line based on a redraw position on the y axis.
 *
 * \param *handle		The block to query.
 * \param ypos			The Y axis coordinate to look up.
 * \return			The line number.
 */

unsigned report_line_find_from_ypos(struct report_line_block *handle, int ypos)
{
	unsigned	a, b, c;

	if (handle == NULL || handle->lines == NULL)
		return 0;

	a = 0;
	b = handle->line_count - 1;

	while (a < b) {
		c = a + ((b - a) / 2);

		if (ypos >= handle->lines[c].ypos)
			b = c;
		else if (c < b)
			a = c + 1;
		else
			a = c;
	}

	return a;
}

