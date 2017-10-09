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

#include "report_cell.h"

/**
 * A single tab stop definition.
 */

struct report_tabs_stop {
	int			font_width;
	int			font_left;

	int			text_width;
};


/**
 * A tab bar definition.
 */

struct report_tabs_bar {
	size_t			stop_count;

	struct report_tabs_stop	*stops;
};


/**
 * A Report Fonts instance data block.
 */

struct report_tabs_block {
	size_t			bar_count;

	struct report_tabs_bar	*bars;
};

/* Static Function Prototypes. */


/**
 * Initialise a Report Tabs block.
 *
 * \return			The block handle, or NULL on failure.
 */

struct report_tabs_block *report_tabs_create(void)
{
	struct report_tabs_block	*new;

	new = heap_alloc(sizeof(struct report_tabs_block));
	if (new == NULL)
		return NULL;


	return new;
}


/**
 * Destroy a Report Tabs instance, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_tabs_destroy(struct report_tabs_block *handle)
{
	if (handle == NULL)
		return;

	heap_free(handle);
}

