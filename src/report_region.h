/* Copyright 2017-2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * No region
 */

#define REPORT_REGION_NONE ((unsigned) 0xffffffffu)

/**
 * The types of region.
 */

enum report_region_type {
	REPORT_REGION_TYPE_NONE,		/**< No content.				*/
	REPORT_REGION_TYPE_TEXT,		/**< Static text.				*/
	REPORT_REGION_TYPE_PAGE_NUMBER,		/**< A page number.				*/
	REPORT_REGION_TYPE_LINES		/**< A block of lines.				*/
};

struct report_region_text {
	unsigned	content;		/**< Offset to the region text.			*/
};

struct report_region_page_number {
	int		major;			/**< The major page number.			*/
	int		minor;			/**< The minor page number, or -1 for none.	*/
};

struct report_region_lines {
	int		page;			/**< The horizontal page that the region is on.	*/
	int		first;			/**< The first line in the region.		*/
	int		last;			/**< The last line in the region.		*/
};

/**
 * A region in a page.
 */

struct report_region_data {
	os_box						position;	/**< The position of the region on the page, in OS Units from top left.	*/

	enum report_region_type				type;		/**< The type of content that the region contains.			*/

	union {
		struct report_region_text		text;		/**< Data associated with a text region.				*/
		struct report_region_page_number	page_number;	/**< Data associated with a page number region.				*/
		struct report_region_lines		lines;		/**< Data associated with a lines region.				*/
	} data;
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
 * Add a static text region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \param *outline		The outline of the region on the page, in OS Units.
 * \param content		The textdump offset to the region content, or REPORT_TEXTDUMP_NULL.
 * \return			The new region number, or REPORT_REGION_NONE.
 */

unsigned report_region_add_text(struct report_region_block *handle, os_box *outline, unsigned content);


/**
 * Add a page number region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \param *outline		The outline of the region on the page, in OS Units.
 * \param major			The major page number.
 * \param minor			The minor page number, or -1 for none.
 * \return			The new region number, or REPORT_REGION_NONE.
 */

unsigned report_region_add_page_number(struct report_region_block *handle, os_box *outline, int major, int minor);


/**
 * Add a lines region to a report region data block.
 *
 * \param *handle		The block to add to.
 * \param *outline		The outline of the region on the page, in OS Units.
 * \param page			The horizontal page that the region is on.
 * \param first			The first line nummber to display in the region.
 * \param last			The last line number to display in the region.
 * \return			The new region number, or REPORT_REGION_NONE.
 */

unsigned report_region_add_lines(struct report_region_block *handle, os_box *outline, int page, int first, int last);


/**
 * Return details about a region held in a report region data block. The data
 * returned is transient, and not guaracteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param region			The region to query.
 * \return			Pointer to the region data block, or NULL.
 */

struct report_region_data *report_region_get_info(struct report_region_block *handle, unsigned region);

#endif

