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
#include "flexutils.h"
#include "report.h"
#include "transact.h"












enum analysis_flags {
	ANALYSIS_REPORT_NONE = 0,
	ANALYSIS_REPORT_FROM = 0x0001,
	ANALYSIS_REPORT_TO = 0x0002,
	ANALYSIS_REPORT_INCLUDE = 0x0004
};


/**
 * Saved Report.
 */

struct analysis_report {
	struct file_block		*file;					/**< The file to which the template belongs.					*/
	char				name[ANALYSIS_SAVED_NAME_LEN];		/**< The name of the saved report template.					*/
	enum analysis_report_type	type;					/**< The type of the template.							*/

	union analysis_report_block	data;					/**< The template-type-specific data.						*/
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

	struct trans_rep		*trans_rep;				/**< Data relating to the transaction report dialogue.				*/
	struct unrec_rep		*unrec_rep;				/**< Data relating to the unreconciled report dialogue.				*/
	struct cashflow_rep		*cashflow_rep;				/**< Data relating to the cashflow report dialogue.				*/
	struct balance_rep		*balance_rep;				/**< Data relating to the balance report dialogue.				*/
};

/* Report windows. */





static struct analysis_report	analysis_report_template;			/**< New report settings for passing to the Report module.			*/

static acct_t			analysis_wildcard_account_list = NULL_ACCOUNT;	/**< Pass a pointer to this to set all accounts.				*/


static void		analysis_find_date_range(struct file_block *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget);

static void		analysis_remove_account_from_report_template(struct analysis_report *template, void *data);
static void		analysis_remove_account_from_template(struct analysis_report *template, acct_t account);
static int		analysis_remove_account_from_list(acct_t account, acct_t *array, int *count);
static void		analysis_clear_account_report_flags(struct file_block *file, struct analysis_data *data);
static void		analysis_set_account_report_flags_from_list(struct file_block *file, struct analysis_data *data, unsigned type, unsigned flags, acct_t *array, int count);

static void		analysis_delete_template(struct file_block *file, int template);
static struct analysis_report	*analysis_create_template(struct analysis_report *base);

static void		analysis_copy_template(struct analysis_report *to, struct analysis_report *from);




/**
 * Initialise the Analysis module and all its dialogue boxes.
 */

