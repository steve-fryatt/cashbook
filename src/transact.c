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
 * OR CONDITIONS OF ANY KIND, either express or implied.a
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: transact.c
 *
 * Transaction implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

/* OSLib header files */

#include "oslib/dragasprite.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"
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

#include "global.h"
#include "transact.h"

#include "account.h"
#include "account_list_menu.h"
#include "account_menu.h"
#include "accview.h"
#include "analysis.h"
#include "analysis_template_menu.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "file_info.h"
#include "filing.h"
#include "find.h"
#include "flexutils.h"
#include "goto.h"
#include "presets.h"
#include "preset_menu.h"
#include "print_dialogue.h"
#include "purge.h"
#include "refdesc_menu.h"
#include "report.h"
#include "sorder.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "transact.h"
#include "transact_list_window.h"
#include "window.h"




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

/**
 * Transatcion data structure
 */

struct transaction {
	date_t			date;						/**< The date of the transaction.							*/
	enum transact_flags	flags;						/**< The flags applying to the transaction.						*/
	acct_t			from;						/**< The account from which money is being transferred.					*/
	acct_t			to;						/**< The account to which money is being transferred.					*/
	amt_t			amount;						/**< The amount of money transferred by the transaction.				*/
	char			reference[TRANSACT_REF_FIELD_LEN];		/**< The transaction reference text.							*/
	char			description[TRANSACT_DESCRIPT_FIELD_LEN];	/**< The transaction description text.							*/

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the transaction data, and are in effect a separate array that is used
	 * for handling entries in the transaction window.
	 */

	tran_t			sort_index;					/**< Point to another transaction, to allow the transaction window to be sorted.	*/
	tran_t			saved_sort;					/**< Preserve the transaction window sort order across transaction data sorts.		*/
	tran_t			sort_workspace;					/**< Workspace used by the sorting code.						*/
};


/**
 * Transatcion Window data structure
 */

struct transact_block {
	/**
	 * The file to which the instance belongs.
	 */
	struct file_block		*file;

	/**
	 * The Transaction List window instance.
	 */
	struct transact_list_window	*transact_window;

	/**
	 * The transaction data for the defined transactions.
	 */
	struct transaction		*transactions;

	/**
	 *The number of transactions defined in the file.
	 */
	int				trans_count;

	/**
	 * Is the transaction data sorted correctly into date order?
	 */
	osbool				date_sort_valid;
};

/* The following are the buffers used by the edit line in the transaction window. */

static char	transact_buffer_row[TRANSACT_ROW_FIELD_LEN];
static char	transact_buffer_date[DATE_FIELD_LEN];
static char	transact_buffer_from_ident[ACCOUNT_IDENT_LEN];
static char	transact_buffer_from_name[ACCOUNT_NAME_LEN];
static char	transact_buffer_from_rec[REC_FIELD_LEN];
static char	transact_buffer_to_ident[ACCOUNT_IDENT_LEN];
static char	transact_buffer_to_name[ACCOUNT_NAME_LEN];
static char	transact_buffer_to_rec[REC_FIELD_LEN];
static char	transact_buffer_reference [TRANSACT_REF_FIELD_LEN];
static char	transact_buffer_amount[AMOUNT_FIELD_LEN];
static char	transact_buffer_description[TRANSACT_DESCRIPT_FIELD_LEN];



static int    new_transaction_window_offset = 0;

/* Transaction Window. */




static void			transact_window_start_drag(struct transact_block *windat, wimp_window_state *window, wimp_i column, int line);
static void			transaction_window_terminate_drag(wimp_dragged *drag, void *data);

static osbool			transact_is_blank(struct file_block *file, tran_t transaction);
static tran_t			transact_find_edit_line_by_transaction(struct transact_block *windat);
static void			transact_place_edit_line_by_transaction(struct transact_block *windat, tran_t transaction);
static osbool			transact_edit_place_line(int line, void *data);
static void			transact_place_edit_line(struct transact_block *windat, int line);
static void			transact_find_edit_line_vertically(struct transact_block *windat);
static osbool			transact_edit_find_field(int line, int xmin, int xmax, enum edit_align target, void *data);
static int			transact_edit_first_blank_line(void *data);
static osbool			transact_edit_test_line(int line, void *data);
static osbool			transact_edit_get_field(struct edit_data *data);
static osbool			transact_edit_put_field(struct edit_data *data);
static void			transact_find_next_reconcile_line(struct transact_block *windat, osbool set);
static osbool			transact_edit_auto_complete(struct edit_data *data);
static char			*transact_complete_description(struct file_block *file, int line, char *buffer, size_t length);
static osbool			transact_edit_insert_preset(int line, wimp_key_no key, void *data);
static wimp_i			transact_convert_preset_icon_number(enum preset_caret caret);
static int			transact_edit_auto_sort(wimp_i icon, void *data);
static wimp_colour		transact_line_colour(struct transact_block *windat, int line);





/**
 * Test whether a transaction number is safe to look up in the transaction data array.
 */

#define transact_valid(windat, transaction) (((transaction) != NULL_TRANSACTION) && ((transaction) >= 0) && ((transaction) < ((windat)->trans_count)))


/**
 * Initialise the transaction system.
 *
 * \param *sprites		The application sprite area.
 */

void transact_initialise(osspriteop_area *sprites)
{
	transact_list_window_initialise(sprites);
}


/**
 * Create a new transaction window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct transact_block *transact_create_instance(struct file_block *file)
{
	struct transact_block	*new;

	new = heap_alloc(sizeof(struct transact_block));
	if (new == NULL)
		return NULL;

	/* Initialise the transaction block. */

	new->file = file;

	new->transactions = NULL;
	new->trans_count = 0;

	new->date_sort_valid = TRUE;

	/* Initialise the transaction window. */

	new->transact_window = transact_list_window_create_instance(new);
	if (new->transact_window == NULL) {
		transact_delete_instance(new);
		return NULL;
	}

	/* Initialise the transaction data. */

	if (!flexutils_initialise((void **) &(new->transactions))) {
		transact_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete a transaction window instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void transact_delete_instance(struct transact_block *windat)
{
	if (windat == NULL)
		return;

	transact_list_window_delete_instance(windat->transact_window);

	if (windat->transactions != NULL)
		flexutils_free((void **) &(windat->transactions));

	heap_free(windat);
}


/**
 * Create and open a Transaction List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void transact_open_window(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL || file->transacts->transact_window == NULL)
		return;

	transact_list_window_open(file->transacts->transact_window);
}


/**
 * Return the name of a transaction window column.
 *
 * \param *file			The file containing the transaction window.
 * \param field			The field representing the required column.
 * \param *buffer		Pointer to a buffer to take the name.
 * \param len			The length of the supplied buffer.
 * \return			Pointer to the supplied buffer, or NULL.
 */

char *transact_get_column_name(struct file_block *file, enum transact_field field, char *buffer, size_t len)
{
	if (file == NULL || file->transacts == NULL)
		return buffer;

	return transact_list_window_get_column_name(file->transacts->transact_window, field, buffer, len);
}


/**
 * Set the extent of the transaction window for the specified file.
 *
 * \param *file			The file containing the window to update.
 */

void transact_set_window_extent(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_list_window_set_extent(file->transacts->transact_window);
}


/**
 * Get the window state of the transaction window belonging to
 * the specified file.
 *
 * \param *file			The file containing the window.
 * \param *state		The structure to hold the window state.
 * \return			Pointer to an error block, or NULL on success.
 */

os_error *transact_get_window_state(struct file_block *file, wimp_window_state *state)
{
	if (file == NULL || file->transacts == NULL)
		return NULL;

	return transact_list_window_get_state(file->transacts->transact_window, state);
}


/**
 * Recreate the title of the Transaction window connected to the given file.
 *
 * \param *file			The file to rebuild the title for.
 */

void transact_build_window_title(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_list_window_build_title(file->transacts->transact_window);
}


/**
 * Force the complete redraw of the Transaction window.
 *
 * \param *file			The file owning the window to redraw.
 */

void transact_redraw_all(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_list_window_redraw_all(file->transacts->transact_window);
}


/**
 * Update the state of the buttons in a transaction window toolbar.
 *
 * \param *file			The file owning the window to update.
 */

void transact_update_toolbar(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_list_window_update_toolbar(file->transacts->transact_window);
}


/**
 * Bring a transaction window to the top of the window stack.
 *
 * \param *file			The file owning the window to bring up.
 */

void transact_bring_window_to_top(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_list_window_bring_to_top(file->transacts->transact_window);
}


/**
 * Scroll a transaction window to either the top (home) or the end.
 *
 * \param *file			The file owning the window to be scrolled.
 * \param direction		The direction to scroll the window in.
 */

void transact_scroll_window_to_end(struct file_block *file, enum transact_scroll_direction direction)
{
	wimp_window_info	window;


	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL ||
			direction == TRANSACT_SCROLL_NONE)
		return;

	window.w = file->transacts->transaction_window;
	wimp_get_window_info_header_only(&window);

	switch (direction) {
	case TRANSACT_SCROLL_HOME:
		window.yscroll = window.extent.y1;
		break;

	case TRANSACT_SCROLL_END:
		window.yscroll = window.extent.y0 - (window.extent.y1 - window.extent.y0);
		break;

	case TRANSACT_SCROLL_NONE:
		break;
	}

 	transact_minimise_window_extent(file);
 	wimp_open_window((wimp_open *) &window);
}


/**
 * Return the transaction number of the transaction nearest to the centre of
 * the visible area of the transaction window which references a given
 * account.
 *
 * \param *file			The file owning the window to be searched.
 * \param account		The account to search for.
 * \return			The transaction found, or NULL_TRANSACTION.
 */

