/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: presets.c
 *
 * Transaction presets implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

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
#include "presets.h"

#include "account.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "saveas.h"
#include "sort.h"
#include "report.h"
#include "templates.h"
#include "window.h"

/* Main Window Icons
 *
 * Note that these correspond to column numbers.
 */

#define PRESET_ICON_KEY 0
#define PRESET_ICON_NAME 1
#define PRESET_ICON_FROM 2
#define PRESET_ICON_FROM_REC 3
#define PRESET_ICON_FROM_NAME 4
#define PRESET_ICON_TO 5
#define PRESET_ICON_TO_REC 6
#define PRESET_ICON_TO_NAME 7
#define PRESET_ICON_AMOUNT 8
#define PRESET_ICON_DESCRIPTION 9

/* Preset menu */

#define PRESET_MENU_SORT 0
#define PRESET_MENU_EDIT 1
#define PRESET_MENU_NEWPRESET 2
#define PRESET_MENU_EXPCSV 3
#define PRESET_MENU_EXPTSV 4
#define PRESET_MENU_PRINT 5

/* Preset Sort Window */

#define PRESET_SORT_OK 2
#define PRESET_SORT_CANCEL 3
#define PRESET_SORT_FROM 4
#define PRESET_SORT_TO 5
#define PRESET_SORT_AMOUNT 6
#define PRESET_SORT_DESCRIPTION 7
#define PRESET_SORT_KEY 8
#define PRESET_SORT_NAME 9
#define PRESET_SORT_ASCENDING 10
#define PRESET_SORT_DESCENDING 11

/* Preset Edit icons. */

#define PRESET_EDIT_OK          0
#define PRESET_EDIT_CANCEL      1
#define PRESET_EDIT_DELETE      2

#define PRESET_EDIT_NAME        4
#define PRESET_EDIT_KEY         6
#define PRESET_EDIT_DATE        10
#define PRESET_EDIT_TODAY       11
#define PRESET_EDIT_FMIDENT     14
#define PRESET_EDIT_FMREC       15
#define PRESET_EDIT_FMNAME      16
#define PRESET_EDIT_TOIDENT     19
#define PRESET_EDIT_TOREC       20
#define PRESET_EDIT_TONAME      21
#define PRESET_EDIT_REF         24
#define PRESET_EDIT_CHEQUE      25
#define PRESET_EDIT_AMOUNT      28
#define PRESET_EDIT_DESC        31
#define PRESET_EDIT_CARETDATE   12
#define PRESET_EDIT_CARETFROM   17
#define PRESET_EDIT_CARETTO     22
#define PRESET_EDIT_CARETREF    26
#define PRESET_EDIT_CARETAMOUNT 29
#define PRESET_EDIT_CARETDESC   32

/* Toolbar icons. */

#define PRESET_PANE_KEY 0
#define PRESET_PANE_NAME 1
#define PRESET_PANE_FROM 2
#define PRESET_PANE_TO 3
#define PRESET_PANE_AMOUNT 4
#define PRESET_PANE_DESCRIPTION 5

#define PRESET_PANE_PARENT 6
#define PRESET_PANE_ADDPRESET 7
#define PRESET_PANE_PRINT 8
#define PRESET_PANE_SORT 9

#define PRESET_PANE_COL_MAP "0;1;2,3,4;5,6,7;8;9"
#define PRESET_PANE_SORT_DIR_ICON 10

/* Preset window details. */

#define PRESET_COLUMNS 10
#define PRESET_TOOLBAR_HEIGHT 132
#define MIN_PRESET_ENTRIES 10
#define PRESET_NAME_LEN 32


/**
 * Preset Entry data structure -- implementation.
 */

struct preset {
	char			name[PRESET_NAME_LEN];					/**< The name of the preset. */
	char			action_key;						/**< The key used to insert it. */

	enum transact_flags	flags;							/**< Preset flags (containing transaction flags, preset flags, etc). */

	enum preset_caret	caret_target;						/**< The target icon for the caret. */

	date_t			date;							/**< Preset details. */
	acct_t			from;
	acct_t			to;
	amt_t			amount;
	char			reference[REF_FIELD_LEN];
	char			description[DESCRIPT_FIELD_LEN];

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the preset data, and are in effect a separate array that is used
	 * for handling entries in the preset window.
	 */

	int		sort_index;       /* Point to another order, to allow the sorder window to be sorted. */
};

/**
 * Preset Instance data structure -- implementation.
 */

struct preset_window {
	struct file_block	*file;						/**< The file to which the window belongs.			*/

	/* Preset window handle and title details. */

	wimp_w			preset_window;					/**< Window handle of the preset window.			*/
	char			window_title[256];
	wimp_w			preset_pane;					/**< Window handle of the preset window toolbar pane.		*/

	/* Display column details. */

	int			column_width[PRESET_COLUMNS];			/**< Array holding the column widths in the transaction window.	*/
	int			column_position[PRESET_COLUMNS];		/**< Array holding the column X-offsets in the transact window.	*/

	/* Other window details. */

	enum sort_type		sort_order;					/**< The order in which the window is sorted.			*/

	char			sort_sprite[12];				/**< Space for the sort icon's indirected data.			*/
};

struct preset_complete_link
{
	char			name[PRESET_NAME_LEN];				/**< The name as it appears in the menu.				*/
	int			preset;						/**< Link to the associated preset.					*/
};


/* Preset Edit Window. */

static wimp_w			preset_edit_window = NULL;			/**< The handle of the preset edit window.				*/
static struct file_block	*preset_edit_file = NULL;			/**< The file currently owning the preset edit window.			*/
static int			preset_edit_number = -1;			/**< The preset currently being edited.					*/

/* Preset Sort Window. */

static struct sort_block	*preset_sort_dialogue = NULL;			/**< The preset window sort dialogue box.			*/

static struct sort_icon preset_sort_columns[] = {				/**< Details of the sort dialogue column icons.				*/
	{PRESET_SORT_FROM, SORT_FROM},
	{PRESET_SORT_TO, SORT_TO},
	{PRESET_SORT_AMOUNT, SORT_AMOUNT},
	{PRESET_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{PRESET_SORT_KEY, SORT_CHAR},
	{PRESET_SORT_NAME, SORT_LEFT},
	{0, SORT_NONE}
};

static struct sort_icon preset_sort_directions[] = {				/**< Details of the sort dialogue direction icons.			*/
	{PRESET_SORT_ASCENDING, SORT_ASCENDING},
	{PRESET_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/* Preset Print Window. */

static struct file_block	*preset_print_file = NULL;			/**< The file currently owning the preset print window.			*/

/* Preset List Window. */

static wimp_window		*preset_window_def = NULL;			/**< The definition for the Preset Window.				*/
static wimp_window		*preset_pane_def = NULL;			/**< The definition for the Preset Window pane.				*/
static wimp_menu		*preset_window_menu = NULL;			/**< The Preset Window menu handle.					*/
static int			preset_window_menu_line = -1;			/**< The line over which the Preset Window Menu was opened.		*/
static wimp_i			preset_substitute_sort_icon = PRESET_PANE_FROM;	/**< The icon currently obscured by the sort icon.			*/

/* Preset Shortcut Menu. */

static wimp_menu			*preset_complete_menu = NULL;		/**< The preset shortcut menu block.					*/
static struct preset_complete_link	*preset_complete_menu_link = NULL;	/**< Links for the shortcut menu.					*/
static char				*preset_complete_menu_title = NULL;	/**< The menu title buffer.						*/

/* SaveAs Dialogue Handles. */

static struct saveas_block	*preset_saveas_csv = NULL;			/**< The Save CSV saveas data handle.					*/
static struct saveas_block	*preset_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.					*/

static void		preset_delete_window(struct preset_window *windat);
static void		preset_close_window_handler(wimp_close *close);
static void		preset_window_click_handler(wimp_pointer *pointer);
static void		preset_pane_click_handler(wimp_pointer *pointer);
static void		preset_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void		preset_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void		preset_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void		preset_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void		preset_window_scroll_handler(wimp_scroll *scroll);
static void		preset_window_redraw_handler(wimp_draw *redraw);
static void		preset_adjust_window_columns(void *data, wimp_i icon, int width);
static void		preset_adjust_sort_icon(struct preset_window *windat);
static void		preset_adjust_sort_icon_data(struct preset_window *windat, wimp_icon *icon);
static void		preset_set_window_extent(struct preset_window *windat);
static void		preset_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

static void		preset_edit_click_handler(wimp_pointer *pointer);
static osbool		preset_edit_keypress_handler(wimp_key *key);
static void		preset_refresh_edit_window(void);
static void		preset_fill_edit_window(struct file_block *file, int preset);
static osbool		preset_process_edit_window(void);
static osbool		preset_delete_from_edit_window(void);

static void		preset_open_sort_window(struct preset_window *windat, wimp_pointer *ptr);
static osbool		preset_process_sort_window(enum sort_type order, void *data);

static void		preset_open_print_window(struct file_block *file, wimp_pointer *ptr, osbool restore);
static void		preset_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);

static int		preset_add(struct file_block *file);
static osbool		preset_delete(struct file_block *file, int preset);

static osbool		preset_save_csv(char *filename, osbool selection, void *data);
static osbool		preset_save_tsv(char *filename, osbool selection, void *data);
static void		preset_export_delimited(struct preset_window *windat, char *filename, enum filing_delimit_type format, int filetype);



/**
 * Initialise the preset system.
 *
 * \param *sprites		The application sprite area.
 */

void preset_initialise(osspriteop_area *sprites)
{
	wimp_w	sort_window;

	preset_edit_window = templates_create_window("EditPreset");
	ihelp_add_window(preset_edit_window, "EditPreset", NULL);
	event_add_window_mouse_event(preset_edit_window, preset_edit_click_handler);
	event_add_window_key_event(preset_edit_window, preset_edit_keypress_handler);
	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETDATE, TRUE);
	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETFROM, TRUE);
	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETTO, TRUE);
	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETREF, TRUE);
	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETAMOUNT, TRUE);
	event_add_window_icon_radio(preset_edit_window, PRESET_EDIT_CARETDESC, TRUE);

	sort_window = templates_create_window("SortPreset");
	ihelp_add_window(sort_window, "SortPreset", NULL);
	preset_sort_dialogue = sort_create_dialogue(sort_window, preset_sort_columns, preset_sort_directions,
			PRESET_SORT_OK, PRESET_SORT_CANCEL, preset_process_sort_window);

	preset_window_def = templates_load_window("Preset");
	preset_window_def->icon_count = 0;

	preset_pane_def = templates_load_window("PresetTB");
	preset_pane_def->sprite_area = sprites;

	preset_window_menu = templates_get_menu(TEMPLATES_MENU_PRESET);

	preset_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", preset_save_csv);
	preset_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", preset_save_tsv);

}


