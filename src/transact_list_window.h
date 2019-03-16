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


#endif

