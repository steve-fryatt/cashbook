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
 * \file: edit.c
 *
 * Transaction editing implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"

/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "edit.h"

#include "account.h"
#include "accview.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "presets.h"
#include "sort.h"	// \TODO -- This needs to be removed.
#include "transact.h"

/**
 * Static constants.
 */

#define ROW_FIELD_LEN 10
#define DATE_FIELD_LEN 11


/**
 * \TODO -- This is an exact duplicate of the struct definition in transact.c.
 *          It needs to be removed once all references to transactions[] in
 *          edit.c have been relocated into transact.c
 *
 * Transatcion data struct.
 */

struct transaction {
	date_t			date;
	enum transact_flags	flags;
	acct_t			from;
	acct_t			to;
	amt_t			amount;
	char			reference[REF_FIELD_LEN];
	char			description[DESCRIPT_FIELD_LEN];

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the transaction data, and are in effect a separate array that is used
	 * for handling entries in the transaction window.
	 */

	int			sort_index;		/**< Point to another transaction, to allow the transaction window to be sorted. */
	int			saved_sort;		/**< Preserve the transaction window sort order across transaction data sorts. */
	int			sort_workspace;		/**< Workspace used by the sorting code. */
};


/**
 * \TODO -- This is an exact duplicate of the struct definition in transact.c.
 *          It needs to be removed once all references to transaction_window in
 *          edit.c have been relocated into transact.c
 *
 * Transatcion Window data structure
 */

struct transact_block {
	struct file_block	*file;						/**< The file to which the window belongs.			*/

	/* Transactcion window handle and title details. */

	wimp_w			transaction_window;				/**< Window handle of the transaction window */
	char			window_title[256];
	wimp_w			transaction_pane;				/**< Window handle of the transaction window toolbar pane */

	/* Display column details. */

	int			column_width[TRANSACT_COLUMNS];			/**< Array holding the column widths in the transaction window. */
	int			column_position[TRANSACT_COLUMNS];		/**< Array holding the column X-offsets in the transact window. */

	/* Other window information. */

	int			display_lines;					/**< How many lines the current work area is formatted to display. */
	int			entry_line;					/**< The line currently marked for data entry, in terms of window lines. */
	enum sort_type		sort_order;					/**< The order in which the window is sorted. */

	char			sort_sprite[12];				/**< Space for the sort icon's indirected data. */

	/* Transaction Data. */

	struct transaction	*transactions;					/**< The transaction data for the defined transactions			*/
	int			trans_count;					/**< The number of transactions defined in the file.			*/
};

/* ==================================================================================================================
 * Global variables.
 */

/* A pointer to the window block belonging to the window currently holding the edit line. */

static struct transact_block *edit_entry_window = NULL;

/* The following are the buffers used by the edit line in the transaction window. */

static char	buffer_row[ROW_FIELD_LEN];
static char	buffer_date[DATE_FIELD_LEN];
static char	buffer_from_ident[ACCOUNT_IDENT_LEN];
static char	buffer_from_name[ACCOUNT_NAME_LEN];
static char	buffer_from_rec[REC_FIELD_LEN];
static char	buffer_to_ident[ACCOUNT_IDENT_LEN];
static char	buffer_to_name[ACCOUNT_NAME_LEN];
static char	buffer_to_rec[REC_FIELD_LEN];
static char	buffer_reference [REF_FIELD_LEN];
static char	buffer_amount[AMOUNT_FIELD_LEN];
static char	buffer_description[DESCRIPT_FIELD_LEN];

wimp_window *edit_transact_window_def = NULL;	/* \TODO -- Ick! */


static void	edit_find_icon_horizontally(struct file_block *file);
static void	edit_set_line_shading(struct file_block *file);
static void	edit_change_transaction_amount(struct file_block *file, int transaction, amt_t new_amount);
static unsigned	edit_raw_insert_preset_into_transaction(struct file_block *file, int transaction, int preset);
static wimp_i	edit_convert_preset_icon_number(enum preset_caret caret);
static void	edit_delete_line_transaction_content(struct file_block *file);
static void	edit_process_content_keypress(struct file_block *file, wimp_key *key);
static char	*find_complete_description(struct file_block *file, int line, char *buffer, size_t length);

/**
 * Test whether an account number is safe to look up in the account data array.
 */

#define transact_valid(windat, transaction) (((transaction) != NULL_TRANSACTION) && ((transaction) >= 0) && ((transaction) < ((windat)->trans_count)))



/**
 * Create an edit line at the specified point in the given file's transaction
 * window. Any existing edit line is deleted first.
 *
 * The caret isn't placed in this routine.  That is left up to the caller, so
 * that they can place it depending on their context.
 *
 * \param *file		The file to place the edit line in.
 * \param line		The line to place the edit line at, in terms of
 *			sorted display (not the raw transaction number).
 */

