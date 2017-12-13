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
 * \file: report_page.c
 *
 * Track the pages of a report.
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

#include "report_page.h"

#include "flexutils.h"

/**
 * The default allocation block size.
 */

#define REPORT_PAGE_ALLOCATION 20

/**
 * The border drawn around page blocks during screen display, in OS Units.
 */

#define REPORT_PAGE_BORDER 60

/**
 * A Report Page instance data block.
 */

struct report_page_block {
	struct report_page_data	*pages;
	size_t			size;
	size_t			page_count;
	size_t			allocation;

	os_coord		page_layout;		/**< The number of pages in the report.				*/

	int			column;			/**< The current column number whilst paginating.		*/

	os_coord		page_size;		/**< The size of a page, in OS Units.				*/
	os_box			margins;		/**< The location of the print margins.				*/

	enum report_page_area	active_areas;		/**< The areas which are active in the page.			*/

	os_box			body;			/**< The location of the page body.				*/
	os_box			header;			/**< The location of the page header.				*/
	os_box			footer;			/**< The location of the page footer.				*/

	osbool			landscape;		/**< TRUE if the layout is landscape; FALSE if portrait.	*/
	osbool			paginated;		/**< TRUE if there is pagination data; FALSE if not.		*/
};

/* Static Function Prototypes. */



