/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
#include "oslib/osspriteop.h"
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
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "interest.h"
#include "list_window.h"
#include "preset.h"
#include "print_dialogue.h"
#include "report.h"
#include "sorder.h"
#include "stringbuild.h"
#include "transact.h"
#include "window.h"


/* Account List Window icons. */

#define ACCOUNT_LIST_WINDOW_IDENT 0
#define ACCOUNT_LIST_WINDOW_NAME 1
#define ACCOUNT_LIST_WINDOW_STATEMENT 2
#define ACCOUNT_LIST_WINDOW_CURRENT 3
#define ACCOUNT_LIST_WINDOW_FINAL 4
#define ACCOUNT_LIST_WINDOW_BUDGET 5
#define ACCOUNT_LIST_WINDOW_HEADING 6
#define ACCOUNT_LIST_WINDOW_SECT_NAME 7
#define ACCOUNT_LIST_WINDOW_SECT_STATEMENT 8
#define ACCOUNT_LIST_WINDOW_SECT_CURRENT 9
#define ACCOUNT_LIST_WINDOW_SECT_FINAL 10
#define ACCOUNT_LIST_WINDOW_SECT_BUDGET 11

/* Account List Window Toolbar icons. */

#define ACCOUNT_LIST_WINDOW_PANE_NAME 0
#define ACCOUNT_LIST_WINDOW_PANE_STATEMENT 1
#define ACCOUNT_LIST_WINDOW_PANE_CURRENT 2
#define ACCOUNT_LIST_WINDOW_PANE_FINAL 3
#define ACCOUNT_LIST_WINDOW_PANE_BUDGET 4

#define ACCOUNT_LIST_WINDOW_PANE_PARENT 5
#define ACCOUNT_LIST_WINDOW_PANE_ADDACCT 6
#define ACCOUNT_LIST_WINDOW_PANE_ADDSECT 7
#define ACCOUNT_LIST_WINDOW_PANE_PRINT 8

/* Account List Window Footer icons. */

#define ACCOUNT_LIST_WINDOW_FOOT_NAME 0
#define ACCOUNT_LIST_WINDOW_FOOT_STATEMENT 1
#define ACCOUNT_LIST_WINDOW_FOOT_CURRENT 2
#define ACCOUNT_LIST_WINDOW_FOOT_FINAL 3
#define ACCOUNT_LIST_WINDOW_FOOT_BUDGET 4

/* Account List Window Numeric Column details. */

#define ACCOUNT_LIST_WINDOW_NUM_COLUMNS 4
#define ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT 0
#define ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT 1
#define ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL 2
#define ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET 3

/* Account List Menu entries. */

#define ACCOUNT_LIST_WINDOW_MENU_VIEWACCT 0
#define ACCOUNT_LIST_WINDOW_MENU_EDITACCT 1
#define ACCOUNT_LIST_WINDOW_MENU_EDITSECT 2
#define ACCOUNT_LIST_WINDOW_MENU_NEWACCT 3
#define ACCOUNT_LIST_WINDOW_MENU_NEWHEADER 4
#define ACCOUNT_LIST_WINDOW_MENU_EXPCSV 5
#define ACCOUNT_LIST_WINDOW_MENU_EXPTSV 6
#define ACCOUNT_LIST_WINDOW_MENU_PRINT 7

/**
 * The minimum number of entries in an Account List Window.
 */

#define ACCOUNT_LIST_WINDOW_MIN_ENTRIES 10

/**
 * The height of the Account List Window toolbar, in OS Units.
 */

#define ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT 132

/**
 * The height of the Accout List Window footer, in OS Units.
 */

#define ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT 36

/**
 * The number of alternative toolbar templates for the Account List window.
 */

#define ACCOUNT_LIST_WINDOW_PANES 2

/**
 * The template index for Account list instances.
 */

#define ACCOUNT_LIST_WINDOW_PANE_ACCOUNT 0

/**
 * The template index for Heading list instances.
 */

#define ACCOUNT_LIST_WINDOW_PANE_HEADING 1

/**
 * The number of draggable columns in the Account List window.
 */

#define ACCOUNT_LIST_WINDOW_COLUMNS 6


/* The Account List Window column map. */

static struct column_map account_list_window_columns[ACCOUNT_LIST_WINDOW_COLUMNS] = {
	{ACCOUNT_LIST_WINDOW_IDENT, wimp_ICON_WINDOW, ACCOUNT_LIST_WINDOW_PANE_NAME, ACCOUNT_LIST_WINDOW_FOOT_NAME, SORT_NONE},
	{ACCOUNT_LIST_WINDOW_NAME, ACCOUNT_LIST_WINDOW_IDENT, ACCOUNT_LIST_WINDOW_PANE_NAME, ACCOUNT_LIST_WINDOW_FOOT_NAME, SORT_NONE},
	{ACCOUNT_LIST_WINDOW_STATEMENT, wimp_ICON_WINDOW, ACCOUNT_LIST_WINDOW_PANE_STATEMENT, ACCOUNT_LIST_WINDOW_FOOT_STATEMENT, SORT_NONE},
	{ACCOUNT_LIST_WINDOW_CURRENT, wimp_ICON_WINDOW, ACCOUNT_LIST_WINDOW_PANE_CURRENT, ACCOUNT_LIST_WINDOW_FOOT_CURRENT, SORT_NONE},
	{ACCOUNT_LIST_WINDOW_FINAL, wimp_ICON_WINDOW, ACCOUNT_LIST_WINDOW_PANE_FINAL, ACCOUNT_LIST_WINDOW_FOOT_FINAL, SORT_NONE},
	{ACCOUNT_LIST_WINDOW_BUDGET, wimp_ICON_WINDOW, ACCOUNT_LIST_WINDOW_PANE_BUDGET, ACCOUNT_LIST_WINDOW_FOOT_BUDGET, SORT_NONE}
};

/* The Account List Window additional column map. */

static struct column_extra account_list_window_extra_columns[] = {
	{ACCOUNT_LIST_WINDOW_HEADING, 0, 5},
	{ACCOUNT_LIST_WINDOW_SECT_NAME, 0, 1},
	{ACCOUNT_LIST_WINDOW_SECT_STATEMENT, 2, 2},
	{ACCOUNT_LIST_WINDOW_SECT_CURRENT, 3, 3},
	{ACCOUNT_LIST_WINDOW_SECT_FINAL, 4, 4},
	{ACCOUNT_LIST_WINDOW_SECT_BUDGET, 5, 5},
	{wimp_ICON_WINDOW, 0, 0}
};

/**
 * Different ways in which an entry in an Account List Window can be
 * overdrawn.
 */

enum account_list_window_overdrawn {
	ACCOUNT_LIST_WINDOW_OVERDRAWN_NONE	= 0x00,			/**< The entry is not overdrawn.			*/
	ACCOUNT_LIST_WINDOW_OVERDRAWN_STATEMENT	= 0x01,			/**< The statement balance is overdrawn (accounts).	*/
	ACCOUNT_LIST_WINDOW_OVERDRAWN_CURRENT	= 0x02,			/**< The current balance is overdrawn (accounts).	*/
	ACCOUNT_LIST_WINDOW_OVERDRAWN_FUTURE	= 0x04,			/**< The future balance is overdrawn (accounts).	*/
	ACCOUNT_LIST_WINDOW_OVERDRAWN_BUDGET	= 0x08			/**< The entry is over budget (headings).		*/
};

/**
 * Account List Window line redraw data.
 */

struct account_list_window_redraw {
	/**
	 * The type of line (account, section heading, section footer, blank, etc).
	 */
	enum account_line_type			type;

	/**
	 * The number of any account or heading relating to the line.
	 */
	acct_t					account;

	/**
	 * The numeric values to display for the line.
	 */
	amt_t					total[ACCOUNT_LIST_WINDOW_NUM_COLUMNS];

