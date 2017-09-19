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
 * \file: account_list_window.c
 *
 * Account List Window implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"
#include "oslib/dragasprite.h"
#include "oslib/wimpspriteop.h"
#include "oslib/hourglass.h"

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
#include "account_list_window.h"

#include "account_account_dialogue.h"
#include "account_heading_dialogue.h"
#include "account_idnum.h"
#include "account_menu.h"
#include "account_section_dialogue.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "interest.h"
#include "presets.h"
#include "print_dialogue.h"
#include "report.h"
#include "sorder.h"
#include "stringbuild.h"
#include "transact.h"
#include "window.h"


/* Account Window icons. */

#define ACCOUNT_ICON_IDENT 0
#define ACCOUNT_ICON_NAME 1
#define ACCOUNT_ICON_STATEMENT 2
#define ACCOUNT_ICON_CURRENT 3
#define ACCOUNT_ICON_FINAL 4
#define ACCOUNT_ICON_BUDGET 5
#define ACCOUNT_ICON_HEADING 6
#define ACCOUNT_ICON_FOOT_NAME 7
#define ACCOUNT_ICON_FOOT_STATEMENT 8
#define ACCOUNT_ICON_FOOT_CURRENT 9
#define ACCOUNT_ICON_FOOT_FINAL 10
#define ACCOUNT_ICON_FOOT_BUDGET 11

/* Toolbar icons. */

#define ACCOUNT_PANE_NAME 0
#define ACCOUNT_PANE_STATEMENT 1
#define ACCOUNT_PANE_CURRENT 2
#define ACCOUNT_PANE_FINAL 3
#define ACCOUNT_PANE_BUDGET 4

#define ACCOUNT_PANE_PARENT 5
#define ACCOUNT_PANE_ADDACCT 6
#define ACCOUNT_PANE_ADDSECT 7
#define ACCOUNT_PANE_PRINT 8

/* Footer icons. */

#define ACCOUNT_FOOTER_NAME 0
#define ACCOUNT_FOOTER_STATEMENT 1
#define ACCOUNT_FOOTER_CURRENT 2
#define ACCOUNT_FOOTER_FINAL 3
#define ACCOUNT_FOOTER_BUDGET 4

/* AccList menu */

#define ACCLIST_MENU_VIEWACCT 0
#define ACCLIST_MENU_EDITACCT 1
#define ACCLIST_MENU_EDITSECT 2
#define ACCLIST_MENU_NEWACCT 3
#define ACCLIST_MENU_NEWHEADER 4
#define ACCLIST_MENU_EXPCSV 5
#define ACCLIST_MENU_EXPTSV 6
#define ACCLIST_MENU_PRINT 7

/* Account window details */

#define ACCOUNT_WINDOWS 3

#define ACCOUNT_COLUMNS 6
#define ACCOUNT_TOOLBAR_HEIGHT 132
#define ACCOUNT_FOOTER_HEIGHT 36
#define MIN_ACCOUNT_ENTRIES 10

#define ACCOUNT_NUM_COLUMNS 4
#define ACCOUNT_NUM_COLUMN_STATEMENT 0
#define ACCOUNT_NUM_COLUMN_CURRENT 1
#define ACCOUNT_NUM_COLUMN_FINAL 2
#define ACCOUNT_NUM_COLUMN_BUDGET 3

#define ACCOUNT_LIST_WINDOW_PANES 2
#define ACCOUNT_PANE_ACCOUNT 0
#define ACCOUNT_PANE_HEADING 1




/* Account Window column map. */

static struct column_map account_columns[] = {
	{ACCOUNT_ICON_IDENT, ACCOUNT_PANE_NAME, ACCOUNT_FOOTER_NAME, SORT_NONE},
	{ACCOUNT_ICON_NAME, ACCOUNT_PANE_NAME, ACCOUNT_FOOTER_NAME, SORT_NONE},
	{ACCOUNT_ICON_STATEMENT, ACCOUNT_PANE_STATEMENT, ACCOUNT_FOOTER_STATEMENT, SORT_NONE},
	{ACCOUNT_ICON_CURRENT, ACCOUNT_PANE_CURRENT, ACCOUNT_FOOTER_CURRENT, SORT_NONE},
	{ACCOUNT_ICON_FINAL, ACCOUNT_PANE_FINAL, ACCOUNT_FOOTER_FINAL, SORT_NONE},
	{ACCOUNT_ICON_BUDGET, ACCOUNT_PANE_BUDGET, ACCOUNT_FOOTER_BUDGET, SORT_NONE}
};

static struct column_extra account_extra_columns[] = {
	{ACCOUNT_ICON_HEADING, 0, 5},
	{ACCOUNT_ICON_FOOT_NAME, 0, 1},
	{ACCOUNT_ICON_FOOT_STATEMENT, 2, 2},
	{ACCOUNT_ICON_FOOT_CURRENT, 3, 3},
	{ACCOUNT_ICON_FOOT_FINAL, 4, 4},
	{ACCOUNT_ICON_FOOT_BUDGET, 5, 5},
	{wimp_ICON_WINDOW, 0, 0}
};

/* Account window line redraw data struct */

struct account_redraw {
	enum account_line_type	type;						/* Type of line (account, header, footer, blank, etc).		*/

	acct_t			account;					/* Number of account.						*/
	amt_t			total[ACCOUNT_NUM_COLUMNS];			/* Balance totals for section.					*/
	char			heading[ACCOUNT_SECTION_LEN];			/* Heading for section.						*/
};


/**
 * Account Window data structure -- implementation.
 */

struct account_window {
	struct account_block	*instance;					/**< The instance owning the block (for reverse lookup).	*/
	int			entry;						/**< The array index of the block (for reverse lookup).		*/

	/* Account window handle and title details. */

	wimp_w			account_window;					/* Window handle of the account window */
	char			window_title[WINDOW_TITLE_LENGTH];
	wimp_w			account_pane;					/* Window handle of the account window toolbar pane */
	wimp_w			account_footer;					/* Window handle of the account window footer pane */
	char			footer_icon[ACCOUNT_NUM_COLUMNS][AMOUNT_FIELD_LEN]; /* Indirected blocks for footer icons. */

	/* Display column details. */

	struct column_block	*columns;					/**< Instance handle of the column definitions.				*/

	/* Data parameters */

	enum account_type	type;						/* Type of accounts contained within the window */

	int			display_lines;					/* Count of the lines in the window */
	struct account_redraw	*line_data;					/* Pointer to array of line data for the redraw */
};



/* Global Variables. */

/**
 * The definition for the Accounts List Window.
 */

static wimp_window		*account_list_window_def = NULL;

/**
 * The definitions for the account and heading list toolbar panes.
 */

static wimp_window		*account_list_window_pane_def[ACCOUNT_LIST_WINDOW_PANES] = {NULL, NULL};

/**
 *
 */

static wimp_window		*account_foot_def = NULL;			/**< The definition for the Accounts List Footer pane.			*/
static wimp_menu		*account_window_menu = NULL;			/**< The Accounts List Window menu handle.				*/
static int			account_window_menu_line = -1;			/**< The line over which the Accounts List Window Menu was opened.	*/

/* SaveAs Dialogue Handles. */

static struct saveas_block	*account_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*account_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/




/* \TODO -- These entries should probably become struct account_window * pointers? */


/* Account List Window drags. */

static osbool			account_dragging_sprite = FALSE;		/**< True if the account line drag is using a sprite.			*/
static struct account_window	*account_dragging_owner = NULL;			/**< The window of the account list in which the drag occurs.		*/
static int			account_dragging_start_line = -1;		/**< The line where an account entry drag was started.			*/


static void			account_delete_window(struct account_window *window);
static void			account_close_window_handler(wimp_close *close);
static void			account_window_click_handler(wimp_pointer *pointer);
static void			account_pane_click_handler(wimp_pointer *pointer);
static void			account_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void			account_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void			account_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void			account_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void			account_window_scroll_handler(wimp_scroll *scroll);
static void			account_window_redraw_handler(wimp_draw *redraw);
static void			account_adjust_window_columns(void *data, wimp_i icon, int width);
static void			account_set_window_extent(struct account_window *windat);
static void			account_build_window_title(struct file_block *file, int entry);
static void			account_force_window_redraw(struct file_block *file, int entry, int from, int to, wimp_i column);
static void			account_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);


static osbool			account_process_account_edit_window(struct account_block *instance, acct_t account, char* name, char *ident,
						amt_t credit_limit, amt_t opening_balance, struct account_idnum *cheque_number, struct account_idnum *payin_number,
						acct_t offset_against, char *account_num, char *sort_code, char address[][ACCOUNT_ADDR_LEN]);
static osbool			account_process_heading_edit_window(struct account_block *instance, acct_t account, char* name, char *ident, amt_t budget, enum account_type type);
static osbool			account_delete_from_edit_window(struct account_block *instance, acct_t account);


