/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: account_heading_dialogue.h
 *
 * Account Heading Edit dialogue interface.
 */

#ifndef CASHBOOK_ACCOUNT_HEADING_DIALOGUE
#define CASHBOOK_ACCOUNT_HEADING_DIALOGUE

#include "account.h"

/**
 * Initialise the Heading Edit dialogue.
 */

void account_heading_dialogue_initialise(void);


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param account		The account number of the heading to be edited, or NULL_ACCOUNT.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *name			The initial name to use for the heading.
 * \param *ident		The initial ident to use for the heading.
 * \param budget		The initial budget limit to use for the heading.
 * \param type			The initial incoming/outgoing type for the heading.
 */

void account_heading_dialogue_open(wimp_pointer *ptr, struct account_block *owner, acc_t account,
		osbool (*update_callback)(struct account_block *, acc_t, char *, char *, amt_t, enum account_type),
		osbool (*delete_callback)(struct account_block *, acc_t), char *name, char *ident, amt_t budget, enum account_type type));


/**
 * Force the closure of the account section edit dialogue if it relates
 * to a given accounts list instance.
 *
 * \param *parent		The parent of the dialogue to be closed,
 *				or NULL to force close.
 */

void account_heading_dialogue_force_close(struct account_window *parent);


/**
 * Check whether the Edit Section dialogue is open for a given accounts
 * list instance.
 *
 * \param *parent		The accounts list instance to check.
 * \return			TRUE if the dialogue is open; else FALSE.
 */

osbool account_heading_dialogue_is_open(struct account_window *parent);

#endif

