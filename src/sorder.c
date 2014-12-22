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
 * \file: sorder.c
 *
 * Standing Order implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "sorder.h"

#include "account.h"
#include "accview.h"
#include "caret.h"
#include "column.h"
#include "conversion.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "saveas.h"
#include "report.h"
#include "templates.h"
#include "transact.h"
#include "window.h"


/* Main Window Icons
 *
 * Note that these correspond to column numbers.
 */

#define SORDER_ICON_FROM 0
#define SORDER_ICON_FROM_REC 1
#define SORDER_ICON_FROM_NAME 2
#define SORDER_ICON_TO 3
#define SORDER_ICON_TO_REC 4
#define SORDER_ICON_TO_NAME 5
#define SORDER_ICON_AMOUNT 6
#define SORDER_ICON_DESCRIPTION 7
#define SORDER_ICON_NEXTDATE 8
#define SORDER_ICON_LEFT 9

/* SOrder menu */

#define SORDER_MENU_SORT 0
#define SORDER_MENU_EDIT 1
#define SORDER_MENU_NEWSORDER 2
#define SORDER_MENU_EXPCSV 3
#define SORDER_MENU_EXPTSV 4
#define SORDER_MENU_PRINT 5
#define SORDER_MENU_FULLREP 6
/* Standing order edit icons. */

#define SORDER_EDIT_OK 0
#define SORDER_EDIT_CANCEL 1
#define SORDER_EDIT_STOP 34
#define SORDER_EDIT_DELETE 35

#define SORDER_EDIT_START 3
#define SORDER_EDIT_NUMBER 5
#define SORDER_EDIT_PERIOD 7
#define SORDER_EDIT_PERDAYS 8
#define SORDER_EDIT_PERMONTHS 9
#define SORDER_EDIT_PERYEARS 10
#define SORDER_EDIT_AVOID 11
#define SORDER_EDIT_SKIPFWD 12
#define SORDER_EDIT_SKIPBACK 13
#define SORDER_EDIT_FMIDENT 17
#define SORDER_EDIT_FMREC 32
#define SORDER_EDIT_FMNAME 18
#define SORDER_EDIT_TOIDENT 20
#define SORDER_EDIT_TOREC 33
#define SORDER_EDIT_TONAME 21
#define SORDER_EDIT_REF 23
#define SORDER_EDIT_AMOUNT 25
#define SORDER_EDIT_FIRSTSW 26
#define SORDER_EDIT_FIRST 27
#define SORDER_EDIT_LASTSW 28
#define SORDER_EDIT_LAST 29
#define SORDER_EDIT_DESC 31

/* Toolbar icons. */

#define SORDER_PANE_FROM 0
#define SORDER_PANE_TO 1
#define SORDER_PANE_AMOUNT 2
#define SORDER_PANE_DESCRIPTION 3
#define SORDER_PANE_NEXTDATE 4
#define SORDER_PANE_LEFT 5

#define SORDER_PANE_PARENT 6
#define SORDER_PANE_ADDSORDER 7
#define SORDER_PANE_PRINT 8
#define SORDER_PANE_SORT 9

/* Standing Order Sort Window */

#define SORDER_SORT_OK 2
#define SORDER_SORT_CANCEL 3
#define SORDER_SORT_FROM 4
#define SORDER_SORT_TO 5
#define SORDER_SORT_AMOUNT 6
#define SORDER_SORT_DESCRIPTION 7
#define SORDER_SORT_NEXTDATE 8
#define SORDER_SORT_LEFT 9
#define SORDER_SORT_ASCENDING 10
#define SORDER_SORT_DESCENDING 11

#define SORDER_PANE_COL_MAP "0,1,2;3,4,5;6;7;8;9"
#define SORDER_PANE_SORT_DIR_ICON 10

/**
 * Standing order data structure -- implementation.
 */

struct sorder {
	date_t			start_date;					/**< The date on which the first order should be processed. */
	int			number;						/**< The number of orders to be added. */
	int			period;						/**< The period between orders. */
	enum date_period	period_unit;					/**< The unit in which the period is measured. */

	date_t			raw_next_date;					/**< The uncorrected date for the next order, used for getting the next. */
	date_t			adjusted_next_date;				/**< The date of the next order, taking into account months, weekends etc. */

	int			left;						/**< The number of orders remaining. */

	enum transact_flags	flags;						/**< Order flags (containing transaction flags, order flags, etc). */

	acct_t			from;						/**< Order details. */
	acct_t			to;
	amt_t			normal_amount;
	amt_t			first_amount;
	amt_t			last_amount;
	char			reference[REF_FIELD_LEN];
	char			description[DESCRIPT_FIELD_LEN];

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the sorder data, and are in effect a separate array that is used
	 * for handling entries in the sorder window.
	 */

	int			sort_index;       /* Point to another order, to allow the sorder window to be sorted. */
};

/* Standing Order Edit Window. */

static wimp_w			sorder_edit_window = NULL;			/**< The handle of the standing order edit window.			*/
static file_data		*sorder_edit_file = NULL;			/**< The file currently owning the standing order edit window.		*/
static int			sorder_edit_number = -1;			/**< The standing order currently being edited.				*/

/* Standing Order Sort Window. */

static wimp_w			sorder_sort_window = NULL;			/**< The handle of the standing order sort window.			*/
static file_data		*sorder_sort_file = NULL;			/**< The file currently owning the standing order sort window.		*/

/* Standing Order Print Window. */

static file_data		*sorder_print_file = NULL;			/**< The file currently owning the standing order print window.		*/

/* Standing Order List Window. */

static wimp_window		*sorder_window_def = NULL;			/**< The definition for the Standing Order Window.			*/
static wimp_window		*sorder_pane_def = NULL;			/**< The definition for the Standing Order Window pane.			*/
static wimp_menu		*sorder_window_menu = NULL;			/**< The Standing Order Window menu handle.				*/
static int			sorder_window_menu_line = -1;			/**< The line over which the Standing Order Window Menu was opened.	*/
static wimp_i			sorder_substitute_sort_icon = SORDER_PANE_FROM;	/**< The icon currently obscured by the sort icon.			*/

/* SaveAs Dialogue Handles. */

static struct saveas_block	*sorder_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*sorder_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/


static void			sorder_close_window_handler(wimp_close *close);
static void			sorder_window_click_handler(wimp_pointer *pointer);
static void			sorder_pane_click_handler(wimp_pointer *pointer);
static void			sorder_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void			sorder_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void			sorder_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void			sorder_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void			sorder_window_scroll_handler(wimp_scroll *scroll);
static void			sorder_window_redraw_handler(wimp_draw *redraw);
static void			sorder_adjust_window_columns(void *data, wimp_i group, int width);
static void			sorder_adjust_sort_icon(file_data *file);
static void			sorder_adjust_sort_icon_data(file_data *file, wimp_icon *icon);
static void			sorder_set_window_extent(file_data *file);
static void			sorder_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

static void			sorder_edit_click_handler(wimp_pointer *pointer);
static osbool			sorder_edit_keypress_handler(wimp_key *key);
static void			sorder_refresh_edit_window(void);
static void			sorder_fill_edit_window(file_data *file, int sorder, osbool edit_mode);
static osbool			sorder_process_edit_window(void);
static osbool			sorder_delete_from_edit_window(void);
static osbool			sorder_stop_from_edit_window(void);

static void			sorder_open_sort_window(file_data *file, wimp_pointer *ptr);
static void			sorder_sort_click_handler(wimp_pointer *pointer);
static osbool			sorder_sort_keypress_handler(wimp_key *key);
static void			sorder_refresh_sort_window(void);
static void			sorder_fill_sort_window(int sort_option);
static osbool			sorder_process_sort_window(void);

static void			sorder_open_print_window(file_data *file, wimp_pointer *ptr, osbool restore);
static void			sorder_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);

static int			sorder_add(file_data *file);
static osbool			sorder_delete(file_data *file, int sorder);

static osbool			sorder_save_csv(char *filename, osbool selection, void *data);
static osbool			sorder_save_tsv(char *filename, osbool selection, void *data);
static void			sorder_export_delimited(file_data *file, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Initialise the standing order system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_initialise(osspriteop_area *sprites)
{
	sorder_edit_window = templates_create_window("EditSOrder");
	ihelp_add_window(sorder_edit_window, "EditSOrder", NULL);
	event_add_window_mouse_event(sorder_edit_window, sorder_edit_click_handler);
	event_add_window_key_event(sorder_edit_window, sorder_edit_keypress_handler);
	event_add_window_icon_radio(sorder_edit_window, SORDER_EDIT_PERDAYS, TRUE);
	event_add_window_icon_radio(sorder_edit_window, SORDER_EDIT_PERMONTHS, TRUE);
	event_add_window_icon_radio(sorder_edit_window, SORDER_EDIT_PERYEARS, TRUE);
	event_add_window_icon_radio(sorder_edit_window, SORDER_EDIT_SKIPFWD, TRUE);
	event_add_window_icon_radio(sorder_edit_window, SORDER_EDIT_SKIPBACK, TRUE);

	sorder_sort_window = templates_create_window("SortSOrder");
	ihelp_add_window(sorder_sort_window, "SortSOrder", NULL);
	event_add_window_mouse_event(sorder_sort_window, sorder_sort_click_handler);
	event_add_window_key_event(sorder_sort_window, sorder_sort_keypress_handler);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_FROM, TRUE);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_TO, TRUE);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_AMOUNT, TRUE);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_DESCRIPTION, TRUE);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_NEXTDATE, TRUE);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_LEFT, TRUE);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_ASCENDING, TRUE);
	event_add_window_icon_radio(sorder_sort_window, SORDER_SORT_DESCENDING, TRUE);

	sorder_window_def = templates_load_window("SOrder");
	sorder_window_def->icon_count = 0;

	sorder_pane_def = templates_load_window("SOrderTB");
	sorder_pane_def->sprite_area = sprites;

	sorder_window_menu = templates_get_menu(TEMPLATES_MENU_SORDER);

	sorder_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", sorder_save_csv);
	sorder_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", sorder_save_tsv);
}