static void			account_open_section_edit_window(struct account_window *window, int line, wimp_pointer *ptr);
static osbool			account_process_section_edit_window(struct account_window *window, int line, char* name, enum account_line_type type);
static osbool			account_delete_from_section_edit_window(struct account_window *window, int line);

static void			account_open_print_window(struct account_window *window, wimp_pointer *ptr, osbool restore);
static struct report		*account_print(struct report *report, void *data);

static void			account_start_drag(struct account_window *windat, int line);
static void			account_terminate_drag(wimp_dragged *drag, void *data);

static osbool			account_save_csv(char *filename, osbool selection, void *data);
static osbool			account_save_tsv(char *filename, osbool selection, void *data);
static void			account_export_delimited(struct account_window *windat, char *filename, enum filing_delimit_type format, int filetype);




/**
 * Initialise the Account List Window.
 */

void account_list_window_initialise(osspriteop_area *sprites)
{
	account_list_window_def = templates_load_window("Account");
	account_list_window_def->icon_count = 0;

	account_list_window_pane_def[ACCOUNT_PANE_ACCOUNT] = templates_load_window("AccountATB");
	account_list_window_pane_def[ACCOUNT_PANE_ACCOUNT]->sprite_area = sprites;

	account_list_window_pane_def[ACCOUNT_PANE_HEADING] = templates_load_window("AccountHTB");
	account_list_window_pane_def[ACCOUNT_PANE_HEADING]->sprite_area = sprites;

	account_foot_def = templates_load_window("AccountTot");

	account_window_menu = templates_get_menu("AccountListMenu");

	account_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", account_save_csv);
	account_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", account_save_tsv);
}



/**
 * Create and open an Accounts List window for the given file and account type.
 *
 * \param *file			The file to open a window for.
 * \param type			The type of account to open a window for.
 */

