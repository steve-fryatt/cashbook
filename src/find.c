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



#define FIND_MESSAGE_BUFFER_LENGTH 64


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

//static struct find_block	*find_window_owner = NULL;			/**< The file currently owning the Find dialogue.			*/
//static struct find_block	find_params;					/**< A copy of the settings for the current search.			*/
//static osbool			find_restore = FALSE;				/**< The restore setting for the current dialogue.			*/


static void		find_reopen_window(wimp_pointer *ptr);
static void		find_close_window(void);
static void		find_result_click_handler(wimp_pointer *pointer);
static osbool		find_process_window(void *owner, struct find_search_dialogue_data *content);
static int		find_from_line(struct find_block *new_params, int new_dir, int start);


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

	find_search_dialogue_open(ptr, restore, windat, windat->file, find_process_window, content);
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
 * Process mouse clicks in the Find Result dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void find_result_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case FOUND_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			find_result_close_window();
		break;

	case FOUND_ICON_PREVIOUS:
		if (pointer->buttons == wimp_CLICK_SELECT && (find_from_line(NULL, FIND_PREVIOUS, NULL_TRANSACTION) == NULL_TRANSACTION))
			find_result_close_window();
		break;

	case FOUND_ICON_NEXT:
		if (pointer->buttons == wimp_CLICK_SELECT && (find_from_line(NULL, FIND_NEXT, NULL_TRANSACTION) == NULL_TRANSACTION))
			find_result_close_window();
		break;

	case FOUND_ICON_NEW:
		if (pointer->buttons == wimp_CLICK_SELECT)
			find_reopen_window(pointer);
		break;
	}
}


/**
 * Process the contents of the Find window, store the details and
 * perform a find operation.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool find_process_window(void *owner, struct find_search_dialogue_data *content)
{
	int		line;


	/* Get the start line. */

	if (find_window_owner->direction == FIND_START)
		line = 0;
	else if (find_window_owner->direction == FIND_END)
		line = transact_get_count(find_window_owner->file) - 1;
	else if (find_window_owner->direction == FIND_DOWN) {
		line = transact_get_caret_line(find_window_owner->file) + 1;
		if (line >= transact_get_count(find_window_owner->file))
			line = transact_get_count(find_window_owner->file) - 1;
	} else /* FUND_UP */ {
		line = transact_get_caret_line(find_window_owner->file) - 1;
		if (line < 0)
			line = 0;
	}

	/* Start the search. */

	line = find_from_line(find_window_owner, FIND_NODIR, line);

	return (line != NULL_TRANSACTION) ? TRUE : FALSE;
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

static int find_from_line(struct find_block *new_params, int new_dir, int start)
{
	int			line;
	enum find_direction	direction;
	enum transact_field	result;
	wimp_pointer		pointer;
	char			buf1[FIND_MESSAGE_BUFFER_LENGTH], buf2[FIND_MESSAGE_BUFFER_LENGTH], ref[TRANSACT_REF_FIELD_LEN + 2], desc[TRANSACT_DESCRIPT_FIELD_LEN + 2];

	/* If new parameters are being supplied, take copies. */

	if (new_params != NULL) {
		find_params.file = new_params->file;
		find_params.date = new_params->date;
		find_params.from = new_params->from;
		find_params.to = new_params->to;
		find_params.reconciled = new_params->reconciled;
		find_params.amount = new_params->amount;
		string_copy(find_params.ref, new_params->ref, TRANSACT_REF_FIELD_LEN);
		string_copy(find_params.desc, new_params->desc, TRANSACT_DESCRIPT_FIELD_LEN);

		find_params.logic = new_params->logic;
		find_params.case_sensitive = new_params->case_sensitive;
		find_params.whole_text = new_params->whole_text;
		find_params.direction = new_params->direction;

		/* Start and End have served their purpose; they now need to convert into up and down. */

		if (find_params.direction == FIND_START)
			find_params.direction = FIND_DOWN;

		if (find_params.direction == FIND_END)
			find_params.direction = FIND_UP;
	}

	/* Take local copies of the two text fields, and add bracketing wildcards as necessary. */

	if ((!find_params.whole_text) && (*(find_params.ref) != '\0'))
		string_printf(ref, TRANSACT_REF_FIELD_LEN + 2, "*%s*", find_params.ref);
	else
		string_copy(ref, find_params.ref, TRANSACT_REF_FIELD_LEN + 2);

	if ((!find_params.whole_text) && (*(find_params.desc) != '\0'))
		string_printf(desc, TRANSACT_DESCRIPT_FIELD_LEN + 2, "*%s*", find_params.desc);
	else
		string_copy(desc, find_params.desc, TRANSACT_DESCRIPT_FIELD_LEN + 2);

	/* If the search needs to change direction, do so now. */

	if (new_dir == FIND_NEXT || new_dir == FIND_PREVIOUS) {
		if (find_params.direction == FIND_UP)
			direction = (new_dir == FIND_NEXT) ? FIND_UP : FIND_DOWN;
		else
			direction = (new_dir == FIND_NEXT) ? FIND_DOWN : FIND_UP;
	} else {
		direction = find_params.direction;
	}

	/* If a new start line is being specified, take note, else use the current edit line. */

	if (start == NULL_TRANSACTION) {
		if (direction == FIND_DOWN) {
			line = transact_get_caret_line(find_window_owner->file) + 1;
			if (line >= transact_get_count(find_window_owner->file))
				line = transact_get_count(find_window_owner->file) - 1;
		} else /* FIND_UP */ {
			line = transact_get_caret_line(find_window_owner->file) - 1;
			if (line < 0)
				line = 0;
		}
	} else {
		line = start;
	}

	result = transact_search(find_window_owner->file, &line, direction == FIND_UP, find_params.case_sensitive, find_params.logic == FIND_AND,
			find_params.date, find_params.from, find_params.to, find_params.reconciled, find_params.amount, ref, desc);

	if (result == TRANSACT_FIELD_NONE) {
		error_msgs_report_info ("BadFind");
		return NULL_TRANSACTION;
	}

	wimp_close_window(find_window);

	transact_place_caret(find_window_owner->file, line, result);

	transact_get_column_name(find_window_owner->file, result, buf1, FIND_MESSAGE_BUFFER_LENGTH);
	string_printf(buf2, FIND_MESSAGE_BUFFER_LENGTH, "%d", transact_get_transaction_number(line));

	icons_msgs_param_lookup(find_result_window, FOUND_ICON_INFO, "Found", buf1, buf2, NULL, NULL);

	if (!windows_get_open(find_result_window)) {
		wimp_get_pointer_info(&pointer);
		windows_open_centred_at_pointer(find_result_window, &pointer);
	} else {
		icons_redraw_group(find_result_window, 1, FOUND_ICON_INFO);
	}

	return line;
}

