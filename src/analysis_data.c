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
 * \file: analysis_data.c
 *
 * Analysis scratch data implementation.
 */

/* ANSI C header files */

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

/* OSLib header files */

//#include "oslib/hourglass.h"
#include "oslib/types.h"

/* SF-Lib header files. */

//#include "sflib/config.h"
//#include "sflib/debug.h"
//#include "sflib/errors.h"
//#include "sflib/event.h"
#include "sflib/heap.h"
//#include "sflib/icons.h"
//#include "sflib/ihelp.h"
//#include "sflib/menus.h"
//#include "sflib/msgs.h"
//#include "sflib/string.h"
//#include "sflib/templates.h"
//#include "sflib/windows.h"

/* Application header files */

#include "analysis_data.h"

#include "account.h"
//#include "account_menu.h"
//#include "analysis_balance.h"
//#include "analysis_cashflow.h"
//#include "analysis_dialogue.h"
//#include "analysis_lookup.h"
//#include "analysis_period.h"
//#include "analysis_template.h"
//#include "analysis_template_save.h"
//#include "analysis_transaction.h"
//#include "analysis_unreconciled.h"
//#include "budget.h"
//#include "caret.h"
#include "currency.h"
#include "date.h"
//#include "file.h"
#include "flexutils.h"
//#include "report.h"
#include "transact.h"



/**
 * Analysis scratch data, associated with an individual account during
 * report generation.
 */

struct analysis_data {
	amt_t				report_total;			/**< Running total for the account.						*/
	amt_t				report_balance;			/**< Balance for the account.							*/
	enum analysis_data_flags	report_flags;			/**< Flags associated with the account.						*/
};

/**
 * An analysis scratch data set.
 */

struct analysis_data_block {
	struct file_block	*file;		/**< The file to which the data applies.	*/
	size_t			count;		/**< The number of entries in the data array.	*/
	struct analysis_data	*data;		/**< Pointer to the data array.			*/
};


/**
 * Allocate a new analysis scratch data set.
 *
 * \param *file			The file to which the block will relate.
 * \return			Pointer to the new data set, or NULL.
 */

struct analysis_data_block *analysis_data_claim(struct file_block *file)
{
	struct analysis_data_block	*new;

	if (file == NULL)
		return NULL;

	new = heap_alloc(sizeof(struct analysis_data_block));
	if (new == NULL)
		return NULL;

	new->file = file;
	new->count = account_get_count(file);
	new->data = NULL;

	if (!flexutils_allocate((void **) &(new->data), sizeof(struct analysis_data), new->count)) {
		heap_free(new);
		return NULL;
	}

	analysis_data_clear_flags(new);

	return new;
}


/**
 * Free an analysis scratch data set.
 *
 * \param *block		Pointer to the block to be freed.
 */

void analysis_data_free(struct analysis_data_block *block)
{
	if (block == NULL)
		return;

	if (block->data != NULL)
		flexutils_free((void **) &(block->data));


	heap_free(block);
}


/**
 * Clear all the account report flags in an analysis scratch data set,
 * to allow them to be re-set for a new report.
 *
 * \param *block		The scratch data to be cleared.
 */

void analysis_data_clear_flags(struct analysis_data_block *block)
{
	int	i;

	if (block == NULL)
		return;

	for (i = 0; i < block->count; i++)
		block->data[i].report_flags = ANALYSIS_DATA_NONE;
}


/**
 * Set the specified report flags for all accounts that match the list given.
 * The account NULL_ACCOUNT will set all the acounts that match the given type.
 *
 * \param *block		The scratch data instance to be updated.
 * \param type			The type(s) of account to match for NULL_ACCOUNT.
 * \param flags			The report flags to set for matching accounts.
 * \param *array		The account list array to use, or NULL for wildcard.
 * \param count			The number of accounts in the account list.
 */

void analysis_data_set_flags_from_account_list(struct analysis_data_block *block, unsigned type, enum analysis_data_flags flags, acct_t *array, size_t count)
{
	int	i;
	size_t	account_count;
	acct_t	account;
	acct_t	wildcard = NULL_ACCOUNT;

	if (block == NULL || block->file == NULL || block->data == NULL)
		return;

	account_count = account_get_count(block->file);

	/* If the array is NULL, point to a single wildcard entry. */

	if (array == NULL) {
		array = &wildcard;
		count = 1;
	}

	for (i = 0; i < count; i++) {
		account = array[i];

		if (account == NULL_ACCOUNT) {
			/* 'Wildcard': set all the accounts which match the given account type. */

			for (account = 0; account < account_count; account++)
				if ((account_get_type(block->file, account) & type) != 0)
					block->data[account].report_flags |= flags;
		} else {
			/* Set a specific account. */

			if (account < account_count)
				block->data[account].report_flags |= flags;
		}
	}
}


/**
 * Test an account in a scratch data block to see whether its flags have
 * a given combination set.
 *
 * \param *block		The scratch data instance to process.
 * \param account		The account to test
 * \param flags			The flags to be matched.
 * \return			TRUE if the flags match; otherwise FALSE.
 */

