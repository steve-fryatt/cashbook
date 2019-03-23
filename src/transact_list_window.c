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
 * The Transaction List Window Sort callbacks.
 */

static struct sort_callback		transact_sort_callbacks;

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
	struct transact_list_window	*owner; // \TODO -- Make this transact_list_window

	/**
	 * The line of the window over which the drag started.
	 */
	int				start_line;

	/**
	 * The column of the window over which the drag started.
	 */
	wimp_i				start_column;

	/**
	 * TRUE if the field drag is using a sprite.
	 */
	osbool				dragging_sprite;
};

/**
 * Instance of the window drag data, held statically to survive across Wimp_Poll.
 */

static struct transact_drag_data transact_window_dragging_data;


/* Static Function Prototypes. */

static void transact_list_window_delete(struct transact_list_window *windat);
static void transact_list_window_open_handler(wimp_open *open);
static void transact_list_window_close_handler(wimp_close *close);
static void transact_list_window_click_handler(wimp_pointer *pointer);
static void transact_list_window_lose_caret_handler(wimp_caret *caret);
static void transact_list_window_pane_click_handler(wimp_pointer *pointer);
static osbool transact_list_window_keypress_handler(wimp_key *key);
static void transact_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void transact_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void transact_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void transact_list_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void transact_list_window_scroll_handler(wimp_scroll *scroll);
static void transact_list_window_redraw_handler(wimp_draw *redraw);
static void transact_list_window_adjust_columns(void *data, wimp_i icon, int width);
static void transact_list_window_adjust_sort_icon(struct transact_list_window *windat);
static void transact_list_window_adjust_sort_icon_data(struct transact_list_window *windat, wimp_icon *icon);



