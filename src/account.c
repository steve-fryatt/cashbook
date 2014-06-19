/* Copyright 2003-2013, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: account.c
 *
 * Account and account list implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/wimp.h"
#include "oslib/dragasprite.h"
#include "oslib/wimpspriteop.h"
#include "oslib/hourglass.h"

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
#include "account.h"

#include "accview.h"
#include "analysis.h"
#include "calculation.h"
#include "conversion.h"
#include "column.h"
#include "caret.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "presets.h"
#include "printing.h"
#include "report.h"
#include "saveas.h"
#include "sorder.h"
#include "templates.h"
#include "transact.h"
#include "window.h"


/* AccList menu */

#define ACCLIST_MENU_VIEWACCT 0
#define ACCLIST_MENU_EDITACCT 1
#define ACCLIST_MENU_EDITSECT 2
#define ACCLIST_MENU_NEWACCT 3
#define ACCLIST_MENU_NEWHEADER 4
#define ACCLIST_MENU_EXPCSV 5
#define ACCLIST_MENU_EXPTSV 6
#define ACCLIST_MENU_PRINT 7


struct account_list_link {
	char		name[ACCOUNT_NAME_LEN];
	acct_t		account;
};

struct account_list_group {
	char		name[ACCOUNT_SECTION_LEN];
	int		entry;
	int		start_line;
};

/* Account Edit Window. */

static wimp_w			account_acc_edit_window = NULL;

/* Heading Edit Window. */

static wimp_w			account_hdg_edit_window = NULL;
static file_data		*edit_account_file = NULL;
static acct_t			edit_account_no = NULL_ACCOUNT;

/* Section Edit Window. */

static wimp_w			account_section_window = NULL;			/**< The handle of the Section Edit window.				*/
static file_data		*account_section_file = NULL;			/**< The file owning the Section Edit window.				*/
static int			account_section_entry = -1;			/**< The entry owning the Section Edit window.				*/
static int			account_section_line = -1;			/**< The line being edited by the Section Edit window.			*/

/* Account List Print Window. */

static file_data		*account_print_file = NULL;			/**< The file owning the Account List Print window.			*/
static enum account_type	account_print_type = ACCOUNT_NULL;		/**< The type of account owning the Account List Print window.		*/

/* Account List Window. */

static wimp_window		*account_window_def = NULL;			/**< The definition for the Accounts List Window.			*/
static wimp_window		*account_pane_def[2] = {NULL, NULL};		/**< The definition for the Accounts List Toolbar pane.			*/
static wimp_window		*account_foot_def = NULL;			/**< The definition for the Accounts List Footer pane.			*/
static wimp_menu		*account_window_menu = NULL;			/**< The Accounts List Window menu handle.				*/
static int			account_window_menu_line = -1;			/**< The line over which the Accounts List Window Menu was opened.	*/


static wimp_menu			*account_list_menu = NULL;		/**< The Account List menu block.					*/
static struct account_list_link		*account_list_menu_link = NULL;		/**< Links for the Account List menu.					*/
static char				*account_list_menu_title = NULL;	/**< Account List menu title buffer.					*/
static file_data			*account_list_menu_file = NULL;		/**< The file to which the Account List menu is currently attached.	*/

static wimp_menu			*account_complete_menu = NULL;		/**< The Account Complete menu block.					*/
static struct account_list_group	*account_complete_menu_group = NULL;	/**< Groups for the Account Complete menu.				*/
static wimp_menu			*account_complete_submenu = NULL;	/**< The Account Complete menu block.					*/
static struct account_list_link		*account_complete_submenu_link = NULL;	/**< Links for the Account Complete menu.				*/
static char				*account_complete_menu_title = NULL;	/**< Account Complete menu title buffer.				*/
static file_data			*account_complete_menu_file = NULL;	/**< The file to which the Account Complete menu is currently attached.	*/

/* SaveAs Dialogue Handles. */

static struct saveas_block	*account_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*account_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/




/* \TODO -- These entries should probably become struct account_window * pointers? */


/* Account List Window drags. */

static osbool			account_dragging_sprite = FALSE;		/**< True if the account line drag is using a sprite.			*/
static file_data		*account_dragging_file = NULL;			/**< The file owning the account list in which the drag occurs.		*/
static int			account_dragging_entry = -1;			/**< The account list entry owning the current drag.			*/
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
static void			account_set_window_extent(file_data *file, int entry);
static void			account_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);





static void			account_acc_edit_click_handler(wimp_pointer *pointer);
static void			account_hdg_edit_click_handler(wimp_pointer *pointer);
static osbool			account_acc_edit_keypress_handler(wimp_key *key);
static osbool			account_hdg_edit_keypress_handler(wimp_key *key);
static void			account_refresh_acc_edit_window(void);
static void			account_refresh_hdg_edit_window(void);
static void			account_fill_acc_edit_window(file_data *file, acct_t account);
static void			account_fill_hdg_edit_window(file_data *file, acct_t account, enum account_type type);
static osbool			account_process_acc_edit_window(void);
static osbool			account_process_hdg_edit_window(void);
static osbool			account_delete_from_edit_window (void);
static void			account_open_section_window(file_data *file, int entry, int line, wimp_pointer *ptr);
static void			account_section_click_handler(wimp_pointer *pointer);
static osbool			account_section_keypress_handler(wimp_key *key);
static void			account_refresh_section_window(void);
static void			account_fill_section_window(file_data *file, int entry, int line);
static osbool			account_process_section_window(void);
static osbool			account_delete_from_section_window(void);
static void			account_open_print_window(file_data *file, enum account_type type, wimp_pointer *ptr, osbool restore);
static void			account_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);



static void			account_add_to_lists(file_data *file, acct_t account);
static int			account_add_list_display_line(file_data *file, int entry);



static void			start_account_drag(file_data *file, int entry, int line);
static void			account_terminate_drag(wimp_dragged *drag, void *data);

static osbool			account_save_csv(char *filename, osbool selection, void *data);
static osbool			account_save_tsv(char *filename, osbool selection, void *data);



/**
 * Initialise the account system.
 *
 * \param *sprites		The application sprite area.
 */

void account_initialise(osspriteop_area *sprites)
{
	account_acc_edit_window = templates_create_window("EditAccount");
	ihelp_add_window(account_acc_edit_window, "EditAccount", NULL);
	event_add_window_mouse_event(account_acc_edit_window, account_acc_edit_click_handler);
	event_add_window_key_event(account_acc_edit_window, account_acc_edit_keypress_handler);

	account_hdg_edit_window = templates_create_window("EditHeading");
	ihelp_add_window(account_hdg_edit_window, "EditHeading", NULL);
	event_add_window_mouse_event(account_hdg_edit_window, account_hdg_edit_click_handler);
	event_add_window_key_event(account_hdg_edit_window, account_hdg_edit_keypress_handler);
	event_add_window_icon_radio(account_hdg_edit_window, HEAD_EDIT_INCOMING, TRUE);
	event_add_window_icon_radio(account_hdg_edit_window, HEAD_EDIT_OUTGOING, TRUE);

	account_section_window = templates_create_window("EditAccSect");
	ihelp_add_window(account_section_window, "EditAccSect", NULL);
	event_add_window_mouse_event(account_section_window, account_section_click_handler);
	event_add_window_key_event(account_section_window, account_section_keypress_handler);
	event_add_window_icon_radio(account_section_window, SECTION_EDIT_HEADER, TRUE);
	event_add_window_icon_radio(account_section_window, SECTION_EDIT_FOOTER, TRUE);

	account_window_def = templates_load_window("Account");
	account_window_def->icon_count = 0;

	account_pane_def[0] = templates_load_window("AccountATB");
	account_pane_def[0]->sprite_area = sprites;

	account_pane_def[1] = templates_load_window("AccountHTB");
	account_pane_def[1]->sprite_area = sprites;

	account_foot_def = templates_load_window("AccountTot");

	account_window_menu = templates_get_menu(TEMPLATES_MENU_ACCLIST);

	account_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", account_save_csv);
	account_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", account_save_tsv);
}


/**
 * Create and open an Accounts List window for the given file and account type.
 *
 * \param *file			The file to open a window for.
 * \param type			The type of account to open a window for.
 */