/**
 * Create and open a Standing Order List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void sorder_open_window(file_data *file)
{
	int			i, j, height;
	wimp_window_state	parent;
	os_error		*error;

	/* Create or re-open the window. */

	if (file->sorder_window.sorder_window != NULL) {
		windows_open(file->sorder_window.sorder_window);
		return;
	}

	#ifdef DEBUG
	debug_printf("\\CCreating standing order window");
	#endif

	/* Create the new window data and build the window. */

	*(file->sorder_window.window_title) = '\0';
	sorder_window_def->title_data.indirected_text.text = file->sorder_window.window_title;

	height =  (file->sorder_count > MIN_SORDER_ENTRIES) ? file->sorder_count : MIN_SORDER_ENTRIES;

	parent.w = file->transaction_window.transaction_pane;
	wimp_get_window_state(&parent);

	set_initial_window_area (sorder_window_def,
			file->sorder_window.column_position[SORDER_COLUMNS-1] +
			file->sorder_window.column_width[SORDER_COLUMNS-1],
			((ICON_HEIGHT+LINE_GUTTER) * height) + SORDER_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	file->child_x_offset++;
	if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
		file->child_x_offset = 0;

	error = xwimp_create_window(sorder_window_def, &(file->sorder_window.sorder_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Create the toolbar. */

	windows_place_as_toolbar(sorder_window_def, sorder_pane_def, SORDER_TOOLBAR_HEIGHT-4);

	#ifdef DEBUG
	debug_printf("Window extents set...");
	#endif

	for (i=0, j=0; j < SORDER_COLUMNS; i++, j++) {
		sorder_pane_def->icons[i].extent.x0 = file->sorder_window.column_position[j];

		j = column_get_rightmost_in_group(SORDER_PANE_COL_MAP, i);

		sorder_pane_def->icons[i].extent.x1 = file->sorder_window.column_position[j] +
				file->sorder_window.column_width[j] +
				COLUMN_HEADING_MARGIN;
	}

	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) file->sorder_window.sort_sprite;
	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			sorder_pane_def->sprite_area;

	sorder_adjust_sort_icon_data(file, &(sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON]));

	#ifdef DEBUG
	debug_printf("Toolbar icons adjusted...");
	#endif

	error = xwimp_create_window(sorder_pane_def, &(file->sorder_window.sorder_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	sorder_build_window_title(file);

	/* Open the window. */

	ihelp_add_window(file->sorder_window.sorder_window , "SOrder", sorder_decode_window_help);
	ihelp_add_window(file->sorder_window.sorder_pane , "SOrderTB", NULL);

	windows_open(file->sorder_window.sorder_window);
	windows_open_nested_as_toolbar(file->sorder_window.sorder_pane,
			file->sorder_window.sorder_window,
			SORDER_TOOLBAR_HEIGHT-4);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(file->sorder_window.sorder_window, &(file->sorder_window));
	event_add_window_menu(file->sorder_window.sorder_window, sorder_window_menu);
	event_add_window_close_event(file->sorder_window.sorder_window, sorder_close_window_handler);
	event_add_window_mouse_event(file->sorder_window.sorder_window, sorder_window_click_handler);
	event_add_window_scroll_event(file->sorder_window.sorder_window, sorder_window_scroll_handler);
	event_add_window_redraw_event(file->sorder_window.sorder_window, sorder_window_redraw_handler);
	event_add_window_menu_prepare(file->sorder_window.sorder_window, sorder_window_menu_prepare_handler);
	event_add_window_menu_selection(file->sorder_window.sorder_window, sorder_window_menu_selection_handler);
	event_add_window_menu_warning(file->sorder_window.sorder_window, sorder_window_menu_warning_handler);
	event_add_window_menu_close(file->sorder_window.sorder_window, sorder_window_menu_close_handler);

	event_add_window_user_data(file->sorder_window.sorder_pane, &(file->sorder_window));
	event_add_window_menu(file->sorder_window.sorder_pane, sorder_window_menu);
	event_add_window_mouse_event(file->sorder_window.sorder_pane, sorder_pane_click_handler);
	event_add_window_menu_prepare(file->sorder_window.sorder_pane, sorder_window_menu_prepare_handler);
	event_add_window_menu_selection(file->sorder_window.sorder_pane, sorder_window_menu_selection_handler);
	event_add_window_menu_warning(file->sorder_window.sorder_pane, sorder_window_menu_warning_handler);
	event_add_window_menu_close(file->sorder_window.sorder_pane, sorder_window_menu_close_handler);
}


/**
 * Close and delete the Standing order List Window associated with the given
 * file block.
 *
 * \param *windat		The window to delete.
 */

void sorder_delete_window(struct sorder_window *windat)
{
	#ifdef DEBUG
	debug_printf ("\\RDeleting standing order window");
	#endif

	if (windat == NULL)
		return;

	if (windat->sorder_window != NULL) {
		ihelp_remove_window (windat->sorder_window);
		event_delete_window(windat->sorder_window);
		wimp_delete_window(windat->sorder_window);
		windat->sorder_window = NULL;
	}

	if (windat->sorder_pane != NULL) {
		ihelp_remove_window (windat->sorder_pane);
		event_delete_window(windat->sorder_pane);
		wimp_delete_window(windat->sorder_pane);
		windat->sorder_pane = NULL;
	}
}


/**
 * Handle Close events on Standing Order List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void sorder_close_window_handler(wimp_close *close)
{
	struct sorder_window	*windat;

	#ifdef DEBUG
	debug_printf("\\RClosing Standing Order window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat == NULL)
		return;

	/* Close the window */

	sorder_delete_window(windat);
}


/**
 * Process mouse clicks in the Standing Order List window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sorder_window_click_handler(wimp_pointer *pointer)
{
	struct sorder_window	*windat;
	file_data		*file;
	int			line;
	wimp_window_state	window;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* Find the window type and get the line clicked on. */

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - SORDER_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

	if (line < 0 || line >= file->sorder_count)
		line = -1;

	/* Handle double-clicks, which will open an edit standing order window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1)
		sorder_open_edit_window(file, file->sorders[line].sort_index, pointer);
}


/**
 * Process mouse clicks in the Standing Order List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sorder_pane_click_handler(wimp_pointer *pointer)
{
	struct sorder_window	*windat;
	file_data		*file;
	wimp_window_state	window;
	wimp_icon_state		icon;
	int			ox;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	if (pointer->i == SORDER_PANE_SORT_DIR_ICON)
		pointer->i = sorder_substitute_sort_icon;

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case SORDER_PANE_PARENT:
			windows_open(file->transaction_window.transaction_window);
			break;

		case SORDER_PANE_PRINT:
			sorder_open_print_window(file, pointer, config_opt_read("RememberValues"));
			break;

		case SORDER_PANE_ADDSORDER:
			sorder_open_edit_window(file, NULL_SORDER, pointer);
			break;

		case SORDER_PANE_SORT:
			sorder_open_sort_window(file, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case SORDER_PANE_PRINT:
			sorder_open_print_window(file, pointer, !config_opt_read("RememberValues"));
			break;

		case SORDER_PANE_SORT:
			sorder_sort(file);
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
			case SORDER_PANE_FROM:
				windat->sort_order = SORT_FROM;
				break;

			case SORDER_PANE_TO:
				windat->sort_order = SORT_TO;
				break;

			case SORDER_PANE_AMOUNT:
				windat->sort_order = SORT_AMOUNT;
				break;

			case SORDER_PANE_DESCRIPTION:
				windat->sort_order = SORT_DESCRIPTION;
				break;

			case SORDER_PANE_NEXTDATE:
				windat->sort_order = SORT_NEXTDATE;
				break;

			case SORDER_PANE_LEFT:
				windat->sort_order = SORT_LEFT;
				break;
			}

			if (windat->sort_order != SORT_NONE) {
				if (pointer->buttons == wimp_CLICK_SELECT * 256)
					windat->sort_order |= SORT_ASCENDING;
				else
					windat->sort_order |= SORT_DESCENDING;
			}

			sorder_adjust_sort_icon(file);
			windows_redraw(windat->sorder_pane);
			sorder_sort(file);
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT) {
		column_start_drag(pointer, windat, windat->sorder_window,
				SORDER_PANE_COL_MAP, config_str_read("LimSOrderCols"), sorder_adjust_window_columns);
	}
}


/**
 * Process menu prepare events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void sorder_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct sorder_window	*windat;
	int			line;
	wimp_window_state	window;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (pointer != NULL) {
		sorder_window_menu_line = -1;

		if (w == windat->sorder_window) {
			window.w = w;
			wimp_get_window_state(&window);


			line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - SORDER_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

			if (line >= 0 && line < windat->file->sorder_count)
				sorder_window_menu_line = line;
		}

		saveas_initialise_dialogue(sorder_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(sorder_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_shade_entry(sorder_window_menu, SORDER_MENU_EDIT, sorder_window_menu_line == -1);
}


/**
 * Process menu selection events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void sorder_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct sorder_window	*windat;
	file_data		*file;
	wimp_pointer		pointer;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case SORDER_MENU_SORT:
		sorder_open_sort_window(file, &pointer);
		break;

	case SORDER_MENU_EDIT:
		if (sorder_window_menu_line != -1)
			sorder_open_edit_window(file, file->sorders[sorder_window_menu_line].sort_index, &pointer);
		break;

	case SORDER_MENU_NEWSORDER:
		sorder_open_edit_window(file, NULL_PRESET, &pointer);
		break;

	case SORDER_MENU_PRINT:
		sorder_open_print_window(file, &pointer, config_opt_read("RememberValues"));
		break;

	case SORDER_MENU_FULLREP:
		sorder_full_report(file);
		break;
	}
}


/**
 * Process submenu warning events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void sorder_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct sorder_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	switch (warning->selection.items[0]) {
	case SORDER_MENU_EXPCSV:
		saveas_prepare_dialogue(sorder_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case SORDER_MENU_EXPTSV:
		saveas_prepare_dialogue(sorder_saveas_tsv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process menu close events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void sorder_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	sorder_window_menu_line = -1;
}


/**
 * Process scroll events in the Standing Order List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void sorder_window_scroll_handler(wimp_scroll *scroll)
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

	height = (scroll->visible.y1 - scroll->visible.y0) - SORDER_TOOLBAR_HEIGHT;

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
 * Process redraw events in the Standing Order List window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void sorder_window_redraw_handler(wimp_draw *redraw)
{
	struct sorder_window	*windat;
	file_data		*file;
	int			ox, oy, top, base, y, i, t;
	char			icon_buffer[DESCRIPT_FIELD_LEN], rec_char[REC_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	/* Set the horizontal positions of the icons. */

	for (i=0; i < SORDER_COLUMNS; i++) {
		sorder_window_def->icons[i].extent.x0 = windat->column_position[i];
		sorder_window_def->icons[i].extent.x1 = windat->column_position[i] +
				windat->column_width[i];
		sorder_window_def->icons[i].data.indirected_text.text = icon_buffer;
	}

	/* Perform the redraw. */

	while (more) {
		/* Calculate the rows to redraw. */

		top = (oy - redraw->clip.y1 - SORDER_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
		if (top < 0)
			top = 0;

		base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2) +
				oy - redraw->clip.y0 - SORDER_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			t = (y < file->sorder_count) ? file->sorders[y].sort_index : 0;

			/* Plot out the background with a filled white rectangle. */

			wimp_set_colour(wimp_COLOUR_WHITE);
			os_plot(os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - SORDER_TOOLBAR_HEIGHT);
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO,
					ox + windat->column_position[SORDER_COLUMNS-1] +
					windat->column_width[SORDER_COLUMNS-1],
					oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - SORDER_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

			/* From field */

			sorder_window_def->icons[SORDER_ICON_FROM].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_FROM].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;

			sorder_window_def->icons[SORDER_ICON_FROM_REC].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_FROM_REC].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;

			sorder_window_def->icons[SORDER_ICON_FROM_NAME].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_FROM_NAME].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;

			if (y < file->sorder_count && file->sorders[t].from != NULL_ACCOUNT) {
				sorder_window_def->icons[SORDER_ICON_FROM].data.indirected_text.text =
						account_get_ident(file, file->sorders[t].from);
				sorder_window_def->icons[SORDER_ICON_FROM_REC].data.indirected_text.text = icon_buffer;
				sorder_window_def->icons[SORDER_ICON_FROM_NAME].data.indirected_text.text =
						account_get_name(file, file->sorders[t].from);

				if (file->sorders[t].flags & TRANS_REC_FROM)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else {
				sorder_window_def->icons[SORDER_ICON_FROM].data.indirected_text.text = icon_buffer;
				sorder_window_def->icons[SORDER_ICON_FROM_REC].data.indirected_text.text = icon_buffer;
				sorder_window_def->icons[SORDER_ICON_FROM_NAME].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_FROM]));
			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_FROM_REC]));
			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_FROM_NAME]));

			/* To field */

			sorder_window_def->icons[SORDER_ICON_TO].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_TO].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;

			sorder_window_def->icons[SORDER_ICON_TO_REC].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_TO_REC].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;

			sorder_window_def->icons[SORDER_ICON_TO_NAME].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_TO_NAME].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;

			if (y < file->sorder_count && file->sorders[t].to != NULL_ACCOUNT) {
				sorder_window_def->icons[SORDER_ICON_TO].data.indirected_text.text =
						account_get_ident(file, file->sorders[t].to);
				sorder_window_def->icons[SORDER_ICON_TO_REC].data.indirected_text.text = icon_buffer;
				sorder_window_def->icons[SORDER_ICON_TO_NAME].data.indirected_text.text =
						account_get_name(file, file->sorders[t].to);

				if (file->sorders[t].flags & TRANS_REC_TO)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else {
				sorder_window_def->icons[SORDER_ICON_TO].data.indirected_text.text = icon_buffer;
				sorder_window_def->icons[SORDER_ICON_TO_REC].data.indirected_text.text = icon_buffer;
				sorder_window_def->icons[SORDER_ICON_TO_NAME].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_TO]));
			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_TO_REC]));
			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_TO_NAME]));

			/* Amount field */

			sorder_window_def->icons[SORDER_ICON_AMOUNT].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_AMOUNT].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;
			if (y < file->sorder_count)
				convert_money_to_string(file->sorders[t].normal_amount, icon_buffer);
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_AMOUNT]));

			/* Comments field */

			sorder_window_def->icons[SORDER_ICON_DESCRIPTION].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_DESCRIPTION].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;
			if (y < file->sorder_count){
				sorder_window_def->icons[SORDER_ICON_DESCRIPTION].data.indirected_text.text = file->sorders[t].description;
			} else {
				sorder_window_def->icons[SORDER_ICON_DESCRIPTION].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}
			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_DESCRIPTION]));

			/* Next date field */

			sorder_window_def->icons[SORDER_ICON_NEXTDATE].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_NEXTDATE].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;
			if (y < file->sorder_count) {
				if (file->sorders[t].adjusted_next_date != NULL_DATE)
					convert_date_to_string(file->sorders[t].adjusted_next_date, icon_buffer);
				else
					msgs_lookup("SOrderStopped", icon_buffer, sizeof (icon_buffer));
			} else {
				*icon_buffer = '\0';
			}
			wimp_plot_icon(&(sorder_window_def->icons[SORDER_ICON_NEXTDATE]));

			/* Left field */

			sorder_window_def->icons[SORDER_ICON_LEFT].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
			sorder_window_def->icons[SORDER_ICON_LEFT].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					SORDER_TOOLBAR_HEIGHT;
			if (y < file->sorder_count)
				sprintf (icon_buffer, "%d", file->sorders[t].left);
			else
				*icon_buffer = '\0';
			wimp_plot_icon (&(sorder_window_def->icons[SORDER_ICON_LEFT]));
			}

		more = wimp_get_rectangle (redraw);
	}
}


