/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: accview.c
 *
 * Account statement view implementation
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "accview.h"

#include "account.h"
#include "conversion.h"
#include "caret.h"
#include "column.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "report.h"
#include "saveas.h"
#include "templates.h"
#include "transact.h"
#include "window.h"


/* Main Window Icons
 *
 * Note that these correspond to column numbers.
 */

#define ACCVIEW_ICON_ROW 0
#define ACCVIEW_ICON_DATE 1
#define ACCVIEW_ICON_IDENT 2
#define ACCVIEW_ICON_REC 3
#define ACCVIEW_ICON_FROMTO 4
#define ACCVIEW_ICON_REFERENCE 5
#define ACCVIEW_ICON_PAYMENTS 6
#define ACCVIEW_ICON_RECEIPTS 7
#define ACCVIEW_ICON_BALANCE 8
#define ACCVIEW_ICON_DESCRIPTION 9

/* Toolbar icons */

#define ACCVIEW_PANE_ROW 0
#define ACCVIEW_PANE_DATE 1
#define ACCVIEW_PANE_FROMTO 2
#define ACCVIEW_PANE_REFERENCE 3
#define ACCVIEW_PANE_PAYMENTS 4
#define ACCVIEW_PANE_RECEIPTS 5
#define ACCVIEW_PANE_BALANCE 6
#define ACCVIEW_PANE_DESCRIPTION 7

#define ACCVIEW_PANE_PARENT 9
#define ACCVIEW_PANE_PRINT 10
#define ACCVIEW_PANE_EDIT 11
#define ACCVIEW_PANE_GOTOEDIT 12
#define ACCVIEW_PANE_SORT 13

#define ACCVIEW_PANE_SORT_DIR_ICON 8

#define ACCVIEW_COLUMN_RECONCILE 3

#define ACCVIEW_PANE_COL_MAP "0;1;2,3,4;5;6;7;8;9"

#define ACCVIEW_SORT_OK 2
#define ACCVIEW_SORT_CANCEL 3
#define ACCVIEW_SORT_DATE 4
#define ACCVIEW_SORT_FROMTO 5
#define ACCVIEW_SORT_REFERENCE 6
#define ACCVIEW_SORT_PAYMENTS 7
#define ACCVIEW_SORT_RECEIPTS 8
#define ACCVIEW_SORT_BALANCE 9
#define ACCVIEW_SORT_DESCRIPTION 10
#define ACCVIEW_SORT_ASCENDING 11
#define ACCVIEW_SORT_DESCENDING 12

/* AccView menu */

#define ACCVIEW_MENU_FINDTRANS 0
#define ACCVIEW_MENU_GOTOTRANS 1
#define ACCVIEW_MENU_SORT 2
#define ACCVIEW_MENU_EDITACCT 3
#define ACCVIEW_MENU_EXPCSV 4
#define ACCVIEW_MENU_EXPTSV 5
#define ACCVIEW_MENU_PRINT 6

struct accview_redraw {
	int			transaction;					/**< Pointer to the transaction entry.					*/
	int			balance;					/**< Running balance at this point.					*/

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the redraw data, and are in effect a separate array that is used
	 * for handling entries in the account view window.
	 */

	int			sort_index;					/**< Point to another line, to allow the window to be sorted.		*/
};

struct accview_window {
	file_data		*file;						/**< The handle of the parent file.					*/
	acct_t			account;					/**< The account number of the parent account.				*/

	/* Account window handle and title details. */

	wimp_w			accview_window;					/**< Window handle of the account window.				*/
	char			window_title[256];
	wimp_w			accview_pane;					/**< Window handle of the account window toolbar pane.			*/

	/* Display column details. */

	int			column_width[ACCVIEW_COLUMNS];			/**< Array holding the column widths in the account window.		*/
	int			column_position[ACCVIEW_COLUMNS];		/**< Array holding the column X-offsets in the acct window.		*/

	/* Data parameters */

	int			display_lines;					/**< Count of the lines in the window.					*/
	struct accview_redraw	*line_data;					/**< Pointer to array of line data for the redraw.			*/

	int			sort_order;

	char			sort_sprite[12];				/**< Space for the sort icon's indirected data.				*/
};




/* Account View Sort Window. */

static wimp_w			accview_sort_window = NULL;			/**< The account listsort window handle.				*/
static file_data		*accview_sort_file = NULL;			/**< The file currently owning the account view sort window.		*/
static acct_t			accview_sort_account = NULL_ACCOUNT;		/**< The account currently owning the account view print window.	*/

/* Account View Print Window. */

static file_data		*accview_print_file = NULL;			/**< The file currently owning the account view print window.		*/
static acct_t			accview_print_account = NULL_ACCOUNT;		/**< The account currently owning the account view print window.	*/

/* Account View Window. */

static wimp_window		*accview_window_def = NULL;			/**< The definition for the Account View Window.			*/
static wimp_window		*accview_pane_def = NULL;			/**< The definition for the Account View Window pane.			*/
static wimp_menu		*accview_window_menu = NULL;			/**< The Account View Window menu handle.				*/
static int			accview_window_menu_line = -1;			/**< The line over which the Account View Window Menu was opened.	*/
static wimp_i			accview_substitute_sort_icon = ACCVIEW_PANE_DATE;/**< The icon currently obscured by the sort icon.			*/

/* SaveAs Dialogue Handles. */

static struct saveas_block	*accview_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*accview_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/


static void			accview_close_window_handler(wimp_close *close);
static void			accview_window_click_handler(wimp_pointer *pointer);
static void			accview_pane_click_handler(wimp_pointer *pointer);
static void			accview_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void			accview_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void			accview_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void			accview_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void			accview_window_scroll_handler(wimp_scroll *scroll);
static void			accview_window_redraw_handler(wimp_draw *redraw);
static void			accview_adjust_window_columns(void *data, wimp_i group, int width);
static void			accview_adjust_sort_icon(file_data *file, acct_t account);
static void			accview_adjust_sort_icon_data(file_data *file, acct_t account, wimp_icon *icon);
static void			accview_set_window_extent(file_data *file, acct_t account);
static void			accview_force_window_redraw(file_data *file, acct_t account, int from, int to);
static void			accview_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

static void			accview_open_sort_window(file_data *file, acct_t account, wimp_pointer *ptr);
static void			accview_sort_click_handler(wimp_pointer *pointer);
static osbool			accview_sort_keypress_handler(wimp_key *key);
static void			accview_refresh_sort_window(void);
static void			accview_fill_sort_window(int sort_option);
static osbool			accview_process_sort_window(void);

static void			accview_open_print_window(file_data *file, acct_t account, wimp_pointer *ptr, osbool restore);
static void			accview_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum, date_t from, date_t to);

static int			accview_build(file_data *file, acct_t account);
static int			accview_calculate(file_data *file, acct_t account);

static int			accview_get_line_from_transaction(file_data *file, acct_t account, int transaction);
static int			accview_get_line_from_transact_window(file_data *file, acct_t account);
static int			accview_get_y_offset_from_transact_window(file_data *file, acct_t account);
static void			accview_scroll_to_transact_window(file_data *file, acct_t account);
static void			accview_scroll_to_line(file_data *file, acct_t account, int line);

static osbool			accview_save_csv(char *filename, osbool selection, void *data);
static osbool			accview_save_tsv(char *filename, osbool selection, void *data);
static void			accview_export_delimited(file_data *file, acct_t account, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Initialise the account view system.
 *
 * \param *sprites		The application sprite area.
 */

void accview_initialise(osspriteop_area *sprites)
{
	accview_sort_window = templates_create_window("SortAccView");
	ihelp_add_window(accview_sort_window, "SortAccView", NULL);
	event_add_window_mouse_event(accview_sort_window, accview_sort_click_handler);
	event_add_window_key_event(accview_sort_window, accview_sort_keypress_handler);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_DATE, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_FROMTO, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_REFERENCE, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_PAYMENTS, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_RECEIPTS, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_BALANCE, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_DESCRIPTION, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_ASCENDING, TRUE);
	event_add_window_icon_radio(accview_sort_window, ACCVIEW_SORT_DESCENDING, TRUE);

	accview_window_def = templates_load_window("AccView");
	accview_window_def->icon_count = 0;


	accview_pane_def = templates_load_window("AccViewTB");
	accview_pane_def->sprite_area = sprites;

	accview_window_menu = templates_get_menu(TEMPLATES_MENU_ACCVIEW);

	accview_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", accview_save_csv);
	accview_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", accview_save_tsv);
}