void account_open_window(struct file_block *file, enum account_type type)
{
	int			entry, tb_type, height;
	os_error		*error;
	wimp_window_state	parent;
	struct account_window	*window;

	/* Find the window block to use. */

	entry = account_find_window_entry_from_type(file, type);

	if (entry == -1)
		return;

	window = &(file->accounts->account_windows[entry]);

	/* Create or re-open the window. */

	if (window->account_window != NULL) {
		windows_open(window->account_window);
		return;
	}

	/* Set the main window extent and create it. */

	*(window->window_title) = '\0';
	account_list_window_def->title_data.indirected_text.text = window->window_title;

	height =  (window->display_lines > MIN_ACCOUNT_ENTRIES) ? window->display_lines : MIN_ACCOUNT_ENTRIES;

	/* Find the position to open the window at. */

	transact_get_window_state(file, &parent);

	window_set_initial_area(account_list_window_def, column_get_window_width(window->columns),
			(height * WINDOW_ROW_HEIGHT) + ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT + 2,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window (account_list_window_def, &(window->account_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		error_report_info("Main window");
		account_delete_window(window);
		return;
	}

	/* Create the toolbar pane. */

	tb_type = (type == ACCOUNT_FULL) ? ACCOUNT_PANE_ACCOUNT : ACCOUNT_PANE_HEADING;

	windows_place_as_toolbar(account_list_window_def, account_list_window_pane_def[tb_type], ACCOUNT_TOOLBAR_HEIGHT-4);
	columns_place_heading_icons(window->columns, account_list_window_pane_def[tb_type]);

	error = xwimp_create_window(account_list_window_pane_def[tb_type], &(window->account_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		error_report_info("Toolbar");
		account_delete_window(window);
		return;
	}

	/* Create the footer pane. */

	windows_place_as_footer(account_list_window_def, account_foot_def, ACCOUNT_FOOTER_HEIGHT);
	columns_place_footer_icons(window->columns, account_foot_def, ACCOUNT_FOOTER_HEIGHT);

	account_foot_def->icons[ACCOUNT_FOOTER_STATEMENT].data.indirected_text.text = window->footer_icon[ACCOUNT_NUM_COLUMN_STATEMENT];
	account_foot_def->icons[ACCOUNT_FOOTER_CURRENT].data.indirected_text.text = window->footer_icon[ACCOUNT_NUM_COLUMN_CURRENT];
	account_foot_def->icons[ACCOUNT_FOOTER_FINAL].data.indirected_text.text = window->footer_icon[ACCOUNT_NUM_COLUMN_FINAL];
	account_foot_def->icons[ACCOUNT_FOOTER_BUDGET].data.indirected_text.text = window->footer_icon[ACCOUNT_NUM_COLUMN_BUDGET];

	error = xwimp_create_window(account_foot_def, &(window->account_footer));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		error_report_info("Footer bar");
		account_delete_window(window);
		return;
	}

	/* Set the title */

	account_build_window_title(file, entry);

	/* Open the window. */

	if (type == ACCOUNT_FULL) {
		ihelp_add_window(window->account_window , "AccList", account_decode_window_help);
		ihelp_add_window(window->account_pane , "AccListTB", NULL);
		ihelp_add_window(window->account_footer , "AccListFB", NULL);
	} else {
		ihelp_add_window(window->account_window , "HeadList", account_decode_window_help);
		ihelp_add_window(window->account_pane , "HeadListTB", NULL);
		ihelp_add_window(window->account_footer , "HeadListFB", NULL);
	}

	windows_open(window->account_window);
	windows_open_nested_as_toolbar(window->account_pane, window->account_window, ACCOUNT_TOOLBAR_HEIGHT-4);
	windows_open_nested_as_footer(window->account_footer, window->account_window, ACCOUNT_FOOTER_HEIGHT);

	/* Register event handlers for the two windows. */
	/* \TODO -- Should this be all three windows?   */

	event_add_window_user_data(window->account_window, window);
	event_add_window_menu(window->account_window, account_window_menu);
	event_add_window_close_event(window->account_window, account_close_window_handler);
	event_add_window_mouse_event(window->account_window, account_window_click_handler);
	event_add_window_scroll_event(window->account_window, account_window_scroll_handler);
	event_add_window_redraw_event(window->account_window, account_window_redraw_handler);
	event_add_window_menu_prepare(window->account_window, account_window_menu_prepare_handler);
	event_add_window_menu_selection(window->account_window, account_window_menu_selection_handler);
	event_add_window_menu_warning(window->account_window, account_window_menu_warning_handler);
	event_add_window_menu_close(window->account_window, account_window_menu_close_handler);

	event_add_window_user_data(window->account_pane, window);
	event_add_window_menu(window->account_pane, account_window_menu);
	event_add_window_mouse_event(window->account_pane, account_pane_click_handler);
	event_add_window_menu_prepare(window->account_pane, account_window_menu_prepare_handler);
	event_add_window_menu_selection(window->account_pane, account_window_menu_selection_handler);
	event_add_window_menu_warning(window->account_pane, account_window_menu_warning_handler);
	event_add_window_menu_close(window->account_pane, account_window_menu_close_handler);
}


/**
 * Close and delete an Accounts List Window associated with the given
 * account window block.
 *
 * \param *windat		The window to delete.
 */

static void account_delete_window(struct account_window *windat)
{
	if (windat == NULL)
		return;

	#ifdef DEBUG
	debug_printf ("\\RDeleting accounts window");
	#endif

	/* Close any dialogues which belong to this window. */

	account_account_dialogue_force_close(windat->instance);
	account_heading_dialogue_force_close(windat->instance);
	account_section_dialogue_force_close(windat);

	/* Delete the window, if it exists. */

	if (windat->account_window != NULL) {
		ihelp_remove_window(windat->account_window);
		event_delete_window(windat->account_window);
		wimp_delete_window(windat->account_window);
		windat->account_window = NULL;
	}

	if (windat->account_pane != NULL) {
		ihelp_remove_window(windat->account_pane);
		event_delete_window(windat->account_pane);
		wimp_delete_window(windat->account_pane);
		windat->account_pane = NULL;
	}

	if (windat->account_footer != NULL) {
		ihelp_remove_window(windat->account_footer);
		wimp_delete_window(windat->account_footer);
		windat->account_footer = NULL;
	}
}


/**
 * Handle Close events on Accounts List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void account_close_window_handler(wimp_close *close)
{
	struct account_window	*windat;

	#ifdef DEBUG
	debug_printf ("\\RClosing Accounts List window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat != NULL)
		account_delete_window(windat);
}


/**
 * Process mouse clicks in the Accounts List window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_window_click_handler(wimp_pointer *pointer)
{
	struct account_window	*windat;
	int			line;
	wimp_window_state	window;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL)
		return;

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = window_calculate_click_row(&(pointer->pos), &window, ACCOUNT_TOOLBAR_HEIGHT, windat->display_lines);

	/* Handle double-clicks, which will open a statement view or an edit accout window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1) {
		if (windat->line_data[line].type == ACCOUNT_LINE_DATA)
			accview_open_window(windat->instance->file, windat->line_data[line].account);
	} else if (pointer->buttons == wimp_DOUBLE_ADJUST && line != -1) {
		switch (windat->line_data[line].type) {
		case ACCOUNT_LINE_DATA:
			account_open_edit_window(windat->instance->file, windat->line_data[line].account, ACCOUNT_NULL, pointer);
			break;

		case ACCOUNT_LINE_HEADER:
		case ACCOUNT_LINE_FOOTER:
			account_open_section_edit_window(windat, line, pointer);
			break;
		default:
			break;
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && line != -1) {
		account_start_drag(windat, line);
	}
}


/**
 * Process mouse clicks in the Accounts List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_pane_click_handler(wimp_pointer *pointer)
{
	struct account_window		*windat;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL)
		return;

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case ACCOUNT_PANE_PARENT:
			transact_bring_window_to_top(windat->instance->file);
			break;

		case ACCOUNT_PANE_PRINT:
			account_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case ACCOUNT_PANE_ADDACCT:
			account_open_edit_window(windat->instance->file, NULL_ACCOUNT, windat->type, pointer);
			break;

		case ACCOUNT_PANE_ADDSECT:
			account_open_section_edit_window(windat, -1, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case ACCOUNT_PANE_PRINT:
			account_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimAccountCols"));
		column_start_drag(windat->columns, pointer, windat, windat->account_window, account_adjust_window_columns);
	}
}


/**
 * Process menu prepare events in the Accounts List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void account_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct account_window	*windat;
	int			line;
	wimp_window_state	window;
	enum account_line_type	data;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (pointer != NULL) {
		account_window_menu_line = -1;

		if (w == windat->account_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = window_calculate_click_row(&(pointer->pos), &window, ACCOUNT_TOOLBAR_HEIGHT, windat->display_lines);

			if (line != -1)
				account_window_menu_line = line;
		}

		data = (account_window_menu_line == -1) ? ACCOUNT_LINE_BLANK : windat->line_data[account_window_menu_line].type;

		saveas_initialise_dialogue(account_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(account_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);

		switch (windat->type) {
		case ACCOUNT_FULL:
			msgs_lookup("AcclistMenuTitleAcc", account_window_menu->title_data.text, 12);
			msgs_lookup("AcclistMenuViewAcc", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_VIEWACCT), 20);
			msgs_lookup("AcclistMenuEditAcc", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_EDITACCT), 20);
			msgs_lookup("AcclistMenuNewAcc", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_NEWACCT), 20);
			ihelp_add_menu(account_window_menu, "AccListMenu");
			break;

		case ACCOUNT_IN:
		case ACCOUNT_OUT:
			msgs_lookup("AcclistMenuTitleHead", account_window_menu->title_data.text, 12);
			msgs_lookup("AcclistMenuViewHead", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_VIEWACCT), 20);
			msgs_lookup("AcclistMenuEditHead", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_EDITACCT), 20);
			msgs_lookup("AcclistMenuNewHead", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_NEWACCT), 20);
			ihelp_add_menu(account_window_menu, "HeadListMenu");
			break;
		default:
			break;
		}
	} else {
		data = (account_window_menu_line == -1) ? ACCOUNT_LINE_BLANK : windat->line_data[account_window_menu_line].type;
	}

	menus_shade_entry(account_window_menu, ACCLIST_MENU_VIEWACCT, account_window_menu_line == -1 || data != ACCOUNT_LINE_DATA);
	menus_shade_entry(account_window_menu, ACCLIST_MENU_EDITACCT, account_window_menu_line == -1 || data != ACCOUNT_LINE_DATA);
	menus_shade_entry(account_window_menu, ACCLIST_MENU_EDITSECT, account_window_menu_line == -1 || (data != ACCOUNT_LINE_HEADER && data != ACCOUNT_LINE_FOOTER));
}


/**
 * Process menu selection events in the Accounts List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void account_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct account_window	*windat;
	wimp_pointer		pointer;
//	osbool			data;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->instance == NULL || windat->instance->file == NULL)
		return;

// \TODO -- Not sure what this is for, but the logic of the ? looks to be
// inverted and the result certainly isn't an osbool!

//	data = (account_window_menu_line != -1) ? ACCOUNT_LINE_BLANK : windat->line_data[account_window_menu_line].type;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case ACCLIST_MENU_VIEWACCT:
		accview_open_window(windat->instance->file, windat->line_data[account_window_menu_line].account);
		break;

	case ACCLIST_MENU_EDITACCT:
		account_open_edit_window(windat->instance->file, windat->line_data[account_window_menu_line].account, ACCOUNT_NULL, &pointer);
		break;

	case ACCLIST_MENU_EDITSECT:
		account_open_section_edit_window(windat, account_window_menu_line, &pointer);
		break;

	case ACCLIST_MENU_NEWACCT:
		account_open_edit_window(windat->instance->file, -1, windat->type, &pointer);
		break;

	case ACCLIST_MENU_NEWHEADER:
		account_open_section_edit_window(windat, -1, &pointer);
		break;

	case ACCLIST_MENU_PRINT:
		account_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
		break;
 	}
}


/**
 * Process submenu warning events in the Accounts List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void account_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct account_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	switch (warning->selection.items[0]) {
	case ACCLIST_MENU_EXPCSV:
		saveas_prepare_dialogue(account_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case ACCLIST_MENU_EXPTSV:
		saveas_prepare_dialogue(account_saveas_tsv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process menu close events in the Accounts List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void account_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	account_window_menu_line = -1;
	ihelp_remove_menu(account_window_menu);
}


/**
 * Process scroll events in the Accounts List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void account_window_scroll_handler(wimp_scroll *scroll)
{
	window_process_scroll_effect(scroll, ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT);

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

	wimp_open_window((wimp_open *) scroll);
}


/**
 * Process redraw events in the Account View window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void account_window_redraw_handler(wimp_draw *redraw)
{
	int			ox, oy, top, base, y, shade_overdrawn_col, icon_fg_col, width;
	char			icon_buffer[AMOUNT_FIELD_LEN];
	osbool			more, shade_overdrawn;
	struct account_window	*windat;
	struct account_block	*instance;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->columns == NULL)
		return;

	instance = windat->instance;

	shade_overdrawn = config_opt_read("ShadeAccounts");
	shade_overdrawn_col = config_int_read("ShadeAccountsColour");

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	/* Set the horizontal positions of the icons for the account lines. */

	columns_place_table_icons_horizontally(windat->columns, account_list_window_def, icon_buffer, AMOUNT_FIELD_LEN);
	width = column_get_window_width(windat->columns);

	window_set_icon_templates(account_list_window_def);

	/* Perform the redraw. */

	while (more) {
		/* Calculate the rows to redraw. */

		top = WINDOW_REDRAW_TOP(ACCOUNT_TOOLBAR_HEIGHT, oy - redraw->clip.y1);
		if (top < 0)
			top = 0;

		base = WINDOW_REDRAW_BASE(ACCOUNT_TOOLBAR_HEIGHT, oy - redraw->clip.y0);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			/* Plot out the background with a filled white rectangle. */

			wimp_set_colour(wimp_COLOUR_WHITE);
			os_plot(os_MOVE_TO, ox, oy + WINDOW_ROW_TOP(ACCOUNT_TOOLBAR_HEIGHT, y));
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO, ox + width, oy + WINDOW_ROW_BASE(ACCOUNT_TOOLBAR_HEIGHT, y));

			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(windat->columns, account_list_window_def,
					WINDOW_ROW_Y0(ACCOUNT_TOOLBAR_HEIGHT, y), WINDOW_ROW_Y1(ACCOUNT_TOOLBAR_HEIGHT, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= windat->display_lines) {
				columns_plot_empty_table_icons(windat->columns);
				continue;
			}

			switch (windat->line_data[y].type) {
			case ACCOUNT_LINE_DATA:
				window_plot_text_field(ACCOUNT_ICON_IDENT, instance->accounts[windat->line_data[y].account].ident, wimp_COLOUR_BLACK);
				window_plot_text_field(ACCOUNT_ICON_NAME, instance->accounts[windat->line_data[y].account].name, wimp_COLOUR_BLACK);

				switch (windat->type) {
				case ACCOUNT_FULL:
					if (shade_overdrawn &&
							(instance->accounts[windat->line_data[y].account].statement_balance <
							-instance->accounts[windat->line_data[y].account].credit_limit))
						icon_fg_col = shade_overdrawn_col;
					else
						icon_fg_col = wimp_COLOUR_BLACK;

					window_plot_currency_field(ACCOUNT_ICON_STATEMENT, instance->accounts[windat->line_data[y].account].statement_balance, icon_fg_col);

					if (shade_overdrawn &&
							(instance->accounts[windat->line_data[y].account].current_balance <
							-instance->accounts[windat->line_data[y].account].credit_limit))
						icon_fg_col = shade_overdrawn_col;
					else
						icon_fg_col = wimp_COLOUR_BLACK;

					window_plot_currency_field(ACCOUNT_ICON_CURRENT, instance->accounts[windat->line_data[y].account].current_balance, icon_fg_col);

					if (shade_overdrawn &&
							(instance->accounts[windat->line_data[y].account].trial_balance < 0))
						icon_fg_col = shade_overdrawn_col;
					else
						icon_fg_col = wimp_COLOUR_BLACK;

					window_plot_currency_field(ACCOUNT_ICON_FINAL, instance->accounts[windat->line_data[y].account].trial_balance, icon_fg_col);

					window_plot_currency_field(ACCOUNT_ICON_BUDGET, instance->accounts[windat->line_data[y].account].budget_balance, wimp_COLOUR_BLACK);
					break;

				case ACCOUNT_IN:
					if (shade_overdrawn &&
							(-instance->accounts[windat->line_data[y].account].budget_balance <
							instance->accounts[windat->line_data[y].account].budget_amount))
						icon_fg_col = shade_overdrawn_col;
					else
						icon_fg_col = wimp_COLOUR_BLACK;

					window_plot_currency_field(ACCOUNT_ICON_STATEMENT, -instance->accounts[windat->line_data[y].account].future_balance, wimp_COLOUR_BLACK);
					window_plot_currency_field(ACCOUNT_ICON_CURRENT, instance->accounts[windat->line_data[y].account].budget_amount, wimp_COLOUR_BLACK);
					window_plot_currency_field(ACCOUNT_ICON_FINAL, -instance->accounts[windat->line_data[y].account].budget_balance, icon_fg_col);
					window_plot_currency_field(ACCOUNT_ICON_BUDGET, instance->accounts[windat->line_data[y].account].budget_result, icon_fg_col);
					break;

				case ACCOUNT_OUT:
					if (shade_overdrawn &&
							(instance->accounts[windat->line_data[y].account].budget_balance >
							instance->accounts[windat->line_data[y].account].budget_amount))
						icon_fg_col = shade_overdrawn_col;
					else
						icon_fg_col = wimp_COLOUR_BLACK;

					window_plot_currency_field(ACCOUNT_ICON_STATEMENT, instance->accounts[windat->line_data[y].account].future_balance, wimp_COLOUR_BLACK);
					window_plot_currency_field(ACCOUNT_ICON_CURRENT, instance->accounts[windat->line_data[y].account].budget_amount, wimp_COLOUR_BLACK);
					window_plot_currency_field(ACCOUNT_ICON_FINAL, instance->accounts[windat->line_data[y].account].budget_balance, icon_fg_col);
					window_plot_currency_field(ACCOUNT_ICON_BUDGET, instance->accounts[windat->line_data[y].account].budget_result, icon_fg_col);
					break;

				default:
					break;
				}
				break;

			case ACCOUNT_LINE_HEADER:

				/* Block header line */

				window_plot_text_field(ACCOUNT_ICON_HEADING, windat->line_data[y].heading, wimp_COLOUR_WHITE);
				break;

			case ACCOUNT_LINE_FOOTER:

				/* Block footer line */

				window_plot_text_field(ACCOUNT_ICON_FOOT_NAME, windat->line_data[y].heading, wimp_COLOUR_BLACK);

				window_plot_currency_field(ACCOUNT_ICON_FOOT_STATEMENT, windat->line_data[y].total[0], wimp_COLOUR_BLACK);
				window_plot_currency_field(ACCOUNT_ICON_FOOT_CURRENT, windat->line_data[y].total[1], wimp_COLOUR_BLACK);
				window_plot_currency_field(ACCOUNT_ICON_FOOT_FINAL, windat->line_data[y].total[2], wimp_COLOUR_BLACK);
				window_plot_currency_field(ACCOUNT_ICON_FOOT_BUDGET, windat->line_data[y].total[3], wimp_COLOUR_BLACK);
				break;

			case ACCOUNT_LINE_BLANK:
				break;
			}
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

static void account_adjust_window_columns(void *data, wimp_i icon, int width)
{
	struct account_window	*windat = (struct account_window *) data;
	int			new_extent;
	wimp_window_info	window;

	if (windat == NULL || windat->instance == NULL || windat->instance->file == NULL)
		return;

	columns_update_dragged(windat->columns, windat->account_pane, windat->account_footer, icon, width);

	new_extent = column_get_window_width(windat->columns);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(windat->account_window);
	windows_redraw(windat->account_pane);
	windows_redraw(windat->account_footer);

	/* Set the horizontal extent of the window and pane. */

	window.w = windat->account_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = windat->account_footer;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = windat->account_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(windat->instance->file, TRUE);
}


/**
 * Set the extent of an account window for the specified file.
 *
 * \param *windat		The window to be updated.
 */

static void account_set_window_extent(struct account_window *windat)
{
	int	lines;

	/* Set the extent. */

	if (windat == NULL || windat->account_window == NULL)
		return;

	lines = (windat->display_lines > MIN_ACCOUNT_ENTRIES) ? windat->display_lines : MIN_ACCOUNT_ENTRIES;

	window_set_extent(windat->account_window, lines, ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT + 2,
			column_get_window_width(windat->columns));
}

/**
 * Recreate the title of the specified Account window connected to the
 * given file.
 *
 * \param *file			The file to rebuild the title for.
 * \param entry			The entry of the window to to be updated.
 */

static void account_build_window_title(struct file_block *file, int entry)
{
	char	name[WINDOW_TITLE_LENGTH];

	if (file == NULL || file->accounts == NULL || file->accounts->account_windows[entry].account_window == NULL)
		return;

	file_get_leafname(file, name, WINDOW_TITLE_LENGTH);

	switch (file->accounts->account_windows[entry].type) {
	case ACCOUNT_FULL:
		msgs_param_lookup("AcclistTitleAcc", file->accounts->account_windows[entry].window_title, WINDOW_TITLE_LENGTH,
				name, NULL, NULL, NULL);
		break;

	case ACCOUNT_IN:
		msgs_param_lookup("AcclistTitleHIn", file->accounts->account_windows[entry].window_title, WINDOW_TITLE_LENGTH,
				name, NULL, NULL, NULL);
		break;

	case ACCOUNT_OUT:
		msgs_param_lookup("AcclistTitleHOut", file->accounts->account_windows[entry].window_title, WINDOW_TITLE_LENGTH,
				name, NULL, NULL, NULL);
		break;

	default:
		break;
	}

	wimp_force_redraw_title(file->accounts->account_windows[entry].account_window);
}


/**
 * Force a redraw of the Account List window, for the given range of
 * lines.
 *
 * \param *file			The file owning the window.
 * \param entry			The account list window to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void account_force_window_redraw(struct file_block *file, int entry, int from, int to, wimp_i column)
{
	wimp_window_info	window;

	if (file == NULL || file->accounts == NULL || file->accounts->account_windows[entry].account_window == NULL)
		return;

	window.w = file->accounts->account_windows[entry].account_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(file->accounts->account_windows[entry].columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(ACCOUNT_TOOLBAR_HEIGHT, from);
	window.extent.y0 = WINDOW_ROW_BASE(ACCOUNT_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(file->accounts->account_windows[entry].account_window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);

	/* Force a redraw of the three total icons in the footer. */

	icons_redraw_group(file->accounts->account_windows[entry].account_footer, 4, 1, 2, 3, 4);
}


/**
 * Turn a mouse position over an Account List window into an interactive
 * help token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void account_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int				xpos;
	wimp_i				icon;
	wimp_window_state		window;
	struct account_window		*windat;

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

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, account_list_window_def->icons[icon].data.indirected_text.validation, 'N')) {
		snprintf(buffer, IHELP_INAME_LEN, "Col%d", icon);
		buffer[IHELP_INAME_LEN - 1] = '\0';
	}
}


/**
 * Open the Account Edit dialogue for a given account list window.
 *
 * If account == NULL_ACCOUNT, type determines the type of the new account
 * to be created.  Otherwise, type is ignored and the type derived from the
 * account data block.
 *
 * \param *file			The file to own the dialogue.
 * \param account		The account to edit, or NULL_ACCOUNT for add new.
 * \param type			The type of new account to create if account
 *				is NULL_ACCOUNT.
 * \param *ptr			The current Wimp pointer position.
 */

void account_open_edit_window(struct file_block *file, acct_t account, enum account_type type, wimp_pointer *ptr)
{
	struct account	*data;
	rate_t		rate;

	if (file == NULL || file->accounts == NULL)
		return;

	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	account_account_dialogue_force_close(NULL);
	account_heading_dialogue_force_close(NULL);
	account_section_dialogue_force_close(NULL);

	/* Select the window to use and set the contents up. */

	if (account == NULL_ACCOUNT) {
		if (type & ACCOUNT_FULL) {
			account_account_dialogue_open(ptr, file->accounts, NULL_ACCOUNT, account_process_account_edit_window, account_delete_from_edit_window,
					"", "", NULL_CURRENCY, NULL_CURRENCY, NULL, NULL, NULL_RATE, NULL_ACCOUNT, "", "", NULL);
		} else if (type & ACCOUNT_IN || type & ACCOUNT_OUT) {
			account_heading_dialogue_open(ptr, file->accounts, NULL_ACCOUNT, account_process_heading_edit_window, account_delete_from_edit_window,
					"", "", NULL_CURRENCY, type);
		}
	} else if (account_valid(file->accounts, account)) {
		data = &(file->accounts->accounts[account]);

		if (data->type & ACCOUNT_FULL) {
			rate = interest_get_current_rate(file->interest, account, date_today());
			account_account_dialogue_open(ptr, file->accounts, account, account_process_account_edit_window, account_delete_from_edit_window,
					data->name, data->ident, data->credit_limit, data->opening_balance,
					&(data->cheque_number), &(data->payin_number), rate, data->offset_against,
					data->account_no, data->sort_code, data->address);
		} else if (data->type & ACCOUNT_IN || data->type & ACCOUNT_OUT) {
			account_heading_dialogue_open(ptr, file->accounts, account, account_process_heading_edit_window, account_delete_from_edit_window,
					data->name, data->ident, data->budget_amount, data->type);
		}
	}
}


/**
 * Take the contents of an updated Account Edit window and process the data.
 *
 * \param *instance		The accounts instance to which the account belongs.
 * \param account		The account number of the account, or NULL_ACCOUNT
 *				for a new one.
 * \param *name			The name of the account.
 * \param *ident		The ident of the account.
 * \param credit_limit		The credit limit for the aaccount.
 * \param opening_balance	The opening balance for the account.
 * \param *cheque_number	The cheque number details for the account.
 * \param *payin_number		The paying in slip number details for the account.
 * \param offset_against	The account to offset this account against.
 * \param *account_num		The account number of the account.
 * \param *sort_code		The sort code of this account.
 * \param **address		The address details of this account.
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_process_account_edit_window(struct account_block *instance, acct_t account, char* name, char *ident,
		amt_t credit_limit, amt_t opening_balance, struct account_idnum *cheque_number, struct account_idnum *payin_number,
		acct_t offset_against, char *account_num, char *sort_code, char address[][ACCOUNT_ADDR_LEN])
{
	int	i;
	acct_t	check_ident;

	/* Check that the ident is valid and unused. As a full account,
	 * we need to check all full accounts and also all headers.
	 */

	check_ident = account_find_by_ident(instance->file, ident, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT);

	if ((check_ident != NULL_ACCOUNT) && (check_ident != account)) {
		error_msgs_report_error("UsedAcctIdent");
		return FALSE;
	}

	/* If the account doesn't exist, create it. */

	if (account == NULL_ACCOUNT) {
		account = account_add(instance->file, name, ident, ACCOUNT_FULL);

		if (account == NULL_ACCOUNT)
			return FALSE;
	} else {
		string_copy(instance->accounts[account].name, name, ACCOUNT_NAME_LEN);
		string_copy(instance->accounts[account].ident, ident, ACCOUNT_IDENT_LEN);
	}

	/* Store the remaining data. */

	instance->accounts[account].credit_limit = credit_limit;
	instance->accounts[account].opening_balance = opening_balance;
	instance->accounts[account].opening_balance = opening_balance;

	account_idnum_copy(&(instance->accounts[account].cheque_number), cheque_number);
	account_idnum_copy(&(instance->accounts[account].payin_number), payin_number);

	string_copy(instance->accounts[account].account_no, account_num, ACCOUNT_NO_LEN);
	string_copy(instance->accounts[account].sort_code, sort_code, ACCOUNT_SRTCD_LEN);

	for (i = 0; i < ACCOUNT_ADDR_LINES; i++)
		string_copy(instance->accounts[account].address[i], address[i], ACCOUNT_ADDR_LEN);

	/* Tidy up and redraw the windows */

	sorder_trial(instance->file);
	account_recalculate_all(instance->file);
	accview_recalculate(instance->file, account, 0);
	transact_redraw_all(instance->file);
	accview_redraw_all(instance->file);
	file_set_data_integrity(instance->file, TRUE);

	return TRUE;
}


/**
 * Take the contents of an updated Heading Edit window and process the data.
 *
 * \param *instance		The accounts instance to which the heading belongs.
 * \param account		The account number of the heading, or NULL_ACCOUNT
 *				for a new one.
 * \param *name			The name of the heading.
 * \param *ident		The ident of the heading.
 * \param budget		The budget amount for the heading.
 * \param type			The incoming/outgoing type of the heading.
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_process_heading_edit_window(struct account_block *instance, acct_t account, char *name, char *ident, amt_t budget, enum account_type type)
{
	acct_t check_ident;

	/* Check that the ident is valid and unused. As a header, we need
	 * to check all full accounts and also any headers in the same
	 * category as this one.
	 */

	check_ident = account_find_by_ident(instance->file, ident, ACCOUNT_FULL | type);

	if ((check_ident != NULL_ACCOUNT) && (check_ident != account)) {
		error_msgs_report_error("UsedAcctIdent");
		return FALSE;
	}

	/* If the heading doesn't exist, create it. */

	if (account == NULL_ACCOUNT) {
		account = account_add(instance->file, name, ident, type);

		if (account == NULL_ACCOUNT)
			return FALSE;
	} else {
		string_copy(instance->accounts[account].name, name, ACCOUNT_NAME_LEN);
		string_copy(instance->accounts[account].ident, ident, ACCOUNT_IDENT_LEN);
	}

	/* Store the remaining data. */

	instance->accounts[account].budget_amount = budget;

	/* Tidy up and redraw the windows */

	account_recalculate_all(instance->file);
	transact_redraw_all(instance->file);
	accview_redraw_all(instance->file);
	file_set_data_integrity(instance->file, TRUE);

	return TRUE;
}


