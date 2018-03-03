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
 * \file: analysis_dialogue.h
 *
 * Analysis report dialogue interface.
 */

#ifndef CASHBOOK_ANALYSIS_DIALOGUE
#define CASHBOOK_ANALYSIS_DIALOGUE

#include "analysis.h"
#include "dialogue.h"

/**
 * An analysis dialogue definition.
 */

struct analysis_dialogue_block;

/**
 * An analysis dialogue contents definition.
 */

struct analysis_dialogue_definition {
	/**
	 * The type of report to which the dialogue relates.
	 */
	enum analysis_report_type	type;

	/**
	 * The size of the saved report template block.
	 */
	size_t				block_size;

	/**
	 * The definition block for the dialogue box.
	 */
	struct dialogue_definition	dialogue;
};


/**
 * Initialise a new analysis dialogue window instance.
 *
 * \param *definition		The dialogue definition from the client.
 * \return			Pointer to the dialogue structure, or NULL on failure.
 */

struct analysis_dialogue_block *analysis_dialogue_initialise(struct analysis_dialogue_definition *definition);


/**
 * Open a new analysis dialogue.
 * 
 * \param *dialogue		The analysis dialogue instance to open.
 * \param *parent		The analysis instance to be the parent.
 * \param *ptr			The current Wimp Pointer details.
 * \param template		The report template to use for the dialogue.
 * \param *settings		The dialogue settings to use when no template available.
 *				These are assumed to belong to the file instance, and will
 *				be updated if the Generate button is clicked.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_dialogue_open(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, wimp_pointer *pointer, template_t template, void *settings, osbool restore);


/**
 * Force an analysis dialogue instance to close if it is currently open
 * on screen.
 *
 * \param *dialogue		The dialogue instance to close.
 * \param *parent		If not NULL, only close the dialogue if
 *				this is the parent analysis instance.
 */

void analysis_dialogue_close(struct analysis_dialogue_block *dialogue, struct analysis_block *parent);


/**
 * Update an analysis dialogue instance's template pointer if a template
 * is deleted from the parent analysis instance.
 *
 * \param *dialogue		The dialogue instance to process.
 * \param *parent		The analysis instance from which the template
 *				was deleted.
 * \param template		The template which was deleted.
 */

void analysis_dialogue_remove_template(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, template_t template);


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

void analysis_dialogue_rename_template(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, template_t template, char *name);

#endif

