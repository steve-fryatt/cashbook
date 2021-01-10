/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
#include "accview.h"

#include "account.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "flexutils.h"
#include "print_dialogue.h"
#include "report.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
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

#define ACCVIEW_PANE_SORT_DIR_ICON 8

#define ACCVIEW_PANE_PARENT 9
#define ACCVIEW_PANE_PRINT 10
#define ACCVIEW_PANE_EDIT 11
#define ACCVIEW_PANE_GOTOEDIT 12
#define ACCVIEW_PANE_SORT 13

#define ACCVIEW_COLUMN_RECONCILE 3

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
#define ACCVIEW_SORT_ROW 13

/* AccView menu */

#define ACCVIEW_MENU_FINDTRANS 0
#define ACCVIEW_MENU_GOTOTRANS 1
#define ACCVIEW_MENU_SORT 2
#define ACCVIEW_MENU_EDITACCT 3
#define ACCVIEW_MENU_EXPCSV 4
#define ACCVIEW_MENU_EXPTSV 5
#define ACCVIEW_MENU_PRINT 6

/* Account View window details */

#define ACCVIEW_COLUMNS 10
#define ACCVIEW_TOOLBAR_HEIGHT 132
#define MIN_ACCVIEW_ENTRIES 10

/* Account View window column mapping. */

static struct column_map accview_columns[ACCVIEW_COLUMNS] = {
	{ACCVIEW_ICON_ROW, ACCVIEW_PANE_ROW, wimp_ICON_WINDOW, SORT_ROW},
	{ACCVIEW_ICON_DATE, ACCVIEW_PANE_DATE, wimp_ICON_WINDOW, SORT_DATE},
	{ACCVIEW_ICON_IDENT, ACCVIEW_PANE_FROMTO, wimp_ICON_WINDOW, SORT_FROMTO},
	{ACCVIEW_ICON_REC, ACCVIEW_PANE_FROMTO, wimp_ICON_WINDOW, SORT_FROMTO},
	{ACCVIEW_ICON_FROMTO, ACCVIEW_PANE_FROMTO, wimp_ICON_WINDOW, SORT_FROMTO},
	{ACCVIEW_ICON_REFERENCE, ACCVIEW_PANE_REFERENCE, wimp_ICON_WINDOW, SORT_REFERENCE},
	{ACCVIEW_ICON_PAYMENTS, ACCVIEW_PANE_PAYMENTS, wimp_ICON_WINDOW, SORT_PAYMENTS},
	{ACCVIEW_ICON_RECEIPTS, ACCVIEW_PANE_RECEIPTS, wimp_ICON_WINDOW, SORT_RECEIPTS},
	{ACCVIEW_ICON_BALANCE, ACCVIEW_PANE_BALANCE, wimp_ICON_WINDOW, SORT_BALANCE},
	{ACCVIEW_ICON_DESCRIPTION, ACCVIEW_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION}
};

struct accview_block {
	struct file_block	*file;						/**< The file to which the instance belongs.				*/

	/* Default display details for the accview windows. */

	struct column_block	*columns;					/**< Base column data for the account view windows.			*/

	/* Default sort details for account view windows. */

	struct sort_block	*sort;
};

/**
 * The direction of an accview account entry.
 */

enum accview_direction {
	ACCVIEW_DIRECTION_NONE = 0,						/**< The entry isn't used, or isn't determined.				*/
	ACCVIEW_DIRECTION_FROM,							/**< The entry is a transfer from the viewed account.			*/
	ACCVIEW_DIRECTION_TO							/**< The entry is a transfer to the viewed account.			*/
};

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
	struct file_block	*file;						/**< The handle of the parent file.					*/
	acct_t			account;					/**< The account number of the parent account.				*/

	/* Account window handle and title details. */

	wimp_w			accview_window;					/**< Window handle of the account window.				*/
	char			window_title[WINDOW_TITLE_LENGTH];
	wimp_w			accview_pane;					/**< Window handle of the account window toolbar pane.			*/

	/* Display column details. */

	struct column_block	*columns;					/**< Instance handle of the column definitions.				*/

	/* Window sorting information. */

	struct sort_block	*sort;						/**< Instance handle for the sort code.					*/

	char			sort_sprite[COLUMN_SORT_SPRITE_LEN];		/**< Space for the sort icon's indirected data.				*/

	/* Data parameters */

	int			display_lines;					/**< Count of the lines in the window.					*/
	struct accview_redraw	*line_data;					/**< Pointer to array of line data for the redraw.			*/
};




/* Account View Sort Window. */

static struct sort_dialogue_block	*accview_sort_dialogue = NULL;		/**< The preset window sort dialogue box.			*/

