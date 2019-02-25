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
 * \file: find.c
 *
 * Transaction contents search implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

//#include "global.h"
#include "find.h"

#include "find_result_dialogue.h"
#include "find_search_dialogue.h"

#include "account.h"
#include "account_menu.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "transact.h"


/**
 * Search data.
 */

struct find_block {
	struct file_block	*file;						/**< The file to which this instance belongs.				*/

	date_t			date;						/**< The date to match, or NULL_DATE for none.				*/
	acct_t			from;						/**< The From account to match, or NULL_ACCOUNT for none.		*/
	acct_t			to;						/**< The To account to match, or NULL_ACCOUNT for none.			*/
	enum transact_flags	reconciled;					/**< The From and To Accounts' reconciled status.			*/
	amt_t			amount;						/**< The Amount to match, or NULL_CURRENCY for "don't care".		*/
	char			ref[TRANSACT_REF_FIELD_LEN];			/**< The Reference to match; NULL or '\0' for "don't care".		*/
	char			desc[TRANSACT_DESCRIPT_FIELD_LEN];		/**< The Description to match; NULL or '\0' for "don't care".		*/

	enum find_logic		logic;						/**< The logic to use to combine the fields specified above.		*/
	osbool			case_sensitive;					/**< TRUE to match case of strings; FALSE to ignore.			*/
	osbool			whole_text;					/**< TRUE to match strings exactly; FALSE to allow substrings.		*/
	enum find_direction	direction;					/**< The direction to search in.					*/
};

//static struct find_block	find_params;					/**< A copy of the settings for the current search.			*/

/* Static Function Prototypes. */

static void		find_reopen_window(wimp_pointer *ptr);
static osbool		find_process_search_window(void *owner, struct find_search_dialogue_data *content);
static osbool		find_process_result_window(wimp_pointer *pointer, void *owner, struct find_result_dialogue_data *content);
static tran_t		find_from_line(struct find_block *new_params, int new_dir, int start);


/**
 * Initialise the Find module.
 */

void find_initialise(void)
{
	find_search_dialogue_initialise();
	find_result_dialogue_initialise();
}


/**
 * Construct new find data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct find_block *find_create(struct file_block *file)
{
	struct find_block	*new;

	new = heap_alloc(sizeof(struct find_block));
	if (new == NULL)
		return NULL;

	new->file = file;
	new->date = NULL_DATE;
	new->from = NULL_ACCOUNT;
	new->to = NULL_ACCOUNT;
	new->reconciled = TRANS_FLAGS_NONE;
	new->amount = NULL_CURRENCY;
	*(new->ref) = '\0';
	*(new->desc) = '\0';

	new->logic = FIND_OR;
	new->case_sensitive = FALSE;
	new->whole_text = FALSE;
	new->direction = FIND_START;

	return new;
}


/**
 * Delete a find data block.
 *
 * \param *windat	Pointer to the find window data to delete.
 */

void find_delete(struct find_block *windat)
{
	if (windat != NULL)
		heap_free(windat);
}


/**
 * Open the Find dialogue box.
 *
 * \param *windat	The Find instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void find_open_window(struct find_block *windat, wimp_pointer *ptr, osbool restore)
{
	struct find_search_dialogue_data *content = NULL;

	if (windat == NULL || ptr == NULL)
		return;

	content = heap_alloc(sizeof(struct find_search_dialogue_data));
	if (content == NULL)
		return;

	debug_printf("Allocating find block 0x%x", content);

	content->date = windat->date;
	content->from = windat->from;
	content->to = windat->to;
	content->reconciled = windat->reconciled;
	content->amount = windat->amount;
	content->logic = windat->logic;
	content->case_sensitive = windat->case_sensitive;
	content->whole_text = windat->whole_text;
	content->direction = windat->direction;

	string_copy(content->ref, windat->ref, TRANSACT_REF_FIELD_LEN);
	string_copy(content->desc, windat->desc, TRANSACT_DESCRIPT_FIELD_LEN);

	find_search_dialogue_open(ptr, restore, windat, windat->file, find_process_search_window, content);
}


/**
 * Re-open the Find window, from the 'modify' icon in the Found window, with
 * the current search params.
 *
 * \param *ptr			The Wimp pointer details.
 */