/**
 * Delete the account associated with the currently open Account or
 * Heading Edit window.
 *
 * \param *instance		The accounts instance owning the account.
 * \param account		The account to be deleted.
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_delete_from_edit_window(struct account_block *instance, acct_t account)
{
	if (instance == NULL || instance->file == NULL || account == NULL_ACCOUNT)
		return FALSE;

	/* Check that the account isn't in use. */

	if (account_used_in_file(instance, account)) {
		error_msgs_report_info("CantDelAcct");
		return FALSE;
	}

	return account_delete(instance->file, account);
}


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *window		The account window to own the dialogue.
 * \param line			The line to be editied, or -1 for none.
 * \param *ptr			The current Wimp pointer position.
 */

static void account_open_section_edit_window(struct account_window *window, int line, wimp_pointer *ptr)
{
	if (window == NULL)
		return;

	/* Close any other edit dialogues relating to this account list window. */

	account_account_dialogue_force_close(NULL);
	account_heading_dialogue_force_close(NULL);
	account_section_dialogue_force_close(NULL);

	/* Open the dialogue box. */

	account_section_dialogue_open(ptr, window, line, account_process_section_edit_window, account_delete_from_section_edit_window,
			(line == -1) ? "" : window->line_data[line].heading,
			(line == -1) ? ACCOUNT_LINE_HEADER : window->line_data[line].type);
}