void analysis_initialise(void)
{
	analysis_transaction_initialise();
	analysis_unreconciled_initialise();
	analysis_cashflow_initialise();
	analysis_balance_initialise();
	analysis_template_save_initialise();
	analysis_lookup_initialise();
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
 * \param *instance	The instance to be deleted.
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
	int		i, transactions;
	osbool		find_start, find_end;
	date_t		date;

	if (budget) {
		/* Get the start and end dates from the budget settings. */

		budget_get_dates(file, start_date, end_date);
	} else {
		/* Get the start and end dates from the icon text. */

		*start_date = date1;
		*end_date = date2;
	}

	find_start = (*start_date == NULL_DATE);
	find_end = (*end_date == NULL_DATE);

	/* If either of the dates wasn't specified, we need to find the earliest and latest dates in the file. */

	if (find_start || find_end) {
		transactions = transact_get_count(file);

		if (find_start)
			*start_date = (transactions > 0) ? transact_get_date(file, 0) : NULL_DATE;

		if (find_end)
			*end_date = (transactions > 0) ? transact_get_date(file, transactions - 1) : NULL_DATE;

		for (i = 0; i < transactions; i++) {
			date = transact_get_date(file, i);

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

	analysis_transaction_remove_account(file->analysis->trans_rep, account);
	analysis_unreconciled_remove_account(file->analysis->unrec_rep, account);
	analysis_cashflow_remove_account(file->analysis->cashflow_rep, account);
	analysis_balance_remove_account(file->analysis->balance_rep, account);

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

	analysis_remove_account_from_template(template, *account);
}

/**
 * Remove any references to an account from an analysis template.
 *
 * \param *template		The template to process.
 * \param account		The account to be removed.
 */

static void analysis_remove_account_from_template(struct analysis_report *template, acct_t account)
{
	if (template == NULL)
		return;

	switch (template->type) {
	case REPORT_TYPE_TRANS:
		analysis_transaction_remove_account(&(template->data.transaction), account);
		break;

	case REPORT_TYPE_UNREC:
		analysis_unreconciled_remove_account(&(template->data.unreconciled), account);
		break;

	case REPORT_TYPE_CASHFLOW:
		analysis_cashflow_remove_account(&(template->data.cashflow), account);
		break;

	case REPORT_TYPE_BALANCE:
		analysis_balance_remove_account(&(template->data.balance), account);
		break;

	case REPORT_TYPE_NONE:
		break;
	}
}


/**
 * Remove any references to an account from an account list array.
 *
 * \param account		The account to remove, if present.
 * \param *array		The account list array.
 * \param *count		Pointer to number of accounts in the array, which
 *				is updated before return.
 * \return			The new account count in the array.
 */

static int analysis_remove_account_from_list(acct_t account, acct_t *array, int *count)
{
	int	i = 0, j = 0;

	while (i < *count && j < *count) {
		/* Skip j on until it finds an account that is to be left in. */

		while (j < *count && array[j] == account)
			j++;

		/* If pointers are different, and not pointing to the account, copy down the account. */
		if (i < j && i < *count && j < *count && array[j] != account)
			array[i] = array[j];

		/* Increment the pointers if necessary */
		if (array[i] != account) {
			i++;
			j++;
		}
	}

	*count = i;

	return i;
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
	int	i;

	if (file == NULL || data == NULL)
		return;

	for (i = 0; i < account_get_count(file); i++)
		data[i].report_flags = ANALYSIS_REPORT_NONE;
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
	int	account, i;

	for (i = 0; i < count; i++) {
		account = array[i];

		if (account == NULL_ACCOUNT) {
			/* 'Wildcard': set all the accounts which match the
			 * given account type.
			 */

			for (account = 0; account < account_get_count(file); account++)
				if ((account_get_type(file, account) & type) != 0)
					data[account].report_flags |= flags;
		} else {
			/* Set a specific account. */

			if (account < account_get_count(file))
				data[account].report_flags |= flags;
		}
	}
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
	char	*copy, *ident;
	int	account, i = 0;

	copy = strdup(list);

	if (copy == NULL)
		return 0;

	ident = strtok(copy, ",");

	while (ident != NULL && i < ANALYSIS_ACC_LIST_LEN) {
		if (strcmp(ident, "*") == 0) {
			array[i++] = NULL_ACCOUNT;
		} else {
			account = account_find_by_ident(file, ident, type);

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
 * \param *file			The file to process.
 * \param *list			Pointer to the buffer to take the textual
 *				list, which must be ANALYSIS_ACC_SPEC_LEN long.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_account_list_to_idents(struct analysis_block *instance, char *list, acct_t *array, int len)
{
	char	buffer[ACCOUNT_IDENT_LEN];
	int	account, i;

	if (instance == NULL || instance->file == NULL)
		return;

	*list = '\0';

	for (i = 0; i < len; i++) {
		account = array[i];

		if (account != NULL_ACCOUNT)
			strncpy(buffer, account_get_ident(instance->file, account), ACCOUNT_IDENT_LEN);
		else
			strncpy(buffer, "*", ACCOUNT_IDENT_LEN);

		buffer[ACCOUNT_IDENT_LEN - 1] = '\0';

		if (strlen(list) > 0 && strlen(list) + 1 < ANALYSIS_ACC_SPEC_LEN)
			strcat(list, ",");

		if (strlen(list) + strlen(buffer) < ANALYSIS_ACC_SPEC_LEN)
			strcat(list, buffer);
	}
}


/**
 * Convert a textual comma-separated list of hex numbers into a numeric
 * account list array.
 *
 * \param *file			The file to process.
 * \param *list			The textual hex number list to process.
 * \param *array		Pointer to memory to take the numeric list,
 *				with space for ANALYSIS_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

int analysis_account_hex_to_list(struct file_block *file, char *list, acct_t *array)
{
	char	*copy, *value;
	int	i = 0;

	copy = strdup(list);

	if (copy == NULL)
		return 0;

	value = strtok(copy, ",");

	while (value != NULL && i < ANALYSIS_ACC_LIST_LEN) {
		array[i++] = strtoul(value, NULL, 16);

		value = strtok(NULL, ",");
	}

	free(copy);

	return i;
}


/* Convert a numeric account list array into a textual list of comma-separated
 * hex values.
 *
 * \param *file			The file to process.
 * \param *list			Pointer to the buffer to take the textual list.
 * \param size			The size of the buffer.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_account_list_to_hex(struct file_block *file, char *list, size_t size, acct_t *array, int len)
{
	char	buffer[32];
	int	i;

	*list = '\0';

	for (i=0; i<len; i++) {
		sprintf(buffer, "%x", array[i]);

		if (strlen(list) > 0 && strlen(list)+1 < size)
			strcat(list, ",");

		if (strlen(list) + strlen(buffer) < size)
			strcat(list, buffer);
	}
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
	case REPORT_TYPE_TRANS:
		analysis_transaction_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_UNREC:
		analysis_unreconciled_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_CASHFLOW:
		analysis_cashflow_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_BALANCE:
		analysis_balance_open_window(file->analysis, ptr, template, config_opt_read("RememberValues"));
		break;

	case REPORT_TYPE_NONE:
		break;
	}
}


/**
 * Return the number of templates in the given file.
 * 
 * \FIXME -- This should probably be passed direct on to the templates module when possible.
 * 
 * \param *file			The file to report on.
 * \return			The number of templates, or 0 on error.
 */
 
int analysis_get_template_count(struct file_block *file)
{
	if (file == NULL || file->analysis == NULL)
		return 0;

	return analysis_template_get_count(file->analysis->templates);
}


/**
 * Return a volatile pointer to a template block from within a file's saved
 * templates. This is a pointer into a flex heap block, so it will only
 * be valid until an operation occurs to shift the blocks.
 * 
 * \FIXME -- This should probably be passed direct on to the templates module when possible.
 * 
 * \param *file			The file containing the template of interest.
 * \param template		The number of the requied template.
 * \return			Volatile pointer to the template, or NULL.
 */

struct analysis_report *analysis_get_template(struct file_block *file, template_t template)
{
	if (file == NULL || file->analysis == NULL)
		return NULL;

	return analysis_template_get_report(file->analysis->templates, template);
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
	if (template == NULL || template->type != type)
		return NULL;

	return &(template->data);
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
	if (template == NULL)
		return NULL;

	return template->file;
}

/**
 * Return the name for an analysis template.
 *
 * If a buffer is supplied, the name is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * name in the template array is returned instead. In the latter case, this
 * pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *template		Pointer to the template to return the name of.
 * \param *buffer		Pointer to a buffer to take the name, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting name string,
 *				either the supplied buffer or the original.
 */

char *analysis_get_template_name(struct analysis_report *template, char *buffer, size_t length)
{
	if (template == NULL) {
		if (buffer != NULL && length > 0) {
			*buffer = '\0';
			return buffer;
		}

		return NULL;
	}

	if (buffer == NULL || length == 0)
		return template->name;

	strncpy(buffer, template->name, length);
	buffer[length - 1] = '\0';

	return buffer;
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
	if (file == NULL || file->analysis == NULL)
		return NULL_TEMPLATE;

	return analysis_template_get_from_name(file->analysis->templates, name);
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
	if (file == NULL || file->analysis == NULL || report == NULL)
		return;

	analysis_template_store(file->analysis->templates, report, template, name);
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
	if (file == NULL || file->analysis == NULL || name == NULL)
		return;

	analysis_template_rename(file->analysis->templates, template, name);
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

	if (template < 0 || template >= file->analysis->saved_report_count)
		return;

	type = file->analysis->saved_reports[template].type;

	/* First remove the template from the flex block. */

	if (!flexutils_delete_object((void **) &(file->analysis->saved_reports), sizeof(struct analysis_report), template)) {
		error_msgs_report_error("BadDelete");
		return;
	}

	file->analysis->saved_report_count--;
	file_set_data_integrity(file, TRUE);

	/* If the rename template window is open for this template, close it now before the pointer is lost. */

	analysis_template_save_force_rename_close(file, template);

	/* Now adjust any other template pointers for currently open report dialogues,
	 * which may be pointing further up the array.
	 * In addition, if the rename template pointer (analysis_save_template) is pointing to the deleted
	 * item, it needs to be unset.
	 */

	switch (type) {
	case REPORT_TYPE_TRANS:
		analysis_transaction_remove_template(file->analysis, template);
		break;
	case REPORT_TYPE_UNREC:
		analysis_unreconciled_remove_template(file->analysis, template);
		break;
	case REPORT_TYPE_CASHFLOW:
		analysis_cashflow_remove_template(file->analysis, template);
		break;
	case REPORT_TYPE_BALANCE:
		analysis_balance_remove_template(file->analysis, template);
		break;
	case REPORT_TYPE_NONE:
		break;
	}

	analysis_template_save_delete_template(file, template);
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
	struct analysis_report	*new;
	
	new = heap_alloc(sizeof(struct analysis_report));
	if (new == NULL)
		return NULL;

	if (base == NULL) {
		*(new->name) = '\0';
		new->type = REPORT_TYPE_NONE;
		new->file = NULL;
	} else {
		analysis_copy_template(new, base);
	}

	return new;
}


/**
 * Copy a Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_copy_template(struct analysis_report *to, struct analysis_report *from)
{
	if (from == NULL || to == NULL)
		return;

#ifdef DEBUG
	debug_printf("Copy template from 0x%x to 0x%x", from, to);
#endif

	strcpy(to->name, from->name);
	to->type = from->type;
	to->file = from->file;

	switch (from->type) {
	case REPORT_TYPE_TRANS:
		analysis_copy_transaction_template(&(to->data.transaction), &(from->data.transaction));
		break;

	case REPORT_TYPE_UNREC:
		analysis_copy_unreconciled_template(&(to->data.unreconciled), &(from->data.unreconciled));
		break;

	case REPORT_TYPE_CASHFLOW:
		analysis_copy_cashflow_template(&(to->data.cashflow), &(from->data.cashflow));
		break;

	case REPORT_TYPE_BALANCE:
		analysis_copy_balance_template(&(to->data.balance), &(from->data.balance));
		break;

	case REPORT_TYPE_NONE:
		break;
	}
}


/**
 * Save the Report Template details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void analysis_write_file(struct file_block *file, FILE *out)
{
	int	i;
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	fprintf(out, "\n[Reports]\n");

	fprintf(out, "Entries: %x\n", file->analysis->saved_report_count);

	for (i = 0; i < file->analysis->saved_report_count; i++) {
		switch(file->analysis->saved_reports[i].type) {
		case REPORT_TYPE_TRANS:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_TRANS,
					file->analysis->saved_reports[i].data.transaction.date_from,
					file->analysis->saved_reports[i].data.transaction.date_to,
					file->analysis->saved_reports[i].data.transaction.budget,
					file->analysis->saved_reports[i].data.transaction.group,
					file->analysis->saved_reports[i].data.transaction.period,
					file->analysis->saved_reports[i].data.transaction.period_unit,
					file->analysis->saved_reports[i].data.transaction.lock,
					file->analysis->saved_reports[i].data.transaction.output_trans,
					file->analysis->saved_reports[i].data.transaction.output_summary,
					file->analysis->saved_reports[i].data.transaction.output_accsummary);
			if (*(file->analysis->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->analysis->saved_reports[i].name);
			if (file->analysis->saved_reports[i].data.transaction.from_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.transaction.from,
						file->analysis->saved_reports[i].data.transaction.from_count);
				config_write_token_pair(out, "From", buffer);
			}
			if (file->analysis->saved_reports[i].data.transaction.to_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.transaction.to,
						file->analysis->saved_reports[i].data.transaction.to_count);
				config_write_token_pair(out, "To", buffer);
			}
			if (*(file->analysis->saved_reports[i].data.transaction.ref) != '\0')
				config_write_token_pair(out, "Ref", file->analysis->saved_reports[i].data.transaction.ref);
			if (file->analysis->saved_reports[i].data.transaction.amount_min != NULL_CURRENCY ||
					file->analysis->saved_reports[i].data.transaction.amount_max != NULL_CURRENCY) {
				sprintf(buffer, "%x,%x",
						file->analysis->saved_reports[i].data.transaction.amount_min,
						file->analysis->saved_reports[i].data.transaction.amount_max);
				config_write_token_pair(out, "Amount", buffer);
			}
			if (*(file->analysis->saved_reports[i].data.transaction.desc) != '\0')
				config_write_token_pair(out, "Desc", file->analysis->saved_reports[i].data.transaction.desc);
			break;

		case REPORT_TYPE_UNREC:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_UNREC,
					file->analysis->saved_reports[i].data.unreconciled.date_from,
					file->analysis->saved_reports[i].data.unreconciled.date_to,
					file->analysis->saved_reports[i].data.unreconciled.budget,
					file->analysis->saved_reports[i].data.unreconciled.group,
					file->analysis->saved_reports[i].data.unreconciled.period,
					file->analysis->saved_reports[i].data.unreconciled.period_unit,
					file->analysis->saved_reports[i].data.unreconciled.lock);
			if (*(file->analysis->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->analysis->saved_reports[i].name);
			if (file->analysis->saved_reports[i].data.unreconciled.from_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.unreconciled.from,
						file->analysis->saved_reports[i].data.unreconciled.from_count);
				config_write_token_pair(out, "From", buffer);
			}
			if (file->analysis->saved_reports[i].data.unreconciled.to_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.unreconciled.to,
						file->analysis->saved_reports[i].data.unreconciled.to_count);
				config_write_token_pair(out, "To", buffer);
			}
			break;

		case REPORT_TYPE_CASHFLOW:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_CASHFLOW,
					file->analysis->saved_reports[i].data.cashflow.date_from,
					file->analysis->saved_reports[i].data.cashflow.date_to,
					file->analysis->saved_reports[i].data.cashflow.budget,
					file->analysis->saved_reports[i].data.cashflow.group,
					file->analysis->saved_reports[i].data.cashflow.period,
					file->analysis->saved_reports[i].data.cashflow.period_unit,
					file->analysis->saved_reports[i].data.cashflow.lock,
					file->analysis->saved_reports[i].data.cashflow.tabular,
					file->analysis->saved_reports[i].data.cashflow.empty);
			if (*(file->analysis->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->analysis->saved_reports[i].name);
			if (file->analysis->saved_reports[i].data.cashflow.accounts_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.cashflow.accounts,
						file->analysis->saved_reports[i].data.cashflow.accounts_count);
				config_write_token_pair(out, "Accounts", buffer);
			}
			if (file->analysis->saved_reports[i].data.cashflow.incoming_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.cashflow.incoming,
						file->analysis->saved_reports[i].data.cashflow.incoming_count);
				config_write_token_pair(out, "Incoming", buffer);
			}
			if (file->analysis->saved_reports[i].data.cashflow.outgoing_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.cashflow.outgoing,
						file->analysis->saved_reports[i].data.cashflow.outgoing_count);
				config_write_token_pair(out, "Outgoing", buffer);
			}
			break;

		case REPORT_TYPE_BALANCE:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_BALANCE,
					file->analysis->saved_reports[i].data.balance.date_from,
					file->analysis->saved_reports[i].data.balance.date_to,
					file->analysis->saved_reports[i].data.balance.budget,
					file->analysis->saved_reports[i].data.balance.group,
					file->analysis->saved_reports[i].data.balance.period,
					file->analysis->saved_reports[i].data.balance.period_unit,
					file->analysis->saved_reports[i].data.balance.lock,
					file->analysis->saved_reports[i].data.balance.tabular);
			if (*(file->analysis->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->analysis->saved_reports[i].name);
			if (file->analysis->saved_reports[i].data.balance.accounts_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.balance.accounts,
						file->analysis->saved_reports[i].data.balance.accounts_count);
				config_write_token_pair(out, "Accounts", buffer);
			}
			if (file->analysis->saved_reports[i].data.balance.incoming_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.balance.incoming,
						file->analysis->saved_reports[i].data.balance.incoming_count);
				config_write_token_pair(out, "Incoming", buffer);
			}
			if (file->analysis->saved_reports[i].data.balance.outgoing_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->analysis->saved_reports[i].data.balance.outgoing,
						file->analysis->saved_reports[i].data.balance.outgoing_count);
				config_write_token_pair(out, "Outgoing", buffer);
			}
			break;

		case REPORT_TYPE_NONE:
			break;
		}
	}
}


/**
 * Read Report Template details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool analysis_read_file(struct file_block *file, struct filing_block *in)
{
	size_t			block_size;
	template_t		template = NULL_TEMPLATE;

#ifdef DEBUG
	debug_printf("\\GLoading Analysis Reports.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->analysis->saved_reports), sizeof(struct analysis_report), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->analysis->saved_report_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize((void **) &(file->analysis->saved_reports), block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->analysis->saved_report_count;
			}
		} else if (filing_test_token(in, "@")) {
			file->analysis->saved_report_count++;
			if (file->analysis->saved_report_count > block_size) {
				block_size = file->analysis->saved_report_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				if (!flexutils_load_resize((void **) &(file->analysis->saved_reports), block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			}
			template = file->analysis->saved_report_count - 1;
			file->analysis->saved_reports[template].type = analysis_get_report_type_field(in);
			switch(file->analysis->saved_reports[template].type) {
			case REPORT_TYPE_TRANS:
				file->analysis->saved_reports[template].data.transaction.date_from = date_get_date_field(in);
				file->analysis->saved_reports[template].data.transaction.date_to = date_get_date_field(in);
				file->analysis->saved_reports[template].data.transaction.budget = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.transaction.group = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.transaction.period = filing_get_int_field(in);
				file->analysis->saved_reports[template].data.transaction.period_unit = date_get_period_field(in);
				file->analysis->saved_reports[template].data.transaction.lock = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.transaction.output_trans = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.transaction.output_summary = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.transaction.output_accsummary = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.transaction.amount_min = NULL_CURRENCY;
				file->analysis->saved_reports[template].data.transaction.amount_max = NULL_CURRENCY;
				file->analysis->saved_reports[template].data.transaction.from_count = 0;
				file->analysis->saved_reports[template].data.transaction.to_count = 0;
				*(file->analysis->saved_reports[template].data.transaction.ref) = '\0';
				*(file->analysis->saved_reports[template].data.transaction.desc) = '\0';
				break;

			case REPORT_TYPE_UNREC:
				file->analysis->saved_reports[template].data.unreconciled.date_from = date_get_date_field(in);
				file->analysis->saved_reports[template].data.unreconciled.date_to = date_get_date_field(in);
				file->analysis->saved_reports[template].data.unreconciled.budget = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.unreconciled.group = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.unreconciled.period = filing_get_int_field(in);
				file->analysis->saved_reports[template].data.unreconciled.period_unit = date_get_period_field(in);
				file->analysis->saved_reports[template].data.unreconciled.lock = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.unreconciled.from_count = 0;
				file->analysis->saved_reports[template].data.unreconciled.to_count = 0;
				break;

			case REPORT_TYPE_CASHFLOW:
				file->analysis->saved_reports[template].data.cashflow.date_from = date_get_date_field(in);
				file->analysis->saved_reports[template].data.cashflow.date_to = date_get_date_field(in);
				file->analysis->saved_reports[template].data.cashflow.budget = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.cashflow.group = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.cashflow.period = filing_get_int_field(in);
				file->analysis->saved_reports[template].data.cashflow.period_unit = date_get_period_field(in);
				file->analysis->saved_reports[template].data.cashflow.lock = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.cashflow.tabular = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.cashflow.empty = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.cashflow.accounts_count = 0;
				file->analysis->saved_reports[template].data.cashflow.incoming_count = 0;
				file->analysis->saved_reports[template].data.cashflow.outgoing_count = 0;
				break;

			case REPORT_TYPE_BALANCE:
				file->analysis->saved_reports[template].data.balance.date_from = date_get_date_field(in);
				file->analysis->saved_reports[template].data.balance.date_to = date_get_date_field(in);
				file->analysis->saved_reports[template].data.balance.budget = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.balance.group = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.balance.period = filing_get_int_field(in);
				file->analysis->saved_reports[template].data.balance.period_unit = date_get_period_field(in);
				file->analysis->saved_reports[template].data.balance.lock = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.balance.tabular = filing_get_opt_field(in);
				file->analysis->saved_reports[template].data.balance.accounts_count = 0;
				file->analysis->saved_reports[template].data.balance.incoming_count = 0;
				file->analysis->saved_reports[template].data.balance.outgoing_count = 0;
				break;

			case REPORT_TYPE_NONE:
				break;
			default:
				filing_set_status(in, FILING_STATUS_UNEXPECTED);
				break;
			}
		} else if (template != NULL_TEMPLATE && filing_test_token(in, "Name")) {
			filing_get_text_value(in, file->analysis->saved_reports[template].name, ANALYSIS_SAVED_NAME_LEN);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_CASHFLOW &&
				filing_test_token(in, "Accounts")) {
			file->analysis->saved_reports[template].data.cashflow.accounts_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.cashflow.accounts);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_CASHFLOW &&
				filing_test_token(in, "Incoming")) {
			file->analysis->saved_reports[template].data.cashflow.incoming_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.cashflow.incoming);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_CASHFLOW &&
				filing_test_token(in, "Outgoing")) {
			file->analysis->saved_reports[template].data.cashflow.outgoing_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.cashflow.outgoing);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_BALANCE &&
				filing_test_token(in, "Accounts")) {
			file->analysis->saved_reports[template].data.balance.accounts_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.balance.accounts);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_BALANCE &&
				filing_test_token(in, "Incoming")) {
			file->analysis->saved_reports[template].data.balance.incoming_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.balance.incoming);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_BALANCE &&
				filing_test_token(in, "Outgoing")) {
			file->analysis->saved_reports[template].data.balance.outgoing_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.balance.outgoing);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_TRANS &&
				filing_test_token(in, "From")) {
			file->analysis->saved_reports[template].data.transaction.from_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.transaction.from);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_TRANS &&
				filing_test_token(in, "To")) {
			file->analysis->saved_reports[template].data.transaction.to_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.transaction.to);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_UNREC &&
				filing_test_token(in, "From")) {
			file->analysis->saved_reports[template].data.unreconciled.from_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.unreconciled.from);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_UNREC &&
				filing_test_token(in, "To")) {
			file->analysis->saved_reports[template].data.unreconciled.to_count =
					analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), file->analysis->saved_reports[template].data.unreconciled.to);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_TRANS &&
				filing_test_token(in, "Ref")) {
			filing_get_text_value(in, file->analysis->saved_reports[template].data.transaction.ref, TRANSACT_REF_FIELD_LEN);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_TRANS &&
				filing_test_token(in, "Amount")) {
			file->analysis->saved_reports[template].data.transaction.amount_min = currency_get_currency_field(in);
			file->analysis->saved_reports[template].data.transaction.amount_max = currency_get_currency_field(in);
		} else if (template != NULL_TEMPLATE && file->analysis->saved_reports[template].type == REPORT_TYPE_TRANS &&
				filing_test_token(in, "Desc")) {
			filing_get_text_value(in, file->analysis->saved_reports[template].data.transaction.desc, TRANSACT_DESCRIPT_FIELD_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink((void **) &(file->analysis->saved_reports), file->analysis->saved_report_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
}
