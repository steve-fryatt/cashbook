/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: transact.c
 *
 * Transaction and transaction window implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
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
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "caret.h"
#include "clipboard.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "find.h"
#include "goto.h"
#include "mainmenu.h"
#include "presets.h"
#include "printing.h"
#include "purge.h"
#include "report.h"
#include "sorder.h"
#include "sort.h"
#include "transact.h"
#include "window.h"

/* Main Window Icons
 *
 * Note that these correspond to column numbers.
 */

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

/* Toolbar icons */

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

/* Main Menu */

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

#define TRANS_SORT_OK 2
#define TRANS_SORT_CANCEL 3
#define TRANS_SORT_DATE 4
#define TRANS_SORT_FROM 5
#define TRANS_SORT_TO 6
#define TRANS_SORT_REFERENCE 7
#define TRANS_SORT_AMOUNT 8
#define TRANS_SORT_DESCRIPTION 9
#define TRANS_SORT_ASCENDING 10
#define TRANS_SORT_DESCENDING 11
#define TRANS_SORT_ROW 12

#define FILEINFO_ICON_FILENAME 1
#define FILEINFO_ICON_MODIFIED 3
#define FILEINFO_ICON_DATE 5
#define FILEINFO_ICON_ACCOUNTS 9
#define FILEINFO_ICON_TRANSACT 11
#define FILEINFO_ICON_HEADINGS 13
#define FILEINFO_ICON_SORDERS 15
#define FILEINFO_ICON_PRESETS 17

/* Transaction Window details. */

#define TRANSACT_COLUMNS 11
#define TRANSACT_TOOLBAR_HEIGHT 132
#define MIN_TRANSACT_ENTRIES 10
#define MIN_TRANSACT_BLANK_LINES 1
#define TRANSACTION_WINDOW_OPEN_OFFSET 48
#define TRANSACTION_WINDOW_OFFSET_LIMIT 8

/* Transaction Window column mapping. */

static struct column_map transact_columns[] = {
	{TRANSACT_ICON_ROW, TRANSACT_PANE_ROW, wimp_ICON_WINDOW},
	{TRANSACT_ICON_DATE, TRANSACT_PANE_DATE, wimp_ICON_WINDOW},
	{TRANSACT_ICON_FROM, TRANSACT_PANE_FROM, wimp_ICON_WINDOW},
	{TRANSACT_ICON_FROM_REC, TRANSACT_PANE_FROM, wimp_ICON_WINDOW},
	{TRANSACT_ICON_FROM_NAME, TRANSACT_PANE_FROM, wimp_ICON_WINDOW},
	{TRANSACT_ICON_TO, TRANSACT_PANE_TO, wimp_ICON_WINDOW},
	{TRANSACT_ICON_TO_REC, TRANSACT_PANE_TO, wimp_ICON_WINDOW},
	{TRANSACT_ICON_TO_NAME, TRANSACT_PANE_TO, wimp_ICON_WINDOW},
	{TRANSACT_ICON_REFERENCE, TRANSACT_PANE_REFERENCE, wimp_ICON_WINDOW},
	{TRANSACT_ICON_AMOUNT, TRANSACT_PANE_AMOUNT, wimp_ICON_WINDOW},
	{TRANSACT_ICON_DESCRIPTION, TRANSACT_PANE_DESCRIPTION, wimp_ICON_WINDOW}
};

/**
 * Transatcion data structure
 */

struct transaction {
	date_t			date;						/**< The date of the transaction.							*/
	enum transact_flags	flags;						/**< The flags applying to the transaction.						*/
	acct_t			from;						/**< The account from which money is being transferred.					*/
	acct_t			to;						/**< The account to which money is being transferred.					*/
	amt_t			amount;						/**< The amount of money transferred by the transaction.				*/
	char			reference[TRANSACT_REF_FIELD_LEN];		/**< The transaction reference text.							*/
	char			description[TRANSACT_DESCRIPT_FIELD_LEN];	/**< The transaction description text.							*/

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the transaction data, and are in effect a separate array that is used
	 * for handling entries in the transaction window.
	 */

	int			sort_index;					/**< Point to another transaction, to allow the transaction window to be sorted.	*/
	int			saved_sort;					/**< Preserve the transaction window sort order across transaction data sorts.		*/
	int			sort_workspace;					/**< Workspace used by the sorting code.						*/
};


/**
 * Transatcion Window data structure
 */

struct transact_block {
	struct file_block	*file;						/**< The file to which the window belongs.				*/

	/* Transactcion window handle and title details. */

	wimp_w			transaction_window;				/**< Window handle of the transaction window */
	char			window_title[256];
	wimp_w			transaction_pane;				/**< Window handle of the transaction window toolbar pane */

	/* Edit line details. */

	struct edit_block	*edit_line;					/**< Instance handle of the edit line.					*/

	/* Display column details. */

	struct column_block	*columns;					/**< Instance handle of the column definitions.				*/

	/* Other window information. */

	int			display_lines;					/**< How many lines the current work area is formatted to display. */
	enum sort_type		sort_order;					/**< The order in which the window is sorted. */

	char			sort_sprite[COLUMN_SORT_SPRITE_LEN];		/**< Space for the sort icon's indirected data. */

	/* Transaction Data. */

	struct transaction	*transactions;					/**< The transaction data for the defined transactions			*/
	int			trans_count;					/**< The number of transactions defined in the file.			*/

};

/* The following are the buffers used by the edit line in the transaction window. */

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


struct transact_list_link {
	char	name[TRANSACT_DESCRIPT_FIELD_LEN]; /* This assumes that references are always shorter than descriptions...*/
};



static int    new_transaction_window_offset = 0;
static wimp_i transaction_pane_sort_substitute_icon = TRANSACT_PANE_DATE;

/* Transaction Window. */

static struct edit_callback	transact_edit_callbacks;			/**< Callback details for the edit line instances.					*/

/* Transaction Sort Window. */

static struct sort_block	*transact_sort_dialogue = NULL;			/**< The transaction window sort dialogue box.						*/

static struct sort_icon transact_sort_columns[] = {				/**< Details of the sort dialogue column icons.						*/
	{TRANS_SORT_ROW, SORT_ROW},
	{TRANS_SORT_DATE, SORT_DATE},
	{TRANS_SORT_FROM, SORT_FROM},
	{TRANS_SORT_TO, SORT_TO},
	{TRANS_SORT_REFERENCE, SORT_REFERENCE},
	{TRANS_SORT_AMOUNT, SORT_AMOUNT},
	{TRANS_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{0, SORT_NONE}
};

static struct sort_icon transact_sort_directions[] = {				/**< Details of the sort dialogue direction icons.					*/
	{TRANS_SORT_ASCENDING, SORT_ASCENDING},
	{TRANS_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/* Transaction Print Window. */

static struct transact_block	*transact_print_owner = NULL;			/**< The instance currently owning the transaction print window.			*/

/* File Info Window. */

static wimp_w			transact_fileinfo_window = NULL;		/**< The handle of the file info window.						*/

/* Transaction List Window. */

static wimp_window		*transact_window_def = NULL;			/**< The definition for the Transaction List Window.					*/
static wimp_window		*transact_pane_def = NULL;			/**< The definition for the Transaction List Toolbar pane.				*/
static wimp_menu		*transact_window_menu = NULL;			/**< The Transaction List Window menu handle.						*/
static wimp_menu		*transact_window_menu_account = NULL;		/**< The Transaction List Window Account submenu handle.				*/
static wimp_menu		*transact_window_menu_transact = NULL;		/**< The Transaction List Window Transaction submenu handle.				*/
static wimp_menu		*transact_window_menu_analysis = NULL;		/**< The Transaction List Window Analysis submenu handle.				*/
static int			transact_window_menu_line = -1;			/**< The line over which the Transaction List Window Menu was opened.			*/

static wimp_menu		*transact_account_list_menu = NULL;		/**< The toolbar's Account List popup menu handle.					*/

static wimp_menu			*transact_complete_menu = NULL;		/**< The Reference/Description List menu block.						*/
static struct transact_list_link	*transact_complete_menu_link = NULL;	/**< Links for the Reference/Description List menu.					*/
static char				*transact_complete_menu_title = NULL;	/**< Reference/Description List menu title buffer.					*/
static struct file_block		*transact_complete_menu_file = NULL;	/**< The file to which the Reference/Description List menu is currently attached.	*/
static enum transact_list_menu_type	transact_complete_menu_type = REFDESC_MENU_NONE;

/* SaveAs Dialogue Handles. */

static struct saveas_block	*transact_saveas_file = NULL;			/**< The Save File saveas data handle.					*/
static struct saveas_block	*transact_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*transact_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/


static void			transact_delete_window(struct transact_block *windat);
static void			transact_window_open_handler(wimp_open *open);
static void			transact_window_close_handler(wimp_close *close);
static void			transact_window_click_handler(wimp_pointer *pointer);
static void			transact_window_lose_caret_handler(wimp_caret *caret);
static void			transact_pane_click_handler(wimp_pointer *pointer);
static osbool			transact_window_keypress_handler(wimp_key *key);
static void			transact_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void			transact_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void			transact_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void			transact_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void			transact_window_scroll_handler(wimp_scroll *scroll);
static void			transact_window_redraw_handler(wimp_draw *redraw);
static void			transact_adjust_window_columns(void *data, wimp_i icon, int width);
static void			transact_adjust_sort_icon(struct transact_block *windat);
static void			transact_adjust_sort_icon_data(struct transact_block *windat, wimp_icon *icon);
static void			transact_force_window_redraw(struct transact_block *windat, int from, int to);
static void			transact_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

static void			transact_complete_menu_add_entry(struct transact_list_link **entries, int *count, int *max, char *new);
static int			transact_complete_menu_compare(const void *va, const void *vb);

static osbool			transact_is_blank(struct file_block *file, tran_t transaction);
static tran_t			transact_find_edit_line_by_transaction(struct transact_block *windat);
static void			transact_place_edit_line_by_transaction(struct transact_block *windat, tran_t transaction);
static osbool			transact_edit_place_line(int line, void *data);
static void			transact_place_edit_line(struct transact_block *windat, int line);
static void			transact_find_edit_line_vertically(struct transact_block *windat);
static osbool			transact_edit_find_field(int line, int xmin, int xmax, enum edit_align target, void *data);
static int			transact_edit_first_blank_line(void *data);
static osbool			transact_edit_test_line(int line, void *data);
static osbool			transact_edit_get_field(struct edit_data *data);
static osbool			transact_edit_put_field(struct edit_data *data);
static void			transact_find_next_reconcile_line(struct transact_block *windat, osbool set);
static osbool			transact_edit_auto_complete(struct edit_data *data);
static char			*transact_complete_description(struct file_block *file, int line, char *buffer, size_t length);
static osbool			transact_edit_insert_preset(int line, wimp_key_no key, void *data);
static wimp_i			transact_convert_preset_icon_number(enum preset_caret caret);
static int			transact_edit_auto_sort(wimp_i icon, void *data);
static wimp_colour		transact_line_colour(struct transact_block *windat, int line);

static void			transact_open_sort_window(struct transact_block *windat, wimp_pointer *ptr);
static osbool			transact_process_sort_window(enum sort_type order, void *data);
static void			transact_open_print_window(struct file_block *file, wimp_pointer *ptr, int clear);
static void			transact_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum, date_t from, date_t to);

static void			transact_start_direct_save(struct transact_block *windat);
static osbool			transact_save_file(char *filename, osbool selection, void *data);
static osbool			transact_save_csv(char *filename, osbool selection, void *data);
static osbool			transact_save_tsv(char *filename, osbool selection, void *data);
static void			transact_export_delimited(struct transact_block *windat, char *filename, enum filing_delimit_type format, int filetype);
static osbool			transact_load_csv(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data);

static void			transact_prepare_fileinfo(struct file_block *file);


/**
 * Test whether a transaction number is safe to look up in the transaction data array.
 */

#define transact_valid(windat, transaction) (((transaction) != NULL_TRANSACTION) && ((transaction) >= 0) && ((transaction) < ((windat)->trans_count)))


/**
 * Initialise the transaction system.
 *
 * \param *sprites		The application sprite area.
 */

void transact_initialise(osspriteop_area *sprites)
{
	wimp_w	sort_window;

	sort_window = templates_create_window("SortTrans");
	ihelp_add_window(sort_window, "SortTrans", NULL);
	transact_sort_dialogue = sort_create_dialogue(sort_window, transact_sort_columns, transact_sort_directions,
			TRANS_SORT_OK, TRANS_SORT_CANCEL, transact_process_sort_window);

	transact_fileinfo_window = templates_create_window("FileInfo");
	ihelp_add_window(transact_fileinfo_window, "FileInfo", NULL);
	templates_link_menu_dialogue("file_info", transact_fileinfo_window);

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

	transact_edit_callbacks.get_field = transact_edit_get_field;
	transact_edit_callbacks.put_field = transact_edit_put_field;
	transact_edit_callbacks.test_line = transact_edit_test_line;
	transact_edit_callbacks.place_line = transact_edit_place_line;
	transact_edit_callbacks.find_field = transact_edit_find_field;
	transact_edit_callbacks.first_blank_line = transact_edit_first_blank_line;
	transact_edit_callbacks.auto_sort = transact_edit_auto_sort;
	transact_edit_callbacks.auto_complete = transact_edit_auto_complete;
	transact_edit_callbacks.insert_preset = transact_edit_insert_preset;
}


/**
 * Create a new transaction window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct transact_block *transact_create_instance(struct file_block *file)
{
	struct transact_block	*new;

	new = heap_alloc(sizeof(struct transact_block));
	if (new == NULL)
		return NULL;

	/* Initialise the transaction window. */

	new->file = file;

	new->transaction_window = NULL;
	new->transaction_pane = NULL;
	new->edit_line = NULL;
	new->columns = NULL;

	new->columns = column_create_instance(TRANSACT_COLUMNS, transact_columns, NULL);
	if (new->columns == NULL) {
		transact_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read("LimTransactCols"));
	column_init_window(new->columns, 0, FALSE, config_str_read("TransactCols"));

	new->sort_order = SORT_DATE | SORT_ASCENDING;

	/* Initialise the transaction data. */

	new->trans_count = 0;
	new->transactions = NULL;