/**
 * Callback handler for completing the drag of a column heading.
 *
 * \param *data			The window block for the origin of the drag.
 * \param group			The column group which has been dragged.
 * \param width			The new width for the group.
 */

static void sorder_adjust_window_columns(void *data, wimp_i group, int width)
{
	struct sorder_window	*windat = (struct sorder_window *) data;
	file_data		*file;
	int			i, j, new_extent;
	wimp_icon_state		icon;
	wimp_window_info	window;

	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	update_dragged_columns(SORDER_PANE_COL_MAP, config_str_read("LimSOrderCols"), group, width,
			file->sorder_window.column_width,
			file->sorder_window.column_position, SORDER_COLUMNS);

	/* Re-adjust the icons in the pane. */

	for (i=0, j=0; j < SORDER_COLUMNS; i++, j++) {
		icon.w = file->sorder_window.sorder_pane;
		icon.i = i;
		wimp_get_icon_state(&icon);

		icon.icon.extent.x0 = file->sorder_window.column_position[j];

		j = column_get_rightmost_in_group(SORDER_PANE_COL_MAP, i);

		icon.icon.extent.x1 = file->sorder_window.column_position[j] +
				file->sorder_window.column_width[j] + COLUMN_HEADING_MARGIN;

		wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
				icon.icon.extent.x1, icon.icon.extent.y1);

		new_extent = file->sorder_window.column_position[SORDER_COLUMNS-1] +
				file->sorder_window.column_width[SORDER_COLUMNS-1];
	}

	sorder_adjust_sort_icon(file);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(file->sorder_window.sorder_window);
	windows_redraw(file->sorder_window.sorder_pane);

	/* Set the horizontal extent of the window and pane. */

	window.w = file->sorder_window.sorder_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = file->sorder_window.sorder_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(file, TRUE);
}


/**
 * Adjust the sort icon in a standing order window, to reflect the current column
 * heading positions.
 *
 * \param *file			The file to update the window for.
 */

static void sorder_adjust_sort_icon(file_data *file)
{
	wimp_icon_state		icon;

	icon.w = file->sorder_window.sorder_pane;
	icon.i = SORDER_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	sorder_adjust_sort_icon_data(file, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current standing order sort settings.
 *
 * \param *file			The file to be updated.
 * \param *icon			The icon to be updated.
 */

static void sorder_adjust_sort_icon_data(file_data *file, wimp_icon *icon)
{
	int	i = 0, width, anchor;

	if (file->sorder_window.sort_order & SORT_ASCENDING)
		strcpy(file->sorder_window.sort_sprite, "sortarrd");
	else if (file->sorder_window.sort_order & SORT_DESCENDING)
		strcpy(file->sorder_window.sort_sprite, "sortarru");

	switch (file->sorder_window.sort_order & SORT_MASK) {
	case SORT_FROM:
		i = SORDER_ICON_FROM_NAME;
		sorder_substitute_sort_icon = SORDER_PANE_FROM;
		break;

	case SORT_TO:
		i = SORDER_ICON_TO_NAME;
		sorder_substitute_sort_icon = SORDER_PANE_TO;
		break;

	case SORT_AMOUNT:
		i = SORDER_ICON_AMOUNT;
		sorder_substitute_sort_icon = SORDER_PANE_AMOUNT;
		break;

	case SORT_DESCRIPTION:
		i = SORDER_ICON_DESCRIPTION;
		sorder_substitute_sort_icon = SORDER_PANE_DESCRIPTION;
		break;

	case SORT_NEXTDATE:
		i = SORDER_ICON_NEXTDATE;
		sorder_substitute_sort_icon = SORDER_PANE_NEXTDATE;
		break;

	case SORT_LEFT:
		i = SORDER_ICON_LEFT;
		sorder_substitute_sort_icon = SORDER_PANE_LEFT;
		break;
	}

	width = icon->extent.x1 - icon->extent.x0;

	if ((file->sorder_window.sort_order & SORT_MASK) == SORT_AMOUNT ||
			(file->sorder_window.sort_order & SORT_MASK) == SORT_LEFT) {
		anchor = file->sorder_window.column_position[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
		icon->extent.x1 = icon->extent.x0 + width;
	} else {
		anchor = file->sorder_window.column_position[i] +
				file->sorder_window.column_width[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
		icon->extent.x0 = icon->extent.x1 - width;
	}
}


/**
 * Set the extent of the standing order window for the specified file.
 *
 * \param *file			The file to update.
 */

static void sorder_set_window_extent(file_data *file)
{
	wimp_window_state	state;
	os_box			extent;
	int			new_lines, visible_extent, new_extent, new_scroll;

	/* Set the extent. */

	if (file == NULL || file->sorder_window.sorder_window == NULL)
		return;

	/* Get the number of rows to show in the window, and work out the window extent from this. */

	new_lines = (file->sorder_count > MIN_SORDER_ENTRIES) ? file->sorder_count : MIN_SORDER_ENTRIES;

	new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_lines) - SORDER_TOOLBAR_HEIGHT;

	/* Get the current window details, and find the extent of the bottom of the visible area. */

	state.w = file->sorder_window.sorder_window;
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
	extent.x1 = file->sorder_window.column_position[SORDER_COLUMNS-1] +
			file->sorder_window.column_width[SORDER_COLUMNS-1] + COLUMN_GUTTER;
	extent.y0 = new_extent;

	wimp_set_extent(file->sorder_window.sorder_window, &extent);
}


/**
 * Recreate the title of the Standing Order List window connected to the given
 * file.
 *
 * \param *file			The file to rebuild the title for.
 */

void sorder_build_window_title(file_data *file)
{
	char	name[256];

	if (file == NULL || file->sorder_window.sorder_window == NULL)
		return;

	file_get_leafname(file, name, sizeof(name));

	msgs_param_lookup("SOrderTitle", file->sorder_window.window_title,
			sizeof(file->sorder_window.window_title),
			name, NULL, NULL, NULL);

	wimp_force_redraw_title(file->sorder_window.sorder_window);
}


/**
 * Force a redraw of the Standing Order list window, for the given range of
 * lines.
 *
 * \param *file			The file owning the window.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

void sorder_force_window_redraw(file_data *file, int from, int to)
{
	int			y0, y1;
	wimp_window_info	window;

	if (file == NULL || file->sorder_window.sorder_window == NULL)
		return;

	window.w = file->sorder_window.sorder_window;
	wimp_get_window_info_header_only(&window);

	y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - SORDER_TOOLBAR_HEIGHT;
	y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - SORDER_TOOLBAR_HEIGHT;

	wimp_force_redraw(file->sorder_window.sorder_window, window.extent.x0, y0, window.extent.x1, y1);
}


/**
 * Turn a mouse position over the Standing Order List window into an interactive
 * help token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void sorder_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int			column, xpos;
	wimp_window_state	window;
	struct sorder_window	*windat;

	*buffer = '\0';

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	for (column = 0;
			column < SORDER_COLUMNS && xpos > (windat->column_position[column] + windat->column_width[column]);
			column++);

	sprintf(buffer, "Col%d", column);
}


/**
 * Open the Standing Order Edit dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void sorder_open_edit_window(file_data *file, int sorder, wimp_pointer *ptr)
{
	osbool		edit_mode;

	/* If the window is already open, another standing is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 */

	if (windows_get_open(sorder_edit_window))
		wimp_close_window(sorder_edit_window);

	/* Determine what can be edited.  If the order exists and there are more entries to be added, some bits can not
	 * be changed.
	 */

	edit_mode = (sorder != NULL_SORDER && file->sorders[sorder].adjusted_next_date != NULL_DATE);

	/* Set the contents of the window up. */

	if (sorder == NULL_SORDER) {
		msgs_lookup("NewSO", windows_get_indirected_title_addr(sorder_edit_window), 50);
		msgs_lookup("NewAcctAct", icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_OK), 12);
	} else {
		msgs_lookup("EditSO", windows_get_indirected_title_addr(sorder_edit_window), 50);
		msgs_lookup("EditAcctAct", icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_OK), 12);
	}

	sorder_fill_edit_window(file, sorder, edit_mode);

	/* Set the pointers up so we can find this lot again and open the window. */

	sorder_edit_file = file;
	sorder_edit_number = sorder;

	windows_open_centred_at_pointer(sorder_edit_window, ptr);
	place_dialogue_caret(sorder_edit_window, edit_mode ? SORDER_EDIT_NUMBER : SORDER_EDIT_START);
}


