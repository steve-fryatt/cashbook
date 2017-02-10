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

#include "account.h"

struct analysis_block;

typedef int template_t;

#define NULL_TEMPLATE ((template_t) (-1))

/**
 * The length of a saved report template name.
 */

#define ANALYSIS_SAVED_NAME_LEN 32

/**
 * Saved Report Types.
 */

enum analysis_report_type {
	REPORT_TYPE_NONE = 0,							/**< No report.									*/
	REPORT_TYPE_TRANS = 1,							/**< Transaction report.							*/
	REPORT_TYPE_UNREC = 2,							/**< Unreconciled transaction report.						*/
	REPORT_TYPE_CASHFLOW = 3,						/**< Cashflow report.								*/
	REPORT_TYPE_BALANCE = 4							/**< Balance report.								*/
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
 * Construct new transaction report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct trans_rep *analysis_create_transaction(void);


/**
 * Delete a transaction report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_transaction(struct trans_rep *report);


/**
 * Open the Transaction Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_transaction_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore);


/**
 * Construct new unreconciled report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct unrec_rep *analysis_create_unreconciled(void);


/**
 * Delete an unreconciled report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_unreconciled(struct unrec_rep *report);


/**
 * Open the Transaction Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_unreconciled_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore);


/**
 * Construct new cashflow report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct cashflow_rep *analysis_create_cashflow(void);


/**
 * Delete a cashflow report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_cashflow(struct cashflow_rep *report);


/**
 * Open the Cashflow Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_cashflow_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore);


/**
 * Construct new balance report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct balance_rep *analysis_create_balance(void);


/**
 * Delete a balance report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_balance(struct balance_rep *report);


/**
 * Open the Balance Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_balance_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore);


/**
 * Remove an account from all of the report templates in a file (pending
 * deletion).
 *
 * \param *file			The file to process.
 * \param account		The account to remove.
 */

void analysis_remove_account_from_templates(struct file_block *file, acct_t account);


/**
 * Convert a textual comma-separated list of hex numbers into a numeric
 * account list array.
 *
 * \param *file			The file to process.
 * \param *list			The textual hex number list to process.
 * \param *array		Pointer to memory to take the numeric list,
 *				with space for REPORT_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

int analysis_account_hex_to_list(struct file_block *file, char *list, acct_t *array);


/**
 * Convert a numeric account list array into a textual list of comma-separated
 * hex values.
 *
 * \param *file			The file to process.
 * \param *list			Pointer to the buffer to take the textual list.
 * \param size			The size of the buffer.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_account_list_to_hex(struct file_block *file, char *list, size_t size, acct_t *array, int len);


/**
 * Open a report from a saved template.
 *
 * \param *file			The file owning the template.
 * \param *ptr			The Wimp pointer details.
 * \param selection		The menu selection entry.
 */

void analysis_open_template(struct file_block *file, wimp_pointer *ptr, template_t template);


/**
 * Return the number of templates in the given file.
 * 
 * \param *file			The file to report on.
 * \return			The number of templates, or 0 on error.
 */
 
int analysis_get_template_count(struct file_block *file);


/**
 * Return a volatile pointer to a template block from within a file's saved
 * templates. This is a pointre into a flex heap block, so it will only
 * be valid until an operation occurs to shift the blocks.
 * 
 * \param *file			The file containing the template of interest.
 * \param template		The number of the requied template.
 * \return			Volatile pointer to the template, or NULL.
 */

struct analysis_report *analysis_get_template(struct file_block *file, template_t template);


/**
 * Return the file which owns a template.
 * 
 * \param *pointer		Pointer to return the owning file for.
 * \return			The owning file block, or NULL.
 */

struct file_block *analysis_get_template_file(struct analysis_report *template);


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

char *analysis_get_template_name(struct analysis_report *template, char *buffer, size_t length);


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
 * Save the Report Template details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void analysis_write_file(struct file_block *file, FILE *out);


/**
 * Read Report Template details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool analysis_read_file(struct file_block *file, struct filing_block *in);

#endif