	if (flex_alloc((flex_ptr) &(new->transactions), 4) == 0) {
		transact_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete a transaction window instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void transact_delete_instance(struct transact_block *windat)
{
	if (windat == NULL)
		return;

	transact_delete_window(windat);

	column_delete_instance(windat->columns);

	if (windat->transactions != NULL)
		flex_free((flex_ptr) &(windat->transactions));

	heap_free(windat);
}


/**
 * Create and open a Transaction List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void transact_open_window(struct file_block *file)
{
	int		height;
	os_error	*error;

	if (file == NULL || file->transacts == NULL)
		return;

	if (file->transacts->transaction_window != NULL) {
		windows_open(file->transacts->transaction_window);
		return;
	}

	/* Set the default values */

	file->transacts->display_lines = (file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
			file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;

	/* Create the new window data and build the window. */

	*(file->transacts->window_title) = '\0';
	transact_window_def->title_data.indirected_text.text = file->transacts->window_title;

	height =  file->transacts->display_lines;

	set_initial_window_area(transact_window_def, column_get_window_width(file->transacts->columns),
			(height * WINDOW_ROW_HEIGHT) + TRANSACT_TOOLBAR_HEIGHT,
			-1, -1, new_transaction_window_offset * TRANSACTION_WINDOW_OPEN_OFFSET);

	error = xwimp_create_window(transact_window_def, &(file->transacts->transaction_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	new_transaction_window_offset++;
	if (new_transaction_window_offset >= TRANSACTION_WINDOW_OFFSET_LIMIT)
		new_transaction_window_offset = 0;

	/* Create the toolbar pane. */

	windows_place_as_toolbar(transact_window_def, transact_pane_def, TRANSACT_TOOLBAR_HEIGHT-4);
	columns_place_heading_icons(file->transacts->columns, transact_pane_def);

	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) file->transacts->sort_sprite;
	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			transact_pane_def->sprite_area;
	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

	transact_adjust_sort_icon_data(file->transacts, &(transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON]));

	error = xwimp_create_window(transact_pane_def, &(file->transacts->transaction_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Construct the edit line. */

	file->transacts->edit_line = edit_create_instance(file, transact_window_def, file->transacts->transaction_window,
			file->transacts->columns, TRANSACT_TOOLBAR_HEIGHT,
			&transact_edit_callbacks, file->transacts);
	if (file->transacts->edit_line == NULL) {
		error_msgs_report_error("TransactNoMem");
		return;
	}

	edit_add_field(file->transacts->edit_line, EDIT_FIELD_DISPLAY, 0,
			TRANSACT_ICON_ROW, transact_buffer_row, TRANSACT_ROW_FIELD_LEN);
	edit_add_field(file->transacts->edit_line, EDIT_FIELD_DATE, 1,
			TRANSACT_ICON_DATE, transact_buffer_date, DATE_FIELD_LEN);
	edit_add_field(file->transacts->edit_line, EDIT_FIELD_ACCOUNT_IN, 2,
			TRANSACT_ICON_FROM, transact_buffer_from_ident, ACCOUNT_IDENT_LEN,
			TRANSACT_ICON_FROM_REC, transact_buffer_from_rec, REC_FIELD_LEN,
			TRANSACT_ICON_FROM_NAME, transact_buffer_from_name, ACCOUNT_NAME_LEN);
	edit_add_field(file->transacts->edit_line, EDIT_FIELD_ACCOUNT_OUT, 5,
			TRANSACT_ICON_TO, transact_buffer_to_ident, ACCOUNT_IDENT_LEN,
			TRANSACT_ICON_TO_REC, transact_buffer_to_rec, REC_FIELD_LEN,
			TRANSACT_ICON_TO_NAME, transact_buffer_to_name, ACCOUNT_NAME_LEN);
	edit_add_field(file->transacts->edit_line, EDIT_FIELD_TEXT, 8,
			TRANSACT_ICON_REFERENCE, transact_buffer_reference, TRANSACT_REF_FIELD_LEN);
	edit_add_field(file->transacts->edit_line, EDIT_FIELD_CURRENCY, 9,
			TRANSACT_ICON_AMOUNT, transact_buffer_amount, AMOUNT_FIELD_LEN);
	edit_add_field(file->transacts->edit_line, EDIT_FIELD_TEXT, 10,
			TRANSACT_ICON_DESCRIPTION, transact_buffer_description, TRANSACT_DESCRIPT_FIELD_LEN);

	if (!edit_complete(file->transacts->edit_line)) {
		edit_delete_instance(file->transacts->edit_line);
		error_msgs_report_error("TransactNoMem");
		return;
	}

	/* Set the title */

	transact_build_window_title(file);

	/* Update the toolbar */

	transact_update_toolbar(file);


	/* Open the window. */

	windows_open(file->transacts->transaction_window);
	windows_open_nested_as_toolbar(file->transacts->transaction_pane,
			file->transacts->transaction_window,
			TRANSACT_TOOLBAR_HEIGHT-4);

	ihelp_add_window(file->transacts->transaction_window , "Transact", transact_decode_window_help);
	ihelp_add_window(file->transacts->transaction_pane , "TransactTB", NULL);

	/* Register event handlers for the two windows. */
	/* \TODO -- Should this be all three windows?   */

	event_add_window_user_data(file->transacts->transaction_window, file->transacts);
	event_add_window_menu(file->transacts->transaction_window, transact_window_menu);
	event_add_window_open_event(file->transacts->transaction_window, transact_window_open_handler);
	event_add_window_close_event(file->transacts->transaction_window, transact_window_close_handler);
	event_add_window_lose_caret_event(file->transacts->transaction_window, transact_window_lose_caret_handler);
	event_add_window_mouse_event(file->transacts->transaction_window, transact_window_click_handler);
	event_add_window_key_event(file->transacts->transaction_window, transact_window_keypress_handler);
	event_add_window_scroll_event(file->transacts->transaction_window, transact_window_scroll_handler);
	event_add_window_redraw_event(file->transacts->transaction_window, transact_window_redraw_handler);
	event_add_window_menu_prepare(file->transacts->transaction_window, transact_window_menu_prepare_handler);
	event_add_window_menu_selection(file->transacts->transaction_window, transact_window_menu_selection_handler);
	event_add_window_menu_warning(file->transacts->transaction_window, transact_window_menu_warning_handler);
	event_add_window_menu_close(file->transacts->transaction_window, transact_window_menu_close_handler);

	event_add_window_user_data(file->transacts->transaction_pane, file->transacts);
	event_add_window_menu(file->transacts->transaction_pane, transact_window_menu);
	event_add_window_mouse_event(file->transacts->transaction_pane, transact_pane_click_handler);
	event_add_window_menu_prepare(file->transacts->transaction_pane, transact_window_menu_prepare_handler);
	event_add_window_menu_selection(file->transacts->transaction_pane, transact_window_menu_selection_handler);
	event_add_window_menu_warning(file->transacts->transaction_pane, transact_window_menu_warning_handler);
	event_add_window_menu_close(file->transacts->transaction_pane, transact_window_menu_close_handler);
	event_add_window_icon_popup(file->transacts->transaction_pane, TRANSACT_PANE_VIEWACCT, transact_account_list_menu, -1, NULL);

	dataxfer_set_drop_target(dataxfer_TYPE_CSV, file->transacts->transaction_window, -1, NULL, transact_load_csv, file);
	dataxfer_set_drop_target(dataxfer_TYPE_CSV, file->transacts->transaction_pane, -1, NULL, transact_load_csv, file);

	/* Put the caret into the first empty line. */

	transact_place_caret(file, file->transacts->trans_count, TRANSACT_FIELD_DATE);
}


/**
 * Close and delete a Transaction List Window associated with the given
 * transaction window block.
 *
 * \param *windat		The window to delete.
 */

static void transact_delete_window(struct transact_block *windat)
{
	#ifdef DEBUG
	debug_printf("\\RDeleting transaction window");
	#endif

	if (windat == NULL)
		return;

	sort_close_dialogue(transact_sort_dialogue, windat);

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
}


/**
 * Handle Open events on Transaction List windows, to adjust the extent.
 *
 * \param *open			The Wimp Open data block.
 */

static void transact_window_open_handler(wimp_open *open)
{
	struct transact_block	*windat;

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

static void transact_window_close_handler(wimp_close *close)
{
	struct transact_block	*windat;
	wimp_pointer		pointer;
	char			buffer[1024], *pathcopy;

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
			snprintf(buffer, sizeof(buffer), "%%Filer_OpenDir %s", string_find_pathname(pathcopy));
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

static void transact_window_click_handler(wimp_pointer *pointer)
{
	struct transact_block	*windat;
	struct file_block	*file;
	int			line, transaction, xpos;
	wimp_i			column;
	wimp_window_state	window;
	wimp_pointer		ptr;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* Force a refresh of the current edit line, if there is one.  We avoid refreshing the icon where the mouse
	 * was clicked.
	 */

	edit_refresh_line_contents(NULL, -1, pointer->i);

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
			icons_put_caret_at_end (pointer->w, TRANSACT_ICON_FROM);
		} else if (pointer->i == TRANSACT_ICON_TO_REC || pointer->i == TRANSACT_ICON_TO_NAME) {
			icons_put_caret_at_end (pointer->w, TRANSACT_ICON_TO);
		}
	}

	if (pointer->buttons == wimp_CLICK_ADJUST) {
		/* Adjust clicks don't care about icons, as we only need to know which line and column we're in. */

		window.w = pointer->w;
		wimp_get_window_state(&window);

		line = window_calculate_click_row(&(pointer->pos), &window, TRANSACT_TOOLBAR_HEIGHT, -1);

		/* If the line was in range, find the column that the click occurred in by scanning through the column
		 * positions.
		 */

		if (line >= 0) {
			xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;

			column = column_get_position(windat->columns, xpos);

#ifdef DEBUG
			debug_printf("Adjust transaction window click (line %d, column %d, xpos %d)", line, column, xpos);
#endif

			/* The first options can take place on any line in the window... */

			if (column == TRANSACT_ICON_DATE) {
				/* If the column is Date, open the date menu. */
				open_date_menu(file, line, pointer);
			} else if (column == TRANSACT_ICON_FROM_NAME) {
				/* If the column is the From name, open the from account menu. */
				open_account_menu(file, ACCOUNT_MENU_FROM, line, NULL, 0, 0, 0, pointer);
			} else if (column == TRANSACT_ICON_TO_NAME) {
				/* If the column is the To name, open the to account menu. */
				open_account_menu(file, ACCOUNT_MENU_TO, line, NULL, 0, 0, 0, pointer);
			} else if (column == TRANSACT_ICON_REFERENCE) {
				/* If the column is the Reference, open the to reference menu. */
				open_refdesc_menu(file, REFDESC_MENU_REFERENCE, line, pointer);
			} else if (column == TRANSACT_ICON_DESCRIPTION) {
				/* If the column is the Description, open the to description menu. */
				open_refdesc_menu(file, REFDESC_MENU_DESCRIPTION, line, pointer);
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
	}
}


/**
 * Process lose caret events for the Transaction List window.
 *
 * \param *caret		The caret event block to handle.
 */

static void transact_window_lose_caret_handler(wimp_caret *caret)
{
	struct transact_block	*windat;

	windat = event_get_window_user_data(caret->w);
	if (windat == NULL || windat->file == NULL)
		return;

	edit_refresh_line_contents(windat->edit_line, -1, -1);
}

/**
 * Process mouse clicks in the Transaction List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void transact_pane_click_handler(wimp_pointer *pointer)
{
	struct transact_block	*windat;
	struct file_block	*file;
	wimp_window_state	window;
	wimp_icon_state		icon;
	int			ox;
	char			*filename;


	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	if (pointer->i == TRANSACT_PANE_SORT_DIR_ICON)
		pointer->i = transaction_pane_sort_substitute_icon;

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case TRANSACT_PANE_SAVE:
			if (file_check_for_filepath(windat->file))
				filename = windat->file->filename;
			else
				filename = NULL;

			saveas_initialise_dialogue(transact_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
			saveas_prepare_dialogue(transact_saveas_file);
			saveas_open_dialogue(transact_saveas_file, pointer);
			break;

		case TRANSACT_PANE_PRINT:
			transact_open_print_window(file, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_ACCOUNTS:
			account_open_window(file, ACCOUNT_FULL);
			break;

		case TRANSACT_PANE_ADDACCT:
			account_open_edit_window(file, -1, ACCOUNT_FULL, pointer);
			break;

		case TRANSACT_PANE_IN:
			account_open_window(file, ACCOUNT_IN);
			break;

		case TRANSACT_PANE_OUT:
			account_open_window(file, ACCOUNT_OUT);
			break;

		case TRANSACT_PANE_ADDHEAD:
			account_open_edit_window(file, -1, ACCOUNT_IN, pointer);
			break;

		case TRANSACT_PANE_FIND:
			find_open_window(file->find, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_GOTO:
			goto_open_window(file->go_to, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_SORT:
			transact_open_sort_window(windat, pointer);
			break;

		case TRANSACT_PANE_RECONCILE:
			file->auto_reconcile = !file->auto_reconcile;
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
			transact_start_direct_save(windat);
			break;

		case TRANSACT_PANE_PRINT:
			transact_open_print_window(file, pointer, !config_opt_read("RememberValues"));
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
			file->auto_reconcile = !file->auto_reconcile;
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
			windat->sort_order = SORT_NONE;

			switch (pointer->i) {
			case TRANSACT_PANE_ROW:
				windat->sort_order = SORT_ROW;
				break;

			case TRANSACT_PANE_DATE:
				windat->sort_order = SORT_DATE;
				break;

			case TRANSACT_PANE_FROM:
				windat->sort_order = SORT_FROM;
				break;

			case TRANSACT_PANE_TO:
				windat->sort_order = SORT_TO;
				break;

			case TRANSACT_PANE_REFERENCE:
				windat->sort_order = SORT_REFERENCE;
				break;

			case TRANSACT_PANE_AMOUNT:
				windat->sort_order = SORT_AMOUNT;
				break;

			case TRANSACT_PANE_DESCRIPTION:
				windat->sort_order = SORT_DESCRIPTION;
				break;
			}

			if (windat->sort_order != SORT_NONE) {
				if (pointer->buttons == wimp_CLICK_SELECT * 256)
					windat->sort_order |= SORT_ASCENDING;
				else
					windat->sort_order |= SORT_DESCENDING;
			}

			transact_adjust_sort_icon(windat);
			windows_redraw(windat->transaction_pane);
			transact_sort(windat);
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimTransactCols"));
		column_start_drag(windat->columns, pointer, windat, windat->transaction_window, transact_adjust_window_columns);
	}
}


/**
 * Process keypresses in the Transaction List window.  All hotkeys are handled,
 * then any remaining presses are passed on to the edit line handling code
 * for attention.
 *
 * \param *key		The keypress event block to handle.
 */

static osbool transact_window_keypress_handler(wimp_key *key)
{
	struct transact_block	*windat;
	struct file_block	*file;
	wimp_pointer		pointer;
	char			*filename;

	windat = event_get_window_user_data(key->w);
	if (windat == NULL || windat->file == NULL)
		return FALSE;

	file = windat->file;

	/* The global clipboard keys. */

	if (key->c == 3) /* Ctrl- C */
		clipboard_copy_from_icon(key);
	else if (key->c == 18) /* Ctrl-R */
		account_recalculate_all(file);
	else if (key->c == 22) /* Ctrl-V */
		clipboard_paste_to_icon(key);
	else if (key->c == 24) /* Ctrl-X */
		clipboard_cut_from_icon(key);

	/* Other keyboard shortcuts */

	else if (key->c == wimp_KEY_PRINT) {
		wimp_get_pointer_info(&pointer);
		transact_open_print_window(file, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_CONTROL + wimp_KEY_F1) {
		wimp_get_pointer_info(&pointer);
		transact_prepare_fileinfo(file);
		menus_create_standard_menu((wimp_menu *) transact_fileinfo_window, &pointer);
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
		transact_start_direct_save(windat);
	} else if (key->c == wimp_KEY_F4) {
		wimp_get_pointer_info(&pointer);
		find_open_window(file->find, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_F5) {
		wimp_get_pointer_info(&pointer);
		goto_open_window(file->go_to, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_F6) {
		wimp_get_pointer_info(&pointer);
		transact_open_sort_window(windat, &pointer);
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

		transact_window_scroll_handler(&scroll);
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

static void transact_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct transact_block	*windat;
	int			line;
	wimp_window_state	window;
	char			*filename;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
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

		transact_window_menu_account->entries[MAIN_MENU_ACCOUNTS_VIEW].sub_menu = account_list_menu_build(windat->file);
		transact_window_menu_analysis->entries[MAIN_MENU_ANALYSIS_SAVEDREP].sub_menu = analysis_template_menu_build(windat->file, FALSE);

		/* If the submenus concerned are greyed out, give them a valid submenu pointer so that the arrow shows. */

		if (account_get_count(windat->file) == 0)
			transact_window_menu_account->entries[MAIN_MENU_ACCOUNTS_VIEW].sub_menu = (wimp_menu *) 0x8000; /* \TODO -- Ugh! */
		if (windat->file->saved_report_count == 0)
			transact_window_menu_analysis->entries[MAIN_MENU_ANALYSIS_SAVEDREP].sub_menu = (wimp_menu *) 0x8000; /* \TODO -- Ugh! */

		if (file_check_for_filepath(windat->file))
			filename = windat->file->filename;
		else
			filename = NULL;

		saveas_initialise_dialogue(transact_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(transact_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(transact_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_tick_entry(transact_window_menu_transact, MAIN_MENU_TRANS_RECONCILE, windat->file->auto_reconcile);
	menus_shade_entry(transact_window_menu_account, MAIN_MENU_ACCOUNTS_VIEW, account_count_type_in_file(windat->file, ACCOUNT_FULL) == 0);
	menus_shade_entry(transact_window_menu_analysis, MAIN_MENU_ANALYSIS_SAVEDREP, windat->file->saved_report_count == 0);
	account_list_menu_prepare();
}


/**
 * Process menu selection events in the Transaction List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void transact_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct transact_block	*windat;
	struct file_block	*file;
	wimp_pointer		pointer;

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
			transact_start_direct_save(windat);
			break;

		case MAIN_MENU_FILE_CONTINUE:
			purge_open_window(windat->file->purge, &pointer, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_FILE_PRINT:
			transact_open_print_window(windat->file, &pointer, config_opt_read("RememberValues"));
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
			account_open_edit_window(windat->file, -1, ACCOUNT_FULL, &pointer);
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
			account_open_edit_window(windat->file, -1, ACCOUNT_IN, &pointer);
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
			transact_open_sort_window(windat, &pointer);
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
			windat->file->auto_reconcile = !windat->file->auto_reconcile;
			icons_set_selected(windat->transaction_pane, TRANSACT_PANE_RECONCILE, windat->file->auto_reconcile);
			break;
		}
		break;

	case MAIN_MENU_SUB_UTILS:
		switch (selection->items[1]) {
		case MAIN_MENU_ANALYSIS_BUDGET:
			budget_open_window(windat->file->budget, &pointer);
			break;

		case MAIN_MENU_ANALYSIS_SAVEDREP:
			if (selection->items[2] != -1)
				analysis_open_template_from_menu(windat->file, &pointer, selection->items[2]);
			break;

		case MAIN_MENU_ANALYSIS_MONTHREP:
			analysis_open_transaction_window(windat->file, &pointer, NULL_TEMPLATE, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_UNREC:
			analysis_open_unreconciled_window(windat->file, &pointer, NULL_TEMPLATE, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_CASHFLOW:
			analysis_open_cashflow_window(windat->file, &pointer, NULL_TEMPLATE, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_ANALYSIS_BALANCE:
			analysis_open_balance_window(windat->file, &pointer, NULL_TEMPLATE, config_opt_read("RememberValues"));
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

static void transact_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct transact_block	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (menu != transact_window_menu)
		return;

	switch (warning->selection.items[0]) {
	case MAIN_MENU_SUB_FILE:
		switch (warning->selection.items[1]) {
		case MAIN_MENU_FILE_INFO:
			transact_prepare_fileinfo(windat->file);
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

static void transact_window_menu_close_handler(wimp_w w, wimp_menu *menu)
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

static void transact_window_scroll_handler(wimp_scroll *scroll)
{
	int			width, height, line, error;
	struct transact_block	*windat;

	windat = event_get_window_user_data(scroll->w);
	if (windat == NULL || windat->file == NULL)
		return;

	/* Add in the X scroll offset. */

	width = scroll->visible.x1 - scroll->visible.x0;

	switch (scroll->xmin) {
	case wimp_SCROLL_COLUMN_LEFT:
		scroll->xscroll -= HORIZONTAL_SCROLL;
		break;

	case wimp_SCROLL_COLUMN_RIGHT:
		scroll->xscroll += HORIZONTAL_SCROLL;
		break;

	case wimp_SCROLL_PAGE_LEFT:
		scroll->xscroll -= width;
		break;

	case wimp_SCROLL_PAGE_RIGHT:
	scroll->xscroll += width;
		break;
	}

	/* Add in the Y scroll offset. */

	height = (scroll->visible.y1 - scroll->visible.y0) - TRANSACT_TOOLBAR_HEIGHT;

	switch (scroll->ymin) {
	case wimp_SCROLL_LINE_UP:
		scroll->yscroll += WINDOW_ROW_HEIGHT;
		if ((error = ((scroll->yscroll) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= WINDOW_ROW_HEIGHT + error;
		break;

	case wimp_SCROLL_LINE_DOWN:
		scroll->yscroll -= WINDOW_ROW_HEIGHT;
		if ((error = ((scroll->yscroll - height) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= error;

		/* Extend the window if necessary. */

		line = (-scroll->yscroll + height) / WINDOW_ROW_HEIGHT;
		if (line > windat->display_lines) {
			windat->display_lines = line;
			transact_set_window_extent(windat->file);
		}
		break;

	case wimp_SCROLL_PAGE_UP:
		scroll->yscroll += height;
		if ((error = ((scroll->yscroll) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= WINDOW_ROW_HEIGHT + error;
		break;

	case wimp_SCROLL_PAGE_DOWN:
		scroll->yscroll -= height;
		if ((error = ((scroll->yscroll - height) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= error;
		break;
	}

	/* Re-open the window, then try and reduce the window extent if possible.
	 *
	 * It is assumed that the wimp will deal with out-of-bounds offsets for us.
	 */

	wimp_open_window((wimp_open *) scroll);
	transact_minimise_window_extent(windat->file);
}


/**
 * Process redraw events in the Transaction List window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void transact_window_redraw_handler(wimp_draw *redraw)
{
	struct transact_block	*windat;
	int			ox, oy, top, base, y, t, shade_rec, shade_rec_col, icon_fg_col, width, entry_line;
	char			icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN], rec_char[REC_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL || windat->columns == NULL)
		return;

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);
	shade_rec = config_opt_read("ShadeReconciled");
	shade_rec_col = config_int_read("ShadeReconciledColour");

	/* Set the horizontal positions of the icons. */

	columns_place_table_icons(windat->columns, transact_window_def, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);

	width = column_get_window_width(windat->columns);
	entry_line = edit_get_line(windat->edit_line);

	/* Perform the redraw. */

	while (more) {
		/* Calculate the rows to redraw. */

		top = WINDOW_REDRAW_TOP(TRANSACT_TOOLBAR_HEIGHT, oy - redraw->clip.y1);
		if (top < 0)
			top = 0;
 
		base = WINDOW_REDRAW_BASE(TRANSACT_TOOLBAR_HEIGHT, oy - redraw->clip.y0);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			t = (y < windat->trans_count) ? windat->transactions[y].sort_index : 0;

			/* Work out the foreground colour for the line, based on whether
			 * the line is to be shaded or not.
			 */

			if (shade_rec && (y < windat->trans_count) &&
					((windat->transactions[t].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
				icon_fg_col = (shade_rec_col << wimp_ICON_FG_COLOUR_SHIFT);
			else
				icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

			/* Plot out the background with a filled grey rectangle. */

			wimp_set_colour(wimp_COLOUR_VERY_LIGHT_GREY);
			os_plot(os_MOVE_TO, ox, oy + WINDOW_ROW_TOP(TRANSACT_TOOLBAR_HEIGHT, y));
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO, ox + width, oy + WINDOW_ROW_BASE(TRANSACT_TOOLBAR_HEIGHT, y));

			/* We don't need to plot the current edit line, as that has real
			 * icons in it.
			 */

			if (y == entry_line)
				continue;

			/* Row field. */

			transact_window_def->icons[TRANSACT_ICON_ROW].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_ROW].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_ROW].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_ROW].flags |= icon_fg_col;

			if (y < windat->trans_count)
				snprintf(icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN, "%d", transact_get_transaction_number(t));
			else
				*icon_buffer = '\0';

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_ROW]));

			/* Date field */

			transact_window_def->icons[TRANSACT_ICON_DATE].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_DATE].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_DATE].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_DATE].flags |= icon_fg_col;

			if (y < windat->trans_count)
				date_convert_to_string(windat->transactions[t].date, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);
			else
				*icon_buffer = '\0';

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_DATE]));

			/* From field */

			transact_window_def->icons[TRANSACT_ICON_FROM].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_FROM].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_FROM_REC].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_FROM_REC].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_FROM].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_FROM].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_FROM_REC].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_FROM_REC].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].flags |= icon_fg_col;

			if (y < windat->trans_count && windat->transactions[t].from != NULL_ACCOUNT) {
				transact_window_def->icons[TRANSACT_ICON_FROM].data.indirected_text.text =
						account_get_ident(windat->file, windat->transactions[t].from);
				transact_window_def->icons[TRANSACT_ICON_FROM_REC].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_FROM_NAME].data.indirected_text.text =
						account_get_name(windat->file, windat->transactions[t].from);

				if (windat->transactions[t].flags & TRANS_REC_FROM)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else {
				transact_window_def->icons[TRANSACT_ICON_FROM].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_FROM_REC].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_FROM_NAME].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_FROM]));
			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_FROM_REC]));
			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_FROM_NAME]));

			/* To field */

			transact_window_def->icons[TRANSACT_ICON_TO].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_TO].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_TO_REC].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_TO_REC].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_TO_NAME].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_TO_NAME].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_TO].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_TO].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_TO_REC].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_TO_REC].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_TO_NAME].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_TO_NAME].flags |= icon_fg_col;

			if (y < windat->trans_count && windat->transactions[t].to != NULL_ACCOUNT) {
				transact_window_def->icons[TRANSACT_ICON_TO].data.indirected_text.text =
						account_get_ident(windat->file, windat->transactions[t].to);
				transact_window_def->icons[TRANSACT_ICON_TO_REC].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_TO_NAME].data.indirected_text.text =
						account_get_name(windat->file, windat->transactions[t].to);

				if (windat->transactions[t].flags & TRANS_REC_TO)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else {
				transact_window_def->icons[TRANSACT_ICON_TO].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_TO_REC].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_TO_NAME].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_TO]));
			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_TO_REC]));
			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_TO_NAME]));

			/* Reference field */

			transact_window_def->icons[TRANSACT_ICON_REFERENCE].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_REFERENCE].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_REFERENCE].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_REFERENCE].flags |= icon_fg_col;

			if (y < windat->trans_count) {
				transact_window_def->icons[TRANSACT_ICON_REFERENCE].data.indirected_text.text = windat->transactions[t].reference;
			} else {
				transact_window_def->icons[TRANSACT_ICON_REFERENCE].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_REFERENCE]));

			/* Amount field */

			transact_window_def->icons[TRANSACT_ICON_AMOUNT].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_AMOUNT].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_AMOUNT].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_AMOUNT].flags |= icon_fg_col;

			if (y < windat->trans_count)
				currency_convert_to_string(windat->transactions[t].amount, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);
			else
				*icon_buffer = '\0';

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_AMOUNT]));

			/* Description field */

			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].extent.y0 = WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, y);
			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].extent.y1 = WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, y);

			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].flags |= icon_fg_col;

			if (y < windat->trans_count) {
				transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].data.indirected_text.text = windat->transactions[t].description;
			} else {
				transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_DESCRIPTION]));
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

