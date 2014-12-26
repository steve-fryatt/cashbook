/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
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


/**
 * Initialise the Budget module.
 */

void budget_initialise(void);


/**
 * Construct new budget data for a new file. The block will be allocated with
 * heap_alloc(), and should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct budget *budget_create(void);


/**
 * Open the Budget dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 */

void budget_open_window(file_data *file, wimp_pointer *ptr);


/**
 * Force the closure of the Budget window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void budget_force_window_closed(file_data *file);


/**
 * Return the budget start and finish dates for a file.
 *
 * \param *file			The file to return dates for.
 * \param *start		Pointer to location to store the start date,
 *				or NULL to not return it.
 * \param *finish		Pointer to location to store the finish date,
 *				or NULL to not return it.
 */

void budget_get_dates(file_data *file, date_t *start, date_t *finish);


/**
 * Return the standing order trial period for a file.
 *
 * \param *file			The file to return the period for.
 * \return			The trial period, in days (or 0 on error).
 */

int budget_get_sorder_trial(file_data *file);


/**
 * Return the postdated transaction limit option for a file (whether postdated
 * transactions should be limited to the standing order trial period in
 * reports and budgeting.
 *
 * \param *file			The file to return the postdated option for.
 * \return			TRUE if transactions should be limited to the
 *				standing order trial period; FALSE to include all.
 */

osbool budget_get_limit_postdated(file_data *file);


/**
 * Save the budget details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void budget_write_file(file_data *file, FILE *out);


/**
 * Read budget details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

enum config_read_status budget_read_file(file_data *file, FILE *in, char *section, char *token, char *value, osbool *unknown_data);

#endif