	/**
	 * Any section heading text for the line.
	 */
	char					heading[ACCOUNT_SECTION_LEN];

	/**
	 * Flags showing the overdrawn status for the line.
	 */
	enum account_list_window_overdrawn	overdrawn;
};


/**
 * Account List Window instance data structure.
 */

struct account_list_window {
	/**
	 * The accounts instance owning the Accounts List Window.
	 */
	struct account_block			*instance;

	/**
	 * The type of accounts contained within the window.
	 */
	enum account_type			type;

	/**
	 * Wimp window handle for the main Accounts List Window.
	 */
	wimp_w					account_window;

	/**
	 * Indirected title data for the window.
	 */
	char					window_title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the Accounts List Window toolbar pane.
	 */
	wimp_w					account_pane;

	/**
	 * Wimp window handle for the Accounts List Window footer pane.
	 */
	wimp_w					account_footer;

	/**
	 * Indirected icon data for the footer pane icons.
	 */
	char					footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMNS][AMOUNT_FIELD_LEN];

	/**
	 * Instance handle for the window's column definitions.
	 */
	struct column_block			*columns;

	/**
	 * Count of the number of populated display lines in the window.
	 */
	int					display_lines;

	/**
	 * Flex array holding the line data for the window.
	 */
	struct account_list_window_redraw	*line_data;
};

/**
 * The definition for the Accounts List Window.
 */

static wimp_window			*account_list_window_def = NULL;

/**
 * The definitions for the account and heading list toolbar panes.
 */

static wimp_window			*account_list_window_pane_def[ACCOUNT_LIST_WINDOW_PANES] = {NULL, NULL};

/**
 * The defintion for the Accounts List footer pane.
 */

static wimp_window			*account_list_window_footer_def = NULL;

/**
 * The handle of the Accounts List Window menu.
 */

static wimp_menu			*account_list_window_menu = NULL;

/**
 * The window line associated with the most recent menu opening.
 */

static int				account_list_window_menu_line = -1;

/**
 *The Save CSV saveas data handle.
 */

static struct saveas_block		*account_list_window_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block		*account_list_window_saveas_tsv = NULL;


/**
 * Data relating to line dragging.
 */

struct account_list_window_drag_data {
	/**
	 * The Account List Window instance currently owning the line drag.
	 */
	struct account_list_window	*owner;

	/**
	 * The line of the window over which the drag started.
	 */
	int				start_line;

	/**
	 * TRUE if the window line drag is using a sprite.
	 */
	osbool				dragging_sprite;
};

/**
 * Instance of the line drag data, held statically to survive across Wimp_Poll.
 */

static struct account_list_window_drag_data	account_list_window_dragging_data;

/* Static Function Prototypes. */

static void account_list_window_delete(struct account_list_window *window);
static void account_list_window_close_handler(wimp_close *close);
static void account_list_window_click_handler(wimp_pointer *pointer);
static void account_list_window_pane_click_handler(wimp_pointer *pointer);
static void account_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void account_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void account_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void account_list_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static void account_list_window_scroll_handler(wimp_scroll *scroll);
static void account_list_window_redraw_handler(wimp_draw *redraw);
static void account_list_window_adjust_columns(void *data, wimp_i icon, int width);
static void account_list_window_set_extent(struct account_list_window *windat);
static void account_list_window_force_redraw(struct account_list_window *windat, int from, int to, wimp_i column);
static void account_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static void account_list_window_open_section_edit_window(struct account_list_window *window, int line, wimp_pointer *ptr);
static osbool account_list_window_process_section_edit_window(void *parent, struct account_section_dialogue_data *content);
static void account_list_window_open_print_window(struct account_list_window *window, wimp_pointer *ptr, osbool restore);
static struct report *account_list_window_print(struct report *report, void *data, date_t from, date_t to);
static int account_list_window_add_line(struct account_list_window *windat);
static void account_list_window_start_drag(struct account_list_window *windat, wimp_window_state *window, int line);
static void account_list_window_terminate_drag(wimp_dragged *drag, void *data);
static osbool account_list_window_save_csv(char *filename, osbool selection, void *data);
static osbool account_list_window_save_tsv(char *filename, osbool selection, void *data);
static void account_list_window_export_delimited(struct account_list_window *windat, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Test whether a line number is safe to look up in the redraw data array.
 */

#define account_list_window_line_valid(windat, line) (((line) >= 0) && ((line) < ((windat)->display_lines)))



/**
 * Initialise the Account List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void account_list_window_initialise(osspriteop_area *sprites)
{
	account_list_window_def = templates_load_window("Account");
	account_list_window_def->icon_count = 0;

	account_list_window_pane_def[ACCOUNT_LIST_WINDOW_PANE_ACCOUNT] = templates_load_window("AccountATB");
	account_list_window_pane_def[ACCOUNT_LIST_WINDOW_PANE_ACCOUNT]->sprite_area = sprites;

	account_list_window_pane_def[ACCOUNT_LIST_WINDOW_PANE_HEADING] = templates_load_window("AccountHTB");
	account_list_window_pane_def[ACCOUNT_LIST_WINDOW_PANE_HEADING]->sprite_area = sprites;

	account_list_window_footer_def = templates_load_window("AccountTot");

	account_list_window_menu = templates_get_menu("AccountListMenu");

	account_list_window_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", account_list_window_save_csv);
	account_list_window_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", account_list_window_save_tsv);
}


/**
 * Create a new Account List Window instance.
 *
 * \param *parent		The parent accounts instance.
 * \param type			The type of account that the instance contains.
 * \return			Pointer to the new instance, or NULL.
 */

struct account_list_window *account_list_window_create_instance(struct account_block *parent, enum account_type type)
{
	struct account_list_window	*new;

	new = heap_alloc(sizeof(struct account_list_window));
	if (new == NULL)
		return NULL;

	new->instance = parent;
	new->type = type;

	new->account_window = NULL;
	new->account_pane = NULL;
	new->account_footer = NULL;
	new->columns = NULL;

	new->display_lines = 0;
	new->line_data = NULL;

	/* Blank out the footer icons. */

	*new->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] = '\0';
	*new->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] = '\0';
	*new->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] = '\0';
	*new->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] = '\0';

	/* Initialise the window columns. */

	new->columns = column_create_instance(ACCOUNT_LIST_WINDOW_COLUMNS, account_list_window_columns, account_list_window_extra_columns, wimp_ICON_WINDOW);
	if (new->columns == NULL) {
		account_list_window_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read("LimAccountCols"));
	column_init_window(new->columns, 0, FALSE, config_str_read("AccountCols"));

	/* Set the initial lines up */

	if (!flexutils_initialise((void **) &(new->line_data))) {
		account_list_window_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy an Account List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void account_list_window_delete_instance(struct account_list_window *windat)
{
	if (windat == NULL)
		return;

	if (windat->line_data != NULL)
		flexutils_free((void **) &(windat->line_data));

	column_delete_instance(windat->columns);

	account_list_window_delete(windat);

	heap_free(windat);
}


/**
 * Create and open an Accounts List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void account_list_window_open(struct account_list_window *windat)
{
	int			tb_type, height;
	os_error		*error;
	wimp_window_state	parent;
	struct file_block	*file;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Create or re-open the window. */

	if (windat->account_window != NULL) {
		windows_open(windat->account_window);
		return;
	}

	/* Set the main window extent and create it. */

	*(windat->window_title) = '\0';
	account_list_window_def->title_data.indirected_text.text = windat->window_title;

	height =  (windat->display_lines > ACCOUNT_LIST_WINDOW_MIN_ENTRIES) ? windat->display_lines : ACCOUNT_LIST_WINDOW_MIN_ENTRIES;

	/* Find the position to open the window at. */

	list_window_get_state(file, &parent);

	window_set_initial_area(account_list_window_def, column_get_window_width(windat->columns),
			(height * WINDOW_ROW_HEIGHT) + ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT + ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT + 2,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window(account_list_window_def, &(windat->account_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		account_list_window_delete(windat);
		return;
	}

	/* Create the toolbar pane. */

	tb_type = (windat->type == ACCOUNT_FULL) ? ACCOUNT_LIST_WINDOW_PANE_ACCOUNT : ACCOUNT_LIST_WINDOW_PANE_HEADING;

	windows_place_as_toolbar(account_list_window_def, account_list_window_pane_def[tb_type], ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT-4);
	columns_place_heading_icons(windat->columns, account_list_window_pane_def[tb_type]);

	error = xwimp_create_window(account_list_window_pane_def[tb_type], &(windat->account_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		account_list_window_delete(windat);
		return;
	}

	/* Create the footer pane. */

	windows_place_as_footer(account_list_window_def, account_list_window_footer_def, ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT);
	columns_place_footer_icons(windat->columns, account_list_window_footer_def, ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT);

	account_list_window_footer_def->icons[ACCOUNT_LIST_WINDOW_FOOT_STATEMENT].data.indirected_text.text = windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT];
	account_list_window_footer_def->icons[ACCOUNT_LIST_WINDOW_FOOT_CURRENT].data.indirected_text.text = windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT];
	account_list_window_footer_def->icons[ACCOUNT_LIST_WINDOW_FOOT_FINAL].data.indirected_text.text = windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL];
	account_list_window_footer_def->icons[ACCOUNT_LIST_WINDOW_FOOT_BUDGET].data.indirected_text.text = windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET];

	error = xwimp_create_window(account_list_window_footer_def, &(windat->account_footer));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		account_list_window_delete(windat);
		return;
	}

	/* Set the title */

	account_list_window_build_title(windat);

	/* Open the window. */

	if (windat->type == ACCOUNT_FULL) {
		ihelp_add_window(windat->account_window , "AccList", account_list_window_decode_help);
		ihelp_add_window(windat->account_pane , "AccListTB", NULL);
		ihelp_add_window(windat->account_footer , "AccListFB", NULL);
	} else {
		ihelp_add_window(windat->account_window , "HeadList", account_list_window_decode_help);
		ihelp_add_window(windat->account_pane , "HeadListTB", NULL);
		ihelp_add_window(windat->account_footer , "HeadListFB", NULL);
	}

	windows_open(windat->account_window);
	windows_open_nested_as_toolbar(windat->account_pane, windat->account_window, ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT - 4, FALSE);
	windows_open_nested_as_footer(windat->account_footer, windat->account_window, ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT, FALSE);

	/* Register event handlers for the two windows. */
	/* \TODO -- Should this be all three windows?   */

	event_add_window_user_data(windat->account_window, windat);
	event_add_window_menu(windat->account_window, account_list_window_menu);
	event_add_window_close_event(windat->account_window, account_list_window_close_handler);
	event_add_window_mouse_event(windat->account_window, account_list_window_click_handler);
	event_add_window_scroll_event(windat->account_window, account_list_window_scroll_handler);
	event_add_window_redraw_event(windat->account_window, account_list_window_redraw_handler);
	event_add_window_menu_prepare(windat->account_window, account_list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->account_window, account_list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->account_window, account_list_window_menu_warning_handler);
	event_add_window_menu_close(windat->account_window, account_list_window_menu_close_handler);

	event_add_window_user_data(windat->account_pane, windat);
	event_add_window_menu(windat->account_pane, account_list_window_menu);
	event_add_window_mouse_event(windat->account_pane, account_list_window_pane_click_handler);
	event_add_window_menu_prepare(windat->account_pane, account_list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->account_pane, account_list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->account_pane, account_list_window_menu_warning_handler);
	event_add_window_menu_close(windat->account_pane, account_list_window_menu_close_handler);
}