void edit_place_new_line(struct file_block *file, int line)
{
	int			i, transaction;
	wimp_icon_create	icon_block;


	if (file == NULL || file->transacts == NULL || line == -1)
		return;

	/* Start by deleting any existing edit line, from any open transaction window. */

	if (edit_entry_window != NULL) {
		/* The assumption is that the data will be safe as it's always copied into
		 * memory as soon as a key is pressed in any of the writable icons...
		 */

		for (i = 0; i < TRANSACT_COLUMNS; i++)
			wimp_delete_icon(edit_entry_window->transaction_window, (wimp_i) i);

		edit_entry_window->entry_line = -1;
		edit_entry_window = NULL;
	}

	/* Extend the window work area if required. */

	if (line >= file->transacts->display_lines) {
		file->transacts->display_lines = line + 1;
		transact_set_window_extent(file);
	}

	/* Create the icon block required for the icon definitions. */

	icon_block.w = file->transacts->transaction_window;

	/* Set up the indirected buffers. */

	edit_transact_window_def->icons[EDIT_ICON_ROW].data.indirected_text.text = buffer_row;
	edit_transact_window_def->icons[EDIT_ICON_ROW].data.indirected_text.size = ROW_FIELD_LEN;

	edit_transact_window_def->icons[EDIT_ICON_DATE].data.indirected_text.text = buffer_date;
	edit_transact_window_def->icons[EDIT_ICON_DATE].data.indirected_text.size = DATE_FIELD_LEN;

	edit_transact_window_def->icons[EDIT_ICON_FROM].data.indirected_text.text = buffer_from_ident;
	edit_transact_window_def->icons[EDIT_ICON_FROM].data.indirected_text.size = ACCOUNT_IDENT_LEN;

	edit_transact_window_def->icons[EDIT_ICON_FROM_REC].data.indirected_text.text = buffer_from_rec;
	edit_transact_window_def->icons[EDIT_ICON_FROM_REC].data.indirected_text.size = REC_FIELD_LEN;

	edit_transact_window_def->icons[EDIT_ICON_FROM_NAME].data.indirected_text.text = buffer_from_name;
	edit_transact_window_def->icons[EDIT_ICON_FROM_NAME].data.indirected_text.size = ACCOUNT_NAME_LEN;

	edit_transact_window_def->icons[EDIT_ICON_TO].data.indirected_text.text = buffer_to_ident;
	edit_transact_window_def->icons[EDIT_ICON_TO].data.indirected_text.size = ACCOUNT_IDENT_LEN;

	edit_transact_window_def->icons[EDIT_ICON_TO_REC].data.indirected_text.text = buffer_to_rec;
	edit_transact_window_def->icons[EDIT_ICON_TO_REC].data.indirected_text.size = REC_FIELD_LEN;

	edit_transact_window_def->icons[EDIT_ICON_TO_NAME].data.indirected_text.text = buffer_to_name;
	edit_transact_window_def->icons[EDIT_ICON_TO_NAME].data.indirected_text.size = ACCOUNT_NAME_LEN;

	edit_transact_window_def->icons[EDIT_ICON_REF].data.indirected_text.text = buffer_reference;
	edit_transact_window_def->icons[EDIT_ICON_REF].data.indirected_text.size = REF_FIELD_LEN;

	edit_transact_window_def->icons[EDIT_ICON_AMOUNT].data.indirected_text.text = buffer_amount;
	edit_transact_window_def->icons[EDIT_ICON_AMOUNT].data.indirected_text.size = AMOUNT_FIELD_LEN;

	edit_transact_window_def->icons[EDIT_ICON_DESCRIPT].data.indirected_text.text = buffer_description;
	edit_transact_window_def->icons[EDIT_ICON_DESCRIPT].data.indirected_text.size = DESCRIPT_FIELD_LEN;

	/* Initialise the data. */

	if (transact_valid(file->transacts, line)) {
		transaction = file->transacts->transactions[line].sort_index;

		snprintf(buffer_row, ROW_FIELD_LEN, "%d", transact_get_transaction_number(transaction));
		date_convert_to_string(file->transacts->transactions[transaction].date, buffer_date, DATE_FIELD_LEN);
		strncpy(buffer_from_ident, account_get_ident(file, file->transacts->transactions[transaction].from), ACCOUNT_IDENT_LEN);
		strncpy(buffer_from_name, account_get_name(file, file->transacts->transactions[transaction].from), ACCOUNT_NAME_LEN);
		if (file->transacts->transactions[transaction].flags & TRANS_REC_FROM)
			msgs_lookup("RecChar", buffer_from_rec, REC_FIELD_LEN);
		else
			*buffer_from_rec = '\0';
		strncpy(buffer_to_ident, account_get_ident(file, file->transacts->transactions[transaction].to), ACCOUNT_IDENT_LEN);
		strncpy(buffer_to_name, account_get_name(file, file->transacts->transactions[transaction].to), ACCOUNT_NAME_LEN);
		if (file->transacts->transactions[transaction].flags & TRANS_REC_TO)
			msgs_lookup("RecChar", buffer_to_rec, REC_FIELD_LEN);
		else
			*buffer_to_rec = '\0';
		strncpy(buffer_reference, file->transacts->transactions[transaction].reference, REF_FIELD_LEN);
		currency_convert_to_string(file->transacts->transactions[transaction].amount, buffer_amount, AMOUNT_FIELD_LEN);
		strncpy(buffer_description, file->transacts->transactions[transaction].description, DESCRIPT_FIELD_LEN);
	} else {
		*buffer_row = '\0';
		*buffer_date = '\0';
		*buffer_from_ident = '\0';
		*buffer_from_rec = '\0';
		*buffer_from_name = '\0';
		*buffer_to_ident = '\0';
		*buffer_to_rec = '\0';
		*buffer_to_name = '\0';
		*buffer_reference = '\0';
		*buffer_amount = '\0';
		*buffer_description = '\0';
	}

	/* Set the icon positions correctly and create them. */

	for (i=0; i < TRANSACT_COLUMNS; i++) {
		memcpy(&(icon_block.icon), &(edit_transact_window_def->icons[i]), sizeof(wimp_icon));

		icon_block.icon.extent.x0 = file->transacts->column_position[i];
		icon_block.icon.extent.x1 = file->transacts->column_position[i]
				+ file->transacts->column_width[i];
		icon_block.icon.extent.y0 = (-line * (ICON_HEIGHT+LINE_GUTTER))
				- TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
		icon_block.icon.extent.y1 = (-line * (ICON_HEIGHT+LINE_GUTTER))
				- TRANSACT_TOOLBAR_HEIGHT;

		wimp_create_icon(&icon_block);
	}

	/* Update the window data to show the line being edited. */

	file->transacts->entry_line = line;
	edit_entry_window = file->transacts;

	edit_set_line_shading(file);
}


/**
 * Place a new edit line by raw transaction number.
 *
 * \param *file		The file to place the line in.
 * \param transaction	The transaction to place the line on.
 */

void edit_place_new_line_by_transaction(struct file_block *file, int transaction)
{
	int        i;
	wimp_caret caret;

	if (file == NULL || file->transacts == NULL || edit_entry_window != file->transacts)
		return;

	if (transaction != NULL_TRANSACTION) {
		for (i = 0; i < file->transacts->trans_count; i++) {
			if (file->transacts->transactions[i].sort_index == transaction) {
				edit_place_new_line(file, i);
				wimp_get_caret_position(&caret);
				if (caret.w == file->transacts->transaction_window)
					icons_put_caret_at_end(file->transacts->transaction_window, EDIT_ICON_DATE);
				edit_find_line_vertically(file);

				break;
			}
		}
	} else {
		edit_place_new_line(file, file->transacts->trans_count);
		wimp_get_caret_position(&caret);
		if (caret.w == file->transacts->transaction_window)
			icons_put_caret_at_end(file->transacts->transaction_window, EDIT_ICON_DATE);
		edit_find_line_vertically(file);
	}
}


/**
 * Inform the edit line code that a file has been deleted: this removes any
 * references to the edit line if it is within that file's transaction window.
 * 
 * Note that it isn't possible to delete an edit line and its icons: it will
 * only be completely destroyed if the parent window is deleted.
 *
 * \param *file		The file to be deleted.
 */

void edit_file_deleted(struct file_block *file)
{
	if (file != NULL && edit_entry_window == file->transacts)
		edit_entry_window = NULL;
}


/**
 * Bring the edit line into view in the window in a vertical direction.
 *
 * \param *file		The file that we're interested in working on
 */

