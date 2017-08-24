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
 * \file: analysis.c
 *
 * Analysis interface implementation.
 */

/* ANSI C header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "analysis.h"

#include "account.h"
#include "account_menu.h"
#include "analysis_balance.h"
#include "analysis_cashflow.h"
#include "analysis_data.h"
#include "analysis_dialogue.h"
#include "analysis_lookup.h"
#include "analysis_period.h"
#include "analysis_template.h"
#include "analysis_template_save.h"
#include "analysis_transaction.h"
#include "analysis_unreconciled.h"
#include "budget.h"
#include "caret.h"
#include "currency.h"
#include "date.h"
#include "file.h"
//#include "flexutils.h"
#include "report.h"
#include "transact.h"

/**
 * The number of analysis report type definitions.
 */

#define ANALYSIS_REPORT_TYPE_COUNT 5

/**
 * The analysis report details in a file.
 */

struct analysis_block
{
	/**
	 * The parent file.
	 */

	struct file_block			*file;

	/**
	 * The saved analysis templates associated with this instance.
	 */

	struct analysis_template_block		*templates;

	/**
	 * The report instances which make up this analysis instance.
	 */

	void					*reports[ANALYSIS_REPORT_TYPE_COUNT];
};

//static struct analysis_report		analysis_report_template;			/**< New report settings for passing to the Report module.			*/


/**
 * Details of all of the analysis report types.
 */

static struct analysis_report_details	*analysis_report_types[ANALYSIS_REPORT_TYPE_COUNT];


/* Static Function Prototypes. */

static void analysis_remove_account_from_report_template(struct analysis_report *template, void *data);



/**
 * Initialise the Analysis module and all its dialogue boxes.
 */

void analysis_initialise(void)
{
	analysis_report_types[REPORT_TYPE_NONE] = 
	analysis_report_types[REPORT_TYPE_TRANSACTION] = analysis_transaction_initialise();
	analysis_report_types[REPORT_TYPE_UNRECONCILED] = analysis_unreconciled_initialise();
	analysis_report_types[REPORT_TYPE_CASHFLOW] = analysis_cashflow_initialise();
	analysis_report_types[REPORT_TYPE_BALANCE] = analysis_balance_initialise();
	analysis_template_save_initialise();
	analysis_lookup_initialise();
}


/**
 * Return the report type definition for a report type, or NULL if one
 * isn't defined.
 *
 * \param type			The type of report to look up.
 * \return			Pointer to the report type definition, or NULL.
 */

struct analysis_report_details *analysis_get_report_details(enum analysis_report_type type)
{
	if (type < 0 || type >= ANALYSIS_REPORT_TYPE_COUNT)
		return NULL;

	return analysis_report_types[type];
}


/**
 * Create a new analysis report instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct analysis_block *analysis_create_instance(struct file_block *file)
{
	enum analysis_report_type	type;
	struct analysis_block		*new;
	struct analysis_report_details	*report_details;

	new = heap_alloc(sizeof(struct analysis_block));
	if (new == NULL)
		return NULL;

	new->file = file;

	new->templates = NULL;

	for (type = 0; type < ANALYSIS_REPORT_TYPE_COUNT; type++)
		new->reports[type] = NULL;

	/* Set up the saved template data. */

	new->templates = analysis_template_create_instance(new);
	if (new->templates == NULL) {
		analysis_delete_instance(new);
		return NULL;
	}

	/* Set up the individual reports. */

	for (type = 0; type < ANALYSIS_REPORT_TYPE_COUNT; type++) {
		report_details = analysis_get_report_details(type);

		if (report_details == NULL || report_details->create_instance == NULL)
			continue;

		new->reports[type] = report_details->create_instance(new);
		if (new->reports[type] == NULL) {
			analysis_delete_instance(new);
			return NULL;
		}
	}

	return new;
}


