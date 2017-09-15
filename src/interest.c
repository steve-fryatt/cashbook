/* Copyright 2016-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: interest.c
 *
 * Interest Rate manager implementation.
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

#include "interest.h"

#include "column.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "flexutils.h"
#include "window.h"

/**
 * The length of a format string generated by the conversion to a string
 * value.
 */

#define INTEREST_FORMAT_LENGTH 20

/**
 * The length of an interest rate description field.
 */

#define INTEREST_DESCRIPTION_LEN 101

/* Interest Window details. */

/**
 * The height of the interest toolbar pane. */

#define INTEREST_TOOLBAR_HEIGHT 132

/**
 * The number of columns in the interest window. */

#define INTEREST_COLUMNS 4

/* The interest window icons. */

#define INTEREST_ICON_DATE 0
#define INTEREST_ICON_RATE 1
#define INTEREST_ICON_BALANCE 2
#define INTEREST_ICON_DESCRIPTION 3

/* The toolbar pane icons. */

#define INTEREST_PANE_DATE 0
#define INTEREST_PANE_RATE 1
#define INTEREST_PANE_BALANCE 2
#define INTEREST_PANE_DESCRIPTION 3

#define INTEREST_PANE_SORT_DIR_ICON 4

/* Interest Rate Window column mapping. */

static struct column_map interest_columns[] = {
	{INTEREST_ICON_DATE, INTEREST_PANE_DATE, wimp_ICON_WINDOW, SORT_DATE},
	{INTEREST_ICON_RATE, INTEREST_PANE_RATE, wimp_ICON_WINDOW, SORT_RATE},
	{INTEREST_ICON_BALANCE, INTEREST_PANE_BALANCE, wimp_ICON_WINDOW, SORT_BALANCE},
	{INTEREST_ICON_DESCRIPTION, INTEREST_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION}
};

/* Interest Rate List Window. */

static wimp_window		*interest_window_def = NULL;			/**< The definition for the Interest Rate List Window.			*/
static wimp_window		*interest_pane_def = NULL;			/**< The definition for the Interest Rate List Toolbar pane.		*/
static wimp_window		*interest_foot_def = NULL;			/**< The definition for the Interest Rate List Footer pane.		*/
static wimp_menu		*interest_window_menu = NULL;			/**< The Interest Rate List Window menu handle.				*/
static int			interest_window_menu_line = -1;			/**< The line over which the Interest Rate List Window Menu was opened.	*/


static int			interest_decimal_places = 0;			/**< The number of decimal places with which to show interest amounts.	*/
static char			interest_decimal_point = '.';			/**< The character to use for a decimal point.				*/

/* Interest Rate sorting. */

static struct sort_callback	interest_sort_callbacks;

/**
 * Interest Rate entry data structure.
 */

struct interest_rate {
	acct_t			account;

	rate_t			rate;

	date_t			effective_date;

	amt_t			minimum_balance;

	char			description[INTEREST_DESCRIPTION_LEN];
};

/**
 * Interest Rate instance data structure
 */

struct interest_block {
	struct file_block	*file;						/**< The file to which the instance belongs.				*/

	/* The window handles associated with the instance. */

	wimp_w			interest_window;
	char			window_title[WINDOW_TITLE_LENGTH];
	wimp_w			interest_pane;
	wimp_w			interest_footer;

	/* Display column details. */

	struct column_block	*columns;					/**< Instance handle of the column definitions.				*/

	/* Window sorting information. */

	struct sort_block	*sort;						/**< Instance handle for the sort code.					*/

	char			sort_sprite[COLUMN_SORT_SPRITE_LEN];		/**< Space for the sort icon's indirected data.				*/

	/* The account currently associated with this instance. */

	acct_t			active_account;

	/* The data. */

	struct interest_rate	*rates;

	int			rate_count;

	int			*display_index;

	int			display_count;
	int			display_lines;

};

/* Static function prototypes. */

