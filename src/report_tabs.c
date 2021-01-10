/* Copyright 2017-2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: report_tabs.c
 *
 * Handle tab bars for a report.
 */

/* ANSI C Header files. */

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

/* Acorn C Header files. */

/* SFLib Header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* OSLib Header files. */

#include "oslib/colourtrans.h"
#include "oslib/font.h"
#include "oslib/pdriver.h"
#include "oslib/types.h"

/* Application header files. */

#include "report_tabs.h"

#include "flexutils.h"
#include "report_cell.h"
#include "report_draw.h"


/**
 * A tab bar definition.
 */

struct report_tabs_bar {
	struct report_tabs_block	*parent;			/**< The parent tabs instance.						*/

	size_t				stop_allocation;		/**< The number of tab stops allocated for during assembly.		*/
	size_t				stop_count;			/**< The number of tab stops in use.					*/

	/**
	 * The left-hand inset of the bar, to allow for the end rule. This allows
	 * the bar to be pushed in to the page to avoid any left-hand vertical
	 * rule if the formatting includes one. In OS Units.
	 */

	int				inset;

	/**
	 * The tab stop data array.
	 */

	struct report_tabs_stop		*stops;
};


/**
 * A Report Fonts instance data block.
 */

struct report_tabs_block {
	osbool				closed;				/**< If TRUE, the instance is closed to adding new bars or stops.	*/

	/* Storage for the tab bars. */

	size_t				bar_allocation;			/**< The size of the bar reference array.				*/

	struct report_tabs_bar		**bars;				/**< Array holding the bar references.					*/

	/* Storage for the formatting information. */

	struct report_tabs_bar		*line_bar_handle;		/**< The bar that's currently active in a reflow action.		*/

	size_t				line_allocation;

	int				*line_font_width;
	int				*line_text_width;
};

/**
 * The number of tab bars allocated at a time.
 */

#define REPORT_TABS_BAR_BLOCK_SIZE 5

/**
 * The number of tab stops allocated to a bar at a time.
 */

#define REPORT_TABS_STOP_BLOCK_SIZE 10

/**
 * The horizontal space between a tab stop edge and the cell inside it,
 * in OS Units.
 */

#define REPORT_TABS_COLUMN_SPACE 10


/**
 * The space between text columns, in characters.
 */

#define REPORT_TEXT_COLUMN_SPACE 1

/* Static Function Prototypes. */

static struct report_tabs_bar *report_tabs_get_bar(struct report_tabs_block *handle, int bar);
static struct report_tabs_bar *report_tabs_create_bar(struct report_tabs_block *parent);
static void report_tabs_destroy_bar(struct report_tabs_bar *handle);
static size_t report_tabs_close_bar(struct report_tabs_bar *handle);
static void report_tabs_reset_bar_columns(struct report_tabs_bar *handle);
static osbool report_tabs_check_stop(struct report_tabs_bar *handle, int stop);
static void report_tabs_initialise_stop(struct report_tabs_stop *stop);
static void report_tabs_zero_stop(struct report_tabs_stop *stop);
static int report_tabs_calculate_bar_columns(struct report_tabs_bar *handle, osbool grid);
static int report_tabs_get_min_bar_column_width(struct report_tabs_bar *handle);
static int report_tabs_paginate_bar(struct report_tabs_bar *handle, int width);
static struct report_tabs_stop *report_tabs_bar_get_stop(struct report_tabs_bar *handle, int stop);


/**
 * Initialise a Report Tabs block.
 *
 * \return			The block handle, or NULL on failure.
 */

struct report_tabs_block *report_tabs_create(void)
{
	struct report_tabs_block	*new;
	int				i;

	new = heap_alloc(sizeof(struct report_tabs_block));
	if (new == NULL)
		return NULL;

	new->closed = FALSE;

	new->bar_allocation = REPORT_TABS_BAR_BLOCK_SIZE;
	new->bars = NULL;

	new->line_bar_handle = NULL;
	new->line_allocation = 0;
	new->line_font_width = NULL;
	new->line_text_width = NULL;