/**
 * Create a new Preset window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct preset_window *preset_create_instance(struct file_block *file)
{
	struct preset_window	*new;

	new = heap_alloc(sizeof(struct preset_window));
	if (new == NULL)
		return NULL;

	/* Initialise the preset window. */

	new->file = file;

	new->preset_window = NULL;
	new->preset_pane = NULL;

	column_init_window(new->column_width, new->column_position, PRESET_COLUMNS, 0, FALSE,
			config_str_read("PresetCols"));

	new->sort_order = SORT_CHAR | SORT_ASCENDING;

	return new;
}


/**
 * Delete a preset instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void preset_delete_instance(struct preset_window *windat)
{
	if (windat == NULL)
		return;

	preset_delete_window(windat);

	heap_free(windat);
}


/**
 * Create and open a Preset List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void preset_open_window(struct file_block *file)
{
	int			i, j, height;
	wimp_window_state	parent;
	os_error		*error;

	/* Create or re-open the window. */

	if (file->preset_window->preset_window != NULL) {
		/* The window is open, so just bring it forward. */

		windows_open(file->preset_window->preset_window);
		return;
	}

	#ifdef DEBUG
	debug_printf("\\CCreating preset window");
	#endif

	/* Create the new window data and build the window. */

	*(file->preset_window->window_title) = '\0';
	preset_window_def->title_data.indirected_text.text = file->preset_window->window_title;

	height = (file->preset_count > MIN_PRESET_ENTRIES) ? file->preset_count : MIN_PRESET_ENTRIES;

	transact_get_window_state(file, &parent);

	set_initial_window_area(preset_window_def,
			file->preset_window->column_position[PRESET_COLUMNS-1] +
			file->preset_window->column_width[PRESET_COLUMNS-1],
			((ICON_HEIGHT+LINE_GUTTER) * height) + PRESET_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	file->child_x_offset++;
	if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
		file->child_x_offset = 0;

	error = xwimp_create_window(preset_window_def, &(file->preset_window->preset_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Create the toolbar. */

	windows_place_as_toolbar(preset_window_def, preset_pane_def, PRESET_TOOLBAR_HEIGHT-4);

	#ifdef DEBUG
	debug_printf ("Window extents set...");
	#endif

	for (i=0, j=0; j < PRESET_COLUMNS; i++, j++) {
		preset_pane_def->icons[i].extent.x0 = file->preset_window->column_position[j];

		j = column_get_rightmost_in_group(PRESET_PANE_COL_MAP, i);

		preset_pane_def->icons[i].extent.x1 = file->preset_window->column_position[j] +
				file->preset_window->column_width[j] +
				COLUMN_HEADING_MARGIN;
	}

	preset_pane_def->icons[PRESET_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) file->preset_window->sort_sprite;
	preset_pane_def->icons[PRESET_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			preset_pane_def->sprite_area;

	preset_adjust_sort_icon_data(file->preset_window, &(preset_pane_def->icons[PRESET_PANE_SORT_DIR_ICON]));

	#ifdef DEBUG
	debug_printf ("Toolbar icons adjusted...");
	#endif

	error = xwimp_create_window(preset_pane_def, &(file->preset_window->preset_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Set the title */

	preset_build_window_title(file);

	/* Open the window. */

	ihelp_add_window(file->preset_window->preset_window , "Preset", preset_decode_window_help);
	ihelp_add_window(file->preset_window->preset_pane , "PresetTB", NULL);

	windows_open(file->preset_window->preset_window);
	windows_open_nested_as_toolbar(file->preset_window->preset_pane,
			file->preset_window->preset_window, PRESET_TOOLBAR_HEIGHT-4);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(file->preset_window->preset_window, file->preset_window);
	event_add_window_menu(file->preset_window->preset_window, preset_window_menu);
	event_add_window_close_event(file->preset_window->preset_window, preset_close_window_handler);
	event_add_window_mouse_event(file->preset_window->preset_window, preset_window_click_handler);
	event_add_window_scroll_event(file->preset_window->preset_window, preset_window_scroll_handler);
	event_add_window_redraw_event(file->preset_window->preset_window, preset_window_redraw_handler);
	event_add_window_menu_prepare(file->preset_window->preset_window, preset_window_menu_prepare_handler);
	event_add_window_menu_selection(file->preset_window->preset_window, preset_window_menu_selection_handler);
	event_add_window_menu_warning(file->preset_window->preset_window, preset_window_menu_warning_handler);
	event_add_window_menu_close(file->preset_window->preset_window, preset_window_menu_close_handler);

	event_add_window_user_data(file->preset_window->preset_pane, file->preset_window);
	event_add_window_menu(file->preset_window->preset_pane, preset_window_menu);
	event_add_window_mouse_event(file->preset_window->preset_pane, preset_pane_click_handler);
	event_add_window_menu_prepare(file->preset_window->preset_pane, preset_window_menu_prepare_handler);
	event_add_window_menu_selection(file->preset_window->preset_pane, preset_window_menu_selection_handler);
	event_add_window_menu_warning(file->preset_window->preset_pane, preset_window_menu_warning_handler);
	event_add_window_menu_close(file->preset_window->preset_pane, preset_window_menu_close_handler);
}


/**
 * Close and delete the Preset List Window associated with the given
 * file block.
 *
 * \param *windat			The window to delete.
 */

static void preset_delete_window(struct preset_window *windat)
{
	#ifdef DEBUG
	debug_printf ("\\RDeleting preset window");
	#endif

	if (windat == NULL)
		return;

	sort_close_dialogue(preset_sort_dialogue, windat);

	if (preset_edit_file == windat->file && windows_get_open(preset_edit_window))
		close_dialogue_with_caret(preset_edit_window);

	if (windat->preset_window != NULL) {
		ihelp_remove_window(windat->preset_window);
		event_delete_window(windat->preset_window);
		wimp_delete_window(windat->preset_window);
		windat->preset_window = NULL;
	}

	if (windat->preset_pane != NULL) {
		ihelp_remove_window(windat->preset_pane);
		event_delete_window(windat->preset_pane);
		wimp_delete_window(windat->preset_pane);
		windat->preset_pane = NULL;
	}
}


/**
 * Handle Close events on Preset List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void preset_close_window_handler(wimp_close *close)
{
	struct preset_window	*windat;

	#ifdef DEBUG
	debug_printf("\\RClosing Preset window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat == NULL)
		return;

	/* Close the window */

	preset_delete_window(windat);
}


/**
 * Process mouse clicks in the Preset List window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void preset_window_click_handler(wimp_pointer *pointer)
{
	struct preset_window	*windat;
	int			line;
	wimp_window_state	window;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	/* Find the window type and get the line clicked on. */

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - PRESET_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

	if (line < 0 || line >= windat->file->preset_count)
		line = -1;

	/* Handle double-clicks, which will open an edit preset window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1)
		preset_open_edit_window(windat->file, windat->file->presets[line].sort_index, pointer);
}


/**
 * Process mouse clicks in the Preset List pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void preset_pane_click_handler(wimp_pointer *pointer)
{
	struct preset_window	*windat;
	struct file_block	*file;
	wimp_window_state	window;
	wimp_icon_state		icon;
	int			ox;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	if (pointer->i == PRESET_PANE_SORT_DIR_ICON)
		pointer->i = preset_substitute_sort_icon;

	/* Process toolbar clicks and column heading drags. */

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case PRESET_PANE_PARENT:
			transact_bring_window_to_top(windat->file);
			break;

		case PRESET_PANE_PRINT:
			preset_open_print_window(file, pointer, config_opt_read("RememberValues"));
			break;

		case PRESET_PANE_ADDPRESET:
			preset_open_edit_window(file, NULL_PRESET, pointer);
			break;

		case PRESET_PANE_SORT:
			preset_open_sort_window(windat, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case PRESET_PANE_PRINT:
			preset_open_print_window(file, pointer, !config_opt_read("RememberValues"));
			break;

		case PRESET_PANE_SORT:
			preset_sort(file);
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
			case PRESET_PANE_KEY:
				windat->sort_order = SORT_CHAR;
				break;

			case PRESET_PANE_NAME:
				windat->sort_order = SORT_NAME;
				break;

			case PRESET_PANE_FROM:
				windat->sort_order = SORT_FROM;
				break;

			case PRESET_PANE_TO:
				windat->sort_order = SORT_TO;
				break;

			case PRESET_PANE_AMOUNT:
				windat->sort_order = SORT_AMOUNT;
				break;

			case PRESET_PANE_DESCRIPTION:
				windat->sort_order = SORT_DESCRIPTION;
				break;
			}

			if (windat->sort_order != SORT_NONE) {
				if (pointer->buttons == wimp_CLICK_SELECT * 256)
					windat->sort_order |= SORT_ASCENDING;
				else
					windat->sort_order |= SORT_DESCENDING;
			}

			preset_adjust_sort_icon(windat);
			windows_redraw(windat->preset_pane);
			preset_sort(file);
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT) {
		column_start_drag(pointer, windat, windat->preset_window,
				PRESET_PANE_COL_MAP, config_str_read("LimPresetCols"), preset_adjust_window_columns);
	}
}


/**
 * Process menu prepare events in the Preset List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void preset_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct preset_window	*windat;
	int			line;
	wimp_window_state	window;


	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	if (pointer != NULL) {
		preset_window_menu_line = -1;

		if (w == windat->preset_window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - PRESET_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

			if (line >= 0 && line < windat->file->preset_count)
				preset_window_menu_line = line;
		}

		saveas_initialise_dialogue(preset_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(preset_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_shade_entry(preset_window_menu, PRESET_MENU_EDIT, preset_window_menu_line == -1);
}


/**
 * Process menu selection events in the Preset List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void preset_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct preset_window	*windat;
	wimp_pointer	pointer;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case PRESET_MENU_SORT:
		preset_open_sort_window(windat, &pointer);
		break;

	case PRESET_MENU_EDIT:
		if (preset_window_menu_line != -1)
			preset_open_edit_window(windat->file, windat->file->presets[preset_window_menu_line].sort_index, &pointer);
		break;

	case PRESET_MENU_NEWPRESET:
		preset_open_edit_window(windat->file, NULL_PRESET, &pointer);
		break;

	case PRESET_MENU_PRINT:
		preset_open_print_window(windat->file, &pointer, config_opt_read("RememberValues"));
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

static void preset_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct preset_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->file == NULL)
		return;

	switch (warning->selection.items[0]) {
	case PRESET_MENU_EXPCSV:
		saveas_prepare_dialogue(preset_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case PRESET_MENU_EXPTSV:
		saveas_prepare_dialogue(preset_saveas_tsv);
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

static void preset_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	preset_window_menu_line = -1;
}


/**
 * Process scroll events in the Preset List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void preset_window_scroll_handler(wimp_scroll *scroll)
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

	height = (scroll->visible.y1 - scroll->visible.y0) - PRESET_TOOLBAR_HEIGHT;

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
 * Process redraw events in the Preset List window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void preset_window_redraw_handler(wimp_draw *redraw)
{
	struct preset_window	*windat;
	struct file_block	*file;
	int			ox, oy, top, base, y, i, t;
	char			icon_buffer[DESCRIPT_FIELD_LEN], rec_char[REC_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL)
		return;

	file = windat->file;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	/* Set the horizontal positions of the icons. */

	for (i=0; i < PRESET_COLUMNS; i++) {
		preset_window_def->icons[i].extent.x0 = windat->column_position[i];
		preset_window_def->icons[i].extent.x1 = windat->column_position[i] +
				windat->column_width[i];
		preset_window_def->icons[i].data.indirected_text.text = icon_buffer;
	}

	/* Perform the redraw. */

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		/* Calculate the rows to redraw. */

		top = (oy - redraw->clip.y1 - PRESET_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
		if (top < 0)
			top = 0;

		base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2) +
				oy - redraw->clip.y0 - PRESET_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			t = (y < file->preset_count) ? file->presets[y].sort_index : 0;

			/* Plot out the background with a filled white rectangle. */

			wimp_set_colour(wimp_COLOUR_WHITE);
			os_plot(os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - PRESET_TOOLBAR_HEIGHT);
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO,
					ox + windat->column_position[PRESET_COLUMNS-1] +
					windat->column_width[PRESET_COLUMNS-1],
					oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - PRESET_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

			/* Key field */

			preset_window_def->icons[PRESET_ICON_KEY].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_KEY].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;
			if (y < file->preset_count)
				sprintf(icon_buffer, "%c", file->presets[t].action_key);
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_KEY]));

			/* Name field */

			preset_window_def->icons[PRESET_ICON_NAME].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_NAME].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;
			if (y < file->preset_count) {
				preset_window_def->icons[PRESET_ICON_NAME].data.indirected_text.text = file->presets[t].name;
			} else {
				preset_window_def->icons[PRESET_ICON_NAME].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}
			wimp_plot_icon (&(preset_window_def->icons[PRESET_ICON_NAME]));

			/* From field */

			preset_window_def->icons[PRESET_ICON_FROM].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_FROM].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;

			preset_window_def->icons[PRESET_ICON_FROM_REC].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_FROM_REC].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;

			preset_window_def->icons[PRESET_ICON_FROM_NAME].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_FROM_NAME].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;


			if (y < file->preset_count && file->presets[t].from != NULL_ACCOUNT) {
				preset_window_def->icons[PRESET_ICON_FROM].data.indirected_text.text = account_get_ident(file, file->presets[t].from);
				preset_window_def->icons[PRESET_ICON_FROM_REC].data.indirected_text.text = icon_buffer;
				preset_window_def->icons[PRESET_ICON_FROM_NAME].data.indirected_text.text = account_get_name(file, file->presets[t].from);

				if (file->presets[t].flags & TRANS_REC_FROM)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else {
				preset_window_def->icons[PRESET_ICON_FROM].data.indirected_text.text = icon_buffer;
				preset_window_def->icons[PRESET_ICON_FROM_REC].data.indirected_text.text = icon_buffer;
				preset_window_def->icons[PRESET_ICON_FROM_NAME].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_FROM]));
			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_FROM_REC]));
			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_FROM_NAME]));

			/* To field */

			preset_window_def->icons[PRESET_ICON_TO].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_TO].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;

			preset_window_def->icons[PRESET_ICON_TO_REC].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_TO_REC].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;

			preset_window_def->icons[PRESET_ICON_TO_NAME].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_TO_NAME].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;

			if (y < file->preset_count && file->presets[t].to != NULL_ACCOUNT) {
				preset_window_def->icons[PRESET_ICON_TO].data.indirected_text.text = account_get_ident(file, file->presets[t].to);
				preset_window_def->icons[PRESET_ICON_TO_REC].data.indirected_text.text = icon_buffer;
				preset_window_def->icons[PRESET_ICON_TO_NAME].data.indirected_text.text = account_get_name(file, file->presets[t].to);

				if (file->presets[t].flags & TRANS_REC_TO)
					strcpy(icon_buffer, rec_char);
				else
					*icon_buffer = '\0';
			} else {
				preset_window_def->icons[PRESET_ICON_TO].data.indirected_text.text = icon_buffer;
				preset_window_def->icons[PRESET_ICON_TO_REC].data.indirected_text.text = icon_buffer;
				preset_window_def->icons[PRESET_ICON_TO_NAME].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}

			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_TO]));
			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_TO_REC]));
			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_TO_NAME]));

			/* Amount field */

			preset_window_def->icons[PRESET_ICON_AMOUNT].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_AMOUNT].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;
			if (y < file->preset_count)
				currency_convert_to_string(file->presets[t].amount, icon_buffer, DESCRIPT_FIELD_LEN);
			else
				*icon_buffer = '\0';
			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_AMOUNT]));

			/* Comments field */

			preset_window_def->icons[PRESET_ICON_DESCRIPTION].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
			preset_window_def->icons[PRESET_ICON_DESCRIPTION].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER)) -
					PRESET_TOOLBAR_HEIGHT;
			if (y < file->preset_count) {
				preset_window_def->icons[PRESET_ICON_DESCRIPTION].data.indirected_text.text = file->presets[t].description;
			} else {
				preset_window_def->icons[PRESET_ICON_DESCRIPTION].data.indirected_text.text = icon_buffer;
				*icon_buffer = '\0';
			}
			wimp_plot_icon(&(preset_window_def->icons[PRESET_ICON_DESCRIPTION]));
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

