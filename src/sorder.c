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

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

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
#include "sorder.h"

#include "account.h"
#include "account_menu.h"
#include "accview.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "print_dialogue.h"
#include "sorder_dialogue.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
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

#define SORDER_PANE_SORT_DIR_ICON 10

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

/* Standing order window details. */

#define SORDER_COLUMNS 10
#define SORDER_TOOLBAR_HEIGHT 132
#define MIN_SORDER_ENTRIES 10

/* Buffers used in the standing order report. */

#define SORDER_REPORT_LINE_LENGTH 1024
#define SORDER_REPORT_BUF1_LENGTH 256
#define SORDER_REPORT_BUF2_LENGTH 32
#define SORDER_REPORT_BUF3_LENGTH 32


/* Standing Order Window column mapping. */

static struct column_map sorder_columns[SORDER_COLUMNS] = {
	{SORDER_ICON_FROM, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_FROM_REC, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_FROM_NAME, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_TO, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_TO_REC, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_TO_NAME, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_AMOUNT, SORDER_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{SORDER_ICON_DESCRIPTION, SORDER_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION},
	{SORDER_ICON_NEXTDATE, SORDER_PANE_NEXTDATE, wimp_ICON_WINDOW, SORT_NEXTDATE},
	{SORDER_ICON_LEFT, SORDER_PANE_LEFT, wimp_ICON_WINDOW, SORT_LEFT}
};

/**
 * Standing Order Data structure -- implementation.
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
	char			reference[TRANSACT_REF_FIELD_LEN];
	char			description[TRANSACT_DESCRIPT_FIELD_LEN];

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the sorder data, and are in effect a separate array that is used
	 * for handling entries in the sorder window.
	 */

	sorder_t		sort_index;					/**< Point to another order, to allow the sorder window to be sorted.	*/
};


/**
 * Standing Order Window data structure
 */

struct sorder_block {
	struct file_block	*file;						/**< The file to which the window belongs.				*/

	/* Transactcion window handle and title details. */

	wimp_w			sorder_window;					/**< Window handle of the standing order window.			*/
	char			window_title[WINDOW_TITLE_LENGTH];
	wimp_w			sorder_pane;					/**< Window handle of the standing order window toolbar pane.		*/

	/* Display column details. */

	struct column_block	*columns;					/**< Instance handle of the column definitions.				*/

	/* Window sorting information. */

	struct sort_block	*sort;						/**< Instance handle for the sort code.					*/

	char			sort_sprite[COLUMN_SORT_SPRITE_LEN];		/**< Space for the sort icon's indirected data.				*/

	/* Standing Order data. */

	struct sorder		*sorders;					/**< The standing order data for the defined standing orders		*/
	sorder_t		sorder_count;					/**< The number of standing orders defined in the file.			*/
};

/* Standing Order Sort Window. */

static struct sort_dialogue_block	*sorder_sort_dialogue = NULL;		/**< The standing order window sort dialogue box.			*/

