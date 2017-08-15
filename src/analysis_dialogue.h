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
 * Window Icon Types.
 */

enum analysis_dialogue_icon_type {
	ANALYSIS_DIALOGUE_ICON_RADIO = 0x00000001,		/**< A radio icon.				*/
	ANALYSIS_DIALOGUE_ICON_SHADE_ON = 0x00000002,		/**< Shade icon when target is selected.	*/
	ANALYSIS_DIALOGUE_ICON_SHADE_OFF = 0x00000004,		/**< Shade icon when target is not selected.	*/
	ANALYSIS_DIALOGUE_ICON_SHADE_TARGET = 0x00000008,	/**< A target for shading other icons.		*/
	ANALYSIS_DIALOGUE_ICON_END = 0x80000000			/**< The last entry in the icon sequence.	*/
};

#define ANALYSIS_DIALOGUE_NO_ICON ((wimp_i) -1)

/**
 * Window icon definitions.
 */

struct analysis_dialogue_icon {
	enum analysis_dialogue_icon_type	type;
	wimp_i					icon;
	wimp_i					target;
};


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
	 * The name of the window template to use for the dialogue.
	 */
	char				*template_name;

	/**
	 * The interactive help token prefix to use for the dialogue.
	 */
	char				*ihelp_token;

	/**
	 * The Generate button icon handle.
	 */
	wimp_i				generate_button;

	/**
	 * The Cancel button icon handle.
	 */
	wimp_i				cancel_button;

	/**
	 * The Delete button icon handle.
	 */
	wimp_i				delete_button;

	/**
	 * The Rename button icon handle.
	 */
	wimp_i				rename_button;

	/**
	 * A list of significant icons in the dialogue.
	 */
	struct analysis_dialogue_icon	*icons;
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
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_dialogue_open(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, wimp_pointer *pointer, template_t template, void *settings, osbool restore);


/**
 * Force an analysis dialogue instance to close if it is currently open
 * on screen.
 *
 * \param* dialogue		The dialogue instance to close.
 */

void analysis_dialogue_close(struct analysis_dialogue_block *dialogue);


#endif