/**
 * Create and open a Account View window for the given file and account.
 *
 * \param *file			The file to open a window for.
 * \param account		The account to open a window for.
 */

void accview_open_window(file_data *file, acct_t account)
{
	int			i, j, height;
	os_error		*error;
	wimp_window_state	parent;
	struct accview_window	*new;

	/* Create or re-open the window. */

	if (file->accounts[account].account_view != NULL) {
		windows_open(file->accounts[account].account_view->accview_window);
		return;
	}

	if (!(file->sort_valid))
		transact_sort_file_data(file);

	/* The block pointer is put into the new variable, as the file->accounts[account].account_view pointer may move
	 * as a result of the flex heap shifting for heap_alloc ().
	 */

	new = heap_alloc(sizeof(struct accview_window));
	file->accounts[account].account_view = new;

	if (new == NULL) {
		error_msgs_report_info("AccviewMemErr1");
		return;
	}

	new->file = file;
	new->account = account;

	#ifdef DEBUG
	debug_printf("\\BCreate Account View for %d", account);
	#endif

	accview_build(file, account);

	/* Set the main window extent and create it. */

	*(new->window_title) = '\0';
	accview_window_def->title_data.indirected_text.text =
			new->window_title;

	for (i=0; i<ACCVIEW_COLUMNS; i++) {
		new->column_width[i] = file->accview_column_width[i];
		new->column_position[i] = file->accview_column_position[i];
	}

	height = (new->display_lines > MIN_ACCVIEW_ENTRIES) ?
			new->display_lines : MIN_ACCVIEW_ENTRIES;

	new->sort_order = file->accview_sort_order;

	/* Find the position to open the window at. */

	parent.w = file->transaction_window.transaction_pane;
	wimp_get_window_state(&parent);

	set_initial_window_area(accview_window_def,
			new->column_position[ACCVIEW_COLUMNS-1] +
			new->column_width[ACCVIEW_COLUMNS-1],
			((ICON_HEIGHT+LINE_GUTTER) * height) + ACCVIEW_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	file->child_x_offset++;
	if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
		file->child_x_offset = 0;

	/* Set the scroll offset to show the contents of the transaction window */

	accview_window_def->yscroll = accview_get_y_offset_from_transact_window(file, account);

	error = xwimp_create_window(accview_window_def, &(new->accview_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	#ifdef DEBUG
	debug_printf("Created window: %d, %d, %d, %d...", accview_window_def->visible.x0,
			accview_window_def->visible.x1, accview_window_def->visible.y0,
			accview_window_def->visible.y1);
	#endif

	/* Create the toolbar pane. */

	windows_place_as_toolbar(accview_window_def, accview_pane_def, ACCVIEW_TOOLBAR_HEIGHT-4);

	for (i=0, j=0; j < ACCVIEW_COLUMNS; i++, j++) {
		accview_pane_def->icons[i].extent.x0 = new->column_position[j];

		j = column_get_rightmost_in_group (ACCVIEW_PANE_COL_MAP, i);

		accview_pane_def->icons[i].extent.x1 = new->column_position[j] +
				new->column_width[j] + COLUMN_HEADING_MARGIN;
	}

	accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) new->sort_sprite;
	accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			accview_pane_def->sprite_area;

	accview_adjust_sort_icon_data(file, account, &(accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON]));

	error = xwimp_create_window(accview_pane_def, &(new->accview_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	accview_build_window_title(file, account);

	/* Sort the window contents. */

	accview_sort(file, account);

	/* Open the window. */

	if (file->accounts[account].type == ACCOUNT_FULL) {
		ihelp_add_window(new->accview_window, "AccView", accview_decode_window_help);
		ihelp_add_window(new->accview_pane, "AccViewTB", NULL);
	} else {
		ihelp_add_window(new->accview_window, "HeadView", accview_decode_window_help);
		ihelp_add_window(new->accview_pane, "HeadViewTB", NULL);
	}

	windows_open(new->accview_window);
	windows_open_nested_as_toolbar(new->accview_pane,
			new->accview_window,
			ACCVIEW_TOOLBAR_HEIGHT-4);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(new->accview_window, new);
	event_add_window_menu(new->accview_window, accview_window_menu);
	event_add_window_close_event(new->accview_window, accview_close_window_handler);
	event_add_window_mouse_event(new->accview_window, accview_window_click_handler);
	event_add_window_scroll_event(new->accview_window, accview_window_scroll_handler);
	event_add_window_redraw_event(new->accview_window, accview_window_redraw_handler);
	event_add_window_menu_prepare(new->accview_window, accview_window_menu_prepare_handler);
	event_add_window_menu_selection(new->accview_window, accview_window_menu_selection_handler);
	event_add_window_menu_warning(new->accview_window, accview_window_menu_warning_handler);
	event_add_window_menu_close(new->accview_window, accview_window_menu_close_handler);

	event_add_window_user_data(new->accview_pane, new);
	event_add_window_menu(new->accview_pane, accview_window_menu);
	event_add_window_mouse_event(new->accview_pane, accview_pane_click_handler);
	event_add_window_menu_prepare(new->accview_pane, accview_window_menu_prepare_handler);
	event_add_window_menu_selection(new->accview_pane, accview_window_menu_selection_handler);
	event_add_window_menu_warning(new->accview_pane, accview_window_menu_warning_handler);
	event_add_window_menu_close(new->accview_pane, accview_window_menu_close_handler);
}


/**
 * Close and delete the Account View Window associated with the given
 * file block and account.
 *
 * \param *file			The file to use.
 * \param account		The account to close the window for.
 */

void accview_delete_window(file_data *file, acct_t account)
{
	#ifdef DEBUG
	debug_printf("\\RDeleting account view window");
	debug_printf("Account: %d", account);
	#endif

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL)
		return;

	if ((file->accounts[account].account_view)->accview_window != NULL) {
		ihelp_remove_window((file->accounts[account].account_view)->accview_window);
		event_delete_window((file->accounts[account].account_view)->accview_window);
		wimp_delete_window((file->accounts[account].account_view)->accview_window);
	}

	if (file->accounts[account].account_view->accview_pane != NULL) {
		ihelp_remove_window((file->accounts[account].account_view)->accview_pane);
		event_delete_window((file->accounts[account].account_view)->accview_pane);
		wimp_delete_window((file->accounts[account].account_view)->accview_pane);
	}

	flex_free((flex_ptr) &((file->accounts[account].account_view)->line_data));

	heap_free(file->accounts[account].account_view);
	file->accounts[account].account_view = NULL;
}


/**
 * Handle Close events on Account View windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void accview_close_window_handler(wimp_close *close)
{
	struct accview_window	*windat;

	#ifdef DEBUG
	debug_printf ("\\RClosing Account View window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat != NULL && windat->file != NULL && windat->account != NULL_ACCOUNT)
		accview_delete_window(windat->file, windat->account);
}


/**
 * Process mouse clicks in the Account View window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void accview_window_click_handler(wimp_pointer *pointer)
{
	file_data		*file;
	int			line, column, xpos, transaction, toggle_flag;
	int			trans_col_from[] = {0,1,2,2,2,8,9,9,9,10}, trans_col_to[] = {0,1,5,5,5,8,9,9,9,10}, *trans_col;
	wimp_window_state	window;
	struct accview_window	*windat;

	#ifdef DEBUG
	debug_printf("Accview window click: %d", pointer->buttons);
	#endif

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL)
		return;

	/* Find the window's account, and get the line clicked on. */

	file = windat->file;
	if (file == NULL || windat->account == NULL_ACCOUNT)
		return;

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - ACCVIEW_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
	if (line < 0 || line >= windat->display_lines)
		line = -1;

	/* If the line is a transaction, handle mouse clicks over it.  Menu clicks are ignored and dealt with in the
	 * else clause.
	 */

	if (line == -1)
		return;

	transaction = windat->line_data[windat->line_data[line].sort_index].transaction;

	xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;

	for (column = 0;
			column < ACCVIEW_COLUMNS && xpos > (windat->column_position[column] + windat->column_width[column]);
			column++);

	if (column != ACCVIEW_COLUMN_RECONCILE && (pointer->buttons == wimp_DOUBLE_SELECT || pointer->buttons == wimp_DOUBLE_ADJUST)) {
		/* Handle double-clicks, which will locate the transaction in the main window.  Clicks in the reconcile
		 * column are not used, as these are used to toggle the reconcile flag.
		 */

		trans_col = (file->transactions[transaction].from == windat->account) ? trans_col_to : trans_col_from;

		edit_place_new_line(file, locate_transaction_in_transact_window (file, transaction));
		icons_put_caret_at_end(file->transaction_window.transaction_window, trans_col[column]);
		edit_find_line_vertically(file);

		if (pointer->buttons == wimp_DOUBLE_ADJUST)
			windows_open(file->transaction_window.transaction_window);
	} else if (column == ACCVIEW_COLUMN_RECONCILE && pointer->buttons == wimp_SINGLE_ADJUST) {
		/* Handle adjust-clicks in the reconcile column, to toggle the status. */

		toggle_flag = (file->transactions[transaction].from == windat->account) ? TRANS_REC_FROM : TRANS_REC_TO;
		edit_toggle_transaction_reconcile_flag(file, transaction, toggle_flag);
	}
}


/**
 * Process mouse clicks in the Account View pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void accview_pane_click_handler(wimp_pointer *pointer)
{
	struct accview_window	*windat;
	file_data		*file;
	acct_t			account;
	wimp_window_state	window;
	wimp_icon_state		icon;
	int			ox;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL)
		return;

	/* Find the window's account. */

	file = windat->file;
	account = windat->account;
	if (file == NULL || account == NULL_ACCOUNT)
		return;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	if (pointer->i == ACCVIEW_PANE_SORT_DIR_ICON)
		pointer->i = accview_substitute_sort_icon;

	/* Decode the mouse click. */

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case ACCVIEW_PANE_PARENT:
			windows_open(file->transaction_window.transaction_window);
			break;

		case ACCVIEW_PANE_PRINT:
			accview_open_print_window(file, account, pointer, config_opt_read ("RememberValues"));
			break;

		case ACCVIEW_PANE_EDIT:
			account_open_edit_window(file, account, -1, pointer);
			break;

		case ACCVIEW_PANE_GOTOEDIT:
			accview_scroll_to_transact_window(file, account);
			break;

		case ACCVIEW_PANE_SORT:
			accview_open_sort_window(file, account, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case ACCVIEW_PANE_PRINT:
			accview_open_print_window(file, account, pointer, !config_opt_read ("RememberValues"));
			break;

		case ACCVIEW_PANE_SORT:
			accview_sort(file, account);
			break;
		}
	} else if ((pointer->buttons == wimp_CLICK_SELECT * 256 || pointer->buttons == wimp_CLICK_ADJUST * 256) &&
			pointer->i != wimp_ICON_WINDOW) {
		window.w = pointer->w;
		wimp_get_window_state(&window);

		ox = window.visible.x0 - window.xscroll;

		icon.w = pointer->w;
		icon.i = pointer->i;
		wimp_get_icon_state(&icon);

		if (pointer->pos.x < (ox + icon.icon.extent.x1 - COLUMN_DRAG_HOTSPOT)) {
			windat->sort_order = SORT_NONE;

			switch (pointer->i) {
			case ACCVIEW_PANE_DATE:
				windat->sort_order = SORT_DATE;
				break;

			case ACCVIEW_PANE_FROMTO:
				windat->sort_order = SORT_FROMTO;
				break;

			case ACCVIEW_PANE_REFERENCE:
				windat->sort_order = SORT_REFERENCE;
				break;

			case ACCVIEW_PANE_PAYMENTS:
				windat->sort_order = SORT_PAYMENTS;
				break;

			case ACCVIEW_PANE_RECEIPTS:
				windat->sort_order = SORT_RECEIPTS;
				break;

			case ACCVIEW_PANE_BALANCE:
				windat->sort_order = SORT_BALANCE;
				break;

			case ACCVIEW_PANE_DESCRIPTION:
				windat->sort_order = SORT_DESCRIPTION;
				break;
			}

			if (windat->sort_order != SORT_NONE) {
				if (pointer->buttons == wimp_CLICK_SELECT * 256)
					windat->sort_order |= SORT_ASCENDING;
				else
					windat->sort_order |= SORT_DESCENDING;
			}

			accview_adjust_sort_icon(file, account);
			windows_redraw(windat->accview_pane);
			accview_sort(file, account);

			file->accview_sort_order = windat->sort_order;
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT) {
		column_start_drag(pointer, windat, windat->accview_window,
				ACCVIEW_PANE_COL_MAP, config_str_read("LimAccViewCols"), accview_adjust_window_columns);
	}
}


