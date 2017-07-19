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
 * \file: analysis_unreconciled.h
 *
 * Analysis Unreconciled Report interface.
 */


#ifndef CASHBOOK_ANALYSIS_UNRECONCILED
#define CASHBOOK_ANALYSIS_UNRECONCILED

/* Unreconciled Report dialogue. */

struct analysis_unreconciled_report;


/**
 * Initialise the Unreconciled Transactions analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_unreconciled_initialise(void);


/**
 * Construct new unreconciled report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \param *parent	Pointer to the parent analysis instance.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct analysis_unreconciled_report *analysis_unreconciled_create_instance(struct analysis_block *parent);


/**
 * Delete an unreconciled report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_unreconciled_delete_instance(struct analysis_unreconciled_report *report);






/**
 * Remove any references to an account if it appears within an
 * unreconciled transaction report template.
 *
 * \param *report	The transaction report to be processed.
 * \param account	The account to be removed.
 */

//void analysis_unreconciled_remove_account(struct unrec_rep *report, acct_t account);


/**
 * Remove any references to a report template.
 * 
 * \param template	The template to be removed.
 */

//void analysis_unreconciled_remove_template(template_t template);

#endif
