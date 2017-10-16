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
 * \file: report_cell.h
 *
 * Track the cells of a report.
 */

#ifndef CASHBOOK_REPORT_CELL
#define CASHBOOK_REPORT_CELL

/**
 * Flags relating to a cell in a report.
 */

enum report_cell_flags {
	REPORT_CELL_FLAGS_NONE		= 0x0000,		/**< No column flags are set.						*/
	REPORT_CELL_FLAGS_INDENT	= 0x0001,		/**< The cell contents should be indented from the left.		*/
	REPORT_CELL_FLAGS_BOLD		= 0x0002,		/**< The cell contents should be presented in a bold font.		*/
	REPORT_CELL_FLAGS_ITALIC	= 0x0004,		/**< The cell contents should be presented in an italic font.		*/
	REPORT_CELL_FLAGS_UNDERLINE	= 0x0008,		/**< The cell contents should be underlined.				*/
	REPORT_CELL_FLAGS_CENTRE	= 0x0010,		/**< The cell contents should be centred.				*/ 
	REPORT_CELL_FLAGS_RIGHT		= 0x0020,		/**< The cell contents should be right aligned.				*/
	REPORT_CELL_FLAGS_NUMERIC	= 0x0040,		/**< The cell contents should be treated as numeric.			*/ 
	REPORT_CELL_FLAGS_SPILL		= 0x0080,		/**< The cell is used for spill from cells to the left.			*/
	REPORT_CELL_FLAGS_RULE_BEFORE	= 0x0100		/**< The cell should have a vertical rule to its left.			*/
};


/**
 * A cell in a report.
 */

struct report_cell_data {
	enum report_cell_flags	flags;					/**< Flags relating to the report line.				*/
	unsigned		offset;					/**< Offset of the column cell data in the text dump block.	*/
	int			tab_stop;				/**< The tab stop in which the cell is located.			*/
};


/**
 * A Report Cell instance handle.
 */

struct report_cell_block;


/**
 * 'NULL' value for use with the unsigned cell block offsets.
 */

#define REPORT_CELL_NULL 0xffffffff


/**
 * Initialise a report cell data block
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct report_cell_block *report_cell_create(size_t allocation);


/**
 * Destroy a report cell data block, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_cell_destroy(struct report_cell_block *handle);


/**
 * Clear the contents of a report cell data block, so that it will behave
 * as if just created.
 *
 * \param *handle		The block to be cleared.
 */

void report_cell_clear(struct report_cell_block *handle);


/**
 * Close a report cell data block, so that its allocation shrinks to
 * occupy only the space used by data.
 *
 * \param *handle		The block to be closed.
 */

void report_cell_close(struct report_cell_block *handle);


/**
 * Add a cell to a report cell data block.
 *
 * \param *handle		The block to add to.
 * \param offset		The offset of the cell data in the data store.
 * \param tab_stop		The tab stop which applies to the cell.
 * \param flags			The flags associated with the cell.
 * \return			The cell's offset, or REPORT_CELL_NULL on failure.
 */

unsigned report_cell_add(struct report_cell_block *handle, unsigned offset, int tab_stop, enum report_cell_flags flags);


/**
 * Return details about a line held in a report line data block. The data
 * returned is transient, and not guaranteed to remain valid if the flex
 * heap shifts.
 *
 * \param *handle		The block to query.
 * \param line			The cell to query.
 * \return			Pointer to the cell data block, or NULL.
 */

struct report_cell_data *report_cell_get_info(struct report_cell_block *handle, unsigned cell);

#endif