/**
 * Process mouse clicks in the Standing Order Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sorder_edit_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case SORDER_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(sorder_edit_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			sorder_refresh_edit_window();
		break;

	case SORDER_EDIT_OK:
		if (sorder_process_edit_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(sorder_edit_window);
		break;

	case SORDER_EDIT_STOP:
		if (sorder_stop_from_edit_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(sorder_edit_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			sorder_refresh_edit_window();
		break;

	case SORDER_EDIT_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && sorder_delete_from_edit_window())
			close_dialogue_with_caret(sorder_edit_window);
		break;

	case SORDER_EDIT_AVOID:
		icons_set_group_shaded_when_off(sorder_edit_window, SORDER_EDIT_AVOID, 2,
				SORDER_EDIT_SKIPFWD, SORDER_EDIT_SKIPBACK);
		break;

	case SORDER_EDIT_FIRSTSW:
		icons_set_group_shaded_when_off(sorder_edit_window, SORDER_EDIT_FIRSTSW, 1, SORDER_EDIT_FIRST);
		icons_replace_caret_in_window(sorder_edit_window);
		break;

	case SORDER_EDIT_LASTSW:
		icons_set_group_shaded_when_off(sorder_edit_window, SORDER_EDIT_LASTSW, 1, SORDER_EDIT_LAST);
		icons_replace_caret_in_window(sorder_edit_window);
		break;

	case SORDER_EDIT_FMNAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			open_account_menu(sorder_edit_file, ACCOUNT_MENU_FROM, 0,
					sorder_edit_window, SORDER_EDIT_FMIDENT, SORDER_EDIT_FMNAME, SORDER_EDIT_FMREC, pointer);
		break;

	case SORDER_EDIT_TONAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			open_account_menu(sorder_edit_file, ACCOUNT_MENU_TO, 0,
					sorder_edit_window, SORDER_EDIT_TOIDENT, SORDER_EDIT_TONAME, SORDER_EDIT_TOREC, pointer);
		break;

	case SORDER_EDIT_FMREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			toggle_account_reconcile_icon(sorder_edit_window, SORDER_EDIT_FMREC);
		break;

	case SORDER_EDIT_TOREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			toggle_account_reconcile_icon(sorder_edit_window, SORDER_EDIT_TOREC);
		break;
	}
}


/**
 * Process keypresses in the Standing Order Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool sorder_edit_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (sorder_process_edit_window())
			close_dialogue_with_caret(sorder_edit_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(sorder_edit_window);
		break;

	default:
		if (key->i != SORDER_EDIT_FMIDENT && key->i != SORDER_EDIT_TOIDENT)
			return FALSE;

		if (key->i == SORDER_EDIT_FMIDENT)
			account_lookup_field(sorder_edit_file, key->c, ACCOUNT_IN | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
					sorder_edit_window, SORDER_EDIT_FMIDENT, SORDER_EDIT_FMNAME, SORDER_EDIT_FMREC);

		else if (key->i == SORDER_EDIT_TOIDENT)
			account_lookup_field(sorder_edit_file, key->c, ACCOUNT_OUT | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
					sorder_edit_window, SORDER_EDIT_TOIDENT, SORDER_EDIT_TONAME, SORDER_EDIT_TOREC);
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Standing Order Edit window.
 */

static void sorder_refresh_edit_window(void)
{
	sorder_fill_edit_window(sorder_edit_file, sorder_edit_number,
			sorder_edit_number != NULL_SORDER &&
			sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date != NULL_DATE);
	icons_redraw_group(sorder_edit_window, 14,
			SORDER_EDIT_START, SORDER_EDIT_NUMBER, SORDER_EDIT_PERIOD,
			SORDER_EDIT_FMIDENT, SORDER_EDIT_FMREC, SORDER_EDIT_FMNAME,
			SORDER_EDIT_TOIDENT, SORDER_EDIT_TOREC, SORDER_EDIT_TONAME,
			SORDER_EDIT_REF, SORDER_EDIT_AMOUNT, SORDER_EDIT_FIRST, SORDER_EDIT_LAST,
			SORDER_EDIT_DESC);
	icons_replace_caret_in_window(sorder_edit_window);
}

/**
 * Update the contents of the Standing Order Edit window to reflect the current
 * settings of the given file and standing order.
 *
 * \param *file			The file to use.
 * \param preset		The preset to display, or NULL_PRESET for none.
 */