osbool analysis_data_test_account(struct analysis_data_block *block, acct_t account, enum analysis_data_flags flags)
{
	if (block == NULL || block->data == NULL)
		return FALSE;

	if (account == NULL_ACCOUNT || account < 0 || account >= block->count)
		return FALSE;

	return ((block->data[account].report_flags & flags) == flags) ? TRUE : FALSE;
}


/**
 * Return the calculated total for an account from a scratch data block.
 *
 * \param *block		The scratch data instance to process.
 * \param account		The account for which to return the total.
 * \return			The calculated account total.
 */

amt_t analysis_data_get_total(struct analysis_data_block *block, acct_t account)
{
	if (block == NULL || block->data == NULL)
		return FALSE;

	if (account == NULL_ACCOUNT || account < 0 || account >= block->count)
		return FALSE;

	return block->data[account].report_total;
}


/**
 * Update the balance for an account in a scratch data block, using the
 * current total, and return the new balance.
 *
 * \param *block		The scratch data instance to process.
 * \param account		The account for which to update and return the balance.
 * \return			The calculated account balance.
 */

amt_t analysis_data_update_balance(struct analysis_data_block *block, acct_t account)
{
	if (block == NULL || block->data == NULL)
		return 0;

	if (account == NULL_ACCOUNT || account < 0 || account >= block->count)
		return 0;

	block->data[account].report_balance -= block->data[account].report_total;
	return block->data[account].report_balance;
}


/**
 * Count the number of entries in a scratch data block with a given flag
 * combination set.
 *
 * \param *block		The scratch data instance to process.
 * \param flags			The flags to be matched.
 * \return			The number of matching entries.
 */

int analysis_data_count_matches(struct analysis_data_block *block, enum analysis_data_flags flags)
{
	int i, count = 0;

	if (block == NULL || block->data == NULL)
		return 0;

	for (i = 0; i < block->count; i++) {
		if ((block->data[i].report_flags & flags) == flags)
			count++;
	}

	return count;
}


/**
 * Zero the report totals in a scratch data block.
 *
 * \param *block		The scratch data block to process.
 */

void analysis_data_zero_totals(struct analysis_data_block *block)
{
	acct_t account;

	if (block == NULL || block->data == NULL)
		return;

	for (account = 0; account < block->count; account++)
		block->data[account].report_total = 0;
}


/**
 * Reset the remaining balances in a scratch data block.
 *
 * \param *block		The scratch data block to process.
 */

void analysis_data_initialise_balances(struct analysis_data_block *block)
{
	acct_t			account;
	enum account_type	type;

	if (block == NULL || block->file == NULL || block->data == NULL)
		return;

	/* Check that the accounts in the file haven't changed. */

	if (block->count != account_get_count(block->file))
		return;

	/* Reset the values. */

	for (account = 0; account < block->count; account++){
		type = account_get_type(block->file, account);
	
		if (type & ACCOUNT_OUT)
			block->data[account].report_balance = account_get_budget_amount(block->file, account);
		else if (type & ACCOUNT_IN)
			block->data[account].report_balance = -account_get_budget_amount(block->file, account);
	}
}


/**
 * Calculate the account balances on a given date.
 *
 * \param *block		The scratch data instance to process.
 * \param start_date		The first date to include in the balances,
 *				or NULL_DATE.
 * \param end_date		The last date to include in the balances,
 *				or NULL_DATE.
 * \param opening		TRUE to include opening balances, FALSE to
 *				omit and start from zero.
 * \return			The number of transactions included in the
 *				returned totals.
 */

int analysis_data_calculate_balances(struct analysis_data_block *block, date_t start_date, date_t end_date, osbool opening)
{
	int		transaction_count, transactions_found = 0;
	date_t		date;
	acct_t		account;
	tran_t		transaction;

	if (block == NULL || block->file == NULL || block->data == NULL)
		return 0;

	/* Check that the accounts in the file haven't changed. */

	if (block->count != account_get_count(block->file))
		return 0;

	for (account = 0; account < block->count; account++)
		block->data[account].report_total = (opening == TRUE) ? account_get_opening_balance(block->file, account) : 0;

	/* Scan through the transactions, adding the values up for those occurring before the end of the current
	 * period and outputting them to the screen.
	 */

	transaction_count = transact_get_count(block->file);

	for (transaction = 0; transaction < transaction_count; transaction++) {
		date = transact_get_date(block->file, transaction);

		if ((start_date == NULL_DATE || date >= start_date) && (end_date == NULL_DATE || date <= end_date)) {
			analysis_data_add_transaction(block, transaction);

			transactions_found++;
		}
	}

	return transactions_found;
}


/**
 * Add a transaction's details to an analysis scratch space.
 *
 * \param *block		The scratch data instance to process.
 * \param transaction		The transaction to add.
 */

void analysis_data_add_transaction(struct analysis_data_block *block, tran_t transaction)
{
	acct_t	from, to;
	amt_t	amount;

	if (block == NULL || block->file == NULL || block->data == NULL)
		return;

	from = transact_get_from(block->file, transaction);
	to = transact_get_to(block->file, transaction);
	amount = transact_get_amount(block->file, transaction);

	if (from != NULL_ACCOUNT && from >= 0 && from < block->count)
		block->data[from].report_total -= amount;

	if (to != NULL_ACCOUNT && to >= 0 && to < block->count)
		block->data[to].report_total += amount;
}