/**
 * Process menu prepare events in the Account View window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void accview_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct accview_window	*windat;
	int			line;
	wimp_window_state	window;


	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (windat->file == NULL || windat->account == NULL_ACCOUNT)
		return;

	if (pointer != NULL) {
		accview_window_menu_line = -1;

		if (w == windat->accview_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - ACCVIEW_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

			if (line >= 0 && line < windat->display_lines)
				accview_window_menu_line = line;
		}

		saveas_initialise_dialogue(accview_saveas_csv, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(accview_saveas_tsv, "DefTSVFile", NULL, FALSE, FALSE, windat);

		switch (windat->file->accounts[windat->account].type) {
		case ACCOUNT_FULL:
			msgs_lookup("AccviewMenuTitleAcc", accview_window_menu->title_data.text, 12);
			msgs_lookup("AccviewMenuEditAcc", menus_get_indirected_text_addr(accview_window_menu, ACCVIEW_MENU_EDITACCT), 20);
			templates_set_menu_token("AccViewMenu");
			break;

		case ACCOUNT_IN:
		case ACCOUNT_OUT:
			msgs_lookup("AccviewMenuTitleHead", accview_window_menu->title_data.text, 12);
			msgs_lookup("AccviewMenuEditHead", menus_get_indirected_text_addr(accview_window_menu, ACCVIEW_MENU_EDITACCT), 20);
			templates_set_menu_token("HeadViewMenu");
			break;

		case ACCOUNT_NULL:
		default:
			break;
		}
	}

	menus_shade_entry(accview_window_menu, ACCVIEW_MENU_FINDTRANS, accview_window_menu_line == -1);
}


/**
 * Process menu selection events in the Account View window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void accview_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct accview_window	*windat;
	wimp_pointer		pointer;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	/* Find the window's account. */

	if (windat->file == NULL || windat->account == NULL_ACCOUNT)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case ACCVIEW_MENU_FINDTRANS:
		edit_place_new_line(windat->file, locate_transaction_in_transact_window(windat->file,
				windat->line_data[windat->line_data[accview_window_menu_line].sort_index].transaction));
		icons_put_caret_at_end(windat->file->transaction_window.transaction_window, EDIT_ICON_DATE);
		edit_find_line_vertically(windat->file);
		break;

	case ACCVIEW_MENU_GOTOTRANS:
		accview_scroll_to_transact_window(windat->file, windat->account);
		break;

	case ACCVIEW_MENU_SORT:
		accview_open_sort_window(windat->file, windat->account, &pointer);
		break;

	case ACCVIEW_MENU_EDITACCT:
		account_open_edit_window(windat->file, windat->account, -1, &pointer);
		break;

	case ACCVIEW_MENU_PRINT:
		accview_open_print_window(windat->file, windat->account, &pointer, config_opt_read("RememberValues"));
		break;
	}
}


/**
 * Process submenu warning events in the Account View window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void accview_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct accview_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	switch (warning->selection.items[0]) {
	case ACCVIEW_MENU_EXPCSV:
		saveas_prepare_dialogue(accview_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case ACCVIEW_MENU_EXPTSV:
		saveas_prepare_dialogue(accview_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process menu close events in the Account View window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void accview_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	accview_window_menu_line = -1;
	templates_set_menu_token(NULL);
}


/**
 * Process scroll events in the Account View window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void accview_window_scroll_handler(wimp_scroll *scroll)
{
	int		width, height, error;

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

	height = (scroll->visible.y1 - scroll->visible.y0) - ACCVIEW_TOOLBAR_HEIGHT;

	switch (scroll->ymin) {
	case wimp_SCROLL_LINE_UP:
		scroll->yscroll += (ICON_HEIGHT + LINE_GUTTER);
		if ((error = ((scroll->yscroll) % (ICON_HEIGHT+LINE_GUTTER))))
			scroll->yscroll -= (ICON_HEIGHT+LINE_GUTTER) + error;
		break;

	case wimp_SCROLL_LINE_DOWN:
		scroll->yscroll -= (ICON_HEIGHT + LINE_GUTTER);
		if ((error = ((scroll->yscroll - height) % (ICON_HEIGHT+LINE_GUTTER))))
			scroll->yscroll -= error;
		break;

	case wimp_SCROLL_PAGE_UP:
		scroll->yscroll += height;
		if ((error = ((scroll->yscroll) % (ICON_HEIGHT+LINE_GUTTER))))
			scroll->yscroll -= (ICON_HEIGHT+LINE_GUTTER) + error;
		break;

	case wimp_SCROLL_PAGE_DOWN:
		scroll->yscroll -= height;
		if ((error = ((scroll->yscroll - height) % (ICON_HEIGHT+LINE_GUTTER))))
			scroll->yscroll -= error;
		break;
	}

	/* Re-open the window.
	 *
	 * It is assumed that the wimp will deal with out-of-bounds offsets for us.
	 */

	wimp_open_window((wimp_open *) scroll);
}