static void transact_adjust_window_columns(void *data, wimp_i target, int width)
{
	struct transact_block	*windat = (struct transact_block *) data;
	int			new_extent;
	wimp_window_info	window;
	wimp_caret		caret;

	if (windat == NULL || windat->file == NULL || windat->transaction_window == NULL || windat->transaction_pane == NULL)
		return;

	columns_update_dragged(windat->columns, windat->transaction_pane, NULL, target, width);

	new_extent = column_get_window_width(windat->columns);

	transact_adjust_sort_icon(windat);

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
 * Return the name of a transaction window column.
 *
 * \param *file			The file containing the transaction window.
 * \param field			The field representing the required column.
 * \param *buffer		Pointer to a buffer to take the name.
 * \param len			The length of the supplied buffer.
 * \return			Pointer to the supplied buffer, or NULL.
 */

char *transact_get_column_name(struct file_block *file, enum transact_field field, char *buffer, size_t len)
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
 * Adjust the sort icon in a transaction window, to reflect the current column
 * heading positions.
 *
 * \param *windat		The window to be updated.
 */

static void transact_adjust_sort_icon(struct transact_block *windat)
{
	wimp_icon_state icon;

	if (windat == NULL)
		return;

	icon.w = windat->transaction_pane;
	icon.i = TRANSACT_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	transact_adjust_sort_icon_data(windat, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current transaction sort settings.
 *
 * \param *windat		The window to be updated.
 * \param *icon			The icon to be updated.
 */

static void transact_adjust_sort_icon_data(struct transact_block *windat, wimp_icon *icon)
{
	if (windat == NULL)
		return;

	switch (windat->sort_order & SORT_MASK) {
	case SORT_ROW:
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_ROW;
		break;
	case SORT_DATE:
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_DATE;
		break;
	case SORT_FROM:
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_FROM;
		break;
	case SORT_TO:
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_TO;
		break;
	case SORT_REFERENCE:
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_REFERENCE;
		break;
	case SORT_AMOUNT:
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_AMOUNT;
		break;
	case SORT_DESCRIPTION:
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_DESCRIPTION;
		break;
	}

	column_update_search_indicator(windat->columns, icon, transact_pane_def, transaction_pane_sort_substitute_icon, windat->sort_order);
}


/**
 * Set the extent of the transaction window for the specified file.
 *
 * \param *file			The file containing the window to update.
 */

void transact_set_window_extent(struct file_block *file)
{
	wimp_window_state	state;
	os_box			extent;
	int			visible_extent, new_extent, new_scroll;

	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL)
		return;

	/* If the window display length is too small, extend it to one blank line after the data. */

	if (file->transacts->display_lines <= (file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES)) {
		file->transacts->display_lines = (file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
				file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;
	}

	/* Work out the new extent. */

	new_extent = (-WINDOW_ROW_HEIGHT * file->transacts->display_lines) - TRANSACT_TOOLBAR_HEIGHT;

	/* Get the current window details, and find the extent of the bottom of the visible area. */

	state.w = file->transacts->transaction_window;
	wimp_get_window_state(&state);

	visible_extent = state.yscroll + (state.visible.y0 - state.visible.y1);

	/* If the visible area falls outside the new window extent, then the window needs to be re-opened first. */

	if (new_extent > visible_extent) {
		/* Calculate the required new scroll offset.  If this is greater than zero, the current window is too
		 * big and will need shrinking down.  Otherwise, just set the new scroll offset.
		 */

		new_scroll = new_extent - (state.visible.y0 - state.visible.y1);

		if (new_scroll > 0) {
			state.visible.y0 += new_scroll;
			state.yscroll = 0;
		} else {
			state.yscroll = new_scroll;
		}

		wimp_open_window((wimp_open *) &state);
	}

	/* Finally, call Wimp_SetExtent to update the extent, safe in the knowledge that the visible area will still
	 * exist.
	 */

	extent.x0 = 0;
	extent.y1 = 0;
	extent.x1 = column_get_window_width(file->transacts->columns) + COLUMN_GUTTER;
	extent.y0 = new_extent;

	wimp_set_extent(file->transacts->transaction_window, &extent);
}


/**
 * Minimise the extent of the transaction window, by removing unnecessary
 * blank lines as they are scrolled out of sight.
 *
 * \param *file			The file containing the window to update.
 */

void transact_minimise_window_extent(struct file_block *file)
{
	int			height, last_visible_line, minimum_length, entry_line;
	wimp_window_state	window;

	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL)
		return;

	window.w = file->transacts->transaction_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the window and the last line that
	 * is on display in the visible area.
	 */

	height = (window.visible.y1 - window.visible.y0) - TRANSACT_TOOLBAR_HEIGHT;
	last_visible_line = (-window.yscroll + height) / WINDOW_ROW_HEIGHT;

	/* Work out what the minimum length of the window needs to be,
	 * taking into account minimum window size, entries and blank lines
	 * and the location of the edit line.
	 */

	minimum_length = (file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
			file->transacts->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;

	entry_line = edit_get_line(file->transacts->edit_line);

	if (entry_line >= minimum_length)
		minimum_length = entry_line + 1;

	if (last_visible_line > minimum_length)
		minimum_length = last_visible_line;

	/* Shrink the window. */

	if (file->transacts->display_lines > minimum_length) {
		file->transacts->display_lines = minimum_length;
		transact_set_window_extent(file);
	}
}


/**
 * Get the window state of the transaction window belonging to
 * the specified file.
 *
 * \param *file			The file containing the window.
 * \param *state		The structure to hold the window state.
 * \return			Pointer to an error block, or NULL on success.
 */

os_error *transact_get_window_state(struct file_block *file, wimp_window_state *state)
{
	if (file == NULL || file->transacts == NULL || state == NULL)
		return NULL;

	state->w = file->transacts->transaction_pane;
	return xwimp_get_window_state(state);
}


/**
 * Recreate the title of the Transaction window connected to the given file.
 *
 * \param *file			The file to rebuild the title for.
 */

void transact_build_window_title(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return;

	file_get_pathname(file, file->transacts->window_title,
			sizeof(file->transacts->window_title) - 2);

	if (file_get_data_integrity(file))
		strcat(file->transacts->window_title, " *");

	if (file->transacts->transaction_window != NULL)
		wimp_force_redraw_title(file->transacts->transaction_window);
}


/**
 * Force the complete redraw of the Transaction window.
 *
 * \param *file			The file owning the window to redraw.
 */

void transact_redraw_all(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_force_window_redraw(file->transacts, 0, file->transacts->trans_count - 1);
}


/**
 * Force a redraw of the Transaction window, for the given range of lines.
 *
 * \param *windat		The window to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

static void transact_force_window_redraw(struct transact_block *windat, int from, int to)
{
	int			y0, y1, line;
	wimp_window_info	window;
  
	if (windat == NULL || windat->transaction_window == NULL)
		return;

	/* If the edit line falls inside the redraw range, refresh it. */

	line = edit_get_line(windat->edit_line);

	if (line >= from && line <= to) {
		edit_refresh_line_contents(windat->edit_line, -1, -1);
		edit_set_line_colour(windat->edit_line, transact_line_colour(windat, line));
		icons_replace_caret_in_window(windat->transaction_window);
	}

	/* Now force a redraw of the whole window range. */

	window.w = windat->transaction_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	y1 = WINDOW_ROW_TOP(TRANSACT_TOOLBAR_HEIGHT, from);
	y0 = WINDOW_ROW_BASE(TRANSACT_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(windat->transaction_window, window.extent.x0, y0, window.extent.x1, y1);
}


/**
 * Update the state of the buttons in a transaction window toolbar.
 *
 * \param *file			The file owning the window to update.
 */

void transact_update_toolbar(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL || file->transacts->transaction_pane == NULL)
		return;

	icons_set_shaded(file->transacts->transaction_pane, TRANSACT_PANE_VIEWACCT,
			account_count_type_in_file(file, ACCOUNT_FULL) == 0);
}


/**
 * Bring a transaction window to the top of the window stack.
 *
 * \param *file			The file owning the window to bring up.
 */

void transact_bring_window_to_top(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL)
		return;

	windows_open(file->transacts->transaction_window);
}


/**
 * Scroll a transaction window to either the top (home) or the end.
 *
 * \param *file			The file owning the window to be scrolled.
 * \param direction		The direction to scroll the window in.
 */

void transact_scroll_window_to_end(struct file_block *file, enum transact_scroll_direction direction)
{
	wimp_window_info	window;


	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL ||
			direction == TRANSACT_SCROLL_NONE)
		return;

	window.w = file->transacts->transaction_window;
	wimp_get_window_info_header_only(&window);

	switch (direction) {
	case TRANSACT_SCROLL_HOME:
		window.yscroll = window.extent.y1;
		break;

	case TRANSACT_SCROLL_END:
		window.yscroll = window.extent.y0 - (window.extent.y1 - window.extent.y0);
		break;

	case TRANSACT_SCROLL_NONE:
		break;
	}
 
 	transact_minimise_window_extent(file);
 	wimp_open_window((wimp_open *) &window);
}


