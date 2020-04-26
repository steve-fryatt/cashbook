/* Copyright 2003-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "oslib/types.h"

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
#include "transact_list_window.h"

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
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "file_info.h"
#include "filing.h"
#include "find.h"
#include "flexutils.h"
#include "goto.h"
#include "list_window.h"
#include "preset.h"
#include "preset_menu.h"
#include "print_dialogue.h"
#include "purge.h"
#include "refdesc_menu.h"
#include "report.h"
#include "sorder.h"
#include "sorder_full_report.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "transact.h"
#include "window.h"

/* Transaction List Window icons. */

#define TRANSACT_LIST_WINDOW_ROW 0
#define TRANSACT_LIST_WINDOW_DATE 1
#define TRANSACT_LIST_WINDOW_FROM 2
#define TRANSACT_LIST_WINDOW_FROM_REC 3
#define TRANSACT_LIST_WINDOW_FROM_NAME 4
#define TRANSACT_LIST_WINDOW_TO 5
#define TRANSACT_LIST_WINDOW_TO_REC 6
#define TRANSACT_LIST_WINDOW_TO_NAME 7
#define TRANSACT_LIST_WINDOW_REFERENCE 8
#define TRANSACT_LIST_WINDOW_AMOUNT 9
#define TRANSACT_LIST_WINDOW_DESCRIPTION 10

/* Transaction List Window Toolbar icons. */

#define TRANSACT_LIST_WINDOW_PANE_ROW 0
#define TRANSACT_LIST_WINDOW_PANE_DATE 1
#define TRANSACT_LIST_WINDOW_PANE_FROM 2
#define TRANSACT_LIST_WINDOW_PANE_TO 3
#define TRANSACT_LIST_WINDOW_PANE_REFERENCE 4
#define TRANSACT_LIST_WINDOW_PANE_AMOUNT 5
#define TRANSACT_LIST_WINDOW_PANE_DESCRIPTION 6

#define TRANSACT_LIST_WINDOW_PANE_SORT_DIR_ICON 7

#define TRANSACT_LIST_WINDOW_PANE_SAVE 8
#define TRANSACT_LIST_WINDOW_PANE_PRINT 9
#define TRANSACT_LIST_WINDOW_PANE_ACCOUNTS 10
#define TRANSACT_LIST_WINDOW_PANE_VIEWACCT 11
#define TRANSACT_LIST_WINDOW_PANE_ADDACCT 12
#define TRANSACT_LIST_WINDOW_PANE_IN 13
#define TRANSACT_LIST_WINDOW_PANE_OUT 14
#define TRANSACT_LIST_WINDOW_PANE_ADDHEAD 15
#define TRANSACT_LIST_WINDOW_PANE_SORDER 16
#define TRANSACT_LIST_WINDOW_PANE_ADDSORDER 17
#define TRANSACT_LIST_WINDOW_PANE_PRESET 18
#define TRANSACT_LIST_WINDOW_PANE_ADDPRESET 19
#define TRANSACT_LIST_WINDOW_PANE_FIND 20
#define TRANSACT_LIST_WINDOW_PANE_GOTO 21
#define TRANSACT_LIST_WINDOW_PANE_SORT 22
#define TRANSACT_LIST_WINDOW_PANE_RECONCILE 23

/* Transaction List Window Menu entries. */

#define TRANSACT_LIST_WINDOW_MENU_SUB_FILE 0
#define TRANSACT_LIST_WINDOW_MENU_SUB_ACCOUNTS 1
#define TRANSACT_LIST_WINDOW_MENU_SUB_HEADINGS 2
#define TRANSACT_LIST_WINDOW_MENU_SUB_TRANS 3
#define TRANSACT_LIST_WINDOW_MENU_SUB_UTILS 4

#define TRANSACT_LIST_WINDOW_MENU_FILE_INFO 0
#define TRANSACT_LIST_WINDOW_MENU_FILE_SAVE 1
#define TRANSACT_LIST_WINDOW_MENU_FILE_EXPCSV 2
#define TRANSACT_LIST_WINDOW_MENU_FILE_EXPTSV 3
#define TRANSACT_LIST_WINDOW_MENU_FILE_CONTINUE 4
#define TRANSACT_LIST_WINDOW_MENU_FILE_PRINT 5

#define TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_VIEW 0
#define TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_LIST 1
#define TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_NEW 2

#define TRANSACT_LIST_WINDOW_MENU_HEADINGS_LISTIN 0
#define TRANSACT_LIST_WINDOW_MENU_HEADINGS_LISTOUT 1
#define TRANSACT_LIST_WINDOW_MENU_HEADINGS_NEW 2

#define TRANSACT_LIST_WINDOW_MENU_TRANS_FIND 0
#define TRANSACT_LIST_WINDOW_MENU_TRANS_GOTO 1
#define TRANSACT_LIST_WINDOW_MENU_TRANS_SORT 2
#define TRANSACT_LIST_WINDOW_MENU_TRANS_AUTOVIEW 3
#define TRANSACT_LIST_WINDOW_MENU_TRANS_AUTONEW 4
#define TRANSACT_LIST_WINDOW_MENU_TRANS_PRESET 5
#define TRANSACT_LIST_WINDOW_MENU_TRANS_PRESETNEW 6
#define TRANSACT_LIST_WINDOW_MENU_TRANS_RECONCILE 7

#define TRANSACT_LIST_WINDOW_MENU_ANALYSIS_BUDGET 0
#define TRANSACT_LIST_WINDOW_MENU_ANALYSIS_SAVEDREP 1
#define TRANSACT_LIST_WINDOW_MENU_ANALYSIS_MONTHREP 2
#define TRANSACT_LIST_WINDOW_MENU_ANALYSIS_UNREC 3
#define TRANSACT_LIST_WINDOW_MENU_ANALYSIS_CASHFLOW 4
#define TRANSACT_LIST_WINDOW_MENU_ANALYSIS_BALANCE 5
#define TRANSACT_LIST_WINDOW_MENU_ANALYSIS_SOREP 6

/* Transaction List Sort Window icons. */

#define TRANSACT_LIST_WINDOW_SORT_OK 2
#define TRANSACT_LIST_WINDOW_SORT_CANCEL 3
#define TRANSACT_LIST_WINDOW_SORT_DATE 4
#define TRANSACT_LIST_WINDOW_SORT_FROM 5
#define TRANSACT_LIST_WINDOW_SORT_TO 6
#define TRANSACT_LIST_WINDOW_SORT_REFERENCE 7
#define TRANSACT_LIST_WINDOW_SORT_AMOUNT 8
#define TRANSACT_LIST_WINDOW_SORT_DESCRIPTION 9
#define TRANSACT_LIST_WINDOW_SORT_ASCENDING 10
#define TRANSACT_LIST_WINDOW_SORT_DESCENDING 11
#define TRANSACT_LIST_WINDOW_SORT_ROW 12

/**
 * The minimum number of entries in the Transaction List Window.
 */

#define TRANSACT_LIST_WINDOW_MIN_ENTRIES 10

/**
 * The minimum number of blank lines in the Transaction List Window.
 */

#define TRANSACT_LIST_WINDOW_MIN_BLANK_LINES 1

/**
 * The height of the Transaction List Window toolbar, in OS Units.
 */
 
#define TRANSACT_LIST_WINDOW_TOOLBAR_HEIGHT 132

/**
 * The number of draggable columns in the Transaction List Window.
 */

#define TRANSACT_LIST_WINDOW_COLUMNS 11

/* The Transaction List Window column map. */

