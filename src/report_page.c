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
 * The number of millipoints in an OS Unit.
 */

#define REPORT_PAGE_MPOINTS_TO_OS 400

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
	os_coord		display_size;		/**> The display size of a page, in OS Units.			*/
	os_box			margins;		/**< The location of the print margins.				*/

	enum report_page_area	active_areas;		/**< The areas which are active in the page.			*/

	os_box			body;			/**< The location of the page body.				*/
	os_box			header;			/**< The location of the page header.				*/
	os_box			footer;			/**< The location of the page footer.				*/

	int			scale;			/**< The print scale, as used in the transformation matrix.	*/
	os_hom_trfm		print_transform;	/**< The printer driver transformation matrix.			*/

	osbool			landscape;		/**< TRUE if the layout is landscape; FALSE if portrait.	*/
	osbool			paginated;		/**< TRUE if there is pagination data; FALSE if not.		*/
};

/* Static Function Prototypes. */

static void report_page_scale_area(os_box *area, int scale);

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

#ifdef DEBUG
	debug_printf("Page data: %d records, using %dKb", handle->page_count, handle->page_count * sizeof(struct report_page_data) / 1024);
	debug_printf("Page layout: x=%d, y=%d", handle->page_layout.x, handle->page_layout.y);
#endif
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
 * \param first_region		The offset of the first region on the page.
 * \param region_count		The number of regions on the page.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_page_add(struct report_page_block *handle, unsigned first_region, size_t region_count)
{
	unsigned new;

	if (handle == NULL || handle->pages == NULL)
		return FALSE;

	/* Check and increase the memory allocation if required. */

	if (handle->page_count >= handle->size) {
		if (!flexutils_resize((void **) &(handle->pages), sizeof(struct report_page_data), handle->size + handle->allocation))
			return FALSE;

		handle->size += handle->allocation;
	}

	/* Set up the new page. */

	new = handle->page_count++;

	handle->pages[new].position.x = handle->column++;
	handle->pages[new].position.y = handle->page_layout.y - 1;

	handle->pages[new].first_region = first_region;
	handle->pages[new].region_count = region_count;

	if (handle->column > handle->page_layout.x)
		handle->page_layout.x = handle->column;

	return TRUE;
}


/**
 * Return the number of pages held in a report page data block.
 *
 * \param *handle		The block to query.
 * \return			The number of pages in the block, or 0.
 */

size_t report_page_get_count(struct report_page_block *handle)
{
	if (handle == NULL || handle->pages == NULL)
		return 0;

	return handle->page_count;
}


/**
 * Return details about a page held in a report page data block. The data
 * returned is transient, and not guaracteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param line			The page to query.
 * \return			Pointer to the page data block, or NULL.
 */

struct report_page_data *report_page_get_info(struct report_page_block *handle, unsigned page)
{
	if (handle == NULL || handle->pages == NULL)
		return NULL;

	if (page >= handle->page_count)
		return NULL;

	return handle->pages + page;
}


/**
 * Return the number of pages in the X and Y directions. No page counts can be
 * returned if the pages have not been calculated.
 *
 * \param *handle		The block to query.
 * \param *x			Pointer to a variable to take the X page count.
 * \param *y			Pointer to a variable to take the Y page count.
 * \return			TRUE if successful; FALSE if no page counts could
 *				be returned.
 */

osbool report_page_get_layout_pages(struct report_page_block *handle, int *x, int *y)
{
	if (handle == NULL || handle->paginated == FALSE)
		return FALSE;

	if (x != NULL)
		*x = handle->page_layout.x;

	if (y != NULL)
		*y = handle->page_layout.y;

	return TRUE;
}


/**
 * Calculate the extent of an on-screen representation of the pages,
 * based on the 2D layout and the on-screen page size. No size can be
 * returned if the pages have not been calculated.
 *
 * \param *handle		The block to query.
 * \param *x			Pointer to a variable to take the X size, in OS Units.
 * \param *y			Pointer to a variable to take the Y size, in OS Units.
 * \return			TRUE if successful; FALSE if no size could
 *				be returned.
 */

osbool report_page_get_layout_extent(struct report_page_block *handle, int *x, int *y)
{
	if (handle == NULL || handle->paginated == FALSE)
		return FALSE;

	if (x != NULL)
		*x = (handle->display_size.x + (2 * REPORT_PAGE_BORDER)) * handle->page_layout.x;

	if (y != NULL)
		*y = (handle->display_size.y + (2 * REPORT_PAGE_BORDER)) * handle->page_layout.y;

	return TRUE;
}


