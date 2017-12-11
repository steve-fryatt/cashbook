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
 * \file: report_region.c
 *
 * Track the regions of a report page.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/heap.h"

/* OSLib Header files. */

#include "oslib/os.h"
#include "oslib/pdriver.h"
#include "oslib/types.h"

/* Application header files. */

#include "report_region.h"

#include "flexutils.h"

/**
 * The default allocation block size.
 */

#define REPORT_REGION_ALLOCATION 20

/**
 * A Report Page instance data block.
 */

struct report_region_block {
	struct report_region_data	*regions;
	size_t				size;
	size_t				region_count;
	size_t				allocation;
};

/* Static Function Prototypes. */



/**
 * Initialise a report page region data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_region_block *report_region_create(size_t allocation)
{
	struct report_region_block	*new;

	new = heap_alloc(sizeof(struct report_region_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? REPORT_REGION_ALLOCATION : allocation;

	new->regions = NULL;
	new->region_count = 0;
	new->size = new->allocation;

	/* Claim the memory for the regions themselves. */

	if (!flexutils_allocate((void **) &(new->regions), sizeof(struct report_region_data), new->allocation)) {
		heap_free(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a report page region data block, freeing the memory associated
 * with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_region_destroy(struct report_region_block *handle)
{
	if (handle == NULL)
		return;

	flexutils_free((void **) &(handle->regions));

	heap_free(handle);
}


/**
 * Clear the contents of a report page region data block, so that it will
 * behave as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_region_clear(struct report_region_block *handle)
{
	if (handle == NULL || handle->regions == NULL)
		return;

	handle->region_count = 0;

	if (flexutils_resize((void **) &(handle->regions), sizeof(struct report_region_data), handle->allocation))
		handle->size = handle->allocation;
}


/**
 * Close a report page region data block, so that its allocation shrinks
 * to occupy only the space used by data.
 *
 * \param *handle		The block to be closed.
 */

void report_region_close(struct report_region_block *handle)
{
	if (handle == NULL || handle->regions == NULL)
		return;

	if (flexutils_resize((void **) &(handle->regions), sizeof(struct report_region_data), handle->region_count))
		handle->size = handle->region_count;

	debug_printf("Region data: %d records, using %dKb", handle->region_count, handle->region_count * sizeof (struct report_region_data) / 1024);
}


/**
 * Add a region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_region_add(struct report_region_block *handle)
{
	unsigned new;

	if (handle == NULL || handle->regions == NULL)
		return FALSE;

	if (handle->region_count >= handle->size) {
		if (!flexutils_resize((void **) &(handle->regions), sizeof(struct report_region_data), handle->size + handle->allocation))
			return FALSE;

		handle->size += handle->allocation;
	}

	new = handle->region_count++;

	handle->regions[new].position.x0 = 0;
	handle->regions[new].position.y0 = 0;
	handle->regions[new].position.x1 = 0;
	handle->regions[new].position.y1 = 0;

	return TRUE;
}
#if 0

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
 * Find a page based on a redraw position on the X axis.
 *
 * \param *handle		The block to query.
 * \param ypos			The X axis coordinate to look up.
 * \return			The page number.
 */

unsigned report_page_find_from_xpos(struct report_page_block *handle, int xpos)
{
	int page, position;

	if (handle == NULL || handle->pages == NULL || handle->paginated == FALSE)
		return REPORT_PAGE_NONE;

	page = xpos / (handle->page_size.x + (2 * REPORT_PAGE_BORDER));

	position = xpos % (handle->page_size.x + (2 * REPORT_PAGE_BORDER));
	if (position > handle->page_size.x + REPORT_PAGE_BORDER)
		page += 1;

	if (page >= handle->page_layout.x)
		page = handle->page_layout.x - 1;

	if (page < 0)
		page = 0;

	return page;
}


/**
 * Find a page based on a redraw position on the Y axis.
 *
 * \param *handle		The block to query.
 * \param ypos			The Y axis coordinate to look up.
 * \return			The page number.
 */

unsigned report_page_find_from_ypos(struct report_page_block *handle, int ypos)
{
	int page, position;

	if (handle == NULL || handle->pages == NULL || handle->paginated == FALSE)
		return REPORT_PAGE_NONE;

	page = -ypos / (handle->page_size.y + (2 * REPORT_PAGE_BORDER));

	position = -ypos % (handle->page_size.y + (2 * REPORT_PAGE_BORDER));
	if (position > handle->page_size.y + REPORT_PAGE_BORDER)
		page += 1;

	if (page >= handle->page_layout.y)
		page = handle->page_layout.y - 1;

	if (page < 0)
		page = 0;

	return page;
}


osbool report_page_get_outline(struct report_page_block *handle, int x, int y, os_box *area)
{
	if (handle == NULL || area == NULL || handle->paginated == FALSE)
		return FALSE;

	if (x < 0 || x >= handle->page_layout.x || y < 0 || y >= handle->page_layout.y)
		return FALSE;

	area->x0 = x * (handle->page_size.x + (2 * REPORT_PAGE_BORDER)) + REPORT_PAGE_BORDER;
	area->x1 = area->x0 + handle->page_size.x;
	area->y1 = -(y * (handle->page_size.y + (2 * REPORT_PAGE_BORDER)) + REPORT_PAGE_BORDER);
	area->y0 = area->y1 - handle->page_size.y;

	return TRUE;
}

osbool report_page_get_layout_extent(struct report_page_block *handle, int *x, int *y)
{
	if (handle == NULL || handle->paginated == FALSE)
		return FALSE;

	if (x != NULL)
		*x = (handle->page_size.x + (2 * REPORT_PAGE_BORDER)) * handle->page_layout.x;

	if (y != NULL)
		*y = (handle->page_size.y + (2 * REPORT_PAGE_BORDER)) * handle->page_layout.y;

	return TRUE;
}

#endif