static void preset_adjust_window_columns(void *data, wimp_i group, int width)
{
	struct preset_window	*windat = (struct preset_window *) data;
	int			i, j, new_extent;
	wimp_icon_state		icon;
	wimp_window_info	window;

	if (windat == NULL || windat->file == NULL)
		return;

	update_dragged_columns(PRESET_PANE_COL_MAP, config_str_read("LimPresetCols"), group, width,
			windat->column_width, windat->column_position, PRESET_COLUMNS);

	/* Re-adjust the icons in the pane. */

	for (i=0, j=0; j < PRESET_COLUMNS; i++, j++) {
		icon.w = windat->preset_pane;
		icon.i = i;
		wimp_get_icon_state(&icon);

		icon.icon.extent.x0 = windat->column_position[j];

		j = column_get_rightmost_in_group(PRESET_PANE_COL_MAP, i);

		icon.icon.extent.x1 = windat->column_position[j] +
				windat->column_width[j] + COLUMN_HEADING_MARGIN;

		wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
				icon.icon.extent.x1, icon.icon.extent.y1);

		new_extent = windat->column_position[PRESET_COLUMNS-1] +
				windat->column_width[PRESET_COLUMNS-1];
	}

	preset_adjust_sort_icon(windat);

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

	file_set_data_integrity(windat->file, TRUE);
}