void edit_find_line_vertically(struct file_block *file)
{
	wimp_window_state	window;
	int			height, top, bottom;


	if (file == NULL || file->transacts == NULL || file->transacts != edit_entry_window)
		return;

	window.w = file->transacts->transaction_window;
	wimp_get_window_state(&window);

	/* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
	 * This will allow the edit line to be aligned with the top or bottom of the window.
	 */

	height = window.visible.y1 - window.visible.y0 - ICON_HEIGHT - LINE_GUTTER - TRANSACT_TOOLBAR_HEIGHT;

	/* Calculate the top full line and bottom full line that are showing in the window.  Part lines don't
	 * count and are discarded.
	 */

	top = (-window.yscroll + ICON_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
	bottom = height / (ICON_HEIGHT+LINE_GUTTER) + top;

#ifdef DEBUG
	debug_printf("\\BFind transaction edit line");
	debug_printf("Top: %d, Bottom: %d, Entry line: %d", top, bottom, file->transacts->entry_line);
#endif

	/* If the edit line is above or below the visible area, bring it into range. */

	if (file->transacts->entry_line < top) {
	window.yscroll = -(file->transacts->entry_line * (ICON_HEIGHT+LINE_GUTTER));
	wimp_open_window((wimp_open *) &window);
	transact_minimise_window_extent(file);
	}

	if (file->transacts->entry_line > bottom) {
		window.yscroll = -((file->transacts->entry_line) * (ICON_HEIGHT+LINE_GUTTER) - height);
		wimp_open_window((wimp_open *) &window);
		transact_minimise_window_extent(file);
	}
}


/**
 * Bring the current edit line icon (the one containing the caret) into view in
 * the window in a horizontal direction.
 *
 * \param *file		The file that we're interested in working on
 */

static void edit_find_icon_horizontally(struct file_block *file)
{
	wimp_window_state	window;
	wimp_icon_state		icon;
	wimp_caret		caret;
	int			window_width, window_xmin, window_xmax;
	int			icon_width, icon_xmin, icon_xmax, icon_target, group;


	if (file == NULL || file->transacts == NULL || file->transacts != edit_entry_window)
		return;

	window.w = file->transacts->transaction_window;
	wimp_get_window_state(&window);
	wimp_get_caret_position(&caret);

	if (caret.w != window.w || caret.i == -1)
		return;

	/* Find the group holding the current icon. */

	group = 0;
	while (caret.i > column_get_rightmost_in_group(TRANSACT_PANE_COL_MAP, group))
		group++;

	/* Get the left hand icon dimension */

	icon.w = window.w;
	icon.i = column_get_leftmost_in_group(TRANSACT_PANE_COL_MAP, group);
	wimp_get_icon_state(&icon);
	icon_xmin = icon.icon.extent.x0;

	/* Get the right hand icon dimension */

	icon.w = window.w;
	icon.i = column_get_rightmost_in_group(TRANSACT_PANE_COL_MAP, group);
	wimp_get_icon_state(&icon);
	icon_xmax = icon.icon.extent.x1;

	icon_width = icon_xmax - icon_xmin;

	/* Establish the window dimensions. */

	window_width = window.visible.x1 - window.visible.x0;
	window_xmin = window.xscroll;
	window_xmax = window.xscroll + window_width;

	if (window_width > icon_width) {
		/* If the icon group fits into the visible window, just pull the overlap into view. */

		if (icon_xmin < window_xmin) {
			window.xscroll = icon_xmin;
			wimp_open_window((wimp_open *) &window);
		} else if (icon_xmax > window_xmax) {
			window.xscroll = icon_xmax - window_width;
			wimp_open_window((wimp_open *) &window);
		}
	} else {
		/* If the icon is bigger than the window, however, get the justification end of the icon and ensure that it
		 * is aligned against that side of the window.
		 */

		icon.w = window.w;
		icon.i = caret.i;
		wimp_get_icon_state(&icon);

		icon_target = (icon.icon.flags & wimp_ICON_RJUSTIFIED) ? icon.icon.extent.x1 : icon.icon.extent.x0;

		if ((icon_target < window_xmin || icon_target > window_xmax) && !(icon.icon.flags & wimp_ICON_RJUSTIFIED)) {
			window.xscroll = icon_target;
			wimp_open_window((wimp_open *) &window);
		} else if ((icon_target < window_xmin || icon_target > window_xmax) && (icon.icon.flags & wimp_ICON_RJUSTIFIED)) {
			window.xscroll = icon_target - window_width;
			wimp_open_window((wimp_open *) &window);
		}
	}
}


/**
 * Refresh the contents of the edit line icons, copying the contents of memory
 * back into them.
 *
 * \param w		If NULL, refresh any window; otherwise, only refresh if
 *			the parent transaction window handle matches w.
 * \param only		If -1, refresh all icons in the line; otherwise, only
 *			refresh if the icon handle matches i.
 * \param avoid		If -1, refresh all icons in the line; otherwise, only
 *			refresh if the icon handle does not match avoid.
 */

void edit_refresh_line_content(wimp_w w, wimp_i only, wimp_i avoid)
{
	int	transaction;

	if (edit_entry_window == NULL || edit_entry_window->file == NULL || (w != edit_entry_window->transaction_window && w != NULL) ||
			edit_entry_window->entry_line >= edit_entry_window->trans_count)
		return;

	transaction = edit_entry_window->transactions[edit_entry_window->entry_line].sort_index;

	if ((only == -1 || only == EDIT_ICON_ROW) && avoid != EDIT_ICON_ROW) {
		/* Replace the row number. */

		snprintf(buffer_row, ROW_FIELD_LEN, "%d", transact_get_transaction_number(transaction));
		wimp_set_icon_state(edit_entry_window->transaction_window, EDIT_ICON_ROW, 0, 0);
	}

	if ((only == -1 || only == EDIT_ICON_DATE) && avoid != EDIT_ICON_DATE) {
		/* Re-convert the date, so that it is displayed in standard format. */

		date_convert_to_string(edit_entry_window->transactions[transaction].date, buffer_date, DATE_FIELD_LEN);
		wimp_set_icon_state(edit_entry_window->transaction_window, EDIT_ICON_DATE, 0, 0);
	}

	if ((only == -1 || only == EDIT_ICON_FROM) && avoid != EDIT_ICON_FROM) {
		/* Remove the from ident if it didn't match an account. */

		if (edit_entry_window->transactions[transaction].from == NULL_ACCOUNT) {
			*buffer_from_ident = '\0';
			*buffer_from_name = '\0';
			*buffer_from_rec = '\0';
			wimp_set_icon_state(edit_entry_window->transaction_window, EDIT_ICON_FROM, 0, 0);
		} else {
			strcpy(buffer_from_ident, account_get_ident(edit_entry_window->file, edit_entry_window->transactions[transaction].from));
			strcpy(buffer_from_name, account_get_name(edit_entry_window->file, edit_entry_window->transactions[transaction].from));
			if (edit_entry_window->transactions[transaction].flags & TRANS_REC_FROM)
				msgs_lookup("RecChar", buffer_from_rec, REC_FIELD_LEN);
			else
				*buffer_from_rec = '\0';
		}
	}

	if ((only == -1 || only == EDIT_ICON_TO) && avoid != EDIT_ICON_TO) {
		/* Remove the to ident if it didn't match an account. */

		if (edit_entry_window->transactions[transaction].to == NULL_ACCOUNT) {
			*buffer_to_ident = '\0';
			*buffer_to_name = '\0';
			*buffer_to_rec = '\0';
			wimp_set_icon_state(edit_entry_window->transaction_window, EDIT_ICON_TO, 0, 0);
		} else {
			strcpy(buffer_to_ident, account_get_ident(edit_entry_window->file, edit_entry_window->transactions[transaction].to));
			strcpy(buffer_to_name, account_get_name(edit_entry_window->file, edit_entry_window->transactions[transaction].to));
			if (edit_entry_window->transactions[transaction].flags & TRANS_REC_TO)
				msgs_lookup("RecChar", buffer_to_rec, REC_FIELD_LEN);
			else
				*buffer_to_rec = '\0';
		}
	}

	if ((only == -1 || only == EDIT_ICON_REF) && avoid != EDIT_ICON_REF) {
		/* Copy the contents back into the icon. */

		strcpy(buffer_reference, edit_entry_window->transactions[transaction].reference);
		wimp_set_icon_state(edit_entry_window->transaction_window, EDIT_ICON_REF, 0, 0);
	}

	if ((only == -1 || only == EDIT_ICON_AMOUNT) && avoid != EDIT_ICON_AMOUNT) {
		/* Re-convert the amount so that it is displayed in standard format. */

		currency_convert_to_string(edit_entry_window->transactions[transaction].amount, buffer_amount, AMOUNT_FIELD_LEN);
		wimp_set_icon_state(edit_entry_window->transaction_window, EDIT_ICON_AMOUNT, 0, 0);
	}

	if ((only == -1 || only == EDIT_ICON_DESCRIPT) && avoid != EDIT_ICON_DESCRIPT) {
		/* Copy the contents back into the icon. */

		strcpy(buffer_description, edit_entry_window->transactions[transaction].description);
		wimp_set_icon_state(edit_entry_window->transaction_window, EDIT_ICON_DESCRIPT, 0, 0);
	}
}


/**
 * Set the shading of the transaction line to show the current reconcile status
 * of the transactions.
 *
 * \param *file		The file that should be processed.
 */

static void edit_set_line_shading(struct file_block *file)
{
	int	icon_fg_col, transaction;
	wimp_i	i;

	if (file == NULL || file->transacts == NULL || edit_entry_window != file->transacts ||
			file->transacts->trans_count == 0 ||
			!transact_valid(file->transacts, file->transacts->entry_line))
		return;

	transaction = file->transacts->transactions[file->transacts->entry_line].sort_index;

	if (config_opt_read("ShadeReconciled") && (file->transacts->entry_line < file->transacts->trans_count) &&
			((file->transacts->transactions[transaction].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
		icon_fg_col = (config_int_read("ShadeReconciledColour") << wimp_ICON_FG_COLOUR_SHIFT);
	else
		icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

	for (i = 0; i < TRANSACT_COLUMNS; i++)
		wimp_set_icon_state(edit_entry_window->transaction_window, i, icon_fg_col, wimp_ICON_FG_COLOUR);
}


/**
 * Get the underlying transaction number relating to the current edit line
 * position.
 *
 * \param *file		The file that we're interested in.
 * \return		The transaction number, or NULL_TRANSACTION if the
 *			line isn't in the specified file.
 */

int edit_get_line_transaction(struct file_block *file)
{
	int	transaction;

	transaction = NULL_TRANSACTION;

	if (file != NULL && file->transacts != NULL && edit_entry_window == file->transacts && transact_valid(file->transacts, file->transacts->entry_line))
		transaction = file->transacts->transactions[file->transacts->entry_line].sort_index;

	return (transaction);
}


/**
 * Toggle the state of one of the reconciled flags for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param change_flag	Indicate which reconciled flags to change.
 */

void edit_toggle_transaction_reconcile_flag(struct file_block *file, tran_t transaction, enum transact_flags change_flag)
{
	int	change_icon, line;
	osbool	changed = FALSE;


	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	/* Establish which icon it is that will need to be updated. */

	change_icon = (change_flag == TRANS_REC_FROM) ? EDIT_ICON_FROM_REC : EDIT_ICON_TO_REC;

	/* Only do anything if the transaction is inside the limit of the file. */

	account_remove_transaction(file, transaction);

	/* Update the reconcile flag, either removing it, or adding it in.  If the
	 * line is the edit line, the icon contents must be manually updated as well.
	 *
	 * If a change is made, this is flagged to allow the update to be recorded properly.
	 */

	if (file->transacts->transactions[transaction].flags & change_flag) {
		file->transacts->transactions[transaction].flags &= ~change_flag;

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction)
			*icons_get_indirected_text_addr(file->transacts->transaction_window, change_icon) = '\0';

		changed = TRUE;
	} else if ((change_flag == TRANS_REC_FROM && file->transacts->transactions[transaction].from != NULL_ACCOUNT) ||
			(change_flag == TRANS_REC_TO && file->transacts->transactions[transaction].to != NULL_ACCOUNT)) {
		file->transacts->transactions[transaction].flags |= change_flag;

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction)
			msgs_lookup("RecChar",
					icons_get_indirected_text_addr(file->transacts->transaction_window, change_icon),
					REC_FIELD_LEN);

		changed = 1;
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

		/* If the line is the edit line, setting the shading uses
		 * wimp_set_icon_state() and the line will effectively be redrawn
		 * for free.
		 */

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction) {
			edit_set_line_shading(file);
		} else {
			line = transact_get_line_from_transaction(file, transaction);
			transact_force_window_redraw(file, line, line);
		}

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the date for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param new_date	The new date to set the transaction to.
 */

void edit_change_transaction_date(struct file_block *file, tran_t transaction, date_t new_date)
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
		file->sort_valid = FALSE;
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

		/* If the line is the edit line, setting the shading uses
		 * wimp_set_icon_state() and the line will effectively be redrawn
		 * for free.
		 */

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction) {
			edit_refresh_line_content(file->transacts->transaction_window, EDIT_ICON_DATE, -1);
			edit_set_line_shading(file);
			icons_replace_caret_in_window(file->transacts->transaction_window);
		} else {
			line = transact_get_line_from_transaction(file, transaction);
			transact_force_window_redraw(file, line, line);
		}

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

static void edit_change_transaction_amount(struct file_block *file, tran_t transaction, amt_t new_amount)
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

		/* If the line is the edit line, setting the shading uses
		 * wimp_set_icon_state() and the line will effectively be redrawn
		 * for free.
		 */

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction) {
			edit_refresh_line_content(file->transacts->transaction_window, EDIT_ICON_AMOUNT, -1);
			edit_set_line_shading(file);
			icons_replace_caret_in_window(file->transacts->transaction_window);
		} else {
			line = transact_get_line_from_transaction(file, transaction);
			transact_force_window_redraw(file, line, line);
		}

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

void edit_change_transaction_refdesc(struct file_block *file, tran_t transaction, wimp_i target, char *new_text)
{
	int	line;
	osbool	changed = FALSE;
	char	*field = NULL;


	/* Only do anything if the transaction is inside the limit of the file. */

	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction))
		return;

	/* Find the field that will be getting changed. */

	switch (target) {
	case EDIT_ICON_REF:
		field = file->transacts->transactions[transaction].reference;
		break;

	case EDIT_ICON_DESCRIPT:
		field = file->transacts->transactions[transaction].description;
		break;

	default:
		field = NULL;
		break;
	}


	if (field != NULL && strcmp(field, new_text) != 0) {
		changed = TRUE;
		strcpy(field, new_text);
	}

	/* If any changes were made, refresh the relevant account listings, redraw
	 * the transaction window line and mark the file as modified.
	 */

	if (changed == TRUE) {
		/* Refresh any account views that may be affected. */

		accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
		accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);

		/* If the line is the edit line, setting the shading uses
		 * wimp_set_icon_state() and the line will effectively be redrawn
		 * for free.
		 */

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction) {
			edit_refresh_line_content(file->transacts->transaction_window, target, -1);
			edit_set_line_shading(file);
			icons_replace_caret_in_window(file->transacts->transaction_window);
		} else {
			line = transact_get_line_from_transaction(file, transaction);
			transact_force_window_redraw(file, line, line);
		}

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Change the reference or description associated with a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param target	The target field to change.
 * \param new_account	The new account to set the field to.
 */

