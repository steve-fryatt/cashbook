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

#include <string.h>
#include <stdio.h>

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
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

//#include "global.h"
#include "find.h"

#include "account.h"
#include "account_menu.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "transact.h"

#define FIND_ICON_OK 26
#define FIND_ICON_CANCEL 27

#define FIND_ICON_DATE 2
#define FIND_ICON_FMIDENT 4
#define FIND_ICON_FMREC 5
#define FIND_ICON_FMNAME 6
#define FIND_ICON_TOIDENT 8
#define FIND_ICON_TOREC 9
#define FIND_ICON_TONAME 10
#define FIND_ICON_REF 12
#define FIND_ICON_AMOUNT 14
#define FIND_ICON_DESC 16

#define FIND_ICON_AND 18
#define FIND_ICON_OR 19

#define FIND_ICON_CASE 17
#define FIND_ICON_WHOLE 28

#define FIND_ICON_START 22
#define FIND_ICON_DOWN 23
#define FIND_ICON_UP 25
#define FIND_ICON_END 24

#define FOUND_ICON_CANCEL 1
#define FOUND_ICON_PREVIOUS 0
#define FOUND_ICON_NEXT 2
#define FOUND_ICON_NEW 3
#define FOUND_ICON_INFO 4

/**
 * Search logic options.
 */

enum find_logic {
	FIND_NOLOGIC = 0,							/**< No logic has been specified.					*/
	FIND_AND = 1,								/**< Search using AND logic to combine the fields.			*/
	FIND_OR = 2								/**< Search using OR logic to comnine the fields.			*/
};

/**
 * Search direction options.
 */

enum find_direction {
	FIND_NODIR = 0,								/**< No direction has been specified.					*/
	FIND_START = 1,								/**< Begin searching down from the start of the file.			*/
	FIND_END = 2,								/**< Begin searching up from the end of the file.			*/
	FIND_UP = 3,								/**< Continue searching up.						*/
	FIND_DOWN = 4,								/**< Continue searching down.						*/
	FIND_NEXT = 5,								/**< Find the next match in the current direction.			*/
	FIND_PREVIOUS = 6							/**< Find the previous match in the current direction.			*/
};

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

static struct find_block	*find_window_owner = NULL;			/**< The file currently owning the Find dialogue.			*/
static struct find_block	find_params;					/**< A copy of the settings for the current search.			*/
static osbool			find_restore = FALSE;				/**< The restore setting for the current dialogue.			*/

static wimp_w			find_window = NULL;				/**< The handle of the find window.					*/
static wimp_w			find_result_window = NULL;			/**< The handle of the 'found' window.					*/


static void		find_reopen_window(wimp_pointer *ptr);
static void		find_close_window(void);
static void		find_result_close_window(void);
static void		find_click_handler(wimp_pointer *pointer);
static osbool		find_keypress_handler(wimp_key *key);
static void		find_result_click_handler(wimp_pointer *pointer);
static void		find_refresh_window(void);
static void		find_fill_window(struct find_block *find_data, osbool restore);
static osbool		find_process_window(void);
static int		find_from_line(struct find_block *new_params, int new_dir, int start);


/**
 * Initialise the Find module.
 */

void find_initialise(void)
{
	find_window  = templates_create_window("Find");
	ihelp_add_window(find_window , "Find", NULL);
	event_add_window_mouse_event(find_window, find_click_handler);
	event_add_window_key_event(find_window, find_keypress_handler);
	event_add_window_icon_radio(find_window, FIND_ICON_AND, TRUE);
	event_add_window_icon_radio(find_window, FIND_ICON_OR, TRUE);
	event_add_window_icon_radio(find_window, FIND_ICON_START, TRUE);
	event_add_window_icon_radio(find_window, FIND_ICON_END, TRUE);
	event_add_window_icon_radio(find_window, FIND_ICON_UP, TRUE);
	event_add_window_icon_radio(find_window, FIND_ICON_DOWN, TRUE);



	find_result_window = templates_create_window("Found");
	ihelp_add_window(find_result_window, "Found", NULL);
	event_add_window_mouse_event(find_result_window, find_result_click_handler);
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
	if (find_window_owner == windat) {
		if (windows_get_open(find_window))
			find_close_window();

		if (windows_get_open(find_result_window))
			find_result_close_window();
	}

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
	/* If either of the find/found windows are already open, close them to start with. */

	if (windows_get_open(find_window))
		wimp_close_window(find_window);

	if (windows_get_open(find_result_window))
		wimp_close_window(find_result_window);

	/* Blank out the icon contents. */

	find_fill_window(windat, restore);

	/* Set the pointer up to find the window again and open the window. */

	find_window_owner = windat;
	find_restore = restore;

	windows_open_centred_at_pointer(find_window, ptr);
	place_dialogue_caret(find_window, FIND_ICON_DATE);
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

	if (windows_get_open(find_window))
		wimp_close_window(find_window);

	if (windows_get_open(find_result_window))
		wimp_close_window(find_result_window);

	/* Blank out the icon contents. */

	find_fill_window(&find_params, TRUE);

	find_restore = TRUE;

	windows_open_centred_at_pointer(find_window, ptr);
	place_dialogue_caret(find_window, FIND_ICON_DATE);
}