/**
 * Process data associated with the currently open Section Edit window.
 *
 * \param *window		The Account List window holding the section.
 * \param line			The selected section to be updated, or -1.
 * \param *name			The new name for the section.
 * \param type			The new type of section.
 * \return			TRUE if processed; else FALSE.
 */

static osbool account_process_section_edit_window(struct account_window *window, int line, char* name, enum account_line_type type)
{
	if (window == NULL)
		return FALSE;

	/* If the section doesn't exsit, create space for it. */

	if (line == -1) {
		line = account_add_list_display_line(window->instance->file, window->entry);

		if (line == -1) {
			error_msgs_report_error("NoMemNewSect");
			return FALSE;
		}
	}

	/* Update the line details. */

	string_copy(window->line_data[line].heading, name, ACCOUNT_SECTION_LEN);
	window->line_data[line].type = type;

	/* Tidy up and redraw the windows */

	account_recalculate_all(window->instance->file);
	account_set_window_extent(window);
	windows_open(window->account_window);
	account_force_window_redraw(window->instance->file, window->entry, line, line, wimp_ICON_WINDOW);
	file_set_data_integrity(window->instance->file, TRUE);

	return TRUE;
}


/**
 * Delete the section associated with the currently open Section Edit
 * window.
 *
 * \param *window		The Account List window holding the section.
 * \param line			The selected line to be deleted.
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_delete_from_section_edit_window(struct account_window *window, int line)
{
	if (window == NULL || line == -1)
		return FALSE;

	/* Delete the heading */

	if (!flexutils_delete_object((void **) &(window->line_data), sizeof(struct account_redraw), line)) {
		error_msgs_report_error("BadDelete");
		return FALSE;
	}

	window->display_lines--;

	/* Update the accounts display window. */

	account_set_window_extent(window);
	windows_open(window->account_window);
	account_force_window_redraw(window->instance->file, window->entry, line, window->display_lines, wimp_ICON_WINDOW);
	file_set_data_integrity(window->instance->file, TRUE);

	return TRUE;
}


/**
 * Open the Account Print dialogue for a given account list window.
 *
 * \param *window		The account list window to be printed.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to retain the previous settings; FALSE to
 *				return to defaults.
 */