/**
 * Return the transaction number of the transaction nearest to the centre of
 * the visible area of the transaction window which references a given
 * account.
 *
 * \param *file			The file owning the window to be searched.
 * \param account		The account to search for.
 * \return			The transaction found, or NULL_TRANSACTION.
 */

int transact_find_nearest_window_centre(struct file_block *file, acct_t account)
{
	wimp_window_state	window;
	int			height, i, centre, result;


	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL ||
			account == NULL_ACCOUNT)
		return NULL_TRANSACTION;

	window.w = file->transacts->transaction_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out
	 * any OS units taken up by part lines.
	 */

	height = window.visible.y1 - window.visible.y0 - WINDOW_ROW_HEIGHT - TRANSACT_TOOLBAR_HEIGHT;

	/* Calculate the centre line in the window. If this is greater than
	 * the number of actual tracsactions in the window, reduce it
	 * accordingly.
	 */

	centre = ((-window.yscroll + WINDOW_ROW_ICON_HEIGHT) / WINDOW_ROW_HEIGHT) + ((height / 2) / WINDOW_ROW_HEIGHT);

	if (centre >= file->transacts->trans_count)
		centre = file->transacts->trans_count - 1;

	/* If there are no transactions, we can't return one. */

	if (centre < 0)
		return NULL_TRANSACTION;

	/* If the centre transaction is a match, return it. */

	if (file->transacts->transactions[file->transacts->transactions[centre].sort_index].from == account ||
			file->transacts->transactions[file->transacts->transactions[centre].sort_index].to == account)
		return file->transacts->transactions[centre].sort_index;

	/* Start searching out from the centre until we find a match or hit
	 * both the start and end of the file.
	 */

	result = NULL_TRANSACTION;
	i = 1;

	while (centre + i < file->transacts->trans_count || centre - i >= 0) {
		if (centre + i < file->transacts->trans_count &&
				(file->transacts->transactions[file->transacts->transactions[centre + i].sort_index].from == account ||
				file->transacts->transactions[file->transacts->transactions[centre + i].sort_index].to == account)) {
			result = file->transacts->transactions[centre + i].sort_index;
			break;
		}

		if (centre - i >= 0 &&
				(file->transacts->transactions[file->transacts->transactions[centre - i].sort_index].from == account ||
				file->transacts->transactions[file->transacts->transactions[centre - i].sort_index].to == account)) {
			result = file->transacts->transactions[centre - i].sort_index;
			break;
		}

		i++;
	}
 
	return result;
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

static void transact_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int			xpos;
	wimp_i			icon;
	wimp_window_state	window;
	struct transact_block	*windat;

	*buffer = '\0';

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	icon = column_get_position(windat->columns, xpos);
	if (icon == wimp_ICON_WINDOW)
		return;

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, transact_window_def->icons[icon].data.indirected_text.validation, 'N')) {
		snprintf(buffer, IHELP_INAME_LEN, "Col%d", icon);
		buffer[IHELP_INAME_LEN - 1] = '\0';
	}
}


/**
 * Find the display line in a transaction window which points to the
 * specified transaction under the applied sort.
 *
 * \param *file			The file to use the transaction window in.
 * \param transaction		The transaction to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

int transact_get_line_from_transaction(struct file_block *file, tran_t transaction)
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
 * Find the transaction which corresponds to a display line in a transaction
 * window.
 *
 * \param *file			The file to use the transaction window in.
 * \param line			The display line to return the transaction for.
 * \return			The appropriate transaction, or NULL_TRANSACTION.
 */

int transact_get_transaction_from_line(struct file_block *file, int line)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, line))
		return NULL_TRANSACTION;

	return file->transacts->transactions[line].sort_index;
}


/**
 * Find the number of transactions in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of transactions in the file.
 */

int transact_get_count(struct file_block *file)
{
	return (file != NULL && file->transacts != NULL) ? file->transacts->trans_count : 0;
}


/**
 * Find the display line number of the current transaction entry line.
 *
 * \param *file			The file to interrogate.
 * \return			The display line number of the line with the caret.
 */

int transact_get_caret_line(struct file_block *file)
{
	int entry_line = 0;

	if (file == NULL || file->transacts == NULL)
		return 0;

	entry_line = edit_get_line(file->transacts->edit_line);

	return (entry_line >= 0) ? entry_line : 0;
}






























































/**
 * Build a Reference or Drescription Complete menu for a given file.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \param start_line		The line of the window to start from.
 * \return			The menu block, or NULL.
 */

wimp_menu *transact_complete_menu_build(struct file_block *file, enum transact_list_menu_type menu_type, int start_line)
{
	int		i, range, line, width, items, max_items, item_limit;
	osbool		no_original;
	char		*title_token;

	if (file == NULL || file->transacts == NULL)
		return NULL;

	hourglass_on();

	account_complete_menu_destroy();

	transact_complete_menu_type = menu_type;
	transact_complete_menu_file = file;

	/* Claim enough memory to build the menu in. */

	max_items = REFDESC_MENU_BLOCKSIZE;
	transact_complete_menu_link = heap_alloc((sizeof(struct transact_list_link) * max_items));

	items = 0;
	item_limit = config_int_read("MaxAutofillLen");

	/* In the Reference menu, the first item needs to be the Cheque No. entry, so insert that manually. */

	if (transact_complete_menu_link != NULL && menu_type == REFDESC_MENU_REFERENCE)
		msgs_lookup("RefMenuChq", transact_complete_menu_link[items++].name, TRANSACT_DESCRIPT_FIELD_LEN);

	/* Bring the start line into range for the current file.  no_original is set true if the line fell off the end
	 * of the file, as this needs to be a special case of "no text".  If not, lines off the end of the file will
	 * be matched against the final transaction as a result of pulling start_line into range.
	 */

	if (start_line >= file->transacts->trans_count) {
		start_line = file->transacts->trans_count - 1;
		no_original = TRUE;
	} else {
		no_original = FALSE;
	}

	if (file->transacts->trans_count > 0 && transact_complete_menu_link != NULL) {
		/* Find the largest range from the current line to one end of the transaction list. */

		range = ((file->transacts->trans_count - start_line - 1) > start_line) ? (file->transacts->trans_count - start_line - 1) : start_line;

		/* Work out from the line to the edges of the transaction window. For each transaction, check the entries
		 * and add them into the list.
		 */

		if (menu_type == REFDESC_MENU_REFERENCE) {
			for (i=1; i<=range && (item_limit == 0 || items <= item_limit); i++) {
				if ((start_line+i < file->transacts->trans_count) && (no_original ||
						(*(file->transacts->transactions[file->transacts->transactions[start_line].sort_index].reference) == '\0') ||
						(string_nocase_strstr(file->transacts->transactions[file->transacts->transactions[start_line+i].sort_index].reference,
						file->transacts->transactions[file->transacts->transactions[start_line].sort_index].reference) ==
						file->transacts->transactions[file->transacts->transactions[start_line+i].sort_index].reference))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
							file->transacts->transactions[file->transacts->transactions[start_line+i].sort_index].reference);
				}
				if ((start_line-i >= 0) && (no_original ||
						(*(file->transacts->transactions[file->transacts->transactions[start_line].sort_index].reference) == '\0') ||
						(string_nocase_strstr(file->transacts->transactions[file->transacts->transactions[start_line-i].sort_index].reference,
						file->transacts->transactions[file->transacts->transactions[start_line].sort_index].reference) ==
						file->transacts->transactions[file->transacts->transactions[start_line-i].sort_index].reference))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
						file->transacts->transactions[file->transacts->transactions[start_line-i].sort_index].reference);
				}
			}
		} else if (menu_type == REFDESC_MENU_DESCRIPTION) {
			for (i=1; i<=range && (item_limit == 0 || items < item_limit); i++) {
				if ((start_line+i < file->transacts->trans_count) && (no_original ||
						(*(file->transacts->transactions[file->transacts->transactions[start_line].sort_index].description) == '\0') ||
						(string_nocase_strstr(file->transacts->transactions[file->transacts->transactions[start_line+i].sort_index].description,
						file->transacts->transactions[file->transacts->transactions[start_line].sort_index].description) ==
						file->transacts->transactions[file->transacts->transactions[start_line+i].sort_index].description))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
							file->transacts->transactions[file->transacts->transactions[start_line+i].sort_index].description);
				}
				if ((start_line-i >= 0) && (no_original ||
						(*(file->transacts->transactions[file->transacts->transactions[start_line].sort_index].description) == '\0') ||
						(string_nocase_strstr(file->transacts->transactions[file->transacts->transactions[start_line-i].sort_index].description,
						file->transacts->transactions[file->transacts->transactions[start_line].sort_index].description) ==
						file->transacts->transactions[file->transacts->transactions[start_line-i].sort_index].description))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
						file->transacts->transactions[file->transacts->transactions[start_line-i].sort_index].description);
				}
			}
		}
	  }

	/* If there are items in the menu, claim the extra memory required to build the Wimp menu structure and
	 * set up the pointers.   If there are not, transact_complete_menu will remain NULL and the menu won't exist.
	 *
	 * transact_complete_menu_link may be NULL if memory allocation failed at any stage of the build process.
	 */

	if (transact_complete_menu_link == NULL || items == 0) {
		transact_complete_menu_destroy();
		hourglass_off();
		return NULL;
	}

	transact_complete_menu = heap_alloc(28 + (24 * max_items));
	transact_complete_menu_title = heap_alloc(ACCOUNT_MENU_TITLE_LEN);

	if (transact_complete_menu == NULL || transact_complete_menu_title == NULL) {
		transact_complete_menu_destroy();
		hourglass_off();
		return NULL;
	}

	/* Populate the menu. */

	line = 0;
	width = 0;

	if (menu_type == REFDESC_MENU_REFERENCE)
		qsort(transact_complete_menu_link + 1, items - 1, sizeof(struct transact_list_link), transact_complete_menu_compare);
	else
		qsort(transact_complete_menu_link, items, sizeof(struct transact_list_link), transact_complete_menu_compare);

	if (items > 0) {
		for (i=0; i < items; i++) {
			if (strlen(transact_complete_menu_link[line].name) > width)
				width = strlen(transact_complete_menu_link[line].name);

			/* Set the menu and icon flags up. */

			if (menu_type == REFDESC_MENU_REFERENCE && i == REFDESC_MENU_CHEQUE)
				transact_complete_menu->entries[line].menu_flags = (items > 1) ? wimp_MENU_SEPARATE : 0;
			else
				transact_complete_menu->entries[line].menu_flags = 0;

			transact_complete_menu->entries[line].sub_menu = (wimp_menu *) -1;
			transact_complete_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			transact_complete_menu->entries[line].data.indirected_text.text = transact_complete_menu_link[line].name;
			transact_complete_menu->entries[line].data.indirected_text.validation = NULL;
			transact_complete_menu->entries[line].data.indirected_text.size = TRANSACT_DESCRIPT_FIELD_LEN;

			line++;
		}
	}

	/* Finish off the menu, marking the last entry and filling in the header. */

	transact_complete_menu->entries[(line > 0) ? line-1 : 0].menu_flags |= wimp_MENU_LAST;

	if (transact_complete_menu_title == NULL)
		transact_complete_menu_title = (char *) malloc (ACCOUNT_MENU_TITLE_LEN);

	switch (menu_type) {
	case REFDESC_MENU_REFERENCE:
		title_token = "RefMenuTitle";
		break;

	case REFDESC_MENU_DESCRIPTION:
	default:
		title_token = "DescMenuTitle";
		break;
	}

	msgs_lookup(title_token, transact_complete_menu_title, ACCOUNT_MENU_TITLE_LEN);

	transact_complete_menu->title_data.indirected_text.text = transact_complete_menu_title;
	transact_complete_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	transact_complete_menu->title_fg = wimp_COLOUR_BLACK;
	transact_complete_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	transact_complete_menu->work_fg = wimp_COLOUR_BLACK;
	transact_complete_menu->work_bg = wimp_COLOUR_WHITE;

	transact_complete_menu->width = (width + 1) * 16;
	transact_complete_menu->height = 44;
	transact_complete_menu->gap = 0;

	hourglass_off();

	return transact_complete_menu;
}


/**
 * Destroy any Reference or Description Complete menu which is currently open.
 */

void transact_complete_menu_destroy(void)
{
	if (transact_complete_menu != NULL)
		heap_free(transact_complete_menu);

	if (transact_complete_menu_link != NULL)
		heap_free(transact_complete_menu_link);

	if (transact_complete_menu_title != NULL)
		heap_free(transact_complete_menu_title);

	transact_complete_menu = NULL;
	transact_complete_menu_link = NULL;
	transact_complete_menu_title = NULL;
	transact_complete_menu_file = NULL;
	transact_complete_menu_type = REFDESC_MENU_NONE;
}


/**
 * Prepare the currently active Reference or Description menu for opening or
 * reopening, by shading lines which shouldn't be selectable.
 *
 * \param line			The line that the menu is open over.
 */

void transact_complete_menu_prepare(int line)
{
	acct_t		account;

	if (transact_complete_menu == NULL || transact_complete_menu_file == NULL || transact_complete_menu_file->transacts == NULL ||
			transact_complete_menu_type != REFDESC_MENU_REFERENCE)
		return;

	if ((line < transact_complete_menu_file->transacts->trans_count) &&
			((account = transact_complete_menu_file->transacts->transactions[transact_complete_menu_file->transacts->transactions[line].sort_index].from) !=
					 NULL_ACCOUNT) &&
			account_cheque_number_available(transact_complete_menu_file, account))
		transact_complete_menu->entries[0].icon_flags &= ~wimp_ICON_SHADED;
	else
		transact_complete_menu->entries[0].icon_flags |= wimp_ICON_SHADED;
}


/**
 * Decode menu selections from the Reference or Description menu.
 *
 * \param *selection		The menu selection to be decoded.
 * \return			Pointer to the selected text, or NULL if
 *				the Cheque Number field was selected or there
 *				was not valid menu open.
 */

char *transact_complete_menu_decode(wimp_selection *selection)
{
	if (transact_complete_menu == NULL || selection == NULL || selection->items[0] == -1)
		return NULL;

	if (transact_complete_menu_type == REFDESC_MENU_REFERENCE && selection->items[0] == REFDESC_MENU_CHEQUE)
		return NULL;

	if (transact_complete_menu_link == NULL)
		return NULL;

	return transact_complete_menu_link[selection->items[0]].name;
}


/**
 * Add a reference or description text to the list menu.
 *
 * \param **entries		Pointer to the reference struct pointer, allowing
 *				it to be updated if the memory needs to grow.
 * \param *count		Pointer to the item count, for update.
 * \param *max			Pointer to the max item count, for update.
 * \param *new			The nex text item to be added.
 */

static void transact_complete_menu_add_entry(struct transact_list_link **entries, int *count, int *max, char *new)
{
	int		i;
	osbool		found = FALSE;

	if (*entries == NULL || new == NULL || *new == '\0')
		return;

	for (i = 0; (i < *count) && !found; i++)
		if (string_nocase_strcmp((*entries)[i].name, new) == 0)
			found = TRUE;

	if (!found && *count < (*max))
		strcpy((*entries)[(*count)++].name, new);

	/* Extend the block *after* the copy, in anticipation of the next call, because this could easily move the
	 * flex blocks around and that would invalidate the new pointer...
	 */

	if (*count >= (*max)) {
		*entries = heap_extend(*entries, sizeof(struct transact_list_link) * (*max + REFDESC_MENU_BLOCKSIZE));
		*max += REFDESC_MENU_BLOCKSIZE;
	}
}


/**
 * Compare two menu entries, for qsort().
 *
 * \param *va			The first entry to compare.
 * \param *vb			The second entry to compare.
 * \return			The comparison result.
 */

static int transact_complete_menu_compare(const void *va, const void *vb)
{
	struct transact_list_link *a = (struct transact_list_link *) va, *b = (struct transact_list_link *) vb;

	return (string_nocase_strcmp(a->name, b->name));
}



















































/* ==================================================================================================================
 * Transaction handling
 */

/**
 * Adds a new transaction to the end of the list, using the details supplied.
 *
 * \param *file			The file to add the transaction to.
 * \param date			The date of the transaction, or NULL_DATE.
 * \param from			The account to transfer from, or NULL_ACCOUNT.
 * \param to			The account to transfer to, or NULL_ACCOUNT.
 * \param flags			The transaction flags.
 * \param amount		The amount to transfer, or NULL_CURRENCY.
 * \param *ref			Pointer to the transaction reference, or NULL.
 * \param *description		Pointer to the transaction description, or NULL.
 */

