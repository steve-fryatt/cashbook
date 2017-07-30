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

/**
 * An analysis dialogue definition.
 */

struct analysis_dialogue_block;

/**
 * An analysis dialogue contents definition.
 */

struct analysis_dialogue_definition {
	/**
	 * The name of the window template to use for the dialogue.
	 */
	char		*template_name;

	/**
	 * The interactive help token prefix to use for the dialogue.
	 */
	char		*ihelp_token;

	/**
	 * The Generate button icon handle.
	 */
	wimp_i		generate_button;

	/**
	 * The Cancel button icon handle.
	 */
	wimp_i		cancel_button;

	/**
	 * The Delete button icon handle.
	 */
	wimp_i		delete_button;

	/**
	 * The Rename button icon handle.
	 */
	wimp_i		rename_button;
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
 * \param *templates		The analysis templates instance to use.
 * \param *ptr			The current Wimp Pointer details.
 * \param template		The report template to use for the dialogue.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_dialogue_open(struct analysis_dialogue_block *dialogue, struct analysis_template_block *templates, wimp_pointer *pointer, template_t template, osbool restore);


/**
 * Force an analysis dialogue instance to close if it is currently open
 * on screen.
 *
 * \param* dialogue		The dialogue instance to close.
 */

void analysis_dialogue_close(struct analysis_dialogue_block *dialogue);


#endif