/**
 * Close and delete an Accounts List Window associated with the given
 * instance.
 *
 * \param *windat		The window to delete.
 */

static void account_list_window_delete(struct account_list_window *windat)
{
	if (windat == NULL)
		return;

	#ifdef DEBUG
	debug_printf ("\\RDeleting accounts list window");
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

	/* Close any dialogues which belong to this window. */

	dialogue_force_all_closed(NULL, windat->instance);
	dialogue_force_all_closed(NULL, windat);
}


/**
 * Handle Close events on Accounts List windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void account_list_window_close_handler(wimp_close *close)
{
	struct account_list_window	*windat;

	#ifdef DEBUG
	debug_printf ("\\RClosing Accounts List window");
	#endif

	windat = event_get_window_user_data(close->w);
	if (windat != NULL)
		account_list_window_delete(windat);
}


/**
 * Process mouse clicks in an Accounts List Window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_list_window_click_handler(wimp_pointer *pointer)
{
	struct account_list_window	*windat;
	struct file_block		*file;
	int				ypos, line;
	wimp_window_state		window;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	window.w = pointer->w;
	wimp_get_window_state(&window);

	ypos = (pointer->pos.y - window.visible.y1) + window.yscroll;

	line = window_calculate_click_row(ypos, ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, windat->display_lines);

	/* Handle double-clicks, which will open a statement view or an edit accout window. */

	if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1) {
		if (windat->line_data[line].type == ACCOUNT_LINE_DATA)
			accview_open_window(file, windat->line_data[line].account);
	} else if (pointer->buttons == wimp_DOUBLE_ADJUST && line != -1) {
		switch (windat->line_data[line].type) {
		case ACCOUNT_LINE_DATA:
			account_open_edit_window(file, windat->line_data[line].account, ACCOUNT_NULL, pointer);
			break;

		case ACCOUNT_LINE_HEADER:
		case ACCOUNT_LINE_FOOTER:
			account_list_window_open_section_edit_window(windat, line, pointer);
			break;
		default:
			break;
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && line != -1) {
		account_list_window_start_drag(windat, &window, line);
	}
}


