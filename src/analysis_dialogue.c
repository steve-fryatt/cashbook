/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: analysis_dialogue.c
 *
 * Analysis report dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>

/* SF-Lib header files. */

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
#include "analysis_dialogue.h"

#include "account.h"
#include "account_menu.h"
#include "analysis.h"
#include "analysis_template.h"
#include "analysis_template_save.h"
#include "dialogue.h"


/**
 * An analysis dialogue definition.
 */

struct analysis_dialogue_block {
	struct dialogue_block			*dialogue;		/**< The dialogue instance.					*/
	struct analysis_dialogue_definition	*definition;		/**< The dialogue definition from the client.			*/
	struct analysis_block			*parent;		/**< The parent analysis instance.				*/
	template_t				template;		/**< The template associated with the dialogue.			*/
	void					*dialogue_settings;	/**< The settings block associated with the dialogue.		*/
	void					*file_settings;		/**< The settings block associated with the file instance.	*/
};

/* Static Function Prototypes. */

static void analysis_dialogue_closing(struct file_block *file, wimp_w window, void *data);
static osbool analysis_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static osbool analysis_dialogue_delete(struct analysis_dialogue_block *dialogue);
static void analysis_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);

/**
 * Initialise a new analysis dialogue window instance.
 *
 * \param *definition		The dialogue definition from the client.
 * \return			Pointer to the dialogue structure, or NULL on failure.
 */

struct analysis_dialogue_block *analysis_dialogue_initialise(struct analysis_dialogue_definition *definition)
{
	struct analysis_dialogue_block	*new;

	if (definition == NULL)
		return NULL;

	/* Create the instance. */

	new = heap_alloc(sizeof(struct analysis_dialogue_block));
	if (new == NULL)
		return NULL;

	new->dialogue_settings = NULL;
	new->file_settings = NULL;
	new->definition = definition;
	new->parent = NULL;
	new->template = NULL_TEMPLATE;

	/* Claim a local template store to hold the live dialogue contents. */

	new->dialogue_settings = heap_alloc(definition->block_size);
	if (new->dialogue_settings == NULL) {
		heap_free(new);
		return NULL;
	}

	/* Create the dialogue window. */

	new->dialogue = dialogue_create(&(definition->dialogue));
	if (new->dialogue == NULL) {
		heap_free(new);
		return NULL;
	}

	/* Set the callback handlers for our client. */

	definition->dialogue.callback_fill = analysis_dialogue_fill;
	definition->dialogue.callback_process = analysis_dialogue_process;
	definition->dialogue.callback_close = analysis_dialogue_closing;

	return new;
}


/**
 * Open a new analysis dialogue.
 * 
 * \param *dialogue		The analysis dialogue instance to open.
 * \param *report		The report instance opening the dialogue.
 * \param *parent		The analysis instance to be the parent.
 * \param *ptr			The current Wimp Pointer details.
 * \param template		The report template to use for the dialogue.
 * \param *settings		The dialogue settings to use when no template available.
 *				These are assumed to belong to the file instance, and will
 *				be updated if the Generate button is clicked.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_dialogue_open(struct analysis_dialogue_block *dialogue, void *report, struct analysis_block *parent, wimp_pointer *pointer, template_t template, void *settings, osbool restore)
{
	struct analysis_report		*template_block;
	struct analysis_template_block	*templates;
	struct analysis_report_details	*report_details;

	if (dialogue == NULL || dialogue->definition == NULL || parent == NULL || pointer == NULL)
		return;

	templates = analysis_get_templates(parent);
	if (templates == NULL)
		return;

	report_details = analysis_get_report_details(dialogue->definition->type);
	if (report_details == NULL)
		return;

	/* Copy the settings block contents into a static place that won't shift about on the flex heap
	 * while the dialogue is open.
	 */

	template_block = analysis_template_get_report(templates, template);

	if (template_block != NULL) {
		report_details->copy_template(dialogue->dialogue_settings, analysis_template_get_data(template_block));
		dialogue->template = template;

		dialogue_set_title(dialogue->dialogue, "GenRepTitle",
				analysis_template_get_name(template_block, NULL, 0), NULL, NULL, NULL);

		dialogue_set_hidden_icons(dialogue->dialogue, DIALOGUE_ICON_ANALYSIS_DELETE | DIALOGUE_ICON_ANALYSIS_RENAME, FALSE);

		/* If we use a template, we always want to reset to the template! */

		restore = TRUE;
	} else {
		report_details->copy_template(dialogue->dialogue_settings, settings);
		dialogue->template = NULL_TEMPLATE;

		dialogue_set_title(dialogue->dialogue, dialogue->definition->title_token,
				analysis_template_get_name(template_block, NULL, 0), NULL, NULL, NULL);

		dialogue_set_hidden_icons(dialogue->dialogue, DIALOGUE_ICON_ANALYSIS_DELETE | DIALOGUE_ICON_ANALYSIS_RENAME, TRUE);
	}

	/* Set the pointers up so we can find this lot again and open the window. */

	dialogue->parent = parent;
	dialogue->file_settings = settings;

	/* Set the window contents up. */

	dialogue_open(dialogue->dialogue, restore, analysis_get_file(dialogue->parent), report, pointer, dialogue);
}


