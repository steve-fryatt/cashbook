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
 * \file: account_heading_dialogue.h
 *
 * Account Heading Edit dialogue interface.
 */

#ifndef CASHBOOK_ACCOUNT_HEADING_DIALOGUE
#define CASHBOOK_ACCOUNT_HEADING_DIALOGUE

#include "account.h"

/**
 * The requested action from the dialogue.
 */

enum account_heading_dialogue_action {
	/**
	 * No action defined.
	 */
	ACCOUNT_HEADING_DIALOGUE_ACTION_NONE,

	/**
	 * Create or update the heading using the supplied details.
	 */
	ACCOUNT_HEADING_DIALOGUE_ACTION_OK,

	/**
	 * Delete the heading.
	 */
	ACCOUNT_HEADING_DIALOGUE_ACTION_DELETE
};

/**
 * The analysis heading data held by the dialogue.
 */

struct account_heading_dialogue_data {
	/**
	 * The requested action from the dialogue.
	 */
	enum account_heading_dialogue_action	action;

	/**
	 * The target heading account.
	 */
	acct_t					account;

	/**
	 * The name for the heading.
	 */
	char					name[ACCOUNT_NAME_LEN];

	/**
	 * The ident for the heading.
	 */
	char					ident[ACCOUNT_IDENT_LEN];

	/**
	 * The budget limit for the heading.
	 */
	amt_t					budget;

	/**
	 * The type for the heading.
	 */
	enum account_type			type;
};

/**
 * Initialise the Heading Edit dialogue.
 */

void account_heading_dialogue_initialise(void);


/**
 * Open the Heading Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void account_heading_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*callback)(void *, struct account_heading_dialogue_data *), struct account_heading_dialogue_data *content);

#endif

