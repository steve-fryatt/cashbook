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
#include "transact.h"

#include "account.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "caret.h"
#include "clipboard.h"
#include "column.h"
#include "conversion.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "find.h"
#include "goto.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "presets.h"
#include "printing.h"
#include "purge.h"
#include "report.h"
#include "saveas.h"
#include "sorder.h"
#include "templates.h"
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

#define TRANSACT_PANE_DRAG_LIMIT 6 /* The last icon to allow column drags on. */
#define TRANSACT_PANE_SORT_DIR_ICON 7

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


struct transact_list_link {
	char	name[DESCRIPT_FIELD_LEN]; /* This assumes that references are always shorter than descriptions...*/
};


static int    new_transaction_window_offset = 0;
static wimp_i transaction_pane_sort_substitute_icon = TRANSACT_PANE_DATE;

/* Transaction Sort Window. */

static wimp_w			transact_sort_window = NULL;			/**< The handle of the transaction sort window.						*/
static file_data		*transact_sort_file = NULL;			/**< The file currently owning the transaction sort window.				*/

/* Transaction Print Window. */

static file_data		*transact_print_file = NULL;			/**< The file currently owning the transaction print window.				*/

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
static file_data			*transact_complete_menu_file = NULL;	/**< The file to which the Reference/Description List menu is currently attached.	*/
static enum transact_list_menu_type	transact_complete_menu_type = REFDESC_MENU_NONE;

/* SaveAs Dialogue Handles. */

static struct saveas_block	*transact_saveas_file = NULL;			/**< The Save File saveas data handle.					*/
static struct saveas_block	*transact_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*transact_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/


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
static void			transact_adjust_sort_icon(file_data *file);
static void			transact_adjust_sort_icon_data(file_data *file, wimp_icon *icon);

static void			transact_complete_menu_add_entry(struct transact_list_link **entries, int *count, int *max, char *new);
static int			transact_complete_menu_compare(const void *va, const void *vb);


static void			transact_open_sort_window(file_data *file, wimp_pointer *ptr);
static void			transact_sort_click_handler(wimp_pointer *pointer);
static osbool			transact_sort_keypress_handler(wimp_key *key);
static void			transact_refresh_sort_window(void);
static void			transact_fill_sort_window(int sort_option);
static osbool			transact_process_sort_window(void);

static void			transact_open_print_window(file_data *file, wimp_pointer *ptr, int clear);
static void			transact_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum, date_t from, date_t to);

static void			transact_start_direct_save(struct transaction_window *windat);
static osbool			transact_save_file(char *filename, osbool selection, void *data);
static osbool			transact_save_csv(char *filename, osbool selection, void *data);
static osbool			transact_save_tsv(char *filename, osbool selection, void *data);
static void			transact_export_delimited(file_data *file, char *filename, enum filing_delimit_type format, int filetype);
static osbool			transact_load_csv(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data);

static void			transact_prepare_fileinfo(file_data *file);


/**
 * Initialise the transaction system.
 *
 * \param *sprites		The application sprite area.
 */

void transact_initialise(osspriteop_area *sprites)
{
	transact_sort_window = templates_create_window("SortTrans");
	ihelp_add_window(transact_sort_window, "SortTrans", NULL);
	event_add_window_mouse_event(transact_sort_window, transact_sort_click_handler);
	event_add_window_key_event(transact_sort_window, transact_sort_keypress_handler);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_ROW, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_DATE, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_FROM, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_TO, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_REFERENCE, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_AMOUNT, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_DESCRIPTION, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_ASCENDING, TRUE);
	event_add_window_icon_radio(transact_sort_window, TRANS_SORT_DESCENDING, TRUE);

	transact_fileinfo_window = templates_create_window("FileInfo");
	ihelp_add_window(transact_fileinfo_window, "FileInfo", NULL);
	templates_link_menu_dialogue("file_info", transact_fileinfo_window);

	transact_window_def = templates_load_window("Transact");
	transact_window_def->icon_count = 0;

	edit_transact_window_def = transact_window_def;			/* \TODO -- Keep us compiling until the edit.c mess is fixed. */

	transact_pane_def = templates_load_window("TransactTB");
	transact_pane_def->sprite_area = sprites;

	transact_window_menu = templates_get_menu(TEMPLATES_MENU_MAIN);
	transact_window_menu_account = templates_get_menu(TEMPLATES_MENU_MAIN_ACCOUNTS);
	transact_window_menu_transact = templates_get_menu(TEMPLATES_MENU_MAIN_TRANSACTIONS);
	transact_window_menu_analysis = templates_get_menu(TEMPLATES_MENU_MAIN_ANALYSIS);

	transact_saveas_file = saveas_create_dialogue(FALSE, "file_1ca", transact_save_file);
	transact_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", transact_save_csv);
	transact_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", transact_save_tsv);
}