static void find_reopen_window(wimp_pointer *ptr)
{
	/* If either of the find/found windows are already open, close them to start with. */

//	if (windows_get_open(find_window))
//		wimp_close_window(find_window);

//	if (windows_get_open(find_result_window))
//		wimp_close_window(find_result_window);

	/* Blank out the icon contents. */

//	find_fill_window(&find_params, TRUE);

//	find_restore = TRUE;

//	windows_open_centred_at_pointer(find_window, ptr);
//	place_dialogue_caret(find_window, FIND_ICON_DATE);
}


/**
 * Process the contents of the Find window, store the details and
 * perform a find operation.
 *
 * \param *owner		The find instance currently owning the dialogue.
 * \param *content		The data from the dialogue which is to be processed.
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool find_process_search_window(void *owner, struct find_search_dialogue_data *content)
{
	struct find_block	*windat = owner;
	int			line;

	if (windat == NULL || content == NULL)
		return TRUE;

	/* Get the start line. */

	if (content->direction == FIND_START)
		line = 0;
	else if (content->direction == FIND_END)
		line = transact_get_count(windat->file) - 1;
	else if (content->direction == FIND_DOWN) {
		line = transact_get_caret_line(windat->file) + 1;
		if (line >= transact_get_count(windat->file))
			line = transact_get_count(windat->file) - 1;
	} else if (content->direction == FIND_UP) {
		line = transact_get_caret_line(windat->file) - 1;
		if (line < 0)
			line = 0;
	} else {
		return FALSE;
	}

	/* Store the new data. */

	windat->date = content->date;
	windat->from = content->from;
	windat->to = content->to;
	windat->reconciled = content->reconciled;
	windat->amount = content->amount;
	windat->logic = content->logic;
	windat->case_sensitive = content->case_sensitive;
	windat->whole_text = content->whole_text;
	windat->direction = content->direction;

	string_copy(windat->ref, content->ref, TRANSACT_REF_FIELD_LEN);
	string_copy(windat->desc, content->desc, TRANSACT_DESCRIPT_FIELD_LEN);

	/* Start the search. */

	line = find_from_line(windat, FIND_NODIR, line);

	return (line != NULL_TRANSACTION) ? TRUE : FALSE;
}


/**
 * Process the contents of the Found window, store the details and
 * perform a new find operation as required.
 *
 * \param *pointer		The Wimp pointer data from the last dialogue click.
 * \param *owner		The find instance currently owning the dialogue.
 * \param *content		The data from the dialogue which is to be processed.
 * \param *pointer		The mouse event block to handle.
 */

static osbool find_process_result_window(wimp_pointer *pointer, void *owner, struct find_result_dialogue_data *content)
{
	struct find_block	*windat = owner;

	if (windat == NULL || content == NULL)
		return TRUE;

	debug_printf("New action: %d", content->action);

	switch (content->action) {
//	case FOUND_ICON_CANCEL:
//		if (pointer->buttons == wimp_CLICK_SELECT)
//			find_result_close_window();
//		break;

	case FIND_RESULT_DIALOGUE_PREVIOUS:
		if (find_from_line(NULL, FIND_PREVIOUS, NULL_TRANSACTION) == NULL_TRANSACTION)
			return TRUE;
		break;

	case FIND_RESULT_DIALOGUE_NEXT:
		if (find_from_line(NULL, FIND_NEXT, NULL_TRANSACTION) == NULL_TRANSACTION)
			return TRUE;
		break;

	case FIND_RESULT_DIALOGUE_NEW:
		find_reopen_window(pointer);
		break;

	case FIND_RESULT_DIALOGUE_NONE:
		break;
	}

	return TRUE;
}


/**
 * Perform a search
 *
 * \param *new_params		The search parameters to use, or NULL to continue
 *				with the existing set.
 * \param new_dir		The direction to search in.
 * \param start			The line to start the search from (inclusive).
 * \return			The resulting matching transaction.
 */

