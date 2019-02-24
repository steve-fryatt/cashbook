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
 * \file: find_search_dialogue.h
 *
 * High-level find search dialogue interface.
 */

#ifndef CASHBOOK_FIND_SEARCH_DIALOGUE
#define CASHBOOK_FIND_SEARCH_DIALOGUE

#include "account.h"
#include "currency.h"
#include "date.h"
#include "transact.h"
#include "oslib/types.h"
#include "find.h"

/**
 * The find search data held by the dialogue.
 */

struct find_search_dialogue_data {
	date_t			date;						/**< The date to match, or NULL_DATE for none.				*/
	acct_t			from;						/**< The From account to match, or NULL_ACCOUNT for none.		*/
	acct_t			to;						/**< The To account to match, or NULL_ACCOUNT for none.			*/
	enum transact_flags	reconciled;					/**< The From and To Accounts' reconciled status.			*/
	amt_t			amount;						/**< The Amount to match, or NULL_CURRENCY for "don't care".		*/
	char			ref[TRANSACT_REF_FIELD_LEN];			/**< The Reference to match; NULL or '\0' for "don't care".		*/
	char			desc[TRANSACT_DESCRIPT_FIELD_LEN];		/**< The Description to match; NULL or '\0' for "don't care".		*/

	enum find_logic		logic;						/**< The logic to use to combine the fields specified above.		*/
	osbool			case_sensitive;					/**< TRUE to match case of strings; FALSE to ignore.			*/
	osbool			whole_text;					/**< TRUE to match strings exactly; FALSE to allow substrings.		*/
	enum find_direction	direction;					/**< The direction to search in.					*/
};

/**
 * Initialise the find_search dialogue.
 */

void find_search_dialogue_initialise(void);

/**
 * Open the find_search dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current dialogue content, otherwise FALSE
 * \param *owner		The find dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void find_search_dialogue_open(wimp_pointer *ptr, osbool restore, void *owner, struct file_block *file, osbool (*callback)(void *, struct find_search_dialogue_data *),
		struct find_search_dialogue_data *data);

#endif