void account_open_window(file_data *file, enum account_type type)
{
	int			entry, i, j, tb_type, height;
	os_error		*error;
	wimp_window_state	parent;
	struct account_window	*window;

	/* Find the window block to use. */

	entry = account_find_window_entry_from_type(file, type);

	if (entry == -1)
		return;

	window = &(file->account_windows[entry]);

	/* Create or re-open the window. */

	if (window->account_window != NULL) {
		windows_open(window->account_window);
		return;
	}

	/* Set the main window extent and create it. */

	*(window->window_title) = '\0';
	account_window_def->title_data.indirected_text.text = window->window_title;

	height =  (window->display_lines > MIN_ACCOUNT_ENTRIES) ? window->display_lines : MIN_ACCOUNT_ENTRIES;

	/* Find the position to open the window at. */

	parent.w = file->transaction_window.transaction_pane;
	wimp_get_window_state(&parent);

	set_initial_window_area(account_window_def,
			window->column_position[ACCOUNT_COLUMNS-1] +
			window->column_width[ACCOUNT_COLUMNS-1],
			((ICON_HEIGHT+LINE_GUTTER) * height) +
			(ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT + 2),
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	file->child_x_offset++;
	if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
		file->child_x_offset = 0;

	error = xwimp_create_window (account_window_def, &(window->account_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		error_report_info("Main window");
		account_delete_window(window);
		return;
	}

	/* Create the toolbar pane. */

	tb_type = (type == ACCOUNT_FULL) ? 0 : 1; /* Use toolbar 0 if it's a full account, 1 otherwise. */

	windows_place_as_toolbar(account_window_def, account_pane_def[tb_type], ACCOUNT_TOOLBAR_HEIGHT-4);

	for (i=0, j=0; j < ACCOUNT_COLUMNS; i++, j++) {
		account_pane_def[tb_type]->icons[i].extent.x0 = window->column_position[j];
		j = column_get_rightmost_in_group(ACCOUNT_PANE_COL_MAP, i);
		account_pane_def[tb_type]->icons[i].extent.x1 = window->column_position[j] + window->column_width[j] + COLUMN_HEADING_MARGIN;
	}

	error = xwimp_create_window(account_pane_def[tb_type], &(window->account_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		error_report_info("Toolbar");
		account_delete_window(window);
		return;
	}

	/* Create the footer pane. */

	windows_place_as_footer(account_window_def, account_foot_def, ACCOUNT_FOOTER_HEIGHT);

	for (i=0; i < ACCOUNT_NUM_COLUMNS; i++)
		account_foot_def->icons[i+1].data.indirected_text.text = window->footer_icon[i];

	for (i=0, j=0; j < ACCOUNT_COLUMNS; i++, j++) {
		account_foot_def->icons[i].extent.x0 = window->column_position[j];
		account_foot_def->icons[i].extent.y0 = -ACCOUNT_FOOTER_HEIGHT;
		account_foot_def->icons[i].extent.y1 = 0;

		j = column_get_rightmost_in_group(ACCOUNT_PANE_COL_MAP, i);

		account_foot_def->icons[i].extent.x1 = window->column_position[j] + window->column_width[j];
	}

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

	line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - ACCOUNT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
	if (line < 0 || line >= windat->display_lines)
		line = -1;

	/* Handle double-clicks, which will open a statement view or an edit accout window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1) {
		if (windat->line_data[line].type == ACCOUNT_LINE_DATA)
			accview_open_window(windat->file, windat->line_data[line].account);
	} else if (pointer->buttons == wimp_DOUBLE_ADJUST && line != -1) {
		switch (windat->line_data[line].type) {
		case ACCOUNT_LINE_DATA:
			account_open_edit_window(windat->file, windat->line_data[line].account, ACCOUNT_NULL, pointer);
			break;

		case ACCOUNT_LINE_HEADER:
		case ACCOUNT_LINE_FOOTER:
			account_open_section_window(windat->file, windat->entry, line, pointer);
			break;
		default:
			break;
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && line != -1) {
		start_account_drag(windat->file, windat->entry, line);
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
			windows_open(windat->file->transaction_window.transaction_window);
			break;

		case ACCOUNT_PANE_PRINT:
			account_open_print_window(windat->file, windat->type, pointer, config_opt_read("RememberValues"));
			break;

		case ACCOUNT_PANE_ADDACCT:
			account_open_edit_window(windat->file, NULL_ACCOUNT, windat->type, pointer);
			break;

		case ACCOUNT_PANE_ADDSECT:
			account_open_section_window(windat->file, windat->entry, -1, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case ACCOUNT_PANE_PRINT:
			account_open_print_window(windat->file, windat->type, pointer, !config_opt_read("RememberValues"));
			break;
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT) {
		column_start_drag(pointer, windat, windat->account_window, ACCOUNT_PANE_COL_MAP, config_str_read("LimAccountCols"), account_adjust_window_columns);
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
	if (windat == NULL || windat->file == NULL)
		return;

	if (pointer != NULL) {
		account_window_menu_line = -1;

		if (w == windat->account_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - ACCOUNT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

			if (line >= 0 && line < windat->display_lines)
				account_window_menu_line = line;
		}

		data = (account_window_menu_line == -1) ? ACCOUNT_LINE_BLANK : windat->line_data[account_window_menu_line].type;

		saveas_initialise_dialogue(account_saveas_csv, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(account_saveas_tsv, "DefTSVFile", NULL, FALSE, FALSE, windat);

		switch (windat->type) {
		case ACCOUNT_FULL:
			msgs_lookup("AcclistMenuTitleAcc", account_window_menu->title_data.text, 12);
			msgs_lookup("AcclistMenuViewAcc", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_VIEWACCT), 20);
			msgs_lookup("AcclistMenuEditAcc", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_EDITACCT), 20);
			msgs_lookup("AcclistMenuNewAcc", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_NEWACCT), 20);
			templates_set_menu_token("AccListMenu");
			break;

		case ACCOUNT_IN:
		case ACCOUNT_OUT:
			msgs_lookup("AcclistMenuTitleHead", account_window_menu->title_data.text, 12);
			msgs_lookup("AcclistMenuViewHead", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_VIEWACCT), 20);
			msgs_lookup("AcclistMenuEditHead", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_EDITACCT), 20);
			msgs_lookup("AcclistMenuNewHead", menus_get_indirected_text_addr(account_window_menu, ACCLIST_MENU_NEWACCT), 20);
			templates_set_menu_token("HeadListMenu");
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
	osbool			data;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	data = (account_window_menu_line != -1) ? ACCOUNT_LINE_BLANK : windat->line_data[account_window_menu_line].type;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case ACCLIST_MENU_VIEWACCT:
		accview_open_window(windat->file, windat->line_data[account_window_menu_line].account);
		break;

	case ACCLIST_MENU_EDITACCT:
		account_open_edit_window(windat->file, windat->line_data[account_window_menu_line].account, ACCOUNT_NULL, &pointer);
		break;

	case ACCLIST_MENU_EDITSECT:
		account_open_section_window(windat->file, windat->entry, account_window_menu_line, &pointer);
		break;

	case ACCLIST_MENU_NEWACCT:
		account_open_edit_window(windat->file, -1, windat->type, &pointer);
		break;

	case ACCLIST_MENU_NEWHEADER:
		account_open_section_window(windat->file, windat->entry, -1, &pointer);
		break;

	case ACCLIST_MENU_PRINT:
		account_open_print_window(windat->file, windat->type, &pointer, config_opt_read("RememberValues"));
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
	if (windat == NULL || windat->file == NULL)
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
	templates_set_menu_token(NULL);
}


/**
 * Process scroll events in the Accounts List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void account_window_scroll_handler(wimp_scroll *scroll)
{
	int	width, height, error;

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

	height = (scroll->visible.y1 - scroll->visible.y0) - (ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT);

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

static void account_window_redraw_handler(wimp_draw *redraw)
{
	int			ox, oy, top, base, y, i, shade_overdrawn_col, icon_fg_col;
	char			icon_buffer1[AMOUNT_FIELD_LEN], icon_buffer2[AMOUNT_FIELD_LEN], icon_buffer3[AMOUNT_FIELD_LEN],
				icon_buffer4[AMOUNT_FIELD_LEN];
	osbool			more, shade_overdrawn;
	struct account_window	*windat;
	file_data		*file;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL)
		return;

	file = windat->file;

	shade_overdrawn = config_opt_read("ShadeAccounts");
	shade_overdrawn_col = config_int_read("ShadeAccountsColour");

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	/* Set the horizontal positions of the icons for the account lines. */

	for (i=0; i < ACCOUNT_COLUMNS; i++) {
		account_window_def->icons[i].extent.x0 = windat->column_position[i];
		account_window_def->icons[i].extent.x1 = windat->column_position[i] + windat->column_width[i];
	}

	/* Set the positions for the heading lines. */

	account_window_def->icons[6].extent.x0 = windat->column_position[0];
	account_window_def->icons[6].extent.x1 = windat->column_position[ACCOUNT_COLUMNS-1] + windat->column_width[ACCOUNT_COLUMNS-1];

	/* Set the positions for the footer lines. */

	account_window_def->icons[7].extent.x0 = windat->column_position[0];
	account_window_def->icons[7].extent.x1 = windat->column_position[1] + windat->column_width[1];

	account_window_def->icons[8].extent.x0 = windat->column_position[2];
	account_window_def->icons[8].extent.x1 = windat->column_position[2] + windat->column_width[2];

	account_window_def->icons[9].extent.x0 = windat->column_position[3];
	account_window_def->icons[9].extent.x1 = windat->column_position[3] + windat->column_width[3];

	account_window_def->icons[10].extent.x0 = windat->column_position[4];
	account_window_def->icons[10].extent.x1 = windat->column_position[4] + windat->column_width[4];

	account_window_def->icons[11].extent.x0 = windat->column_position[5];
	account_window_def->icons[11].extent.x1 = windat->column_position[ACCOUNT_COLUMNS-1] + windat->column_width[ACCOUNT_COLUMNS-1];

	/* The three numerical columns keep their icon buffers for the whole time, so set them up now. */

	account_window_def->icons[2].data.indirected_text.text = icon_buffer1;
	account_window_def->icons[3].data.indirected_text.text = icon_buffer2;
	account_window_def->icons[4].data.indirected_text.text = icon_buffer3;
	account_window_def->icons[5].data.indirected_text.text = icon_buffer4;

	account_window_def->icons[8].data.indirected_text.text = icon_buffer1;
	account_window_def->icons[9].data.indirected_text.text = icon_buffer2;
	account_window_def->icons[10].data.indirected_text.text = icon_buffer3;
	account_window_def->icons[11].data.indirected_text.text = icon_buffer4;

	/* Reset all the icon colours. */

	account_window_def->icons[2].flags &= ~wimp_ICON_FG_COLOUR;
	account_window_def->icons[2].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

	account_window_def->icons[3].flags &= ~wimp_ICON_FG_COLOUR;
	account_window_def->icons[3].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

	account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
	account_window_def->icons[4].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

	account_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
	account_window_def->icons[5].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

	/* Perform the redraw. */

	while (more) {
		/* Calculate the rows to redraw. */

		top = (oy - redraw->clip.y1 - ACCOUNT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
		if (top < 0)
			top = 0;

		base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2) +
				oy - redraw->clip.y0 - ACCOUNT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			/* Plot out the background with a filled white rectangle. */

			wimp_set_colour(wimp_COLOUR_WHITE);
			os_plot(os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCOUNT_TOOLBAR_HEIGHT);
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO,
					ox + windat->column_position[ACCOUNT_COLUMNS-1] + windat->column_width[ACCOUNT_COLUMNS-1],
					oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCOUNT_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

			if (y<windat->display_lines && windat->line_data[y].type == ACCOUNT_LINE_DATA) {
				/* Account field */

				account_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[0].data.indirected_text.text =
						file->accounts[windat->line_data[y].account].ident;
				account_window_def->icons[1].data.indirected_text.text =
						file->accounts[windat->line_data[y].account].name;

				wimp_plot_icon(&(account_window_def->icons[0]));
				wimp_plot_icon(&(account_window_def->icons[1]));

				/* Place the four numerical columns. */

				account_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				/* Set the column data depending on the window type. */

				switch (windat->type) {
				case ACCOUNT_FULL:
					convert_money_to_string(file->accounts[windat->line_data[y].account].statement_balance, icon_buffer1);
					convert_money_to_string(file->accounts[windat->line_data[y].account].current_balance, icon_buffer2);
					convert_money_to_string(file->accounts[windat->line_data[y].account].trial_balance, icon_buffer3);
					convert_money_to_string(file->accounts[windat->line_data[y].account].budget_balance, icon_buffer4);

					if (shade_overdrawn &&
							(file->accounts[windat->line_data[y].account].statement_balance <
							-file->accounts[windat->line_data[y].account].credit_limit))
						icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
					else
						icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
					account_window_def->icons[2].flags &= ~wimp_ICON_FG_COLOUR;
					account_window_def->icons[2].flags |= icon_fg_col;

					if (shade_overdrawn &&
							(file->accounts[windat->line_data[y].account].current_balance <
							-file->accounts[windat->line_data[y].account].credit_limit))
						icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
					else
						icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
					account_window_def->icons[3].flags &= ~wimp_ICON_FG_COLOUR;
					account_window_def->icons[3].flags |= icon_fg_col;

					if (shade_overdrawn &&
							(file->accounts[windat->line_data[y].account].trial_balance < 0))
						icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
					else
						icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
					account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
					account_window_def->icons[4].flags |= icon_fg_col;
					break;

				case ACCOUNT_IN:
					convert_money_to_string(-file->accounts[windat->line_data[y].account].future_balance, icon_buffer1);
					convert_money_to_string(file->accounts[windat->line_data[y].account].budget_amount, icon_buffer2);
					convert_money_to_string(-file->accounts[windat->line_data[y].account].budget_balance, icon_buffer3);
					convert_money_to_string(file->accounts[windat->line_data[y].account].budget_result, icon_buffer4);

					if (shade_overdrawn &&
							(-file->accounts[windat->line_data[y].account].budget_balance <
							file->accounts[windat->line_data[y].account].budget_amount))
						icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
					else
						icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
					account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
					account_window_def->icons[4].flags |= icon_fg_col;
					account_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
					account_window_def->icons[5].flags |= icon_fg_col;
					break;

				case ACCOUNT_OUT:
					convert_money_to_string(file->accounts[windat->line_data[y].account].future_balance, icon_buffer1);
					convert_money_to_string(file->accounts[windat->line_data[y].account].budget_amount, icon_buffer2);
					convert_money_to_string(file->accounts[windat->line_data[y].account].budget_balance, icon_buffer3);
					convert_money_to_string(file->accounts[windat->line_data[y].account].budget_result, icon_buffer4);

					if (shade_overdrawn &&
							(file->accounts[windat->line_data[y].account].budget_balance >
							file->accounts[windat->line_data[y].account].budget_amount))
						icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
					else
						icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

					account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
					account_window_def->icons[4].flags |= icon_fg_col;
					account_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
					account_window_def->icons[5].flags |= icon_fg_col;
					break;

				default:
					break;
				}

				/* Plot the three icons. */

				wimp_plot_icon(&(account_window_def->icons[2]));
				wimp_plot_icon(&(account_window_def->icons[3]));
				wimp_plot_icon(&(account_window_def->icons[4]));
				wimp_plot_icon(&(account_window_def->icons[5]));
			} else if (y<windat->display_lines && windat->line_data[y].type == ACCOUNT_LINE_HEADER) {
				/* Block header line */

				account_window_def->icons[6].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[6].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[6].data.indirected_text.text = windat->line_data[y].heading;

				wimp_plot_icon(&(account_window_def->icons[6]));
			} else if (y<windat->display_lines && windat->line_data[y].type == ACCOUNT_LINE_FOOTER) {
				/* Block footer line */

				account_window_def->icons[7].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[7].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[8].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[8].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[9].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[9].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[10].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[10].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[11].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[11].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[7].data.indirected_text.text = windat->line_data[y].heading;
				convert_money_to_string(windat->line_data[y].total[0], icon_buffer1);
				convert_money_to_string(windat->line_data[y].total[1], icon_buffer2);
				convert_money_to_string(windat->line_data[y].total[2], icon_buffer3);
				convert_money_to_string(windat->line_data[y].total[3], icon_buffer4);

				wimp_plot_icon(&(account_window_def->icons[7]));
				wimp_plot_icon(&(account_window_def->icons[8]));
				wimp_plot_icon(&(account_window_def->icons[9]));
				wimp_plot_icon(&(account_window_def->icons[10]));
				wimp_plot_icon(&(account_window_def->icons[11]));
			} else {
				/* Blank line */
				account_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
				account_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
						ACCOUNT_TOOLBAR_HEIGHT;

				account_window_def->icons[0].data.indirected_text.text = icon_buffer1;
				account_window_def->icons[1].data.indirected_text.text = icon_buffer1;
				*icon_buffer1 = '\0';
				*icon_buffer2 = '\0';
				*icon_buffer3 = '\0';
				*icon_buffer4 = '\0';

				wimp_plot_icon(&(account_window_def->icons[0]));
				wimp_plot_icon(&(account_window_def->icons[1]));
				wimp_plot_icon(&(account_window_def->icons[2]));
				wimp_plot_icon(&(account_window_def->icons[3]));
				wimp_plot_icon(&(account_window_def->icons[4]));
				wimp_plot_icon(&(account_window_def->icons[5]));
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
	file_data		*file;
	int			entry, i, j, new_extent;
	wimp_icon_state		icon1, icon2;
	wimp_window_info	window;

	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;
	entry = windat->entry;

	update_dragged_columns(ACCOUNT_PANE_COL_MAP, config_str_read("LimAccountCols"), icon, width,
			file->account_windows[entry].column_width,
			file->account_windows[entry].column_position, ACCOUNT_COLUMNS);

	/* Re-adjust the icons in the pane. */

	for (i = 0, j = 0; j < ACCOUNT_COLUMNS; i++, j++) {
		icon1.w = file->account_windows[entry].account_pane;
		icon1.i = i;
		wimp_get_icon_state(&icon1);

		icon2.w = file->account_windows[entry].account_footer;
		icon2.i = i;
		wimp_get_icon_state(&icon2);

		icon1.icon.extent.x0 = file->account_windows[entry].column_position[j];

		icon2.icon.extent.x0 = file->account_windows[entry].column_position[j];

		j = column_get_rightmost_in_group(ACCOUNT_PANE_COL_MAP, i);

		icon1.icon.extent.x1 = file->account_windows[entry].column_position[j] +
				file->account_windows[entry].column_width[j] + COLUMN_HEADING_MARGIN;

		icon2.icon.extent.x1 = file->account_windows[entry].column_position[j] +
				file->account_windows[entry].column_width[j];

		wimp_resize_icon(icon1.w, icon1.i, icon1.icon.extent.x0, icon1.icon.extent.y0,
				icon1.icon.extent.x1, icon1.icon.extent.y1);

		wimp_resize_icon(icon2.w, icon2.i, icon2.icon.extent.x0, icon2.icon.extent.y0,
				icon2.icon.extent.x1, icon2.icon.extent.y1);

		new_extent = file->account_windows[entry].column_position[ACCOUNT_COLUMNS-1] +
				file->account_windows[entry].column_width[ACCOUNT_COLUMNS-1];
	}

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(file->account_windows[entry].account_window);
	windows_redraw(file->account_windows[entry].account_pane);
	windows_redraw(file->account_windows[entry].account_footer);

	/* Set the horizontal extent of the window and pane. */

	window.w = file->account_windows[entry].account_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = file->account_windows[entry].account_footer;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = file->account_windows[entry].account_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	set_file_data_integrity(file, TRUE);
}


/**
 * Set the extent of an account window for the specified file.
 *
 * \param *file			The file to update.
 * \param entry			The entry of the window to to be updated.
 */

static void account_set_window_extent(file_data *file, int entry)
{
	wimp_window_state	state;
	os_box			extent;
	int			new_height, visible_extent, new_extent, new_scroll;

	/* Set the extent. */

	if (file == NULL || file->account_windows[entry].account_window == NULL)
		return;

	/* Get the number of rows to show in the window, and work out the window extent from this. */

	new_height =  (file->account_windows[entry].display_lines > MIN_ACCOUNT_ENTRIES) ?
			file->account_windows[entry].display_lines : MIN_ACCOUNT_ENTRIES;

	new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_height) - (ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT + 2);

	/* Get the current window details, and find the extent of the bottom of the visible area. */

	state.w = file->account_windows[entry].account_window;
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
	extent.x1 = file->account_windows[entry].column_position[ACCOUNT_COLUMNS-1] +
			file->account_windows[entry].column_width[ACCOUNT_COLUMNS-1];

	extent.y0 = new_extent;
	extent.y1 = 0;

	wimp_set_extent(file->account_windows[entry].account_window, &extent);
}


/**
 * Recreate the title of the specified Account window connected to the
 * given file.
 *
 * \param *file			The file to rebuild the title for.
 * \param entry			The entry of the window to to be updated.
 */

void account_build_window_title(file_data *file, int entry)
{
	char	name[256];

	if (file == NULL || file->account_windows[entry].account_window == NULL)
		return;

	make_file_leafname(file, name, sizeof(name));

	switch (file->account_windows[entry].type) {
	case ACCOUNT_FULL:
		msgs_param_lookup("AcclistTitleAcc", file->account_windows[entry].window_title,
				sizeof(file->account_windows[entry].window_title),
				name, NULL, NULL, NULL);
		break;

	case ACCOUNT_IN:
		msgs_param_lookup("AcclistTitleHIn", file->account_windows[entry].window_title,
				sizeof(file->account_windows[entry].window_title),
				name, NULL, NULL, NULL);
		break;

	case ACCOUNT_OUT:
		msgs_param_lookup("AcclistTitleHOut", file->account_windows[entry].window_title,
				sizeof(file->account_windows[entry].window_title),
				name, NULL, NULL, NULL);
		break;

	default:
		break;
	}

	wimp_force_redraw_title(file->account_windows[entry].account_window);
}


/**
 * Force a redraw of the Account List window, for the given range of
 * lines.
 *
 * \param *file			The file owning the window.
 * \param entry			The account list window to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

void account_force_window_redraw(file_data *file, int entry, int from, int to)
{
	int			y0, y1;
	wimp_window_info	window;

	if (file == NULL || file->account_windows[entry].account_window == NULL)
		return;

	window.w = file->account_windows[entry].account_window;
	wimp_get_window_info_header_only(&window);

	y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - ACCOUNT_TOOLBAR_HEIGHT;
	y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - ACCOUNT_TOOLBAR_HEIGHT;

	wimp_force_redraw(file->account_windows[entry].account_window, window.extent.x0, y0, window.extent.x1, y1);

	/* Force a redraw of the three total icons in the footer. */

	icons_redraw_group(file->account_windows[entry].account_footer, 4, 1, 2, 3, 4);
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
	int				column, xpos;
	wimp_window_state		window;
	struct account_window		*windat;

	*buffer = '\0';

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	for (column = 0;
			column < ACCOUNT_COLUMNS && xpos > (windat->column_position[column] + windat->column_width[column]);
			column++);

	sprintf(buffer, "Col%d", column);
}























/**
 * Build an Account List menu for a file, and return the pointer.  This is a
 * list of Full Accounts, used for opening a Account List view.
 *
 * \param *file			The file to build the menu for.
 * \return			The created menu, or NULL for an error.
 */


wimp_menu *account_list_menu_build(file_data *file)
{
	int	i, line, accounts, entry, width;

	account_list_menu_destroy();

	if (file == NULL)
		return NULL;

	entry = account_find_window_entry_from_type(file, ACCOUNT_FULL);

	/* Find out how many accounts there are. */

	accounts = account_count_type_in_file(file, ACCOUNT_FULL);

	if (accounts == 0)
		return NULL;

	#ifdef DEBUG
	debug_printf("\\GBuilding account menu for %d accounts", accounts);
	#endif

	/* Claim enough memory to build the menu in. */

	account_list_menu = heap_alloc(28 + (24 * accounts));
	account_list_menu_link = heap_alloc(sizeof(struct account_list_link) * accounts);
	account_list_menu_title = heap_alloc(ACCOUNT_MENU_TITLE_LEN);

	if (account_list_menu == NULL || account_list_menu_link == NULL || account_list_menu_title == NULL) {
		account_list_menu_destroy();
		return NULL;
	}

	account_list_menu_file = file;

	/* Populate the menu. */

	line = 0;
	i = 0;
	width = 0;

	while (line < accounts && i < file->account_windows[entry].display_lines) {
		/* If the line is an account, add it to the manu... */

		if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
			/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
			 * well move while the menu is open.  The account number is also stored, to allow the account to be found.
			 */

			strcpy(account_list_menu_link[line].name, file->accounts[file->account_windows[entry].line_data[i].account].name);
			account_list_menu_link[line].account = file->account_windows[entry].line_data[i].account;
			if (strlen(account_list_menu_link[line].name) > width)
				width = strlen(account_list_menu_link[line].name);

			/* Set the menu and icon flags up. */

			account_list_menu->entries[line].menu_flags = 0;

			account_list_menu->entries[line].sub_menu = (wimp_menu *) -1;
			account_list_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			account_list_menu->entries[line].data.indirected_text.text = account_list_menu_link[line].name;
			account_list_menu->entries[line].data.indirected_text.validation = NULL;
			account_list_menu->entries[line].data.indirected_text.size = ACCOUNT_NAME_LEN;

			#ifdef DEBUG
			debug_printf ("Line %d: '%s'", line, account_list_menu_link[line].name);
			#endif

			line++;
		} else if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER && line > 0) {
			/* If the line is a header, and the menu has an item in it, add a separator... */
			account_list_menu->entries[line-1].menu_flags |= wimp_MENU_SEPARATE;
		}

		i++;
	}

	account_list_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	msgs_lookup("ViewaccMenuTitle", account_list_menu_title, ACCOUNT_MENU_TITLE_LEN);
	account_list_menu->title_data.indirected_text.text = account_list_menu_title;
	account_list_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_list_menu->title_fg = wimp_COLOUR_BLACK;
	account_list_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_list_menu->work_fg = wimp_COLOUR_BLACK;
	account_list_menu->work_bg = wimp_COLOUR_WHITE;

	account_list_menu->width = (width + 1) * 16;
	account_list_menu->height = 44;
	account_list_menu->gap = 0;

	return account_list_menu;
}


/**
 * Destroy any Account List menu which is currently open.
 */

void account_list_menu_destroy(void)
{
	if (account_list_menu != NULL)
		heap_free(account_list_menu);

	if (account_list_menu_link != NULL)
		heap_free(account_list_menu_link);

	if (account_list_menu_title != NULL)
		heap_free(account_list_menu_title);

	account_list_menu = NULL;
	account_list_menu_link = NULL;
	account_list_menu_title = NULL;
	account_list_menu_file = NULL;
}


/**
 * Prepare the Account List menu for opening or reopening, by ticking those
 * accounts which have Account List windows already open.
 */

void account_list_menu_prepare(void)
{
	int i = 0;

	if (account_list_menu == NULL || account_list_menu_link == NULL || account_list_menu_file == NULL)
		return;

	do {
		if (account_list_menu_file->accounts[account_list_menu_link[i].account].account_view != NULL)
			account_list_menu->entries[i].menu_flags |= wimp_MENU_TICKED;
		else
			account_list_menu->entries[i].menu_flags &= ~wimp_MENU_TICKED;
	} while ((account_list_menu->entries[i++].menu_flags & wimp_MENU_LAST) == 0);
}



/**
 * Decode a selection from the Account List menu, converting to an account
 * number.
 *
 * \param selection		The menu selection to decode.
 * \return			The account numer, or NULL_ACCOUNT.
 */

acct_t account_list_menu_decode(int selection)
{
	if (account_list_menu_link == NULL || selection == -1)
		return NULL_ACCOUNT;

	return account_list_menu_link[selection].account;
}


/**
 * Build an Account Complete menu for a given file and account type.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \return			The menu block, or NULL.
 */

wimp_menu *account_complete_menu_build(file_data *file, enum account_menu_type type)
{
	int			i, group, line, entry, width, sublen;
	int			groups = 3, maxsublen = 0, headers = 0;
	enum account_type	include, sequence[] = {ACCOUNT_FULL, ACCOUNT_IN, ACCOUNT_OUT};
	osbool			shade;
	char			*title;

	account_complete_menu_destroy();

	switch (type) {
	case ACCOUNT_MENU_FROM:
		include = ACCOUNT_FULL | ACCOUNT_IN;
		title = "ViewAccMenuTitleFrom";
		break;

	case ACCOUNT_MENU_TO:
		include = ACCOUNT_FULL | ACCOUNT_OUT;
		title = "ViewAccMenuTitleTo";
		break;

	case ACCOUNT_MENU_ACCOUNTS:
		include = ACCOUNT_FULL;
		title = "ViewAccMenuTitleAcc";
		break;

	case ACCOUNT_MENU_INCOMING:
		include = ACCOUNT_IN;
		title = "ViewAccMenuTitleIn";
		break;

	case ACCOUNT_MENU_OUTGOING:
		include = ACCOUNT_OUT;
		title = "ViewAccMenuTitleOut";
		break;

	default:
		include = ACCOUNT_NULL;
		title = "";
		break;
	}

	account_complete_menu_file = file;

	/* Find out how many accounts there are, by counting entries in the groups. */

	/* for each group that will be included in the menu, count through the window definition. */

	for (group = 0; group < groups; group++) {
		if (include & sequence[group]) {
			i = 0;
			sublen = 0;
			entry = account_find_window_entry_from_type(file, sequence[group]);

			while (i < file->account_windows[entry].display_lines) {
				if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER) {
					/* If the line is a header, increment the header count, and start a new sub-menu. */

					if (sublen > maxsublen)
						maxsublen = sublen;

					sublen = 0;
					headers++;
				} else if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
					/* Else if the line is an account entry, increment the submenu length count.  If the line is the first in the
					 * group, it must fall outwith any headers and so will require its own submenu.
					 */

					sublen++;

					if (i == 0)
						headers++;
				}

				i++;
			}

			if (sublen > maxsublen)
				maxsublen = sublen;
		}
	}

	#ifdef DEBUG
	debug_printf("\\GBuilding accounts menu for %d headers, maximum submenu of %d", headers, maxsublen);
	#endif

	if (headers == 0 || maxsublen == 0)
		return NULL;

	/* Claim enough memory to build the menu in. */

	account_complete_menu = heap_alloc(28 + (24 * headers));
	account_complete_menu_group = heap_alloc(headers * sizeof(struct account_list_group));
	account_complete_submenu = heap_alloc(28 + (24 * maxsublen));
	account_complete_submenu_link = heap_alloc(maxsublen * sizeof(struct account_list_link));
	account_complete_menu_title = heap_alloc(ACCOUNT_MENU_TITLE_LEN);

	if (account_complete_menu == NULL || account_complete_menu_group == NULL || account_complete_menu_title == NULL ||
			account_complete_submenu == NULL || account_complete_submenu_link == NULL) {
		account_complete_menu_destroy();
		return NULL;
	}

	/* Populate the menu. */

	line = 0;
	width = 0;
	shade = TRUE;

	for (group = 0; group < groups; group++) {
		if (include & sequence[group]) {
			i = 0;
			entry = account_find_window_entry_from_type(file, sequence[group]);

			/* Start the group with a separator if there are lines in the menu already. */

			if (line > 0)
				account_complete_menu->entries[line-1].menu_flags |= wimp_MENU_SEPARATE;

			while (i < file->account_windows[entry].display_lines) {
				/* If the line is a section header, add it to the menu... */

				if (line < headers && file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER) {
					/* Test for i>0 because if this is the first line of a new entry, the last group of the last entry will
					 * already have been dealt with at the end of the main loop.  shade will be FALSE if there have been any
					 * ACCOUNT_LINE_DATA since the last ACCOUNT_LINE_HEADER.
					 */
					if (shade && line > 0 && i > 0)
						account_complete_menu->entries[line - 1].icon_flags |= wimp_ICON_SHADED;

					shade = TRUE;

					/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
					 * well move while the menu is open.
					 */

					strcpy(account_complete_menu_group[line].name, file->account_windows[entry].line_data[i].heading);
					if (strlen(account_complete_menu_group[line].name) > width)
						width = strlen(account_complete_menu_group[line].name);
					account_complete_menu_group[line].entry = entry;
					account_complete_menu_group[line].start_line = i+1;

					/* Set the menu and icon flags up. */

					account_complete_menu->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

					account_complete_menu->entries[line].sub_menu = account_complete_submenu;
					account_complete_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
							wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
							wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

					/* Set the menu icon contents up. */

					account_complete_menu->entries[line].data.indirected_text.text = account_complete_menu_group[line].name;
					account_complete_menu->entries[line].data.indirected_text.validation = NULL;
					account_complete_menu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

					line++;
				} else if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
					shade = FALSE;

					/* If this is the first line of the list, and it's a data line, there is no group header and a default
					 * group will be required.
					 */

					if (i == 0 && line < headers) {
						switch (sequence[group]) {
						case ACCOUNT_FULL:
							msgs_lookup("ViewaccMenuAccs", account_complete_menu_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						case ACCOUNT_IN:
							msgs_lookup("ViewaccMenuHIn", account_complete_menu_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						case ACCOUNT_OUT:
							msgs_lookup("ViewaccMenuHOut", account_complete_menu_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						default:
							break;
						}
						if (strlen(account_complete_menu_group[line].name) > width)
							width = strlen(account_complete_menu_group[line].name);
						account_complete_menu_group[line].entry = entry;
						account_complete_menu_group[line].start_line = i;

						/* Set the menu and icon flags up. */

						account_complete_menu->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

						account_complete_menu->entries[line].sub_menu = account_complete_submenu;
						account_complete_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
								wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
								wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

						/* Set the menu icon contents up. */

						account_complete_menu->entries[line].data.indirected_text.text = account_complete_menu_group[line].name;
						account_complete_menu->entries[line].data.indirected_text.validation = NULL;
						account_complete_menu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

						line++;
					}
				}

				i++;
			}

			/* Update the maximum submenu length count again. */

			if (shade && line > 0)
				account_complete_menu->entries[line - 1].icon_flags |= wimp_ICON_SHADED;
		}
	}

	/* Finish off the menu, marking the last entry and filling in the header. */

	account_complete_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;
	account_complete_menu->entries[line - 1].menu_flags &= ~wimp_MENU_SEPARATE;

	msgs_lookup(title, account_complete_menu_title, ACCOUNT_MENU_TITLE_LEN);
	account_complete_menu->title_data.indirected_text.text = account_complete_menu_title;
	account_complete_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_complete_menu->title_fg = wimp_COLOUR_BLACK;
	account_complete_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_complete_menu->work_fg = wimp_COLOUR_BLACK;
	account_complete_menu->work_bg = wimp_COLOUR_WHITE;

	account_complete_menu->width = (width + 1) * 16;
	account_complete_menu->height = 44;
	account_complete_menu->gap = 0;

	return account_complete_menu;
}


/**
 * Build a submenu for the Account Complete menu on the fly, using information
 * and memory allocated and assembled in account_complete_menu_build().
 *
 * The memory to hold the menu has been allocated and is pointed to by
 * account_complete_submenu and account_complete_submenu_link; if either of these
 *  are NULL, the fucntion must refuse to run.
 *
 * \param *submenu		The submenu warning message block to use.
 * \return			Pointer to the submenu block, or NULL on failure.
 */

wimp_menu *account_complete_submenu_build(wimp_message_menu_warning *submenu)
{
	int		i, line = 0, entry, width = 0;

	if (account_complete_submenu == NULL || account_complete_submenu_link == NULL ||
			account_complete_menu_file == NULL)
		return NULL;

	entry = account_complete_menu_group[submenu->selection.items[0]].entry;
	i = account_complete_menu_group[submenu->selection.items[0]].start_line;

	while (i < account_complete_menu_file->account_windows[entry].display_lines &&
			account_complete_menu_file->account_windows[entry].line_data[i].type != ACCOUNT_LINE_HEADER) {
		/* If the line is an account entry, add it to the manu... */

		if (account_complete_menu_file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
			/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
			 * well move while the menu is open.
			 */

			strcpy(account_complete_submenu_link[line].name,
					account_complete_menu_file->accounts[account_complete_menu_file->account_windows[entry].line_data[i].account].name);
			if (strlen(account_complete_submenu_link[line].name) > width)
				width = strlen(account_complete_submenu_link[line].name);
			account_complete_submenu_link[line].account = account_complete_menu_file->account_windows[entry].line_data[i].account;

			/* Set the menu and icon flags up. */

			account_complete_submenu->entries[line].menu_flags = 0;

			account_complete_submenu->entries[line].sub_menu = (wimp_menu *) -1;
			account_complete_submenu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			account_complete_submenu->entries[line].data.indirected_text.text = account_complete_submenu_link[line].name;
			account_complete_submenu->entries[line].data.indirected_text.validation = NULL;
			account_complete_submenu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

			line++;
		}

		i++;
	}

	account_complete_submenu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	account_complete_submenu->title_data.indirected_text.text = account_complete_menu_group[submenu->selection.items[0]].name;
	account_complete_submenu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_complete_submenu->title_fg = wimp_COLOUR_BLACK;
	account_complete_submenu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_complete_submenu->work_fg = wimp_COLOUR_BLACK;
	account_complete_submenu->work_bg = wimp_COLOUR_WHITE;

	account_complete_submenu->width = (width + 1) * 16;
	account_complete_submenu->height = 44;
	account_complete_submenu->gap = 0;

	return account_complete_submenu;
}


/**
 * Destroy any Account Complete menu which is currently open.
 */

void account_complete_menu_destroy(void)
{
	if (account_complete_menu != NULL)
		heap_free(account_complete_menu);

	if (account_complete_menu_group != NULL)
		heap_free(account_complete_menu_group);

	if (account_complete_submenu != NULL)
		heap_free(account_complete_submenu);

	if (account_complete_submenu_link != NULL)
		heap_free(account_complete_submenu_link);

	if (account_complete_menu_title != NULL)
		heap_free(account_complete_menu_title);

	account_complete_menu = NULL;
	account_complete_menu_group = NULL;
	account_complete_submenu = NULL;
	account_complete_submenu_link = NULL;
	account_complete_menu_title = NULL;
	account_complete_menu_file = NULL;
}


/**
 * Decode a selection from the Account Complete menu, converting to an account
 * number.
 *
 * \param *selection		The menu selection to decode.
 * \return			The account numer, or NULL_ACCOUNT.
 */

acct_t account_complete_menu_decode(wimp_selection *selection)
{
	if (account_complete_submenu_link == NULL || selection == NULL ||
			selection->items[0] == -1 || selection->items[1] == -1)
		return NULL_ACCOUNT;

	return account_complete_submenu_link[selection->items[1]].account;
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

void account_open_edit_window(file_data *file, acct_t account, enum account_type type, wimp_pointer *ptr)
{
	wimp_w		win = NULL;

	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(account_acc_edit_window))
		wimp_close_window(account_acc_edit_window);

	if (windows_get_open(account_hdg_edit_window))
		wimp_close_window(account_hdg_edit_window);

	if (windows_get_open(account_section_window))
		wimp_close_window(account_section_window);

	/* Select the window to use and set the contents up. */

	if (account == NULL_ACCOUNT) {
		if (type & ACCOUNT_FULL) {
			account_fill_acc_edit_window(file, account);
			win = account_acc_edit_window;

			msgs_lookup("NewAcct", windows_get_indirected_title_addr(win), 50);
			icons_msgs_lookup(win, ACCT_EDIT_OK, "NewAcctAct");
		} else if (type & ACCOUNT_IN || type & ACCOUNT_OUT) {
			account_fill_hdg_edit_window(file, account, type);
			win = account_hdg_edit_window;

			msgs_lookup("NewHdr", windows_get_indirected_title_addr(win), 50);
			icons_msgs_lookup(win, HEAD_EDIT_OK, "NewAcctAct");
		}
	} else {
		if (file->accounts[account].type & ACCOUNT_FULL) {
			account_fill_acc_edit_window(file, account);
			win = account_acc_edit_window;

			msgs_lookup("EditAcct", windows_get_indirected_title_addr(win), 50);
			icons_msgs_lookup(win, ACCT_EDIT_OK, "EditAcctAct");
		} else if (file->accounts[account].type & ACCOUNT_IN || file->accounts[account].type & ACCOUNT_OUT) {
			account_fill_hdg_edit_window(file, account, type);
			win = account_hdg_edit_window;

			msgs_lookup("EditHdr", windows_get_indirected_title_addr(win), 50);
			icons_msgs_lookup(win, HEAD_EDIT_OK, "EditAcctAct");
		}
	}

	/* Set the pointers up so we can find this lot again and open the window. */

	if (win != NULL) {
		edit_account_file = file;
		edit_account_no = account;

		windows_open_centred_at_pointer(win, ptr);
		if (win == account_acc_edit_window)
			place_dialogue_caret (win, ACCT_EDIT_NAME);
		else
			place_dialogue_caret (win, HEAD_EDIT_NAME);
	}
}


/**
 * Process mouse clicks in the Account Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_acc_edit_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ACCT_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_acc_edit_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			account_refresh_acc_edit_window();
		break;

	case ACCT_EDIT_OK:
		if (account_process_acc_edit_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_acc_edit_window);
		break;

	case ACCT_EDIT_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && account_delete_from_edit_window())
			close_dialogue_with_caret(account_acc_edit_window);
		break;
	}
}


/**
 * Process keypresses in the Account Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool account_acc_edit_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (account_process_acc_edit_window())
			close_dialogue_with_caret(account_acc_edit_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(account_acc_edit_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process mouse clicks in the Heading Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_hdg_edit_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case HEAD_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_hdg_edit_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			account_refresh_hdg_edit_window();
		break;

	case HEAD_EDIT_OK:
		if (account_process_hdg_edit_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_hdg_edit_window);
		break;

	case HEAD_EDIT_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && account_delete_from_edit_window())
			close_dialogue_with_caret(account_hdg_edit_window);
		break;
	}
}


/**
 * Process keypresses in the Heading Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool account_hdg_edit_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (account_process_hdg_edit_window())
			close_dialogue_with_caret(account_hdg_edit_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(account_hdg_edit_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Account Edit window.
 */

static void account_refresh_acc_edit_window(void)
{
	account_fill_acc_edit_window(edit_account_file, edit_account_no);
	icons_redraw_group(account_acc_edit_window, 10, ACCT_EDIT_NAME, ACCT_EDIT_IDENT, ACCT_EDIT_CREDIT, ACCT_EDIT_BALANCE,
			ACCT_EDIT_ACCNO, ACCT_EDIT_SRTCD,
			ACCT_EDIT_ADDR1, ACCT_EDIT_ADDR2, ACCT_EDIT_ADDR3, ACCT_EDIT_ADDR4);
	icons_replace_caret_in_window(account_acc_edit_window);
}


/**
 * Refresh the contents of the Heading Edit window.
 */

static void account_refresh_hdg_edit_window(void)
{
	account_fill_hdg_edit_window(edit_account_file, edit_account_no, ACCOUNT_NULL);
	icons_redraw_group(account_hdg_edit_window, 3, HEAD_EDIT_NAME, HEAD_EDIT_IDENT, HEAD_EDIT_BUDGET);
	icons_replace_caret_in_window(account_hdg_edit_window);
}


/**
 * Update the contents of the Account Edit window to reflect the current
 * settings of the given file and account.
 *
 * \param *file			The file to use.
 * \param account		The account to display, or NULL_ACCOUNT for none.
 */

static void account_fill_acc_edit_window(file_data *file, acct_t account)
{
	int	i;

	if (account == NULL_ACCOUNT) {
		*icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_NAME) = '\0';
		*icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_IDENT) = '\0';

		convert_money_to_string(0, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_CREDIT));
		convert_money_to_string(0, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_BALANCE));

		*icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_PAYIN) = '\0';
		*icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_CHEQUE) = '\0';

		*icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_ACCNO) = '\0';
		*icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_SRTCD) = '\0';

		for (i = ACCT_EDIT_ADDR1; i < (ACCT_EDIT_ADDR1 + ACCOUNT_ADDR_LINES); i++)
			 *icons_get_indirected_text_addr(account_acc_edit_window, i) = '\0';

		icons_set_deleted(account_acc_edit_window, ACCT_EDIT_DELETE, 1);
	} else {
		icons_strncpy(account_acc_edit_window, ACCT_EDIT_NAME, file->accounts[account].name);
		icons_strncpy(account_acc_edit_window, ACCT_EDIT_IDENT, file->accounts[account].ident);

		convert_money_to_string(file->accounts[account].credit_limit, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_CREDIT));
		convert_money_to_string(file->accounts[account].opening_balance, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_BALANCE));

		account_get_next_cheque_number(file, NULL_ACCOUNT, account, 0, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_PAYIN),
				icons_get_indirected_text_length(account_acc_edit_window, ACCT_EDIT_PAYIN));
		account_get_next_cheque_number(file, account, NULL_ACCOUNT, 0, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_CHEQUE),
				icons_get_indirected_text_length(account_acc_edit_window, ACCT_EDIT_CHEQUE));

		icons_strncpy(account_acc_edit_window, ACCT_EDIT_ACCNO, file->accounts[account].account_no);
		icons_strncpy(account_acc_edit_window, ACCT_EDIT_SRTCD, file->accounts[account].sort_code);

		for (i = ACCT_EDIT_ADDR1; i < (ACCT_EDIT_ADDR1 + ACCOUNT_ADDR_LINES); i++)
			icons_strncpy(account_acc_edit_window, i, file->accounts[account].address[i-ACCT_EDIT_ADDR1]);

		icons_set_deleted(account_acc_edit_window, ACCT_EDIT_DELETE, 0);
	}
}


