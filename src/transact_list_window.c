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
 * The Transaction List Window Sort Dialogue column icons.
 */

static struct sort_dialogue_icon transact_sort_columns[] = {
	{TRANS_SORT_ROW, SORT_ROW},
	{TRANS_SORT_DATE, SORT_DATE},
	{TRANS_SORT_FROM, SORT_FROM},
	{TRANS_SORT_TO, SORT_TO},
	{TRANS_SORT_REFERENCE, SORT_REFERENCE},
	{TRANS_SORT_AMOUNT, SORT_AMOUNT},
	{TRANS_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{0, SORT_NONE}
};

/**
 * The Transaction List Window Sort Dialogue direction icons.
 */

static struct sort_dialogue_icon transact_sort_directions[] = {
	{TRANS_SORT_ASCENDING, SORT_ASCENDING},
	{TRANS_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
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
	osbool					auto_reconcile;	
};

/**
 * The definition for the Transaction List Window.
 */

static wimp_window			*transact_window_def = NULL;

/**
 * The definition for the Transaction List Window toolbar pane.
 */

static wimp_window			*transact_pane_def = NULL;

/**
 * The handle of the Transaction List Window menu.
 */

static wimp_menu			*transact_window_menu = NULL;

/**
 * The Transaction List Window Account submenu handle.
 */

static wimp_menu			*transact_window_menu_account = NULL;

/**
 * The Transaction List Window Transaction submenu handle.
 */

static wimp_menu			*transact_window_menu_transact = NULL;

/**
 * The Transaction List Window Analysis submenu handle.
 */

static wimp_menu			*transact_window_menu_analysis = NULL;

/**
 * The Transaction List Window Toolbar's Account List popup menu handle.
 */

static wimp_menu			*transact_account_list_menu = NULL;

/**
 * The window line associated with the most recent menu opening.
 */

static int				transact_window_menu_line = -1;

/**
 * The Transaction List Window Sort dialogue.
 */

static struct sort_dialogue_block	*transact_sort_dialogue = NULL;

/**
 * The Save File saveas data handle.
 */

static struct saveas_block		*transact_saveas_file = NULL;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block		*transact_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block		*transact_saveas_tsv = NULL;

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

static void transact_list_window_delete(struct transact_list_window *windat);
static void transact_window_open_handler(wimp_open *open);
static void transact_list_window_close_handler(wimp_close *close);


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
	wimp_w sort_window;

	sort_window = templates_create_window("SortTrans");
	ihelp_add_window(sort_window, "SortTrans", NULL);
	transact_sort_dialogue = sort_dialogue_create(sort_window, transact_sort_columns, transact_sort_directions,
			TRANS_SORT_OK, TRANS_SORT_CANCEL, transact_process_sort_window);

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


/**
 * Create a new Transaction List Window instance.
 *
 * \param *parent		The parent transact instance.
 * \return			Pointer to the new instance, or NULL.
 */

struct transact_list_window *transact_list_window_create_instance(struct transact_block *parent)
{
	struct transact_list_window *new;

	new = heap_alloc(sizeof(struct transact_list_window));
	if (new == NULL)
		return NULL;

	new->instance = parent;

	new->transaction_window = NULL;
	new->transaction_pane = NULL;
	new->edit_line = NULL;
	new->columns = NULL;
	new->sort = NULL;

	new->display_lines = 0;
	new->line_data = NULL;

	new->auto_reconcile = FALSE;

	/* Initialise the window columns. */

	new->columns = column_create_instance(TRANSACT_COLUMNS, transact_columns, NULL, TRANSACT_PANE_SORT_DIR_ICON);
	if (new->columns == NULL) {
		transact_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read("LimTransactCols"));
	column_init_window(new->columns, 0, FALSE, config_str_read("TransactCols"));

	/* Initialise the window sort. */

	new->sort = sort_create_instance(SORT_DATE | SORT_ASCENDING, SORT_ROW | SORT_ASCENDING,  &transact_sort_callbacks, new);
	if (new->sort == NULL) {
		transact_delete_instance(new);
		return NULL;
	}

	/* Set the initial lines up */

