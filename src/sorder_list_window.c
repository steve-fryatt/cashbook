/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: sorder_list_window.c
 *
 * Standing Order List Window implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "sorder.h"

#include "account.h"
#include "account_menu.h"
#include "accview.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "print_dialogue.h"
#include "sorder_dialogue.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
#include "transact.h"
#include "window.h"

/* Standing Order List Window icons. */

#define SORDER_ICON_FROM 0
#define SORDER_ICON_FROM_REC 1
#define SORDER_ICON_FROM_NAME 2
#define SORDER_ICON_TO 3
#define SORDER_ICON_TO_REC 4
#define SORDER_ICON_TO_NAME 5
#define SORDER_ICON_AMOUNT 6
#define SORDER_ICON_DESCRIPTION 7
#define SORDER_ICON_NEXTDATE 8
#define SORDER_ICON_LEFT 9

/* Standing Order List Window Toolbar icons. */

#define SORDER_PANE_FROM 0
#define SORDER_PANE_TO 1
#define SORDER_PANE_AMOUNT 2
#define SORDER_PANE_DESCRIPTION 3
#define SORDER_PANE_NEXTDATE 4
#define SORDER_PANE_LEFT 5

#define SORDER_PANE_PARENT 6
#define SORDER_PANE_ADDSORDER 7
#define SORDER_PANE_PRINT 8
#define SORDER_PANE_SORT 9

#define SORDER_PANE_SORT_DIR_ICON 10

/* Standing Order List Window Menu entries. */

#define SORDER_MENU_SORT 0
#define SORDER_MENU_EDIT 1
#define SORDER_MENU_NEWSORDER 2
#define SORDER_MENU_EXPCSV 3
#define SORDER_MENU_EXPTSV 4
#define SORDER_MENU_PRINT 5
#define SORDER_MENU_FULLREP 6

/**
 * The minimum number of entries in the Standing Order List Window.
 */

#define MIN_SORDER_ENTRIES 10

/**
 * The height of the Standing Order List Window toolbar, in OS Units.
 */

#define SORDER_TOOLBAR_HEIGHT 132

/**
 * The number of draggable columns in the Standing Order List Window.
 */

#define SORDER_COLUMNS 10

/* The Standing Order List Window column map. */

static struct column_map sorder_columns[SORDER_COLUMNS] = {
	{SORDER_ICON_FROM, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_FROM_REC, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_FROM_NAME, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_TO, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_TO_REC, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_TO_NAME, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_AMOUNT, SORDER_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{SORDER_ICON_DESCRIPTION, SORDER_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION},
	{SORDER_ICON_NEXTDATE, SORDER_PANE_NEXTDATE, wimp_ICON_WINDOW, SORT_NEXTDATE},
	{SORDER_ICON_LEFT, SORDER_PANE_LEFT, wimp_ICON_WINDOW, SORT_LEFT}
};

/**
 * Standing Order List Window line redraw data.
 */

struct sorder_list_window_redraw {
	/**
	 * The number of the standing order relating to the line.
	 */
	sorder_t				sorder;
};

/**
 * Standing Order List Window instance data structure.
 */

struct sorder_list_window {
	/**
	 * The standing order instance owning the Standing Order List Window.
	 */
	struct sorder_block			*instance;

	/**
	 * Wimp window handle for the main Standing Order List Window.
	 */
	wimp_w					sorder_window;

	/**
	 * Indirected title data for the window.
	 */
	char					window_title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the Standing Order List Window Toolbar pane.
	 */
	wimp_w					sorder_pane;

	/**
	 * Instance handle for the window's column definitions.
	 */
	struct column_block			*columns;

	/**
	 * Instance handle for the window's sort code.
	 */
	struct sort_block			*sort;

	/**
	 * Indirected text data for the sort sprite icon.
	 */
	char					sort_sprite[COLUMN_SORT_SPRITE_LEN];

	/**
	 * Count of the number of populated display lines in the window.
	 */
	int					display_lines;

	/**
	 * Flex array holding the line data for the window.
	 */
	struct sorder_list_window_redraw	*line_data;
};

/**
 * The definition for the Standing Order List Window.
 */

static wimp_window		*sorder_window_def = NULL;

/**
 * The definition for the Standing Order List Window toolbar pane.
 */

static wimp_window		*sorder_pane_def = NULL;

/**
 * The handle of the Standing Order List Window menu.
 */

static wimp_menu		*sorder_window_menu = NULL;

/**
 * The window line associated with the most recent menu opening.
 */

static int			sorder_window_menu_line = -1;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block	*sorder_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block	*sorder_saveas_tsv = NULL;

/* Static Function Prototypes. */




/**
 * Test whether a line number is safe to look up in the redraw data array.
 */

#define sorder_list_window_line_valid(windat, line) (((line) >= 0) && ((line) < ((windat)->display_lines)))


/**
 * Initialise the Standing Order List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_list_window_initialise(osspriteop_area *sprites)
{
	sorder_window_def = templates_load_window("SOrder");
	sorder_window_def->icon_count = 0;

	sorder_pane_def = templates_load_window("SOrderTB");
	sorder_pane_def->sprite_area = sprites;

	sorder_window_menu = templates_get_menu("SOrderMenu");
	ihelp_add_menu(sorder_window_menu, "SorderMenu");

	sorder_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", sorder_save_csv);
	sorder_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", sorder_save_tsv);
}