/**
 * Process redraw events in the Account View window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void accview_window_redraw_handler(wimp_draw *redraw)
{
	struct accview_window	*windat;
	file_data		*file;
	acct_t			account;
	int			ox, oy, top, base, y, i, transaction, shade_budget_col, shade_overdrawn_col, icon_fg_col, icon_fg_balance_col;
	char			icon_buffer[DESCRIPT_FIELD_LEN], rec_char[REC_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more, shade_budget, shade_overdrawn;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL)
		return;

	file = windat->file;
	account = windat->account;
	if (file == NULL || account == NULL_ACCOUNT)
		return;

	shade_budget = (file->accounts[account].type & (ACCOUNT_IN | ACCOUNT_OUT)) && config_opt_read ("ShadeBudgeted") &&
			(file->budget.start != NULL_DATE || file->budget.finish != NULL_DATE);
	shade_budget_col = config_int_read ("ShadeBudgetedColour");

	shade_overdrawn = (file->accounts[account].type & ACCOUNT_FULL) && config_opt_read ("ShadeOverdrawn");
	shade_overdrawn_col = config_int_read ("ShadeOverdrawnColour");

	more = wimp_redraw_window (redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);

	/* Set the horizontal positions of the icons for the account lines. */

	for (i=0; i < ACCVIEW_COLUMNS; i++) {
		accview_window_def->icons[i].extent.x0 = windat->column_position[i];
		accview_window_def->icons[i].extent.x1 = windat->column_position[i] + windat->column_width[i];
		accview_window_def->icons[i].data.indirected_text.text = icon_buffer;
	}

	/* Perform the redraw. */

	while (more) {
		/* Calculate the rows to redraw. */

		top = (oy - redraw->clip.y1 - ACCVIEW_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
		if (top < 0)
			top = 0;

		base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2) +
			oy - redraw->clip.y0 - ACCVIEW_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);


		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			/* Plot out the background with a filled white rectangle. */

			wimp_set_colour (wimp_COLOUR_WHITE);
			os_plot (os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT);
			os_plot (os_PLOT_RECTANGLE + os_PLOT_TO,
					ox + windat->column_position[ACCVIEW_COLUMNS-1] + windat->column_width[ACCVIEW_COLUMNS-1],
					oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

			/* Find the transaction that applies to this line. */

			transaction = (y < windat->display_lines) ? (windat->line_data)[(windat->line_data)[y].sort_index].transaction : 0;

			/* work out the foreground colour for the line, based on whether the line is to be shaded or not. */

			if (shade_budget && (y < windat->display_lines) &&
					((file->budget.start == NULL_DATE || file->transactions[transaction].date < file->budget.start) ||
					(file->budget.finish == NULL_DATE || file->transactions[transaction].date > file->budget.finish))) {
				icon_fg_col = (shade_budget_col << wimp_ICON_FG_COLOUR_SHIFT);
				icon_fg_balance_col = (shade_budget_col << wimp_ICON_FG_COLOUR_SHIFT);
			} else if (shade_overdrawn && (y < windat->display_lines) &&
					((windat->line_data)[(windat->line_data)[y].sort_index].balance < - file->accounts[account].credit_limit)) {
				icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
				icon_fg_balance_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
			} else {
				icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
				icon_fg_balance_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
			}

			*icon_buffer = '\0';

			/* Row field */

			accview_window_def->icons[ACCVIEW_ICON_ROW].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_ROW].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_ROW].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_ROW].flags |= icon_fg_col;

			if (y < windat->display_lines)
				snprintf(icon_buffer, DESCRIPT_FIELD_LEN, "%d", transact_get_transaction_number(transaction));
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_ROW]));

			/* Date field */

			accview_window_def->icons[ACCVIEW_ICON_DATE].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_DATE].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_DATE].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_DATE].flags |= icon_fg_col;

			if (y < windat->display_lines)
				convert_date_to_string(file->transactions[transaction].date, icon_buffer);
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_DATE]));

			/* From / To field */

			accview_window_def->icons[ACCVIEW_ICON_IDENT].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_IDENT].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_IDENT].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_IDENT].flags |= icon_fg_col;

			accview_window_def->icons[ACCVIEW_ICON_REC].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_REC].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_REC].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_REC].flags |= icon_fg_col;

			accview_window_def->icons[ACCVIEW_ICON_FROMTO].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_FROMTO].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_FROMTO].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_FROMTO].flags |= icon_fg_col;

			if (y < windat->display_lines && file->transactions[transaction].from == account &&
					file->transactions[transaction].to != NULL_ACCOUNT) {
				accview_window_def->icons[ACCVIEW_ICON_IDENT].data.indirected_text.text =
						file->accounts[file->transactions[transaction].to].ident;
				accview_window_def->icons[ACCVIEW_ICON_REC].data.indirected_text.text = icon_buffer;
				accview_window_def->icons[ACCVIEW_ICON_FROMTO].data.indirected_text.text =
						file->accounts[file->transactions[transaction].to].name;

				if (file->transactions[transaction].flags & TRANS_REC_FROM)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else if (y < windat->display_lines && file->transactions[transaction].to == account &&
					file->transactions[transaction].from != NULL_ACCOUNT) {
				accview_window_def->icons[ACCVIEW_ICON_IDENT].data.indirected_text.text =
						file->accounts[file->transactions[transaction].from].ident;
				accview_window_def->icons[ACCVIEW_ICON_REC].data.indirected_text.text = icon_buffer;
				accview_window_def->icons[ACCVIEW_ICON_FROMTO].data.indirected_text.text =
						file->accounts[file->transactions[transaction].from].name;

				if (file->transactions[transaction].flags & TRANS_REC_TO)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else {
				accview_window_def->icons[ACCVIEW_ICON_IDENT].data.indirected_text.text = icon_buffer;
				accview_window_def->icons[ACCVIEW_ICON_REC].data.indirected_text.text = icon_buffer;
				accview_window_def->icons[ACCVIEW_ICON_FROMTO].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_IDENT]));
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_REC]));
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_FROMTO]));

			/* Reference field */

			accview_window_def->icons[ACCVIEW_ICON_REFERENCE].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_REFERENCE].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_REFERENCE].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_REFERENCE].flags |= icon_fg_col;

			if (y < windat->display_lines) {
				accview_window_def->icons[ACCVIEW_ICON_REFERENCE].data.indirected_text.text = file->transactions[transaction].reference;
			} else {
				accview_window_def->icons[ACCVIEW_ICON_REFERENCE].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_REFERENCE]));

			/* Payments field */

			accview_window_def->icons[ACCVIEW_ICON_PAYMENTS].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_PAYMENTS].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_PAYMENTS].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_PAYMENTS].flags |= icon_fg_col;

			if (y < windat->display_lines && file->transactions[transaction].from == account)
				convert_money_to_string(file->transactions[transaction].amount, icon_buffer);
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_PAYMENTS]));

			/* Receipts field */

			accview_window_def->icons[ACCVIEW_ICON_RECEIPTS].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_RECEIPTS].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_RECEIPTS].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_RECEIPTS].flags |= icon_fg_col;

			if (y < windat->display_lines && file->transactions[transaction].to == account)
				convert_money_to_string(file->transactions[transaction].amount, icon_buffer);
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_RECEIPTS]));

			/* Balance field */

			accview_window_def->icons[ACCVIEW_ICON_BALANCE].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_BALANCE].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_BALANCE].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_BALANCE].flags |= icon_fg_balance_col;

			if (y < windat->display_lines)
				convert_money_to_string((windat->line_data)[(windat->line_data)[y].sort_index].balance, icon_buffer);
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_BALANCE]));

			/* Comments field */

			accview_window_def->icons[ACCVIEW_ICON_DESCRIPTION].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
			accview_window_def->icons[ACCVIEW_ICON_DESCRIPTION].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					ACCVIEW_TOOLBAR_HEIGHT;

			accview_window_def->icons[ACCVIEW_ICON_DESCRIPTION].flags &= ~wimp_ICON_FG_COLOUR;
			accview_window_def->icons[ACCVIEW_ICON_DESCRIPTION].flags |= icon_fg_col;

			if (y < windat->display_lines) {
				accview_window_def->icons[ACCVIEW_ICON_DESCRIPTION].data.indirected_text.text = file->transactions[transaction].description;
			} else {
				accview_window_def->icons[ACCVIEW_ICON_DESCRIPTION].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}
			wimp_plot_icon(&(accview_window_def->icons[ACCVIEW_ICON_DESCRIPTION]));
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