/**
 * Create and open a Transaction List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void transact_open_window(file_data *file)
{
	int		i, j, height;
	os_error	*error;

	if (file->transaction_window.transaction_window != NULL) {
		windows_open(file->transaction_window.transaction_window);
		return;
	}

	/* Create the new window data and build the window. */

	*(file->transaction_window.window_title) = '\0';
	transact_window_def->title_data.indirected_text.text = file->transaction_window.window_title;

	file->transaction_window.display_lines = (file->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
			file->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;

	height =  file->transaction_window.display_lines;

	set_initial_window_area(transact_window_def,
			file->transaction_window.column_position[TRANSACT_COLUMNS-1] +
			file->transaction_window.column_width[TRANSACT_COLUMNS-1],
			((ICON_HEIGHT+LINE_GUTTER) * height) + TRANSACT_TOOLBAR_HEIGHT,
			-1, -1, new_transaction_window_offset * TRANSACTION_WINDOW_OPEN_OFFSET);

	error = xwimp_create_window(transact_window_def, &(file->transaction_window.transaction_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	new_transaction_window_offset++;
	if (new_transaction_window_offset >= TRANSACTION_WINDOW_OFFSET_LIMIT)
		new_transaction_window_offset = 0;

	/* Create the toolbar pane. */

	windows_place_as_toolbar(transact_window_def, transact_pane_def, TRANSACT_TOOLBAR_HEIGHT-4);

	for (i=0, j=0; j < TRANSACT_COLUMNS; i++, j++) {
		transact_pane_def->icons[i].extent.x0 = file->transaction_window.column_position[j];
		j = column_get_rightmost_in_group(TRANSACT_PANE_COL_MAP, i);
		transact_pane_def->icons[i].extent.x1 = file->transaction_window.column_position[j] +
				file->transaction_window.column_width[j] +
				COLUMN_HEADING_MARGIN;
	}

	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) file->transaction_window.sort_sprite;
	transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			transact_pane_def->sprite_area;

	transact_adjust_sort_icon_data(file, &(transact_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON]));

	error = xwimp_create_window(transact_pane_def, &(file->transaction_window.transaction_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	build_transaction_window_title(file);

	/* Update the toolbar */

	update_transaction_window_toolbar(file);

	/* Set the default values */

	file->transaction_window.entry_line = -1;
	file->transaction_window.display_lines = MIN_TRANSACT_ENTRIES;

	/* Open the window. */

	windows_open(file->transaction_window.transaction_window);
	windows_open_nested_as_toolbar(file->transaction_window.transaction_pane,
			file->transaction_window.transaction_window,
			TRANSACT_TOOLBAR_HEIGHT-4);

	ihelp_add_window(file->transaction_window.transaction_window , "Transact", decode_transact_window_help);
	ihelp_add_window(file->transaction_window.transaction_pane , "TransactTB", NULL);

	/* Register event handlers for the two windows. */
	/* \TODO -- Should this be all three windows?   */

	event_add_window_user_data(file->transaction_window.transaction_window, &(file->transaction_window));
	event_add_window_menu(file->transaction_window.transaction_window, transact_window_menu);
	event_add_window_open_event(file->transaction_window.transaction_window, transact_window_open_handler);
	event_add_window_close_event(file->transaction_window.transaction_window, transact_window_close_handler);
	event_add_window_lose_caret_event(file->transaction_window.transaction_window, transact_window_lose_caret_handler);
	event_add_window_mouse_event(file->transaction_window.transaction_window, transact_window_click_handler);
	event_add_window_key_event(file->transaction_window.transaction_window, transact_window_keypress_handler);
	event_add_window_scroll_event(file->transaction_window.transaction_window, transact_window_scroll_handler);
	event_add_window_redraw_event(file->transaction_window.transaction_window, transact_window_redraw_handler);
	event_add_window_menu_prepare(file->transaction_window.transaction_window, transact_window_menu_prepare_handler);
	event_add_window_menu_selection(file->transaction_window.transaction_window, transact_window_menu_selection_handler);
	event_add_window_menu_warning(file->transaction_window.transaction_window, transact_window_menu_warning_handler);
	event_add_window_menu_close(file->transaction_window.transaction_window, transact_window_menu_close_handler);

	event_add_window_user_data(file->transaction_window.transaction_pane, &(file->transaction_window));
	event_add_window_menu(file->transaction_window.transaction_pane, transact_window_menu);
	event_add_window_mouse_event(file->transaction_window.transaction_pane, transact_pane_click_handler);
	event_add_window_menu_prepare(file->transaction_window.transaction_pane, transact_window_menu_prepare_handler);
	event_add_window_menu_selection(file->transaction_window.transaction_pane, transact_window_menu_selection_handler);
	event_add_window_menu_warning(file->transaction_window.transaction_pane, transact_window_menu_warning_handler);
	event_add_window_menu_close(file->transaction_window.transaction_pane, transact_window_menu_close_handler);
	event_add_window_icon_popup(file->transaction_window.transaction_pane, TRANSACT_PANE_VIEWACCT, transact_account_list_menu, -1, NULL);

	dataxfer_set_load_target(CSV_FILE_TYPE, file->transaction_window.transaction_window, -1, transact_load_csv, file);
	dataxfer_set_load_target(CSV_FILE_TYPE, file->transaction_window.transaction_pane, -1, transact_load_csv, file);

	/* Put the caret into the first empty line. */

	edit_place_new_line(file, file->trans_count);
	icons_put_caret_at_end(file->transaction_window.transaction_window, EDIT_ICON_DATE);
	edit_find_line_vertically(file);
}


/**
 * Close and delete a Transaction List Window associated with the given
 * transaction window block.
 *
 * \param *windat		The window to delete.
 */

void transact_delete_window(struct transaction_window *windat)
{
	#ifdef DEBUG
	debug_printf("\\RDeleting transaction window");
	#endif

	if (windat == NULL)
		return;

	if (windat->transaction_window != NULL) {
		ihelp_remove_window(windat->transaction_window);
		event_delete_window(windat->transaction_window);
		wimp_delete_window(windat->transaction_window);
		dataxfer_delete_load_target(CSV_FILE_TYPE, windat->transaction_window, -1);
		windat->transaction_window = NULL;
	}

	if (windat->transaction_pane != NULL) {
		ihelp_remove_window(windat->transaction_pane);
		event_delete_window(windat->transaction_pane);
		dataxfer_delete_load_target(CSV_FILE_TYPE, windat->transaction_pane, -1);
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
	struct transaction_window	*windat;

	windat = event_get_window_user_data(open->w);
	if (windat != NULL && windat->file != NULL)
		minimise_transaction_window_extent(windat->file);

	wimp_open_window(open);
}


/**
 * Handle Close events on Transaction List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void transact_window_close_handler(wimp_close *close)
{
	struct transaction_window	*windat;
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
	struct transaction_window	*windat;
	file_data			*file;
	int				line, transaction, xpos, column;
	wimp_window_state		window;
	wimp_pointer			ptr;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* Force a refresh of the current edit line, if there is one.  We avoid refreshing the icon where the mouse
	 * was clicked.
	 */

	edit_refresh_line_content(NULL, -1, pointer->i);

	if (pointer->buttons == wimp_CLICK_SELECT) {
		if (pointer->i == wimp_ICON_WINDOW) {
			window.w = pointer->w;
			wimp_get_window_state(&window);

			line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - TRANSACT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

			if (line >= 0) {
				edit_place_new_line(file, line);

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

		line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - TRANSACT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

		/* If the line was in range, find the column that the click occurred in by scanning through the column
		 * positions.
		 */

		if (line >= 0) {
			xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;

			for (column = 0; column < TRANSACT_COLUMNS &&
					xpos > (file->transaction_window.column_position[column] + file->transaction_window.column_width[column]);
					column++);

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
			} else if (line < file->trans_count) {
				/* ...while the rest have to occur over a transaction line. */
				transaction = file->transactions[line].sort_index;

				if (column == TRANSACT_ICON_FROM_REC) {
					/* If the column is the from reconcile flag, toggle its status. */
					edit_toggle_transaction_reconcile_flag(file, transaction, TRANS_REC_FROM);
				} else if (column == TRANSACT_ICON_TO_REC) {
					/* If the column is the to reconcile flag, toggle its status. */
					edit_toggle_transaction_reconcile_flag(file, transaction, TRANS_REC_TO);
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
	edit_refresh_line_content(caret->w, -1, -1);
}

/**
 * Process mouse clicks in the Transaction List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void transact_pane_click_handler(wimp_pointer *pointer)
{
	struct transaction_window	*windat;
	file_data			*file;
	wimp_window_state		window;
	wimp_icon_state			icon;
	int				ox;
	char				*filename;


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
			find_open_window(file, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_GOTO:
			goto_open_window(file, pointer, config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_SORT:
			transact_open_sort_window(file, pointer);
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
			find_open_window(file, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_GOTO:
			goto_open_window(file, pointer, !config_opt_read("RememberValues"));
			break;

		case TRANSACT_PANE_SORT:
			transact_sort(file);
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
			file->transaction_window.sort_order = SORT_NONE;

			switch (pointer->i) {
			case TRANSACT_PANE_ROW:
				file->transaction_window.sort_order = SORT_ROW;
				break;

			case TRANSACT_PANE_DATE:
				file->transaction_window.sort_order = SORT_DATE;
				break;

			case TRANSACT_PANE_FROM:
				file->transaction_window.sort_order = SORT_FROM;
				break;

			case TRANSACT_PANE_TO:
				file->transaction_window.sort_order = SORT_TO;
				break;

			case TRANSACT_PANE_REFERENCE:
				file->transaction_window.sort_order = SORT_REFERENCE;
				break;

			case TRANSACT_PANE_AMOUNT:
				file->transaction_window.sort_order = SORT_AMOUNT;
				break;

			case TRANSACT_PANE_DESCRIPTION:
				file->transaction_window.sort_order = SORT_DESCRIPTION;
				break;
			}

			if (file->transaction_window.sort_order != SORT_NONE) {
				if (pointer->buttons == wimp_CLICK_SELECT * 256)
					file->transaction_window.sort_order |= SORT_ASCENDING;
				else
					file->transaction_window.sort_order |= SORT_DESCENDING;
			}

			transact_adjust_sort_icon(file);
			windows_redraw(file->transaction_window.transaction_pane);
			transact_sort(file);
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && pointer->i <= TRANSACT_PANE_DRAG_LIMIT) {
		column_start_drag(pointer, windat, file->transaction_window.transaction_window, TRANSACT_PANE_COL_MAP,
				config_str_read("LimTransactCols"),  transact_adjust_window_columns);
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
	struct transaction_window	*windat;
	file_data			*file;
	wimp_pointer			pointer;
	char				*filename;

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
		find_open_window(file, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_F5) {
		wimp_get_pointer_info(&pointer);
		goto_open_window(file, &pointer, config_opt_read("RememberValues"));
	} else if (key->c == wimp_KEY_F6) {
		wimp_get_pointer_info(&pointer);
		transact_open_sort_window(file, &pointer);
	} else if (key->c == wimp_KEY_F9) {
		account_open_window(file, ACCOUNT_FULL);
	} else if (key->c == wimp_KEY_F10) {
		account_open_window(file, ACCOUNT_IN);
	} else if (key->c == wimp_KEY_F11) {
		account_open_window(file, ACCOUNT_OUT);
	} else if (key->c == wimp_KEY_PAGE_UP || key->c == wimp_KEY_PAGE_DOWN) {
		wimp_scroll scroll;

		/* Make up a Wimp_ScrollRequest block and pass it to the scroll request handler. */

		scroll.w = file->transaction_window.transaction_window;
		wimp_get_window_state((wimp_window_state *) &scroll);

		scroll.xmin = wimp_SCROLL_NONE;
		scroll.ymin = (key->c == wimp_KEY_PAGE_UP) ? wimp_SCROLL_PAGE_UP : wimp_SCROLL_PAGE_DOWN;

		transact_window_scroll_handler(&scroll);
	} else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_UP) || key->c == wimp_KEY_HOME) {
		scroll_transaction_window_to_end(file, -1);
	} else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_DOWN) ||
			(key->c == wimp_KEY_COPY && config_opt_read ("IyonixKeys"))) {
		scroll_transaction_window_to_end(file, +1);
	} else {
		/* Pass any keys that are left on to the edit line handler. */
		return edit_process_keypress(file, key);
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
	struct transaction_window	*windat;
	int				line;
	wimp_window_state		window;
	char				*filename;

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
			templates_set_menu(TEMPLATES_MENU_ACCOPEN, transact_account_list_menu);
		}

		account_list_menu_prepare();
		return;
	}

	/* Otherwsie, this is the standard window menu.
	 */

	if (pointer != NULL) {
		transact_window_menu_line = -1;

		if (w == windat->transaction_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - TRANSACT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

			if (line >= 0 && line < windat->file->trans_count)
				transact_window_menu_line = line;
		}

		transact_window_menu_account->entries[MAIN_MENU_ACCOUNTS_VIEW].sub_menu = account_list_menu_build(windat->file);
		transact_window_menu_analysis->entries[MAIN_MENU_ANALYSIS_SAVEDREP].sub_menu = analysis_template_menu_build(windat->file, FALSE);

		/* If the submenus concerned are greyed out, give them a valid submenu pointer so that the arrow shows. */

		if (windat->file->account_count == 0)
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
	struct transaction_window	*windat;
	file_data			*file;
	wimp_pointer			pointer;

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
			purge_open_window(windat->file, &pointer, config_opt_read("RememberValues"));
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
			find_open_window(windat->file, &pointer, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_TRANS_GOTO:
			goto_open_window(windat->file, &pointer, config_opt_read("RememberValues"));
			break;

		case MAIN_MENU_TRANS_SORT:
			transact_open_sort_window(windat->file, &pointer);
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
			icons_set_selected(windat->file->transaction_window.transaction_pane, TRANSACT_PANE_RECONCILE, windat->file->auto_reconcile);
			break;
		}
		break;

	case MAIN_MENU_SUB_UTILS:
		switch (selection->items[1]) {
		case MAIN_MENU_ANALYSIS_BUDGET:
			budget_open_window(windat->file, &pointer);
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
	struct transaction_window	*windat;

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
	int				width, height, line, error;
	struct transaction_window	*windat;
	file_data			*file;

	windat = event_get_window_user_data(scroll->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;


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
		scroll->yscroll += (ICON_HEIGHT + LINE_GUTTER);
		if ((error = ((scroll->yscroll) % (ICON_HEIGHT+LINE_GUTTER))))
			scroll->yscroll -= (ICON_HEIGHT+LINE_GUTTER) + error;
		break;

	case wimp_SCROLL_LINE_DOWN:
		scroll->yscroll -= (ICON_HEIGHT + LINE_GUTTER);
		if ((error = ((scroll->yscroll - height) % (ICON_HEIGHT+LINE_GUTTER))))
			scroll->yscroll -= error;

		/* Extend the window if necessary. */

		line = (-scroll->yscroll + height) / (ICON_HEIGHT+LINE_GUTTER);
		if (line > file->transaction_window.display_lines) {
			file->transaction_window.display_lines = line;
			transact_set_window_extent(file);
		}
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

	/* Re-open the window, then try and reduce the window extent if possible.
	 *
	 * It is assumed that the wimp will deal with out-of-bounds offsets for us.
	 */

	wimp_open_window((wimp_open *) scroll);
	minimise_transaction_window_extent(file);
}


/**
 * Process redraw events in the Transaction List window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void transact_window_redraw_handler(wimp_draw *redraw)
{
	struct transaction_window	*windat;
	file_data			*file;
	int				ox, oy, top, base, y, i, t, shade_rec, shade_rec_col, icon_fg_col;
	char				icon_buffer[DESCRIPT_FIELD_LEN], rec_char[REC_FIELD_LEN]; /* Assumes descript is longest. */
	osbool				more;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);
	shade_rec = config_opt_read("ShadeReconciled");
	shade_rec_col = config_int_read("ShadeReconciledColour");

	/* Set the horizontal positions of the icons. */

	for (i=0; i < TRANSACT_COLUMNS; i++) {
		transact_window_def->icons[i].extent.x0 = file->transaction_window.column_position[i];
		transact_window_def->icons[i].extent.x1 = file->transaction_window.column_position[i] +
				file->transaction_window.column_width[i];
		transact_window_def->icons[i].data.indirected_text.text = icon_buffer;
	}

	/* Perform the redraw. */

	while (more) {
		/* Calculate the rows to redraw. */

		top = (oy - redraw->clip.y1 - TRANSACT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
		if (top < 0)
			top = 0;
 
		base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2)
				+ oy - redraw->clip.y0 - TRANSACT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			t = (y < file->trans_count) ? file->transactions[y].sort_index : 0;

			/* Work out the foreground colour for the line, based on whether
			 * the line is to be shaded or not.
			 */

			if (shade_rec && (y < file->trans_count) &&
					((file->transactions[t].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
				icon_fg_col = (shade_rec_col << wimp_ICON_FG_COLOUR_SHIFT);
			else
				icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

			/* Plot out the background with a filled grey rectangle. */

			wimp_set_colour(wimp_COLOUR_VERY_LIGHT_GREY);
			os_plot(os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - TRANSACT_TOOLBAR_HEIGHT);
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO,
					ox + file->transaction_window.column_position[TRANSACT_COLUMNS-1]
					+ file->transaction_window.column_width[TRANSACT_COLUMNS-1],
					oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - TRANSACT_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

			/* We don't need to plot the current edit line, as that has real
			 * icons in it.
			 */

			if (y == file->transaction_window.entry_line)
				continue;

			/* Row field. */

			transact_window_def->icons[TRANSACT_ICON_ROW].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_ROW].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_ROW].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_ROW].flags |= icon_fg_col;

			if (y < file->trans_count)
				snprintf(icon_buffer, DESCRIPT_FIELD_LEN, "%d", transact_get_transaction_number(t));
			else
				*icon_buffer = '\0';

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_ROW]));

			/* Date field */

			transact_window_def->icons[TRANSACT_ICON_DATE].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_DATE].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_DATE].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_DATE].flags |= icon_fg_col;

			if (y < file->trans_count)
				convert_date_to_string(file->transactions[t].date, icon_buffer);
			else
				*icon_buffer = '\0';

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_DATE]));

			/* From field */

			transact_window_def->icons[TRANSACT_ICON_FROM].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_FROM].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_FROM_REC].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_FROM_REC].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_FROM].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_FROM].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_FROM_REC].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_FROM_REC].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_FROM_NAME].flags |= icon_fg_col;

			if (y < file->trans_count && file->transactions[t].from != NULL_ACCOUNT) {
				transact_window_def->icons[TRANSACT_ICON_FROM].data.indirected_text.text =
						account_get_ident(file, file->transactions[t].from);
				transact_window_def->icons[TRANSACT_ICON_FROM_REC].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_FROM_NAME].data.indirected_text.text =
						account_get_name(file, file->transactions[t].from);

				if (file->transactions[t].flags & TRANS_REC_FROM)
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

			transact_window_def->icons[TRANSACT_ICON_TO].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_TO].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_TO_REC].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_TO_REC].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_TO_NAME].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_TO_NAME].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_TO].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_TO].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_TO_REC].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_TO_REC].flags |= icon_fg_col;

			transact_window_def->icons[TRANSACT_ICON_TO_NAME].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_TO_NAME].flags |= icon_fg_col;

			if (y < file->trans_count && file->transactions[t].to != NULL_ACCOUNT) {
				transact_window_def->icons[TRANSACT_ICON_TO].data.indirected_text.text =
						account_get_ident(file, file->transactions[t].to);
				transact_window_def->icons[TRANSACT_ICON_TO_REC].data.indirected_text.text = icon_buffer;
				transact_window_def->icons[TRANSACT_ICON_TO_NAME].data.indirected_text.text =
						account_get_name(file, file->transactions[t].to);

				if (file->transactions[t].flags & TRANS_REC_TO)
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

			transact_window_def->icons[TRANSACT_ICON_REFERENCE].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_REFERENCE].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_REFERENCE].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_REFERENCE].flags |= icon_fg_col;

			if (y < file->trans_count) {
				transact_window_def->icons[TRANSACT_ICON_REFERENCE].data.indirected_text.text = file->transactions[t].reference;
			} else {
				transact_window_def->icons[TRANSACT_ICON_REFERENCE].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_REFERENCE]));

			/* Amount field */

			transact_window_def->icons[TRANSACT_ICON_AMOUNT].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_AMOUNT].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_AMOUNT].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_AMOUNT].flags |= icon_fg_col;

			if (y < file->trans_count)
				convert_money_to_string(file->transactions[t].amount, icon_buffer);
			else
				*icon_buffer = '\0';

			wimp_plot_icon(&(transact_window_def->icons[TRANSACT_ICON_AMOUNT]));

			/* Description field */

			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
					- TRANSACT_TOOLBAR_HEIGHT;

			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].flags &= ~wimp_ICON_FG_COLOUR;
			transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].flags |= icon_fg_col;

			if (y < file->trans_count) {
				transact_window_def->icons[TRANSACT_ICON_DESCRIPTION].data.indirected_text.text = file->transactions[t].description;
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
	struct transaction_window	*windat = (struct transaction_window *) data;
	file_data			*file;
	int				i, j, new_extent;
	wimp_icon_state			icon;
	wimp_window_info		window;
	wimp_caret			caret;

	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	update_dragged_columns(TRANSACT_PANE_COL_MAP, config_str_read("LimTransactCols"), target, width,
			file->transaction_window.column_width,
			file->transaction_window.column_position, TRANSACT_COLUMNS);

	/* Re-adjust the icons in the pane. */

	for (i=0, j=0; j < TRANSACT_COLUMNS; i++, j++) {
		icon.w = file->transaction_window.transaction_pane;
		icon.i = i;
		wimp_get_icon_state(&icon);

		icon.icon.extent.x0 = file->transaction_window.column_position[j];

		j = column_get_rightmost_in_group(TRANSACT_PANE_COL_MAP, i);

		icon.icon.extent.x1 = file->transaction_window.column_position[j] +
				file->transaction_window.column_width[j] + COLUMN_HEADING_MARGIN;

		wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
				icon.icon.extent.x1, icon.icon.extent.y1);

		new_extent = file->transaction_window.column_position[TRANSACT_COLUMNS-1] +
				file->transaction_window.column_width[TRANSACT_COLUMNS-1];
	}

	transact_adjust_sort_icon(file);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	wimp_get_caret_position(&caret);

	edit_place_new_line(file, file->transaction_window.entry_line);
	windows_redraw(file->transaction_window.transaction_window);
	windows_redraw(file->transaction_window.transaction_pane);

	/* If the caret's position was in the current transaction window, we need to replace it in the same position
	 * now, so that we don't lose input focus.
	 */

	if (file->transaction_window.transaction_window != NULL &&
			file->transaction_window.transaction_window == caret.w)
		wimp_set_caret_position(caret.w, caret.i, 0, 0, -1, caret.index);

	/* Set the horizontal extent of the window and pane. */

	window.w = file->transaction_window.transaction_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = file->transaction_window.transaction_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(file, TRUE);
}