static void		interest_close_window_handler(wimp_close *close);
static void		interest_pane_click_handler(wimp_pointer *pointer);
static void		interest_window_scroll_handler(wimp_scroll *scroll);
static void		interest_window_redraw_handler(wimp_draw *redraw);
static void		interest_adjust_window_columns(void *data, wimp_i group, int width);
static void		interest_adjust_sort_icon(struct interest_block *windat);
static void		interest_adjust_sort_icon_data(struct interest_block *windat, wimp_icon *icon);
static void		interest_set_window_extent(struct interest_block *windat);
static void		interest_force_window_redraw(struct interest_block *windat, int from, int to, wimp_i column);
static void		interest_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);



/**
 * Initialise the transaction system.
 *
 * \param *sprites		The application sprite area.
 */

void interest_initialise(osspriteop_area *sprites)
{
	interest_window_def = templates_load_window("Interest");
	interest_window_def->icon_count = 0;

	interest_pane_def = templates_load_window("InterestTB");
	interest_pane_def->sprite_area = sprites;

	interest_foot_def = templates_load_window("InterestFB");

//	interest_window_menu = templates_get_menu("InterestMenu");

//	account_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", account_save_csv);
//	account_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", account_save_tsv);

	interest_decimal_places = 2;

	interest_sort_callbacks.compare = NULL; //interest_sort_compare;
	interest_sort_callbacks.swap = NULL; //interest_sort_swap;
}


