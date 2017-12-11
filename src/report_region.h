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
 * \file: report_region.h
 *
 * Track the page regions of a report.
 */

#ifndef CASHBOOK_REPORT_REGION
#define CASHBOOK_REPORT_REGION

#include "oslib/os.h"
#include "oslib/types.h"

/**
 * Np region
 */

#define REPORT_REGION_NONE ((unsigned) 0xffffffffu)

/**
 * A region in a page.
 */

struct report_region_data {
	os_box			position;				/**< The position of the region on the page, in OS Units from top left.	*/
//	enum report_line_flags	flags;					/**< Flags relating to the report line.					*/
//	unsigned		first_cell;				/**< Offset of the line's first cell in the cell data block.		*/
//	size_t			cell_count;				/**< The number of cells in the line.					*/
//	int			tab_bar;				/**< The tab bar which relates to the line.				*/
//	int			ypos;					/**< The vertical position of the line in the window, in OS Units.	*/
};


/**
 * A Report Region instance handle.
 */

struct report_region_block;


/**
 * Initialise a report page region data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_region_block *report_region_create(size_t allocation);


/**
 * Destroy a report page region data block, freeing the memory associated
 * with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_region_destroy(struct report_region_block *handle);


/**
 * Clear the contents of a report page region data block, so that it will
 * behave as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_region_clear(struct report_region_block *handle);


/**
 * Close a report page region data block, so that its allocation shrinks
 * to occupy only the space used by data.
 *
 * \param *handle		The block to be closed.
 */

void report_region_close(struct report_region_block *handle);


/**
 * Add a region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_region_add(struct report_region_block *handle);


/**
 * Return the number of lines held in a report line data block.
 *
 * \param *handle		The block to query.
 * \return			The number of lines in the block, or 0.
 */

//size_t report_line_get_count(struct report_line_block *handle);


/**
 * Return details about a line held in a report line data block. The data
 * returned is transient, and not guaracteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param line			The line to query.
 * \return			Pointer to the line data block, or NULL.
 */

//struct report_line_data *report_line_get_info(struct report_line_block *handle, unsigned line);


/**
 * Find a page based on a redraw position on the X axis.
 *
 * \param *handle		The block to query.
 * \param ypos			The X axis coordinate to look up.
 * \return			The line number.
 */

//unsigned report_page_find_from_xpos(struct report_page_block *handle, int xpos);


/**
 * Find a page based on a redraw position on the Y axis.
 *
 * \param *handle		The block to query.
 * \param ypos			The Y axis coordinate to look up.
 * \return			The line number.
 */

//unsigned report_page_find_from_ypos(struct report_page_block *handle, int ypos);


//osbool report_page_get_outline(struct report_page_block *handle, int x, int y, os_box *area);

//osbool report_page_get_layout_extent(struct report_page_block *handle, int *x, int *y);

#endif

