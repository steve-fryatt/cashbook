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
 * Set the extent of the transaction window for the specified file.
 *
 * \param *windat		The transaction list window to update.
 */

void transact_list_window_set_extent(struct transact_list_window *windat);


/**
 * Get the window state of the transaction window belonging to
 * the specified file.
 *
 * \param *file			The file containing the window.
 * \param *state		The structure to hold the window state.
 * \return			Pointer to an error block, or NULL on success.
 */

os_error *transact_list_window_get_state(struct transact_list_window *windat, wimp_window_state *state);


/**
 * Recreate the title of the given Transaction window.
 *
 * \param *windat		The transaction window to rebuild the title for.
 */

void transact_list_window_build_title(struct transact_list_window *windat);


/**
 * Force the complete redraw of a given Transaction window.
 *
 * \param *windat		The transaction window to redraw.
 */

void transact_list_window_redraw_all(struct transact_list_window *windat);


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
 * Sort the contents of the transaction list window based on the instance's
 * sort setting.
 *
 * \param *windat		The transaction window instance to sort.
 */

void transact_list_window_sort(struct transact_list_window *windat);

#endif