static void accview_adjust_window_columns(void *data, wimp_i group, int width)
{
	struct accview_window	*windat = (struct accview_window *) data;
	file_data		*file;
	acct_t			account;
	int			i, j, new_extent;
	wimp_icon_state		icon;
	wimp_window_info	window;

	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;
	account = windat->account;

	update_dragged_columns(ACCVIEW_PANE_COL_MAP, config_str_read("LimAccViewCols"), group, width,
			(file->accounts[account].account_view)->column_width,
			(file->accounts[account].account_view)->column_position,
			ACCVIEW_COLUMNS);

	for (i=0; i<ACCVIEW_COLUMNS; i++) {
		file->accview_column_width[i] = (file->accounts[account].account_view)->column_width[i];
		file->accview_column_position[i] = (file->accounts[account].account_view)->column_position[i];
	}

	/* Re-adjust the icons in the pane. */

	for (i=0, j=0; j < ACCVIEW_COLUMNS; i++, j++) {
		icon.w = (file->accounts[account].account_view)->accview_pane;
		icon.i = i;
		wimp_get_icon_state(&icon);

		icon.icon.extent.x0 = (file->accounts[account].account_view)->column_position[j];

		j = column_get_rightmost_in_group(ACCVIEW_PANE_COL_MAP, i);

		icon.icon.extent.x1 = (file->accounts[account].account_view)->column_position[j] +
				(file->accounts[account].account_view)->column_width[j] + COLUMN_HEADING_MARGIN;

		wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
				icon.icon.extent.x1, icon.icon.extent.y1);

		new_extent = (file->accounts[account].account_view)->column_position[ACCVIEW_COLUMNS-1] +
				(file->accounts[account].account_view)->column_width[ACCVIEW_COLUMNS-1];
	}

	accview_adjust_sort_icon(file, account);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw((file->accounts[account].account_view)->accview_window);
	windows_redraw((file->accounts[account].account_view)->accview_pane);

	/* Set the horizontal extent of the window and pane. */

	window.w = (file->accounts[account].account_view)->accview_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = (file->accounts[account].account_view)->accview_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	set_file_data_integrity(file, TRUE);
}


/**
 * Adjust the sort icon in an Account View window, to reflect the current column
 * heading positions.
 *
 * \param *file			The file to update the window for.
 * \param account		The account to update the window for.
 */

static void accview_adjust_sort_icon(file_data *file, acct_t account)
{
	wimp_icon_state		icon;

	icon.w = (file->accounts[account].account_view)->accview_pane;
	icon.i = ACCVIEW_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	accview_adjust_sort_icon_data(file, account, &(icon.icon));

	wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current Account View sort settings.
 *
 * \param *file			The file to be updated.
 * \param account		The account to be updated.
 * \param *icon			The icon to be updated.
 */

static void accview_adjust_sort_icon_data(file_data *file, acct_t account, wimp_icon *icon)
{
	int		i = 1, width, anchor;

	if ((file->accounts[account].account_view)->sort_order & SORT_ASCENDING)
		strcpy((file->accounts[account].account_view)->sort_sprite, "sortarrd");
	else if ((file->accounts[account].account_view)->sort_order & SORT_DESCENDING)
		strcpy((file->accounts[account].account_view)->sort_sprite, "sortarru");

	switch ((file->accounts[account].account_view)->sort_order & SORT_MASK) {
	case SORT_DATE:
		i = ACCVIEW_ICON_DATE;
		accview_substitute_sort_icon = ACCVIEW_PANE_DATE;
		break;

	case SORT_FROMTO:
		i = ACCVIEW_ICON_FROMTO;
		accview_substitute_sort_icon = ACCVIEW_PANE_FROMTO;
		break;

	case SORT_REFERENCE:
		i = ACCVIEW_ICON_REFERENCE;
		accview_substitute_sort_icon = ACCVIEW_PANE_REFERENCE;
		break;

	case SORT_PAYMENTS:
		i = ACCVIEW_ICON_PAYMENTS;
		accview_substitute_sort_icon = ACCVIEW_PANE_PAYMENTS;
		break;

	case SORT_RECEIPTS:
		i = ACCVIEW_ICON_RECEIPTS;
		accview_substitute_sort_icon = ACCVIEW_PANE_RECEIPTS;
		break;

	case SORT_BALANCE:
		i = ACCVIEW_ICON_BALANCE;
		accview_substitute_sort_icon = ACCVIEW_PANE_BALANCE;
		break;

	case SORT_DESCRIPTION:
		i = ACCVIEW_ICON_DESCRIPTION;
		accview_substitute_sort_icon = ACCVIEW_PANE_DESCRIPTION;
		break;
	}

	width = icon->extent.x1 - icon->extent.x0;

	if (((file->accounts[account].account_view)->sort_order & SORT_MASK) == SORT_PAYMENTS ||
			((file->accounts[account].account_view)->sort_order & SORT_MASK) == SORT_RECEIPTS ||
			((file->accounts[account].account_view)->sort_order & SORT_MASK) == SORT_BALANCE) {
		anchor = (file->accounts[account].account_view)->column_position[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
		icon->extent.x1 = icon->extent.x0 + width;
	} else {
		anchor = (file->accounts[account].account_view)->column_position[i] +
				(file->accounts[account].account_view)->column_width[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
		icon->extent.x0 = icon->extent.x1 - width;
	}
}


/**
 * Set the extent of an account view window for the specified file.
 *
 * \param *file			The file to update.
 * \param account		The account whose window is to be updated.
 */

static void accview_set_window_extent(file_data *file, acct_t account)
{
	wimp_window_state	state;
	os_box			extent;
	int			new_height, visible_extent, new_extent, new_scroll;

	/* Set the extent. */

	if (account != NULL_ACCOUNT &&
			file->accounts[account].account_view != NULL && (file->accounts[account].account_view)->accview_window != NULL) {
		/* Get the number of rows to show in the window, and work out the window extent from this. */

		new_height = ((file->accounts[account].account_view)->display_lines > MIN_ACCVIEW_ENTRIES) ?
				(file->accounts[account].account_view)->display_lines : MIN_ACCVIEW_ENTRIES;

		new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_height) - ACCVIEW_TOOLBAR_HEIGHT;

		/* Get the current window details, and find the extent of the bottom of the visible area. */

		state.w = (file->accounts[account].account_view)->accview_window;
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
		extent.x1 = (file->accounts[account].account_view)->column_position[ACCVIEW_COLUMNS-1] +
				(file->accounts[account].account_view)->column_width[ACCVIEW_COLUMNS-1];

		extent.y0 = new_extent;
		extent.y1 = 0;

		wimp_set_extent((file->accounts[account].account_view)->accview_window, &extent);
	}
}


/**
 * Recreate the title of the specified Account View window connected to the
 * given file.
 *
 * \param *file			The file to rebuild the title for.
 * \param account		The account to rebilld the window title for.
 */

void accview_build_window_title(file_data *file, acct_t account)
{
	char	name[256];

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL)
		return;

	make_file_leafname(file, name, sizeof(name));

	msgs_param_lookup("AccviewTitle", (file->accounts[account].account_view)->window_title,
			sizeof((file->accounts[account].account_view)->window_title),
			file->accounts[account].name, name, NULL, NULL);

	wimp_force_redraw_title((file->accounts[account].account_view)->accview_window);
}


/**
 * Force a redraw of the Account View window, for the given range of
 * lines.
 *
 * \param *file			The file owning the window.
 * \param account		The account to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

static void accview_force_window_redraw(file_data *file, acct_t account, int from, int to)
{
	int			y0, y1;
	wimp_window_info	window;

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL ||
			(file->accounts[account].account_view)->accview_window == NULL)
		return;

	window.w = (file->accounts[account].account_view)->accview_window;
	wimp_get_window_info_header_only(&window);

	y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - ACCVIEW_TOOLBAR_HEIGHT;
	y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - ACCVIEW_TOOLBAR_HEIGHT;

	wimp_force_redraw((file->accounts[account].account_view)->accview_window,
			window.extent.x0, y0, window.extent.x1, y1);
}


/**
 * Turn a mouse position over an Account View window into an interactive
 * help token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void accview_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int			column, xpos;
	wimp_window_state	window;
	struct accview_window	*windat;

	*buffer = '\0';

	windat = event_get_window_user_data(w);

	if (windat == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	for (column = 0;
			column < TRANSACT_COLUMNS && xpos > (windat->column_position[column] + windat->column_width[column]);
			column++);

	sprintf(buffer, "Col%d", column);
}


/**
 * Open the Account List Sort dialogue for a given account list window.
 *
 * \param *file			The file to own the dialogue.
 * \param account		The account whose window is to be sorted.
 * \param *ptr			The current Wimp pointer position.
 */

static void accview_open_sort_window(file_data *file, acct_t account, wimp_pointer *ptr)
{
	if (windows_get_open(accview_sort_window))
		wimp_close_window(accview_sort_window);

	accview_fill_sort_window((file->accounts[account].account_view)->sort_order);

	accview_sort_file = file;
	accview_sort_account = account;

	windows_open_centred_at_pointer(accview_sort_window, ptr);
	place_dialogue_caret(accview_sort_window, wimp_ICON_WINDOW);
}


/**
 * Process mouse clicks in the Account List Sort dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void accview_sort_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ACCVIEW_SORT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(accview_sort_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			accview_refresh_sort_window();
		break;

	case ACCVIEW_SORT_OK:
		if (accview_process_sort_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(accview_sort_window);
		break;
	}
}


/**
 * Process keypresses in the Account List Sort window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool accview_sort_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (accview_process_sort_window())
			close_dialogue_with_caret(accview_sort_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(accview_sort_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Account View Sort window.
 */

static void accview_refresh_sort_window(void)
{
	accview_fill_sort_window((accview_sort_file->accounts[accview_sort_account].account_view)->sort_order);
}


/**
 * Update the contents of the Account View Sort window to reflect the current
 * settings of the given file.
 *
 * \param sort_option		The sort option currently in force.
 */

static void accview_fill_sort_window(int sort_option)
{
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_DATE, (sort_option & SORT_MASK) == SORT_DATE);
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_FROMTO, (sort_option & SORT_MASK) == SORT_FROMTO);
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_REFERENCE, (sort_option & SORT_MASK) == SORT_REFERENCE);
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_PAYMENTS, (sort_option & SORT_MASK) == SORT_PAYMENTS);
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_RECEIPTS, (sort_option & SORT_MASK) == SORT_RECEIPTS);
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_BALANCE, (sort_option & SORT_MASK) == SORT_BALANCE);
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_DESCRIPTION, (sort_option & SORT_MASK) == SORT_DESCRIPTION);

	icons_set_selected(accview_sort_window, ACCVIEW_SORT_ASCENDING, sort_option & SORT_ASCENDING);
	icons_set_selected(accview_sort_window, ACCVIEW_SORT_DESCENDING, sort_option & SORT_DESCENDING);
}