int transact_find_nearest_window_centre(struct file_block *file, acct_t account)
{
	wimp_window_state	window;
	int			height, i, centre, result;


	if (file == NULL || file->transacts == NULL || file->transacts->transaction_window == NULL ||
			account == NULL_ACCOUNT)
		return NULL_TRANSACTION;

	window.w = file->transacts->transaction_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out
	 * any OS units taken up by part lines.
	 */

	height = window.visible.y1 - window.visible.y0 - WINDOW_ROW_HEIGHT - TRANSACT_TOOLBAR_HEIGHT;

	/* Calculate the centre line in the window. If this is greater than
	 * the number of actual tracsactions in the window, reduce it
	 * accordingly.
	 */

	centre = ((-window.yscroll + WINDOW_ROW_ICON_HEIGHT) / WINDOW_ROW_HEIGHT) + ((height / 2) / WINDOW_ROW_HEIGHT);

	if (centre >= file->transacts->trans_count)
		centre = file->transacts->trans_count - 1;

	/* If there are no transactions, we can't return one. */

	if (centre < 0)
		return NULL_TRANSACTION;

	/* If the centre transaction is a match, return it. */

	if (file->transacts->transactions[file->transacts->transactions[centre].sort_index].from == account ||
			file->transacts->transactions[file->transacts->transactions[centre].sort_index].to == account)
		return file->transacts->transactions[centre].sort_index;

	/* Start searching out from the centre until we find a match or hit
	 * both the start and end of the file.
	 */

	result = NULL_TRANSACTION;
	i = 1;

	while (centre + i < file->transacts->trans_count || centre - i >= 0) {
		if (centre + i < file->transacts->trans_count &&
				(file->transacts->transactions[file->transacts->transactions[centre + i].sort_index].from == account ||
				file->transacts->transactions[file->transacts->transactions[centre + i].sort_index].to == account)) {
			result = file->transacts->transactions[centre + i].sort_index;
			break;
		}

		if (centre - i >= 0 &&
				(file->transacts->transactions[file->transacts->transactions[centre - i].sort_index].from == account ||
				file->transacts->transactions[file->transacts->transactions[centre - i].sort_index].to == account)) {
			result = file->transacts->transactions[centre - i].sort_index;
			break;
		}

		i++;
	}

	return result;
}


/**
 * Find the display line in a transaction window which points to the
 * specified transaction under the applied sort.
 *
 * \param *file			The file to use the transaction window in.
 * \param transaction		The transaction to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

int transact_get_line_from_transaction(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL)
		return NULL_TRANSACTION;

	return transact_list_window_get_line_from_transaction(file->transacts->transact_window, line);
}


/**
 * Find the transaction which corresponds to a display line in a transaction
 * window.
 *
 * \param *file			The file to use the transaction window in.
 * \param line			The display line to return the transaction for.
 * \return			The appropriate transaction, or NULL_TRANSACTION.
 */

tran_t transact_get_transaction_from_line(struct file_block *file, int line)
{
	if (file == NULL || file->transacts == NULL)
		return NULL_TRANSACTION;

	return transact_list_window_get_transaction_from_line(file->transacts->transact_window, line);
}


/**
 * Find the number of transactions in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of transactions in the file.
 */

int transact_get_count(struct file_block *file)
{
	return (file != NULL && file->transacts != NULL) ? file->transacts->trans_count : 0;
}


/**
 * Find the display line number of the current transaction entry line.
 *
 * \param *file			The file to interrogate.
 * \return			The display line number of the line with the caret.
 */

int transact_get_caret_line(struct file_block *file)
{
	int entry_line = 0;

	if (file == NULL || file->transacts == NULL)
		return 0;

	entry_line = edit_get_line(file->transacts->edit_line);

	return (entry_line >= 0) ? entry_line : 0;
}


/**
 * Start a transaction window drag, to copy data within the window.
 *
 * \param *windat		The Transaction Window being dragged.
 * \param *window		The window state of the transaction window.
 * \param line			The line of the Transaction Window being dragged.
 */

static void transact_window_start_drag(struct transact_block *windat, wimp_window_state *window, wimp_i column, int line)
{
	wimp_auto_scroll_info	auto_scroll;
	wimp_drag		drag;
	int			ox, oy, xmin, xmax;

	if (windat == NULL || window == NULL)
		return;

	xmin = column_get_window_width(windat->columns);
	xmax = 0;

	column_get_xpos(windat->columns, column, &xmin, &xmax);

	ox = window->visible.x0 - window->xscroll;
	oy = window->visible.y1 - window->yscroll;

	/* Set up the drag parameters. */

	drag.w = windat->transaction_window;
	drag.type = wimp_DRAG_USER_FIXED;

	drag.initial.x0 = ox + xmin;
	drag.initial.y0 = oy + WINDOW_ROW_Y0(TRANSACT_TOOLBAR_HEIGHT, line);
	drag.initial.x1 = ox + xmax;
	drag.initial.y1 = oy + WINDOW_ROW_Y1(TRANSACT_TOOLBAR_HEIGHT, line);

	drag.bbox.x0 = window->visible.x0;
	drag.bbox.y0 = window->visible.y0;
	drag.bbox.x1 = window->visible.x1;
	drag.bbox.y1 = window->visible.y1;

	/* Read CMOS RAM to see if solid drags are required.
	 *
	 * \TODO -- Solid drags are never actually used, although they could be
	 *          if a suitable sprite were to be created.
	 */

	transact_window_dragging_data.dragging_sprite = ((osbyte2(osbyte_READ_CMOS, osbyte_CONFIGURE_DRAG_ASPRITE, 0) &
                       osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0);

	if (FALSE && transact_window_dragging_data.dragging_sprite) {
		dragasprite_start(dragasprite_HPOS_CENTRE | dragasprite_VPOS_CENTRE | dragasprite_NO_BOUND |
				dragasprite_BOUND_POINTER | dragasprite_DROP_SHADOW, wimpspriteop_AREA,
				"", &(drag.initial), &(drag.bbox));
	} else {
		wimp_drag_box(&drag);
	}

	/* Initialise the autoscroll. */

	if (xos_swi_number_from_string("Wimp_AutoScroll", NULL) == NULL) {
		auto_scroll.w = windat->transaction_window;
		auto_scroll.pause_zone_sizes.x0 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.y0 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.x1 = AUTO_SCROLL_MARGIN;
		auto_scroll.pause_zone_sizes.y1 = AUTO_SCROLL_MARGIN + TRANSACT_TOOLBAR_HEIGHT;
		auto_scroll.pause_duration = 0;
		auto_scroll.state_change = (void *) 1;

		wimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_HORIZONTAL | wimp_AUTO_SCROLL_ENABLE_VERTICAL, &auto_scroll);
	}

	transact_window_dragging_data.owner = windat;
	transact_window_dragging_data.start_line = line;
	transact_window_dragging_data.start_column = column;

	event_set_drag_handler(transaction_window_terminate_drag, NULL, &transact_window_dragging_data);
}


/**
 * Handle drag-end events relating to dragging rows of an Transaction
 * Window instance.
 *
 * \param *drag			The Wimp drag end data.
 * \param *data			Unused client data sent via Event Lib.
 */

static void transaction_window_terminate_drag(wimp_dragged *drag, void *data)
{
	wimp_pointer			pointer;
	wimp_window_state		window;
	int				end_line, xpos;
	wimp_i				end_column;
	tran_t				start_transaction;
	osbool				changed = FALSE;
	struct transact_drag_data	*drag_data = data;
	struct transact_block		*windat;
	enum edit_field_type		start_field_type;
	struct edit_data		*transfer;
	enum account_type		target_type;

	if (drag_data == NULL)
		return;

	/* Terminate the drag and end the autoscroll. */

	if (xos_swi_number_from_string("Wimp_AutoScroll", NULL) == NULL)
		wimp_auto_scroll(0, NULL);

	if (drag_data->dragging_sprite)
		dragasprite_stop();

	/* Check that the returned data is valid. */

	windat = drag_data->owner;
	if (windat == NULL || windat->file == NULL)
		return;

	/* Get the line at which the drag ended. */

	wimp_get_pointer_info(&pointer);

	window.w = windat->transaction_window;
	wimp_get_window_state(&window);

	end_line = window_calculate_click_row(&(pointer.pos), &window, TRANSACT_TOOLBAR_HEIGHT, -1);

	xpos = (pointer.pos.x - window.visible.x0) + window.xscroll;
	end_column = column_find_icon_from_xpos(windat->columns, xpos);

	if ((end_line < 0) || (end_column == wimp_ICON_WINDOW))
		return;

	#ifdef DEBUG
	debug_printf("Drag data from line %d, column %d to line %d, column %d", drag_data->start_line, drag_data->start_column, end_line, end_column);
	#endif

	start_transaction = transact_get_transaction_from_line(windat->file, drag_data->start_line);
	if (start_transaction == NULL_TRANSACTION)
		return;

	start_field_type = edit_get_field_type(windat->edit_line, drag_data->start_column);
	if (start_field_type == EDIT_FIELD_NONE)
		return;

	transact_place_edit_line(windat, end_line);

	transfer = edit_request_field_contents_update(windat->edit_line, end_column);
	if (transfer == NULL)
		return;

	switch (transfer->type) {
	case EDIT_FIELD_TEXT:
		if (start_field_type == EDIT_FIELD_TEXT) {
			switch (drag_data->start_column) {
			case TRANSACT_ICON_REFERENCE:
				string_copy(transfer->text.text, windat->transactions[start_transaction].reference, transfer->text.length);
				break;
			case TRANSACT_ICON_DESCRIPTION:
				string_copy(transfer->text.text, windat->transactions[start_transaction].description, transfer->text.length);
				break;
			}
			changed = TRUE;
		}
		break;
	case EDIT_FIELD_CURRENCY:
		if (start_field_type == EDIT_FIELD_CURRENCY) {
			transfer->currency.amount = windat->transactions[start_transaction].amount;
			changed = TRUE;
		}
		break;
	case EDIT_FIELD_DATE:
		if (start_field_type == EDIT_FIELD_DATE) {
			transfer->date.date = windat->transactions[start_transaction].date;
			changed = TRUE;
		}
		break;
	case EDIT_FIELD_ACCOUNT_IN:
	case EDIT_FIELD_ACCOUNT_OUT:
		target_type = (transfer->type == EDIT_FIELD_ACCOUNT_IN) ? ACCOUNT_FULL_IN : ACCOUNT_FULL_OUT;
	
		if (start_field_type == EDIT_FIELD_ACCOUNT_IN || start_field_type == EDIT_FIELD_ACCOUNT_OUT) {
			switch (drag_data->start_column) {
			case TRANSACT_ICON_FROM:
			case TRANSACT_ICON_FROM_REC:
			case TRANSACT_ICON_FROM_NAME:
				if (account_get_type(windat->file, windat->transactions[start_transaction].from) & target_type) {
					transfer->account.account = windat->transactions[start_transaction].from;
					transfer->account.reconciled = (windat->transactions[start_transaction].flags & TRANS_REC_FROM) ? TRUE : FALSE;
					changed = TRUE;
				}
				break;
			case TRANSACT_ICON_TO:
			case TRANSACT_ICON_TO_REC:
			case TRANSACT_ICON_TO_NAME:
				if (account_get_type(windat->file, windat->transactions[start_transaction].to) & target_type) {
					transfer->account.account = windat->transactions[start_transaction].to;
					transfer->account.reconciled = (windat->transactions[start_transaction].flags & TRANS_REC_TO) ? TRUE : FALSE;
					changed = TRUE;
				}
				break;
			}
		}
		break;
	case EDIT_FIELD_DISPLAY:
	case EDIT_FIELD_NONE:
		break;
	}

	edit_submit_field_contents_update(windat->edit_line, transfer, changed);
}