/**
 * Initialise a report page data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_page_block *report_page_create(size_t allocation)
{
	struct report_page_block	*new;

	new = heap_alloc(sizeof(struct report_page_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? REPORT_PAGE_ALLOCATION : allocation;

	new->pages = NULL;
	new->size = new->allocation;

	new->page_count = 0;
	new->page_layout.x = 0;
	new->page_layout.y = 0;
	new->column = 0;

	new->active_areas = REPORT_PAGE_AREA_NONE;

	new->paginated = FALSE;

	/* Claim the memory for the pages themselves. */

	if (!flexutils_allocate((void **) &(new->pages), sizeof(struct report_page_data), new->allocation)) {
		heap_free(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a report page data block, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_page_destroy(struct report_page_block *handle)
{
	if (handle == NULL)
		return;

	flexutils_free((void **) &(handle->pages));

	heap_free(handle);
}


/**
 * Clear the contents of a report page data block, so that it will behave
 * as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_page_clear(struct report_page_block *handle)
{
	if (handle == NULL || handle->pages == NULL)
		return;

	handle->page_count = 0;
	handle->page_layout.x = 0;
	handle->page_layout.y = 0;
	handle->column = 0;

	handle->active_areas = REPORT_PAGE_AREA_NONE;

	handle->paginated = FALSE;

	if (flexutils_resize((void **) &(handle->pages), sizeof(struct report_page_data), handle->allocation))
		handle->size = handle->allocation;
}


/**
 * Close a report page data block, so that its allocation shrinks to
 * occupy only the space used by data. This will also mark the data
 * as being valid.
 *
 * \param *handle		The block to be closed.
 */

void report_page_close(struct report_page_block *handle)
{
	if (handle == NULL || handle->pages == NULL)
		return;

	if (flexutils_resize((void **) &(handle->pages), sizeof(struct report_page_data), handle->page_count))
		handle->size = handle->page_count;

	if (handle->page_count > 0)
		handle->paginated = TRUE;

	debug_printf("Page data: %d records, using %dKb", handle->page_count, handle->page_count * sizeof(struct report_page_data) / 1024);
	debug_printf("Page layout: x=%d, y=%d", handle->page_layout.x, handle->page_layout.y);
}


/**
 * Report whether a page data block contains valid pagination data.
 *
 * \param *handle		The block to test.
 * \return			TRUE if the data is valid; FALSE if not.
 */

osbool report_page_paginated(struct report_page_block *handle)
{
	return (handle == NULL) ? FALSE : handle->paginated;
}


/**
 * Start a new row of pages in the page output.
 *
 * \param *handle		The block to add a row to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_page_new_row(struct report_page_block *handle)
{
	if (handle == NULL)
		return FALSE;

	handle->page_layout.y++;
	handle->column = 0;

	return TRUE;
}


/**
 * Add a page to the page output.
 *
 * \param *handle		The block to add the page to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_page_add(struct report_page_block *handle)
{
	unsigned new;

	if (handle == NULL || handle->pages == NULL)
		return FALSE;

	/* Check and increase the memory allocation if required. */

	if (handle->page_count >= handle->size) {
		if (!flexutils_resize((void **) &(handle->pages), sizeof(struct report_page_data), handle->size + handle->allocation))
			return FALSE;

		handle->size += handle->page_layout.x + handle->allocation;
	}

	/* Set up the new pages. */

	new = handle->page_count++;

	handle->pages[new].position.x = ++handle->column;
	handle->pages[new].position.y = handle->page_layout.y;

	if (handle->column > handle->page_layout.x)
		handle->page_layout.x = handle->column;

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
#endif


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





/**
 * Read the current printer page size, and work out from the configured margins
 * where on the page the printed body, header and footer will go.
 *
 * \param *handle		The page block to update.
 * \param landscape		TRUE to rotate the page to Landscape format; else FALSE.
 * \param body_width		The required width of the page body, in OS Units, or zero.
 * \param header_size		The required height of the header, in OS Units, or zero.
 * \param footer_size		The required height of the footer, in OS Units, or zero.
 * \return			Pointer to an error block on failure, or NULL on success.
 */

os_error *report_page_calculate_areas(struct report_page_block *handle, osbool landscape, int body_width, int header_size, int footer_size)
{
	os_error			*error;
	osbool				margin_fail = FALSE;
	enum report_page_area		areas = REPORT_PAGE_AREA_NONE;
	int				page_xsize, page_ysize, page_left, page_right, page_top, page_bottom;
	int				margin_left, margin_right, margin_top, margin_bottom;

	if (handle == NULL)
		return NULL;

	handle->active_areas = REPORT_PAGE_AREA_NONE;

	/* Get the page dimensions, and set up the print margins.  If the margins are bigger than the print
	 * borders, the print borders are increased to match.
	 */

	error = xpdriver_page_size(&page_xsize, &page_ysize, &page_left, &page_bottom, &page_right, &page_top);
	if (error != NULL)
		return error;

	handle->page_size.x = page_xsize / 400;
	handle->page_size.y = page_ysize / 400;

	margin_left = page_left;

	if (config_int_read("PrintMarginLeft") > 0 && config_int_read("PrintMarginLeft") > margin_left) {
		margin_left = config_int_read("PrintMarginLeft");
		page_left = margin_left;
	} else if (config_int_read("PrintMarginLeft") > 0) {
		margin_fail = TRUE;
	}

	margin_bottom = page_bottom;

	if (config_int_read("PrintMarginBottom") > 0 && config_int_read("PrintMarginBottom") > margin_bottom) {
		margin_bottom = config_int_read("PrintMarginBottom");
		page_bottom = margin_bottom;
	} else if (config_int_read("PrintMarginBottom") > 0) {
		margin_fail = TRUE;
	}

	margin_right = page_xsize - page_right;

	if (config_int_read("PrintMarginRight") > 0 && config_int_read("PrintMarginRight") > margin_right) {
		margin_right = config_int_read("PrintMarginRight");
		page_right = page_xsize - margin_right;
	} else if (config_int_read("PrintMarginRight") > 0) {
		margin_fail = TRUE;
	}

	margin_top = page_ysize - page_top;

	if (config_int_read("PrintMarginTop") > 0 && config_int_read("PrintMarginTop") > margin_top) {
		margin_top = config_int_read("PrintMarginTop");
		page_top = page_ysize - margin_top;
	} else if (config_int_read("PrintMarginTop") > 0) {
		margin_fail = TRUE;
	}

	areas |= REPORT_PAGE_AREA_BODY;

	if (landscape) {
		handle->body.x0 = page_bottom / 400;
		handle->body.x1 = page_top / 400;
		handle->body.y0 = page_right / 400;
		handle->body.y1 = page_left / 400;
	} else {
		handle->body.x0 = page_left / 400;
		handle->body.x1 = page_right / 400;
		handle->body.y0 = page_bottom / 400;
		handle->body.y1 = page_top / 400;
	}

	if (header_size > 0) {
		handle->header.x0 = handle->body.x0;
		handle->header.x1 = handle->body.x1;
		handle->header.y1 = handle->body.y1;

		handle->header.y0 = handle->header.y1 - (header_size * ((landscape) ? -1 : 1));
		handle->body.y1 = handle->header.y0 - (config_int_read("PrintMarginInternal") * ((landscape) ? -1 : 1));

		areas |= REPORT_PAGE_AREA_HEADER;
	}

	if (footer_size > 0) {
		handle->footer.x0 = handle->body.x0;
		handle->footer.x1 = handle->body.x1;
		handle->footer.y0 = handle->body.y0;

		handle->footer.y1 = handle->footer.y0 + (footer_size * ((landscape) ? -1 : 1));
		handle->body.y0 = handle->footer.y1 + (config_int_read("PrintMarginInternal") * ((landscape) ? -1 : 1));

		areas |= REPORT_PAGE_AREA_FOOTER;
	}

	if (margin_fail)
		error_msgs_report_error("BadPrintMargins");

	return NULL;
}