static void sorder_fill_edit_window(file_data *file, int sorder, osbool edit_mode)
{
	if (sorder == NULL_SORDER) {
		/* Set start date. */

		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_START) = '\0';

		/* Set number. */

		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_NUMBER) = '\0';

		/* Set period details. */

		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_PERIOD) = '\0';

		icons_set_selected(sorder_edit_window, SORDER_EDIT_PERDAYS, TRUE);
		icons_set_selected(sorder_edit_window, SORDER_EDIT_PERMONTHS, FALSE);
		icons_set_selected(sorder_edit_window, SORDER_EDIT_PERYEARS, FALSE);

		/* Set the ignore weekends details. */

		icons_set_selected(sorder_edit_window, SORDER_EDIT_AVOID, FALSE);

		icons_set_selected(sorder_edit_window, SORDER_EDIT_SKIPFWD, TRUE);
		icons_set_selected(sorder_edit_window, SORDER_EDIT_SKIPBACK, FALSE);

		icons_set_shaded(sorder_edit_window, SORDER_EDIT_SKIPFWD, TRUE);
		icons_set_shaded(sorder_edit_window, SORDER_EDIT_SKIPBACK, TRUE);

		/* Fill in the from and to fields. */

		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FMIDENT) = '\0';
		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FMNAME) = '\0';
		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FMREC) = '\0';

		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_TOIDENT) = '\0';
		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_TONAME) = '\0';
		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_TOREC) = '\0';

		/* Fill in the reference field. */

		*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_REF) = '\0';

		/* Fill in the amount fields. */

		convert_money_to_string(0, icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_AMOUNT));

		convert_money_to_string(0, icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FIRST));
		icons_set_shaded(sorder_edit_window, SORDER_EDIT_FIRST, TRUE);
		icons_set_selected(sorder_edit_window, SORDER_EDIT_FIRSTSW, FALSE);

		convert_money_to_string(0, icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_LAST));
		icons_set_shaded(sorder_edit_window, SORDER_EDIT_LAST, TRUE);
		icons_set_selected(sorder_edit_window, SORDER_EDIT_LASTSW, FALSE);

		/* Fill in the description field. */

		*icons_get_indirected_text_addr (sorder_edit_window, SORDER_EDIT_DESC) = '\0';
	} else {
		/* Set start date. */

		convert_date_to_string(file->sorders[sorder].start_date,
				icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_START));

		/* Set number. */

		icons_printf(sorder_edit_window, SORDER_EDIT_NUMBER, "%d", file->sorders[sorder].number);

		/* Set period details. */

		icons_printf(sorder_edit_window, SORDER_EDIT_PERIOD, "%d", file->sorders[sorder].period);

		icons_set_selected(sorder_edit_window, SORDER_EDIT_PERDAYS,
				file->sorders[sorder].period_unit == PERIOD_DAYS);
		icons_set_selected(sorder_edit_window, SORDER_EDIT_PERMONTHS,
				file->sorders[sorder].period_unit == PERIOD_MONTHS);
		icons_set_selected(sorder_edit_window, SORDER_EDIT_PERYEARS,
				file->sorders[sorder].period_unit == PERIOD_YEARS);

		/* Set the ignore weekends details. */

		icons_set_selected(sorder_edit_window, SORDER_EDIT_AVOID,
				(file->sorders[sorder].flags & TRANS_SKIP_FORWARD ||
				file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

		icons_set_selected(sorder_edit_window, SORDER_EDIT_SKIPFWD, !(file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));
		icons_set_selected(sorder_edit_window, SORDER_EDIT_SKIPBACK, (file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

		icons_set_shaded(sorder_edit_window, SORDER_EDIT_SKIPFWD,
				!(file->sorders[sorder].flags & TRANS_SKIP_FORWARD ||
				file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

		icons_set_shaded(sorder_edit_window, SORDER_EDIT_SKIPBACK,
				!(file->sorders[sorder].flags & TRANS_SKIP_FORWARD ||
				file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

		/* Fill in the from and to fields. */

		fill_account_field(file, file->sorders[sorder].from, file->sorders[sorder].flags & TRANS_REC_FROM,
				sorder_edit_window, SORDER_EDIT_FMIDENT, SORDER_EDIT_FMNAME, SORDER_EDIT_FMREC);

		fill_account_field(file, file->sorders[sorder].to, file->sorders[sorder].flags & TRANS_REC_TO,
				sorder_edit_window, SORDER_EDIT_TOIDENT, SORDER_EDIT_TONAME, SORDER_EDIT_TOREC);

		/* Fill in the reference field. */

		icons_strncpy(sorder_edit_window, SORDER_EDIT_REF, file->sorders[sorder].reference);

		/* Fill in the amount fields. */

		convert_money_to_string(file->sorders[sorder].normal_amount,
				icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_AMOUNT));


		convert_money_to_string(file->sorders[sorder].first_amount,
				icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FIRST));

		icons_set_shaded(sorder_edit_window, SORDER_EDIT_FIRST,
				(file->sorders[sorder].first_amount == file->sorders[sorder].normal_amount));

		icons_set_selected(sorder_edit_window, SORDER_EDIT_FIRSTSW,
				(file->sorders[sorder].first_amount != file->sorders[sorder].normal_amount));

		convert_money_to_string(file->sorders[sorder].last_amount,
				icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_LAST));

		icons_set_shaded(sorder_edit_window, SORDER_EDIT_LAST,
				(file->sorders[sorder].last_amount == file->sorders[sorder].normal_amount));

		icons_set_selected(sorder_edit_window, SORDER_EDIT_LASTSW,
				(file->sorders[sorder].last_amount != file->sorders[sorder].normal_amount));

		/* Fill in the description field. */

		icons_strncpy(sorder_edit_window, SORDER_EDIT_DESC, file->sorders[sorder].description);
	}

	/* Shade icons as required for the edit mode.
	 * This assumes that none of the relevant icons get shaded for any other reason...
	 */

	icons_set_shaded(sorder_edit_window, SORDER_EDIT_START, edit_mode);
	icons_set_shaded(sorder_edit_window, SORDER_EDIT_PERIOD, edit_mode);
	icons_set_shaded(sorder_edit_window, SORDER_EDIT_PERDAYS, edit_mode);
	icons_set_shaded(sorder_edit_window, SORDER_EDIT_PERMONTHS, edit_mode);
	icons_set_shaded(sorder_edit_window, SORDER_EDIT_PERYEARS, edit_mode);

	/* Detele the irrelevant action buttins for a new standing order. */

	icons_set_shaded(sorder_edit_window, SORDER_EDIT_STOP, !edit_mode && sorder != NULL_SORDER);

	icons_set_deleted(sorder_edit_window, SORDER_EDIT_STOP, sorder == NULL_SORDER);
	icons_set_deleted(sorder_edit_window, SORDER_EDIT_DELETE, sorder == NULL_SORDER);
}


/**
 * Take the contents of an updated Standing Order Edit window and process the
 * data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool sorder_process_edit_window(void)
{
	int		done, new_period_unit;
	date_t		new_start_date;

	/* Extract the start date from the dialogue.  Do this first so that we can test the value;
	 * the info isn't stored until later.
	 */

	if (icons_get_selected (sorder_edit_window, SORDER_EDIT_PERDAYS))
		new_period_unit = PERIOD_DAYS;
	else if (icons_get_selected (sorder_edit_window, SORDER_EDIT_PERMONTHS))
		new_period_unit = PERIOD_MONTHS;
	else if (icons_get_selected (sorder_edit_window, SORDER_EDIT_PERYEARS))
		new_period_unit = PERIOD_YEARS;
	else
		new_period_unit = PERIOD_NONE;

	/* If the period is months, 31 days are always allowed in the date conversion to allow for the longest months.
	 * IF another period is used, the default of the number of days in the givem month is used.
	 */

	new_start_date = convert_string_to_date(icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_START), NULL_DATE,
			((new_period_unit == PERIOD_MONTHS) ? 31 : 0));

	/* If the standing order doesn't exsit, create it.  If it does exist, validate any data that requires it. */

	if (sorder_edit_number == NULL_SORDER) {
		sorder_edit_number = sorder_add(sorder_edit_file);
		sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date = NULL_DATE; /* Set to allow editing. */

		done = 0;
	} else {
		done = sorder_edit_file->sorders[sorder_edit_number].number - sorder_edit_file->sorders[sorder_edit_number].left;
		if (atoi(icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_NUMBER)) < done &&
				sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date != NULL_DATE) {
			error_msgs_report_error("BadSONumber");
			return FALSE;
		}

		if (sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date == NULL_DATE &&
				sorder_edit_file->sorders[sorder_edit_number].start_date == new_start_date &&
				(error_msgs_report_question("CheckSODate", "CheckSODateB") == 2))
			return FALSE;
	}

	/* If the standing order was created OK, store the rest of the data. */

	if (sorder_edit_number == NULL_SORDER)
		return FALSE;

	/* Zero the flags and reset them as required. */

	sorder_edit_file->sorders[sorder_edit_number].flags = 0;

	/* Get the avoid mode. */

	if (icons_get_selected(sorder_edit_window, SORDER_EDIT_AVOID)) {
		if (icons_get_selected(sorder_edit_window, SORDER_EDIT_SKIPFWD))
			sorder_edit_file->sorders[sorder_edit_number].flags |= TRANS_SKIP_FORWARD;
		else if (icons_get_selected(sorder_edit_window, SORDER_EDIT_SKIPBACK))
			sorder_edit_file->sorders[sorder_edit_number].flags |= TRANS_SKIP_BACKWARD;
	}

	/* If it's a new/finished order, get the start date and period and set up the date fields. */

	if (sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date == NULL_DATE) {
		sorder_edit_file->sorders[sorder_edit_number].period_unit = new_period_unit;

		sorder_edit_file->sorders[sorder_edit_number].start_date = new_start_date;

		sorder_edit_file->sorders[sorder_edit_number].raw_next_date =
				sorder_edit_file->sorders[sorder_edit_number].start_date;

		sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date =
				get_sorder_date(sorder_edit_file->sorders[sorder_edit_number].raw_next_date,
				sorder_edit_file->sorders[sorder_edit_number].flags);

		sorder_edit_file->sorders[sorder_edit_number].period =
				atoi(icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_PERIOD));

		done = 0;
	}

	/* Get the number of transactions. */

	sorder_edit_file->sorders[sorder_edit_number].number =
			atoi(icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_NUMBER));

	sorder_edit_file->sorders[sorder_edit_number].left =
			sorder_edit_file->sorders[sorder_edit_number].number - done;

	if (sorder_edit_file->sorders[sorder_edit_number].left == 0)
		sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date = NULL_DATE;

	/* Get the from and to fields. */

	sorder_edit_file->sorders[sorder_edit_number].from = account_find_by_ident(sorder_edit_file,
			icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FMIDENT), ACCOUNT_FULL | ACCOUNT_IN);

	sorder_edit_file->sorders[sorder_edit_number].to = account_find_by_ident(sorder_edit_file,
			icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_TOIDENT), ACCOUNT_FULL | ACCOUNT_OUT);

	if (*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FMREC) != '\0')
		sorder_edit_file->sorders[sorder_edit_number].flags |= TRANS_REC_FROM;

	if (*icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_TOREC) != '\0')
		sorder_edit_file->sorders[sorder_edit_number].flags |= TRANS_REC_TO;

	/* Get the amounts. */

	sorder_edit_file->sorders[sorder_edit_number].normal_amount =
		convert_string_to_money(icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_AMOUNT));

	if (icons_get_selected(sorder_edit_window, SORDER_EDIT_FIRSTSW))
		sorder_edit_file->sorders[sorder_edit_number].first_amount =
				convert_string_to_money(icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_FIRST));
	else
		sorder_edit_file->sorders[sorder_edit_number].first_amount = sorder_edit_file->sorders[sorder_edit_number].normal_amount;

	if (icons_get_selected(sorder_edit_window, SORDER_EDIT_LASTSW))
		sorder_edit_file->sorders[sorder_edit_number].last_amount =
				convert_string_to_money(icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_LAST));
	else
		sorder_edit_file->sorders[sorder_edit_number].last_amount = sorder_edit_file->sorders[sorder_edit_number].normal_amount;

	/* Store the reference. */

	strcpy(sorder_edit_file->sorders[sorder_edit_number].reference, icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_REF));

	/* Store the description. */

	strcpy(sorder_edit_file->sorders[sorder_edit_number].description, icons_get_indirected_text_addr(sorder_edit_window, SORDER_EDIT_DESC));

	if (config_opt_read("AutoSortSOrders"))
		sorder_sort(sorder_edit_file);
	else
		sorder_force_window_redraw(sorder_edit_file, sorder_edit_number, sorder_edit_number);

	file_set_data_integrity(sorder_edit_file, TRUE);
	sorder_process(sorder_edit_file);
	account_recalculate_all(sorder_edit_file);
	transact_set_window_extent(sorder_edit_file);

	return TRUE;
}


/**
 * Delete the standing order associated with the currently open Standing
 * Order Edit window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool sorder_delete_from_edit_window(void)
{
	if (error_msgs_report_question("DeleteSOrder", "DeleteSOrderB") == 2)
		return FALSE;

	return sorder_delete(sorder_edit_file, sorder_edit_number);
}



/**
 * Stop the standing order associated with the currently open Standing
 * Order Edit window.  Set the next dates to NULL and zero the left count.
 *
 * \return			TRUE if stopped; else FALSE.
 */

static osbool sorder_stop_from_edit_window(void)
{
	if (error_msgs_report_question("StopSOrder", "StopSOrderB") == 2)
		return FALSE;

	/* Stop the order */

	sorder_edit_file->sorders[sorder_edit_number].raw_next_date = NULL_DATE;
	sorder_edit_file->sorders[sorder_edit_number].adjusted_next_date = NULL_DATE;
	sorder_edit_file->sorders[sorder_edit_number].left = 0;

	/* Redraw the standing order edit window's contents. */

	sorder_refresh_edit_window();

	/* Update the main standing order display window. */

	if (config_opt_read("AutoSortSOrders"))
		sorder_sort(sorder_edit_file);
	else
		sorder_force_window_redraw(sorder_edit_file, sorder_edit_number, sorder_edit_number);
	file_set_data_integrity(sorder_edit_file, TRUE);

	return TRUE;
}


/**
 * Open the Standing Order Sort dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void sorder_open_sort_window(file_data *file, wimp_pointer *ptr)
{
	if (windows_get_open(sorder_sort_window))
		wimp_close_window(sorder_sort_window);

	sorder_fill_sort_window(file->sorder_window.sort_order);

	sorder_sort_file = file;

	windows_open_centred_at_pointer(sorder_sort_window, ptr);
	place_dialogue_caret(sorder_sort_window, wimp_ICON_WINDOW);
}


/**
 * Process mouse clicks in the Standing Order Sort dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sorder_sort_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case SORDER_SORT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(sorder_sort_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			sorder_refresh_sort_window();
		break;

	case SORDER_SORT_OK:
		if (sorder_process_sort_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(sorder_sort_window);
		break;
	}
}


/**
 * Process keypresses in the Standing Order Sort window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool sorder_sort_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (sorder_process_sort_window())
			close_dialogue_with_caret(sorder_sort_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(sorder_sort_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Standing Order Sort window.
 */