void edit_change_transaction_account(struct file_block *file, tran_t transaction, wimp_i target, acct_t new_account)
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
	case EDIT_ICON_FROM:
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
	case EDIT_ICON_TO:
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

	if (changed == TRUE) {
		switch (target) {
		case EDIT_ICON_FROM:
			accview_rebuild(file, old_acct);
			accview_rebuild(file, file->transacts->transactions[transaction].from);
			accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);
			break;
		case EDIT_ICON_TO:
			accview_rebuild(file, old_acct);
			accview_rebuild(file, file->transacts->transactions[transaction].to);
			accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
			break;
		}

		/* If the line is the edit line, setting the shading uses
		 * wimp_set_icon_state() and the line will effectively be redrawn
		 * for free.
		 */

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction) {
			edit_refresh_line_content(file->transacts->transaction_window, target, -1);
			edit_set_line_shading(file);
			icons_replace_caret_in_window(file->transacts->transaction_window);
		} else {
			line = transact_get_line_from_transaction(file, transaction);
			transact_force_window_redraw(file, line, line);
		}

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Insert a preset into a pre-existing transaction, taking care of updating all
 * the file data in a clean way.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to update.
 * \param preset	The preset to insert into the transaction.
 */

void edit_insert_preset_into_transaction(struct file_block *file, tran_t transaction, preset_t preset)
{
	int		line;
	unsigned	changed = 0;


	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction) || !preset_test_index_valid(file, preset))
		return;  
  
	account_remove_transaction(file, transaction);

	changed = edit_raw_insert_preset_into_transaction(file, transaction, preset);

	/* Return the line to the calculations.  This will automatically update
	 * all the account listings.
	 */

	account_restore_transaction(file, transaction);

	/* If any changes were made, refresh the relevant account listing, redraw
	 * the transaction window line and mark the file as modified.
	 */

	edit_place_new_line_by_transaction(file, transaction);

	icons_put_caret_at_end(file->transacts->transaction_window,
			edit_convert_preset_icon_number(preset_get_caret_destination(file, preset)));

	if (changed != 0) {
		accview_rebuild_all(file);

		/* If the line is the edit line, setting the shading uses
		 * wimp_set_icon_state() and the line will effectively be
		 * redrawn for free.
		 */

		if (file->transacts->transactions[file->transacts->entry_line].sort_index == transaction) {
			edit_refresh_line_content(file->transacts->transaction_window, -1, -1);
			edit_set_line_shading(file);
			icons_replace_caret_in_window(file->transacts->transaction_window);
		} else {
			line = transact_get_line_from_transaction(file, transaction);
			transact_force_window_redraw(file, line, line);
		}

		file_set_data_integrity(file, TRUE);
	}
}


