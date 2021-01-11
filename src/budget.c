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
 * \file: budget.c
 *
 * Budgeting and budget dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "budget.h"

#include "account.h"
#include "caret.h"
#include "date.h"
#include "file.h"
#include "sorder.h"


#define BUDGET_ICON_OK 0
#define BUDGET_ICON_CANCEL 1

#define BUDGET_ICON_START 5
#define BUDGET_ICON_FINISH 7
#define BUDGET_ICON_TRIAL 11
#define BUDGET_ICON_RESTRICT 13

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

static struct budget_block	*budget_window_owner = NULL;			/**< The file currently owning the Budget window.			*/
static wimp_w			budget_window = NULL;				/**< The Budget window handle.						*/


static void		budget_click_handler(wimp_pointer *pointer);
static osbool		budget_keypress_handler(wimp_key *key);
static void		budget_refresh_window(void);
static void		budget_fill_window(struct budget_block *windat);
static osbool		budget_process_window(void);


/**
 * Initialise the Budget module.
 */

void budget_initialise(void)
{
	budget_window = templates_create_window("Budget");
	ihelp_add_window(budget_window, "Budget", NULL);
	event_add_window_mouse_event(budget_window, budget_click_handler);
	event_add_window_key_event(budget_window, budget_keypress_handler);
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
	if (budget_window_owner == windat && windows_get_open(budget_window))
		close_dialogue_with_caret(budget_window);

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
	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 */

	if (windows_get_open(budget_window))
		wimp_close_window(budget_window);

	/* Set the window contents up. */

	budget_fill_window(windat);

	/* Set the pointers up so we can find this lot again and open the window. */

	budget_window_owner = windat;

	windows_open_centred_at_pointer(budget_window, ptr);
	place_dialogue_caret(budget_window, BUDGET_ICON_START);
}


/**
 * Process mouse clicks in the Budget dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void budget_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case BUDGET_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(budget_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			budget_refresh_window();
		break;

	case BUDGET_ICON_OK:
		if (budget_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(budget_window);
		break;
	}
}


/**
 * Process keypresses in the Budget window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool budget_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (budget_process_window())
			close_dialogue_with_caret(budget_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(budget_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Budget window.
 */

static void budget_refresh_window(void)
{
	budget_fill_window(budget_window_owner);
	icons_redraw_group(budget_window, 3, BUDGET_ICON_START, BUDGET_ICON_FINISH, BUDGET_ICON_TRIAL);
	icons_replace_caret_in_window(budget_window);
}



/**
 * Fill the Budget window with values from the file.
 *
 * \param: *windat		The data to use to fill the window.
 */

static void budget_fill_window(struct budget_block *windat)
{
	if (windat == NULL)
		return;

	date_convert_to_string(windat->start, icons_get_indirected_text_addr(budget_window, BUDGET_ICON_START),
			icons_get_indirected_text_length(budget_window, BUDGET_ICON_START));
	date_convert_to_string(windat->finish, icons_get_indirected_text_addr(budget_window, BUDGET_ICON_FINISH),
			icons_get_indirected_text_length(budget_window, BUDGET_ICON_FINISH));

	icons_printf(budget_window, BUDGET_ICON_TRIAL, "%d", windat->sorder_trial);

	icons_set_selected(budget_window, BUDGET_ICON_RESTRICT, windat->limit_postdate);
}


/**
 * Process the contents of the Budget window, storing the details in the
 * owning file.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool budget_process_window(void)
{
	budget_window_owner->start =
			date_convert_from_string(icons_get_indirected_text_addr(budget_window, BUDGET_ICON_START), NULL_DATE, 0);
	budget_window_owner->finish =
			date_convert_from_string(icons_get_indirected_text_addr(budget_window, BUDGET_ICON_FINISH), NULL_DATE, 0);

	budget_window_owner->sorder_trial = atoi(icons_get_indirected_text_addr(budget_window, BUDGET_ICON_TRIAL));

	budget_window_owner->limit_postdate = icons_get_selected(budget_window, BUDGET_ICON_RESTRICT);

	/* Tidy up and redraw the windows */

	sorder_trial(budget_window_owner->file);
	account_recalculate_all(budget_window_owner->file);
	file_set_data_integrity(budget_window_owner->file, TRUE);
	file_redraw_windows(budget_window_owner->file);

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
