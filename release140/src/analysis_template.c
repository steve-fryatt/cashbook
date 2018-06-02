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

#include "analysis_template_save.h"
#include "file.h"
#include "flexutils.h"


/**
 * The size of a buffer used to convert integers into hex strings.
 */

#define ANALYSIS_TEMPLATE_HEX_BUFFER_LEN 32

/**
 * Saved Report.
 */

struct analysis_report {
	struct analysis_template_block	*instance;				/**< The template store instance to which the template belongs.			*/
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

/* Function Prototypes. */

static void analysis_template_copy(struct analysis_report *to, struct analysis_report *from);

/* Macros. */

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

#define analysis_template_data_address(instance, template) ((void *)((instance)->saved_reports + ((template) * analysis_template_full_block_size) + sizeof(struct analysis_report)))

/**
 * Given the top of a template block, return a pointer to the data.
 */

#define analysis_template_data_from_address(template) ((void*)((byte *)(template)) + sizeof(struct analysis_report))

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
 * Return the analysis template storage instance owning a report.
 *
 * \param *report		The report of interest.
 * \return			The owning instance, or NULL.
 */ 

struct analysis_template_block *analysis_template_get_instance(struct analysis_report *report)
{
	if (report == NULL)
		return NULL;

	return report->instance;
}


/**
 * Return the cashbook file owning a template storage instance.
 *
 * \param *instance		The instance of interest.
 * \return			The owning file, or NULL.
 */

struct file_block *analysis_template_get_file(struct analysis_template_block *instance)
{
	if (instance == NULL || instance->parent == NULL)
		return NULL;

	return analysis_get_file(instance->parent);
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
	template_t			i;
	struct analysis_report		*template = NULL;

	if (instance == NULL)
		return;