static struct sort_dialogue_icon sorder_sort_columns[] = {				/**< Details of the sort dialogue column icons.				*/
	{SORDER_SORT_FROM, SORT_FROM},
	{SORDER_SORT_TO, SORT_TO},
	{SORDER_SORT_AMOUNT, SORT_AMOUNT},
	{SORDER_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{SORDER_SORT_NEXTDATE, SORT_NEXTDATE},
	{SORDER_SORT_LEFT, SORT_LEFT},
	{0, SORT_NONE}
};

static struct sort_dialogue_icon sorder_sort_directions[] = {				/**< Details of the sort dialogue direction icons.			*/
	{SORDER_SORT_ASCENDING, SORT_ASCENDING},
	{SORDER_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/* Standing Order sorting. */

static struct sort_callback	sorder_sort_callbacks;

/* Standing Order List Window. */

static wimp_window		*sorder_window_def = NULL;			/**< The definition for the Standing Order Window.			*/
static wimp_window		*sorder_pane_def = NULL;			/**< The definition for the Standing Order Window pane.			*/
static wimp_menu		*sorder_window_menu = NULL;			/**< The Standing Order Window menu handle.				*/
static int			sorder_window_menu_line = -1;			/**< The line over which the Standing Order Window Menu was opened.	*/

/* SaveAs Dialogue Handles. */

static struct saveas_block	*sorder_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*sorder_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/


static void			sorder_delete_window(struct sorder_block *windat);
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
static void			sorder_adjust_sort_icon(struct sorder_block *windat);
static void			sorder_adjust_sort_icon_data(struct sorder_block *windat, wimp_icon *icon);
static void			sorder_set_window_extent(struct sorder_block *windat);
static void			sorder_force_window_redraw(struct sorder_block *windat, int from, int to, wimp_i column);
static void			sorder_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static int			sorder_get_line_from_sorder(struct sorder_block *windat, sorder_t sorder);

static osbool			sorder_process_edit_window(void *parent, struct sorder_dialogue_data *content);
static osbool			sorder_stop_from_edit_window(struct sorder_block *windat, sorder_t sorder);

static void			sorder_open_sort_window(struct sorder_block *windat, wimp_pointer *ptr);
static osbool			sorder_process_sort_window(enum sort_type order, void *data);

static void			sorder_open_print_window(struct sorder_block *windat, wimp_pointer *ptr, osbool restore);
static struct report		*sorder_print(struct report *report, void *data, date_t from, date_t to);

static int			sorder_sort_compare(enum sort_type type, int index1, int index2, void *data);
static void			sorder_sort_swap(int index1, int index2, void *data);

static sorder_t			sorder_add(struct file_block *file);
static osbool			sorder_delete(struct file_block *file, sorder_t sorder);

static osbool			sorder_save_csv(char *filename, osbool selection, void *data);
static osbool			sorder_save_tsv(char *filename, osbool selection, void *data);
static void			sorder_export_delimited(struct sorder_block *windat, char *filename, enum filing_delimit_type format, int filetype);

static enum date_adjust		sorder_get_date_adjustment(enum transact_flags flags);


/**
 * Test whether a standing order number is safe to look up in the standing order data array.
 */

#define sorder_valid(windat, sorder) (((sorder) != NULL_TRANSACTION) && ((sorder) >= 0) && ((sorder) < ((windat)->sorder_count)))

/**
 * Initialise the standing order system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_initialise(osspriteop_area *sprites)
{
	wimp_w	sort_window;

	sort_window = templates_create_window("SortSOrder");
	ihelp_add_window(sort_window, "SortSOrder", NULL);
	sorder_sort_dialogue = sort_dialogue_create(sort_window, sorder_sort_columns, sorder_sort_directions,
			SORDER_SORT_OK, SORDER_SORT_CANCEL, sorder_process_sort_window);

	sorder_window_def = templates_load_window("SOrder");
	sorder_window_def->icon_count = 0;

	sorder_pane_def = templates_load_window("SOrderTB");
	sorder_pane_def->sprite_area = sprites;

	sorder_window_menu = templates_get_menu("SOrderMenu");
	ihelp_add_menu(sorder_window_menu, "SorderMenu");

	sorder_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", sorder_save_csv);
	sorder_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", sorder_save_tsv);

	sorder_sort_callbacks.compare = sorder_sort_compare;
	sorder_sort_callbacks.swap = sorder_sort_swap;

	sorder_dialogue_initialise();
}


/**
 * Create a new Standing Order window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct sorder_block *sorder_create_instance(struct file_block *file)
{
	struct sorder_block	*new;

	new = heap_alloc(sizeof(struct sorder_block));
	if (new == NULL)
		return NULL;

	/* Initialise the standing order window. */

	new->file = file;

	new->sorder_window = NULL;
	new->sorder_pane = NULL;
	new->columns = NULL;
	new->sort = NULL;

	new->sorders = NULL;
	new->sorder_count = 0;

	/* Initialise the window columns. */

	new-> columns = column_create_instance(SORDER_COLUMNS, sorder_columns, NULL, SORDER_PANE_SORT_DIR_ICON);
	if (new->columns == NULL) {
		sorder_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read("LimSOrderCols"));
	column_init_window(new->columns, 0, FALSE, config_str_read("SOrderCols"));

	/* Initialise the window sort. */

	new->sort = sort_create_instance(SORT_NEXTDATE | SORT_DESCENDING, SORT_NONE, &sorder_sort_callbacks, new);
	if (new->sort == NULL) {
		sorder_delete_instance(new);
		return NULL;
	}

	/* Set up the standing order data structures. */

	if (!flexutils_initialise((void **) &(new->sorders))) {
		sorder_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete a Standing Order instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void sorder_delete_instance(struct sorder_block *windat)
{
	if (windat == NULL)
		return;

	sorder_delete_window(windat);

	column_delete_instance(windat->columns);
	sort_delete_instance(windat->sort);

	if (windat->sorders != NULL)
		flexutils_free((void **) &(windat->sorders));

	heap_free(windat);
}


/**
 * Create and open a Standing Order List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void sorder_open_window(struct file_block *file)
{
	int			height;
	wimp_window_state	parent;
	os_error		*error;

	if (file == NULL || file->sorders == NULL)
		return;

	/* Create or re-open the window. */

	if (file->sorders->sorder_window != NULL) {
		windows_open(file->sorders->sorder_window);
		return;
	}

	#ifdef DEBUG
	debug_printf("\\CCreating standing order window");
	#endif

	/* Create the new window data and build the window. */

	*(file->sorders->window_title) = '\0';
	sorder_window_def->title_data.indirected_text.text = file->sorders->window_title;

	height = (file->sorders->sorder_count > MIN_SORDER_ENTRIES) ? file->sorders->sorder_count : MIN_SORDER_ENTRIES;

	transact_get_window_state(file, &parent);

	window_set_initial_area(sorder_window_def, column_get_window_width(file->sorders->columns),
			(height * WINDOW_ROW_HEIGHT) + SORDER_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window(sorder_window_def, &(file->sorders->sorder_window));
	if (error != NULL) {
		sorder_delete_window(file->sorders);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Create the toolbar. */

	windows_place_as_toolbar(sorder_window_def, sorder_pane_def, SORDER_TOOLBAR_HEIGHT-4);

	#ifdef DEBUG
	debug_printf("Window extents set...");
	#endif

	columns_place_heading_icons(file->sorders->columns, sorder_pane_def);

	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) file->sorders->sort_sprite;
	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			sorder_pane_def->sprite_area;
	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

	sorder_adjust_sort_icon_data(file->sorders, &(sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON]));

	#ifdef DEBUG
	debug_printf("Toolbar icons adjusted...");
	#endif

	error = xwimp_create_window(sorder_pane_def, &(file->sorders->sorder_pane));
	if (error != NULL) {
		sorder_delete_window(file->sorders);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	sorder_build_window_title(file);

	/* Open the window. */

	ihelp_add_window(file->sorders->sorder_window , "SOrder", sorder_decode_window_help);
	ihelp_add_window(file->sorders->sorder_pane , "SOrderTB", NULL);

	windows_open(file->sorders->sorder_window);
	windows_open_nested_as_toolbar(file->sorders->sorder_pane,
			file->sorders->sorder_window,
			SORDER_TOOLBAR_HEIGHT-4, FALSE);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(file->sorders->sorder_window, file->sorders);
	event_add_window_menu(file->sorders->sorder_window, sorder_window_menu);
	event_add_window_close_event(file->sorders->sorder_window, sorder_close_window_handler);
	event_add_window_mouse_event(file->sorders->sorder_window, sorder_window_click_handler);
	event_add_window_scroll_event(file->sorders->sorder_window, sorder_window_scroll_handler);
	event_add_window_redraw_event(file->sorders->sorder_window, sorder_window_redraw_handler);
	event_add_window_menu_prepare(file->sorders->sorder_window, sorder_window_menu_prepare_handler);
	event_add_window_menu_selection(file->sorders->sorder_window, sorder_window_menu_selection_handler);
	event_add_window_menu_warning(file->sorders->sorder_window, sorder_window_menu_warning_handler);
	event_add_window_menu_close(file->sorders->sorder_window, sorder_window_menu_close_handler);

	event_add_window_user_data(file->sorders->sorder_pane, file->sorders);
	event_add_window_menu(file->sorders->sorder_pane, sorder_window_menu);
	event_add_window_mouse_event(file->sorders->sorder_pane, sorder_pane_click_handler);
	event_add_window_menu_prepare(file->sorders->sorder_pane, sorder_window_menu_prepare_handler);
	event_add_window_menu_selection(file->sorders->sorder_pane, sorder_window_menu_selection_handler);
	event_add_window_menu_warning(file->sorders->sorder_pane, sorder_window_menu_warning_handler);
	event_add_window_menu_close(file->sorders->sorder_pane, sorder_window_menu_close_handler);
}


/**
 * Close and delete the Standing order List Window associated with the given
 * file block.
 *
 * \param *windat		The window to delete.
 */

static void sorder_delete_window(struct sorder_block *windat)
{
	#ifdef DEBUG
	debug_printf ("\\RDeleting standing order window");
	#endif

	if (windat == NULL)
		return;

	sort_dialogue_close(sorder_sort_dialogue, windat);

	dialogue_force_all_closed(NULL, windat);

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

	/* Close any related dialogues. */

	dialogue_force_all_closed(NULL, windat);
}


/**
 * Handle Close events on Standing Order List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void sorder_close_window_handler(wimp_close *close)
{
	struct sorder_block	*windat;

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
	struct sorder_block	*windat;
	int			line;
	wimp_window_state	window;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	/* Find the window type and get the line clicked on. */

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = window_calculate_click_row(&(pointer->pos), &window, SORDER_TOOLBAR_HEIGHT, windat->sorder_count);

	/* Handle double-clicks, which will open an edit standing order window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1)
		sorder_open_edit_window(windat->file, windat->sorders[line].sort_index, pointer);
}


/**
 * Process mouse clicks in the Standing Order List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sorder_pane_click_handler(wimp_pointer *pointer)
{
	struct sorder_block	*windat;
	struct file_block	*file;
	wimp_window_state	window;
	wimp_icon_state		icon;
	int			ox;
	enum sort_type		sort_order;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	column_update_heading_icon_click(windat->columns, pointer);

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case SORDER_PANE_PARENT:
			transact_bring_window_to_top(windat->file);
			break;

		case SORDER_PANE_PRINT:
			sorder_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case SORDER_PANE_ADDSORDER:
			sorder_open_edit_window(file, NULL_SORDER, pointer);
			break;

		case SORDER_PANE_SORT:
			sorder_open_sort_window(windat, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case SORDER_PANE_PRINT:
			sorder_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;

		case SORDER_PANE_SORT:
			sorder_sort(file->sorders);
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
				sorder_adjust_sort_icon(windat);
				windows_redraw(windat->sorder_pane);
				sorder_sort(windat);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimSOrderCols"));
		column_start_drag(windat->columns, pointer, windat, windat->sorder_window, sorder_adjust_window_columns);
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
	struct sorder_block	*windat;
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

			line = window_calculate_click_row(&(pointer->pos), &window, SORDER_TOOLBAR_HEIGHT, windat->sorder_count);

			if (line != -1)
				sorder_window_menu_line = line;
		}

		saveas_initialise_dialogue(sorder_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(sorder_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_shade_entry(sorder_window_menu, SORDER_MENU_EDIT, sorder_window_menu_line == -1);

	sorder_force_window_redraw(windat, sorder_window_menu_line, sorder_window_menu_line, wimp_ICON_WINDOW);
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
	struct sorder_block	*windat;
	wimp_pointer		pointer;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case SORDER_MENU_SORT:
		sorder_open_sort_window(windat, &pointer);
		break;

	case SORDER_MENU_EDIT:
		if (sorder_window_menu_line != -1)
			sorder_open_edit_window(windat->file, windat->sorders[sorder_window_menu_line].sort_index, &pointer);
		break;

	case SORDER_MENU_NEWSORDER:
		sorder_open_edit_window(windat->file, NULL_SORDER, &pointer);
		break;

	case SORDER_MENU_PRINT:
		sorder_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
		break;

	case SORDER_MENU_FULLREP:
		sorder_full_report(windat->file);
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
	struct sorder_block	*windat;

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
	struct sorder_block	*windat;

	windat = event_get_window_user_data(w);
	if (windat != NULL)
		sorder_force_window_redraw(windat, sorder_window_menu_line, sorder_window_menu_line, wimp_ICON_WINDOW);

	sorder_window_menu_line = -1;
}


/**
 * Process scroll events in the Standing Order List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void sorder_window_scroll_handler(wimp_scroll *scroll)
{
	window_process_scroll_event(scroll, SORDER_TOOLBAR_HEIGHT);

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

	wimp_open_window((wimp_open *) scroll);
}


/**
 * Process redraw events in the Standing Order List window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void sorder_window_redraw_handler(wimp_draw *redraw)
{
	struct sorder_block	*windat;
	int			top, base, y, select, t;
	char			icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL || windat->columns == NULL)
		return;

	/* Identify if there is a selected line to highlight. */

	if (redraw->w == event_get_current_menu_window())
		select = sorder_window_menu_line;
	else
		select = -1;

	/* Set the horizontal positions of the icons. */

	columns_place_table_icons_horizontally(windat->columns, sorder_window_def, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);

	window_set_icon_templates(sorder_window_def);

	/* Perform the redraw. */

	more = wimp_redraw_window(redraw);

	while (more) {
		window_plot_background(redraw, SORDER_TOOLBAR_HEIGHT, wimp_COLOUR_WHITE, select, &top, &base);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			t = (y < windat->sorder_count) ? windat->sorders[y].sort_index : 0;

			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(windat->columns, sorder_window_def,
					WINDOW_ROW_Y0(SORDER_TOOLBAR_HEIGHT, y), WINDOW_ROW_Y1(SORDER_TOOLBAR_HEIGHT, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= windat->sorder_count) {
				columns_plot_empty_table_icons(windat->columns);
				continue;
			}

			/* From field */

			window_plot_text_field(SORDER_ICON_FROM, account_get_ident(windat->file, windat->sorders[t].from), wimp_COLOUR_BLACK);
			window_plot_reconciled_field(SORDER_ICON_FROM_REC, (windat->sorders[t].flags & TRANS_REC_FROM), wimp_COLOUR_BLACK);
			window_plot_text_field(SORDER_ICON_FROM_NAME, account_get_name(windat->file, windat->sorders[t].from), wimp_COLOUR_BLACK);

			/* To field */

			window_plot_text_field(SORDER_ICON_TO, account_get_ident(windat->file, windat->sorders[t].to), wimp_COLOUR_BLACK);
			window_plot_reconciled_field(SORDER_ICON_TO_REC, (windat->sorders[t].flags & TRANS_REC_TO), wimp_COLOUR_BLACK);
			window_plot_text_field(SORDER_ICON_TO_NAME, account_get_name(windat->file, windat->sorders[t].to), wimp_COLOUR_BLACK);

			/* Amount field */

			window_plot_currency_field(SORDER_ICON_AMOUNT, windat->sorders[t].normal_amount, wimp_COLOUR_BLACK);

			/* Description field */

			window_plot_text_field(SORDER_ICON_DESCRIPTION, windat->sorders[t].description, wimp_COLOUR_BLACK);

			/* Next date field */

			if (windat->sorders[t].adjusted_next_date != NULL_DATE)
				window_plot_date_field(SORDER_ICON_NEXTDATE, windat->sorders[t].adjusted_next_date, wimp_COLOUR_BLACK);
			else
				window_plot_message_field(SORDER_ICON_NEXTDATE, "SOrderStopped", wimp_COLOUR_BLACK);

			/* Left field */

			window_plot_int_field(SORDER_ICON_LEFT,windat->sorders[t].left, wimp_COLOUR_BLACK);
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
	struct sorder_block	*windat = (struct sorder_block *) data;
	int			new_extent;
	wimp_window_info	window;

	if (windat == NULL || windat->file == NULL)
		return;

	columns_update_dragged(windat->columns, windat->sorder_pane, NULL, group, width);

	new_extent = column_get_window_width(windat->columns);

	sorder_adjust_sort_icon(windat);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(windat->sorder_window);
	windows_redraw(windat->sorder_pane);

	/* Set the horizontal extent of the window and pane. */

	window.w = windat->sorder_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = windat->sorder_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(windat->file, TRUE);
}


/**
 * Adjust the sort icon in a standing order window, to reflect the current column
 * heading positions.
 *
 * \param *windat		The standing order window to update.
 */

static void sorder_adjust_sort_icon(struct sorder_block *windat)
{
	wimp_icon_state		icon;

	if (windat == NULL)
		return;

	icon.w = windat->sorder_pane;
	icon.i = SORDER_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	sorder_adjust_sort_icon_data(windat, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current standing order sort settings.
 *
 * \param *windat		The standing order window to be updated.
 * \param *icon			The icon to be updated.
 */

static void sorder_adjust_sort_icon_data(struct sorder_block *windat, wimp_icon *icon)
{
	enum sort_type	sort_order;

	if (windat == NULL)
		return;

	sort_order = sort_get_order(windat->sort);

	column_update_sort_indicator(windat->columns, icon, sorder_pane_def, sort_order);
}


/**
 * Set the extent of the standing order window for the specified file.
 *
 * \param *windat		The standing order window to update.
 */

static void sorder_set_window_extent(struct sorder_block *windat)
{
	int	lines;


	if (windat == NULL || windat->sorder_window == NULL)
		return;

	lines = (windat->sorder_count > MIN_SORDER_ENTRIES) ? windat->sorder_count : MIN_SORDER_ENTRIES;

	window_set_extent(windat->sorder_window, lines, SORDER_TOOLBAR_HEIGHT, column_get_window_width(windat->columns));
}


/**
 * Recreate the title of the Standing Order List window connected to the given
 * file.
 *
 * \param *file			The file to rebuild the title for.
 */

void sorder_build_window_title(struct file_block *file)
{
	char	name[WINDOW_TITLE_LENGTH];

	if (file == NULL || file->sorders == NULL || file->sorders->sorder_window == NULL)
		return;

	file_get_leafname(file, name, WINDOW_TITLE_LENGTH);

	msgs_param_lookup("SOrderTitle", file->sorders->window_title, WINDOW_TITLE_LENGTH,
			name, NULL, NULL, NULL);

	wimp_force_redraw_title(file->sorders->sorder_window);
}


/**
 * Force the complete redraw of the Standing Order list window.
 *
 * \param *file			The file owning the window to redraw.
 */

void sorder_redraw_all(struct file_block *file)
{
	if (file == NULL || file->sorders == NULL)
		return;

	sorder_force_window_redraw(file->sorders, 0, file->sorders->sorder_count - 1, wimp_ICON_WINDOW);
}


/**
 * Force a redraw of the Standing Order list window, for the given range of
 * lines.
 *
 * \param *windat		The standing order window to redraw.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void sorder_force_window_redraw(struct sorder_block *windat, int from, int to, wimp_i column)
{
	wimp_window_info	window;

	if (windat == NULL || windat->sorder_window == NULL)
		return;

	window.w = windat->sorder_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(windat->columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(SORDER_TOOLBAR_HEIGHT, from);
	window.extent.y0 = WINDOW_ROW_BASE(SORDER_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(windat->sorder_window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);
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
	int			xpos;
	wimp_i			icon;
	wimp_window_state	window;
	struct sorder_block	*windat;

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

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, sorder_window_def->icons[icon].data.indirected_text.validation, 'N'))
		string_printf(buffer, IHELP_INAME_LEN, "Col%d", icon);
}


/**
 * Find the display line in a standing order window which points to the
 * specified standing order under the applied sort.
 *
 * \param *windat		The standing order window to search.
 * \param sorder		The standing order to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

static int sorder_get_line_from_sorder(struct sorder_block *windat, sorder_t sorder)
{
	int	i;
	int	line = -1;

	if (windat == NULL || !sorder_valid(windat, sorder))
		return line;

	for (i = 0; i < windat->sorder_count; i++) {
		if (windat->sorders[i].sort_index == sorder) {
			line = i;
			break;
		}
	}

	return line;
}


/**
 * Open the Standing Order Edit dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void sorder_open_edit_window(struct file_block *file, sorder_t sorder, wimp_pointer *ptr)
{
	struct sorder_dialogue_data	*content;

	if (file == NULL || file->sorders == NULL)
		return;

	/* Open the dialogue box. */

	content = heap_alloc(sizeof(struct sorder_dialogue_data));
	if (content == NULL)
		return;

	content->action = SORDER_DIALOGUE_ACTION_NONE;
	content->sorder = sorder;

	if (sorder == NULL_SORDER) {
		content->active = FALSE;
		content->start_date = NULL_DATE;
		content->number = 0;
		content->period = 0;
		content->period_unit = DATE_PERIOD_DAYS;
		content->flags = TRANS_FLAGS_NONE;
		content->from = NULL_ACCOUNT;
		content->to = NULL_ACCOUNT;
		content->normal_amount = NULL_CURRENCY;
		content->first_amount = NULL_CURRENCY;
		content->last_amount = NULL_CURRENCY;
		*(content->reference) = '\0';
		*(content->description) = '\0';
	} else {
		content->active = (file->sorders->sorders[sorder].adjusted_next_date != NULL_DATE) ? TRUE : FALSE;
		content->start_date = file->sorders->sorders[sorder].start_date;
		content->number = file->sorders->sorders[sorder].number;
		content->period = file->sorders->sorders[sorder].period;
		content->period_unit = file->sorders->sorders[sorder].period_unit;
		content->flags = file->sorders->sorders[sorder].flags;
		content->from = file->sorders->sorders[sorder].from;
		content->to = file->sorders->sorders[sorder].to;
		content->normal_amount = file->sorders->sorders[sorder].normal_amount;
		content->first_amount = file->sorders->sorders[sorder].first_amount;
		content->last_amount = file->sorders->sorders[sorder].last_amount;
		string_copy(content->reference, file->sorders->sorders[sorder].reference, TRANSACT_REF_FIELD_LEN);
		string_copy(content->description, file->sorders->sorders[sorder].description, TRANSACT_DESCRIPT_FIELD_LEN);
	}

	sorder_dialogue_open(ptr, file->sorders, file, sorder_process_edit_window, content);
}


/**
 * Process data associated with the currently open Standing Order Edit window.
 *
 * \param *parent		The standing order instance holding the section.
 * \param *content		The content of the dialogue box.
 * \return			TRUE if processed; else FALSE.
 */

static osbool sorder_process_edit_window(void *parent, struct sorder_dialogue_data *content)
{
	struct sorder_block	*windat = parent;
	int			done, line;

	if (content == NULL || windat == NULL)
		return FALSE;

	if (content->action == SORDER_DIALOGUE_ACTION_DELETE) {
		if (error_msgs_report_question("DeleteSOrder", "DeleteSOrderB") == 4)
			return FALSE;

		return sorder_delete(windat->file, content->sorder);
	} else if (content->action == SORDER_DIALOGUE_ACTION_STOP) {
		sorder_stop_from_edit_window(windat, content->sorder);
	} else if (content->action != SORDER_DIALOGUE_ACTION_OK) {
		return FALSE;
	}

	/* If the standing order doesn't exsit, create it.  If it does exist, validate any data that requires it. */

	if (content->sorder == NULL_SORDER) {
		content->sorder = sorder_add(windat->file);
		if (content->sorder == NULL_SORDER) {
			// \TODO - Error no mem!
			return FALSE;
		}

		windat->sorders[content->sorder].adjusted_next_date = NULL_DATE; /* Set to allow editing. */

		done = 0;
	} else {
		done = windat->sorders[content->sorder].number - windat->sorders[content->sorder].left;
		if (content->number < done && windat->sorders[content->sorder].adjusted_next_date != NULL_DATE) {
			error_msgs_report_error("BadSONumber");
			return FALSE;
		}

		if (windat->sorders[content->sorder].adjusted_next_date == NULL_DATE &&
				windat->sorders[content->sorder].start_date == content->start_date &&
				(error_msgs_report_question("CheckSODate", "CheckSODateB") == 4))
			return FALSE;
	}

	/* If the standing order was created OK, store the rest of the data. */

	if (content->sorder == NULL_SORDER)
		return FALSE;

	/* If it's a new/finished order, get the start date and period and set up the date fields. */

	if (windat->sorders[content->sorder].adjusted_next_date == NULL_DATE) {
		windat->sorders[content->sorder].period_unit = content->period_unit;
		windat->sorders[content->sorder].start_date = content->start_date;
		windat->sorders[content->sorder].raw_next_date = content->start_date;

		windat->sorders[content->sorder].adjusted_next_date =
				date_find_working_day(windat->sorders[content->sorder].raw_next_date,
				sorder_get_date_adjustment(windat->sorders[content->sorder].flags));

		windat->sorders[content->sorder].period = content->period;

		done = 0;
	}

	/* Get the number of transactions. */

	windat->sorders[content->sorder].number = content->number;

	windat->sorders[content->sorder].left = content->number - done;

	if (windat->sorders[content->sorder].left == 0)
		windat->sorders[content->sorder].adjusted_next_date = NULL_DATE;

	/* Get the flags. */

	windat->sorders[content->sorder].flags = content->flags;

	/* Get the from and to fields. */

	windat->sorders[content->sorder].from = content->from;
	windat->sorders[content->sorder].to = content->to;

	/* Get the amounts. */

	windat->sorders[content->sorder].normal_amount = content->normal_amount;
	windat->sorders[content->sorder].first_amount = content->first_amount;
	windat->sorders[content->sorder].last_amount = content->last_amount;

	/* Store the reference. */

	string_copy(windat->sorders[content->sorder].reference, content->reference, TRANSACT_REF_FIELD_LEN);
	string_copy(windat->sorders[content->sorder].description, content->description, TRANSACT_DESCRIPT_FIELD_LEN);

	/* Update the display. */

	if (config_opt_read("AutoSortSOrders")) {
		sorder_sort(windat);
	} else {
		line = sorder_get_line_from_sorder(windat, content->sorder);
		if (line != -1)
			sorder_force_window_redraw(windat, line, line, wimp_ICON_WINDOW);
	}

	file_set_data_integrity(windat->file, TRUE);
	sorder_process(windat->file);
	account_recalculate_all(windat->file);
	transact_set_window_extent(windat->file);

	return TRUE;
}



/**
 * Stop the standing order associated with the currently open Standing
 * Order Edit window.  Set the next dates to NULL and zero the left count.
 *
 * \param *windat		The standing order instance holding the order.
 * \param sorder		The order to stop.
 * \return			TRUE if stopped; else FALSE.
 */

static osbool sorder_stop_from_edit_window(struct sorder_block *windat, sorder_t sorder)
{
	int line;

	if (windat == NULL)
		return FALSE;

	if (error_msgs_report_question("StopSOrder", "StopSOrderB") == 4)
		return FALSE;

	/* Stop the order */

	windat->sorders[sorder].raw_next_date = NULL_DATE;
	windat->sorders[sorder].adjusted_next_date = NULL_DATE;
	windat->sorders[sorder].left = 0;

	/* Redraw the standing order edit window's contents. */

//	\TODO - This needs to be fixed!
//	sorder_refresh_edit_window();

	/* Update the main standing order display window. */

	if (config_opt_read("AutoSortSOrders")) {
		sorder_sort(windat);
	} else {
		line = sorder_get_line_from_sorder(windat, sorder);

		if (line != -1) {
			sorder_force_window_redraw(windat, line, line, SORDER_PANE_NEXTDATE);
			sorder_force_window_redraw(windat, line, line, SORDER_PANE_LEFT);
		}
	}
	file_set_data_integrity(windat->file, TRUE);

	return TRUE;
}


/**
 * Open the Standing Order Sort dialogue for a given standing order list window.
 *
 * \param *windat		The standing order window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void sorder_open_sort_window(struct sorder_block *windat, wimp_pointer *ptr)
{
	if (windat == NULL || ptr == NULL)
		return;

	sort_dialogue_open(sorder_sort_dialogue, ptr, sort_get_order(windat->sort), windat);
}


/**
 * Take the contents of an updated Standing Order Sort window and process
 * the data.
 *
 * \param order			The new sort order selected by the user.
 * \param *data			The standing order window associated with the Sort dialogue.
 * \return			TRUE if successful; else FALSE.
 */

static osbool sorder_process_sort_window(enum sort_type order, void *data)
{
	struct sorder_block	*windat = (struct sorder_block *) data;

	if (windat == NULL)
		return FALSE;

	sort_set_order(windat->sort, order);

	sorder_adjust_sort_icon(windat);
	windows_redraw(windat->sorder_pane);
	sorder_sort(windat);

	return TRUE;
}


/**
 * Open the Standing Order Print dialogue for a given standing order list window.
 *
 * \param *file			The standing order window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void sorder_open_print_window(struct sorder_block *windat, wimp_pointer *ptr, osbool restore)
{
	if (windat == NULL || windat->file == NULL)
		return;

	print_dialogue_open(windat->file->print, ptr, FALSE, restore, "PrintSOrder", "PrintTitleSOrder", windat, sorder_print, windat);
}


/**
 * Send the contents of the Standing Order Window to the printer, via the reporting
 * system.
 *
 * \param *report		The report handle to use for output.
 * \param *data			The standing order window structure to be printed.
 * \param from			The date to print from.
 * \param to			The date to print to.
 * \return			Pointer to the report, or NULL on failure.
 */

static struct report *sorder_print(struct report *report, void *data, date_t from, date_t to)
{
	struct sorder_block	*windat = data;
	int			line, column;
	sorder_t		sorder;
	char			rec_char[REC_FIELD_LEN];
	wimp_i			columns[SORDER_COLUMNS];

	if (report == NULL || windat == NULL || windat->file == NULL)
		return NULL;

	if (!column_get_icons(windat->columns, columns, SORDER_COLUMNS, FALSE))
		return NULL;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	hourglass_on();

	/* Output the page title. */

	stringbuild_reset();

	stringbuild_add_string("\\b\\u");
	stringbuild_add_message_param("SOrderTitle",
			file_get_leafname(windat->file, NULL, 0),
			NULL, NULL, NULL);

	stringbuild_report_line(report, 1);

	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	stringbuild_reset();
	columns_print_heading_names(windat->columns, windat->sorder_pane/*, report, 0*/);
	stringbuild_report_line(report, 0);

	/* Output the standing order data as a set of delimited lines. */

	for (line = 0; line < windat->sorder_count; line++) {
		sorder = windat->sorders[line].sort_index;

		stringbuild_reset();

		for (column = 0; column < SORDER_COLUMNS; column++) {
			if (column == 0)
				stringbuild_add_string("\\k");
			else
				stringbuild_add_string("\\t");

			switch (columns[column]) {
			case SORDER_ICON_FROM:
				stringbuild_add_string(account_get_ident(windat->file, windat->sorders[sorder].from));
				break;
			case SORDER_ICON_FROM_REC:
				if (windat->sorders[sorder].flags & TRANS_REC_FROM)
					stringbuild_add_string(rec_char);
				break;
			case SORDER_ICON_FROM_NAME:
				stringbuild_add_string("\\v");
				stringbuild_add_string(account_get_name(windat->file, windat->sorders[sorder].from));
				break;
			case SORDER_ICON_TO:
				stringbuild_add_string(account_get_ident(windat->file, windat->sorders[sorder].to));
				break;
			case SORDER_ICON_TO_REC:
				if (windat->sorders[sorder].flags & TRANS_REC_TO)
					stringbuild_add_string(rec_char);
				break;
			case SORDER_ICON_TO_NAME:
				stringbuild_add_string("\\v");
				stringbuild_add_string(account_get_name(windat->file, windat->sorders[sorder].to));
				break;
			case SORDER_ICON_AMOUNT:
				stringbuild_add_string("\\v\\d\\r");
				stringbuild_add_currency(windat->sorders[sorder].normal_amount, FALSE);
				break;
			case SORDER_ICON_DESCRIPTION:
				stringbuild_add_string("\\v");
				stringbuild_add_string(windat->sorders[sorder].description);
				break;
			case SORDER_ICON_NEXTDATE:
				stringbuild_add_string("\\v\\c");
				if (windat->sorders[sorder].adjusted_next_date != NULL_DATE)
					stringbuild_add_date(windat->sorders[sorder].adjusted_next_date);
				else
					stringbuild_add_message("SOrderStopped");
				break;
			case SORDER_ICON_LEFT:
				stringbuild_add_printf("\\v\\d\\r%d", windat->sorders[sorder].left);
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
 * Sort the standing orders in a given file based on that file's sort setting.
 *
 * \param *windat		The standing order instance to sort.
 */

void sorder_sort(struct sorder_block *windat)
{
	if (windat == NULL)
		return;

	#ifdef DEBUG
	debug_printf("Sorting standing order window");
	#endif

	hourglass_on();

	/* Run the sort. */

	sort_process(windat->sort, windat->sorder_count);

	sorder_force_window_redraw(windat, 0, windat->sorder_count - 1, wimp_ICON_WINDOW);

	hourglass_off();
}


/**
 * Compare two lines of a standing order list, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The index of the first line to be compared.
 * \param index2		The index of the second line to be compared.
 * \param *data			Client specific data, which is our window block.
 * \return			The comparison result.
 */

static int sorder_sort_compare(enum sort_type type, int index1, int index2, void *data)
{
	struct sorder_block *windat = data;

	if (windat == NULL)
		return 0;

	switch (type) {
	case SORT_FROM:
		return strcmp(account_get_name(windat->file, windat->sorders[windat->sorders[index1].sort_index].from),
				account_get_name(windat->file, windat->sorders[windat->sorders[index2].sort_index].from));

	case SORT_TO:
		return strcmp(account_get_name(windat->file, windat->sorders[windat->sorders[index1].sort_index].to),
				account_get_name(windat->file, windat->sorders[windat->sorders[index2].sort_index].to));

	case SORT_AMOUNT:
		return (windat->sorders[windat->sorders[index1].sort_index].normal_amount -
				windat->sorders[windat->sorders[index2].sort_index].normal_amount);

	case SORT_DESCRIPTION:
		return strcmp(windat->sorders[windat->sorders[index1].sort_index].description,
				windat->sorders[windat->sorders[index2].sort_index].description);

	case SORT_NEXTDATE:
		return ((windat->sorders[windat->sorders[index2].sort_index].adjusted_next_date & DATE_SORT_MASK) -
				(windat->sorders[windat->sorders[index1].sort_index].adjusted_next_date & DATE_SORT_MASK));

	case SORT_LEFT:
		return  (windat->sorders[windat->sorders[index1].sort_index].left -
				windat->sorders[windat->sorders[index2].sort_index].left);

	default:
		return 0;
	}
}


/**
 * Swap the sort index of two lines of a standing order list.
 *
 * \param index1		The index of the first line to be swapped.
 * \param index2		The index of the second line to be swapped.
 * \param *data			Client specific data, which is our window block.
 */

static void sorder_sort_swap(int index1, int index2, void *data)
{
	struct sorder_block	*windat = data;
	int			temp;

	if (windat == NULL)
		return;

	temp = windat->sorders[index1].sort_index;
	windat->sorders[index1].sort_index = windat->sorders[index2].sort_index;
	windat->sorders[index2].sort_index = temp;
}

/**
 * Create a new standing order with null details.  Values are left to be set
 * up later.
 *
 * \param *file			The file to add the standing order to.
 * \return			The new standing order index, or NULL_SORDER.
 */

static sorder_t sorder_add(struct file_block *file)
{
	int	new;

	if (file == NULL || file->sorders == NULL)
		return NULL_SORDER;

	if (!flexutils_resize((void **) &(file->sorders->sorders), sizeof(struct sorder), file->sorders->sorder_count + 1)) {
		error_msgs_report_error("NoMemNewSO");
		return NULL_SORDER;
	}

	new = file->sorders->sorder_count++;

	file->sorders->sorders[new].start_date = NULL_DATE;
	file->sorders->sorders[new].raw_next_date = NULL_DATE;
	file->sorders->sorders[new].adjusted_next_date = NULL_DATE;

	file->sorders->sorders[new].number = 0;
	file->sorders->sorders[new].left = 0;
	file->sorders->sorders[new].period = 0;
	file->sorders->sorders[new].period_unit = 0;

	file->sorders->sorders[new].flags = 0;

	file->sorders->sorders[new].from = NULL_ACCOUNT;
	file->sorders->sorders[new].to = NULL_ACCOUNT;
	file->sorders->sorders[new].normal_amount = NULL_CURRENCY;
	file->sorders->sorders[new].first_amount = NULL_CURRENCY;
	file->sorders->sorders[new].last_amount = NULL_CURRENCY;

	*file->sorders->sorders[new].reference = '\0';
	*file->sorders->sorders[new].description = '\0';

	file->sorders->sorders[new].sort_index = new;

	sorder_set_window_extent(file->sorders);

	return new;
}


/**
 * Delete a standing order from a file.
 *
 * \param *file			The file to act on.
 * \param sorder		The standing order to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

static osbool sorder_delete(struct file_block *file, sorder_t sorder)
{
	int	i, index;

	if (file == NULL || file->sorders == NULL || sorder == NULL_SORDER || sorder >= file->sorders->sorder_count)
		return FALSE;

	/* Find the index entry for the deleted order, and if it doesn't index itself, shuffle all the indexes along
	 * so that they remain in the correct places. */

	for (i = 0; i < file->sorders->sorder_count && file->sorders->sorders[i].sort_index != sorder; i++);

	if (file->sorders->sorders[i].sort_index == sorder && i != sorder) {
		index = i;

		if (index > sorder)
			for (i = index; i > sorder; i--)
				file->sorders->sorders[i].sort_index = file->sorders->sorders[i-1].sort_index;
		else
			for (i = index; i < sorder; i++)
				file->sorders->sorders[i].sort_index = file->sorders->sorders[i+1].sort_index;
	}

	/* Delete the order */

	if (!flexutils_delete_object((void **) &(file->sorders->sorders), sizeof(struct sorder), sorder)) {
		error_msgs_report_error("BadDelete");
		return FALSE;
	}

	file->sorders->sorder_count--;

	/* Adjust the sort indexes that point to entries above the deleted one, by reducing any indexes that are
	 * greater than the deleted entry by one.
	 */

	for (i = 0; i < file->sorders->sorder_count; i++)
		if (file->sorders->sorders[i].sort_index > sorder)
			file->sorders->sorders[i].sort_index = file->sorders->sorders[i].sort_index - 1;

	/* Update the main standing order display window. */

	sorder_set_window_extent(file->sorders);
	if (file->sorders->sorder_window != NULL) {
		windows_open(file->sorders->sorder_window);
		if (config_opt_read("AutoSortSOrders")) {
			sorder_sort(file->sorders);
			sorder_force_window_redraw(file->sorders, file->sorders->sorder_count, file->sorders->sorder_count, wimp_ICON_WINDOW);
		} else {
			sorder_force_window_redraw(file->sorders, 0, file->sorders->sorder_count, wimp_ICON_WINDOW);
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

void sorder_purge(struct file_block *file)
{
	int	i;

	if (file == NULL || file->sorders == NULL)
		return;

	for (i = 0; i < file->sorders->sorder_count; i++) {
		if (file->sorders->sorders[i].adjusted_next_date == NULL_DATE && sorder_delete(file, i))
			i--; /* Account for the record having been deleted. */
	}
}


/**
 * Find the number of standing orders in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of standing orders in the file.
 */

int sorder_get_count(struct file_block *file)
{
	return (file != NULL && file->sorders != NULL) ? file->sorders->sorder_count : 0;
}


/**
 * Scan the standing orders in a file, adding transactions for any which have
 * fallen due.
 *
 * \param *file			The file to process.
 */

void sorder_process(struct file_block *file)
{
	unsigned int	today;
	sorder_t	order;
	amt_t		amount;
	osbool		changed;
	int		line;
	char		ref[TRANSACT_REF_FIELD_LEN], desc[TRANSACT_DESCRIPT_FIELD_LEN];

	if (file == NULL || file->sorders == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\YStanding Order processing");
	#endif

	today = date_today();
	changed = FALSE;

	for (order = 0; order < file->sorders->sorder_count; order++) {
		#ifdef DEBUG
		debug_printf ("Processing order %d...", order);
		#endif

		/* While the next date for the standing order is today or before today, process it. */

		while (file->sorders->sorders[order].adjusted_next_date!= NULL_DATE && file->sorders->sorders[order].adjusted_next_date <= today) {
			/* Action the standing order. */

			if (file->sorders->sorders[order].left == file->sorders->sorders[order].number)
				amount = file->sorders->sorders[order].first_amount;
			else if (file->sorders->sorders[order].left == 1)
				amount = file->sorders->sorders[order].last_amount;
			else
				amount = file->sorders->sorders[order].normal_amount;

			/* Reference and description are copied out of the block first as pointers are passed in to transact_add_raw_entry()
			 * and the act of adding the transaction will move the flex block and invalidate those pointers before they
			 * get used.
			 */

			string_copy(ref, file->sorders->sorders[order].reference, TRANSACT_REF_FIELD_LEN);
			string_copy(desc, file->sorders->sorders[order].description, TRANSACT_DESCRIPT_FIELD_LEN);

			transact_add_raw_entry(file, file->sorders->sorders[order].adjusted_next_date,
					file->sorders->sorders[order].from, file->sorders->sorders[order].to,
					file->sorders->sorders[order].flags & (TRANS_REC_FROM | TRANS_REC_TO), amount, ref, desc);

			#ifdef DEBUG
			debug_printf("Adding SO, ref '%s', desc '%s'...", file->sorders->sorders[order].reference, file->sorders->sorders[order].description);
			#endif

			changed = TRUE;

			/* Decrement the outstanding orders. */

			file->sorders->sorders[order].left--;

			/* If there are outstanding orders to carry out, work out the next date and remember that. */

			if (file->sorders->sorders[order].left > 0) {
				file->sorders->sorders[order].raw_next_date = date_add_period(file->sorders->sorders[order].raw_next_date,
						file->sorders->sorders[order].period_unit, file->sorders->sorders[order].period);

				file->sorders->sorders[order].adjusted_next_date = date_find_working_day(file->sorders->sorders[order].raw_next_date,
						sorder_get_date_adjustment(file->sorders->sorders[order].flags));
			} else {
				file->sorders->sorders[order].adjusted_next_date = NULL_DATE;
			}

			/* Redraw the standing order in the standing order window. */

			line = sorder_get_line_from_sorder(file->sorders, order);

			if (line != -1) {
				sorder_force_window_redraw(file->sorders, order, order, SORDER_PANE_NEXTDATE);
				sorder_force_window_redraw(file->sorders, order, order, SORDER_PANE_LEFT);
			}
		}
	}

	/* Update the trial values for the file. */

	sorder_trial(file);

	/* Refresh things if they have changed. */

	if (changed) {
		file_set_data_integrity(file, TRUE);

		if (config_opt_read("SortAfterSOrders")) {
			transact_sort(file->transacts);
		} else {
			transact_redraw_all(file);
		}

		if (config_opt_read("AutoSortSOrders"))
			sorder_sort(file->sorders);

		accview_rebuild_all(file);
	}
}


/**
 * Scan the standing orders in a file, and update the traial values to reflect
 * any pending transactions.
 *
 * \param *file			The file to scan.
 */

void sorder_trial(struct file_block *file)
{
	unsigned int	trial_date, raw_next_date, adjusted_next_date;
	sorder_t	order;
	int		amount, left;

	#ifdef DEBUG
	debug_printf("\\YStanding Order trialling");
	#endif

	/* Find the cutoff date for the trial. */

	trial_date = date_add_period(date_today(), DATE_PERIOD_DAYS, budget_get_sorder_trial(file));

	/* Zero the order trial values. */

	account_zero_sorder_trial(file);

	/* Process the standing orders. */

	for (order = 0; order < file->sorders->sorder_count; order++) {
		#ifdef DEBUG
		debug_printf("Trialling order %d...", order);
		#endif

		/* While the next date for the standing order is today or before today, process it. */

		raw_next_date = file->sorders->sorders[order].raw_next_date;
		adjusted_next_date = file->sorders->sorders[order].adjusted_next_date;
		left = file->sorders->sorders[order].left;

		while (adjusted_next_date!= NULL_DATE && adjusted_next_date <= trial_date) {
			/* Action the standing order. */

			if (left == file->sorders->sorders[order].number)
				amount = file->sorders->sorders[order].first_amount;
			else if (file->sorders->sorders[order].left == 1)
				amount = file->sorders->sorders[order].last_amount;
			else
				amount = file->sorders->sorders[order].normal_amount;

			#ifdef DEBUG
			debug_printf("Adding trial SO, ref '%s', desc '%s'...", file->sorders->sorders[order].reference, file->sorders->sorders[order].description);
			#endif

			if (file->sorders->sorders[order].from != NULL_ACCOUNT)
				account_adjust_sorder_trial(file, file->sorders->sorders[order].from, -amount);

			if (file->sorders->sorders[order].to != NULL_ACCOUNT)
				account_adjust_sorder_trial(file, file->sorders->sorders[order].to, +amount);

			/* Decrement the outstanding orders. */

			left--;

			/* If there are outstanding orders to carry out, work out the next date and remember that. */

			if (left > 0) {
				raw_next_date = date_add_period(raw_next_date, file->sorders->sorders[order].period_unit, file->sorders->sorders[order].period);
				adjusted_next_date = date_find_working_day(raw_next_date, sorder_get_date_adjustment(file->sorders->sorders[order].flags));
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

void sorder_full_report(struct file_block *file)
{
	struct report	*report;
	sorder_t	sorder;
	char		line[SORDER_REPORT_LINE_LENGTH], numbuf1[SORDER_REPORT_BUF1_LENGTH], numbuf2[SORDER_REPORT_BUF2_LENGTH], numbuf3[SORDER_REPORT_BUF3_LENGTH];

	if (file == NULL || file->sorders == NULL)
		return;

	if (!stringbuild_initialise(line, SORDER_REPORT_LINE_LENGTH))
		return;

	msgs_lookup("SORWinT", line, SORDER_REPORT_LINE_LENGTH);
	report = report_open(file, line, NULL);

	if (report == NULL) {
		stringbuild_cancel();
		return;
	}

	hourglass_on();

	stringbuild_reset();
	stringbuild_add_message_param("SORTitle", file_get_leafname(file, NULL, 0), NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	stringbuild_reset();
	date_convert_to_string(date_today(), numbuf1, SORDER_REPORT_BUF1_LENGTH);
	stringbuild_add_message_param("SORHeader", numbuf1, NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	stringbuild_reset();
	string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", file->sorders->sorder_count);
	stringbuild_add_message_param("SORCount", numbuf1, NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	/* Output the data for each of the standing orders in turn. */

	for (sorder = 0; sorder < file->sorders->sorder_count; sorder++) {
		report_write_line(report, 0, ""); /* Separate each entry with a blank line. */

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", sorder + 1);
		stringbuild_add_message_param("SORNumber", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORFrom", account_get_name(file, file->sorders->sorders[sorder].from), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORTo", account_get_name(file, file->sorders->sorders[sorder].to), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORRef", file->sorders->sorders[sorder].reference, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		currency_convert_to_string(file->sorders->sorders[sorder].normal_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORAmount", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		if (file->sorders->sorders[sorder].normal_amount != file->sorders->sorders[sorder].first_amount) {
			stringbuild_reset();
			currency_convert_to_string(file->sorders->sorders[sorder].first_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
			stringbuild_add_message_param("SORFirst", numbuf1, NULL, NULL, NULL);
			stringbuild_report_line(report, 0);
		}

		if (file->sorders->sorders[sorder].normal_amount != file->sorders->sorders[sorder].last_amount) {
			stringbuild_reset();
			currency_convert_to_string(file->sorders->sorders[sorder].last_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
			stringbuild_add_message_param("SORFirst", numbuf1, NULL, NULL, NULL);
			stringbuild_report_line(report, 0);
		}

		stringbuild_reset();
		stringbuild_add_message_param("SORDesc", file->sorders->sorders[sorder].description, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", file->sorders->sorders[sorder].number);
		string_printf(numbuf2, SORDER_REPORT_BUF2_LENGTH, "%d", file->sorders->sorders[sorder].number - file->sorders->sorders[sorder].left);
		string_printf(numbuf3, SORDER_REPORT_BUF3_LENGTH, "%d", file->sorders->sorders[sorder].left);
		stringbuild_add_message_param("SORCounts", numbuf1, numbuf2, numbuf3, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		date_convert_to_string(file->sorders->sorders[sorder].start_date, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORStart", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", file->sorders->sorders[sorder].period);
		switch (file->sorders->sorders[sorder].period_unit) {
		case DATE_PERIOD_DAYS:
			msgs_lookup("SOrderDays", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		case DATE_PERIOD_MONTHS:
			msgs_lookup("SOrderMonths", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		case DATE_PERIOD_YEARS:
			msgs_lookup("SOrderYears", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		default:
			*numbuf2 = '\0';
			break;
		}
		stringbuild_add_message_param("SOREvery", numbuf1, numbuf2, NULL, NULL);
		stringbuild_report_line(report, 0);

		if (file->sorders->sorders[sorder].flags & TRANS_SKIP_FORWARD) {
			stringbuild_reset();
			stringbuild_add_message("SORAvoidFwd");
			stringbuild_report_line(report, 0);
		} else if (file->sorders->sorders[sorder].flags & TRANS_SKIP_BACKWARD) {
			stringbuild_reset();
			stringbuild_add_message("SORAvoidBack");
			stringbuild_report_line(report, 0);
		}

		stringbuild_reset();
		if (file->sorders->sorders[sorder].adjusted_next_date != NULL_DATE)
			date_convert_to_string(file->sorders->sorders[sorder].adjusted_next_date, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		else
			msgs_lookup("SOrderStopped", numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORNext", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);
	}

	/* Close the report. */

	stringbuild_cancel();

	report_close(report);

	hourglass_off();
}


/**
 * Save the standing order details from a file to a CashBook file.
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void sorder_write_file(struct file_block *file, FILE *out)
{
	int	i;
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (file == NULL || file->sorders == NULL)
		return;

	fprintf(out, "\n[StandingOrders]\n");

	fprintf(out, "Entries: %x\n", file->sorders->sorder_count);

	column_write_as_text(file->sorders->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	sort_write_as_text(file->sorders->sort, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "SortOrder: %s\n", buffer);

	for (i = 0; i < file->sorders->sorder_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
				file->sorders->sorders[i].start_date, file->sorders->sorders[i].number, file->sorders->sorders[i].period,
				file->sorders->sorders[i].period_unit, file->sorders->sorders[i].raw_next_date, file->sorders->sorders[i].adjusted_next_date,
				file->sorders->sorders[i].left, file->sorders->sorders[i].flags, file->sorders->sorders[i].from, file->sorders->sorders[i].to,
				file->sorders->sorders[i].normal_amount, file->sorders->sorders[i].first_amount, file->sorders->sorders[i].last_amount);
		if (*(file->sorders->sorders[i].reference) != '\0')
			config_write_token_pair(out, "Ref", file->sorders->sorders[i].reference);
		if (*(file->sorders->sorders[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->sorders->sorders[i].description);
	}
}


/**
 * Read standing order details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sorder_read_file(struct file_block *file, struct filing_block *in)
{
	sorder_t		sorder = NULL_SORDER;
	size_t			block_size;

	if (file == NULL || file->sorders == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Standing Orders.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->sorders->sorders), sizeof(struct sorder), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->sorders->sorder_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->sorders->sorder_count;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			column_init_window(file->sorders->columns, 0, TRUE, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "SortOrder")) {
			sort_read_from_text(file->sorders->sort, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			file->sorders->sorder_count++;
			if (file->sorders->sorder_count > block_size) {
				block_size = file->sorders->sorder_count;
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
			}
			sorder = file->sorders->sorder_count - 1;
			file->sorders->sorders[sorder].start_date = date_get_date_field(in);
			file->sorders->sorders[sorder].number = filing_get_int_field(in);
			file->sorders->sorders[sorder].period = filing_get_int_field(in);
			file->sorders->sorders[sorder].period_unit = date_get_period_field(in);
			file->sorders->sorders[sorder].raw_next_date = date_get_date_field(in);
			file->sorders->sorders[sorder].adjusted_next_date = date_get_date_field(in);
			file->sorders->sorders[sorder].left = filing_get_int_field(in);
			file->sorders->sorders[sorder].flags = transact_get_flags_field(in);
			file->sorders->sorders[sorder].from = account_get_account_field(in);
			file->sorders->sorders[sorder].to = account_get_account_field(in);
			file->sorders->sorders[sorder].normal_amount = currency_get_currency_field(in);
			file->sorders->sorders[sorder].first_amount = currency_get_currency_field(in);
			file->sorders->sorders[sorder].last_amount = currency_get_currency_field(in);
			*(file->sorders->sorders[sorder].reference) = '\0';
			*(file->sorders->sorders[sorder].description) = '\0';
			file->sorders->sorders[sorder].sort_index = sorder;
		} else if (sorder != NULL_SORDER && filing_test_token(in, "Ref")) {
			filing_get_text_value(in, file->sorders->sorders[sorder].reference, TRANSACT_REF_FIELD_LEN);
		} else if (sorder != NULL_SORDER && filing_test_token(in, "Desc")) {
			filing_get_text_value(in, file->sorders->sorders[sorder].description, TRANSACT_DESCRIPT_FIELD_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(file->sorders->sorder_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
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
	struct sorder_block *windat = data;

	if (windat == NULL)
		return FALSE;

	sorder_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

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
	struct sorder_block *windat = data;

	if (windat == NULL)
		return FALSE;

	sorder_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

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

static void sorder_export_delimited(struct sorder_block *windat, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE			*out;
	int			line;
	sorder_t		sorder;
	char			buffer[FILING_DELIMITED_FIELD_LEN];

	if (windat == NULL || windat->file == NULL)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	columns_export_heading_names(windat->columns, windat->sorder_pane, out, format, buffer, FILING_DELIMITED_FIELD_LEN);

	/* Output the standing order data as a set of delimited lines. */

	for (line = 0; line < windat->sorder_count; line++) {
		sorder = windat->sorders[line].sort_index;

		account_build_name_pair(windat->file, windat->sorders[sorder].from, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		account_build_name_pair(windat->file, windat->sorders[sorder].to, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		currency_convert_to_string(windat->sorders[sorder].normal_amount, buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, windat->sorders[sorder].description, format, DELIMIT_NONE);

		if (windat->sorders[sorder].adjusted_next_date != NULL_DATE)
			date_convert_to_string(windat->sorders[sorder].adjusted_next_date, buffer, FILING_DELIMITED_FIELD_LEN);
		else
			msgs_lookup("SOrderStopped", buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		string_printf(buffer, FILING_DELIMITED_FIELD_LEN, "%d", windat->sorders[sorder].left);
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

osbool sorder_check_account(struct file_block *file, int account)
{
	int		i;

	if (file == NULL || file->sorders == NULL)
		return FALSE;

	for (i = 0; i < file->sorders->sorder_count; i++)
		if (file->sorders->sorders[i].from == account || file->sorders->sorders[i].to == account)
			return TRUE;

	return FALSE;
}


/**
 * Return a standing order date adjustment value based on the standing order
 * flags.
 *
 * \param flags			The flags to be converted.
 * \return			The corresponding date adjustment;
 */

static enum date_adjust sorder_get_date_adjustment(enum transact_flags flags)
{
	if (flags & TRANS_SKIP_FORWARD)
		return DATE_ADJUST_FORWARD;
	else if (flags & TRANS_SKIP_BACKWARD)
		return DATE_ADJUST_BACKWARD;
	else
		return DATE_ADJUST_NONE;
}

