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

#define REPORT_LINE_ALLOCATION 10240

/**
 * A line in a report.
 */

struct report_line_data {
	unsigned		offset;					/**< Offset of the line data in the text dump block.			*/
	int			tab_bar;				/**< The tab bar which relates to the line.				*/
	int			ypos;					/**< The vertical position of the line in the window, in OS Units.	*/
};

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

	if (!flexutils_allocate((void **) &(new->lines), sizeof(struct report_line_block), new->allocation)) {
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

	if (flexutils_resize((void **) &(handle->lines), sizeof(struct report_line_block), handle->allocation))
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

	if (flexutils_resize((void **) &(handle->lines), sizeof(struct report_line_block), handle->line_count))
		handle->size = handle->line_count;
}


/**
 * Add a line to a report line data block.
 *
 * \param *handle		The block to add to.
 * \param offset		The offset of the line data in the data store.
 * \param tab_bar		The tab bar which applies to the line.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_line_add(struct report_line_block *handle, unsigned offset, int tab_bar)
{
	if (handle == NULL || handle->lines == NULL)
		return FALSE;

	if (handle->line_count >= handle->size) {
		if (!flexutils_resize((void **) &(handle->lines), sizeof(struct report_line_block), handle->size + handle->allocation))
			return FALSE;

		handle->size += handle->allocation;
	}

	handle->lines[handle->line_count].offset = offset;
	handle->lines[handle->line_count].tab_bar = tab_bar;
	handle->lines[handle->line_count].ypos = 0;

	handle->line_count++;

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
 * Return details about a line held in a report line data block.
 *
 * \param *handle		The block to query.
 * \param line			The line to query.
 * \param *offset		Pointer to variable to take the line data offset.
 * \param *tab_bar		Pointer to variable to take the line's tab bar.
 * \return			TRUE on success; FALSE on failure.
 */

osbool report_line_get_info(struct report_line_block *handle, unsigned line, unsigned *offset, int *tab_bar)
{
	if (handle == NULL || handle->lines == NULL)
		return FALSE;

	if (line >= handle->line_count)
		return FALSE;

	if (offset != NULL)
		*offset = handle->lines[line].offset;

	if (tab_bar != NULL)
		*tab_bar = handle->lines[line].tab_bar;

	return TRUE;
}

