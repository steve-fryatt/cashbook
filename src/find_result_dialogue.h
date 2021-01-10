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
 * \file: find_result_dialogue.h
 *
 * High-level find results dialogue interface.
 */

#ifndef CASHBOOK_FIND_RESULT_DIALOGUE
#define CASHBOOK_FIND_RESULT_DIALOGUE

#include "find.h"

#include "date.h"
#include "transact.h"

#include "oslib/types.h"

/**
 * Actions which the user can request.
 */

enum find_result_dialogue_action {
	FIND_RESULT_DIALOGUE_NONE,
	FIND_RESULT_DIALOGUE_PREVIOUS,
	FIND_RESULT_DIALOGUE_NEXT,
	FIND_RESULT_DIALOGUE_NEW
};

/**
 * The find result data held by the dialogue.
 */

struct find_result_dialogue_data {
	date_t					date;					/**< The date to match, or NULL_DATE for none.				*/
	acct_t					from;					/**< The From account to match, or NULL_ACCOUNT for none.		*/
	acct_t					to;					/**< The To account to match, or NULL_ACCOUNT for none.			*/
	enum transact_flags			reconciled;				/**< The From and To Accounts' reconciled status.			*/
	amt_t					amount;					/**< The Amount to match, or NULL_CURRENCY for "don't care".		*/
	char					ref[TRANSACT_REF_FIELD_LEN];		/**< The Reference to match; NULL or '\0' for "don't care".		*/
	char					desc[TRANSACT_DESCRIPT_FIELD_LEN];	/**< The Description to match; NULL or '\0' for "don't care".		*/

	enum find_logic				logic;					/**< The logic to use to combine the fields specified above.		*/
	osbool					case_sensitive;				/**< TRUE to match case of strings; FALSE to ignore.			*/
	osbool					whole_text;				/**< TRUE to match strings exactly; FALSE to allow substrings.		*/
	enum find_direction			direction;				/**< The direction to search in.					*/

	enum transact_field			result;
	tran_t					transaction;

	enum find_result_dialogue_action	action;
};

/**
 * Initialise the find result dialogue.
 */

void find_result_dialogue_initialise(void);

/**
 * Open the find result dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The find dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void find_result_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(wimp_pointer *, void *, struct find_result_dialogue_data *),
		struct find_result_dialogue_data *data);

#endif