	if (!flexutils_allocate((void **) &(new->bars), sizeof(struct report_tabs_bar *), new->bar_allocation)) {
		report_tabs_destroy(new);
		return NULL;
	}

	for (i = 0; i < new->bar_allocation; i++)
		new->bars[i] = NULL;

	return new;
}


/**
 * Destroy a Report Tabs instance, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_tabs_destroy(struct report_tabs_block *handle)
{
	int bar;

	if (handle == NULL)
		return;

	/* Destroy any tab bars which exist. */

	for (bar = 0; bar < handle->bar_allocation; bar++)
		report_tabs_destroy_bar(handle->bars[bar]);

	/* Free the tab bar reference and column size arrays. */

	flexutils_free((void **) &(handle->bars));

	flexutils_free((void **) &(handle->line_font_width));
	flexutils_free((void **) &(handle->line_text_width));

	/* Free the instance. */

	heap_free(handle);
}


/**
 * Close a Report Tabs instance, so that its allocation shrinks to
 * occupy only the space used by bars that it contains.
 *
 * \param *handle		The block to be closed.
 */

void report_tabs_close(struct report_tabs_block *handle)
{
	size_t	bars = 0, stops, total_stops = 0;
	int	bar;

	if (handle == NULL || handle->bars == NULL)
		return;

	handle->closed = TRUE;

	handle->line_allocation = 0;

	for (bar = 0; bar < handle->bar_allocation; bar++) {
		if (handle->bars[bar] != NULL) {
			stops = report_tabs_close_bar(handle->bars[bar]);
			if (stops > handle->line_allocation)
				handle->line_allocation = stops;

			total_stops += stops;

			bars = bar + 1;
		}
	}

	if (flexutils_resize((void **) &(handle->bars), sizeof(struct report_tabs_bar *), bars))
		handle->bar_allocation = bars;

#ifdef DEBUG
	debug_printf("Tab Bar data: %d bars, using %dKb", bars, total_stops * sizeof(struct report_tabs_stop) / 1024);
#endif
}


/**
 * Update the flags for a tab stop within a tab bar.
 *
 * \param *handle		The Report Tabs instance holding the bar.
 * \param bar			The bar containing the stop to update.
 * \param stop			The stop to updatre the flags for.
 * \param flags			The new flag settings.
 * \return			TRUE if successful; FALSE on failure.
 */
 
osbool report_tabs_set_stop_flags(struct report_tabs_block *handle, int bar, int stop, enum report_tabs_stop_flags flags)
{
	struct report_tabs_bar	*bar_handle;

	if (handle == NULL)
		return FALSE;

	/* Get the bar block handle. */

	bar_handle = report_tabs_get_bar(handle, bar);
	if (bar_handle == NULL)
		return FALSE;

	/* Ensure that the tab stop exists. */

	if (!report_tabs_check_stop(bar_handle, stop))
		return FALSE;

	/* Update the flags. */

	bar_handle->stops[stop].flags |= flags;

	return TRUE;
}


/**
 * Reset the tab stop columns in a Report Tabs instance.
 *
 * \param *handle		The Report Tabs instance to reset.
 */

void report_tabs_reset_columns(struct report_tabs_block *handle)
{
	int bar;

	if (handle == NULL || handle->bars == NULL)
		return;

	for (bar = 0; bar < handle->bar_allocation; bar++)
		report_tabs_reset_bar_columns(handle->bars[bar]);
}


/**
 * Prepare to update the tab stops for a line of a report.
 *
 * \param *handle		The Report Tabs instance to prepare.
 * \param bar			The tab bar to be updated.
 * \return			TRUE on success; FALSE on failure.
 */

osbool report_tabs_start_line_format(struct report_tabs_block *handle, int bar)
{
	int i;

	if (handle == NULL || handle->bars == NULL)
		return FALSE;

	handle->line_bar_handle = report_tabs_get_bar(handle, bar);

	if (handle->line_bar_handle == NULL)
		return FALSE;

	/* Ensure that memory is allocated for the line widths. */

	if (handle->line_font_width == NULL && !flexutils_allocate((void **) &(handle->line_font_width), sizeof(int), handle->line_allocation))
		return FALSE;

	if (handle->line_text_width == NULL && !flexutils_allocate((void **) &(handle->line_text_width), sizeof(int), handle->line_allocation))
		return FALSE;

	/* Set up the initial data for the line. */

	for (i = 0; i < handle->line_allocation; i++) {
		handle->line_font_width[i] = 0;
		handle->line_text_width[i] = 0;
	}

	return TRUE;
}


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

