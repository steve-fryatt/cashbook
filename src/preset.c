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
 * \file: presets.c
 *
 * preset presets implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/msgs.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "preset.h"
#include "account.h"
#include "account_menu.h"
#include "currency.h"
#include "date.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "preset_dialogue.h"
#include "preset_list_window.h"







/**
 * Preset Entry data structure -- implementation.
 */

struct preset {
	char			name[PRESET_NAME_LEN];					/**< The name of the preset. */
	char			action_key;						/**< The key used to insert it. */

	enum transact_flags	flags;							/**< Preset flags (containing preset flags, preset flags, etc). */

	enum preset_caret	caret_target;						/**< The target icon for the caret. */

	date_t			date;							/**< Preset details. */
	acct_t			from;
	acct_t			to;
	amt_t			amount;
	char			reference[TRANSACT_REF_FIELD_LEN];
	char			description[TRANSACT_DESCRIPT_FIELD_LEN];
};

/**
 * Preset Instance data structure -- implementation.
 */

struct preset_block {
	/**
	 * The file to which the instance belongs.
	 */
	struct file_block		*file;

	/**
	 * The Preset List window instance.
	 */
	struct preset_list_window	*preset_window;

	/**
	 * The preset data for the defined presets.
	 */
	struct preset			*presets;

	/**
	 * The number of presets defined in the file.
	 */
	int				preset_count;
};

/* Static function prototypes. */

static osbool		preset_process_edit_window(void *parent, struct preset_dialogue_data *content);

static int		preset_add(struct file_block *file);
static osbool		preset_delete(struct file_block *file, int preset);


/**
 * Test whether a preset number is safe to look up in the preset data array.
 */

#define preset_valid(windat, preset) (((preset) != NULL_PRESET) && ((preset) >= 0) && ((preset) < ((windat)->preset_count)))


/**
 * Initialise the preset system.
 *
 * \param *sprites		The application sprite area.
 */

void preset_initialise(osspriteop_area *sprites)
{
	preset_list_window_initialise(sprites);
	preset_dialogue_initialise();
}


/**
 * Create a new Preset window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct preset_block *preset_create_instance(struct file_block *file)
{
	struct preset_block	*new;

	new = heap_alloc(sizeof(struct preset_block));
	if (new == NULL)
		return NULL;

	/* Initialise the preset block. */

	new->file = file;

	new->presets = NULL;
	new->preset_count = 0;

	/* Initialise the preset window. */

	new->preset_window = preset_list_window_create_instance(new);
	if (new->preset_window == NULL) {
		preset_delete_instance(new);
		return NULL;
	}

	/* Set up the preset data structures. */

	if (!flexutils_initialise((void **) &(new->presets))) {
		preset_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete a preset instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void preset_delete_instance(struct preset_block *windat)
{
	if (windat == NULL)
		return;

	preset_list_window_delete_instance(windat->preset_window);

	if (windat->presets != NULL)
		flexutils_free((void **) &(windat->presets));

	heap_free(windat);
}


/**
 * Create and open a Preset List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void preset_open_window(struct file_block *file)
{
	if (file == NULL || file->presets == NULL)
		return;

	preset_list_window_open(file->presets->preset_window);
}


/**
 * Find the preset which corresponds to a display line in a preset
 * window.
 *
 * \param *file			The file to use the preset window in.
 * \param line			The display line to return the preset for.
 * \return			The appropriate preset, or NULL_preset.
 */

preset_t preset_get_preset_from_line(struct file_block *file, int line)
{
	if (file == NULL || file->presets == NULL)
		return NULL_PRESET;

	return preset_list_window_get_preset_from_line(file->presets->preset_window, line);
}


/**
 * Find the number of presets in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of presets in the file.
 */

int preset_get_count(struct file_block *file)
{
	return (file != NULL && file->presets != NULL) ? file->presets->preset_count : 0;
}


/**
 * Return the file associated with a preset instance.
 *
 * \param *instance		The preset instance to query.
 * \return			The associated file, or NULL.
 */

struct file_block *preset_get_file(struct preset_block *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->file;
}


/**
 * Test the validity of a preset index.
 *
 * \param *file			The file to test against.
 * \param preset		The preset index to test.
 * \return			TRUE if the index is valid; FALSE if not.
 */

osbool preset_test_index_valid(struct file_block *file, preset_t preset)
{
	return (preset_valid(file->presets, preset)) ? TRUE : FALSE;
}


/**
 * Return the name for a preset.
 *
 * If a buffer is supplied, the name is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * name in the preset array is returned instead. In the latter case, this
 * pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the name of.
 * \param *buffer		Pointer to a buffer to take the name, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting name string,
 *				either the supplied buffer or the original.
 */

char *preset_get_name(struct file_block *file, preset_t preset, char *buffer, size_t length)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset)) {
		if (buffer != NULL && length > 0) {
			*buffer = '\0';
			return buffer;
		}

		return NULL;
	}

	if (buffer == NULL || length == 0)
		return file->presets->presets[preset].name;

	string_copy(buffer, file->presets->presets[preset].name, length);

	return buffer;
}


