/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * The requested action from the dialogue.
 */

enum account_account_dialogue_action {
	/**
	 * No action defined.
	 */
	ACCOUNT_ACCOUNT_DIALOGUE_ACTION_NONE,

	/**
	 * Create or update the account using the supplied details.
	 */
	ACCOUNT_ACCOUNT_DIALOGUE_ACTION_OK,

	/**
	 * Delete the account.
	 */
	ACCOUNT_ACCOUNT_DIALOGUE_ACTION_DELETE,

	/**
	 * Open the Interest Rates dialogue.
	 */
	ACCOUNT_ACCOUNT_DIALOGUE_ACTION_RATES
};

/**
 * The account data held by the dialogue.
 */

struct account_account_dialogue_data {
	/**
	 * The requested action from the dialogue.
	 */
	enum account_account_dialogue_action	action;

	/**
	 * The target account.
	 */
	acct_t					account;

	/**
	 * The name for the account.
	 */
	char					name[ACCOUNT_NAME_LEN];

	/**
	 * The ident for the account.
	 */
	char					ident[ACCOUNT_IDENT_LEN];

	/**
	 * The credit limit for the account.
	 */
	amt_t					credit_limit;

	/**
	 * The opening balance for the account.
	 */
	amt_t					opening_balance;

	/**
	 * The cheque number for the account.
	 */
	struct account_idnum			cheque_number;

	/**
	 * The paying in for the account.
	 */
	struct account_idnum			payin_number;

	/**
	 * The interest rate for the account.
	 */
	rate_t					interest_rate;

	/**
	 * The offset account for the account.
	 */
	acct_t					offset_against;

	/**
	 * The starting account number for the account.
	 */
	char					account_num[ACCOUNT_NO_LEN];

	/**
	 * The sort code for the account.
	 */
	char					sort_code[ACCOUNT_SRTCD_LEN];

	/**
	 * The address information for the account.
	 */
	char					address[ACCOUNT_ADDR_LINES][ACCOUNT_ADDR_LEN];
};

/**
 * Initialise the Account Edit dialogue.
 */

void account_account_dialogue_initialise(void);


/**
 * Open the Account Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void account_account_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*callback)(void *, struct account_account_dialogue_data *), struct account_account_dialogue_data *content);

#endif

