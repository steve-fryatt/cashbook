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
 * Initialise the Analysis module and all its dialogue boxes.
 */

void analysis_initialise(void);


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
 * Force the closure of any Analysis windows which are open and relate
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void analysis_force_windows_closed(struct file_block *file);


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
 * Return the file which owns a template.
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
 * \param *file			The file containing the template.
 * \param template		The template to return the name of.
 * \param *buffer		Pointer to a buffer to take the name, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting name string,
 *				either the supplied buffer or the original.
 */

char *analysis_get_template_name(struct file_block *file, template_t template, char *buffer, size_t length);


/**
 * Copy a Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

void analysis_copy_template(struct analysis_report *to, struct analysis_report *from);


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
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *load_status		Pointer to return the current status of the load operation.
 * \return			The state of the config read operation.
  */

enum config_read_status analysis_read_file(struct file_block *file, FILE *in, char *section, char *token, char *value, enum filing_status *load_status);

#endif

