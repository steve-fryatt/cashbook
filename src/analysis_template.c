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




/**
 * The analysis template details relating to an analysis instance.
 */

struct analysis_template_block
{
	struct analysis_block		*parent;				/**< The parent analysis instance.						*/

	struct analysis_report		*saved_reports;				/**< Pointer to an array of saved report templates.				*/
	int				saved_report_count;			/**< The number of saved report templates.					*/
};


/**
 * Test whether a template number is safe to look up in the template data array.
 */

#define analysis_template_valid(instance, template) (((template) != NULL_TEMPLATE) && ((template) >= 0) && ((template) < ((instance)->saved_report_count)))


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

	for (template = 0; template < file->analysis->saved_report_count; template++) {
		analysis_remove_account_from_template(&(file->analysis->saved_reports[template]), account);
	}
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

	return instance->saved_reports[template].type
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

	return instance->saved_reports + template;
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

	for (i = 0; i < instance->saved_report_count && found == NULL_TEMPLATE; i++) {
		if (string_nocase_strcmp(instance->saved_reports[i].name, name) == 0) {
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

	analysis_copy_template(instance->saved_reports + template, report);
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
	if (instance == NULL || instance->saved_reports == NULL || !analysis_template_valid(instance, template))
		return;

	/* Copy the new name across into the template. */

	strncpy(instance->saved_reports[template].name, name, ANALYSIS_SAVED_NAME_LEN);
	instance->saved_reports[template].name[ANALYSIS_SAVED_NAME_LEN - 1] = '\0';

	/* Mark the file has being modified. */

	file_set_data_integrity(file, TRUE);
}
