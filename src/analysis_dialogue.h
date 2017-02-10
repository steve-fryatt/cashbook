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

/**
 * An analysis dialogue definition.
 */

struct analysis_dialogue_block;

struct analysis_dialogue_callback {
	/**
	 * Copy a report template from one location to another.
	 */

	void (*copy_template)(struct analysis_report *from, struct analysis_report *to);
	
};

/**
 * Initialise a new analysis dialogue window.
 *
 * \param *template		The name of the template to use for the dialogue.
 * \param *ihelp		The interactive help token to use for the dialogue.
 * \return			Pointer to the dialogue structure, or NULL on failure.
 */

struct analysis_dialogue_block *analysis_dialogue_initialise(char *template, char *ihelp);

#endif