/**
 * Process mouse clicks in an Accounts List Window pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_list_window_pane_click_handler(wimp_pointer *pointer)
{
	struct account_list_window	*windat;
	struct file_block		*file;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case ACCOUNT_LIST_WINDOW_PANE_PARENT:
			transact_bring_window_to_top(file);
			break;

		case ACCOUNT_LIST_WINDOW_PANE_PRINT:
			account_list_window_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case ACCOUNT_LIST_WINDOW_PANE_ADDACCT:
			account_open_edit_window(file, NULL_ACCOUNT, windat->type, pointer);
			break;

		case ACCOUNT_LIST_WINDOW_PANE_ADDSECT:
			account_list_window_open_section_edit_window(windat, -1, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case ACCOUNT_LIST_WINDOW_PANE_PRINT:
			account_list_window_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimAccountCols"));
		column_start_drag(windat->columns, pointer, windat, windat->account_window, account_list_window_adjust_columns);
	}
}


/**
 * Process menu prepare events in an Accounts List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void account_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct account_list_window	*windat;
	int				ypos, line;
	wimp_window_state		window;
	enum account_line_type		data;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (pointer != NULL) {
		account_list_window_menu_line = -1;

		if (w == windat->account_window) {
			window.w = w;
			wimp_get_window_state(&window);

			ypos = (pointer->pos.y - window.visible.y1) + window.yscroll;
			line = window_calculate_click_row(ypos, ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, windat->display_lines);

			if (line != -1)
				account_list_window_menu_line = line;
		}

		data = (account_list_window_menu_line == -1) ? ACCOUNT_LINE_BLANK : windat->line_data[account_list_window_menu_line].type;

		saveas_initialise_dialogue(account_list_window_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(account_list_window_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);

		switch (windat->type) {
		case ACCOUNT_FULL:
			msgs_lookup("AcclistMenuTitleAcc", account_list_window_menu->title_data.text, 12);
			msgs_lookup("AcclistMenuViewAcc", menus_get_indirected_text_addr(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_VIEWACCT), 20);
			msgs_lookup("AcclistMenuEditAcc", menus_get_indirected_text_addr(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_EDITACCT), 20);
			msgs_lookup("AcclistMenuNewAcc", menus_get_indirected_text_addr(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_NEWACCT), 20);
			ihelp_add_menu(account_list_window_menu, "AccListMenu");
			break;

		case ACCOUNT_IN:
		case ACCOUNT_OUT:
			msgs_lookup("AcclistMenuTitleHead", account_list_window_menu->title_data.text, 12);
			msgs_lookup("AcclistMenuViewHead", menus_get_indirected_text_addr(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_VIEWACCT), 20);
			msgs_lookup("AcclistMenuEditHead", menus_get_indirected_text_addr(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_EDITACCT), 20);
			msgs_lookup("AcclistMenuNewHead", menus_get_indirected_text_addr(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_NEWACCT), 20);
			ihelp_add_menu(account_list_window_menu, "HeadListMenu");
			break;
		default:
			break;
		}
	} else {
		data = (account_list_window_menu_line == -1) ? ACCOUNT_LINE_BLANK : windat->line_data[account_list_window_menu_line].type;
	}

	menus_shade_entry(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_VIEWACCT, account_list_window_menu_line == -1 || data != ACCOUNT_LINE_DATA);
	menus_shade_entry(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_EDITACCT, account_list_window_menu_line == -1 || data != ACCOUNT_LINE_DATA);
	menus_shade_entry(account_list_window_menu, ACCOUNT_LIST_WINDOW_MENU_EDITSECT, account_list_window_menu_line == -1 || (data != ACCOUNT_LINE_HEADER && data != ACCOUNT_LINE_FOOTER));

	account_list_window_force_redraw(windat, account_list_window_menu_line, account_list_window_menu_line, wimp_ICON_WINDOW);
}


/**
 * Process menu selection events in an Accounts List Window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void account_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct account_list_window	*windat;
	struct file_block		*file;
	wimp_pointer			pointer;

	windat = event_get_window_user_data(w);
	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case ACCOUNT_LIST_WINDOW_MENU_VIEWACCT:
		accview_open_window(file, windat->line_data[account_list_window_menu_line].account);
		break;

	case ACCOUNT_LIST_WINDOW_MENU_EDITACCT:
		account_open_edit_window(file, windat->line_data[account_list_window_menu_line].account, ACCOUNT_NULL, &pointer);
		break;

	case ACCOUNT_LIST_WINDOW_MENU_EDITSECT:
		account_list_window_open_section_edit_window(windat, account_list_window_menu_line, &pointer);
		break;

	case ACCOUNT_LIST_WINDOW_MENU_NEWACCT:
		account_open_edit_window(file, -1, windat->type, &pointer);
		break;

	case ACCOUNT_LIST_WINDOW_MENU_NEWHEADER:
		account_list_window_open_section_edit_window(windat, -1, &pointer);
		break;

	case ACCOUNT_LIST_WINDOW_MENU_PRINT:
		account_list_window_open_print_window(windat, &pointer, config_opt_read("RememberValues"));
		break;
 	}
}


/**
 * Process submenu warning events in an Accounts List Window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void account_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct account_list_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	switch (warning->selection.items[0]) {
	case ACCOUNT_LIST_WINDOW_MENU_EXPCSV:
		saveas_prepare_dialogue(account_list_window_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case ACCOUNT_LIST_WINDOW_MENU_EXPTSV:
		saveas_prepare_dialogue(account_list_window_saveas_tsv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process menu close events in an Accounts List Window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void account_list_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	struct account_list_window	*windat;

	windat = event_get_window_user_data(w);
	if (windat != NULL)
		account_list_window_force_redraw(windat, account_list_window_menu_line, account_list_window_menu_line, wimp_ICON_WINDOW);

	account_list_window_menu_line = -1;
	ihelp_remove_menu(account_list_window_menu);
}


/**
 * Process scroll events in an Accounts List Window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void account_list_window_scroll_handler(wimp_scroll *scroll)
{
	window_process_scroll_event(scroll, ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT + ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT);

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

	wimp_open_window((wimp_open *) scroll);
}


/**
 * Process redraw events in an Account View Window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void account_list_window_redraw_handler(wimp_draw *redraw)
{
	int				top, base, y, select, shade_overdrawn_col, icon_fg_col;
	char				icon_buffer[AMOUNT_FIELD_LEN];
	osbool				more, shade_overdrawn;
	struct file_block		*file;
	struct account_list_window	*windat;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->columns == NULL)
		return;

	file = account_get_file(windat->instance);

	shade_overdrawn = config_opt_read("ShadeAccounts");
	shade_overdrawn_col = config_int_read("ShadeAccountsColour");

	/* Identify if there is a selected line to highlight. */

	if (redraw->w == event_get_current_menu_window())
		select = account_list_window_menu_line;
	else
		select = -1;

	/* Set the horizontal positions of the icons for the account lines. */

	columns_place_table_icons_horizontally(windat->columns, account_list_window_def, icon_buffer, AMOUNT_FIELD_LEN);

	window_set_icon_templates(account_list_window_def);

	/* Perform the redraw. */

	more = wimp_redraw_window(redraw);

	while (more) {
		window_plot_background(redraw, ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, wimp_COLOUR_WHITE, select, &top, &base);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(windat->columns, account_list_window_def,
					WINDOW_ROW_Y0(ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, y), WINDOW_ROW_Y1(ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= windat->display_lines) {
				columns_plot_empty_table_icons(windat->columns);
				continue;
			}

			switch (windat->line_data[y].type) {
			case ACCOUNT_LINE_DATA:
				window_plot_text_field(ACCOUNT_LIST_WINDOW_IDENT, account_get_ident(file, windat->line_data[y].account), wimp_COLOUR_BLACK);
				window_plot_text_field(ACCOUNT_LIST_WINDOW_NAME, account_get_name(file, windat->line_data[y].account), wimp_COLOUR_BLACK);

				/* Statement Column. */

				if (shade_overdrawn && (windat->line_data[y].overdrawn & ACCOUNT_LIST_WINDOW_OVERDRAWN_STATEMENT))
					icon_fg_col = shade_overdrawn_col;
				else
					icon_fg_col = wimp_COLOUR_BLACK;

				window_plot_currency_field(ACCOUNT_LIST_WINDOW_STATEMENT, windat->line_data[y].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT], icon_fg_col);

				/* Current Column. */

				if (shade_overdrawn && (windat->line_data[y].overdrawn & ACCOUNT_LIST_WINDOW_OVERDRAWN_CURRENT))
					icon_fg_col = shade_overdrawn_col;
				else
					icon_fg_col = wimp_COLOUR_BLACK;

				window_plot_currency_field(ACCOUNT_LIST_WINDOW_CURRENT, windat->line_data[y].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT], icon_fg_col);

				/* Future Column. */

				if (shade_overdrawn && ((windat->line_data[y].overdrawn & ACCOUNT_LIST_WINDOW_OVERDRAWN_FUTURE) ||
						(windat->line_data[y].overdrawn & ACCOUNT_LIST_WINDOW_OVERDRAWN_BUDGET)))
					icon_fg_col = shade_overdrawn_col;
				else
					icon_fg_col = wimp_COLOUR_BLACK;

				window_plot_currency_field(ACCOUNT_LIST_WINDOW_FINAL, windat->line_data[y].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL], icon_fg_col);

				/* Budget Column. */

				if (shade_overdrawn && (windat->line_data[y].overdrawn & ACCOUNT_LIST_WINDOW_OVERDRAWN_BUDGET))
					icon_fg_col = shade_overdrawn_col;
				else
					icon_fg_col = wimp_COLOUR_BLACK;

				window_plot_currency_field(ACCOUNT_LIST_WINDOW_BUDGET, windat->line_data[y].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET], icon_fg_col);
				break;

			case ACCOUNT_LINE_HEADER:

				/* Block header line */

				window_plot_text_field(ACCOUNT_LIST_WINDOW_HEADING, windat->line_data[y].heading, wimp_COLOUR_WHITE);
				break;

			case ACCOUNT_LINE_FOOTER:

				/* Block footer line */

				window_plot_text_field(ACCOUNT_LIST_WINDOW_SECT_NAME, windat->line_data[y].heading, wimp_COLOUR_BLACK);

				window_plot_currency_field(ACCOUNT_LIST_WINDOW_SECT_STATEMENT, windat->line_data[y].total[0], wimp_COLOUR_BLACK);
				window_plot_currency_field(ACCOUNT_LIST_WINDOW_SECT_CURRENT, windat->line_data[y].total[1], wimp_COLOUR_BLACK);
				window_plot_currency_field(ACCOUNT_LIST_WINDOW_SECT_FINAL, windat->line_data[y].total[2], wimp_COLOUR_BLACK);
				window_plot_currency_field(ACCOUNT_LIST_WINDOW_SECT_BUDGET, windat->line_data[y].total[3], wimp_COLOUR_BLACK);
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