/* ==================================================================================================================
 * Transaction handling
 */

/**
 * Adds a new transaction to the end of the list, using the details supplied.
 *
 * \param *file			The file to add the transaction to.
 * \param date			The date of the transaction, or NULL_DATE.
 * \param from			The account to transfer from, or NULL_ACCOUNT.
 * \param to			The account to transfer to, or NULL_ACCOUNT.
 * \param flags			The transaction flags.
 * \param amount		The amount to transfer, or NULL_CURRENCY.
 * \param *ref			Pointer to the transaction reference, or NULL.
 * \param *description		Pointer to the transaction description, or NULL.
 */

void transact_add_raw_entry(struct file_block *file, date_t date, acct_t from, acct_t to, enum transact_flags flags,
		amt_t amount, char *ref, char *description)
{
	int new;

	if (file == NULL || file->transacts == NULL)
		return;

	if (!flexutils_resize((void **) &(file->transacts->transactions), sizeof(struct transaction), file->transacts->trans_count + 1)) {
		error_msgs_report_error("NoMemNewTrans");
		return;
	}

	new = file->transacts->trans_count++;

	file->transacts->transactions[new].date = date;
	file->transacts->transactions[new].amount = amount;
	file->transacts->transactions[new].from = from;
	file->transacts->transactions[new].to = to;
	file->transacts->transactions[new].flags = flags;
	string_copy(file->transacts->transactions[new].reference, (ref != NULL) ? ref : "", TRANSACT_REF_FIELD_LEN);
	string_copy(file->transacts->transactions[new].description, (description != NULL) ? description : "", TRANSACT_DESCRIPT_FIELD_LEN);
	file->transacts->transactions[new].sort_index = new;

	file_set_data_integrity(file, TRUE);
	if (date != NULL_DATE)
		file->transacts->date_sort_valid = FALSE;
}


/**
 * Clear a transaction from a file, returning it to an empty state. Note that
 * the transaction remains in-situ, and no memory is cleared. It will be
 * necessary to subsequently call transact_strip_blanks() to free the memory.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to be cleared.
 */

void transact_clear_raw_entry(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	file->transacts->transactions[transaction].date = NULL_DATE;
	file->transacts->transactions[transaction].from = NULL_ACCOUNT;
	file->transacts->transactions[transaction].to = NULL_ACCOUNT;
	file->transacts->transactions[transaction].flags = TRANS_FLAGS_NONE;
	file->transacts->transactions[transaction].amount = NULL_CURRENCY;
	*file->transacts->transactions[transaction].reference = '\0';
	*file->transacts->transactions[transaction].description = '\0';

	file->transacts->date_sort_valid = FALSE;
}


/**
 * Test to see if a transaction entry in a file is completely empty.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to be tested.
 * \return			TRUE if the transaction is empty; FALSE if not.
 */

static osbool transact_is_blank(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return FALSE;

	return (file->transacts->transactions[transaction].date == NULL_DATE &&
			file->transacts->transactions[transaction].from == NULL_ACCOUNT &&
			file->transacts->transactions[transaction].to == NULL_ACCOUNT &&
			file->transacts->transactions[transaction].amount == NULL_CURRENCY &&
			*file->transacts->transactions[transaction].reference == '\0' &&
			*file->transacts->transactions[transaction].description == '\0') ? TRUE : FALSE;
}


/**
 * Strip any blank transactions from the end of the file, releasing any memory
 * associated with them. To be sure to remove all blank transactions, it is
 * necessary to sort the transaction list before calling this function.
 *
 * \param *file			The file to be processed.
 */

void transact_strip_blanks_from_end(struct file_block *file)
{
	tran_t	transaction;
	int	line, old_count;
	osbool	found;


	if (file == NULL || file->transacts == NULL)
		return;

	transaction = file->transacts->trans_count - 1;

	while (transact_is_blank(file, transaction)) {

		/* Search the transaction sort index to find the line pointing
		 * to the current blank. If one is found, shuffle all of the
		 * following indexes up a space to compensate.
		 */

		found = FALSE;

		for (line = 0; line < transaction; line++) {
			if (file->transacts->transactions[line].sort_index == transaction)
				found = TRUE;

			if (found == TRUE)
				file->transacts->transactions[line].sort_index = file->transacts->transactions[line + 1].sort_index;
		}

		/* Remove the transaction. */

		transaction--;
	}

	/* If any transactions were removed, free up the unneeded memory
	 * from the end of the file.
	 */

	if (transaction < file->transacts->trans_count - 1) {
		old_count = file->transacts->trans_count - 1;
		file->transacts->trans_count = transaction + 1;

		if (!flexutils_resize((void **) &(file->transacts->transactions), sizeof(struct transaction), file->transacts->trans_count))
			error_msgs_report_error("BadDelete");

		transact_force_window_redraw(file->transacts, 0, old_count, TRANSACT_PANE_ROW);
	}
}


/**
 * Return the date of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the date for.
 * \return			The date of the transaction, or NULL_DATE.
 */

date_t transact_get_date(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_DATE;

	return file->transacts->transactions[transaction].date;
}


/**
 * Return the from account of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the from account for.
 * \return			The from account of the transaction, or NULL_ACCOUNT.
 */

acct_t transact_get_from(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_ACCOUNT;

	return file->transacts->transactions[transaction].from;
}


/**
 * Return the to account of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the to account for.
 * \return			The to account of the transaction, or NULL_ACCOUNT.
 */

acct_t transact_get_to(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_ACCOUNT;

	return file->transacts->transactions[transaction].to;
}


/**
 * Return the transaction flags for a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the flags for.
 * \return			The flags of the transaction, or TRANS_FLAGS_NONE.
 */

enum transact_flags transact_get_flags(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return TRANS_FLAGS_NONE;

	return file->transacts->transactions[transaction].flags;
}


/**
 * Return the amount of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the amount of.
 * \return			The amount of the transaction, or NULL_CURRENCY.
 */

amt_t transact_get_amount(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL_CURRENCY;

	return file->transacts->transactions[transaction].amount;
}


/**
 * Return the reference for a transaction.
 *
 * If a buffer is supplied, the reference is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * reference in the transaction array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the reference of.
 * \param *buffer		Pointer to a buffer to take the reference, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting reference string,
 *				either the supplied buffer or the original.
 */

char *transact_get_reference(struct file_block *file, tran_t transaction, char *buffer, size_t length)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return NULL;

	if (buffer == NULL || length == 0)
		return file->transacts->transactions[transaction].reference;

	string_copy(buffer, file->transacts->transactions[transaction].reference, length);

	return buffer;
}


/**
 * Return the description for a transaction.
 *
 * If a buffer is supplied, the description is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * description in the transaction array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the description of.
 * \param *buffer		Pointer to a buffer to take the description, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting description string,
 *				either the supplied buffer or the original.
 */

char *transact_get_description(struct file_block *file, tran_t transaction, char *buffer, size_t length)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction)) {
		if (buffer != NULL && length > 0) {
			*buffer = '\0';
			return buffer;
		}

		return NULL;
	}

	if (buffer == NULL || length == 0)
		return file->transacts->transactions[transaction].description;

	string_copy(buffer, file->transacts->transactions[transaction].description, length);

	return buffer;
}


/**
 * Return the sort workspace for a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the workspace of.
 * \return			The sort workspace for the transaction, or 0.
 */

int transact_get_sort_workspace(struct file_block *file, tran_t transaction)
{
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return 0;

	return file->transacts->transactions[transaction].sort_workspace;
}


/**
 * Test the validity of a transaction index.
 *
 * \param *file			The file to test against.
 * \param transaction		The transaction index to test.
 * \return			TRUE if the index is valid; FALSE if not.
 */

osbool transact_test_index_valid(struct file_block *file, tran_t transaction)
{
	return (transact_valid(file->transacts, transaction)) ? TRUE : FALSE;
}


/**
 * Get the underlying transaction number relating to the current edit line
 * position.
 *
 * \param *file			The file that we're interested in.
 * \return			The transaction number, or NULL_TRANSACTION if the
 *				line isn't in the specified file.
 */

static tran_t transact_find_edit_line_by_transaction(struct transact_block *windat)
{
	int	line;

	if (windat == NULL || windat->transactions == NULL)
		return NULL_TRANSACTION;

	line = edit_get_line(windat->edit_line);
	if (!transact_valid(windat, line))
		return NULL_TRANSACTION;

	return windat->transactions[line].sort_index;
}


/**
 * Place a new edit line by raw transaction number.
 *
 * \param *windat		The transaction window to place the line in.
 * \param transaction		The transaction to place the line on.
 */