void transact_add_raw_entry(struct file_block *file, date_t date, acct_t from, acct_t to, enum transact_flags flags,
		amt_t amount, char *ref, char *description)
{
	int new;

	if (file == NULL || file->transacts == NULL)
		return;

	if (flex_extend((flex_ptr) &(file->transacts->transactions), sizeof(struct transaction) * (file->transacts->trans_count + 1)) != 1) {
		error_msgs_report_error("NoMemNewTrans");
		return;
	}

	new = file->transacts->trans_count++;

	file->transacts->transactions[new].date = date;
	file->transacts->transactions[new].amount = amount;
	file->transacts->transactions[new].from = from;
	file->transacts->transactions[new].to = to;
	file->transacts->transactions[new].flags = flags;
	strncpy(file->transacts->transactions[new].reference, (ref != NULL) ? ref : "", TRANSACT_REF_FIELD_LEN);
	file->transacts->transactions[new].reference[TRANSACT_REF_FIELD_LEN - 1] = '\0';
	strncpy(file->transacts->transactions[new].description, (description != NULL) ? description : "", TRANSACT_DESCRIPT_FIELD_LEN);
	file->transacts->transactions[new].description[TRANSACT_DESCRIPT_FIELD_LEN - 1] = '\0';
	file->transacts->transactions[new].sort_index = new;

	file_set_data_integrity(file, TRUE);
	if (date != NULL_DATE)
		file->sort_valid = FALSE;
}


/**
 * Clear a transaction from a file, returning it to an empty state. Note that
 * the transaction remains in-situ, and no memory is cleared. It will be
 * necessary to subsequently call transact_strip_blanks() to free the memory.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to be cleared.
 */

void transact_clear_raw_entry(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	file->transacts->transactions[transaction].date = NULL_DATE;
	file->transacts->transactions[transaction].from = NULL_ACCOUNT;
	file->transacts->transactions[transaction].to = NULL_ACCOUNT;
	file->transacts->transactions[transaction].flags = TRANS_FLAGS_NONE;
	file->transacts->transactions[transaction].amount = NULL_CURRENCY;
	*file->transacts->transactions[transaction].reference = '\0';
	*file->transacts->transactions[transaction].description = '\0';

	file->sort_valid = FALSE;
}


/**
 * Test to see if a transaction entry in a file is completely empty.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to be tested.
 * \return			TRUE if the transaction is empty; FALSE if not.
 */

static osbool transact_is_blank(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return FALSE;

	return (file->transacts->transactions[transaction].date == NULL_DATE &&
			file->transacts->transactions[transaction].from == NULL_ACCOUNT &&
			file->transacts->transactions[transaction].to == NULL_ACCOUNT &&
			file->transacts->transactions[transaction].amount == NULL_CURRENCY &&
			*file->transacts->transactions[transaction].reference == '\0' &&
			*file->transacts->transactions[transaction].description == '\0') ? TRUE : FALSE;
}


/**
 * Strip any blank transactions from the end of the file, releasing any memory
 * associated with them. To be sure to remove all blank transactions, it is
 * necessary to sort the transaction list before calling this function.
 *
 * \param *file			The file to be processed.
 */

void transact_strip_blanks_from_end(struct file_block *file)
{
	int	i, j;
	osbool	found;


	if (file == NULL || file->transacts == NULL)
		return;

	i = file->transacts->trans_count - 1;

	while (transact_is_blank(file, i)) {

		/* Search the trasnaction sort index to find the line pointing
		 * to the current blank. If one is found, shuffle all of the
		 * following indexes up a space to compensate.
		 */

		found = FALSE;

		for (j = 0; j < i; j++) {
			if (file->transacts->transactions[j].sort_index == i)
				found = TRUE;

			if (found)
				file->transacts->transactions[j].sort_index = file->transacts->transactions[j + 1].sort_index;
		}

		/* Remove the transaction. */

		i--;
	}

	/* If any transactions were removed, free up the unneeded memory
	 * from the end of the file.
	 */

	if (i < file->transacts->trans_count - 1) {
		file->transacts->trans_count = i + 1;

		flex_extend((flex_ptr) &(file->transacts->transactions), sizeof(struct transaction) * file->transacts->trans_count);
	}
}


/**
 * Return the date of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the date for.
 * \return			The date of the transaction, or NULL_DATE.
 */

date_t transact_get_date(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_DATE;

	return file->transacts->transactions[transaction].date;
}


/**
 * Return the from account of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the from account for.
 * \return			The from account of the transaction, or NULL_ACCOUNT.
 */

acct_t transact_get_from(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_ACCOUNT;

	return file->transacts->transactions[transaction].from;
}


/**
 * Return the to account of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the to account for.
 * \return			The to account of the transaction, or NULL_ACCOUNT.
 */

acct_t transact_get_to(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_ACCOUNT;

	return file->transacts->transactions[transaction].to;
}


/**
 * Return the transaction flags for a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the flags for.
 * \return			The flags of the transaction, or TRANS_FLAGS_NONE.
 */

enum transact_flags transact_get_flags(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return TRANS_FLAGS_NONE;

	return file->transacts->transactions[transaction].flags;
}


/**
 * Return the amount of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the amount of.
 * \return			The amount of the transaction, or NULL_CURRENCY.
 */

amt_t transact_get_amount(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_CURRENCY;

	return file->transacts->transactions[transaction].amount;
}


/**
 * Return the reference for a transaction.
 *
 * If a buffer is supplied, the reference is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * reference in the transaction array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the reference of.
 * \param *buffer		Pointer to a buffer to take the reference, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting reference string,
 *				either the supplied buffer or the original.
 */

char *transact_get_reference(struct file_block *file, tran_t transaction, char *buffer, size_t length)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL;

	if (buffer == NULL || length == 0)
		return file->transacts->transactions[transaction].reference;

	strncpy(buffer, file->transacts->transactions[transaction].reference, length);
	buffer[length - 1] = '\0';

	return buffer;
}


/**
 * Return the description for a transaction.
 *
 * If a buffer is supplied, the description is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * description in the transaction array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the description of.
 * \param *buffer		Pointer to a buffer to take the description, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting description string,
 *				either the supplied buffer or the original.
 */

char *transact_get_description(struct file_block *file, tran_t transaction, char *buffer, size_t length)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL;

	if (buffer == NULL || length == 0)
		return file->transacts->transactions[transaction].description;

	strncpy(buffer, file->transacts->transactions[transaction].description, length);
	buffer[length - 1] = '\0';

	return buffer;
}


/**
 * Return the sort workspace for a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the workspace of.
 * \return			The sort workspace for the transaction, or 0.
 */

int transact_get_sort_workspace(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return 0;

	return file->transacts->transactions[transaction].sort_workspace;
}


/**
 * Test the validity of a transaction index.
 *
 * \param *file			The file to test against.
 * \param transaction		The transaction index to test.
 * \return			TRUE if the index is valid; FALSE if not.
 */

osbool transact_test_index_valid(struct file_block *file, tran_t transaction)
{
	return (transact_valid(file->transacts, transaction)) ? TRUE : FALSE;
}


/**
 * Get the underlying transaction number relating to the current edit line
 * position.
 *
 * \param *file			The file that we're interested in.
 * \return			The transaction number, or NULL_TRANSACTION if the
 *				line isn't in the specified file.
 */

static tran_t transact_find_edit_line_by_transaction(struct transact_block *windat)
{
	int	line;

	if (windat == NULL || windat->transactions == NULL)
		return NULL_TRANSACTION;

	line = edit_get_line(windat->edit_line);
	if (!transact_valid(windat, line))
		return NULL_TRANSACTION;

	return windat->transactions[line].sort_index;
}


/**
 * Place a new edit line by raw transaction number.
 *
 * \param *windat		The transaction window to place the line in.
 * \param transaction		The transaction to place the line on.
 */

static void transact_place_edit_line_by_transaction(struct transact_block *windat, tran_t transaction)
{
	int		i, line;
	wimp_caret	caret;

	if (windat == NULL)
		return;

	line = windat->trans_count;

	if (transaction != NULL_TRANSACTION) {
		for (i = 0; i < windat->trans_count; i++) {
			if (windat->transactions[i].sort_index == transaction) {
				line = i;
				break;
			}
		}
	}

	transact_place_edit_line(windat, line);

	wimp_get_caret_position(&caret);
	if (caret.w == windat->transaction_window)
		icons_put_caret_at_end(windat->transaction_window, TRANSACT_ICON_DATE);
	transact_find_edit_line_vertically(windat);
}


/**
 * Callback to allow the edit line to move.
 *
 * \param line			The line in which to place the edit line.
 * \param *data			Client data: windat.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool transact_edit_place_line(int line, void *data)
{
	struct transact_block	*windat = data;

	if (windat == NULL)
		return FALSE;

	transact_place_edit_line(windat, line);
	transact_find_edit_line_vertically(windat);

	return TRUE;
}


/**
 * Place a new edit line in a transaction window by visible line number,
 * extending the window if required.
 *
 * \param *windat		The transaction window to place the line in.
 * \param line			The line to place.
 */

static void transact_place_edit_line(struct transact_block *windat, int line)
{
	wimp_colour	colour;

	if (windat == NULL)
		return;

	if (line >= windat->display_lines) {
		windat->display_lines = line + 1;
		transact_set_window_extent(windat->file);
	}

	colour = transact_line_colour(windat, line);
	edit_place_new_line(windat->edit_line, line, colour);
}


/**
 * Bring the edit line into view in a vertical direction within a transaction
 * window.
 *
 * \param *windat		The transaction window to be updated.
 */

static void transact_find_edit_line_vertically(struct transact_block *windat)
{
	wimp_window_state	window;
	int			height, top, bottom, line;


	if (windat == NULL || windat->file == NULL || windat->transaction_window == NULL || !edit_get_active(windat->edit_line))
		return;

	window.w = windat->transaction_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
	 * This will allow the edit line to be aligned with the top or bottom of the window.
	 */

	height = window.visible.y1 - window.visible.y0 - WINDOW_ROW_HEIGHT - TRANSACT_TOOLBAR_HEIGHT;

	/* Calculate the top full line and bottom full line that are showing in the window.  Part lines don't
	 * count and are discarded.
	 */

	top = (-window.yscroll + WINDOW_ROW_ICON_HEIGHT) / WINDOW_ROW_HEIGHT;
	bottom = (height / WINDOW_ROW_HEIGHT) + top;

	/* If the edit line is above or below the visible area, bring it into range. */

	line = edit_get_line(windat->edit_line);
	if (line < 0)
		return;

#ifdef DEBUG
	debug_printf("\\BFind transaction edit line");
	debug_printf("Top: %d, Bottom: %d, Entry line: %d", top, bottom, line);
#endif

	if (line < top) {
		window.yscroll = -(line * WINDOW_ROW_HEIGHT);
		wimp_open_window((wimp_open *) &window);
		transact_minimise_window_extent(windat->file);
	}

	if (line > bottom) {
		window.yscroll = -(line * WINDOW_ROW_HEIGHT - height);
		wimp_open_window((wimp_open *) &window);
		transact_minimise_window_extent(windat->file);
	}
}


/**
 * Handle requests from the edit line to bring a given line and field into
 * view in the visible area of the window.
 *
 * \param line			The line in the window to be brought into view.
 * \param xmin			The minimum X coordinate to be shown.
 * \param xmax			The maximum X coordinate to be shown.
 * \param target		The target requirement: left edge, right edge or centred.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE if handled successfully; FALSE if not.
 */

static osbool transact_edit_find_field(int line, int xmin, int xmax, enum edit_align target, void *data)
{
	struct transact_block	*windat = data;
	wimp_window_state	window;
	int			icon_width, window_width, window_xmin, window_xmax;

	if (windat == NULL)
		return FALSE;

	window.w = windat->transaction_window;
	wimp_get_window_state(&window);

	icon_width = xmax - xmin;

	/* Establish the window dimensions. */

	window_width = window.visible.x1 - window.visible.x0;
	window_xmin = window.xscroll;
	window_xmax = window.xscroll + window_width;

	if (window_width > icon_width) {
		/* If the icon group fits into the visible window, just pull the overlap into view. */

		if (xmin < window_xmin) {
			window.xscroll = xmin;
			wimp_open_window((wimp_open *) &window);
		} else if (xmax > window_xmax) {
			window.xscroll = xmax - window_width;
			wimp_open_window((wimp_open *) &window);
		}
	} else {
		/* If the icon is bigger than the window, however, align the target with the edge of the window. */

		switch (target) {
		case EDIT_ALIGN_LEFT:
		case EDIT_ALIGN_CENTRE:
			if (xmin < window_xmin || xmin > window_xmax) {
				window.xscroll = xmin;
				wimp_open_window((wimp_open *) &window);
			}
			break;
		case EDIT_ALIGN_RIGHT:
			if (xmax < window_xmin || xmax > window_xmax) {
				window.xscroll = xmax - window_width;
				wimp_open_window((wimp_open *) &window);
			}
			break;
		case EDIT_ALIGN_NONE:
			break;
		}
	}

	/* Make sure that the line is visible vertically, as well. 
	 * NB: This currently ignores the line parameter, as transact_find_edit_line_vertically()
	 * queries the edit line directly. At present, these both yield the same result.
	 */

	transact_find_edit_line_vertically(windat);

	return TRUE;
}


/**
 * Inform the edit line about the location of the first blank transaction
 * line in our window.
 *
 * \param *data			Our client data, holding the window instance.
 * \return			The line number of the first blank line, or -1.
 */

static int transact_edit_first_blank_line(void *data)
{
	struct transact_block	*windat = data;

	if (windat == NULL || windat->file == NULL)
		return -1;

	return transact_find_first_blank_line(windat->file);
}


/**
 * Find and return the line number of the first blank line in a file, based on
 * display order.
 *
 * \param *file			The file to search.
 * \return			The first blank display line.
 */

int transact_find_first_blank_line(struct file_block *file)
{
	int line;

	if (file == NULL || file->transacts == NULL)
		return 0;

	#ifdef DEBUG
	debug_printf("\\DFinding first blank line");
	#endif

	line = file->transacts->trans_count;

	while (line > 0 && transact_is_blank(file, file->transacts->transactions[line - 1].sort_index)) {
		line--;

		#ifdef DEBUG
		debug_printf("Stepping back up...");
		#endif
	}

	return line;
}


/**
 * Inform the edit line whether a given line in the window contains a valid
 * transaction.
 *
 * \param line			The line in the window to be tested.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE if valid; FALSE if not or on error.
 */

static osbool transact_edit_test_line(int line, void *data)
{
	struct transact_block	*windat = data;

	if (windat == NULL)
		return FALSE;

	return (transact_valid(windat, line)) ? TRUE : FALSE;
}


/**
 * Change the date for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param new_date	The new date to set the transaction to.
 */

