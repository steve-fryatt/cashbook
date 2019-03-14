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
 * OR CONDITIONS OF ANY KIND, either express or implied.a
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: transact_list_window.c
 *
 * Transaction List Window implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

/* OSLib header files */

#include "oslib/dragasprite.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"
#include "oslib/territory.h"

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
#include "transact.h"

#include "account.h"
#include "account_list_menu.h"
#include "account_menu.h"
#include "accview.h"
#include "analysis.h"
#include "analysis_template_menu.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "file_info.h"
#include "filing.h"
#include "find.h"
#include "flexutils.h"
#include "goto.h"
#include "presets.h"
#include "preset_menu.h"
#include "print_dialogue.h"
#include "purge.h"
#include "refdesc_menu.h"
#include "report.h"
#include "sorder.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "transact.h"
#include "window.h"

/* Transaction List Window icons. */

#define TRANSACT_ICON_ROW 0
#define TRANSACT_ICON_DATE 1
#define TRANSACT_ICON_FROM 2
#define TRANSACT_ICON_FROM_REC 3
#define TRANSACT_ICON_FROM_NAME 4
#define TRANSACT_ICON_TO 5
#define TRANSACT_ICON_TO_REC 6
#define TRANSACT_ICON_TO_NAME 7
#define TRANSACT_ICON_REFERENCE 8
#define TRANSACT_ICON_AMOUNT 9
#define TRANSACT_ICON_DESCRIPTION 10

/* Transaction List Window Toolbar icons. */

#define TRANSACT_PANE_ROW 0
#define TRANSACT_PANE_DATE 1
#define TRANSACT_PANE_FROM 2
#define TRANSACT_PANE_TO 3
#define TRANSACT_PANE_REFERENCE 4
#define TRANSACT_PANE_AMOUNT 5
#define TRANSACT_PANE_DESCRIPTION 6

#define TRANSACT_PANE_SORT_DIR_ICON 7

#define TRANSACT_PANE_SAVE 8
#define TRANSACT_PANE_PRINT 9
#define TRANSACT_PANE_ACCOUNTS 10
#define TRANSACT_PANE_VIEWACCT 11
#define TRANSACT_PANE_ADDACCT 12
#define TRANSACT_PANE_IN 13
#define TRANSACT_PANE_OUT 14
#define TRANSACT_PANE_ADDHEAD 15
#define TRANSACT_PANE_SORDER 16
#define TRANSACT_PANE_ADDSORDER 17
#define TRANSACT_PANE_PRESET 18
#define TRANSACT_PANE_ADDPRESET 19
#define TRANSACT_PANE_FIND 20
#define TRANSACT_PANE_GOTO 21
#define TRANSACT_PANE_SORT 22
#define TRANSACT_PANE_RECONCILE 23

/* Transaction List Window Menu entries. */

#define MAIN_MENU_SUB_FILE 0
#define MAIN_MENU_SUB_ACCOUNTS 1
#define MAIN_MENU_SUB_HEADINGS 2
#define MAIN_MENU_SUB_TRANS 3
#define MAIN_MENU_SUB_UTILS 4

#define MAIN_MENU_FILE_INFO 0
#define MAIN_MENU_FILE_SAVE 1
#define MAIN_MENU_FILE_EXPCSV 2
#define MAIN_MENU_FILE_EXPTSV 3
#define MAIN_MENU_FILE_CONTINUE 4
#define MAIN_MENU_FILE_PRINT 5

#define MAIN_MENU_ACCOUNTS_VIEW 0
#define MAIN_MENU_ACCOUNTS_LIST 1
#define MAIN_MENU_ACCOUNTS_NEW 2

#define MAIN_MENU_HEADINGS_LISTIN 0
#define MAIN_MENU_HEADINGS_LISTOUT 1
#define MAIN_MENU_HEADINGS_NEW 2

#define MAIN_MENU_TRANS_FIND 0
#define MAIN_MENU_TRANS_GOTO 1
#define MAIN_MENU_TRANS_SORT 2
#define MAIN_MENU_TRANS_AUTOVIEW 3
#define MAIN_MENU_TRANS_AUTONEW 4
#define MAIN_MENU_TRANS_PRESET 5
#define MAIN_MENU_TRANS_PRESETNEW 6
#define MAIN_MENU_TRANS_RECONCILE 7

#define MAIN_MENU_ANALYSIS_BUDGET 0
#define MAIN_MENU_ANALYSIS_SAVEDREP 1
#define MAIN_MENU_ANALYSIS_MONTHREP 2
#define MAIN_MENU_ANALYSIS_UNREC 3
#define MAIN_MENU_ANALYSIS_CASHFLOW 4
#define MAIN_MENU_ANALYSIS_BALANCE 5
#define MAIN_MENU_ANALYSIS_SOREP 6

/**
 * The minimum number of entries in the Transaction List Window.
 */

#define MIN_TRANSACT_ENTRIES 10

/**
 * The minimum number of blank lines in the Transaction List Window.
 */

#define MIN_TRANSACT_BLANK_LINES 1

/**
 * The height of the Transaction List Window toolbar, in OS Units.
 */
 
#define TRANSACT_TOOLBAR_HEIGHT 132

/**
 * The number of draggable columns in the Transaction List Window.
 */

