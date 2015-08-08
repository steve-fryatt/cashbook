/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: presets.h
 *
 * Transaction presets implementation.
 */

#ifndef CASHBOOK_PRESETS
#define CASHBOOK_PRESETS

#include "currency.h"
#include "filing.h"

/* A preset number. */

typedef int preset_t;

struct preset_block;

#define NULL_PRESET ((preset_t) (-1))

/* Caret end locations */

enum preset_caret {
	PRESET_CARET_DATE = 0,
	PRESET_CARET_FROM = 1,
	PRESET_CARET_TO = 2,
	PRESET_CARET_REFERENCE = 3,
	PRESET_CARET_AMOUNT = 4,
	PRESET_CARET_DESCRIPTION = 5
};

/**
 * Initialise the preset system.
 *
 * \param *sprites		The application sprite area.
 */

void preset_initialise(osspriteop_area *sprites);


/**
 * Create a new Preset window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct preset_block *preset_create_instance(struct file_block *file);


/**
 * Delete a preset instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void preset_delete_instance(struct preset_block *windat);


/**
 * Create and open a Preset List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void preset_open_window(struct file_block *file);


/**
 * Recreate the title of the Preset List window connected to the given file.
 *
 * \param *file			The file to rebuild the title for.
 */

void preset_build_window_title(struct file_block *file);


/**
 * Force the complete redraw of the Preset list window.
 *
 * \param *file			The file owning the window to redraw.
 */

void preset_redraw_all(struct file_block *file);


/**
 * Open the Preset Edit dialogue for a given preset list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void preset_open_edit_window(struct file_block *file, int preset, wimp_pointer *ptr);


/**
 * Build a Preset Complete menu and return the pointer.
 *
 * \param *file			The file to build the menu for.
 * \return			The created menu, or NULL for an error.
 */

wimp_menu *preset_complete_menu_build(struct file_block *file);


/**
 * Destroy any Preset Complete menu which is currently open.
 */

void preset_complete_menu_destroy(void);


/**
 * Decode a selection from the Preset Complete menu, converting to a preset
 * index number.
 *
 * \param *selection		The Wimp Menu Selection to decode.
 * \return			The preset index, or NULL_PRESET.
 */

int preset_complete_menu_decode(wimp_selection *selection);


/**
 * Sort the presets in a given instance based on that instance's sort setting.
 *
 * \param *windat		The preset window instance to sort.
 */

void preset_sort(struct preset_block *windat);


/**
 * Find a preset index based on its shortcut key.  If the key is '\0', or no
 * match is found, NULL_PRESET is returned.
 *
 * \param *file			The file to search in.
 * \param key			The shortcut key to search for.
 * \return			The matching preset index, or NULL_PRESET.
 */

int preset_find_from_keypress(struct file_block *file, char key);


/**
 * Find the caret target for the given preset.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to check.
 * \return			The preset's caret target.
 */

enum preset_caret preset_get_caret_destination(struct file_block *file, int preset);


/**
 * Find the number of presets in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of presets in the file.
 */

int preset_get_count(struct file_block *file);


/**
 * Apply a preset to fields of a transaction.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to apply.
 * \param *date			The date field to be updated.
 * \param *from			The from field to be updated.
 * \param *to			The to field to be updated.
 * \param *flags		The flags field to be updated.
 * \param *amount		The amount field to be updated.
 * \param *reference		The reference field to be updated.
 * \param *description		The description field to be updated.
 * \return			Bitfield indicating which fields have changed.
 */

unsigned preset_apply(struct file_block *file, int preset, date_t *date, acct_t *from, acct_t *to, unsigned *flags, amt_t *amount, char *reference, char *description);


/**
 * Save the standing order details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void preset_write_file(struct file_block *file, FILE *out);


/**
 * Read preset details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

enum config_read_status preset_read_file(struct file_block *file, FILE *in, char *section, char *token, char *value, osbool *unknown_data);


/**
 * Check the presets in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool preset_check_account(struct file_block *file, int account);

#endif

