/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: account_list_window.h
 *
 * AAccount List Window implementation.
 */

#ifndef CASHBOOK_ACCOUNT_LIST_WINDOW
#define CASHBOOK_ACCOUNT_LIST_WINDOW

#include "oslib/osspriteop.h"
#include "account.h"

/**
 * An Account List Window instance.
 */

struct account_list_window;


/**
 * Initialise the Account List Window.
 */

void account_list_window_initialise(osspriteop_area *sprites);


/**
 * Create a new Account List Window instance.
 *
 * \param *parent		The parent accounts instance.
 * \param type			The type of account that the instance contains.
 * \return			Pointer to the new instance, or NULL.
 */

struct account_list_window *account_list_window_create_instance(struct account_block *parent, enum account_type type);


/**
 * Destroy an Account List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void account_list_window_delete_instance(struct account_list_window *windat);


/**
 * Create and open an Accounts List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void account_list_window_open(struct account_list_window *windat);


/**
 * Add an account to the end of an Account List Window.
 *
 * \param *windat		The Account List Window instance to add
 *				the account to.
 * \param account		The account to add.
 */

void account_list_window_add_account(struct account_list_window *windat, acct_t account);


/**
 * Remove an account from an Account List Window instance.
 *
 * \param *windat		The Account List Window instance.
 * \param account		The account to remove.
 */

void account_list_window_remove_account(struct account_list_window *windat, acct_t account);


/**
 * Recreate the title of an Account List Window.
 *
 * \param *windat		The window instance to update
 */

void account_list_window_build_title(struct account_list_window *windat);


/**
 * Force the complete redraw of an Account List Window.
 *
 * \param *windat		The window instance to redraw.
 */

void account_list_window_redraw_all(struct account_list_window *windat);


/**
 * Find the number of entries in the given Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \return			The number of entries, or 0.
 */

int account_list_window_get_length(struct account_list_window *windat);


/**
 * Return the type of a given line in an Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \param line			The line to return the details for.
 * \return			The type of data on that line.
 */

enum account_line_type account_list_window_get_entry_type(struct account_list_window *windat, int line);


/**
 * Return the account on a given line of an Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \param line			The line to return the details for.
 * \return			The account on that line, or NULL_ACCOUNT if the
 *				line isn't an account.
 */

acct_t account_list_window_get_entry_account(struct account_list_window *windat, int line);


/**
 * Return the text on a given line of an Account List Window instance.
 *
 * \param *windat		The Account List Window instance to query.
 * \param line			The line to return the details for.
 * \return			A volatile pointer to the text on the line,
 *				or NULL.
 */

char *account_list_window_get_entry_text(struct account_list_window *windat, int line);


/**
 * Recalculate the data in the the given Account List window instance
 * (totals, sub-totals, budget totals, etc) and refresh the display.
 *
 * \param *windat		The Account List Window instance to
 *				recalculate.
 */

void account_list_window_recalculate(struct account_list_window *windat);


/**
 * Save an Account List Window's details to a CashBook file
 *
 * \param *windat		The Account List Window instance to write
 * \param *out			The file handle to write to.
 */

void account_list_window_write_file(struct account_list_window *windat, FILE *out);


/**
 * Read account list details from a CashBook file into an Account List
 * Window instance.
 *
 * \param *windat		The Account List Window instance to populate.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool account_list_window_read_file(struct account_list_window *windat, struct filing_block *in);

#endif