/**
 * Adjust the sort icon in a preset window, to reflect the current column
 * heading positions.
 *
 * \param *windat		The window to be updated.
 */

static void preset_adjust_sort_icon(struct preset_window *windat)
{
	wimp_icon_state		icon;

	if (windat == NULL)
		return;

	icon.w = windat->preset_pane;
	icon.i = PRESET_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	preset_adjust_sort_icon_data(windat, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current preset sort settings.
 *
 * \param *file			The preset window to be updated.
 * \param *icon			The icon to be updated.
 */

static void preset_adjust_sort_icon_data(struct preset_window *windat, wimp_icon *icon)
{
	int	i = 0, width, anchor;

	if (windat == NULL)
		return;

	if (windat->sort_order & SORT_ASCENDING)
		strcpy(windat->sort_sprite, "sortarrd");
	else if (windat->sort_order & SORT_DESCENDING)
		strcpy(windat->sort_sprite, "sortarru");

	switch (windat->sort_order & SORT_MASK) {
	case SORT_CHAR:
		i = PRESET_ICON_KEY;
		preset_substitute_sort_icon = PRESET_PANE_KEY;
		break;

	case SORT_NAME:
		i = PRESET_ICON_NAME;
		preset_substitute_sort_icon = PRESET_PANE_NAME;
		break;

	case SORT_FROM:
		i = PRESET_ICON_FROM_NAME;
		preset_substitute_sort_icon = PRESET_PANE_FROM;
		break;

	case SORT_TO:
		i = PRESET_ICON_TO_NAME;
		preset_substitute_sort_icon = PRESET_PANE_TO;
		break;

	case SORT_AMOUNT:
		i = PRESET_ICON_AMOUNT;
		preset_substitute_sort_icon = PRESET_PANE_AMOUNT;
		break;

	case SORT_DESCRIPTION:
		i = PRESET_ICON_DESCRIPTION;
		preset_substitute_sort_icon = PRESET_PANE_DESCRIPTION;
		break;
	}

	width = icon->extent.x1 - icon->extent.x0;

	if ((windat->sort_order & SORT_MASK) == SORT_AMOUNT) {
		anchor = windat->column_position[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
		icon->extent.x1 = icon->extent.x0 + width;
	} else {
		anchor = windat->column_position[i] + windat->column_width[i] + COLUMN_HEADING_MARGIN;
		icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
		icon->extent.x0 = icon->extent.x1 - width;
	}
}


/**
 * Set the extent of the preset window for the specified file.
 *
 * \param *windat		The preset window to update.
 */

static void preset_set_window_extent(struct preset_window *windat)
{
	wimp_window_state	state;
	os_box			extent;
	int			new_lines, visible_extent, new_extent, new_scroll;


	/* Set the extent. */

	if (windat == NULL || windat->file == NULL || windat->preset_window == NULL)
		return;

	/* Get the number of rows to show in the window, and work out the window extent from this. */

	new_lines = (windat->file->preset_count > MIN_PRESET_ENTRIES) ? windat->file->preset_count : MIN_PRESET_ENTRIES;

	new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_lines) - PRESET_TOOLBAR_HEIGHT;

	/* Get the current window details, and find the extent of the bottom of the visible area. */

	state.w = windat->file->preset_window->preset_window;
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
	extent.x1 = windat->column_position[PRESET_COLUMNS-1] +
			windat->column_width[PRESET_COLUMNS-1] + COLUMN_GUTTER;
	extent.y0 = new_extent;

	wimp_set_extent(windat->preset_window, &extent);
}


/**
 * Recreate the title of the Preset List window connected to the given file.
 *
 * \param *file			The file to rebuild the title for.
 */

void preset_build_window_title(struct file_block *file)
{
	char	name[256];

	if (file == NULL || file->preset_window == NULL || file->preset_window->preset_window == NULL)
		return;

	file_get_leafname(file, name, sizeof(name));

	msgs_param_lookup("PresetTitle", file->preset_window->window_title,
			sizeof(file->preset_window->window_title),
			name, NULL, NULL, NULL);

	wimp_force_redraw_title(file->preset_window->preset_window);
}


/**
 * Force a redraw of the Preset list window, for the given range of lines.
 *
 * \param *file			The file owning the window.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

void preset_force_window_redraw(struct file_block *file, int from, int to)
{
	int			y0, y1;
	wimp_window_info	window;

	if (file == NULL || file->preset_window == NULL || file->preset_window->preset_window == NULL)
		return;

	window.w = file->preset_window->preset_window;
	wimp_get_window_info_header_only(&window);

	y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - PRESET_TOOLBAR_HEIGHT;
	y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - PRESET_TOOLBAR_HEIGHT;

	wimp_force_redraw(file->preset_window->preset_window, window.extent.x0, y0, window.extent.x1, y1);
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

static void preset_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int			column, xpos;
	wimp_window_state	window;
	struct preset_window	*windat;

	*buffer = '\0';

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	for (column = 0;
			column < PRESET_COLUMNS && xpos > (windat->column_position[column] + windat->column_width[column]);
			column++);

	sprintf(buffer, "Col%d", column);
}


/**
 * Open the Preset Edit dialogue for a given preset list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void preset_open_edit_window(struct file_block *file, int preset, wimp_pointer *ptr)
{
	/* If the window is already open, another preset is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 */

	if (windows_get_open(preset_edit_window))
		wimp_close_window(preset_edit_window);

	/* Set the contents of the window up. */

	if (preset == NULL_PRESET) {
		msgs_lookup("NewPreset", windows_get_indirected_title_addr(preset_edit_window), 50);
		msgs_lookup("NewAcctAct", icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_OK), 12);
	} else {
		msgs_lookup("EditPreset", windows_get_indirected_title_addr(preset_edit_window), 50);
		msgs_lookup("EditAcctAct", icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_OK), 12);
	}

	preset_fill_edit_window(file, preset);

	/* Set the pointers up so we can find this lot again and open the window. */

	preset_edit_file = file;
	preset_edit_number = preset;

	windows_open_centred_at_pointer(preset_edit_window, ptr);
	place_dialogue_caret(preset_edit_window, PRESET_EDIT_NAME);
}


/**
 * Process mouse clicks in the Preset Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void preset_edit_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case PRESET_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(preset_edit_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			preset_refresh_edit_window();
		break;

	case PRESET_EDIT_OK:
		if (preset_process_edit_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(preset_edit_window);
		break;

	case PRESET_EDIT_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && preset_delete_from_edit_window())
			close_dialogue_with_caret(preset_edit_window);
		break;

	case PRESET_EDIT_TODAY:
		icons_set_group_shaded_when_on(preset_edit_window, PRESET_EDIT_TODAY, 1, PRESET_EDIT_DATE);
		icons_replace_caret_in_window(preset_edit_window);
		break;

	case PRESET_EDIT_CHEQUE:
		icons_set_group_shaded_when_on(preset_edit_window, PRESET_EDIT_CHEQUE, 1, PRESET_EDIT_REF);
		icons_replace_caret_in_window(preset_edit_window);
		break;

	case PRESET_EDIT_FMNAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			open_account_menu(preset_edit_file, ACCOUNT_MENU_FROM, 0,
					preset_edit_window, PRESET_EDIT_FMIDENT, PRESET_EDIT_FMNAME, PRESET_EDIT_FMREC, pointer);
		break;

	case PRESET_EDIT_TONAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			open_account_menu(preset_edit_file, ACCOUNT_MENU_TO, 0,
					preset_edit_window, PRESET_EDIT_TOIDENT, PRESET_EDIT_TONAME, PRESET_EDIT_TOREC, pointer);
		break;

	case PRESET_EDIT_FMREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			toggle_account_reconcile_icon(preset_edit_window, PRESET_EDIT_FMREC);
		break;

	case PRESET_EDIT_TOREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			toggle_account_reconcile_icon(preset_edit_window, PRESET_EDIT_TOREC);
		break;
	}
}


/**
 * Process keypresses in the Preset Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool preset_edit_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (preset_process_edit_window())
			close_dialogue_with_caret(preset_edit_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(preset_edit_window);
		break;

	default:
		if (key->i != PRESET_EDIT_FMIDENT && key->i != PRESET_EDIT_TOIDENT)
			return FALSE;

		if (key->i == PRESET_EDIT_FMIDENT)
			account_lookup_field(preset_edit_file, key->c, ACCOUNT_IN | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
					preset_edit_window, PRESET_EDIT_FMIDENT, PRESET_EDIT_FMNAME, PRESET_EDIT_FMREC);

		else if (key->i == PRESET_EDIT_TOIDENT)
			account_lookup_field(preset_edit_file, key->c, ACCOUNT_OUT | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
					preset_edit_window, PRESET_EDIT_TOIDENT, PRESET_EDIT_TONAME, PRESET_EDIT_TOREC);
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Preset Edit window.
 */