static void account_list_window_adjust_columns(void *data, wimp_i icon, int width)
{
	struct account_list_window	*windat = (struct account_list_window *) data;
	struct file_block		*file;
	int				new_extent;
	wimp_window_info		window;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
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

	file_set_data_integrity(file, TRUE);
}


/**
 * Add an account to the end of an Account List Window.
 *
 * \param *windat		The Account List Window instance to add
 *				the account to.
 * \param account		The account to add.
 */

void account_list_window_add_account(struct account_list_window *windat, acct_t account)
{
	int line;

	if (windat == NULL)
		return;

	line = account_list_window_add_line(windat);

	if (line == -1) {
		error_msgs_report_error("NoMemLinkAcct");
		return;
	}

	windat->line_data[line].type = ACCOUNT_LINE_DATA;
	windat->line_data[line].account = account;

	/* If the target window is open, change the extent as necessary. */

	account_list_window_set_extent(windat);
}


/**
 * Remove an account from an Account List Window instance.
 *
 * \param *windat		The Account List Window instance.
 * \param account		The account to remove.
 */

void account_list_window_remove_account(struct account_list_window *windat, acct_t account)
{
	int line;

	if (windat == NULL)
		return;

	for (line = windat->display_lines - 1; line >= 0; line--) {
		if (windat->line_data[line].type == ACCOUNT_LINE_DATA && windat->line_data[line].account == account) {
			#ifdef DEBUG
			debug_printf("Deleting entry type %x", windat->line_data[line].type);
			#endif

			if (!flexutils_delete_object((void **) &(windat->line_data), sizeof(struct account_list_window_redraw), line)) {
				error_msgs_report_error("BadDelete");
				continue;
			}

			windat->display_lines--;
			line--; /* Take into account that the array has just shortened. */
		}
	}

	account_list_window_set_extent(windat);

	if (windat->account_window != NULL) {
		windows_open(windat->account_window);
		account_list_window_force_redraw(windat, 0, windat->display_lines - 1, wimp_ICON_WINDOW);
	}
}


/**
 * Set the extent of an Accounts List Window for the specified instance.
 *
 * \param *windat		The window to be updated.
 */

static void account_list_window_set_extent(struct account_list_window *windat)
{
	int	lines;

	/* Set the extent. */

	if (windat == NULL || windat->account_window == NULL)
		return;

	lines = (windat->display_lines > ACCOUNT_LIST_WINDOW_MIN_ENTRIES) ? windat->display_lines : ACCOUNT_LIST_WINDOW_MIN_ENTRIES;

	window_set_extent(windat->account_window, lines, ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT + ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT + 2,
			column_get_window_width(windat->columns));
}


/**
 * Recreate the title of an Account List Window.
 *
 * \param *windat		The window instance to update
 */

