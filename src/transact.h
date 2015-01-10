/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: transact.h
 *
 * Transaction and transaction window implementation.
 */

#ifndef CASHBOOK_TRANSACT
#define CASHBOOK_TRANSACT

#include "oslib/wimp.h"

#include "global.h"

#include "currency.h"
#include "filing.h"

/* Transaction flags (bitwise allocation) */

enum transact_flags {
	TRANS_FLAGS_NONE = 0,

	TRANS_REC_FROM = 0x0001,						/**< Reconciled from					*/
	TRANS_REC_TO = 0x0002,							/**< Reconciled to					*/

	/* The following flags are used by standing orders. */

	TRANS_SKIP_FORWARD = 0x0100,						/**< Move forward to avoid illegal days.		*/
	TRANS_SKIP_BACKWARD = 0x0200,						/**< Move backwards to avoid illegal days.		*/

	/* The following flags are used by presets. */

	TRANS_TAKE_TODAY = 0x1000,						/**< Always take today's date				*/
	TRANS_TAKE_CHEQUE = 0x2000						/**< Always take the next cheque number			*/
};

enum transact_list_menu_type {
	REFDESC_MENU_NONE = 0,
	REFDESC_MENU_REFERENCE,
	REFDESC_MENU_DESCRIPTION
};

enum transact_field {
	TRANSACT_FIELD_NONE = 0,
	TRANSACT_FIELD_DATE = 0x01,
	TRANSACT_FIELD_FROM = 0x02,
	TRANSACT_FIELD_TO = 0x04,
	TRANSACT_FIELD_AMOUNT = 0x08,
	TRANSACT_FIELD_REF = 0x10,
	TRANSACT_FIELD_DESC = 0x20
};


/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define TRANSACT_PANE_COL_MAP "0;1;2,3,4;5,6,7;8;9;10"


/**
 * Convert an internal transaction number into a user-facing one.
 */

#define transact_get_transaction_number(x) ((x) + 1)


/**
 * Convert a user-facing transaction number into an internal one.
 */

#define transact_find_transaction_number(x) ((x) - 1)


/**
 * Initialise the transaction system.
 *
 * \param *sprites		The application sprite area.
 */

void transact_initialise(osspriteop_area *sprites);


/**
 * Create and open a Transaction List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void transact_open_window(file_data *file);


/**
 * Close and delete a Transaction List Window associated with the given
 * transaction window block.
 *
 * \param *windat		The window to delete.
 */

void transact_delete_window(struct transaction_window *windat);


/**
 * Set the extent of the transaction window for the specified file.
 *
 * \TODO -- Should this be non-static?
 *
 * \param *file			The file to update.
 */

void transact_set_window_extent(file_data *file);


void minimise_transaction_window_extent (file_data *file);
void build_transaction_window_title (file_data *file);
void force_transaction_window_redraw (file_data *file, int from, int to);
void update_transaction_window_toolbar (file_data *file);

void scroll_transaction_window_to_end (file_data *file, int dir);

int find_transaction_window_centre (file_data *file, int account);





/**
 * Find the display line in a transaction window which points to the
 * specified transaction under the applied sort.
 *
 * \param *file			The file to use the transaction window in.
 * \param transaction		The transaction to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

int transact_get_line_from_transaction(file_data *file, int transaction);


/**
 * Find the transaction which corresponds to a display line in a transaction
 * window.
 *
 * \param *file			The file to use the transaction window in.
 * \param line			The display line to return the transaction for.
 * \return			The appropriate transaction, or NULL_TRANSACTION.
 */

int transact_get_transaction_from_line(file_data *file, int line);























/**
 * Build a Reference or Drescription Complete menu for a given file.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \param start_line		The line of the window to start from.
 * \return			The menu block, or NULL.
 */

wimp_menu *transact_complete_menu_build(file_data *file, enum transact_list_menu_type menu_type, int start_line);


/**
 * Destroy any Reference or Description Complete menu which is currently open.
 */

void transact_complete_menu_destroy(void);


/**
 * Prepare the currently active Reference or Description menu for opening or
 * reopening, by shading lines which shouldn't be selectable.
 *
 * \param line			The line that the menu is open over.
 */