/**
 * Update the contents of the Heading Edit window to reflect the current
 * settings of the given file and account.
 *
 * \param *file			The file to use.
 * \param account		The account to display, or NULL_ACCOUNT for none.
 * \param type			The type of heading, if account is NULL_ACCOUNT.
 */

static void account_fill_hdg_edit_window(file_data *file, acct_t account, enum account_type type)
{
	if (account == NULL_ACCOUNT) {
		*icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_NAME) = '\0';
		*icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_IDENT) = '\0';

		convert_money_to_string(0, icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_BUDGET));

		icons_set_shaded(account_hdg_edit_window, HEAD_EDIT_INCOMING, 0);
		icons_set_shaded(account_hdg_edit_window, HEAD_EDIT_OUTGOING, 0);
		icons_set_selected(account_hdg_edit_window, HEAD_EDIT_INCOMING, (type & ACCOUNT_IN) || (type == ACCOUNT_NULL));
		icons_set_selected(account_hdg_edit_window, HEAD_EDIT_OUTGOING, (type & ACCOUNT_OUT));

		icons_set_deleted(account_hdg_edit_window, HEAD_EDIT_DELETE, 1);
	} else {
		icons_strncpy(account_hdg_edit_window, HEAD_EDIT_NAME, file->accounts[account].name);
		icons_strncpy(account_hdg_edit_window, HEAD_EDIT_IDENT, file->accounts[account].ident);

		convert_money_to_string(file->accounts[account].budget_amount, icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_BUDGET));

		icons_set_shaded(account_hdg_edit_window, HEAD_EDIT_INCOMING, 1);
		icons_set_shaded(account_hdg_edit_window, HEAD_EDIT_OUTGOING, 1);
		icons_set_selected(account_hdg_edit_window, HEAD_EDIT_INCOMING, (file->accounts[account].type & ACCOUNT_IN));
		icons_set_selected(account_hdg_edit_window, HEAD_EDIT_OUTGOING, (file->accounts[account].type & ACCOUNT_OUT));

		icons_set_deleted(account_hdg_edit_window, HEAD_EDIT_DELETE, 0);
	}
}


