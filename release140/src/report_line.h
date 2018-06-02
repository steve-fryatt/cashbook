/* Copyright 2017-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: report_line.h
 *
 * Track the lines of a report.
 */

#ifndef CASHBOOK_REPORT_LINE
#define CASHBOOK_REPORT_LINE

/**
 * No line
 */

#define REPORT_LINE_NONE ((unsigned) 0xffffffffu)

/**
 * Flags relating to a line in a report.
 */

enum report_line_flags {
	REPORT_LINE_FLAGS_NONE		= 0x00,		/**< No line flags are set.											*/
	REPORT_LINE_FLAGS_RULE_BELOW	= 0x01,		/**< The row should have a horizontal rule plotted below it.							*/
	REPORT_LINE_FLAGS_RULE_ABOVE	= 0x02,		/**< The row should have a horizontal rule plotted above it.							*/
	REPORT_LINE_FLAGS_RULE_LAST	= 0x04,		/**< This is the final row in the current grid.									*/
	REPORT_LINE_FLAGS_KEEP_TOGETHER	= 0x08		/**< The row is part of a keep-together block, the first line of which is to be repeated on page breaks.	*/
};


/**
 * A line in a report.
 */

struct report_line_data {
	enum report_line_flags	flags;					/**< Flags relating to the report line.					*/
	unsigned		first_cell;				/**< Offset of the line's first cell in the cell data block.		*/
	size_t			cell_count;				/**< The number of cells in the line.					*/
	int			tab_bar;				/**< The tab bar which relates to the line.				*/
	int			ypos;					/**< The vertical position of the line in the window, in OS Units.	*/
};


/**
 * A Report Line instance handle.
 */

struct report_line_block;


/**
 * Initialise a report line data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_line_block *report_line_create(size_t allocation);


/**
 * Destroy a report line data block, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_line_destroy(struct report_line_block *handle);


/**
 * Clear the contents of a report line data block, so that it will behave
 * as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_line_clear(struct report_line_block *handle);


/**
 * Close a report line data block, so that its allocation shrinks to
 * occupy only the space used by data.
 *
 * \param *handle		The block to be closed.
 */

void report_line_close(struct report_line_block *handle);


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

osbool report_line_add(struct report_line_block *handle, unsigned first_cell, size_t cell_count, int tab_bar, enum report_line_flags flags);


/**
 * Return the number of lines held in a report line data block.
 *
 * \param *handle		The block to query.
 * \return			The number of lines in the block, or 0.
 */

size_t report_line_get_count(struct report_line_block *handle);


/**
 * Return details about a line held in a report line data block. The data
 * returned is transient, and not guaracteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param line			The line to query.
 * \return			Pointer to the line data block, or NULL.
 */

struct report_line_data *report_line_get_info(struct report_line_block *handle, unsigned line, int *height);


/**
 * Find a line based on a redraw position on the y axis.
 *
 * \param *handle		The block to query.
 * \param ypos			The Y axis coordinate to look up.
 * \return			The line number.
 */

unsigned report_line_find_from_ypos(struct report_line_block *handle, int ypos);

#endif