void account_list_window_build_title(struct account_list_window *windat)
{
	struct file_block	*file;
	char			*token, name[WINDOW_TITLE_LENGTH];

	if (windat == NULL || windat->instance == NULL || windat->account_window == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	file_get_leafname(file, name, WINDOW_TITLE_LENGTH);

	switch (windat->type) {
	case ACCOUNT_FULL:
		token = "AcclistTitleAcc";
		break;

	case ACCOUNT_IN:
		token = "AcclistTitleHIn";
		break;

	case ACCOUNT_OUT:
		token = "AcclistTitleHOut";
		break;

	default:
		token = NULL;
		break;
	}

	if (token == NULL)
		return;

	msgs_param_lookup(token, windat->window_title, WINDOW_TITLE_LENGTH, name, NULL, NULL, NULL);
	wimp_force_redraw_title(windat->account_window);
}


/**
 * Force the complete redraw of an Account List Window.
 *
 * \param *windat		The window instance to redraw.
 */

void account_list_window_redraw_all(struct account_list_window *windat)
{
	if (windat == NULL)
		return;

	account_list_window_force_redraw(windat, 0, windat->display_lines - 1, wimp_ICON_WINDOW);
}


/**
 * Force a redraw of the Account List window, for the given range of
 * lines.
 *
 * \param *windat		The Account List Window instance to redraw.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void account_list_window_force_redraw(struct account_list_window *windat, int from, int to, wimp_i column)
{
	wimp_window_info	window;

	if (windat == NULL)
		return;

	window.w = windat->account_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(windat->columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, from);
	window.extent.y0 = WINDOW_ROW_BASE(ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(windat->account_window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);

	/* Force a redraw of the three total icons in the footer. */

	icons_redraw_group(windat->account_footer, 4, ACCOUNT_LIST_WINDOW_FOOT_STATEMENT, ACCOUNT_LIST_WINDOW_FOOT_CURRENT,
			ACCOUNT_LIST_WINDOW_FOOT_FINAL, ACCOUNT_LIST_WINDOW_FOOT_BUDGET);
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

static void account_list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int				xpos;
	wimp_i				icon;
	wimp_window_state		window;
	struct account_list_window		*windat;

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
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *window		The account window to own the dialogue.
 * \param line			The line to be editied, or -1 for none.
 * \param *ptr			The current Wimp pointer position.
 */

static void account_list_window_open_section_edit_window(struct account_list_window *window, int line, wimp_pointer *ptr)
{
	struct account_section_dialogue_data	*content = NULL;
	struct file_block			*file = NULL;

	if (window == NULL)
		return;

	file = account_get_file(window->instance);
	if (file == NULL)
		return;

	/* Close any other edit dialogues relating to this account list window. */

	dialogue_force_group_closed(DIALOGUE_GROUP_EDIT);

	/* Open the dialogue box. */

	content = heap_alloc(sizeof(struct account_section_dialogue_data));
	if (content == NULL)
		return;

	content->action = ACCOUNT_SECTION_DIALOGUE_ACTION_NONE;
	content->line = line;

	if (line == -1) {
		*(content->name) = '\0';
		content->type = ACCOUNT_LINE_HEADER;
	} else {
		string_copy(content->name, window->line_data[line].heading, ACCOUNT_SECTION_LEN);
		content->type = window->line_data[line].type;
	}

	account_section_dialogue_open(ptr, window, file,
			account_list_window_process_section_edit_window, content);
}


/**
 * Process data associated with the currently open Section Edit window.
 *
 * \param *parent		The Account List window holding the section.
 * \param *content		The content of the dialogue box.
 * \return			TRUE if processed; else FALSE.
 */

static osbool account_list_window_process_section_edit_window(void *parent, struct account_section_dialogue_data *content)
{
	struct account_list_window	*windat = parent;
	struct file_block		*file = NULL;
	int				redraw_to = 0;

	if (content == NULL || windat == NULL || windat->instance == NULL)
		return FALSE;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return FALSE;

	switch (content->action) {
	case ACCOUNT_SECTION_DIALOGUE_ACTION_OK:
		/* If the section doesn't exsit, create space for it. */

		if (content->line == -1) {
			content->line = account_list_window_add_line(windat);

			if (content->line == -1) {
				error_msgs_report_error("NoMemNewSect");
				return FALSE;
			}
		}

		/* Update the line details. */

		string_copy(windat->line_data[content->line].heading, content->name, ACCOUNT_SECTION_LEN);
		windat->line_data[content->line].type = content->type;

		/* Set the redraw range. */

		redraw_to = content->line;
		break;

	case ACCOUNT_SECTION_DIALOGUE_ACTION_DELETE:
		if (content->line == -1)
			return FALSE;

		/* Check that the user really wishes to proceed. */

		if (error_msgs_report_question("DeleteSection", "DeleteSectionB") == 4)
			return FALSE;

		/* Delete the heading */

		if (!flexutils_delete_object((void **) &(windat->line_data), sizeof(struct account_list_window_redraw), content->line)) {
			error_msgs_report_error("BadDelete");
			return FALSE;
		}

		windat->display_lines--;

		/* Set the redraw range. */

		redraw_to = windat->display_lines - 1;
		break;

	case ACCOUNT_SECTION_DIALOGUE_ACTION_NONE:
		return FALSE;
	}

	/* Tidy up and redraw the windows */

	account_recalculate_all(file);
	account_list_window_set_extent(windat);
	windows_open(windat->account_window);
	account_list_window_force_redraw(windat, content->line, redraw_to, wimp_ICON_WINDOW);
	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Open the Account Print dialogue for a given account list window.
 *
 * \param *windat		The account list window to be printed.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to retain the previous settings; FALSE to
 *				return to defaults.
 */

static void account_list_window_open_print_window(struct account_list_window *windat, wimp_pointer *ptr, osbool restore)
{
	struct file_block *file;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Open the print dialogue box. */

	if (windat->type & ACCOUNT_FULL) {
		print_dialogue_open(file->print, ptr, FALSE, restore, "PrintAcclistAcc", "PrintTitleAcclistAcc",
				windat, account_list_window_print, windat);
	} else if (windat->type & ACCOUNT_IN || windat->type & ACCOUNT_OUT) {
		print_dialogue_open(file->print, ptr, FALSE, restore, "PrintAcclistHead", "PrintTitleAcclistHead",
				windat, account_list_window_print, windat);
	}
}


/**
 * Send the contents of the Account Window to the printer, via the reporting
 * system.
 *
 * \param *report		The report handle to use for output.
 * \param *data			The account window structure to be printed.
 * \param from			The date to print from.
 * \param to			The date to print to.
 * \return			Pointer to the report, or NULL on failure.
 */

static struct report *account_list_window_print(struct report *report, void *data, date_t from, date_t to)
{
	struct account_list_window	*windat = data;
	struct file_block		*file;
	int				line, column;
	char				*filename, date_buffer[DATE_FIELD_LEN];
	date_t				start, finish;
	wimp_i				columns[ACCOUNT_LIST_WINDOW_COLUMNS];

	if (report == NULL || windat->instance == NULL)
		return NULL;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return NULL;

	if (!column_get_icons(windat->columns, columns, ACCOUNT_LIST_WINDOW_COLUMNS, FALSE))
		return NULL;

	hourglass_on();

	/* Output the page title. */

	stringbuild_reset();
	stringbuild_add_string("\\b\\u");

	filename = file_get_leafname(file, NULL, 0);

	switch (windat->type) {
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

	budget_get_dates(file, &start, &finish);

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
	columns_print_heading_names(windat->columns, windat->account_pane);
	stringbuild_report_line(report, 0);

	/* Output the account data as a set of delimited lines. */

	for (line = 0; line < windat->display_lines; line++) {
		stringbuild_reset();

		for (column = 0; column < ACCOUNT_LIST_WINDOW_COLUMNS; column++) {
			if (column == 0)
				stringbuild_add_string("\\k\\v");
			else
				stringbuild_add_string("\\t\\v");

			switch (windat->line_data[line].type) {
			case ACCOUNT_LINE_DATA:
				switch (columns[column]) {
				case ACCOUNT_LIST_WINDOW_IDENT:
					stringbuild_add_string(account_get_ident(file, windat->line_data[line].account));
					break;
				case ACCOUNT_LIST_WINDOW_NAME:
					stringbuild_add_string(account_get_name(file, windat->line_data[line].account));
					break;
				case ACCOUNT_LIST_WINDOW_STATEMENT:
					stringbuild_add_string("\\r\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT], FALSE);
					break;
				case ACCOUNT_LIST_WINDOW_CURRENT:
					stringbuild_add_string("\\r\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT], FALSE);
					break;
				case ACCOUNT_LIST_WINDOW_FINAL:
					stringbuild_add_string("\\r\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL], FALSE);
					break;
				case ACCOUNT_LIST_WINDOW_BUDGET:
					stringbuild_add_string("\\r\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET], FALSE);
					break;
				default:
					stringbuild_add_string("\\s");
					break;
				}
				break;

			case ACCOUNT_LINE_FOOTER:
				switch (columns[column]) {
				case ACCOUNT_LIST_WINDOW_IDENT:
					stringbuild_add_string(windat->line_data[line].heading);
					break;
				case ACCOUNT_LIST_WINDOW_STATEMENT:
					stringbuild_add_string("\\r\\b\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT], FALSE);
					break;
				case ACCOUNT_LIST_WINDOW_CURRENT:
					stringbuild_add_string("\\r\\b\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT], FALSE);
					break;
				case ACCOUNT_LIST_WINDOW_FINAL:
					stringbuild_add_string("\\r\\b\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL], FALSE);
					break;
				case ACCOUNT_LIST_WINDOW_BUDGET:
					stringbuild_add_string("\\r\\b\\d");
					stringbuild_add_currency(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET], FALSE);
					break;
				default:
					stringbuild_add_string("\\s");
					break;
				}
				break;

			case ACCOUNT_LINE_HEADER:
				switch (columns[column]) {
				case ACCOUNT_LIST_WINDOW_IDENT:
					stringbuild_add_printf("\\u%s", windat->line_data[line].heading);
					break;
				default:
					stringbuild_add_string("\\s");
					break;
				}
				break;

			default:
				stringbuild_add_string("\\s");
				break;
			}
		}

		stringbuild_report_line(report, 0);
	}

	/* Output the grand total line, taking the text from the window icons. */

	stringbuild_reset();

	for (column = 0; column < ACCOUNT_LIST_WINDOW_COLUMNS; column++) {
		if (column == 0)
			stringbuild_add_string("\\k\\v");
		else
			stringbuild_add_string("\\t\\v");

		switch (columns[column]) {
		case ACCOUNT_LIST_WINDOW_IDENT:
			stringbuild_add_string("\\b\\u");
			stringbuild_add_icon(windat->account_footer, ACCOUNT_LIST_WINDOW_FOOT_NAME);
			break;
		case ACCOUNT_LIST_WINDOW_STATEMENT:
			stringbuild_add_string("\\r\\b\\u");
			stringbuild_add_string(windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT]);
			break;
		case ACCOUNT_LIST_WINDOW_CURRENT:
			stringbuild_add_string("\\r\\b\\u");
			stringbuild_add_string(windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT]);
			break;
		case ACCOUNT_LIST_WINDOW_FINAL:
			stringbuild_add_string("\\r\\b\\u");
			stringbuild_add_string(windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL]);
			break;
		case ACCOUNT_LIST_WINDOW_BUDGET:
			stringbuild_add_string("\\r\\b\\u");
			stringbuild_add_string(windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET]);
			break;
		default:
			stringbuild_add_string("\\s");
			break;
		}
	}

	stringbuild_report_line(report, 0);

	hourglass_off();

	return report;
}


/**
 * Create a new display line block at the end of the given Account List
 * Window instance, fill it with blank data and return the number.
 *
 * \param *windat		The Account List Window instance to update.
 * \return			The new line, or -1 on failure.
 */

static int account_list_window_add_line(struct account_list_window *windat)
{
	int	line;

	if (windat == NULL)
		return -1;

	if (!flexutils_resize((void **) &(windat->line_data), sizeof(struct account_list_window_redraw), windat->display_lines + 1))
		return -1;

	line = windat->display_lines++;

	#ifdef DEBUG
	debug_printf("Creating new display line %d", line);
	#endif

	windat->line_data[line].type = ACCOUNT_LINE_BLANK;
	windat->line_data[line].account = NULL_ACCOUNT;

	windat->line_data[line].heading[0] = '\0';

	return line;
}


/**
 * Find the number of entries in the given Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \return			The number of entries, or 0.
 */

int account_list_window_get_length(struct account_list_window *windat)
{
	if (windat == NULL)
		return 0;

	return windat->display_lines;
}


/**
 * Return the type of a given line in an Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \param line			The line to return the details for.
 * \return			The type of data on that line.
 */

enum account_line_type account_list_window_get_entry_type(struct account_list_window *windat, int line)
{
	if (windat == NULL || !account_list_window_line_valid(windat, line))
		return ACCOUNT_LINE_BLANK;

	return windat->line_data[line].type;
}


/**
 * Return the account on a given line of an Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \param line			The line to return the details for.
 * \return			The account on that line, or NULL_ACCOUNT if the
 *				line isn't an account.
 */

acct_t account_list_window_get_entry_account(struct account_list_window *windat, int line)
{
	if (windat == NULL || !account_list_window_line_valid(windat, line))
		return ACCOUNT_LINE_BLANK;

	if (windat->line_data[line].type != ACCOUNT_LINE_DATA)
		return NULL_ACCOUNT;

	return windat->line_data[line].account;
}


/**
 * Return the text on a given line of an Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \param line			The line to return the details for.
 * \return			A volatile pointer to the text on the line,
 *				or NULL.
 */

char *account_list_window_get_entry_text(struct account_list_window *windat, int line)
{
	struct file_block *file;

	if (windat == NULL || windat->instance == NULL || !account_list_window_line_valid(windat, line))
		return ACCOUNT_LINE_BLANK;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return ACCOUNT_LINE_BLANK;

	if (windat->line_data[line].type == ACCOUNT_LINE_DATA)
		return account_get_name(file, windat->line_data[line].account);

	return windat->line_data[line].heading;
}


/**
 * Start an account list window drag, to re-order the entries in the window.
 *
 * \param *windat		The Account List Window being dragged.
 * \param *window		The state of the window being dragged.
 * \param line			The line of the Account list being dragged.
 */

static void account_list_window_start_drag(struct account_list_window *windat, wimp_window_state *window, int line)
{
	wimp_auto_scroll_info	auto_scroll;
	wimp_drag		drag;
	int			ox, oy;

	if (windat == NULL || window == NULL)
		return;

	/* The drag is not started if any of the account window edit dialogues
	 * are open, as these will have pointers into the data which won't like
	 * the data moving beneath them.
	 */

	if (dialogue_any_open(NULL, windat->instance) || dialogue_any_open(NULL, windat))
		return;

	ox = window->visible.x0 - window->xscroll;
	oy = window->visible.y1 - window->yscroll;

	/* Set up the drag parameters. */

	drag.w = windat->account_window;
	drag.type = wimp_DRAG_USER_FIXED;

	drag.initial.x0 = ox;
	drag.initial.y0 = oy + WINDOW_ROW_Y0(ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, line);
	drag.initial.x1 = ox + (window->visible.x1 - window->visible.x0);
	drag.initial.y1 = oy + WINDOW_ROW_Y1(ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT, line);

	drag.bbox.x0 = window->visible.x0;
	drag.bbox.y0 = window->visible.y0;
	drag.bbox.x1 = window->visible.x1;
	drag.bbox.y1 = window->visible.y1;

	/* Read CMOS RAM to see if solid drags are required.
	 *
	 * \TODO -- Solid drags are never actually used, although they could be
	 *          if a suitable sprite were to be created.
	 */

	account_list_window_dragging_data.dragging_sprite = ((osbyte2(osbyte_READ_CMOS, osbyte_CONFIGURE_DRAG_ASPRITE, 0) &
                       osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0);

	if (FALSE && account_list_window_dragging_data.dragging_sprite) {
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
		auto_scroll.pause_zone_sizes.y0 = AUTO_SCROLL_MARGIN + ACCOUNT_LIST_WINDOW_FOOTER_HEIGHT;
		auto_scroll.pause_zone_sizes.x1 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.y1 = AUTO_SCROLL_MARGIN + ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT;
		auto_scroll.pause_duration = 0;
		auto_scroll.state_change = (void *) 1;

		wimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_HORIZONTAL | wimp_AUTO_SCROLL_ENABLE_VERTICAL, &auto_scroll);
	}

	account_list_window_dragging_data.owner = windat;
	account_list_window_dragging_data.start_line = line;

	event_set_drag_handler(account_list_window_terminate_drag, NULL, &account_list_window_dragging_data);
}


/**
 * Handle drag-end events relating to dragging rows of an Account List
 * Window instance.
 *
 * \param *drag			The Wimp drag end data.
 * \param *data			Unused client data sent via Event Lib.
 */

static void account_list_window_terminate_drag(wimp_dragged *drag, void *data)
{
	wimp_pointer				pointer;
	wimp_window_state			window;
	int					line;
	struct file_block			*file;
	struct account_list_window_redraw	block;
	struct account_list_window_drag_data	*drag_data = data;
	struct account_list_window		*windat;

	if (drag_data == NULL)
		return;

	/* Terminate the drag and end the autoscroll. */

	if (xos_swi_number_from_string("Wimp_AutoScroll", NULL) == NULL)
		wimp_auto_scroll(0, NULL);

	if (drag_data->dragging_sprite)
		dragasprite_stop();

	/* Check that the returned data is valid. */

	windat = drag_data->owner;
	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	/* Get the line at which the drag ended. We do this manually, as there's
	 * no requirement for the user to hit a row exactly.
	 */

	wimp_get_pointer_info(&pointer);

	window.w = windat->account_window;
	wimp_get_window_state(&window);

	line = ((window.visible.y1 - pointer.pos.y) - window.yscroll - ACCOUNT_LIST_WINDOW_TOOLBAR_HEIGHT) / WINDOW_ROW_HEIGHT;

	if (line < 0)
		line = 0;

	if (line >= windat->display_lines)
		line = windat->display_lines - 1;

	/* Move the blocks around. */

	block = windat->line_data[drag_data->start_line];

	if (line < drag_data->start_line) {
		memmove(&(windat->line_data[line + 1]), &(windat->line_data[line]),
				(drag_data->start_line - line) * sizeof(struct account_list_window_redraw));

		windat->line_data[line] = block;
	} else if (line > drag_data->start_line) {
		memmove(&(windat->line_data[drag_data->start_line]),
				&(windat->line_data[drag_data->start_line + 1]),
				(line - drag_data->start_line) * sizeof(struct account_list_window_redraw));

		windat->line_data[line] = block;
	}

	/* Tidy up and redraw the windows */

	account_recalculate_all(file);
	file_set_data_integrity(file, TRUE);
	account_list_window_force_redraw(windat, 0, windat->display_lines - 1, wimp_ICON_WINDOW);

	#ifdef DEBUG
	debug_printf("Move account from line %d to line %d", drag_data->start_line, line);
	#endif
}


/**
 * Recalculate the data in the the given Account List window instance
 * (totals, sub-totals, budget totals, etc) and refresh the display.
 *
 * \param *windat		The Account List Window instance to
 *				recalculate.
 */

void account_list_window_recalculate(struct account_list_window *windat)
{
	int line, column, sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMNS], total[ACCOUNT_LIST_WINDOW_NUM_COLUMNS];
	amt_t statement_balance, current_balance, future_balance, budget_amount, budget_balance, trial_balance, budget_result, credit_limit;

	if (windat == NULL)
		return;

	/* Reset the totals. */

	for (column = 0; column < ACCOUNT_LIST_WINDOW_NUM_COLUMNS; column++) {
		sub_total[column] = 0;
		total[column] = 0;
	}

	/* Add up the line data. */

	for (line = 0; line < windat->display_lines; line++) {
		windat->line_data[line].overdrawn = ACCOUNT_LIST_WINDOW_OVERDRAWN_NONE;

		switch (windat->line_data[line].type) {
		case ACCOUNT_LINE_DATA:
			if (!account_get_data(windat->instance, windat->line_data[line].account,
					&statement_balance, &current_balance, &future_balance, &credit_limit,
					&budget_amount, &budget_balance, &trial_balance, NULL))
				continue;

			switch (windat->type) {
			case ACCOUNT_FULL:
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] += statement_balance;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] += current_balance;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] += trial_balance;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] += budget_balance;

				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] += statement_balance;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] += current_balance;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] += trial_balance;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] += budget_balance;

				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] = statement_balance;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] = current_balance;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] = trial_balance;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] = budget_balance;

				if (statement_balance < -credit_limit)
					windat->line_data[line].overdrawn |= ACCOUNT_LIST_WINDOW_OVERDRAWN_STATEMENT;

				if (current_balance < -credit_limit)
					windat->line_data[line].overdrawn |= ACCOUNT_LIST_WINDOW_OVERDRAWN_CURRENT;

				if (trial_balance < 0)
					windat->line_data[line].overdrawn |= ACCOUNT_LIST_WINDOW_OVERDRAWN_FUTURE;
				break;

			case ACCOUNT_IN:
				if (budget_amount != NULL_CURRENCY)
					budget_result = -budget_amount - budget_balance;
				else
					budget_result = NULL_CURRENCY;

				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] -= future_balance;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] += budget_amount;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] -= budget_balance;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] += budget_result;

				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] -= future_balance;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] += budget_amount;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] -= budget_balance;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] += budget_result;

				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] = -future_balance;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] = budget_amount;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] = -budget_balance;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] = budget_result;

				if (-budget_balance < budget_amount)
					windat->line_data[line].overdrawn |= ACCOUNT_LIST_WINDOW_OVERDRAWN_BUDGET;
				break;

			case ACCOUNT_OUT:
				if (budget_amount != NULL_CURRENCY)
					budget_result = budget_amount - budget_balance;
				else
					budget_result = NULL_CURRENCY;

				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] += future_balance;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] += budget_amount;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] += budget_balance;
				sub_total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] += budget_result;

				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] += future_balance;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] += budget_amount;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] += budget_balance;
				total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] += budget_result;

				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT] = future_balance;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT] = budget_amount;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL] = budget_balance;
				windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET] = budget_result;

				if (budget_balance > budget_amount)
					windat->line_data[line].overdrawn |= ACCOUNT_LIST_WINDOW_OVERDRAWN_BUDGET;
				break;
			default:
				break;
			}
			break;

		case ACCOUNT_LINE_HEADER:
			for (column = 0; column < ACCOUNT_LIST_WINDOW_NUM_COLUMNS; column++)
				sub_total[column] = 0;
			break;

		case ACCOUNT_LINE_FOOTER:
			for (column = 0; column < ACCOUNT_LIST_WINDOW_NUM_COLUMNS; column++)
				windat->line_data[line].total[column] = sub_total[column];
			break;

		default:
			break;
		}
	}

	for (column = 0; column < ACCOUNT_LIST_WINDOW_NUM_COLUMNS; column++)
		currency_convert_to_string(total[column], windat->footer_icon[column], AMOUNT_FIELD_LEN);
}