/**
 * Take the contents of an updated Account Edit window and process the data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_process_acc_edit_window(void)
{
	int		check_ident, i, len;

	/* Check if the ident is valid.  It's an account, so check all the possibilities.   If it fails, exit with
	 * an error.
	 */

	check_ident = account_find_by_ident(edit_account_file, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_IDENT),
			ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT);

	if (check_ident != NULL_ACCOUNT && check_ident != edit_account_no) {
		error_msgs_report_error("UsedAcctIdent");
		return FALSE;
	}

	/* If the account doesn't exsit, create it.  Otherwise, copy the standard fields back from the window into
	 * memory.
	 */

	if (edit_account_no == NULL_ACCOUNT) {
		edit_account_no = account_add(edit_account_file, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_NAME),
				icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_IDENT), ACCOUNT_FULL);
	} else {
		strcpy(edit_account_file->accounts[edit_account_no].name, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_NAME));
		strcpy(edit_account_file->accounts[edit_account_no].ident, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_IDENT));
	}

	if (edit_account_no == NULL_ACCOUNT)
		return FALSE;

	/* If the account was created OK, store the rest of the data. */

	edit_account_file->accounts[edit_account_no].opening_balance =
			convert_string_to_money(icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_BALANCE));

	edit_account_file->accounts[edit_account_no].credit_limit =
			convert_string_to_money(icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_CREDIT));

	len = strlen(icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_PAYIN));
	if (len > 0) {
		edit_account_file->accounts[edit_account_no].payin_num_width = len;
		edit_account_file->accounts[edit_account_no].next_payin_num = atoi(icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_PAYIN));
	} else {
		edit_account_file->accounts[edit_account_no].payin_num_width = 0;
		edit_account_file->accounts[edit_account_no].next_payin_num = 0;
	}

	len = strlen (icons_get_indirected_text_addr (account_acc_edit_window, ACCT_EDIT_CHEQUE));
	if (len > 0) {
		edit_account_file->accounts[edit_account_no].cheque_num_width = len;
		edit_account_file->accounts[edit_account_no].next_cheque_num = atoi(icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_CHEQUE));
	} else {
		edit_account_file->accounts[edit_account_no].cheque_num_width = 0;
		edit_account_file->accounts[edit_account_no].next_cheque_num = 0;
	}

	strcpy(edit_account_file->accounts[edit_account_no].account_no, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_ACCNO));
	strcpy(edit_account_file->accounts[edit_account_no].sort_code, icons_get_indirected_text_addr(account_acc_edit_window, ACCT_EDIT_SRTCD));

	for (i = ACCT_EDIT_ADDR1; i < (ACCT_EDIT_ADDR1 + ACCOUNT_ADDR_LINES); i++)
		strcpy(edit_account_file->accounts[edit_account_no].address[i-ACCT_EDIT_ADDR1], icons_get_indirected_text_addr(account_acc_edit_window, i));

	sorder_trial(edit_account_file);
	perform_full_recalculation(edit_account_file);
	accview_recalculate(edit_account_file, edit_account_no, 0);
	force_transaction_window_redraw(edit_account_file, 0, edit_account_file->trans_count - 1);
	refresh_transaction_edit_line_icons(edit_account_file->transaction_window.transaction_window, -1, -1);
	accview_redraw_all(edit_account_file);
	set_file_data_integrity(edit_account_file, 1);

	return TRUE;
}


