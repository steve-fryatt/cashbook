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
#include "report_textdump.h"

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

static unsigned report_region_add(struct report_region_block *handle, int x0, int y0, int x1, int y1);


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
 * Add a static text region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \param *outline		The outline of the region on the page, in OS Units.
 * \param content		The textdump offset to the region content, or REPORT_TEXTDUMP_NULL.
 * \return			The new region number, or REPORT_REGION_NONE.
 */

unsigned report_region_add_text(struct report_region_block *handle, os_box *outline, unsigned content)
{
	unsigned new;

	new = report_region_add(handle, outline->x0, outline->y0, outline->x1, outline->y1);
	if (new == REPORT_REGION_NONE)
		return new;

	handle->regions[new].type = REPORT_REGION_TYPE_TEXT;

	handle->regions[new].data.text.content = REPORT_TEXTDUMP_NULL;

	return new;
}


/**
 * Add a page number region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \param *outline		The outline of the region on the page, in OS Units.
 * \param major			The major page number.
 * \param minor			The minor page number, or -1 for none.
 * \return			The new region number, or REPORT_REGION_NONE.
 */

unsigned report_region_add_page_number(struct report_region_block *handle, os_box *outline, int major, int minor)
{
	unsigned new;

	new = report_region_add(handle, outline->x0, outline->y0, outline->x1, outline->y1);
	if (new == REPORT_REGION_NONE)
		return new;

	handle->regions[new].type = REPORT_REGION_TYPE_PAGE_NUMBER;

	handle->regions[new].data.page_number.major = major;
	handle->regions[new].data.page_number.minor = minor;

	return new;
}


/**
 * Add a lines region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \param *outline		The outline of the region on the page, in OS Units.
 * \param first			The first line nummber to display in the region.
 * \param last			The last line number to display in the region.
 * \return			The new region number, or REPORT_REGION_NONE.
 */

unsigned report_region_add_lines(struct report_region_block *handle, os_box *outline, int first, int last)
{
	unsigned new;

	new = report_region_add(handle, outline->x0, outline->y0, outline->x1, outline->y1);
	if (new == REPORT_REGION_NONE)
		return new;

	handle->regions[new].type = REPORT_REGION_TYPE_LINES;

	handle->regions[new].data.lines.first = first;
	handle->regions[new].data.lines.last = last;

	return new;
}


/**
 * Add an empty region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \return			The new region number, or REPORT_REGION_NONE.
 */

static unsigned report_region_add(struct report_region_block *handle, int x0, int y0, int x1, int y1)
{
	unsigned new;

	if (handle == NULL || handle->regions == NULL)
		return REPORT_REGION_NONE;

	if (handle->region_count >= handle->size) {
		if (!flexutils_resize((void **) &(handle->regions), sizeof(struct report_region_data), handle->size + handle->allocation))
			return REPORT_REGION_NONE;

		handle->size += handle->allocation;
	}

	new = handle->region_count++;

	handle->regions[new].position.x0 = x0;
	handle->regions[new].position.y0 = y0;
	handle->regions[new].position.x1 = x1;
	handle->regions[new].position.y1 = y1;

	handle->regions[new].type = REPORT_REGION_TYPE_NONE;

	return new;
}


/**
 * Return details about a region held in a report region data block. The data
 * returned is transient, and not guaracteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param region			The region to query.
 * \return			Pointer to the region data block, or NULL.
 */

struct report_region_data *report_region_get_info(struct report_region_block *handle, unsigned region)
{
	if (handle == NULL || handle->regions == NULL)
		return NULL;

	if (region >= handle->region_count)
		return NULL;

	return handle->regions + region;
}

