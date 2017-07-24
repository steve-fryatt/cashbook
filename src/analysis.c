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








enum analysis_flags {
	ANALYSIS_REPORT_NONE = 0,
	ANALYSIS_REPORT_FROM = 0x0001,
	ANALYSIS_REPORT_TO = 0x0002,
	ANALYSIS_REPORT_INCLUDE = 0x0004
};





/**
 * Analysis Scratch Data
 *
 * Data associated with an individual account during report generation.
 */

struct analysis_data
{
	int				report_total;				/**< Running total for the account.						*/
	int				report_balance;				/**< Balance for the account.							*/
	enum analysis_flags		report_flags;				/**< Flags associated with the account.						*/
};

/**
 * The analysis report details in a file.
 */

struct analysis_block
{
	struct file_block		*file;					/**< The parent file.								*/

	struct analysis_template_block	*templates;				/**< The saved analysis templates.						*/

	/* Analysis Report Content. */

	struct analysis_transaction_report	*trans_rep;			/**< Data relating to the transaction report dialogue.				*/
	struct analysis_unreconciled_report	*unrec_rep;			/**< Data relating to the unreconciled report dialogue.				*/
	struct analysis_cashflow_report		*cashflow_rep;			/**< Data relating to the cashflow report dialogue.				*/
	struct analysis_balance_report		*balance_rep;			/**< Data relating to the balance report dialogue.				*/
};

/* Report windows. */





static struct analysis_report	analysis_report_template;			/**< New report settings for passing to the Report module.			*/

static acct_t			analysis_wildcard_account_list = NULL_ACCOUNT;	/**< Pass a pointer to this to set all accounts.				*/

/**
 * The number of analysis report type definitions.
 */

#define ANALYSIS_REPORT_TYPE_COUNT 5

/**
 * Details of all of the analysis report types.
 */

static struct analysis_report_details	*analysis_report_types[ANALYSIS_REPORT_TYPE_COUNT];


static void		analysis_find_date_range(struct file_block *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget);

static void		analysis_remove_account_from_report_template(struct analysis_report *template, void *data);
static void		analysis_remove_account_from_template(struct analysis_report *template, acct_t account);
static int		analysis_remove_account_from_list(acct_t account, acct_t *array, int *count);
static void		analysis_clear_account_report_flags(struct file_block *file, struct analysis_data *data);
static void		analysis_set_account_report_flags_from_list(struct file_block *file, struct analysis_data *data, unsigned type, unsigned flags, acct_t *array, int count);