void transact_change_date(struct file_block *file, tran_t transaction, date_t new_date)
{
	int	line;
	osbool	changed = FALSE;
	date_t	old_date = NULL_DATE;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	account_remove_transaction(file, transaction);

	/* Look up the existing date, change it and compare the two. If the field
	 * has changed, flag this up.
	 */

	old_date = file->transacts->transactions[transaction].date;

	file->transacts->transactions[transaction].date = new_date;

	if (old_date != file->transacts->transactions[transaction].date) {
		changed = TRUE;
		file->sort_valid = FALSE;
	}

	/* Return the line to the calculations. This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* If any changes were made, refresh the relevant account listings, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == TRUE) {
		/* Ideally, we would want to recalculate just the affected two
		 * accounts.  However, because the date sort is unclean, any rebuild
		 * will force a resort of the transactions, which will require a
		 * full rebuild of all the open account views. Therefore, call
		 * accview_recalculate_all() to force a full recalculation. This
		 * will in turn sort the data if required.
		 *
		 * The big assumption here is that, because no from or to entries
		 * have changed, none of the accounts will change length and so a
		 * full rebuild is not required.
		 */

		accview_recalculate_all(file);

		/* Force a redraw of the affected line. */

		line = transact_get_line_from_transaction(file, transaction);
		transact_force_window_redraw(file->transacts, line, line);

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the from or to account associated with a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param target	The target field to change.
 * \param new_account	The new account to set the field to.
 */

void transact_change_account(struct file_block *file, tran_t transaction, enum transact_field target, acct_t new_account)
{
	int		line;
	osbool		changed = FALSE;
	unsigned	old_flags;
	acct_t		old_acct = NULL_ACCOUNT;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	account_remove_transaction(file, transaction);

	/* Update the reconcile flag, either removing it, or adding it in. If the
	 * line is the edit line, the icon contents must be manually updated as well.
	 *
	 * If a change is made, this is flagged to allow the update to be recorded properly.
	 */

	/* Look up the account ident as it stands, store the result and
	 * update the name field.  The reconciled flag is set if the
	 * account changes to an income heading; else it is cleared.
	 */

	switch (target) {
	case TRANSACT_FIELD_FROM:
		old_acct = file->transacts->transactions[transaction].from;
		old_flags = file->transacts->transactions[transaction].flags;

		file->transacts->transactions[transaction].from = new_account;

		if (account_get_type(file, new_account) == ACCOUNT_FULL)
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_FROM;
		else
			file->transacts->transactions[transaction].flags |= TRANS_REC_FROM;

		if (old_acct != file->transacts->transactions[transaction].from || old_flags != file->transacts->transactions[transaction].flags)
			changed = TRUE;
		break;
	case TRANSACT_FIELD_TO:
		old_acct = file->transacts->transactions[transaction].to;
		old_flags = file->transacts->transactions[transaction].flags;

		file->transacts->transactions[transaction].to = new_account;

		if (account_get_type(file, new_account) == ACCOUNT_FULL)
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_TO;
		else
			file->transacts->transactions[transaction].flags |= TRANS_REC_TO;

		if (old_acct != file->transacts->transactions[transaction].to || old_flags != file->transacts->transactions[transaction].flags)
			changed = TRUE;
		break;
	case TRANSACT_FIELD_ROW:
	case TRANSACT_FIELD_DATE:
	case TRANSACT_FIELD_REF:
	case TRANSACT_FIELD_AMOUNT:
	case TRANSACT_FIELD_DESC:
	case TRANSACT_FIELD_NONE:
		break;
	}

	/* Return the line to the calculations. This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* Trust that any account views that are open must be based on a valid
	 * date order, and only rebuild those that are directly affected.
         */

	/* If any changes were made, refresh the relevant account listing, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == FALSE)
		return;

	switch (target) {
	case TRANSACT_FIELD_FROM:
		accview_rebuild(file, old_acct);
		accview_rebuild(file, file->transacts->transactions[transaction].from);
		accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);
		break;
	case TRANSACT_FIELD_TO:
		accview_rebuild(file, old_acct);
		accview_rebuild(file, file->transacts->transactions[transaction].to);
		accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
		break;
	case TRANSACT_FIELD_ROW:
	case TRANSACT_FIELD_DATE:
	case TRANSACT_FIELD_REF:
	case TRANSACT_FIELD_AMOUNT:
	case TRANSACT_FIELD_DESC:
	case TRANSACT_FIELD_NONE:
		break;
	}

	/* Force a redraw of the affected line. */

	line = transact_get_line_from_transaction(file, transaction);
	transact_force_window_redraw(file->transacts, line, line);

	file_set_data_integrity(file, TRUE);
}


/**
 * Toggle the state of one of the reconciled flags for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param change_flag	Indicate which reconciled flags to change.
 */

void transact_toggle_reconcile_flag(struct file_block *file, tran_t transaction, enum transact_flags change_flag)
{
	int	line;
	osbool	changed = FALSE;


	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	/* Only do anything if the transaction is inside the limit of the file. */

	account_remove_transaction(file, transaction);

	/* Update the reconcile flag, either removing it, or adding it in.  If the
	 * line is the edit line, the icon contents must be manually updated as well.
	 *
	 * If a change is made, this is flagged to allow the update to be recorded properly.
	 */

	if (file->transacts->transactions[transaction].flags & change_flag) {
		file->transacts->transactions[transaction].flags &= ~change_flag;
		changed = TRUE;
	} else if ((change_flag == TRANS_REC_FROM && file->transacts->transactions[transaction].from != NULL_ACCOUNT) ||
			(change_flag == TRANS_REC_TO && file->transacts->transactions[transaction].to != NULL_ACCOUNT)) {
		file->transacts->transactions[transaction].flags |= change_flag;
		changed = TRUE;
	}

	/* Return the line to the calculations. This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* If any changes were made, refresh the relevant account listing, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == TRUE) {
		if (change_flag == TRANS_REC_FROM)
			accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
		else
			accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);

		/* Force a redraw of the affected line. */

		line = transact_get_line_from_transaction(file, transaction);
		transact_force_window_redraw(file->transacts, line, line);

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the amount of money for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param new_amount	The new amount to set the transaction to.
 */

void transact_change_amount(struct file_block *file, tran_t transaction, amt_t new_amount)
{
	int	line;
	osbool	changed = FALSE;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;
 
	account_remove_transaction(file, transaction);

	/* Look up the existing date, change it and compare the two. If the field
	 * has changed, flag this up.
	 */

	if (new_amount != file->transacts->transactions[transaction].amount) {
		changed = TRUE;
		file->transacts->transactions[transaction].amount = new_amount;
	}

	/* Return the line to the calculations.   This will automatically update all
	 * the account listings.
	  */

	account_restore_transaction(file, transaction);

	/* If any changes were made, refresh the relevant account listings, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == TRUE) {
		accview_recalculate(file, file->transacts->transactions[transaction].from, transaction);
		accview_recalculate(file, file->transacts->transactions[transaction].to, transaction);

		/* Force a redraw of the affected line. */

		line = transact_get_line_from_transaction(file, transaction);
		transact_force_window_redraw(file->transacts, line, line);

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the reference or description associated with a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param target	The target field to change.
 * \param new_text	The new text to set the field to.
 */

void transact_change_refdesc(struct file_block *file, tran_t transaction, enum transact_field target, char *new_text)
{
	int	line;
	osbool	changed = FALSE;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	/* Find the field that will be getting changed. */

	switch (target) {
	case TRANSACT_FIELD_REF:
		if (strcmp(file->transacts->transactions[transaction].reference, new_text) == 0)
			break;

		strncpy(file->transacts->transactions[transaction].reference, new_text, TRANSACT_REF_FIELD_LEN);
		file->transacts->transactions[transaction].reference[TRANSACT_REF_FIELD_LEN - 1] = '\0';
		changed = TRUE;
		break;

	case TRANSACT_FIELD_DESC:
		if (strcmp(file->transacts->transactions[transaction].description, new_text) == 0)
			break;

		strncpy(file->transacts->transactions[transaction].description, new_text, TRANSACT_DESCRIPT_FIELD_LEN);
		file->transacts->transactions[transaction].description[TRANSACT_DESCRIPT_FIELD_LEN - 1] = '\0';
		changed = TRUE;
		break;

	default:
		break;
	}

	/* If any changes were made, refresh the relevant account listings, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == FALSE)
		return;

	/* Refresh any account views that may be affected. */

	accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
	accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);

	/* Force a redraw of the affected line. */

	line = transact_get_line_from_transaction(file, transaction);
	transact_force_window_redraw(file->transacts, line, line);

	file_set_data_integrity(file, TRUE);
}


/**
 * Handle requests for field data from the edit line.
 *
 * \param *data			The block to hold the requested data.
 * \return			TRUE if valid data was returned; FALSE if not.
 */

static osbool transact_edit_get_field(struct edit_data *data)
{
	tran_t			t;
	struct transact_block	*windat;

	if (data == NULL || data->data == NULL)
		return FALSE;

	windat = data->data;

	if (!transact_valid(windat, data->line))
		return FALSE;

	t = windat->transactions[data->line].sort_index;

	switch (data->icon) {
	case TRANSACT_ICON_ROW:
		if (data->type != EDIT_FIELD_DISPLAY || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		snprintf(data->display.text, data->display.length, "%d", transact_get_transaction_number(t));
		data->display.text[data->display.length - 1] = '\0';
		break;
	case TRANSACT_ICON_DATE:
		if (data->type != EDIT_FIELD_DATE)
			return FALSE;

		data->date.date = windat->transactions[t].date;
		break;
	case TRANSACT_ICON_FROM:
		if (data->type != EDIT_FIELD_ACCOUNT_IN)
			return FALSE;

		data->account.account = windat->transactions[t].from;
		data->account.reconciled = (windat->transactions[t].flags & TRANS_REC_FROM) ? TRUE : FALSE;
		break;
	case TRANSACT_ICON_TO:
		if (data->type != EDIT_FIELD_ACCOUNT_OUT)
			return FALSE;

		data->account.account = windat->transactions[t].to;
		data->account.reconciled = (windat->transactions[t].flags & TRANS_REC_TO) ? TRUE : FALSE;
		break;
	case TRANSACT_ICON_AMOUNT:
		if (data->type != EDIT_FIELD_CURRENCY)
			return FALSE;

		data->currency.amount = windat->transactions[t].amount;
		break;
	case TRANSACT_ICON_REFERENCE:
		if (data->type != EDIT_FIELD_TEXT || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		strncpy(data->text.text, windat->transactions[t].reference, data->text.length);
		data->display.text[data->display.length - 1] = '\0';
		break;
	case TRANSACT_ICON_DESCRIPTION:
		if (data->type != EDIT_FIELD_TEXT || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		strncpy(data->text.text, windat->transactions[t].description, data->text.length);
		data->display.text[data->display.length - 1] = '\0';
		break;
	}

	return TRUE;
}


/**
 * Handle returned field data from the edit line.
 *
 * \param *data			The block holding the returned data.
 * \return			TRUE if valid data was returned; FALSE if not.
 */

static osbool transact_edit_put_field(struct edit_data *data)
{
	struct transact_block	*windat;
	int			i;
	tran_t			transaction;
	acct_t			old_account;
	osbool			changed;

#ifdef DEBUG
	debug_printf("Returning complex data for icon %d in row %d", data->icon, data->line);
#endif

	if (data == NULL)
		return FALSE;

	windat = data->data;
	if (windat == NULL || windat->file == NULL)
		return FALSE;

	/* If there is not a transaction entry for the current edit line location
	 * (ie. if this is the first keypress in a new line), extend the transaction
	 * entries to reach the current location.
	 */

	if (data->line >= windat->trans_count) {
		for (i = windat->trans_count; i <= data->line; i++)
			transact_add_raw_entry(windat->file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

		edit_refresh_line_contents(windat->edit_line, TRANSACT_ICON_ROW, -1);
	}

	/* Get out if we failed to create the necessary transactions. */

	if (data->line >= windat->trans_count)
		return FALSE;

	transaction = windat->transactions[data->line].sort_index;

	/* Take the transaction out of the fully calculated results.
	 *
	 * Presets occur with the caret in the Date column, so they will have the
	 * transaction correctly removed before anything happens.
	 */

	if (data->icon != TRANSACT_ICON_REFERENCE && data->icon != TRANSACT_ICON_DESCRIPTION)
		account_remove_transaction(windat->file, transaction);

	/* Process the supplied data. */

	changed = FALSE;
	old_account = NULL_ACCOUNT;

	switch (data->icon) {
	case TRANSACT_ICON_DATE:
		windat->transactions[transaction].date = data->date.date;
		changed = TRUE;
		windat->file->sort_valid = 0;
		break;
	case TRANSACT_ICON_FROM:
		old_account = windat->transactions[transaction].from;
		windat->transactions[transaction].from = data->account.account;
		if (data->account.reconciled == TRUE)
			windat->transactions[transaction].flags |= TRANS_REC_FROM;
		else
			windat->transactions[transaction].flags &= ~TRANS_REC_FROM;

		edit_set_line_colour(windat->edit_line, transact_line_colour(windat, data->line));
		changed = TRUE;
		break;
	case TRANSACT_ICON_TO:
		old_account = windat->transactions[transaction].to;
		windat->transactions[transaction].to = data->account.account;
		if (data->account.reconciled == TRUE)
			windat->transactions[transaction].flags |= TRANS_REC_TO;
		else
			windat->transactions[transaction].flags &= ~TRANS_REC_TO;

		edit_set_line_colour(windat->edit_line, transact_line_colour(windat, data->line));
		changed = TRUE;
		break;
	case TRANSACT_ICON_AMOUNT:
		windat->transactions[transaction].amount = data->currency.amount;
		changed = TRUE;
		break;
	case TRANSACT_ICON_REFERENCE:
		strncpy(windat->transactions[transaction].reference, data->text.text, TRANSACT_REF_FIELD_LEN);
		windat->transactions[transaction].reference[TRANSACT_REF_FIELD_LEN - 1] = '\0';
		changed = TRUE;
		break;
	case TRANSACT_ICON_DESCRIPTION:
		strncpy(windat->transactions[transaction].description, data->text.text, TRANSACT_DESCRIPT_FIELD_LEN);
		windat->transactions[transaction].description[TRANSACT_DESCRIPT_FIELD_LEN - 1] = '\0';
		changed = TRUE;
		break;
	}

	/* Add the transaction back into the accounts calculations.
	 *
	 * From this point on, it is now OK to change the sort order of the
	 * transaction data again!
	 */

	if (data->icon != TRANSACT_ICON_REFERENCE && data->icon != TRANSACT_ICON_DESCRIPTION)
		account_restore_transaction(windat->file, transaction);

	/* Mark the data as unsafe and perform any post-change recalculations that
	 * may affect the order of the transaction data.
	 */

	if (changed == TRUE) {
		file_set_data_integrity(windat->file, TRUE);
		if (data->icon == TRANSACT_ICON_DATE) {
			/* Ideally, we would want to recalculate just the affected two
			 * accounts.  However, because the date sort is unclean, any rebuild
			 * will force a resort of the transactions, which will require a
			 * full rebuild of all the open account views.  Therefore, call
			 * accview_recalculate_all() to force a full recalculation.  This
			 * will in turn sort the data if required.
			 *
			 * The big assumption here is that, because no from or to entries
			 * have changed, none of the accounts will change length and so a
			 * full rebuild is not required.
			 */

			accview_recalculate_all(windat->file);
		} else if (data->icon == TRANSACT_ICON_FROM) {
			/* Trust that any accuout views that are open must be based on a
			 * valid date order, and only rebuild those that are directly affected.
			 */

			accview_rebuild(windat->file, old_account);
//FIXME -- Changed?	transaction = windat->transactions[line].sort_index;
			accview_rebuild(windat->file, windat->transactions[transaction].from);
//FIXME -- Changed?	transaction = windat->transactions[line].sort_index;
			accview_redraw_transaction(windat->file, windat->transactions[transaction].to, transaction);
		} else if (data->icon == TRANSACT_ICON_TO) {
			/* Trust that any accuout views that are open must be based on a
			 * valid date order, and only rebuild those that are directly affected.
			 */

			accview_rebuild(windat->file, old_account);
//FIXME -- Changed?	transaction = windat->transactions[line].sort_index;
			accview_rebuild(windat->file, windat->transactions[transaction].to);
//FIXME -- Changed?	transaction = windat->transactions[line].sort_index;
			accview_redraw_transaction(windat->file, windat->transactions[transaction].from, transaction);
		} else if (data->icon == TRANSACT_ICON_AMOUNT) {
			accview_recalculate(windat->file, windat->transactions[transaction].from, transaction);
			accview_recalculate(windat->file, windat->transactions[transaction].to, transaction);
		} else if (data->icon == TRANSACT_ICON_REFERENCE || data->icon == TRANSACT_ICON_DESCRIPTION) {
			accview_redraw_transaction(windat->file, windat->transactions[transaction].from, transaction);
			accview_redraw_transaction(windat->file, windat->transactions[transaction].to, transaction);
		}
	}

	/* Finally, look for the next reconcile line if that is necessary.
	 *
	 * This is done last, as the only hold we have over the line being edited
	 * is the edit line location.  Move that and we've lost everything.
	 */

	if ((data->icon == TRANSACT_ICON_FROM || data->icon == TRANSACT_ICON_TO) &&
			(data->key == '+' || data->key == '=' || data->key == '-' || data->key == '_'))
		transact_find_next_reconcile_line(windat, FALSE);

	return TRUE;
}


/**
 * Find the next line of an account, based on its reconcoled status, and place
 * the caret into the unreconciled account field.
 *
 * \param *windat		The transaction window instance to search in.
 * \param set			TRUE to match reconciled lines; FALSE to match unreconciled ones.
 */

static void transact_find_next_reconcile_line(struct transact_block *windat, osbool set)
{
	int			line;
	acct_t			account;
	enum transact_field	found;
	wimp_caret		caret;

	if (windat == NULL || windat->file == NULL || windat->file->auto_reconcile == FALSE)
		return;

	line = edit_get_line(windat->edit_line);
	account = NULL_ACCOUNT;

	wimp_get_caret_position(&caret);

	if (caret.i == TRANSACT_ICON_FROM)
		account = windat->transactions[windat->transactions[line].sort_index].from;
	else if (caret.i == TRANSACT_ICON_TO)
		account = windat->transactions[windat->transactions[line].sort_index].to;

	if (account == NULL_ACCOUNT)
		return;

	line++;
	found = TRANSACT_FIELD_NONE;

	while ((line < windat->trans_count) && (found == TRANSACT_FIELD_NONE)) {
		if (windat->transactions[windat->transactions[line].sort_index].from == account &&
				((windat->transactions[windat->transactions[line].sort_index].flags & TRANS_REC_FROM) ==
						((set) ? TRANS_REC_FROM : TRANS_FLAGS_NONE)))
			found = TRANSACT_FIELD_FROM;
		else if (windat->transactions[windat->transactions[line].sort_index].to == account &&
				((windat->transactions[windat->transactions[line].sort_index].flags & TRANS_REC_TO) ==
						((set) ? TRANS_REC_TO : TRANS_FLAGS_NONE)))
			found = TRANSACT_FIELD_TO;
		else
			line++;
	}

	if (found != TRANSACT_FIELD_NONE)
		transact_place_caret(windat->file, line, found);
}


/**
 * Process auto-complete requests from the edit line for one of the transaction
 * fields.
 * 
 * \param *data			The field auto-complete data.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool transact_edit_auto_complete(struct edit_data *data)
{
	struct transact_block	*windat;
	tran_t			transaction;

	if (data == NULL)
		return FALSE;

	windat = data->data;
	if (windat == NULL || windat->file == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("Requesting auto-completion");
#endif

	/* We can only complete text fields at the moment, as none of the others make sense. */

	if (data->type != EDIT_FIELD_TEXT)
		return FALSE;

	/* Process the Reference or Descripton field as appropriate. */

	switch (data->icon) {
	case TRANSACT_ICON_REFERENCE:
		/* To complete the reference, we need to be in a valid transaction which contains
		 * at least one of the From or To fields.
		 */

		if (data->line >= windat->trans_count)
			return FALSE;

		transaction = windat->transactions[data->line].sort_index;

		if (!transact_valid(windat, transaction))
			return FALSE;

		account_get_next_cheque_number(windat->file, windat->transactions[transaction].from, windat->transactions[transaction].to,
				1, data->text.text, data->text.length);
		break;

	case TRANSACT_ICON_DESCRIPTION:
		/* The description field can be completed whether or not there's an underlying
		 * transaction, as we just search back up from the last valid line.
		 */

		transact_complete_description(windat->file, data->line, data->text.text, data->text.length);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}


/**
 * Complete a description field, by finding the most recent description in the file
 * which starts with the same characters as the current line.
 *
 * \param *file		The file containing the transaction.
 * \param line		The transaction line to be completed.
 * \param *buffer	Pointer to the buffer to be completed.
 * \param length	The length of the buffer.
 * \return		Pointer to the completed buffer.
 */

static char *transact_complete_description(struct file_block *file, int line, char *buffer, size_t length)
{
	tran_t	t;

	if (file == NULL || file->transacts == NULL || buffer == NULL)
		return buffer;

	if (line >= file->transacts->trans_count)
		line = file->transacts->trans_count - 1;
	else
		line -= 1;

	for (; line >= 0; line--) {
		t = file->transacts->transactions[line].sort_index;

		if (*(file->transacts->transactions[t].description) != '\0' &&
				string_nocase_strstr(file->transacts->transactions[t].description, buffer) == file->transacts->transactions[t].description) {
			strncpy(buffer, file->transacts->transactions[t].description, length);
			buffer[length - 1] = '\0';
			break;
		}
	}

	return buffer;
}


/**
 * Process preset insertion requests from the edit line.
 * 
 * \param line			The line at which to insert the preset.
 * \param key			The Wimp Key number triggering the request.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool transact_edit_insert_preset(int line, wimp_key_no key, void *data)
{
	struct transact_block	*windat;
	preset_t		preset;

	if (data == NULL)
		return FALSE;

	windat = data;
	if (windat == NULL || windat->file == NULL)
		return FALSE;

	/* Identify the preset to be inserted. */

	preset = preset_find_from_keypress(windat->file, toupper(key));

	if (preset == NULL_PRESET)
		return TRUE;

	return transact_insert_preset_into_line(windat->file, line, preset);
}


/**
 * Insert a preset into a pre-existing transaction, taking care of updating all
 * the file data in a clean way.
 *
 * \param *file		The file to edit.
 * \param line		The line in the transaction window to update.
 * \param preset	The preset to insert into the transaction.
 * \return		TRUE if successful; FALSE on failure.
 */

osbool transact_insert_preset_into_line(struct file_block *file, int line, preset_t preset)
{
	enum transact_field	changed = TRANSACT_FIELD_NONE;
	tran_t			transaction;
	int			i;


	if (file == NULL || file->transacts == NULL || file->transacts->edit_line == NULL || !preset_test_index_valid(file, preset))
		return FALSE;

	/* Identify the transaction to be updated. */
	/* If there is not a transaction entry for the current edit line location
	 * (ie. if this is the first keypress in a new line), extend the transaction
	 * entries to reach the current location.
	 */

	if (line >= file->transacts->trans_count) {
		for (i = file->transacts->trans_count; i <= line; i++)
			transact_add_raw_entry(file->transacts->file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

		edit_refresh_line_contents(file->transacts->edit_line, TRANSACT_ICON_ROW, -1);
	}

	if (line >= file->transacts->trans_count)
		return FALSE;

	transaction = file->transacts->transactions[line].sort_index;

	if (!transact_valid(file->transacts, transaction))
		return FALSE;

	/* Remove the target transaction from all calculations. */

	account_remove_transaction(file, transaction);

	/* Apply the preset to the transaction. */

	changed = preset_apply(file, preset, &(file->transacts->transactions[transaction].date),
			&(file->transacts->transactions[transaction].from),
			&(file->transacts->transactions[transaction].to),
			&(file->transacts->transactions[transaction].flags),
			&(file->transacts->transactions[transaction].amount),
			file->transacts->transactions[transaction].reference,
			file->transacts->transactions[transaction].description);

	/* Return the line to the calculations.  This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* Replace the edit line to make it pick up the changes. */

	transact_place_edit_line(file->transacts, line);

	/* Put the caret at the end of the preset destination field. */

	icons_put_caret_at_end(file->transacts->transaction_window,
			transact_convert_preset_icon_number(preset_get_caret_destination(file, preset)));

	/* If nothing changed, there's no more to do. */

	if (changed == TRANSACT_FIELD_NONE)
		return TRUE;

	if (changed & TRANSACT_FIELD_DATE)
		file->sort_valid = FALSE;

	/* If any changes were made, refresh the relevant account listing, redraw
	 * the transaction window line and mark the file as modified.
	 */

	accview_rebuild_all(file);

	/* Force a redraw of the affected line. */

	transact_force_window_redraw(file->transacts, line, line);


	/* If we're auto-sorting, and the sort column has been updated as
	 * part of the preset, then do an auto sort now.
	 *
	 * We will always sort if the sort column is Date, because pressing
	 * a preset key is analagous to hitting Return.
	 */

	if (config_opt_read("AutoSort") && (
			((file->transacts->sort_order & SORT_MASK) == SORT_DATE) ||
			((changed & TRANSACT_FIELD_FROM) && ((file->transacts->sort_order & SORT_MASK) == SORT_FROM)) ||
			((changed & TRANSACT_FIELD_TO) && ((file->transacts->sort_order & SORT_MASK) == SORT_TO)) ||
			((changed & TRANSACT_FIELD_REF) && ((file->transacts->sort_order & SORT_MASK) == SORT_REFERENCE)) ||
			((changed & TRANSACT_FIELD_AMOUNT) && ((file->transacts->sort_order & SORT_MASK) == SORT_AMOUNT)) ||
			((changed & TRANSACT_FIELD_DESC) && ((file->transacts->sort_order & SORT_MASK) == SORT_DESCRIPTION)))) {
		transact_sort(file->transacts);
		
		if (transact_valid(file->transacts, line)) {
			accview_sort(file, file->transacts->transactions[file->transacts->transactions[line].sort_index].from);
			accview_sort(file, file->transacts->transactions[file->transacts->transactions[line].sort_index].to);
		}
	}


	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Take a preset caret destination as used in the preset blocks, and convert it
 * into an icon number for the transaction edit line.
 *
 * \param caret		The preset caret destination to be converted.
 * \return		The corresponding icon number.
 */

static wimp_i transact_convert_preset_icon_number(enum preset_caret caret)
{
	wimp_i	icon;

	switch (caret) {
	case PRESET_CARET_DATE:
		icon = TRANSACT_ICON_DATE;
		break;

	case PRESET_CARET_FROM:
		icon = TRANSACT_ICON_FROM;
		break;

	case PRESET_CARET_TO:
		icon = TRANSACT_ICON_TO;
		break;

	case PRESET_CARET_REFERENCE:
		icon = TRANSACT_ICON_REFERENCE;
		break;

	case PRESET_CARET_AMOUNT:
		icon = TRANSACT_ICON_AMOUNT;
		break;

	case PRESET_CARET_DESCRIPTION:
		icon = TRANSACT_ICON_DESCRIPTION;
		break;

	default:
		icon = TRANSACT_ICON_DATE;
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

static int transact_edit_auto_sort(wimp_i icon, void *data)
{
	struct transact_block	*windat = data;
	int			entry_line;

	if (windat == NULL || windat->file == NULL)
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

	if ((icon == TRANSACT_ICON_DATE && (windat->sort_order & SORT_MASK) != SORT_DATE) ||
			(icon == TRANSACT_ICON_FROM && (windat->sort_order & SORT_MASK) != SORT_FROM) ||
			(icon == TRANSACT_ICON_TO && (windat->sort_order & SORT_MASK) != SORT_TO) ||
			(icon == TRANSACT_ICON_REFERENCE && (windat->sort_order & SORT_MASK) != SORT_REFERENCE) ||
			(icon == TRANSACT_ICON_AMOUNT && (windat->sort_order & SORT_MASK) != SORT_AMOUNT) ||
			(icon == TRANSACT_ICON_DESCRIPTION && (windat->sort_order & SORT_MASK) != SORT_DESCRIPTION))
		return TRUE;

	/* Sort the transactions. */

	transact_sort(windat);

	/* Re-sort any affected account views. */

	entry_line = edit_get_line(windat->edit_line);

	if (transact_valid(windat, entry_line)) {
		accview_sort(windat->file, windat->transactions[windat->transactions[entry_line].sort_index].from);
		accview_sort(windat->file, windat->transactions[windat->transactions[entry_line].sort_index].to);
	}

	return TRUE;
}


/**
 * Find the Wimp colour of a given line in a transaction window.
 * 
 * \param *windat		The transaction window instance of interest.
 * \param line			The line of interest.
 * \return			The required Wimp colour, or Black on failure.
 */

static wimp_colour transact_line_colour(struct transact_block *windat, int line)
{
	tran_t	transaction;

	if (windat == NULL || windat->transactions == NULL || line < 0 || line >= windat->trans_count)
		return wimp_COLOUR_BLACK;

	transaction = windat->transactions[line].sort_index;

	if (config_opt_read("ShadeReconciled") &&
			((windat->transactions[transaction].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
		return config_int_read("ShadeReconciledColour");
	else
		return wimp_COLOUR_BLACK;
}


/**
 * Open the Transaction List Sort dialogue for a given transaction list window.
 *
 * \param *windat		The transaction window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void transact_open_sort_window(struct transact_block *windat, wimp_pointer *ptr)
{
	if (windat == NULL || ptr == NULL)
		return;

	sort_open_dialogue(transact_sort_dialogue, ptr, windat->sort_order, windat);
}


/**
 * Take the contents of an updated Transaction List Sort window and process
 * the data.
 *
 * \param order			The new sort order selected by the user.
 * \param *data			The transaction window associated with the Sort dialogue.
 * \return			TRUE if successful; else FALSE.
 */

static osbool transact_process_sort_window(enum sort_type order, void *data)
{
	struct transact_block	*windat = (struct transact_block *) data;

	if (windat == NULL)
		return FALSE;

	windat->sort_order = order;

	transact_adjust_sort_icon(windat);
	windows_redraw(windat->transaction_pane);
	transact_sort(windat);

	return TRUE;
}


/**
 * Sort the contents of the transaction window based on the file's sort setting.
 *
 * \param *windat		The transaction window instance to sort.
 */

void transact_sort(struct transact_block *windat)
{
	wimp_caret	caret;
	int		gap, comb, temp, order;
	tran_t		edit_transaction;
	osbool		sorted, reorder;

	if (windat == NULL || windat->file == NULL)
		return;

#ifdef DEBUG
	debug_printf("Sorting transaction window");
#endif

	hourglass_on();

	/* Find the caret position and edit line before sorting. */

	wimp_get_caret_position(&caret);
	edit_transaction = transact_find_edit_line_by_transaction(windat);

	/* Sort the entries using a combsort.  This has the advantage over qsort() that the order of entries is only
	 * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
	 */

	gap = windat->trans_count - 1;

	order = windat->sort_order;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((windat->trans_count >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < windat->trans_count; comb++) {
			switch (order) {
			case SORT_ROW | SORT_ASCENDING:
				reorder = (transact_get_transaction_number(windat->transactions[comb+gap].sort_index) <
						transact_get_transaction_number(windat->transactions[comb].sort_index));
				break;

			case SORT_ROW | SORT_DESCENDING:
				reorder = (transact_get_transaction_number(windat->transactions[comb+gap].sort_index) >
						transact_get_transaction_number(windat->transactions[comb].sort_index));
				break;

			case SORT_DATE | SORT_ASCENDING:
				reorder = (windat->transactions[windat->transactions[comb+gap].sort_index].date <
						windat->transactions[windat->transactions[comb].sort_index].date);
				break;

			case SORT_DATE | SORT_DESCENDING:
				reorder = (windat->transactions[windat->transactions[comb+gap].sort_index].date >
						windat->transactions[windat->transactions[comb].sort_index].date);
				break;

			case SORT_FROM | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(windat->file, windat->transactions[windat->transactions[comb+gap].sort_index].from),
						account_get_name(windat->file, windat->transactions[windat->transactions[comb].sort_index].from)) < 0);
				break;

			case SORT_FROM | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(windat->file, windat->transactions[windat->transactions[comb+gap].sort_index].from),
						account_get_name(windat->file, windat->transactions[windat->transactions[comb].sort_index].from)) > 0);
				break;

			case SORT_TO | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(windat->file, windat->transactions[windat->transactions[comb+gap].sort_index].to),
						account_get_name(windat->file, windat->transactions[windat->transactions[comb].sort_index].to)) < 0);
				break;

			case SORT_TO | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(windat->file, windat->transactions[windat->transactions[comb+gap].sort_index].to),
						account_get_name(windat->file, windat->transactions[windat->transactions[comb].sort_index].to)) > 0);
				break;

			case SORT_REFERENCE | SORT_ASCENDING:
				reorder = (strcmp(windat->transactions[windat->transactions[comb+gap].sort_index].reference,
						windat->transactions[windat->transactions[comb].sort_index].reference) < 0);
				break;

			case SORT_REFERENCE | SORT_DESCENDING:
				reorder = (strcmp(windat->transactions[windat->transactions[comb+gap].sort_index].reference,
						windat->transactions[windat->transactions[comb].sort_index].reference) > 0);
				break;

			case SORT_AMOUNT | SORT_ASCENDING:
				reorder = (windat->transactions[windat->transactions[comb+gap].sort_index].amount <
						windat->transactions[windat->transactions[comb].sort_index].amount);
				break;

			case SORT_AMOUNT | SORT_DESCENDING:
				reorder = (windat->transactions[windat->transactions[comb+gap].sort_index].amount >
						windat->transactions[windat->transactions[comb].sort_index].amount);
				break;

			case SORT_DESCRIPTION | SORT_ASCENDING:
				reorder = (strcmp(windat->transactions[windat->transactions[comb+gap].sort_index].description,
						windat->transactions[windat->transactions[comb].sort_index].description) < 0);
				break;

			case SORT_DESCRIPTION | SORT_DESCENDING:
				reorder = (strcmp(windat->transactions[windat->transactions[comb+gap].sort_index].description,
						windat->transactions[windat->transactions[comb].sort_index].description) > 0);
				break;

			default:
				reorder = FALSE;
				break;
			}

			if (reorder) {
				temp = windat->transactions[comb+gap].sort_index;
				windat->transactions[comb+gap].sort_index = windat->transactions[comb].sort_index;
				windat->transactions[comb].sort_index = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

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
 * Sort the underlying transaction data within a file, to put them into date order.
 * This does not affect the view in the transaction window -- to sort this, use
 * transact_sort().  As a result, we do not need to look after the location of
 * things like the edit line; it does need to keep track of transactions[].sort_index,
 * however.
 *
 * \param *file			The file to be sorted.
 */

void transact_sort_file_data(struct file_block *file)
{
	int			i, gap, comb;
	osbool			sorted;
	struct transaction	temp;

#ifdef DEBUG
	debug_printf("Sorting transactions");
#endif

	if (file == NULL || file->transacts == NULL)
		return;

	hourglass_on();

	/* Start by recording the order of the transactions on display in the
	 * main window, and also the order of the transactions themselves.
	 */

	for (i=0; i < file->transacts->trans_count; i++) {
		file->transacts->transactions[file->transacts->transactions[i].sort_index].saved_sort = i;	/* Record transaction window lines. */
		file->transacts->transactions[i].sort_index = i;					/* Record old transaction locations. */
	}

	/* Sort the entries using a combsort.  This has the advantage over qsort()
	 * that the order of entries is only affected if they are not equal and are
	 * in descending order.  Otherwise, the status quo is left.
	 */

	gap = file->transacts->trans_count - 1;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((file->transacts->trans_count >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < file->transacts->trans_count; comb++) {
			if (file->transacts->transactions[comb+gap].date < file->transacts->transactions[comb].date) {
				temp = file->transacts->transactions[comb+gap];
				file->transacts->transactions[comb+gap] = file->transacts->transactions[comb];
				file->transacts->transactions[comb] = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

	/* Finally, restore the order of the transactions on display in the
	 * main window.
	 */

	for (i=0; i < file->transacts->trans_count; i++)
		file->transacts->transactions[file->transacts->transactions[i].sort_index].sort_workspace = i;

	accview_reindex_all(file);

	for (i=0; i < file->transacts->trans_count; i++)
		file->transacts->transactions[file->transacts->transactions[i].saved_sort].sort_index = i;

	file->sort_valid = TRUE;

	hourglass_off();
}


/**
 * Purge unused transactions from a file.
 *
 * \param *file			The file to purge.
 * \param cutoff		The cutoff date before which transactions should be removed.
 */

void transact_purge(struct file_block *file, date_t cutoff)
{
	tran_t			transaction;
	enum transact_flags	flags;
	acct_t			from, to;
	date_t			date;
	amt_t			amount;

	if (file == NULL || file->transacts == NULL)
		return;

	for (transaction = 0; transaction < file->transacts->trans_count; transaction++) {
		date = transact_get_date(file, transaction);
		flags = transact_get_flags(file, transaction);
	
		if ((flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO) &&
				(cutoff == NULL_DATE || date < cutoff)) {
			from = transact_get_from(file, transaction);
			to = transact_get_to(file, transaction);
			amount = transact_get_amount(file, transaction);

			/* If the from and to accounts are full accounts, */

			if (from != NULL_ACCOUNT && (account_get_type(file, from) & ACCOUNT_FULL) != 0)
				account_adjust_opening_balance(file, from, -amount);

			if (to != NULL_ACCOUNT && (account_get_type(file, to) & ACCOUNT_FULL) != 0)
				account_adjust_opening_balance(file, to, +amount);

			transact_clear_raw_entry(file, transaction);
		}
	}

	if (file->sort_valid == 0)
		transact_sort_file_data(file);

	transact_strip_blanks_from_end(file);
}


/**
 * Open the Transaction Print dialogue for a given account list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void transact_open_print_window(struct file_block *file, wimp_pointer *ptr, int clear)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_print_owner = file->transacts;

	printing_open_advanced_window(file, ptr, clear, "PrintTransact", transact_print);
}


/**
 * Send the contents of the Transaction Window to the printer, via the reporting
 * system.
 *
 * \param text			TRUE to print in text format; FALSE for graphics.
 * \param format		TRUE to apply text formatting in text mode.
 * \param scale			TRUE to scale width in graphics mode.
 * \param rotate		TRUE to print landscape in grapics mode.
 * \param pagenum		TRUE to include page numbers in graphics mode.
 * \param from			The date to print from.
 * \param to			The date to print to.
 */

static void transact_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum, date_t from, date_t to)
{
	struct report		*report;
	int			i, t;
	char			line[4096], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];

	if (transact_print_owner == NULL || transact_print_owner->file == NULL)
		return;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);
	msgs_lookup("PrintTitleTransact", buffer, sizeof(buffer));
	report = report_open(transact_print_owner->file, buffer, NULL);

	if (report == NULL) {
		error_msgs_report_error("PrintMemFail");
		return;
	}

	hourglass_on();

	/* Output the page title. */

	file_get_leafname(transact_print_owner->file, numbuf1, sizeof(numbuf1));
	msgs_param_lookup("TransTitle", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
	sprintf(line, "\\b\\u%s", buffer);
	report_write_line(report, 1, line);
	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	*line = '\0';
	sprintf(buffer, "\\k\\b\\u%s\\t", icons_copy_text(transact_print_owner->transaction_pane, TRANSACT_PANE_ROW, numbuf1, sizeof(numbuf1)));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(transact_print_owner->transaction_pane, TRANSACT_PANE_DATE, numbuf1, sizeof(numbuf1)));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(transact_print_owner->transaction_pane, TRANSACT_PANE_FROM, numbuf1, sizeof(numbuf1)));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(transact_print_owner->transaction_pane, TRANSACT_PANE_TO, numbuf1, sizeof(numbuf1)));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(transact_print_owner->transaction_pane, TRANSACT_PANE_REFERENCE, numbuf1, sizeof(numbuf1)));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\r\\u%s\\t", icons_copy_text(transact_print_owner->transaction_pane, TRANSACT_PANE_AMOUNT, numbuf1, sizeof(numbuf1)));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(transact_print_owner->transaction_pane, TRANSACT_PANE_DESCRIPTION, numbuf1, sizeof(numbuf1)));
	strcat(line, buffer);

	report_write_line(report, 0, line);

	/* Output the transaction data as a set of delimited lines. */

	for (i=0; i < transact_print_owner->trans_count; i++) {
		if ((from == NULL_DATE || transact_print_owner->transactions[i].date >= from) &&
				(to == NULL_DATE || transact_print_owner->transactions[i].date <= to)) {
			*line = '\0';

			t = transact_print_owner->transactions[i].sort_index;

			date_convert_to_string(transact_print_owner->transactions[t].date, numbuf1, sizeof(numbuf1));
			sprintf(buffer, "\\k\\d\\r%d\\t%s\\t", transact_get_transaction_number(t), numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_ident(transact_print_owner->file, transact_print_owner->transactions[t].from));
			strcat(line, buffer);

			strcpy(numbuf1, (transact_print_owner->transactions[t].flags & TRANS_REC_FROM) ? rec_char : "");
			sprintf(buffer, "%s\\t", numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_name(transact_print_owner->file, transact_print_owner->transactions[t].from));
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_ident(transact_print_owner->file, transact_print_owner->transactions[t].to));
			strcat(line, buffer);

			strcpy(numbuf1, (transact_print_owner->transactions[t].flags & TRANS_REC_TO) ? rec_char : "");
			sprintf(buffer, "%s\\t", numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_name(transact_print_owner->file, transact_print_owner->transactions[t].to));
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", transact_print_owner->transactions[t].reference);
			strcat(line, buffer);

			currency_convert_to_string(transact_print_owner->transactions[t].amount, numbuf1, sizeof(numbuf1));
			sprintf(buffer, "\\r%s\\t", numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", transact_print_owner->transactions[t].description);
			strcat(line, buffer);

			report_write_line(report, 0, line);
		}
	}

	hourglass_off();

	report_close_and_print(report, text, format, scale, rotate, pagenum);
}












/**
 * Save the transaction details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void transact_write_file(struct file_block *file, FILE *out)
{
	int	i;
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (file == NULL || file->transacts == NULL)
		return;

	fprintf(out, "\n[Transactions]\n");

	fprintf(out, "Entries: %x\n", file->transacts->trans_count);

	column_write_as_text(file->transacts->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	fprintf(out, "SortOrder: %x\n", file->transacts->sort_order);

	for (i = 0; i < file->transacts->trans_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x,%x\n",
				file->transacts->transactions[i].date, file->transacts->transactions[i].flags, file->transacts->transactions[i].from,
				file->transacts->transactions[i].to, file->transacts->transactions[i].amount);
		if (*(file->transacts->transactions[i].reference) != '\0')
			config_write_token_pair(out, "Ref", file->transacts->transactions[i].reference);
		if (*(file->transacts->transactions[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->transacts->transactions[i].description);
	}
}


/**
 * Read transaction details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param format		The format number of the file.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

enum config_read_status transact_read_file(struct file_block *file, FILE *in, char *section, char *token, char *value, int format, osbool *unknown_data)
{
	int	result, block_size, i = -1;

	block_size = flex_size((flex_ptr) &(file->transacts->transactions)) / sizeof(struct transaction);

	do {
		if (string_nocase_strcmp(token, "Entries") == 0) {
			block_size = strtoul(value, NULL, 16);
			if (block_size > file->transacts->trans_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->transacts->transactions), sizeof(struct transaction) * block_size);
			} else {
				block_size = file->transacts->trans_count;
			}
		} else if (string_nocase_strcmp(token, "WinColumns") == 0) {
			/* For file format 1.00 or older, there's no row column at the
			 * start of the line so skip on to colunn 1 (date).
			 */
			column_init_window(file->transacts->columns, (format <= 100) ? 1 : 0, TRUE, value);
		} else if (string_nocase_strcmp(token, "SortOrder") == 0){
			file->transacts->sort_order = strtoul(value, NULL, 16);
		} else if (string_nocase_strcmp(token, "@") == 0) {
			file->transacts->trans_count++;
			if (file->transacts->trans_count > block_size) {
				block_size = file->transacts->trans_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->transacts->transactions), sizeof(struct transaction) * block_size);
			}
			i = file->transacts->trans_count-1;
			file->transacts->transactions[i].date = strtoul(next_field (value, ','), NULL, 16);
			file->transacts->transactions[i].flags = strtoul(next_field (NULL, ','), NULL, 16);
			file->transacts->transactions[i].from = strtoul(next_field (NULL, ','), NULL, 16);
			file->transacts->transactions[i].to = strtoul(next_field (NULL, ','), NULL, 16);
			file->transacts->transactions[i].amount = strtoul(next_field (NULL, ','), NULL, 16);

			*(file->transacts->transactions[i].reference) = '\0';
			*(file->transacts->transactions[i].description) = '\0';

			file->transacts->transactions[i].sort_index = i;
		} else if (i != -1 && string_nocase_strcmp(token, "Ref") == 0) {
			strcpy(file->transacts->transactions[i].reference, value);
		} else if (i != -1 && string_nocase_strcmp(token, "Desc") == 0) {
			strcpy(file->transacts->transactions[i].description, value);
		} else {
			*unknown_data = TRUE;
		}

		result = config_read_token_pair(in, token, value, section);
	} while (result != sf_CONFIG_READ_EOF && result != sf_CONFIG_READ_NEW_SECTION);

	block_size = flex_size((flex_ptr) &(file->transacts->transactions)) / sizeof(struct transaction);

	#ifdef DEBUG
	debug_printf("Transaction block size: %d, required: %d", block_size, file->transacts->trans_count);
	#endif

	if (block_size > file->transacts->trans_count) {
		block_size = file->transacts->trans_count;
		flex_extend((flex_ptr) &(file->transacts->transactions), sizeof(struct transaction) * block_size);

		#ifdef DEBUG
		debug_printf("Block shrunk to %d", block_size);
		#endif
	}

	return result;
}