osbool report_tabs_set_cell_width(struct report_tabs_block *handle, int stop, int font_width, int text_width)
{
	if (handle == NULL || handle->bars == NULL || handle->line_bar_handle == NULL)
		return FALSE;

	if (!report_tabs_check_stop(handle->line_bar_handle, stop))
		return FALSE;

	if (handle->line_font_width != NULL && stop >= 0 && stop < handle->line_allocation)
		handle->line_font_width[stop] = font_width;

	if (handle->line_text_width != NULL && stop >= 0 && stop < handle->line_allocation)
		handle->line_text_width[stop] = text_width;

	return TRUE;
}


/**
 * End the formatting of a line in a Report Tabs instance.
 *
 * \param *handle		The Report Tabs instance being updated.
 * \return			TRUE on success; FALSE on failure.
 */

osbool report_tabs_end_line_format(struct report_tabs_block *handle)
{
	int	stop;

	if (handle == NULL || handle->bars == NULL ||
			handle->line_bar_handle == NULL || handle->line_bar_handle->stops == NULL)
		return FALSE;

	/* If the column is a spill column, the width is carried over from the width of the
	 * preceeding column, minus the inter-column gap.  The previous column is then zeroed.
	 */

	for (stop = 0; stop < handle->line_bar_handle->stop_count; stop++) {
		if (handle->line_font_width[stop] == REPORT_TABS_SPILL_WIDTH) {
			if (stop > 0) {
				handle->line_font_width[stop] = handle->line_font_width[stop - 1] - (2 * REPORT_TABS_COLUMN_SPACE);
				handle->line_font_width[stop - 1] = 0;
			} else {
				handle->line_font_width[stop] = 0;
			}
		}

		if (handle->line_text_width[stop] == REPORT_TABS_SPILL_WIDTH) {
			if (stop > 0) {
				handle->line_text_width[stop] = handle->line_text_width[stop - 1] - (2 * REPORT_TEXT_COLUMN_SPACE);
				handle->line_text_width[stop - 1] = 0;
			} else {
				handle->line_text_width[stop] = 0;
			}
		}
	}

	/* Update the maximum column widths. */

	for (stop = 0; stop < handle->line_bar_handle->stop_count; stop++) {
		if (handle->line_font_width[stop] > handle->line_bar_handle->stops[stop].font_width)
			handle->line_bar_handle->stops[stop].font_width = handle->line_font_width[stop];

		if (handle->line_text_width[stop] > handle->line_bar_handle->stops[stop].text_width)
			handle->line_bar_handle->stops[stop].text_width = handle->line_text_width[stop];
	}

	/* Release the bar at the end of the line. */

	handle->line_bar_handle = NULL;

	return TRUE;
}


/**
 * Calculate the column positions of all the bars in a Report Tabs instance.
 *
 * \param *handle		The instance to recalculate.
 * \param grid			TRUE if a grid is being displayed; else FALSE;
 * \return			The width, in OS Units, of the widest bar
 *				when in font mode.
 */

int report_tabs_calculate_columns(struct report_tabs_block *handle, osbool grid)
{
	int bar, width, max_width = 0;

	if (handle == NULL || handle->bars == NULL)
		return 0;

	for (bar = 0; bar < handle->bar_allocation; bar++) {
		width = report_tabs_calculate_bar_columns(handle->bars[bar], grid);
		if (width > max_width)
			max_width = width;
	}

	return max_width;
}


/**
 * Identify the widest column in a Report Tabs instance.
 *
 * \param *handle		The instance to query.
 * \return			The widest column, in OS Units.
 */

int report_tabs_get_min_column_width(struct report_tabs_block *handle)
{
	int	bar, width, max_width = 0;

	if (handle == NULL || handle->bars == NULL)
		return 0;

	for (bar = 0; bar < handle->bar_allocation; bar++) {
		width = report_tabs_get_min_bar_column_width(handle->bars[bar]);
		if (width == 0)
			return 0;
	
		if (width > max_width)
			max_width = width;
	}

	return max_width;
}