void transact_complete_menu_prepare(int line);


/**
 * Decode menu selections from the Reference or Description menu.
 *
 * \param *selection		The menu selection to be decoded.
 * \return			Pointer to the selected text, or NULL if
 *				the Cheque Number field was selected or there
 *				was not valid menu open.
 */

char *transact_complete_menu_decode(wimp_selection *selection);


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

void transact_add_raw_entry(file_data *file, date_t date, acct_t from, acct_t to, enum transact_flags flags,
		amt_t amount, char *ref, char *description);


/**
 * Clear a transaction from a file, returning it to an empty state. Note that
 * the transaction remains in-situ, and no memory is cleared.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to be cleared.
 */

void transact_clear_raw_entry(file_data *file, int transaction);


/**
 * Strip any blank transactions from the end of the file, releasing any memory
 * associated with them. To be sure to remove all blank transactions, it is
 * necessary to sort the transaction list before calling this function.
 *
 * \param *file			The file to be processed.
 */

void transact_strip_blanks_from_end(file_data *file);


/**
 * Return the date of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the date for.
 * \return			The date of the transaction, or NULL_DATE.
 */

date_t transact_get_date(file_data *file, int transaction);


/**
 * Return the from account of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the from account for.
 * \return			The from account of the transaction, or NULL_ACCOUNT.
 */

acct_t transact_get_from(file_data *file, int transaction);


/**
 * Return the to account of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the to account for.
 * \return			The to account of the transaction, or NULL_ACCOUNT.
 */

acct_t transact_get_to(file_data *file, int transaction);


/**
 * Return the transaction flags for a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the flags for.
 * \return			The flags of the transaction, or TRANS_FLAGS_NONE.
 */

enum transact_flags transact_get_flags(file_data *file, int transaction);


/**
 * Return the amount of a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the amount of.
 * \return			The amount of the transaction, or NULL_CURRENCY.
 */

amt_t transact_get_amount(file_data *file, int transaction);


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

char *transact_get_reference(file_data *file, int transaction, char *buffer, size_t length);


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

char *transact_get_description(file_data *file, int transaction, char *buffer, size_t length);


/**
 * Return the sort workspace for a transaction.
 *
 * \param *file			The file containing the transaction.
 * \param transaction		The transaction to return the workspace of.
 * \return			The sort workspace for the transaction, or 0.
 */

int transact_get_sort_workspace(file_data *file, int transaction);


/**
 * Force the closure of the Transaction List sort and edit windows if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void transact_force_windows_closed(file_data *file);


/**
 * Sort the contents of the transaction window based on the file's sort setting.
 *
 * \param *file			The file to sort.
 */

void transact_sort(file_data *file);


/**
 * Sort the underlying transaction data within a file, to put them into date order.
 * This does not affect the view in the transaction window -- to sort this, use
 * transact_sort().  As a result, we do not need to look after the location of
 * things like the edit line; it does need to keep track of transactions[].sort_index,
 * however.
 *
 * \param *file			The file to be sorted.
 */

void transact_sort_file_data(file_data *file);



/* Finding transactions */

void find_next_reconcile_line (file_data *file, int set);

int find_first_blank_line (file_data *file);








/**
 * Save the transaction details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void transact_write_file(file_data *file, FILE *out);


/**
 * Read transaction details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param format		The format number of the file.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

enum config_read_status transact_read_file(file_data *file, FILE *in, char *section, char *token, char *value, int format, osbool *unknown_data);


/**
 * Search the transactions, returning the entry from nearest the target date.
 * The transactions will be sorted into order if they are not already.
 *
 * \param *file			The file to search in.
 * \param target		The target date.
 * \return			The transaction number for the date, or
 *				NULL_TRANSACTION.
 */

int transact_find_date(file_data *file, date_t target);


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

enum transact_field transact_search(file_data *file, int *line, osbool back, osbool case_sensitive, osbool logic_and,
		date_t date, acct_t from, acct_t to, enum transact_flags flags, amt_t amount, char *ref, char *desc);

/**
 * Check the transactions in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool transact_check_account(file_data *file, int account);

#endif