/**
 * Take the contents of an updated Account List window and process the data.
 *
 * \return			TRUE if successful; else FALSE.
 */

static osbool accview_process_sort_window(void)
{
	(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_NONE;

	if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_DATE))
		(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_DATE;
	else if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_FROMTO))
		(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_FROMTO;
	else if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_REFERENCE))
		(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_REFERENCE;
	else if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_PAYMENTS))
		(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_PAYMENTS;
	else if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_RECEIPTS))
		(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_RECEIPTS;
	else if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_BALANCE))
		(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_BALANCE;
	else if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_DESCRIPTION))
		(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order = SORT_DESCRIPTION;

	if ((accview_sort_file->accounts[accview_sort_account].account_view)->sort_order != SORT_NONE) {
		if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_ASCENDING))
			(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order |= SORT_ASCENDING;
		else if (icons_get_selected(accview_sort_window, ACCVIEW_SORT_DESCENDING))
			(accview_sort_file->accounts[accview_sort_account].account_view)->sort_order |= SORT_DESCENDING;
	}

	accview_adjust_sort_icon(accview_sort_file, accview_sort_account);
	windows_redraw((accview_sort_file->accounts[accview_sort_account].account_view)->accview_pane);
	accview_sort(accview_sort_file, accview_sort_account);

	accview_sort_file->accview_sort_order = (accview_sort_file->accounts[accview_sort_account].account_view)->sort_order;

	return TRUE;
}