static void preset_refresh_edit_window(void)
{
	preset_fill_edit_window(preset_edit_file, preset_edit_number);
	icons_redraw_group(preset_edit_window, 12,
			PRESET_EDIT_NAME, PRESET_EDIT_KEY, PRESET_EDIT_DATE,
			PRESET_EDIT_FMIDENT, PRESET_EDIT_FMREC, PRESET_EDIT_FMNAME,
			PRESET_EDIT_TOIDENT, PRESET_EDIT_TOREC, PRESET_EDIT_TONAME,
			PRESET_EDIT_REF, PRESET_EDIT_AMOUNT, PRESET_EDIT_DESC);
	icons_replace_caret_in_window(preset_edit_window);
}

/**
 * Update the contents of the Preset Edit window to reflect the current
 * settings of the given file and preset.
 *
 * \param *file			The file to use.
 * \param preset		The preset to display, or NULL_PRESET for none.
 */

static void preset_fill_edit_window(struct file_block *file, int preset)
{
	if (preset == NULL_PRESET) {
		/* Set name and key. */

		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_NAME) = '\0';
		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_KEY) = '\0';

		/* Set date. */

		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_DATE) = '\0';
		icons_set_selected(preset_edit_window, PRESET_EDIT_TODAY, 0);
		icons_set_shaded(preset_edit_window, PRESET_EDIT_DATE, 0);

		/* Fill in the from and to fields. */

		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_FMIDENT) = '\0';
		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_FMNAME) = '\0';
		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_FMREC) = '\0';

		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_TOIDENT) = '\0';
		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_TONAME) = '\0';
		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_TOREC) = '\0';

		/* Fill in the reference field. */

		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_REF) = '\0';
		icons_set_selected(preset_edit_window, PRESET_EDIT_CHEQUE, 0);
		icons_set_shaded(preset_edit_window, PRESET_EDIT_REF, 0);

		/* Fill in the amount fields. */

		currency_convert_to_string(0, icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_AMOUNT),
				icons_get_indirected_text_length(preset_edit_window, PRESET_EDIT_AMOUNT));

		/* Fill in the description field. */

		*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_DESC) = '\0';

		/* Set the caret location icons. */

		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETDATE, 1);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETFROM, 0);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETTO, 0);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETREF, 0);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETAMOUNT, 0);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETDESC, 0);
	} else {
		/* Set name and key. */

		icons_strncpy(preset_edit_window, PRESET_EDIT_NAME, file->presets[preset].name);
		icons_printf(preset_edit_window, PRESET_EDIT_KEY, "%c", file->presets[preset].action_key);

		/* Set date. */

		date_convert_to_string(file->presets[preset].date,
				icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_DATE),
				icons_get_indirected_text_length(preset_edit_window, PRESET_EDIT_DATE));
		icons_set_selected(preset_edit_window, PRESET_EDIT_TODAY, file->presets[preset].flags & TRANS_TAKE_TODAY);
		icons_set_shaded(preset_edit_window, PRESET_EDIT_DATE, file->presets[preset].flags & TRANS_TAKE_TODAY);

		/* Fill in the from and to fields. */

		fill_account_field(file, file->presets[preset].from, file->presets[preset].flags & TRANS_REC_FROM,
				preset_edit_window, PRESET_EDIT_FMIDENT, PRESET_EDIT_FMNAME, PRESET_EDIT_FMREC);

		fill_account_field(file, file->presets[preset].to, file->presets[preset].flags & TRANS_REC_TO,
				preset_edit_window, PRESET_EDIT_TOIDENT, PRESET_EDIT_TONAME, PRESET_EDIT_TOREC);

		/* Fill in the reference field. */

		icons_strncpy(preset_edit_window, PRESET_EDIT_REF, file->presets[preset].reference);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CHEQUE, file->presets[preset].flags & TRANS_TAKE_CHEQUE);
		icons_set_shaded(preset_edit_window, PRESET_EDIT_REF, file->presets[preset].flags & TRANS_TAKE_CHEQUE);

		/* Fill in the amount fields. */

		currency_convert_to_string(file->presets[preset].amount,
				icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_AMOUNT),
				icons_get_indirected_text_length(preset_edit_window, PRESET_EDIT_AMOUNT));

		/* Fill in the description field. */

		icons_strncpy(preset_edit_window, PRESET_EDIT_DESC, file->presets[preset].description);

		/* Set the caret location icons. */

		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETDATE,
				file->presets[preset].caret_target == PRESET_CARET_DATE);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETFROM,
				file->presets[preset].caret_target == PRESET_CARET_FROM);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETTO,
				file->presets[preset].caret_target == PRESET_CARET_TO);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETREF,
				file->presets[preset].caret_target == PRESET_CARET_REFERENCE);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETAMOUNT,
				file->presets[preset].caret_target == PRESET_CARET_AMOUNT);
		icons_set_selected(preset_edit_window, PRESET_EDIT_CARETDESC,
				file->presets[preset].caret_target == PRESET_CARET_DESCRIPTION);
	}

	/* Detele the irrelevant action buttons for a new preset. */

	icons_set_deleted(preset_edit_window, PRESET_EDIT_DELETE, preset == NULL_PRESET);
}


/**
 * Take the contents of an updated Preset Edit window and process the data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool preset_process_edit_window(void)
{
	char	copyname[PRESET_NAME_LEN];
	int	check_key;

	/* Test that the preset has been given a name, and reject the data if not. */

	string_ctrl_strcpy(copyname,icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_NAME));

	if (*string_strip_surrounding_whitespace(copyname) == '\0') {
		error_msgs_report_error("NoPresetName");
		return FALSE;
	}

	/* Test that the key, if any, is unique. */

	check_key = preset_find_from_keypress(preset_edit_file, *icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_KEY));

	if (check_key != NULL_PRESET && check_key != preset_edit_number) {
		error_msgs_report_error("BadPresetNo");
		return FALSE;
	}

	/* If the preset doesn't exsit, create it.  If it does exist, validate any data that requires it. */

	if (preset_edit_number == NULL_PRESET)
		preset_edit_number = preset_add(preset_edit_file);

	/* If the preset was created OK, store the rest of the data.
	 *
	 * \TODO -- This should error before returning FALSE.
	 */

	if (preset_edit_number == NULL_PRESET)
		return FALSE;

	/* Zero the flags and reset them as required. */

	preset_edit_file->presets[preset_edit_number].flags = 0;

	/* Store the name. */

	strcpy(preset_edit_file->presets[preset_edit_number].name,
			icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_NAME));

	/* Store the key. */

	preset_edit_file->presets[preset_edit_number].action_key =
			toupper(*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_KEY));

	/* Get the date and today settings. */

	preset_edit_file->presets[preset_edit_number].date =
			date_convert_from_string(icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_DATE), NULL_DATE, 0);

	if (icons_get_selected(preset_edit_window, PRESET_EDIT_TODAY))
		preset_edit_file->presets[preset_edit_number].flags |= TRANS_TAKE_TODAY;

	/* Get the from and to fields. */

	preset_edit_file->presets[preset_edit_number].from =
			account_find_by_ident(preset_edit_file, icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_FMIDENT), ACCOUNT_FULL | ACCOUNT_IN);

	preset_edit_file->presets[preset_edit_number].to =
			account_find_by_ident(preset_edit_file, icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_TOIDENT), ACCOUNT_FULL | ACCOUNT_OUT);

	if (*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_FMREC) != '\0')
		preset_edit_file->presets[preset_edit_number].flags |= TRANS_REC_FROM;

	if (*icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_TOREC) != '\0')
		preset_edit_file->presets[preset_edit_number].flags |= TRANS_REC_TO;

	/* Get the amounts. */

	preset_edit_file->presets[preset_edit_number].amount =
		currency_convert_from_string(icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_AMOUNT));

	/* Store the reference. */

	strcpy(preset_edit_file->presets[preset_edit_number].reference, icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_REF));

	if (icons_get_selected(preset_edit_window, PRESET_EDIT_CHEQUE))
		preset_edit_file->presets[preset_edit_number].flags |= TRANS_TAKE_CHEQUE;

	/* Store the description. */

	strcpy(preset_edit_file->presets[preset_edit_number].description,
		icons_get_indirected_text_addr(preset_edit_window, PRESET_EDIT_DESC));

	/* Store the caret target. */

	preset_edit_file->presets[preset_edit_number].caret_target = PRESET_CARET_DATE;

	if (icons_get_selected(preset_edit_window, PRESET_EDIT_CARETFROM))
		preset_edit_file->presets[preset_edit_number].caret_target = PRESET_CARET_FROM;
	else if (icons_get_selected(preset_edit_window, PRESET_EDIT_CARETTO))
		preset_edit_file->presets[preset_edit_number].caret_target = PRESET_CARET_TO;
	else if (icons_get_selected(preset_edit_window, PRESET_EDIT_CARETREF))
		preset_edit_file->presets[preset_edit_number].caret_target = PRESET_CARET_REFERENCE;
	else if (icons_get_selected(preset_edit_window, PRESET_EDIT_CARETAMOUNT))
		preset_edit_file->presets[preset_edit_number].caret_target = PRESET_CARET_AMOUNT;
	else if (icons_get_selected(preset_edit_window, PRESET_EDIT_CARETDESC))
		preset_edit_file->presets[preset_edit_number].caret_target = PRESET_CARET_DESCRIPTION;

	if (config_opt_read("AutoSortPresets"))
		preset_sort(preset_edit_file);
	else
		preset_force_window_redraw(preset_edit_file, preset_edit_number, preset_edit_number);

	file_set_data_integrity(preset_edit_file, TRUE);

	return TRUE;
}