#define TRANSACT_COLUMNS 11

/**
 * The screen offset at which to open new Transaction List Windows, in OS Units.
 */

#define TRANSACTION_WINDOW_OPEN_OFFSET 48

/**
 * The maximum number of offsets to apply, before wrapping around.
 */

#define TRANSACTION_WINDOW_OFFSET_LIMIT 8

/* The Transaction List Window column map. */

static struct column_map transact_columns[TRANSACT_COLUMNS] = {
	{TRANSACT_ICON_ROW, TRANSACT_PANE_ROW, wimp_ICON_WINDOW, SORT_ROW},
	{TRANSACT_ICON_DATE, TRANSACT_PANE_DATE, wimp_ICON_WINDOW, SORT_DATE},
	{TRANSACT_ICON_FROM, TRANSACT_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{TRANSACT_ICON_FROM_REC, TRANSACT_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{TRANSACT_ICON_FROM_NAME, TRANSACT_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{TRANSACT_ICON_TO, TRANSACT_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{TRANSACT_ICON_TO_REC, TRANSACT_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{TRANSACT_ICON_TO_NAME, TRANSACT_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{TRANSACT_ICON_REFERENCE, TRANSACT_PANE_REFERENCE, wimp_ICON_WINDOW, SORT_REFERENCE},
	{TRANSACT_ICON_AMOUNT, TRANSACT_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{TRANSACT_ICON_DESCRIPTION, TRANSACT_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION}
};

/**
 * Transaction List Window line redraw data.
 */

struct transact_list_window_redraw {
	/**
	 * The number of the transaction relating to the line.
	 */
	trans_t					transact;
};

/**
 * Transaction List Window instance data structure.
 */

struct transact_list_window {
	/**
	 * The Transaction instance owning the Transaction List Window.
	 */
	struct transact_block			*instance;

	/**
	 * Wimp window handle for the main Transaction List Window.
	 */
	wimp_w					transaction_window;

	/**
	 * Indirected title data for the window.
	 */
	char					window_title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the Transaction List Window Toolbar pane.
	 */
	wimp_w					transaction_pane;

	/**
	 * Instance handle for the window's edit line.
	 */
	struct edit_block			*edit_line;

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

	/**
	 * True if reconcile should automatically jump to the next unreconciled entry.
	 */
	osbool			auto_reconcile;	
};

/**
 * The definition for the Transaction List Window.
 */

static wimp_window		*transact_window_def = NULL;

/**
 * The definition for the Transaction List Window toolbar pane.
 */

static wimp_window		*transact_pane_def = NULL;

/**
 * The handle of the Transaction List Window menu.
 */

static wimp_menu		*transact_window_menu = NULL;

/**
 * The Transaction List Window Account submenu handle.
 */

static wimp_menu		*transact_window_menu_account = NULL;

/**
 * The Transaction List Window Transaction submenu handle.
 */

static wimp_menu		*transact_window_menu_transact = NULL;

/**
 * The Transaction List Window Analysis submenu handle.
 */

static wimp_menu		*transact_window_menu_analysis = NULL;

/**
 * The Transaction List Window Toolbar's Account List popup menu handle.
 */

static wimp_menu		*transact_account_list_menu = NULL;

/**
 * The window line associated with the most recent menu opening.
 */

static int			transact_window_menu_line = -1;

/**
 * The Save File saveas data handle.
 */

static struct saveas_block	*transact_saveas_file = NULL;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block	*transact_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block	*transact_saveas_tsv = NULL;

/**
 * Data relating to field dragging.
 */

struct transact_drag_data {
	/**
	 * The Transaction List Window instance currently owning the line drag.
	 */
	struct transact_block	*owner; // \TODO -- Make this transact_list_window

	/**
	 * The line of the window over which the drag started.
	 */
	int			start_line;

	/**
	 * The column of the window over which the drag started.
	 */
	wimp_i			start_column;

	/**
	 * TRUE if the field drag is using a sprite.
	 */
	osbool			dragging_sprite;
};

/**
 * Instance of the window drag data, held statically to survive across Wimp_Poll.
 */

static struct transact_drag_data transact_window_dragging_data;


/* Static Function Prototypes. */





/**
 * Test whether a line number is safe to look up in the redraw data array.
 */

#define transact_list_window_line_valid(windat, line) (((line) >= 0) && ((line) < ((windat)->display_lines)))


/**
 * Initialise the Transaction List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void transact_list_window_initialise(osspriteop_area *sprites)
{
	transact_window_def = templates_load_window("Transact");
	transact_window_def->icon_count = 0;

	transact_pane_def = templates_load_window("TransactTB");
	transact_pane_def->sprite_area = sprites;

	transact_window_menu = templates_get_menu("MainMenu");
	ihelp_add_menu(transact_window_menu, "MainMenu");
	transact_window_menu_account = templates_get_menu("MainAccountsSubmenu");
	transact_window_menu_transact = templates_get_menu("MainTransactionsSubmenu");
	transact_window_menu_analysis = templates_get_menu("MainAnalysisSubmenu");

	transact_saveas_file = saveas_create_dialogue(FALSE, "file_1ca", transact_save_file);
	transact_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", transact_save_csv);
	transact_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", transact_save_tsv);
}

