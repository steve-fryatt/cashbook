/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: purge_dialogue.h
 *
 * High-level purge dialogue interface.
 */

#ifndef CASHBOOK_PURGE_DIALOGUE
#define CASHBOOK_PURGE_DIALOGUE

#include "date.h"
#include "oslib/types.h"

/**
 * The purge data held by the dialogue.
 */

struct purge_dialogue_data {
	/* Purge options. */

	osbool			remove_transactions;			/**< TRUE to remove reconciled transactions.			*/
	osbool			remove_accounts;			/**< TRUE to remove unused accounts.				*/
	osbool			remove_headings;			/**< TRUE to remove unused headings.				*/
	osbool			remove_sorders;				/**< TRUE to remove completed standing orders.			*/

	/* Transaction date limits */

	date_t			keep_from;				/**< A date after which to retain reconciled transactions.	*/
};

/**
 * Initialise the purge dialogue.
 */

void purge_dialogue_initialise(void);

/**
 * Open the purge dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The goto dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void purge_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(void *, struct purge_dialogue_data *),
		struct purge_dialogue_data *data);

#endif