/**
 * Save an Account List Window's details to a CashBook file
 *
 * \param *windat		The Account List Window instance to write
 * \param *out			The file handle to write to.
 */

void account_list_window_write_file(struct account_list_window *windat, FILE *out)
{
	int		line;
	char		buffer[FILING_MAX_FILE_LINE_LEN];

	if (windat == NULL)
		return;

	/* Output the Accounts Windows data. */


	fprintf(out, "\n[AccountList:%x]\n", windat->type);

	fprintf(out, "Entries: %x\n", windat->display_lines);

	column_write_as_text(windat->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	for (line = 0; line < windat->display_lines; line++) {
		fprintf(out, "@: %x,%x\n", windat->line_data[line].type, windat->line_data[line].account);

		if ((windat->line_data[line].type == ACCOUNT_LINE_HEADER || windat->line_data[line].type == ACCOUNT_LINE_FOOTER) &&
				*(windat->line_data[line].heading) != '\0')
			config_write_token_pair(out, "Heading", windat->line_data[line].heading);
	}
}


/**
 * Read account list details from a CashBook file into an Account List
 * Window instance.
 *
 * \param *windat		The Account List Window instance to populate.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool account_list_window_read_file(struct account_list_window *windat, struct filing_block *in)
{
	size_t			block_size;
	int			line = -1;

	if (windat == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Account List Data.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(windat->line_data), sizeof(struct account_list_window_redraw), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > windat->display_lines) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = windat->display_lines;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			column_init_window(windat->columns, 0, TRUE, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			windat->display_lines++;
			if (windat->display_lines > block_size) {
				block_size = windat->display_lines;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			}
			line = windat->display_lines - 1;
			*(windat->line_data[line].heading) = '\0';
			windat->line_data[line].type = account_get_account_line_type_field(in);
			windat->line_data[line].account = account_get_account_field(in);
		} else if (line != -1 && filing_test_token(in, "Heading")) {
			filing_get_text_value(in, windat->line_data[line].heading, ACCOUNT_SECTION_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(windat->display_lines)) {
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

static osbool account_list_window_save_csv(char *filename, osbool selection, void *data)
{
	struct account_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	account_list_window_export_delimited(windat, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the account data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool account_list_window_save_tsv(char *filename, osbool selection, void *data)
{
	struct account_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	account_list_window_export_delimited(windat, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

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

static void account_list_window_export_delimited(struct account_list_window *windat, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE			*out;
	int			line;
	char			buffer[FILING_DELIMITED_FIELD_LEN];
	struct file_block	*file;

	if (windat == NULL || windat->instance == NULL)
		return;

	file = account_get_file(windat->instance);
	if (file == NULL)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	columns_export_heading_names(windat->columns, windat->account_pane, out, format, buffer, FILING_DELIMITED_FIELD_LEN);

	/* Output the transaction data as a set of delimited lines. */

	for (line = 0; line < windat->display_lines; line++) {
		if (windat->line_data[line].type == ACCOUNT_LINE_DATA) {
			account_build_name_pair(file, windat->line_data[line].account, buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
		} else if (windat->line_data[line].type == ACCOUNT_LINE_HEADER) {
			filing_output_delimited_field(out, windat->line_data[line].heading, format, DELIMIT_LAST);
		} else if (windat->line_data[line].type == ACCOUNT_LINE_FOOTER) {
			filing_output_delimited_field(out, windat->line_data[line].heading, format, DELIMIT_NONE);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

			currency_convert_to_string(windat->line_data[line].total[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET], buffer, FILING_DELIMITED_FIELD_LEN);
			filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
		}
	}

	/* Output the grand total line, taking the text from the window icons. */

	icons_copy_text(windat->account_footer, ACCOUNT_LIST_WINDOW_FOOT_NAME, buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_STATEMENT], format, DELIMIT_NUM);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_CURRENT], format, DELIMIT_NUM);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_FINAL], format, DELIMIT_NUM);
	filing_output_delimited_field(out, windat->footer_icon[ACCOUNT_LIST_WINDOW_NUM_COLUMN_BUDGET], format, DELIMIT_NUM | DELIMIT_LAST);

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}