/**
 * Find a page based on a redraw position on the X axis. Note that at
 * the edges of the display area, this might return pages outside of the
 * active range.
 *
 * \param *handle		The block to query.
 * \param ypos			The X axis coordinate to look up.
 * \param high			TRUE if we're matching the high coordinate.
 * \return			The page number in the X direction.
 */

int report_page_find_from_xpos(struct report_page_block *handle, int xpos, osbool high)
{
	int page, position;

	/* Returning 0 for low and -1 for high ensures that the redraw
	 * loop is not entered if the exit route is consistantly called
	 * for both ends of the range.
	 */

	if (handle == NULL || handle->pages == NULL || handle->paginated == FALSE)
		return (high) ? -1 : 0;

	page = xpos / (handle->display_size.x + (2 * REPORT_PAGE_BORDER));

	position = xpos % (handle->display_size.x + (2 * REPORT_PAGE_BORDER));

	if ((high == TRUE) && (position < REPORT_PAGE_BORDER))
		page -= 1;
	else if ((high == FALSE) && (position > (handle->display_size.x + REPORT_PAGE_BORDER)))
		page += 1;

	return page;
}


/**
 * Find a page based on a redraw position on the Y axis. Note that at
 * the edges of the display area, this might return pages outside of the
 * active range.
 *
 * \param *handle		The block to query.
 * \param ypos			The Y axis coordinate to look up.
 * \param high			TRUE if we're matching the high coordinate.
 * \return			The page number in the Y direction.
 */

int report_page_find_from_ypos(struct report_page_block *handle, int ypos, osbool high)
{
	int page, position;

	/* Returning 0 for low and -1 for high ensures that the redraw
	 * loop is not entered if the exit route is consistantly called
	 * for both ends of the range.
	 */

	if (handle == NULL || handle->pages == NULL || handle->paginated == FALSE)
		return (high) ? -1 : 0;

	page = -ypos / (handle->display_size.y + (2 * REPORT_PAGE_BORDER));

	position = -ypos % (handle->display_size.y + (2 * REPORT_PAGE_BORDER));

	if ((high == TRUE) && (position < REPORT_PAGE_BORDER))
		page -= 1;
	else if ((high == FALSE) && (position > (handle->display_size.y + REPORT_PAGE_BORDER)))
		page += 1;

	return page;
}


/**
 * Based on an X and Y page position, identify the redraw area in an
 * on-screen representation and return the page index number of the
 * associated page.
 *
 * \param *handle		The page data block holding the page.
 * \param x			The X position of the page, in pages.
 * \param y			The Y position of the page, in pages.
 * \param *area			Pointer to an OS Box to take the returned
 *				outline, in OS Units.
 * \return			The page index number, or REPORT_PAGE_NONE.
 */

unsigned report_page_get_outline(struct report_page_block *handle, int x, int y, os_box *area)
{
	unsigned page;

	if (handle == NULL || area == NULL || handle->paginated == FALSE)
		return REPORT_PAGE_NONE;

	/* Does the required page fall within the current layout. */

	if (x < 0 || x >= handle->page_layout.x || y < 0 || y >= handle->page_layout.y)
		return REPORT_PAGE_NONE;

	/* Search the page list for the page in question. */

	page = 0;

	while (page < handle->page_count && (handle->pages[page].position.x != x || handle->pages[page].position.y != y))
		page++;

	if (page >= handle->page_count)
		return REPORT_PAGE_NONE;

	/* Calculate the on-screen area of the page. */

	area->x0 = x * (handle->display_size.x + (2 * REPORT_PAGE_BORDER)) + REPORT_PAGE_BORDER;
	area->x1 = area->x0 + handle->display_size.x;
	area->y1 = -(y * (handle->display_size.y + (2 * REPORT_PAGE_BORDER)) + REPORT_PAGE_BORDER);
	area->y0 = area->y1 - handle->display_size.y;

	return page;
}


/**
 * Read the current printer page size, and work out from the configured margins
 * where on the page the printed body, header and footer will go.
 *
 * \param *handle		The page block to update.
 * \param landscape		TRUE to rotate the page to Landscape format; else FALSE.
 * \param target_width		The required width of the page body, in OS Units, or zero.
 * \param header_size		The required height of the header, in OS Units, or zero.
 * \param footer_size		The required height of the footer, in OS Units, or zero.
 * \return			Pointer to an error block on failure, or NULL on success.
 */

