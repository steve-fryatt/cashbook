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
 * \file: preset_list_window.c
 *
 * Transaction Preset List Window implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

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
#include "preset_list_window.h"

#include "account.h"
#include "account_menu.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "list_window.h"
#include "preset.h"
#include "preset_dialogue.h"
#include "print_dialogue.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
#include "window.h"

/* Preset List Window icons. */

#define PRESET_LIST_WINDOW_KEY 0
#define PRESET_LIST_WINDOW_NAME 1
#define PRESET_LIST_WINDOW_FROM 2
#define PRESET_LIST_WINDOW_FROM_REC 3
#define PRESET_LIST_WINDOW_FROM_NAME 4
#define PRESET_LIST_WINDOW_TO 5
#define PRESET_LIST_WINDOW_TO_REC 6
#define PRESET_LIST_WINDOW_TO_NAME 7
#define PRESET_LIST_WINDOW_AMOUNT 8
#define PRESET_LIST_WINDOW_DESCRIPTION 9

/* Preset List Toolbar icons. */

#define PRESET_LIST_WINDOW_PANE_KEY 0
#define PRESET_LIST_WINDOW_PANE_NAME 1
#define PRESET_LIST_WINDOW_PANE_FROM 2
#define PRESET_LIST_WINDOW_PANE_TO 3
#define PRESET_LIST_WINDOW_PANE_AMOUNT 4
#define PRESET_LIST_WINDOW_PANE_DESCRIPTION 5

#define PRESET_LIST_WINDOW_PANE_PARENT 6
#define PRESET_LIST_WINDOW_PANE_ADDPRESET 7
#define PRESET_LIST_WINDOW_PANE_PRINT 8
#define PRESET_LIST_WINDOW_PANE_SORT 9

#define PRESET_LIST_WINDOW_PANE_SORT_DIR_ICON 10

/* Preset List Menu entries. */

#define PRESET_LIST_WINDOW_MENU_SORT 0
#define PRESET_LIST_WINDOW_MENU_EDIT 1
#define PRESET_LIST_WINDOW_MENU_NEWPRESET 2
#define PRESET_LIST_WINDOW_MENU_EXPCSV 3
#define PRESET_LIST_WINDOW_MENU_EXPTSV 4
#define PRESET_LIST_WINDOW_MENU_PRINT 5

/* Preset Sort Window icons */

#define PRESET_LIST_WINDOW_SORT_OK 2
#define PRESET_LIST_WINDOW_SORT_CANCEL 3
#define PRESET_LIST_WINDOW_SORT_FROM 4
#define PRESET_LIST_WINDOW_SORT_TO 5
#define PRESET_LIST_WINDOW_SORT_AMOUNT 6
#define PRESET_LIST_WINDOW_SORT_DESCRIPTION 7
#define PRESET_LIST_WINDOW_SORT_KEY 8
#define PRESET_LIST_WINDOW_SORT_NAME 9
#define PRESET_LIST_WINDOW_SORT_ASCENDING 10
#define PRESET_LIST_WINDOW_SORT_DESCENDING 11

/**
 * The minimum number of entries in the Preset List Window.
 */

#define PRESET_LIST_WINDOW_MIN_ENTRIES 10

/**
 * The height of the Preset List Window toolbar, in OS Units.
 */

#define PRESET_LIST_WINDOW_TOOLBAR_HEIGHT 132

/**
 * The number of draggable columns in the Preset List Window.
 */

#define PRESET_LIST_WINDOW_COLUMNS 10

/**
 * The Preset List Window column map.
 */

