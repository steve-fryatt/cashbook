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
 * \file: budget.c
 *
 * Budgeting and budget dialogue implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/heap.h"

/* Application header files */

#include "budget.h"

#include "budget_dialogue.h"
#include "date.h"
#include "file.h"
#include "sorder.h"



/**
 * Budget data structure
 */

struct budget_block {
	struct file_block	*file;						/**< The file owning the budget dialogue.				*/

	/* Budget date limits */

	date_t			start;						/**< The start date of the budget.					*/
	date_t			finish;						/**< The finish date of the budget.					*/

	/* Standing order trail limits. */

	int			sorder_trial;					/**< The number of days ahead to trial standing orders.			*/
	osbool			limit_postdate;					/**< TRUE to limit post-dated transactions to the SO trial period.	*/
};

/* Static Function Prototypes */

static osbool budget_process_window(void *owner, struct budget_dialogue_data *content);

/**
 * Initialise the Budget module.
 */

void budget_initialise(void)
{
	budget_dialogue_initialise();
}


/**
 * Construct new budget data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to which this instance belongs.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct budget_block *budget_create(struct file_block *file)
{
	struct budget_block	*new;

	new = heap_alloc(sizeof(struct budget_block));
	if (new == NULL)
		return NULL;

	new->file = file;

	new->start = NULL_DATE;
	new->finish = NULL_DATE;

	new->sorder_trial = 0;
	new->limit_postdate = FALSE;

	return new;
}


/**
 * Delete a budget data block.
 *
 * \param *windat	Pointer to the budget to delete.
 */

void budget_delete(struct budget_block *windat)
{
	if (windat != NULL)
		heap_free(windat);
}


/**
 * Open the Budget dialogue box.
 *
 * \param *windat	The budget instance to the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 */

void budget_open_window(struct budget_block *windat, wimp_pointer *ptr)
{
	struct budget_dialogue_data *content = NULL;

	if (windat == NULL || ptr == NULL)
		return;

	content = heap_alloc(sizeof(struct budget_dialogue_data));
	if (content == NULL)
		return;

	content->start = windat->start;
	content->finish = windat->finish;
	content->sorder_trial = windat->sorder_trial;
	content->limit_postdate = windat->limit_postdate;

	budget_dialogue_open(ptr, windat, windat->file, budget_process_window, content);
}


/**
 * Process the contents of the Budget window, storing the details in the
 * owning file.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool budget_process_window(void *owner, struct budget_dialogue_data *content)
{
	struct budget_block *windat = owner;

	if (windat == NULL || content == NULL)
		return TRUE;

	windat->start = content->start;
	windat->finish = content->finish;
	windat->sorder_trial = content->sorder_trial;
	windat->limit_postdate = content->limit_postdate;

	/* Tidy up and redraw the windows */

	sorder_trial(windat->file);
	account_recalculate_all(windat->file);
	file_set_data_integrity(windat->file, TRUE);
	file_redraw_windows(windat->file);

	return TRUE;
}


/**
 * Return the budget start and finish dates for a file.
 *
 * \param *file			The file to return dates for.
 * \param *start		Pointer to location to store the start date,
 *				or NULL to not return it.
 * \param *finish		Pointer to location to store the finish date,
 *				or NULL to not return it.
 */

void budget_get_dates(struct file_block *file, date_t *start, date_t *finish)
{
	if (start != NULL)
		*start = (file != NULL && file->budget != NULL) ? file->budget->start : NULL_DATE;

	if (finish != NULL)
		*finish = (file != NULL && file->budget != NULL) ? file->budget->finish : NULL_DATE;
}


/**
 * Return the standing order trial period for a file.
 *
 * \param *file			The file to return the period for.
 * \return			The trial period, in days (or 0 on error).
 */

int budget_get_sorder_trial(struct file_block *file)
{
	if (file == NULL || file->budget == NULL)
		return 0;

	return file->budget->sorder_trial;
}


/**
 * Return the postdated transaction limit option for a file (whether postdated
 * transactions should be limited to the standing order trial period in
 * reports and budgeting.
 *
 * \param *file			The file to return the postdated option for.
 * \return			TRUE if transactions should be limited to the
 *				standing order trial period; FALSE to include all.
 */

osbool budget_get_limit_postdated(struct file_block *file)
{
	if (file == NULL || file->budget == NULL)
		return FALSE;

	return file->budget->limit_postdate;
}


/**
 * Save the budget details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void budget_write_file(struct file_block *file, FILE *out)
{
	if (file == NULL || file->budget == NULL)
		return;

	/* Output the budget data */

	fprintf(out, "\n[Budget]\n");

	fprintf(out, "Start: %x\n", file->budget->start);
	fprintf(out, "Finish: %x\n", file->budget->finish);
	fprintf(out, "SOTrial: %x\n", file->budget->sorder_trial);
	fprintf(out, "RestrictPost: %s\n", config_return_opt_string(file->budget->limit_postdate));
}


/**
 * Read budget details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool budget_read_file(struct file_block *file, struct filing_block *in)
{
	if (file == NULL || file->budget == NULL)
		return FALSE;

	do {
		if (filing_test_token(in, "Start"))
			file->budget->start = date_get_date_field(in);
		else if (filing_test_token(in, "Finish"))
			file->budget->finish = date_get_date_field(in);
		else if (filing_test_token(in, "SOTrial"))
			file->budget->sorder_trial = filing_get_int_field(in);
		else if (filing_test_token(in, "RestrictPost"))
			file->budget->limit_postdate = filing_get_opt_value(in);
		else
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
	} while (filing_get_next_token(in));

	return TRUE;
}

