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
 * \file: presets.h
 *
 * Transaction presets implementation.
 */

#ifndef CASHBOOK_PRESETS
#define CASHBOOK_PRESETS

#include "file.h"
#include "date.h"

/**
 *  A preset number.
 */

typedef int preset_t;

/**
 * A preset data block istance.
 */

struct preset_block;

/**
 * The NULL, or non-existant, preset.
 */

#define NULL_PRESET ((preset_t) (-1))

/**
 * The maximum length of a preset name.
 */

#define PRESET_NAME_LEN 32

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
 * Get a preset caret target field from an input file.
 */

#define preset_get_caret_field(in) ((enum preset_caret) filing_get_int_field((in)))

#include "account.h"
#include "currency.h"
#include "filing.h"


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
 * Find the preset which corresponds to a display line in a preset
 * window.
 *
 * \param *file			The file to use the transaction window in.
 * \param line			The display line to return the transaction for.
 * \return			The appropriate transaction, or NULL_TRANSACTION.
 */

preset_t preset_get_preset_from_line(struct file_block *file, int line);


/**
 * Find the number of presets in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of presets in the file.
 */

int preset_get_count(struct file_block *file);


/**
 * Return the file associated with a preset instance.
 *
 * \param *instance		The preset instance to query.
 * \return			The associated file, or NULL.
 */

struct file_block *preset_get_file(struct preset_block *instance);


/**
 * Test the validity of a preset index.
 *
 * \param *file			The file to test against.
 * \param preset		The preset index to test.
 * \return			TRUE if the index is valid; FALSE if not.
 */

osbool preset_test_index_valid(struct file_block *file, preset_t preset);


/**
 * Return the name for a preset.
 *
 * If a buffer is supplied, the name is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * name in the preset array is returned instead. In the latter case, this
 * pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the transaction.
 * \param preset		The preset to return the name of.
 * \param *buffer		Pointer to a buffer to take the name, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting name string,
 *				either the supplied buffer or the original.
 */

char *preset_get_name(struct file_block *file, preset_t preset, char *buffer, size_t length);


/**
 * Find the caret target for the given preset.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to check.
 * \return			The preset's caret target.
 */

enum preset_caret preset_get_caret_destination(struct file_block *file, preset_t preset);


/**
 * Find the action key for the given preset.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to check.
 * \return			The preset's action key.
 */

char preset_get_action_key(struct file_block *file, preset_t preset);


/**
 * Return the date for the given preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the date for.
 * \return			The preset's date, or NULL_DATE.
 */

date_t preset_get_date(struct file_block *file, preset_t preset);


/**
 * Return the from account of a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the from account for.
 * \return			The from account of the preset, or NULL_ACCOUNT.
 */

acct_t preset_get_from(struct file_block *file, preset_t preset);


/**
 * Return the to account of a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the to account for.
 * \return			The to account of the preset, or NULL_ACCOUNT.
 */

acct_t preset_get_to(struct file_block *file, preset_t preset);


/**
 * Return the preset flags for a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the flags for.
 * \return			The flags of the preset, or TRANS_FLAGS_NONE.
 */

enum transact_flags preset_get_flags(struct file_block *file, preset_t preset);


/**
 * Return the amount of a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the amount of.
 * \return			The amount of the preset, or NULL_CURRENCY.
 */

amt_t preset_get_amount(struct file_block *file, preset_t preset);


/**
 * Return the reference for a preset.
 *
 * If a buffer is supplied, the reference is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * reference in the preset array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the reference of.
 * \param *buffer		Pointer to a buffer to take the reference, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting reference string,
 *				either the supplied buffer or the original.
 */

char *preset_get_reference(struct file_block *file, preset_t preset, char *buffer, size_t length);


/**
 * Return the description for a preset.
 *
 * If a buffer is supplied, the description is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * description in the preset array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the description of.
 * \param *buffer		Pointer to a buffer to take the description, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting description string,
 *				either the supplied buffer or the original.
 */

char *preset_get_description(struct file_block *file, preset_t preset, char *buffer, size_t length);


/**
 * Open the Preset Edit dialogue for a given preset list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void preset_open_edit_window(struct file_block *file, int preset, wimp_pointer *ptr);


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

preset_t preset_find_from_keypress(struct file_block *file, char key);


/**
 * Save the standing order details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void preset_write_file(struct file_block *file, FILE *out);


/**
 * Read preset order details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool preset_read_file(struct file_block *file, struct filing_block *in);


/**
 * Check the presets in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool preset_check_account(struct file_block *file, acct_t account);

#endif