static void		analysis_delete_template(struct file_block *file, int template);
static struct analysis_report	*analysis_create_template(struct analysis_report *base);




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
	struct analysis_block	*new;

	new = heap_alloc(sizeof(struct analysis_block));
	if (new == NULL)
		return NULL;

	new->file = file;

	new->templates = NULL;

	new->balance_rep = NULL;
	new->cashflow_rep = NULL;
	new->trans_rep = NULL;
	new->unrec_rep = NULL;

	/* Set up the saved template data. */

	new->templates = analysis_template_create_instance(new);
	if (new->templates == NULL) {
		analysis_delete_instance(new);
		return NULL;
	}

	/* Set up the balance report data. */

	new->balance_rep = analysis_balance_create_instance(new);
	if (new->balance_rep == NULL) {
		analysis_delete_instance(new);
		return NULL;
	}

	/* Set up the cashflow report data. */

	new->cashflow_rep = analysis_cashflow_create_instance(new);
	if (new->cashflow_rep == NULL) {
		analysis_delete_instance(new);
		return NULL;
	}

	/* Set up the transaction report data. */

	new->trans_rep = analysis_transaction_create_instance(new);
	if (new->trans_rep == NULL) {
		analysis_delete_instance(new);
		return NULL;
	}

	/* Set up the unreconciled report data. */

	new->unrec_rep = analysis_unreconciled_create_instance(new);
	if (new->unrec_rep == NULL) {
		analysis_delete_instance(new);
		return NULL;
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
	if (instance == NULL)
		return;

	/* Close the template save/rename dialogue. */

	analysis_template_save_force_close(instance->file);

	/* Free any saved report data. */

	if (instance->templates != NULL)
		analysis_template_delete_instance(instance->templates);

	/* Free the report instances. */

	if (instance->balance_rep != NULL)
		analysis_balance_delete_instance(instance->balance_rep);
	if (instance->cashflow_rep != NULL)
		analysis_cashflow_delete_instance(instance->cashflow_rep);
	if (instance->trans_rep != NULL)
		analysis_transaction_delete_instance(instance->trans_rep);
	if (instance->unrec_rep != NULL)
		analysis_unreconciled_delete_instance(instance->unrec_rep);

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
 * Return the report template instance associated with a given file.
 *
 * \param *file			The file to return the instance for.
 * \return			The template instance, or NULL.
 */

struct analysis_template_block *analysis_get_templates(struct file_block *file)
{
	if (file == NULL || file->analysis == NULL)
		return NULL;

	return file->analysis->templates;
}




/**
 * Establish and return the range of dates to report over, based on the values
 * in a dialogue box and the data in the file concerned.
 *
 * \param *file			The file to run the report on.
 * \param *start_date		Return the date to start the report from.
 * \param *end_date		Return the date to end the report at.
 * \param date1			The start date entered in the dialogue, or NULL_DATE.
 * \param date2			The end date entered in the dialogue, or NULL_DATE.
 * \param budget		TRUE to report on the budget period; else FALSE.
 */

static void analysis_find_date_range(struct file_block *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget)
{
//	int		i, transactions;
//	osbool		find_start, find_end;
//	date_t		date;

//	if (budget) {
//		/* Get the start and end dates from the budget settings. */

//		budget_get_dates(file, start_date, end_date);
//	} else {
//		/* Get the start and end dates from the icon text. */

//		*start_date = date1;
//		*end_date = date2;
//	}

//	find_start = (*start_date == NULL_DATE);
//	find_end = (*end_date == NULL_DATE);

	/* If either of the dates wasn't specified, we need to find the earliest and latest dates in the file. */

//	if (find_start || find_end) {
//		transactions = transact_get_count(file);

//		if (find_start)
//			*start_date = (transactions > 0) ? transact_get_date(file, 0) : NULL_DATE;

//		if (find_end)
//			*end_date = (transactions > 0) ? transact_get_date(file, transactions - 1) : NULL_DATE;

//		for (i = 0; i < transactions; i++) {
//			date = transact_get_date(file, i);

//			if (find_start && date != NULL_DATE && date < *start_date)
//				*start_date = date;

//			if (find_end && date != NULL_DATE && date > *end_date)
//				*end_date = date;
//		}
//	}

//	if (*start_date == NULL_DATE)
//		*start_date = DATE_MIN;

//	if (*end_date == NULL_DATE)
//		*end_date = DATE_MAX;
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
	if (file == NULL || file->analysis == NULL)
		return;

	/* Handle the dialogue boxes. */

//	analysis_transaction_remove_account(file->analysis->trans_rep, account);
//	analysis_unreconciled_remove_account(file->analysis->unrec_rep, account);
//	analysis_cashflow_remove_account(file->analysis->cashflow_rep, account);
//	analysis_balance_remove_account(file->analysis->balance_rep, account);

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
 * Clear all the account report flags in a file, to allow them to be re-set
 * for a new report.
 *
 * \param *file			The file to which the data belongs.
 * \param *data			The data to be cleared.
 */

static void analysis_clear_account_report_flags(struct file_block *file, struct analysis_data *data)
{
//	int	i;

//	if (file == NULL || data == NULL)
//		return;

//	for (i = 0; i < account_get_count(file); i++)
//		data[i].report_flags = ANALYSIS_REPORT_NONE;
}


/**
 * Set the specified report flags for all accounts that match the list given.
 * The account NULL_ACCOUNT will set all the acounts that match the given type.
 *
 * \param *file			The file to process.
 * \param *data			The data to be set.
 * \param type			The type(s) of account to match for NULL_ACCOUNT.
 * \param flags			The report flags to set for matching accounts.
 * \param *array		The account list array to use.
 * \param count			The number of accounts in the account list.
 */

static void analysis_set_account_report_flags_from_list(struct file_block *file, struct analysis_data *data, unsigned type, unsigned flags, acct_t *array, int count)
{
//	int	account, i;

//	for (i = 0; i < count; i++) {
//		account = array[i];

//		if (account == NULL_ACCOUNT) {
//			/* 'Wildcard': set all the accounts which match the
//			 * given account type.
//			 */

//			for (account = 0; account < account_get_count(file); account++)
//				if ((account_get_type(file, account) & type) != 0)
//					data[account].report_flags |= flags;
//		} else {
//			/* Set a specific account. */

//			if (account < account_get_count(file))
//				data[account].report_flags |= flags;
//		}
//	}
}


/**
 * Convert a textual comma-separated list of account idents into a numeric
 * account list array.  The special account ident '*' means 'all', and is
 * stored as the 'wildcard' value (in this context) NULL_ACCOUNT.
 *
 * \param *file			The file to process.
 * \param type			The type(s) of account to process.
 * \param *list			The textual account ident list to process.
 * \param *array		Pointer to memory to take the numeric list,
 *				with space for ANALYSIS_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

static int analysis_account_idents_to_list(struct file_block *file, unsigned type, char *list, acct_t *array)
{
//	char	*copy, *ident;
	int	account, i = 0;

//	copy = strdup(list);

//	if (copy == NULL)
//		return 0;

//	ident = strtok(copy, ",");

//	while (ident != NULL && i < ANALYSIS_ACC_LIST_LEN) {
//		if (strcmp(ident, "*") == 0) {
//			array[i++] = NULL_ACCOUNT;
//		} else {
//			account = account_find_by_ident(file, ident, type);

//			if (account != NULL_ACCOUNT)
//				array[i++] = account;
//		}

//		ident = strtok(NULL, ",");
//	}

//	free(copy);

	return i;
}


/* Convert a numeric account list array into a textual list of comma-separated
 * account idents.
 *
 * \param *file			The file to process.
 * \param *list			Pointer to the buffer to take the textual
 *				list, which must be ANALYSIS_ACC_SPEC_LEN long.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_account_list_to_idents(struct analysis_block *instance, char *list, acct_t *array, int len)
{
//	char	buffer[ACCOUNT_IDENT_LEN];
//	int	account, i;

//	if (instance == NULL || instance->file == NULL)
//		return;

//	*list = '\0';

//	for (i = 0; i < len; i++) {
//		account = array[i];

//		if (account != NULL_ACCOUNT)
//			strncpy(buffer, account_get_ident(instance->file, account), ACCOUNT_IDENT_LEN);
//		else
//			strncpy(buffer, "*", ACCOUNT_IDENT_LEN);

//		buffer[ACCOUNT_IDENT_LEN - 1] = '\0';

//		if (strlen(list) > 0 && strlen(list) + 1 < ANALYSIS_ACC_SPEC_LEN)
//			strcat(list, ",");

//		if (strlen(list) + strlen(buffer) < ANALYSIS_ACC_SPEC_LEN)
//			strcat(list, buffer);
//	}
}


/**
 * Open a report from a saved template.
 *
 * \param *file			The file owning the template.
 * \param *ptr			The Wimp pointer details.
 * \param selection		The menu selection entry.
 */

void analysis_open_template(struct file_block *file, wimp_pointer *ptr, template_t template)
{
	if (file == NULL || file->analysis == NULL)
		return;

	switch (analysis_template_type(file->analysis->templates, template)) {
	case REPORT_TYPE_TRANSACTION:
//		analysis_transaction_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_UNRECONCILED:
//		analysis_unreconciled_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_CASHFLOW:
//		analysis_cashflow_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_BALANCE:
//		analysis_balance_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_NONE:
		break;
	}
}


/**
 * Return a volatile pointer to a template data block from within a template,
 * if that template is of a given type. This is a pointer into a flex heap
 * block, so it will only be valid until an operation occurs to shift the blocks.
 * If the template isn't the type specified, NULL is returned.
 * 
 * \param *template		Pointer to the template to use.
 * \param type			The type of template required.
 * \return			Volatile pointer to the template, or NULL.
 */

union analysis_report_block *analysis_get_template_contents(struct analysis_report *template, enum analysis_report_type type)
{
//	if (template == NULL || template->type != type)
		return NULL;

//	return &(template->data);
}


/**
 * Return the file which owns a template.
 * 
 * \param *template		Pointer to the template for which to return
 *				the owning file.
 * \return			The owning file block, or NULL.
 */

struct file_block *analysis_get_template_file(struct analysis_report *template)
{
//	if (template == NULL)
		return NULL;

//	return template->file;
}



/**
 * Find a save template ID based on its name.
 * 
 * \FIXME -- This should probably be passed direct on to the templates module when possible.
 *
 * \param *file			The file to search in.
 * \param *name			The name to search for.
 * \return			The matching template ID, or NULL_TEMPLATE.
 */

template_t analysis_get_template_from_name(struct file_block *file, char *name)
{
//	if (file == NULL || file->analysis == NULL)
		return NULL_TEMPLATE;

//	return analysis_template_get_from_name(file->analysis->templates, name);
}


/**
 * Store a report's template into a file.
 * 
 * \FIXME -- This should probably be passed direct on to the templates module when possible.
 *
 * \param *file			The file to work on.
 * \param *report		The report to take the template from.
 * \param template		The template pointer to save to, or
 *				NULL_TEMPLATE to add a new entry.
 * \param *name			Pointer to a name to give the template, or
 *				NULL to leave it as-is.
 */

void analysis_store_template(struct file_block *file, struct analysis_report *report, template_t template, char *name)
{
//	if (file == NULL || file->analysis == NULL || report == NULL)
//		return;

//	analysis_template_store(file->analysis->templates, report, template, name);
}


/**
 * Rename a template.
 *
 * \param *filer		The file containing the template.
 * \param template		The template to be renamed.
 * \param *name			Pointer to the new name.
 */

void analysis_rename_template(struct file_block *file, template_t template, char *name)
{
//	if (file == NULL || file->analysis == NULL || name == NULL)
//		return;

//	analysis_template_rename(file->analysis->templates, template, name);
}


/**
 * Delete a saved report from the file, and adjust any other template pointers
 * which are currently in use.
 *
 * \param *file			The file to delete the template from.
 * \param template		The template to delete.
 */

static void analysis_delete_template(struct file_block *file, int template)
{
	enum analysis_report_type	type;

	/* Delete the specified template. */

//	if (template < 0 || template >= file->analysis->saved_report_count)
		return;

//	type = file->analysis->saved_reports[template].type;

	/* First remove the template from the flex block. */

//	if (!flexutils_delete_object((void **) &(file->analysis->saved_reports), sizeof(struct analysis_report), template)) {
//		error_msgs_report_error("BadDelete");
//		return;
//	}

//	file->analysis->saved_report_count--;
//	file_set_data_integrity(file, TRUE);

	/* If the rename template window is open for this template, close it now before the pointer is lost. */

//	analysis_template_save_force_rename_close(file, template);

	/* Now adjust any other template pointers for currently open report dialogues,
	 * which may be pointing further up the array.
	 * In addition, if the rename template pointer (analysis_save_template) is pointing to the deleted
	 * item, it needs to be unset.
	 */

//	switch (type) {
//	case REPORT_TYPE_TRANSACTION:
//		analysis_transaction_remove_template(file->analysis, template);
//		break;
//	case REPORT_TYPE_UNRECONCILED:
//		analysis_unreconciled_remove_template(file->analysis, template);
//		break;
//	case REPORT_TYPE_CASHFLOW:
//		analysis_cashflow_remove_template(file->analysis, template);
//		break;
//	case REPORT_TYPE_BALANCE:
//		analysis_balance_remove_template(file->analysis, template);
//		break;
//	case REPORT_TYPE_NONE:
//		break;
//	}

//	analysis_template_save_delete_template(file, template);
}


/**
 * Create a new analysis template in the static heap, optionally copying the
 * contents from an existing template.
 *
 * \param *base			Pointer to a template to copy, or NULL to
 *				create an empty template.
 * \return			Pointer to the new template, or NULL on failure.
 */

static struct analysis_report *analysis_create_template(struct analysis_report *base)
{
//	struct analysis_report	*new;

//	new = heap_alloc(sizeof(struct analysis_report));
//	if (new == NULL)
		return NULL;

//	if (base == NULL) {
//		*(new->name) = '\0';
//		new->type = REPORT_TYPE_NONE;
//		new->file = NULL;
//	} else {
//		analysis_copy_template(new, base);
//	}

//	return new;
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