static void account_open_print_window(struct account_window *window, wimp_pointer *ptr, osbool restore)
{
	if (window == NULL || window->instance == NULL || window->instance->file == NULL)
		return;

	/* Open the print dialogue box. */

	if (window->type & ACCOUNT_FULL) {
		print_dialogue_open_simple(window->instance->file->print, ptr, restore,
				"PrintAcclistAcc", "PrintTitleAcclistAcc",
				account_print, window);
	} else if (window->type & ACCOUNT_IN || window->type & ACCOUNT_OUT) {
		print_dialogue_open_simple(window->instance->file->print, ptr, restore,
				"PrintAcclistHead", "PrintTitleAcclistHead",
				account_print, window);
	}
}


/**
 * Send the contents of the Account Window to the printer, via the reporting
 * system.
 *
 * \param *report		The report handle to use for output.
 * \param *data			The account window structure to be printed.
 * \return			Pointer to the report, or NULL on failure.
 */

static struct report *account_print(struct report *report, void *data)
{
	struct account_window	*window = data;
	struct account_block	*instance;
	int			line;
	char			*filename, date_buffer[DATE_FIELD_LEN];
	date_t			start, finish;

	if (report == NULL || window == NULL)
		return NULL;

	instance = window->instance;
	if (instance == NULL || instance->file == NULL)
		return NULL;

	hourglass_on();

	/* Output the page title. */

	stringbuild_reset();
	stringbuild_add_string("\\b\\u");

	filename = file_get_leafname(window->instance->file, NULL, 0);

	switch (window->type) {
	case ACCOUNT_FULL:
		stringbuild_add_message_param("AcclistTitleAcc", filename, NULL, NULL, NULL);
		break;

	case ACCOUNT_IN:
		stringbuild_add_message_param("AcclistTitleHIn", filename, NULL, NULL, NULL);
		break;

	case ACCOUNT_OUT:
		stringbuild_add_message_param("AcclistTitleHOut", filename, NULL, NULL, NULL);
		break;

	default:
		break;
	}

	stringbuild_report_line(report, 1);

	/* Output budget title. */

	budget_get_dates(instance->file, &start, &finish);

	if (start != NULL_DATE || finish != NULL_DATE) {
		stringbuild_reset();

		stringbuild_add_message("AcclistBudgetTitle");

		if (start != NULL_DATE) {
			date_convert_to_string(start, date_buffer, DATE_FIELD_LEN);
			stringbuild_add_message_param("AcclistBudgetFrom", date_buffer, NULL, NULL, NULL);
		}

		if (finish != NULL_DATE) {
			date_convert_to_string(finish, date_buffer, DATE_FIELD_LEN);
			stringbuild_add_message_param("AcclistBudgetTo", date_buffer, NULL, NULL, NULL);
		}

		stringbuild_add_string(".");

		stringbuild_report_line(report, 1);
	}

	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	stringbuild_reset();

	stringbuild_add_string("\\k\\b\\u");
	stringbuild_add_icon(window->account_pane, ACCOUNT_PANE_NAME);
	stringbuild_add_string("\\t\\s\\t\\b\\u\\r");
	stringbuild_add_icon(window->account_pane, ACCOUNT_PANE_STATEMENT);
	stringbuild_add_string("\\t\\b\\u\\r");
	stringbuild_add_icon(window->account_pane, ACCOUNT_PANE_CURRENT);
	stringbuild_add_string("\\t\\b\\u\\r");
	stringbuild_add_icon(window->account_pane, ACCOUNT_PANE_FINAL);
	stringbuild_add_string("\\t\\b\\u\\r");
	stringbuild_add_icon(window->account_pane, ACCOUNT_PANE_BUDGET);

	stringbuild_report_line(report, 0);

	/* Output the account data as a set of delimited lines. */
	/* Output the transaction data as a set of delimited lines. */

	for (line = 0; line < window->display_lines; line++) {
		stringbuild_reset();

		if (window->line_data[line].type == ACCOUNT_LINE_DATA) {
			stringbuild_add_printf("\\k%s\\t%s\\t\\r",
					account_get_ident(instance->file, window->line_data[line].account),
					account_get_name(instance->file, window->line_data[line].account));

			switch (window->type) {
			case ACCOUNT_FULL:
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].statement_balance, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].current_balance, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].trial_balance, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].budget_balance, FALSE);
				break;

			case ACCOUNT_IN:
				stringbuild_add_currency(-instance->accounts[window->line_data[line].account].future_balance, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].budget_amount, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(-instance->accounts[window->line_data[line].account].budget_balance, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].budget_result, FALSE);
				break;

			case ACCOUNT_OUT:
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].future_balance, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].budget_amount, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].budget_balance, FALSE);
				stringbuild_add_string("\\t\\r");
				stringbuild_add_currency(instance->accounts[window->line_data[line].account].budget_result, FALSE);
				break;

			default:
				break;
			}
		} else if (window->line_data[line].type == ACCOUNT_LINE_HEADER) {
			stringbuild_add_printf("\\k\\u%s", window->line_data[line].heading);
		} else if (window->line_data[line].type == ACCOUNT_LINE_FOOTER) {
			stringbuild_add_printf("\\k%s\\t\\s\\t\\r\\b", window->line_data[line].heading);
			stringbuild_add_currency(window->line_data[line].total[ACCOUNT_NUM_COLUMN_STATEMENT], FALSE);
			stringbuild_add_string("\\t\\r\\b");
			stringbuild_add_currency(window->line_data[line].total[ACCOUNT_NUM_COLUMN_CURRENT], FALSE);
			stringbuild_add_string("\\t\\r\\b");
			stringbuild_add_currency(window->line_data[line].total[ACCOUNT_NUM_COLUMN_FINAL], FALSE);
			stringbuild_add_string("\\t\\r\\b");
			stringbuild_add_currency(window->line_data[line].total[ACCOUNT_NUM_COLUMN_BUDGET], FALSE);
		}

		stringbuild_report_line(report, 0);
	}

	/* Output the grand total line, taking the text from the window icons. */

	stringbuild_reset();

	stringbuild_add_string("\\k\\u");
	stringbuild_add_icon(window->account_footer, ACCOUNT_FOOTER_NAME);
	stringbuild_add_printf("\\t\\s\\t\\r%s\\t\\r%s\\t\\r%s\\t\\r%s",
			window->footer_icon[ACCOUNT_NUM_COLUMN_STATEMENT],
			window->footer_icon[ACCOUNT_NUM_COLUMN_CURRENT],
			window->footer_icon[ACCOUNT_NUM_COLUMN_FINAL],
			window->footer_icon[ACCOUNT_NUM_COLUMN_BUDGET]);

	stringbuild_report_line(report, 0);

	hourglass_off();

	return report;
}


/**
 * Create a new display line block at the end of the given list, fill it with
 * blank data and return the number.
 *
 * \param *file			The file containing the list.
 * \param entry			The list to add an entry to.
 * \return			The new line, or -1 on failure.
 */

static int account_add_list_display_line(struct file_block *file, int entry)
{
	int	line;

	if (file == NULL || file->accounts == NULL)
		return -1;

	if (!flexutils_resize((void **) &(file->accounts->account_windows[entry].line_data), sizeof(struct account_redraw), file->accounts->account_windows[entry].display_lines + 1))
		return -1;

	line = file->accounts->account_windows[entry].display_lines++;

	#ifdef DEBUG
	debug_printf("Creating new display line %d", line);
	#endif

	file->accounts->account_windows[entry].line_data[line].type = ACCOUNT_LINE_BLANK;
	file->accounts->account_windows[entry].line_data[line].account = NULL_ACCOUNT;

	*file->accounts->account_windows[entry].line_data[line].heading = '\0';

	return line;
}
















/**
 * Find the number of entries in the account window of a given account type.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \return			The number of entries, or 0.
 */

int account_get_list_length(struct file_block *file, enum account_type type)
{
	int	entry;

	if (file == NULL || file->accounts == NULL)
		return 0;

	entry = account_find_window_entry_from_type(file, type);
	if (entry == -1)
		return 0;

	return file->accounts->account_windows[entry].display_lines;
}


/**
 * Return the  type of a given line of an account list window.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \param line			The line to return the details for.
 * \return			The type of data on that line.
 */

enum account_line_type account_get_list_entry_type(struct file_block *file, enum account_type type, int line)
{
	int	entry;

	if (file == NULL || file->accounts == NULL || line < 0)
		return ACCOUNT_LINE_BLANK;

	entry = account_find_window_entry_from_type(file, type);
	if (entry == -1 || line >= file->accounts->account_windows[entry].display_lines)
		return ACCOUNT_LINE_BLANK;

	return file->accounts->account_windows[entry].line_data[line].type;
}


/**
 * Return the account on a given line of an account list window.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \param line			The line to return the details for.
 * \return			The account on that line, or NULL_ACCOUNT if the
 *				line isn't an account.
 */