/**
 * Take the contents of an updated Heading Edit window and process the data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_process_hdg_edit_window(void)
{
	int		check_ident, type;

	/* Check if the ident is valid.  It's a header, so check all full accounts and those headings in the same
	 * category.  If it fails, exit with an error.
	 */

	type = icons_get_selected(account_hdg_edit_window, HEAD_EDIT_INCOMING) ? ACCOUNT_IN : ACCOUNT_OUT;

	check_ident = account_find_by_ident(edit_account_file, icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_IDENT),
			ACCOUNT_FULL | type);

	if (check_ident != NULL_ACCOUNT && check_ident != edit_account_no) {
		error_msgs_report_error("UsedAcctIdent");
		return FALSE;
	}

	/* If the heading doesn't exsit, create it.  Otherwise, copy the standard fields back from the window into
	 * memory.
	 */

	if (edit_account_no == NULL_ACCOUNT) {
		edit_account_no = account_add(edit_account_file, icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_NAME),
				icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_IDENT), type);
	} else {
		strcpy(edit_account_file->accounts[edit_account_no].name, icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_NAME));
		strcpy(edit_account_file->accounts[edit_account_no].ident, icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_IDENT));
	}

	if (edit_account_no == NULL_ACCOUNT)
		return FALSE;

	/* If the heading was created OK, store the rest of the data. */

	edit_account_file->accounts[edit_account_no].budget_amount = convert_string_to_money(icons_get_indirected_text_addr(account_hdg_edit_window, HEAD_EDIT_BUDGET));

	/* Tidy up and redraw the windows */

	perform_full_recalculation(edit_account_file);
	force_transaction_window_redraw(edit_account_file, 0, edit_account_file->trans_count - 1);
	refresh_transaction_edit_line_icons(edit_account_file->transaction_window.transaction_window, -1, -1);
	accview_redraw_all(edit_account_file);
	set_file_data_integrity(edit_account_file, 1);

	return TRUE;
}


/**
 * Delete the account associated with the currently open Account or Heading
 * Edit window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_delete_from_edit_window(void)
{
	if (account_used_in_file(edit_account_file, edit_account_no)) {
		error_msgs_report_info("CantDelAcct");
		return FALSE;
	}

	if (error_msgs_report_question("DeleteAcct", "DeleteAcctB") == 2)
		return FALSE;

	return account_delete(edit_account_file, edit_account_no);
}


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * If account == NULL_ACCOUNT, type determines the type of the new account
 * to be created.  Otherwise, type is ignored and the type derived from the
 * account data block.
 *
 * \param *file			The file to own the dialogue.
 * \param entry			The entry of the window owning the dialogue.
 * \param line			The line to be editied, or -1 for none.
 * \param *ptr			The current Wimp pointer position.
 */

static void account_open_section_window(file_data *file, int entry, int line, wimp_pointer *ptr)
{
	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(account_acc_edit_window))
		wimp_close_window(account_acc_edit_window);

	if (windows_get_open(account_hdg_edit_window))
		wimp_close_window(account_hdg_edit_window);

	if (windows_get_open(account_section_window))
		wimp_close_window(account_section_window);

	/* Select the window to use and set the contents up. */

	if (line == -1) {
		account_fill_section_window(file, entry, line);

		msgs_lookup("NewSect", windows_get_indirected_title_addr(account_section_window), 50);
		icons_msgs_lookup(account_section_window, SECTION_EDIT_OK, "NewAcctAct");
	} else {
		account_fill_section_window(file, entry, line);

		msgs_lookup("EditSect", windows_get_indirected_title_addr(account_section_window), 50);
		icons_msgs_lookup(account_section_window, SECTION_EDIT_OK, "EditAcctAct");
	}

	/* Set the pointers up so we can find this lot again and open the window. */

	account_section_file = file;
	account_section_entry = entry;
	account_section_line = line;

	windows_open_centred_at_pointer(account_section_window, ptr);
	place_dialogue_caret(account_section_window, SECTION_EDIT_TITLE);
}


/**
 * Process mouse clicks in the Section Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_section_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case SECTION_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_section_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			account_refresh_section_window();
		break;

	case SECTION_EDIT_OK:
		if (account_process_section_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_section_window);
		break;

	case SECTION_EDIT_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && account_delete_from_section_window())
			close_dialogue_with_caret(account_section_window);
		break;
	}
}


/**
 * Process keypresses in the Section Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool account_section_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (account_process_section_window())
			close_dialogue_with_caret(account_section_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(account_section_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Section Edit window.
 */

static void account_refresh_section_window(void)
{
	account_fill_section_window(account_section_file, account_section_entry, account_section_line);
	icons_redraw_group(account_section_window, 1, SECTION_EDIT_TITLE);
	icons_replace_caret_in_window(account_section_window);
}


/**
 * Update the contents of the Section Edit window to reflect the current
 * settings of the given file and section.
 *
 * \param *file			The file to use.
 * \param entry			The window entry owning the dialogue.
 * \param line			The line to be displayed, or -1 for none.
 */