/**
 * Delete an analysis report instance, and all of its data.
 *
 * \param *instance		The instance to be deleted.
 */

void analysis_delete_instance(struct analysis_block *instance)
{
	enum analysis_report_type	type;
	struct analysis_report_details	*report_details;

	if (instance == NULL)
		return;

	/* Close the template save/rename dialogue. */

	analysis_template_save_force_close(instance);

	/* Free any saved report data. */

	if (instance->templates != NULL)
		analysis_template_delete_instance(instance->templates);

	/* Free the report instances. */

	for (type = 0; type < ANALYSIS_REPORT_TYPE_COUNT; type++) {
		if (instance->reports[type] == NULL)
			continue;

		report_details = analysis_get_report_details(type);

		if (report_details == NULL || report_details->delete_instance == NULL)
			continue;

		report_details->delete_instance(instance->reports[type]);
	}

	/* Free the instance data. */

	heap_free(instance);
}


/**
 * Return the file instance to which an analysis report instance belongs.
 *
 * \param *instance		The instance to look up.
 * \return			The parent file, or NULL.
 */

struct file_block *analysis_get_file(struct analysis_block *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->file;
}


/**
 * Return the report template instance associated with a given analysis instance.
 *
 * \param *instance		The instance to return the templates for.
 * \return			The template instance, or NULL.
 */

struct analysis_template_block *analysis_get_templates(struct analysis_block *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->templates;
}


/**
 * Open a new Analysis Report dialogue box.
 *
 * \param *parent		The analysis instance to own the dialogue.
 * \param *pointer		The current Wimp Pointer details.
 * \param type			The type of report to open.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_open_window(struct analysis_block *instance, wimp_pointer *pointer, enum analysis_report_type type, osbool restore)
{
	struct analysis_report_details *report_details;

	if (instance == NULL || pointer == NULL)
		return;

	report_details = analysis_get_report_details(type);
	if (report_details != NULL && report_details->open_window != NULL)
		report_details->open_window(instance->reports[type], pointer, NULL_TEMPLATE, restore);
}


/**
 * Open a report from a saved template.
 *
 * \param *instance		The analysis instance owning the template.
 * \param *pointer		The current Wimp Pointer details.
 * \param selection		The menu selection entry.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_open_template(struct analysis_block *instance, wimp_pointer *pointer, template_t template, osbool restore)
{
	enum analysis_report_type	type;
	struct analysis_report_details	*report_details;


	if (instance == NULL || instance->templates == NULL)
		return;

	type = analysis_template_type(instance->templates, template);
	if (type == REPORT_TYPE_NONE)
		return;

	report_details = analysis_get_report_details(type);
	if (report_details != NULL && report_details->open_window != NULL)
		report_details->open_window(instance->reports[type], pointer, template, restore);
}






/**
 * Remove an account from all of the report templates in a file (pending
 * deletion).
 *
 * \param *file			The file to process.
 * \param account		The account to remove.
 */

void analysis_remove_account_from_templates(struct file_block *file, acct_t account)
{
	enum analysis_report_type	type;
	struct analysis_report_details	*report_details;

	if (file == NULL || file->analysis == NULL)
		return;

	/* Handle the dialogue boxes. */

	for (type = 0; type < ANALYSIS_REPORT_TYPE_COUNT; type++) {
		report_details = analysis_get_report_details(type);

		if (report_details == NULL || report_details->remove_account == NULL)
			continue;
#ifdef DEBUG
		debug_printf("Removing account from report type %d", type);
#endif
		report_details->remove_account(file->analysis->reports[type], account);
	}

	/* Now process any saved templates. */

	analysis_template_remove_account(file->analysis->templates, account);

	/* Finally, work through any reports in the file. */

	report_process_all_templates(file, analysis_remove_account_from_report_template, &account);
}