static struct column_map transact_list_window_columns[TRANSACT_LIST_WINDOW_COLUMNS] = {
	{TRANSACT_LIST_WINDOW_ROW, wimp_ICON_WINDOW, TRANSACT_LIST_WINDOW_PANE_ROW, wimp_ICON_WINDOW, SORT_ROW},
	{TRANSACT_LIST_WINDOW_DATE, wimp_ICON_WINDOW, TRANSACT_LIST_WINDOW_PANE_DATE, wimp_ICON_WINDOW, SORT_DATE},
	{TRANSACT_LIST_WINDOW_FROM, wimp_ICON_WINDOW, TRANSACT_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{TRANSACT_LIST_WINDOW_FROM_REC, TRANSACT_LIST_WINDOW_FROM, TRANSACT_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{TRANSACT_LIST_WINDOW_FROM_NAME, TRANSACT_LIST_WINDOW_FROM, TRANSACT_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{TRANSACT_LIST_WINDOW_TO, wimp_ICON_WINDOW, TRANSACT_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{TRANSACT_LIST_WINDOW_TO_REC, TRANSACT_LIST_WINDOW_TO, TRANSACT_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{TRANSACT_LIST_WINDOW_TO_NAME, TRANSACT_LIST_WINDOW_TO, TRANSACT_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{TRANSACT_LIST_WINDOW_REFERENCE, wimp_ICON_WINDOW, TRANSACT_LIST_WINDOW_PANE_REFERENCE, wimp_ICON_WINDOW, SORT_REFERENCE},
	{TRANSACT_LIST_WINDOW_AMOUNT, wimp_ICON_WINDOW, TRANSACT_LIST_WINDOW_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{TRANSACT_LIST_WINDOW_DESCRIPTION, wimp_ICON_WINDOW, TRANSACT_LIST_WINDOW_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION}
};

/**
 * The Transaction List Window Sort Dialogue column icons.
 */

static struct sort_dialogue_icon transact_list_window_sort_columns[] = {
	{TRANSACT_LIST_WINDOW_SORT_ROW, SORT_ROW},
	{TRANSACT_LIST_WINDOW_SORT_DATE, SORT_DATE},
	{TRANSACT_LIST_WINDOW_SORT_FROM, SORT_FROM},
	{TRANSACT_LIST_WINDOW_SORT_TO, SORT_TO},
	{TRANSACT_LIST_WINDOW_SORT_REFERENCE, SORT_REFERENCE},
	{TRANSACT_LIST_WINDOW_SORT_AMOUNT, SORT_AMOUNT},
	{TRANSACT_LIST_WINDOW_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{0, SORT_NONE}
};

/**
 * The Transaction List Window Sort Dialogue direction icons.
 */

static struct sort_dialogue_icon transact_list_window_sort_directions[] = {
	{TRANSACT_LIST_WINDOW_SORT_ASCENDING, SORT_ASCENDING},
	{TRANSACT_LIST_WINDOW_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/**
 * The Transaction List Window definition.
 */

static struct list_window_definition transact_list_window_definition = {
	"Transact",					/**< The list window template name.		*/
	"TransactTB",					/**< The list toolbar template name.		*/
	NULL,						/**< The list footer template name.		*/
	"MainMenu",					/**< The list menu template name.		*/
	TRANSACT_LIST_WINDOW_TOOLBAR_HEIGHT,		/**< The list toolbar height, in OS Units.	*/
	0,						/**< The list footer height, in OS Units.	*/
	transact_list_window_columns,			/**< The window column definitions.		*/
	NULL,						/**< The window column extended definitions.	*/
	TRANSACT_LIST_WINDOW_COLUMNS,			/**< The number of column definitions.		*/
	"LimTransactCols",				/**< The column width limit config token.	*/
	"TransacttCols",				/**< The column width config token.		*/
	TRANSACT_LIST_WINDOW_PANE_SORT_DIR_ICON,	/**< The toolbar icon used to show sort order.	*/
	"SortTrans",					/**< The sort dialogue template name.		*/
	transact_list_window_sort_columns,		/**< The sort dialogue column icons.		*/
	transact_list_window_sort_directions,		/**< The sort dialogue direction icons.		*/
	TRANSACT_LIST_WINDOW_SORT_OK,			/**< The sort dialogue OK icon.			*/
	TRANSACT_LIST_WINDOW_SORT_CANCEL,		/**< The sort dialogue Cancel icon.		*/
	NULL,						/*<< Window Title token.			*/
	"Transact",					/**< Window Help token base.			*/
	"TransactTB",					/**< Window Toolbar help token base.		*/
	NULL,						/**< Window Footer help token base.		*/
	"MainMenu",					/**< Window Menu help token base.		*/
	"SortTrans",					/**< Sort dialogue help token base.		*/
	TRANSACT_LIST_WINDOW_MIN_ENTRIES,		/**< The minimum number of rows displayed.	*/
	TRANSACT_LIST_WINDOW_MIN_BLANK_LINES,		/**< The minimum number of blank lines.		*/
	"PrintTransact",				/**< The print dialogue title token.		*/
	"PrintTitleTransact",				/**< The print report title token.		*/
	TRUE,						/**< Should the print dialogue use dates?	*/
	LIST_WINDOW_FLAGS_PARENT |			/**< The window flags.				*/
		LIST_WINDOW_FLAGS_EDIT,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/**
 * Transaction List Window line redraw data.
 */

struct transact_list_window_redraw {
	/**
	 * The number of the transaction relating to the line.
	 */
	tran_t					transaction;
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
	 * The list window for the Preset List Window.
	 */
	struct list_window			*window;

	/**
	 * True if reconcile should automatically jump to the next unreconciled entry.
	 */
	osbool					auto_reconcile;	
};

/**
 * The Transaction List Window base instance.
 */

static struct list_window_block		*transact_list_window_block = NULL;

/**
 * The handle of the Transaction List Window menu.
 */

static wimp_menu			*transact_list_window_menu = NULL;

/**
 * The Transaction List Window Account submenu handle.
 */

static wimp_menu			*transact_list_window_menu_account = NULL;

/**
 * The Transaction List Window Transaction submenu handle.
 */

static wimp_menu			*transact_list_window_menu_transact = NULL;

/**
 * The Transaction List Window Analysis submenu handle.
 */

static wimp_menu			*transact_list_window_menu_analysis = NULL;

/**
 * The Transaction List Window Toolbar's Account List popup menu handle.
 */

static wimp_menu			*transact_list_window_account_list_menu = NULL;

/**
 * The Save File saveas data handle.
 */

static struct saveas_block		*transact_list_window_saveas_file = NULL;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block		*transact_list_window_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block		*transact_list_window_saveas_tsv = NULL;

/**
 * Data relating to window redrawing.
 */

struct transact_list_window_redraw_data {
	wimp_colour	shade_rec_colour;
};

/**
 * Instance of the window redraw data, held statically to avoid heap shifts on every redraw.
 */

static struct transact_list_window_redraw_data transact_list_window_redraw_data_block;

/* Buffers used by the Transaction List Window Edit Line. */

static char	transact_buffer_row[TRANSACT_ROW_FIELD_LEN];
static char	transact_buffer_date[DATE_FIELD_LEN];
static char	transact_buffer_from_ident[ACCOUNT_IDENT_LEN];
static char	transact_buffer_from_name[ACCOUNT_NAME_LEN];
static char	transact_buffer_from_rec[REC_FIELD_LEN];
static char	transact_buffer_to_ident[ACCOUNT_IDENT_LEN];
static char	transact_buffer_to_name[ACCOUNT_NAME_LEN];
static char	transact_buffer_to_rec[REC_FIELD_LEN];
static char	transact_buffer_reference [TRANSACT_REF_FIELD_LEN];
static char	transact_buffer_amount[AMOUNT_FIELD_LEN];
static char	transact_buffer_description[TRANSACT_DESCRIPT_FIELD_LEN];

/* Static Function Prototypes. */

static void transact_list_window_click_handler(wimp_pointer *pointer);
static void transact_list_window_pane_click_handler(wimp_pointer *pointer, struct file_block *file, void *data);
static osbool transact_list_window_keypress_handler(wimp_key *key);
static void transact_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void transact_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void transact_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void transact_list_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void *transact_list_window_redraw_prepare(struct file_block *file, void *data);
static void transact_list_window_redraw_handler(int index, struct file_block *file, void *data, void *redraw);


static void transact_list_window_place_edit_line(struct transact_list_window *windat, int line);
static osbool transact_list_window_edit_get_field(struct edit_data *data);
static osbool transact_list_window_edit_put_field(struct edit_data *data);
static osbool transact_list_window_edit_auto_complete(struct edit_data *data);
static char *transact_list_window_complete_description(struct transact_list_window *windat, int line, char *buffer, size_t length);
static void transact_list_window_find_next_reconcile_line(struct transact_list_window *windat, osbool set);
static osbool transact_list_window_edit_insert_preset(int line, wimp_key_no key, void *data);
static wimp_i transact_list_window_convert_preset_icon_number(enum preset_caret caret);
static int transact_list_window_edit_auto_sort(wimp_i icon, void *data);
static wimp_colour transact_list_window_line_colour(tran_t transaction, struct file_block *file, void *data);
static osbool transact_list_window_extend_display_lines(struct transact_list_window *windat, int line);
static osbool transact_list_window_test_blank(tran_t transaction, struct file_block *file, void *data);


static void transact_list_window_open_print_window(struct transact_list_window *windat, wimp_pointer *ptr, osbool restore);
static osbool transact_list_window_print_include(struct file_block *file, tran_t transaction, date_t from, date_t to);
static struct report *transact_list_window_print_field(struct file_block *file, wimp_i column, tran_t transaction, char *rec_char);
static int transact_list_window_sort_compare(enum sort_type type, int index1, int index2, struct file_block *file);
static void transact_list_window_start_direct_save(struct transact_list_window *windat);
static osbool transact_list_window_save_file(char *filename, osbool selection, void *data);
static osbool transact_list_window_save_csv(char *filename, osbool selection, void *data);
static osbool transact_list_window_save_tsv(char *filename, osbool selection, void *data);
static void transact_list_window_export_delimited_line(FILE *out, enum filing_delimit_type format, struct file_block *file, int index);
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
//	transact_list_window_definition.callback_window_click_handler = transact_list_window_click_handler;
	transact_list_window_definition.callback_pane_click_handler = transact_list_window_pane_click_handler;
	transact_list_window_definition.callback_redraw_prepare = transact_list_window_redraw_prepare;
	transact_list_window_definition.callback_redraw_handler = transact_list_window_redraw_handler;
//	transact_list_window_definition.callback_menu_prepare_handler = transact_list_window_menu_prepare_handler;
//	transact_list_window_definition.callback_menu_selection_handler = transact_list_window_menu_selection_handler;
//	transact_list_window_definition.callback_menu_warning_handler = transact_list_window_menu_warning_handler;
	transact_list_window_definition.callback_sort_compare = transact_list_window_sort_compare;
	transact_list_window_definition.callback_print_include = transact_list_window_print_include;
//	transact_list_window_definition.callback_print_field = transact_list_window_print_field;
	transact_list_window_definition.callback_export_line = transact_list_window_export_delimited_line;
	transact_list_window_definition.callback_is_blank = transact_list_window_test_blank;

	transact_list_window_block = list_window_create(&transact_list_window_definition, sprites);

	transact_list_window_menu_account = templates_get_menu("MainAccountsSubmenu");
	transact_list_window_menu_transact = templates_get_menu("MainTransactionsSubmenu");
	transact_list_window_menu_analysis = templates_get_menu("MainAnalysisSubmenu");

	transact_list_window_saveas_file = saveas_create_dialogue(FALSE, "file_1ca", transact_list_window_save_file);
	transact_list_window_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", transact_list_window_save_csv);
	transact_list_window_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", transact_list_window_save_tsv);
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
	new->window = NULL;

	new->auto_reconcile = FALSE;

	/* Initialise the List Window. */

	new->window = list_window_create_instance(transact_list_window_block, transact_get_file(parent), new);
	if (new->window == NULL) {
		transact_list_window_delete_instance(new);
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

	list_window_delete_instance(windat->window);

	heap_free(windat);
}


/**
 * Create and open a Transaction List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void transact_list_window_open(struct transact_list_window *windat)
{
	if (windat == NULL)
		return;

	list_window_open(windat->window);




//	edit_add_field(windat->edit_line, EDIT_FIELD_DISPLAY,
//			TRANSACT_LIST_WINDOW_ROW, transact_buffer_row, TRANSACT_ROW_FIELD_LEN);
//	edit_add_field(windat->edit_line, EDIT_FIELD_DATE,
//			TRANSACT_LIST_WINDOW_DATE, transact_buffer_date, DATE_FIELD_LEN);
//	edit_add_field(windat->edit_line, EDIT_FIELD_ACCOUNT_IN,
//			TRANSACT_LIST_WINDOW_FROM, transact_buffer_from_ident, ACCOUNT_IDENT_LEN,
//			TRANSACT_LIST_WINDOW_FROM_REC, transact_buffer_from_rec, REC_FIELD_LEN,
//			TRANSACT_LIST_WINDOW_FROM_NAME, transact_buffer_from_name, ACCOUNT_NAME_LEN);
//	edit_add_field(windat->edit_line, EDIT_FIELD_ACCOUNT_OUT,
//			TRANSACT_LIST_WINDOW_TO, transact_buffer_to_ident, ACCOUNT_IDENT_LEN,
//			TRANSACT_LIST_WINDOW_TO_REC, transact_buffer_to_rec, REC_FIELD_LEN,
//			TRANSACT_LIST_WINDOW_TO_NAME, transact_buffer_to_name, ACCOUNT_NAME_LEN);
//	edit_add_field(windat->edit_line, EDIT_FIELD_TEXT,
//			TRANSACT_LIST_WINDOW_REFERENCE, transact_buffer_reference, TRANSACT_REF_FIELD_LEN);
//	edit_add_field(windat->edit_line, EDIT_FIELD_CURRENCY,
//			TRANSACT_LIST_WINDOW_AMOUNT, transact_buffer_amount, AMOUNT_FIELD_LEN);
//	edit_add_field(windat->edit_line, EDIT_FIELD_TEXT,
//			TRANSACT_LIST_WINDOW_DESCRIPTION, transact_buffer_description, TRANSACT_DESCRIPT_FIELD_LEN);

//	if (!edit_complete(windat->edit_line)) {
//		transact_list_window_delete(windat);
//		error_msgs_report_error("TransactNoMem");
//		return;
//	}


	/* Update the toolbar */

//	transact_update_toolbar(file);


	/* Register event handlers for the two windows. */

//	event_add_window_key_event(windat->transaction_window, transact_list_window_keypress_handler);

//	event_add_window_icon_popup(windat->transaction_pane, TRANSACT_LIST_WINDOW_PANE_VIEWACCT, transact_list_window_account_list_menu, -1, NULL);

//	dataxfer_set_drop_target(dataxfer_TYPE_CSV, windat->transaction_window, -1, NULL, transact_list_window_load_csv, file);
//	dataxfer_set_drop_target(dataxfer_TYPE_CSV, windat->transaction_pane, -1, NULL, transact_list_window_load_csv, file);

	/* Put the caret into the first empty line. */

//	transact_list_window_place_caret(windat, windat->display_lines, TRANSACT_FIELD_DATE);
}


/**
 * Close and delete the Transaction List Window associated with the
 * given instance.
 * 
 * THIS IS NOT CALLED BY ANY CODE ANY MORE!!!
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


//	if (windat->transaction_window != NULL) {
//		dataxfer_delete_drop_target(dataxfer_TYPE_CSV, windat->transaction_window, -1);
//	}

//	if (windat->transaction_pane != NULL) {
//		dataxfer_delete_drop_target(dataxfer_TYPE_CSV, windat->transaction_pane, -1);
//	}
}


/**
 * Handle Close events on Transaction List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void transact_list_window_close_handler(wimp_close *close)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	wimp_pointer			pointer;
	char				buffer[1024], *pathcopy;

	#ifdef DEBUG
	debug_printf("\\RClosing Transaction List window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	/* If Adjust was clicked, find the pathname and open the parent directory. */

	if (pointer.buttons == wimp_CLICK_ADJUST && file_check_for_filepath(file)) {
		pathcopy = strdup(file->filename);
		if (pathcopy != NULL) {
			string_printf(buffer, sizeof(buffer), "%%Filer_OpenDir %s", string_find_pathname(pathcopy));
			xos_cli(buffer);
			free(pathcopy);
		}
	}

	/* If it was NOT an Adjust click with Shift held down, close the file. */

	if (!((osbyte1(osbyte_IN_KEY, 0xfc, 0xff) == 0xff || osbyte1(osbyte_IN_KEY, 0xf9, 0xff) == 0xff) &&
			pointer.buttons == wimp_CLICK_ADJUST))
		delete_file(file);
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


	if (pointer->buttons == wimp_CLICK_ADJUST) {
		/* Adjust clicks don't care about icons, as we only need to know which line and column we're in. */

		/* If the line was in range, find the column that the click occurred in by scanning through the column
		 * positions.
		 */

		if (line >= 0) {
			xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;

//			column = column_find_icon_from_xpos(windat->columns, xpos);

#ifdef DEBUG
			debug_printf("Adjust transaction window click (line %d, column %d, xpos %d)", line, column, xpos);
#endif

			/* The first options can take place on any line in the window... */

//			if (column == TRANSACT_LIST_WINDOW_DATE) {
				/* If the column is Date, open the date menu. */
//				preset_menu_open(file, line, pointer);
//			} else if (column == TRANSACT_LIST_WINDOW_FROM_NAME) {
				/* If the column is the From name, open the from account menu. */
//				account_menu_open(file, ACCOUNT_MENU_FROM, line, pointer);
//			} else if (column == TRANSACT_LIST_WINDOW_TO_NAME) {
				/* If the column is the To name, open the to account menu. */
//				account_menu_open(file, ACCOUNT_MENU_TO, line, pointer);
//			} else if (column == TRANSACT_LIST_WINDOW_REFERENCE) {
				/* If the column is the Reference, open the to reference menu. */
//				refdesc_menu_open(file, REFDESC_MENU_REFERENCE, line, pointer);
//			} else if (column == TRANSACT_LIST_WINDOW_DESCRIPTION) {
				/* If the column is the Description, open the to description menu. */
//				refdesc_menu_open(file, REFDESC_MENU_DESCRIPTION, line, pointer);
//			} else if (line < windat->display_lines) {
				/* ...while the rest have to occur over a transaction line. */
//				transaction = windat->line_data[line].transaction;

//				if (column == TRANSACT_LIST_WINDOW_FROM_REC) {
					/* If the column is the from reconcile flag, toggle its status. */
//					transact_toggle_reconcile_flag(file, transaction, TRANS_REC_FROM);
//				} else if (column == TRANSACT_LIST_WINDOW_TO_REC) {
					/* If the column is the to reconcile flag, toggle its status. */
//					transact_toggle_reconcile_flag(file, transaction, TRANS_REC_TO);
//				}
//			}
		}
	}
}


/**
 * Process mouse clicks in the Transaction List pane.
 *
 * \param *pointer		The mouse event block to handle.
 * \param *file			The file owining the window.
 * \param *data			The Standing Order List Window instance.
 */
static void transact_list_window_pane_click_handler(wimp_pointer *pointer, struct file_block *file, void *data)
{
	struct transact_list_window	*windat = data;
	char				*filename;

	if (windat == NULL || windat->instance == NULL)
		return;


	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case TRANSACT_LIST_WINDOW_PANE_SAVE:
			if (file_check_for_filepath(file))
				filename = file->filename;
			else
				filename = NULL;

			saveas_initialise_dialogue(transact_list_window_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
			saveas_prepare_dialogue(transact_list_window_saveas_file);
			saveas_open_dialogue(transact_list_window_saveas_file, pointer);
			break;

		case TRANSACT_LIST_WINDOW_PANE_PRINT:
			transact_list_window_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_PANE_ACCOUNTS:
			account_open_window(file, ACCOUNT_FULL);
			break;

		case TRANSACT_LIST_WINDOW_PANE_ADDACCT:
			account_open_edit_window(file, NULL_ACCOUNT, ACCOUNT_FULL, pointer);
			break;

		case TRANSACT_LIST_WINDOW_PANE_IN:
			account_open_window(file, ACCOUNT_IN);
			break;

		case TRANSACT_LIST_WINDOW_PANE_OUT:
			account_open_window(file, ACCOUNT_OUT);
			break;

		case TRANSACT_LIST_WINDOW_PANE_ADDHEAD:
			account_open_edit_window(file, NULL_ACCOUNT, ACCOUNT_IN, pointer);
			break;

		case TRANSACT_LIST_WINDOW_PANE_FIND:
			find_open_window(file->find, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_PANE_GOTO:
			goto_open_window(file->go_to, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_PANE_SORT:
			list_window_open_sort_window(windat->window, pointer);
			break;

		case TRANSACT_LIST_WINDOW_PANE_RECONCILE:
			windat->auto_reconcile = !windat->auto_reconcile;
			break;

		case TRANSACT_LIST_WINDOW_PANE_SORDER:
			sorder_open_window(file);
			break;

		case TRANSACT_LIST_WINDOW_PANE_ADDSORDER:
			sorder_open_edit_window(file, NULL_SORDER, pointer);
			break;

		case TRANSACT_LIST_WINDOW_PANE_PRESET:
			preset_open_window(file);
			break;

		case TRANSACT_LIST_WINDOW_PANE_ADDPRESET:
			preset_open_edit_window(file, NULL_PRESET, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case TRANSACT_LIST_WINDOW_PANE_SAVE:
			transact_list_window_start_direct_save(windat);
			break;

		case TRANSACT_LIST_WINDOW_PANE_PRINT:
			transact_list_window_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_PANE_FIND:
			find_open_window(file->find, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_PANE_GOTO:
			goto_open_window(file->go_to, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_PANE_SORT:
			transact_list_window_sort(windat);
			break;

		case TRANSACT_LIST_WINDOW_PANE_RECONCILE:
			windat->auto_reconcile = !windat->auto_reconcile;
			break;
		}
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
		return FALSE;

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

		if (file_check_for_filepath(file))
			filename = file->filename;
		else
			filename = NULL;

		saveas_initialise_dialogue(transact_list_window_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
		saveas_prepare_dialogue(transact_list_window_saveas_file);
		saveas_open_dialogue(transact_list_window_saveas_file, &pointer);
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
		list_window_open_sort_window(windat->window, &pointer);
	} else if (key->c == wimp_KEY_F9) {
		account_open_window(file, ACCOUNT_FULL);
	} else if (key->c == wimp_KEY_F10) {
		account_open_window(file, ACCOUNT_IN);
	} else if (key->c == wimp_KEY_F11) {
		account_open_window(file, ACCOUNT_OUT);
	} else if (key->c == wimp_KEY_PAGE_UP || key->c == wimp_KEY_PAGE_DOWN) {
		wimp_scroll scroll;

		/* Make up a Wimp_ScrollRequest block and pass it to the scroll request handler. */

//		scroll.w = windat->transaction_window;
//		wimp_get_window_state((wimp_window_state *) &scroll);

//		scroll.xmin = wimp_SCROLL_NONE;
//		scroll.ymin = (key->c == wimp_KEY_PAGE_UP) ? wimp_SCROLL_PAGE_UP : wimp_SCROLL_PAGE_DOWN;

//		transact_list_window_scroll_handler(&scroll);
	} else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_UP) || key->c == wimp_KEY_HOME) {
		transact_scroll_window_to_end(file, TRANSACT_SCROLL_HOME);
	} else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_DOWN) ||
			(key->c == wimp_KEY_COPY && config_opt_read ("IyonixKeys"))) {
		transact_scroll_window_to_end(file, TRANSACT_SCROLL_END);
	} else {
		/* Pass any keys that are left on to the edit line handler. */
//		return edit_process_keypress(windat->edit_line, key);
		return FALSE;
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

	if (menu != transact_list_window_menu) {
		if (pointer != NULL) {
			transact_list_window_account_list_menu = account_list_menu_build(file);
			event_set_menu_block(transact_list_window_account_list_menu);
			ihelp_add_menu(transact_list_window_account_list_menu, "AccOpenMenu");
		}

		account_list_menu_prepare();
		return;
	}

	/* Otherwise, this is the standard window menu.
	 */

	if (pointer != NULL) {
//		transact_list_window_menu_line = NULL_TRANSACTION;

//		if (w == windat->transaction_window) {
//			window.w = w;
//			wimp_get_window_state(&window);

//			line = window_calculate_click_row(&(pointer->pos), &window, TRANSACT_LIST_WINDOW_TOOLBAR_HEIGHT, -1);

//			if (transact_list_window_line_valid(windat, line))
//				transact_list_window_menu_line = line;
//		}

//		transact_list_window_menu_account->entries[TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_VIEW].sub_menu = account_list_menu_build(file);
//		transact_list_window_menu_analysis->entries[TRANSACT_LIST_WINDOW_MENU_ANALYSIS_SAVEDREP].sub_menu = analysis_template_menu_build(file, FALSE);

		/* If the submenus concerned are greyed out, give them a valid submenu pointer so that the arrow shows. */

//		if (account_get_count(file) == 0)
//			transact_list_window_menu_account->entries[TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_VIEW].sub_menu = (wimp_menu *) 0x8000; /* \TODO -- Ugh! */
//		if (!analysis_template_menu_contains_entries())
//			transact_list_window_menu_analysis->entries[TRANSACT_LIST_WINDOW_MENU_ANALYSIS_SAVEDREP].sub_menu = (wimp_menu *) 0x8000; /* \TODO -- Ugh! */

//		if (file_check_for_filepath(file))
//			filename = file->filename;
//		else
//			filename = NULL;

//		saveas_initialise_dialogue(transact_list_window_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
//		saveas_initialise_dialogue(transact_list_window_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
//		saveas_initialise_dialogue(transact_list_window_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

//	menus_tick_entry(transact_list_window_menu_transact, TRANSACT_LIST_WINDOW_MENU_TRANS_RECONCILE, windat->auto_reconcile);
//	menus_shade_entry(transact_list_window_menu_account, TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_VIEW, account_count_type_in_file(file, ACCOUNT_FULL) == 0);
//	menus_shade_entry(transact_list_window_menu_analysis, TRANSACT_LIST_WINDOW_MENU_ANALYSIS_SAVEDREP, !analysis_template_menu_contains_entries());
//	account_list_menu_prepare();
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
	if (windat == NULL || windat->instance == NULL)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	/* If the menu is the account open menu, then it needs special processing...
	 */

	if (menu == transact_list_window_account_list_menu) {
		if (selection->items[0] != -1)
			accview_open_window(file, account_list_menu_decode(selection->items[0]));

		return;
	}

	/* ...otherwise, handle it as normal.
	 */

	wimp_get_pointer_info(&pointer);
/*
	switch (selection->items[0]){
	case TRANSACT_LIST_WINDOW_MENU_SUB_FILE:
		switch (selection->items[1]) {
		case TRANSACT_LIST_WINDOW_MENU_FILE_SAVE:
			transact_list_window_start_direct_save(windat);
			break;

		case TRANSACT_LIST_WINDOW_MENU_FILE_CONTINUE:
			purge_open_window(file->purge, &pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_FILE_PRINT:
			transact_list_window_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
			break;
		}
		break;

	case TRANSACT_LIST_WINDOW_MENU_SUB_ACCOUNTS:
		switch (selection->items[1]) {
		case TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_VIEW:
			if (selection->items[2] != -1)
				accview_open_window(file, account_list_menu_decode(selection->items[2]));
			break;

		case TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_LIST:
			account_open_window(file, ACCOUNT_FULL);
			break;

		case TRANSACT_LIST_WINDOW_MENU_ACCOUNTS_NEW:
			account_open_edit_window(file, NULL_ACCOUNT, ACCOUNT_FULL, &pointer);
			break;
		}
		break;

	case TRANSACT_LIST_WINDOW_MENU_SUB_HEADINGS:
		switch (selection->items[1]) {
		case TRANSACT_LIST_WINDOW_MENU_HEADINGS_LISTIN:
			account_open_window(file, ACCOUNT_IN);
			break;

		case TRANSACT_LIST_WINDOW_MENU_HEADINGS_LISTOUT:
			account_open_window(file, ACCOUNT_OUT);
			break;

		case TRANSACT_LIST_WINDOW_MENU_HEADINGS_NEW:
			account_open_edit_window(file, NULL_ACCOUNT, ACCOUNT_IN, &pointer);
			break;
		}
		break;

	case TRANSACT_LIST_WINDOW_MENU_SUB_TRANS:
		switch (selection->items[1]) {
		case TRANSACT_LIST_WINDOW_MENU_TRANS_FIND:
			find_open_window(file->find, &pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_TRANS_GOTO:
			goto_open_window(file->go_to, &pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_TRANS_SORT:
			transact_list_window_open_sort_window(windat, &pointer);
			break;

		case TRANSACT_LIST_WINDOW_MENU_TRANS_AUTOVIEW:
			sorder_open_window(file);
			break;

		case TRANSACT_LIST_WINDOW_MENU_TRANS_AUTONEW:
			sorder_open_edit_window(file, NULL_SORDER, &pointer);
			break;

		case TRANSACT_LIST_WINDOW_MENU_TRANS_PRESET:
			preset_open_window(file);
			break;

		case TRANSACT_LIST_WINDOW_MENU_TRANS_PRESETNEW:
			preset_open_edit_window(file, NULL_PRESET, &pointer);
			break;

		case TRANSACT_LIST_WINDOW_MENU_TRANS_RECONCILE:
			windat->auto_reconcile = !windat->auto_reconcile;
			icons_set_selected(windat->transaction_pane, TRANSACT_LIST_WINDOW_PANE_RECONCILE, windat->auto_reconcile);
			break;
		}
		break;

	case TRANSACT_LIST_WINDOW_MENU_SUB_UTILS:
		switch (selection->items[1]) {
		case TRANSACT_LIST_WINDOW_MENU_ANALYSIS_BUDGET:
			budget_open_window(file->budget, &pointer);
			break;

		case TRANSACT_LIST_WINDOW_MENU_ANALYSIS_SAVEDREP:
			template = analysis_template_menu_decode(selection->items[2]);
			if (template != NULL_TEMPLATE)
				analysis_open_template(file->analysis, &pointer, template, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_ANALYSIS_MONTHREP:
			analysis_open_window(file->analysis, &pointer, REPORT_TYPE_TRANSACTION, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_ANALYSIS_UNREC:
			analysis_open_window(file->analysis, &pointer, REPORT_TYPE_UNRECONCILED, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_ANALYSIS_CASHFLOW:
			analysis_open_window(file->analysis, &pointer, REPORT_TYPE_CASHFLOW, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_ANALYSIS_BALANCE:
			analysis_open_window(file->analysis, &pointer, REPORT_TYPE_BALANCE, config_opt_read("RememberValues"));
			break;

		case TRANSACT_LIST_WINDOW_MENU_ANALYSIS_SOREP:
			sorder_full_report(file);
			break;
		}
		break;
	}*/
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
	struct transact_list_window	*windat;
	struct file_block		*file;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	if (menu != transact_list_window_menu)
		return;

	switch (warning->selection.items[0]) {
	case TRANSACT_LIST_WINDOW_MENU_SUB_FILE:
		switch (warning->selection.items[1]) {
		case TRANSACT_LIST_WINDOW_MENU_FILE_INFO:
			file_info_prepare_dialogue(file);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;

		case TRANSACT_LIST_WINDOW_MENU_FILE_SAVE:
			saveas_prepare_dialogue(transact_list_window_saveas_file);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;

		case TRANSACT_LIST_WINDOW_MENU_FILE_EXPCSV:
			saveas_prepare_dialogue(transact_list_window_saveas_csv);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;

		case TRANSACT_LIST_WINDOW_MENU_FILE_EXPTSV:
			saveas_prepare_dialogue(transact_list_window_saveas_tsv);
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
//	if (menu == transact_list_window_menu) {
//		transact_list_window_menu_line = -1;
//		analysis_template_menu_destroy();
//	} else if (menu == transact_list_window_account_list_menu) {
//		account_list_menu_destroy();
//		ihelp_remove_menu(transact_list_window_account_list_menu);
//		transact_list_window_account_list_menu = NULL;
//	}
}


/**
 * Prepare to handle redraw events for the Transaction List window.
 *
 * \param *file			Pointer to the owning file instance.
 * \param *data			Pointer to the Transaction List Window instance.
 * \return			Pointer to the redraw data block, to be passed to all
 *				redraw events.
 */

static void *transact_list_window_redraw_prepare(struct file_block *file, void *data)
{
	/* Identify the colour to use for plotting shaded lines. */

	if (config_opt_read("ShadeReconciled"))
		transact_list_window_redraw_data_block.shade_rec_colour = config_int_read("ShadeReconciledColour");
	else
		transact_list_window_redraw_data_block.shade_rec_colour = wimp_COLOUR_BLACK;

	return &transact_list_window_redraw_data_block;
}


/**
 * Process redraw events in the Transaction List window.
 *
 * \param *index		The index of the item in the line to be redrawn.
 * \param *file			Pointer to the owning file instance.
 * \param *data			Pointer to the Transaction List Window instance.
 * \param *redraw		Pointer to the redraw instance data.
 */

static void transact_list_window_redraw_handler(int index, struct file_block *file, void *data, void *redraw)
{
	struct transact_list_window_redraw_data	*redraw_data = redraw;
	tran_t					transaction = index;
	acct_t					account;
	enum transact_flags			flags;
	wimp_colour				icon_fg_col;

	flags =  transact_get_flags(file, transaction);

	/* Work out the foreground colour for the line, based on whether
		* the line is to be shaded or not.
		*/

	if ((flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO))
		icon_fg_col = redraw_data->shade_rec_colour;
	else
		icon_fg_col = wimp_COLOUR_BLACK;

	/* Row field. */

	window_plot_int_field(TRANSACT_LIST_WINDOW_ROW, transact_get_transaction_number(transaction), icon_fg_col);

	/* Date field */

	window_plot_date_field(TRANSACT_LIST_WINDOW_DATE, transact_get_date(file, transaction), icon_fg_col);

	/* From field */

	account = transact_get_from(file, transaction);

	window_plot_text_field(TRANSACT_LIST_WINDOW_FROM, account_get_ident(file, account), icon_fg_col);
	window_plot_reconciled_field(TRANSACT_LIST_WINDOW_FROM_REC, (flags & TRANS_REC_FROM), icon_fg_col);
	window_plot_text_field(TRANSACT_LIST_WINDOW_FROM_NAME, account_get_name(file, account), icon_fg_col);

	/* To field */

	account = transact_get_to(file, transaction);

	window_plot_text_field(TRANSACT_LIST_WINDOW_TO, account_get_ident(file, account), icon_fg_col);
	window_plot_reconciled_field(TRANSACT_LIST_WINDOW_TO_REC, (flags & TRANS_REC_TO), icon_fg_col);
	window_plot_text_field(TRANSACT_LIST_WINDOW_TO_NAME, account_get_name(file, account), icon_fg_col);

	/* Reference field */

	window_plot_text_field(TRANSACT_LIST_WINDOW_REFERENCE, transact_get_reference(file, transaction, NULL, 0), icon_fg_col);

	/* Amount field */

	window_plot_currency_field(TRANSACT_LIST_WINDOW_AMOUNT, transact_get_amount(file, transaction), icon_fg_col);

	/* Description field */

	window_plot_text_field(TRANSACT_LIST_WINDOW_DESCRIPTION, transact_get_description(file, transaction, NULL, 0), icon_fg_col);
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

	if (windat == NULL)
		return buffer;

	switch (field) {
	case TRANSACT_FIELD_NONE:
		return buffer;
	case TRANSACT_FIELD_ROW:
	case TRANSACT_FIELD_DATE:
		icon = TRANSACT_LIST_WINDOW_DATE;
		break;
	case TRANSACT_FIELD_FROM:
		icon = TRANSACT_LIST_WINDOW_FROM;
		break;
	case TRANSACT_FIELD_TO:
		icon = TRANSACT_LIST_WINDOW_TO;
		break;
	case TRANSACT_FIELD_AMOUNT:
		icon = TRANSACT_LIST_WINDOW_AMOUNT;
		break;
	case TRANSACT_FIELD_REF:
		icon = TRANSACT_LIST_WINDOW_REFERENCE;
		break;
	case TRANSACT_FIELD_DESC:
		icon = TRANSACT_LIST_WINDOW_DESCRIPTION;
		break;
	}

	if (icon == wimp_ICON_WINDOW)
		return buffer;

//	group = column_get_group_icon(windat->columns, icon);

	if (icon == wimp_ICON_WINDOW)
		return buffer;

//	icons_copy_text(windat->transaction_pane, group, buffer, len);

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

	if (windat == NULL)
		return;

//	transact_list_window_place_edit_line(windat, line);

//	switch (field) {
//	case TRANSACT_FIELD_NONE:
//		return;
//	case TRANSACT_FIELD_ROW:
//	case TRANSACT_FIELD_DATE:
//		icon = TRANSACT_LIST_WINDOW_DATE;
//		break;
//	case TRANSACT_FIELD_FROM:
//		icon = TRANSACT_LIST_WINDOW_FROM;
//		break;
//	case TRANSACT_FIELD_TO:
//		icon = TRANSACT_LIST_WINDOW_TO;
//		break;
//	case TRANSACT_FIELD_AMOUNT:
//		icon = TRANSACT_LIST_WINDOW_AMOUNT;
//		break;
//	case TRANSACT_FIELD_REF:
//		icon = TRANSACT_LIST_WINDOW_REFERENCE;
//		break;
//	case TRANSACT_FIELD_DESC:
//		icon = TRANSACT_LIST_WINDOW_DESCRIPTION;
//		break;
//	}

//	if (icon == wimp_ICON_WINDOW)
//		icon = TRANSACT_LIST_WINDOW_DATE;

//	icons_put_caret_at_end(windat->transaction_window, icon);
//	transact_list_window_find_edit_line_vertically(windat);
}


/**
 * Re-index the transactions in a transaction list window.  This can *only*
 * be done after transact_sort_file_data() has been called, as it requires
 * data set up in the transaction block by that call.
 *
 * \param *windat		The transaction window to reindex.
 */

void transact_list_window_reindex(struct transact_list_window *windat)
{
	struct file_block	*file;
	int			line, transaction;

//	file = transact_get_file(windat->instance);
//	if (file == NULL)
//		return;

//	if (windat == NULL || windat->line_data == NULL || windat->transaction_window == NULL)
//		return;

//	for (line = 0; line < windat->display_lines; line++) {
//		transaction = windat->line_data[line].transaction;
//		windat->line_data[line].transaction = transact_get_new_sort_index(file, transaction);
//	}

	// \TODO - Does this require a full redraw?

//	transact_list_window_force_redraw(windat, 0, windat->display_lines - 1, TRANSACT_LIST_WINDOW_PANE_ROW);
}


/**
 * Force the redraw of one or all of the transactions in a given
 * Transaction List window.
 *
 * \param *windat		The transaction window to redraw.
 */

void transact_list_window_redraw(struct transact_list_window *windat, tran_t transaction)
{
	if (windat == NULL)
		return;

	list_window_redraw(windat->window, transaction, 0);
}


/**
 * Update the state of the buttons in a transaction window toolbar.
 *
 * \param *windat		The transaction list window to update.
 */

void transact_list_window_update_toolbar(struct transact_list_window *windat)
{
	struct file_block *file;

//	if (windat == NULL || windat->instance == NULL || windat->transaction_pane == NULL)
//		return;

//	file = transact_get_file(windat->instance);
//	if (file == NULL)
//		return;

//	icons_set_shaded(windat->transaction_pane, TRANSACT_LIST_WINDOW_PANE_VIEWACCT,
//			account_count_type_in_file(file, ACCOUNT_FULL) == 0);
}


/**
 * Bring a transaction window to the top of the window stack.
 *
 * \param *windat		The transaction list window to bring up.
 */

void transact_list_window_bring_to_top(struct transact_list_window *windat)
{
//	if (windat == NULL || windat->transaction_window == NULL)
//		return;

//	windows_open(windat->transaction_window);
}


/**
 * Scroll a transaction window to either the top (home) or the end.
 *
 * \param *windat		The transaction list window to be scrolled.
 * \param direction		The direction to scroll the window in.
 */

void transact_list_window_scroll_to_end(struct transact_list_window *windat, enum transact_scroll_direction direction)
{
	wimp_window_info	window;

//	if (windat == NULL || windat->transaction_window == NULL || direction == TRANSACT_SCROLL_NONE)
//		return;

//	window.w = windat->transaction_window;
//	wimp_get_window_info_header_only(&window);

//	switch (direction) {
//	case TRANSACT_SCROLL_HOME:
//		window.yscroll = window.extent.y1;
//		break;

//	case TRANSACT_SCROLL_END:
//		window.yscroll = window.extent.y0 - (window.extent.y1 - window.extent.y0);
//		break;

//	case TRANSACT_SCROLL_NONE:
//		break;
//	}

//	transact_list_window_minimise_extent(windat);
//	wimp_open_window((wimp_open *) &window);
}


/**
 * Return the transaction number of the transaction nearest to the centre of
 * the visible area of the transaction list window which references a given
 * account.
 *
 * \param *windat		The transaction list window to be searched.
 * \param account		The account to search for.
 * \return			The transaction found, or NULL_TRANSACTION.
 */

int transact_list_window_find_nearest_centre(struct transact_list_window *windat, acct_t account)
{
	struct file_block	*file;
	wimp_window_state	window;
	int			height, i, centre, result;

//	if (windat == NULL || windat->instance == NULL || windat->line_data == NULL ||
//			windat->transaction_window == NULL || account == NULL_ACCOUNT)
//		return NULL_TRANSACTION;

//	file = transact_get_file(windat->instance);
//	if (file == NULL)
//		return NULL_TRANSACTION;

//	window.w = windat->transaction_window;
//	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out
	 * any OS units taken up by part lines.
	 */

//	height = window.visible.y1 - window.visible.y0 - WINDOW_ROW_HEIGHT - TRANSACT_LIST_WINDOW_TOOLBAR_HEIGHT;

	/* Calculate the centre line in the window. If this is greater than
	 * the number of actual tracsactions in the window, reduce it
	 * accordingly.
	 */

//	centre = ((-window.yscroll + WINDOW_ROW_ICON_HEIGHT) / WINDOW_ROW_HEIGHT) + ((height / 2) / WINDOW_ROW_HEIGHT);

//	if (centre >= windat->display_lines)
//		centre = windat->display_lines - 1;

	/* If there are no transactions, we can't return one. */

//	if (centre < 0)
//		return NULL_TRANSACTION;

	/* If the centre transaction is a match, return it. */

//	if (transact_get_from(file, windat->line_data[centre].transaction) == account ||
//			transact_get_to(file, windat->line_data[centre].transaction) == account)
//		return windat->line_data[centre].transaction;

	/* Start searching out from the centre until we find a match or hit
	 * both the start and end of the file.
	 */

	result = NULL_TRANSACTION;
//	i = 1;

//	while (centre + i < windat->display_lines || centre - i >= 0) {
//		if (centre + i < windat->display_lines &&
//				(transact_get_from(file, windat->line_data[centre + i].transaction) == account ||
//			transact_get_to(file, windat->line_data[centre + i].transaction) == account)) {
//			result = windat->line_data[centre + i].transaction;
//			break;
//		}

//		if (centre - i >= 0 &&
//				(transact_get_from(file, windat->line_data[centre - i].transaction) == account ||
//			transact_get_to(file, windat->line_data[centre - i].transaction) == account)) {
//			result = windat->line_data[centre - i].transaction;
//			break;
//		}

//		i++;
//	}

	return result;
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
	if (windat == NULL)
		return -1;

	return list_window_get_line_from_index(windat->window, transaction);
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
	if (windat == NULL)
		return NULL_TRANSACTION;

	return list_window_get_index_from_line(windat->window, line);
}


/**
 * Find the display line number of the current transaction entry line.
 *
 * \param *windat		The transaction list window to interrogate.
 * \return			The display line number of the line with the caret.
 */

int transact_list_window_get_caret_line(struct transact_list_window *windat)
{
	if (windat == NULL)
		return 0;

	return list_window_get_caret_line(windat->window);
}


/**
 * Handle requests for field data from the edit line.
 *
 * \param *data			The block to hold the requested data.
 * \return			TRUE if valid data was returned; FALSE if not.
 */

static osbool transact_list_window_edit_get_field(struct edit_data *data)
{
	tran_t				transaction;
	struct transact_list_window	*windat;
	struct file_block		*file;

	if (data == NULL || data->data == NULL)
		return FALSE;

	windat = data->data;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;
/*
	if (!transact_list_window_line_valid(windat, data->line))
		return FALSE;

	transaction = windat->line_data[data->line].transaction;

	switch (data->icon) {
	case TRANSACT_LIST_WINDOW_ROW:
		if (data->type != EDIT_FIELD_DISPLAY || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		string_printf(data->display.text, data->display.length, "%d", transact_get_transaction_number(transaction));
		break;
	case TRANSACT_LIST_WINDOW_DATE:
		if (data->type != EDIT_FIELD_DATE)
			return FALSE;

		data->date.date = transact_get_date(file, transaction);
		break;
	case TRANSACT_LIST_WINDOW_FROM:
		if (data->type != EDIT_FIELD_ACCOUNT_IN)
			return FALSE;

		data->account.account = transact_get_from(file, transaction);
		data->account.reconciled = (transact_get_flags(file, transaction) & TRANS_REC_FROM) ? TRUE : FALSE;
		break;
	case TRANSACT_LIST_WINDOW_TO:
		if (data->type != EDIT_FIELD_ACCOUNT_OUT)
			return FALSE;

		data->account.account = transact_get_to(file, transaction);
		data->account.reconciled = (transact_get_flags(file, transaction) & TRANS_REC_TO) ? TRUE : FALSE;
		break;
	case TRANSACT_LIST_WINDOW_AMOUNT:
		if (data->type != EDIT_FIELD_CURRENCY)
			return FALSE;

		data->currency.amount = transact_get_amount(file, transaction);
		break;
	case TRANSACT_LIST_WINDOW_REFERENCE:
		if (data->type != EDIT_FIELD_TEXT || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		transact_get_reference(file, transaction, data->text.text, data->text.length);
		break;
	case TRANSACT_LIST_WINDOW_DESCRIPTION:
		if (data->type != EDIT_FIELD_TEXT || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		transact_get_description(file, transaction, data->text.text, data->text.length);
		break;
	}
*/
	return TRUE;
}


/**
 * Handle returned field data from the edit line.
 *
 * \param *data			The block holding the returned data.
 * \return			TRUE if valid data was returned; FALSE if not.
 */

static osbool transact_list_window_edit_put_field(struct edit_data *data)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	tran_t				transaction;

#ifdef DEBUG
	debug_printf("Returning complex data for icon %d in row %d", data->icon, data->line);
#endif

	if (data == NULL)
		return FALSE;

	windat = data->data;
	if (windat == NULL || windat->instance == NULL)
		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;

	/* If there is not a transaction entry for the current edit line location
	 * (ie. if this is the first keypress in a new line), extend the transaction
	 * entries to reach the current location.
	 *
	 * Get out if we failed to create the necessary transactions.
	 */

	if (!transact_list_window_extend_display_lines(windat, data->line))
		return FALSE;

//	transaction = windat->line_data[data->line].transaction;

	/* Process the supplied data.
	 *
	 * During the following operation, forced redraw is disabled for the
	 * window, so that the changes made to the data don't force updates
	 * which then trigger data refetches and further change the edit line.
	 */
/*
	transact_list_window_disable_redraw = TRUE;

	switch (data->icon) {
	case TRANSACT_LIST_WINDOW_DATE:
		transact_change_date(file, transaction, data->date.date);
		break;
	case TRANSACT_LIST_WINDOW_FROM:
		transact_change_account(file, transaction, TRANSACT_FIELD_FROM, data->account.account, data->account.reconciled);

		edit_set_line_colour(windat->edit_line, transact_list_window_line_colour(windat, data->line));
		break;
	case TRANSACT_LIST_WINDOW_TO:
		transact_change_account(file, transaction, TRANSACT_FIELD_TO, data->account.account, data->account.reconciled);

		edit_set_line_colour(windat->edit_line, transact_list_window_line_colour(windat, data->line));
		break;
	case TRANSACT_LIST_WINDOW_AMOUNT:
		transact_change_amount(file, transaction, data->currency.amount);
		break;
	case TRANSACT_LIST_WINDOW_REFERENCE:
		transact_change_refdesc(file, transaction, TRANSACT_FIELD_REF, data->text.text);
		break;
	case TRANSACT_LIST_WINDOW_DESCRIPTION:
		transact_change_refdesc(file, transaction, TRANSACT_FIELD_DESC, data->text.text);
		break;
	}

	transact_list_window_disable_redraw = FALSE;
*/
	/* Finally, look for the next reconcile line if that is necessary.
	 *
	 * This is done last, as the only hold we have over the line being edited
	 * is the edit line location.  Move that and we've lost everything.
	 */

//	if ((data->icon == TRANSACT_LIST_WINDOW_FROM || data->icon == TRANSACT_LIST_WINDOW_TO) &&
//			(data->key == '+' || data->key == '=' || data->key == '-' || data->key == '_'))
//		transact_list_window_find_next_reconcile_line(windat, FALSE);

	return TRUE;
}


/**
 * Process auto-complete requests from the edit line for one of the transaction
 * fields.
 *
 * \param *data			The field auto-complete data.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool transact_list_window_edit_auto_complete(struct edit_data *data)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	tran_t				transaction;

	if (data == NULL)
		return FALSE;

	windat = data->data;
	if (windat == NULL || windat->instance == NULL)
		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("Requesting auto-completion");
#endif

	/* We can only complete text fields at the moment, as none of the others make sense. */

	if (data->type != EDIT_FIELD_TEXT)
		return FALSE;

	/* Process the Reference or Descripton field as appropriate. */

//	switch (data->icon) {
//	case TRANSACT_LIST_WINDOW_REFERENCE:
		/* To complete the reference, we need to be in a valid transaction which contains
		 * at least one of the From or To fields.
		 */

//		if (!transact_list_window_line_valid(windat, data->line))
//			return FALSE;

//		transaction = windat->line_data[data->line].transaction;

//		account_get_next_cheque_number(file, transact_get_from(file, transaction), transact_get_to(file, transaction),
//				1, data->text.text, data->text.length);
//		break;

//	case TRANSACT_LIST_WINDOW_DESCRIPTION:
		/* The description field can be completed whether or not there's an underlying
		 * transaction, as we just search back up from the last valid line.
		 */

//		transact_list_window_complete_description(windat, data->line, data->text.text, data->text.length);
//		break;

//	default:
//		return FALSE;
//	}

	return TRUE;
}


/**
 * Complete a description field, by finding the most recent description in the file
 * which starts with the same characters as the current line.
 *
 * \param *windat	The transaction list window containing the transaction.
 * \param line		The transaction line to be completed.
 * \param *buffer	Pointer to the buffer to be completed.
 * \param length	The length of the buffer.
 * \return		Pointer to the completed buffer.
 */

static char *transact_list_window_complete_description(struct transact_list_window *windat, int line, char *buffer, size_t length)
{
	struct file_block	*file;
	tran_t			transaction;
	char			*description;

	if (windat == NULL || windat->instance == NULL || buffer == NULL)
		return buffer;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;
/*
	if (line >= windat->display_lines)
		line = windat->display_lines - 1;
	else
		line -= 1;

	for (; line >= 0; line--) {
		transaction = windat->line_data[line].transaction;
		description = transact_get_description(file, transaction, NULL, 0);

		if (*description != '\0' && string_nocase_strstr(description, buffer) == description) {
			string_copy(buffer, description, length);
			break;
		}
	}
*/
	return buffer;
}


/**
 * Find the next line of an account, based on its reconcoled status, and place
 * the caret into the unreconciled account field.
 *
 * \param *windat		The transaction window instance to search in.
 * \param set			TRUE to match reconciled lines; FALSE to match unreconciled ones.
 */

static void transact_list_window_find_next_reconcile_line(struct transact_list_window *windat, osbool set)
{
	struct file_block	*file;
	int			line;
	acct_t			account;
	enum transact_field	found;
	wimp_caret		caret;

	if (windat == NULL || windat->instance == NULL || windat->auto_reconcile == FALSE)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;
/*
	line = edit_get_line(windat->edit_line);
	if (!transact_list_window_line_valid(windat, line))
		return;

	account = NULL_ACCOUNT;

	wimp_get_caret_position(&caret);

	if (caret.i == TRANSACT_LIST_WINDOW_FROM)
		account = transact_get_from(file, windat->line_data[line].transaction);
	else if (caret.i == TRANSACT_LIST_WINDOW_TO)
		account = transact_get_to(file, windat->line_data[line].transaction);

	if (account == NULL_ACCOUNT)
		return;

	line++;
	found = TRANSACT_FIELD_NONE;

	while ((line < windat->display_lines) && (found == TRANSACT_FIELD_NONE)) {
		if (transact_get_from(file, windat->line_data[line].transaction) == account &&
				((transact_get_flags(file, windat->line_data[line].transaction) & TRANS_REC_FROM) ==
						((set) ? TRANS_REC_FROM : TRANS_FLAGS_NONE)))
			found = TRANSACT_FIELD_FROM;
		else if (transact_get_to(file, windat->line_data[line].transaction) == account &&
				((transact_get_flags(file, windat->line_data[line].transaction) & TRANS_REC_TO) ==
						((set) ? TRANS_REC_TO : TRANS_FLAGS_NONE)))
			found = TRANSACT_FIELD_TO;
		else
			line++;
	}

	if (found != TRANSACT_FIELD_NONE)
		transact_list_window_place_caret(windat, line, found);*/
}


/**
 * Process preset insertion requests from the edit line.
 *
 * \param line			The line at which to insert the preset.
 * \param key			The Wimp Key number triggering the request.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool transact_list_window_edit_insert_preset(int line, wimp_key_no key, void *data)
{
	struct transact_list_window	*windat;
	struct file_block		*file;
	preset_t			preset;

	if (data == NULL)
		return FALSE;

	windat = data;
	if (windat == NULL || windat->instance == NULL)
		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;

	/* Identify the preset to be inserted. */

	preset = preset_find_from_keypress(file, toupper(key));

	if (preset == NULL_PRESET)
		return TRUE;

	return transact_list_window_insert_preset_into_line(windat, line, preset);
}


/**
 * Insert a preset into a pre-existing transaction, taking care of updating all
 * the file data in a clean way.
 *
 * \param *windat	The window to edit.
 * \param line		The line in the transaction window to update.
 * \param preset	The preset to insert into the transaction.
 * \return		TRUE if successful; FALSE on failure.
 */

osbool transact_list_window_insert_preset_into_line(struct transact_list_window *windat, int line, preset_t preset)
{
	enum transact_field	changed = TRANSACT_FIELD_NONE;
	struct file_block	*file;
	enum transact_flags	flags;
	tran_t			transaction;
	date_t			date;
	acct_t			from, to;
	amt_t			amount;
	char			text[TRANSACT_DESCRIPT_FIELD_LEN]; /* This assumes that the description is longer than the reference. */
	enum sort_type		order;


//	if (windat == NULL || windat->instance == NULL || windat->edit_line == NULL)
//		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL || !preset_test_index_valid(file, preset))
		return FALSE;

	/* Ensure that there is a transaction behind the target line. */

	if (!transact_list_window_extend_display_lines(windat, line))
		return FALSE;

//	transaction = windat->line_data[line].transaction;

	/* Update the transaction from the preset, piece by piece. */

	flags = preset_get_flags(file, preset);

	/* Update the date. */

	date = (flags & TRANS_TAKE_TODAY) ? date_today() : preset_get_date(file, preset);

	if (date != NULL_DATE && transact_change_date(file, transaction, date))
		changed |= TRANSACT_FIELD_DATE;

	/* Update the From account. */

	from = preset_get_from(file, preset);

	if (from != NULL_ACCOUNT && transact_change_account(file, transaction, TRANSACT_FIELD_FROM, from,
				((flags & TRANS_REC_FROM) == TRANS_REC_FROM) ? TRUE : FALSE))
		changed |= TRANSACT_FIELD_FROM;

	/* Update the To account. */

	to = preset_get_to(file, preset);

	if (to != NULL_ACCOUNT && transact_change_account(file, transaction, TRANSACT_FIELD_TO, to,
				((flags & TRANS_REC_TO) == TRANS_REC_TO) ? TRUE : FALSE))
		changed |= TRANSACT_FIELD_TO;

	/* Update the reference. */

	if (flags & TRANS_TAKE_CHEQUE)
		account_get_next_cheque_number(file, from, to, 1, text, TRANSACT_DESCRIPT_FIELD_LEN);
	else
		preset_get_reference(file, preset, text, TRANSACT_DESCRIPT_FIELD_LEN);

	if (*text != '\0' && transact_change_refdesc(file, transaction, TRANSACT_FIELD_REF, text))
		changed |= TRANSACT_FIELD_REF;

	/* Update the amount. */

	amount = preset_get_amount(file, preset);

	if (amount != NULL_CURRENCY && transact_change_amount(file, transaction, amount))
		changed |= TRANSACT_FIELD_AMOUNT;

	/* Update the description. */

	preset_get_description(file, preset, text, TRANSACT_DESCRIPT_FIELD_LEN);

	if (*text != '\0' && transact_change_refdesc(file, transaction, TRANSACT_FIELD_DESC, text))
		changed |= TRANSACT_FIELD_DESC;

	/* Replace the edit line to make it pick up the changes. */

	transact_list_window_place_edit_line(windat, line);

	/* Put the caret at the end of the preset destination field. */

//	icons_put_caret_at_end(windat->transaction_window,
//			transact_list_window_convert_preset_icon_number(preset_get_caret_destination(file, preset)));

	/* If nothing changed, there's no more to do. */

	if (changed == TRANSACT_FIELD_NONE)
		return TRUE;

	/* Force a redraw of the affected line. */

	transact_list_window_force_redraw(windat, line, line, wimp_ICON_WINDOW);

	/* If we're auto-sorting, and the sort column has been updated as
	 * part of the preset, then do an auto sort now.
	 *
	 * We will always sort if the sort column is Date, because pressing
	 * a preset key is analagous to hitting Return.
	 */

//	order = sort_get_order(windat->sort);

//	if (config_opt_read("AutoSort") && (
//			((order & SORT_MASK) == SORT_DATE) ||
//			((changed & TRANSACT_FIELD_FROM) && ((order & SORT_MASK) == SORT_FROM)) ||
//			((changed & TRANSACT_FIELD_TO) && ((order & SORT_MASK) == SORT_TO)) ||
//			((changed & TRANSACT_FIELD_REF) && ((order & SORT_MASK) == SORT_REFERENCE)) ||
//			((changed & TRANSACT_FIELD_AMOUNT) && ((order & SORT_MASK) == SORT_AMOUNT)) ||
//			((changed & TRANSACT_FIELD_DESC) && ((order & SORT_MASK) == SORT_DESCRIPTION)))) {
//		transact_list_window_sort(windat);

//		if (transact_list_window_line_valid(windat, line)) {
//			accview_sort(file, transact_get_from(file, windat->line_data[line].transaction));
//			accview_sort(file, transact_get_to(file, windat->line_data[line].transaction));
//		}
//	}

	return TRUE;
}


/**
 * Take a preset caret destination as used in the preset blocks, and convert it
 * into an icon number for the transaction edit line.
 *
 * \param caret		The preset caret destination to be converted.
 * \return		The corresponding icon number.
 */

static wimp_i transact_list_window_convert_preset_icon_number(enum preset_caret caret)
{
	wimp_i	icon;

	switch (caret) {
	case PRESET_CARET_DATE:
		icon = TRANSACT_LIST_WINDOW_DATE;
		break;

	case PRESET_CARET_FROM:
		icon = TRANSACT_LIST_WINDOW_FROM;
		break;

	case PRESET_CARET_TO:
		icon = TRANSACT_LIST_WINDOW_TO;
		break;

	case PRESET_CARET_REFERENCE:
		icon = TRANSACT_LIST_WINDOW_REFERENCE;
		break;

	case PRESET_CARET_AMOUNT:
		icon = TRANSACT_LIST_WINDOW_AMOUNT;
		break;

	case PRESET_CARET_DESCRIPTION:
		icon = TRANSACT_LIST_WINDOW_DESCRIPTION;
		break;

	default:
		icon = TRANSACT_LIST_WINDOW_DATE;
		break;
	}

	return icon;
}


/**
 * Process auto sort requests from the edit line.
 *
 * \param icon			The icon handle associated with the affected column.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE if successfully handled; else FALSE.
 */

static int transact_list_window_edit_auto_sort(wimp_i icon, void *data)
{
	struct transact_list_window	*windat = data;
	struct file_block		*file;
	int				entry_line;
	enum sort_type			order;

	if (windat == NULL || windat->instance == NULL)
		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("Requesting auto-sort on icon %d", icon);
#endif

	/* Don't do anything if AutoSort is configured off. */

	if (!config_opt_read("AutoSort"))
		return TRUE;

	/* Only sort if the keypress was in the active sort column, as nothing
	 * will be changing otherwise.
	 */

//	order = sort_get_order(windat->sort);

//	if ((icon == TRANSACT_LIST_WINDOW_DATE && (order & SORT_MASK) != SORT_DATE) ||
//			(icon == TRANSACT_LIST_WINDOW_FROM && (order & SORT_MASK) != SORT_FROM) ||
//			(icon == TRANSACT_LIST_WINDOW_TO && (order & SORT_MASK) != SORT_TO) ||
//			(icon == TRANSACT_LIST_WINDOW_REFERENCE && (order & SORT_MASK) != SORT_REFERENCE) ||
//			(icon == TRANSACT_LIST_WINDOW_AMOUNT && (order & SORT_MASK) != SORT_AMOUNT) ||
//			(icon == TRANSACT_LIST_WINDOW_DESCRIPTION && (order & SORT_MASK) != SORT_DESCRIPTION))
//		return TRUE;

	/* Sort the transactions. */

//	transact_list_window_sort(windat);

	/* Re-sort any affected account views. */

//	entry_line = edit_get_line(windat->edit_line);

//	if (transact_list_window_line_valid(windat, entry_line)) {
//		accview_sort(file, transact_get_from(file, windat->line_data[entry_line].transaction));
//		accview_sort(file, transact_get_to(file, windat->line_data[entry_line].transaction));
//	}

	return TRUE;
}


/**
 * Find the Wimp colour of a given line in a transaction window.
 *
 * \param transaction		The transaction index of interest.
 * \param *file			The parent file.
 * \param *data			The transaction window instance of interest.
 * \return			The required Wimp colour, or Black on failure.
 */

static wimp_colour transact_list_window_line_colour(tran_t transaction, struct file_block *file, void *data)
{
	if (config_opt_read("ShadeReconciled") &&
			((transact_get_flags(file, transaction) & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
		return config_int_read("ShadeReconciledColour");
	else
		return wimp_COLOUR_BLACK;
}


/**
 * Extend the active display area of the transaction list window to the
 * specified line (if necessary) by adding blank transaction lines.
 *
 * \param *windat	The transaction list window to extend.
 * \param line		The line to be included in the active range.
 * \return		TRUE on success; FALSE on failure.
 */

static osbool transact_list_window_extend_display_lines(struct transact_list_window *windat, int line)
{
	struct file_block	*file;
	int			start, i;

//	if (windat == NULL || windat->instance == NULL || windat->edit_line == NULL)
//		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;

	/* During the following operation, forced redraw is disabled for the
	 * window, so that the changes made to the data don't force updates
	 * which then trigger data refetches and potentially change the edit line.
	 *
	 * After the raw transactions are added, the Row column is redrawn only.
	 */

//	if (line >= windat->display_lines) {
//		start = windat->display_lines;

//		transact_list_window_disable_redraw = TRUE;

//		for (i = windat->display_lines; i <= line; i++)
//			transact_add_raw_entry(file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

//		transact_list_window_disable_redraw = FALSE;

//		transact_list_window_force_redraw(windat, start, windat->display_lines - 1, TRANSACT_LIST_WINDOW_PANE_ROW);
//	}

	/* If the line is less than the display line count, we succeeded. */

//	return (line < windat->display_lines) ? TRUE : FALSE;
return FALSE;
}


/**
 * Find and return the line number of the first blank line in a file, based on
 * display order.
 *
 * \param *windat		The transaction list window to search.
 * \return			The first blank display line.
 */

int transact_list_window_find_first_blank_line(struct transact_list_window *windat)
{
	if (windat == NULL || windat->window == NULL)
		return 0;

	return list_window_find_first_blank_line(windat->window);
}


/**
 * Test a transaction to see if it is blank.
 *
 * \param transaction		The transaction index to test.
 * \param *file			The file containing the transaction.
 * \param *data			The transaction list window instance.
 * \return			TRUE if the line is considered blank; else FALSE.
 */

static osbool transact_list_window_test_blank(tran_t transaction, struct file_block *file, void *data)
{
	return transact_is_blank(file, transaction);
}


/**
 * Search the transaction list from a file for a set of matching entries.
 *
 * \param *windat		The transaction list window to search in.
 * \param *line			Pointer to the line (under current display sort
 *				order) to search from. Updated on exit to show
 *				the matched line.
 * \param back			TRUE to search back up the file; FALSE to search
 *				down.
 * \param case_sensitive	TRUE to match case in strings; FALSE to ignore.
 * \param logic_and		TRUE to combine the parameters in an AND logic;
 *				FALSE to use an OR logic.
 * \param date			A date to match, or NULL_DATE for none.
 * \param from			A from account to match, or NULL_ACCOUNT for none.
 * \param to			A to account to match, or NULL_ACCOUNT for none.
 * \param flags			Reconcile flags for the from and to accounts, if
 *				these have been specified.
 * \param amount		An amount to match, or NULL_AMOUNT for none.
 * \param *ref			A wildcarded reference to match; NULL or '\0' for none.
 * \param *desc			A wildcarded description to match; NULL or '\0' for none.
 * \return			Transaction field flags set for each matching field;
 *				TRANSACT_FIELD_NONE if no match found.
 */

enum transact_field transact_list_window_search(struct transact_list_window *windat, int *line, osbool back, osbool case_sensitive, osbool logic_and,
		date_t date, acct_t from, acct_t to, enum transact_flags flags, amt_t amount, char *ref, char *desc)
{
	struct file_block	*file;
	enum transact_field	test = TRANSACT_FIELD_NONE, original = TRANSACT_FIELD_NONE;
	osbool			match = FALSE;
	enum transact_flags	from_rec, to_rec;
	int			transaction;

	if (windat == NULL || windat->instance == NULL)
		return TRANSACT_FIELD_NONE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return TRANSACT_FIELD_NONE;

	match = FALSE;

	from_rec = flags & TRANS_REC_FROM;
	to_rec = flags & TRANS_REC_TO;
#if 0
	while (*line < windat->display_lines && *line >= 0 && !match) {
		/* Initialise the test result variable.  The tests all have a bit allocated.  For OR tests, these start unset and
		 * are set if a test passes; a non-zero value at the end indicates a match.  For AND tests, all the required bits
		 * are set at the start and cleared as tests match.  A zero value at the end indicates a match.
		 */

		test = TRANSACT_FIELD_NONE;

		if (logic_and) {
			if (date != NULL_DATE)
				test |= TRANSACT_FIELD_DATE;

			if (from != NULL_ACCOUNT)
				test |= TRANSACT_FIELD_FROM;

			if (to != NULL_ACCOUNT)
				test |= TRANSACT_FIELD_TO;

			if (amount != NULL_CURRENCY)
				test |= TRANSACT_FIELD_AMOUNT;

			if (ref != NULL && *ref != '\0')
				test |= TRANSACT_FIELD_REF;

			if (desc != NULL && *desc != '\0')
				test |= TRANSACT_FIELD_DESC;
		}

		original = test;

		/* Perform the tests.
		 *
		 * \TODO -- The order of these tests could be optimised, to
		 *          skip things that we don't need to bother matching.
		 */

		transaction = windat->line_data[*line].transaction;

		if (desc != NULL && *desc != '\0' && string_wildcard_compare(desc, transact_get_description(file, transaction, NULL, 0), !case_sensitive)) {
			test ^= TRANSACT_FIELD_DESC;
		}

		if (amount != NULL_CURRENCY && amount == transact_get_amount(file, transaction)) {
			test ^= TRANSACT_FIELD_AMOUNT;
		}

		if (ref != NULL && *ref != '\0' && string_wildcard_compare(ref, transact_get_reference(file, transaction, NULL, 0), !case_sensitive)) {
			test ^= TRANSACT_FIELD_REF;
		}

		/* The following two tests check that a) an account has been specified, b) it is the same as the transaction and
		 * c) the two reconcile flags are set the same (if they are, the EOR operation cancels them out).
		 */

		if (to != NULL_ACCOUNT && to == transact_get_to(file, transaction) &&
				((to_rec ^ transact_get_flags(file, transaction)) & TRANS_REC_TO) == 0) {
			test ^= TRANSACT_FIELD_TO;
		}

		if (from != NULL_ACCOUNT && from == transact_get_from(file, transaction) &&
				((from_rec ^ transact_get_flags(file, transaction)) & TRANS_REC_FROM) == 0) {
			test ^= TRANSACT_FIELD_FROM;
		}

		if (date != NULL_DATE && date == transact_get_date(file, transaction)) {
			test ^= TRANSACT_FIELD_DATE;
		}

		/* Check if the test passed or failed. */

		if ((!logic_and && test) || (logic_and && !test)) {
			match = TRUE;
		} else {
			if (back)
				(*line)--;
			else
				(*line)++;
		}
	}
#endif
	if (!match)
		return TRANSACT_FIELD_NONE;

	return (logic_and) ? original : test;
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
	list_window_open_print_window(windat->window, ptr, restore);
}


/**
 * Determine whether to include a transaction in a print job.
 * 
 * \param *file			The file owning the transaction list.
 * \param transaction		The transaction to be output.
 * \param from			The earliest date to be included, or NULL_DATE.
 * \param to			The latest date to be included, or NULL_DATE.
 * \return			TRUE to include the transaction; FALSE to omit it.
 */

static osbool transact_list_window_print_include(struct file_block *file, tran_t transaction, date_t from, date_t to)
{
	date_t date;

	date = transact_get_date(file, transaction);

	return !((from != NULL_DATE && date < from) || (to != NULL_DATE && date > to));
}


/**
 * Send the contents of the Standing Order Window to the printer, via the reporting
 * system.
 *
 * \param *file			The file owning the transaction list.
 * \param column		The column to be output.
 * \param transaction		The transaction to be output.
 * \param *rec_char		A string to use as the reconcile character.
 */

static struct report *transact_list_window_print_field(struct file_block *file, wimp_i column, tran_t transaction, char *rec_char)
{
	switch (column) {
	case TRANSACT_LIST_WINDOW_ROW:
		stringbuild_add_printf("\\v\\d\\r%d", transact_get_transaction_number(transaction));
		break;
	case TRANSACT_LIST_WINDOW_DATE:
		stringbuild_add_string("\\v\\c");
		stringbuild_add_date(transact_get_date(file, transaction));
		break;
	case TRANSACT_LIST_WINDOW_FROM:
		stringbuild_add_string(account_get_ident(file, transact_get_from(file, transaction)));
		break;
	case TRANSACT_LIST_WINDOW_FROM_REC:
		if (transact_get_flags(file, transaction) & TRANS_REC_FROM)
			stringbuild_add_string(rec_char);
		break;
	case TRANSACT_LIST_WINDOW_FROM_NAME:
		stringbuild_add_string("\\v");
		stringbuild_add_string(account_get_name(file, transact_get_from(file, transaction)));
		break;
	case TRANSACT_LIST_WINDOW_TO:
		stringbuild_add_string(account_get_ident(file, transact_get_to(file, transaction)));
		break;
	case TRANSACT_LIST_WINDOW_TO_REC:
		if (transact_get_flags(file, transaction) & TRANS_REC_TO)
			stringbuild_add_string(rec_char);
		break;
	case TRANSACT_LIST_WINDOW_TO_NAME:
		stringbuild_add_string("\\v");
		stringbuild_add_string(account_get_name(file, transact_get_to(file, transaction)));
		break;
	case TRANSACT_LIST_WINDOW_REFERENCE:
		stringbuild_add_string("\\v");
		stringbuild_add_string(transact_get_reference(file, transaction, NULL, 0));
		break;
	case TRANSACT_LIST_WINDOW_AMOUNT:
		stringbuild_add_string("\\v\\d\\r");
		stringbuild_add_currency(transact_get_amount(file, transaction), FALSE);
		break;
	case TRANSACT_LIST_WINDOW_DESCRIPTION:
		stringbuild_add_string("\\v");
		stringbuild_add_string(transact_get_description(file, transaction, NULL, 0));
		break;
	default:
		stringbuild_add_string("\\s");
		break;
	}
}


/**
 * Sort the contents of the transaction list window based on the instance's
 * sort setting.
 *
 * \param *windat		The transaction window instance to sort.
 */

void transact_list_window_sort(struct transact_list_window *windat)
{
	if (windat == NULL)
		return;

	list_window_sort(windat->window);
}


/**
 * Compare two lines of a transaction list, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The index of the first line to be compared.
 * \param index2		The index of the second line to be compared.
 * \param *file			The file relating to the data being sorted.
 * \return			The comparison result.
 */

static int transact_list_window_sort_compare(enum sort_type type, int index1, int index2, struct file_block *file)
{
	switch (type) {
	case SORT_ROW:
		return (transact_get_transaction_number(index1) -
				transact_get_transaction_number(index2));

	case SORT_DATE:
		return ((transact_get_date(file, index1) & DATE_SORT_MASK) -
				(transact_get_date(file, index2) & DATE_SORT_MASK));

	case SORT_FROM:
		return strcmp(account_get_name(file, transact_get_from(file, index1)),
				account_get_name(file, transact_get_from(file, index2)));

	case SORT_TO:
		return strcmp(account_get_name(file, transact_get_to(file, index1)),
				account_get_name(file, transact_get_to(file, index2)));

	case SORT_REFERENCE:
		return strcmp(transact_get_reference(file, index1, NULL, 0),
				transact_get_reference(file, index2, NULL, 0));

	case SORT_AMOUNT:
		return (transact_get_amount(file, index1) -
				transact_get_amount(file, index2));

	case SORT_DESCRIPTION:
		return strcmp(transact_get_description(file, index1, NULL, 0),
				transact_get_description(file, index2, NULL, 0));

	default:
		return 0;
	}
}


/**
 * Initialise the contents of the transaction list window, creating an
 * entry for each of the required transactions.
 *
 * \param *windat		The transaction list window instance to initialise.
 * \param transacts		The number of transactionss to insert.
 * \return			TRUE on success; FALSE on failure.
 */

osbool transact_list_window_initialise_entries(struct transact_list_window *windat, int transacts)
{
	if (windat == NULL)
		return FALSE;

	return list_window_initialise_entries(windat->window, transacts);
}


/**
 * Add a new transaction to an instance of the transaction list window.
 *
 * \param *windat		The transaction list window instance to add to.
 * \param transaction		The transaction index to add.
 * \return			TRUE on success; FALSE on failure.
 */

osbool transact_list_window_add_transaction(struct transact_list_window *windat, tran_t transaction)
{
	if (windat == NULL)
		return FALSE;

	return list_window_add_entry(windat->window, transaction, FALSE);
}


/**
 * Remove a transaction from an instance of the transaction list window,
 * and update the other entries to allow for its deletion.
 *
 * \param *windat		The transaction list window instance to remove from.
 * \param transaction		The transaction index to remove.
 * \return			TRUE on success; FALSE on failure.
 */

osbool transact_list_window_delete_transaction(struct transact_list_window *windat, tran_t transaction)
{
	if (windat == NULL)
		return FALSE;

	return list_window_delete_entry(windat->window, transaction, FALSE);
}


/**
 * Save the transaction list window details from a window to a CashBook
 * file. This assumes that the caller has already created a suitable section
 * in the file to be written.
 *
 * \param *windat		The window whose details to write.
 * \param *out			The file handle to write to.
 */

void transact_list_window_write_file(struct transact_list_window *windat, FILE *out)
{
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (windat == NULL)
		return;

	list_window_write_file(windat->window, out);
}


/**
 * Process a WinColumns line from the Transactions section of a file.
 *
 * \param *windat		The window being read in to.
 * \param format		The format of the disc file.
 * \param *columns		The column text line.
 */

void transact_list_window_read_file_wincolumns(struct transact_list_window *windat, int format, char *columns)
{
	if (windat == NULL)
		return;

	/* For file format 1.00 or older, there's no row column at the
	 * start of the line so skip on to colunn 1 (date).
	 */

	list_window_read_file_wincolumns(windat->window, (format <= 100) ? 1 : 0, TRUE, columns);
}


/**
 * Process a SortOrder line from the Transactions section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The sort order text line.
 */

void transact_list_window_read_file_sortorder(struct transact_list_window *windat, char *order)
{
	if (windat == NULL)
		return;

	list_window_read_file_sortorder(windat->window, order);
}


/**
 * Save a file directly, if it already has a filename associated with it, or
 * open a save dialogue.
 *
 * \param *windat		The window to save.
 */

static void transact_list_window_start_direct_save(struct transact_list_window *windat)
{
	struct file_block	*file;
	wimp_pointer		pointer;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return;

	if (file_check_for_filepath(file)) {
		filing_save_cashbook_file(file, file->filename);
	} else {
		wimp_get_pointer_info(&pointer);

		saveas_initialise_dialogue(transact_list_window_saveas_file, NULL, "DefTransFile", NULL, FALSE, FALSE, windat);
		saveas_prepare_dialogue(transact_list_window_saveas_file);
		saveas_open_dialogue(transact_list_window_saveas_file, &pointer);
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
	struct transact_list_window	*windat = data;
	struct file_block		*file;

	if (windat == NULL || windat->instance == NULL)
		return FALSE;

	file = transact_get_file(windat->instance);
	if (file == NULL)
		return FALSE;

	filing_save_cashbook_file(file, filename);

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

	list_window_export_delimited(windat->window, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

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

	list_window_export_delimited(windat->window, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

	return TRUE;
}


/**
 * Export the standing order data from a file into CSV or TSV format.
 *
 * \param *windat		The standing order window to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void transact_list_window_export_delimited_line(FILE *out, enum filing_delimit_type format, struct file_block *file, int index)
{
	char			buffer[FILING_DELIMITED_FIELD_LEN];

	string_printf(buffer, FILING_DELIMITED_FIELD_LEN, "%d", transact_get_transaction_number(index));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

	date_convert_to_string(transact_get_date(file, index), buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

	account_build_name_pair(file, transact_get_from(file, index), buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

	account_build_name_pair(file, transact_get_to(file, index), buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

	filing_output_delimited_field(out, transact_get_reference(file, index, NULL, 0), format, DELIMIT_NONE);

	currency_convert_to_string(transact_get_amount(file, index), buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

	filing_output_delimited_field(out, transact_get_description(file, index, NULL, 0), format, DELIMIT_LAST);
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

