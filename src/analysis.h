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
#include "analysis_transaction.h"
#include "analysis_unreconciled.h"

/**
 * Saved Report Data Union.
 */
/*
union analysis_report_block {
	struct trans_rep		transaction;
	struct unrec_rep		unreconciled;
	struct cashflow_rep		cashflow;
	struct balance_rep		balance;
};
*/
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
	 * Remove all references to an account from a template.
	 */
	void		(*remove_account)(void *template, acct_t account);
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
 * Return the report template instance associated with a given file.
 *
 * \param *file			The file to return the instance for.
 * \return			The template instance, or NULL.
 */

struct analysis_template_block *analysis_get_templates(struct file_block *file);





















/**
 * Remove an account from all of the report templates in a file (pending
 * deletion).
 *
 * \param *file			The file to process.
 * \param account		The account to remove.
 */

void analysis_remove_account_from_templates(struct file_block *file, acct_t account);


/* Convert a numeric account list array into a textual list of comma-separated
 * account idents.
 *
 * \param *file			The file to process.
 * \param *list			Pointer to the buffer to take the textual
 *				list, which must be ANALYSIS_ACC_SPEC_LEN long.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_account_list_to_idents(struct analysis_block *instance, char *list, acct_t *array, int len);


/**
 * Open a report from a saved template.
 *
 * \param *file			The file owning the template.
 * \param *ptr			The Wimp pointer details.
 * \param selection		The menu selection entry.
 */

void analysis_open_template(struct file_block *file, wimp_pointer *ptr, template_t template);


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

union analysis_report_block *analysis_get_template_contents(struct analysis_report *template, enum analysis_report_type type);


/**
 * Return the file which owns a template.
 * 
 * \param *template		Pointer to the template for which to return
 *				the owning file.
 * \return			The owning file block, or NULL.
 */

struct file_block *analysis_get_template_file(struct analysis_report *template);


/**
 * Find a save template ID based on its name.
 *
 * \param *file			The file to search in.
 * \param *name			The name to search for.
 * \return			The matching template ID, or NULL_TEMPLATE.
 */

template_t analysis_get_template_from_name(struct file_block *file, char *name);


/**
 * Store a report's template into a file.
 *
 * \param *file			The file to work on.
 * \param *report		The report to take the template from.
 * \param template		The template pointer to save to, or
 *				NULL_TEMPLATE to add a new entry.
 * \param *name			Pointer to a name to give the template, or
 *				NULL to leave it as-is.
 */

void analysis_store_template(struct file_block *file, struct analysis_report *report, template_t template, char *name);


/**
 * Rename a template.
 *
 * \param *filer		The file containing the template.
 * \param template		The template to be renamed.
 * \param *name			Pointer to the new name.
 */

void analysis_rename_template(struct file_block *file, template_t template, char *name);



















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

