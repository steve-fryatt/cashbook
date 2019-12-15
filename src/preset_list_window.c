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
	"SortPreset",					/**< The sort dialogue template name.	*/
	preset_list_window_sort_columns,		/**< The sort dialogue column icons.	*/
	preset_list_window_sort_directions,		/**< The sort dialogue direction icons.	*/
	PRESET_LIST_WINDOW_SORT_OK,			/**< The sort dialogue OK icon.		*/
	PRESET_LIST_WINDOW_SORT_CANCEL,			/**< The sort dialogue Cancel icon.	*/
	"PresetTitle",					/**< Window Title token.		*/
	"Preset",					/**< Window Help token base.		*/
	"PresetTB",					/**< Window Toolbar help token base.	*/
	NULL,						/**< Window Footer help token base.	*/
	"SortPreset",					/**< Sort dialogue help token base.	*/
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
//	struct sort_block			*sort;

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
 * The Save CSV saveas data handle.
 */

static struct saveas_block		*preset_list_window_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block		*preset_list_window_saveas_tsv = NULL;

/* Static Function Prototypes. */

static void preset_list_window_delete(void *data);
static void preset_list_window_click_handler(wimp_pointer *pointer, int index, struct file_block *file, void *data);
static void preset_list_window_pane_click_handler(wimp_pointer *pointer, struct file_block *file, void *data);
static void preset_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer, int index, struct file_block *file, void *data);
static void preset_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void preset_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void preset_list_window_scroll_handler(wimp_scroll *scroll);
static void preset_list_window_redraw_handler(int index, struct file_block *file, void *data);
static void preset_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static void preset_list_window_open_print_window(struct preset_list_window *windat, wimp_pointer *ptr, osbool restore);
static struct report *preset_list_window_print(struct report *report, void *data, date_t from, date_t to);
static int preset_list_window_sort_compare(enum sort_type type, int index1, int index2, struct file_block *file);
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
	preset_list_window_definition.callback_window_close_handler = preset_list_window_delete;
	preset_list_window_definition.callback_window_click_handler = preset_list_window_click_handler;
	preset_list_window_definition.callback_pane_click_handler = preset_list_window_pane_click_handler;
	preset_list_window_definition.callback_redraw_handler = preset_list_window_redraw_handler;
	preset_list_window_definition.callback_menu_prepare_handler = preset_list_window_menu_prepare_handler;
	preset_list_window_definition.callback_sort_compare = preset_list_window_sort_compare;

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

	new->display_lines = 0;
	new->line_data = NULL;

	/* Initialise the List Window. */

	new->window = list_window_create_instance(preset_list_window_block, preset_get_file(parent), new);
	if (new->window == NULL) {
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

	list_window_delete_instance(windat->window);

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
 * TODO -- This is probably unnecessary, if dialogues are opened with
 * the list window instance handle as their reference. Should it, and
 * its associated callback, be removed?
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
}


/**
 * Process mouse clicks in the Preset List window.
 *
 * \param *pointer		The mouse event block to handle.
 * \param index			The preset under the pointer.
 * \param *file			The file owining the window.
 * \param *data			The Preset List Window instance.
 */

static void preset_list_window_click_handler(wimp_pointer *pointer, int index, struct file_block *file, void *data)
{
	if (pointer->buttons == wimp_DOUBLE_SELECT)
		preset_open_edit_window(file, index, pointer);
}


/**
 * Process mouse clicks in the Preset List pane.
 *
 * \param *pointer		The mouse event block to handle.
 * \param *file			The file owining the window.
 * \param *data			The Preset List Window instance.
 */

static void preset_list_window_pane_click_handler(wimp_pointer *pointer, struct file_block *file, void *data)
{
	struct preset_list_window *windat = data;

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
			list_window_open_sort_window(windat->window, pointer);
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
	}
}


/**
 * Process menu prepare events in the Preset List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 * \param index		The index of the entry under the menu, or LIST_WINDOW_NULL_INDEX.
 * \param *file		The file owning the preset list window.
 * \param *data		The preset list window instance.
 */

static void preset_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer, int index, struct file_block *file, void *data)
{
	struct preset_list_window	*windat = data;

	if (windat == NULL)
		return;

	if (pointer != NULL) {
		saveas_initialise_dialogue(preset_list_window_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(preset_list_window_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_shade_entry(preset_list_window_menu, PRESET_LIST_WINDOW_MENU_EDIT, index == LIST_WINDOW_NULL_INDEX);
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
		list_window_open_sort_window(windat->window, &pointer);
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

static void preset_list_window_redraw_handler(int index, struct file_block *file, void *data)
{
	acct_t			account;
	enum transact_flags	flags;
	preset_t		preset = index;

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


/**
 * Force the redraw of one or all of the presets in the given Preset list
 * bwindow.
 *
 * \param *windat		The preset window instance to redraw.
 * \param preset		The preset to redraw, or NULL_PRESET for all.
 */

void preset_list_window_redraw(struct preset_list_window *windat, preset_t preset)
{
	if (windat == NULL)
		return;

	list_window_redraw(windat->window, preset);
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

	list_window_sort(windat->window);
}


/**
 * Compare two lines of a preset list, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The index of the first line to be compared.
 * \param index2		The index of the second line to be compared.
 * \param *file			The file relating to the data being sorted.
 * \return			The comparison result.
 */

static int preset_list_window_sort_compare(enum sort_type type, int index1, int index2, struct file_block *file)
{
	switch (type) {
	case SORT_CHAR:
		return (preset_get_action_key(file, index1) -
				preset_get_action_key(file, index2));

	case SORT_NAME:
		return strcmp(preset_get_name(file, index1, NULL, 0),
				preset_get_name(file, index2, NULL, 0));

	case SORT_FROM:
		return strcmp(account_get_name(file, preset_get_from(file, index1)),
				account_get_name(file, preset_get_from(file, index2)));

	case SORT_TO:
		return strcmp(account_get_name(file, preset_get_to(file, index1)),
				account_get_name(file, preset_get_to(file, index2)));

	case SORT_AMOUNT:
		return (preset_get_amount(file, index1) -
				preset_get_amount(file, index2));

	case SORT_DESCRIPTION:
		return strcmp(preset_get_description(file, index1, NULL, 0),
				preset_get_description(file, index2, NULL, 0));

	default:
		return 0;
	}
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
	if (windat == NULL)
		return FALSE;

	return list_window_initialise_entries(windat->window, presets);
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
	if (windat == NULL)
		return FALSE;

	return list_window_add_entry(windat->window, preset, config_opt_read("AutoSortPresets"));
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
	if (windat == NULL)
		return FALSE;

	return list_window_delete_entry(windat->window, preset, config_opt_read("AutoSortPresets"));
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
	if (windat == NULL)
		return;

	list_window_write_file(windat->window, out);
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

	list_window_read_file_sortorder(windat->window, order);
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