/**
 * Create a new interest rate module instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct interest_block *interest_create_instance(struct file_block *file)
{
	struct interest_block	*new;

	new = heap_alloc(sizeof(struct interest_block));
	if (new == NULL)
		return NULL;

	/* Initialise the interest module. */

	new->file = file;

	new->interest_window = NULL;
	new->interest_pane = NULL;
	new->columns = NULL;
	new->sort = NULL;

	new->rates = NULL;
	new->display_index = NULL;

	new->rate_count = 0;
	new->display_count = 0;
	new->display_lines = 0;

	new->active_account = NULL_ACCOUNT;

	/* Initialise the window columns. */

	new-> columns = column_create_instance(INTEREST_COLUMNS, interest_columns, NULL, wimp_ICON_WINDOW);
	if (new->columns == NULL) {
		interest_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read("LimInterestCols"));
	column_init_window(new->columns, 0, FALSE, config_str_read("InterestCols"));

	/* Initialise the window sort. */

	new->sort = sort_create_instance(SORT_DATE | SORT_ASCENDING, SORT_ROW | SORT_ASCENDING,  &interest_sort_callbacks, new);
	if (new->sort == NULL) {
		interest_delete_instance(new);
		return NULL;
	}

	/* Initialise the flex blocks. */

	if (!flexutils_initialise((void **) &(new->rates))) {
		interest_delete_instance(new);
		return NULL;
	}

	if (!flexutils_initialise((void **) &(new->display_index))) {
		interest_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete an interest rate module instance, and all of its data.
 *
 * \param *instance		The instance to be deleted.
 */

void interest_delete_instance(struct interest_block *instance)
{
	if (instance == NULL)
		return;

	if (instance->interest_window != NULL)
		interest_delete_window(instance, NULL_ACCOUNT);

	column_delete_instance(instance->columns);
	sort_delete_instance(instance->sort);

	if (instance->rates != NULL)
		flexutils_free((void **) &(instance->rates));

	if (instance->display_index != NULL)
		flexutils_free((void **) &(instance->display_index));

	heap_free(instance);
}


/**
 * Open an interest window for a given account.
 *
 * \param *instance		The instance to own the window.
 * \param account		The account to open the window for.
 */

void interest_open_window(struct interest_block *instance, acct_t account)
{
	int			height;
	wimp_window_state	parent;
	os_error		*error;

	debug_printf("We want to open an interest window for instance 0x%x, account %d", instance, account);

	if (instance == NULL || instance->file == NULL)
		return;

	/* If there's a different account active, close it down. */

	if (instance->active_account != NULL_ACCOUNT && instance->active_account != account)
		interest_delete_window(instance, instance->active_account);

	/* If the window is currently open, bring it to the top of the stack. */

	if (instance->interest_window != NULL) {
		windows_open(instance->interest_window);
		return;
	}

	/* Set the default values */

	*(instance->window_title) = '\0';
	interest_window_def->title_data.indirected_text.text = instance->window_title;

//	height = (file->sorders->sorder_count > MIN_SORDER_ENTRIES) ? file->sorders->sorder_count : MIN_SORDER_ENTRIES;
	height = 10;

	transact_get_window_state(instance->file, &parent);

	window_set_initial_area(interest_window_def, column_get_window_width(instance->columns),
			(height * WINDOW_ROW_HEIGHT) + INTEREST_TOOLBAR_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(instance->file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window(interest_window_def, &(instance->interest_window));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Create the toolbar pane. */

	windows_place_as_toolbar(interest_window_def, interest_pane_def, INTEREST_TOOLBAR_HEIGHT - 4);
	columns_place_heading_icons(instance->columns, interest_pane_def);

	interest_pane_def->icons[INTEREST_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
			(osspriteop_id) instance->sort_sprite;
	interest_pane_def->icons[INTEREST_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
			interest_pane_def->sprite_area;
	interest_pane_def->icons[INTEREST_PANE_SORT_DIR_ICON].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

	interest_adjust_sort_icon_data(instance, &(interest_pane_def->icons[INTEREST_PANE_SORT_DIR_ICON]));

	error = xwimp_create_window(interest_pane_def, &(instance->interest_pane));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Construct the edit line. */

//	file->transacts->edit_line = edit_create_instance(file, transact_window_def, file->transacts->transaction_window,
//			file->transacts->columns, TRANSACT_TOOLBAR_HEIGHT,
//			&transact_edit_callbacks, file->transacts);
//	if (file->transacts->edit_line == NULL) {
//		error_msgs_report_error("TransactNoMem");
//		return;
//	}

//	edit_add_field(file->transacts->edit_line, EDIT_FIELD_DISPLAY,
//			TRANSACT_ICON_ROW, transact_buffer_row, TRANSACT_ROW_FIELD_LEN);
//	edit_add_field(file->transacts->edit_line, EDIT_FIELD_DATE,
//			TRANSACT_ICON_DATE, transact_buffer_date, DATE_FIELD_LEN);
//	edit_add_field(file->transacts->edit_line, EDIT_FIELD_ACCOUNT_IN,
//			TRANSACT_ICON_FROM, transact_buffer_from_ident, ACCOUNT_IDENT_LEN,
//			TRANSACT_ICON_FROM_REC, transact_buffer_from_rec, REC_FIELD_LEN,
//			TRANSACT_ICON_FROM_NAME, transact_buffer_from_name, ACCOUNT_NAME_LEN);
//	edit_add_field(file->transacts->edit_line, EDIT_FIELD_ACCOUNT_OUT,
//			TRANSACT_ICON_TO, transact_buffer_to_ident, ACCOUNT_IDENT_LEN,
//			TRANSACT_ICON_TO_REC, transact_buffer_to_rec, REC_FIELD_LEN,
//			TRANSACT_ICON_TO_NAME, transact_buffer_to_name, ACCOUNT_NAME_LEN);
//	edit_add_field(file->transacts->edit_line, EDIT_FIELD_TEXT,
//			TRANSACT_ICON_REFERENCE, transact_buffer_reference, TRANSACT_REF_FIELD_LEN);
//	edit_add_field(file->transacts->edit_line, EDIT_FIELD_CURRENCY,
//			TRANSACT_ICON_AMOUNT, transact_buffer_amount, AMOUNT_FIELD_LEN);
//	edit_add_field(file->transacts->edit_line, EDIT_FIELD_TEXT,
//			TRANSACT_ICON_DESCRIPTION, transact_buffer_description, TRANSACT_DESCRIPT_FIELD_LEN);

//	if (!edit_complete(file->transacts->edit_line)) {
//		edit_delete_instance(file->transacts->edit_line);
//		error_msgs_report_error("TransactNoMem");
//		return;
//	}

	instance->active_account = account;

	/* Set the title */

	interest_build_window_title(instance->file);

	/* Update the toolbar */

//	transact_update_toolbar(file);


	/* Open the window. */

	windows_open(instance->interest_window);
	windows_open_nested_as_toolbar(instance->interest_pane, instance->interest_window, INTEREST_TOOLBAR_HEIGHT - 4);

	ihelp_add_window(instance->interest_window , "Interest", interest_decode_window_help);
	ihelp_add_window(instance->interest_pane , "InterestTB", NULL);

	/* Register event handlers for the two windows. */
	/* \TODO -- Should this be all three windows?   */

	event_add_window_user_data(instance->interest_window, instance);
//	event_add_window_menu(file->transacts->transaction_window, transact_window_menu);
//	event_add_window_open_event(file->transacts->transaction_window, transact_window_open_handler);
	event_add_window_close_event(instance->interest_window, interest_close_window_handler);
//	event_add_window_lose_caret_event(file->transacts->transaction_window, transact_window_lose_caret_handler);
//	event_add_window_mouse_event(file->transacts->transaction_window, transact_window_click_handler);
//	event_add_window_key_event(file->transacts->transaction_window, transact_window_keypress_handler);
	event_add_window_scroll_event(instance->interest_window, interest_window_scroll_handler);
	event_add_window_redraw_event(instance->interest_window, interest_window_redraw_handler);
//	event_add_window_menu_prepare(file->transacts->transaction_window, transact_window_menu_prepare_handler);
//	event_add_window_menu_selection(file->transacts->transaction_window, transact_window_menu_selection_handler);
//	event_add_window_menu_warning(file->transacts->transaction_window, transact_window_menu_warning_handler);
//	event_add_window_menu_close(file->transacts->transaction_window, transact_window_menu_close_handler);

	event_add_window_user_data(instance->interest_pane, instance);
//	event_add_window_menu(file->transacts->transaction_pane, transact_window_menu);
	event_add_window_mouse_event(instance->interest_pane, interest_pane_click_handler);
//	event_add_window_menu_prepare(file->transacts->transaction_pane, transact_window_menu_prepare_handler);
//	event_add_window_menu_selection(file->transacts->transaction_pane, transact_window_menu_selection_handler);
//	event_add_window_menu_warning(file->transacts->transaction_pane, transact_window_menu_warning_handler);
//	event_add_window_menu_close(file->transacts->transaction_pane, transact_window_menu_close_handler);
//	event_add_window_icon_popup(file->transacts->transaction_pane, TRANSACT_PANE_VIEWACCT, transact_account_list_menu, -1, NULL);

//	dataxfer_set_drop_target(dataxfer_TYPE_CSV, file->transacts->transaction_window, -1, NULL, transact_load_csv, file);
//	dataxfer_set_drop_target(dataxfer_TYPE_CSV, file->transacts->transaction_pane, -1, NULL, transact_load_csv, file);

	/* Put the caret into the first empty line. */

//	transact_place_caret(file, file->transacts->trans_count, TRANSACT_FIELD_DATE);
}


/**
 * Close an interest window.
 *
 * \param *instance		The instance which owns the window.
 * \param account		The account to close the window for, or NULL_ACCOUNT
 *				to forcibly close any window that the instance has open.
 */

void interest_delete_window(struct interest_block *instance, acct_t account)
{
	if (instance == NULL || (account != NULL_ACCOUNT && account != instance->active_account))
		return;

	debug_printf("We want to close an interest window for instance 0x%x, account %d", instance, account);

	if (instance->interest_window != NULL) {
		ihelp_remove_window(instance->interest_window);
		event_delete_window(instance->interest_window);
		wimp_delete_window(instance->interest_window);
		instance->interest_window = NULL;
	}

	if (instance->interest_pane != NULL) {
		ihelp_remove_window(instance->interest_pane);
		event_delete_window(instance->interest_pane);
		wimp_delete_window(instance->interest_pane);
		instance->interest_pane = NULL;
	}

	instance->active_account = NULL_ACCOUNT;
}


/**
 * Handle Close events on Interest windows, deleting the window.
 *
 * \param *close		The Wimp Close data block.
 */

static void interest_close_window_handler(wimp_close *close)
{
	struct interest_block	*instance;

	#ifdef DEBUG
	debug_printf("\\RClosing Interest window");
	#endif

	instance = event_get_window_user_data(close->w);
	if (instance == NULL)
		return;

	/* Close the window */

	interest_delete_window(instance, instance->active_account);
}


/**
 * Process mouse clicks in the interest rate pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void interest_pane_click_handler(wimp_pointer *pointer)
{
	struct interest_block	*windat;
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
	//	switch (pointer->i) {
	//	case SORDER_PANE_PARENT:
	//		transact_bring_window_to_top(windat->file);
	//		break;

	//	case SORDER_PANE_PRINT:
	//		sorder_open_print_window(file, pointer, config_opt_read("RememberValues"));
	//		break;

	//	case SORDER_PANE_ADDSORDER:
	//		sorder_open_edit_window(file, NULL_SORDER, pointer);
	//		break;

	//	case SORDER_PANE_SORT:
	//		sorder_open_sort_window(windat, pointer);
	//		break;
	//	}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
	//	switch (pointer->i) {
	//	case SORDER_PANE_PRINT:
	//		sorder_open_print_window(file, pointer, !config_opt_read("RememberValues"));
	//		break;

	//	case SORDER_PANE_SORT:
	//		sorder_sort(file->sorders);
	//		break;
	//	}
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
				interest_adjust_sort_icon(windat);
				windows_redraw(windat->interest_pane);
	//			sorder_sort(file->sorders);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(windat->columns, pointer->i)) {
		column_set_minimum_widths(windat->columns, config_str_read("LimInterestCols"));
		column_start_drag(windat->columns, pointer, windat, windat->interest_window, interest_adjust_window_columns);
	}
}


/**
 * Process scroll events in the interest rate window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void interest_window_scroll_handler(wimp_scroll *scroll)
{
	window_process_scroll_effect(scroll, INTEREST_TOOLBAR_HEIGHT);

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

	wimp_open_window((wimp_open *) scroll);
}


/**
 * Process redraw events in the interest rate window.
 *
 * \param *redraw		The draw event block to handle.
 */

static void interest_window_redraw_handler(wimp_draw *redraw)
{
	struct interest_block	*windat;
	int			ox, oy, top, base, y, r, width;
	char			icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more;

	windat = event_get_window_user_data(redraw->w);
	if (windat == NULL || windat->file == NULL || windat->columns == NULL)
		return;

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	/* Set the horizontal positions of the icons. */

	columns_place_table_icons_horizontally(windat->columns, interest_window_def, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);

	width = column_get_window_width(windat->columns);

	window_set_icon_templates(interest_window_def);

	/* Perform the redraw. */

	while (more) {
		/* Calculate the rows to redraw. */

		top = WINDOW_REDRAW_TOP(INTEREST_TOOLBAR_HEIGHT, oy - redraw->clip.y1);
		if (top < 0)
			top = 0;

		base = WINDOW_REDRAW_BASE(INTEREST_TOOLBAR_HEIGHT, oy - redraw->clip.y0);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
	//		r = (y < windat->sorder_count) ? windat->sorders[y].sort_index : 0;
			r = y;

			/* Plot out the background with a filled white rectangle. */

			wimp_set_colour(wimp_COLOUR_VERY_LIGHT_GREY);
			os_plot(os_MOVE_TO, ox, oy + WINDOW_ROW_TOP(INTEREST_TOOLBAR_HEIGHT, y));
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO, ox + width, oy + WINDOW_ROW_BASE(INTEREST_TOOLBAR_HEIGHT, y));

			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(windat->columns, interest_window_def,
					WINDOW_ROW_Y0(INTEREST_TOOLBAR_HEIGHT, y), WINDOW_ROW_Y1(INTEREST_TOOLBAR_HEIGHT, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= windat->rate_count) {
				columns_plot_empty_table_icons(windat->columns);
				continue;
			}

			/* Effective Date field */

			window_plot_date_field(INTEREST_ICON_DATE, windat->rates[r].effective_date, wimp_COLOUR_BLACK);

			/* Rate field */

			window_plot_interest_rate_field(INTEREST_ICON_RATE, windat->rates[r].rate, wimp_COLOUR_BLACK);

			/* Minimum Balance field */

			window_plot_currency_field(INTEREST_ICON_BALANCE, windat->rates[r].minimum_balance, wimp_COLOUR_BLACK);

			/* Description field */

			window_plot_text_field(INTEREST_ICON_DESCRIPTION, windat->rates[r].description, wimp_COLOUR_BLACK);
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

static void interest_adjust_window_columns(void *data, wimp_i group, int width)
{
	struct interest_block	*windat = (struct interest_block *) data;
	int			new_extent;
	wimp_window_info	window;

	if (windat == NULL || windat->file == NULL)
		return;

	columns_update_dragged(windat->columns, windat->interest_pane, NULL, group, width);

	new_extent = column_get_window_width(windat->columns);

	interest_adjust_sort_icon(windat);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(windat->interest_window);
	windows_redraw(windat->interest_pane);

	/* Set the horizontal extent of the window and pane. */

	window.w = windat->interest_pane;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	window.w = windat->interest_window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(windat->file, TRUE);
}


/**
 * Adjust the sort icon in a interest window, to reflect the current column
 * heading positions.
 *
 * \param *windat		The interest window to update.
 */

static void interest_adjust_sort_icon(struct interest_block *windat)
{
	wimp_icon_state		icon;

	if (windat == NULL)
		return;

	icon.w = windat->interest_pane;
	icon.i = INTEREST_PANE_SORT_DIR_ICON;
	wimp_get_icon_state(&icon);

	interest_adjust_sort_icon_data(windat, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current standing order sort settings.
 *
 * \param *windat		The standing order window to be updated.
 * \param *icon			The icon to be updated.
 */

static void interest_adjust_sort_icon_data(struct interest_block *windat, wimp_icon *icon)
{
	enum sort_type	sort_order;

	if (windat == NULL)
		return;

	sort_order = sort_get_order(windat->sort);

	column_update_sort_indicator(windat->columns, icon, interest_pane_def, sort_order);
}


/**
 * Set the extent of the interest rate window for the specified file.
 *
 * \param *windat		The interest rate window to update.
 */

static void interest_set_window_extent(struct interest_block *windat)
{
	int	lines;


	if (windat == NULL || windat->interest_window == NULL)
		return;

//	lines = (windat->preset_count > MIN_PRESET_ENTRIES) ? windat->preset_count : MIN_PRESET_ENTRIES;
	lines = 10;

	window_set_extent(windat->interest_window, lines, INTEREST_TOOLBAR_HEIGHT, column_get_window_width(windat->columns));
}

/**
 * Recreate the title of the Interest List window connected to the given
 * file.
 *
 * \param *file			The file to rebuild the title for.
 */

void interest_build_window_title(struct file_block *file)
{
	char	name[WINDOW_TITLE_LENGTH];

	if (file == NULL || file->interest == NULL || file->interest->interest_window == NULL)
		return;

	file_get_leafname(file, name, WINDOW_TITLE_LENGTH);

	msgs_param_lookup("InterestTitle", file->interest->window_title, WINDOW_TITLE_LENGTH,
			account_get_name(file, file->interest->active_account), name, NULL, NULL);

	wimp_force_redraw_title(file->interest->interest_window);
}


/**
 * Force the complete redraw of the interest rate window.
 *
 * \param *file			The file owning the window to redraw.
 */

void interest_redraw_all(struct file_block *file)
{
	if (file == NULL || file->interest == NULL)
		return;

	interest_force_window_redraw(file->interest, 0, file->interest->rate_count - 1, wimp_ICON_WINDOW);
}


/**
 * Force a redraw of the interest rate window, for the given range of lines.
 *
 * \param *windat		The interest window to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void interest_force_window_redraw(struct interest_block *windat, int from, int to, wimp_i column)
{
	wimp_window_info	window;

	if (windat == NULL || windat->interest_window == NULL)
		return;

	window.w = windat->interest_window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(windat->columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(INTEREST_TOOLBAR_HEIGHT, from);
	window.extent.y0 = WINDOW_ROW_BASE(INTEREST_TOOLBAR_HEIGHT, to);

	wimp_force_redraw(windat->interest_window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);
}


/**
 * Turn a mouse position over the interest window into an interactive
 * help token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void interest_decode_window_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int			xpos;
	wimp_i			icon;
	wimp_window_state	window;
	struct interest_block	*windat;

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

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, interest_window_def->icons[icon].data.indirected_text.validation, 'N'))
		string_printf(buffer, IHELP_INAME_LEN, "Col%d", icon);
}


/**
 * Return an interest rate for a given account on a given date. Returns
 * NULL_RATE on failure.
 *
 * \param *instance		The interest rate module instance to use.
 * \param account		The account to return an interest rate for.
 * \param date			The date to return the rate for.
 * \return			The interest rate on the date in question.
 */

rate_t interest_get_current_rate(struct interest_block *instance, acct_t account, date_t date)
{
	return NULL_RATE;
}


/**
 * Convert an interest rate into a string, placing the result into a
 * supplied buffer.
 *
 * \param rate			The rate to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *interest_convert_to_string(rate_t rate, char *buffer, size_t length)
{
	int	i, places, size;
	char	*end, conversion[INTEREST_FORMAT_LENGTH];

	if (buffer == NULL || length == 0)
		return NULL;

	/* If the value is NULL_RATE, show it as zero. */

	if (rate == NULL_RATE)
		rate = 0;

	/* Convert the integer value into a string. The calculated conversion
	 * string forces a zero for each decimal place plus one extra, to
	 * give us all the actual digits required to turn into a human-
	 * readable number. eg. For 2 decimal places, 0 would become 000 so
	 * that a decimal point can be inserted to give 0.00
	 *
	 * Negative numbers need an additional place in the format string,
	 * as the - sign takes up one of the 'digits' in sprintf().
	 */

	places = interest_decimal_places + 1;
	string_printf(conversion, INTEREST_FORMAT_LENGTH, "%%0%1dd", places + ((rate < 0) ? 1 : 0));

	/* Print the number to the buffer and find the end. */

	string_printf(buffer, length, conversion, rate);
	size = strlen(buffer);
	end = buffer + size;

	debug_printf("Size = %d, length = %d, places = %d", size, length, places);

	/* If there is a decimal point, shuffle the higher digits up one to
	 * make room and insert it.
	 */

	if (places > 1) {
		/* If the string just fits the supplied buffer without a decimal
		 * point, we can't add one so just return an empty string.
		 */

		if (size + 1 >= length) {
			*buffer = '\0';
			return buffer;
		}

		for (i = 1; i <= places; i++) {
			*(end + 1) = *end;
			end--;
		}

		*(++end) = interest_decimal_point;

		size++;
	}

	return buffer;
}











/**
 * Save the interest rate details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void interest_write_file(struct file_block *file, FILE *out)
{
	int	i;
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (file == NULL || file->interest == NULL)
		return;

	fprintf(out, "\n[Interest]\n");

	fprintf(out, "Entries: %x\n", file->interest->rate_count);

	column_write_as_text(file->interest->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	sort_write_as_text(file->interest->sort, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "SortOrder: %s\n", buffer);

	for (i = 0; i < file->interest->rate_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x\n",
				file->interest->rates[i].account, file->interest->rates[i].rate, file->interest->rates[i].effective_date,
				file->interest->rates[i].minimum_balance);
		if (*(file->interest->rates[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->interest->rates[i].description);
	}
}


/**
 * Read interest rate details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool interest_read_file(struct file_block *file, struct filing_block *in)
{
	size_t			block_size;
	rate_t			rate = NULL_RATE;

	if (file == NULL || file->interest == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Interest Rates.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->interest->rates), sizeof(struct interest_rate), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->interest->rate_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->interest->rate_count;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			/* For file format 1.00 or older, there's no row column at the
			 * start of the line so skip on to colunn 1 (date).
			 */
			column_init_window(file->interest->columns, 0, TRUE, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "SortOrder")){
			sort_read_from_text(file->interest->sort, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			file->interest->rate_count++;
			if (file->interest->rate_count > block_size) {
				block_size = file->interest->rate_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			}
			rate = file->interest->rate_count-1;
			file->interest->rates[rate].account = account_get_account_field(in);
			file->interest->rates[rate].rate = interest_get_rate_field(in);
			file->interest->rates[rate].effective_date = date_get_date_field(in);
			file->interest->rates[rate].minimum_balance = currency_get_currency_field(in);

			*(file->interest->rates[rate].description) = '\0';
		} else if (rate != NULL_RATE && filing_test_token(in, "Desc")) {
			filing_get_text_value(in, file->interest->rates[rate].description, INTEREST_DESCRIPTION_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(file->interest->rate_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
}