static void sorder_refresh_sort_window(void)
{
	sorder_fill_sort_window(sorder_sort_file->sorder_window.sort_order);
}


/**
 * Update the contents of the Standing Order Sort window to reflect the current
 * settings of the given file.
 *
 * \param sort_option		The sort option currently in force.
 */

static void sorder_fill_sort_window(int sort_option)
{
	icons_set_selected(sorder_sort_window, SORDER_SORT_FROM, (sort_option & SORT_MASK) == SORT_FROM);
	icons_set_selected(sorder_sort_window, SORDER_SORT_TO, (sort_option & SORT_MASK) == SORT_TO);
	icons_set_selected(sorder_sort_window, SORDER_SORT_AMOUNT, (sort_option & SORT_MASK) == SORT_AMOUNT);
	icons_set_selected(sorder_sort_window, SORDER_SORT_DESCRIPTION, (sort_option & SORT_MASK) == SORT_DESCRIPTION);
	icons_set_selected(sorder_sort_window, SORDER_SORT_NEXTDATE, (sort_option & SORT_MASK) == SORT_NEXTDATE);
	icons_set_selected(sorder_sort_window, SORDER_SORT_LEFT, (sort_option & SORT_MASK) == SORT_LEFT);

	icons_set_selected(sorder_sort_window, SORDER_SORT_ASCENDING, sort_option & SORT_ASCENDING);
	icons_set_selected(sorder_sort_window, SORDER_SORT_DESCENDING, sort_option & SORT_DESCENDING);
}


/**
 * Take the contents of an updated Standing Order Sort window and process
 * the data.
 *
 * \return			TRUE if successful; else FALSE.
 */

static osbool sorder_process_sort_window(void)
{
	sorder_sort_file->sorder_window.sort_order = SORT_NONE;

	if (icons_get_selected(sorder_sort_window, SORDER_SORT_FROM))
		sorder_sort_file->sorder_window.sort_order = SORT_FROM;
	else if (icons_get_selected(sorder_sort_window, SORDER_SORT_TO))
		sorder_sort_file->sorder_window.sort_order = SORT_TO;
	else if (icons_get_selected(sorder_sort_window, SORDER_SORT_AMOUNT))
		sorder_sort_file->sorder_window.sort_order = SORT_AMOUNT;
	else if (icons_get_selected(sorder_sort_window, SORDER_SORT_DESCRIPTION))
		sorder_sort_file->sorder_window.sort_order = SORT_DESCRIPTION;
	else if (icons_get_selected(sorder_sort_window, SORDER_SORT_NEXTDATE))
		sorder_sort_file->sorder_window.sort_order = SORT_NEXTDATE;
	else if (icons_get_selected(sorder_sort_window, SORDER_SORT_LEFT))
		sorder_sort_file->sorder_window.sort_order = SORT_LEFT;

	if (sorder_sort_file->sorder_window.sort_order != SORT_NONE) {
		if (icons_get_selected(sorder_sort_window, SORDER_SORT_ASCENDING))
			sorder_sort_file->sorder_window.sort_order |= SORT_ASCENDING;
		else if (icons_get_selected(sorder_sort_window, SORDER_SORT_DESCENDING))
			sorder_sort_file->sorder_window.sort_order |= SORT_DESCENDING;
	}

	sorder_adjust_sort_icon(sorder_sort_file);
	windows_redraw(sorder_sort_file->sorder_window.sorder_pane);
	sorder_sort(sorder_sort_file);

	return TRUE;
}


/**
 * Force the closure of the Standing Order sort and edit windows if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void sorder_force_windows_closed(file_data *file)
{
	if (sorder_sort_file == file && windows_get_open(sorder_sort_window))
		close_dialogue_with_caret(sorder_sort_window);

	if (sorder_edit_file == file && windows_get_open(sorder_edit_window))
		close_dialogue_with_caret(sorder_edit_window);
}


/**
 * Open the Standing Order Print dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void sorder_open_print_window(file_data *file, wimp_pointer *ptr, osbool restore)
{
	sorder_print_file = file;
	printing_open_simple_window(file, ptr, restore, "PrintSOrder", sorder_print);
}


/**
 * Send the contents of the Standing Order Window to the printer, via the reporting
 * system.
 *
 * \param text			TRUE to print in text format; FALSE for graphics.
 * \param format		TRUE to apply text formatting in text mode.
 * \param scale			TRUE to scale width in graphics mode.
 * \param rotate		TRUE to print landscape in grapics mode.
 * \param pagenum		TRUE to include page numbers in graphics mode.
 */

static void sorder_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum)
{
	struct report		*report;
	int			i, t;
	char			line[1024], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];
	struct sorder_window	*window;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);
	msgs_lookup("PrintTitleSOrder", buffer, sizeof (buffer));
	report = report_open(sorder_print_file, buffer, NULL);

	if (report == NULL) {
		error_msgs_report_error("PrintMemFail");
		return;
	}

	hourglass_on();

	window = &(sorder_print_file->sorder_window);

	/* Output the page title. */

	file_get_leafname(sorder_print_file, numbuf1, sizeof (numbuf1));
	msgs_param_lookup("SOrderTitle", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
	sprintf(line, "\\b\\u%s", buffer);
	report_write_line(report, 0, line);
	report_write_line(report, 0, "");

	/* Output the headings line, taking the text from the window icons. */

	*line = '\0';
	sprintf(buffer, "\\k\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(window->sorder_pane, SORDER_PANE_FROM, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(window->sorder_pane, SORDER_PANE_TO, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u\\r%s\\t", icons_copy_text(window->sorder_pane, SORDER_PANE_AMOUNT, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->sorder_pane, SORDER_PANE_DESCRIPTION, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->sorder_pane, SORDER_PANE_NEXTDATE, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u\\r%s", icons_copy_text(window->sorder_pane, SORDER_PANE_LEFT, numbuf1));
	strcat(line, buffer);

	report_write_line(report, 0, line);

	/* Output the standing order data as a set of delimited lines. */

	for (i=0; i < sorder_print_file->sorder_count; i++) {
		t = sorder_print_file->sorders[i].sort_index;

		*line = '\0';

		sprintf(buffer, "\\k%s\\t", account_get_ident(sorder_print_file, sorder_print_file->sorders[t].from));
		strcat(line, buffer);

		strcpy(numbuf1, (sorder_print_file->sorders[t].flags & TRANS_REC_FROM) ? rec_char : "");
		sprintf(buffer, "%s\\t", numbuf1);
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", account_get_name(sorder_print_file, sorder_print_file->sorders[t].from));
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", account_get_ident(sorder_print_file, sorder_print_file->sorders[t].to));
		strcat(line, buffer);

		strcpy(numbuf1, (sorder_print_file->sorders[t].flags & TRANS_REC_TO) ? rec_char : "");
		sprintf(buffer, "%s\\t", numbuf1);
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", account_get_name(sorder_print_file, sorder_print_file->sorders[t].to));
		strcat(line, buffer);

		convert_money_to_string(sorder_print_file->sorders[t].normal_amount, numbuf1);
		sprintf(buffer, "\\r%s\\t", numbuf1);
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", sorder_print_file->sorders[t].description);
		strcat(line, buffer);

		if (sorder_print_file->sorders[t].adjusted_next_date != NULL_DATE)
			convert_date_to_string(sorder_print_file->sorders[t].adjusted_next_date, numbuf1);
		else
			msgs_lookup("SOrderStopped", numbuf1, sizeof(numbuf1));
		sprintf(buffer, "%s\\t", numbuf1);
		strcat(line, buffer);

		sprintf(buffer, "\\r%d", sorder_print_file->sorders[t].left);
		strcat(line, buffer);

		report_write_line(report, 0, line);
	}

	hourglass_off();

	report_close_and_print(report, text, format, scale, rotate, pagenum);
}


/**
 * Sort the standing orders in a given file based on that file's sort setting.
 *
 * \param *file			The file to sort.
 */

void sorder_sort(file_data *file)
{
	int		gap, comb, temp, order;
	osbool		sorted, reorder;

	#ifdef DEBUG
	debug_printf("Sorting standing order window");
	#endif

	hourglass_on();

	/* Sort the entries using a combsort.  This has the advantage over qsort () that the order of entries is only
	 * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
	 */

	gap = file->sorder_count - 1;

	order = file->sorder_window.sort_order;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((file->sorder_count >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < file->sorder_count; comb++) {
			switch (order) {
			case SORT_FROM | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(file, file->sorders[file->sorders[comb+gap].sort_index].from),
						account_get_name(file, file->sorders[file->sorders[comb].sort_index].from)) < 0);
				break;

			case SORT_FROM | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(file, file->sorders[file->sorders[comb+gap].sort_index].from),
						account_get_name(file, file->sorders[file->sorders[comb].sort_index].from)) > 0);
				break;

			case SORT_TO | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(file, file->sorders[file->sorders[comb+gap].sort_index].to),
						account_get_name(file, file->sorders[file->sorders[comb].sort_index].to)) < 0);
				break;

			case SORT_TO | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(file, file->sorders[file->sorders[comb+gap].sort_index].to),
						account_get_name(file, file->sorders[file->sorders[comb].sort_index].to)) > 0);
				break;

			case SORT_AMOUNT | SORT_ASCENDING:
				reorder = (file->sorders[file->sorders[comb+gap].sort_index].normal_amount <
						file->sorders[file->sorders[comb].sort_index].normal_amount);
				break;

			case SORT_AMOUNT | SORT_DESCENDING:
				reorder = (file->sorders[file->sorders[comb+gap].sort_index].normal_amount >
						file->sorders[file->sorders[comb].sort_index].normal_amount);
				break;

			case SORT_DESCRIPTION | SORT_ASCENDING:
				reorder = (strcmp(file->sorders[file->sorders[comb+gap].sort_index].description,
						file->sorders[file->sorders[comb].sort_index].description) < 0);
				break;

			case SORT_DESCRIPTION | SORT_DESCENDING:
				reorder = (strcmp(file->sorders[file->sorders[comb+gap].sort_index].description,
						file->sorders[file->sorders[comb].sort_index].description) > 0);
				break;

			case SORT_NEXTDATE | SORT_ASCENDING:
				reorder = (file->sorders[file->sorders[comb+gap].sort_index].adjusted_next_date >
						file->sorders[file->sorders[comb].sort_index].adjusted_next_date);
				break;

			case SORT_NEXTDATE | SORT_DESCENDING:
				reorder = (file->sorders[file->sorders[comb+gap].sort_index].adjusted_next_date <
						file->sorders[file->sorders[comb].sort_index].adjusted_next_date);
				break;

			case SORT_LEFT | SORT_ASCENDING:
				reorder = (file->sorders[file->sorders[comb+gap].sort_index].left <
						file->sorders[file->sorders[comb].sort_index].left);
				break;

			case SORT_LEFT | SORT_DESCENDING:
				reorder = (file->sorders[file->sorders[comb+gap].sort_index].left >
						file->sorders[file->sorders[comb].sort_index].left);
				break;

			default:
				reorder = FALSE;
				break;
			}

			if (reorder) {
				temp = file->sorders[comb+gap].sort_index;
				file->sorders[comb+gap].sort_index = file->sorders[comb].sort_index;
				file->sorders[comb].sort_index = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

	sorder_force_window_redraw(file, 0, file->sorder_count - 1);

	hourglass_off();
}


