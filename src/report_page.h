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
 * \file: report_page.h
 *
 * Track the pages of a report.
 */

#ifndef CASHBOOK_REPORT_PAGE
#define CASHBOOK_REPORT_PAGE

#include "oslib/os.h"
#include "oslib/types.h"

/**
 * No page
 */

#define REPORT_PAGE_NONE ((unsigned) 0xffffffffu)

/**
 * Flags representing the areas of a report page.
 */

enum report_page_area {
	REPORT_PAGE_AREA_NONE   = 0,
	REPORT_PAGE_AREA_BODY   = 1,
	REPORT_PAGE_AREA_HEADER = 2,
	REPORT_PAGE_AREA_FOOTER = 4
};


/**
 * Flags relating to a line in a report.
 */

//enum report_line_flags {
//	REPORT_LINE_FLAGS_NONE		= 0x00,		/**< No line flags are set.											*/
//	REPORT_LINE_FLAGS_RULE_BELOW	= 0x01,		/**< The row should have a horizontal rule plotted below it.							*/
//	REPORT_LINE_FLAGS_RULE_ABOVE	= 0x02,		/**< The row should have a horizontal rule plotted above it.							*/
//	REPORT_LINE_FLAGS_KEEP_TOGETHER	= 0x04		/**< The row is part of a keep-together block, the first line of which is to be repeated on page breaks.	*/
//};


/**
 * A page in a report.
 */

struct report_page_data {
	os_coord		position;				/**< The X,Y position of the page, in terms of pages.			*/

	unsigned		first_region;				/**< Offset of the page's first region in the region data block.	*/
	size_t			region_count;				/**< The number of regions on the page.					*/

//	enum report_line_flags	flags;					/**< Flags relating to the report line.					*/
//	unsigned		first_cell;				/**< Offset of the line's first cell in the cell data block.		*/
//	size_t			cell_count;				/**< The number of cells in the line.					*/
//	int			tab_bar;				/**< The tab bar which relates to the line.				*/
//	int			ypos;					/**< The vertical position of the line in the window, in OS Units.	*/
};


/**
 * A Report Page instance handle.
 */

struct report_page_block;


/**
 * Initialise a report page data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_page_block *report_page_create(size_t allocation);


/**
 * Destroy a report page data block, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_page_destroy(struct report_page_block *handle);


/**
 * Clear the contents of a report page data block, so that it will behave
 * as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_page_clear(struct report_page_block *handle);


/**
 * Close a report page data block, so that its allocation shrinks to
 * occupy only the space used by data.
 *
 * \param *handle		The block to be closed.
 */

void report_page_close(struct report_page_block *handle);


/**
 * Report whether a page data block contains valid pagination data.
 *
 * \param *handle		The block to test.
 * \return			TRUE if the data is valid; FALSE if not.
 */

osbool report_page_paginated(struct report_page_block *handle);


/**
 * Start a new row of pages in the page output.
 *
 * \param *handle		The block to add a row to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_page_new_row(struct report_page_block *handle);


/**
 * Add a page to the page output.
 *
 * \param *handle		The block to add the page to.
 * \param first_region		The offset of the first region on the page.
 * \param region_count		The number of regions on the page.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_page_add(struct report_page_block *handle, unsigned first_region, size_t region_count);


/**
 * Return details about a page held in a report page data block. The data
 * returned is transient, and not guaracteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param line			The page to query.
 * \return			Pointer to the page data block, or NULL.
 */

struct report_page_data *report_page_get_info(struct report_page_block *handle, unsigned page);

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

osbool report_page_get_layout_extent(struct report_page_block *handle, int *x, int *y);


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

int report_page_find_from_xpos(struct report_page_block *handle, int xpos, osbool high);


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

int report_page_find_from_ypos(struct report_page_block *handle, int ypos, osbool high);


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

unsigned report_page_get_outline(struct report_page_block *handle, int x, int y, os_box *area);



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

os_error *report_page_calculate_areas(struct report_page_block *handle, osbool landscape, int target_width, int header_size, int footer_size);


/**
 * Get details of the areas of a printed page.
 *
 * \param *handle		The page block to interrogate.
 * \param *body			Pointer to an OS Box to take the body area.
 * \param *header		Pointer to an OS Box to take the header area.
 * \param *footer		Pointer to an OS Box to take the footer area.
 * \return			The areas defined on the page.
 */

enum report_page_area report_page_get_areas(struct report_page_block *handle, os_box *body, os_box *header, os_box *footer);

#endif