/**
 * Calculate the horizontal pagination of all of the bars in a
 * Report Tabs instance.
 *
 * \param *handle		The instance to repaginate.
 * \param width			The available page width, in OS Units.
 * \return			The number of pages required, or 0 on failure.
 */

int report_tabs_paginate(struct report_tabs_block *handle, int width)
{
	int	bar, pages, max_pages = 0;

	if (handle == NULL || handle->bars == NULL)
		return 0;

	for (bar = 0; bar < handle->bar_allocation; bar++) {
		pages = report_tabs_paginate_bar(handle->bars[bar], width);
		if (pages == 0)
			return 0;
	
		if (pages > max_pages)
			max_pages = pages;
	}

	return max_pages;
}


/**
 * Return details of a tab bar relating to a line under pagination. This
 * includes the first and last tab stops to be visible on the current
 * horizontal page.
 *
 * \param *handle		The instance to query.
 * \param *info			The line info structure to update.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool report_tabs_get_line_info(struct report_tabs_block *handle, struct report_tabs_line_info *info)
{
	struct report_tabs_bar	*bar_handle = NULL;
	int			stop;

	if (info == NULL)
		return FALSE;

	bar_handle = report_tabs_get_bar(handle, info->tab_bar);
	if (bar_handle == NULL || bar_handle->stops == NULL)
		return FALSE;

	info->present = FALSE;
	info->first_stop = 0;
	info->last_stop = bar_handle->stop_count - 1;

	for (stop = 0; stop < bar_handle->stop_count; stop++) {
		if ((info->page == -1) || (bar_handle->stops[stop].page == info->page)) {
			if (!info->present) {
				info->first_stop = stop;
				info->present = TRUE;
			}

			info->last_stop = stop;
		}
	}

	info->line_width = (bar_handle->stops[info->last_stop].font_left - bar_handle->stops[info->first_stop].font_left) +
			bar_handle->stops[info->last_stop].font_width + (2 * bar_handle->inset); 

	info->line_inset = bar_handle->inset - bar_handle->stops[info->first_stop].font_left;

	info->cell_inset = bar_handle->inset;

	return TRUE;
}


/**
 * Reset the plot_rules flags for a tab bar, based on a current line's
 * pagination information. The flags are set according to the stops'
 * _PLOT_RULE_AFTER flags.
 *
 * \param *handle		The instance holding the bar in question.
 * \param *info			The line info structure defining the state.
 * \return			TRUE on success; FALSE on failure.
 */

osbool report_tabs_reset_rules(struct report_tabs_block *handle, struct report_tabs_line_info *info)
{
	struct report_tabs_bar	*bar_handle = NULL;
	int			stop;

	if (info == NULL)
		return FALSE;

	bar_handle = report_tabs_get_bar(handle, info->tab_bar);
	if (bar_handle == NULL || bar_handle->stops == NULL)
		return FALSE;

	for (stop = info->first_stop; stop <= info->last_stop; stop++)
		bar_handle->stops[stop].plot_rule = (bar_handle->stops[stop].flags & REPORT_TABS_STOP_FLAGS_RULE_AFTER) ? TRUE : FALSE;

	return TRUE;
}


/**
 * Plot a set of vertical rules for the stops in a tab bar, based on
 * the plot_rules flags and the current pagination information.
 *
 * \param *handle		The instance holding the bar in question.
 * \param *info			The line info structure defining the state.
 * \param *outline		The outline of the required rules, in OS Units.
 * \param *clip			The current clip window, in OS Units.
 * \return			Pointer to an error block, or NULL on success.
 */

