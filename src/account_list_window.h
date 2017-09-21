* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: account_list_window.h
 *
 * AAccount List Window implementation.
 */

#ifndef CASHBOOK_ACCOUNT_LIST_WINDOW
#define CASHBOOK_ACCOUNT_LIST_WINDOW

#include "oslib/osspriteop.h"

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
 * \param entry			The instance's "entry" value.
 * \return			Pointer to the new instance, or NULL.
 */

struct account_list_window *account_list_window_create_instance(struct account_block *parent, enum account_type type, int entry);


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

void account_list_window_open(struct account_list_block *windat);


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
 * Force the complete redraw of an Account List Window.
 *
 * \param *windat		The window instance to redraw.
 */

void account_list_window_redraw_all(struct account_list_window *windat);


#endif

