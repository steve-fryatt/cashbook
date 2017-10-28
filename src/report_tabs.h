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
 * \file: report_tabs.h
 *
 * Handle tab bars for a report.
 */

#ifndef CASHBOOK_REPORT_TABS
#define CASHBOOK_REPORT_TABS

#include <stdlib.h>
#include "oslib/types.h"
#include "report_cell.h"

/**
 * Flags relating to a tab stop in a report tab bar.
 */

enum report_tabs_stop_flags {
	REPORT_TABS_STOP_FLAGS_NONE		= 0x0000,	/**< No tab stop flags are set.						*/
	REPORT_TABS_STOP_FLAGS_RULE_BEFORE	= 0x0001,	/**< The tab stop should have a vertical rule to its left.		*/
	REPORT_TABS_STOP_FLAGS_RULE_AFTER	= 0x0002	/**< The tab stop should have a vertical rule to its right.		*/
};

/**
 * The width of a cell being used to carry overspill from the left.
 */

#define REPORT_TABS_SPILL_WIDTH (-1)

/**
 * A Report Tabs instance handle.
 */

struct report_tabs_block;


/**
 * A single tab stop definition.
 */

struct report_tabs_stop {
	enum report_tabs_stop_flags	flags;

	int				font_width;
	int				font_left;

	int				text_width;
	int				text_left;
};


/**
 * Initialise a Report Tabs block.
 *
 * \return			The block handle, or NULL on failure.
 */

struct report_tabs_block *report_tabs_create(void);


/**
 * Destroy a Report Tabs instance, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_tabs_destroy(struct report_tabs_block *handle);


/**
 * Close a Report Tabs instance, so that its allocation shrinks to
 * occupy only the space used by bars that it contains.
 *
 * \param *handle		The block to be closed.
 */

void report_tabs_close(struct report_tabs_block *handle);


/**
 * Update the flags for a tab stop within a tab bar.
 *
 * \param *handle		The Report Tabs instance holding the bar.
 * \param bar			The bar containing the stop to update.
 * \param stop			The stop to updatre the flags for.
 * \param flags			The new flag settings.
 * \return			TRUE if successful; FALSE on failure.
 */
 
osbool report_tabs_set_stop_flags(struct report_tabs_block *handle, int bar, int stop, enum report_tabs_stop_flags flags);


/**
 * Reset the tab stop columns in a Report Tabs instance.
 *
 * \param *handle		The Report Tabs instance to reset.
 */

void report_tabs_reset_columns(struct report_tabs_block *handle);


/**
 * Prepare to update the tab stops for a line of a report.
 *
 * \param *handle		The Report Tabs instance to prepare.
 * \param bar			The tab bar to be updated.
 * \return			TRUE on success; FALSE on failure.
 */

osbool report_tabs_start_line_format(struct report_tabs_block *handle, int bar);


/**
 * Update the widths of a cell in a line as part of a Report Tabs instance
 * formatting operation.
 *
 * \param *handle		The Report Tabs instance to be updated.
 * \param stop			The tab stop to be updated.
 * \param font_width		The width of the current cell, in OS Units,
 *				or REPORT_TABS_SPILL_WIDTH for spill.
 * \param text_width		The width of the current cell, in characters,
 *				or REPORT_TABS_SPILL_WIDTH for spill.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_tabs_set_cell_width(struct report_tabs_block *handle, int stop, int font_width, int text_width);


/**
 * End the formatting of a line in a Report Tabs instance.
 *
 * \param *handle		The Report Tabs instance being updated.
 * \return			TRUE on success; FALSE on failure.
 */

osbool report_tabs_end_line_format(struct report_tabs_block *handle);


/**
 * Calculate the column positions of all the bars in a Report Tabs instance.
 *
 * \param *handle		The instance to recalculate.
 * \return			The width, in OS Units, of the widest bar
 *				when in font mode.
 */

int report_tabs_calculate_columns(struct report_tabs_block *handle);


/**
 * Return a transient pointer to a tab bar stop.
 *
 * \param *handle		The Report Tabs instance holding the bar.
 * \param bar			The bar holding the required stop.
 * \param stop			The required stop.
 * \return			Transient pointer to the stop data, or NULL.
 */

struct report_tabs_stop *report_tabs_get_stop(struct report_tabs_block *handle, int bar, int stop);

#endif