os_error *report_tabs_plot_rules(struct report_tabs_block *handle, struct report_tabs_line_info *info, os_box *outline, os_box *clip)
{
	struct report_tabs_bar	*bar_handle = NULL;
	int			stop, xpos;
	os_error		*error;

	if (info == NULL)
		return NULL;

	bar_handle = report_tabs_get_bar(handle, info->tab_bar);
	if (bar_handle == NULL || bar_handle->stops == NULL)
		return NULL;

	for (stop = info->first_stop; stop <= info->last_stop; stop++) {
		if (!bar_handle->stops[stop].plot_rule)
			continue;

		xpos = outline->x0 + info->line_inset + bar_handle->stops[stop].font_left + bar_handle->stops[stop].font_width + bar_handle->inset;

		if (((xpos - bar_handle->inset) > clip->x1) || ((xpos + bar_handle->inset) < clip->x0))
			continue;

		error = xcolourtrans_set_gcol(os_COLOUR_BLACK, colourtrans_SET_FG_GCOL, os_ACTION_OVERWRITE, NULL, NULL);
		if (error != NULL)
			return error;

		error = report_draw_line(xpos, outline->y0, xpos, outline->y1);
		if (error != NULL)
			return error;
	}

	return NULL;
}


/**
 * Return a transient pointer to a tab bar stop.
 *
 * \param *handle		The Report Tabs instance holding the bar.
 * \param bar			The bar holding the required stop.
 * \param stop			The required stop.
 * \return			Transient pointer to the stop data, or NULL.
 */

struct report_tabs_stop *report_tabs_get_stop(struct report_tabs_block *handle, int bar, int stop)
{
	struct report_tabs_bar	*bar_handle = NULL;

	bar_handle = report_tabs_get_bar(handle, bar);
	if (bar_handle == NULL)
		return NULL;

	return report_tabs_bar_get_stop(bar_handle, stop);
}

/**
 * Return the block handle for a tab bar with a given index.
 *
 * \param *handle		The Report Tabs instance holding the bar.
 * \param bar			The index of the required bar.
 * \return			The bar block handle, or NULL on failure.
 */

static struct report_tabs_bar *report_tabs_get_bar(struct report_tabs_block *handle, int bar)
{
	struct report_tabs_bar	*bar_handle = NULL;
	size_t			extend;
	int			i;

	if (handle == NULL || handle->bars == NULL)
		return NULL;

	/* Bar indexes can't be negative. */

	if (bar < 0)
		return NULL;

	/* If the bar index falls outside the current range, allocate more space. */

	if ((bar >= handle->bar_allocation) && (handle->closed == FALSE)) {
		extend = ((bar / (int) REPORT_TABS_BAR_BLOCK_SIZE) + 1) * REPORT_TABS_BAR_BLOCK_SIZE;

#ifdef DEBUG
		debug_printf("Bar %d is outside existing range; extending to %d", handle->bar_allocation, extend);
#endif

		if (!flexutils_resize((void **) &(handle->bars), sizeof(struct report_tabs_bar *), extend))
			return NULL;

		for (i = handle->bar_allocation; i < extend; i++)
			handle->bars[i] = NULL;

		handle->bar_allocation = extend;
	}

	/* Check that the bar index is now in range. */

	if (bar >= handle->bar_allocation)
		return NULL;

	/* Get the bar handle. If no bar exists yet, create a new one. */

	bar_handle = handle->bars[bar];

	if ((bar_handle == NULL) && (handle->closed == FALSE)) {
		bar_handle = report_tabs_create_bar(handle);
		handle->bars[bar] = bar_handle;
	}

	return bar_handle;
}


/**
 * Initialise a Report Tabs Bar block.
 *
 * \param *parent		The parent Report Tabs instance.
 * \return			The block handle, or NULL on failure.
 */

static struct report_tabs_bar *report_tabs_create_bar(struct report_tabs_block *parent)
{
	struct report_tabs_bar	*new;
	int			i;

	new = heap_alloc(sizeof(struct report_tabs_bar));
	if (new == NULL)
		return NULL;

#ifdef DEBUG
	debug_printf("Claimed bar block");
#endif

	new->parent = parent;

	new->stop_allocation = REPORT_TABS_STOP_BLOCK_SIZE;
	new->stop_count = 0;
	new->stops = NULL;
	new->inset = 0;

	if (!flexutils_allocate((void **) &(new->stops), sizeof(struct report_tabs_stop), new->stop_allocation)) {
		report_tabs_destroy_bar(new);
		return NULL;
	}