acct_t account_get_list_entry_account(struct file_block *file, enum account_type type, int line)
{
	int	entry;

	if (file == NULL || file->accounts == NULL || line < 0)
		return NULL_ACCOUNT;

	entry = account_find_window_entry_from_type(file, type);
	if (entry == -1 || line >= file->accounts->account_windows[entry].display_lines)
		return NULL_ACCOUNT;

	if (file->accounts->account_windows[entry].line_data[line].type != ACCOUNT_LINE_DATA)
		return NULL_ACCOUNT;

	return file->accounts->account_windows[entry].line_data[line].account;
}


/**
 * Return the text on a given line of an account list window.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \param line			The line to return the details for.
 * \return			A volatile pointer to the text on the line,
 *				or NULL.
 */

char *account_get_list_entry_text(struct file_block *file, enum account_type type, int line)
{
	int	entry;

	if (file == NULL || file->accounts == NULL || line < 0)
		return NULL;

	entry = account_find_window_entry_from_type(file, type);
	if (entry == -1 || line >= file->accounts->account_windows[entry].display_lines)
		return NULL;

	if (file->accounts->account_windows[entry].line_data[line].type == ACCOUNT_LINE_DATA)
		return account_get_name(file, file->accounts->account_windows[entry].line_data[line].account);

	return file->accounts->account_windows[entry].line_data[line].heading;
}





/**
 * Start an account window drag, to re-order the entries in the window.
 *
 * \param *file			The file owning the Account List being dragged.
 * \param entry			The entry of the Account List being dragged.
 * \param line			The line of the Account list being dragged.
 */

static void account_start_drag(struct account_window *windat, int line)
{
	wimp_window_state	window;
	wimp_auto_scroll_info	auto_scroll;
	wimp_drag		drag;
	int			ox, oy;

	if (windat == NULL)
		return;

	/* The drag is not started if any of the account window edit dialogues
	 * are open, as these will have pointers into the data which won't like
	 * the data moving beneath them.
	 */

	if (account_account_dialogue_is_open(windat->instance) || account_heading_dialogue_is_open(windat->instance) || account_section_dialogue_is_open(windat))
		return;

	/* Get the basic information about the window. */

	window.w = windat->account_window;
	wimp_get_window_state(&window);

	ox = window.visible.x0 - window.xscroll;
	oy = window.visible.y1 - window.yscroll;

	/* Set up the drag parameters. */

	drag.w = windat->account_window;
	drag.type = wimp_DRAG_USER_FIXED;

	drag.initial.x0 = ox;
	drag.initial.y0 = oy + WINDOW_ROW_Y0(ACCOUNT_TOOLBAR_HEIGHT, line);
	drag.initial.x1 = ox + (window.visible.x1 - window.visible.x0);
	drag.initial.y1 = oy + WINDOW_ROW_Y1(ACCOUNT_TOOLBAR_HEIGHT, line);

	drag.bbox.x0 = window.visible.x0;
	drag.bbox.y0 = window.visible.y0;
	drag.bbox.x1 = window.visible.x1;
	drag.bbox.y1 = window.visible.y1;

	/* Read CMOS RAM to see if solid drags are required.
	 *
	 * \TODO -- Solid drags are never actually used, although they could be
	 *          if a suitable sprite were to be created.
	 */

	account_dragging_sprite = ((osbyte2(osbyte_READ_CMOS, osbyte_CONFIGURE_DRAG_ASPRITE, 0) &
                       osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0);

	if (FALSE && account_dragging_sprite) {
		dragasprite_start(dragasprite_HPOS_CENTRE | dragasprite_VPOS_CENTRE | dragasprite_NO_BOUND |
				dragasprite_BOUND_POINTER | dragasprite_DROP_SHADOW, wimpspriteop_AREA,
				"", &(drag.initial), &(drag.bbox));
	} else {
		wimp_drag_box(&drag);
	}

	/* Initialise the autoscroll. */

	if (xos_swi_number_from_string("Wimp_AutoScroll", NULL) == NULL) {
		auto_scroll.w = windat->account_window;
		auto_scroll.pause_zone_sizes.x0 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.y0 = AUTO_SCROLL_MARGIN + ACCOUNT_FOOTER_HEIGHT;
		auto_scroll.pause_zone_sizes.x1 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.y1 = AUTO_SCROLL_MARGIN + ACCOUNT_TOOLBAR_HEIGHT;
		auto_scroll.pause_duration = 0;
		auto_scroll.state_change = (void *) 1;

		wimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_HORIZONTAL | wimp_AUTO_SCROLL_ENABLE_VERTICAL, &auto_scroll);
	}

	account_dragging_owner = windat;
	account_dragging_start_line = line;

	event_set_drag_handler(account_terminate_drag, NULL, NULL);
}


/**
 * Handle drag-end events relating to column dragging.
 *
 * \param *drag			The Wimp drag end data.
 * \param *data			Unused client data sent via Event Lib.
 */

static void account_terminate_drag(wimp_dragged *drag, void *data)
{
	wimp_pointer			pointer;
	wimp_window_state		window;
	int				line;
	struct account_redraw		block;

	if (account_dragging_owner == NULL)
		return;

	/* Terminate the drag and end the autoscroll. */

	if (xos_swi_number_from_string("Wimp_AutoScroll", NULL) == NULL)
		wimp_auto_scroll(0, NULL);

	if (account_dragging_sprite)
		dragasprite_stop();

	/* Get the line at which the drag ended. */

	wimp_get_pointer_info(&pointer);

	window.w = account_dragging_owner->account_window;
	wimp_get_window_state(&window);

	line = ((window.visible.y1 - pointer.pos.y) - window.yscroll - ACCOUNT_TOOLBAR_HEIGHT) / WINDOW_ROW_HEIGHT;

	if (line < 0)
		line = 0;

	if (line >= account_dragging_owner->display_lines)
		line = account_dragging_owner->display_lines - 1;

	/* Move the blocks around. */

	block =  account_dragging_owner->line_data[account_dragging_start_line];

	if (line < account_dragging_start_line) {
		memmove(&(account_dragging_owner->line_data[line + 1]), &(account_dragging_owner->line_data[line]),
				(account_dragging_start_line - line) * sizeof(struct account_redraw));

		account_dragging_owner->line_data[line] = block;
	} else if (line > account_dragging_start_line) {
		memmove(&(account_dragging_owner->line_data[account_dragging_start_line]), &(account_dragging_owner->line_data[account_dragging_start_line + 1]),
				(line - account_dragging_start_line) * sizeof(struct account_redraw));

		account_dragging_owner->line_data[line] = block;
	}

	/* Tidy up and redraw the windows */

	account_recalculate_all(account_dragging_owner->instance->file);
	file_set_data_integrity(account_dragging_owner->instance->file, TRUE);
	account_force_window_redraw(account_dragging_owner->instance->file, account_dragging_owner->entry, 0, account_dragging_owner->display_lines - 1, wimp_ICON_WINDOW);

	#ifdef DEBUG
	debug_printf("Move account from line %d to line %d", account_dragging_start_line, line);
	#endif
}


/**
 * Recalculate the data in the account list windows (totals, sub-totals,
 * budget totals, etc) and refresh the display.
 *
 * \param *file		The file to recalculate.
 */