/**
 * Find the caret target for the given preset.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to check.
 * \return			The preset's caret target.
 */

enum preset_caret preset_get_caret_destination(struct file_block *file, preset_t preset)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset))
		return 0;

	return file->presets->presets[preset].caret_target;
}


/**
 * Find the action key for the given preset.
 *
 * \param *file			The file holding the preset.
 * \param preset		The preset to check.
 * \return			The preset's action key.
 */

char preset_get_action_key(struct file_block *file, preset_t preset)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset))
		return '\0';

	return file->presets->presets[preset].action_key;
}


/**
 * Return the date for the given preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the date for.
 * \return			The preset's date, or NULL_DATE.
 */

date_t preset_get_date(struct file_block *file, preset_t preset)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset))
		return NULL_DATE;

	return file->presets->presets[preset].date;
}


/**
 * Return the from account of a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the from account for.
 * \return			The from account of the preset, or NULL_ACCOUNT.
 */

acct_t preset_get_from(struct file_block *file, preset_t preset)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset))
		return NULL_ACCOUNT;

	return file->presets->presets[preset].from;
}


/**
 * Return the to account of a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the to account for.
 * \return			The to account of the preset, or NULL_ACCOUNT.
 */

acct_t preset_get_to(struct file_block *file, preset_t preset)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset))
		return NULL_ACCOUNT;

	return file->presets->presets[preset].to;
}


/**
 * Return the preset flags for a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the flags for.
 * \return			The flags of the preset, or TRANS_FLAGS_NONE.
 */

enum transact_flags preset_get_flags(struct file_block *file, preset_t preset)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset))
		return TRANS_FLAGS_NONE;

	return file->presets->presets[preset].flags;
}


/**
 * Return the amount of a preset.
 *
 * \param *file			The file containing the preset.
 * \param preset		The preset to return the amount of.
 * \return			The amount of the preset, or NULL_CURRENCY.
 */

amt_t preset_get_amount(struct file_block *file, preset_t preset)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset))
		return NULL_CURRENCY;

	return file->presets->presets[preset].amount;
}


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

char *preset_get_reference(struct file_block *file, preset_t preset, char *buffer, size_t length)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset)) {
		if (buffer != NULL && length > 0) {
			*buffer = '\0';
			return buffer;
		}

		return NULL;
	}

	if (buffer == NULL || length == 0)
		return file->presets->presets[preset].reference;

	string_copy(buffer, file->presets->presets[preset].reference, length);

	return buffer;
}


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

char *preset_get_description(struct file_block *file, preset_t preset, char *buffer, size_t length)
{
	if (file == NULL || file->presets == NULL || !preset_valid(file->presets, preset)) {
		if (buffer != NULL && length > 0) {
			*buffer = '\0';
			return buffer;
		}

		return NULL;
	}

	if (buffer == NULL || length == 0)
		return file->presets->presets[preset].description;

	string_copy(buffer, file->presets->presets[preset].description, length);

	return buffer;
}


/**
 * Open the Preset Edit dialogue for a given preset list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void preset_open_edit_window(struct file_block *file, preset_t preset, wimp_pointer *ptr)
{
	struct preset_dialogue_data	*content;

	if (file == NULL || file->presets == NULL)
		return;

	/* Open the dialogue box. */

	content = heap_alloc(sizeof(struct preset_dialogue_data));
	if (content == NULL)
		return;

	content->action = PRESET_DIALOGUE_ACTION_NONE;
	content->preset = preset;

	if (preset == NULL_PRESET) {
		content->action_key = '\0';
		content->flags = TRANS_FLAGS_NONE;
		content->caret_target = PRESET_CARET_DATE;
		content->date = NULL_DATE;
		content->from = NULL_ACCOUNT;
		content->to = NULL_ACCOUNT;
		content->amount = NULL_CURRENCY;
		*(content->name) = '\0';
		*(content->reference) = '\0';
		*(content->description) = '\0';
	} else {
		content->action_key = file->presets->presets[preset].action_key;
		content->flags = file->presets->presets[preset].flags;
		content->caret_target = file->presets->presets[preset].caret_target;
		content->date = file->presets->presets[preset].date;
		content->from = file->presets->presets[preset].from;
		content->to = file->presets->presets[preset].to;
		content->amount = file->presets->presets[preset].amount;
		string_copy(content->name, file->presets->presets[preset].name, PRESET_NAME_LEN);
		string_copy(content->reference, file->presets->presets[preset].reference, TRANSACT_REF_FIELD_LEN);
		string_copy(content->description, file->presets->presets[preset].description, TRANSACT_DESCRIPT_FIELD_LEN);
	}

	preset_dialogue_open(ptr, file->presets, file, preset_process_edit_window, content);
}