/**
 * Save a file directly, if it already has a filename associated with it, or
 * open a save dialogue.
 *
 * \param *windat		The window to save.
 */

static void transact_start_direct_save(struct transact_block *windat)
{
	wimp_pointer	pointer;

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

static osbool transact_save_file(char *filename, osbool selection, void *data)
{
	struct transact_block *windat = data;

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

static osbool transact_save_csv(char *filename, osbool selection, void *data)
{
	struct transact_block *windat = data;

	if (windat == NULL)
		return FALSE;

	transact_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the transaction data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool transact_save_tsv(char *filename, osbool selection, void *data)
{
	struct transact_block *windat = data;

	if (windat == NULL)
		return FALSE;

	transact_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

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

static void transact_export_delimited(struct transact_block *windat, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE	*out;
	int	i, t;
	char	buffer[256];

	if (windat == NULL || windat->file == NULL)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	icons_copy_text(windat->transaction_pane, TRANSACT_PANE_ROW, buffer, sizeof(buffer));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->transaction_pane, TRANSACT_PANE_DATE, buffer, sizeof(buffer));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->transaction_pane, TRANSACT_PANE_FROM, buffer, sizeof(buffer));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->transaction_pane, TRANSACT_PANE_TO, buffer, sizeof(buffer));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->transaction_pane, TRANSACT_PANE_REFERENCE, buffer, sizeof(buffer));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->transaction_pane, TRANSACT_PANE_AMOUNT, buffer, sizeof(buffer));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->transaction_pane, TRANSACT_PANE_DESCRIPTION, buffer, sizeof(buffer));
	filing_output_delimited_field(out, buffer, format, DELIMIT_LAST);

	/* Output the transaction data as a set of delimited lines. */

	for (i=0; i < windat->trans_count; i++) {
		t = windat->transactions[i].sort_index;

		snprintf(buffer, sizeof(buffer), "%d", transact_get_transaction_number(t));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		date_convert_to_string(windat->transactions[t].date, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		account_build_name_pair(windat->file, windat->transactions[t].from, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		account_build_name_pair(windat->file, windat->transactions[t].to, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		filing_output_delimited_field(out, windat->transactions[t].reference, format, DELIMIT_NONE);

		currency_convert_to_string(windat->transactions[t].amount, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, windat->transactions[t].description, format, DELIMIT_LAST);
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

static osbool transact_load_csv(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data)
{
	struct file_block	*file = data;

	if (filetype != dataxfer_TYPE_CSV || file == NULL)
		return FALSE;

	filing_import_csv_file(file, filename);

	return TRUE;
}


/**
 * Search the transactions, returning the entry from nearest the target date.
 * The transactions will be sorted into order if they are not already.
 *
 * \param *file			The file to search in.
 * \param target		The target date.
 * \return			The transaction number for the date, or
 *				NULL_TRANSACTION.
 */

int transact_find_date(struct file_block *file, date_t target)
{
	int		min, max, mid;

	if (file == NULL || file->transacts || file->transacts->trans_count == 0 || target == NULL_DATE)
		return NULL_TRANSACTION;

	/* If the transactions are not already sorted, sort them into date
	 * order.
	 */

	if (file->sort_valid == 0)
		transact_sort_file_data(file);

	/* Search through the sorted array using a binary search. */

	min = 0;
	max = file->transacts->trans_count - 1;

	while (min < max) {
		mid = (min + max)/2;

		if (target <= file->transacts->transactions[mid].date)
			max = mid;
		else
			min = mid + 1;
	}

	return min;
}


/**
 * Place the caret in a given line in a transaction window, and scroll
 * the line into view.
 *
 * \param *file			The file to operate on.
 * \param line			The line (under the current display sort order)
 *				to place the caret in.
 * \param field			The field to place the caret in.
 */

void transact_place_caret(struct file_block *file, int line, enum transact_field field)
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

	icons_put_caret_at_end(file->transacts->transaction_window, icon);
	transact_find_edit_line_vertically(file->transacts);
}


/**
 * Search the transaction list from a file for a set of matching entries.
 *
 * \param *file			The file to search in.
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

enum transact_field transact_search(struct file_block *file, int *line, osbool back, osbool case_sensitive, osbool logic_and,
		date_t date, acct_t from, acct_t to, enum transact_flags flags, amt_t amount, char *ref, char *desc)
{
	enum transact_field	test = TRANSACT_FIELD_NONE, original = TRANSACT_FIELD_NONE;
	osbool			match = FALSE;
	enum transact_flags	from_rec, to_rec;
	int			transaction;


	if (file == NULL || file->transacts == NULL)
		return TRANSACT_FIELD_NONE;

	match = FALSE;

	from_rec = flags & TRANS_REC_FROM;
	to_rec = flags & TRANS_REC_TO;

	while (*line < file->transacts->trans_count && *line >= 0 && !match) {
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

		transaction = file->transacts->transactions[*line].sort_index;

		if (desc != NULL && *desc != '\0' && string_wildcard_compare(desc, file->transacts->transactions[transaction].description, !case_sensitive)) {
			test ^= TRANSACT_FIELD_DESC;
		}

		if (amount != NULL_CURRENCY && amount == file->transacts->transactions[transaction].amount) {
			test ^= TRANSACT_FIELD_AMOUNT;
		}

		if (ref != NULL && *ref != '\0' && string_wildcard_compare(ref, file->transacts->transactions[transaction].reference, !case_sensitive)) {
			test ^= TRANSACT_FIELD_REF;
		}

		/* The following two tests check that a) an account has been specified, b) it is the same as the transaction and
		 * c) the two reconcile flags are set the same (if they are, the EOR operation cancels them out).
		 */

		if (to != NULL_ACCOUNT && to == file->transacts->transactions[transaction].to &&
				((to_rec ^ file->transacts->transactions[transaction].flags) & TRANS_REC_TO) == 0) {
			test ^= TRANSACT_FIELD_TO;
		}

		if (from != NULL_ACCOUNT && from == file->transacts->transactions[transaction].from &&
				((from_rec ^ file->transacts->transactions[transaction].flags) & TRANS_REC_FROM) == 0) {
			test ^= TRANSACT_FIELD_FROM;
		}

		if (date != NULL_DATE && date == file->transacts->transactions[transaction].date) {
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

	if (!match)
		return TRANSACT_FIELD_NONE;

	return (logic_and) ? original : test;
}


/**
 * Check the transactions in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool transact_check_account(struct file_block *file, acct_t account)
{
	int		i;
	osbool		found = FALSE;

	if (file == NULL || file->transacts == NULL)
		return FALSE;

	for (i = 0; i < file->transacts->trans_count && !found; i++)
		if (file->transacts->transactions[i].from == account || file->transacts->transactions[i].to == account)
			found = TRUE;

	return found;
}


/**
 * Calculate the details of a file, and fill the file info dialogue.
 *
 * \param *file			The file to display data for.
 */

static void transact_prepare_fileinfo(struct file_block *file)
{
	file_get_pathname(file, icons_get_indirected_text_addr(transact_fileinfo_window, FILEINFO_ICON_FILENAME), 255);

	if (file_check_for_filepath(file))
		territory_convert_standard_date_and_time(territory_CURRENT, (os_date_and_time const *) file->datestamp,
				icons_get_indirected_text_addr(transact_fileinfo_window, FILEINFO_ICON_DATE), 30);
	else
		icons_msgs_lookup(transact_fileinfo_window, FILEINFO_ICON_DATE, "UnSaved");

	if (file_get_data_integrity(file))
		icons_msgs_lookup(transact_fileinfo_window, FILEINFO_ICON_MODIFIED, "Yes");
	else
		icons_msgs_lookup(transact_fileinfo_window, FILEINFO_ICON_MODIFIED, "No");

	icons_printf(transact_fileinfo_window, FILEINFO_ICON_TRANSACT, "%d", transact_get_count(file));
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_SORDERS, "%d", sorder_get_count(file));
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_PRESETS, "%d", preset_get_count(file));
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_ACCOUNTS, "%d", account_count_type_in_file(file, ACCOUNT_FULL));
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_HEADINGS, "%d", account_count_type_in_file(file, ACCOUNT_IN | ACCOUNT_OUT));
}