static void transact_place_edit_line_by_transaction(struct transact_block *windat, tran_t transaction)
{
	int		i, line;

	if (windat == NULL)
		return;

	line = windat->trans_count;

	if (transaction != NULL_TRANSACTION) {
		for (i = 0; i < windat->trans_count; i++) {
			if (windat->transactions[i].sort_index == transaction) {
				line = i;
				break;
			}
		}
	}

	transact_place_edit_line(windat, line);

	transact_find_edit_line_vertically(windat);
}


/**
 * Callback to allow the edit line to move.
 *
 * \param line			The line in which to place the edit line.
 * \param *data			Client data: windat.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool transact_edit_place_line(int line, void *data)
{
	struct transact_block	*windat = data;

	if (windat == NULL)
		return FALSE;

	transact_place_edit_line(windat, line);
	transact_find_edit_line_vertically(windat);

	return TRUE;
}


/**
 * Place a new edit line in a transaction window by visible line number,
 * extending the window if required.
 *
 * \param *windat		The transaction window to place the line in.
 * \param line			The line to place.
 */

static void transact_place_edit_line(struct transact_block *windat, int line)
{
	wimp_colour	colour;

	if (windat == NULL)
		return;

	if (line >= windat->display_lines) {
		windat->display_lines = line + 1;
		transact_set_window_extent(windat->file);
	}

	colour = transact_line_colour(windat, line);
	edit_place_new_line(windat->edit_line, line, colour);
}


/**
 * Bring the edit line into view in a vertical direction within a transaction
 * window.
 *
 * \param *windat		The transaction window to be updated.
 */

static void transact_find_edit_line_vertically(struct transact_block *windat)
{
	wimp_window_state	window;
	int			height, top, bottom, line;


	if (windat == NULL || windat->file == NULL || windat->transaction_window == NULL || !edit_get_active(windat->edit_line))
		return;

	window.w = windat->transaction_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
	 * This will allow the edit line to be aligned with the top or bottom of the window.
	 */

	height = window.visible.y1 - window.visible.y0 - WINDOW_ROW_HEIGHT - TRANSACT_TOOLBAR_HEIGHT;

	/* Calculate the top full line and bottom full line that are showing in the window.  Part lines don't
	 * count and are discarded.
	 */

	top = (-window.yscroll + WINDOW_ROW_ICON_HEIGHT) / WINDOW_ROW_HEIGHT;
	bottom = (height / WINDOW_ROW_HEIGHT) + top;

	/* If the edit line is above or below the visible area, bring it into range. */

	line = edit_get_line(windat->edit_line);
	if (line < 0)
		return;

#ifdef DEBUG
	debug_printf("\\BFind transaction edit line");
	debug_printf("Top: %d, Bottom: %d, Entry line: %d", top, bottom, line);
#endif

	if (line < top) {
		window.yscroll = -(line * WINDOW_ROW_HEIGHT);
		wimp_open_window((wimp_open *) &window);
		transact_minimise_window_extent(windat->file);
	}

	if (line > bottom) {
		window.yscroll = -(line * WINDOW_ROW_HEIGHT - height);
		wimp_open_window((wimp_open *) &window);
		transact_minimise_window_extent(windat->file);
	}
}


/**
 * Handle requests from the edit line to bring a given line and field into
 * view in the visible area of the window.
 *
 * \param line			The line in the window to be brought into view.
 * \param xmin			The minimum X coordinate to be shown.
 * \param xmax			The maximum X coordinate to be shown.
 * \param target		The target requirement: left edge, right edge or centred.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE if handled successfully; FALSE if not.
 */

static osbool transact_edit_find_field(int line, int xmin, int xmax, enum edit_align target, void *data)
{
	struct transact_block	*windat = data;
	wimp_window_state	window;
	int			icon_width, window_width, window_xmin, window_xmax;

	if (windat == NULL)
		return FALSE;

	window.w = windat->transaction_window;
	wimp_get_window_state(&window);

	icon_width = xmax - xmin;

	/* Establish the window dimensions. */

	window_width = window.visible.x1 - window.visible.x0;
	window_xmin = window.xscroll;
	window_xmax = window.xscroll + window_width;

	if (window_width > icon_width) {
		/* If the icon group fits into the visible window, just pull the overlap into view. */

		if (xmin < window_xmin) {
			window.xscroll = xmin;
			wimp_open_window((wimp_open *) &window);
		} else if (xmax > window_xmax) {
			window.xscroll = xmax - window_width;
			wimp_open_window((wimp_open *) &window);
		}
	} else {
		/* If the icon is bigger than the window, however, align the target with the edge of the window. */

		switch (target) {
		case EDIT_ALIGN_LEFT:
		case EDIT_ALIGN_CENTRE:
			if (xmin < window_xmin || xmin > window_xmax) {
				window.xscroll = xmin;
				wimp_open_window((wimp_open *) &window);
			}
			break;
		case EDIT_ALIGN_RIGHT:
			if (xmax < window_xmin || xmax > window_xmax) {
				window.xscroll = xmax - window_width;
				wimp_open_window((wimp_open *) &window);
			}
			break;
		case EDIT_ALIGN_NONE:
			break;
		}
	}

	/* Make sure that the line is visible vertically, as well.
	 * NB: This currently ignores the line parameter, as transact_find_edit_line_vertically()
	 * queries the edit line directly. At present, these both yield the same result.
	 */

	transact_find_edit_line_vertically(windat);

	return TRUE;
}


/**
 * Inform the edit line about the location of the first blank transaction
 * line in our window.
 *
 * \param *data			Our client data, holding the window instance.
 * \return			The line number of the first blank line, or -1.
 */

static int transact_edit_first_blank_line(void *data)
{
	struct transact_block	*windat = data;

	if (windat == NULL || windat->file == NULL)
		return -1;

	return transact_find_first_blank_line(windat->file);
}


/**
 * Find and return the line number of the first blank line in a file, based on
 * display order.
 *
 * \param *file			The file to search.
 * \return			The first blank display line.
 */

int transact_find_first_blank_line(struct file_block *file)
{
	int line;

	if (file == NULL || file->transacts == NULL)
		return 0;

	#ifdef DEBUG
	debug_printf("\\DFinding first blank line");
	#endif

	line = file->transacts->trans_count;

	while (line > 0 && transact_is_blank(file, file->transacts->transactions[line - 1].sort_index)) {
		line--;

		#ifdef DEBUG
		debug_printf("Stepping back up...");
		#endif
	}

	return line;
}


/**
 * Inform the edit line whether a given line in the window contains a valid
 * transaction.
 *
 * \param line			The line in the window to be tested.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE if valid; FALSE if not or on error.
 */

static osbool transact_edit_test_line(int line, void *data)
{
	struct transact_block	*windat = data;

	if (windat == NULL)
		return FALSE;

	return (transact_valid(windat, line)) ? TRUE : FALSE;
}


/**
 * Change the date for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param new_date	The new date to set the transaction to.
 */

