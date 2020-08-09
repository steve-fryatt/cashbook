/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: preset_dialogue.h
 *
 * Preset Edit dialogue interface.
 */

#ifndef CASHBOOK_PRESET_DIALOGUE
#define CASHBOOK_PRESET_DIALOGUE

#include "preset.h"

/**
 * The requested action from the dialogue.
 */

enum preset_dialogue_action {
	/**
	 * No action defined.
	 */
	PRESET_DIALOGUE_ACTION_NONE,

	/**
	 * Create or update the preset using the supplied details.
	 */
	PRESET_DIALOGUE_ACTION_OK,

	/**
	 * Delete the preset.
	 */
	PRESET_DIALOGUE_ACTION_DELETE
};

/**
 * The preset data held by the dialogue.
 */

struct preset_dialogue_data {
	/**
	 * The requested action from the dialogue.
	 */
	enum preset_dialogue_action	action;

	/**
	 * The preset being edited.
	 */
	preset_t			preset;

	/**
	 * The name of the preset.
	 */
	char				name[PRESET_NAME_LEN];

	/**
	 * The shortcut key used to insert the preset.
	 */
	char				action_key;

	/**
	 * The transaction flags for the preset (including the preset flags).
	 */
	enum transact_flags		flags;

	/**
	 * The target column for the caret.
	 */
	enum preset_caret		caret_target;

	/**
	 * The date to enter for the preset.
	 */
	date_t				date;

	/**
	 * The from account to enter for the preset.
	 */
	acct_t				from;

	/**
	 * The to account to enter for the preset.
	 */
	acct_t				to;

	/**
	 * The amount to enter for the preset.
	 */
	amt_t				amount;

	/**
	 * The reference to enter for the preset.
	 */
	char				reference[TRANSACT_REF_FIELD_LEN];

	/**
	 * The description to enter for the preset.
	 */
	char				description[TRANSACT_DESCRIPT_FIELD_LEN];
};

/**
 * Initialise the Preset Edit dialogue.
 */

void preset_dialogue_initialise(void);


/**
 * Open the Preset Edit dialogue for a given preset.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void preset_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*callback)(void *, struct preset_dialogue_data *), struct preset_dialogue_data *content);

#endif

