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
 * \file: account_section_dialogue.h
 *
 * Account List Section Edit dialogue interface.
 */

#ifndef CASHBOOK_ACCOUNT_SECTION_DIALOGUE
#define CASHBOOK_ACCOUNT_SECTION_DIALOGUE

#include "account_list_window.h"

/**
 * The list section data held by the dialogue.
 */

struct account_section_dialogue_data {
	/**
	 * The line in the account list being edited by the Section Edit window.
	 */
	int			line;

	/**
	 * The name for the section.
	 */
	char			name[ACCOUNT_SECTION_LEN];

	/**
	 * The type for the section.
	 */
	enum account_line_type	type;
};

/**
 * Initialise the account section edit dialogue.
 */

void account_section_dialogue_initialise(void);


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void account_section_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*update_callback)(void *, struct account_section_dialogue_data *),
		osbool (*delete_callback)(struct account_list_window *, int), struct account_section_dialogue_data *content);

#endif

