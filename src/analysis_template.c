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
 * \file: analysis_template.c
 *
 * Analysis Teplate Storage implementation.
 */

/* ANSI C header files */

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "analysis_template.h"

#include "file.h"
#include "flexutils.h"


/**
 * Saved Report.
 */

struct analysis_report {
	struct file_block		*file;					/**< The file to which the template belongs.					*/
	char				name[ANALYSIS_SAVED_NAME_LEN];		/**< The name of the saved report template.					*/
	enum analysis_report_type	type;					/**< The type of the template.							*/
};


/**
 * The analysis template details relating to an analysis instance.
 */

struct analysis_template_block
{
	struct analysis_block		*parent;				/**< The parent analysis instance.						*/

	byte				*saved_reports;				/**< Pointer to an array of saved report templates and their data.		*/
	int				saved_report_count;			/**< The number of saved report templates.					*/
};


/**
 * The size of the largest report template block belonging to any of the
 * clients.
 */

static size_t			analysis_template_block_size = 0;

/**
 * The full size of the block required to hold an analysis template
 * block and the struct analysis_report header.
 */

static size_t			analysis_template_full_block_size = 0;



/**
 * Test whether a template number is safe to look up in the template data array.
 */

#define analysis_template_valid(instance, template) (((template) != NULL_TEMPLATE) && ((template) >= 0) && ((template) < ((instance)->saved_report_count)))

/**
 * Return the address of a template in the template data array.
 */

#define analysis_template_address(instance, template) (struct analysis_report *)((instance)->saved_reports + ((template) * analysis_template_full_block_size))

/**
 * Return the address of a template's data in the template data array.
 */

#define analysis_template_data_address(instance, template) (void *)((instance)->saved_reports + ((template) * analysis_template_full_block_size) + sizeof(struct analysis_report))


/**
 * Allow a analysis client to report the size of its template block.
 *
 * \param size			The size of the block.
 */

void analysis_template_set_block_size(size_t size)
{
	if (size > analysis_template_block_size)
		analysis_template_block_size = size;

	/* Total size is the size of the header struct, plus the size of the largest client data struct. */

	analysis_template_full_block_size = sizeof(struct analysis_report) + analysis_template_block_size;
}


/**
 * Construct a new analysis template storage instance
 *
 * \param *parent		The analysis instance which owns this new store.
 * \return			Pointer to the new instance, or NULL.
 */

struct analysis_template_block *analysis_template_create_instance(struct analysis_block *parent)
{
	struct analysis_template_block	*new;

	new = heap_alloc(sizeof(struct analysis_template_block));
	if (new == NULL)
		return NULL;

	new->parent = parent;

	new->saved_reports = NULL;
	new->saved_report_count = 0;

	/* Initialise the saved report data. */