/**
 * Close the Find dialogue, and deregister it from its client.
 */

static void find_close_window(void)
{
	close_dialogue_with_caret(find_window);

	if (!windows_get_open(find_result_window))
		find_window_owner = NULL;
}


/**
 * Close the Find Result dialogue, and deregister it from its client.
 */

static void find_result_close_window(void)
{
	wimp_close_window(find_result_window);

	if (!windows_get_open(find_window))
		find_window_owner = NULL;
}


/**
 * Process mouse clicks in the Find dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void find_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case FIND_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			find_close_window();
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			find_refresh_window();
		break;

	case FIND_ICON_OK:
		if (find_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			find_close_window();
		break;

	case FIND_ICON_FMNAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			account_menu_open_icon(find_window_owner->file, ACCOUNT_MENU_FROM, find_window,
					FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC, pointer);
		break;

	case FIND_ICON_TONAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			account_menu_open_icon(find_window_owner->file, ACCOUNT_MENU_TO, find_window,
					FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC, pointer);
		break;

	case FIND_ICON_FMREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			account_toggle_reconcile_icon(find_window, FIND_ICON_FMREC);
		break;

	case FIND_ICON_TOREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			account_toggle_reconcile_icon(find_window, FIND_ICON_TOREC);
		break;
	}
}


/**
 * Process keypresses in the Find window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool find_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (find_process_window())
			find_close_window();
		break;

	case wimp_KEY_ESCAPE:
		find_close_window();
		break;

	default:
		if (key->i != FIND_ICON_FMIDENT && key->i != FIND_ICON_TOIDENT)
			return FALSE;

		if (key->i == FIND_ICON_FMIDENT)
			account_lookup_field(find_window_owner->file, key->c, ACCOUNT_IN | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
					find_window, FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC);

		else if (key->i == FIND_ICON_TOIDENT)
			account_lookup_field(find_window_owner->file, key->c, ACCOUNT_OUT | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
					find_window, FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC);
		break;
	}

	return TRUE;
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
 * Refresh the contents of the current Find window.
 */

static void find_refresh_window(void)
{
	find_fill_window(find_window_owner, find_restore);
	icons_redraw_group(find_window, 10, FIND_ICON_DATE,
			FIND_ICON_FMIDENT, FIND_ICON_FMREC, FIND_ICON_FMNAME,
			FIND_ICON_TOIDENT, FIND_ICON_TOREC, FIND_ICON_TONAME,
			FIND_ICON_REF, FIND_ICON_AMOUNT, FIND_ICON_DESC);
	icons_replace_caret_in_window(find_window);
}


