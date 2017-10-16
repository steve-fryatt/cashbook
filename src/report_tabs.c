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

/**
 * A single tab stop definition.
 */

struct report_tabs_stop {
	enum report_tabs_stop_flags	flags;

	int				font_width;
	int				font_left;

	int				text_width;
};


/**
 * A tab bar definition.
 */

struct report_tabs_bar {
	size_t				stop_allocation;
	size_t				stop_count;

	struct report_tabs_stop	*stops;
};


/**
 * A Report Fonts instance data block.
 */

struct report_tabs_block {
	size_t				bar_allocation;

	struct report_tabs_bar		**bars;
};

/**
 * The number of tab bars allocated at a time.
 */

#define REPORT_TABS_BAR_BLOCK_SIZE 5

/**
 * The number of tab stops allocated to a bar at a time.
 */

#define REPORT_TABS_STOP_BLOCK_SIZE 10

/* Static Function Prototypes. */

static struct report_tabs_bar *report_tabs_get_bar(struct report_tabs_block *handle, int bar);
static struct report_tabs_bar *report_tabs_create_bar(void);
static void report_tabs_destroy_bar(struct report_tabs_bar *handle);
static size_t report_tabs_close_bar(struct report_tabs_bar *handle);
static osbool report_tabs_check_stop(struct report_tabs_bar *handle, int stop);
static void report_tabs_initialise_stop(struct report_tabs_stop *stop);


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

	new->bar_allocation = REPORT_TABS_BAR_BLOCK_SIZE;
	new->bars = NULL;

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

	/* Free the tab bar reference array. */

	flexutils_free((void **) &(handle->bars));

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
	size_t	bars = 0, data_size = 0;
	int	bar;

	if (handle == NULL || handle->bars == NULL)
		return;

	for (bar = 0; bar < handle->bar_allocation; bar++) {
		if (handle->bars[bar] != NULL) {
			data_size += report_tabs_close_bar(handle->bars[bar]);
			bars = bar + 1;
		}
	}

	flexutils_resize((void **) &(handle->bars), sizeof(struct report_tabs_bar *), bars);

	debug_printf("Tab Bar data: %d bars, using %dKb", bars, data_size / 1024);
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

	debug_printf("Setting flags for stop %d in bar %d", stop, bar);

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

	if (bar >= handle->bar_allocation) {
		extend = ((bar / (int) REPORT_TABS_BAR_BLOCK_SIZE) + 1) * REPORT_TABS_BAR_BLOCK_SIZE;

		debug_printf("Bar %d is outside existing range; extending to %d", handle->bar_allocation, extend);

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

	if (bar_handle == NULL) {
		debug_printf("Bar %d doesn't yet exist", bar);
		bar_handle = report_tabs_create_bar();
		handle->bars[bar] = bar_handle;
	}

	return bar_handle;
}


/**
 * Initialise a Report Tabs Bar block.
 *
 * \return			The block handle, or NULL on failure.
 */

static struct report_tabs_bar *report_tabs_create_bar(void)
{
	struct report_tabs_bar	*new;
	int			i;

	new = heap_alloc(sizeof(struct report_tabs_bar));
	if (new == NULL)
		return NULL;

	debug_printf("Claimed bar block");

	new->stop_allocation = REPORT_TABS_STOP_BLOCK_SIZE;
	new->stop_count = 0;
	new->stops = NULL;

	if (!flexutils_allocate((void **) &(new->stops), sizeof(struct report_tabs_stop), new->stop_allocation)) {
		debug_printf("FAILED to claim stops block.");
		report_tabs_destroy_bar(new);
		return NULL;
	}

	for (i = 0; i < new->stop_allocation; i++)
		report_tabs_initialise_stop(new->stops + i);

	debug_printf("Stops block claimed OK, bar handle=0x%x", new);

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

	debug_printf("Deleted bar");
}


/**
 * Close a Report Tabs Bar instance, freeing up any unused memory and
 * reducing the stop count to the defined stops.
 *
 * \param *handle		The block handle of the tab bar.
 * \return			The amount of memory used.
 */

static size_t report_tabs_close_bar(struct report_tabs_bar *handle)
{

	if (handle == NULL || handle->stops == NULL)
		return 0;

	if (flexutils_resize((void **) &(handle->stops), sizeof(struct report_tabs_stop), handle->stop_count))
		handle->stop_allocation = handle->stop_count;

	debug_printf("Tab Stop data: %d stops, using %dKb", handle->stop_count, handle->stop_count * sizeof (struct report_tabs_stop) / 1024);

	return handle->stop_count * sizeof (struct report_tabs_stop);
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

	debug_printf("Stop %d doesn't yet exist.", stop);

	/* If the stop index falls outside the current range, allocate more space. */

	if (stop >= handle->stop_allocation) {
		extend = ((stop / (int) REPORT_TABS_STOP_BLOCK_SIZE) + 1) * REPORT_TABS_STOP_BLOCK_SIZE;

		debug_printf("Stop %d is outside existing range; extending to %d", handle->stop_allocation, extend);

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

	debug_printf("Stops extended to include stop %d", stop);

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
	stop->font_width = 0;
	stop->font_left = 0;
	stop->text_width = 0;
}