static struct column_map preset_list_window_columns[PRESET_LIST_WINDOW_COLUMNS] = {
	{PRESET_LIST_WINDOW_KEY, PRESET_LIST_WINDOW_PANE_KEY, wimp_ICON_WINDOW, SORT_CHAR},
	{PRESET_LIST_WINDOW_NAME, PRESET_LIST_WINDOW_PANE_NAME, wimp_ICON_WINDOW, SORT_NAME},
	{PRESET_LIST_WINDOW_FROM, PRESET_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{PRESET_LIST_WINDOW_FROM_REC, PRESET_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{PRESET_LIST_WINDOW_FROM_NAME, PRESET_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{PRESET_LIST_WINDOW_TO, PRESET_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{PRESET_LIST_WINDOW_TO_REC, PRESET_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{PRESET_LIST_WINDOW_TO_NAME, PRESET_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{PRESET_LIST_WINDOW_AMOUNT, PRESET_LIST_WINDOW_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{PRESET_LIST_WINDOW_DESCRIPTION, PRESET_LIST_WINDOW_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION}
};

/**
 * The Preset List Window Sort Dialogue column icons.
 */

static struct sort_dialogue_icon preset_list_window_sort_columns[] = {
	{PRESET_LIST_WINDOW_SORT_FROM, SORT_FROM},
	{PRESET_LIST_WINDOW_SORT_TO, SORT_TO},
	{PRESET_LIST_WINDOW_SORT_AMOUNT, SORT_AMOUNT},
	{PRESET_LIST_WINDOW_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{PRESET_LIST_WINDOW_SORT_KEY, SORT_CHAR},
	{PRESET_LIST_WINDOW_SORT_NAME, SORT_LEFT},
	{0, SORT_NONE}
};

/**
 * The Preset List Window Sort Dialogue direction icons.
 */

static struct sort_dialogue_icon preset_list_window_sort_directions[] = {
	{PRESET_LIST_WINDOW_SORT_ASCENDING, SORT_ASCENDING},
	{PRESET_LIST_WINDOW_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/**
 * The Preset List Window definition.
 */

static struct list_window_definition preset_list_window_definition = {
	"Preset",
	"PresetTB",
	NULL,
	PRESET_LIST_WINDOW_TOOLBAR_HEIGHT,
	0,
	preset_list_window_columns,
	NULL,
	PRESET_LIST_WINDOW_COLUMNS,
	"LimPresetCols",
	"PresetCols",
	PRESET_LIST_WINDOW_PANE_SORT_DIR_ICON,
	preset_list_window_sort_columns,
	preset_list_window_sort_directions,
	"PresetTitle",					/*<< Window Title token.		*/
	"Preset",					/**< Window Help token base.		*/
	"PresetTB",					/**< Window Toolbar help token base.	*/
	NULL,						/**< Window Footer help token base.	*/
	PRESET_LIST_WINDOW_MIN_ENTRIES,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/**
 * Preset List Window line redraw data.
 */

struct preset_list_window_redraw {
	/**
	 * The number of the preset relating to the line.
	 */
	preset_t				preset;
};

/**
 * Preset List Window instance data structure.
 */

struct preset_list_window {
	/**
	 * The presets instance owning the Preset List Window.
	 */
	struct preset_block			*instance;

	/**
	 * The list window for the Preset List Window.
	 */
	struct list_window			*window;

	/**
	 * Wimp window handle for the main Preset List Window.
	 */
	wimp_w					preset_window;

	/**
	 * Indirected title data for the window.
	 */
	char					window_title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the Preset List Window Toolbar pane.
	 */
	wimp_w					preset_pane;

	/**
	 * Instance handle for the window's column definitions.
	 */
	struct column_block			*columns;

	/**
	 * Instance handle for the window's sort code.
	 */
	struct sort_block			*sort;

	/**
	 * Count of the number of populated display lines in the window.
	 */
	int					display_lines;

	/**
	 * Flex array holding the line data for the window.
	 */
	struct preset_list_window_redraw	*line_data;
};

/**
 * The Preset List Window base instance.
 */

static struct list_window_block		*preset_list_window_block = NULL;

/**
 * The handle of the Preset List Window menu.
 */

static wimp_menu			*preset_list_window_menu = NULL;

/**
 * The window line associated with the most recent menu opening.
 */

static int				preset_list_window_menu_line = -1;

/**
 * The Preset List Window Sort dialogue.
 */

static struct sort_dialogue_block	*preset_list_window_sort_dialogue = NULL;

/**
 * The Preset List Window Sort callbacks.
 */

static struct sort_callback		preset_list_window_sort_callbacks;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block		*preset_list_window_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block		*preset_list_window_saveas_tsv = NULL;

/* Static Function Prototypes. */

static void preset_list_window_delete(void *data);
static void preset_list_window_click_handler(wimp_pointer *pointer);
static void preset_list_window_pane_click_handler(wimp_pointer *pointer);
static void preset_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void preset_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void preset_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void preset_list_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void preset_list_window_scroll_handler(wimp_scroll *scroll);
static void preset_list_window_redraw_handler(wimp_draw *redraw, void *data);
static void preset_list_window_adjust_columns(void *data, wimp_i group, int width);
static void preset_list_window_adjust_sort_icon(struct preset_list_window *windat);
static void preset_list_window_adjust_sort_icon_data(struct preset_list_window *windat, wimp_icon *icon);
static void preset_list_window_set_extent(struct preset_list_window *windat);
static void preset_list_window_force_redraw(struct preset_list_window *windat, int from, int to, wimp_i column);
static void preset_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static int preset_list_window_get_line_from_preset(struct preset_list_window *windat, preset_t preset);
static void preset_list_window_open_sort_window(struct preset_list_window *windat, wimp_pointer *ptr);
static osbool preset_list_window_process_sort_window(enum sort_type order, void *data);
static void preset_list_window_open_print_window(struct preset_list_window *windat, wimp_pointer *ptr, osbool restore);
static struct report *preset_list_window_print(struct report *report, void *data, date_t from, date_t to);
static int preset_list_window_sort_compare(enum sort_type type, int index1, int index2, void *data);
static void preset_list_window_sort_swap(int index1, int index2, void *data);
static osbool preset_list_window_save_csv(char *filename, osbool selection, void *data);
static osbool preset_list_window_save_tsv(char *filename, osbool selection, void *data);
static void preset_list_window_export_delimited(struct preset_list_window *windat, char *filename, enum filing_delimit_type format, int filetype);

/**
 * Test whether a line number is safe to look up in the redraw data array.
 */

#define preset_list_window_line_valid(windat, line) (((line) >= 0) && ((line) < ((windat)->display_lines)))


/**
 * Initialise the Preset List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void preset_list_window_initialise(osspriteop_area *sprites)
{
	wimp_w sort_window;

	preset_list_window_definition.callback_window_close_handler = preset_list_window_delete;

	sort_window = templates_create_window("SortPreset");
	ihelp_add_window(sort_window, "SortPreset", NULL);
	preset_list_window_sort_dialogue = sort_dialogue_create(sort_window, preset_list_window_sort_columns, preset_list_window_sort_directions,
			PRESET_LIST_WINDOW_SORT_OK, PRESET_LIST_WINDOW_SORT_CANCEL, preset_list_window_process_sort_window);

	preset_list_window_sort_callbacks.compare = preset_list_window_sort_compare;
	preset_list_window_sort_callbacks.swap = preset_list_window_sort_swap;

	preset_list_window_block = list_window_create(&preset_list_window_definition, sprites);

	preset_list_window_menu = templates_get_menu("PresetMenu");
	ihelp_add_menu(preset_list_window_menu, "PresetMenu");

	preset_list_window_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", preset_list_window_save_csv);
	preset_list_window_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", preset_list_window_save_tsv);
}


/**
 * Create a new Preset List Window instance.
 *
 * \param *parent		The parent presets instance.
 * \return			Pointer to the new instance, or NULL.
 */

struct preset_list_window *preset_list_window_create_instance(struct preset_block *parent)
{
	struct preset_list_window *new;

	new = heap_alloc(sizeof(struct preset_list_window));
	if (new == NULL)
		return NULL;

	new->instance = parent;

	new->window = NULL;
	new->preset_window = NULL;
	new->preset_pane = NULL;
	new->columns = NULL;
	new->sort = NULL;

	new->display_lines = 0;
	new->line_data = NULL;

	/* Initialise the List Window. */

	new->window = list_window_create_instance(preset_list_window_block, preset_get_file(parent), new);
	if (new->window == NULL) {
		preset_list_window_delete_instance(new);
		return NULL;
	}

	/* Initialise the window columns. */

//	new-> columns = column_create_instance(PRESET_LIST_WINDOW_COLUMNS, preset_list_window_columns, NULL, PRESET_LIST_WINDOW_PANE_SORT_DIR_ICON);
//	if (new->columns == NULL) {
//		preset_list_window_delete_instance(new);
//		return NULL;
//	}

//	column_set_minimum_widths(new->columns, config_str_read("LimPresetCols"));
//	column_init_window(new->columns, 0, FALSE, config_str_read("PresetCols"));

	/* Initialise the window sort. */

	new->sort = sort_create_instance(SORT_CHAR | SORT_ASCENDING, SORT_NONE, &preset_list_window_sort_callbacks, new);
	if (new->sort == NULL) {
		preset_list_window_delete_instance(new);
		return NULL;
	}

	/* Set the initial lines up */

	if (!flexutils_initialise((void **) &(new->line_data))) {
		preset_list_window_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a Preset List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void preset_list_window_delete_instance(struct preset_list_window *windat)
{
	if (windat == NULL)
		return;

	if (windat->line_data != NULL)
		flexutils_free((void **) &(windat->line_data));

//	column_delete_instance(windat->columns);
	sort_delete_instance(windat->sort);

	list_window_delete_instance(windat->window);

	preset_list_window_delete(windat);

	heap_free(windat);
}


/**
 * Create and open a Preset List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void preset_list_window_open(struct preset_list_window *windat)
{
	if (windat == NULL)
		return;

	list_window_open(windat->window);
}


/**
 * Close and delete a Preset List Window associated with the given
 * instance.
 *
 * \param *data		The instance being closed.
 */

static void preset_list_window_delete(void *data)
{
	struct preset_list_window *windat = data;

	if (windat == NULL)
		return;

//	#ifdef DEBUG
	debug_printf ("\\RDeleting Preset List window");
//	#endif

	/* Close any dialogues which belong to this window. */

	dialogue_force_all_closed(NULL, windat);
	sort_dialogue_close(preset_list_window_sort_dialogue, windat);
}


/**
 * Process mouse clicks in the Preset List window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void preset_list_window_click_handler(wimp_pointer *pointer)
{
	struct preset_list_window	*windat;
	struct file_block		*file;
	int				line;
	wimp_window_state		window;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Find the window type and get the line clicked on. */

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = window_calculate_click_row(&(pointer->pos), &window, PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, windat->display_lines);

	/* Handle double-clicks, which will open an edit preset window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1)
		preset_open_edit_window(file, windat->line_data[line].preset, pointer);
}


/**
 * Process mouse clicks in the Preset List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void preset_list_window_pane_click_handler(wimp_pointer *pointer)
{
	struct preset_list_window	*windat;
	struct file_block		*file;
	wimp_window_state		window;
	wimp_icon_state			icon;
	int				ox;
	enum sort_type			sort_order;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	column_update_heading_icon_click(windat->columns, pointer);

	/* Process toolbar clicks and column heading drags. */

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case PRESET_LIST_WINDOW_PANE_PARENT:
			transact_bring_window_to_top(file);
			break;

		case PRESET_LIST_WINDOW_PANE_PRINT:
			preset_list_window_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case PRESET_LIST_WINDOW_PANE_ADDPRESET:
			preset_open_edit_window(file, NULL_PRESET, pointer);
			break;

		case PRESET_LIST_WINDOW_PANE_SORT:
			preset_list_window_open_sort_window(windat, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case PRESET_LIST_WINDOW_PANE_PRINT:
			preset_list_window_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;

		case PRESET_LIST_WINDOW_PANE_SORT:
			preset_sort(windat->instance);
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
				preset_list_window_adjust_sort_icon(windat);
				windows_redraw(windat->preset_pane);
				preset_list_window_sort(windat);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimPresetCols"));
		column_start_drag(windat->columns, pointer, windat, windat->preset_window, preset_list_window_adjust_columns);
	}
}


/**
 * Process menu prepare events in the Preset List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void preset_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct preset_list_window	*windat;
	int				line;
	wimp_window_state		window;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (pointer != NULL) {
		preset_list_window_menu_line = -1;

		if (w == windat->preset_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = window_calculate_click_row(&(pointer->pos), &window, PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, windat->display_lines);

			if (line != -1)
				preset_list_window_menu_line = line;
		}

		saveas_initialise_dialogue(preset_list_window_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(preset_list_window_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_shade_entry(preset_list_window_menu, PRESET_LIST_WINDOW_MENU_EDIT, preset_list_window_menu_line == -1);

	preset_list_window_force_redraw(windat, preset_list_window_menu_line, preset_list_window_menu_line, wimp_ICON_WINDOW);
}


/**
 * Process menu selection events in the Preset List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void preset_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct preset_list_window	*windat;
	struct file_block		*file;
	wimp_pointer			pointer;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case PRESET_LIST_WINDOW_MENU_SORT:
		preset_list_window_open_sort_window(windat, &pointer);
		break;

	case PRESET_LIST_WINDOW_MENU_EDIT:
		if (preset_list_window_menu_line != -1)
			preset_open_edit_window(file, windat->line_data[preset_list_window_menu_line].preset, &pointer);
		break;

	case PRESET_LIST_WINDOW_MENU_NEWPRESET:
		preset_open_edit_window(file, NULL_PRESET, &pointer);
		break;

	case PRESET_LIST_WINDOW_MENU_PRINT:
		preset_list_window_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
		break;
	}
}


/**
 * Process submenu warning events in the Preset List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void preset_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct preset_list_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	switch (warning->selection.items[0]) {
	case PRESET_LIST_WINDOW_MENU_EXPCSV:
		saveas_prepare_dialogue(preset_list_window_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case PRESET_LIST_WINDOW_MENU_EXPTSV:
		saveas_prepare_dialogue(preset_list_window_saveas_tsv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process menu close events in the Preset List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void preset_list_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	struct preset_list_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat != NULL)
		preset_list_window_force_redraw(windat, preset_list_window_menu_line, preset_list_window_menu_line, wimp_ICON_WINDOW);

	preset_list_window_menu_line = -1;
}


/**
 * Process scroll events in the Preset List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void preset_list_window_scroll_handler(wimp_scroll *scroll)
{
	window_process_scroll_event(scroll, PRESET_LIST_WINDOW_TOOLBAR_HEIGHT);

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

	wimp_open_window((wimp_open *) scroll);
}


/**
 * Process redraw events in the Preset List window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void preset_list_window_redraw_handler(wimp_draw *redraw, void *data)
{
	struct preset_list_window	*windat;
	struct file_block		*file;
	int				top, base, y, select;
	acct_t				account;
	enum transact_flags		flags;
	preset_t			preset;
	char				icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN]; /* Assumes descript is longest. */
	osbool				more;
	wimp_window			*window_def;

	windat = data;
	if (windat == NULL || windat->instance == NULL || windat->columns == NULL)
		return;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return;

	window_def = list_window_get_window_def(preset_list_window_block);
	if (window_def == NULL)
		return;

	/* Identify if there is a selected line to highlight. */

	if (redraw->w == event_get_current_menu_window())
		select = preset_list_window_menu_line;
	else
		select = -1;

	/* Set the horizontal positions of the icons. */

	columns_place_table_icons_horizontally(windat->columns, window_def, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);

	window_set_icon_templates(window_def);

	/* Perform the redraw. */

	more = wimp_redraw_window(redraw);

	while (more) {
		window_plot_background(redraw, PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, wimp_COLOUR_WHITE, select, &top, &base);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			preset = (y < windat->display_lines) ? windat->line_data[y].preset : 0;

			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(windat->columns, window_def,
					WINDOW_ROW_Y0(PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, y), WINDOW_ROW_Y1(PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= windat->display_lines) {
				columns_plot_empty_table_icons(windat->columns);
				continue;
			}

			flags = preset_get_flags(file, preset);

			/* Key field */

			window_plot_char_field(PRESET_LIST_WINDOW_KEY, preset_get_action_key(file, preset), wimp_COLOUR_BLACK);

			/* Name field */

			window_plot_text_field(PRESET_LIST_WINDOW_NAME, preset_get_name(file, preset, NULL, 0), wimp_COLOUR_BLACK);

			/* From field */

			account = preset_get_from(file, preset);

			window_plot_text_field(PRESET_LIST_WINDOW_FROM, account_get_ident(file, account), wimp_COLOUR_BLACK);
			window_plot_reconciled_field(PRESET_LIST_WINDOW_FROM_REC, (flags & TRANS_REC_FROM), wimp_COLOUR_BLACK);
			window_plot_text_field(PRESET_LIST_WINDOW_FROM_NAME, account_get_name(file, account), wimp_COLOUR_BLACK);

			/* To field */

			account = preset_get_to(file, preset);

			window_plot_text_field(PRESET_LIST_WINDOW_TO, account_get_ident(file, account), wimp_COLOUR_BLACK);
			window_plot_reconciled_field(PRESET_LIST_WINDOW_TO_REC, (flags & TRANS_REC_TO), wimp_COLOUR_BLACK);
			window_plot_text_field(PRESET_LIST_WINDOW_TO_NAME, account_get_name(file, account), wimp_COLOUR_BLACK);

			/* Amount field */

			window_plot_currency_field(PRESET_LIST_WINDOW_AMOUNT, preset_get_amount(file, preset), wimp_COLOUR_BLACK);

			/* Description field */

			window_plot_text_field(PRESET_LIST_WINDOW_DESCRIPTION, preset_get_description(file, preset, NULL, 0), wimp_COLOUR_BLACK);
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

static void preset_list_window_adjust_columns(void *data, wimp_i group, int width)
{
	struct preset_list_window	*windat = (struct preset_list_window *) data;
	struct file_block		*file;
	int				new_extent;
	wimp_window_info		window;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return;

	columns_update_dragged(windat->columns, windat->preset_pane, NULL, group, width);

	new_extent = column_get_window_width(windat->columns);

	preset_list_window_adjust_sort_icon(windat);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(windat->preset_window);
	windows_redraw(windat->preset_pane);

	/* Set the horizontal extent of the window and pane. */

	window.w = windat->preset_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = windat->preset_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(file, TRUE);
}


/**
 * Adjust the sort icon in a preset window, to reflect the current column
 * heading positions.
 *
 * \param *windat		The window to be updated.
 */

static void preset_list_window_adjust_sort_icon(struct preset_list_window *windat)
{
	wimp_icon_state		icon;

	if (windat == NULL)
		return;

	icon.w = windat->preset_pane;
	icon.i = PRESET_LIST_WINDOW_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	preset_list_window_adjust_sort_icon_data(windat, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current preset sort settings.
 *
 * \param *file			The preset window to be updated.
 * \param *icon			The icon to be updated.
 */

static void preset_list_window_adjust_sort_icon_data(struct preset_list_window *windat, wimp_icon *icon)
{
	enum sort_type	sort_order;
	wimp_window	*window_pane_def;

	if (windat == NULL)
		return;

	window_pane_def = list_window_get_toolbar_def(preset_list_window_block);
	if (window_pane_def == NULL)
		return;

	sort_order = sort_get_order(windat->sort);

	column_update_sort_indicator(windat->columns, icon, window_pane_def, sort_order);
}

/**
 * Set the extent of the preset window for the specified file.
 *
 * \param *windat		The preset window to update.
 */

static void preset_list_window_set_extent(struct preset_list_window *windat)
{
	int	lines;


	if (windat == NULL || windat->preset_window == NULL)
		return;

	lines = (windat->display_lines > PRESET_LIST_WINDOW_MIN_ENTRIES) ? windat->display_lines : PRESET_LIST_WINDOW_MIN_ENTRIES;

	window_set_extent(windat->preset_window, lines, PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, column_get_window_width(windat->columns));
}


/**
 * Force the redraw of one or all of the presets in the given Preset list
 * bwindow.
 *
 * \param *file			The file owning the window to redraw.
 * \param preset		The preset to redraw, or NULL_PRESET for all.
 */

void preset_list_window_redraw(struct preset_list_window *windat, preset_t preset)
{
	int from, to;

	if (windat == NULL)
		return;

	if (preset != NULL_PRESET) {
		from = preset_list_window_get_line_from_preset(windat, preset);
		to = from;
	} else {
		from = 0;
		to = windat->display_lines - 1;
	}

	preset_list_window_force_redraw(windat, from, to, wimp_ICON_WINDOW);
}


/**
 * Force a redraw of the Preset list window, for the given range of lines.
 *
 * \param *file			The preset window to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void preset_list_window_force_redraw(struct preset_list_window *windat, int from, int to, wimp_i column)
{
	wimp_window_info	window;

	if (windat == NULL || windat->preset_window == NULL)
		return;

	window.w = windat->preset_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(windat->columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, from);
	window.extent.y0 = WINDOW_ROW_BASE(PRESET_LIST_WINDOW_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(windat->preset_window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);
}


/**
 * Turn a mouse position over the Preset List window into an interactive help
 * token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void preset_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int				xpos;
	wimp_i				icon;
	wimp_window_state		window;
	struct preset_list_window	*windat;
	wimp_window			*window_def;

	*buffer = '\0';

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	window_def = list_window_get_toolbar_def(preset_list_window_block);
	if (window_def == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	icon = column_find_icon_from_xpos(windat->columns, xpos);
	if (icon == wimp_ICON_WINDOW)
		return;

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, window_def->icons[icon].data.indirected_text.validation, 'N'))
		string_printf(buffer, IHELP_INAME_LEN, "Col%d", icon);
}


/**
 * Find the display line in a preset window which points to the specified
 * preset under the applied sort.
 *
 * \param *windat		The preset list window to search.
 * \param preset		The preset to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

static int preset_list_window_get_line_from_preset(struct preset_list_window *windat, preset_t preset)
{
	int	i;
	int	line = -1;

	if (windat == NULL || windat->line_data == NULL)
		return line;

	for (i = 0; i < windat->display_lines; i++) {
		if (windat->line_data[i].preset == preset) {
			line = i;
			break;
		}
	}

	return line;
}


/**
 * Find the preset which corresponds to a display line in the specified
 * preset list window.
 *
 * \param *windat		The preset list window to search in.
 * \param line			The display line to return the preset for.
 * \return			The appropriate transaction, or NULL_PRESET.
 */

preset_t preset_list_window_get_preset_from_line(struct preset_list_window *windat, int line)
{
	if (windat == NULL || windat->line_data == NULL || !preset_list_window_line_valid(windat, line))
		return NULL_PRESET;

	return windat->line_data[line].preset;
}


/**
 * Open the Preset Sort dialogue for a given preset list window.
 *
 * \param *file			The preset window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void preset_list_window_open_sort_window(struct preset_list_window *windat, wimp_pointer *ptr)
{
	if (windat == NULL || ptr == NULL)
		return;

	sort_dialogue_open(preset_list_window_sort_dialogue, ptr, sort_get_order(windat->sort), windat);
}


/**
 * Take the contents of an updated Preset Sort window and process
 * the data.
 *
 * \param order			The new sort order selected by the user.
 * \param *data			The preset window associated with the Sort dialogue.
 * \return			TRUE if successful; else FALSE.
 */

static osbool preset_list_window_process_sort_window(enum sort_type order, void *data)
{
	struct preset_list_window *windat = (struct preset_list_window *) data;

	if (windat == NULL)
		return FALSE;

	sort_set_order(windat->sort, order);

	preset_list_window_adjust_sort_icon(windat);
	windows_redraw(windat->preset_pane);
	preset_list_window_sort(windat);

	return TRUE;
}


/**
 * Open the Preset Print dialogue for a given preset list window.
 *
 * \param *windat		The preset window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to retain the previous settings; FALSE to
 *				return to defaults.
 */

static void preset_list_window_open_print_window(struct preset_list_window *windat, wimp_pointer *ptr, osbool restore)
{
	struct file_block *file;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return;

	print_dialogue_open(file->print, ptr, FALSE, restore, "PrintPreset", "PrintTitlePreset", windat, preset_list_window_print, windat);
}


/**
 * Send the contents of the Preset Window to the printer, via the reporting
 * system.
 *
 * \param *report		The report handle to use for output.
 * \param *data			The preset window structure to be printed.
 * \param from			The date to print from.
 * \param to			The date to print to.
 * \return			Pointer to the report, or NULL on failure.
 */

static struct report *preset_list_window_print(struct report *report, void *data, date_t from, date_t to)
{
	struct preset_list_window	*windat = data;
	struct file_block		*file;
	int				line, column;
	preset_t			preset;
	char				rec_char[REC_FIELD_LEN];
	wimp_i				columns[PRESET_LIST_WINDOW_COLUMNS];

	if (report == NULL || windat == NULL || windat->instance == NULL)
		return NULL;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return NULL;

	if (!column_get_icons(windat->columns, columns, PRESET_LIST_WINDOW_COLUMNS, FALSE))
		return NULL;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	hourglass_on();

	/* Output the page title. */

	stringbuild_reset();

	stringbuild_add_string("\\b\\u");
	stringbuild_add_message_param("PresetTitle",
			file_get_leafname(file, NULL, 0),
			NULL, NULL, NULL);

	stringbuild_report_line(report, 1);

	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	stringbuild_reset();
	columns_print_heading_names(windat->columns, windat->preset_pane);
	stringbuild_report_line(report, 0);

	/* Output the standing order data as a set of delimited lines. */

	for (line = 0; line < windat->display_lines; line++) {
		preset = windat->line_data[line].preset;

		stringbuild_reset();

		for (column = 0; column < PRESET_LIST_WINDOW_COLUMNS; column++) {
			if (column == 0)
				stringbuild_add_string("\\k");
			else
				stringbuild_add_string("\\t");

			switch (columns[column]) {
			case PRESET_LIST_WINDOW_KEY:
				stringbuild_add_printf("\\v\\c%c", preset_get_action_key(file, preset));
				/* Note that action_key can be zero, in which case %c terminates. */
				break;
			case PRESET_LIST_WINDOW_NAME:
				stringbuild_add_printf("\\v%s", preset_get_name(file, preset, NULL, 0));
				break;
			case PRESET_LIST_WINDOW_FROM:
				stringbuild_add_string(account_get_ident(file, preset_get_from(file, preset)));
				break;
			case PRESET_LIST_WINDOW_FROM_REC:
				if (preset_get_flags(file, preset) & TRANS_REC_FROM)
					stringbuild_add_string(rec_char);
				break;
			case PRESET_LIST_WINDOW_FROM_NAME:
				stringbuild_add_string("\\v");
				stringbuild_add_string(account_get_name(file, preset_get_from(file, preset)));
				break;
			case PRESET_LIST_WINDOW_TO:
				stringbuild_add_string(account_get_ident(file, preset_get_to(file, preset)));
				break;
			case PRESET_LIST_WINDOW_TO_REC:
				if (preset_get_flags(file, preset) & TRANS_REC_TO)
					stringbuild_add_string(rec_char);
				break;
			case PRESET_LIST_WINDOW_TO_NAME:
				stringbuild_add_string("\\v");
				stringbuild_add_string(account_get_name(file, preset_get_to(file, preset)));
				break;
			case PRESET_LIST_WINDOW_AMOUNT:
				stringbuild_add_string("\\v\\d\\r");
				stringbuild_add_currency(preset_get_amount(file, preset), FALSE);
				break;
			case PRESET_LIST_WINDOW_DESCRIPTION:
				stringbuild_add_string("\\v");
				stringbuild_add_string(preset_get_description(file, preset, NULL, 0));
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
 * Sort the presets in a given list window based on that instance's sort
 * setting.
 *
 * \param *windat		The preset window instance to sort.
 */

void preset_list_window_sort(struct preset_list_window *windat)
{
	if (windat == NULL)
		return;

	#ifdef DEBUG
	debug_printf("Sorting preset window");
	#endif

	hourglass_on();

	/* Run the sort. */

	sort_process(windat->sort, windat->display_lines);

	preset_list_window_force_redraw(windat, 0, windat->display_lines - 1, wimp_ICON_WINDOW);

	hourglass_off();
}


/**
 * Compare two lines of a preset list, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The index of the first line to be compared.
 * \param index2		The index of the second line to be compared.
 * \param *data			Client specific data, which is our window block.
 * \return			The comparison result.
 */

static int preset_list_window_sort_compare(enum sort_type type, int index1, int index2, void *data)
{
	struct preset_list_window	*windat = data;
	struct file_block		*file = NULL;

	if (windat == NULL || windat->instance == NULL)
		return 0;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return 0;

	switch (type) {
	case SORT_CHAR:
		return (preset_get_action_key(file, windat->line_data[index1].preset) -
				preset_get_action_key(file, windat->line_data[index2].preset));

	case SORT_NAME:
		return strcmp(preset_get_name(file, windat->line_data[index1].preset, NULL, 0),
				preset_get_name(file, windat->line_data[index2].preset, NULL, 0));

	case SORT_FROM:
		return strcmp(account_get_name(file, preset_get_from(file, windat->line_data[index1].preset)),
				account_get_name(file, preset_get_from(file, windat->line_data[index2].preset)));

	case SORT_TO:
		return strcmp(account_get_name(file, preset_get_to(file, windat->line_data[index1].preset)),
				account_get_name(file, preset_get_to(file, windat->line_data[index2].preset)));

	case SORT_AMOUNT:
		return (preset_get_amount(file, windat->line_data[index1].preset) -
				preset_get_amount(file, windat->line_data[index2].preset));

	case SORT_DESCRIPTION:
		return strcmp(preset_get_description(file, windat->line_data[index1].preset, NULL, 0),
				preset_get_description(file, windat->line_data[index2].preset, NULL, 0));

	default:
		return 0;
	}
}


/**
 * Swap the sort index of two lines of a preset list.
 *
 * \param index1		The index of the first line to be swapped.
 * \param index2		The index of the second line to be swapped.
 * \param *data			Client specific data, which is our window block.
 */

static void preset_list_window_sort_swap(int index1, int index2, void *data)
{
	struct preset_list_window	*windat = data;
	int				temp;

	if (windat == NULL)
		return;

	temp = windat->line_data[index1].preset;
	windat->line_data[index1].preset = windat->line_data[index2].preset;
	windat->line_data[index2].preset = temp;
}


/**
 * Initialise the contents of the preset list window, creating an entry
 * for each of the required presets.
 *
 * \param *windat		The preset list window instance to initialise.
 * \param presets		The number of presets to insert.
 * \return			TRUE on success; FALSE on failure.
 */

osbool preset_list_window_initialise_entries(struct preset_list_window *windat, int presets)
{
	int i;

	if (windat == NULL || windat->line_data == NULL)
		return FALSE;

	/* Extend the index array. */

	if (!flexutils_resize((void **) &(windat->line_data), sizeof(struct preset_list_window_redraw), presets))
		return FALSE;

	windat->display_lines = presets;

	/* Initialise and sort the entries. */

	for (i = 0; i < presets; i++)
		windat->line_data[i].preset = i;

	preset_list_window_sort(windat);

	return TRUE;
}


/**
 * Add a new preset to an instance of the preset list window.
 *
 * \param *windat		The preset list window instance to add to.
 * \param preset		The preset index to add.
 * \return			TRUE on success; FALSE on failure.
 */

osbool preset_list_window_add_preset(struct preset_list_window *windat, preset_t preset)
{
	if (windat == NULL || windat->line_data == NULL)
		return FALSE;

	/* Extend the index array. */

	if (!flexutils_resize((void **) &(windat->line_data), sizeof(struct preset_list_window_redraw), windat->display_lines + 1))
		return FALSE;

	windat->display_lines++;

	/* Add the new entry, expand the window and sort the entries. */

	windat->line_data[windat->display_lines - 1].preset = preset;

	preset_list_window_set_extent(windat);

	if (config_opt_read("AutoSortPresets"))
		preset_list_window_sort(windat);
	else
		preset_list_window_force_redraw(windat, windat->display_lines - 1, windat->display_lines - 1, wimp_ICON_WINDOW);

	return TRUE;
}


/**
 * Remove a preset from an instance of the preset list window, and update
 * the other entries to allow for its deletion.
 *
 * \param *windat		The preset list window instance to remove from.
 * \param preset		The preset index to remove.
 * \return			TRUE on success; FALSE on failure.
 */

osbool preset_list_window_delete_preset(struct preset_list_window *windat, preset_t preset)
{
	int i = 0, delete = -1;

	if (windat == NULL || windat->line_data == NULL)
		return FALSE;

	/* Find the preset, and decrement any index entries above it. */

	for (i = 0; i < windat->display_lines; i++) {
		if (windat->line_data[i].preset == preset)
			delete = i;
		else if (windat->line_data[i].preset > preset)
			windat->line_data[i].preset--;
	}

	/* Delete the index entry. */

	if (delete >= 0 && !flexutils_delete_object((void **) &(windat->line_data), sizeof(struct preset_list_window_redraw), delete))
		return FALSE;

	windat->display_lines--;

	/* Update the preset window. */

	preset_list_window_set_extent(windat);

	windows_open(windat->preset_window);

	if (config_opt_read("AutoSortPresets"))
		preset_list_window_sort(windat);
	else
		preset_list_window_force_redraw(windat, delete, windat->display_lines, wimp_ICON_WINDOW);

	return TRUE;
}


/**
 * Save the preset list window details from a window to a CashBook file.
 * This assumes that the caller has already created a suitable section
 * in the file to be written.
 *
 * \param *windat		The window whose details to write.
 * \param *out			The file handle to write to.
 */

void preset_list_window_write_file(struct preset_list_window *windat, FILE *out)
{
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (windat == NULL)
		return;

	/* We should be in a [Presets] section by now. */

	column_write_as_text(windat->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	sort_write_as_text(windat->sort, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "SortOrder: %s\n", buffer);
}


/**
 * Process a WinColumns line from the Presets section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The column text line.
 */

void preset_list_window_read_file_wincolumns(struct preset_list_window *windat, char *columns)
{
	if (windat == NULL)
		return;

	list_window_read_file_wincolumns(windat->window, 0, TRUE, columns);
}


/**
 * Process a SortOrder line from the Presets section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The sort order text line.
 */

void preset_list_window_read_file_sortorder(struct preset_list_window *windat, char *order)
{
	if (windat == NULL)
		return;

	sort_read_from_text(windat->sort, order);
}


/**
 * Callback handler for saving a CSV version of the preset data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool preset_list_window_save_csv(char *filename, osbool selection, void *data)
{
	struct preset_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	preset_list_window_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the preset data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool preset_list_window_save_tsv(char *filename, osbool selection, void *data)
{
	struct preset_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	preset_list_window_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

	return TRUE;
}


/**
 * Export the preset data from a file into CSV or TSV format.
 *
 * \param *windat		The preset window to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void preset_list_window_export_delimited(struct preset_list_window *windat, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE			*out;
	struct file_block	*file;
	int			line;
	preset_t		preset;
	char			buffer[FILING_DELIMITED_FIELD_LEN];

	if (windat == NULL || windat->instance == NULL)
		return;

	file = preset_get_file(windat->instance);
	if (file == NULL)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	columns_export_heading_names(windat->columns, windat->preset_pane, out, format, buffer, FILING_DELIMITED_FIELD_LEN);

	/* Output the preset data as a set of delimited lines. */

	for (line = 0; line < windat->display_lines; line++) {
		preset = windat->line_data[line].preset;

		string_printf(buffer, FILING_DELIMITED_FIELD_LEN, "%c", preset_get_action_key(file, preset));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		filing_output_delimited_field(out, preset_get_name(file, preset, NULL, 0), format, DELIMIT_NONE);

		account_build_name_pair(file, preset_get_from(file, preset), buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		account_build_name_pair(file, preset_get_to(file, preset), buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		currency_convert_to_string(preset_get_amount(file, preset), buffer, FILING_DELIMITED_FIELD_LEN);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, preset_get_description(file, preset, NULL, 0), format, DELIMIT_LAST);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}

