/* Copyright 2017-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: report_cell.c
 *
 * Track the cells of a report.
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

#include "report_cell.h"

#include "flexutils.h"

/**
 * The default allocation block size.
 */

#define REPORT_CELL_ALLOCATION 250


/**
 * A Report Cell instance data block.
 */

struct report_cell_block {
	struct report_cell_data	*cells;
	size_t			size;
	size_t			cell_count;
	size_t			allocation;
};


/**
 * Initialise a report cell data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_cell_block *report_cell_create(size_t allocation)
{
	struct report_cell_block	*new;

	new = heap_alloc(sizeof(struct report_cell_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? REPORT_CELL_ALLOCATION : allocation;

	new->cells = NULL;
	new->cell_count = 0;
	new->size = new->allocation;

	/* Claim the memory for the dump itself. */

	if (!flexutils_allocate((void **) &(new->cells), sizeof(struct report_cell_data), new->allocation)) {
		heap_free(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a report cell data block, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_cell_destroy(struct report_cell_block *handle)
{
	if (handle == NULL)
		return;

	flexutils_free((void **) &(handle->cells));

	heap_free(handle);
}


/**
 * Clear the contents of a report cell data block, so that it will behave
 * as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_cell_clear(struct report_cell_block *handle)
{
	if (handle == NULL || handle->cells == NULL)
		return;

	handle->cell_count = 0;

	if (flexutils_resize((void **) &(handle->cells), sizeof(struct report_cell_data), handle->allocation))
		handle->size = handle->allocation;
}


/**
 * Close a report cell data block, so that its allocation shrinks to
 * occupy only the space used by data.
 *
 * \param *handle		The block to be closed.
 */

void report_cell_close(struct report_cell_block *handle)
{
	if (handle == NULL || handle->cells == NULL)
		return;

	if (flexutils_resize((void **) &(handle->cells), sizeof(struct report_cell_data), handle->cell_count))
		handle->size = handle->cell_count;

#ifdef DEBUG
	debug_printf("Cell data: %d records, using %dKb", handle->cell_count, handle->cell_count * sizeof (struct report_cell_data) / 1024);
#endif
}


/**
 * Add a cell to a report line data block.
 *
 * \param *handle		The block to add to.
 * \param offset		The offset of the cell data in the data store.
 * \param tab_stop		The tab stop which applies to the cell.
 * \param flags			The flags associated with the cell.
 * \return			The cell's offset, or REPORT_CELL_NULL on failure.
 */

unsigned report_cell_add(struct report_cell_block *handle, unsigned offset, int tab_stop, enum report_cell_flags flags)
{
	unsigned new;

	if (handle == NULL || handle->cells == NULL)
		return REPORT_CELL_NULL;

	if (handle->cell_count >= handle->size) {
		if (!flexutils_resize((void **) &(handle->cells), sizeof(struct report_cell_data), handle->size + handle->allocation))
			return REPORT_CELL_NULL;

		handle->size += handle->allocation;
	}

	new = handle->cell_count++;

	handle->cells[new].flags = flags;
	handle->cells[new].offset = offset;
	handle->cells[new].tab_stop = tab_stop;

	return new;
}


/**
 * Return details about a line held in a report line data block. The data
 * returned is transient, and not guaranteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param line			The cell to query.
 * \return			Pointer to the cell data block, or NULL.
 */

struct report_cell_data *report_cell_get_info(struct report_cell_block *handle, unsigned cell)
{
	if (handle == NULL || handle->cells == NULL)
		return NULL;

	if (cell >= handle->cell_count)
		return NULL;

	return handle->cells + cell;
}

