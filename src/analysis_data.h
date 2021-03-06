/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: analysis_data.h
 *
 * Analysis scratch data implementation.
 */

#ifndef CASHBOOK_ANALYSIS_DATA
#define CASHBOOK_ANALYSIS_DATA

#include "oslib/types.h"

#include "account.h"
#include "currency.h"
#include "date.h"


/**
 * An analysis scratch data instance.
 */

struct analysis_data_block;

/**
 * Flags used by the analysis scratch space.
 */

enum analysis_data_flags {
	ANALYSIS_DATA_NONE	= 0x0000,
	ANALYSIS_DATA_FROM	= 0x0001,
	ANALYSIS_DATA_TO	= 0x0002,
	ANALYSIS_DATA_INCLUDE	= 0x0004
};


/**
 * Allocate a new analysis scratch data set.
 *
 * \param *file			The file to which the block will relate.
 * \return			Pointer to the new data set, or NULL.
 */

struct analysis_data_block *analysis_data_claim(struct file_block *file);


/**
 * Free an analysis scratch data set.
 *
 * \param *block		Pointer to the block to be freed.
 */

void analysis_data_free(struct analysis_data_block *block);


/**
 * Clear all the account report flags in an analysis scratch data set,
 * to allow them to be re-set for a new report.
 *
 * \param *block		The scratch data to be cleared.
 */

void analysis_data_clear_flags(struct analysis_data_block *block);


/**
 * Set the specified report flags for all accounts that match the list given.
 * The account NULL_ACCOUNT will set all the acounts that match the given type.
 *
 * \param *block		The scratch data instance to be updated.
 * \param type			The type(s) of account to match for NULL_ACCOUNT.
 * \param flags			The report flags to set for matching accounts.
 * \param *array		The account list array to use.
 * \param count			The number of accounts in the account list.
 */

void analysis_data_set_flags_from_account_list(struct analysis_data_block *block, unsigned type, enum analysis_data_flags flags, acct_t *array, size_t count);


/**
 * Test an account in a scratch data block to see whether its flags have
 * a given combination set.
 *
 * \param *block		The scratch data instance to process.
 * \param account		The account to test
 * \param flags			The flags to be matched.
 * \return			TRUE if the flags match; otherwise FALSE.
 */

osbool analysis_data_test_account(struct analysis_data_block *block, acct_t account, enum analysis_data_flags flags);


/**
 * Return the calculated total for an account from a scratch data block.
 *
 * \param *block		The scratch data instance to process.
 * \param account		The account for which to return the total.
 * \return			The calculated account total.
 */

amt_t analysis_data_get_total(struct analysis_data_block *block, acct_t account);


/**
 * Update the balance for an account in a scratch data block, using the
 * current total, and return the new balance.
 *
 * \param *block		The scratch data instance to process.
 * \param account		The account for which to update and return the balance.
 * \return			The calculated account balance.
 */

amt_t analysis_data_update_balance(struct analysis_data_block *block, acct_t account);

/**
 * Count the number of entries in a scratch data block with a given flag
 * combination set.
 *
 * \param *block		The scratch data instance to process.
 * \param flags			The flags to be matched.
 * \return			The number of matching entries.
 */

int analysis_data_count_matches(struct analysis_data_block *block, enum analysis_data_flags flags);


/**
 * Zero the report totals in a scratch data block.
 *
 * \param *block		The scratch data block to process.
 */

void analysis_data_zero_totals(struct analysis_data_block *block);


/**
 * Reset the remaining balances in a scratch data block.
 *
 * \param *block		The scratch data block to process.
 */

void analysis_data_initialise_balances(struct analysis_data_block *block);


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

int analysis_data_calculate_balances(struct analysis_data_block *block, date_t start_date, date_t end_date, osbool opening);


/**
 * Add a transaction's details to an analysis scratch space.
 *
 * \param *block		The scratch data instance to process.
 * \param transaction		The transaction to add.
 */

void analysis_data_add_transaction(struct analysis_data_block *block, tran_t transaction);

#endif