void transact_change_date(struct file_block *file, tran_t transaction, date_t new_date)
{
	int	line;
	osbool	changed = FALSE;
	date_t	old_date = NULL_DATE;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	account_remove_transaction(file, transaction);

	/* Look up the existing date, change it and compare the two. If the field
	 * has changed, flag this up.
	 */

	old_date = file->transacts->transactions[transaction].date;

	file->transacts->transactions[transaction].date = new_date;

	if (old_date != file->transacts->transactions[transaction].date) {
		changed = TRUE;
		file->transacts->date_sort_valid = FALSE;
	}

	/* Return the line to the calculations. This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* If any changes were made, refresh the relevant account listings, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == TRUE) {
		/* Ideally, we would want to recalculate just the affected two
		 * accounts.  However, because the date sort is unclean, any rebuild
		 * will force a resort of the transactions, which will require a
		 * full rebuild of all the open account views. Therefore, call
		 * accview_recalculate_all() to force a full recalculation. This
		 * will in turn sort the data if required.
		 *
		 * The big assumption here is that, because no from or to entries
		 * have changed, none of the accounts will change length and so a
		 * full rebuild is not required.
		 */

		accview_recalculate_all(file);

		/* Force a redraw of the affected line. */

		line = transact_get_line_from_transaction(file, transaction);
		transact_force_window_redraw(file->transacts, line, line, wimp_ICON_WINDOW);

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the from or to account associated with a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param target	The target field to change.
 * \param new_account	The new account to set the field to.
 */

void transact_change_account(struct file_block *file, tran_t transaction, enum transact_field target, acct_t new_account)
{
	int		line;
	osbool		changed = FALSE;
	unsigned	old_flags;
	acct_t		old_acct = NULL_ACCOUNT;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	account_remove_transaction(file, transaction);

	/* Update the reconcile flag, either removing it, or adding it in. If the
	 * line is the edit line, the icon contents must be manually updated as well.
	 *
	 * If a change is made, this is flagged to allow the update to be recorded properly.
	 */

	/* Look up the account ident as it stands, store the result and
	 * update the name field.  The reconciled flag is set if the
	 * account changes to an income heading; else it is cleared.
	 */

	switch (target) {
	case TRANSACT_FIELD_FROM:
		old_acct = file->transacts->transactions[transaction].from;
		old_flags = file->transacts->transactions[transaction].flags;

		file->transacts->transactions[transaction].from = new_account;

		if (account_get_type(file, new_account) == ACCOUNT_FULL)
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_FROM;
		else
			file->transacts->transactions[transaction].flags |= TRANS_REC_FROM;

		if (old_acct != file->transacts->transactions[transaction].from || old_flags != file->transacts->transactions[transaction].flags)
			changed = TRUE;
		break;
	case TRANSACT_FIELD_TO:
		old_acct = file->transacts->transactions[transaction].to;
		old_flags = file->transacts->transactions[transaction].flags;

		file->transacts->transactions[transaction].to = new_account;

		if (account_get_type(file, new_account) == ACCOUNT_FULL)
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_TO;
		else
			file->transacts->transactions[transaction].flags |= TRANS_REC_TO;

		if (old_acct != file->transacts->transactions[transaction].to || old_flags != file->transacts->transactions[transaction].flags)
			changed = TRUE;
		break;
	case TRANSACT_FIELD_ROW:
	case TRANSACT_FIELD_DATE:
	case TRANSACT_FIELD_REF:
	case TRANSACT_FIELD_AMOUNT:
	case TRANSACT_FIELD_DESC:
	case TRANSACT_FIELD_NONE:
		break;
	}

	/* Return the line to the calculations. This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* Trust that any account views that are open must be based on a valid
	 * date order, and only rebuild those that are directly affected.
         */

	/* If any changes were made, refresh the relevant account listing, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == FALSE)
		return;

	switch (target) {
	case TRANSACT_FIELD_FROM:
		accview_rebuild(file, old_acct);
		accview_rebuild(file, file->transacts->transactions[transaction].from);
		accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);
		break;
	case TRANSACT_FIELD_TO:
		accview_rebuild(file, old_acct);
		accview_rebuild(file, file->transacts->transactions[transaction].to);
		accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
		break;
	case TRANSACT_FIELD_ROW:
	case TRANSACT_FIELD_DATE:
	case TRANSACT_FIELD_REF:
	case TRANSACT_FIELD_AMOUNT:
	case TRANSACT_FIELD_DESC:
	case TRANSACT_FIELD_NONE:
		break;
	}

	/* Force a redraw of the affected line. */

	line = transact_get_line_from_transaction(file, transaction);
	transact_force_window_redraw(file->transacts, line, line, wimp_ICON_WINDOW);

	file_set_data_integrity(file, TRUE);
}


/**
 * Toggle the state of one of the reconciled flags for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param change_flag	Indicate which reconciled flags to change.
 */

void transact_toggle_reconcile_flag(struct file_block *file, tran_t transaction, enum transact_flags change_flag)
{
	int	line;
	osbool	changed = FALSE;


	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	/* Only do anything if the transaction is inside the limit of the file. */

	account_remove_transaction(file, transaction);

	/* Update the reconcile flag, either removing it, or adding it in.  If the
	 * line is the edit line, the icon contents must be manually updated as well.
	 *
	 * If a change is made, this is flagged to allow the update to be recorded properly.
	 */

	if (file->transacts->transactions[transaction].flags & change_flag) {
		file->transacts->transactions[transaction].flags &= ~change_flag;
		changed = TRUE;
	} else if ((change_flag == TRANS_REC_FROM && file->transacts->transactions[transaction].from != NULL_ACCOUNT) ||
			(change_flag == TRANS_REC_TO && file->transacts->transactions[transaction].to != NULL_ACCOUNT)) {
		file->transacts->transactions[transaction].flags |= change_flag;
		changed = TRUE;
	}

	/* Return the line to the calculations. This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* If any changes were made, refresh the relevant account listing, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == TRUE) {
		if (change_flag == TRANS_REC_FROM)
			accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
		else
			accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);

		/* Force a redraw of the affected line. */

		line = transact_get_line_from_transaction(file, transaction);
		transact_force_window_redraw(file->transacts, line, line, wimp_ICON_WINDOW);

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the amount of money for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param new_amount	The new amount to set the transaction to.
 */

void transact_change_amount(struct file_block *file, tran_t transaction, amt_t new_amount)
{
	int	line;
	osbool	changed = FALSE;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	account_remove_transaction(file, transaction);

	/* Look up the existing date, change it and compare the two. If the field
	 * has changed, flag this up.
	 */

	if (new_amount != file->transacts->transactions[transaction].amount) {
		changed = TRUE;
		file->transacts->transactions[transaction].amount = new_amount;
	}

	/* Return the line to the calculations.   This will automatically update all
	 * the account listings.
	  */

	account_restore_transaction(file, transaction);

	/* If any changes were made, refresh the relevant account listings, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == TRUE) {
		accview_recalculate(file, file->transacts->transactions[transaction].from, transaction);
		accview_recalculate(file, file->transacts->transactions[transaction].to, transaction);

		/* Force a redraw of the affected line. */

		line = transact_get_line_from_transaction(file, transaction);
		transact_force_window_redraw(file->transacts, line, line, wimp_ICON_WINDOW);

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the reference or description associated with a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param target	The target field to change.
 * \param new_text	The new text to set the field to.
 */

void transact_change_refdesc(struct file_block *file, tran_t transaction, enum transact_field target, char *new_text)
{
	int	line;
	osbool	changed = FALSE;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	/* Find the field that will be getting changed. */

	switch (target) {
	case TRANSACT_FIELD_REF:
		if (strcmp(file->transacts->transactions[transaction].reference, new_text) == 0)
			break;

		string_copy(file->transacts->transactions[transaction].reference, new_text, TRANSACT_REF_FIELD_LEN);
		changed = TRUE;
		break;

	case TRANSACT_FIELD_DESC:
		if (strcmp(file->transacts->transactions[transaction].description, new_text) == 0)
			break;

		string_copy(file->transacts->transactions[transaction].description, new_text, TRANSACT_DESCRIPT_FIELD_LEN);
		changed = TRUE;
		break;

	default:
		break;
	}

	/* If any changes were made, refresh the relevant account listings, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == FALSE)
		return;

	/* Refresh any account views that may be affected. */

	accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
	accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);

	/* Force a redraw of the affected line. */

	line = transact_get_line_from_transaction(file, transaction);
	transact_force_window_redraw(file->transacts, line, line, wimp_ICON_WINDOW);

	file_set_data_integrity(file, TRUE);
}


/**
 * Handle requests for field data from the edit line.
 *
 * \param *data			The block to hold the requested data.
 * \return			TRUE if valid data was returned; FALSE if not.
 */

static osbool transact_edit_get_field(struct edit_data *data)
{
	tran_t			t;
	struct transact_block	*windat;

	if (data == NULL || data->data == NULL)
		return FALSE;

	windat = data->data;

	if (!transact_valid(windat, data->line))
		return FALSE;

	t = windat->transactions[data->line].sort_index;

	switch (data->icon) {
	case TRANSACT_ICON_ROW:
		if (data->type != EDIT_FIELD_DISPLAY || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		string_printf(data->display.text, data->display.length, "%d", transact_get_transaction_number(t));
		break;
	case TRANSACT_ICON_DATE:
		if (data->type != EDIT_FIELD_DATE)
			return FALSE;

		data->date.date = windat->transactions[t].date;
		break;
	case TRANSACT_ICON_FROM:
		if (data->type != EDIT_FIELD_ACCOUNT_IN)
			return FALSE;

		data->account.account = windat->transactions[t].from;
		data->account.reconciled = (windat->transactions[t].flags & TRANS_REC_FROM) ? TRUE : FALSE;
		break;
	case TRANSACT_ICON_TO:
		if (data->type != EDIT_FIELD_ACCOUNT_OUT)
			return FALSE;

		data->account.account = windat->transactions[t].to;
		data->account.reconciled = (windat->transactions[t].flags & TRANS_REC_TO) ? TRUE : FALSE;
		break;
	case TRANSACT_ICON_AMOUNT:
		if (data->type != EDIT_FIELD_CURRENCY)
			return FALSE;

		data->currency.amount = windat->transactions[t].amount;
		break;
	case TRANSACT_ICON_REFERENCE:
		if (data->type != EDIT_FIELD_TEXT || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		string_copy(data->text.text, windat->transactions[t].reference, data->text.length);
		break;
	case TRANSACT_ICON_DESCRIPTION:
		if (data->type != EDIT_FIELD_TEXT || data->display.text == NULL || data->display.length == 0)
			return FALSE;

		string_copy(data->text.text, windat->transactions[t].description, data->text.length);
		break;
	}

	return TRUE;
}


/**
 * Handle returned field data from the edit line.
 *
 * \param *data			The block holding the returned data.
 * \return			TRUE if valid data was returned; FALSE if not.
 */

static osbool transact_edit_put_field(struct edit_data *data)
{
	struct transact_block	*windat;
	int			start, i;
	tran_t			transaction;
	acct_t			old_account;
	osbool			changed;

#ifdef DEBUG
	debug_printf("Returning complex data for icon %d in row %d", data->icon, data->line);
#endif

	if (data == NULL)
		return FALSE;

	windat = data->data;
	if (windat == NULL || windat->file == NULL)
		return FALSE;

	/* If there is not a transaction entry for the current edit line location
	 * (ie. if this is the first keypress in a new line), extend the transaction
	 * entries to reach the current location.
	 */

	if (data->line >= windat->trans_count) {
		start = windat->trans_count;
		
		for (i = windat->trans_count; i <= data->line; i++)
			transact_add_raw_entry(windat->file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

		edit_refresh_line_contents(windat->edit_line, TRANSACT_ICON_ROW, wimp_ICON_WINDOW);
		transact_force_window_redraw(windat, start, windat->trans_count - 1, TRANSACT_PANE_ROW);
	}

	/* Get out if we failed to create the necessary transactions. */

	if (data->line >= windat->trans_count)
		return FALSE;

	transaction = windat->transactions[data->line].sort_index;

	/* Take the transaction out of the fully calculated results.
	 *
	 * Presets occur with the caret in the Date column, so they will have the
	 * transaction correctly removed before anything happens.
	 */

	if (data->icon != TRANSACT_ICON_REFERENCE && data->icon != TRANSACT_ICON_DESCRIPTION)
		account_remove_transaction(windat->file, transaction);

	/* Process the supplied data. */

	changed = FALSE;
	old_account = NULL_ACCOUNT;

	switch (data->icon) {
	case TRANSACT_ICON_DATE:
		windat->transactions[transaction].date = data->date.date;
		changed = TRUE;
		windat->date_sort_valid = FALSE;
		break;
	case TRANSACT_ICON_FROM:
		old_account = windat->transactions[transaction].from;
		windat->transactions[transaction].from = data->account.account;
		if (data->account.reconciled == TRUE)
			windat->transactions[transaction].flags |= TRANS_REC_FROM;
		else
			windat->transactions[transaction].flags &= ~TRANS_REC_FROM;

		edit_set_line_colour(windat->edit_line, transact_line_colour(windat, data->line));
		changed = TRUE;
		break;
	case TRANSACT_ICON_TO:
		old_account = windat->transactions[transaction].to;
		windat->transactions[transaction].to = data->account.account;
		if (data->account.reconciled == TRUE)
			windat->transactions[transaction].flags |= TRANS_REC_TO;
		else
			windat->transactions[transaction].flags &= ~TRANS_REC_TO;

		edit_set_line_colour(windat->edit_line, transact_line_colour(windat, data->line));
		changed = TRUE;
		break;
	case TRANSACT_ICON_AMOUNT:
		windat->transactions[transaction].amount = data->currency.amount;
		changed = TRUE;
		break;
	case TRANSACT_ICON_REFERENCE:
		string_copy(windat->transactions[transaction].reference, data->text.text, TRANSACT_REF_FIELD_LEN);
		changed = TRUE;
		break;
	case TRANSACT_ICON_DESCRIPTION:
		string_copy(windat->transactions[transaction].description, data->text.text, TRANSACT_DESCRIPT_FIELD_LEN);
		changed = TRUE;
		break;
	}

	/* Add the transaction back into the accounts calculations.
	 *
	 * From this point on, it is now OK to change the sort order of the
	 * transaction data again!
	 */

	if (data->icon != TRANSACT_ICON_REFERENCE && data->icon != TRANSACT_ICON_DESCRIPTION)
		account_restore_transaction(windat->file, transaction);

	/* Mark the data as unsafe and perform any post-change recalculations that
	 * may affect the order of the transaction data.
	 */

	if (changed == TRUE) {
		file_set_data_integrity(windat->file, TRUE);
		if (data->icon == TRANSACT_ICON_DATE) {
			/* Ideally, we would want to recalculate just the affected two
			 * accounts.  However, because the date sort is unclean, any rebuild
			 * will force a resort of the transactions, which will require a
			 * full rebuild of all the open account views.  Therefore, call
			 * accview_recalculate_all() to force a full recalculation.  This
			 * will in turn sort the data if required.
			 *
			 * The big assumption here is that, because no from or to entries
			 * have changed, none of the accounts will change length and so a
			 * full rebuild is not required.
			 */

			accview_recalculate_all(windat->file);
		} else if (data->icon == TRANSACT_ICON_FROM) {
			/* Trust that any account views that are open must be based on a
			 * valid date order, and only rebuild those that are directly affected.
			 * The rebuild might resort the transaction data, so we need to re-find
			 * the transaction entry for the edit line each time.
			 */

			accview_rebuild(windat->file, old_account);
			transaction = windat->transactions[data->line].sort_index;

			accview_rebuild(windat->file, windat->transactions[transaction].from);
			transaction = windat->transactions[data->line].sort_index;

			accview_redraw_transaction(windat->file, windat->transactions[transaction].to, transaction);
		} else if (data->icon == TRANSACT_ICON_TO) {
			/* Trust that any account views that are open must be based on a
			 * valid date order, and only rebuild those that are directly affected.
			 * The rebuild might resort the transaction data, so we need to re-find
			 * the transaction entry for the edit line each time.
			 */

			accview_rebuild(windat->file, old_account);
			transaction = windat->transactions[data->line].sort_index;

			accview_rebuild(windat->file, windat->transactions[transaction].to);
			transaction = windat->transactions[data->line].sort_index;

			accview_redraw_transaction(windat->file, windat->transactions[transaction].from, transaction);
		} else if (data->icon == TRANSACT_ICON_AMOUNT) {
			accview_recalculate(windat->file, windat->transactions[transaction].from, transaction);
			accview_recalculate(windat->file, windat->transactions[transaction].to, transaction);
		} else if (data->icon == TRANSACT_ICON_REFERENCE || data->icon == TRANSACT_ICON_DESCRIPTION) {
			accview_redraw_transaction(windat->file, windat->transactions[transaction].from, transaction);
			accview_redraw_transaction(windat->file, windat->transactions[transaction].to, transaction);
		}
	}

	/* Finally, look for the next reconcile line if that is necessary.
	 *
	 * This is done last, as the only hold we have over the line being edited
	 * is the edit line location.  Move that and we've lost everything.
	 */

	if ((data->icon == TRANSACT_ICON_FROM || data->icon == TRANSACT_ICON_TO) &&
			(data->key == '+' || data->key == '=' || data->key == '-' || data->key == '_'))
		transact_find_next_reconcile_line(windat, FALSE);

	return TRUE;
}


/**
 * Find the next line of an account, based on its reconcoled status, and place
 * the caret into the unreconciled account field.
 *
 * \param *windat		The transaction window instance to search in.
 * \param set			TRUE to match reconciled lines; FALSE to match unreconciled ones.
 */

static void transact_find_next_reconcile_line(struct transact_block *windat, osbool set)
{
	int			line;
	acct_t			account;
	enum transact_field	found;
	wimp_caret		caret;

	if (windat == NULL || windat->file == NULL || windat->auto_reconcile == FALSE)
		return;

	line = edit_get_line(windat->edit_line);
	account = NULL_ACCOUNT;

	wimp_get_caret_position(&caret);

	if (caret.i == TRANSACT_ICON_FROM)
		account = windat->transactions[windat->transactions[line].sort_index].from;
	else if (caret.i == TRANSACT_ICON_TO)
		account = windat->transactions[windat->transactions[line].sort_index].to;

	if (account == NULL_ACCOUNT)
		return;

	line++;
	found = TRANSACT_FIELD_NONE;

	while ((line < windat->trans_count) && (found == TRANSACT_FIELD_NONE)) {
		if (windat->transactions[windat->transactions[line].sort_index].from == account &&
				((windat->transactions[windat->transactions[line].sort_index].flags & TRANS_REC_FROM) ==
						((set) ? TRANS_REC_FROM : TRANS_FLAGS_NONE)))
			found = TRANSACT_FIELD_FROM;
		else if (windat->transactions[windat->transactions[line].sort_index].to == account &&
				((windat->transactions[windat->transactions[line].sort_index].flags & TRANS_REC_TO) ==
						((set) ? TRANS_REC_TO : TRANS_FLAGS_NONE)))
			found = TRANSACT_FIELD_TO;
		else
			line++;
	}

	if (found != TRANSACT_FIELD_NONE)
		transact_place_caret(windat->file, line, found);
}


/**
 * Process auto-complete requests from the edit line for one of the transaction
 * fields.
 *
 * \param *data			The field auto-complete data.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool transact_edit_auto_complete(struct edit_data *data)
{
	struct transact_block	*windat;
	tran_t			transaction;

	if (data == NULL)
		return FALSE;

	windat = data->data;
	if (windat == NULL || windat->file == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("Requesting auto-completion");
#endif

	/* We can only complete text fields at the moment, as none of the others make sense. */

	if (data->type != EDIT_FIELD_TEXT)
		return FALSE;

	/* Process the Reference or Descripton field as appropriate. */

	switch (data->icon) {
	case TRANSACT_ICON_REFERENCE:
		/* To complete the reference, we need to be in a valid transaction which contains
		 * at least one of the From or To fields.
		 */

		if (data->line >= windat->trans_count)
			return FALSE;

		transaction = windat->transactions[data->line].sort_index;

		if (!transact_valid(windat, transaction))
			return FALSE;

		account_get_next_cheque_number(windat->file, windat->transactions[transaction].from, windat->transactions[transaction].to,
				1, data->text.text, data->text.length);
		break;

	case TRANSACT_ICON_DESCRIPTION:
		/* The description field can be completed whether or not there's an underlying
		 * transaction, as we just search back up from the last valid line.
		 */

		transact_complete_description(windat->file, data->line, data->text.text, data->text.length);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}


/**
 * Complete a description field, by finding the most recent description in the file
 * which starts with the same characters as the current line.
 *
 * \param *file		The file containing the transaction.
 * \param line		The transaction line to be completed.
 * \param *buffer	Pointer to the buffer to be completed.
 * \param length	The length of the buffer.
 * \return		Pointer to the completed buffer.
 */

static char *transact_complete_description(struct file_block *file, int line, char *buffer, size_t length)
{
	tran_t	t;

	if (file == NULL || file->transacts == NULL || buffer == NULL)
		return buffer;

	if (line >= file->transacts->trans_count)
		line = file->transacts->trans_count - 1;
	else
		line -= 1;

	for (; line >= 0; line--) {
		t = file->transacts->transactions[line].sort_index;

		if (*(file->transacts->transactions[t].description) != '\0' &&
				string_nocase_strstr(file->transacts->transactions[t].description, buffer) == file->transacts->transactions[t].description) {
			string_copy(buffer, file->transacts->transactions[t].description, length);
			break;
		}
	}

	return buffer;
}


/**
 * Process preset insertion requests from the edit line.
 *
 * \param line			The line at which to insert the preset.
 * \param key			The Wimp Key number triggering the request.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool transact_edit_insert_preset(int line, wimp_key_no key, void *data)
{
	struct transact_block	*windat;
	preset_t		preset;

	if (data == NULL)
		return FALSE;

	windat = data;
	if (windat == NULL || windat->file == NULL)
		return FALSE;

	/* Identify the preset to be inserted. */

	preset = preset_find_from_keypress(windat->file, toupper(key));

	if (preset == NULL_PRESET)
		return TRUE;

	return transact_insert_preset_into_line(windat->file, line, preset);
}


/**
 * Insert a preset into a pre-existing transaction, taking care of updating all
 * the file data in a clean way.
 *
 * \param *file		The file to edit.
 * \param line		The line in the transaction window to update.
 * \param preset	The preset to insert into the transaction.
 * \return		TRUE if successful; FALSE on failure.
 */

osbool transact_insert_preset_into_line(struct file_block *file, int line, preset_t preset)
{
	enum transact_field	changed = TRANSACT_FIELD_NONE;
	tran_t			transaction;
	int			i;
	enum sort_type		order;


	if (file == NULL || file->transacts == NULL || file->transacts->edit_line == NULL || !preset_test_index_valid(file, preset))
		return FALSE;

	/* Identify the transaction to be updated. */
	/* If there is not a transaction entry for the current edit line location
	 * (ie. if this is the first keypress in a new line), extend the transaction
	 * entries to reach the current location.
	 */

	if (line >= file->transacts->trans_count) {
		for (i = file->transacts->trans_count; i <= line; i++)
			transact_add_raw_entry(file->transacts->file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

		edit_refresh_line_contents(file->transacts->edit_line, TRANSACT_ICON_ROW, wimp_ICON_WINDOW);
	}

	if (line >= file->transacts->trans_count)
		return FALSE;

	transaction = file->transacts->transactions[line].sort_index;

	if (!transact_valid(file->transacts, transaction))
		return FALSE;

	/* Remove the target transaction from all calculations. */

	account_remove_transaction(file, transaction);

	/* Apply the preset to the transaction. */

	changed = preset_apply(file, preset, &(file->transacts->transactions[transaction].date),
			&(file->transacts->transactions[transaction].from),
			&(file->transacts->transactions[transaction].to),
			&(file->transacts->transactions[transaction].flags),
			&(file->transacts->transactions[transaction].amount),
			file->transacts->transactions[transaction].reference,
			file->transacts->transactions[transaction].description);

	/* Return the line to the calculations.  This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* Replace the edit line to make it pick up the changes. */

	transact_place_edit_line(file->transacts, line);

	/* Put the caret at the end of the preset destination field. */

	icons_put_caret_at_end(file->transacts->transaction_window,
			transact_convert_preset_icon_number(preset_get_caret_destination(file, preset)));

	/* If nothing changed, there's no more to do. */

	if (changed == TRANSACT_FIELD_NONE)
		return TRUE;

	if (changed & TRANSACT_FIELD_DATE)
		file->transacts->date_sort_valid = FALSE;

	/* If any changes were made, refresh the relevant account listing, redraw
	 * the transaction window line and mark the file as modified.
	 */

	accview_rebuild_all(file);

	/* Force a redraw of the affected line. */

	transact_force_window_redraw(file->transacts, line, line, wimp_ICON_WINDOW);


	/* If we're auto-sorting, and the sort column has been updated as
	 * part of the preset, then do an auto sort now.
	 *
	 * We will always sort if the sort column is Date, because pressing
	 * a preset key is analagous to hitting Return.
	 */

	order = sort_get_order(file->transacts->sort);

	if (config_opt_read("AutoSort") && (
			((order & SORT_MASK) == SORT_DATE) ||
			((changed & TRANSACT_FIELD_FROM) && ((order & SORT_MASK) == SORT_FROM)) ||
			((changed & TRANSACT_FIELD_TO) && ((order & SORT_MASK) == SORT_TO)) ||
			((changed & TRANSACT_FIELD_REF) && ((order & SORT_MASK) == SORT_REFERENCE)) ||
			((changed & TRANSACT_FIELD_AMOUNT) && ((order & SORT_MASK) == SORT_AMOUNT)) ||
			((changed & TRANSACT_FIELD_DESC) && ((order & SORT_MASK) == SORT_DESCRIPTION)))) {
		transact_sort(file->transacts);

		if (transact_valid(file->transacts, line)) {
			accview_sort(file, file->transacts->transactions[file->transacts->transactions[line].sort_index].from);
			accview_sort(file, file->transacts->transactions[file->transacts->transactions[line].sort_index].to);
		}
	}


	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Take a preset caret destination as used in the preset blocks, and convert it
 * into an icon number for the transaction edit line.
 *
 * \param caret		The preset caret destination to be converted.
 * \return		The corresponding icon number.
 */

static wimp_i transact_convert_preset_icon_number(enum preset_caret caret)
{
	wimp_i	icon;

	switch (caret) {
	case PRESET_CARET_DATE:
		icon = TRANSACT_ICON_DATE;
		break;

	case PRESET_CARET_FROM:
		icon = TRANSACT_ICON_FROM;
		break;

	case PRESET_CARET_TO:
		icon = TRANSACT_ICON_TO;
		break;

	case PRESET_CARET_REFERENCE:
		icon = TRANSACT_ICON_REFERENCE;
		break;

	case PRESET_CARET_AMOUNT:
		icon = TRANSACT_ICON_AMOUNT;
		break;

	case PRESET_CARET_DESCRIPTION:
		icon = TRANSACT_ICON_DESCRIPTION;
		break;

	default:
		icon = TRANSACT_ICON_DATE;
		break;
	}

	return icon;
}


/**
 * Process auto sort requests from the edit line.
 *
 * \param icon			The icon handle associated with the affected column.
 * \param *data			Our client data, holding the window instance.
 * \return			TRUE if successfully handled; else FALSE.
 */

static int transact_edit_auto_sort(wimp_i icon, void *data)
{
	struct transact_block	*windat = data;
	int			entry_line;
	enum sort_type		order;

	if (windat == NULL || windat->file == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("Requesting auto-sort on icon %d", icon);
#endif

	/* Don't do anything if AutoSort is configured off. */

	if (!config_opt_read("AutoSort"))
		return TRUE;

	/* Only sort if the keypress was in the active sort column, as nothing
	 * will be changing otherwise.
	 */

	order = sort_get_order(windat->sort);

	if ((icon == TRANSACT_ICON_DATE && (order & SORT_MASK) != SORT_DATE) ||
			(icon == TRANSACT_ICON_FROM && (order & SORT_MASK) != SORT_FROM) ||
			(icon == TRANSACT_ICON_TO && (order & SORT_MASK) != SORT_TO) ||
			(icon == TRANSACT_ICON_REFERENCE && (order & SORT_MASK) != SORT_REFERENCE) ||
			(icon == TRANSACT_ICON_AMOUNT && (order & SORT_MASK) != SORT_AMOUNT) ||
			(icon == TRANSACT_ICON_DESCRIPTION && (order & SORT_MASK) != SORT_DESCRIPTION))
		return TRUE;

	/* Sort the transactions. */

	transact_sort(windat);

	/* Re-sort any affected account views. */

	entry_line = edit_get_line(windat->edit_line);

	if (transact_valid(windat, entry_line)) {
		accview_sort(windat->file, windat->transactions[windat->transactions[entry_line].sort_index].from);
		accview_sort(windat->file, windat->transactions[windat->transactions[entry_line].sort_index].to);
	}

	return TRUE;
}


/**
 * Find the Wimp colour of a given line in a transaction window.
 *
 * \param *windat		The transaction window instance of interest.
 * \param line			The line of interest.
 * \return			The required Wimp colour, or Black on failure.
 */

static wimp_colour transact_line_colour(struct transact_block *windat, int line)
{
	tran_t	transaction;

	if (windat == NULL || windat->transactions == NULL || line < 0 || line >= windat->trans_count)
		return wimp_COLOUR_BLACK;

	transaction = windat->transactions[line].sort_index;

	if (config_opt_read("ShadeReconciled") &&
			((windat->transactions[transaction].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
		return config_int_read("ShadeReconciledColour");
	else
		return wimp_COLOUR_BLACK;
}


/**
 * Sort the contents of the transaction window based on the file's sort setting.
 *
 * \param *windat		The transaction window instance to sort.
 */

void transact_sort(struct transact_block *windat)
{
	if (windat == NULL)
		return;

	transact_list_window_sort(windat->transaction_window);
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

void transact_sort_file_data(struct file_block *file)
{
	int			i, gap, comb;
	osbool			sorted;
	struct transaction	temp;

#ifdef DEBUG
	debug_printf("Sorting transactions");
#endif

	if (file == NULL || file->transacts == NULL || file->transacts->date_sort_valid == TRUE)
		return;

	hourglass_on();

	/* Start by recording the order of the transactions on display in the
	 * main window, and also the order of the transactions themselves.
	 */

	for (i=0; i < file->transacts->trans_count; i++) {
		file->transacts->transactions[file->transacts->transactions[i].sort_index].saved_sort = i;	/* Record transaction window lines. */
		file->transacts->transactions[i].sort_index = i;						/* Record old transaction locations. */
	}

	/* Sort the entries using a combsort.  This has the advantage over qsort()
	 * that the order of entries is only affected if they are not equal and are
	 * in descending order.  Otherwise, the status quo is left.
	 */

	gap = file->transacts->trans_count - 1;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((file->transacts->trans_count >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < file->transacts->trans_count; comb++) {
			if (file->transacts->transactions[comb+gap].date < file->transacts->transactions[comb].date) {
				temp = file->transacts->transactions[comb+gap];
				file->transacts->transactions[comb+gap] = file->transacts->transactions[comb];
				file->transacts->transactions[comb] = temp;

				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);

	/* Finally, restore the order of the transactions on display in the
	 * main window.
	 */

	for (i=0; i < file->transacts->trans_count; i++)
		file->transacts->transactions[file->transacts->transactions[i].sort_index].sort_workspace = i;

	accview_reindex_all(file);

	for (i=0; i < file->transacts->trans_count; i++)
		file->transacts->transactions[file->transacts->transactions[i].saved_sort].sort_index = i;

	transact_force_window_redraw(file->transacts, 0, file->transacts->trans_count - 1, TRANSACT_PANE_ROW);

	file->transacts->date_sort_valid = TRUE;

	hourglass_off();
}


/**
 * Purge unused transactions from a file.
 *
 * \param *file			The file to purge.
 * \param cutoff		The cutoff date before which transactions should be removed.
 */

void transact_purge(struct file_block *file, date_t cutoff)
{
	tran_t			transaction;
	enum transact_flags	flags;
	acct_t			from, to;
	date_t			date;
	amt_t			amount;

	if (file == NULL || file->transacts == NULL)
		return;

	for (transaction = 0; transaction < file->transacts->trans_count; transaction++) {
		date = transact_get_date(file, transaction);
		flags = transact_get_flags(file, transaction);

		if ((flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO) &&
				(cutoff == NULL_DATE || date < cutoff)) {
			from = transact_get_from(file, transaction);
			to = transact_get_to(file, transaction);
			amount = transact_get_amount(file, transaction);

			/* If the from and to accounts are full accounts, */

			if (from != NULL_ACCOUNT && (account_get_type(file, from) & ACCOUNT_FULL) != 0)
				account_adjust_opening_balance(file, from, -amount);

			if (to != NULL_ACCOUNT && (account_get_type(file, to) & ACCOUNT_FULL) != 0)
				account_adjust_opening_balance(file, to, +amount);

			transact_clear_raw_entry(file, transaction);
		}
	}

	transact_sort_file_data(file);

	transact_strip_blanks_from_end(file);
}













/**
 * Save the transaction details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void transact_write_file(struct file_block *file, FILE *out)
{
	int	i;
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (file == NULL || file->transacts == NULL)
		return;

	fprintf(out, "\n[Transactions]\n");

	fprintf(out, "Entries: %x\n", file->transacts->trans_count);

	column_write_as_text(file->transacts->columns, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "WinColumns: %s\n", buffer);

	sort_write_as_text(file->transacts->sort, buffer, FILING_MAX_FILE_LINE_LEN);
	fprintf(out, "SortOrder: %s\n", buffer);

	for (i = 0; i < file->transacts->trans_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x,%x\n",
				file->transacts->transactions[i].date, file->transacts->transactions[i].flags, file->transacts->transactions[i].from,
				file->transacts->transactions[i].to, file->transacts->transactions[i].amount);
		if (*(file->transacts->transactions[i].reference) != '\0')
			config_write_token_pair(out, "Ref", file->transacts->transactions[i].reference);
		if (*(file->transacts->transactions[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->transacts->transactions[i].description);
	}
}


/**
 * Read transaction details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool transact_read_file(struct file_block *file, struct filing_block *in)
{
	size_t			block_size;
	tran_t			transaction = NULL_TRANSACTION;

	if (file == NULL || file->transacts == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Transactions.");
#endif

	/* The load is probably going to invalidate the sort order. */

	file->transacts->date_sort_valid = FALSE;

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->transacts->transactions), sizeof(struct transaction), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->transacts->trans_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->transacts->trans_count;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			/* For file format 1.00 or older, there's no row column at the
			 * start of the line so skip on to colunn 1 (date).
			 */
			column_init_window(file->transacts->columns, (filing_get_format(in) <= 100) ? 1 : 0, TRUE, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "SortOrder")){
			sort_read_from_text(file->transacts->sort, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			file->transacts->trans_count++;
			if (file->transacts->trans_count > block_size) {
				block_size = file->transacts->trans_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			}
			transaction = file->transacts->trans_count - 1;
			file->transacts->transactions[transaction].date = date_get_date_field(in);
			file->transacts->transactions[transaction].flags = transact_get_flags_field(in);
			file->transacts->transactions[transaction].from = account_get_account_field(in);
			file->transacts->transactions[transaction].to = account_get_account_field(in);
			file->transacts->transactions[transaction].amount = currency_get_currency_field(in);

			*(file->transacts->transactions[transaction].reference) = '\0';
			*(file->transacts->transactions[transaction].description) = '\0';

			file->transacts->transactions[transaction].sort_index = transaction;
		} else if (transaction != NULL_TRANSACTION && filing_test_token(in, "Ref")) {
			filing_get_text_value(in, file->transacts->transactions[transaction].reference, TRANSACT_REF_FIELD_LEN);
		} else if (transaction != NULL_TRANSACTION && filing_test_token(in, "Desc")) {
			filing_get_text_value(in, file->transacts->transactions[transaction].description, TRANSACT_DESCRIPT_FIELD_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}

	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(file->transacts->trans_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
}


/**
 * Search the transactions, returning the entry from nearest the target date.
 * The transactions will be sorted into order if they are not already.
 *
 * \param *file			The file to search in.
 * \param target		The target date.
 * \return			The transaction number for the date, or
 *				NULL_TRANSACTION.
 */

int transact_find_date(struct file_block *file, date_t target)
{
	int		min, max, mid;

	if (file == NULL || file->transacts == NULL || file->transacts->trans_count == 0 || target == NULL_DATE)
		return NULL_TRANSACTION;

	/* If the transactions are not already sorted, sort them into date
	 * order.
	 */

	transact_sort_file_data(file);

	/* Search through the sorted array using a binary search. */

	min = 0;
	max = file->transacts->trans_count - 1;

	while (min < max) {
		mid = (min + max)/2;

		if (target <= file->transacts->transactions[mid].date)
			max = mid;
		else
			min = mid + 1;
	}

	return min;
}


/**
 * Place the caret in a given line in a transaction window, and scroll
 * the line into view.
 *
 * \param *file			The file to operate on.
 * \param line			The line (under the current display sort order)
 *				to place the caret in.
 * \param field			The field to place the caret in.
 */

void transact_place_caret(struct file_block *file, int line, enum transact_field field)
{
	if (file == NULL || file->transacts == NULL)
		return;

	transact_list_window_place_caret(file->transacts->transact_window, line, field);
}


/**
 * Search the transaction list from a file for a set of matching entries.
 *
 * \param *file			The file to search in.
 * \param *line			Pointer to the line (under current display sort
 *				order) to search from. Updated on exit to show
 *				the matched line.
 * \param back			TRUE to search back up the file; FALSE to search
 *				down.
 * \param case_sensitive	TRUE to match case in strings; FALSE to ignore.
 * \param logic_and		TRUE to combine the parameters in an AND logic;
 *				FALSE to use an OR logic.
 * \param date			A date to match, or NULL_DATE for none.
 * \param from			A from account to match, or NULL_ACCOUNT for none.
 * \param to			A to account to match, or NULL_ACCOUNT for none.
 * \param flags			Reconcile flags for the from and to accounts, if
 *				these have been specified.
 * \param amount		An amount to match, or NULL_AMOUNT for none.
 * \param *ref			A wildcarded reference to match; NULL or '\0' for none.
 * \param *desc			A wildcarded description to match; NULL or '\0' for none.
 * \return			Transaction field flags set for each matching field;
 *				TRANSACT_FIELD_NONE if no match found.
 */

enum transact_field transact_search(struct file_block *file, int *line, osbool back, osbool case_sensitive, osbool logic_and,
		date_t date, acct_t from, acct_t to, enum transact_flags flags, amt_t amount, char *ref, char *desc)
{
	enum transact_field	test = TRANSACT_FIELD_NONE, original = TRANSACT_FIELD_NONE;
	osbool			match = FALSE;
	enum transact_flags	from_rec, to_rec;
	int			transaction;


	if (file == NULL || file->transacts == NULL)
		return TRANSACT_FIELD_NONE;

	match = FALSE;

	from_rec = flags & TRANS_REC_FROM;
	to_rec = flags & TRANS_REC_TO;

	while (*line < file->transacts->trans_count && *line >= 0 && !match) {
		/* Initialise the test result variable.  The tests all have a bit allocated.  For OR tests, these start unset and
		 * are set if a test passes; a non-zero value at the end indicates a match.  For AND tests, all the required bits
		 * are set at the start and cleared as tests match.  A zero value at the end indicates a match.
		 */

		test = TRANSACT_FIELD_NONE;

		if (logic_and) {
			if (date != NULL_DATE)
				test |= TRANSACT_FIELD_DATE;

			if (from != NULL_ACCOUNT)
				test |= TRANSACT_FIELD_FROM;

			if (to != NULL_ACCOUNT)
				test |= TRANSACT_FIELD_TO;

			if (amount != NULL_CURRENCY)
				test |= TRANSACT_FIELD_AMOUNT;

			if (ref != NULL && *ref != '\0')
				test |= TRANSACT_FIELD_REF;

			if (desc != NULL && *desc != '\0')
				test |= TRANSACT_FIELD_DESC;
		}

		original = test;

		/* Perform the tests.
		 *
		 * \TODO -- The order of these tests could be optimised, to
		 *          skip things that we don't need to bother matching.
		 */

		transaction = file->transacts->transactions[*line].sort_index;

		if (desc != NULL && *desc != '\0' && string_wildcard_compare(desc, file->transacts->transactions[transaction].description, !case_sensitive)) {
			test ^= TRANSACT_FIELD_DESC;
		}

		if (amount != NULL_CURRENCY && amount == file->transacts->transactions[transaction].amount) {
			test ^= TRANSACT_FIELD_AMOUNT;
		}

		if (ref != NULL && *ref != '\0' && string_wildcard_compare(ref, file->transacts->transactions[transaction].reference, !case_sensitive)) {
			test ^= TRANSACT_FIELD_REF;
		}

		/* The following two tests check that a) an account has been specified, b) it is the same as the transaction and
		 * c) the two reconcile flags are set the same (if they are, the EOR operation cancels them out).
		 */

		if (to != NULL_ACCOUNT && to == file->transacts->transactions[transaction].to &&
				((to_rec ^ file->transacts->transactions[transaction].flags) & TRANS_REC_TO) == 0) {
			test ^= TRANSACT_FIELD_TO;
		}

		if (from != NULL_ACCOUNT && from == file->transacts->transactions[transaction].from &&
				((from_rec ^ file->transacts->transactions[transaction].flags) & TRANS_REC_FROM) == 0) {
			test ^= TRANSACT_FIELD_FROM;
		}

		if (date != NULL_DATE && date == file->transacts->transactions[transaction].date) {
			test ^= TRANSACT_FIELD_DATE;
		}

		/* Check if the test passed or failed. */

		if ((!logic_and && test) || (logic_and && !test)) {
			match = TRUE;
		} else {
			if (back)
				(*line)--;
			else
				(*line)++;
		}
	}

	if (!match)
		return TRANSACT_FIELD_NONE;

	return (logic_and) ? original : test;
}


/**
 * Check the transactions in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool transact_check_account(struct file_block *file, acct_t account)
{
	int		i;

	if (file == NULL || file->transacts == NULL)
		return FALSE;

	for (i = 0; i < file->transacts->trans_count; i++)
		if (file->transacts->transactions[i].from == account || file->transacts->transactions[i].to == account)
			return TRUE;

	return FALSE;
}

