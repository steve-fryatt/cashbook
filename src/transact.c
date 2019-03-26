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

//	tran_t			sort_index;					/**< Point to another transaction, to allow the transaction window to be sorted.	*/
//	tran_t			saved_sort;					/**< Preserve the transaction window sort order across transaction data sorts.		*/
//	tran_t			sort_workspace;					/**< Workspace used by the sorting code.						*/
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

/* Static Function Prototypes. */

static osbool			transact_is_blank(struct file_block *file, tran_t transaction);

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
	if (file == NULL || file->transacts == NULL)
		return;

	transact_list_window_scroll_to_end(file->transacts->transact_window, direction);
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
	if (file == NULL || file->transacts == NULL)
		return NULL_TRANSACTION;

	return transact_list_window_find_nearest_centre(file->transacts->transact_window, account);
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

	return transact_list_window_get_line_from_transaction(file->transacts->transact_window, transaction);
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
 * Return the file associated with a transactions instance.
 *
 * \param *instance		The transactions instance to query.
 * \return			The associated file, or NULL.
 */

struct file_block *transact_get_file(struct transact_block *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->file;
}


/**
 * Find the display line number of the current transaction entry line.
 *
 * \param *file			The file to interrogate.
 * \return			The display line number of the line with the caret.
 */

int transact_get_caret_line(struct file_block *file)
{
	if (file == NULL || file->transacts == NULL)
		return 0;

	return transact_list_window_get_caret_line(file->transacts->transact_window);
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
// \TODO
//		found = FALSE;

//		for (line = 0; line < transaction; line++) {
//			if (file->transacts->transactions[line].sort_index == transaction)
//				found = TRUE;

//			if (found == TRUE)
//				file->transacts->transactions[line].sort_index = file->transacts->transactions[line + 1].sort_index;
//		}

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
//\TODO
//		transact_force_window_redraw(file->transacts, 0, old_count, TRANSACT_PANE_ROW);
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
	if (file == NULL || file->transacts == NULL || !transact_valid(file->transacts, transaction)) {
		if (buffer != NULL && length > 0) {
			*buffer = '\0';
			return buffer;
		}

		return NULL;
	}

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
// \TODO
	return NULL; //file->transacts->transactions[transaction].sort_workspace;
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
 * \param reconciled	TRUE if the account is reconciled; else FALSE.
 */

void transact_change_account(struct file_block *file, tran_t transaction, enum transact_field target, acct_t new_account, osbool reconciled)
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

		if (reconciled)
			file->transacts->transactions[transaction].flags |= TRANS_REC_FROM;
		else
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_FROM;

		if (old_acct != file->transacts->transactions[transaction].from || old_flags != file->transacts->transactions[transaction].flags)
			changed = TRUE;
		break;
	case TRANSACT_FIELD_TO:
		old_acct = file->transacts->transactions[transaction].to;
		old_flags = file->transacts->transactions[transaction].flags;

		file->transacts->transactions[transaction].to = new_account;

		if (reconciled)
			file->transacts->transactions[transaction].flags |= TRANS_REC_TO;
		else
			file->transacts->transactions[transaction].flags &= ~TRANS_REC_TO;

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
	default:
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
 * Sort the contents of the transaction window based on the file's sort setting.
 *
 * \param *windat		The transaction window instance to sort.
 */

void transact_sort(struct transact_block *windat)
{
	if (windat == NULL)
		return;

	transact_list_window_sort(windat->transact_window);
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

// \TODO
//	for (i=0; i < file->transacts->trans_count; i++) {
//		file->transacts->transactions[file->transacts->transactions[i].sort_index].saved_sort = i;	/* Record transaction window lines. */
//		file->transacts->transactions[i].sort_index = i;						/* Record old transaction locations. */
//	}

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
// \TODO
//	for (i=0; i < file->transacts->trans_count; i++)
//		file->transacts->transactions[file->transacts->transactions[i].sort_index].sort_workspace = i;

	accview_reindex_all(file);

// \TODO
//	for (i=0; i < file->transacts->trans_count; i++)
//		file->transacts->transactions[file->transacts->transactions[i].saved_sort].sort_index = i;

//	transact_force_window_redraw(file->transacts, 0, file->transacts->trans_count - 1, TRANSACT_PANE_ROW);

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

	if (file == NULL || file->transacts == NULL)
		return;

	fprintf(out, "\n[Transactions]\n");

	fprintf(out, "Entries: %x\n", file->transacts->trans_count);

	transact_list_window_write_file(file->transacts->transact_window, out);

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
			transact_list_window_read_file_wincolumns(file->transacts->transact_window, filing_get_format(in), filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "SortOrder")){
			transact_list_window_read_file_sortorder(file->transacts->transact_window, filing_get_text_value(in, NULL, 0));
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