/**
 * Delete the preset associated with the currently open Preset Edit window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool preset_delete_from_edit_window(void)
{
	if (error_msgs_report_question ("DeletePreset", "DeletePresetB") == 2)
		return FALSE;

	return preset_delete(preset_edit_file, preset_edit_number);
}


/**
 * Open the Preset Sort dialogue for a given preset list window.
 *
 * \param *file			The preset window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void preset_open_sort_window(struct preset_window *windat, wimp_pointer *ptr)
{
	if (windat == NULL || ptr == NULL)
		return;

	sort_open_dialogue(preset_sort_dialogue, ptr, windat->sort_order, windat);
}


/**
 * Take the contents of an updated Preset Sort window and process
 * the data.
 *
 * \param order			The new sort order selected by the user.
 * \param *data			The preset window associated with the Sort dialogue.
 * \return			TRUE if successful; else FALSE.
 */

static osbool preset_process_sort_window(enum sort_type order, void *data)
{
	struct preset_window	*windat = (struct preset_window *) data;

	if (windat == NULL)
		return FALSE;

	windat->sort_order = order;

	preset_adjust_sort_icon(windat);
	windows_redraw(windat->preset_pane);
	preset_sort(windat->file);

	return TRUE;
}


/**
 * Open the Preset Print dialogue for a given preset list window.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to retain the previous settings; FALSE to
 *				return to defaults.
 */

static void preset_open_print_window(struct file_block *file, wimp_pointer *ptr, osbool restore)
{
	preset_print_file = file;
	printing_open_simple_window(file, ptr, restore, "PrintPreset", preset_print);
}


/**
 * Send the contents of the Preset Window to the printer, via the reporting
 * system.
 *
 * \param text			TRUE to print in text format; FALSE for graphics.
 * \param format		TRUE to apply text formatting in text mode.
 * \param scale			TRUE to scale width in graphics mode.
 * \param rotate		TRUE to print landscape in grapics mode.
 * \param pagenum		TRUE to include page numbers in graphics mode.
 */

static void preset_print(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum)
{
	struct report		*report;
	int			i, t;
	char			line[1024], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];
	struct preset_window	*window;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);
	msgs_lookup("PrintTitlePreset", buffer, sizeof(buffer));
	report = report_open(preset_print_file, buffer, NULL);

	if (report == NULL) {
		error_msgs_report_error("PrintMemFail");
		return;
	}

	hourglass_on();

	window = preset_print_file->preset_window;

	/* Output the page title. */

	file_get_leafname(preset_print_file, numbuf1, sizeof(numbuf1));
	msgs_param_lookup("PresetTitle", buffer, sizeof(buffer), numbuf1, NULL, NULL, NULL);
	sprintf(line, "\\b\\u%s", buffer);
	report_write_line(report, 0, line);
	report_write_line(report, 0, "");

	/* Output the headings line, taking the text from the window icons. */

	*line = '\0';
	sprintf(buffer, "\\k\\b\\u%s\\t", icons_copy_text(window->preset_pane, PRESET_PANE_KEY, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->preset_pane, PRESET_PANE_NAME, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(window->preset_pane, PRESET_PANE_FROM, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icons_copy_text(window->preset_pane, PRESET_PANE_TO, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u\\r%s\\t", icons_copy_text(window->preset_pane, PRESET_PANE_AMOUNT, numbuf1));
	strcat(line, buffer);
	sprintf(buffer, "\\b\\u%s\\t", icons_copy_text(window->preset_pane, PRESET_PANE_DESCRIPTION, numbuf1));
	strcat(line, buffer);

	report_write_line(report, 0, line);

	/* Output the standing order data as a set of delimited lines. */

	for (i=0; i < preset_print_file->preset_count; i++) {
		t = preset_print_file->presets[i].sort_index;

		*line = '\0';

		/* The tab after the first field is in the second, as the %c can be zero in which case the
		 * first string will end there and then.
		 */

		sprintf(buffer, "\\k%c", preset_print_file->presets[t].action_key);
		strcat(line, buffer);

		sprintf(buffer, "\\t%s\\t", preset_print_file->presets[t].name);
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", account_get_ident (preset_print_file, preset_print_file->presets[t].from));
		strcat(line, buffer);

		strcpy(numbuf1, (preset_print_file->presets[t].flags & TRANS_REC_FROM) ? rec_char : "");
		sprintf(buffer, "%s\\t", numbuf1);
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", account_get_name (preset_print_file, preset_print_file->presets[t].from));
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", account_get_ident (preset_print_file, preset_print_file->presets[t].to));
		strcat(line, buffer);

		strcpy(numbuf1, (preset_print_file->presets[t].flags & TRANS_REC_TO) ? rec_char : "");
		sprintf(buffer, "%s\\t", numbuf1);
		strcat(line, buffer);

		sprintf(buffer, "%s\\t", account_get_name (preset_print_file, preset_print_file->presets[t].to));
		strcat(line, buffer);

		currency_convert_to_string(preset_print_file->presets[t].amount, numbuf1, sizeof(numbuf1));
		sprintf(buffer, "\\r%s\\t", numbuf1);
		strcat(line, buffer);

		sprintf(buffer, "%s", preset_print_file->presets[t].description);
		strcat(line, buffer);

		report_write_line(report, 0, line);
	}

	hourglass_off();

	report_close_and_print(report, text, format, scale, rotate, pagenum);
}


/**
 * Build a Preset Complete menu and return the pointer.
 *
 * \param *file			The file to build the menu for.
 * \return			The created menu, or NULL for an error.
 */

wimp_menu *preset_complete_menu_build(struct file_block *file)
{
	int	line, width, i, p;

	/* Claim enough memory to build the menu in. */

	preset_complete_menu_destroy();

	if (file == NULL)
		return NULL;

	preset_complete_menu = heap_alloc(28 + 24 * (file->preset_count + 1));
	preset_complete_menu_link = heap_alloc(sizeof(struct preset_complete_link) * (file->preset_count + 1));
	preset_complete_menu_title = heap_alloc(ACCOUNT_MENU_TITLE_LEN);

	if (preset_complete_menu == NULL || preset_complete_menu_link == NULL || preset_complete_menu_title == NULL) {
		preset_complete_menu_destroy();
		return NULL;
	}

	/* Populate the menu. */

	line = 0;

	/* Set up the today's date field. */

	msgs_lookup("DateMenuToday", preset_complete_menu_link[line].name, PRESET_NAME_LEN);
	preset_complete_menu_link[line].preset = NULL_PRESET;

	width = strlen(preset_complete_menu_link[line].name);

	/* Set the menu and icon flags up. */

	preset_complete_menu->entries[line].menu_flags = (file->preset_count > 0) ? wimp_MENU_SEPARATE : 0;
	preset_complete_menu->entries[line].sub_menu = (wimp_menu *) -1;
	preset_complete_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
			wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
			wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

	/* Set the menu icon contents up. */

	preset_complete_menu->entries[line].data.indirected_text.text = preset_complete_menu_link[line].name;
	preset_complete_menu->entries[line].data.indirected_text.validation = NULL;
	preset_complete_menu->entries[line].data.indirected_text.size = PRESET_NAME_LEN;

	if (file->preset_count > 0) {
		for (i=0; i<file->preset_count; i++) {
			line++;

			p = file->presets[i].sort_index;

			strcpy (preset_complete_menu_link[line].name, file->presets[p].name);
			preset_complete_menu_link[line].preset = p;

			if (strlen (preset_complete_menu_link[line].name) > width)
				width = strlen (preset_complete_menu_link[line].name);

			/* Set the menu and icon flags up. */

			preset_complete_menu->entries[line].menu_flags = 0;

			preset_complete_menu->entries[line].sub_menu = (wimp_menu *) -1;
			preset_complete_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			preset_complete_menu->entries[line].data.indirected_text.text = preset_complete_menu_link[line].name;
			preset_complete_menu->entries[line].data.indirected_text.validation = NULL;
			preset_complete_menu->entries[line].data.indirected_text.size = PRESET_NAME_LEN;
		}
	}

	/* Finish off the menu, marking the last entry and filling in the header. */

	preset_complete_menu->entries[line].menu_flags |= wimp_MENU_LAST;

	msgs_lookup ("DateMenuTitle", preset_complete_menu_title, ACCOUNT_MENU_TITLE_LEN);
	preset_complete_menu->title_data.indirected_text.text = preset_complete_menu_title;
	preset_complete_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	preset_complete_menu->title_fg = wimp_COLOUR_BLACK;
	preset_complete_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	preset_complete_menu->work_fg = wimp_COLOUR_BLACK;
	preset_complete_menu->work_bg = wimp_COLOUR_WHITE;

	preset_complete_menu->width = (width + 1) * 16;
	preset_complete_menu->height = 44;
	preset_complete_menu->gap = 0;

	return preset_complete_menu;
}


/**
 * Destroy any Preset Complete menu which is currently open.
 */

void preset_complete_menu_destroy(void)
{
	if (preset_complete_menu != NULL)
		heap_free(preset_complete_menu);

	if (preset_complete_menu_link != NULL)
		heap_free(preset_complete_menu_link);

	if (preset_complete_menu_title != NULL)
		heap_free(preset_complete_menu_title);

	preset_complete_menu = NULL;
	preset_complete_menu_link = NULL;
	preset_complete_menu_title = NULL;
}


/**
 * Decode a selection from the Preset Complete menu, converting to a preset
 * index number.
 *
 * \param *selection		The Wimp Menu Selection to decode.
 * \return			The preset index, or NULL_PRESET.
 */

int preset_complete_menu_decode(wimp_selection *selection)
{
	if (preset_complete_menu_link == NULL || selection == NULL || selection->items[0] == -1)
		return NULL_PRESET;

	return preset_complete_menu_link[selection->items[0]].preset;
}


/**
 * Sort the presets in a given file based on that file's sort setting.
 *
 * \param *file			The file to sort.
 */

void preset_sort(struct file_block *file)
{
	int		gap, comb, temp, order;
	osbool		sorted, reorder;

	if (file == NULL || file->preset_window == NULL)
		return;

	#ifdef DEBUG
	debug_printf("Sorting preset window");
	#endif

	hourglass_on();

	/* Sort the entries using a combsort.  This has the advantage over qsort() that the order of entries is only
	 * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
	 */

	gap = file->preset_count - 1;

	order = file->preset_window->sort_order;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((file->preset_count >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < file->preset_count; comb++) {
			switch (order) {
			case SORT_CHAR | SORT_ASCENDING:
				reorder = (file->presets[file->presets[comb+gap].sort_index].action_key <
						file->presets[file->presets[comb].sort_index].action_key);
				break;

			case SORT_CHAR | SORT_DESCENDING:
				reorder = (file->presets[file->presets[comb+gap].sort_index].action_key >
						file->presets[file->presets[comb].sort_index].action_key);
				break;

			case SORT_NAME | SORT_ASCENDING:
				reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].name,
						file->presets[file->presets[comb].sort_index].name) < 0);
				break;

			case SORT_NAME | SORT_DESCENDING:
				reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].name,
						file->presets[file->presets[comb].sort_index].name) > 0);
				break;

			case SORT_FROM | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(file, file->presets[file->presets[comb+gap].sort_index].from),
						account_get_name(file, file->presets[file->presets[comb].sort_index].from)) < 0);
				break;

			case SORT_FROM | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(file, file->presets[file->presets[comb+gap].sort_index].from),
						account_get_name(file, file->presets[file->presets[comb].sort_index].from)) > 0);
				break;

			case SORT_TO | SORT_ASCENDING:
				reorder = (strcmp(account_get_name(file, file->presets[file->presets[comb+gap].sort_index].to),
						account_get_name(file, file->presets[file->presets[comb].sort_index].to)) < 0);
				break;

			case SORT_TO | SORT_DESCENDING:
				reorder = (strcmp(account_get_name(file, file->presets[file->presets[comb+gap].sort_index].to),
						account_get_name(file, file->presets[file->presets[comb].sort_index].to)) > 0);
				break;

			case SORT_AMOUNT | SORT_ASCENDING:
				reorder = (file->presets[file->presets[comb+gap].sort_index].amount <
						file->presets[file->presets[comb].sort_index].amount);
				break;

			case SORT_AMOUNT | SORT_DESCENDING:
				reorder = (file->presets[file->presets[comb+gap].sort_index].amount >
						file->presets[file->presets[comb].sort_index].amount);
				break;

			case SORT_DESCRIPTION | SORT_ASCENDING:
				reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].description,
						file->presets[file->presets[comb].sort_index].description) < 0);
				break;

			case SORT_DESCRIPTION | SORT_DESCENDING:
				reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].description,
						file->presets[file->presets[comb].sort_index].description) > 0);
				break;

			default:
				reorder = FALSE;
				break;
			}

			if (reorder) {
				temp = file->presets[comb+gap].sort_index;
				file->presets[comb+gap].sort_index = file->presets[comb].sort_index;
				file->presets[comb].sort_index = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

	preset_force_window_redraw(file, 0, file->preset_count - 1);

	hourglass_off();
}