	if (!flexutils_initialise((void **) &(new->saved_reports))) {
		analysis_template_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete an analysis template storage instance.
 *
 * \param *instance		The instance to be deleted.
 */

void analysis_template_delete_instance(struct analysis_template_block *instance)
{
	if (instance == NULL)
		return;

	/* Free any saved report data. */

	if (instance->saved_reports != NULL)
		flexutils_free((void **) &(instance->saved_reports));
}


/**
 * Remove any references to a given account from all of the saved analysis
 * templates in an instance.
 * 
 * \param *instance		The instance to be updated.
 * \param account		The account to be removed.
 */

void analysis_template_remove_account(struct analysis_template_block *instance, acct_t account)
{
	template_t template;

	if (instance == NULL)
		return;

//	for (template = 0; template < file->analysis->saved_report_count; template++) {
//		analysis_remove_account_from_template(&(file->analysis->saved_reports[template]), account);
//	}
}


/**
 * Return the type of template which is stored at a given index.
 * 
 * \param *instance		The save report instance to query.
 * \param template		The template to query.
 * \return			The template type, or REPORT_TYPE_NONE.
 */

enum analysis_report_type analysis_template_type(struct analysis_template_block *instance, template_t template)
{
	if (instance == NULL || !analysis_template_valid(instance, template))
		return REPORT_TYPE_NONE;

//	return instance->saved_reports[template].type;
}


/**
 * Return the number of templates in the given instance.
 * 
 * \param *file			The instance to report on.
 * \return			The number of templates, or 0 on error.
 */
 
int analysis_template_get_count(struct analysis_template_block *instance)
{
	if (instance == NULL)
		return 0;

	return instance->saved_report_count;
}


/**
 * Return a volatile pointer to a template block from within an instance's
 * saved templates. This is a pointer into a flex heap block, so it will
 * only be valid until an operation occurs to shift the blocks.
 * 
 * \param *instance		The instance containing the template of interest.
 * \param template		The number of the requied template.
 * \return			Volatile pointer to the template, or NULL.
 */

struct analysis_report *analysis_template_get_report(struct analysis_template_block *instance, template_t template)
{
	if (instance == NULL || !analysis_template_valid(instance, template))
		return NULL;

//	return instance->saved_reports + template;
}


/**
 * Find a saved template ID based on its name.
 *
 * \param *file			The saved template instance to search in.
 * \param *name			The name to search for.
 * \return			The matching template ID, or NULL_TEMPLATE.
 */

template_t analysis_template_get_from_name(struct analysis_template_block *instance, char *name)
{
	template_t	i, found = NULL_TEMPLATE;

	if (instance == NULL || instance->saved_report_count <= 0)
		return NULL_TEMPLATE;

//	for (i = 0; i < instance->saved_report_count && found == NULL_TEMPLATE; i++) {
//		if (string_nocase_strcmp(instance->saved_reports[i].name, name) == 0) {
//			found = i;
//			break;
//		}
//	}

	return found;
}


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

void analysis_template_store(struct analysis_template_block *instance, struct analysis_report *report, template_t template, char *name)
{
	if (instance == NULL || instance->saved_reports == NULL || report == NULL)
		return;

	if (template == NULL_TEMPLATE) {
		if (!flexutils_resize((void **) &(instance->saved_reports), sizeof(struct analysis_report), instance->saved_report_count + 1)) {
			error_msgs_report_error("NoMemNewTemp");
			return;
		}

		template = instance->saved_report_count++;
	}

	if (!analysis_template_valid(instance, template))
		return;

	if (name != NULL) {
		strncpy(report->name, name, ANALYSIS_SAVED_NAME_LEN);
		report->name[ANALYSIS_SAVED_NAME_LEN - 1] = '\0';
	}

//	analysis_copy_template(instance->saved_reports + template, report);
//	file_set_data_integrity(file, TRUE);
}


/**
 * Rename a template in a saved templates instance.
 *
 * \param *instance		The saved templates instance containing the template.
 * \param template		The template to be renamed.
 * \param *name			Pointer to the new name.
 */

void analysis_template_rename(struct analysis_template_block *instance, template_t template, char *name)
{
	if (instance == NULL || instance->saved_reports == NULL || !analysis_template_valid(instance, template))
		return;

	/* Copy the new name across into the template. */

//	strncpy(instance->saved_reports[template].name, name, ANALYSIS_SAVED_NAME_LEN);
//	instance->saved_reports[template].name[ANALYSIS_SAVED_NAME_LEN - 1] = '\0';

	/* Mark the file has being modified. */

//	file_set_data_integrity(file, TRUE);
}







/**
 * Save the Report Template details from a saved templates instance to a
 * CashBook file
 *
 * \param *file			The file to which the instance belongs.
 * \param *instance		The saved templates instance to write.
 * \param *out			The file handle to write to.
 * \param *reports		An array of report type definitions.
 * \param count			The number of report type definitions.
 */

void analysis_template_write_file(struct file_block *file, struct analysis_template_block *instance, FILE *out, struct analysis_report_details *reports[], size_t count)
{
	int			i;
	struct analysis_report	*template = NULL;
	void			*data = NULL;


	if (file == NULL || instance == NULL || reports == NULL)
		return FALSE;

	/* Write the section header. */

	fprintf(out, "\n[Reports]\n");

	fprintf(out, "Entries: %x\n", instance->saved_report_count);

	for (i = 0; i < instance->saved_report_count; i++) {
		template = analysis_template_address(instance, i);
		data = analysis_template_data_address(instance, i);

		if (template->type > 0 && template->type < count && reports[template->type] != NULL && reports[template->type]->write_file_template != NULL)
			reports[template->type]->write_file_template(file, data, out, template->name);
	}
}


/**
 * Read Report Template details from a CashBook file into a saved templates
 * instance.
 *
 * \param *file			The file to which the instance belongs.
 * \param *instance		The saved templates instance to read in to.
 * \param *in			The filing handle to read in from.
 * \param *reports		An array of report type definitions.
 * \param count			The number of report type definitions.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool analysis_template_read_file(struct file_block *file, struct analysis_template_block *instance, struct filing_block *in, struct analysis_report_details *reports[], size_t count)
{
	size_t			block_size;
	struct analysis_report	*template = NULL;
	void			*data = NULL;


	if (file == NULL || instance == NULL || reports == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Analysis Reports.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(instance->saved_reports), analysis_template_full_block_size, &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > instance->saved_report_count) {
#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = instance->saved_report_count;
			}
		} else if (filing_test_token(in, "@")) {
			instance->saved_report_count++;
			if (instance->saved_report_count > block_size) {
				block_size = instance->saved_report_count;
#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			}
			template = analysis_template_address(instance, instance->saved_report_count - 1);
			data = analysis_template_data_address(instance, instance->saved_report_count - 1);
#ifdef DEBUG
			debug_printf("Starting to store template %d at 0x%x with data at 0x%x", instance->saved_report_count - 1, template, data);
#endif
			template->file = file;
			template->type = analysis_get_report_type_field(in);
			template->name[0] = '\0';

			if (template->type > 0 && template->type < count && reports[template->type] != NULL && reports[template->type]->process_file_token != NULL)
				reports[template->type]->process_file_token(file, data, in);
			else
				filing_set_status(in, FILING_STATUS_UNEXPECTED);
		} else if (template != NULL && filing_test_token(in, "Name")) {
			filing_get_text_value(in, template->name, ANALYSIS_SAVED_NAME_LEN);
		} else if (template != NULL && template->type > 0 && template->type < count && reports[template->type] != NULL &&
				reports[template->type]->process_file_token != NULL) {
			reports[template->type]->process_file_token(file, data, in);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(instance->saved_report_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
}

