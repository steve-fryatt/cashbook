/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: analysis_template.h
 *
 * Analysis Teplate Storage interface.
 */


#ifndef CASHBOOK_ANALYSIS_TEMPLATE
#define CASHBOOK_ANALYSIS_TEMPLATE

#include "analysis.h"

/**
 * The length of a saved report template name.
 */

#define ANALYSIS_SAVED_NAME_LEN 32

/**
 * Structure holding an instance of the analysis template data.
 */

struct analysis_template_block;


/**
 * Structure holding a saved report.
 */

struct analysis_report;


/**
 * Allow a analysis client to report the size of its template block.
 *
 * \param size			The size of the block.
 */

void analysis_template_set_block_size(size_t size);


/**
 * Construct a new analysis template storage instance
 *
 * \param *parent		The analysis instance which owns this new store.
 * \return			Pointer to the new instance, or NULL.
 */

struct analysis_template_block *analysis_template_create_instance(struct analysis_block *parent);


/**
 * Delete an analysis template storage instance.
 *
 * \param *instance		The instance to be deleted.
 */

void analysis_template_delete_instance(struct analysis_template_block *instance);


/**
 * Return the analysis template storage instance owning a report.
 *
 * \param *report		The report of interest.
 * \return			The owning instance, or NULL.
 */ 

struct analysis_template_block *analysis_template_get_instance(struct analysis_report *report);


/**
 * Return the cashbook file owning a template storage instance.
 *
 * \param *instance		The instance of interest.
 * \return			The owning file, or NULL.
 */

struct file_block *analysis_template_get_file(struct analysis_template_block *instance);


/**
 * Remove any references to a given account from all of the saved analysis
 * templates in an instance.
 * 
 * \param *instance		The instance to be updated.
 * \param account		The account to be removed.
 */

void analysis_template_remove_account(struct analysis_template_block *instance, acct_t account);


/**
 * Remove any references to a given account from an analysis template.
 *
 * \param *report		The report to process.
 * \param account		The account to be removed.
 */

void analysis_template_remove_account_from_template(struct analysis_report *report, acct_t account);


/**
 * Remove any references to an account from an account list array.
 *
 * \param account		The account to remove, if present.
 * \param *array		The account list array.
 * \param *count		Pointer to number of accounts in the array, which
 *				is updated before return.
 * \return			The new account count in the array.
 */

int analysis_template_remove_account_from_list(acct_t account, acct_t *array, int *count);


/**
 * Return the type of template which is stored at a given index.
 * 
 * \param *instance		The save report instance to query.
 * \param template		The template to query.
 * \return			The template type, or REPORT_TYPE_NONE.
 */

enum analysis_report_type analysis_template_type(struct analysis_template_block *instance, template_t template);


/**
 * Return the number of templates in the given instance.
 * 
 * \param *file			The instance to report on.
 * \return			The number of templates, or 0 on error.
 */
 
int analysis_template_get_count(struct analysis_template_block *instance);


/**
 * Return a volatile pointer to a report block from within an instance's
 * saved templates. This is a pointer into a flex heap block, so it will
 * only be valid until an operation occurs to shift the blocks.
 * 
 * \param *instance		The instance containing the report of interest.
 * \param template		The number of the requied report.
 * \return			Volatile pointer to the report, or NULL.
 */

struct analysis_report *analysis_template_get_report(struct analysis_template_block *instance, template_t template);


/**
 * Return the data associated with an analysis template.
 *
 * \param *template		Pointer to the template to return the data for.
 * \return			Pointer to the data, or NULL.
 */

void *analysis_template_get_data(struct analysis_report *template);


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

char *analysis_template_get_name(struct analysis_report *template, char *buffer, size_t length);


/**
 * Find a saved template ID based on its name.
 *
 * \param *file			The saved template instance to search in.
 * \param *name			The name to search for.
 * \return			The matching template ID, or NULL_TEMPLATE.
 */

template_t analysis_template_get_from_name(struct analysis_template_block *instance, char *name);


/**
 * Store a report's template into a saved templates instance.
 *
 * \param *instance		The saved template instance to save to.
 * \param *report		The report to take the template from.
 * \param template		The template pointer to save to, or
 *				NULL_TEMPLATE to add a new entry.
 * \param *name			Pointer to a name to give the template, or
 *				NULL to leave it as-is.
 */

void analysis_template_store(struct analysis_template_block *instance, struct analysis_report *report, template_t template, char *name);


/**
 * Rename a template in a saved templates instance.
 *
 * \param *instance		The saved templates instance containing the template.
 * \param template		The template to be renamed.
 * \param *name			Pointer to the new name.
 */

void analysis_template_rename(struct analysis_template_block *instance, template_t template, char *name);


/**
 * Delete a saved report from the file, and adjust any other template
 * pointers which are currently in use.
 *
 * \param *instance		The saved templates instance containing the template.
 * \param template		The template to be deleted.
 * \return			TRUE on deletion; FALSE on failure.
 */

osbool analysis_template_delete(struct analysis_template_block *instance, template_t template);


/**
 * Create a new analysis template in the static heap, using data from
 * a report's settings
 *
 * \param *parent		Pointer to the analysis template instance
 *				own the new template.
 * \param *name			Pointer to the name of the new template, or
 *				NULL to leave empty.
 * \param type			The type of template data to be copied.
 * \param *data			Pointer to the data to be copied into the
 *				new template.
 * \return			Pointer to the new template, or NULL on failure.
 */

struct analysis_report *analysis_template_create_new(struct analysis_template_block *parent, char *name, enum analysis_report_type type, void *data);


/**
 * Save the Report Template details from a saved templates instance to a
 * CashBook file
 *
 * \param *instance		The saved templates instance to write.
 * \param *out			The file handle to write to.
 */

void analysis_template_write_file(struct analysis_template_block *instance, FILE *out);


/**
 * Read Report Template details from a CashBook file into a saved templates
 * instance.
 *
 * \param *instance		The saved templates instance to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool analysis_template_read_file(struct analysis_template_block *instance, struct filing_block *in);


/**
 * Convert a textual comma-separated list of hex numbers into a numeric
 * account list array.
 *
 * \param *list			The textual hex number list to process.
 * \param *array		Pointer to memory to take the numeric list,
 *				with space for REPORT_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

int analysis_template_account_hex_to_list(char *list, acct_t *array);


/**
 * Convert a numeric account list array into a textual list of comma-separated
 * hex values.
 *
 * \param *list			Pointer to the buffer to take the textual list.
 * \param size			The size of the buffer.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_template_account_list_to_hex(char *list, size_t size, acct_t *array, int len);

#endif