/**
 * Insert the contents of a preset into a transaction, if that transaction already
 * exists in the target file.
 *
 * This function is assumed to be called by code that takes care of updating the
 * transaction record and all the associated totals. Normally, this would be done
 * by wrapping the call up inside a pair of account_remove_transaction() and
 * account_restore_transaction() calls.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to update.
 * \param preset	The preset to insert into the transaction.
 * \return		A bitfield showing which icons have been edited.
 */

static unsigned edit_raw_insert_preset_into_transaction(struct file_block *file, tran_t transaction, preset_t preset)
{
	unsigned	changed = 0;

	if (transact_valid(file->transacts, transaction) && preset_test_index_valid(file, preset))
		changed = preset_apply(file, preset, &(file->transacts->transactions[transaction].date),
				&(file->transacts->transactions[transaction].from),
				&(file->transacts->transactions[transaction].to),
				&(file->transacts->transactions[transaction].flags),
				&(file->transacts->transactions[transaction].amount),
				file->transacts->transactions[transaction].reference,
				file->transacts->transactions[transaction].description);

	return changed;
}


/**
 * Take a preset caret destination as used in the preset blocks, and convert it
 * into an icon number for the transaction edit line.
 *
 * \param caret		The preset caret destination to be converted.
 * \return		The corresponding icon number.
 */

static wimp_i edit_convert_preset_icon_number(enum preset_caret caret)
{
	wimp_i	icon;

	switch (caret) {
	case PRESET_CARET_DATE:
		icon = EDIT_ICON_DATE;
		break;

	case PRESET_CARET_FROM:
		icon = EDIT_ICON_FROM;
		break;

	case PRESET_CARET_TO:
		icon = EDIT_ICON_TO;
		break;

	case PRESET_CARET_REFERENCE:
		icon = EDIT_ICON_REF;
		break;

	case PRESET_CARET_AMOUNT:
		icon = EDIT_ICON_AMOUNT;
		break;

	case PRESET_CARET_DESCRIPTION:
		icon = EDIT_ICON_DESCRIPT;
		break;

	default:
		icon = EDIT_ICON_DATE;
		break;
	}

	return icon;
}


/**
 * If transaction deletion is enabled, delete the contents of the transaction
 * at the edit line from the file. The transaction will be left in place, but
 * will be blank.
 *
 * \param *file		The file to operate on.
 */

static void edit_delete_line_transaction_content(struct file_block *file)
{
	unsigned transaction;

	if (file == NULL || file->transacts == NULL)
		return;

	/* Only start if the delete line option is enabled, the file is the
	 * current entry window, and the line is in range.
	 */

	if (!config_opt_read("AllowTransDelete") || file == NULL || file->transacts == NULL ||
			edit_entry_window != file->transacts || !transact_valid(file->transacts, file->transacts->entry_line))
		return;

	transaction = file->transacts->transactions[file->transacts->entry_line].sort_index;

	/* Take the transaction out of the fully calculated results. */

	account_remove_transaction(file, transaction);

	/* Blank out all of the transaction. */

	file->transacts->transactions[transaction].date = NULL_DATE;
	file->transacts->transactions[transaction].from = NULL_ACCOUNT;
	file->transacts->transactions[transaction].to = NULL_ACCOUNT;
	file->transacts->transactions[transaction].flags = TRANS_FLAGS_NONE;
	file->transacts->transactions[transaction].amount = NULL_CURRENCY;
	*(file->transacts->transactions[transaction].reference) = '\0';
	*(file->transacts->transactions[transaction].description) = '\0';

	/* Put the transaction back into the sorted totals. */

	account_restore_transaction(file, transaction);

	/* Mark the data as unsafe and perform any post-change recalculations that may affect
	 * the order of the transaction data.
	 */

	file->sort_valid = FALSE;

	accview_rebuild_all(file);

	edit_refresh_line_content(file->transacts->transaction_window, -1, -1);
	edit_set_line_shading(file);

	file_set_data_integrity(file, TRUE);
}