static void account_fill_section_window(file_data *file, int entry, int line)
{
	if (line == -1) {
		*icons_get_indirected_text_addr(account_section_window, SECTION_EDIT_TITLE) = '\0';

		icons_set_selected(account_section_window, SECTION_EDIT_HEADER, 1);
		icons_set_selected(account_section_window, SECTION_EDIT_FOOTER, 0);
	} else {
		icons_strncpy(account_section_window, SECTION_EDIT_TITLE, file->account_windows[entry].line_data[line].heading);

		icons_set_selected(account_section_window, SECTION_EDIT_HEADER,
				(file->account_windows[entry].line_data[line].type == ACCOUNT_LINE_HEADER));
		icons_set_selected(account_section_window, SECTION_EDIT_FOOTER,
				(file->account_windows[entry].line_data[line].type == ACCOUNT_LINE_FOOTER));
	}

	icons_set_deleted(account_section_window, SECTION_EDIT_DELETE, line == -1);
}


/**
 * Take the contents of an updated Section Edit window and process the data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_process_section_window(void)
{
	/* If the section doesn't exsit, create it.  Otherwise, copy the standard fields back from the window into
	 * memory.
	 */

	if (account_section_line == -1) {
		account_section_line = account_add_list_display_line(account_section_file, account_section_entry);

		if (account_section_line == -1) {
			error_msgs_report_error("NoMemNewSect");
			return FALSE;
		}
	}

	strcpy(account_section_file->account_windows[account_section_entry].line_data[account_section_line].heading,
			icons_get_indirected_text_addr(account_section_window, SECTION_EDIT_TITLE));

	if (icons_get_selected(account_section_window, SECTION_EDIT_HEADER))
		account_section_file->account_windows[account_section_entry].line_data[account_section_line].type = ACCOUNT_LINE_HEADER;
	else if (icons_get_selected(account_section_window, SECTION_EDIT_FOOTER))
		account_section_file->account_windows[account_section_entry].line_data[account_section_line].type = ACCOUNT_LINE_FOOTER;
	else
		account_section_file->account_windows[account_section_entry].line_data[account_section_line].type = ACCOUNT_LINE_BLANK;

	/* Tidy up and redraw the windows */

	perform_full_recalculation(account_section_file);
	account_set_window_extent(account_section_file, account_section_entry);
	windows_open(account_section_file->account_windows[account_section_entry].account_window);
	account_force_window_redraw(account_section_file, account_section_entry,
			0, account_section_file->account_windows[account_section_entry].display_lines);
	set_file_data_integrity(account_section_file, 1);

	return TRUE;
}


/**
 * Delete the section associated with the currently open Section Edit
 * window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_delete_from_section_window(void)
{
	if (error_msgs_report_question("DeleteSection", "DeleteSectionB") == 2)
		return FALSE;

	/* Delete the heading */

	flex_midextend((flex_ptr) &(account_section_file->account_windows[account_section_entry].line_data),
			(account_section_line + 1) * sizeof(struct account_redraw), -sizeof(struct account_redraw));
	account_section_file->account_windows[account_section_entry].display_lines--;

	/* Update the accounts display window. */

	account_set_window_extent(account_section_file, account_section_entry);
	windows_open(account_section_file->account_windows[account_section_entry].account_window);
	account_force_window_redraw(account_section_file, account_section_entry,
			0, account_section_file->account_windows[account_section_entry].display_lines);
	set_file_data_integrity(account_section_file, 1);

	return TRUE;
}


/**
 * Force the closure of the Account, Heading and Section Edit windows if the
 * owning file disappears.
 *
 * \param *file			The file which has closed.
 */

void account_force_windows_closed(file_data *file)
{
	if (edit_account_file == file) {
		if (windows_get_open(account_acc_edit_window))
			close_dialogue_with_caret(account_acc_edit_window);

		if (windows_get_open(account_hdg_edit_window))
			close_dialogue_with_caret(account_hdg_edit_window);
	}

	if (account_section_file == file && windows_get_open(account_section_window))
		close_dialogue_with_caret(account_section_window);
}


/**
 * Open the Account Print dialogue for a given account list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to retain the previous settings; FALSE to
 *				return to defaults.
 */

static void account_open_print_window(file_data *file, enum account_type type, wimp_pointer *ptr, osbool restore)
{
	/* Set the pointers up so we can find this lot again and open the window. */

	account_print_file = file;
	account_print_type = type;

	if (type & ACCOUNT_FULL)
		printing_open_simple_window(file, ptr, restore, "PrintAcclistAcc", account_print);
	else if (type & ACCOUNT_IN || type & ACCOUNT_OUT)
		printing_open_simple_window(file, ptr, restore, "PrintAcclistHead", account_print);
}


/**
 * Send the contents of the Account Window to the printer, via the reporting
 * system.
 *
 * \param text			TRUE to print in text format; FALSE for graphics.
 * \param format		TRUE to apply text formatting in text mode.
 * \param scale			TRUE to scale width in graphics mode.
 * \param rotate		TRUE to print landscape in grapics mode.
 * \param pagenum		TRUE to include page numbers in graphics mode.
 */

static void account_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum)
{
	report_data		*report;
	int			i, entry;
	char			line[4096], buffer[256], numbuf1[64], numbuf2[64], numbuf3[64], numbuf4[64];
	struct account_window	*window;

	msgs_lookup((account_print_type & ACCOUNT_FULL) ? "PrintTitleAcclistAcc" : "PrintTitleAcclistHead", buffer, sizeof(buffer));
	report = report_open(account_print_file, buffer, NULL);

	if (report == NULL)
		return;

	hourglass_on();

	entry = account_find_window_entry_from_type(account_print_file, account_print_type);
	window = &(account_print_file->account_windows[entry]);

	/* Output the page title. */

	make_file_leafname(account_print_file, numbuf1, sizeof(numbuf1));
	switch (window->type) {
	case ACCOUNT_FULL:
		msgs_param_lookup("AcclistTitleAcc", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
		break;

	case ACCOUNT_IN:
		msgs_param_lookup("AcclistTitleHIn", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
		break;

	case ACCOUNT_OUT:
		msgs_param_lookup("AcclistTitleHOut", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
		break;

	default:
		break;
	}
	snprintf(line, sizeof(line), "\\b\\u%s", buffer);
	report_write_line(report, 0, line);

	if (account_print_file->budget.start != NULL_DATE || account_print_file->budget.finish != NULL_DATE) {
		*line = '\0';
		msgs_lookup("AcclistBudgetTitle", buffer, sizeof(buffer));
		strcat(line, buffer);

		if (account_print_file->budget.start != NULL_DATE) {
			convert_date_to_string(account_print_file->budget.start, numbuf1);
			msgs_param_lookup("AcclistBudgetFrom", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
			strcat(line, buffer);
		}

		if (account_print_file->budget.finish != NULL_DATE) {
			convert_date_to_string(account_print_file->budget.finish, numbuf1);
			msgs_param_lookup("AcclistBudgetTo", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
			strcat(line, buffer);
		}

		strcat(line, ".");

		report_write_line(report, 0, line);
	}

	report_write_line(report, 0, "");

	/* Output the headings line, taking the text from the window icons. */

	*line = '\0';
	snprintf(buffer, sizeof(buffer), "\\k\\b\\u%s\\t\\s\\t", icons_copy_text(window->account_pane, 0, numbuf1));
	strcat(line, buffer);
	snprintf(buffer, sizeof(buffer), "\\b\\u\\r%s\\t", icons_copy_text(window->account_pane, 1, numbuf1));
	strcat(line, buffer);
	snprintf(buffer, sizeof(buffer), "\\b\\u\\r%s\\t", icons_copy_text(window->account_pane, 2, numbuf1));
	strcat(line, buffer);
	snprintf(buffer, sizeof(buffer), "\\b\\u\\r%s\\t", icons_copy_text(window->account_pane, 3, numbuf1));
	strcat(line, buffer);
	snprintf(buffer, sizeof(buffer), "\\b\\u\\r%s", icons_copy_text(window->account_pane, 4, numbuf1));
	strcat(line, buffer);

	report_write_line(report, 0, line);

	/* Output the account data as a set of delimited lines. */
	/* Output the transaction data as a set of delimited lines. */

	for (i=0; i < window->display_lines; i++) {
		*line = '\0';

		if (window->line_data[i].type == ACCOUNT_LINE_DATA) {
			account_build_name_pair(account_print_file, window->line_data[i].account, buffer, sizeof(buffer));

			switch (window->type) {
			case ACCOUNT_FULL:
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].statement_balance, numbuf1);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].current_balance, numbuf2);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].trial_balance, numbuf3);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].budget_balance, numbuf4);
				break;

			case ACCOUNT_IN:
				convert_money_to_string(-account_print_file->accounts[window->line_data[i].account].future_balance, numbuf1);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].budget_amount, numbuf2);
				convert_money_to_string(-account_print_file->accounts[window->line_data[i].account].budget_balance, numbuf3);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].budget_result, numbuf4);
				break;

			case ACCOUNT_OUT:
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].future_balance, numbuf1);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].budget_amount, numbuf2);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].budget_balance, numbuf3);
				convert_money_to_string(account_print_file->accounts[window->line_data[i].account].budget_result, numbuf4);
				break;

			default:
				break;
			}
			snprintf(line, sizeof(line), "\\k%s\\t%s\\t\\r%s\\t\\r%s\\t\\r%s\\t\\r%s",
					account_get_ident(account_print_file, window->line_data[i].account),
			account_get_name(account_print_file, window->line_data[i].account),
					numbuf1, numbuf2, numbuf3, numbuf4);
		} else if (window->line_data[i].type == ACCOUNT_LINE_HEADER) {
			snprintf(line, sizeof(line), "\\k\\u%s", window->line_data[i].heading);
		} else if (window->line_data[i].type == ACCOUNT_LINE_FOOTER) {
			convert_money_to_string(window->line_data[i].total[0], numbuf1);
			convert_money_to_string(window->line_data[i].total[1], numbuf2);
			convert_money_to_string(window->line_data[i].total[2], numbuf3);
			convert_money_to_string(window->line_data[i].total[3], numbuf4);

			snprintf(line, sizeof(line), "\\k%s\\t\\s\\t\\r\\b%s\\t\\r\\b%s\\t\\r\\b%s\\t\\r\\b%s",
					window->line_data[i].heading, numbuf1, numbuf2, numbuf3, numbuf4);
		}

		report_write_line(report, 0, line);
	}

	/* Output the grand total line, taking the text from the window icons. */

	icons_copy_text(window->account_footer, 0, buffer);
	snprintf(line, sizeof(line), "\\k\\u%s\\t\\s\\t\\r%s\\t\\r%s\\t\\r%s\\t\\r%s", buffer,
			window->footer_icon[0], window->footer_icon[1], window->footer_icon[2], window->footer_icon[3]);
	report_write_line(report, 0, line);

	hourglass_off();

	report_close_and_print(report, text, format, scale, rotate, pagenum);
}

















/**
 * Create a new account with null details.  Core details are set up, but some
 * values are zeroed and left to be set up later.
 *
 * \param *file			The file to add the account to.
 * \param *name			The name to give the account.
 * \param *ident		The ident to give the account.
 * \param type			The type of account to be created.
 * \return			The new account index, or NULL_ACCOUNT.
 */

acct_t account_add(file_data *file, char *name, char *ident, enum account_type type)
{
	acct_t		new;
	int		i;

	new = NULL_ACCOUNT;

	if (strcmp(ident, "") == 0) {
		error_msgs_report_error("BadAcctIdent");
		return new;
	}

	/* First, look for deleted accounts and re-use the first one found. */

	for (i = 0; i < file->account_count; i++) {
		if (file->accounts[i].type == ACCOUNT_NULL) {
			new = i;
			#ifdef DEBUG
			debug_printf("Found empty account: %d", new);
			#endif
			break;
		}
	}

	/* If that fails, create a new entry. */

	if (new == NULL_ACCOUNT) {
		if (flex_extend((flex_ptr) &(file->accounts), sizeof(account) * (file->account_count+1)) == 1) {
			new = file->account_count++;
			#ifdef DEBUG
			debug_printf("Created new account: %d", new);
			#endif
		}
	}

	/* If a new account was created, fill it. */

	if (new == NULL_ACCOUNT) {
		error_msgs_report_error("NoMemNewAcct");
		return new;
	}

	strcpy(file->accounts[new].name, name);
	strcpy(file->accounts[new].ident, ident);
	file->accounts[new].type = type;
	file->accounts[new].opening_balance = 0;
	file->accounts[new].credit_limit = 0;
	file->accounts[new].budget_amount = 0;
	file->accounts[new].next_payin_num = 0;
	file->accounts[new].payin_num_width = 0;
	file->accounts[new].next_cheque_num = 0;
	file->accounts[new].cheque_num_width = 0;

	*file->accounts[new].account_no = '\0';
	*file->accounts[new].sort_code = '\0';
	for (i=0; i<ACCOUNT_ADDR_LINES; i++)
		*file->accounts[new].address[i] = '\0';

	file->accounts[new].account_view = NULL;

	account_add_to_lists(file, new);
	update_transaction_window_toolbar(file);

	return new;
}


/**
 * Add an account to the approprite account lists, if it isn't already
 * in them.
 *
 * \param *file			The file containing the account.
 * \param account		The account to process.
 */