/**
 * Process data associated with the currently open Preset Edit window.
 *
 * \param *parent		The preset instance holding the section.
 * \param *content		The content of the dialogue box.
 * \return			TRUE if processed; else FALSE.
 */

static osbool preset_process_edit_window(void *parent, struct preset_dialogue_data *content)
{
	struct preset_block	*windat = parent;
	char		copyname[PRESET_NAME_LEN];
	preset_t	check_key;

	if (content == NULL || windat == NULL)
		return FALSE;

	if (content->action == PRESET_DIALOGUE_ACTION_DELETE) {
		if (error_msgs_report_question ("DeletePreset", "DeletePresetB") == 4)
			return FALSE;

		return preset_delete(windat->file, content->preset);
	} else if (content->action != PRESET_DIALOGUE_ACTION_OK) {
		return FALSE;
	}

	/* Test that the preset has been given a name, and reject the data if not. */

	string_copy(copyname, content->name, PRESET_NAME_LEN);

	if (*string_strip_surrounding_whitespace(copyname) == '\0') {
		error_msgs_report_error("NoPresetName");
		return FALSE;
	}

	/* Test that the key, if any, is unique. */

	check_key = preset_find_from_keypress(windat->file, content->action_key);

	if (check_key != NULL_PRESET && check_key != content->preset) {
		error_msgs_report_error("BadPresetNo");
		return FALSE;
	}

	/* If the preset doesn't exsit, create it.  If it does exist, validate any data that requires it. */

	if (content->preset == NULL_PRESET)
		content->preset = preset_add(windat->file);

	/* If the preset was created OK, store the rest of the data. */

	if (content->preset == NULL_PRESET)
		return FALSE;

	if (!preset_valid(windat, content->preset))
		return FALSE;

	string_copy(windat->presets[content->preset].name, content->name, PRESET_NAME_LEN);
	string_copy(windat->presets[content->preset].reference, content->reference, TRANSACT_REF_FIELD_LEN);
	string_copy(windat->presets[content->preset].description, content->description, TRANSACT_DESCRIPT_FIELD_LEN);

	windat->presets[content->preset].action_key = content->action_key;
	windat->presets[content->preset].flags = content->flags;
	windat->presets[content->preset].date = content->date;
	windat->presets[content->preset].from = content->from;
	windat->presets[content->preset].to = content->to;
	windat->presets[content->preset].amount = content->amount;
	windat->presets[content->preset].caret_target = content->caret_target;

	/* Update the display. */

	if (config_opt_read("AutoSortPresets"))
		preset_sort(windat);
	else
		preset_list_window_redraw(windat->preset_window, content->preset);

	file_set_data_integrity(windat->file, TRUE);

	return TRUE;
}


/**
 * Sort the presets in a given instance based on that instance's sort setting.
 *
 * \param *windat		The preset window instance to sort.
 */

void preset_sort(struct preset_block *windat)
{
	if (windat == NULL)
		return;

	preset_list_window_sort(windat->preset_window);
}


/**
 * Create a new preset with null details.  Values are left to be set up later.
 *
 * \param *file			The file to add the preset to.
 * \return			The new preset index, or NULL_PRESET.
 */

static int preset_add(struct file_block *file)
{
	int	new;

	if (file == NULL || file->presets == NULL)
		return NULL_PRESET;

	if (!flexutils_resize((void **) &(file->presets->presets), sizeof(struct preset), file->presets->preset_count + 1)) {
		error_msgs_report_error("NoMemNewPreset");
		return NULL_PRESET;
	}

	new = file->presets->preset_count++;

	*file->presets->presets[new].name = '\0';
	file->presets->presets[new].action_key = 0;

	file->presets->presets[new].flags = 0;

	file->presets->presets[new].date = NULL_DATE;
	file->presets->presets[new].from = NULL_ACCOUNT;
	file->presets->presets[new].to = NULL_ACCOUNT;
	file->presets->presets[new].amount = NULL_CURRENCY;

	*file->presets->presets[new].reference = '\0';
	*file->presets->presets[new].description = '\0';

	preset_list_window_add_preset(file->presets->preset_window, new);

	file_set_data_integrity(file, TRUE);

	return new;
}