os_error *report_page_calculate_areas(struct report_page_block *handle, osbool landscape, int target_width, int header_size, int footer_size)
{
	os_error			*error;
	osbool				margin_fail = FALSE;
	int				page_xsize, page_ysize, page_left, page_right, page_top, page_bottom;
	int				body_width;


	if (handle == NULL)
		return NULL;

	handle->active_areas = REPORT_PAGE_AREA_NONE;

	/* Get the current page and margin dimensions, in millipoints.
	 * All the measurements are taken from the bottom-left corner of
	 * the paper.
	 */

	error = xpdriver_page_size(&page_xsize, &page_ysize, &page_left, &page_bottom, &page_right, &page_top);
	if (error != NULL)
		return error;

	/* Calculate the left margin in millipoints from the left-hand egde. */

	if (config_int_read("PrintMarginLeft") > 0 && config_int_read("PrintMarginLeft") > page_left)
		page_left = config_int_read("PrintMarginLeft");
	else if (config_int_read("PrintMarginLeft") > 0)
		margin_fail = TRUE;

	/* Calcumate the right margin in millipoints from the right-hand edge. */

	page_right = page_xsize - page_right;

	if (config_int_read("PrintMarginRight") > 0 && config_int_read("PrintMarginRight") > page_right)
		page_right = config_int_read("PrintMarginRight");
	else if (config_int_read("PrintMarginRight") > 0)
		margin_fail = TRUE;

	/* Calculate the top margin in millipoints from the top edge. */

	page_top = page_ysize - page_top;

	if (config_int_read("PrintMarginTop") > 0 && config_int_read("PrintMarginTop") > page_top)
		page_top = config_int_read("PrintMarginTop");
	else if (config_int_read("PrintMarginTop") > 0)
		margin_fail = TRUE;

	/* Calculate the bottom margin in millipoints from the bottom edge. */

	if (config_int_read("PrintMarginBottom") > 0 && config_int_read("PrintMarginBottom") > page_bottom)
		page_bottom = config_int_read("PrintMarginBottom");
	else if (config_int_read("PrintMarginBottom") > 0)
		margin_fail = TRUE;

	/* Convert the page sizes into OS Units. */

	handle->page_size.x = page_xsize / REPORT_PAGE_MPOINTS_TO_OS;
	handle->page_size.y = page_ysize / REPORT_PAGE_MPOINTS_TO_OS;

	/* Calculate the page body area, taking into account any
	 * need to rotate into landscape format.
	 */

	handle->active_areas |= REPORT_PAGE_AREA_BODY;

	if (landscape) {
		handle->body.x0 = page_bottom / REPORT_PAGE_MPOINTS_TO_OS;
		handle->body.x1 = (page_ysize - page_top) / REPORT_PAGE_MPOINTS_TO_OS;
		handle->body.y0 = (page_right - page_xsize) / REPORT_PAGE_MPOINTS_TO_OS;
		handle->body.y1 = -page_left / REPORT_PAGE_MPOINTS_TO_OS;

		handle->display_size.x = handle->page_size.y;
		handle->display_size.y = handle->page_size.x;
	} else {
		handle->body.x0 = page_left / REPORT_PAGE_MPOINTS_TO_OS;
		handle->body.x1 = (page_xsize - page_right) / REPORT_PAGE_MPOINTS_TO_OS;
		handle->body.y0 = (page_bottom - page_ysize) / REPORT_PAGE_MPOINTS_TO_OS;
		handle->body.y1 = -page_top / REPORT_PAGE_MPOINTS_TO_OS;

		handle->display_size.x = handle->page_size.x;
		handle->display_size.y = handle->page_size.y;
	}

	if (header_size > 0) {
		handle->header.x0 = handle->body.x0;
		handle->header.x1 = handle->body.x1;
		handle->header.y1 = handle->body.y1;

		handle->header.y0 = handle->header.y1 - header_size;
		handle->body.y1 = handle->header.y0 - (config_int_read("PrintMarginInternal") / REPORT_PAGE_MPOINTS_TO_OS);

		handle->active_areas |= REPORT_PAGE_AREA_HEADER;
	}

	if (footer_size > 0) {
		handle->footer.x0 = handle->body.x0;
		handle->footer.x1 = handle->body.x1;
		handle->footer.y0 = handle->body.y0;

		handle->footer.y1 = handle->footer.y0 + footer_size;
		handle->body.y0 = handle->footer.y1 + (config_int_read("PrintMarginInternal") / REPORT_PAGE_MPOINTS_TO_OS);

		handle->active_areas |= REPORT_PAGE_AREA_FOOTER;
	}

	if (margin_fail)
		error_msgs_report_error("BadPrintMargins");

	/* Work out the print scaling: if we're fitting to page, the pages are
	 * made bigger so that we plot at 1:1 and then scale down via the
	 * printer drivers. Otherwise, the scaling matrix is 1:1.
	 */

	body_width = handle->body.x1 - handle->body.x0;

	if (target_width <= body_width) {
		handle->scale = 1 << 16;
	} else {
		handle->scale = (1 << 16) * body_width / target_width;

		handle->display_size.x = handle->display_size.x * (1 << 16) / handle->scale;
		handle->display_size.y = handle->display_size.y * (1 << 16) / handle->scale;

		if (handle->active_areas & REPORT_PAGE_AREA_BODY)
			report_page_scale_area(&(handle->body), handle->scale);

		if (handle->active_areas & REPORT_PAGE_AREA_HEADER)
			report_page_scale_area(&(handle->header), handle->scale);

		if (handle->active_areas & REPORT_PAGE_AREA_FOOTER)
			report_page_scale_area(&(handle->footer), handle->scale);
	}

	/* Set the transformation matrix up, to handle any rotated printing. */

	if (landscape) {
		handle->print_transform.entries[0][0] = 0;
		handle->print_transform.entries[0][1] = handle->scale;
		handle->print_transform.entries[1][0] = -handle->scale;
		handle->print_transform.entries[1][1] = 0;
	} else {
		handle->print_transform.entries[0][0] = handle->scale;
		handle->print_transform.entries[0][1] = 0;
		handle->print_transform.entries[1][0] = 0;
		handle->print_transform.entries[1][1] = handle->scale;
	}

	return NULL;
}