/**
 * Callback function to accept analysis templates and account numbers from
 * the report system, when removing an account from all open report templates.
 *
 * \param *template		The template to be processed.
 * \param *data			Our supplied data (pointer to the number of
 *				the account to be removed).
 */

static void analysis_remove_account_from_report_template(struct analysis_report *template, void *data)
{
	acct_t	*account = data;

	if (template == NULL || account == NULL)
		return;

	analysis_template_remove_account_from_template(template, *account);
}











/**
 * Run a report, using supplied template data.
 *
 * \param *instance		The analysis instance owning the report.
 * \param type			The type of report to run.
 * \param *template		The template data
 */

void analysis_run_report(struct analysis_block *instance, enum analysis_report_type type, void *template)
{
	struct analysis_report_details	*report_details;
	struct analysis_data_block	*data = NULL;
	struct report			*report = NULL;


//	osbool			group, lock, tabular;
//	int			i, acc, items, unit, period, total;
//	char			line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
//	date_t			start_date, end_date, next_start, next_end;
//	acct_t			from, to;
//	amt_t			amount;
//	struct report		*report;
//	struct analysis_report	*template;
//	int			entries, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	report_details = analysis_get_report_details(type);
	if (report_details == NULL || instance == NULL || instance->file == NULL || template == NULL)
		return;

	data = analysis_data_claim(instance->file);
	if (data == NULL) {
		error_msgs_report_info("NoMemReport");
		return;
	}

	hourglass_on();

	/* Start to output the report details. */

//	analysis_copy_balance_template(&(analysis_report_template.data.balance), file->balance_rep);
//	if (analysis_balance_template == NULL_TEMPLATE)
//		*(analysis_report_template.name) = '\0';
//	else
//		strcpy(analysis_report_template.name, file->analysis->saved_reports[analysis_balance_template].name);
//	analysis_report_template.type = REPORT_TYPE_BALANCE;
//	analysis_report_template.file = file;

//	template = analysis_create_template(&analysis_report_template);
//	if (template == NULL) {
//		if (data != NULL)
//			analysis_data_free(data);
//		hourglass_off();
//		error_msgs_report_info("NoMemReport");
//		return;
//	}

//	msgs_lookup("BRWinT", line, sizeof(line));
//	report = report_open(file, line, template);
	report = report_open(instance->file, "Test Report", NULL);

	if (report == NULL) {
		if (data != NULL)
			analysis_data_free(data);
		hourglass_off();
///		error_msgs_report_info("???");
		return;
	}

	/* Sort the file data, ready for the report. */

	transact_sort_file_data(instance->file);

	/* Run the report. */

	report_details->run_report(instance, template, report, data);

	/* Close the report. */

	report_close(report);

	/* Free the account scratch space. */

	if (data != NULL)
		analysis_data_free(data);

	hourglass_off();
}


/**
 * Establish and return the range of dates to report over, based on the values
 * in a dialogue box and the data in the file concerned.
 *
 * \param *instance		The analysis instance to own the report.
 * \param *start_date		Return the date to start the report from.
 * \param *end_date		Return the date to end the report at.
 * \param date1			The start date entered in the dialogue, or NULL_DATE.
 * \param date2			The end date entered in the dialogue, or NULL_DATE.
 * \param budget		TRUE to report on the budget period; else FALSE.
 */