	for (i = 0; i < instance->saved_report_count; i++) {
		template = analysis_template_address(instance, i);
		analysis_template_remove_account_from_template(template, account);
	}
}


/**
 * Remove any references to a given account from an analysis template.
 *
 * \param *report		The report to process.
 * \param account		The account to be removed.
 */

void analysis_template_remove_account_from_template(struct analysis_report *report, acct_t account)
{
	void				*data;
	struct analysis_report_details	*report_details;

	if (report == NULL)
		return;

	report_details = analysis_get_report_details(report->type);
	if (report_details == NULL || report_details->remove_account == NULL)
		return;

	data = analysis_template_data_from_address(report);

	report_details->remove_account(data, account);
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

int analysis_template_remove_account_from_list(acct_t account, acct_t *array, int *count)
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
 * Return the type of template which is stored at a given index.
 * 
 * \param *instance		The save report instance to query.
 * \param template		The template to query.
 * \return			The template type, or REPORT_TYPE_NONE.
 */

enum analysis_report_type analysis_template_type(struct analysis_template_block *instance, template_t template)
{
	struct analysis_report *address;

	if (instance == NULL || !analysis_template_valid(instance, template))
		return REPORT_TYPE_NONE;

	address = analysis_template_address(instance, template);
	if (address == NULL)
		return REPORT_TYPE_NONE;

	return address->type;
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
 * Return a volatile pointer to a report block from within an instance's
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

	return analysis_template_address(instance, template);
}


/**
 * Return the data associated with an analysis template.
 *
 * \param *template		Pointer to the template to return the data for.
 * \return			Pointer to the data, or NULL.
 */

void *analysis_template_get_data(struct analysis_report *template)
{
	if (template == NULL)
		return NULL;

	return analysis_template_data_from_address(template);
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

char *analysis_template_get_name(struct analysis_report *template, char *buffer, size_t length)
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

	string_copy(buffer, template->name, length);

	return buffer;
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
	template_t		i, found = NULL_TEMPLATE;
	struct analysis_report	*template;

	if (instance == NULL || instance->saved_report_count <= 0)
		return NULL_TEMPLATE;

	for (i = 0; i < instance->saved_report_count && found == NULL_TEMPLATE; i++) {
		template = analysis_template_address(instance, i);
		if (template == NULL)
			continue;

		if (string_nocase_strcmp(template->name, name) == 0) {
			found = i;
			break;
		}
	}

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
	struct file_block	*file;
	struct analysis_report	*destination;

	if (instance == NULL || instance->saved_reports == NULL || report == NULL)
		return;

	if (template == NULL_TEMPLATE) {
		if (!flexutils_resize((void **) &(instance->saved_reports), analysis_template_full_block_size, instance->saved_report_count + 1)) {
			error_msgs_report_error("NoMemNewTemp");
			return;
		}

		template = instance->saved_report_count++;
	}

	if (!analysis_template_valid(instance, template))
		return;

	if (name != NULL)
		string_copy(report->name, name, ANALYSIS_SAVED_NAME_LEN);

	destination = analysis_template_address(instance, template);
	analysis_template_copy(destination, report);

	/* Mark the file has being modified. */

	file = analysis_get_file(instance->parent);
	if (file != NULL)
		file_set_data_integrity(file, TRUE);
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
	struct file_block		*file;
	struct analysis_report		*block;
	struct analysis_report_details	*report_details;

	if (instance == NULL || instance->saved_reports == NULL || !analysis_template_valid(instance, template))
		return;

	/* Copy the new name across into the template. */

	block = analysis_template_address(instance, template);
	if (block == NULL)
		return;

	string_copy(block->name, name, ANALYSIS_SAVED_NAME_LEN);

	/* Inform the owning report. */

	report_details = analysis_get_report_details(block->type);
	if (report_details != NULL && report_details->rename_template != NULL)
		report_details->rename_template(instance->parent, template, name);

	/* Mark the file has being modified. */

	file = analysis_get_file(instance->parent);
	if (file != NULL)
		file_set_data_integrity(file, TRUE);
}


/**
 * Copy a Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_template_copy(struct analysis_report *to, struct analysis_report *from)
{
	struct analysis_report_details	*report_details;

	if (from == NULL || to == NULL)
		return;

	report_details = analysis_get_report_details(from->type);
	if (report_details == NULL || report_details->copy_template == NULL)
		return;

#ifdef DEBUG
	debug_printf("Copy template from 0x%x to 0x%x, using copy function 0x%x", from, to, report_details->copy_template);
#endif

	string_copy(to->name, from->name, ANALYSIS_SAVED_NAME_LEN);

	to->type = from->type;
	to->instance = from->instance;

	report_details->copy_template(analysis_template_data_from_address(to), analysis_template_data_from_address(from));
}


/**
 * Delete a saved report from the file, and adjust any other template
 * pointers which are currently in use.
 *
 * \param *instance		The saved templates instance containing the template.
 * \param template		The template to be deleted.
 * \return			TRUE on deletion; FALSE on failure.
 */

osbool analysis_template_delete(struct analysis_template_block *instance, template_t template)
{
	enum analysis_report_type	type;
	struct file_block		*file;
	struct analysis_report		*block;
	struct analysis_report_details	*report_details;

	if (instance == NULL || instance->saved_reports == NULL || !analysis_template_valid(instance, template))
		return FALSE;

	block = analysis_template_address(instance, template);
	if (block == NULL)
		return FALSE;

	type = block->type;

	/* First remove the template from the flex block. */

	if (!flexutils_delete_object((void **) &(instance->saved_reports), analysis_template_full_block_size, template)) {
		error_msgs_report_error("BadDelete");
		return FALSE;
	}

	instance->saved_report_count--;

	/* Mark the file has being modified. */

	file = analysis_get_file(instance->parent);
	if (file != NULL)
		file_set_data_integrity(file, TRUE);

	/* If the rename template window is open for this template, close it now before the pointer is lost. */

	analysis_template_save_force_rename_close(instance->parent, template);

	/* Update any affected report dialogue. */

	report_details = analysis_get_report_details(type);
	if (report_details != NULL && report_details->remove_template != NULL)
		report_details->remove_template(instance->parent, template);

	/* Notify the save/rename dialogue. */

	analysis_template_save_delete_template(instance->parent, template);

	return TRUE;
}


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

struct analysis_report *analysis_template_create_new(struct analysis_template_block *parent, char *name, enum analysis_report_type type, void *data)
{
	struct analysis_report		*new;
	struct analysis_report_details	*report_details;

	if (parent == NULL || data == NULL)
		return NULL;

	report_details = analysis_get_report_details(type);
	if (report_details == NULL || report_details->copy_template == NULL)
		return NULL;

	new = heap_alloc(analysis_template_full_block_size);
	if (new == NULL)
		return NULL;

	new->instance = parent;
	new->type = type;
	if (name != NULL)
		string_copy(new->name, name, ANALYSIS_SAVED_NAME_LEN);
	else
		new->name[0] = '\0';

	report_details->copy_template(analysis_template_data_from_address(new), data);

	return new;
}


/**
 * Save the Report Template details from a saved templates instance to a
 * CashBook file
 *
 * \param *instance		The saved templates instance to write.
 * \param *out			The file handle to write to.
 */

void analysis_template_write_file(struct analysis_template_block *instance, FILE *out)
{
	template_t			i;
	struct analysis_report		*template = NULL;
	struct analysis_report_details	*report_details;
	void				*data = NULL;


	if (instance == NULL)
		return;

	/* Write the section header. */

	fprintf(out, "\n[Reports]\n");

	fprintf(out, "Entries: %x\n", instance->saved_report_count);

	for (i = 0; i < instance->saved_report_count; i++) {
		template = analysis_template_address(instance, i);
		data = analysis_template_data_address(instance, i);

		if (template == NULL)
			continue;

		report_details = analysis_get_report_details(template->type);
		if (report_details != NULL && report_details->write_file_template != NULL)
			report_details->write_file_template(data, out, template->name);
	}
}


/**
 * Read Report Template details from a CashBook file into a saved templates
 * instance.
 *
 * \param *instance		The saved templates instance to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool analysis_template_read_file(struct analysis_template_block *instance, struct filing_block *in)
{
	size_t				block_size;
	struct analysis_report		*template = NULL;
	struct analysis_report_details	*report_details = NULL;
	void				*data = NULL;


	if (instance == NULL)
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
			template->instance = instance;
			template->type = analysis_get_report_type_field(in);
			template->name[0] = '\0';

			report_details = analysis_get_report_details(template->type);

			if (report_details != NULL && report_details->process_file_token != NULL)
				report_details->process_file_token(data, in);
			else
				filing_set_status(in, FILING_STATUS_UNEXPECTED);
		} else if (template != NULL && filing_test_token(in, "Name")) {
			filing_get_text_value(in, template->name, ANALYSIS_SAVED_NAME_LEN);
		} else if (template != NULL && data != NULL && report_details != NULL && report_details->process_file_token != NULL) {
			report_details->process_file_token(data, in);
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


/**
 * Convert a textual comma-separated list of hex numbers into a numeric
 * account list array.
 *
 * \param *list			The textual hex number list to process.
 * \param *array		Pointer to memory to take the numeric list,
 *				with space for ANALYSIS_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

int analysis_template_account_hex_to_list(char *list, acct_t *array)
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
 * \param *list			Pointer to the buffer to take the textual list.
 * \param size			The size of the buffer.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_template_account_list_to_hex(char *list, size_t size, acct_t *array, int len)
{
	char	buffer[ANALYSIS_TEMPLATE_HEX_BUFFER_LEN];
	int	i;

	*list = '\0';

	for (i = 0; i < len; i++) {
		string_printf(buffer, ANALYSIS_TEMPLATE_HEX_BUFFER_LEN, "%x", array[i]);

		if (strlen(list) > 0 && strlen(list) + 1 < size)
			strcat(list, ",");

		if (strlen(list) + strlen(buffer) < size)
			strcat(list, buffer);
	}
}

