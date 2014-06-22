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

#include "filing.h"

enum transact_list_menu_type {
	REFDESC_MENU_NONE = 0,
	REFDESC_MENU_REFERENCE,
	REFDESC_MENU_DESCRIPTION
};


/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define TRANSACT_PANE_COL_MAP "0;1;2,3,4;5,6,7;8;9;10"



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

void decode_transact_window_help (char *buffer, wimp_w window, wimp_i icon, os_coord pos, wimp_mouse_state buttons);

int locate_transaction_in_transact_window (file_data *file, int transaction);






















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












/* Transaction handling */

void add_raw_transaction (file_data *file, unsigned date, int from, int to, unsigned flags, int amount,
                          char *ref, char *description);
int is_transaction_blank (file_data *file, int line);
void strip_blank_transactions (file_data *file);




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

int transact_read_file(file_data *file, FILE *in, char *section, char *token, char *value, int format, osbool *unknown_data);


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