	for (i = 0; i < new->stop_allocation; i++)
		report_tabs_initialise_stop(new->stops + i);

#ifdef DEBUG
	debug_printf("Stops block claimed OK, bar handle=0x%x", new);
#endif

	return new;
}


/**
 * Destroy a Report Tabs Bar instance, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

static void report_tabs_destroy_bar(struct report_tabs_bar *handle)
{
	if (handle == NULL)
		return;

	flexutils_free((void **) &(handle->stops));

	heap_free(handle);

#ifdef DEBUG
	debug_printf("Deleted bar");
#endif
}


/**
 * Close a Report Tabs Bar instance, freeing up any unused memory and
 * reducing the stop count to the defined stops.
 *
 * \param *handle		The block handle of the tab bar.
 * \return			The number of tab stops defined.
 */

static size_t report_tabs_close_bar(struct report_tabs_bar *handle)
{

	if (handle == NULL || handle->stops == NULL)
		return 0;

	if (flexutils_resize((void **) &(handle->stops), sizeof(struct report_tabs_stop), handle->stop_count))
		handle->stop_allocation = handle->stop_count;

#ifdef DEBUG
	debug_printf("Tab Stop data: %d stops, using %dKb", handle->stop_count, handle->stop_count * sizeof (struct report_tabs_stop) / 1024);
#endif

	return handle->stop_count;
}


/**
 * Reset the tab stop columns in a Report Tabs Bar instance.
 *
 * \param *handle		The Report Tabs Bar instance to reset.
 */

static void report_tabs_reset_bar_columns(struct report_tabs_bar *handle)
{
	int stop;

	if (handle == NULL || handle->stops == NULL)
		return;

	for (stop = 0; stop < handle->stop_count; stop++)
		report_tabs_zero_stop(handle->stops + stop);
}


/**
 * Check that a tab stop exists in a given bar, and try to allocate memory
 * if it doesn't.
 *
 * \param *handle		The block handle of the tab bar.
 * \param stop			The tab stop to check and initialise.
 * \return			TRUE if the stop exists, else FALSE.
 */

static osbool report_tabs_check_stop(struct report_tabs_bar *handle, int stop)
{
	size_t	extend;
	int	i;

	if (handle == NULL || handle->stops == NULL)
		return FALSE;

	/* Stop indexes can't be negative. */

	if (stop < 0)
		return FALSE;

	/* If the stop already exists, there's nothing to be done. */

	if (stop < handle->stop_count)
		return TRUE;

	if ((handle->parent == NULL) || (handle->parent->closed == TRUE))
		return FALSE;

#ifdef DEBUG
	debug_printf("Stop %d doesn't yet exist.", stop);
#endif

	/* If the stop index falls outside the current range, allocate more space. */

	if (stop >= handle->stop_allocation) {
		extend = ((stop / (int) REPORT_TABS_STOP_BLOCK_SIZE) + 1) * REPORT_TABS_STOP_BLOCK_SIZE;

#ifdef DEBUG
		debug_printf("Stop %d is outside existing range; extending to %d", handle->stop_allocation, extend);
#endif

		if (!flexutils_resize((void **) &(handle->stops), sizeof(struct report_tabs_stop), extend))
			return FALSE;

		for (i = handle->stop_allocation; i < extend; i++)
			report_tabs_initialise_stop(handle->stops + i);

		handle->stop_allocation = extend;
	}

	/* Check that the bar index is now in range. */

	if (stop >= handle->stop_allocation)
		return FALSE;

	/* Update the stop count to reflect the requested stop. */

	handle->stop_count = stop + 1;

#ifdef DEBUG
	debug_printf("Stops extended to include stop %d", stop);
#endif

	return TRUE;
}


/**
 * Initialise the data in a tab stop structure.
 *
 * \param *stop			Pointer to the tab stop to initialise.
 */

static void report_tabs_initialise_stop(struct report_tabs_stop *stop)
{
	if (stop == NULL)
		return;

	stop->flags = REPORT_TABS_STOP_FLAGS_NONE;
	report_tabs_zero_stop(stop);
}