static void account_add_to_lists(file_data *file, acct_t account)
{
	int	entry, line;

	entry = account_find_window_entry_from_type(file, file->accounts[account].type);

	if (entry == -1)
		return;

	line = account_add_list_display_line(file, entry);

	if (line == -1) {
		error_msgs_report_error("NoMemLinkAcct");
		return;
	}

	file->account_windows[entry].line_data[line].type = ACCOUNT_LINE_DATA;
	file->account_windows[entry].line_data[line].account = account;

	/* If the target window is open, change the extent as necessary. */

	account_set_window_extent(file, entry);
}


/**
 * Create a new display line block at the end of the given list, fill it with
 * blank data and return the number.
 *
 * \param *file			The file containing the list.
 * \param entry			The list to add an entry to.
 * \return			The new line, or -1 on failure.
 */

static int account_add_list_display_line(file_data *file, int entry)
{
	int	line;

	line = -1;

	if (flex_extend((flex_ptr) &(file->account_windows[entry].line_data),
			sizeof(struct account_redraw) * ((file->account_windows[entry].display_lines)+1)) == 1) {
		line = file->account_windows[entry].display_lines++;

		#ifdef DEBUG
		debug_printf("Creating new display line %d", line);
		#endif

		file->account_windows[entry].line_data[line].type = ACCOUNT_LINE_BLANK;
		file->account_windows[entry].line_data[line].account = NULL_ACCOUNT;

		*file->account_windows[entry].line_data[line].heading = '\0';
	}

	return line;
}


/**
 * Delete an account from a file.
 *
 * \param *file			The file to act on.
 * \param account		The account to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

osbool account_delete(file_data *file, acct_t account)
{
	int	i, j;

	/* Delete the entry from the listing windows. */

	#ifdef DEBUG
	debug_printf("Trying to delete account %d", account);
	#endif

	if (account_used_in_file(file, account))
		return FALSE;

	for (i = 0; i < ACCOUNT_WINDOWS; i++) {
		for (j = file->account_windows[i].display_lines-1; j >= 0; j--) {
			if (file->account_windows[i].line_data[j].type == ACCOUNT_LINE_DATA &&
					file->account_windows[i].line_data[j].account == account) {
				#ifdef DEBUG
				debug_printf("Deleting entry type %x", file->account_windows[i].line_data[j].type);
				#endif

				flex_midextend((flex_ptr) &(file->account_windows[i].line_data),
						(j + 1) * sizeof(struct account_redraw), -sizeof(struct account_redraw));
				file->account_windows[i].display_lines--;
				j--; /* Take into account that the array has just shortened. */
			}
		}

		account_set_window_extent (file, i);
		if (file->account_windows[i].account_window != NULL) {
			windows_open(file->account_windows[i].account_window);
			account_force_window_redraw(file, i, 0, file->account_windows[i].display_lines);
		}
	}

	/* Close the account view window. */

	if (file->accounts[account].account_view != NULL)
		accview_delete_window(file, account);

	/* Remove the account from any report templates. */

	analysis_remove_account_from_templates(file, account);

	/* Blank out the account. */

	file->accounts[account].type = ACCOUNT_NULL;

	/* Update the transaction window toolbar. */

	update_transaction_window_toolbar(file);
	set_file_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Find the account window entry index which corresponds to a given account type.
 *
 * \TODO -- Depending upon how we set the array indicies up, this could probably
 *          be replaced by a simple lookup?
 *
 * \param *file			The file to use.
 * \param type			The account type to find the entry for.
 * \return			The corresponding index, or -1 if not found.
 */

int account_find_window_entry_from_type(file_data *file, enum account_type type)
{
	int	i, entry = -1;

	if (file == NULL)
		return entry;

	/* Find the window block to use. */

	for (i = 0; i < ACCOUNT_WINDOWS; i++)
		if (file->account_windows[i].type == type)
			entry = i;

	return entry;
}


/**
 * Find an account by looking up an ident string against accounts of a
 * given type.
 *
 * \param *file			The file containing the account.
 * \param *ident		The ident to look up.
 * \param type			The type(s) of account to include.
 * \return			The account number, or NULL_ACCOUNT if not found.
 */

acct_t account_find_by_ident(file_data *file, char *ident, enum account_type type)
{
	int account = 0;

	while ((account < file->account_count) &&
			((string_nocase_strcmp(ident, file->accounts[account].ident) != 0) || ((file->accounts[account].type & type) == 0)))
		account++;

	if (account == file->account_count)
		account = NULL_ACCOUNT;

	return account;
}


/**
 * Return a pointer to a string repesenting the ident of an account, or ""
 * if the account is not valid.
 *
 * \param *file			The file containing the account.
 * \param account		The account to return an ident for.
 * \return			Pointer to the ident string, or "".
 */

char *account_get_ident(file_data *file, acct_t account)
{
	if (file == NULL || account == NULL_ACCOUNT || account >= file->account_count || file->accounts[account].type == ACCOUNT_NULL)
		return "";

	return file->accounts[account].ident;
}


/**
 * Return a pointer to a string repesenting the name of an account, or ""
 * if the account is not valid.
 *
 * \param *file			The file containing the account.
 * \param account		The account to return an name for.
 * \return			Pointer to the name string, or "".
 */

char *account_get_name(file_data *file, acct_t account)
{
	if (file == NULL || account == NULL_ACCOUNT || account >= file->account_count || file->accounts[account].type == ACCOUNT_NULL)
		return "";

	return file->accounts[account].name;
}


/**
 * Build a textual "Ident:Account Name" pair for the given account, and
 * insert it into the supplied buffer.
 *
 * \param *file			The file containing the account.
 * \param account		The account to give a name to.
 * \param *buffer		The buffer to take the Ident:Name.
 * \param size			The number of bytes in the buffer.
 * \return			Pointer to the start of the ident.
 */

char *account_build_name_pair(file_data *file, acct_t account, char *buffer, size_t size)
{
	*buffer = '\0';

	if (file != NULL && account != NULL_ACCOUNT && account < file->account_count && file->accounts[account].type != ACCOUNT_NULL)
		snprintf(buffer, size, "%s:%s", account_get_ident(file, account), account_get_name(file, account));

	return buffer;
}





/* ================================================================================================================== */

int lookup_account_field (file_data *file, char key, int type, int account, int *reconciled,
                          wimp_w window, wimp_i ident, wimp_i name, wimp_i rec)
{
  int new_rec = 0;
  char *ident_ptr, *name_ptr, *rec_ptr;


  /* Look up the text fields for the icons. */

  ident_ptr = icons_get_indirected_text_addr (window, ident);
  name_ptr = icons_get_indirected_text_addr (window, name);
  rec_ptr = icons_get_indirected_text_addr (window, rec);

  /* If the character is an alphanumeric or a delete, look up the ident as it stends. */

  if (isalnum (key) || iscntrl (key))
  {
    /* Look up the account number based on the ident. */

    account = account_find_by_ident (file, ident_ptr, type);

    /* Copy the corresponding name into the name field. */

    strcpy (name_ptr, account_get_name (file, account));
    wimp_set_icon_state (window, name, 0, 0);

    /* Do the auto-reconciliation. */

    if (account != NULL_ACCOUNT && !(file->accounts[account].type & ACCOUNT_FULL))
    {
      /* If the account exists, and it is a heading (ie. it isn't a full account), reconcile it... */

      new_rec = 1;
      msgs_lookup ("RecChar", rec_ptr, REC_FIELD_LEN);
      wimp_set_icon_state (window, rec, 0, 0);
    }
    else
    {
      /* ...otherwise unreconcile it. */

      new_rec = 0;
      *rec_ptr = '\0';
      wimp_set_icon_state (window, rec, 0, 0);
    }
  }

  /* If the key pressed was a reconcile one, set or clear the bit as required. */

  if (key == '+' || key == '=')
  {
    new_rec = 1;
    msgs_lookup ("RecChar", rec_ptr, REC_FIELD_LEN);
    wimp_set_icon_state (window, rec, 0, 0);
  }

  if (key == '-' || key == '_')
  {
    new_rec = 0;
    *rec_ptr = '\0';
    wimp_set_icon_state (window, rec, 0, 0);
  }

  /* Return the new reconciled state if applicable. */

  if (reconciled != NULL)
  {
    *reconciled = new_rec;
  }

  return (account);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Fill three icons with account name, ident and reconciled status. */

void fill_account_field (file_data *file, acct_t account, int reconciled,
                         wimp_w window, wimp_i ident, wimp_i name, wimp_i rec_field)
{
  strcpy (icons_get_indirected_text_addr (window, ident), account_get_ident (file, account));

  if (reconciled)
  {
    msgs_lookup ("RecChar", icons_get_indirected_text_addr (window, rec_field), REC_FIELD_LEN);
  }
  else
  {
    *icons_get_indirected_text_addr (window, rec_field) = '\0';
  }
  strcpy (icons_get_indirected_text_addr (window, name), account_get_name (file, account));
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Toggle the reconcile status in an icon. */

void toggle_account_reconcile_icon (wimp_w window, wimp_i icon)
{
  if (*icons_get_indirected_text_addr (window, icon) == '\0')
  {
    msgs_lookup ("RecChar", icons_get_indirected_text_addr (window, icon), REC_FIELD_LEN);
  }
  else
  {
    *icons_get_indirected_text_addr (window, icon) = '\0';
  }

  wimp_set_icon_state (window, icon, 0, 0);
}

/* ================================================================================================================== */


/* ================================================================================================================== */

/**
 * Check if an account is used in anywhere in a file.
 *
 * \param *file			The file to check.
 * \param account		The account to check for.
 * \return			TRUE if the account is found; else FALSE.
 */

osbool account_used_in_file(file_data *file, acct_t account)
{
	osbool		found = FALSE;

	if (transact_check_account(file, account))
		found = TRUE;

	if (sorder_check_account(file, account))
		found = TRUE;

	if (preset_check_account(file, account))
		found = TRUE;

	return found;
}


/**
 * Count the number of accounts of a given type in a file.
 *
 * \param *file			The account to use.
 * \param type			The type of account to count.
 * \return			The number of accounts found.
 */

int account_count_type_in_file(file_data *file, enum account_type type)
{
	int	i, accounts = 0;

	if (file == NULL)
		return accounts;

	for (i = 0; i < file->account_count; i++)
		if ((file->accounts[i].type & type) != 0)
			accounts++;

	return accounts;
}


/**
 * Start an account window drag, to re-order the entries in the window.
 *
 * \param *file			The file owning the Account List being dragged.
 * \param entry			The entry of the Account List being dragged.
 * \param line			The line of the Account list being dragged.
 */

static void start_account_drag(file_data *file, int entry, int line)
{
	wimp_window_state	window;
	wimp_auto_scroll_info	auto_scroll;
	wimp_drag		drag;
	int			ox, oy;

	/* The drag is not started if any of the account window edit dialogues
	 * are open, as these will have pointers into the data which won't like
	 * the data moving beneath them.
	 */

	if (windows_get_open(account_acc_edit_window) || windows_get_open(account_hdg_edit_window) || windows_get_open (account_section_window))
		return;

	/* Get the basic information about the window. */

	window.w = file->account_windows[entry].account_window;
	wimp_get_window_state(&window);

	ox = window.visible.x0 - window.xscroll;
	oy = window.visible.y1 - window.yscroll;

	/* Set up the drag parameters. */

	drag.w = file->account_windows[entry].account_window;
	drag.type = wimp_DRAG_USER_FIXED;

	drag.initial.x0 = ox;
	drag.initial.y0 = oy + -(line * (ICON_HEIGHT+LINE_GUTTER) + ACCOUNT_TOOLBAR_HEIGHT + ICON_HEIGHT);
	drag.initial.x1 = ox + (window.visible.x1 - window.visible.x0);
	drag.initial.y1 = oy + -(line * (ICON_HEIGHT+LINE_GUTTER) + ACCOUNT_TOOLBAR_HEIGHT);

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
		auto_scroll.w = file->account_windows[entry].account_window;
		auto_scroll.pause_zone_sizes.x0 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.y0 = AUTO_SCROLL_MARGIN + ACCOUNT_FOOTER_HEIGHT;
		auto_scroll.pause_zone_sizes.x1 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.y1 = AUTO_SCROLL_MARGIN + ACCOUNT_TOOLBAR_HEIGHT;
		auto_scroll.pause_duration = 0;
		auto_scroll.state_change = (void *) 1;

		wimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_HORIZONTAL | wimp_AUTO_SCROLL_ENABLE_VERTICAL, &auto_scroll);
	}

	account_dragging_file = file;
	account_dragging_start_line = line;
	account_dragging_entry = entry;

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

	/* Terminate the drag and end the autoscroll. */

	if (xos_swi_number_from_string("Wimp_AutoScroll", NULL) == NULL)
		wimp_auto_scroll(0, NULL);

	if (account_dragging_sprite)
		dragasprite_stop();

	/* Get the line at which the drag ended. */

	wimp_get_pointer_info(&pointer);

	window.w = account_dragging_file->account_windows[account_dragging_entry].account_window;
	wimp_get_window_state(&window);

	line = ((window.visible.y1 - pointer.pos.y) - window.yscroll - ACCOUNT_TOOLBAR_HEIGHT)
			/ (ICON_HEIGHT+LINE_GUTTER);

	if (line < 0)
		line = 0;

	if (line >= account_dragging_file->account_windows[account_dragging_entry].display_lines)
		line = account_dragging_file->account_windows[account_dragging_entry].display_lines - 1;

	/* Move the blocks around. */

	block =  account_dragging_file->account_windows[account_dragging_entry].line_data[account_dragging_start_line];

	if (line < account_dragging_start_line) {
		memmove(&(account_dragging_file->account_windows[account_dragging_entry].line_data[line+1]),
				&(account_dragging_file->account_windows[account_dragging_entry].line_data[line]),
				(account_dragging_start_line - line) * sizeof(struct account_redraw));

		account_dragging_file->account_windows[account_dragging_entry].line_data[line] = block;
	} else if (line > account_dragging_start_line) {
		memmove(&(account_dragging_file->account_windows[account_dragging_entry].line_data[account_dragging_start_line]),
				&(account_dragging_file->account_windows[account_dragging_entry].line_data[account_dragging_start_line+1]),
				(line - account_dragging_start_line) * sizeof(struct account_redraw));

		account_dragging_file->account_windows[account_dragging_entry].line_data[line] = block;
	}

	/* Tidy up and redraw the windows */

	perform_full_recalculation(account_dragging_file);
	set_file_data_integrity(account_dragging_file, 1);
	account_force_window_redraw(account_dragging_file, account_dragging_entry,
			0, account_dragging_file->account_windows[account_dragging_entry].display_lines - 1);

	#ifdef DEBUG
	debug_printf("Move account from line %d to line %d", account_dragging_start_line, line);
	#endif
}


