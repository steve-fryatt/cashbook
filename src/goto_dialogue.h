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
 * \file: goto_dialogue.h
 *
 * High-level Goto dialogue interface.
 */

#ifndef CASHBOOK_GOTO_DIALOGUE
#define CASHBOOK_GOTO_DIALOGUE

#include "date.h"

/**
 * The type of Go To target held by the dialogue.
 */

enum goto_dialogue_type {
	GOTO_DIALOGUE_TYPE_LINE = 0,					/**< The goto target is given as a line number.			*/
	GOTO_DIALOGUE_TYPE_DATE = 1					/**< The goto target is given as a date.			*/
};

/**
 * The Go To target held by the dialogue.
 */

union goto_dialogue_target {
	int			line;					/**< The transaction number if the target is a line.		*/
	date_t			date;					/**< The transaction date if the target is a date.		*/
};

/**
 * The Go To data held by the dialogue.
 */

struct goto_dialogue_data {
	enum goto_dialogue_type		type;				/**< The type of target held by the dialogue.			*/
	union goto_dialogue_target	target;				/**< The target held by the dialogue.				*/
};

/**
 * Initialise the goto dialogue.
 */

void goto_dialogue_initialise(void);

/**
 * Open the Goto dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current dialogue content, otherwise FALSE
 * \param *owner		The goto dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void goto_dialogue_open(wimp_pointer *ptr, osbool restore, void *owner, struct file_block *file, osbool (*callback)(void *, struct goto_dialogue_data *),
		struct goto_dialogue_data *data);

#endif