/**
 * Handle a dialogue being closed.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window		The window handle of the dialogue.
 * \param *data			The associated analysis dialogue instance.
 */

static void analysis_dialogue_closing(struct file_block *file, wimp_w window, void *data)
{
	struct analysis_dialogue_block *dialogue = data;

	if (dialogue == NULL || dialogue->template == NULL_TEMPLATE)
		return;

	dialogue_force_all_closed(NULL, dialogue);
}


/**
 * Update an analysis dialogue instance's template pointer if a template
 * is deleted from the parent analysis instance.
 *
 * \param *dialogue		The dialogue instance to process.
 * \param *parent		The analysis instance from which the template
 *				was deleted.
 * \param template		The template which was deleted.
 */

void analysis_dialogue_remove_template(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, template_t template)
{
	if (dialogue == NULL || dialogue->parent != parent)
		return;

	if (dialogue->template != NULL_TEMPLATE && dialogue->template > template)
		dialogue->template--;
}


/**
 * Tidy up after a template being renamed, by updating the window title
 * if the template belongs to this instance.
 *
 * \param *dialogue		The dialogue instance to check.
 * \param *parent		The parent analysis instance owning the
 *				renamed report.
 * \param template		The report being renamed.
 * \param *name			The new name for the report.
 */

void analysis_dialogue_rename_template(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, template_t template, char *name)
{
	if (dialogue == NULL || dialogue->parent != parent || dialogue->template != template)
		return;

	dialogue_set_title(dialogue->dialogue, "GenRepTitle", name, NULL, NULL, NULL);
}


/**
 * Process the contents of a dialogue and return it to the client.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window		The handle of the dialogue box to fill.
 * \param *pointer		The wimp pointer data, or NULL on an Enter keypress.
 * \param type			The dialogue icon type of the target icon.
 * \param *report		The parent analysis report instance.
 * \param *data			The associated analysis dialogue instance.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool analysis_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct analysis_dialogue_block *dialogue = data;
	struct analysis_report_details	*report_details;

	if (window == NULL || dialogue == NULL || dialogue->definition == NULL || dialogue->parent == NULL)
		return FALSE;

	if (type & DIALOGUE_ICON_OK) {
		report_details = analysis_get_report_details(dialogue->definition->type);
		if (report_details == NULL)
			return FALSE;

		/* Request the client to read the data from the dialogue. */

		if (report_details->read_window != NULL)
			report_details->read_window(dialogue->parent, window, dialogue->file_settings);

		/* Run the report itself */

		analysis_run_report(dialogue->parent, dialogue->definition->type, dialogue->file_settings, dialogue->template);

		return TRUE;
	} else if (type & DIALOGUE_ICON_ANALYSIS_DELETE) {
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_dialogue_delete(dialogue))
			return TRUE;
	} else if (type & DIALOGUE_ICON_ANALYSIS_RENAME) {
		if (pointer->buttons == wimp_CLICK_SELECT && dialogue->template != NULL_TEMPLATE)
			analysis_template_save_open_rename_window(dialogue->parent, dialogue, dialogue->template, pointer);
	}

	return FALSE;
}


/**
 * Delete the template associated with a dialogue.
 *
 * \param *dialogue		The dialogue instance to process.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool analysis_dialogue_delete(struct analysis_dialogue_block *dialogue)
{
	struct analysis_template_block	*templates;

	if (dialogue == NULL || dialogue->parent == NULL || dialogue->template == NULL_TEMPLATE)
		return FALSE;

	templates = analysis_get_templates(dialogue->parent);
	if (templates == NULL)
		return FALSE;

	if (error_msgs_report_question("DeleteTemp", "DeleteTempB") != 3)
		return FALSE;

	if (!analysis_template_delete(templates, dialogue->template))
		return FALSE;

	dialogue->template = NULL_TEMPLATE;

	return TRUE;
}


/**
 * Request the client to fill a dialogue, and update the shaded icons
 * based on the end result.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window		The handle of the dialogue box to fill.
 * \param restore		TRUE if the dialogue should restore from the template.
 * \param *data			The associated analysis dialogue instance.
 */

static void analysis_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct analysis_dialogue_block *dialogue = data;
	struct analysis_report_details	*report_details;

	if (window == NULL || dialogue == NULL || dialogue->definition == NULL || dialogue->parent == NULL)
		return;

	report_details = analysis_get_report_details(dialogue->definition->type);
	if (report_details == NULL)
		return;

	/* Request the client to fill the dialogue. */

	if (report_details->fill_window != NULL)
		report_details->fill_window(dialogue->parent, window, restore ? dialogue->dialogue_settings : NULL);
}