static tran_t find_from_line(struct find_block *new_params, int new_dir, int start)
{
	struct find_result_dialogue_data	*saved_params;
	int					line;
	enum find_direction			direction;
	enum transact_field			result;
	wimp_pointer				pointer;
	char					ref[TRANSACT_REF_FIELD_LEN + 2], desc[TRANSACT_DESCRIPT_FIELD_LEN + 2];

	debug_printf("Starting to find a transaction...");

	if (new_params == NULL)
		return NULL_TRANSACTION;

	/* Take a copy of the saved parameters. */

	saved_params = heap_alloc(sizeof(struct find_result_dialogue_data));
	if (saved_params == NULL)
		return NULL_TRANSACTION;

	debug_printf("Allocating found block 0x%x", saved_params);

	saved_params->date = new_params->date;
	saved_params->from = new_params->from;
	saved_params->to = new_params->to;
	saved_params->reconciled = new_params->reconciled;
	saved_params->amount = new_params->amount;
	string_copy(saved_params->ref, new_params->ref, TRANSACT_REF_FIELD_LEN);
	string_copy(saved_params->desc, new_params->desc, TRANSACT_DESCRIPT_FIELD_LEN);

	saved_params->logic = new_params->logic;
	saved_params->case_sensitive = new_params->case_sensitive;
	saved_params->whole_text = new_params->whole_text;
	saved_params->direction = new_params->direction;

	saved_params->result = TRANSACT_FIELD_NONE;
	saved_params->transaction = NULL_TRANSACTION;
	saved_params->action = FIND_RESULT_DIALOGUE_NONE;

	/* Start and End have served their purpose; they now need to convert into up and down. */

	if (saved_params->direction == FIND_START)
		saved_params->direction = FIND_DOWN;

	if (saved_params->direction == FIND_END)
		saved_params->direction = FIND_UP;

	/* Take local copies of the two text fields, and add bracketing wildcards as necessary. */

	if ((!saved_params->whole_text) && (*(saved_params->ref) != '\0'))
		string_printf(ref, TRANSACT_REF_FIELD_LEN + 2, "*%s*", saved_params->ref);
	else
		string_copy(ref, saved_params->ref, TRANSACT_REF_FIELD_LEN + 2);

	if ((!saved_params->whole_text) && (*(saved_params->desc) != '\0'))
		string_printf(desc, TRANSACT_DESCRIPT_FIELD_LEN + 2, "*%s*", saved_params->desc);
	else
		string_copy(desc, saved_params->desc, TRANSACT_DESCRIPT_FIELD_LEN + 2);

	/* If the search needs to change direction, do so now. */

	if (new_dir == FIND_NEXT || new_dir == FIND_PREVIOUS) {
		if (saved_params->direction == FIND_UP)
			direction = (new_dir == FIND_NEXT) ? FIND_UP : FIND_DOWN;
		else
			direction = (new_dir == FIND_NEXT) ? FIND_DOWN : FIND_UP;
	} else {
		direction = saved_params->direction;
	}

	/* If a new start line is being specified, take note, else use the current edit line. */

	if (start == NULL_TRANSACTION) {
		if (direction == FIND_DOWN) {
			line = transact_get_caret_line(new_params->file) + 1;
			if (line >= transact_get_count(new_params->file))
				line = transact_get_count(new_params->file) - 1;
		} else /* FIND_UP */ {
			line = transact_get_caret_line(new_params->file) - 1;
			if (line < 0)
				line = 0;
		}
	} else {
		line = start;
	}

	result = transact_search(new_params->file, &line, direction == FIND_UP, saved_params->case_sensitive, saved_params->logic == FIND_AND,
			saved_params->date, saved_params->from, saved_params->to, saved_params->reconciled, saved_params->amount, ref, desc);

	debug_printf("Find result: %d", result);

	if (result == TRANSACT_FIELD_NONE) {
		heap_free(saved_params);

		error_msgs_report_info ("BadFind");
		return NULL_TRANSACTION;
	}

	/* Store and act on the result. */

	transact_place_caret(new_params->file, line, result);

	saved_params->result = result;
	saved_params->transaction = line;

	wimp_get_pointer_info(&pointer);
	find_result_dialogue_open(&pointer, new_params, new_params->file, find_process_result_window, saved_params);

	return line;
}