/**
 * Create a new preset with null details.  Values are left to be set up later.
 *
 * \param *file			The file to add the preset to.
 * \return			The new preset index, or NULL_PRESET.
 */

static int preset_add(struct file_block *file)
{
	int	new;


	if (flex_extend((flex_ptr) &(file->presets), sizeof(struct preset) * (file->preset_count+1)) != 1) {
		error_msgs_report_error("NoMemNewPreset");
		return NULL_PRESET;
 	}

	new = file->preset_count++;

	*file->presets[new].name = '\0';
	file->presets[new].action_key = 0;

	file->presets[new].flags = 0;

	file->presets[new].date = NULL_DATE;
	file->presets[new].from = NULL_ACCOUNT;
	file->presets[new].to = NULL_ACCOUNT;
	file->presets[new].amount = NULL_CURRENCY;

	*file->presets[new].reference = '\0';
	*file->presets[new].description = '\0';

	file->presets[new].sort_index = new;

	preset_set_window_extent(file->preset_window);

	return new;
}


/**
 * Delete a preset from a file.
 *
 * \param *file			The file to act on.
 * \param preset		The preset to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

static osbool preset_delete(struct file_block *file, int preset)
{
	int	i, index;

	if (file == NULL || file->preset_window == NULL)
		return FALSE;

	/* Find the index entry for the deleted preset, and if it doesn't index itself, shuffle all the indexes along
	 * so that they remain in the correct places. */

	for (i=0; i<file->preset_count && file->presets[i].sort_index != preset; i++);

	if (file->presets[i].sort_index == preset && i != preset) {
		index = i;

		if (index > preset)
			for (i=index; i>preset; i--)
				file->presets[i].sort_index = file->presets[i-1].sort_index;
		else
			for (i=index; i<preset; i++)
				file->presets[i].sort_index = file->presets[i+1].sort_index;
	}

	/* Delete the preset */

	flex_midextend((flex_ptr) &(file->presets), (preset + 1) * sizeof(struct preset), -sizeof(struct preset));
	file->preset_count--;

	/* Adjust the sort indexes that pointe to entries above the deleted one, by reducing any indexes that are
	 * greater than the deleted entry by one.
	 */

	for (i=0; i<file->preset_count; i++)
		if (file->presets[i].sort_index > preset)
			file->presets[i].sort_index = file->presets[i].sort_index - 1;

	/* Update the main preset display window. */

	preset_set_window_extent(file->preset_window);

	if (file->preset_window->preset_window != NULL) {
		windows_open(file->preset_window->preset_window);
		if (config_opt_read("AutoSortPresets")) {
			preset_sort(file);
			preset_force_window_redraw(file, file->preset_count, file->preset_count);
		} else {
			preset_force_window_redraw(file, 0, file->preset_count);
		}
	}

	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Find a preset index based on its shortcut key.  If the key is '\0', or no
 * match is found, NULL_PRESET is returned.
 *
 * \param *file			The file to search in.
 * \param key			The shortcut key to search for.
 * \return			The matching preset index, or NULL_PRESET.
 */

int preset_find_from_keypress(struct file_block *file, char key)
{
	int	preset = NULL_PRESET;


	if (key == '\0')
		return preset;

	preset = 0;

	while ((preset < file->preset_count) && (file->presets[preset].action_key != key))
		preset++;

	if (preset == file->preset_count)
		preset = NULL_PRESET;

	return preset;
}


/**
 * Find the caret target for the given preset.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to check.
 * \return			The preset's caret target.
 */

enum preset_caret preset_get_caret_destination(struct file_block *file, int preset)
{
	if (file == NULL || preset == NULL_PRESET || preset < 0 || preset >= file->preset_count)
		return 0;

	return file->presets[preset].caret_target;

}