static void transact_list_window_force_redraw(struct transact_list_window *windat, int from, int to, wimp_i column);
static void transact_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static void transact_list_window_open_sort_window(struct transact_list_window *windat, wimp_pointer *ptr);
static osbool transact_list_window_process_sort_window(enum sort_type order, void *data);
static void transact_list_window_open_print_window(struct transact_list_window *windat, wimp_pointer *ptr, osbool restore);
static struct report *transact_list_window_print(struct report *report, void *data, date_t from, date_t to);
static int transact_list_window_sort_compare(enum sort_type type, int index1, int index2, void *data);
static void transact_list_window_sort_swap(int index1, int index2, void *data);
static void transact_list_window_start_direct_save(struct transact_list_window *windat);
static osbool transact_list_window_save_file(char *filename, osbool selection, void *data);
static osbool transact_list_window_save_csv(char *filename, osbool selection, void *data);
static osbool transact_list_window_save_tsv(char *filename, osbool selection, void *data);
static void transact_list_window_export_delimited(struct transact_list_window *windat, char *filename, enum filing_delimit_type format, int filetype);
static osbool transact_list_window_load_csv(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data);

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
			TRANS_SORT_OK, TRANS_SORT_CANCEL, transact_list_window_process_sort_window);

	transact_sort_callbacks.compare = transact_sort_compare;
	transact_sort_callbacks.swap = transact_sort_swap;

	transact_window_def = templates_load_window("Transact");
	transact_window_def->icon_count = 0;

	transact_pane_def = templates_load_window("TransactTB");
	transact_pane_def->sprite_area = sprites;

	transact_window_menu = templates_get_menu("MainMenu");
	ihelp_add_menu(transact_window_menu, "MainMenu");
	transact_window_menu_account = templates_get_menu("MainAccountsSubmenu");
	transact_window_menu_transact = templates_get_menu("MainTransactionsSubmenu");
	transact_window_menu_analysis = templates_get_menu("MainAnalysisSubmenu");

	transact_saveas_file = saveas_create_dialogue(FALSE, "file_1ca", transact_list_window_save_file);
	transact_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", transact_list_window_save_csv);
	transact_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", transact_list_window_save_tsv);
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

	file = transact_get_file(windat->instance);
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
		transact_delete_window(windat);
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

	transact_list_window_adjust_sort_icon_data(windat->instance, &(transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON]));

	error = xwimp_create_window(transact_pane_def, &(windat->transaction_pane));
	if (error != NULL) {
		transact_delete_window(windat->instance);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Construct the edit line. */

	windat->edit_line = edit_create_instance(file, transact_window_def, windat->transaction_window,
			windat->columns, TRANSACT_TOOLBAR_HEIGHT,
			&transact_edit_callbacks, file->transacts);
	if (windat->edit_line == NULL) {
		transact_delete_window(windat);
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
		transact_delete_window(file);
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

	ihelp_add_window(windat->transaction_window , "Transact", transact_list_window_decode_help);
	ihelp_add_window(windat->transaction_pane , "TransactTB", NULL);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(windat->transaction_window, windat);
	event_add_window_menu(windat->transaction_window, transact_window_menu);
	event_add_window_open_event(windat->transaction_window, transact_list_window_open_handler);
	event_add_window_close_event(windat->transaction_window, transact_window_close_handler);
	event_add_window_lose_caret_event(windat->transaction_window, transact_list_window_lose_caret_handler);
	event_add_window_mouse_event(windat->transaction_window, transact_list_window_click_handler);
	event_add_window_key_event(windat->transaction_window, transact_list_window_keypress_handler);
	event_add_window_scroll_event(windat->transaction_window, transact_list_window_scroll_handler);
	event_add_window_redraw_event(windat->transaction_window, transact_list_window_redraw_handler);
	event_add_window_menu_prepare(windat->transaction_window, transact_list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->transaction_window, transact_list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->transaction_window, transact_list_window_menu_warning_handler);
	event_add_window_menu_close(windat->transaction_window, transact_list_window_menu_close_handler);

	event_add_window_user_data(windat->transaction_pane, windat);
	event_add_window_menu(windat->transaction_pane, transact_window_menu);
	event_add_window_mouse_event(windat->transaction_pane, transact_list_window_pane_click_handler);
	event_add_window_menu_prepare(windat->transaction_pane, transact_list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->transaction_pane, transact_list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->transaction_pane, transact_list_window_menu_warning_handler);
	event_add_window_menu_close(windat->transaction_pane, transact_list_window_menu_close_handler);
	event_add_window_icon_popup(windat->transaction_pane, TRANSACT_PANE_VIEWACCT, transact_account_list_menu, -1, NULL);

	dataxfer_set_drop_target(dataxfer_TYPE_CSV, windat->transaction_window, -1, NULL, transact_list_window_load_csv, file);
	dataxfer_set_drop_target(dataxfer_TYPE_CSV, windat->transaction_pane, -1, NULL, transact_list_window_load_csv, file);

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

	dialogue_force_all_closed(NULL, windat);
	sort_dialogue_close(transact_sort_dialogue, windat);
}


/**
 * Handle Open events on Transaction List windows, to adjust the extent.
 *
 * \param *open			The Wimp Open data block.
 */

static void transact_list_window_open_handler(wimp_open *open)
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


/**
 * Process mouse clicks in the Transaction List window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void transact_list_window_click_handler(wimp_pointer *pointer)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	int				line, transaction, xpos;
	wimp_i				column;
	wimp_window_state		window;
	wimp_pointer			ptr;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Force a refresh of the current edit line, if there is one.  We avoid refreshing the icon where the mouse
	 * was clicked.
	 */

	edit_refresh_line_contents(NULL, wimp_ICON_WINDOW, pointer->i);

	if (pointer->buttons == wimp_CLICK_SELECT) {
		if (pointer->i == wimp_ICON_WINDOW) {
			window.w = pointer->w;
			wimp_get_window_state(&window);

			line = window_calculate_click_row(&(pointer->pos), &window, TRANSACT_TOOLBAR_HEIGHT, -1);

			if (line >= 0) {
				transact_place_edit_line(windat, line);

				/* Find the correct point for the caret and insert it. */

				wimp_get_pointer_info(&ptr);
				window.w = ptr.w;
				wimp_get_window_state(&window);

				if (ptr.i == TRANSACT_ICON_DATE || ptr.i == TRANSACT_ICON_FROM || ptr.i == TRANSACT_ICON_TO ||
						ptr.i == TRANSACT_ICON_REFERENCE || ptr.i == TRANSACT_ICON_AMOUNT || ptr.i == TRANSACT_ICON_DESCRIPTION) {
					int xo, yo;

					xo = ptr.pos.x - window.visible.x0 + window.xscroll - 4;
					yo = ptr.pos.y - window.visible.y1 + window.yscroll - 4;
					wimp_set_caret_position(ptr.w, ptr.i, xo, yo, -1, -1);
				} else if (ptr.i == TRANSACT_ICON_FROM_REC || ptr.i == TRANSACT_ICON_FROM_NAME) {
					icons_put_caret_at_end(ptr.w, TRANSACT_ICON_FROM);
				} else if (ptr.i == TRANSACT_ICON_TO_REC || ptr.i == TRANSACT_ICON_TO_NAME) {
					icons_put_caret_at_end(ptr.w, TRANSACT_ICON_TO);
				}
			}
		} else if (pointer->i == TRANSACT_ICON_FROM_REC || pointer->i == TRANSACT_ICON_FROM_NAME) {
			icons_put_caret_at_end(pointer->w, TRANSACT_ICON_FROM);
		} else if (pointer->i == TRANSACT_ICON_TO_REC || pointer->i == TRANSACT_ICON_TO_NAME) {
			icons_put_caret_at_end(pointer->w, TRANSACT_ICON_TO);
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		/* Adjust clicks don't care about icons, as we only need to know which line and column we're in. */

		window.w = pointer->w;
		wimp_get_window_state(&window);

		line = window_calculate_click_row(&(pointer->pos), &window, TRANSACT_TOOLBAR_HEIGHT, -1);

		/* If the line was in range, find the column that the click occurred in by scanning through the column
		 * positions.
		 */

		if (line >= 0) {
			xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;

			column = column_find_icon_from_xpos(windat->columns, xpos);

#ifdef DEBUG
			debug_printf("Adjust transaction window click (line %d, column %d, xpos %d)", line, column, xpos);
#endif

			/* The first options can take place on any line in the window... */

			if (column == TRANSACT_ICON_DATE) {
				/* If the column is Date, open the date menu. */
				preset_menu_open(file, line, pointer);
			} else if (column == TRANSACT_ICON_FROM_NAME) {
				/* If the column is the From name, open the from account menu. */
				account_menu_open(file, ACCOUNT_MENU_FROM, line, pointer);
			} else if (column == TRANSACT_ICON_TO_NAME) {
				/* If the column is the To name, open the to account menu. */
				account_menu_open(file, ACCOUNT_MENU_TO, line, pointer);
			} else if (column == TRANSACT_ICON_REFERENCE) {
				/* If the column is the Reference, open the to reference menu. */
				refdesc_menu_open(file, REFDESC_MENU_REFERENCE, line, pointer);
			} else if (column == TRANSACT_ICON_DESCRIPTION) {
				/* If the column is the Description, open the to description menu. */
				refdesc_menu_open(file, REFDESC_MENU_DESCRIPTION, line, pointer);
			} else if (line < windat->trans_count) {
				/* ...while the rest have to occur over a transaction line. */
				transaction = windat->transactions[line].sort_index;

				if (column == TRANSACT_ICON_FROM_REC) {
					/* If the column is the from reconcile flag, toggle its status. */
					transact_toggle_reconcile_flag(file, transaction, TRANS_REC_FROM);
				} else if (column == TRANSACT_ICON_TO_REC) {
					/* If the column is the to reconcile flag, toggle its status. */
					transact_toggle_reconcile_flag(file, transaction, TRANS_REC_TO);
				}
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT) {
		if (config_opt_read("TransDragDrop")) {
			window.w = pointer->w;
			wimp_get_window_state(&window);

			line = window_calculate_click_row(&(pointer->pos), &window, TRANSACT_TOOLBAR_HEIGHT, -1);
			xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;
			column = column_find_icon_from_xpos(windat->columns, xpos);

			if (line >= 0 && column != wimp_ICON_WINDOW)
				transact_window_start_drag(windat, &window, column, line);
		}
	}
}


/**
 * Process lose caret events for the Transaction List window.
 *
 * \param *caret		The caret event block to handle.
 */

static void transact_list_window_lose_caret_handler(wimp_caret *caret)
{
	struct transact_list_window	*windat;

	windat = event_get_window_user_data(caret->w);
	if (windat == NULL || windat->file == NULL)
		return;

	edit_refresh_line_contents(windat->edit_line, wimp_ICON_WINDOW, wimp_ICON_WINDOW);
}

/**
 * Process mouse clicks in the Transaction List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void transact_list_window_pane_click_handler(wimp_pointer *pointer)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	wimp_window_state		window;
	wimp_icon_state			icon;
	int				ox;
	char				*filename;
	enum sort_type			sort_order;


	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	column_update_heading_icon_click(windat->columns, pointer);

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case TRANSACT_PANE_SAVE:
			if (file_check_for_filepath(file))
				filename = windat->file->filename;
			else
				filename = NULL;

			saveas_initialise_dialogue(transact_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
			saveas_prepare_dialogue(transact_saveas_file);
			saveas_open_dialogue(transact_saveas_file, pointer);
			break;

		case TRANSACT_PANE_PRINT:
			transact_list_window_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_ACCOUNTS:
			account_open_window(file, ACCOUNT_FULL);
			break;

		case TRANSACT_PANE_ADDACCT:
			account_open_edit_window(file, NULL_ACCOUNT, ACCOUNT_FULL, pointer);
			break;

		case TRANSACT_PANE_IN:
			account_open_window(file, ACCOUNT_IN);
			break;

		case TRANSACT_PANE_OUT:
			account_open_window(file, ACCOUNT_OUT);
			break;

		case TRANSACT_PANE_ADDHEAD:
			account_open_edit_window(file, NULL_ACCOUNT, ACCOUNT_IN, pointer);
			break;

		case TRANSACT_PANE_FIND:
			find_open_window(file->find, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_GOTO:
			goto_open_window(file->go_to, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_SORT:
			transact_list_window_open_sort_window(windat, pointer);
			break;

		case TRANSACT_PANE_RECONCILE:
			windat->auto_reconcile = !windat->auto_reconcile;
			break;

		case TRANSACT_PANE_SORDER:
			sorder_open_window(file);
			break;

		case TRANSACT_PANE_ADDSORDER:
			sorder_open_edit_window(file, NULL_SORDER, pointer);
			break;

		case TRANSACT_PANE_PRESET:
			preset_open_window(file);
			break;

		case TRANSACT_PANE_ADDPRESET:
			preset_open_edit_window(file, NULL_PRESET, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case TRANSACT_PANE_SAVE:
			transact_list_window_start_direct_save(windat);
			break;

		case TRANSACT_PANE_PRINT:
			transact_list_window_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_FIND:
			find_open_window(file->find, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_GOTO:
			goto_open_window(file->go_to, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_SORT:
			transact_sort(windat);
			break;

		case TRANSACT_PANE_RECONCILE:
			windat->auto_reconcile = !windat->auto_reconcile;
			break;
		}
	} else if ((pointer->buttons == wimp_CLICK_SELECT * 256 ||
			pointer->buttons == wimp_CLICK_ADJUST * 256) && pointer->i != wimp_ICON_WINDOW) {

		/* Process clicks on the column headings, for sorting the data.  This tests to see if the click was
		 * outside of the column size drag hotspot before proceeding.
		 */

		window.w = pointer->w;
		wimp_get_window_state(&window);

		ox = window.visible.x0 - window.xscroll;

		icon.w = pointer->w;
		icon.i = pointer->i;
		wimp_get_icon_state(&icon);

		if (pointer->pos.x < (ox + icon.icon.extent.x1 - COLUMN_DRAG_HOTSPOT)) {
			sort_order = column_get_sort_type_from_heading(windat->columns, pointer->i);

			if (sort_order != SORT_NONE) {
				sort_order |= (pointer->buttons == wimp_CLICK_SELECT * 256) ? SORT_ASCENDING : SORT_DESCENDING;

				sort_set_order(windat->sort, sort_order);
				transact_list_window_adjust_sort_icon(windat);
				windows_redraw(windat->transaction_pane);
				transact_sort(windat);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimTransactCols"));
		column_start_drag(windat->columns, pointer, windat, windat->transaction_window, transact_list_window_adjust_columns);
	}
}


/**
 * Process keypresses in the Transaction List window.  All hotkeys are handled,
 * then any remaining presses are passed on to the edit line handling code
 * for attention.
 *
 * \param *key		The keypress event block to handle.
 */

static osbool transact_list_window_keypress_handler(wimp_key *key)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	wimp_w				window;
	wimp_pointer			pointer;
	char				*filename;

	windat = event_get_window_user_data(key->w);
	if (windat == NULL || windat->instance == NULL)
		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Keyboard shortcuts */

	if (key->c == 18) /* Ctrl-R */
		account_recalculate_all(file);
	else if (key->c == wimp_KEY_PRINT) {
		wimp_get_pointer_info(&pointer);
		transact_list_window_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_CONTROL + wimp_KEY_F1) {
		wimp_get_pointer_info(&pointer);
		window = file_info_prepare_dialogue(file);
		menus_create_standard_menu((wimp_menu *) window, &pointer);
	} else if (key->c == wimp_KEY_CONTROL + wimp_KEY_F2) {
		delete_file(file);
	} else if (key->c == wimp_KEY_F3) {
		wimp_get_pointer_info(&pointer);

		if (file_check_for_filepath(windat->file))
			filename = windat->file->filename;
		else
			filename = NULL;

		saveas_initialise_dialogue(transact_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
		saveas_prepare_dialogue(transact_saveas_file);
		saveas_open_dialogue(transact_saveas_file, &pointer);
	} else if (key->c == wimp_KEY_CONTROL + wimp_KEY_F3) {
		transact_list_window_start_direct_save(windat);
	} else if (key->c == wimp_KEY_F4) {
		wimp_get_pointer_info(&pointer);
		find_open_window(file->find, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_F5) {
		wimp_get_pointer_info(&pointer);
		goto_open_window(file->go_to, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_F6) {
		wimp_get_pointer_info(&pointer);
		transact_list_window_open_sort_window(windat, &pointer);
	} else if (key->c == wimp_KEY_F9) {
		account_open_window(file, ACCOUNT_FULL);
	} else if (key->c == wimp_KEY_F10) {
		account_open_window(file, ACCOUNT_IN);
	} else if (key->c == wimp_KEY_F11) {
		account_open_window(file, ACCOUNT_OUT);
	} else if (key->c == wimp_KEY_PAGE_UP || key->c == wimp_KEY_PAGE_DOWN) {
		wimp_scroll scroll;

		/* Make up a Wimp_ScrollRequest block and pass it to the scroll request handler. */

		scroll.w = windat->transaction_window;
		wimp_get_window_state((wimp_window_state *) &scroll);

		scroll.xmin = wimp_SCROLL_NONE;
		scroll.ymin = (key->c == wimp_KEY_PAGE_UP) ? wimp_SCROLL_PAGE_UP : wimp_SCROLL_PAGE_DOWN;

		transact_list_window_scroll_handler(&scroll);
	} else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_UP) || key->c == wimp_KEY_HOME) {
		transact_scroll_window_to_end(file, TRANSACT_SCROLL_HOME);
	} else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_DOWN) ||
			(key->c == wimp_KEY_COPY && config_opt_read ("IyonixKeys"))) {
		transact_scroll_window_to_end(file, TRANSACT_SCROLL_END);
	} else {
		/* Pass any keys that are left on to the edit line handler. */
		return edit_process_keypress(windat->edit_line, key);
	}

	return TRUE;
}


/**
 * Process menu prepare events in the Transaction List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void transact_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	int				line;
	wimp_window_state		window;
	char				*filename;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	/* If the menu isn't the standard window menu, it must be the account
	 * open menu which needs special handling.
	 */

	if (menu != transact_window_menu) {
		if (pointer != NULL) {
			transact_account_list_menu = account_list_menu_build(windat->file);
			event_set_menu_block(transact_account_list_menu);
			ihelp_add_menu(transact_account_list_menu, "AccOpenMenu");
		}

		account_list_menu_prepare();
		return;
	}

	/* Otherwsie, this is the standard window menu.
	 */

	if (pointer != NULL) {
		transact_window_menu_line = NULL_TRANSACTION;

		if (w == windat->transaction_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = window_calculate_click_row(&(pointer->pos), &window, TRANSACT_TOOLBAR_HEIGHT, -1);

			if (transact_valid(windat, line))
				transact_window_menu_line = line;
		}

		transact_window_menu_account->entries[MAIN_MENU_ACCOUNTS_VIEW].sub_menu = account_list_menu_build(file);
		transact_window_menu_analysis->entries[MAIN_MENU_ANALYSIS_SAVEDREP].sub_menu = analysis_template_menu_build(file, FALSE);

		/* If the submenus concerned are greyed out, give them a valid submenu pointer so that the arrow shows. */

		if (account_get_count(file) == 0)
			transact_window_menu_account->entries[MAIN_MENU_ACCOUNTS_VIEW].sub_menu = (wimp_menu *) 0x8000; /* \TODO -- Ugh! */
		if (!analysis_template_menu_contains_entries())
			transact_window_menu_analysis->entries[MAIN_MENU_ANALYSIS_SAVEDREP].sub_menu = (wimp_menu *) 0x8000; /* \TODO -- Ugh! */

		if (file_check_for_filepath(file))
			filename = windat->file->filename;
		else
			filename = NULL;

		saveas_initialise_dialogue(transact_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(transact_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(transact_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_tick_entry(transact_window_menu_transact, MAIN_MENU_TRANS_RECONCILE, windat->auto_reconcile);
	menus_shade_entry(transact_window_menu_account, MAIN_MENU_ACCOUNTS_VIEW, account_count_type_in_file(windat->file, ACCOUNT_FULL) == 0);
	menus_shade_entry(transact_window_menu_analysis, MAIN_MENU_ANALYSIS_SAVEDREP, !analysis_template_menu_contains_entries());
	account_list_menu_prepare();
}


/**
 * Process menu selection events in the Transaction List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void transact_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	wimp_pointer			pointer;
	template_t			template;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* If the menu is the account open menu, then it needs special processing...
	 */

	if (menu == transact_account_list_menu) {
		if (selection->items[0] != -1)
			accview_open_window(file, account_list_menu_decode(selection->items[0]));

		return;
	}

	/* ...otherwise, handle it as normal.
	 */

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case MAIN_MENU_SUB_FILE:
		switch (selection->items[1]) {
		case MAIN_MENU_FILE_SAVE:
			transact_list_window_start_direct_save(windat);
			break;

		case MAIN_MENU_FILE_CONTINUE:
			purge_open_window(windat->file->purge, &pointer, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_FILE_PRINT:
			transact_list_window_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
			break;
		}
		break;

	case MAIN_MENU_SUB_ACCOUNTS:
		switch (selection->items[1]) {
		case MAIN_MENU_ACCOUNTS_VIEW:
			if (selection->items[2] != -1)
				accview_open_window(windat->file, account_list_menu_decode(selection->items[2]));
			break;

		case MAIN_MENU_ACCOUNTS_LIST:
			account_open_window(windat->file, ACCOUNT_FULL);
			break;

		case MAIN_MENU_ACCOUNTS_NEW:
			account_open_edit_window(windat->file, NULL_ACCOUNT, ACCOUNT_FULL, &pointer);
			break;
		}
		break;

	case MAIN_MENU_SUB_HEADINGS:
		switch (selection->items[1]) {
		case MAIN_MENU_HEADINGS_LISTIN:
			account_open_window(windat->file, ACCOUNT_IN);
			break;

		case MAIN_MENU_HEADINGS_LISTOUT:
			account_open_window(windat->file, ACCOUNT_OUT);
			break;

		case MAIN_MENU_HEADINGS_NEW:
			account_open_edit_window(windat->file, NULL_ACCOUNT, ACCOUNT_IN, &pointer);
			break;
		}
		break;

	case MAIN_MENU_SUB_TRANS:
		switch (selection->items[1]) {
		case MAIN_MENU_TRANS_FIND:
			find_open_window(windat->file->find, &pointer, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_TRANS_GOTO:
			goto_open_window(windat->file->go_to, &pointer, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_TRANS_SORT:
			transact_list_window_open_sort_window(windat, &pointer);
			break;

		case MAIN_MENU_TRANS_AUTOVIEW:
			sorder_open_window(windat->file);
			break;

		case MAIN_MENU_TRANS_AUTONEW:
			sorder_open_edit_window(windat->file, NULL_SORDER, &pointer);
			break;

		case MAIN_MENU_TRANS_PRESET:
			preset_open_window(windat->file);
			break;

		case MAIN_MENU_TRANS_PRESETNEW:
			preset_open_edit_window(windat->file, NULL_PRESET, &pointer);
			break;

		case MAIN_MENU_TRANS_RECONCILE:
			windat->auto_reconcile = !windat->auto_reconcile;
			icons_set_selected(windat->transaction_pane, TRANSACT_PANE_RECONCILE, windat->auto_reconcile);
			break;
		}
		break;

	case MAIN_MENU_SUB_UTILS:
		switch (selection->items[1]) {
		case MAIN_MENU_ANALYSIS_BUDGET:
			budget_open_window(windat->file->budget, &pointer);
			break;

		case MAIN_MENU_ANALYSIS_SAVEDREP:
			template = analysis_template_menu_decode(selection->items[2]);
			if (template != NULL_TEMPLATE)
				analysis_open_template(windat->file->analysis, &pointer, template, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_MONTHREP:
			analysis_open_window(windat->file->analysis, &pointer, REPORT_TYPE_TRANSACTION, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_UNREC:
			analysis_open_window(windat->file->analysis, &pointer, REPORT_TYPE_UNRECONCILED, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_CASHFLOW:
			analysis_open_window(windat->file->analysis, &pointer, REPORT_TYPE_CASHFLOW, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_BALANCE:
			analysis_open_window(windat->file->analysis, &pointer, REPORT_TYPE_BALANCE, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_SOREP:
			sorder_full_report(windat->file);
			break;
		}
		break;
	}
}


/**
 * Process submenu warning events in the Transaction` List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void transact_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct transact_list_window *windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (menu != transact_window_menu)
		return;

	switch (warning->selection.items[0]) {
	case MAIN_MENU_SUB_FILE:
		switch (warning->selection.items[1]) {
		case MAIN_MENU_FILE_INFO:
			file_info_prepare_dialogue(windat->file);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;

		case MAIN_MENU_FILE_SAVE:
			saveas_prepare_dialogue(transact_saveas_file);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;

		case MAIN_MENU_FILE_EXPCSV:
			saveas_prepare_dialogue(transact_saveas_csv);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;

		case MAIN_MENU_FILE_EXPTSV:
			saveas_prepare_dialogue(transact_saveas_tsv);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		}
		break;
	}
}


/**
 * Process menu close events in the Transaction List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void transact_list_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	if (menu == transact_window_menu) {
		transact_window_menu_line = -1;
		analysis_template_menu_destroy();
	} else if (menu == transact_account_list_menu) {
		account_list_menu_destroy();
		ihelp_remove_menu(transact_account_list_menu);
		transact_account_list_menu = NULL;
	}
}


/**
 * Process scroll events in the Transaction List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void transact_list_window_scroll_handler(wimp_scroll *scroll)
{
	int				line;
	struct transact_list_window	*windat;

	windat = event_get_window_user_data(scroll->w);
	if (windat == NULL || windat->file == NULL)
		return;

	window_process_scroll_event(scroll, TRANSACT_TOOLBAR_HEIGHT);

	/* Extend the window downwards if necessary. */

	if (scroll->ymin == wimp_SCROLL_LINE_DOWN) {
		line = (-scroll->yscroll + (scroll->visible.y1 - scroll->visible.y0) - TRANSACT_TOOLBAR_HEIGHT) / WINDOW_ROW_HEIGHT;

		if (line > windat->display_lines) {
			windat->display_lines = line;
			transact_set_window_extent(windat->file);
		}
	}

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

	wimp_open_window((wimp_open *) scroll);

	/* Try to reduce the window extent, if possible. */

	transact_minimise_window_extent(windat->file);
}


/**
 * Process redraw events in the Transaction List window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void transact_list_window_redraw_handler(wimp_draw *redraw)
{
	struct transact_list_window	*windat;
	int				top, base, y, entry_line;
	tran_t				t;
	wimp_colour			shade_rec_col, icon_fg_col;
	char				icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN]; /* Assumes descript is longest. */
	osbool				shade_rec, more;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL || windat->columns == NULL)
		return;

	more = wimp_redraw_window(redraw);

	shade_rec = config_opt_read("ShadeReconciled");
	shade_rec_col = config_int_read("ShadeReconciledColour");

	/* Set the horizontal positions of the icons. */

	columns_place_table_icons_horizontally(windat->columns, transact_window_def, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);
	entry_line = edit_get_line(windat->edit_line);

	window_set_icon_templates(transact_window_def);

	/* Perform the redraw. */

	while (more) {
		window_plot_background(redraw, TRANSACT_TOOLBAR_HEIGHT, wimp_COLOUR_VERY_LIGHT_GREY, -1, &top, &base);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			t = (y < windat->trans_count) ? windat->transactions[y].sort_index : 0;

			/* Work out the foreground colour for the line, based on whether
			 * the line is to be shaded or not.
			 */

			if (shade_rec && (y < windat->trans_count) &&
					((windat->transactions[t].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
				icon_fg_col = shade_rec_col;
			else
				icon_fg_col = wimp_COLOUR_BLACK;

			/* We don't need to plot the current edit line, as that has real icons in it. */

			if (y == entry_line)
				continue;

			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(windat->columns, transact_window_def,
					WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y), WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= windat->trans_count) {
				columns_plot_empty_table_icons(windat->columns);
				continue;
			}

			/* Row field. */

			window_plot_int_field(TRANSACT_ICON_ROW, transact_get_transaction_number(t), icon_fg_col);

			/* Date field */

			window_plot_date_field(TRANSACT_ICON_DATE, windat->transactions[t].date, icon_fg_col);

			/* From field */

			window_plot_text_field(TRANSACT_ICON_FROM, account_get_ident(windat->file, windat->transactions[t].from), icon_fg_col);
			window_plot_reconciled_field(TRANSACT_ICON_FROM_REC, (windat->transactions[t].flags & TRANS_REC_FROM), icon_fg_col);
			window_plot_text_field(TRANSACT_ICON_FROM_NAME, account_get_name(windat->file, windat->transactions[t].from), icon_fg_col);

			/* To field */

			window_plot_text_field(TRANSACT_ICON_TO, account_get_ident(windat->file, windat->transactions[t].to), icon_fg_col);
			window_plot_reconciled_field(TRANSACT_ICON_TO_REC, (windat->transactions[t].flags & TRANS_REC_TO), icon_fg_col);
			window_plot_text_field(TRANSACT_ICON_TO_NAME, account_get_name(windat->file, windat->transactions[t].to), icon_fg_col);

			/* Reference field */

			window_plot_text_field(TRANSACT_ICON_REFERENCE, windat->transactions[t].reference, icon_fg_col);

			/* Amount field */

			window_plot_currency_field(TRANSACT_ICON_AMOUNT, windat->transactions[t].amount, icon_fg_col);

			/* Description field */

			window_plot_text_field(TRANSACT_ICON_DESCRIPTION, windat->transactions[t].description, icon_fg_col);
		}

		more = wimp_get_rectangle(redraw);
	}
}


/**
 * Callback handler for completing the drag of a column heading.
 *
 * \param *data			The window block for the origin of the drag.
 * \param group			The column group which has been dragged.
 * \param width			The new width for the group.
 */

static void transact_list_window_adjust_columns(void *data, wimp_i target, int width)
{
	struct transact_list_window	*windat = (struct transact_list_window *) data;
	int				new_extent;
	wimp_window_info		window;
	wimp_caret			caret;

	if (windat == NULL || windat->file == NULL || windat->transaction_window == NULL || windat->transaction_pane == NULL)
		return;

	columns_update_dragged(windat->columns, windat->transaction_pane, NULL, target, width);

	new_extent = column_get_window_width(windat->columns);

	transact_list_window_adjust_sort_icon(windat);

	/* Replace the edit line to update the icon positions and redraw the rest of the window. */

	wimp_get_caret_position(&caret);

	edit_place_new_line(windat->edit_line, -1, wimp_COLOUR_BLACK);
	windows_redraw(windat->transaction_window);
	windows_redraw(windat->transaction_pane);


	/* If the caret's position was in the current transaction window, we need to replace it in the same position
	 * now, so that we don't lose input focus.
	 */

	if (windat->transaction_window == caret.w)
		wimp_set_caret_position(caret.w, caret.i, 0, 0, -1, caret.index);

	/* Set the horizontal extent of the window and pane. */

	window.w = windat->transaction_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = windat->transaction_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(windat->file, TRUE);
}


/**
 * Adjust the sort icon in a transaction window, to reflect the current column
 * heading positions.
 *
 * \param *windat		The window to be updated.
 */

static void transact_list_window_adjust_sort_icon(struct transact_list_window *windat)
{
	wimp_icon_state icon;

	if (windat == NULL)
		return;

	icon.w = windat->transaction_pane;
	icon.i = TRANSACT_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	transact_list_window_adjust_sort_icon_data(windat, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current transaction sort settings.
 *
 * \param *windat		The window to be updated.
 * \param *icon			The icon to be updated.
 */

static void transact_list_window_adjust_sort_icon_data(struct transact_list_window *windat, wimp_icon *icon)
{
	enum sort_type	sort_order;

	if (windat == NULL)
		return;

	sort_order = sort_get_order(windat->sort);

	column_update_sort_indicator(windat->columns, icon, transact_pane_def, sort_order);
}


/**
 * Return the name of a transaction window column.
 *
 * \param *windat		The transaction list window instance to query.
 * \param field			The field representing the required column.
 * \param *buffer		Pointer to a buffer to take the name.
 * \param len			The length of the supplied buffer.
 * \return			Pointer to the supplied buffer, or NULL.
 */

char *transact_list_window_get_column_name(struct transact_list_window *windat, enum transact_field field, char *buffer, size_t len)
{
	wimp_i	group, icon = wimp_ICON_WINDOW;

	if (buffer == NULL || len == 0)
		return NULL;

	*buffer = '\0';

	if (file == NULL || file->transacts == NULL)
		return buffer;

	switch (field) {
	case TRANSACT_FIELD_NONE:
		return buffer;
	case TRANSACT_FIELD_ROW:
	case TRANSACT_FIELD_DATE:
		icon = TRANSACT_ICON_DATE;
		break;
	case TRANSACT_FIELD_FROM:
		icon = TRANSACT_ICON_FROM;
		break;
	case TRANSACT_FIELD_TO:
		icon = TRANSACT_ICON_TO;
		break;
	case TRANSACT_FIELD_AMOUNT:
		icon = TRANSACT_ICON_AMOUNT;
		break;
	case TRANSACT_FIELD_REF:
		icon = TRANSACT_ICON_REFERENCE;
		break;
	case TRANSACT_FIELD_DESC:
		icon = TRANSACT_ICON_DESCRIPTION;
		break;
	}

	if (icon == wimp_ICON_WINDOW)
		return buffer;

	group = column_get_group_icon(file->transacts->columns, icon);

	if (icon == wimp_ICON_WINDOW)
		return buffer;

	icons_copy_text(file->transacts->transaction_pane, group, buffer, len);

	return buffer;
}


/**
 * Place the caret in a given line in a transaction window, and scroll
 * the line into view.
 *
 * \param *windat		The transaction list window to operate on.
 * \param line			The line (under the current display sort order)
 *				to place the caret in.
 * \param field			The field to place the caret in.
 */

void transact_list_window_place_caret(struct transact_list_window *windat, int line, enum transact_field field)
{
	wimp_i icon = wimp_ICON_WINDOW;

	if (file == NULL || file->transacts == NULL)
		return;

	transact_place_edit_line(file->transacts, line);

	switch (field) {
	case TRANSACT_FIELD_NONE:
		return;
	case TRANSACT_FIELD_ROW:
	case TRANSACT_FIELD_DATE:
		icon = TRANSACT_ICON_DATE;
		break;
	case TRANSACT_FIELD_FROM:
		icon = TRANSACT_ICON_FROM;
		break;
	case TRANSACT_FIELD_TO:
		icon = TRANSACT_ICON_TO;
		break;
	case TRANSACT_FIELD_AMOUNT:
		icon = TRANSACT_ICON_AMOUNT;
		break;
	case TRANSACT_FIELD_REF:
		icon = TRANSACT_ICON_REFERENCE;
		break;
	case TRANSACT_FIELD_DESC:
		icon = TRANSACT_ICON_DESCRIPTION;
		break;
	}

	if (icon == wimp_ICON_WINDOW)
		icon = TRANSACT_ICON_DATE;

	icons_put_caret_at_end(windat->transaction_window, icon);
	transact_find_edit_line_vertically(file->transacts);
}



























/**
 * Set the extent of the transaction window for the specified file.
 *
 * \param *file			The file containing the window to update.
 */

void transact_list_window_set_extent(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL)
		return;

	/* If the window display length is too small, extend it to one blank line after the data. */

	if (file->transacts->display_lines <= (file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES)) {
		file->transacts->display_lines = (file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
				file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;
	}

	window_set_extent(file->transacts->transaction_window, file->transacts->display_lines, TRANSACT_TOOLBAR_HEIGHT,
			column_get_window_width(file->transacts->columns));
}



/**
 * Recreate the title of the given Transaction window.
 *
 * \param *windat		The transaction window to rebuild the title for.
 */

void transact_list_window_build_window_title(struct transact_list_window *windat)
{
	if (file == NULL || file->transacts == NULL)
		return;

	file_get_pathname(file, file->transacts->window_title, WINDOW_TITLE_LENGTH - 2);

	if (file_get_data_integrity(file))
		strcat(file->transacts->window_title, " *");

	if (file->transacts->transaction_window != NULL)
		wimp_force_redraw_title(file->transacts->transaction_window);
}


/**
 * Force the complete redraw of a given Transaction window.
 *
 * \param *windat		The transaction window to redraw.
 */

void transact_list_window_redraw_all(struct transact_list_window *windat)
{
	if (windat == NULL)
		return;

	transact_list_window_force_redraw(windat, 0, windat->trans_count - 1, wimp_ICON_WINDOW);
}


/**
 * Force a redraw of the Transaction window, for the given range of lines.
 *
 * \param *windat		The window to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void transact_list_window_force_redraw(struct transact_list_window *windat, int from, int to, wimp_i column)
{
	int			line;
	wimp_window_info	window;

	if (windat == NULL || windat->transaction_window == NULL)
		return;

	/* If the edit line falls inside the redraw range, refresh it. */

	line = edit_get_line(windat->edit_line);

	if (line >= from && line <= to) {
		edit_refresh_line_contents(windat->edit_line, column, wimp_ICON_WINDOW);
		edit_set_line_colour(windat->edit_line, transact_line_colour(windat, line));
		icons_replace_caret_in_window(windat->transaction_window);
	}

	/* Now force a redraw of the whole window range. */

	window.w = windat->transaction_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(windat->columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(TRANSACT_TOOLBAR_HEIGHT, from);
	window.extent.y0 = WINDOW_ROW_BASE(TRANSACT_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(windat->transaction_window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);
}
/**
 * Turn a mouse position over a Transaction window into an interactive
 * help token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void transact_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int				xpos;
	wimp_i				icon;
	wimp_window_state		window;
	struct transact_list_window	*windat;

	*buffer = '\0';

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	icon = column_find_icon_from_xpos(windat->columns, xpos);
	if (icon == wimp_ICON_WINDOW)
		return;

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, transact_window_def->icons[icon].data.indirected_text.validation, 'N'))
		string_printf(buffer, IHELP_INAME_LEN, "Col%d", icon);
}


/**
 * Find the display line in a transaction window which points to the
 * specified transaction under the applied sort.
 *
 * \param *windat		The transaction list window to search in
 * \param transaction		The transaction to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

int transact_list_window_get_line_from_transaction(struct transact_list_window *windat, tran_t transaction)
{
	int	i;
	int	line = -1;

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return line;

	for (i = 0; i < file->transacts->trans_count; i++) {
		if (file->transacts->transactions[i].sort_index == transaction) {
			line = i;
			break;
		}
	}

	return line;
}


/**
 * Find the transaction which corresponds to a display line in a given
 * transaction window.
 *
 * \param *windat		The transaction list window to search in
 * \param line			The display line to return the transaction for.
 * \return			The appropriate transaction, or NULL_TRANSACTION.
 */

tran_t transact_list_window_get_transaction_from_line(struct transact_list_window *windat, int line)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, line))
		return NULL_TRANSACTION;

	return file->transacts->transactions[line].sort_index;
}


/**
 * Open the Transaction List Sort dialogue for a given transaction list window.
 *
 * \param *windat		The transaction window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void transact_list_window_open_sort_window(struct transact_list_window *windat, wimp_pointer *ptr)
{
	if (windat == NULL || ptr == NULL)
		return;

	sort_dialogue_open(transact_sort_dialogue, ptr, sort_get_order(windat->sort), windat);
}


/**
 * Take the contents of an updated Transaction List Sort window and process
 * the data.
 *
 * \param order			The new sort order selected by the user.
 * \param *data			The transaction window associated with the Sort dialogue.
 * \return			TRUE if successful; else FALSE.
 */

static osbool transact_list_window_process_sort_window(enum sort_type order, void *data)
{
	struct transact_list_window *windat = (struct transact_list_window *) data;

	if (windat == NULL)
		return FALSE;

	sort_set_order(windat->sort, order);

	transact_list_window_adjust_sort_icon(windat);
	windows_redraw(windat->transaction_pane);
	transact_sort(windat);

	return TRUE;
}


/**
 * Open the Transaction Print dialogue for a given account list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void transact_list_window_open_print_window(struct transact_list_window *windat, wimp_pointer *ptr, osbool restore)
{
	if (windat == NULL || windat->file == NULL)
		return;

	print_dialogue_open(windat->file->print, ptr, TRUE, restore, "PrintTransact", "PrintTitleTransact", NULL, transact_list_window_print, windat);
}


/**
 * Send the contents of the Transaction Window to the printer, via the reporting
 * system.
 *
 * \param *report		The report handle to use for output.
 * \param *data			The transaction window structure to be printed.
 * \param from			The date to print from.
 * \param to			The date to print to.
 * \return			Pointer to the report, or NULL on failure.
 */

static struct report *transact_list_window_print(struct report *report, void *data, date_t from, date_t to)
{
	struct transact_list_window	*windat = data;
	int				line, column;
	tran_t				transaction;
	char				rec_char[REC_FIELD_LEN];
	wimp_i				columns[TRANSACT_COLUMNS];

	if (report == NULL || windat == NULL || windat->file == NULL)
		return NULL;

	if (!column_get_icons(windat->columns, columns, TRANSACT_COLUMNS, FALSE))
		return NULL;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	hourglass_on();

	/* Output the page title. */

	stringbuild_reset();

	stringbuild_add_string("\\b\\u");
	stringbuild_add_message_param("TransTitle",
			file_get_leafname(windat->file, NULL, 0),
			NULL, NULL, NULL);

	stringbuild_report_line(report, 1);

	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	stringbuild_reset();
	columns_print_heading_names(windat->columns, windat->transaction_pane);
	stringbuild_report_line(report, 0);

	/* Output the transaction data as a set of delimited lines. */

	for (line = 0; line < windat->trans_count; line++) {
		transaction = windat->transactions[line].sort_index;

		if ((from != NULL_DATE && windat->transactions[transaction].date < from) ||
				(to != NULL_DATE && windat->transactions[transaction].date > to))
			continue;

		stringbuild_reset();

		for (column = 0; column < TRANSACT_COLUMNS; column++) {
			if (column == 0)
				stringbuild_add_string("\\k");
			else
				stringbuild_add_string("\\t");

			switch (columns[column]) {
			case TRANSACT_ICON_ROW:
				stringbuild_add_printf("\\v\\d\\r%d", transact_get_transaction_number(transaction));
				break;
			case TRANSACT_ICON_DATE:
				stringbuild_add_string("\\v\\c");
				stringbuild_add_date(windat->transactions[transaction].date);
				break;
			case TRANSACT_ICON_FROM:
				stringbuild_add_string(account_get_ident(windat->file, windat->transactions[transaction].from));
				break;
			case TRANSACT_ICON_FROM_REC:
				if (windat->transactions[transaction].flags & TRANS_REC_FROM)
					stringbuild_add_string(rec_char);
				break;
			case TRANSACT_ICON_FROM_NAME:
				stringbuild_add_string("\\v");
				stringbuild_add_string(account_get_name(windat->file, windat->transactions[transaction].from));
				break;
			case TRANSACT_ICON_TO:
				stringbuild_add_string(account_get_ident(windat->file, windat->transactions[transaction].to));
				break;
			case TRANSACT_ICON_TO_REC:
				if (windat->transactions[transaction].flags & TRANS_REC_TO)
					stringbuild_add_string(rec_char);
				break;
			case TRANSACT_ICON_TO_NAME:
				stringbuild_add_string("\\v");
				stringbuild_add_string(account_get_name(windat->file, windat->transactions[transaction].to));
				break;
			case TRANSACT_ICON_REFERENCE:
				stringbuild_add_string("\\v");
				stringbuild_add_string(windat->transactions[transaction].reference);
				break;
			case TRANSACT_ICON_AMOUNT:
				stringbuild_add_string("\\v\\d\\r");
				stringbuild_add_currency(windat->transactions[transaction].amount, FALSE);
				break;
			case TRANSACT_ICON_DESCRIPTION:
				stringbuild_add_string("\\v");
				stringbuild_add_string(windat->transactions[transaction].description);
				break;
			default:
				stringbuild_add_string("\\s");
				break;
			}
		}

		stringbuild_report_line(report, 0);
	}

	hourglass_off();

	return report;
}


/**
 * Sort the contents of the transaction list window based on the instance's
 * sort setting.
 *
 * \param *windat		The transaction window instance to sort.
 */

void transact_list_window_sort(struct transact_list_window *windat)
{
	wimp_caret	caret;
	tran_t		edit_transaction;

	if (windat == NULL || windat->file == NULL)
		return;

#ifdef DEBUG
	debug_printf("Sorting transaction window");
#endif

	hourglass_on();

	/* Find the caret position and edit line before sorting. */

	wimp_get_caret_position(&caret);
	edit_transaction = transact_find_edit_line_by_transaction(windat);

	/* Run the sort. */

	sort_process(windat->sort, windat->trans_count);

	/* Replace the edit line where we found it prior to the sort. */

	transact_place_edit_line_by_transaction(windat, edit_transaction);

	/* If the caret's position was in the current transaction window, we need to
	 * replace it in the same position now, so that we don't lose input focus.
	 */

	if (windat->transaction_window != NULL && windat->transaction_window == caret.w)
		wimp_set_caret_position(caret.w, caret.i, 0, 0, -1, caret.index);

	transact_redraw_all(windat->file);

	hourglass_off();
}


/**
 * Compare two lines of a transaction list, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The index of the first line to be compared.
 * \param index2		The index of the second line to be compared.
 * \param *data			Client specific data, which is our window block.
 * \return			The comparison result.
 */

static int transact_list_window_sort_compare(enum sort_type type, int index1, int index2, void *data)
{
	struct transact_list_window *windat = data;

	if (windat == NULL)
		return 0;

	switch (type) {
	case SORT_ROW:
		return (transact_get_transaction_number(windat->transactions[index1].sort_index) -
				transact_get_transaction_number(windat->transactions[index2].sort_index));

	case SORT_DATE:
		return ((windat->transactions[windat->transactions[index1].sort_index].date & DATE_SORT_MASK) -
				(windat->transactions[windat->transactions[index2].sort_index].date & DATE_SORT_MASK));

	case SORT_FROM:
		return strcmp(account_get_name(windat->file, windat->transactions[windat->transactions[index1].sort_index].from),
				account_get_name(windat->file, windat->transactions[windat->transactions[index2].sort_index].from));

	case SORT_TO:
		return strcmp(account_get_name(windat->file, windat->transactions[windat->transactions[index1].sort_index].to),
				account_get_name(windat->file, windat->transactions[windat->transactions[index2].sort_index].to));

	case SORT_REFERENCE:
		return strcmp(windat->transactions[windat->transactions[index1].sort_index].reference,
				windat->transactions[windat->transactions[index2].sort_index].reference);

	case SORT_AMOUNT:
		return (windat->transactions[windat->transactions[index1].sort_index].amount -
				windat->transactions[windat->transactions[index2].sort_index].amount);

	case SORT_DESCRIPTION:
		return strcmp(windat->transactions[windat->transactions[index1].sort_index].description,
				windat->transactions[windat->transactions[index2].sort_index].description);

	default:
		return 0;
	}
}


/**
 * Swap the sort index of two lines of a transaction list.
 *
 * \param index1		The index of the first line to be swapped.
 * \param index2		The index of the second line to be swapped.
 * \param *data			Client specific data, which is our window block.
 */

static void transact_list_window_sort_swap(int index1, int index2, void *data)
{
	struct transact_list_window	*windat = data;
	int				temp;

	if (windat == NULL)
		return;

	temp = windat->transactions[index1].sort_index;
	windat->transactions[index1].sort_index = windat->transactions[index2].sort_index;
	windat->transactions[index2].sort_index = temp;
}


/**
 * Save a file directly, if it already has a filename associated with it, or
 * open a save dialogue.
 *
 * \param *windat		The window to save.
 */

static void transact_list_window_start_direct_save(struct transact_list_window *windat)
{
	wimp_pointer pointer;

	if (windat == NULL || windat->file == NULL)
		return;

	if (file_check_for_filepath(windat->file)) {
		filing_save_cashbook_file(windat->file, windat->file->filename);
	} else {
		wimp_get_pointer_info(&pointer);

		saveas_initialise_dialogue(transact_saveas_file, NULL, "DefTransFile", NULL, FALSE, FALSE, windat);
		saveas_prepare_dialogue(transact_saveas_file);
		saveas_open_dialogue(transact_saveas_file, &pointer);
	}
}


/**
 * Callback handler for saving a cashbook file.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool transact_list_window_save_file(char *filename, osbool selection, void *data)
{
	struct transact_list_window *windat = data;

	if (windat == NULL || windat->file == NULL)
		return FALSE;

	filing_save_cashbook_file(windat->file, filename);

	return TRUE;
}


/**
 * Callback handler for saving a CSV version of the transaction data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool transact_list_window_save_csv(char *filename, osbool selection, void *data)
{
	struct transact_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	transact_list_window_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the transaction data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool transact_list_window_save_tsv(char *filename, osbool selection, void *data)
{
	struct transact_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	transact_list_window_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

	return TRUE;
}


/**
 * Export the transaction data from a file into CSV or TSV format.
 *
 * \param *windat		The window to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void transact_list_window_export_delimited(struct transact_list_window *windat, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE	*out;
	int	line;
	tran_t	transaction;
	char	buffer[FILING_DELIMITED_FIELD_LEN];

	if (windat == NULL || windat->file == NULL)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	columns_export_heading_names(windat->columns, windat->transaction_pane, out, format, buffer, FILING_DELIMITED_FIELD_LEN);

	/* Output the transaction data as a set of delimited lines. */

	for (line = 0; line < windat->trans_count; line++) {
		transaction = windat->transactions[line].sort_index;

		string_printf(buffer, FILING_DELIMITED_FIELD_LEN, "%d", transact_get_transaction_number(transaction));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		date_convert_to_string(windat->transactions[transaction].date, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		account_build_name_pair(windat->file, windat->transactions[transaction].from, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		account_build_name_pair(windat->file, windat->transactions[transaction].to, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		filing_output_delimited_field(out, windat->transactions[transaction].reference, format, DELIMIT_NONE);

		currency_convert_to_string(windat->transactions[transaction].amount, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, windat->transactions[transaction].description, format, DELIMIT_LAST);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}


/**
 * Handle attempts to load CSV files to the window.
 *
 * \param w			The target window handle.
 * \param i			The target icon handle.
 * \param filetype		The filetype being loaded.
 * \param *filename		The name of the file being loaded.
 * \param *data			Unused NULL pointer.
 * \return			TRUE on loading; FALSE on passing up.
 */

static osbool transact_list_window_load_csv(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data)
{
	struct file_block *file = data;

	if (filetype != dataxfer_TYPE_CSV || file == NULL)
		return FALSE;

	filing_import_csv_file(file, filename);

	return TRUE;
}