/**
 * Delete a preset from a file.
 *
 * \param *file			The file to act on.
 * \param preset		The preset to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

static osbool preset_delete(struct file_block *file, int preset)
{
	if (file == NULL || file->presets == NULL || preset == NULL_PRESET || !preset_valid(file->presets, preset))
		return FALSE;

	/* Delete the preset */

	if (!flexutils_delete_object((void **) &(file->presets->presets), sizeof(struct preset), preset)) {
		error_msgs_report_error("BadDelete");
		return FALSE;
	}

	file->presets->preset_count--;

	if (!preset_list_window_delete_preset(file->presets->preset_window, preset)) {
		error_msgs_report_error("BadDelete");
		return FALSE;
	}

	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Find a preset index based on its shortcut key.  If the key is '\0', or no
 * match is found, NULL_PRESET is returned.
 *
 * \param *file			The file to search in.
 * \param key			The shortcut key to search for.
 * \return			The matching preset index, or NULL_PRESET.
 */

preset_t preset_find_from_keypress(struct file_block *file, char key)
{
	preset_t preset = NULL_PRESET;


	if (file == NULL || file->presets == NULL || key == '\0')
		return preset;

	preset = 0;

	while ((preset < file->presets->preset_count) && (file->presets->presets[preset].action_key != key))
		preset++;

	if (preset == file->presets->preset_count)
		preset = NULL_PRESET;

	return preset;
}


/**
 * Save the preset details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void preset_write_file(struct file_block *file, FILE *out)
{
	int	i;

	if (file == NULL || file->presets == NULL)
		return;

	fprintf(out, "\n[Presets]\n");

	fprintf(out, "Entries: %x\n", file->presets->preset_count);

	preset_list_window_write_file(file->presets->preset_window, out);

	for (i = 0; i < file->presets->preset_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x\n",
				file->presets->presets[i].action_key, file->presets->presets[i].caret_target,
				file->presets->presets[i].date, file->presets->presets[i].flags,
				file->presets->presets[i].from, file->presets->presets[i].to, file->presets->presets[i].amount);
		if (*(file->presets->presets[i].name) != '\0')
			config_write_token_pair(out, "Name", file->presets->presets[i].name);
		if (*(file->presets->presets[i].reference) != '\0')
			config_write_token_pair(out, "Ref", file->presets->presets[i].reference);
		if (*(file->presets->presets[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->presets->presets[i].description);
	}
}


/**
 * Read preset order details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool preset_read_file(struct file_block *file, struct filing_block *in)
{
	size_t			block_size;
	preset_t		preset = NULL_PRESET;

	if (file == NULL || file->presets == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading preset Presets.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->presets->presets), sizeof(struct preset), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->presets->preset_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->presets->preset_count;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			preset_list_window_read_file_wincolumns(file->presets->preset_window, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "SortOrder")) {
			preset_list_window_read_file_sortorder(file->presets->preset_window, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			file->presets->preset_count++;
			if (file->presets->preset_count > block_size) {
				block_size = file->presets->preset_count;
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
			}
			preset = file->presets->preset_count - 1;
			file->presets->presets[preset].action_key = filing_get_char_field(in);
			file->presets->presets[preset].caret_target = preset_get_caret_field(in);
			file->presets->presets[preset].date = date_get_date_field(in);
			file->presets->presets[preset].flags = transact_get_flags_field(in);
			file->presets->presets[preset].from = account_get_account_field(in);
			file->presets->presets[preset].to = account_get_account_field(in);
			file->presets->presets[preset].amount = currency_get_currency_field(in);
			*(file->presets->presets[preset].name) = '\0';
			*(file->presets->presets[preset].reference) = '\0';
			*(file->presets->presets[preset].description) = '\0';
		} else if (preset != NULL_PRESET && filing_test_token(in, "Name")) {
			filing_get_text_value(in, file->presets->presets[preset].name, PRESET_NAME_LEN);
		} else if (preset != NULL_PRESET && filing_test_token(in, "Ref")) {
			filing_get_text_value(in, file->presets->presets[preset].reference, TRANSACT_REF_FIELD_LEN);
		} else if (preset != NULL_PRESET && filing_test_token(in, "Desc")) {
			filing_get_text_value(in, file->presets->presets[preset].description, TRANSACT_DESCRIPT_FIELD_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(file->presets->preset_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Initialise the preset list window contents. */

	if (!preset_list_window_initialise_entries(file->presets->preset_window, file->presets->preset_count)) {
		filing_set_status(in, FILING_STATUS_MEMORY);
		return FALSE;
	}

	return TRUE;
}


/**
 * Check the presets in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool preset_check_account(struct file_block *file, acct_t account)
{
	int		i;

	if (file == NULL || file->presets == NULL)
		return FALSE;

	for (i = 0; i < file->presets->preset_count; i++)
		if (file->presets->presets[i].from == account || file->presets->presets[i].to == account)
			return TRUE;

	return FALSE;
}

