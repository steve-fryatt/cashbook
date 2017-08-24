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
 * \file: analysis.h
 *
 * Analysis interface implementation.
 */

#ifndef CASHBOOK_ANALYSIS
#define CASHBOOK_ANALYSIS

struct analysis_block;


/**
 * The different types of analysis report.
 */




typedef int template_t;

#define NULL_TEMPLATE ((template_t) (-1))

/**
 * The length of an account list text field in a dialogue.
 */

#define ANALYSIS_ACC_SPEC_LEN 128

/**
 * The number of accounts which can be stored in a list.
 */

#define ANALYSIS_ACC_LIST_LEN 64

#include "account.h"
#include "analysis_balance.h"
#include "analysis_cashflow.h"
#include "analysis_data.h"
#include "analysis_transaction.h"
#include "analysis_unreconciled.h"

/**
 * Saved Report Types.
 */

enum analysis_report_type {
	REPORT_TYPE_NONE = 0,							/**< No report.									*/
	REPORT_TYPE_TRANSACTION = 1,						/**< Transaction report.							*/
	REPORT_TYPE_UNRECONCILED = 2,						/**< Unreconciled transaction report.						*/
	REPORT_TYPE_CASHFLOW = 3,						/**< Cashflow report.								*/
	REPORT_TYPE_BALANCE = 4							/**< Balance report.								*/
};


/**
 * Details to be supplied by individual report types.
 */

struct analysis_report_details {
	/**
	 * Construct a new analysis report instance.
	 */
	void*		(*create_instance)(struct analysis_block *parent);

	/**
	 * Delete an analysis report instance.
	 */
	void		(*delete_instance)(void *instance);

	/**
	 * Open a new analysis report dialogue.
	 */
	void		(*open_window)(void *instance, wimp_pointer *pointer, template_t template, osbool restore);

	/**
	 * Fill an analysis report dialogue.
	 */
	void		(*fill_window)(struct analysis_block *parent, wimp_w window, void *template);

	/**
	 * Read the values from an analysis report dialogue.
	 */
	void		(*read_window)(struct analysis_block *parent, wimp_w window, void *template);

	/**
	 * Run an analysis report.
	 */
	void		(*run_report)(struct analysis_block *parent, struct report *report, void *template, struct analysis_data_block *scratch);

	/**
	 * Read a template token in from a saved CashBook file.
	 */
	void		(*process_file_token)(void *template, struct filing_block *in);

	/**
	 * Write a template out to a saved CashBook file.
	 */
	void		(*write_file_template)(void *template, FILE *out, char *name);

	/**
	 * Copy a template from one location to another.
	 */
	void		(*copy_template)(void *from, void *to);

	/**
	 * Rename a template.
	 */ 
	void		(*rename_template)(struct analysis_block *parent, template_t template, char *name);

	/**
	 * Remove all references to an account from a template.
	 */
	void		(*remove_account)(void *template, acct_t account);

	/**
	 * Remove a template definition.
	 */
	void		(*remove_template)(struct analysis_block *parent, template_t template);
};


/**
 * Get a report type field from an input file.
 */

#define analysis_get_report_type_field(in) ((enum analysis_report_type) filing_get_int_field((in)))


/**
 * Initialise the Analysis module and all its dialogue boxes.
 */

void analysis_initialise(void);


/**
 * Return the report type definition for a report type, or NULL if one
 * isn't defined.
 *
 * \param type			The type of report to look up.
 * \return			Pointer to the report type definition, or NULL.
 */

struct analysis_report_details *analysis_get_report_details(enum analysis_report_type type);


/**
 * Create a new analysis report instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct analysis_block *analysis_create_instance(struct file_block *file);


/**
 * Delete an analysis report instance, and all of its data.
 *
 * \param *instance	The instance to be deleted.
 */

void analysis_delete_instance(struct analysis_block *instance);


/**
 * Return the file instance to which an analysis report instance belongs.
 *
 * \param *instance	The instance to look up.
 * \return		The parent file, or NULL.
 */

struct file_block *analysis_get_file(struct analysis_block *instance);


/**
 * Return the report template instance associated with a given analysis instance.
 *
 * \param *instance		The instance to return the templates for.
 * \return			The template instance, or NULL.
 */

struct analysis_template_block *analysis_get_templates(struct analysis_block *instance);


/**
 * Open a new Analysis Report dialogue box.
 *
 * \param *parent		The analysis instance to own the dialogue.
 * \param *pointer		The current Wimp Pointer details.
 * \param type			The type of report to open.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_open_window(struct analysis_block *instance, wimp_pointer *pointer, enum analysis_report_type type, osbool restore);


/**
 * Open a report from a saved template.
 *
 * \param *instance		The analysis instance owning the template.
 * \param *pointer		The current Wimp Pointer details.
 * \param selection		The menu selection entry.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_open_template(struct analysis_block *instance, wimp_pointer *pointer, template_t template, osbool restore);


/**
 * Remove an account from all of the report templates in a file (pending
 * deletion).
 *
 * \param *file			The file to process.
 * \param account		The account to remove.
 */

void analysis_remove_account_from_templates(struct file_block *file, acct_t account);


/**
 * Run a report, using supplied template data.
 *
 * \param *instance		The analysis instance owning the report.
 * \param type			The type of report to run.
 * \param *template		The template data
 */

void analysis_run_report(struct analysis_block *instance, enum analysis_report_type type, void *template);


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

void analysis_find_date_range(struct analysis_block *instance, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget);


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

size_t analysis_account_idents_to_list(struct analysis_block *instance, enum account_type type, char *list, acct_t *array, size_t length);


/* Convert a numeric account list array into a textual list of comma-separated
 * account idents.
 *
 * \param *instance		The analysis instance owning the data.
 * \param *list			Pointer to the buffer to take the textual list.
 * \param length		The size of the buffer.
 * \param *array		The account list array to be converted.
 * \param count			The number of accounts in the list.
 */

void analysis_account_list_to_idents(struct analysis_block *instance, char *list, size_t length, acct_t *array, size_t count);


/**
 * Save the analysis template details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void analysis_write_file(struct file_block *file, FILE *out);


/**
 * Read analysis template details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool analysis_read_file(struct file_block *file, struct filing_block *in);

#endif