/**
 * Force the closure of the Account List sort window if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void accview_force_windows_closed(file_data *file)
{
	if (accview_sort_file == file && windows_get_open(accview_sort_window))
		close_dialogue_with_caret(accview_sort_window);
}


/**
 * Open the Account List Print dialogue for a given account list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void accview_open_print_window(file_data *file, acct_t account, wimp_pointer *ptr, osbool restore)
{
	accview_print_file = file;
	accview_print_account = account;

	printing_open_advanced_window(file, ptr, restore, "PrintAccview", accview_print);
}


/**
 * Send the contents of the Account List Window to the printer, via the reporting
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

static void accview_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum, date_t from, date_t to)
{
	report_data			*report;
	int				i, transaction=0;
	char				line[4096], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];
	struct accview_window		*window;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);
	msgs_lookup("PrintTitleAccview", buffer, sizeof(buffer));
	report = report_open(accview_print_file, buffer, NULL);

	if (report == NULL) {
		error_msgs_report_error("PrintMemFail");
		return;
	}

	hourglass_on();

	window = accview_print_file->accounts[accview_print_account].account_view;

	/* Output the page title. */

	make_file_leafname(accview_print_file, numbuf1, sizeof(numbuf1));
	msgs_param_lookup("AccviewTitle", buffer, sizeof(buffer),
			accview_print_file->accounts[accview_print_account].name, numbuf1, NULL, NULL);
	sprintf(line, "\\b\\u%s", buffer);
	report_write_line(report, 1, line);
	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	*line = '\0';
	sprintf(buffer, "\\k\\b\\u\\r%s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_ROW, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_DATE, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_FROMTO, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_REFERENCE, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u\\r%s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_PAYMENTS, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u\\r%s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_RECEIPTS, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u\\r%s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_BALANCE, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->accview_pane, ACCVIEW_PANE_DESCRIPTION, numbuf1));
	strcat(line, buffer);

	report_write_line(report, 0, line);

	/* Output the transaction data as a set of delimited lines. */

	for (i=0; i < window->display_lines; i++) {
		transaction = (window->line_data)[(window->line_data)[i].sort_index].transaction;

		if ((from == NULL_DATE || accview_print_file->transactions[transaction].date >= from) &&
				(to == NULL_DATE || accview_print_file->transactions[transaction].date <= to)) {
			*line = '\0';

			convert_date_to_string(accview_print_file->transactions[transaction].date, numbuf1);
			sprintf(buffer, "\\k\\r%d\\t%s\\t", transact_get_transaction_number(transaction), numbuf1);
			strcat(line, buffer);

			if (accview_print_file->transactions[transaction].from == accview_print_account) {
				sprintf(buffer, "%s\\t", account_get_ident(accview_print_file, accview_print_file->transactions[transaction].to));
				strcat(line, buffer);

				strcpy(numbuf1, (accview_print_file->transactions[transaction].flags & TRANS_REC_FROM) ? rec_char : "");
				sprintf(buffer, "%s\\t", numbuf1);
				strcat(line, buffer);

				sprintf(buffer, "%s\\t", account_get_name(accview_print_file, accview_print_file->transactions[transaction].to));
				strcat(line, buffer);
			} else {
				sprintf(buffer, "%s\\t", account_get_ident(accview_print_file, accview_print_file->transactions[transaction].from));
				strcat(line, buffer);

				strcpy(numbuf1, (accview_print_file->transactions[transaction].flags & TRANS_REC_TO) ? rec_char : "");
				sprintf(buffer, "%s\\t", numbuf1);
				strcat(line, buffer);

				sprintf(buffer, "%s\\t", account_get_name(accview_print_file, accview_print_file->transactions[transaction].from));
				strcat(line, buffer);
			}

			sprintf(buffer, "%s\\t", accview_print_file->transactions[transaction].reference);
			strcat(line, buffer);

			if (accview_print_file->transactions[transaction].from == accview_print_account) {
				convert_money_to_string(accview_print_file->transactions[transaction].amount, numbuf1);
				sprintf(buffer, "\\r%s\\t\\r\\t", numbuf1);
			} else {
				convert_money_to_string(accview_print_file->transactions[transaction].amount, numbuf1);
				sprintf(buffer, "\\r\\t\\r%s\\t", numbuf1);
			}
			strcat(line, buffer);

			convert_money_to_string(window->line_data[i].balance, numbuf1);
			sprintf(buffer, "\\r%s\\t", numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", accview_print_file->transactions[transaction].description);
			strcat(line, buffer);

			report_write_line(report, 0, line);
		}
	}

	hourglass_off();

	report_close_and_print(report, text, format, scale, rotate, pagenum);
}


/**
 * Sort the account view list in a given file based on that file's sort setting.
 *
 * \param *file			The file to sort.
 * \param account		The account to sort.
 */

void accview_sort(file_data *file, int account)
{
	int			gap, comb, temp, order;
	osbool			sorted, reorder;
	struct accview_window	*window;

	#ifdef DEBUG
	debug_printf("Sorting accview window");
	#endif

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL)
		return;

	hourglass_on();

	/* Find the address of the accview block now, to save indirecting to it every time. */

	window = file->accounts[account].account_view;

	/* Sort the entries using a combsort.  This has the advantage over qsort () that the order of entries is only
	 * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
	 */

	gap = window->display_lines - 1;
	order = window->sort_order;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((window->display_lines >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < window->display_lines; comb++) {
			switch (order) {
			case SORT_DATE | SORT_ASCENDING:
				reorder = (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].date <
						file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].date);
				break;

			case SORT_DATE | SORT_DESCENDING:
				reorder = (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].date >
						file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].date);
				break;

			case SORT_FROMTO | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(file, (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].to) :
						(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from)),
						account_get_name(file, (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].to) :
						(file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from))) < 0);
				break;

			case SORT_FROMTO | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(file, (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].to) :
						(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from)),
						account_get_name(file, (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].to) :
						(file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from))) > 0);
				break;

			case SORT_REFERENCE | SORT_ASCENDING:
				reorder = (strcmp(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].reference,
						file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].reference) < 0);
				break;

			case SORT_REFERENCE | SORT_DESCENDING:
				reorder = (strcmp(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].reference,
						file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].reference) > 0);
				break;

			case SORT_PAYMENTS | SORT_ASCENDING:
				reorder = (((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount) : 0) <
						((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount) : 0));
				break;

			case SORT_PAYMENTS | SORT_DESCENDING:
				reorder = (((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount) : 0) >
						((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
						(file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount) : 0));
				break;

			case SORT_RECEIPTS | SORT_ASCENDING:
				reorder = (((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
						0 : (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount)) <
						((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
						0 : (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount)));
				break;

			case SORT_RECEIPTS | SORT_DESCENDING:
				reorder = (((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
						0 : (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount)) >
						((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
						0 : (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount)));
				break;

			case SORT_BALANCE | SORT_ASCENDING:
				reorder = (window->line_data[window->line_data[comb+gap].sort_index].balance <
						window->line_data[window->line_data[comb].sort_index].balance);
				break;

			case SORT_BALANCE | SORT_DESCENDING:
				reorder = (window->line_data[window->line_data[comb+gap].sort_index].balance >
						window->line_data[window->line_data[comb].sort_index].balance);
				break;

			case SORT_DESCRIPTION | SORT_ASCENDING:
				reorder = (strcmp(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].description,
						file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].description) < 0);
				break;

			case SORT_DESCRIPTION | SORT_DESCENDING:
				reorder = (strcmp(file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].description,
						file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].description) > 0);
				break;

			default:
				reorder = FALSE;
				break;
			}

			if (reorder) {
				temp = window->line_data[comb+gap].sort_index;
				window->line_data[comb+gap].sort_index = window->line_data[comb].sort_index;
				window->line_data[comb].sort_index = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

	accview_force_window_redraw(file, account, 0, window->display_lines - 1);

	hourglass_off();
}


/**
 * Build a redraw list for an account statement view window from scratch.
 * Allocate a flex block big enough to take all the transactions in the file,
 * fill it as required, then shrink it down again to the correct size.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be built.
 */

static int accview_build(file_data *file, acct_t account)
{
	int	i, lines = 0;

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL)
		return lines;


	#ifdef DEBUG
	debug_printf("\\BBuilding account statement view");
	#endif

	if (flex_alloc((flex_ptr) &((file->accounts[account].account_view)->line_data), file->trans_count * sizeof(struct accview_redraw)) == 0) {
		error_msgs_report_info("AccviewMemErr2");
		return lines;
	}

	lines = accview_calculate(file, account);

	flex_extend((flex_ptr) &((file->accounts[account].account_view)->line_data), lines * sizeof(struct accview_redraw));

	for (i = 0; i < lines; i++)
		(file->accounts[account].account_view)->line_data[i].sort_index = i;

	return lines;
}


/**
 * Rebuild a pre-existing account view from scratch, possibly becuase one of
 * the account's From/To entries has been changed, so all bets are off...
 * Delete the flex block and rebuild it, then resize the window and refresh the
 * whole thing.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be refreshed.
 */

void accview_rebuild(file_data *file, acct_t account)
{
	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\BRebuilding account statement view");
	#endif

	if (!(file->sort_valid))
		transact_sort_file_data(file);

	if ((file->accounts[account].account_view)->line_data != NULL)
		flex_free((flex_ptr) &((file->accounts[account].account_view)->line_data));

	accview_build(file, account);
	accview_set_window_extent(file, account);
	accview_sort(file, account);
	windows_redraw((file->accounts[account].account_view)->accview_window);
}


/* Calculate the contents of an account view redraw block: entering transaction references and calculating a running
 * balance for the display.
 *
 * This relies on there being enough space in the block to take a line for every transaction.  If it is called for
 * an existing view, it relies on the number of lines not having changed!
 */

static int accview_calculate(file_data *file, acct_t account)
{
	int	lines = 0, i, balance;

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL)
		return lines;

	hourglass_on();

	balance = file->accounts[account].opening_balance;

	for (i=0; i<file->trans_count; i++) {
		if (file->transactions[i].from == account || file->transactions[i].to == account) {
			((file->accounts[account].account_view)->line_data)[lines].transaction = i;

			if (file->transactions[i].from == account)
				balance -= file->transactions[i].amount;
			else
				balance += file->transactions[i].amount;

			((file->accounts[account].account_view)->line_data)[lines].balance = balance;

			lines++;
		}
	}

	(file->accounts[account].account_view)->display_lines = lines;

	hourglass_off();

	return lines;
}


/**
 * Recalculate the account view.  An amount entry or date has been changed, so
 * the number of transactions will remain the same.  Just re-fill the existing
 * flex block, then resize the window and refresh the whole thing.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be recalculated.
 * \param transaction		The transaction which has been changed.
 */

void accview_recalculate(file_data *file, acct_t account, int transaction)
{
	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\BRecalculating account statement view");
	#endif

	if (!(file->sort_valid))
		transact_sort_file_data(file);

	accview_calculate(file, account);
	accview_force_window_redraw(file, account, accview_get_line_from_transaction(file, account, transaction),
			(file->accounts[account].account_view)->display_lines-1);
}


/**
 * Redraw the line in an account view corresponding to the given transaction.
 * If the transaction does not feature in the account, nothing is done.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be redrawn.
 * \param transaction		The transaction to be redrawn.
 */

void accview_redraw_transaction(file_data *file, acct_t account, int transaction)
{
	int	line;

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	line = accview_get_line_from_transaction(file, account, transaction);

	if (line != -1)
		accview_force_window_redraw(file, account, line, line);
}


/**
 * Re-index the account views in a file.  This can *only* be done after
 * transact_sort_file_data() has been called, as it requires data set up
 * in the transaction block by that call.
 *
 * \param *file			The file to reindex.
 */

