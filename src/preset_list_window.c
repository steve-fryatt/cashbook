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
 * \file: preset_list_window.c
 *
 * Transaction Preset List Window implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

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
#include "presets.h"

#include "account.h"
#include "account_menu.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "preset_dialogue.h"
#include "print_dialogue.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
#include "window.h"

/* Preset List Window icons. */

#define PRESET_ICON_KEY 0
#define PRESET_ICON_NAME 1
#define PRESET_ICON_FROM 2
#define PRESET_ICON_FROM_REC 3
#define PRESET_ICON_FROM_NAME 4
#define PRESET_ICON_TO 5
#define PRESET_ICON_TO_REC 6
#define PRESET_ICON_TO_NAME 7
#define PRESET_ICON_AMOUNT 8
#define PRESET_ICON_DESCRIPTION 9

/* Preset List Toolbar icons. */

#define PRESET_PANE_KEY 0
#define PRESET_PANE_NAME 1
#define PRESET_PANE_FROM 2
#define PRESET_PANE_TO 3
#define PRESET_PANE_AMOUNT 4
#define PRESET_PANE_DESCRIPTION 5

#define PRESET_PANE_PARENT 6
#define PRESET_PANE_ADDPRESET 7
#define PRESET_PANE_PRINT 8
#define PRESET_PANE_SORT 9

#define PRESET_PANE_SORT_DIR_ICON 10

/* Preset List Menu entries. */

#define PRESET_MENU_SORT 0
#define PRESET_MENU_EDIT 1
#define PRESET_MENU_NEWPRESET 2
#define PRESET_MENU_EXPCSV 3
#define PRESET_MENU_EXPTSV 4
#define PRESET_MENU_PRINT 5

/**
 * The minimum number of entries in the Preset List Window.
 */

#define MIN_PRESET_ENTRIES 10

/**
 * The height of the Preset List Window toolbar, in OS Units.
 */

#define PRESET_TOOLBAR_HEIGHT 132

/**
 * The number of draggable columns in the Preset List Window.
 */

#define PRESET_COLUMNS 10

/* The Preset List Window column map. */

static struct column_map preset_columns[PRESET_COLUMNS] = {
	{PRESET_ICON_KEY, PRESET_PANE_KEY, wimp_ICON_WINDOW, SORT_CHAR},
	{PRESET_ICON_NAME, PRESET_PANE_NAME, wimp_ICON_WINDOW, SORT_NAME},
	{PRESET_ICON_FROM, PRESET_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{PRESET_ICON_FROM_REC, PRESET_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{PRESET_ICON_FROM_NAME, PRESET_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{PRESET_ICON_TO, PRESET_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{PRESET_ICON_TO_REC, PRESET_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{PRESET_ICON_TO_NAME, PRESET_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{PRESET_ICON_AMOUNT, PRESET_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{PRESET_ICON_DESCRIPTION, PRESET_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION}
};

/**
 * Preset List Window line redraw data.
 */

struct preset_list_window_redraw {
	/**
	 * The number of the preset relating to the line.
	 */
	preset_t				preset;
};

/**
 * Preset List Window instance data structure.
 */

struct preset_list_window {
	/**
	 * The presets instance owning the Preset List Window.
	 */
	struct preset_block			*instance;

	/**
	 * Wimp window handle for the main Preset List Window.
	 */
	wimp_w					preset_window;

	/**
	 * Indirected title data for the window.
	 */
	char					window_title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the Preset List Window Toolbar pane.
	 */
	wimp_w					preset_pane;

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
	struct preset_list_window_redraw	*line_data;
};

/**
 * The definition for the Preset List Window.
 */

static wimp_window		*preset_window_def = NULL;

/**
 * The definition for the Preset List Window toolbar pane.
 */

static wimp_window		*preset_pane_def = NULL;

/**
 * The handle of the Preset List Window menu.
 */

static wimp_menu		*preset_window_menu = NULL;

/**
 * The window line associated with the most recent menu opening.
 */

static int			preset_window_menu_line = -1;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block	*preset_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block	*preset_saveas_tsv = NULL;

/* Static Function Prototypes. */


/**
 * Test whether a line number is safe to look up in the redraw data array.
 */

#define preset_list_window_line_valid(windat, line) (((line) >= 0) && ((line) < ((windat)->display_lines)))


/**
 * Initialise the Preset List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void preset_list_window_initialise(osspriteop_area *sprites)
{
	preset_window_def = templates_load_window("Preset");
	preset_window_def->icon_count = 0;

	preset_pane_def = templates_load_window("PresetTB");
	preset_pane_def->sprite_area = sprites;

	preset_window_menu = templates_get_menu("PresetMenu");
	ihelp_add_menu(preset_window_menu, "PresetMenu");

	preset_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", preset_save_csv);
	preset_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", preset_save_tsv);
}

