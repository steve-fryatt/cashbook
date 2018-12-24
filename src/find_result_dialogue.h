/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: find_result_dialogue.h
 *
 * High-level find results dialogue interface.
 */

#ifndef CASHBOOK_FIND_RESULT_DIALOGUE
#define CASHBOOK_FIND_RESULT_DIALOGUE

#include "find.h"

#include "date.h"
#include "oslib/types.h"

/**
 * The find result data held by the dialogue.
 */

struct find_result_dialogue_data {
	enum find_direction	direction;
};

/**
 * Initialise the find result dialogue.
 */

void find_result_dialogue_initialise(void);

/**
 * Open the find result dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The find dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void find_result_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(void *, struct find_result_dialogue_data *),
		struct find_result_dialogue_data *data);

#endif

