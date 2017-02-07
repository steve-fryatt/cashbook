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
 * \file: budget.h
 *
 * Budgeting and budget dialogue implementation.
 */

#ifndef CASHBOOK_BUDGET
#define CASHBOOK_BUDGET

#include "date.h"
#include "file.h"
#include "filing.h"

/**
 * Initialise the Budget module.
 */

void budget_initialise(void);


/**
 * Construct new budget data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to which this instance belongs.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct budget_block *budget_create(struct file_block *file);


/**
 * Delete a budget data block.
 *
 * \param *windat	Pointer to the budget to delete.
 */

void budget_delete(struct budget_block *windat);


/**
 * Open the Budget dialogue box.
 *
 * \param *windat	The budget instance to the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 */

void budget_open_window(struct budget_block *windat, wimp_pointer *ptr);


/**
 * Return the budget start and finish dates for a file.
 *
 * \param *file			The file to return dates for.
 * \param *start		Pointer to location to store the start date,
 *				or NULL to not return it.
 * \param *finish		Pointer to location to store the finish date,
 *				or NULL to not return it.
 */

void budget_get_dates(struct file_block *file, date_t *start, date_t *finish);


/**
 * Return the standing order trial period for a file.
 *
 * \param *file			The file to return the period for.
 * \return			The trial period, in days (or 0 on error).
 */

int budget_get_sorder_trial(struct file_block *file);


/**
 * Return the postdated transaction limit option for a file (whether postdated
 * transactions should be limited to the standing order trial period in
 * reports and budgeting.
 *
 * \param *file			The file to return the postdated option for.
 * \return			TRUE if transactions should be limited to the
 *				standing order trial period; FALSE to include all.
 */

osbool budget_get_limit_postdated(struct file_block *file);


/**
 * Save the budget details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void budget_write_file(struct file_block *file, FILE *out);


/**
 * Read budget details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool budget_read_file(struct file_block *file, struct filing_block *in);

#endif