/**
 * Zero the data in a tab stop structure.
 *
 * \param *stop			Pointer to the tab stop to initialise.
 */

static void report_tabs_zero_stop(struct report_tabs_stop *stop)
{
	if (stop == NULL)
		return;

	stop->font_width = 0;
	stop->font_left = 0;
	stop->text_width = 0;
	stop->text_left = 0;
	stop->page = -1;
}


/**
 * Calculate the column positions for a tab bar.
 *
 * \param *handle		The bar to calculate the positions for.
 * \param grid			TRUE if a grid is to be displayed; else FALSE.
 * \return			The width, in OS Units, of the bar in font mode.
 */

static int report_tabs_calculate_bar_columns(struct report_tabs_bar *handle, osbool grid)
{
	int	stop, width = 0;
	osbool	gridlines = FALSE;

	if (handle == NULL || handle->stops == NULL || handle->stop_count == 0)
		return 0;

	handle->inset = 0;

	handle->stops[0].font_left = 0;
	handle->stops[0].text_left = 0;

	for (stop = 1; stop < handle->stop_count; stop++) {
		handle->stops[stop].font_left = handle->stops[stop - 1].font_left + handle->stops[stop - 1].font_width + (2 * REPORT_TABS_COLUMN_SPACE);
		handle->stops[stop].text_left = handle->stops[stop - 1].text_left + handle->stops[stop - 1].text_width + (2 * REPORT_TEXT_COLUMN_SPACE);

		if ((handle->stops[stop].font_width > 0) && ((handle->stops[stop].font_left + handle->stops[stop].font_width) > width))
			width = handle->stops[stop].font_left + handle->stops[stop].font_width;

		if (handle->stops[stop].flags & (REPORT_TABS_STOP_FLAGS_RULE_BEFORE | REPORT_TABS_STOP_FLAGS_RULE_AFTER))
			gridlines = TRUE;
	}

	/* If there's a grid, add in space for the outside edge lines. */

	if ((grid == TRUE) && (gridlines == TRUE))
		handle->inset = REPORT_TABS_COLUMN_SPACE;

	return width + (2 * handle->inset);
}


/**
 * Calculate the widest cell on a report, based on the current column widths.
 *
 * \param *handle		The bar to measure.
 * \return			The maximum width, in OS Units.
 */

static int report_tabs_get_min_bar_column_width(struct report_tabs_bar *handle)
{
	int	stop, column_width, max_width = 0;

	if (handle == NULL || handle->stops == NULL || handle->stop_count == 0)
		return 0;

	for (stop = 0; stop < handle->stop_count; stop++) {
		column_width = handle->stops[stop].font_width + REPORT_TABS_COLUMN_SPACE;

		if (column_width > max_width)
			max_width = column_width;
	}

	return max_width;
}


/**
 * Put the tabs of a tab bar onto pages horizontally, based on the current
 * column widths and a given page width in OS Units.
 *
 * \param *handle		The bar to paginate.
 * \param width			The available width of the page.
 * \return			The number of pages required, or 0 on failure.
 */

static int report_tabs_paginate_bar(struct report_tabs_bar *handle, int width)
{
	int	stop, page, position, column_width;

	if (handle == NULL || handle->stops == NULL || handle->stop_count == 0)
		return 0;

	page = 0;
	position = handle->inset;

	for (stop = 0; stop < handle->stop_count; stop++) {
		column_width = handle->stops[stop].font_width + REPORT_TABS_COLUMN_SPACE;

		if (position + column_width > width) {
			page++;
			position = handle->inset;
		}

		handle->stops[stop].page = page;
		position += column_width + REPORT_TABS_COLUMN_SPACE;
	}

	return page + 1;
}


/**
 * Return a transient pointer to a tab stop.
 *
 * \param *handle		The bar holding the stop.
 * \param stop			The stop to return.
 * \return			Pointer to a transient stop block, or NULL.
 */

static struct report_tabs_stop *report_tabs_bar_get_stop(struct report_tabs_bar *handle, int stop)
{
	if (handle == NULL || handle->stops == NULL || stop < 0 || stop >= handle->stop_count)
		return NULL;

	return handle->stops + stop;
}