/**
 * Fill the Find window with values.
 *
 * \param: *find_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void find_fill_window(struct find_block *find_data, osbool restore)
{
	if (!restore) {
		*icons_get_indirected_text_addr(find_window, FIND_ICON_DATE) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_FMIDENT) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_FMREC) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_FMNAME) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_TOIDENT) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_TOREC) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_TONAME) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_REF) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_AMOUNT) = '\0';
		*icons_get_indirected_text_addr(find_window, FIND_ICON_DESC) = '\0';

		icons_set_selected(find_window, FIND_ICON_AND, 1);
		icons_set_selected(find_window, FIND_ICON_OR, 0);

		icons_set_selected(find_window, FIND_ICON_START, 1);
		icons_set_selected(find_window, FIND_ICON_DOWN, 0);
		icons_set_selected(find_window, FIND_ICON_UP, 0);
		icons_set_selected(find_window, FIND_ICON_END, 0);
		icons_set_selected(find_window, FIND_ICON_CASE, 0);
	} else {
		date_convert_to_string(find_data->date, icons_get_indirected_text_addr(find_window, FIND_ICON_DATE),
				icons_get_indirected_text_length(find_window, FIND_ICON_DATE));

		account_fill_field(find_data->file, find_data->from, (find_data->reconciled & TRANS_REC_FROM) ? TRUE : FALSE,
				find_window, FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC);

		account_fill_field(find_data->file, find_data->to, (find_data->reconciled & TRANS_REC_TO) ? TRUE : FALSE,
				find_window, FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC);

		icons_strncpy(find_window, FIND_ICON_REF, find_data->ref);
		currency_convert_to_string(find_data->amount, icons_get_indirected_text_addr(find_window, FIND_ICON_AMOUNT),
				icons_get_indirected_text_length(find_window, FIND_ICON_AMOUNT));
		icons_strncpy(find_window, FIND_ICON_DESC, find_data->desc);

		icons_set_selected(find_window, FIND_ICON_AND, find_data->logic == FIND_AND);
		icons_set_selected(find_window, FIND_ICON_OR, find_data->logic == FIND_OR);

		icons_set_selected(find_window, FIND_ICON_START, find_data->direction == FIND_START);
		icons_set_selected(find_window, FIND_ICON_DOWN, find_data->direction == FIND_DOWN);
		icons_set_selected(find_window, FIND_ICON_UP, find_data->direction == FIND_UP);
		icons_set_selected(find_window, FIND_ICON_END, find_data->direction == FIND_END);

		icons_set_selected(find_window, FIND_ICON_CASE, find_data->case_sensitive);
		icons_set_selected(find_window, FIND_ICON_WHOLE, find_data->whole_text);
	}
}


/**
 * Process the contents of the Find window, store the details and
 * perform a find operation with them.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool find_process_window(void)
{
	int		line;

	/* Get the window contents. */

	find_window_owner->date = date_convert_from_string(icons_get_indirected_text_addr(find_window, FIND_ICON_DATE), NULL_DATE, 0);
	find_window_owner->from = account_find_by_ident(find_window_owner->file, icons_get_indirected_text_addr(find_window, FIND_ICON_FMIDENT),
			ACCOUNT_FULL | ACCOUNT_IN);
	find_window_owner->to = account_find_by_ident(find_window_owner->file, icons_get_indirected_text_addr(find_window, FIND_ICON_TOIDENT),
			ACCOUNT_FULL | ACCOUNT_OUT);
	find_window_owner->reconciled = TRANS_FLAGS_NONE;
	if (*icons_get_indirected_text_addr(find_window, FIND_ICON_FMREC) != '\0')
		find_window_owner->reconciled |= TRANS_REC_FROM;
	if (*icons_get_indirected_text_addr(find_window, FIND_ICON_TOREC) != '\0')
		find_window_owner->reconciled |= TRANS_REC_TO;
	find_window_owner->amount = currency_convert_from_string(icons_get_indirected_text_addr(find_window, FIND_ICON_AMOUNT));
	strcpy(find_window_owner->ref, icons_get_indirected_text_addr(find_window, FIND_ICON_REF));
	strcpy(find_window_owner->desc, icons_get_indirected_text_addr(find_window, FIND_ICON_DESC));

	/* Read find logic. */

	if (icons_get_selected(find_window, FIND_ICON_AND))
		find_window_owner->logic = FIND_AND;
	else if (icons_get_selected(find_window, FIND_ICON_OR))
		find_window_owner->logic = FIND_OR;

	/* Read search direction. */

	if (icons_get_selected(find_window, FIND_ICON_START))
		find_window_owner->direction = FIND_START;
	else if (icons_get_selected(find_window, FIND_ICON_END))
		find_window_owner->direction = FIND_END;
	else if (icons_get_selected(find_window, FIND_ICON_DOWN))
		find_window_owner->direction = FIND_DOWN;
	else if (icons_get_selected(find_window, FIND_ICON_UP))
		find_window_owner->direction = FIND_UP;

	find_window_owner->case_sensitive = icons_get_selected(find_window, FIND_ICON_CASE);
	find_window_owner->whole_text = icons_get_selected(find_window, FIND_ICON_WHOLE);

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
	char			buf1[64], buf2[64], ref[TRANSACT_REF_FIELD_LEN + 2], desc[TRANSACT_DESCRIPT_FIELD_LEN + 2];

	/* If new parameters are being supplied, take copies. */

	if (new_params != NULL) {
		find_params.file = new_params->file;
		find_params.date = new_params->date;
		find_params.from = new_params->from;
		find_params.to = new_params->to;
		find_params.reconciled = new_params->reconciled;
		find_params.amount = new_params->amount;
		strcpy(find_params.ref, new_params->ref);
		strcpy(find_params.desc, new_params->desc);

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
		snprintf(ref, TRANSACT_REF_FIELD_LEN + 2, "*%s*", find_params.ref);
	else
		strcpy(ref, find_params.ref);

	if ((!find_params.whole_text) && (*(find_params.desc) != '\0'))
		snprintf(desc, TRANSACT_DESCRIPT_FIELD_LEN + 2, "*%s*", find_params.desc);
	else
		strcpy(desc, find_params.desc);

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

	transact_get_column_name(find_window_owner->file, result, buf1, sizeof(buf1));
	snprintf(buf2, sizeof(buf2), "%d", transact_get_transaction_number(line));

	msgs_param_lookup("Found", icons_get_indirected_text_addr(find_result_window, FOUND_ICON_INFO), 64, buf1, buf2, NULL, NULL);

	if (!windows_get_open(find_result_window)) {
		wimp_get_pointer_info(&pointer);
		windows_open_centred_at_pointer(find_result_window, &pointer);
	} else {
		icons_redraw_group(find_result_window, 1, FOUND_ICON_INFO);
	}

	return line;
}