/**
 * Apply a preset to fields of a transaction.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to apply.
 * \param *date			The date field to be updated.
 * \param *from			The from field to be updated.
 * \param *to			The to field to be updated.
 * \param *flags		The flags field to be updated.
 * \param *amount		The amount field to be updated.
 * \param *reference		The reference field to be updated.
 * \param *description		The description field to be updated.
 * \return			Bitfield indicating which fields have changed.
 */

unsigned preset_apply(struct file_block *file, int preset, date_t *date, acct_t *from, acct_t *to, unsigned *flags, amt_t *amount, char *reference, char *description)
{
	unsigned	changed = 0;

	if (file == NULL || preset == NULL_PRESET || preset < 0 || preset >= file->preset_count)
		return changed;

	/* Update the transaction, piece by piece.
	 *
	 * Start with the date.
	 */

	if (file->presets[preset].flags & TRANS_TAKE_TODAY) {
		*date = date_today();
		changed |= (1 << EDIT_ICON_DATE);
	} else if (file->presets[preset].date != NULL_DATE && *date != file->presets[preset].date) {
		*date = file->presets[preset].date;
		changed |= (1 << EDIT_ICON_DATE);
	}

	/* Update the From account. */

	if (file->presets[preset].from != NULL_ACCOUNT) {
		*from = file->presets[preset].from;
		*flags = (*flags & ~TRANS_REC_FROM) | (file->presets[preset].flags & TRANS_REC_FROM);
		changed |= (1 << EDIT_ICON_FROM);
	}

	/* Update the To account. */

	if (file->presets[preset].to != NULL_ACCOUNT) {
		*to = file->presets[preset].to;
		*flags = (*flags & ~TRANS_REC_TO) | (file->presets[preset].flags & TRANS_REC_TO);
		changed |= (1 << EDIT_ICON_TO);
	}

	/* Update the reference. */

	if (file->presets[preset].flags & TRANS_TAKE_CHEQUE) {
		account_get_next_cheque_number(file, *from, *to, 1, reference, REF_FIELD_LEN);
		changed |= (1 << EDIT_ICON_REF);
	} else if (*(file->presets[preset].reference) != '\0' && strcmp(reference, file->presets[preset].reference) != 0) {
		strcpy(reference, file->presets[preset].reference);
		changed |= (1 << EDIT_ICON_REF);
	}

	/* Update the amount. */

	if (file->presets[preset].amount != NULL_CURRENCY && *amount != file->presets[preset].amount) {
		*amount = file->presets[preset].amount;
		changed |= (1 << EDIT_ICON_AMOUNT);
	}

	/* Update the description. */

	if (*(file->presets[preset].description) != '\0' && strcmp(description, file->presets[preset].description) != 0) {
		strcpy(description, file->presets[preset].description);
		changed |= (1 << EDIT_ICON_DESCRIPT);
	}

	return changed;
}


/**
 * Save the preset details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void preset_write_file(struct file_block *file, FILE *out)
{
	int	i;
	char	buffer[MAX_FILE_LINE_LEN];

	if (file == NULL || file->preset_window == NULL)
		return;

	fprintf(out, "\n[Presets]\n");

	fprintf(out, "Entries: %x\n", file->preset_count);

	column_write_as_text(file->preset_window->column_width, PRESET_COLUMNS, buffer);
	fprintf(out, "WinColumns: %s\n", buffer);

	fprintf(out, "SortOrder: %x\n", file->preset_window->sort_order);

	for (i = 0; i < file->preset_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x\n",
				file->presets[i].action_key, file->presets[i].caret_target,
				file->presets[i].date, file->presets[i].flags,
				file->presets[i].from, file->presets[i].to, file->presets[i].amount);
		if (*(file->presets[i].name) != '\0')
			config_write_token_pair(out, "Name", file->presets[i].name);
		if (*(file->presets[i].reference) != '\0')
			config_write_token_pair(out, "Ref", file->presets[i].reference);
		if (*(file->presets[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->presets[i].description);
	}
}


/**
 * Read preset details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

enum config_read_status preset_read_file(struct file_block *file, FILE *in, char *section, char *token, char *value, osbool *unknown_data)
{
	int	result, block_size, i = -1;

	block_size = flex_size((flex_ptr) &(file->presets)) / sizeof(struct preset);

	do {
		if (string_nocase_strcmp(token, "Entries") == 0) {
			block_size = strtoul (value, NULL, 16);
			if (block_size > file->preset_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->presets), sizeof(struct preset) * block_size);
			} else {
				block_size = file->preset_count;
			}
		} else if (string_nocase_strcmp(token, "WinColumns") == 0) {
			column_init_window(file->preset_window->column_width,
					file->preset_window->column_position,
					PRESET_COLUMNS, 0, TRUE, value);
		} else if (string_nocase_strcmp(token, "SortOrder") == 0) {
			file->preset_window->sort_order = strtoul(value, NULL, 16);
		} else if (string_nocase_strcmp(token, "@") == 0) {
			file->preset_count++;
			if (file->preset_count > block_size) {
				block_size = file->preset_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				flex_extend((flex_ptr) &(file->presets), sizeof(struct preset) * block_size);
			}
			i = file->preset_count-1;
			file->presets[i].action_key = strtoul(next_field (value, ','), NULL, 16);
			file->presets[i].caret_target = strtoul(next_field (NULL, ','), NULL, 16);
			file->presets[i].date = strtoul(next_field (NULL, ','), NULL, 16);
			file->presets[i].flags = strtoul(next_field (NULL, ','), NULL, 16);
			file->presets[i].from = strtoul(next_field (NULL, ','), NULL, 16);
			file->presets[i].to = strtoul(next_field (NULL, ','), NULL, 16);
			file->presets[i].amount = strtoul(next_field (NULL, ','), NULL, 16);
			*(file->presets[i].name) = '\0';
			*(file->presets[i].reference) = '\0';
			*(file->presets[i].description) = '\0';
			file->presets[i].sort_index = i;
		} else if (i != -1 && string_nocase_strcmp(token, "Name") == 0) {
			strcpy(file->presets[i].name, value);
		} else if (i != -1 && string_nocase_strcmp(token, "Ref") == 0) {
			strcpy(file->presets[i].reference, value);
		} else if (i != -1 && string_nocase_strcmp(token, "Desc") == 0) {
			strcpy(file->presets[i].description, value);
		} else {
			*unknown_data = TRUE;
		}

		result = config_read_token_pair(in, token, value, section);
	} while (result != sf_CONFIG_READ_EOF && result != sf_CONFIG_READ_NEW_SECTION);

	block_size = flex_size((flex_ptr) &(file->presets)) / sizeof(struct preset);

	#ifdef DEBUG
	debug_printf("Preset block size: %d, required: %d", block_size, file->preset_count);
	#endif

	if (block_size > file->preset_count) {
		block_size = file->preset_count;
		flex_extend((flex_ptr) &(file->presets), sizeof(struct preset) * block_size);

		#ifdef DEBUG
		debug_printf("Block shrunk to %d", block_size);
		#endif
	}

	return result;
}


/**
 * Callback handler for saving a CSV version of the preset data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool preset_save_csv(char *filename, osbool selection, void *data)
{
	struct preset_window *windat = data;
	
	if (windat == NULL)
		return FALSE;

	preset_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
	
	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the preset data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool preset_save_tsv(char *filename, osbool selection, void *data)
{
	struct preset_window *windat = data;
	
	if (windat == NULL)
		return FALSE;
		
	preset_export_delimited(windat, filename, DELIMIT_TAB, TSV_FILE_TYPE);
	
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

static void preset_export_delimited(struct preset_window *windat, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE			*out;
	int			i, t;
	char			buffer[256];

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	icons_copy_text(windat->preset_pane, PRESET_PANE_KEY, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->preset_pane, PRESET_PANE_NAME, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->preset_pane, PRESET_PANE_FROM, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->preset_pane, PRESET_PANE_TO, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->preset_pane, PRESET_PANE_AMOUNT, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	icons_copy_text(windat->preset_pane, PRESET_PANE_DESCRIPTION, buffer);
	filing_output_delimited_field(out, buffer, format, DELIMIT_LAST);

	/* Output the preset data as a set of delimited lines. */

	for (i = 0; i < windat->file->preset_count; i++) {
		t = windat->file->presets[i].sort_index;

		sprintf(buffer, "%c", windat->file->presets[t].action_key);
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		filing_output_delimited_field(out, windat->file->presets[t].name, format, DELIMIT_NONE);

		account_build_name_pair(windat->file, windat->file->presets[t].from, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		account_build_name_pair(windat->file, windat->file->presets[t].to, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

		currency_convert_to_string(windat->file->presets[t].amount, buffer, sizeof(buffer));
		filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

		filing_output_delimited_field(out, windat->file->presets[t].description, format, DELIMIT_LAST);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}


/**
 * Check the presets in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool preset_check_account(struct file_block *file, int account)
{
	int		i;
	osbool		found = FALSE;

	for (i = 0; i < file->preset_count && !found; i++)
		if (file->presets[i].from == account || file->presets[i].to == account)
			found = TRUE;

	return found;
}