/**
 * Adjust the sort icon in a transaction window, to reflect the current column
 * heading positions.
 *
 * \param *file			The file to update the window for.
 */

static void transact_adjust_sort_icon(file_data *file)
{
	wimp_icon_state icon;

	icon.w = file->transaction_window.transaction_pane;
	icon.i = TRANSACT_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	transact_adjust_sort_icon_data(file, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current transaction sort settings.
 *
 * \param *file			The file to be updated.
 * \param *icon			The icon to be updated.
 */

static void transact_adjust_sort_icon_data(file_data *file, wimp_icon *icon)
{
	int	i = 0, width, anchor;


	if (file->transaction_window.sort_order & SORT_ASCENDING)
		strcpy(file->transaction_window.sort_sprite, "sortarrd");
	else if (file->transaction_window.sort_order & SORT_DESCENDING)
		strcpy(file->transaction_window.sort_sprite, "sortarru");

	switch (file->transaction_window.sort_order & SORT_MASK) {
	case SORT_ROW:
		i = TRANSACT_ICON_ROW;
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_ROW;
		break;

	case SORT_DATE:
		i = TRANSACT_ICON_DATE;
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_DATE;
		break;

	case SORT_FROM:
		i = TRANSACT_ICON_FROM_NAME;
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_FROM;
		break;

	case SORT_TO:
		i = TRANSACT_ICON_TO_NAME;
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_TO;
		break;

	case SORT_REFERENCE:
		i = TRANSACT_ICON_REFERENCE;
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_REFERENCE;
		break;

	case SORT_AMOUNT:
		i = TRANSACT_ICON_AMOUNT;
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_AMOUNT;
		break;

	case SORT_DESCRIPTION:
		i = TRANSACT_ICON_DESCRIPTION;
		transaction_pane_sort_substitute_icon = TRANSACT_PANE_DESCRIPTION;
		break;
	}

	width = icon->extent.x1 - icon->extent.x0;

	if ((file->transaction_window.sort_order & SORT_MASK) == SORT_ROW ||
			(file->transaction_window.sort_order & SORT_MASK) == SORT_AMOUNT) {
		anchor = file->transaction_window.column_position[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
		icon->extent.x1 = icon->extent.x0 + width;
	} else {
		anchor = file->transaction_window.column_position[i] +
				file->transaction_window.column_width[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
		icon->extent.x0 = icon->extent.x1 - width;
	}
}


/**
 * Set the extent of the transaction window for the specified file.
 *
 * \param *file			The file to update.
 */

void transact_set_window_extent(file_data *file)
{
	wimp_window_state	state;
	os_box			extent;
	int			visible_extent, new_extent, new_scroll;


	/* If the window display length is too small, extend it to one blank line after the data. */

	if (file->transaction_window.display_lines <= file->trans_count)
		file->transaction_window.display_lines = file->trans_count + 1;

	/* Work out the new extent. */

	new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * file->transaction_window.display_lines) - TRANSACT_TOOLBAR_HEIGHT;

	/* Get the current window details, and find the extent of the bottom of the visible area. */

	state.w = file->transaction_window.transaction_window;
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
	extent.x1 = file->transaction_window.column_position[TRANSACT_COLUMNS-1] +
			file->transaction_window.column_width[TRANSACT_COLUMNS-1];
	extent.y0 = new_extent;

	wimp_set_extent(file->transaction_window.transaction_window, &extent);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Try to minimise the extent of the transaction window, by removing redundant blank lines as they are scrolled
 * out of sight.
 */

void minimise_transaction_window_extent (file_data *file)
{
  int               height, last_visible_line, minimum_length;
  wimp_window_state window;


  window.w = file->transaction_window.transaction_window;
  wimp_get_window_state (&window);

  /* Calculate the height of the window and the last line that is visible. */

  height = (window.visible.y1 - window.visible.y0) - TRANSACT_TOOLBAR_HEIGHT;
  last_visible_line = (-window.yscroll + height) / (ICON_HEIGHT+LINE_GUTTER);

  /* Work out what the minimum length of the window needs to be, taking into account minimum window size, entries
   * and blank lines and the location of the edit line.
   */

  minimum_length = (file->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
                    file->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;

  if (file->transaction_window.entry_line >= minimum_length)
  {
    minimum_length = file->transaction_window.entry_line + 1;
  }

  if (last_visible_line > minimum_length)
  {
    minimum_length = last_visible_line;
  }

  /* Shrink the window. */

  if (file->transaction_window.display_lines > minimum_length)
  {
    file->transaction_window.display_lines = minimum_length;
    transact_set_window_extent (file);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void build_transaction_window_title (file_data *file)
{
  file_get_pathname(file, file->transaction_window.window_title, sizeof (file->transaction_window.window_title));

  if (file->modified)
  {
    strcat (file->transaction_window.window_title, " *");
  }

  xwimp_force_redraw_title (file->transaction_window.transaction_window); /* Nested Wimp only! */
}

/* ------------------------------------------------------------------------------------------------------------------ */

void force_transaction_window_redraw (file_data *file, int from, int to)
{
  int              y0, y1;
  wimp_window_info window;

  window.w = file->transaction_window.transaction_window;
  if (xwimp_get_window_info_header_only (&window) == NULL)
  {
    y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - TRANSACT_TOOLBAR_HEIGHT;
    y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - TRANSACT_TOOLBAR_HEIGHT;

    wimp_force_redraw (file->transaction_window.transaction_window, window.extent.x0, y0, window.extent.x1, y1);
  }
  /* **** NB This doesn't redraw the edit line -- the icons need to be refreshed **** */
}

/* ------------------------------------------------------------------------------------------------------------------ */

void update_transaction_window_toolbar (file_data *file)
{
  icons_set_shaded (file->transaction_window.transaction_pane, TRANSACT_PANE_VIEWACCT,
                    account_count_type_in_file (file, ACCOUNT_FULL) == 0);
}

/* ------------------------------------------------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------------------------------------------------ */

/* Scroll the transaction window to the top or the end. */

void scroll_transaction_window_to_end (file_data *file, int dir)
{
  wimp_window_info window;


  window.w = file->transaction_window.transaction_window;
  wimp_get_window_info_header_only (&window);

  if (dir > 0)
  {
    window.yscroll = window.extent.y0 - (window.extent.y1 - window.extent.y0);
  }
  else if (dir < 0)
  {
    window.yscroll = window.extent.y1;
  }

  minimise_transaction_window_extent (file);
  wimp_open_window ((wimp_open *) &window);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the transaction number of the transaction in the centre (or nearest the centre) of the transactions
 * window which references the given account.
 *
 * First find the centre line, and see if that matches the account.  If so, return the transaction.  If not, search
 * outwards from that point towards the ends of the window, looking for a match.
 */

int find_transaction_window_centre (file_data *file, int account)
{
  wimp_window_state window;
  int               centre, height, result, i;


  window.w = file->transaction_window.transaction_window;
  wimp_get_window_state (&window);

  /* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
   */

  height = window.visible.y1 - window.visible.y0 - ICON_HEIGHT - LINE_GUTTER - TRANSACT_TOOLBAR_HEIGHT;

  /* Calculate the centre line in the window.  If this is greater than the number of actual tracsactions in the window,
   * reduce it accordingly.
   */

  centre = ((-window.yscroll + ICON_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER)) + ((height / 2) / (ICON_HEIGHT+LINE_GUTTER));

  if (centre >= file->trans_count)
  {
    centre = file->trans_count - 1;
  }

  if (centre > -1)
  {
    if (file->transactions[file->transactions[centre].sort_index].from == account ||
        file->transactions[file->transactions[centre].sort_index].to == account)
    {
      result = file->transactions[centre].sort_index;
    }
    else
    {
      i = 1;

      result = NULL_TRANSACTION;

      while (centre+i < file->trans_count || centre-i >= 0)
      {
        if (centre+i < file->trans_count
            && (file->transactions[file->transactions[centre+i].sort_index].from == account
            || file->transactions[file->transactions[centre+i].sort_index].to == account))
        {
          result = file->transactions[centre+i].sort_index;
          break;
        }

        if (centre-i >= 0
            && (file->transactions[file->transactions[centre-i].sort_index].from == account
            || file->transactions[file->transactions[centre-i].sort_index].to == account))
        {
          result = file->transactions[centre-i].sort_index;
          break;
        }

        i++;
      }
    }
  }
  else
  {
    result = NULL_TRANSACTION;
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_transact_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
  int                 column, xpos;
  wimp_window_state   window;
  file_data           *file;

  *buffer = '\0';

  file = find_transaction_window_file_block (w);

  if (file != NULL)
  {
    window.w = w;
    wimp_get_window_state (&window);

    xpos = (pos.x - window.visible.x0) + window.xscroll;

    for (column = 0;
         column < TRANSACT_COLUMNS &&
         xpos > (file->transaction_window.column_position[column] + file->transaction_window.column_width[column]);
         column++);

    sprintf (buffer, "Col%d", column);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find and return the line in the transaction window that points to the specified transaction. */

int locate_transaction_in_transact_window (file_data *file, int transaction)
{
  int        i, line;


  line = -1;

  for (i=0; i < file->trans_count; i++)
  {
    if (file->transactions[i].sort_index == transaction)
    {
      line = i;
      break;
    }
  }

  return (line);
}




































































/**
 * Build a Reference or Drescription Complete menu for a given file.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \param start_line		The line of the window to start from.
 * \return			The menu block, or NULL.
 */

wimp_menu *transact_complete_menu_build(file_data *file, enum transact_list_menu_type menu_type, int start_line)
{
	int		i, range, line, width, items, max_items, item_limit;
	osbool		no_original;
	char		*title_token;

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
		msgs_lookup("RefMenuChq", transact_complete_menu_link[items++].name, DESCRIPT_FIELD_LEN);

	/* Bring the start line into range for the current file.  no_original is set true if the line fell off the end
	 * of the file, as this needs to be a special case of "no text".  If not, lines off the end of the file will
	 * be matched against the final transaction as a result of pulling start_line into range.
	 */

	if (start_line >= file->trans_count) {
		start_line = file->trans_count - 1;
		no_original = TRUE;
	} else {
		no_original = FALSE;
	}

	if (file->trans_count > 0 && transact_complete_menu_link != NULL) {
		/* Find the largest range from the current line to one end of the transaction list. */

		range = ((file->trans_count - start_line - 1) > start_line) ? (file->trans_count - start_line - 1) : start_line;

		/* Work out from the line to the edges of the transaction window. For each transaction, check the entries
		 * and add them into the list.
		 */

		if (menu_type == REFDESC_MENU_REFERENCE) {
			for (i=1; i<=range && (item_limit == 0 || items <= item_limit); i++) {
				if ((start_line+i < file->trans_count) && (no_original ||
						(*(file->transactions[file->transactions[start_line].sort_index].reference) == '\0') ||
						(string_nocase_strstr(file->transactions[file->transactions[start_line+i].sort_index].reference,
						file->transactions[file->transactions[start_line].sort_index].reference) ==
						file->transactions[file->transactions[start_line+i].sort_index].reference))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
							file->transactions[file->transactions[start_line+i].sort_index].reference);
				}
				if ((start_line-i >= 0) && (no_original ||
						(*(file->transactions[file->transactions[start_line].sort_index].reference) == '\0') ||
						(string_nocase_strstr(file->transactions[file->transactions[start_line-i].sort_index].reference,
						file->transactions[file->transactions[start_line].sort_index].reference) ==
						file->transactions[file->transactions[start_line-i].sort_index].reference))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
						file->transactions[file->transactions[start_line-i].sort_index].reference);
				}
			}
		} else if (menu_type == REFDESC_MENU_DESCRIPTION) {
			for (i=1; i<=range && (item_limit == 0 || items < item_limit); i++) {
				if ((start_line+i < file->trans_count) && (no_original ||
						(*(file->transactions[file->transactions[start_line].sort_index].description) == '\0') ||
						(string_nocase_strstr(file->transactions[file->transactions[start_line+i].sort_index].description,
						file->transactions[file->transactions[start_line].sort_index].description) ==
						file->transactions[file->transactions[start_line+i].sort_index].description))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
							file->transactions[file->transactions[start_line+i].sort_index].description);
				}
				if ((start_line-i >= 0) && (no_original ||
						(*(file->transactions[file->transactions[start_line].sort_index].description) == '\0') ||
						(string_nocase_strstr(file->transactions[file->transactions[start_line-i].sort_index].description,
						file->transactions[file->transactions[start_line].sort_index].description) ==
						file->transactions[file->transactions[start_line-i].sort_index].description))) {
					transact_complete_menu_add_entry(&transact_complete_menu_link, &items, &max_items,
						file->transactions[file->transactions[start_line-i].sort_index].description);
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
			transact_complete_menu->entries[line].data.indirected_text.size = DESCRIPT_FIELD_LEN;

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

	if (transact_complete_menu == NULL || transact_complete_menu_type != REFDESC_MENU_REFERENCE)
		return;

	if ((line < transact_complete_menu_file->trans_count) &&
			((account = transact_complete_menu_file->transactions[transact_complete_menu_file->transactions[line].sort_index].from) != NULL_ACCOUNT) &&
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

/* Adds a new transaction to the end of the list. */

void add_raw_transaction (file_data *file, unsigned date, int from, int to, unsigned flags, int amount,
                          char *ref, char *description)
{
  int new;


  if (flex_extend ((flex_ptr) &(file->transactions), sizeof (struct transaction) * (file->trans_count+1)) == 1)
  {
    new = file->trans_count++;

    file->transactions[new].date = date;
    file->transactions[new].amount = amount;
    file->transactions[new].from = from;
    file->transactions[new].to = to;
    file->transactions[new].flags = flags;
    strcpy (file->transactions[new].reference, ref);
    strcpy (file->transactions[new].description, description);

    file->transactions[new].sort_index = new;

    file_set_data_integrity(file, TRUE);
    if (date != NULL_DATE)
    {
      file->sort_valid = 0;
    }
  }
  else
  {
    error_msgs_report_error ("NoMemNewTrans");
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return true if the transaction specified is completely empty. */

int is_transaction_blank (file_data *file, int transaction)
{
  return (file->transactions[transaction].date == NULL_DATE &&
          file->transactions[transaction].from == NULL_ACCOUNT &&
          file->transactions[transaction].to == NULL_ACCOUNT &&
          file->transactions[transaction].amount == NULL_CURRENCY &&
          *file->transactions[transaction].reference == '\0' &&
          *file->transactions[transaction].description == '\0');
}

/* ------------------------------------------------------------------------------------------------------------------ */


/* Strip blank transactions from the file.  This relies on the blank transactions being at the end, which relies
 * on a transaction list sort having occurred just before the function is called.
 */

void strip_blank_transactions (file_data *file)
{
  int i, j, found;


  i = file->trans_count - 1;

  while (is_transaction_blank (file, i))
  {
    /* Search the trasnaction sort index, looking for a line pointing to the blank.  If one is found, shuffle all
     * the following indexes up to compensate.
     */

    found = 0;

    for (j=0; j<i; j++)
    {
      if (file->transactions[j].sort_index == i)
      {
        found = 1;
      }

      if (found)
      {
        file->transactions[j].sort_index = file->transactions[j+1].sort_index;
      }
    }

    /* Remove the transaction. */

    i--;
  }

  /* Shuffle memory to reduce the transaction space. */

  if (i < file->trans_count-1)
  {
    file->trans_count = i+1;

    flex_extend ((flex_ptr) &(file->transactions), sizeof (struct transaction) * file->trans_count);
  }
}








/* ------------------------------------------------------------------------------------------------------------------ */










/**
 * Open the Transaction List Sort dialogue for a given transaction list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void transact_open_sort_window(file_data *file, wimp_pointer *ptr)
{
	if (windows_get_open(transact_sort_window))
		wimp_close_window(transact_sort_window);

	transact_fill_sort_window(file->transaction_window.sort_order);

	transact_sort_file = file;

	windows_open_centred_at_pointer(transact_sort_window, ptr);
	place_dialogue_caret(transact_sort_window, wimp_ICON_WINDOW);
}


/**
 * Process mouse clicks in the Transaction List Sort dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void transact_sort_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case TRANS_SORT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(transact_sort_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			transact_refresh_sort_window();
		break;

	case TRANS_SORT_OK:
		if (transact_process_sort_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(transact_sort_window);
		break;
	}
}


/**
 * Process keypresses in the Transaction List Sort window.
 *
 * \param *key			The keypress event block to handle.
 * \return			TRUE if the event was handled; else FALSE.
 */

static osbool transact_sort_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (transact_process_sort_window())
			close_dialogue_with_caret(transact_sort_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(transact_sort_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Transaction List Sort window.
 */

static void transact_refresh_sort_window(void)
{
	transact_fill_sort_window(transact_sort_file->transaction_window.sort_order);
}


/**
 * Update the contents of the Transaction List Sort window to reflect the
 * current settings of the given file.
 *
 * \param sort_option		The sort option currently in force.
 */

static void transact_fill_sort_window(int sort_option)
{
	icons_set_selected(transact_sort_window, TRANS_SORT_ROW, (sort_option & SORT_MASK) == SORT_ROW);
	icons_set_selected(transact_sort_window, TRANS_SORT_DATE, (sort_option & SORT_MASK) == SORT_DATE);
	icons_set_selected(transact_sort_window, TRANS_SORT_FROM, (sort_option & SORT_MASK) == SORT_FROM);
	icons_set_selected(transact_sort_window, TRANS_SORT_TO, (sort_option & SORT_MASK) == SORT_TO);
	icons_set_selected(transact_sort_window, TRANS_SORT_REFERENCE, (sort_option & SORT_MASK) == SORT_REFERENCE);
	icons_set_selected(transact_sort_window, TRANS_SORT_AMOUNT, (sort_option & SORT_MASK) == SORT_AMOUNT);
	icons_set_selected(transact_sort_window, TRANS_SORT_DESCRIPTION, (sort_option & SORT_MASK) == SORT_DESCRIPTION);

	icons_set_selected(transact_sort_window, TRANS_SORT_ASCENDING, sort_option & SORT_ASCENDING);
	icons_set_selected(transact_sort_window, TRANS_SORT_DESCENDING, sort_option & SORT_DESCENDING);
}


/**
 * Take the contents of an updated Transaction List Sort window and process
 * the data.
 *
 * \return			TRUE if successful; else FALSE.
 */

static osbool transact_process_sort_window(void)
{
	transact_sort_file->transaction_window.sort_order = SORT_NONE;

	if (icons_get_selected(transact_sort_window, TRANS_SORT_ROW))
		transact_sort_file->transaction_window.sort_order = SORT_ROW;
	else if (icons_get_selected(transact_sort_window, TRANS_SORT_DATE))
		transact_sort_file->transaction_window.sort_order = SORT_DATE;
	else if (icons_get_selected(transact_sort_window, TRANS_SORT_FROM))
		transact_sort_file->transaction_window.sort_order = SORT_FROM;
	else if (icons_get_selected(transact_sort_window, TRANS_SORT_TO))
		transact_sort_file->transaction_window.sort_order = SORT_TO;
	else if (icons_get_selected(transact_sort_window, TRANS_SORT_REFERENCE))
		transact_sort_file->transaction_window.sort_order = SORT_REFERENCE;
	else if (icons_get_selected(transact_sort_window, TRANS_SORT_AMOUNT))
		transact_sort_file->transaction_window.sort_order = SORT_AMOUNT;
	else if (icons_get_selected(transact_sort_window, TRANS_SORT_DESCRIPTION))
		transact_sort_file->transaction_window.sort_order = SORT_DESCRIPTION;

	if (transact_sort_file->transaction_window.sort_order != SORT_NONE) {
		if (icons_get_selected(transact_sort_window, TRANS_SORT_ASCENDING))
			transact_sort_file->transaction_window.sort_order |= SORT_ASCENDING;
		else if (icons_get_selected(transact_sort_window, TRANS_SORT_DESCENDING))
			transact_sort_file->transaction_window.sort_order |= SORT_DESCENDING;
	}

	transact_adjust_sort_icon(transact_sort_file);
	windows_redraw(transact_sort_file->transaction_window.transaction_pane);
	transact_sort(transact_sort_file);

	return TRUE;
}


/**
 * Force the closure of the Transaction List sort and edit windows if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void transact_force_windows_closed(file_data *file)
{
	if (transact_sort_file == file && windows_get_open(transact_sort_window))
		close_dialogue_with_caret(transact_sort_window);
}


/**
 * Sort the contents of the transaction window based on the file's sort setting.
 *
 * \param *file			The file to sort.
 */

void transact_sort(file_data *file)
{
	wimp_caret	caret;
	int		gap, comb, temp, order, edit_transaction;
	osbool		sorted, reorder;

#ifdef DEBUG
	debug_printf("Sorting transaction window");
#endif

	hourglass_on();

	/* Find the caret position and edit line before sorting. */

	wimp_get_caret_position(&caret);
	edit_transaction = edit_get_line_transaction(file);

	/* Sort the entries using a combsort.  This has the advantage over qsort() that the order of entries is only
	 * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
	 */

	gap = file->trans_count - 1;

	order = file->transaction_window.sort_order;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((file->trans_count >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < file->trans_count; comb++) {
			switch (order) {
			case SORT_ROW | SORT_ASCENDING:
				reorder = (transact_get_transaction_number(file->transactions[comb+gap].sort_index) <
						transact_get_transaction_number(file->transactions[comb].sort_index));
				break;

			case SORT_ROW | SORT_DESCENDING:
				reorder = (transact_get_transaction_number(file->transactions[comb+gap].sort_index) >
						transact_get_transaction_number(file->transactions[comb].sort_index));
				break;

			case SORT_DATE | SORT_ASCENDING:
				reorder = (file->transactions[file->transactions[comb+gap].sort_index].date <
						file->transactions[file->transactions[comb].sort_index].date);
				break;

			case SORT_DATE | SORT_DESCENDING:
				reorder = (file->transactions[file->transactions[comb+gap].sort_index].date >
						file->transactions[file->transactions[comb].sort_index].date);
				break;

			case SORT_FROM | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(file, file->transactions[file->transactions[comb+gap].sort_index].from),
						account_get_name(file, file->transactions[file->transactions[comb].sort_index].from)) < 0);
				break;

			case SORT_FROM | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(file, file->transactions[file->transactions[comb+gap].sort_index].from),
						account_get_name(file, file->transactions[file->transactions[comb].sort_index].from)) > 0);
				break;

			case SORT_TO | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(file, file->transactions[file->transactions[comb+gap].sort_index].to),
						account_get_name(file, file->transactions[file->transactions[comb].sort_index].to)) < 0);
				break;

			case SORT_TO | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(file, file->transactions[file->transactions[comb+gap].sort_index].to),
						account_get_name(file, file->transactions[file->transactions[comb].sort_index].to)) > 0);
				break;

			case SORT_REFERENCE | SORT_ASCENDING:
				reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].reference,
						file->transactions[file->transactions[comb].sort_index].reference) < 0);
				break;

			case SORT_REFERENCE | SORT_DESCENDING:
				reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].reference,
						file->transactions[file->transactions[comb].sort_index].reference) > 0);
				break;

			case SORT_AMOUNT | SORT_ASCENDING:
				reorder = (file->transactions[file->transactions[comb+gap].sort_index].amount <
						file->transactions[file->transactions[comb].sort_index].amount);
				break;

			case SORT_AMOUNT | SORT_DESCENDING:
				reorder = (file->transactions[file->transactions[comb+gap].sort_index].amount >
						file->transactions[file->transactions[comb].sort_index].amount);
				break;

			case SORT_DESCRIPTION | SORT_ASCENDING:
				reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].description,
						file->transactions[file->transactions[comb].sort_index].description) < 0);
				break;

			case SORT_DESCRIPTION | SORT_DESCENDING:
				reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].description,
						file->transactions[file->transactions[comb].sort_index].description) > 0);
				break;

			default:
				reorder = FALSE;
				break;
			}

			if (reorder) {
				temp = file->transactions[comb+gap].sort_index;
				file->transactions[comb+gap].sort_index = file->transactions[comb].sort_index;
				file->transactions[comb].sort_index = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

	/* Replace the edit line where we found it prior to the sort. */

	edit_place_new_line_by_transaction(file, edit_transaction);

	/* If the caret's position was in the current transaction window, we need to
	 * replace it in the same position now, so that we don't lose input focus.
	 */

	if (file->transaction_window.transaction_window != NULL &&
			file->transaction_window.transaction_window == caret.w)
		wimp_set_caret_position(caret.w, caret.i, 0, 0, -1, caret.index);

	force_transaction_window_redraw(file, 0, file->trans_count - 1);

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

void transact_sort_file_data(file_data *file)
{
	int			i, gap, comb;
	osbool			sorted;
	struct transaction	temp;

#ifdef DEBUG
	debug_printf("Sorting transactions");
#endif

	hourglass_on();

	/* Start by recording the order of the transactions on display in the
	 * main window, and also the order of the transactions themselves.
	 */

	for (i=0; i < file->trans_count; i++) {
		file->transactions[file->transactions[i].sort_index].saved_sort = i;	/* Record transaction window lines. */
		file->transactions[i].sort_index = i;					/* Record old transaction locations. */
	}

	/* Sort the entries using a combsort.  This has the advantage over qsort()
	 * that the order of entries is only affected if they are not equal and are
	 * in descending order.  Otherwise, the status quo is left.
	 */

	gap = file->trans_count - 1;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((file->trans_count >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < file->trans_count; comb++) {
			if (file->transactions[comb+gap].date < file->transactions[comb].date) {
				temp = file->transactions[comb+gap];
				file->transactions[comb+gap] = file->transactions[comb];
				file->transactions[comb] = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

	/* Finally, restore the order of the transactions on display in the
	 * main window.
	 */

	for (i=0; i < file->trans_count; i++)
		file->transactions[file->transactions[i].sort_index].sort_workspace = i;

	accview_reindex_all(file);

	for (i=0; i < file->trans_count; i++)
		file->transactions[file->transactions[i].saved_sort].sort_index = i;

	file->sort_valid = TRUE;

	hourglass_off();
}













/* ==================================================================================================================
 * Finding transactions
 */

void find_next_reconcile_line (file_data *file, int set)
{
  int        line, found, account;
  wimp_caret caret;


  if (file->auto_reconcile)
  {
    line = file->transaction_window.entry_line;
    account = NULL_ACCOUNT;

    wimp_get_caret_position (&caret);

    if (caret.i == 1)
    {
      account = file->transactions[file->transactions[line].sort_index].from;
    }
    else if (caret.i == 4)
    {
      account = file->transactions[file->transactions[line].sort_index].to;
    }

    if (account != NULL_ACCOUNT)
    {
      line++;
      found = 0;

      while (line < file->trans_count && !found)
      {
        if (file->transactions[file->transactions[line].sort_index].from == account &&
            ((file->transactions[file->transactions[line].sort_index].flags & TRANS_REC_FROM) == set * TRANS_REC_FROM))
        {
          found = 1;
        }

        else if (file->transactions[file->transactions[line].sort_index].to == account &&
                 ((file->transactions[file->transactions[line].sort_index].flags & TRANS_REC_TO) == set * TRANS_REC_TO))
        {
          found = 4;
        }

        else
        {
          line++;
        }
      }

      if (found)
      {
        edit_place_new_line(file, line);
        icons_put_caret_at_end(file->transaction_window.transaction_window, found);
        edit_find_line_vertically(file);
      }
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the first blank line at the end of the transaction window */

int find_first_blank_line (file_data *file)
{
  int line;


  #ifdef DEBUG
  debug_printf ("\\DFinding first blank line");
  #endif

  line = file->trans_count;

  while (line > 0 && is_transaction_blank (file, file->transactions[line - 1].sort_index))
  {
    line--;

    #ifdef DEBUG
    debug_printf ("Stepping back up...");
    #endif
  }

  return (line);
}



/**
 * Open the Transaction Print dialogue for a given account list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void transact_open_print_window(file_data *file, wimp_pointer *ptr, int clear)
{
	transact_print_file = file;

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
	struct report			*report;
	int				i, t;
	char				line[4096], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];
	struct transaction_window	*window;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);
	msgs_lookup("PrintTitleTransact", buffer, sizeof(buffer));
	report = report_open(transact_print_file, buffer, NULL);

	if (report == NULL) {
		error_msgs_report_error("PrintMemFail");
		return;
	}

	hourglass_on();

	window = &(transact_print_file->transaction_window);

	/* Output the page title. */

	file_get_leafname(transact_print_file, numbuf1, sizeof(numbuf1));
	msgs_param_lookup("TransTitle", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
	sprintf(line, "\\b\\u%s", buffer);
	report_write_line(report, 1, line);
	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	*line = '\0';
	sprintf(buffer, "\\k\\b\\u%s\\t", icons_copy_text(window->transaction_pane, TRANSACT_PANE_ROW, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->transaction_pane, TRANSACT_PANE_DATE, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(window->transaction_pane, TRANSACT_PANE_FROM, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(window->transaction_pane, TRANSACT_PANE_TO, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->transaction_pane, TRANSACT_PANE_REFERENCE, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\r\\u%s\\t", icons_copy_text(window->transaction_pane, TRANSACT_PANE_AMOUNT, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->transaction_pane, TRANSACT_PANE_DESCRIPTION, numbuf1));
	strcat(line, buffer);

	report_write_line(report, 0, line);

	/* Output the transaction data as a set of delimited lines. */

	for (i=0; i < transact_print_file->trans_count; i++) {
		if ((from == NULL_DATE || transact_print_file->transactions[i].date >= from) &&
				(to == NULL_DATE || transact_print_file->transactions[i].date <= to)) {
			*line = '\0';

			t = transact_print_file->transactions[i].sort_index;

			convert_date_to_string(transact_print_file->transactions[t].date, numbuf1);
			sprintf(buffer, "\\k\\d\\r%d\\t%s\\t", transact_get_transaction_number(t), numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_ident(transact_print_file, transact_print_file->transactions[t].from));
			strcat(line, buffer);

			strcpy(numbuf1, (transact_print_file->transactions[t].flags & TRANS_REC_FROM) ? rec_char : "");
			sprintf(buffer, "%s\\t", numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_name(transact_print_file, transact_print_file->transactions[t].from));
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_ident(transact_print_file, transact_print_file->transactions[t].to));
			strcat(line, buffer);

			strcpy(numbuf1, (transact_print_file->transactions[t].flags & TRANS_REC_TO) ? rec_char : "");
			sprintf(buffer, "%s\\t", numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", account_get_name(transact_print_file, transact_print_file->transactions[t].to));
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", transact_print_file->transactions[t].reference);
			strcat(line, buffer);

			convert_money_to_string(transact_print_file->transactions[t].amount, numbuf1);
			sprintf(buffer, "\\r%s\\t", numbuf1);
			strcat(line, buffer);

			sprintf(buffer, "%s\\t", transact_print_file->transactions[t].description);
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

void transact_write_file(file_data *file, FILE *out)
{
	int	i;
	char	buffer[MAX_FILE_LINE_LEN];

	fprintf(out, "\n[Transactions]\n");

	fprintf(out, "Entries: %x\n", file->trans_count);

	column_write_as_text(file->transaction_window.column_width, TRANSACT_COLUMNS, buffer);
	fprintf(out, "WinColumns: %s\n", buffer);

	fprintf(out, "SortOrder: %x\n", file->transaction_window.sort_order);

	for (i = 0; i < file->trans_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x,%x\n",
				file->transactions[i].date, file->transactions[i].flags, file->transactions[i].from,
				file->transactions[i].to, file->transactions[i].amount);
		if (*(file->transactions[i].reference) != '\0')
			config_write_token_pair(out, "Ref", file->transactions[i].reference);
		if (*(file->transactions[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->transactions[i].description);
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

int transact_read_file(file_data *file, FILE *in, char *section, char *token, char *value, int format, osbool *unknown_data)
{
	int	result, block_size, i = -1;

	block_size = flex_size((flex_ptr) &(file->transactions)) / sizeof(struct transaction);

	do {
		if (string_nocase_strcmp(token, "Entries") == 0) {
			block_size = strtoul(value, NULL, 16);
			if (block_size > file->trans_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->transactions), sizeof(struct transaction) * block_size);
			} else {
				block_size = file->trans_count;
			}
		} else if (string_nocase_strcmp(token, "WinColumns") == 0) {
			/* For file format 1.00 or older, there's no row column at the
			 * start of the line so skip on to colukn 1 (date).
			 */
			column_init_window(file->transaction_window.column_width,
				file->transaction_window.column_position,
				TRANSACT_COLUMNS, (format <= 100) ? 1 : 0, TRUE, value);
		} else if (string_nocase_strcmp(token, "SortOrder") == 0){
			file->transaction_window.sort_order = strtoul(value, NULL, 16);
		} else if (string_nocase_strcmp(token, "@") == 0) {
			file->trans_count++;
			if (file->trans_count > block_size) {
				block_size = file->trans_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->transactions), sizeof(struct transaction) * block_size);
			}
			i = file->trans_count-1;
			file->transactions[i].date = strtoul(next_field (value, ','), NULL, 16);
			file->transactions[i].flags = strtoul(next_field (NULL, ','), NULL, 16);
			file->transactions[i].from = strtoul(next_field (NULL, ','), NULL, 16);
			file->transactions[i].to = strtoul(next_field (NULL, ','), NULL, 16);
			file->transactions[i].amount = strtoul(next_field (NULL, ','), NULL, 16);

			*(file->transactions[i].reference) = '\0';
			*(file->transactions[i].description) = '\0';

			file->transactions[i].sort_index = i;
		} else if (i != -1 && string_nocase_strcmp(token, "Ref") == 0) {
			strcpy(file->transactions[i].reference, value);
		} else if (i != -1 && string_nocase_strcmp(token, "Desc") == 0) {
			strcpy(file->transactions[i].description, value);
		} else {
			*unknown_data = TRUE;
		}

		result = config_read_token_pair(in, token, value, section);
	} while (result != sf_READ_CONFIG_EOF && result != sf_READ_CONFIG_NEW_SECTION);

	block_size = flex_size((flex_ptr) &(file->transactions)) / sizeof(struct transaction);

	#ifdef DEBUG
	debug_printf("Transaction block size: %d, required: %d", block_size, file->trans_count);
	#endif

	if (block_size > file->trans_count) {
		block_size = file->trans_count;
		flex_extend((flex_ptr) &(file->transactions), sizeof(struct transaction) * block_size);

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

static void transact_start_direct_save(struct transaction_window *windat)
{
	wimp_pointer	pointer;
	char		*filename;

	if (windat == NULL || windat->file == NULL)
		return;

	if (file_check_for_filepath(windat->file)) {
		save_transaction_file(windat->file, windat->file->filename);
	} else {
		wimp_get_pointer_info(&pointer);


			if (file_check_for_filepath(windat->file))
				filename = windat->file->filename;
			else
				filename = NULL;

		saveas_initialise_dialogue(transact_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, windat);
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
	struct account_window *windat = data;

	if (windat == NULL || windat->file == NULL)
		return FALSE;

	save_transaction_file(windat->file, filename);

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
	struct account_window *windat = data;

	if (windat == NULL || windat->file == NULL)
		return FALSE;

	transact_export_delimited(windat->file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);

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
	struct account_window *windat = data;

	if (windat == NULL || windat->file == NULL)
		return FALSE;

	transact_export_delimited(windat->file, filename, DELIMIT_TAB, TSV_FILE_TYPE);

	return TRUE;
}


/**
 * Export the transaction data from a file into CSV or TSV format.
 *
 * \param *file			The file to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void transact_export_delimited(file_data *file, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE	*out;
	int	i, t;
	char	buffer[256];

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	icons_copy_text(file->transaction_window.transaction_pane, TRANSACT_PANE_ROW, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(file->transaction_window.transaction_pane, TRANSACT_PANE_DATE, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(file->transaction_window.transaction_pane, TRANSACT_PANE_FROM, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(file->transaction_window.transaction_pane, TRANSACT_PANE_TO, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(file->transaction_window.transaction_pane, TRANSACT_PANE_REFERENCE, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(file->transaction_window.transaction_pane, TRANSACT_PANE_AMOUNT, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(file->transaction_window.transaction_pane, TRANSACT_PANE_DESCRIPTION, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_LAST);

	/* Output the transaction data as a set of delimited lines. */

	for (i=0; i < file->trans_count; i++) {
		t = file->transactions[i].sort_index;

		snprintf(buffer, 256, "%d", transact_get_transaction_number(t));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		convert_date_to_string(file->transactions[t].date, buffer);
		filing_output_delimited_field(out, buffer, format, 0);

		account_build_name_pair(file, file->transactions[t].from, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, 0);

		account_build_name_pair(file, file->transactions[t].to, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, 0);

		filing_output_delimited_field(out, file->transactions[t].reference, format, 0);

		convert_money_to_string(file->transactions[t].amount, buffer);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, file->transactions[t].description, format, DELIMIT_LAST);
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
	file_data	*file = data;

	if (filetype != CSV_FILE_TYPE || file == NULL)
		return FALSE;

	import_csv_file(file, filename);

	return TRUE;
}


/**
 * Check the transactions in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool transact_check_account(file_data *file, int account)
{
	int		i;
	osbool		found = FALSE;

	for (i = 0; i < file->trans_count && !found; i++)
		if (file->transactions[i].from == account || file->transactions[i].to == account)
			found = TRUE;

	return found;
}


/**
 * Calculate the details of a file, and fill the file info dialogue.
 *
 * \param *file			The file to display data for.
 */

static void transact_prepare_fileinfo(file_data *file)
{
	file_get_pathname(file, icons_get_indirected_text_addr(transact_fileinfo_window, FILEINFO_ICON_FILENAME), 255);

	if (file_check_for_filepath(file))
		territory_convert_standard_date_and_time(territory_CURRENT, (os_date_and_time const *) file->datestamp,
				icons_get_indirected_text_addr(transact_fileinfo_window, FILEINFO_ICON_DATE), 30);
	else
		icons_msgs_lookup(transact_fileinfo_window, FILEINFO_ICON_DATE, "UnSaved");

	if (file->modified)
		icons_msgs_lookup(transact_fileinfo_window, FILEINFO_ICON_MODIFIED, "Yes");
	else
		icons_msgs_lookup(transact_fileinfo_window, FILEINFO_ICON_MODIFIED, "No");

	icons_printf(transact_fileinfo_window, FILEINFO_ICON_TRANSACT, "%d", file->trans_count);
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_SORDERS, "%d", file->sorder_count);
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_PRESETS, "%d", file->preset_count);
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_ACCOUNTS, "%d", account_count_type_in_file(file, ACCOUNT_FULL));
	icons_printf(transact_fileinfo_window, FILEINFO_ICON_HEADINGS, "%d", account_count_type_in_file(file, ACCOUNT_IN | ACCOUNT_OUT));
}