	if (!flexutils_initialise((void **) &(new->line_data))) {
		sorder_list_window_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a Transaction List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void transact_list_window_delete_instance(struct transact_list_window *windat)
{
	if (windat == NULL)
		return;

	if (windat->line_data != NULL)
		flexutils_free((void **) &(windat->line_data));

	column_delete_instance(windat->columns);
	sort_delete_instance(windat->sort);

	sorder_list_window_delete(windat);

	heap_free(windat);
}


/**
 * Create and open a Transaction List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void transact_list_window_open(struct transact_list_window *windat)
{
	int			height;
	os_error		*error;
	wimp_window_state	parent;
	struct file_block	*file;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Create or re-open the window. */

	if (windat->transact_window != NULL) {
		windows_open(windat->transact_window);
		return;
	}

	#ifdef DEBUG
	debug_printf("\\CCreating transaction window");
	#endif

	/* Set the default values */

	windat->display_lines = (windat->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
			windat->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;

	/* Create the new window data and build the window. */

	*(windat->window_title) = '\0';
	transact_window_def->title_data.indirected_text.text = windat->window_title;

	height =  windat->display_lines;

	window_set_initial_area(transact_window_def, column_get_window_width(windat->columns),
			(height * WINDOW_ROW_HEIGHT) + TRANSACT_TOOLBAR_HEIGHT,
			-1, -1, new_transaction_window_offset * TRANSACTION_WINDOW_OPEN_OFFSET);

	error = xwimp_create_window(transact_window_def, &(windat->transaction_window));
	if (error != NULL) {
		transact_delete_window(file->transacts);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	new_transaction_window_offset++;
	if (new_transaction_window_offset >= TRANSACTION_WINDOW_OFFSET_LIMIT)
		new_transaction_window_offset = 0;

	/* Create the toolbar pane. */

	windows_place_as_toolbar(transact_window_def, transact_pane_def, TRANSACT_TOOLBAR_HEIGHT-4);
	columns_place_heading_icons(windat->columns, transact_pane_def);

	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) windat->sort_sprite;
	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			transact_pane_def->sprite_area;
	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

	transact_adjust_sort_icon_data(file->transacts, &(transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON]));

	error = xwimp_create_window(transact_pane_def, &(windat->transaction_pane));
	if (error != NULL) {
		transact_delete_window(file->transacts);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Construct the edit line. */

	windat->edit_line = edit_create_instance(file, transact_window_def, windat->transaction_window,
			windat->columns, TRANSACT_TOOLBAR_HEIGHT,
			&transact_edit_callbacks, file->transacts);
	if (windat->edit_line == NULL) {
		transact_delete_window(file->transacts);
		error_msgs_report_error("TransactNoMem");
		return;
	}

	edit_add_field(windat->edit_line, EDIT_FIELD_DISPLAY,
			TRANSACT_ICON_ROW, transact_buffer_row, TRANSACT_ROW_FIELD_LEN);
	edit_add_field(windat->edit_line, EDIT_FIELD_DATE,
			TRANSACT_ICON_DATE, transact_buffer_date, DATE_FIELD_LEN);
	edit_add_field(windat->edit_line, EDIT_FIELD_ACCOUNT_IN,
			TRANSACT_ICON_FROM, transact_buffer_from_ident, ACCOUNT_IDENT_LEN,
			TRANSACT_ICON_FROM_REC, transact_buffer_from_rec, REC_FIELD_LEN,
			TRANSACT_ICON_FROM_NAME, transact_buffer_from_name, ACCOUNT_NAME_LEN);
	edit_add_field(windat->edit_line, EDIT_FIELD_ACCOUNT_OUT,
			TRANSACT_ICON_TO, transact_buffer_to_ident, ACCOUNT_IDENT_LEN,
			TRANSACT_ICON_TO_REC, transact_buffer_to_rec, REC_FIELD_LEN,
			TRANSACT_ICON_TO_NAME, transact_buffer_to_name, ACCOUNT_NAME_LEN);
	edit_add_field(windat->edit_line, EDIT_FIELD_TEXT,
			TRANSACT_ICON_REFERENCE, transact_buffer_reference, TRANSACT_REF_FIELD_LEN);
	edit_add_field(windat->edit_line, EDIT_FIELD_CURRENCY,
			TRANSACT_ICON_AMOUNT, transact_buffer_amount, AMOUNT_FIELD_LEN);
	edit_add_field(windat->edit_line, EDIT_FIELD_TEXT,
			TRANSACT_ICON_DESCRIPTION, transact_buffer_description, TRANSACT_DESCRIPT_FIELD_LEN);

	if (!edit_complete(windat->edit_line)) {
		transact_delete_window(file->transacts);
		error_msgs_report_error("TransactNoMem");
		return;
	}

	/* Set the title */

	transact_build_window_title(file);

	/* Update the toolbar */

	transact_update_toolbar(file);

	/* Open the window. */

	windows_open(windat->transaction_window);
	windows_open_nested_as_toolbar(windat->transaction_pane,
			windat->transaction_window,
			TRANSACT_TOOLBAR_HEIGHT-4, FALSE);

	ihelp_add_window(windat->transaction_window , "Transact", transact_decode_window_help);
	ihelp_add_window(windat->transaction_pane , "TransactTB", NULL);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(windat->transaction_window, file->transacts);
	event_add_window_menu(windat->transaction_window, transact_window_menu);
	event_add_window_open_event(windat->transaction_window, transact_window_open_handler);
	event_add_window_close_event(windat->transaction_window, transact_window_close_handler);
	event_add_window_lose_caret_event(windat->transaction_window, transact_window_lose_caret_handler);
	event_add_window_mouse_event(windat->transaction_window, transact_window_click_handler);
	event_add_window_key_event(windat->transaction_window, transact_window_keypress_handler);
	event_add_window_scroll_event(windat->transaction_window, transact_window_scroll_handler);
	event_add_window_redraw_event(windat->transaction_window, transact_window_redraw_handler);
	event_add_window_menu_prepare(windat->transaction_window, transact_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->transaction_window, transact_window_menu_selection_handler);
	event_add_window_menu_warning(windat->transaction_window, transact_window_menu_warning_handler);
	event_add_window_menu_close(windat->transaction_window, transact_window_menu_close_handler);

	event_add_window_user_data(windat->transaction_pane, file->transacts);
	event_add_window_menu(windat->transaction_pane, transact_window_menu);
	event_add_window_mouse_event(windat->transaction_pane, transact_pane_click_handler);
	event_add_window_menu_prepare(windat->transaction_pane, transact_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->transaction_pane, transact_window_menu_selection_handler);
	event_add_window_menu_warning(windat->transaction_pane, transact_window_menu_warning_handler);
	event_add_window_menu_close(windat->transaction_pane, transact_window_menu_close_handler);
	event_add_window_icon_popup(windat->transaction_pane, TRANSACT_PANE_VIEWACCT, transact_account_list_menu, -1, NULL);

	dataxfer_set_drop_target(dataxfer_TYPE_CSV, windat->transaction_window, -1, NULL, transact_load_csv, file);
	dataxfer_set_drop_target(dataxfer_TYPE_CSV, windat->transaction_pane, -1, NULL, transact_load_csv, file);

	/* Put the caret into the first empty line. */

	transact_place_caret(file, windat->trans_count, TRANSACT_FIELD_DATE);
}


/**
 * Close and delete the Transaction List Window associated with the
 * given instance.
 *
 * \param *windat		The window to delete.
 */

static void transact_list_window_delete(struct transact_list_window *windat)
{
	#ifdef DEBUG
	debug_printf ("\\RDeleting standing order window");
	#endif

	if (windat == NULL)
		return;

	/* Delete the window, if it exists. */

	if (windat->edit_line != NULL) {
		edit_delete_instance(windat->edit_line);
		windat->edit_line = NULL;
	}

	if (windat->transaction_window != NULL) {
		ihelp_remove_window(windat->transaction_window);
		event_delete_window(windat->transaction_window);
		wimp_delete_window(windat->transaction_window);
		dataxfer_delete_drop_target(dataxfer_TYPE_CSV, windat->transaction_window, -1);
		windat->transaction_window = NULL;
	}

	if (windat->transaction_pane != NULL) {
		ihelp_remove_window(windat->transaction_pane);
		event_delete_window(windat->transaction_pane);
		dataxfer_delete_drop_target(dataxfer_TYPE_CSV, windat->transaction_pane, -1);
		wimp_delete_window(windat->transaction_pane);
		windat->transaction_pane = NULL;
	}

	/* Close any dialogues which belong to this window. */

	sort_dialogue_close(transact_sort_dialogue, windat);
}


/**
 * Handle Open events on Transaction List windows, to adjust the extent.
 *
 * \param *open			The Wimp Open data block.
 */

static void transact_window_open_handler(wimp_open *open)
{
	struct transact_list_window *windat;

	windat = event_get_window_user_data(open->w);
	if (windat != NULL && windat->file != NULL)
		transact_minimise_window_extent(windat->file);

	wimp_open_window(open);
}


/**
 * Handle Close events on Transaction List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void transact_list_window_close_handler(wimp_close *close)
{
	struct transact_list_window	*windat;
	wimp_pointer			pointer;
	char				buffer[1024], *pathcopy;

	#ifdef DEBUG
	debug_printf("\\RClosing Transaction List window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat == NULL || windat->file == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	/* If Adjust was clicked, find the pathname and open the parent directory. */

	if (pointer.buttons == wimp_CLICK_ADJUST && file_check_for_filepath(windat->file)) {
		pathcopy = strdup(windat->file->filename);
		if (pathcopy != NULL) {
			string_printf(buffer, sizeof(buffer), "%%Filer_OpenDir %s", string_find_pathname(pathcopy));
			xos_cli(buffer);
			free(pathcopy);
		}
	}

	/* If it was NOT an Adjust click with Shift held down, close the file. */

	if (!((osbyte1(osbyte_IN_KEY, 0xfc, 0xff) == 0xff || osbyte1(osbyte_IN_KEY, 0xf9, 0xff) == 0xff) &&
			pointer.buttons == wimp_CLICK_ADJUST))
		delete_file(windat->file);
}


