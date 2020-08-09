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
 * \file: sorder_dialogue.h
 *
 * Standing Order Edit dialogue interface.
 */

#ifndef CASHBOOK_SORDER_DIALOGUE
#define CASHBOOK_SORDER_DIALOGUE

#include "sorder.h"

/**
 * The requested action from the dialogue.
 */

enum sorder_dialogue_action {
	/**
	 * No action defined.
	 */
	SORDER_DIALOGUE_ACTION_NONE,

	/**
	 * Create or update the standing order using the supplied details.
	 */
	SORDER_DIALOGUE_ACTION_OK,

	/**
	 * Delete the standing order.
	 */
	SORDER_DIALOGUE_ACTION_DELETE,

	/**
	 * Stop the standing order.
	 */
	SORDER_DIALOGUE_ACTION_STOP
};

/**
 * The standing order data held by the dialogue.
 */

struct sorder_dialogue_data {
	/**
	 * The requested action from the dialogue.
	 */
	enum sorder_dialogue_action	action;

	/**
	 * The standing order being edited.
	 */
	sorder_t			sorder;

	/**
	 * True if the order is currently active; otherwise FALSE.
	 */
	osbool				active;

	/**
	 * The starting date for the standing order.
	 */
	date_t				start_date;

	/**
	 * The number of orders to be added.
	 */
	int				number;

	/**
	 * The period between orders.
	 */
	int				period;

	/**
	 * The unit in which the period is measured.
	 */
	enum date_period		period_unit;

	/**
	 * The transaction flags for the order (including the order flags).
	 */
	enum transact_flags		flags;

	/**
	 * The account from which the order is taken.
	 */
	acct_t				from;

	/**
	 * The account to which the order is sent.
	 */
	acct_t				to;

	/**
	 * The standard amount for the order.
	 */
	amt_t				normal_amount;

	/**
	 * The amount for the first order.
	 */
	amt_t				first_amount;

	/**
	 * The amount for the last order.
	 */
	amt_t				last_amount;

	/**
	 * The reference for the order.
	 */
	char				reference[TRANSACT_REF_FIELD_LEN];

	/**
	 * The description for the order.
	 */
	char				description[TRANSACT_DESCRIPT_FIELD_LEN];
};

/**
 * Initialise the Standing Order Edit dialogue.
 */

void sorder_dialogue_initialise(void);


/**
 * Open the Standing Order Edit dialogue for a given standing order window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void sorder_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*callback)(void *, struct sorder_dialogue_data *), struct sorder_dialogue_data *content);

#endif