/**
 * Create a new standing order with null details.  Values are left to be set
 * up later.
 *
 * \param *file			The file to add the standing order to.
 * \return			The new standing order index, or NULL_SORDER.
 */

static int sorder_add(file_data *file)
{
	int	new;

	if (flex_extend((flex_ptr) &(file->sorders), sizeof(struct sorder) * (file->sorder_count+1)) != 1) {
		error_msgs_report_error("NoMemNewSO");
		return NULL_SORDER;
	}

	new = file->sorder_count++;

	file->sorders[new].start_date = NULL_DATE;
	file->sorders[new].raw_next_date = NULL_DATE;
	file->sorders[new].adjusted_next_date = NULL_DATE;

	file->sorders[new].number = 0;
	file->sorders[new].left = 0;
	file->sorders[new].period = 0;
	file->sorders[new].period_unit = 0;

	file->sorders[new].flags = 0;

	file->sorders[new].from = NULL_ACCOUNT;
	file->sorders[new].to = NULL_ACCOUNT;
	file->sorders[new].normal_amount = NULL_CURRENCY;
	file->sorders[new].first_amount = NULL_CURRENCY;
	file->sorders[new].last_amount = NULL_CURRENCY;

	*file->sorders[new].reference = '\0';
	*file->sorders[new].description = '\0';

	file->sorders[new].sort_index = new;

	sorder_set_window_extent(file);

	return new;
}


/**
 * Delete a standing order from a file.
 *
 * \param *file			The file to act on.
 * \param sorder		The standing order to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

static osbool sorder_delete(file_data *file, int sorder)
{
	int	i, index;

	/* Find the index entry for the deleted order, and if it doesn't index itself, shuffle all the indexes along
	 * so that they remain in the correct places. */

	for (i = 0; i < file->sorder_count && file->sorders[i].sort_index != sorder; i++);

	if (file->sorders[i].sort_index == sorder && i != sorder) {
		index = i;

		if (index > sorder)
			for (i = index; i > sorder; i--)
				file->sorders[i].sort_index = file->sorders[i-1].sort_index;
		else
			for (i = index; i < sorder; i++)
				file->sorders[i].sort_index = file->sorders[i+1].sort_index;
	}

	/* Delete the order */

	flex_midextend((flex_ptr) &(file->sorders), (sorder + 1) * sizeof(struct sorder), -sizeof(struct sorder));
	file->sorder_count--;

	/* Adjust the sort indexes that point to entries above the deleted one, by reducing any indexes that are
	 * greater than the deleted entry by one.
	 */

	for (i = 0; i < file->sorder_count; i++)
		if (file->sorders[i].sort_index > sorder)
			file->sorders[i].sort_index = file->sorders[i].sort_index - 1;

	/* Update the main standing order display window. */

	sorder_set_window_extent(file);
	if (file->sorder_window.sorder_window != NULL) {
		windows_open(file->sorder_window.sorder_window);
		if (config_opt_read("AutoSortSOrders")) {
			sorder_sort(file);
			sorder_force_window_redraw(file, file->sorder_count, file->sorder_count);
		} else {
			sorder_force_window_redraw(file, 0, file->sorder_count);
		}
	}

	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Purge unused standing orders from a file.
 *
 * \param *file			The file to purge.
 */

void sorder_purge(file_data *file)
{
	int	i;

	for (i = 0; i < file->sorder_count; i++) {
		if (file->sorders[i].adjusted_next_date == NULL_DATE && sorder_delete(file, i))
			i--; /* Account for the record having been deleted. */
	}
}


/**
 * Scan the standing orders in a file, adding transactions for any which have
 * fallen due.
 *
 * \param *file			The file to process.
 */

void sorder_process(file_data *file)
{
	unsigned int	today;
	int		order, amount;
	osbool		changed;
	char		ref[REF_FIELD_LEN], desc[DESCRIPT_FIELD_LEN];

	#ifdef DEBUG
	debug_printf("\\YStanding Order processing");
	#endif

	today = get_current_date();
	changed = FALSE;

	for (order=0; order<file->sorder_count; order++) {
		#ifdef DEBUG
		debug_printf ("Processing order %d...", order);
		#endif

		/* While the next date for the standing order is today or before today, process it. */

		while (file->sorders[order].adjusted_next_date!= NULL_DATE && file->sorders[order].adjusted_next_date <= today) {
			/* Action the standing order. */

			if (file->sorders[order].left == file->sorders[order].number)
				amount = file->sorders[order].first_amount;
			else if (file->sorders[order].left == 1)
				amount = file->sorders[order].last_amount;
			else
				amount = file->sorders[order].normal_amount;

			/* Reference and description are copied out of the block first as pointers are passed in to add_raw_transaction ()
			 * and the act of adding the transaction will move the flex block and invalidate those pointers before they
			 * get used.
			 */

			strcpy(ref, file->sorders[order].reference);
			strcpy(desc, file->sorders[order].description);

			add_raw_transaction(file, file->sorders[order].adjusted_next_date,
					file->sorders[order].from, file->sorders[order].to,
					file->sorders[order].flags & (TRANS_REC_FROM | TRANS_REC_TO), amount, ref, desc);

			#ifdef DEBUG
			debug_printf("Adding SO, ref '%s', desc '%s'...", file->sorders[order].reference, file->sorders[order].description);
			#endif

			changed = TRUE;

			/* Decrement the outstanding orders. */

			file->sorders[order].left--;

			/* If there are outstanding orders to carry out, work out the next date and remember that. */

			if (file->sorders[order].left > 0) {
				file->sorders[order].raw_next_date = add_to_date(file->sorders[order].raw_next_date,
						file->sorders[order].period_unit, file->sorders[order].period);

				file->sorders[order].adjusted_next_date = get_sorder_date(file->sorders[order].raw_next_date,
						file->sorders[order].flags);
			} else {
				file->sorders[order].adjusted_next_date = NULL_DATE;
			}

			sorder_force_window_redraw(file, order, order);
		}
	}

	/* Update the trial values for the file. */

	sorder_trial(file);

	/* Refresh things if they have changed. */

	if (changed) {
		file_set_data_integrity(file, TRUE);
		file->sort_valid = 0;

		if (config_opt_read("SortAfterSOrders")) {
			transact_sort(file);
		} else {
			force_transaction_window_redraw(file, 0, file->trans_count - 1);
			edit_refresh_line_content(file->transaction_window.transaction_window, -1, -1);
		}

		if (config_opt_read("AutoSortSOrders"))
			sorder_sort(file);

		accview_rebuild_all(file);
	}
}


/**
 * Scan the standing orders in a file, and update the traial values to reflect
 * any pending transactions.
 *
 * \param *file			The file to scan.
 */

void sorder_trial(file_data *file)
{
	unsigned int	trial_date, raw_next_date, adjusted_next_date;
	int		order, amount, left;

	#ifdef DEBUG
	debug_printf("\\YStanding Order trialling");
	#endif

	/* Find the cutoff date for the trial. */

	trial_date = add_to_date(get_current_date(), PERIOD_DAYS, file->budget.sorder_trial);

	/* Zero the order trial values. */

	account_zero_sorder_trial(file);

	/* Process the standing orders. */

	for (order=0; order<file->sorder_count; order++) {
		#ifdef DEBUG
		debug_printf("Trialling order %d...", order);
		#endif

		/* While the next date for the standing order is today or before today, process it. */

		raw_next_date = file->sorders[order].raw_next_date;
		adjusted_next_date = file->sorders[order].adjusted_next_date;
		left = file->sorders[order].left;

		while (adjusted_next_date!= NULL_DATE && adjusted_next_date <= trial_date) {
			/* Action the standing order. */

			if (left == file->sorders[order].number)
				amount = file->sorders[order].first_amount;
			else if (file->sorders[order].left == 1)
				amount = file->sorders[order].last_amount;
			else
				amount = file->sorders[order].normal_amount;

			#ifdef DEBUG
			debug_printf("Adding trial SO, ref '%s', desc '%s'...", file->sorders[order].reference, file->sorders[order].description);
			#endif

			if (file->sorders[order].from != NULL_ACCOUNT)
				account_adjust_sorder_trial(file, file->sorders[order].from, -amount);

			if (file->sorders[order].to != NULL_ACCOUNT)
				account_adjust_sorder_trial(file, file->sorders[order].to, +amount);

			/* Decrement the outstanding orders. */

			left--;

			/* If there are outstanding orders to carry out, work out the next date and remember that. */

			if (left > 0) {
				raw_next_date = add_to_date(raw_next_date, file->sorders[order].period_unit, file->sorders[order].period);
				adjusted_next_date = get_sorder_date(raw_next_date, file->sorders[order].flags);
			} else {
				adjusted_next_date = NULL_DATE;
			}
		}
	}
}


/**
 * Generate a report detailing all of the standing orders in a file.
 *
 * \param *file			The file to report on.
 */

