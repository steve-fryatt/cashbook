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
			SORDER_SORT_OK, SORDER_SORT_CANCEL, sorder_process_sort_window);

	sorder_window_def = templates_load_window("SOrder");
	sorder_window_def->icon_count = 0;

	sorder_pane_def = templates_load_window("SOrderTB");
	sorder_pane_def->sprite_area = sprites;

	sorder_window_menu = templates_get_menu("SOrderMenu");
	ihelp_add_menu(sorder_window_menu, "SorderMenu");

	sorder_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", sorder_save_csv);
	sorder_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", sorder_save_tsv);
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

	file = account_get_file(windat->instance);
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

	height = (windat->sorder_count > MIN_SORDER_ENTRIES) ? windat->sorder_count : MIN_SORDER_ENTRIES;

	transact_get_window_state(file, &parent);

	window_set_initial_area(sorder_window_def, column_get_window_width(windat->columns),
			(height * WINDOW_ROW_HEIGHT) + SORDER_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window(sorder_window_def, &(windat->sorder_window));
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

	columns_place_heading_icons(windat->columns, sorder_pane_def);

	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) windat->sort_sprite;
	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			sorder_pane_def->sprite_area;
	sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

	sorder_adjust_sort_icon_data(file->sorders, &(sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON]));

	#ifdef DEBUG
	debug_printf("Toolbar icons adjusted...");
	#endif

	error = xwimp_create_window(sorder_pane_def, &(windat->sorder_pane));
	if (error != NULL) {
		sorder_delete_window(file->sorders);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	sorder_build_window_title(file);

	/* Open the window. */

	ihelp_add_window(windat->sorder_window , "SOrder", sorder_decode_window_help);
	ihelp_add_window(windat->sorder_pane , "SOrderTB", NULL);

	windows_open(windat->sorder_window);
	windows_open_nested_as_toolbar(windat->sorder_pane,
			windat->sorder_window,
			SORDER_TOOLBAR_HEIGHT-4, FALSE);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(windat->sorder_window, file->sorders);
	event_add_window_menu(windat->sorder_window, sorder_window_menu);
	event_add_window_close_event(windat->sorder_window, sorder_close_window_handler);
	event_add_window_mouse_event(windat->sorder_window, sorder_window_click_handler);
	event_add_window_scroll_event(windat->sorder_window, sorder_window_scroll_handler);
	event_add_window_redraw_event(windat->sorder_window, sorder_window_redraw_handler);
	event_add_window_menu_prepare(windat->sorder_window, sorder_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->sorder_window, sorder_window_menu_selection_handler);
	event_add_window_menu_warning(windat->sorder_window, sorder_window_menu_warning_handler);
	event_add_window_menu_close(windat->sorder_window, sorder_window_menu_close_handler);

	event_add_window_user_data(windat->sorder_pane, file->sorders);
	event_add_window_menu(windat->sorder_pane, sorder_window_menu);
	event_add_window_mouse_event(windat->sorder_pane, sorder_pane_click_handler);
	event_add_window_menu_prepare(windat->sorder_pane, sorder_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->sorder_pane, sorder_window_menu_selection_handler);
	event_add_window_menu_warning(windat->sorder_pane, sorder_window_menu_warning_handler);
	event_add_window_menu_close(windat->sorder_pane, sorder_window_menu_close_handler);
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