static void account_recalculate_windows(struct file_block *file)
{
	int	entry, i, j, sub_total[ACCOUNT_NUM_COLUMNS], total[ACCOUNT_NUM_COLUMNS];

	if (file == NULL || file->accounts == NULL)
		return;

	/* Calculate the full accounts details. */

	entry = account_find_window_entry_from_type(file, ACCOUNT_FULL);

	for (i = 0; i < ACCOUNT_NUM_COLUMNS; i++) {
		sub_total[i] = 0;
		total[i] = 0;
	}

	for (i = 0; i < file->accounts->account_windows[entry].display_lines; i++) {
		switch (file->accounts->account_windows[entry].line_data[i].type) {
		case ACCOUNT_LINE_DATA:
			sub_total[ACCOUNT_NUM_COLUMN_STATEMENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].statement_balance;
			sub_total[ACCOUNT_NUM_COLUMN_CURRENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].current_balance;
			sub_total[ACCOUNT_NUM_COLUMN_FINAL] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].trial_balance;
			sub_total[ACCOUNT_NUM_COLUMN_BUDGET] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;

			total[ACCOUNT_NUM_COLUMN_STATEMENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].statement_balance;
			total[ACCOUNT_NUM_COLUMN_CURRENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].current_balance;
			total[ACCOUNT_NUM_COLUMN_FINAL] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].trial_balance;
			total[ACCOUNT_NUM_COLUMN_BUDGET] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;
			break;

		case ACCOUNT_LINE_HEADER:
			for (j = 0; j < ACCOUNT_NUM_COLUMNS; j++)
				sub_total[j] = 0;
			break;

		case ACCOUNT_LINE_FOOTER:
			for (j = 0; j < ACCOUNT_NUM_COLUMNS; j++)
				file->accounts->account_windows[entry].line_data[i].total[j] = sub_total[j];
			break;

		default:
			break;
		}
	}

	for (i = 0; i < ACCOUNT_NUM_COLUMNS; i++)
		currency_convert_to_string(total[i], file->accounts->account_windows[entry].footer_icon[i], AMOUNT_FIELD_LEN);

	/* Calculate the incoming account details. */

	entry = account_find_window_entry_from_type(file, ACCOUNT_IN);

	for (i = 0; i < ACCOUNT_NUM_COLUMNS; i++) {
		sub_total[i] = 0;
		total[i] = 0;
	}

	for (i = 0; i < file->accounts->account_windows[entry].display_lines; i++) {
		switch (file->accounts->account_windows[entry].line_data[i].type) {
		case ACCOUNT_LINE_DATA:
			if (file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount != NULL_CURRENCY) {
				file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result =
						-file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount -
						file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;
			} else {
				file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result = NULL_CURRENCY;
			}

			sub_total[ACCOUNT_NUM_COLUMN_STATEMENT] -= file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].future_balance;
			sub_total[ACCOUNT_NUM_COLUMN_CURRENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount;
			sub_total[ACCOUNT_NUM_COLUMN_FINAL] -= file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;
			sub_total[ACCOUNT_NUM_COLUMN_BUDGET] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result;

			total[ACCOUNT_NUM_COLUMN_STATEMENT] -= file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].future_balance;
			total[ACCOUNT_NUM_COLUMN_CURRENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount;
			total[ACCOUNT_NUM_COLUMN_FINAL] -= file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;
			total[ACCOUNT_NUM_COLUMN_BUDGET] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result;
			break;

		case ACCOUNT_LINE_HEADER:
			for (j = 0; j < ACCOUNT_NUM_COLUMNS; j++)
				sub_total[j] = 0;
			break;

		case ACCOUNT_LINE_FOOTER:
			for (j = 0; j < ACCOUNT_NUM_COLUMNS; j++)
				file->accounts->account_windows[entry].line_data[i].total[j] = sub_total[j];
			break;

		default:
			break;
		}
	}

	for (i = 0; i < ACCOUNT_NUM_COLUMNS; i++)
		currency_convert_to_string(total[i], file->accounts->account_windows[entry].footer_icon[i], AMOUNT_FIELD_LEN);

	/* Calculate the outgoing account details. */

	entry = account_find_window_entry_from_type(file, ACCOUNT_OUT);

	for (i = 0; i < ACCOUNT_NUM_COLUMNS; i++) {
		sub_total[i] = 0;
		total[i] = 0;
	}

	for (i = 0; i < file->accounts->account_windows[entry].display_lines; i++) {
		switch (file->accounts->account_windows[entry].line_data[i].type) {
		case ACCOUNT_LINE_DATA:
			if (file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount != NULL_CURRENCY) {
				file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result =
						file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount -
						file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;
			} else {
				file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result = NULL_CURRENCY;
			}

			sub_total[ACCOUNT_NUM_COLUMN_STATEMENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].future_balance;
			sub_total[ACCOUNT_NUM_COLUMN_CURRENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount;
			sub_total[ACCOUNT_NUM_COLUMN_FINAL] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;
			sub_total[ACCOUNT_NUM_COLUMN_BUDGET] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result;

			total[ACCOUNT_NUM_COLUMN_STATEMENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].future_balance;
			total[ACCOUNT_NUM_COLUMN_CURRENT] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_amount;
			total[ACCOUNT_NUM_COLUMN_FINAL] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_balance;
			total[ACCOUNT_NUM_COLUMN_BUDGET] += file->accounts->accounts[file->accounts->account_windows[entry].line_data[i].account].budget_result;
			break;

		case ACCOUNT_LINE_HEADER:
			for (j = 0; j < ACCOUNT_NUM_COLUMNS; j++)
				sub_total[j] = 0;
			break;

		case ACCOUNT_LINE_FOOTER:
			for (j = 0; j < ACCOUNT_NUM_COLUMNS; j++)
				file->accounts->account_windows[entry].line_data[i].total[j] = sub_total[j];
			break;

		default:
			break;
		}
	}

	for (i = 0; i < ACCOUNT_NUM_COLUMNS; i++)
		currency_convert_to_string(total[i], file->accounts->account_windows[entry].footer_icon[i], AMOUNT_FIELD_LEN);
}



/**
 * Read account list details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool account_read_list_file(struct file_block *file, struct filing_block *in)
{
	size_t			block_size;
	int			line = -1, type, entry;

	if (file == NULL || file->accounts == NULL)
		return FALSE;

	type = filing_get_account_type_suffix(in);
	if (type == ACCOUNT_NULL) {
		filing_set_status(in, FILING_STATUS_CORRUPT);
		return FALSE;
	}
	entry = account_find_window_entry_from_type(file, type);
	if (entry == -1)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Account List Data.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->accounts->account_windows[entry].line_data), sizeof(struct account_redraw), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->accounts->account_windows[entry].display_lines) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->accounts->account_windows[entry].display_lines;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			column_init_window(file->accounts->account_windows[entry].columns, 0, TRUE, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			file->accounts->account_windows[entry].display_lines++;
			if (file->accounts->account_windows[entry].display_lines > block_size) {
				block_size = file->accounts->account_windows[entry].display_lines;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			}
			line = file->accounts->account_windows[entry].display_lines - 1;
			*(file->accounts->account_windows[entry].line_data[line].heading) = '\0';
			file->accounts->account_windows[entry].line_data[line].type = account_get_account_line_type_field(in);
			file->accounts->account_windows[entry].line_data[line].account = account_get_account_field(in);
		} else if (line != -1 && filing_test_token(in, "Heading")) {
			filing_get_text_value(in, file->accounts->account_windows[entry].line_data[line].heading, ACCOUNT_SECTION_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(file->accounts->account_windows[entry].display_lines)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
}


/**
 * Callback handler for saving a CSV version of the account data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool account_save_csv(char *filename, osbool selection, void *data)
{
	struct account_window *windat = data;

	if (windat == NULL)
		return FALSE;

	account_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the account data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool account_save_tsv(char *filename, osbool selection, void *data)
{
	struct account_window *windat = data;

	if (windat == NULL)
		return FALSE;

	account_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

	return TRUE;
}


/**
 * Export the preset data from a file into CSV or TSV format.
 *
 * \param *windat		The account window to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void account_export_delimited(struct account_window *windat, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE			*out;
	int			i;
	char			buffer[FILING_DELIMITED_FIELD_LEN];

	if (windat == NULL)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	icons_copy_text(windat->account_pane, ACCOUNT_PANE_NAME, buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->account_pane, ACCOUNT_PANE_STATEMENT, buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->account_pane, ACCOUNT_PANE_CURRENT, buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->account_pane, ACCOUNT_PANE_FINAL, buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->account_pane, ACCOUNT_PANE_BUDGET, buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_LAST);

	/* Output the transaction data as a set of delimited lines. */

	for (i = 0; i < windat->display_lines; i++) {
		if (windat->line_data[i].type == ACCOUNT_LINE_DATA) {
			account_build_name_pair(windat->instance->file, windat->line_data[i].account, buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

			switch (windat->type) {
			case ACCOUNT_FULL:
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].statement_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].current_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].trial_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].budget_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
				break;

			case ACCOUNT_IN:
				currency_convert_to_string(-windat->instance->accounts[windat->line_data[i].account].future_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].budget_amount, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(-windat->instance->accounts[windat->line_data[i].account].budget_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].budget_result, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
				break;

			case ACCOUNT_OUT:
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].future_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].budget_amount, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].budget_balance, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);
				currency_convert_to_string(windat->instance->accounts[windat->line_data[i].account].budget_result, buffer, FILING_DELIMITED_FIELD_LEN);
				filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
				break;

			default:
				break;
			}
		} else if (windat->line_data[i].type == ACCOUNT_LINE_HEADER) {
			filing_output_delimited_field(out, windat->line_data[i].heading, format, DELIMIT_LAST);
		} else if (windat->line_data[i].type == ACCOUNT_LINE_FOOTER) {
			filing_output_delimited_field(out, windat->line_data[i].heading, format, DELIMIT_NONE);

			currency_convert_to_string(windat->line_data[i].total[ACCOUNT_NUM_COLUMN_STATEMENT], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[i].total[ACCOUNT_NUM_COLUMN_CURRENT], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[i].total[ACCOUNT_NUM_COLUMN_FINAL], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[i].total[ACCOUNT_NUM_COLUMN_BUDGET], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
		}
	}

	/* Output the grand total line, taking the text from the window icons. */

	icons_copy_text(windat->account_footer, ACCOUNT_FOOTER_NAME, buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_NUM_COLUMN_STATEMENT], format, DELIMIT_NUM);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_NUM_COLUMN_CURRENT], format, DELIMIT_NUM);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_NUM_COLUMN_FINAL], format, DELIMIT_NUM);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_NUM_COLUMN_BUDGET], format, DELIMIT_NUM | DELIMIT_LAST);

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}

