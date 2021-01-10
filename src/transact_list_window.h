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
 * \file: transact_list_window.h
 *
 * Transaction List Window interface.
 */

#ifndef CASHBOOK_TRANSACT_LIST_WINDOW
#define CASHBOOK_TRANSACT_LIST_WINDOW

/**
 * Initialise the Transaction List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void transact_list_window_initialise(osspriteop_area *sprites);


/**
 * Create a new Transaction List Window instance.
 *
 * \param *parent		The parent transact instance.
 * \return			Pointer to the new instance, or NULL.
 */

struct transact_list_window *transact_list_window_create_instance(struct transact_block *parent);


/**
 * Destroy a Transaction List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void transact_list_window_delete_instance(struct transact_list_window *windat);


/**
 * Create and open a Transaction List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void transact_list_window_open(struct transact_list_window *windat);


/**
 * Return the name of a transaction window column.
 *
 * \param *windat		The transaction list window instance to query.
 * \param field			The field representing the required column.
 * \param *buffer		Pointer to a buffer to take the name.
 * \param len			The length of the supplied buffer.
 * \return			Pointer to the supplied buffer, or NULL.
 */

char *transact_list_window_get_column_name(struct transact_list_window *windat, enum transact_field field, char *buffer, size_t len);


/**
 * Place the caret in a given line in a transaction window, and scroll
 * the line into view.
 *
 * \param *windat		The transaction list window to operate on.
 * \param line			The line (under the current display sort order)
 *				to place the caret in.
 * \param field			The field to place the caret in.
 */

void transact_list_window_place_caret(struct transact_list_window *windat, int line, enum transact_field field);


/**
 * Re-index the transactions in a transaction list window.  This can *only*
 * be done after transact_sort_file_data() has been called, as it requires
 * data set up in the transaction block by that call.
 *
 * \param *windat		The transaction window to reindex.
 */

void transact_list_window_reindex(struct transact_list_window *windat);


/**
 * Force the redraw of one or all of the transactions in a given
 * Transaction List window.
 *
 * \param *windat		The transaction window to redraw.
 */

void transact_list_window_redraw(struct transact_list_window *windat, tran_t transaction);


/**
 * Update the state of the buttons in a transaction window toolbar.
 *
 * \param *windat		The transaction list window to update.
 */

void transact_list_window_update_toolbar(struct transact_list_window *windat);


/**
 * Bring a transaction window to the top of the window stack.
 *
 * \param *windat		The transaction list window to bring up.
 */

void transact_list_window_bring_to_top(struct transact_list_window *windat);


/**
 * Scroll a transaction window to either the top (home) or the end.
 *
 * \param *windat		The transaction list window to be scrolled.
 * \param direction		The direction to scroll the window in.
 */

void transact_list_window_scroll_to_end(struct transact_list_window *windat, enum transact_scroll_direction direction);


/**
 * Return the transaction number of the transaction nearest to the centre of
 * the visible area of the transaction list window which references a given
 * account.
 *
 * \param *windat		The transaction list window to be searched.
 * \param account		The account to search for.
 * \return			The transaction found, or NULL_TRANSACTION.
 */

int transact_list_window_find_nearest_centre(struct transact_list_window *windat, acct_t account);


/**
 * Find the display line in a transaction window which points to the
 * specified transaction under the applied sort.
 *
 * \param *windat		The transaction list window to search in
 * \param transaction		The transaction to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

int transact_list_window_get_line_from_transaction(struct transact_list_window *windat, tran_t transaction);


/**
 * Find the transaction which corresponds to a display line in a given
 * transaction window.
 *
 * \param *windat		The transaction list window to search in
 * \param line			The display line to return the transaction for.
 * \return			The appropriate transaction, or NULL_TRANSACTION.
 */

tran_t transact_list_window_get_transaction_from_line(struct transact_list_window *windat, int line);


/**
 * Find the display line number of the current transaction entry line.
 *
 * \param *windat		The transaction list window to interrogate.
 * \return			The display line number of the line with the caret.
 */

int transact_list_window_get_caret_line(struct transact_list_window *windat);


/**
 * Insert a preset into a pre-existing transaction, taking care of updating all
 * the file data in a clean way.
 *
 * \param *windat		The window to edit.
 * \param line			The line in the transaction window to update.
 * \param preset		The preset to insert into the transaction.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool transact_list_window_insert_preset_into_line(struct transact_list_window *windat, int line, preset_t preset);



/**
 * Find and return the line number of the first blank line in a file, based on
 * display order.
 *
 * \param *windat		The transaction list window to search.
 * \return			The first blank display line.
 */

int transact_list_window_find_first_blank_line(struct transact_list_window *windat);


/**
 * Search the transaction list from a file for a set of matching entries.
 *
 * \param *windat		The transaction list window to search in.
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

enum transact_field transact_list_window_search(struct transact_list_window *windat, int *line, osbool back, osbool case_sensitive, osbool logic_and,
		date_t date, acct_t from, acct_t to, enum transact_flags flags, amt_t amount, char *ref, char *desc);


/**
 * Sort the contents of the transaction list window based on the instance's
 * sort setting.
 *
 * \param *windat		The transaction window instance to sort.
 */

void transact_list_window_sort(struct transact_list_window *windat);

/**
 * Initialise the contents of the transaction list window, creating an
 * entry for each of the required transactions.
 *
 * \param *windat		The transaction list window instance to initialise.
 * \param transacts		The number of transactionss to insert.
 * \return			TRUE on success; FALSE on failure.
 */

osbool transact_list_window_initialise_entries(struct transact_list_window *windat, int transacts);


/**
 * Add a new transaction to an instance of the transaction list window.
 *
 * \param *windat		The transaction list window instance to add to.
 * \param transaction		The transaction index to add.
 * \return			TRUE on success; FALSE on failure.
 */

osbool transact_list_window_add_transaction(struct transact_list_window *windat, tran_t transaction);


/**
 * Remove a transaction from an instance of the transaction list window,
 * and update the other entries to allow for its deletion.
 *
 * \param *windat		The transaction list window instance to remove from.
 * \param transaction		The transaction index to remove.
 * \return			TRUE on success; FALSE on failure.
 */

osbool transact_list_window_delete_transaction(struct transact_list_window *windat, tran_t transaction);


/**
 * Save the transaction list window details from a window to a CashBook
 * file. This assumes that the caller has already created a suitable section
 * in the file to be written.
 *
 * \param *windat		The window whose details to write.
 * \param *out			The file handle to write to.
 */

void transact_list_window_write_file(struct transact_list_window *windat, FILE *out);


/**
 * Process a WinColumns line from the Transactions section of a file.
 *
 * \param *windat		The window being read in to.
 * \param format		The format of the disc file.
 * \param *columns		The column text line.
 */

void transact_list_window_read_file_wincolumns(struct transact_list_window *windat, int format, char *columns);


/**
 * Process a SortOrder line from the Transactions section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The sort order text line.
 */

void transact_list_window_read_file_sortorder(struct transact_list_window *windat, char *order);

#endif