/**
 * Handle keypresses in an edit line (and hence a transaction window). Process
 * any function keys, then pass content keys on to the edit handler.
 *
 * \param *file		The file to pass they keys to.
 * \param *key		The Wimp's key event block.
 * \return		TRUE if the key was handled; FALSE if not.
 */

osbool edit_process_keypress(struct file_block *file, wimp_key *key)
{
	wimp_caret	caret;
	wimp_i		icon;
	int		transaction, previous;


	if (file == NULL || file->transacts == NULL)
		return FALSE;

	if (key->c == wimp_KEY_F10 + wimp_KEY_CONTROL) {

		/* Ctrl-F10 deletes the whole line. */

		edit_delete_line_transaction_content(file);
	} else if (key->c == wimp_KEY_UP) {

		/* Up and down cursor keys -- move the edit line up or down a row
		 * at a time, refreshing the icon the caret was in first.
		 */

		if (edit_entry_window == file->transacts && file->transacts->entry_line > 0) {
			wimp_get_caret_position(&caret);
			edit_refresh_line_content(edit_entry_window->transaction_window, caret.i, -1);
			edit_place_new_line(file, file->transacts->entry_line - 1);
			wimp_set_caret_position(caret.w, caret.i, caret.pos.x, caret.pos.y - (ICON_HEIGHT+LINE_GUTTER), -1, -1);
			edit_find_line_vertically(file);
		}
	} else if (key->c == wimp_KEY_DOWN) {
		if (edit_entry_window == file->transacts) {
			wimp_get_caret_position(&caret);
			edit_refresh_line_content(edit_entry_window->transaction_window, caret.i, -1);
			edit_place_new_line(file, file->transacts->entry_line + 1);
			wimp_set_caret_position(caret.w, caret.i, caret.pos.x, caret.pos.y + (ICON_HEIGHT+LINE_GUTTER), -1, -1);
			edit_find_line_vertically(file);
		}
	} else if (key->c == wimp_KEY_RETURN || key->c == wimp_KEY_TAB || key->c == wimp_KEY_TAB + wimp_KEY_CONTROL) {

		/* Return and Tab keys -- move the caret into the next icon,
		 * refreshing the icon it was moved from first.  Wrap at the end
		 * of a line.
		 */

		if (edit_entry_window == file->transacts) {
			wimp_get_caret_position(&caret);
			if ((osbyte1(osbyte_SCAN_KEYBOARD, 129, 0) == 0xff) && (file->transacts->entry_line > 0)) {

				/* Test for Ctrl-Tab or Ctrl-Return, and fill down
				 * from the line above if present.
				 *
				 * Find the previous or next transaction. If the
				 * entry line falls within the actual transactions,
				 * we just set up two pointers.  If it is on the
				 * line after the final transaction, add a new one
				 * and again set the pointers.  Otherwise, the
				 * line before MUST be blank, so we do nothing.
				 */

				if (file->transacts->entry_line <= file->transacts->trans_count) {
					if (file->transacts->entry_line == file->transacts->trans_count)
						transact_add_raw_entry(file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");
					transaction = file->transacts->transactions[file->transacts->entry_line].sort_index;
					previous = file->transacts->transactions[file->transacts->entry_line-1].sort_index;
				} else {
					transaction = -1;
					previous = -1;
				}

				/* If there is a transaction to fill in, use appropriate
				 * routines to do the work.
				 */

				if (transaction > -1) {
					switch (caret.i) {
					case EDIT_ICON_DATE:
						edit_change_transaction_date(file, transaction, file->transacts->transactions[previous].date);
						break;

					case EDIT_ICON_FROM:
						edit_change_transaction_account(file, transaction, EDIT_ICON_FROM, file->transacts->transactions[previous].from);
						if ((file->transacts->transactions[previous].flags & TRANS_REC_FROM) !=
								(file->transacts->transactions[transaction].flags & TRANS_REC_FROM))
						edit_toggle_transaction_reconcile_flag(file, transaction, TRANS_REC_FROM);
						break;

					case EDIT_ICON_TO:
						edit_change_transaction_account(file, transaction, EDIT_ICON_TO, file->transacts->transactions[previous].to);
						if ((file->transacts->transactions[previous].flags & TRANS_REC_TO) !=
								(file->transacts->transactions[transaction].flags & TRANS_REC_TO))
						edit_toggle_transaction_reconcile_flag(file, transaction, TRANS_REC_TO);
						break;

					case EDIT_ICON_REF:
						edit_change_transaction_refdesc(file, transaction, EDIT_ICON_REF,
								file->transacts->transactions[previous].reference);
						break;

					case EDIT_ICON_AMOUNT:
						edit_change_transaction_amount(file, transaction, file->transacts->transactions[previous].amount);
						break;

					case EDIT_ICON_DESCRIPT:
						edit_change_transaction_refdesc(file, transaction, EDIT_ICON_DESCRIPT,
								file->transacts->transactions[previous].description);
						break;
					}
				}
			} else {
				/* There's no point refreshing the line if we've
				 * just done a Ctrl- complete, as the relevant subroutines
				 * will already have done the work.
				 */

				edit_refresh_line_content(edit_entry_window->transaction_window, caret.i, -1);
			}

			if (key->c == wimp_KEY_RETURN &&
					((caret.i == EDIT_ICON_DATE     && (file->transacts->sort_order & SORT_MASK) == SORT_DATE) ||
					 (caret.i == EDIT_ICON_FROM     && (file->transacts->sort_order & SORT_MASK) == SORT_FROM) ||
					 (caret.i == EDIT_ICON_TO       && (file->transacts->sort_order & SORT_MASK) == SORT_TO) ||
					 (caret.i == EDIT_ICON_REF      && (file->transacts->sort_order & SORT_MASK) == SORT_REFERENCE) ||
					 (caret.i == EDIT_ICON_AMOUNT   && (file->transacts->sort_order & SORT_MASK) == SORT_AMOUNT) ||
					 (caret.i == EDIT_ICON_DESCRIPT && (file->transacts->sort_order & SORT_MASK) == SORT_DESCRIPTION)) &&
					config_opt_read("AutoSort")) {
				transact_sort(file->transacts);

				if (transact_valid(file->transacts, file->transacts->entry_line)) {
					accview_sort(file, file->transacts->transactions[file->transacts->transactions[file->transacts->entry_line].sort_index].from);
					accview_sort(file, file->transacts->transactions[file->transacts->transactions[file->transacts->entry_line].sort_index].to);
				}
			}

			if (caret.i < EDIT_ICON_DESCRIPT) {
				icon = caret.i + 1;
				if (icon == EDIT_ICON_FROM_REC)
					icon = EDIT_ICON_TO;
				if (icon == EDIT_ICON_TO_REC)
					icon = EDIT_ICON_REF;
				icons_put_caret_at_end(edit_entry_window->transaction_window, icon);
				edit_find_icon_horizontally(file);
			} else {
				if (key->c == wimp_KEY_RETURN)
					edit_place_new_line(file, transact_find_first_blank_line(file));
				else
					edit_place_new_line(file, file->transacts->entry_line + 1);

				icons_put_caret_at_end(edit_entry_window->transaction_window, EDIT_ICON_DATE);
				edit_find_icon_horizontally(file);
				edit_find_line_vertically(file);
			}
		}
	} else if (key->c == (wimp_KEY_TAB + wimp_KEY_SHIFT)) {

		/* Shift-Tab key -- move the caret back to the previous icon,
		 * refreshing the icon moved from first.  Wrap up at the start
		 * of a line.
		 */

		if (edit_entry_window == file->transacts) {
			wimp_get_caret_position(&caret);
			edit_refresh_line_content(edit_entry_window->transaction_window, caret.i, -1);

			if (caret.i > EDIT_ICON_DATE) {
				icon = caret.i - 1;
				if (icon == EDIT_ICON_TO_NAME)
					icon = EDIT_ICON_TO;
				if (icon == EDIT_ICON_FROM_NAME)
					icon = EDIT_ICON_FROM;

				icons_put_caret_at_end(edit_entry_window->transaction_window, icon);
				edit_find_icon_horizontally(file);
				edit_find_line_vertically(file);
			} else {
				if (file->transacts->entry_line > 0) {
					edit_place_new_line(file, file->transacts->entry_line - 1);
					icons_put_caret_at_end(edit_entry_window->transaction_window, EDIT_ICON_DESCRIPT);
					edit_find_icon_horizontally(file);
				}
			}
		}
	} else {
		/* Any unrecognised keys get passed on to the final stage. */

		edit_process_content_keypress(file, key);

		return (key->c != wimp_KEY_F12 &&
				key->c != (wimp_KEY_SHIFT | wimp_KEY_F12) &&
				key->c != (wimp_KEY_CONTROL | wimp_KEY_F12) &&
				key->c != (wimp_KEY_SHIFT | wimp_KEY_CONTROL | wimp_KEY_F12)) ? TRUE : FALSE;
	}

	return TRUE;
}


/**
 * Process content-editing keypresses in the edit line.
 *
 * \param *file		The file to operate on.
 * \param *key		The Wimp's key event data block.
 */

static void edit_process_content_keypress(struct file_block *file, wimp_key *key)
{
	int		line, transaction, i, preset;
	unsigned	previous_date, date, amount, old_acct, old_flags, preset_changes = 0;
	osbool		changed = FALSE, reconciled = FALSE;
	
	preset = NULL_PRESET;
	old_acct = NULL_ACCOUNT;

	if (file == NULL || file->transacts == NULL || edit_entry_window != file->transacts)
		return;

	/* If there is not a transaction entry for the current edit line location
	 * (ie. if this is the first keypress in a new line), extend the transaction
	 * entries to reach the current location.
	 */

	line = file->transacts->entry_line;

	if (line >= file->transacts->trans_count) {
		for (i = file->transacts->trans_count; i <= line; i++) {
			transact_add_raw_entry(file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");
			edit_refresh_line_content(key->w, EDIT_ICON_ROW, -1);
		}
	}

	transaction = file->transacts->transactions[line].sort_index;

	/* Take the transaction out of the fully calculated results.
	 *
	 * Presets occur with the caret in the Date column, so they will have the
	 * transaction correctly removed before anything happens.
	 */

	if (key->i != EDIT_ICON_REF && key->i != EDIT_ICON_DESCRIPT)
		account_remove_transaction(file, transaction);

	/* Process the keypress.
	 *
	 * Care needs to be taken that between here and the call to
	 * account_restore_transaction() that nothing is done which will affect
	 * the sort order of the transaction data. If the order is changed, the
	 * calculated totals will be incorrect until a full recalculation is performed.
	 */

	if (key->i == EDIT_ICON_DATE) {
		if (isalpha(key->c)) {
			preset = preset_find_from_keypress(file, toupper(key->c));

			if (preset != NULL_PRESET && (preset_changes = edit_raw_insert_preset_into_transaction(file, transaction, preset))) {
				changed = TRUE;

				if (preset_changes & (1 << EDIT_ICON_DATE))
					file->sort_valid = FALSE;
			}
		} else {
			if (key->c == wimp_KEY_F1) {
				date_convert_to_string(date_today(), buffer_date, DATE_FIELD_LEN);
				wimp_set_icon_state(key->w, EDIT_ICON_DATE, 0, 0);
				icons_replace_caret_in_window(key->w);
			}

			previous_date = (line > 0) ? file->transacts->transactions[file->transacts->transactions[line - 1].sort_index].date : NULL_DATE;
			date = date_convert_from_string(buffer_date, previous_date, 0);
			if (date != file->transacts->transactions[transaction].date) {
				file->transacts->transactions[transaction].date = date;
				changed = TRUE;
				file->sort_valid = 0;
			}
		}
	} else if (key->i == EDIT_ICON_FROM) {
		/* Look up the account ident as it stands, store the result and
		 * update the name field.  The reconciled flag is set if the account
		 * changes to an income heading; else it is cleared.
		 */

		old_acct = file->transacts->transactions[transaction].from;
		old_flags = file->transacts->transactions[transaction].flags;

		file->transacts->transactions[transaction].from = account_lookup_field(file, key->c, ACCOUNT_IN | ACCOUNT_FULL,
				file->transacts->transactions[transaction].from, &reconciled,
				file->transacts->transaction_window, EDIT_ICON_FROM, EDIT_ICON_FROM_NAME, EDIT_ICON_FROM_REC);

		if (reconciled == TRUE)
			file->transacts->transactions[transaction].flags |= TRANS_REC_FROM;
		else
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_FROM;

		edit_set_line_shading(file);

		if (old_acct != file->transacts->transactions[transaction].from || old_flags != file->transacts->transactions[transaction].flags)
			changed = TRUE;
	} else if (key->i == EDIT_ICON_TO) {
		/* Look up the account ident as it stands, store the result and
		 * update the name field.
		 */

		old_acct = file->transacts->transactions[transaction].to;
		old_flags = file->transacts->transactions[transaction].flags;

		file->transacts->transactions[transaction].to = account_lookup_field(file, key->c, ACCOUNT_OUT | ACCOUNT_FULL,
				file->transacts->transactions[transaction].to, &reconciled,
				file->transacts->transaction_window, EDIT_ICON_TO, EDIT_ICON_TO_NAME, EDIT_ICON_TO_REC);

		if (reconciled == TRUE)
			file->transacts->transactions[transaction].flags |= TRANS_REC_TO;
		else
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_TO;

		edit_set_line_shading(file);

		if (old_acct != file->transacts->transactions[transaction].to || old_flags != file->transacts->transactions[transaction].flags)
			changed = TRUE;
	} else if (key->i == EDIT_ICON_REF) {
		if (key->c == wimp_KEY_F1) {
			account_get_next_cheque_number(file, file->transacts->transactions[transaction].from,
					file->transacts->transactions[transaction].to, 1, buffer_reference, sizeof(buffer_reference));
			wimp_set_icon_state(key->w, EDIT_ICON_REF, 0, 0);
			icons_replace_caret_in_window(key->w);
		}

		if (strcmp(file->transacts->transactions[transaction].reference, buffer_reference) != 0) {
			strcpy(file->transacts->transactions[transaction].reference, buffer_reference);
			changed = TRUE;
		}
	} else if (key->i == EDIT_ICON_AMOUNT) {
		amount = currency_convert_from_string(buffer_amount);
		if (amount != file->transacts->transactions[transaction].amount) {
			file->transacts->transactions[transaction].amount = amount;
			changed = TRUE;
		}
	} else if (key->i == EDIT_ICON_DESCRIPT) {
		if (key->c == wimp_KEY_F1) {
			find_complete_description(file, line, buffer_description, DESCRIPT_FIELD_LEN);
			wimp_set_icon_state(key->w, EDIT_ICON_DESCRIPT, 0, 0);
			icons_replace_caret_in_window(key->w);
		}

		if (strcmp(file->transacts->transactions[transaction].description, buffer_description) != 0) {
			strcpy(file->transacts->transactions[transaction].description, buffer_description);
			changed = TRUE;
		}
	}

	/* Add the transaction back into the accounts calculations.
	 *
	 * From this point on, it is now OK to change the sort order of the
	 * transaction data again!
	 */

	if (key->i != EDIT_ICON_REF && key->i != EDIT_ICON_DESCRIPT)
		account_restore_transaction(file, transaction);

	/* Mark the data as unsafe and perform any post-change recalculations that
	 * may affect the order of the transaction data.
	 */

	if (changed == TRUE) {
		file_set_data_integrity(file, TRUE);

		if (preset != NULL_PRESET) {
			/* There is a special case for a preset, since although the caret
			 * may have been in the Date column, the effects of the update could
			 *  affect all columns and have much wider ramifications.  This is
			 * the only update code that runs for preset entries.
			 *
			 * Unlike all the other options, presets must refresh the line on screen too.
			 */

			accview_rebuild_all(file);

			edit_refresh_line_content(file->transacts->transaction_window, -1, -1);
			edit_set_line_shading(file);
			icons_put_caret_at_end(file->transacts->transaction_window,
			edit_convert_preset_icon_number(preset_get_caret_destination(file, preset)));

			/* If we're auto-sorting, and the sort column has been updated as
			 * part of the preset, then do an auto sort now.
			 *
			 * We will always sort if the sort column is Date, because pressing
			 * a preset key is analagous to hitting Return.
			 */

			if ((((file->transacts->sort_order & SORT_MASK) == SORT_DATE) ||
					((preset_changes & (1 << EDIT_ICON_FROM))
						&& ((file->transacts->sort_order & SORT_MASK) == SORT_FROM)) ||
					((preset_changes & (1 << EDIT_ICON_TO))
						&& ((file->transacts->sort_order & SORT_MASK) == SORT_TO)) ||
					((preset_changes & (1 << EDIT_ICON_REF))
						&& ((file->transacts->sort_order & SORT_MASK) == SORT_REFERENCE)) ||
					((preset_changes & (1 << EDIT_ICON_AMOUNT))
						&& ((file->transacts->sort_order & SORT_MASK) == SORT_AMOUNT)) ||
					((preset_changes & (1 << EDIT_ICON_DESCRIPT))
						&& ((file->transacts->sort_order & SORT_MASK) == SORT_DESCRIPTION))) &&
					config_opt_read("AutoSort")) {
				transact_sort(file->transacts);
				if (transact_valid(file->transacts, file->transacts->entry_line)) {
					accview_sort(file, file->transacts->transactions[file->transacts->transactions[file->transacts->entry_line].sort_index].from);
					accview_sort(file, file->transacts->transactions[file->transacts->transactions[file->transacts->entry_line].sort_index].to);
				}
			}
		} else if (key->i == EDIT_ICON_DATE) {
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

			accview_recalculate_all(file);
		} else if (key->i == EDIT_ICON_FROM) {
			/* Trust that any accuout views that are open must be based on a
			 * valid date order, and only rebuild those that are directly affected.
			 */

			accview_rebuild(file, old_acct);
			transaction = file->transacts->transactions[line].sort_index;
			accview_rebuild(file, file->transacts->transactions[transaction].from);
			transaction = file->transacts->transactions[line].sort_index;
			accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);
		} else if (key->i == EDIT_ICON_TO) {
			/* Trust that any accuout views that are open must be based on a
			 * valid date order, and only rebuild those that are directly affected.
			 */

			accview_rebuild(file, old_acct);
			transaction = file->transacts->transactions[line].sort_index;
			accview_rebuild(file, file->transacts->transactions[transaction].to);
			transaction = file->transacts->transactions[line].sort_index;
			accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
		} else if (key->i == EDIT_ICON_REF) {
			accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
			accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);
		} else if (key->i == EDIT_ICON_AMOUNT) {
			accview_recalculate(file, file->transacts->transactions[transaction].from, transaction);
			accview_recalculate(file, file->transacts->transactions[transaction].to, transaction);
		} else if (key->i == EDIT_ICON_DESCRIPT) {
			 accview_redraw_transaction(file, file->transacts->transactions[transaction].from, transaction);
			accview_redraw_transaction(file, file->transacts->transactions[transaction].to, transaction);
		}
	}

	/* Finally, look for the next reconcile line if that is necessary.
	 *
	 * This is done last, as the only hold we have over the line being edited
	 * is the edit line location.  Move that and we've lost everything.
	 */

	if ((key->i == EDIT_ICON_FROM || key->i == EDIT_ICON_TO) &&
			(key->c == '+' || key->c == '=' || key->c == '-' || key->c == '_'))
		transact_find_next_reconcile_line(file, FALSE);
}

/* ==================================================================================================================
 * Descript completion.
 */

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

static char *find_complete_description(struct file_block *file, int line, char *buffer, size_t length)
{
	int i, t;

	if (file == NULL || file->transacts == NULL || buffer == NULL)
		return buffer;

	if (line >= file->transacts->trans_count)
		line = file->transacts->trans_count - 1;

	for (i = line - 1; i >= 0; i--) {
		t = file->transacts->transactions[i].sort_index;

		if (*(file->transacts->transactions[t].description) != '\0' &&
				string_nocase_strstr(file->transacts->transactions[t].description, buffer) == file->transacts->transactions[t].description) {
			strncpy(buffer, file->transacts->transactions[t].description, length);
			break;
		}
	}

	return buffer;
}