void analysis_find_date_range(struct analysis_block *instance, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget)
{
	tran_t			i;
	int			transactions;
	osbool			find_start, find_end;
	date_t			date;

	if (instance == NULL || instance->file == NULL || start_date == NULL || end_date == NULL)
		return;

	if (budget) {
		/* Get the start and end dates from the budget settings. */

		budget_get_dates(instance->file, start_date, end_date);
	} else {
		/* Get the start and end dates from the icon text. */

		*start_date = date1;
		*end_date = date2;
	}

	find_start = (*start_date == NULL_DATE);
	find_end = (*end_date == NULL_DATE);

	/* If either of the dates wasn't specified, we need to find the earliest and latest dates in the file. */

	if (find_start || find_end) {
		transactions = transact_get_count(instance->file);

		if (find_start)
			*start_date = (transactions > 0) ? transact_get_date(instance->file, 0) : NULL_DATE;

		if (find_end)
			*end_date = (transactions > 0) ? transact_get_date(instance->file, transactions - 1) : NULL_DATE;

		for (i = 0; i < transactions; i++) {
			date = transact_get_date(instance->file, i);

			if (find_start && date != NULL_DATE && date < *start_date)
				*start_date = date;

			if (find_end && date != NULL_DATE && date > *end_date)
				*end_date = date;
		}
	}

	if (*start_date == NULL_DATE)
		*start_date = DATE_MIN;

	if (*end_date == NULL_DATE)
		*end_date = DATE_MAX;
}


/**
 * Convert a textual comma-separated list of account idents into a numeric
 * account list array.  The special account ident '*' means 'all', and is
 * stored as the 'wildcard' value (in this context) NULL_ACCOUNT.
 *
 * \param *instance		The analysis instance owning the data.
 * \param type			The type(s) of account to process.
 * \param *list			The textual account ident list to process.
 * \param *array		Pointer to memory to take the numeric list.
 * \param length		The number of entries that the list can hold.
 * \return			The number of entries added to the list.
 */

size_t analysis_account_idents_to_list(struct analysis_block *instance, enum account_type type, char *list, acct_t *array, size_t length)
{
	char	*copy, *ident;
	acct_t	account;
	int	i = 0;

	if (instance == NULL || instance->file == NULL)
		return 0;

	copy = strdup(list);

	if (copy == NULL)
		return 0;

	ident = strtok(copy, ",");

	while (ident != NULL && i < length) {
		if (strcmp(ident, "*") == 0) {
			array[i++] = NULL_ACCOUNT;
		} else {
			account = account_find_by_ident(instance->file, ident, type);

			if (account != NULL_ACCOUNT)
				array[i++] = account;
		}

		ident = strtok(NULL, ",");
	}

	free(copy);

	return i;
}


/* Convert a numeric account list array into a textual list of comma-separated
 * account idents.
 *
 * \param *instance		The analysis instance owning the data.
 * \param *list			Pointer to the buffer to take the textual list.
 * \param length		The size of the buffer.
 * \param *array		The account list array to be converted.
 * \param count			The number of accounts in the list.
 */

void analysis_account_list_to_idents(struct analysis_block *instance, char *list, size_t length, acct_t *array, size_t count)
{
	char	buffer[ACCOUNT_IDENT_LEN];
	acct_t	account;
	int	i;

	if (instance == NULL || instance->file == NULL)
		return;

	if (list == NULL || length <= 0)
		return;

	*list = '\0';

	for (i = 0; i < count; i++) {
		account = array[i];

		if (account != NULL_ACCOUNT)
			strncpy(buffer, account_get_ident(instance->file, account), ACCOUNT_IDENT_LEN);
		else
			strncpy(buffer, "*", ACCOUNT_IDENT_LEN);

		buffer[ACCOUNT_IDENT_LEN - 1] = '\0';

		if (strlen(list) > 0 && strlen(list) + 1 < length)
			strcat(list, ",");

		if (strlen(list) + strlen(buffer) < length)
			strcat(list, buffer);
	}
}


/**
 * Save the analysis template details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void analysis_write_file(struct file_block *file, FILE *out)
{
	if (file == NULL || file->analysis == NULL || file->analysis->templates == NULL)
		return;

	analysis_template_write_file(file->analysis->templates, out);
}


/**
 * Read analysis template details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool analysis_read_file(struct file_block *file, struct filing_block *in)
{
	if (file == NULL || file->analysis == NULL || file->analysis->templates == NULL)
		return FALSE;

	return analysis_template_read_file(file->analysis->templates, in);
}

