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
 * \file: import_dialogue.h
 *
 * Import Complete dialogue interface.
 */

#ifndef CASHBOOK_IMPORT_DIALOGUE
#define CASHBOOK_IMPORT_DIALOGUE

/**
 * The requested action from the dialogue.
 */

enum import_dialogue_action {
	/**
	 * No action defined.
	 */
	IMPORT_DIALOGUE_ACTION_NONE,

	/**
	 * Close the dialogue and delete the report
	 */
	IMPORT_DIALOGUE_ACTION_CLOSE,

	/**
	 * Close the dialogue and display the report.
	 */
	IMPORT_DIALOGUE_ACTION_VIEW_REPORT,
};

/**
 * The Import Complete data held by the dialogue.
 */

struct import_dialogue_data {
	/**
	 * The requested action from the dialogue.
	 */
	enum import_dialogue_action	action;

	/**
	 * The number of transactions imported.
	 */
	int				imported;

	/**
	 * The number of entries rejected.
	 */
	int				rejected;
};

/**
 * Initialise the Import Complete Edit dialogue.
 */

void import_dialogue_initialise(void);


/**
 * Open the Import Complete Edit dialogue for a given Import Complete window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void import_dialogue_open(wimp_pointer *ptr, struct file_block *file,
		osbool (*callback)(void *, struct import_dialogue_data *), struct import_dialogue_data *content);

#endif

