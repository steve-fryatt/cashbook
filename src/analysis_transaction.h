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
 * \file: analysis_transaction.h
 *
 * Analysis Transaction Report interface.
 */


#ifndef CASHBOOK_ANALYSIS_TRANSACTION
#define CASHBOOK_ANALYSIS_TRANSACTION

/**
 * Transaction Report dialogue.
 */

struct analysis_transaction_report;

/**
 * Initialise the Transaction analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_transaction_initialise(void);


/**
 * Construct new transaction report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \param *parent	Pointer to the parent analysis instance.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct analysis_transaction_report *analysis_transaction_create_instance(struct analysis_block *parent);


/**
 * Delete a transaction report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_transaction_delete_instance(struct analysis_transaction_report *report);
















/**
 * Remove any references to an account if it appears within a
 * transaction report template.
 *
 * \param *report	The transaction report to be processed.
 * \param account	The account to be removed.
 */

//void analysis_transaction_remove_account(struct trans_rep *report, acct_t account);


/**
 * Remove any references to a report template.
 * 
 * \param template	The template to be removed.
 */

//void analysis_transaction_remove_template(template_t template);

#endif