static struct sort_dialogue_icon accview_sort_columns[] = {				/**< Details of the sort dialogue column icons.				*/
	{ACCVIEW_SORT_ROW, SORT_ROW},
	{ACCVIEW_SORT_DATE, SORT_DATE},
	{ACCVIEW_SORT_FROMTO, SORT_FROMTO},
	{ACCVIEW_SORT_REFERENCE, SORT_REFERENCE},
	{ACCVIEW_SORT_PAYMENTS, SORT_PAYMENTS},
	{ACCVIEW_SORT_RECEIPTS, SORT_RECEIPTS},
	{ACCVIEW_SORT_BALANCE, SORT_BALANCE},
	{ACCVIEW_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{0, SORT_NONE}
};

static struct sort_dialogue_icon accview_sort_directions[] = {				/**< Details of the sort dialogue direction icons.			*/
	{ACCVIEW_SORT_ASCENDING, SORT_ASCENDING},
	{ACCVIEW_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/* Account View sorting. */

static struct sort_callback	accview_sort_callbacks;

/* Account View Window. */

static wimp_window		*accview_window_def = NULL;			/**< The definition for the Account View Window.			*/
static wimp_window		*accview_pane_def = NULL;			/**< The definition for the Account View Window pane.			*/
static wimp_menu		*accview_window_menu = NULL;			/**< The Account View Window menu handle.				*/
static int			accview_window_menu_line = -1;			/**< The line over which the Account View Window Menu was opened.	*/

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
static void			accview_adjust_sort_icon(struct accview_window *view);
static void			accview_adjust_sort_icon_data(struct accview_window *view, wimp_icon *icon);
static void			accview_set_window_extent(struct accview_window *view);
static void			accview_force_window_redraw(struct accview_window *view, int from, int to, wimp_i column);
static void			accview_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

static void			accview_open_sort_window(struct accview_window *view, wimp_pointer *ptr);
static osbool			accview_process_sort_window(enum sort_type order, void *data);

static void			accview_open_print_window(struct accview_window *view, wimp_pointer *ptr, osbool restore);
static struct report		*accview_print(struct report *report, void *data, date_t from, date_t to);

static int			accview_sort_compare(enum sort_type type, int index1, int index2, void *data);
static void			accview_sort_swap(int index1, int index2, void *data);

static osbool			accview_build(struct accview_window *view);
static int			accview_calculate(struct accview_window *view);

static enum accview_direction	accview_get_transaction_direction(struct accview_window *view, int transaction);
static int			accview_get_line_from_transaction(struct accview_window *view, int transaction);
static int			accview_get_line_from_transact_window(struct accview_window *view);
static int			accview_get_y_offset_from_transact_window(struct accview_window *view);
static void			accview_scroll_to_transact_window(struct accview_window *view);
static void			accview_scroll_to_line(struct accview_window *view, int line);

static osbool			accview_save_csv(char *filename, osbool selection, void *data);
static osbool			accview_save_tsv(char *filename, osbool selection, void *data);
static void			accview_export_delimited(struct accview_window *view, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Initialise the account view system.
 *
 * \param *sprites		The application sprite area.
 */

void accview_initialise(osspriteop_area *sprites)
{
	wimp_w	sort_window;

	sort_window = templates_create_window("SortAccView");
	ihelp_add_window(sort_window, "SortAccView", NULL);
	accview_sort_dialogue = sort_dialogue_create(sort_window, accview_sort_columns, accview_sort_directions,
			ACCVIEW_SORT_OK, ACCVIEW_SORT_CANCEL, accview_process_sort_window);

	accview_window_def = templates_load_window("AccView");
	accview_window_def->icon_count = 0;


	accview_pane_def = templates_load_window("AccViewTB");
	accview_pane_def->sprite_area = sprites;

	accview_window_menu = templates_get_menu("AccountViewMenu");

	accview_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", accview_save_csv);
	accview_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", accview_save_tsv);

	accview_sort_callbacks.compare = accview_sort_compare;
	accview_sort_callbacks.swap = accview_sort_swap;
}


/**
 * Create a new global account view module instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct accview_block *accview_create_instance(struct file_block *file)
{
	struct accview_block	*new;

	new = heap_alloc(sizeof(struct accview_block));
	if (new == NULL)
		return NULL;

	/* Initialise the transaction window. */

	new->file = file;
	new->columns = NULL;
	new->sort = NULL;

	/* Initialise the global window columns defaults. */

	new->columns = column_create_instance(ACCVIEW_COLUMNS, accview_columns, NULL, ACCVIEW_PANE_SORT_DIR_ICON);
	if (new->columns == NULL) {
		accview_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read("LimAccViewCols"));
	column_init_window(new->columns, 0, FALSE, config_str_read("AccViewCols"));

	/* Initialise the global window sort defaults. */

	new->sort = sort_create_instance(SORT_DATE | SORT_ASCENDING, SORT_ROW | SORT_ASCENDING, NULL, NULL);
	if (new->sort == NULL) {
		accview_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete a global account view module instance, and all of its data.
 *
 * \param *instance		The instance to be deleted.
 */

void accview_delete_instance(struct accview_block *instance)
{
	if (instance == NULL)
		return;

	column_delete_instance(instance->columns);
	sort_delete_instance(instance->sort);

	heap_free(instance);
}


/**
 * Create and open a Account View window for the given file and account.
 *
 * \param *file			The file to open a window for.
 * \param account		The account to open a window for.
 */

void accview_open_window(struct file_block *file, acct_t account)
{
	int			height;
	os_error		*error;
	wimp_window_state	parent;
	struct accview_window	*view;

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	view = account_get_accview(file, account);

	/* Create or re-open the window. */

	if (view != NULL) {
		windows_open(view->accview_window);
		return;
	}

	transact_sort_file_data(file);

	/* The block pointer is put into the new variable, as the file->accounts[account].account_view pointer may move
	 * as a result of the flex heap shifting for heap_alloc().
	 */

	view = heap_alloc(sizeof(struct accview_window));
	account_set_accview(file, account, view);

	if (view == NULL) {
		error_msgs_report_info("AccviewMemErr1");
		return;
	}

	view->file = file;
	view->account = account;

	#ifdef DEBUG
	debug_printf("\\BCreate Account View for %d", account);
	#endif

	view->columns = NULL;
	view->sort = NULL;

	view->line_data = NULL;

	if (!accview_build(view)) {
		accview_delete_window(file, account);
		return;
	}

	/* Set the main window extent and create it. */

	*(view->window_title) = '\0';
	accview_window_def->title_data.indirected_text.text =
			view->window_title;

	height = (view->display_lines > MIN_ACCVIEW_ENTRIES) ?
			view->display_lines : MIN_ACCVIEW_ENTRIES;

	/* Clone the column settings from the global defaults. */

	view->columns = column_clone_instance(file->accviews->columns);
	if (view->columns == NULL) {
		accview_delete_window(file, account);
		error_msgs_report_info("AccviewMemErr1");
		return;
	}

	/* Clone the sort settings from the global defaults. */

	view->sort = sort_create_instance(SORT_DATE | SORT_ASCENDING, SORT_ROW | SORT_ASCENDING, &accview_sort_callbacks, view);
	if (view->sort == NULL) {
		accview_delete_window(file, account);
		error_msgs_report_info("AccviewMemErr1");
		return;
	}

	sort_copy_order(view->sort, file->accviews->sort);

	/* Find the position to open the window at. */

	transact_get_window_state(file, &parent);

	window_set_initial_area(accview_window_def, column_get_window_width(view->columns),
			(height * WINDOW_ROW_HEIGHT) + ACCVIEW_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	/* Set the scroll offset to show the contents of the transaction window */

	accview_window_def->yscroll = accview_get_y_offset_from_transact_window(view);

	error = xwimp_create_window(accview_window_def, &(view->accview_window));
	if (error != NULL) {
		accview_delete_window(file, account);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	#ifdef DEBUG
	debug_printf("Created window: %d, %d, %d, %d...", accview_window_def->visible.x0,
			accview_window_def->visible.x1, accview_window_def->visible.y0,
			accview_window_def->visible.y1);
	#endif

	/* Create the toolbar pane. */

	windows_place_as_toolbar(accview_window_def, accview_pane_def, ACCVIEW_TOOLBAR_HEIGHT - 4);
	columns_place_heading_icons(view->columns, accview_pane_def);

	accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) view->sort_sprite;
	accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			accview_pane_def->sprite_area;
	accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

	accview_adjust_sort_icon_data(view, &(accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON]));

	error = xwimp_create_window(accview_pane_def, &(view->accview_pane));
	if (error != NULL) {
		accview_delete_window(file, account);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	accview_build_window_title(file, account);

	/* Sort the window contents. */

	accview_sort(file, account);

	/* Open the window. */

	if (account_get_type(file, account) == ACCOUNT_FULL) {
		ihelp_add_window(view->accview_window, "AccView", accview_decode_window_help);
		ihelp_add_window(view->accview_pane, "AccViewTB", NULL);
	} else {
		ihelp_add_window(view->accview_window, "HeadView", accview_decode_window_help);
		ihelp_add_window(view->accview_pane, "HeadViewTB", NULL);
	}

	windows_open(view->accview_window);
	windows_open_nested_as_toolbar(view->accview_pane, view->accview_window,
			ACCVIEW_TOOLBAR_HEIGHT - 4, FALSE);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(view->accview_window, view);
	event_add_window_menu(view->accview_window, accview_window_menu);
	event_add_window_close_event(view->accview_window, accview_close_window_handler);
	event_add_window_mouse_event(view->accview_window, accview_window_click_handler);
	event_add_window_scroll_event(view->accview_window, accview_window_scroll_handler);
	event_add_window_redraw_event(view->accview_window, accview_window_redraw_handler);
	event_add_window_menu_prepare(view->accview_window, accview_window_menu_prepare_handler);
	event_add_window_menu_selection(view->accview_window, accview_window_menu_selection_handler);
	event_add_window_menu_warning(view->accview_window, accview_window_menu_warning_handler);
	event_add_window_menu_close(view->accview_window, accview_window_menu_close_handler);

	event_add_window_user_data(view->accview_pane, view);
	event_add_window_menu(view->accview_pane, accview_window_menu);
	event_add_window_mouse_event(view->accview_pane, accview_pane_click_handler);
	event_add_window_menu_prepare(view->accview_pane, accview_window_menu_prepare_handler);
	event_add_window_menu_selection(view->accview_pane, accview_window_menu_selection_handler);
	event_add_window_menu_warning(view->accview_pane, accview_window_menu_warning_handler);
	event_add_window_menu_close(view->accview_pane, accview_window_menu_close_handler);
}


/**
 * Close and delete the Account View Window associated with the given
 * file block and account.
 *
 * \param *file			The file to use.
 * \param account		The account to close the window for.
 */

void accview_delete_window(struct file_block *file, acct_t account)
{
	struct accview_window	*view;

	#ifdef DEBUG
	debug_printf("\\RDeleting account view window");
	debug_printf("Account: %d", account);
	#endif

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	view = account_get_accview(file, account);
	if (view == NULL)
		return;

	sort_dialogue_close(accview_sort_dialogue, view);

	column_delete_instance(view->columns);
	sort_delete_instance(view->sort);

	if (view->accview_window != NULL) {
		ihelp_remove_window(view->accview_window);
		event_delete_window(view->accview_window);
		wimp_delete_window(view->accview_window);
	}

	if (view->accview_pane != NULL) {
		ihelp_remove_window(view->accview_pane);
		event_delete_window(view->accview_pane);
		wimp_delete_window(view->accview_pane);
	}

	if (view->line_data != NULL)
		flexutils_free((void **) &(view->line_data));

	account_set_accview(file, account, NULL);
	heap_free(view);
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
	struct file_block	*file;
	int			line, xpos;
	tran_t			transaction;
	wimp_i			column;
	enum transact_field 	trans_col_from[] = {
					TRANSACT_FIELD_ROW,
					TRANSACT_FIELD_DATE,
					TRANSACT_FIELD_FROM,
					TRANSACT_FIELD_FROM,
					TRANSACT_FIELD_FROM,
					TRANSACT_FIELD_REF,
					TRANSACT_FIELD_AMOUNT,
					TRANSACT_FIELD_AMOUNT,
					TRANSACT_FIELD_AMOUNT,
					TRANSACT_FIELD_DESC
				};
	enum transact_field	trans_col_to[] = {
					TRANSACT_FIELD_ROW,
					TRANSACT_FIELD_DATE,
					TRANSACT_FIELD_TO,
					TRANSACT_FIELD_TO,
					TRANSACT_FIELD_TO,
					TRANSACT_FIELD_REF,
					TRANSACT_FIELD_AMOUNT,
					TRANSACT_FIELD_AMOUNT,
					TRANSACT_FIELD_AMOUNT,
					TRANSACT_FIELD_DESC
				};
	enum transact_field	*trans_col;
	wimp_window_state	window;
	enum transact_flags	toggle_flag;
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

	line = window_calculate_click_row(&(pointer->pos), &window, ACCVIEW_TOOLBAR_HEIGHT, windat->display_lines);

	/* If the line is a transaction, handle mouse clicks over it.  Menu clicks are ignored and dealt with in the
	 * else clause.
	 */

	if (line == -1)
		return;

	transaction = windat->line_data[windat->line_data[line].sort_index].transaction;

	xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;
	column = column_find_icon_from_xpos(windat->columns, xpos);

	if (column != ACCVIEW_COLUMN_RECONCILE && (pointer->buttons == wimp_DOUBLE_SELECT || pointer->buttons == wimp_DOUBLE_ADJUST)) {
		/* Handle double-clicks, which will locate the transaction in the main window.  Clicks in the reconcile
		 * column are not used, as these are used to toggle the reconcile flag.
		 */

		trans_col = (transact_get_from(file, transaction) == windat->account) ? trans_col_to : trans_col_from;

		transact_place_caret(file, transact_get_line_from_transaction(file, transaction), trans_col[column]);

		if (pointer->buttons == wimp_DOUBLE_ADJUST)
			transact_bring_window_to_top(file);
	} else if (column == ACCVIEW_COLUMN_RECONCILE && pointer->buttons == wimp_SINGLE_ADJUST) {
		/* Handle adjust-clicks in the reconcile column, to toggle the status. */

		toggle_flag = (transact_get_from(file, transaction) == windat->account) ? TRANS_REC_FROM : TRANS_REC_TO;
		transact_toggle_reconcile_flag(file, transaction, toggle_flag);
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
	struct file_block	*file;
	acct_t			account;
	wimp_window_state	window;
	wimp_icon_state		icon;
	int			ox;
	enum sort_type		sort_order;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL)
		return;

	/* Find the window's account. */

	file = windat->file;
	account = windat->account;
	if (file == NULL || account == NULL_ACCOUNT)
		return;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	column_update_heading_icon_click(windat->columns, pointer);

	/* Decode the mouse click. */

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case ACCVIEW_PANE_PARENT:
			transact_bring_window_to_top(windat->file);
			break;

		case ACCVIEW_PANE_PRINT:
			accview_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case ACCVIEW_PANE_EDIT:
			account_open_edit_window(windat->file, account, ACCOUNT_NULL, pointer);
			break;

		case ACCVIEW_PANE_GOTOEDIT:
			accview_scroll_to_transact_window(windat);
			break;

		case ACCVIEW_PANE_SORT:
			accview_open_sort_window(windat, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case ACCVIEW_PANE_PRINT:
			accview_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
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
			sort_order = column_get_sort_type_from_heading(windat->columns, pointer->i);

			if (sort_order != SORT_NONE) {
				sort_order |= (pointer->buttons == wimp_CLICK_SELECT * 256) ? SORT_ASCENDING : SORT_DESCENDING;

				sort_set_order(windat->sort, sort_order);

				accview_adjust_sort_icon(windat);
				windows_redraw(windat->accview_pane);
				accview_sort(file, account);

				sort_copy_order(file->accviews->sort, windat->sort);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimAccViewCols"));;
		column_start_drag(windat->columns, pointer, windat, windat->accview_window, accview_adjust_window_columns);
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

			line = window_calculate_click_row(&(pointer->pos), &window, ACCVIEW_TOOLBAR_HEIGHT, windat->display_lines);

			if (line != -1)
				accview_window_menu_line = line;
		}

		saveas_initialise_dialogue(accview_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(accview_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);

		switch (account_get_type(windat->file, windat->account)) {
		case ACCOUNT_FULL:
			msgs_lookup("AccviewMenuTitleAcc", accview_window_menu->title_data.text, 12);
			msgs_lookup("AccviewMenuEditAcc", menus_get_indirected_text_addr(accview_window_menu, ACCVIEW_MENU_EDITACCT), 20);
			ihelp_add_menu(accview_window_menu, "AccViewMenu");
			break;

		case ACCOUNT_IN:
		case ACCOUNT_OUT:
			msgs_lookup("AccviewMenuTitleHead", accview_window_menu->title_data.text, 12);
			msgs_lookup("AccviewMenuEditHead", menus_get_indirected_text_addr(accview_window_menu, ACCVIEW_MENU_EDITACCT), 20);
			ihelp_add_menu(accview_window_menu, "HeadViewMenu");
			break;

		case ACCOUNT_NULL:
		default:
			break;
		}
	}

	menus_shade_entry(accview_window_menu, ACCVIEW_MENU_FINDTRANS, accview_window_menu_line == -1);

	accview_force_window_redraw(windat, accview_window_menu_line, accview_window_menu_line, wimp_ICON_WINDOW);
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
		transact_place_caret(windat->file, transact_get_line_from_transaction(windat->file,
				windat->line_data[windat->line_data[accview_window_menu_line].sort_index].transaction),
				TRANSACT_FIELD_DATE);
		break;

	case ACCVIEW_MENU_GOTOTRANS:
		accview_scroll_to_transact_window(windat);
		break;

	case ACCVIEW_MENU_SORT:
		accview_open_sort_window(windat, &pointer);
		break;

	case ACCVIEW_MENU_EDITACCT:
		account_open_edit_window(windat->file, windat->account, ACCOUNT_NULL, &pointer);
		break;

	case ACCVIEW_MENU_PRINT:
		accview_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
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
		saveas_prepare_dialogue(accview_saveas_tsv);
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
	struct accview_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat != NULL)
		accview_force_window_redraw(windat, accview_window_menu_line, accview_window_menu_line, wimp_ICON_WINDOW);

	accview_window_menu_line = -1;
	ihelp_remove_menu(accview_window_menu);
}


/**
 * Process scroll events in the Account View window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void accview_window_scroll_handler(wimp_scroll *scroll)
{
	window_process_scroll_event(scroll, ACCVIEW_TOOLBAR_HEIGHT);

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

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
	enum accview_direction	transaction_direction;
	acct_t			account, transaction_account;
	date_t			transaction_date;
	enum account_type	account_type;
	int			top, base, y, select, credit_limit;
	tran_t			transaction;
	wimp_colour		shade_budget_col, shade_overdrawn_col, icon_fg_col, icon_fg_balance_col;
	char			icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more, shade_budget, shade_overdrawn;
	date_t			budget_start, budget_finish;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL || windat->account == NULL_ACCOUNT)
		return;

	account = windat->account;

	account_type = account_get_type(windat->file, account);
	credit_limit = account_get_credit_limit(windat->file, account);

	budget_get_dates(windat->file, &budget_start, &budget_finish);

	shade_budget = (account_type & (ACCOUNT_IN | ACCOUNT_OUT)) && config_opt_read ("ShadeBudgeted") &&
			(budget_start != NULL_DATE || budget_finish != NULL_DATE);
	shade_budget_col = config_int_read ("ShadeBudgetedColour");

	shade_overdrawn = (account_type & ACCOUNT_FULL) && config_opt_read ("ShadeOverdrawn");
	shade_overdrawn_col = config_int_read ("ShadeOverdrawnColour");

	/* Identify if there is a selected line to highlight. */

	if (redraw->w == event_get_current_menu_window())
		select = accview_window_menu_line;
	else
		select = -1;

	/* Set the horizontal positions of the icons for the account lines. */

	columns_place_table_icons_horizontally(windat->columns, accview_window_def, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);

	window_set_icon_templates(accview_window_def);

	/* Perform the redraw. */

	more = wimp_redraw_window(redraw);

	while (more) {
		window_plot_background(redraw, ACCVIEW_TOOLBAR_HEIGHT, wimp_COLOUR_WHITE, select, &top, &base);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(windat->columns, accview_window_def,
					WINDOW_ROW_Y0(ACCVIEW_TOOLBAR_HEIGHT, y), WINDOW_ROW_Y1(ACCVIEW_TOOLBAR_HEIGHT, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= windat->display_lines) {
				columns_plot_empty_table_icons(windat->columns);
				continue;
			}

			/* Find the transaction that applies to this line. */

			transaction = (y < windat->display_lines) ? (windat->line_data)[(windat->line_data)[y].sort_index].transaction : 0;

			transaction_direction = accview_get_transaction_direction(windat, transaction);
			transaction_date = transact_get_date(windat->file, transaction);

			/* work out the foreground colour for the line, based on whether the line is to be shaded or not. */

			if (shade_budget && (y < windat->display_lines) &&
					((budget_start == NULL_DATE || transaction_date < budget_start) ||
					(budget_finish == NULL_DATE || transaction_date > budget_finish))) {
				icon_fg_col = shade_budget_col;
				icon_fg_balance_col = shade_budget_col;
			} else if (shade_overdrawn && (y < windat->display_lines) &&
					((windat->line_data)[(windat->line_data)[y].sort_index].balance < -credit_limit)) {
				icon_fg_col = wimp_COLOUR_BLACK;
				icon_fg_balance_col = shade_overdrawn_col;
			} else {
				icon_fg_col = wimp_COLOUR_BLACK;
				icon_fg_balance_col = wimp_COLOUR_BLACK;
			}

			/* Row field */

			window_plot_int_field(ACCVIEW_ICON_ROW, transact_get_transaction_number(transaction), icon_fg_col);

			/* Date field */

			window_plot_date_field(ACCVIEW_ICON_DATE, transaction_date, icon_fg_col);

			/* From / To field */

			transaction_account = NULL_ACCOUNT;

			switch (transaction_direction) {
			case ACCVIEW_DIRECTION_FROM:
				transaction_account = transact_get_to(windat->file, transaction);
				window_plot_reconciled_field(ACCVIEW_ICON_REC, (transact_get_flags(windat->file, transaction) & TRANS_REC_FROM), icon_fg_col);
				break;
			case ACCVIEW_DIRECTION_TO:
				transaction_account = transact_get_from(windat->file, transaction);
				window_plot_reconciled_field(ACCVIEW_ICON_REC, (transact_get_flags(windat->file, transaction) & TRANS_REC_TO), icon_fg_col);
				break;
			case ACCVIEW_DIRECTION_NONE:
				window_plot_empty_field(ACCVIEW_ICON_REC);
			}

			window_plot_text_field(ACCVIEW_ICON_IDENT, account_get_ident(windat->file, transaction_account), icon_fg_col);
			window_plot_text_field(ACCVIEW_ICON_FROMTO, account_get_name(windat->file, transaction_account), icon_fg_col);

			/* Reference field */

			window_plot_text_field(ACCVIEW_ICON_REFERENCE, transact_get_reference(windat->file, transaction, NULL, 0), icon_fg_col);

			/* Payments and Receipts fields */

			switch (transaction_direction) {
			case ACCVIEW_DIRECTION_FROM:
				window_plot_currency_field(ACCVIEW_ICON_PAYMENTS, transact_get_amount(windat->file, transaction), icon_fg_col);
				window_plot_empty_field(ACCVIEW_ICON_RECEIPTS);
				break;
			case ACCVIEW_DIRECTION_TO:
				window_plot_empty_field(ACCVIEW_ICON_PAYMENTS);
				window_plot_currency_field(ACCVIEW_ICON_RECEIPTS, transact_get_amount(windat->file, transaction), icon_fg_col);
				break;
			case ACCVIEW_DIRECTION_NONE:
				window_plot_empty_field(ACCVIEW_ICON_PAYMENTS);
				window_plot_empty_field(ACCVIEW_ICON_RECEIPTS);
				break;
			}

			/* Balance field */

			window_plot_currency_field(ACCVIEW_ICON_BALANCE, (windat->line_data)[(windat->line_data)[y].sort_index].balance, icon_fg_balance_col);

			/* Description field */

			window_plot_text_field(ACCVIEW_ICON_DESCRIPTION, transact_get_description(windat->file, transaction, NULL, 0), icon_fg_col);
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
	struct file_block	*file;
	int			new_extent;
	wimp_window_info	window;

	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	columns_update_dragged(windat->columns, windat->accview_pane, NULL, group, width);

	column_copy_instance(windat->columns, file->accviews->columns);

	new_extent = column_get_window_width(windat->columns);

	accview_adjust_sort_icon(windat);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(windat->accview_window);
	windows_redraw(windat->accview_pane);

	/* Set the horizontal extent of the window and pane. */

	window.w = windat->accview_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = windat->accview_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(file, TRUE);
}


/**
 * Adjust the sort icon in an Account View window, to reflect the current column
 * heading positions.
 *
 * \param *view			The view to update the window for.
 */

static void accview_adjust_sort_icon(struct accview_window *view)
{
	wimp_icon_state		icon;

	if (view == NULL)
		return;

	icon.w = view->accview_pane;
	icon.i = ACCVIEW_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	accview_adjust_sort_icon_data(view, &(icon.icon));

	wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current Account View sort settings.
 *
 * \param *view			The view to update the window for.
 * \param *icon			The icon to be updated.
 */

static void accview_adjust_sort_icon_data(struct accview_window *view, wimp_icon *icon)
{
	enum sort_type	sort_order;

	if (view == NULL)
		return;

	sort_order = sort_get_order(view->sort);

	column_update_sort_indicator(view->columns, icon, accview_pane_def, sort_order);
}


/**
 * Set the extent of an account view window for the specified file.
 *
 * \param *view			The view to update the window for.
 */

static void accview_set_window_extent(struct accview_window *view)
{
	int	lines;


	if (view == NULL || view->account == NULL_ACCOUNT || view->accview_window == NULL)
		return;

	lines = (view->display_lines > MIN_ACCVIEW_ENTRIES) ? view->display_lines : MIN_ACCVIEW_ENTRIES;

	window_set_extent(view->accview_window, lines, ACCVIEW_TOOLBAR_HEIGHT, column_get_window_width(view->columns));
}


/**
 * Recreate the title of the specified Account View window connected to the
 * given file.
 *
 * \param *file			The file to rebuild the title for.
 * \param account		The account to rebilld the window title for.
 */

void accview_build_window_title(struct file_block *file, acct_t account)
{
	struct accview_window	*view;
	char			name[WINDOW_TITLE_LENGTH];

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	view = account_get_accview(file, account);
	if (view == NULL)
		return;

	file_get_leafname(file, name, WINDOW_TITLE_LENGTH);

	msgs_param_lookup("AccviewTitle", view->window_title, WINDOW_TITLE_LENGTH,
			account_get_name(file, account), name, NULL, NULL);

	wimp_force_redraw_title(view->accview_window);
}


/**
 * Force a redraw of the Account View window, for the given range of
 * lines.
 *
 * \param *view			The view to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void accview_force_window_redraw(struct accview_window *view, int from, int to, wimp_i column)
{
	wimp_window_info	window;

	if (view == NULL)
		return;

	window.w = view->accview_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(view->columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(ACCVIEW_TOOLBAR_HEIGHT, from);
	window.extent.y0 = WINDOW_ROW_BASE(ACCVIEW_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(view->accview_window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);
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
	int			xpos;
	wimp_i			icon;
	wimp_window_state	window;
	struct accview_window	*windat;

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

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, accview_window_def->icons[icon].data.indirected_text.validation, 'N'))
		string_printf(buffer, IHELP_INAME_LEN, "Col%d", icon);
}


/**
 * Open the Account List Sort dialogue for a given account list window.
 *
 * \param *view			The view to be sorted.
 * \param *ptr			The current Wimp pointer position.
 */

static void accview_open_sort_window(struct accview_window *view, wimp_pointer *ptr)
{
	if (view == NULL || ptr == NULL)
		return;

	sort_dialogue_open(accview_sort_dialogue, ptr, sort_get_order(view->sort), view);
}


/**
 * Take the contents of an updated Account List Sort window and process
 * the data.
 *
 * \param order			The new sort order selected by the user.
 * \param *data			The account list window associated with the Sort dialogue.
 * \return			TRUE if successful; else FALSE.
 */

static osbool accview_process_sort_window(enum sort_type order, void *data)
{
	struct accview_window	*windat = (struct accview_window *) data;

	if (windat == NULL)
		return FALSE;

	sort_set_order(windat->sort, order);

	accview_adjust_sort_icon(windat);
	windows_redraw(windat->accview_pane);
	accview_sort(windat->file, windat->account);

	sort_copy_order(windat->file->accviews->sort, windat->sort);

	return TRUE;
}


/**
 * Open the Account List Print dialogue for a given account list window.
 *
 * \param *view			The account view to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void accview_open_print_window(struct accview_window *view, wimp_pointer *ptr, osbool restore)
{
	if (view == NULL || view->file == NULL)
		return;

	print_dialogue_open_advanced(view->file->print, ptr, restore, "PrintAccview", "PrintTitleAccview", accview_print, view);
}


/**
 * Send the contents of the Account List Window to the printer, via the reporting
 * system.
 *
 * \param *report		The report handle to use for output.
 * \param *data			The account view window structure to be printed.
 * \param from			The date to print from.
 * \param to			The date to print to.
 * \return			Pointer to the report, or NULL on failure.
 */

static struct report *accview_print(struct report *report, void *data, date_t from, date_t to)
{
	struct accview_window	*view = data;
	int			line, column, sort;
	tran_t			transaction = 0;
	enum accview_direction	transaction_direction;
	date_t			transaction_date;
	acct_t			transaction_account;
	char			rec_char[REC_FIELD_LEN];
	wimp_i			columns[ACCVIEW_COLUMNS];

	if (report == NULL || view == NULL || view->file == NULL)
		return NULL;

	if (!column_get_icons(view->columns, columns, ACCVIEW_COLUMNS, FALSE))
		return NULL;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	hourglass_on();

	/* Output the page title. */

	stringbuild_reset();

	stringbuild_add_string("\\b\\u");
	stringbuild_add_message_param("AccviewTitle",
			account_get_name(view->file, view->account),
			file_get_leafname(view->file, NULL, 0),
			NULL, NULL);

	stringbuild_report_line(report, 1);

	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	stringbuild_reset();
	columns_print_heading_names(view->columns, view->accview_pane);
	stringbuild_report_line(report, 0);

	/* Output the transaction data as a set of delimited lines. */

	for (line = 0; line < view->display_lines; line++) {
		sort = view->line_data[line].sort_index;
		transaction = view->line_data[sort].transaction;

		transaction_date = transact_get_date(view->file, transaction);

		if ((from != NULL_DATE && transaction_date < from) || (to != NULL_DATE && transaction_date > to))
			continue;

		transaction_direction = accview_get_transaction_direction(view, transaction);

		if (transaction_direction == ACCVIEW_DIRECTION_FROM)
			transaction_account = transact_get_to(view->file, transaction);
		else
			transaction_account = transact_get_from(view->file, transaction);

		stringbuild_reset();

		for (column = 0; column < ACCVIEW_COLUMNS; column++) {
			if (column == 0)
				stringbuild_add_string("\\k");
			else
				stringbuild_add_string("\\t");

			switch (columns[column]) {
			case ACCVIEW_ICON_ROW:
				stringbuild_add_printf("\\v\\d\\r%d", transact_get_transaction_number(transaction));
				break;
			case ACCVIEW_ICON_DATE:
				stringbuild_add_string("\\v\\c");
				stringbuild_add_date(transaction_date);
				break;
			case ACCVIEW_ICON_IDENT:
				stringbuild_add_string(account_get_ident(view->file, transaction_account));
				break;
			case ACCVIEW_ICON_REC:
				if (transaction_direction == ACCVIEW_DIRECTION_FROM) {
					if (transact_get_flags(view->file, transaction) & TRANS_REC_FROM)
						stringbuild_add_string(rec_char);
				} else {
					if (transact_get_flags(view->file, transaction) & TRANS_REC_TO)
						stringbuild_add_string(rec_char);
				}
				break;
			case ACCVIEW_ICON_FROMTO:
				stringbuild_add_string("\\v");
				stringbuild_add_string(account_get_name(view->file, transaction_account));
				break;
			case ACCVIEW_ICON_REFERENCE:
				stringbuild_add_string("\\v");
				stringbuild_add_string(transact_get_reference(view->file, transaction, NULL, 0));
				break;
			case ACCVIEW_ICON_PAYMENTS:
				stringbuild_add_string("\\v\\d\\r");
				if (transaction_direction == ACCVIEW_DIRECTION_FROM)
					stringbuild_add_currency(transact_get_amount(view->file, transaction), FALSE);
				break;
			case ACCVIEW_ICON_RECEIPTS:
				stringbuild_add_string("\\v\\d\\r");
				if (transaction_direction == ACCVIEW_DIRECTION_TO)
					stringbuild_add_currency(transact_get_amount(view->file, transaction), FALSE);
				break;
			case ACCVIEW_ICON_BALANCE:
				stringbuild_add_string("\\v\\d\\r");
				stringbuild_add_currency(view->line_data[sort].balance, FALSE);
				break;
			case ACCVIEW_ICON_DESCRIPTION:
				stringbuild_add_string("\\v");
				stringbuild_add_string(transact_get_description(view->file, transaction, NULL, 0));
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
 * Sort the account view list in a given file based on that file's sort setting.
 *
 * \param *file			The file to work on.
 * \param account		The account to sort the view for.
 */

void accview_sort(struct file_block *file, acct_t account)
{
	struct accview_window	*view;

	#ifdef DEBUG
	debug_printf("Sorting accview window");
	#endif

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	view = account_get_accview(file, account);
	if (view == NULL)
		return;

	hourglass_on();

	/* Run the sort. */

	sort_process(view->sort, view->display_lines);

	accview_force_window_redraw(view, 0, view->display_lines - 1, wimp_ICON_WINDOW);

	hourglass_off();
}


/**
 * Compare two lines of an account view, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The index of the first line to be compared.
 * \param index2		The index of the second line to be compared.
 * \param *data			Client specific data, which is our window block.
 * \return			The comparison result.
 */

static int accview_sort_compare(enum sort_type type, int index1, int index2, void *data)
{
	struct accview_window *view = data;

	if (view == NULL)
		return 0;

	switch (type) {
	case SORT_ROW:
		return (transact_get_transaction_number(view->line_data[view->line_data[index1].sort_index].transaction) -
				transact_get_transaction_number(view->line_data[view->line_data[index2].sort_index].transaction));

	case SORT_DATE:
		return ((transact_get_date(view->file, view->line_data[view->line_data[index1].sort_index].transaction) & DATE_SORT_MASK) -
				(transact_get_date(view->file, view->line_data[view->line_data[index2].sort_index].transaction) & DATE_SORT_MASK));

	case SORT_FROMTO:
		return strcmp(account_get_name(view->file,
				(accview_get_transaction_direction(view, view->line_data[view->line_data[index1].sort_index].transaction) == ACCVIEW_DIRECTION_FROM) ?
				(transact_get_to(view->file, view->line_data[view->line_data[index1].sort_index].transaction)) :
				(transact_get_from(view->file, view->line_data[view->line_data[index1].sort_index].transaction))),
				account_get_name(view->file,
				(accview_get_transaction_direction(view, view->line_data[view->line_data[index2].sort_index].transaction) == ACCVIEW_DIRECTION_FROM) ?
				(transact_get_to(view->file, view->line_data[view->line_data[index2].sort_index].transaction)) :
				(transact_get_from(view->file, view->line_data[view->line_data[index2].sort_index].transaction))));

	case SORT_REFERENCE:
		return strcmp(transact_get_reference(view->file, view->line_data[view->line_data[index1].sort_index].transaction, NULL, 0),
				transact_get_reference(view->file, view->line_data[view->line_data[index2].sort_index].transaction, NULL, 0));

	case SORT_PAYMENTS:
		return (((accview_get_transaction_direction(view, view->line_data[view->line_data[index1].sort_index].transaction) == ACCVIEW_DIRECTION_FROM) ?
				(transact_get_amount(view->file, view->line_data[view->line_data[index1].sort_index].transaction)) : 0) -
				((accview_get_transaction_direction(view, view->line_data[view->line_data[index2].sort_index].transaction) == ACCVIEW_DIRECTION_FROM) ?
				(transact_get_amount(view->file, view->line_data[view->line_data[index2].sort_index].transaction)) : 0));

	case SORT_RECEIPTS:
		return (((accview_get_transaction_direction(view, view->line_data[view->line_data[index1].sort_index].transaction) == ACCVIEW_DIRECTION_TO) ?
				(transact_get_amount(view->file, view->line_data[view->line_data[index1].sort_index].transaction)) : 0) -
				((accview_get_transaction_direction(view, view->line_data[view->line_data[index2].sort_index].transaction) == ACCVIEW_DIRECTION_TO) ?
				(transact_get_amount(view->file, view->line_data[view->line_data[index2].sort_index].transaction)) : 0));

	case SORT_BALANCE:
		return (view->line_data[view->line_data[index1].sort_index].balance -
				view->line_data[view->line_data[index2].sort_index].balance);

	case SORT_DESCRIPTION:
		return strcmp(transact_get_description(view->file, view->line_data[view->line_data[index1].sort_index].transaction, NULL, 0),
				transact_get_description(view->file, view->line_data[view->line_data[index2].sort_index].transaction, NULL, 0));

	default:
		return 0;
	}
}


/**
 * Swap the sort index of two lines of am account view.
 *
 * \param index1		The index of the first line to be swapped.
 * \param index2		The index of the second line to be swapped.
 * \param *data			Client specific data, which is our window block.
 */

static void accview_sort_swap(int index1, int index2, void *data)
{
	struct accview_window	*view = data;
	int			temp;

	if (view == NULL)
		return;

	temp = view->line_data[index1].sort_index;
	view->line_data[index1].sort_index = view->line_data[index2].sort_index;
	view->line_data[index2].sort_index = temp;
}


/**
 * Build a redraw list for an account statement view window from scratch.
 * Allocate a flex block big enough to take all the transactions in the file,
 * fill it as required, then shrink it down again to the correct size.
 *
 * \param *view			The view to be built.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool accview_build(struct accview_window *view)
{
	int	i, lines = 0;

	if (view == NULL || view->file == NULL)
		return FALSE;


	#ifdef DEBUG
	debug_printf("\\BBuilding account statement view");
	#endif

	if (!flexutils_allocate((void **) &(view->line_data), sizeof(struct accview_redraw), transact_get_count(view->file))) {
		error_msgs_report_info("AccviewMemErr2");
		return FALSE;
	}

	lines = accview_calculate(view);

	if (!flexutils_resize((void **) &(view->line_data), sizeof(struct accview_redraw), lines)) {
		flexutils_free((void **) &(view->line_data));
		error_msgs_report_info("BadMemory");
		return FALSE;
	}

	for (i = 0; i < lines; i++)
		view->line_data[i].sort_index = i;

	return TRUE;
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

void accview_rebuild(struct file_block *file, acct_t account)
{
	struct accview_window	*view;

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	view = account_get_accview(file, account);
	if (view == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\BRebuilding account statement view");
	#endif

	transact_sort_file_data(file);

	if (view->line_data != NULL)
		flexutils_free((void **) &(view->line_data));

	if (!accview_build(view)) {
		accview_delete_window(file, account);
		return;
	}

	accview_set_window_extent(view);
	accview_sort(file, account);
	windows_redraw(view->accview_window);
}


/**
 * Calculate the contents of an account view redraw block: entering
 * transaction references and calculating a running balance for the display.
 *
 * This relies on there being enough space in the block to take a line for
 * every transaction.  If it is called for an existing view, it relies on the
 * number of lines not having changed!
 */

static int accview_calculate(struct accview_window *view)
{
	int			lines = 0, i, balance;
	enum accview_direction	direction;
	struct file_block	*file;

	if (view == NULL || view->file == NULL || view->account == NULL_ACCOUNT)
		return lines;

	file = view->file;

	hourglass_on();

	balance = account_get_opening_balance(view->file, view->account);

	for (i = 0; i < transact_get_count(file); i++) {
		direction = accview_get_transaction_direction(view, i);

		if (direction != ACCVIEW_DIRECTION_NONE) {
			(view->line_data)[lines].transaction = i;

			if (direction == ACCVIEW_DIRECTION_FROM)
				balance -= transact_get_amount(file, i);
			else
				balance += transact_get_amount(file, i);

			(view->line_data)[lines].balance = balance;

			lines++;
		}
	}

	view->display_lines = lines;

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

void accview_recalculate(struct file_block *file, acct_t account, int transaction)
{
	struct accview_window	*view;

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	view = account_get_accview(file, account);
	if (view == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\BRecalculating account statement view");
	#endif

	transact_sort_file_data(file);

	accview_calculate(view);
	accview_force_window_redraw(view, accview_get_line_from_transaction(view, transaction),
			view->display_lines - 1, wimp_ICON_WINDOW);
}


/**
 * Redraw the line in an account view corresponding to the given transaction.
 * If the transaction does not feature in the account, nothing is done.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be redrawn.
 * \param transaction		The transaction to be redrawn.
 */

void accview_redraw_transaction(struct file_block *file, acct_t account, int transaction)
{
	struct accview_window	*view;
	int			line;

	if (file == NULL || account == NULL_ACCOUNT)
		return;

	view = account_get_accview(file, account);
	if (view == NULL)
		return;

	line = accview_get_line_from_transaction(view, transaction);

	if (view != NULL && line != -1)
		accview_force_window_redraw(view, line, line, wimp_ICON_WINDOW);
}


/**
 * Re-index the account views in a file.  This can *only* be done after
 * transact_sort_file_data() has been called, as it requires data set up
 * in the transaction block by that call.
 *
 * \param *file			The file to reindex.
 */

void accview_reindex_all(struct file_block *file)
{
	acct_t			account;
	int			line, transaction;
	struct accview_window	*view;

	if (file == NULL)
		return;

	#ifdef DEBUG
	debug_printf("Reindexing account views...");
	#endif

	for (account = 0; account < account_get_count(file); account++) {
		view = account_get_accview(file, account);

		if (view != NULL && view->line_data != NULL) {
			for (line = 0; line < view->display_lines; line++) {
				transaction = (view->line_data)[line].transaction;
				(view->line_data)[line].transaction = transact_get_sort_workspace(file, transaction);
			}

			accview_force_window_redraw(view, 0, view->display_lines - 1, ACCVIEW_PANE_ROW);
		}
	}
}


/**
 * Fully redraw all of the open account views in a file.
 *
 * \param *file			The file to be redrawn.
 */

void accview_redraw_all(struct file_block *file)
{
	acct_t			account;
	struct accview_window	*view;

	if (file == NULL)
		return;

	for (account = 0; account < account_get_count(file); account++) {
		view = account_get_accview(file, account);

		if (view != NULL && view->accview_window != NULL)
			windows_redraw(view->accview_window);
	}
}


/**
 * Fully recalculate all of the open account views in a file.
 *
 * \param *file			The file to be recalculated.
 */

void accview_recalculate_all(struct file_block *file)
{
	acct_t			account;
	struct accview_window	*view;

	if (file == NULL)
		return;

	for (account = 0; account < account_get_count(file); account++) {
		view = account_get_accview(file, account);

		if (view != NULL && view->accview_window != NULL)
			accview_recalculate(file, account, 0);
	}
}


/**
 * Fully rebuild all of the open account views in a file.
 *
 * \param *file			The file to be rebuilt.
 */

void accview_rebuild_all(struct file_block *file)
{
	acct_t			account;
	struct accview_window	*view;

	if (file == NULL)
		return;

	for (account = 0; account < account_get_count(file); account++) {
		view = account_get_accview(file, account);

		if (view != NULL && view->accview_window != NULL)
			accview_rebuild(file, account);
	}
}


/**
 * Test a transaction to see if it is a transfer from or to a given account
 * view.
 *
 * \param *view			The view to test the transaction against.
 * \param transaction		The transaction to test.
 * \return			The status of the transaction.
 */

static enum accview_direction accview_get_transaction_direction(struct accview_window *view, int transaction)
{
	if (view == NULL || view->file == NULL)
		return ACCVIEW_DIRECTION_NONE;
	else if (transact_get_from(view->file, transaction) == view->account)
		return ACCVIEW_DIRECTION_FROM;
	else if (transact_get_to(view->file, transaction) == view->account)
		return ACCVIEW_DIRECTION_TO;
	else
		return ACCVIEW_DIRECTION_NONE;
}


/**
 * Convert a transaction number into a line in a given account view.
 *
 * \param *view			The view to locate the transaction in.
 * \param transaction		The transaction to locate.
 * \return			The corresponding account view line, or -1.
 */

static int accview_get_line_from_transaction(struct accview_window *view, int transaction)
{
	int	line = -1, index = -1, i;

	if (view == NULL || view->line_data == NULL)
		return index;

	for (i = 0; i < view->display_lines && line == -1; i++)
		if ((view->line_data)[i].transaction == transaction)
			line = i;

	if (line != -1)
		for (i = 0; i < view->display_lines && index == -1; i++)
			if ((view->line_data)[i].sort_index == line)
				index = i;

	return index;
}


/**
 * Return the line in an account view which is most closely associated
 * with the transaction at the centre of the transaction window for the
 * parent file.
 *
 * \param *view			The view to use.
 * \return			The line in the account view, or 0.
 */

static int accview_get_line_from_transact_window(struct accview_window *view)
{
	int	centre_transact, line = 0;

	if (view == NULL || view->file == NULL || view->account == NULL_ACCOUNT)
		return line;

	centre_transact = transact_find_nearest_window_centre(view->file, view->account);
	line = accview_get_line_from_transaction(view, centre_transact);

	if (line == -1)
		line = 0;

	return line;
}


/**
 * Get a Y offset in OS units for an account view window based on the transaction
 * which is at the centre of the transaction window.
 *
 * \param *view			The account view to use.
 * \return			The Y offset in OS units, or 0.
 */

static int accview_get_y_offset_from_transact_window(struct accview_window *view)
{
	if (view == NULL)
		return 0;

	return -accview_get_line_from_transact_window(view) * WINDOW_ROW_HEIGHT;
}


/**
 * Scroll an account view window so that it displays lines close to the current
 * transaction window scroll offset.
 *
 * \param *view			The account view to work on.
 */

static void accview_scroll_to_transact_window(struct accview_window *view)
{
	if (view == NULL)
		return;

	accview_scroll_to_line(view, accview_get_line_from_transact_window(view));
}


/**
 * Scroll an account view window so that the specified line appears within
 * the visible area.
 *
 * \param *view			The account view to work on.
 * \param line			The line to scroll in to view.
 */

static void accview_scroll_to_line(struct accview_window *view, int line)
{
	wimp_window_state	window;
	int	height, top, bottom;

	if (view == NULL || view->file == NULL || view->account == NULL_ACCOUNT || view->accview_window == NULL)
		return;

	window.w = view->accview_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
	 * This will allow the edit line to be aligned with the top or bottom of the window.
	 */

	height = window.visible.y1 - window.visible.y0 - WINDOW_ROW_HEIGHT - ACCVIEW_TOOLBAR_HEIGHT;

	/* Calculate the top full line and bottom full line that are showing in the window.  Part lines don't
	 * count and are discarded.
	 */

	top = (-window.yscroll + WINDOW_ROW_ICON_HEIGHT) / WINDOW_ROW_HEIGHT;
	bottom = height / WINDOW_ROW_HEIGHT + top;

	/* If the edit line is above or below the visible area, bring it into range. */

	if (line < top) {
		window.yscroll = -(line * WINDOW_ROW_HEIGHT);
		wimp_open_window((wimp_open *) &window);
	}

	if (line > bottom) {
		window.yscroll = -(line * WINDOW_ROW_HEIGHT - height);
		wimp_open_window((wimp_open *) &window);
	}
}


/**
 * Save the accview details to a CashBook file.
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void accview_write_file(struct file_block *file, FILE *out)
{
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	column_write_as_text(file->accviews->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	sort_write_as_text(file->accviews->sort, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "SortOrder: %s\n", buffer);
}


/**
 * Process a WinColumns line from the Accounts section of a file.
 *
 * \param *file			The file being read in to.
 * \param format		The format of the disc file.
 * \param *columns		The column text line.
 */

void accview_read_file_wincolumns(struct file_block *file, int format, char *columns)
{
	/* For file format 1.00 or older, there's no row column at the
	 * start of the line so skip on to column 1 (date).
	 */

	column_init_window(file->accviews->columns, (format <= 100) ? 1 : 0, TRUE, columns);
}


/**
 * Process a SortOrder line from the Accounts section of a file.
 *
 * \param *file			The file being read in to.
 * \param *columns		The sort order text line.
 */

void accview_read_file_sortorder(struct file_block *file, char *order)
{
	sort_read_from_text(file->accviews->sort, order);
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

	accview_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

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

	accview_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

	return TRUE;
}


/**
 * Export the account view transaction data from a file into CSV or TSV format.
 *
 * \param *view			The account view to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void accview_export_delimited(struct accview_window *view, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE			*out;
	enum accview_direction	transaction_direction;
	int			line;
	tran_t			transaction;
	char			buffer[FILING_DELIMITED_FIELD_LEN];

	if (view == NULL || view->file == NULL || view->account == NULL_ACCOUNT)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	columns_export_heading_names(view->columns, view->accview_pane, out, format, buffer, FILING_DELIMITED_FIELD_LEN);

	/* Output the transaction data as a set of delimited lines. */
	for (line = 0; line < view->display_lines; line++) {
		transaction = (view->line_data)[(view->line_data)[line].sort_index].transaction;
		transaction_direction = accview_get_transaction_direction(view, transaction);

		string_printf(buffer, FILING_DELIMITED_FIELD_LEN, "%d", transact_get_transaction_number(transaction));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		date_convert_to_string(transact_get_date(view->file, transaction), buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		if (transaction_direction == ACCVIEW_DIRECTION_FROM)
			account_build_name_pair(view->file, transact_get_to(view->file, transaction), buffer, FILING_DELIMITED_FIELD_LEN);
		else
			account_build_name_pair(view->file, transact_get_from(view->file, transaction), buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		filing_output_delimited_field(out, transact_get_reference(view->file, transaction, buffer, FILING_DELIMITED_FIELD_LEN),
				format, DELIMIT_NONE);

		if (transaction_direction == ACCVIEW_DIRECTION_FROM) {
			currency_convert_to_string(transact_get_amount(view->file, transaction), buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
			filing_output_delimited_field(out, "", format, DELIMIT_NUM);
		} else {
			currency_convert_to_string(transact_get_amount(view->file, transaction), buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, "", format, DELIMIT_NUM);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
		}

		currency_convert_to_string(view->line_data[line].balance, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, transact_get_description(view->file, transaction, buffer, FILING_DELIMITED_FIELD_LEN),
				format, DELIMIT_LAST);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}