void sorder_full_report(file_data *file)
{
	struct report	*report;
	int			i;
	char			line[1024], buffer[256], numbuf1[256], numbuf2[32], numbuf3[32];

	msgs_lookup("SORWinT", buffer, sizeof(buffer));
	report = report_open(file, buffer, NULL);

	if (report == NULL)
		return;

	hourglass_on();

	file_get_leafname(file, numbuf1, sizeof(numbuf1));
	msgs_param_lookup("SORTitle", line, sizeof(line), numbuf1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	convert_date_to_string (get_current_date(), numbuf1);
	msgs_param_lookup("SORHeader", line, sizeof(line), numbuf1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	snprintf(numbuf1, sizeof(numbuf1), "%d", file->sorder_count);
	msgs_param_lookup("SORCount", line, sizeof(line), numbuf1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	/* Output the data for each of the standing orders in turn. */

	for (i=0; i < file->sorder_count; i++) {
		report_write_line(report, 0, ""); /* Separate each entry with a blank line. */

		snprintf(numbuf1, sizeof(numbuf1), "%d", i+1);
		msgs_param_lookup("SORNumber", line, sizeof(line), numbuf1, NULL, NULL, NULL);
		report_write_line(report, 0, line);

		msgs_param_lookup("SORFrom", line, sizeof(line), account_get_name(file, file->sorders[i].from), NULL, NULL, NULL);
		report_write_line(report, 0, line);

		msgs_param_lookup("SORTo", line, sizeof(line), account_get_name(file, file->sorders[i].to), NULL, NULL, NULL);
		report_write_line(report, 0, line);

		msgs_param_lookup("SORRef", line, sizeof(line), file->sorders[i].reference, NULL, NULL, NULL);
		report_write_line(report, 0, line);

		convert_money_to_string(file->sorders[i].normal_amount, numbuf1);
		msgs_param_lookup("SORAmount", line, sizeof(line), numbuf1, NULL, NULL, NULL);
		report_write_line(report, 0, line);

		if (file->sorders[i].normal_amount != file->sorders[i].first_amount) {
			convert_money_to_string(file->sorders[i].first_amount, numbuf1);
			msgs_param_lookup("SORFirst", line, sizeof(line), numbuf1, NULL, NULL, NULL);
			report_write_line(report, 0, line);
		}

		if (file->sorders[i].normal_amount != file->sorders[i].last_amount) {
			convert_money_to_string(file->sorders[i].last_amount, numbuf1);
			msgs_param_lookup("SORFirst", line, sizeof(line), numbuf1, NULL, NULL, NULL);
			report_write_line(report, 0, line);
		}

		msgs_param_lookup("SORDesc", line, sizeof(line), file->sorders[i].description, NULL, NULL, NULL);
		report_write_line(report, 0, line);

		snprintf(numbuf1, sizeof(numbuf1), "%d", file->sorders[i].number);
		snprintf(numbuf2, sizeof(numbuf2), "%d", file->sorders[i].number - file->sorders[i].left);
		snprintf(numbuf3, sizeof(numbuf3), "%d", file->sorders[i].left);
		msgs_param_lookup("SORCounts", line, sizeof(line), numbuf1, numbuf2, numbuf3, NULL);
		report_write_line(report, 0, line);

		convert_date_to_string(file->sorders[i].start_date, numbuf1);
		msgs_param_lookup("SORStart", line, sizeof(line), numbuf1, NULL, NULL, NULL);
		report_write_line(report, 0, line);

		snprintf(numbuf1, sizeof(numbuf1), "%d", file->sorders[i].period);
		*numbuf2 = '\0';
		switch (file->sorders[i].period_unit) {
		case PERIOD_DAYS:
			msgs_lookup("SOrderDays", numbuf2, sizeof(numbuf2));
			break;

		case PERIOD_MONTHS:
			msgs_lookup("SOrderMonths", numbuf2, sizeof(numbuf2));
			break;

		case PERIOD_YEARS:
			msgs_lookup("SOrderYears", numbuf2, sizeof(numbuf2));
			break;
		
		default:
			*numbuf2 = '\0';
			break;
		}
		msgs_param_lookup("SOREvery", line, sizeof(line), numbuf1, numbuf2, NULL, NULL);
		report_write_line(report, 0, line);

		if (file->sorders[i].flags & TRANS_SKIP_FORWARD) {
			msgs_lookup("SORAvoidFwd", line, sizeof(line));
			report_write_line(report, 0, line);
		} else if (file->sorders[i].flags & TRANS_SKIP_BACKWARD) {
			msgs_lookup("SORAvoidBack", line, sizeof(line));
			report_write_line(report, 0, line);
		}

		if (file->sorders[i].adjusted_next_date != NULL_DATE)
			convert_date_to_string(file->sorders[i].adjusted_next_date, numbuf1);
		else
			msgs_lookup("SOrderStopped", numbuf1, sizeof(numbuf1));
		msgs_param_lookup("SORNext", line, sizeof(line), numbuf1, NULL, NULL, NULL);
		report_write_line(report, 0, line);
	}

	/* Close the report. */

	report_close(report);

	hourglass_off();
}


/**
 * Save the standing order details from a file to a CashBook file.
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void sorder_write_file(file_data *file, FILE *out)
{
	int	i;
	char	buffer[MAX_FILE_LINE_LEN];

	fprintf (out, "\n[StandingOrders]\n");

	fprintf (out, "Entries: %x\n", file->sorder_count);

	column_write_as_text (file->sorder_window.column_width, SORDER_COLUMNS, buffer);
	fprintf (out, "WinColumns: %s\n", buffer);

	fprintf (out, "SortOrder: %x\n", file->sorder_window.sort_order);

	for (i = 0; i < file->sorder_count; i++) {
		fprintf (out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
				file->sorders[i].start_date, file->sorders[i].number, file->sorders[i].period,
				file->sorders[i].period_unit, file->sorders[i].raw_next_date, file->sorders[i].adjusted_next_date,
				file->sorders[i].left, file->sorders[i].flags, file->sorders[i].from, file->sorders[i].to,
				file->sorders[i].normal_amount, file->sorders[i].first_amount, file->sorders[i].last_amount);
		if (*(file->sorders[i].reference) != '\0')
			config_write_token_pair (out, "Ref", file->sorders[i].reference);
		if (*(file->sorders[i].description) != '\0')
			config_write_token_pair (out, "Desc", file->sorders[i].description);
	}
}


/**
 * Read standing order details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

int sorder_read_file(file_data *file, FILE *in, char *section, char *token, char *value, osbool *unknown_data)
{
	int	result, block_size, i = -1;

	block_size = flex_size((flex_ptr) &(file->sorders)) / sizeof(struct sorder);

	do {
		if (string_nocase_strcmp(token, "Entries") == 0) {
			block_size = strtoul(value, NULL, 16);
			if (block_size > file->sorder_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->sorders), sizeof(struct sorder) * block_size);
			} else {
				block_size = file->sorder_count;
			}
		} else if (string_nocase_strcmp(token, "WinColumns") == 0) {
			column_init_window(file->sorder_window.column_width,
					file->sorder_window.column_position,
					SORDER_COLUMNS, 0, TRUE, value);
		} else if (string_nocase_strcmp(token, "SortOrder") == 0) {
			file->sorder_window.sort_order = strtoul(value, NULL, 16);
		} else if (string_nocase_strcmp (token, "@") == 0) {
			file->sorder_count++;
			if (file->sorder_count > block_size) {
				block_size = file->sorder_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->sorders), sizeof(struct sorder) * block_size);
			}
			i = file->sorder_count-1;
			file->sorders[i].start_date = strtoul (next_field (value, ','), NULL, 16);
			file->sorders[i].number = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].period = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].period_unit = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].raw_next_date = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].adjusted_next_date = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].left = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].flags = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].from = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].to = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].normal_amount = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].first_amount = strtoul (next_field (NULL, ','), NULL, 16);
			file->sorders[i].last_amount = strtoul (next_field (NULL, ','), NULL, 16);
			*(file->sorders[i].reference) = '\0';
			*(file->sorders[i].description) = '\0';
			file->sorders[i].sort_index = i;
		} else if (i != -1 && string_nocase_strcmp(token, "Ref") == 0) {
			strcpy(file->sorders[i].reference, value);
		} else if (i != -1 && string_nocase_strcmp (token, "Desc") == 0) {
			strcpy(file->sorders[i].description, value);
		} else {
			*unknown_data = TRUE;
		}

		result = config_read_token_pair(in, token, value, section);
	} while (result != sf_READ_CONFIG_EOF && result != sf_READ_CONFIG_NEW_SECTION);

	block_size = flex_size((flex_ptr) &(file->sorders)) / sizeof(struct sorder);

	#ifdef DEBUG
	debug_printf("StandingOrder block size: %d, required: %d", block_size, file->sorder_count);
	#endif

	if (block_size > file->sorder_count) {
		block_size = file->sorder_count;
		flex_extend((flex_ptr) &(file->sorders), sizeof(struct sorder) * block_size);

		#ifdef DEBUG
		debug_printf("Block shrunk to %d", block_size);
		#endif
	}

	return result;
}


/**
 * Callback handler for saving a CSV version of the standing order data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool sorder_save_csv(char *filename, osbool selection, void *data)
{
	struct sorder_window *windat = data;
	
	if (windat == NULL || windat->file == NULL)
		return FALSE;

	sorder_export_delimited(windat->file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
	
	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the standing order data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool sorder_save_tsv(char *filename, osbool selection, void *data)
{
	struct sorder_window *windat = data;
	
	if (windat == NULL || windat->file == NULL)
		return FALSE;
		
	sorder_export_delimited(windat->file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
	
	return TRUE;
}


/**
 * Export the standing order data from a file into CSV or TSV format.
 *
 * \param *file			The file to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void sorder_export_delimited(file_data *file, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE			*out;
	int			i, t;
	char			buffer[256];
	struct sorder_window	*window;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	window = &(file->sorder_window);

	/* Output the headings line, taking the text from the window icons. */

	icons_copy_text(window->sorder_pane, SORDER_PANE_FROM, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(window->sorder_pane, SORDER_PANE_TO, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(window->sorder_pane, SORDER_PANE_AMOUNT, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(window->sorder_pane, SORDER_PANE_DESCRIPTION, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(window->sorder_pane, SORDER_PANE_NEXTDATE, buffer);
	filing_output_delimited_field(out, buffer, format, 0);
	icons_copy_text(window->sorder_pane, SORDER_PANE_LEFT, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_LAST);

	/* Output the standing order data as a set of delimited lines. */

	for (i=0; i < file->sorder_count; i++) {
		t = file->sorders[i].sort_index;

		account_build_name_pair(file, file->sorders[t].from, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, 0);

		account_build_name_pair(file, file->sorders[t].to, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, 0);

		convert_money_to_string(file->sorders[t].normal_amount, buffer);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, file->sorders[t].description, format, 0);

		if (file->sorders[t].adjusted_next_date != NULL_DATE)
			convert_date_to_string(file->sorders[t].adjusted_next_date, buffer);
		else
			msgs_lookup("SOrderStopped", buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, 0);

		sprintf(buffer, "%d", file->sorders[t].left);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}


/**
 * Check the standing orders in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool sorder_check_account(file_data *file, int account)
{
	int		i;
	osbool		found = FALSE;

	for (i = 0; i < file->sorder_count && !found; i++)
		if (file->sorders[i].from == account || file->sorders[i].to == account)
			found = TRUE;

	return found;
}