/**
 * Get the next cheque or paying in book number for a given combination of from and
 * to accounts, storing the value as a string in the buffer and incrementing the
 * next value for the chosen account as specified.
 *
 * \param *file			The file containing the accounts.
 * \param from_account		The account to transfer from.
 * \param to_account		The account to transfer to.
 * \param increment		The amount to increment the chosen number after
 *				use (0 for no increment).
 * \param *buffer		Pointer to the buffer to hold the number.
 * \param size			The size of the buffer.
 * \return			A pointer to the supplied buffer, containing the
 *				next available number.
 */

char *account_get_next_cheque_number(file_data *file, acct_t from_account, acct_t to_account, int increment, char *buffer, size_t size)
{
	char		format[32], mbuf[1024], bbuf[128];
	osbool		from_ok, to_ok;

	/* Test which of the two accounts have an auto-reference attached.  If
	 * both do, the user needs to be asked which one to use in the transaction.
	 */

	from_ok = (from_account != NULL_ACCOUNT && file->accounts[from_account].cheque_num_width > 0) ? TRUE: FALSE;
	to_ok = (to_account != NULL_ACCOUNT && file->accounts[to_account].payin_num_width > 0) ? TRUE : FALSE;

	if (from_ok && to_ok) {
		msgs_param_lookup("ChqOrPayIn", mbuf, sizeof(mbuf),
				file->accounts[to_account].name, file->accounts[from_account].name, NULL, NULL);
		msgs_lookup("ChqOrPayInB", bbuf, sizeof(bbuf));

		if (error_report_question(mbuf, bbuf) == 1)
			to_ok = FALSE;
		else
			from_ok = FALSE;
	}

	/* Now process the reference. */

	if (from_ok) {
		snprintf(format, sizeof(format), "%%0%dd", file->accounts[from_account].cheque_num_width);
		snprintf(buffer, size, format, file->accounts[from_account].next_cheque_num);
		file->accounts[from_account].next_cheque_num += increment;
	} else if (to_ok) {
		snprintf(format, sizeof(format), "%%0%dd", file->accounts[to_account].payin_num_width);
		snprintf(buffer, size, format, file->accounts[to_account].next_payin_num);
		file->accounts[to_account].next_payin_num += increment;
	} else {
		*buffer = '\0';
	}

	return buffer;
}







/**
 * Read account details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param format		The format number of the file.
  * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

int account_read_acct_file(file_data *file, FILE *in, char *section, char *token, char *value, int format, osbool *unknown_data)
{
	int	result, block_size, i = -1, j;

	block_size = flex_size((flex_ptr) &(file->accounts)) / sizeof(account);

	do {
		if (string_nocase_strcmp(token, "Entries") == 0) {
			block_size = strtoul(value, NULL, 16);
			if (block_size > file->account_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->accounts), sizeof(account) * block_size);
			} else {
				block_size = file->account_count;
			}
		} else if (string_nocase_strcmp(token, "WinColumns") == 0) {
			/* For file format 1.00 or older, there's no row column at the
			 * start of the line so skip on to colukn 1 (date).
			 */
			column_init_window(file->accview_column_width, file->accview_column_position,
					ACCVIEW_COLUMNS, (format <= 100) ? 1 : 0, TRUE, value);
		} else if (string_nocase_strcmp(token, "SortOrder") == 0) {
			file->accview_sort_order = strtoul(value, NULL, 16);
		} else if (string_nocase_strcmp(token, "@") == 0) {
			/* A new account.  Take the account number, and see if it falls within the current defined set of
			 * accounts (not the same thing as the pre-expanded account block.  If not, expand the acconut_count
			 * to the new account number and blank all the new entries.
			 */

			i = strtoul(next_field(value, ','), NULL, 16);

			if (i >= file->account_count) {
				j = file->account_count;
				file->account_count = i+1;

				#ifdef DEBUG
				debug_printf("Account range expanded to %d", i);
				#endif

				/* The block isn't big enough, so expand this to the required size. */

				if (file->account_count > block_size) {
					block_size = file->account_count;
					#ifdef DEBUG
					debug_printf("Section block expand to %d", block_size);
					#endif
					flex_extend((flex_ptr) &(file->accounts), sizeof(account) * block_size);
				}

				/* Blank all the intervening entries. */

				while (j < file->account_count) {
					#ifdef DEBUG
					debug_printf("Blanking account entry %d", j);
					#endif

					*(file->accounts[j].name) = '\0';
					*(file->accounts[j].ident) = '\0';
					file->accounts[j].type = ACCOUNT_NULL;
					file->accounts[j].opening_balance = 0;
					file->accounts[j].credit_limit = 0;
					file->accounts[j].budget_amount = 0;
					file->accounts[j].cheque_num_width = 0;
					file->accounts[j].next_cheque_num = 0;
					file->accounts[j].payin_num_width = 0;
					file->accounts[j].next_payin_num = 0;

					file->accounts[j].account_view = NULL;

					*(file->accounts[j].name) = '\0';
					*(file->accounts[j].account_no) = '\0';
					*(file->accounts[j].sort_code) = '\0';
					*(file->accounts[j].address[0]) = '\0';
					*(file->accounts[j].address[1]) = '\0';
					*(file->accounts[j].address[2]) = '\0';
					*(file->accounts[j].address[3]) = '\0';

					j++;
				}
			}

			#ifdef DEBUG
			debug_printf("Loading account entry %d", i);
			#endif

			strcpy(file->accounts[i].ident, next_field(NULL, ','));
			file->accounts[i].type = strtoul(next_field(NULL, ','), NULL, 16);
			file->accounts[i].opening_balance = strtoul(next_field(NULL, ','), NULL, 16);
			file->accounts[i].credit_limit = strtoul(next_field(NULL, ','), NULL, 16);
			file->accounts[i].budget_amount = strtoul(next_field(NULL, ','), NULL, 16);
			file->accounts[i].cheque_num_width = strtoul(next_field(NULL, ','), NULL, 16);
			file->accounts[i].next_cheque_num = strtoul(next_field(NULL, ','), NULL, 16);

			*(file->accounts[i].name) = '\0';
			*(file->accounts[i].account_no) = '\0';
			*(file->accounts[i].sort_code) = '\0';
			*(file->accounts[i].address[0]) = '\0';
			*(file->accounts[i].address[1]) = '\0';
			*(file->accounts[i].address[2]) = '\0';
			*(file->accounts[i].address[3]) = '\0';
		} else if (i != -1 && string_nocase_strcmp(token, "Name") == 0) {
			strcpy(file->accounts[i].name, value);
		} else if (i != -1 && string_nocase_strcmp(token, "AccNo") == 0) {
			strcpy(file->accounts[i].account_no, value);
		} else if (i != -1 && string_nocase_strcmp(token, "SortCode") == 0) {
			strcpy(file->accounts[i].sort_code, value);
		} else if (i != -1 && string_nocase_strcmp(token, "Addr0") == 0) {
			strcpy(file->accounts[i].address[0], value);
		} else if (i != -1 && string_nocase_strcmp(token, "Addr1") == 0) {
			strcpy(file->accounts[i].address[1], value);
		} else if (i != -1 && string_nocase_strcmp(token, "Addr2") == 0) {
			strcpy(file->accounts[i].address[2], value);
		} else if (i != -1 && string_nocase_strcmp(token, "Addr3") == 0) {
			strcpy(file->accounts[i].address[3], value);
		} else if (i != -1 && string_nocase_strcmp(token, "PayIn") == 0) {
			file->accounts[i].payin_num_width = strtoul(next_field(value, ','), NULL, 16);
			file->accounts[i].next_payin_num = strtoul(next_field(NULL, ','), NULL, 16);
		} else {
			*unknown_data = TRUE;
		}

		result = config_read_token_pair(in, token, value, section);
	} while (result != sf_READ_CONFIG_EOF && result != sf_READ_CONFIG_NEW_SECTION);

	block_size = flex_size((flex_ptr) &(file->accounts)) / sizeof(account);

	#ifdef DEBUG
	debug_printf("Account block size: %d, required: %d", block_size, file->account_count);
	#endif

	if (block_size > file->account_count) {
		block_size = file->account_count;
		flex_extend((flex_ptr) &(file->accounts), sizeof(account) * block_size);

		#ifdef DEBUG
		debug_printf("Block shrunk to %d", block_size);
		#endif
	}

	return result;
}


/**
 * Read account list details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *suffix		A string containing the trailing end of the section name.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

int account_read_list_file(file_data *file, FILE *in, char *section, char *token, char *value, char *suffix, osbool *unknown_data)
{
	int	result, block_size, i = -1, type, entry;

	type = strtoul(suffix, NULL, 16);
	entry = account_find_window_entry_from_type(file, type);

	block_size = flex_size((flex_ptr) &(file->account_windows[entry].line_data)) / sizeof(struct account_redraw);

	do {
		if (string_nocase_strcmp(token, "Entries") == 0) {
			block_size = strtoul(value, NULL, 16);
			if (block_size > file->account_windows[entry].display_lines) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->account_windows[entry].line_data),
						sizeof(struct account_redraw) * block_size);
			} else {
				block_size = file->account_windows[entry].display_lines;
			}
		} else if (string_nocase_strcmp(token, "WinColumns") == 0) {
			column_init_window(file->account_windows[entry].column_width,
					file->account_windows[entry].column_position,
					ACCOUNT_COLUMNS, 0, TRUE, value);
		} else if (string_nocase_strcmp(token, "@") == 0) {
			file->account_windows[entry].display_lines++;
			if (file->account_windows[entry].display_lines > block_size) {
				block_size = file->account_windows[entry].display_lines;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->account_windows[entry].line_data),
					sizeof(struct account_redraw) * block_size);
			}
			i = file->account_windows[entry].display_lines-1;
			*(file->account_windows[entry].line_data[i].heading) = '\0';
			file->account_windows[entry].line_data[i].type = strtoul(next_field(value, ','), NULL, 16);
			file->account_windows[entry].line_data[i].account = strtoul(next_field(NULL, ','), NULL, 16);
		} else if (i != -1 && string_nocase_strcmp(token, "Heading") == 0) {
			strcpy (file->account_windows[entry].line_data[i].heading, value);
		} else {
			*unknown_data = TRUE;
		}

		result = config_read_token_pair(in, token, value, section);
	} while (result != sf_READ_CONFIG_EOF && result != sf_READ_CONFIG_NEW_SECTION);

	block_size = flex_size((flex_ptr) &(file->account_windows[entry].line_data)) / sizeof(struct account_redraw);

	#ifdef DEBUG
	debug_printf("AccountList block %d size: %d, required: %d", entry, block_size,
			file->account_windows[entry].display_lines);
	#endif

	if (block_size > file->account_windows[entry].display_lines) {
		block_size = file->account_windows[entry].display_lines;
		flex_extend((flex_ptr) &(file->account_windows[entry].line_data), sizeof(struct account_redraw) * block_size);

		#ifdef DEBUG
		debug_printf("Block shrunk to %d", block_size);
		#endif
	}

	return result;
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

	if (windat == NULL || windat->file == NULL)
		return FALSE;

	export_delimited_accounts_file(windat->file, windat->entry, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);

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

	if (windat == NULL || windat->file == NULL)
		return FALSE;

	export_delimited_accounts_file(windat->file, windat->entry, filename, DELIMIT_TAB, TSV_FILE_TYPE);

	return TRUE;
}


