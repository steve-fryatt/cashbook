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
 * \file: sorder_list_window.c
 *
 * Standing Order List Window implementation.
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
#include "sorder_list_window.h"

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
#include "sorder.h"
#include "sorder_dialogue.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
#include "transact.h"
#include "window.h"

/* Standing Order List Window icons. */

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

/* Standing Order List Window Toolbar icons. */

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

/* Standing Order List Window Menu entries. */

#define SORDER_MENU_SORT 0
#define SORDER_MENU_EDIT 1
#define SORDER_MENU_NEWSORDER 2
#define SORDER_MENU_EXPCSV 3
#define SORDER_MENU_EXPTSV 4
#define SORDER_MENU_PRINT 5
#define SORDER_MENU_FULLREP 6

/* Standing Order Sort Window icons. */

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

/**
 * The minimum number of entries in the Standing Order List Window.
 */

#define MIN_SORDER_ENTRIES 10

/**
 * The height of the Standing Order List Window toolbar, in OS Units.
 */

#define SORDER_TOOLBAR_HEIGHT 132

/**
 * The number of draggable columns in the Standing Order List Window.
 */

#define SORDER_COLUMNS 10

/* The Standing Order List Window column map. */

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
 * The Standing Order List Window Sort Dialogue column icons.
 */

