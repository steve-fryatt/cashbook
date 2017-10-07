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
 * \file: report_line.h
 *
 * Track the lines of a report.
 */

#ifndef CASHBOOK_REPORT_LINE
#define CASHBOOK_REPORT_LINE


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
 * \param offset		The offset of the line data in the data store.
 * \param tab_bar		The tab bar which applies to the line.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_line_add(struct report_line_block *handle, unsigned offset, int tab_bar);


/**
 * Return the number of lines held in a report line data block.
 *
 * \param *handle		The block to query.
 * \return			The number of lines in the block, or 0.
 */

size_t report_line_get_count(struct report_line_block *handle);


/**
 * Return details about a line held in a report line data block.
 *
 * \param *handle		The block to query.
 * \param line			The line to query.
 * \param *offset		Pointer to variable to take the line data offset.
 * \param *tab_bar		Pointer to variable to take the line's tab bar.
 * \return			TRUE on success; FALSE on failure.
 */

osbool report_line_get_info(struct report_line_block *handle, unsigned line, unsigned *offset, int *tab_bar);

#endif

