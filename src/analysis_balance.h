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
 * \file: analysis_balance.h
 *
 * Analysis Balance Report interface.
 */


#ifndef CASHBOOK_ANALYSIS_BALANCE
#define CASHBOOK_ANALYSIS_BALANCE

#include "account.h"
#include "analysis.h"

/* Balance Report dialogue. */

struct analysis_balance_report;


/**
 * Initialise the Balance analysis report module.
 */

void analysis_balance_initialise(void);


/**
 * Construct new balance report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \param *parent	Pointer to the parent analysis instance.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct analysis_balance_report *analysis_balance_create_instance(struct analysis_block *parent);


/**
 * Delete a balance report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_balance_delete_instance(struct analysis_balance_report *report);


/**
 * Open the Balance Report dialogue box.
 *
 * \param *parent	The paranet analysis instance owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_balance_open_window(struct analysis_block *parent, wimp_pointer *ptr, int template, osbool restore);





/**
 * Remove any references to an account if it appears within a
 * balance report template.
 *
 * \param *report	The transaction report to be processed.
 * \param account	The account to be removed.
 */

//void analysis_balance_remove_account(struct balance_rep *report, acct_t account);


/**
 * Remove any references to a report template.
 * 
 * \param *parent	The analysis instance being updated.
 * \param template	The template to be removed.
 */

//void analysis_balance_remove_template(struct analysis_block *parent, template_t template);

#endif