static struct sort_dialogue_icon sorder_sort_columns[] = {
	{SORDER_SORT_FROM, SORT_FROM},
	{SORDER_SORT_TO, SORT_TO},
	{SORDER_SORT_AMOUNT, SORT_AMOUNT},
	{SORDER_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{SORDER_SORT_NEXTDATE, SORT_NEXTDATE},
	{SORDER_SORT_LEFT, SORT_LEFT},
	{0, SORT_NONE}
};

/**
 * The Standing Order List Window Sort Dialogue direction icons.
 */

static struct sort_dialogue_icon sorder_sort_directions[] = {
	{SORDER_SORT_ASCENDING, SORT_ASCENDING},
	{SORDER_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/**
 * Standing Order List Window line redraw data.
 */

struct sorder_list_window_redraw {
	/**
	 * The number of the standing order relating to the line.
	 */
	sorder_t				sorder;
};

/**
 * Standing Order List Window instance data structure.
 */

struct sorder_list_window {
	/**
	 * The standing order instance owning the Standing Order List Window.
	 */
	struct sorder_block			*instance;

	/**
	 * Wimp window handle for the main Standing Order List Window.
	 */
	wimp_w					sorder_window;

	/**
	 * Indirected title data for the window.
	 */
	char					window_title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the Standing Order List Window Toolbar pane.
	 */
	wimp_w					sorder_pane;

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
};

/**
 * The definition for the Standing Order List Window.
 */

static wimp_window			*sorder_window_def = NULL;

/**
 * The definition for the Standing Order List Window toolbar pane.
 */

static wimp_window			*sorder_pane_def = NULL;

/**
 * The handle of the Standing Order List Window menu.
 */

static wimp_menu			*sorder_window_menu = NULL;

/**
 * The window line associated with the most recent menu opening.
 */

static int				sorder_window_menu_line = -1;

/**
 * The Standing Order List Window Sort dialogue.
 */

static struct sort_dialogue_block	*sorder_sort_dialogue = NULL;

/**
 * The Stanmding Order List Window Sort callbacks.
 */

static struct sort_callback		sorder_sort_callbacks;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block		*sorder_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block		*sorder_saveas_tsv = NULL;

/* Static Function Prototypes. */

static void sorder_list_window_delete(struct sorder_list_window *windat);
static void sorder_list_window_close_handler(wimp_close *close);
static void sorder_list_window_click_handler(wimp_pointer *pointer);
static void sorder_list_window_pane_click_handler(wimp_pointer *pointer);
static void sorder_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void sorder_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void sorder_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void sorder_list_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void sorder_list_window_scroll_handler(wimp_scroll *scroll);
static void sorder_list_window_window_redraw_handler(wimp_draw *redraw);
static void sorder_list_window_adjust_columns(void *data, wimp_i group, int width);
static void sorder_list_window_adjust_sort_icon(struct sorder_list_window *windat);
static void sorder_list_window_adjust_sort_icon_data(struct sorder_list_window *windat, wimp_icon *icon);
static void sorder_list_window_set_extent(struct sorder_list_window *windat);
static void sorder_list_window_force_redraw(struct sorder_list_window *windat, int from, int to, wimp_i column);
static void sorder_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static int sorder_list_window_get_line_from_sorder(struct sorder_list_window *windat, sorder_t sorder);
static void sorder_list_window_open_sort_window(struct sorder_list_window *windat, wimp_pointer *ptr);
static osbool sorder_list_window_process_sort_window(enum sort_type order, void *data);
static void sorder_list_window_open_print_window(struct sorder_list_window *windat, wimp_pointer *ptr, osbool restore);
static struct report *sorder_list_window_print(struct report *report, void *data, date_t from, date_t to);
static int sorder_list_window_sort_compare(enum sort_type type, int index1, int index2, void *data);
static void sorder_list_window_sort_swap(int index1, int index2, void *data);
static osbool sorder_list_window_save_csv(char *filename, osbool selection, void *data);
static osbool sorder_list_window_save_tsv(char *filename, osbool selection, void *data);
static void sorder_list_window_export_delimited(struct sorder_list_window *windat, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Test whether a line number is safe to look up in the redraw data array.
 */

#define sorder_list_window_line_valid(windat, line) (((line) >= 0) && ((line) < ((windat)->display_lines)))


/**
 * Initialise the Standing Order List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_list_window_initialise(osspriteop_area *sprites)
{
	wimp_w sort_window;

	sort_window = templates_create_window("SortSOrder");
	ihelp_add_window(sort_window, "SortSOrder", NULL);
	sorder_sort_dialogue = sort_dialogue_create(sort_window, sorder_sort_columns, sorder_sort_directions,
			SORDER_SORT_OK, SORDER_SORT_CANCEL, sorder_list_window_process_sort_window);

	sorder_sort_callbacks.compare = sorder_list_window_sort_compare;
	sorder_sort_callbacks.swap = sorder_list_window_sort_swap;

	sorder_window_def = templates_load_window("SOrder");
	sorder_window_def->icon_count = 0;

	sorder_pane_def = templates_load_window("SOrderTB");
	sorder_pane_def->sprite_area = sprites;

	sorder_window_menu = templates_get_menu("SOrderMenu");
	ihelp_add_menu(sorder_window_menu, "SorderMenu");

	sorder_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", sorder_list_window_save_csv);
	sorder_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", sorder_list_window_save_tsv);
}


/**
 * Create a new Standing Order List Window instance.
 *
 * \param *parent		The parent sorder instance.
 * \return			Pointer to the new instance, or NULL.
 */

struct sorder_list_window *sorder_list_window_create_instance(struct sorder_block *parent)
{
	struct sorder_list_window *new;

	new = heap_alloc(sizeof(struct sorder_list_window));
	if (new == NULL)
		return NULL;

	new->instance = parent;

	new->sorder_window = NULL;
	new->sorder_pane = NULL;
	new->columns = NULL;
	new->sort = NULL;

	new->display_lines = 0;
	new->line_data = NULL;

	/* Initialise the window columns. */

	new-> columns = column_create_instance(SORDER_COLUMNS, sorder_columns, NULL, SORDER_PANE_SORT_DIR_ICON);
	if (new->columns == NULL) {
		sorder_list_window_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read("LimSOrderCols"));
	column_init_window(new->columns, 0, FALSE, config_str_read("SOrderCols"));

	/* Initialise the window sort. */

	new->sort = sort_create_instance(SORT_NEXTDATE | SORT_DESCENDING, SORT_NONE, &sorder_sort_callbacks, new);
	if (new->sort == NULL) {
		sorder_list_window_delete_instance(new);
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
 * Destroy a Standing Order List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void sorder_list_window_delete_instance(struct sorder_list_window *windat)
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
 * Create and open a Standing Order List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void sorder_list_window_open(struct sorder_list_window *windat)
{
	int			height;
	os_error		*error;
	wimp_window_state	parent;
	struct file_block	*file;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = sorder_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Create or re-open the window. */

	if (windat->sorder_window != NULL) {
		windows_open(windat->sorder_window);
		return;
	}

	#ifdef DEBUG
	debug_printf("\\CCreating standing order window");
	#endif

	/* Create the new window data and build the window. */

	*(windat->window_title) = '\0';
	sorder_window_def->title_data.indirected_text.text = windat->window_title;

	height = (windat->display_lines > MIN_SORDER_ENTRIES) ? windat->display_lines : MIN_SORDER_ENTRIES;

	transact_get_window_state(file, &parent);

	window_set_initial_area(sorder_window_def, column_get_window_width(windat->columns),
			(height * WINDOW_ROW_HEIGHT) + SORDER_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window(sorder_window_def, &(windat->sorder_window));
	if (error != NULL) {
		sorder_list_window_delete(windat);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Create the toolbar. */

	windows_place_as_toolbar(sorder_window_def, sorder_pane_def, SORDER_TOOLBAR_HEIGHT-4);

	#ifdef DEBUG
	debug_printf("Window extents set...");
	#endif

	columns_place_heading_icons(windat->columns, sorder_pane_def);

	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) windat->sort_sprite;
	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			sorder_pane_def->sprite_area;
	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

	sorder_list_window_adjust_sort_icon_data(windat, &(sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON]));

	#ifdef DEBUG
	debug_printf("Toolbar icons adjusted...");
	#endif

	error = xwimp_create_window(sorder_pane_def, &(windat->sorder_pane));
	if (error != NULL) {
		sorder_list_window_delete(windat);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	sorder_build_window_title(file);

	/* Open the window. */

	ihelp_add_window(windat->sorder_window , "SOrder", sorder_list_window_decode_help);
	ihelp_add_window(windat->sorder_pane , "SOrderTB", NULL);

	windows_open(windat->sorder_window);
	windows_open_nested_as_toolbar(windat->sorder_pane,
			windat->sorder_window,
			SORDER_TOOLBAR_HEIGHT-4, FALSE);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(windat->sorder_window, windat);
	event_add_window_menu(windat->sorder_window, sorder_window_menu);
	event_add_window_close_event(windat->sorder_window, sorder_list_window_close_handler);
	event_add_window_mouse_event(windat->sorder_window, sorder_list_window_click_handler);
	event_add_window_scroll_event(windat->sorder_window, sorder_list_window_scroll_handler);
	event_add_window_redraw_event(windat->sorder_window, sorder_list_window_window_redraw_handler);
	event_add_window_menu_prepare(windat->sorder_window, sorder_list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->sorder_window, sorder_list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->sorder_window, sorder_list_window_menu_warning_handler);
	event_add_window_menu_close(windat->sorder_window, sorder_list_window_menu_close_handler);

	event_add_window_user_data(windat->sorder_pane, windat);
	event_add_window_menu(windat->sorder_pane, sorder_window_menu);
	event_add_window_mouse_event(windat->sorder_pane, sorder_list_window_pane_click_handler);
	event_add_window_menu_prepare(windat->sorder_pane, sorder_list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->sorder_pane, sorder_list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->sorder_pane, sorder_list_window_menu_warning_handler);
	event_add_window_menu_close(windat->sorder_pane, sorder_list_window_menu_close_handler);
}


/**
 * Close and delete the Standing order List Window associated with the
 * given instance.
 *
 * \param *windat		The window to delete.
 */

static void sorder_list_window_delete(struct sorder_list_window *windat)
{
	#ifdef DEBUG
	debug_printf ("\\RDeleting standing order window");
	#endif

	if (windat == NULL)
		return;

	/* Delete the window, if it exists. */

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

	/* Close any dialogues which belong to this window. */

	dialogue_force_all_closed(NULL, windat);
	sort_dialogue_close(sorder_sort_dialogue, windat);
}


/**
 * Handle Close events on Standing Order List windows, deleting the
 * window.
 *
 * \param *close		The Wimp Close data block.
 */

static void sorder_list_window_close_handler(wimp_close *close)
{
	struct sorder_list_window *windat;

	#ifdef DEBUG
	debug_printf("\\RClosing Standing Order window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat != NULL)
		sorder_list_window_delete(windat);
}


/**
 * Process mouse clicks in the Standing Order List window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sorder_list_window_click_handler(wimp_pointer *pointer)
{
	struct sorder_list_window	*windat;
	struct file_block		*file;
	int				line;
	wimp_window_state		window;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = sorder_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Find the window type and get the line clicked on. */

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = window_calculate_click_row(&(pointer->pos), &window, SORDER_TOOLBAR_HEIGHT, windat->display_lines);

	/* Handle double-clicks, which will open an edit standing order window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1)
		sorder_open_edit_window(file, windat->line_data[line].sorder, pointer);
}


/**
 * Process mouse clicks in the Standing Order List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sorder_list_window_pane_click_handler(wimp_pointer *pointer)
{
	struct sorder_list_window	*windat;
	struct file_block		*file;
	wimp_window_state		window;
	wimp_icon_state			icon;
	int				ox;
	enum sort_type			sort_order;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = sorder_get_file(windat->instance);
	if (file == NULL)
		return;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	column_update_heading_icon_click(windat->columns, pointer);

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case SORDER_PANE_PARENT:
			transact_bring_window_to_top(file);
			break;

		case SORDER_PANE_PRINT:
			sorder_list_window_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case SORDER_PANE_ADDSORDER:
			sorder_open_edit_window(file, NULL_SORDER, pointer);
			break;

		case SORDER_PANE_SORT:
			sorder_list_window_open_sort_window(windat, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case SORDER_PANE_PRINT:
			sorder_list_window_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;

		case SORDER_PANE_SORT:
			sorder_list_window_sort(windat);
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
				sorder_list_window_adjust_sort_icon(windat);
				windows_redraw(windat->sorder_pane);
				sorder_list_window_sort(windat);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimSOrderCols"));
		column_start_drag(windat->columns, pointer, windat, windat->sorder_window, sorder_list_window_adjust_columns);
	}
}


/**
 * Process menu prepare events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void sorder_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct sorder_list_window	*windat;
	int				line;
	wimp_window_state		window;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (pointer != NULL) {
		sorder_window_menu_line = -1;

		if (w == windat->sorder_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = window_calculate_click_row(&(pointer->pos), &window, SORDER_TOOLBAR_HEIGHT, windat->display_lines);

			if (line != -1)
				sorder_window_menu_line = line;
		}

		saveas_initialise_dialogue(sorder_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(sorder_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_shade_entry(sorder_window_menu, SORDER_MENU_EDIT, sorder_window_menu_line == -1);

	sorder_list_window_force_redraw(windat, sorder_window_menu_line, sorder_window_menu_line, wimp_ICON_WINDOW);
}


/**
 * Process menu selection events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void sorder_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct sorder_list_window	*windat;
	wimp_pointer			pointer;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case SORDER_MENU_SORT:
		sorder_list_window_open_sort_window(windat, &pointer);
		break;

	case SORDER_MENU_EDIT:
		if (sorder_window_menu_line != -1)
			sorder_open_edit_window(windat->file, windat->sorders[sorder_window_menu_line].sort_index, &pointer);
		break;

	case SORDER_MENU_NEWSORDER:
		sorder_open_edit_window(windat->file, NULL_SORDER, &pointer);
		break;

	case SORDER_MENU_PRINT:
		sorder_list_window_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
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

static void sorder_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct sorder_list_window *windat;

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

static void sorder_list_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	struct sorder_list_window *windat;

	windat = event_get_window_user_data(w);
	if (windat != NULL)
		sorder_list_window_force_redraw(windat, sorder_window_menu_line, sorder_window_menu_line, wimp_ICON_WINDOW);

	sorder_window_menu_line = -1;
}


/**
 * Process scroll events in the Standing Order List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void sorder_list_window_scroll_handler(wimp_scroll *scroll)
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

static void sorder_list_window_window_redraw_handler(wimp_draw *redraw)
{
	struct sorder_list_window	*windat;
	int				top, base, y, select, t;
	char				icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN]; /* Assumes descript is longest. */
	osbool				more;

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

static void sorder_list_window_adjust_columns(void *data, wimp_i group, int width)
{
	struct sorder_list_window	*windat = (struct sorder_list_window *) data;
	int				new_extent;
	wimp_window_info		window;

	if (windat == NULL || windat->file == NULL)
		return;

	columns_update_dragged(windat->columns, windat->sorder_pane, NULL, group, width);

	new_extent = column_get_window_width(windat->columns);

	sorder_list_window_adjust_sort_icon(windat);

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

static void sorder_list_window_adjust_sort_icon(struct sorder_list_window *windat)
{
	wimp_icon_state		icon;

	if (windat == NULL)
		return;

	icon.w = windat->sorder_pane;
	icon.i = SORDER_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	sorder_list_window_adjust_sort_icon_data(windat, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current standing order sort settings.
 *
 * \param *windat		The standing order window to be updated.
 * \param *icon			The icon to be updated.
 */

static void sorder_list_window_adjust_sort_icon_data(struct sorder_list_window *windat, wimp_icon *icon)
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

static void sorder_list_window_set_extent(struct sorder_list_window *windat)
{
	int	lines;


	if (windat == NULL || windat->sorder_window == NULL)
		return;

	lines = (windat->sorder_count > MIN_SORDER_ENTRIES) ? windat->sorder_count : MIN_SORDER_ENTRIES;

	window_set_extent(windat->sorder_window, lines, SORDER_TOOLBAR_HEIGHT, column_get_window_width(windat->columns));
}


/**
 * Recreate the title of the given Standing Order List window.
 *
 * \param *windat		The standing order window to rebuild the
 *				title for.
 */

void sorder_list_window_build_title(struct sorder_list_window *windat)
{
	char	name[WINDOW_TITLE_LENGTH];

	if (windat == NULL)
		return;

	file_get_leafname(file, name, WINDOW_TITLE_LENGTH);

	msgs_param_lookup("SOrderTitle", windat->window_title, WINDOW_TITLE_LENGTH,
			name, NULL, NULL, NULL);

	wimp_force_redraw_title(windat);
}


/**
 * Force the complete redraw of the given Standing Order list window.
 *
 * \param *windat		The standing order window to redraw.
 * \param sorder		The standing order to redraw, or NULL_SORDER for all.
 * \param stopped		TRUE to redraw just the active columns.
 */

void sorder_list_window_redraw(struct sorder_list_window *windat, sorder_t sorder, osbool stopped)
{
	int	from, to;

	if (windat == NULL)
		return;

	if (sorder != NULL_SORDER) {
		from = sorder_list_window_get_line_from_sorder(windat, sorder);
		to = from;
	} else {
		from = 0;
		to = windat->display_lines - 1;
	}

	if (stopped) {
		sorder_list_window_force_redraw(windat, from, to, SORDER_PANE_NEXTDATE);
		sorder_list_window_force_redraw(windat, from, to, SORDER_PANE_LEFT);
	} else {
		sorder_list_window_force_redraw(windat, from, to, wimp_ICON_WINDOW);
	}
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

static void sorder_list_window_force_redraw(struct sorder_list_window *windat, int from, int to, wimp_i column)
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

static void sorder_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int				xpos;
	wimp_i				icon;
	wimp_window_state		window;
	struct sorder_list_window	*windat;

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

static int sorder_list_window_get_line_from_sorder(struct sorder_list_window *windat, sorder_t sorder)
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
 * Open the Standing Order Sort dialogue for a given standing order list window.
 *
 * \param *windat		The standing order window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void sorder_list_window_open_sort_window(struct sorder_list_window *windat, wimp_pointer *ptr)
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

static osbool sorder_list_window_process_sort_window(enum sort_type order, void *data)
{
	struct sorder_list_window *windat = (struct sorder_list_window *) data;

	if (windat == NULL)
		return FALSE;

	sort_set_order(windat->sort, order);

	sorder_list_window_adjust_sort_icon(windat);
	windows_redraw(windat->sorder_pane);
	sorder_list_window_sort(windat);

	return TRUE;
}


/**
 * Open the Standing Order Print dialogue for a given standing order list window.
 *
 * \param *file			The standing order window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void sorder_list_window_open_print_window(struct sorder_list_window *windat, wimp_pointer *ptr, osbool restore)
{
	if (windat == NULL || windat->file == NULL)
		return;

	print_dialogue_open(windat->file->print, ptr, FALSE, restore, "PrintSOrder", "PrintTitleSOrder", windat, sorder_list_window_print, windat);
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

static struct report *sorder_list_window_print(struct report *report, void *data, date_t from, date_t to)
{
	struct sorder_list_window	*windat = data;
	int				line, column;
	sorder_t			sorder;
	char				rec_char[REC_FIELD_LEN];
	wimp_i				columns[SORDER_COLUMNS];

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
 * Sort the standing orders in a given list window based on that instances's
 * sort setting.
 *
 * \param *windat		The standing order window instance to sort.
 */

void sorder_list_window_sort(struct sorder_list_window *windat)
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

static int sorder_list_window_sort_compare(enum sort_type type, int index1, int index2, void *data)
{
	struct sorder_list_window *windat = data;

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

static void sorder_list_window_sort_swap(int index1, int index2, void *data)
{
	struct sorder_list_window	*windat = data;
	int				temp;

	if (windat == NULL)
		return;

	temp = windat->sorders[index1].sort_index;
	windat->sorders[index1].sort_index = windat->sorders[index2].sort_index;
	windat->sorders[index2].sort_index = temp;
}








static void sorder_list_window_initialise_entries(struct sorder_list_window *windat, int sorders)
{


}


osbool sorder_list_window_add_line(struct sorder_list_window *windat, sorder_t sorder)
{


//	sorder_set_window_extent(file->sorders);
}


osbool sorder_list_window_delete_line(struct sorder_list_window *windat, sorder_t sorder)
{
	int i;

//	if (windat == NULL)
//		return FALSE;

	/* Find the index entry for the deleted order, and if it doesn't index itself, shuffle all the indexes along
	 * so that they remain in the correct places. */

//	for (i = 0; i < file->sorders->sorder_count && file->sorders->sorders[i].sort_index != sorder; i++);

//	if (file->sorders->sorders[i].sort_index == sorder && i != sorder) {
//		index = i;

//		if (index > sorder)
//			for (i = index; i > sorder; i--)
//				file->sorders->sorders[i].sort_index = file->sorders->sorders[i-1].sort_index;
//		else
//			for (i = index; i < sorder; i++)
//				file->sorders->sorders[i].sort_index = file->sorders->sorders[i+1].sort_index;
//	}


	/* Adjust the sort indexes that point to entries above the deleted one, by reducing any indexes that are
	 * greater than the deleted entry by one.
	 */

//	for (i = 0; i < file->sorders->sorder_count; i++)
//		if (file->sorders->sorders[i].sort_index > sorder)
//			file->sorders->sorders[i].sort_index = file->sorders->sorders[i].sort_index - 1;

	/* Update the main standing order display window. */

//	sorder_set_window_extent(file->sorders);
//	if (file->sorders->sorder_window != NULL) {
//		windows_open(file->sorders->sorder_window);
//		if (config_opt_read("AutoSortSOrders")) {
//			sorder_sort(file->sorders);
//			sorder_force_window_redraw(file->sorders, file->sorders->sorder_count, file->sorders->sorder_count, wimp_ICON_WINDOW);
//		} else {
//			sorder_force_window_redraw(file->sorders, 0, file->sorders->sorder_count, wimp_ICON_WINDOW);
//		}
//	}

}






/**
 * Save the standing order list window details from a window to a CashBook
 * file. This assumes that the caller has laready created a suitable section
 * in the file to be written.
 *
 * \param *windat		The window whose details to write.
 * \param *out			The file handle to write to.
 */

void sorder_list_window_write_file(struct sorder_list_window *windat, FILE *out)
{
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (windat == NULL)
		return;

	/* We should be in a [StandingOrders] section by now. */

	column_write_as_text(windat->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	sort_write_as_text(windat->sort, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "SortOrder: %s\n", buffer);
}


/**
 * Process a WinColumns line from the StandingOrders section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The column text line.
 */

void sorder_list_window_read_file_wincolumns(struct sorder_list_window *windat, char *columns)
{
	if (windat == NULL)
		return;

	column_init_window(windat->columns, 0, TRUE, columns);
}


/**
 * Process a SortOrder line from the StandingOrders section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The sort order text line.
 */

void sorder_list_window_read_file_sortorder(struct sorder_list_window *windat, char *order)
{
	if (windat == NULL)
		return;

	sort_read_from_text(windat->sort, order);
}


/**
 * Callback handler for saving a CSV version of the standing order data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool sorder_list_window_save_csv(char *filename, osbool selection, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	sorder_list_window_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the standing order data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool sorder_list_window_save_tsv(char *filename, osbool selection, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	sorder_list_window_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

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

static void sorder_list_window_export_delimited(struct sorder_list_window *windat, char *filename, enum filing_delimit_type format, int filetype)
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

