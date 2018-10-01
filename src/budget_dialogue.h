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
 * \file: budget_dialogue.h
 *
 * High-level budget dialogue interface.
 */

#ifndef CASHBOOK_BUDGET_DIALOGUE
#define CASHBOOK_BUDGET_DIALOGUE

#include "date.h"

/**
 * The Budget data held by the dialogue.
 */

struct budget_dialogue_data {
	/* Budget date limits */

	date_t			start;					/**< The start date of the budget.					*/
	date_t			finish;					/**< The finish date of the budget.					*/

	/* Standing order trail limits. */

	int			sorder_trial;				/**< The number of days ahead to trial standing orders.			*/
	osbool			limit_postdate;				/**< TRUE to limit post-dated transactions to the SO trial period.	*/
};

/**
 * Initialise the budget dialogue.
 */

void budget_dialogue_initialise(void);

/**
 * Open the budget dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The budget dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void budget_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(void *, struct budget_dialogue_data *),
		struct budget_dialogue_data *data);

#endif