/**
 * Get details of the areas of a printed page.
 *
 * \param *handle		The page block to interrogate.
 * \param *layout		Pointer to a struct to take the layout details.
 * \return			The areas defined on the page.
 */

enum report_page_area report_page_get_areas(struct report_page_block *handle, struct report_page_layout *layout)
{
	if (handle == NULL || layout == NULL)
		return REPORT_PAGE_AREA_NONE;

	layout->areas = handle->active_areas;

	if (handle->active_areas & REPORT_PAGE_AREA_BODY) {
		layout->body.x0 = handle->body.x0;
		layout->body.y0 = handle->body.y0;
		layout->body.x1 = handle->body.x1;
		layout->body.y1 = handle->body.y1;
	}

	if (handle->active_areas & REPORT_PAGE_AREA_HEADER) {
		layout->header.x0 = handle->header.x0;
		layout->header.y0 = handle->header.y0;
		layout->header.x1 = handle->header.x1;
		layout->header.y1 = handle->header.y1;
	}

	if (handle->active_areas & REPORT_PAGE_AREA_FOOTER) {
		layout->footer.x0 = handle->footer.x0;
		layout->footer.y0 = handle->footer.y0;
		layout->footer.x1 = handle->footer.x1;
		layout->footer.y1 = handle->footer.y1;
	}

	return handle->active_areas;
}


/**
 * Return a volatile pointer to the transformation matrix to use for
 * printing a given page.
 *
 * \param *handle		The report page instance to query.
 * \return			Pointer to the matrix, or NULL.
 */

os_hom_trfm *report_page_get_transform(struct report_page_block *handle)
{
	if (handle == NULL)
		return NULL;

	return &(handle->print_transform);
}


/**
 * Rotate and scale a region outline to convert it into a page origin.
 *
 * \param *handle		The report page to use for scaling.
 * \param *region		The region to be calculated.
 * \param landscape		TRUE if the output is landscape format; FALSE for portrait.
 * \param *position		Pointer to struct to take the result.
 * \return			TRUE if valid data was returned; otherwise FALSE.
 */

osbool report_page_calculate_position(struct report_page_block *handle, os_box *region, osbool landscape, os_coord *position)
{
	if (handle == NULL)
		return FALSE;

	if (landscape) {
		position->x = ((-region->y0) * handle->scale / (1 << 16)) * REPORT_PAGE_MPOINTS_TO_OS;
		position->y = (region->x0 * handle->scale / (1 << 16)) * REPORT_PAGE_MPOINTS_TO_OS;
	} else {
		position->x = (region->x0 * handle->scale / (1 << 16)) * REPORT_PAGE_MPOINTS_TO_OS;
		position->y = ((handle->display_size.y + region->y0) * handle->scale / (1 << 16)) * REPORT_PAGE_MPOINTS_TO_OS;
	}

	return TRUE;
}


/**
 * Scale the values in an OS Box area to a given transformation.
 *
 * \param *area			Pointer to the area to be scaled.
 * \param scale			The scaling factor to be applied.
 */

static void report_page_scale_area(os_box *area, int scale)
{
	if (area == NULL)
		return;

	area->x0 = area->x0 * (1 << 16) / scale;
	area->y0 = area->y0 * (1 << 16) / scale;
	area->x1 = area->x1 * (1 << 16) / scale;
	area->y1 = area->y1 * (1 << 16) / scale;
}

