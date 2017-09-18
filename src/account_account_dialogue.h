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
 * \file: account_account_dialogue.h
 *
 * Account Heading Edit dialogue interface.
 */

#ifndef CASHBOOK_ACCOUNT_ACCOUNT_DIALOGUE
#define CASHBOOK_ACCOUNT_ACCOUNT_DIALOGUE

#include "account.h"
#include "account_idnum.h"
#include "interest.h"

/**
 * Initialise the Account Edit dialogue.
 */

void account_account_dialogue_initialise(void);


/**
 * Open the Account Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param account		The account number of the account to be edited, or NULL_ACCOUNT.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *name			The initial name to use for the account.
 * \param *ident		The initial ident to use for the account.
 * \param credit_limit		The initial credit limit to use for the account.
 * \param opening_balance	The initial opening balance to use for the account.
 * \param *cheque_number	The initial cheque number to use for the account.
 * \param *payin_number		The initial paying in number to use for the account.
 * \param interest_rate		The initial interest rate to use for the account.
 * \param offset_against	The initial offset account to use for the account.
 * \param *account_num		The initial account number to use for the account.
 * \param *sort_code		The initial sort code to use for the account.
 * \param **address		The initial address details to use for the account, or NULL.
 */

void account_account_dialogue_open(wimp_pointer *ptr, struct account_block *owner, acct_t account,
		osbool (*update_callback)(struct account_block *, acct_t, char *, char *, amt_t, amt_t, struct account_idnum *, struct account_idnum *, acct_t, char *, char *, char *[ACCOUNT_ADDR_LEN]),
		osbool (*delete_callback)(struct account_block *, acct_t),
		char *name, char *ident, amt_t credit_limit, amt_t opening_balance,
		struct account_idnum *cheque_number, struct account_idnum *payin_number,
		rate_t interest_rate, acct_t offset_against,
		char *account_num, char *sort_code, char *address[ACCOUNT_ADDR_LEN]);


/**
 * Force the closure of the account section edit dialogue if it relates
 * to a given accounts instance.
 *
 * \param *parent		The parent of the dialogue to be closed,
 *				or NULL to force close.
 */

void account_account_dialogue_force_close(struct account_block *parent);


/**
 * Check whether the Edit Section dialogue is open for a given accounts
 * instance.
 *
 * \param *parent		The accounts list instance to check.
 * \return			TRUE if the dialogue is open; else FALSE.
 */

osbool account_account_dialogue_is_open(struct account_block *parent);

#endif