void accview_reindex_all(file_data *file)
{
	acct_t		i;
	int		j, t;

	if (file == NULL)
		return;

	#ifdef DEBUG
	debug_printf("Reindexing account views...");
	#endif

	for (i=0; i<file->account_count; i++) {
		if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->line_data != NULL) {
			for (j=0; j<(file->accounts[i].account_view)->display_lines; j++) {
				t = ((file->accounts[i].account_view)->line_data)[j].transaction;
				((file->accounts[i].account_view)->line_data)[j].transaction = file->transactions[t].sort_workspace;
			}
		}
	}
}


/**
 * Fully redraw all of the open account views in a file.
 *
 * \param *file			The file to be redrawn.
 */

void accview_redraw_all(file_data *file)
{
	acct_t		i;

	if (file == NULL)
		return;

	for (i=0; i<file->account_count; i++)
		if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->accview_window != NULL)
			windows_redraw((file->accounts[i].account_view)->accview_window);
}


/**
 * Fully recalculate all of the open account views in a file.
 *
 * \param *file			The file to be recalculated.
 */

void accview_recalculate_all(file_data *file)
{
	acct_t		i;

	if (file == NULL)
		return;

	for (i=0; i<file->account_count; i++)
		if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->accview_window != NULL)
			accview_recalculate(file, i, 0);
}


/**
 * Fully rebuild all of the open account views in a file.
 *
 * \param *file			The file to be rebuilt.
 */

void accview_rebuild_all(file_data *file)
{
	acct_t		i;

	if (file == NULL)
		return;

	for (i=0; i<file->account_count; i++)
		if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->accview_window != NULL)
			accview_rebuild(file, i);
}


/**
 * Convert a transaction number into a line in a given account view.
 *
 * \param *file			The file to work on.
 * \param account		The account to use the view for.
 * \param transaction		The transaction to locate.
 * \return			The corresponding account view line, or -1.
 */

static int accview_get_line_from_transaction(file_data *file, acct_t account, int transaction)
{
	int	line = -1, index = -1, i;

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL ||
			(file->accounts[account].account_view)->line_data == NULL)
		return index;

	for (i = 0; i < (file->accounts[account].account_view)->display_lines && line == -1; i++)
		if (((file->accounts[account].account_view)->line_data)[i].transaction == transaction)
			line = i;

	if (line != -1)
		for (i = 0; i < (file->accounts[account].account_view)->display_lines && index == -1; i++)
			if (((file->accounts[account].account_view)->line_data)[i].sort_index == line)
				index = i;

	return index;
}


/**
 * Return the line in an account view which is most closely associated
 * with the transaction at the centre of the transaction window for the
 * parent file.
 *
 * \param *file			The file to use.
 * \param account		The account to use.
 * \return			The line in the account view, or 0.
 */

static int accview_get_line_from_transact_window(file_data *file, acct_t account)
{
	int			centre_transact, line = 0;

	if (file == NULL || account == NULL_ACCOUNT ||
			file->accounts[account].account_view == NULL)
		return line;

	centre_transact = find_transaction_window_centre(file, account);
	line = accview_get_line_from_transaction(file, account, centre_transact);

	if (line == -1)
		line = 0;

	return line;
}


/**
 * Get a Y offset in OS units for an account view window based on the transaction
 * which is at the centre of the transaction window.
 *
 * \param *file			The file to use.
 * \param account		The account view to use.
 * \return			The Y offset in OS units, or 0.
 */

static int accview_get_y_offset_from_transact_window(file_data *file, acct_t account)
{
	return -accview_get_line_from_transact_window(file, account) * (ICON_HEIGHT + LINE_GUTTER);
}


/**
 * Scroll an account view window so that it displays lines close to the current
 * transaction window scroll offset.
 *
 * \param *file			The file to work on.
 * \param account		The account to work on.
 */

static void accview_scroll_to_transact_window(file_data *file, acct_t account)
{
	accview_scroll_to_line(file, account, accview_get_line_from_transact_window(file, account));
}


/**
 * Scroll an account view window so that the specified line appears within
 * the visible area.
 *
 * \param *file			The file to work on.
 * \param account		The account to work on.
 * \param line			The line to scroll in to view.
 */

static void accview_scroll_to_line(file_data *file, acct_t account, int line)
{
	wimp_window_state	window;
	int	height, top, bottom;

	if (file == NULL || account == NULL_ACCOUNT || file->accounts[account].account_view == NULL ||
			(file->accounts[account].account_view)->accview_window == NULL)
		return;

	window.w = (file->accounts[account].account_view)->accview_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
	 * This will allow the edit line to be aligned with the top or bottom of the window.
	 */

	height = window.visible.y1 - window.visible.y0 - ICON_HEIGHT - LINE_GUTTER - ACCVIEW_TOOLBAR_HEIGHT;

	/* Calculate the top full line and bottom full line that are showing in the window.  Part lines don't
	 * count and are discarded.
	 */

	top = (-window.yscroll + ICON_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
	bottom = height / (ICON_HEIGHT+LINE_GUTTER) + top;

	/* If the edit line is above or below the visible area, bring it into range. */

	if (line < top) {
		window.yscroll = -(line * (ICON_HEIGHT+LINE_GUTTER));
		wimp_open_window((wimp_open *) &window);
	}

	if (line > bottom) {
		window.yscroll = -(line * (ICON_HEIGHT+LINE_GUTTER) - height);
		wimp_open_window((wimp_open *) &window);
	}
}


/**
 * Callback handler for saving a CSV version of the account view transaction data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool accview_save_csv(char *filename, osbool selection, void *data)
{
	struct accview_window *windat = data;
	
	if (windat == NULL || windat->file == NULL)
		return FALSE;

	accview_export_delimited(windat->file, windat->account, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
	
	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the account view transaction data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool accview_save_tsv(char *filename, osbool selection, void *data)
{
	struct accview_window *windat = data;
	
	if (windat == NULL || windat->file == NULL)
		return FALSE;
		
	accview_export_delimited(windat->file, windat->account, filename, DELIMIT_TAB, TSV_FILE_TYPE);
	
	return TRUE;
}


/**
 * Export the account view transaction data from a file into CSV or TSV format.
 *
 * \param *file			The file to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void accview_export_delimited(file_data *file, acct_t account, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE				*out;
	int				i, transaction=0;
	char				buffer[256];
	struct accview_window		*windat;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL) {
		windat = file->accounts[account].account_view;

		/* Output the headings line, taking the text from the window icons. */

		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_ROW, buffer);
		filing_output_delimited_field(out, buffer, format, 0);
		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_DATE, buffer);
		filing_output_delimited_field(out, buffer, format, 0);
		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_FROMTO, buffer);
		filing_output_delimited_field(out, buffer, format, 0);
		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_REFERENCE, buffer);
		filing_output_delimited_field(out, buffer, format, 0);
		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_PAYMENTS, buffer);
		filing_output_delimited_field(out, buffer, format, 0);
		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_RECEIPTS, buffer);
		filing_output_delimited_field(out, buffer, format, 0);
		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_BALANCE, buffer);
		filing_output_delimited_field(out, buffer, format, 0);
		icons_copy_text(windat->accview_pane, ACCVIEW_PANE_DESCRIPTION, buffer);
		filing_output_delimited_field(out, buffer, format, DELIMIT_LAST);

		/* Output the transaction data as a set of delimited lines. */
		for (i=0; i < windat->display_lines; i++) {
			transaction = (windat->line_data)[(windat->line_data)[i].sort_index].transaction;

			snprintf(buffer, 256, "%d", transact_get_transaction_number(transaction));
			filing_output_delimited_field(out, buffer, format, 0);

			convert_date_to_string(file->transactions[transaction].date, buffer);
			filing_output_delimited_field(out, buffer, format, 0);

			if (file->transactions[transaction].from == account)
				account_build_name_pair(file, file->transactions[transaction].to, buffer, sizeof(buffer));
			else
				account_build_name_pair(file, file->transactions[transaction].from, buffer, sizeof(buffer));
			filing_output_delimited_field(out, buffer, format, 0);

			filing_output_delimited_field(out, file->transactions[transaction].reference, format, 0);

			if (file->transactions[transaction].from == account) {
				convert_money_to_string(file->transactions[transaction].amount, buffer);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				filing_output_delimited_field(out, "", format, DELIMIT_NUM);
			} else {
				convert_money_to_string(file->transactions[transaction].amount, buffer);
				filing_output_delimited_field(out, "", format, DELIMIT_NUM);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
			}

			convert_money_to_string(windat->line_data[i].balance, buffer);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			filing_output_delimited_field(out, file->transactions[transaction].description, format, DELIMIT_LAST);
		}
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}

