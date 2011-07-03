/* CashBook - find.c
 *
 * (C) Stephen Fryatt, 2003-2011
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
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "find.h"

#include "account.h"
#include "caret.h"
#include "conversion.h"
#include "date.h"
#include "edit.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "templates.h"
#include "transact.h"
#include "window.h"


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

#define FIND_TEST_DATE   0x01
#define FIND_TEST_FROM   0x02
#define FIND_TEST_TO     0x04
#define FIND_TEST_AMOUNT 0x08
#define FIND_TEST_REF    0x10
#define FIND_TEST_DESC   0x20


static file_data	*find_window_file = NULL;				/**< The file currently owning the Find dialogue.	*/
static find		find_params;						/**< A copy of the settings for the current search.	*/
static osbool		find_restore = FALSE;					/**< The restore setting for the current dialogue.	*/

static wimp_w		find_window = NULL;					/**< The handle of the find window.			*/
static wimp_w		find_result_window = NULL;				/**< The handle of the 'found' window.			*/


static void		find_reopen_window(wimp_pointer *ptr);
static void		find_click_handler(wimp_pointer *pointer);
static osbool		find_keypress_handler(wimp_key *key);
static void		find_result_click_handler(wimp_pointer *pointer);
static void		find_refresh_window(void);
static void		find_fill_window(find *find_data, osbool restore);
static osbool		find_process_window(void);
static int		find_from_line(find *new_params, int new_dir, int start);


/**
 * Initialise the Find module.
 */

void find_initialise(void)
{
	find_window  = templates_create_window("Find");
	ihelp_add_window(find_window , "Find", NULL);
	event_add_window_mouse_event(find_window, find_click_handler);
	event_add_window_key_event(find_window, find_keypress_handler);
	event_add_window_icon_radio(find_window, FIND_ICON_AND);
	event_add_window_icon_radio(find_window, FIND_ICON_OR);
	event_add_window_icon_radio(find_window, FIND_ICON_START);
	event_add_window_icon_radio(find_window, FIND_ICON_END);
	event_add_window_icon_radio(find_window, FIND_ICON_UP);
	event_add_window_icon_radio(find_window, FIND_ICON_DOWN);



	find_result_window = templates_create_window("Found");
	ihelp_add_window(find_result_window, "Found", NULL);
	event_add_window_mouse_event(find_result_window, find_result_click_handler);
}


/**
 * Open the Find dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void find_open_window(file_data *file, wimp_pointer *ptr, osbool restore)
{
	/* If either of the find/found windows are already open, close them to start with. */

	if (windows_get_open(find_window))
		wimp_close_window(find_window);

	if (windows_get_open(find_result_window))
		wimp_close_window(find_result_window);

	/* Blank out the icon contents. */

	find_fill_window(&(file->find), restore);

	/* Set the pointer up to find the window again and open the window. */

	find_window_file = file;
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
 * Process mouse clicks in the Find dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void find_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case FIND_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(find_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			find_refresh_window();
		break;

	case FIND_ICON_OK:
		if (find_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(find_window);
		break;

	case FIND_ICON_FMNAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			open_account_menu(find_window_file, ACCOUNT_MENU_FROM, 0, find_window,
					FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC, pointer);
		break;

	case FIND_ICON_TONAME:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			open_account_menu(find_window_file, ACCOUNT_MENU_TO, 0, find_window,
					FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC, pointer);
		break;

	case FIND_ICON_FMREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			toggle_account_reconcile_icon(find_window, FIND_ICON_FMREC);
		break;

	case FIND_ICON_TOREC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			toggle_account_reconcile_icon(find_window, FIND_ICON_TOREC);
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
			close_dialogue_with_caret(find_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(find_window);
		break;

	default:
		if (key->i != FIND_ICON_FMIDENT && key->i != FIND_ICON_TOIDENT)
			return FALSE;

		if (key->i == FIND_ICON_FMIDENT)
			lookup_account_field(find_window_file, key->c, ACCOUNT_IN | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
					find_window, FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC);

		else if (key->i == FIND_ICON_TOIDENT)
			lookup_account_field(find_window_file, key->c, ACCOUNT_OUT | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
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
			wimp_close_window(find_result_window);
		break;

	case FOUND_ICON_PREVIOUS:
		if (pointer->buttons == wimp_CLICK_SELECT && (find_from_line(NULL, FIND_PREVIOUS, NULL_TRANSACTION) == NULL_TRANSACTION))
			wimp_close_window(find_result_window);
		break;

	case FOUND_ICON_NEXT:
		if (pointer->buttons == wimp_CLICK_SELECT && (find_from_line(NULL, FIND_NEXT, NULL_TRANSACTION) == NULL_TRANSACTION))
			wimp_close_window(find_result_window);
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
	find_fill_window(&(find_window_file->find), find_restore);
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

static void find_fill_window(find *find_data, osbool restore)
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
		convert_date_to_string(find_data->date, icons_get_indirected_text_addr(find_window, FIND_ICON_DATE));

		fill_account_field(find_window_file, find_data->from, find_data->from_rec,
				find_window, FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC);

		fill_account_field(find_window_file, find_data->to, find_data->to_rec,
				find_window, FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC);

		icons_strncpy(find_window, FIND_ICON_REF, find_data->ref);
		convert_money_to_string(find_data->amount, icons_get_indirected_text_addr(find_window, FIND_ICON_AMOUNT));
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

	find_window_file->find.date = convert_string_to_date(icons_get_indirected_text_addr(find_window, FIND_ICON_DATE), NULL_DATE, 0);
	find_window_file->find.from = find_account(find_window_file, icons_get_indirected_text_addr(find_window, FIND_ICON_FMIDENT),
			ACCOUNT_FULL | ACCOUNT_IN);
	find_window_file->find.from_rec = (*icons_get_indirected_text_addr(find_window, FIND_ICON_FMREC) == '\0') ? 0 : TRANS_REC_FROM;
	find_window_file->find.to = find_account(find_window_file, icons_get_indirected_text_addr(find_window, FIND_ICON_TOIDENT),
			ACCOUNT_FULL | ACCOUNT_OUT);
	find_window_file->find.to_rec = (*icons_get_indirected_text_addr(find_window, FIND_ICON_TOREC) == '\0') ? 0 : TRANS_REC_TO;
	find_window_file->find.amount = convert_string_to_money(icons_get_indirected_text_addr(find_window, FIND_ICON_AMOUNT));
	strcpy(find_window_file->find.ref, icons_get_indirected_text_addr(find_window, FIND_ICON_REF));
	strcpy(find_window_file->find.desc, icons_get_indirected_text_addr(find_window, FIND_ICON_DESC));

	/* Read find logic. */

	if (icons_get_selected(find_window, FIND_ICON_AND))
		find_window_file->find.logic = FIND_AND;
	else if (icons_get_selected(find_window, FIND_ICON_OR))
		find_window_file->find.logic = FIND_OR;

	/* Read search direction. */

	if (icons_get_selected(find_window, FIND_ICON_START))
		find_window_file->find.direction = FIND_START;
	else if (icons_get_selected(find_window, FIND_ICON_END))
		find_window_file->find.direction = FIND_END;
	else if (icons_get_selected(find_window, FIND_ICON_DOWN))
		find_window_file->find.direction = FIND_DOWN;
	else if (icons_get_selected(find_window, FIND_ICON_UP))
		find_window_file->find.direction = FIND_UP;

	find_window_file->find.case_sensitive = icons_get_selected(find_window, FIND_ICON_CASE);
	find_window_file->find.whole_text = icons_get_selected(find_window, FIND_ICON_WHOLE);

	/* Get the start line. */

	if (find_window_file->find.direction == FIND_START)
		line = 0;
	else if (find_window_file->find.direction == FIND_END)
		line = find_window_file->trans_count-1;
	else if (find_window_file->find.direction == FIND_DOWN) {
		line = find_window_file->transaction_window.entry_line+1;
		if (line >= find_window_file->trans_count)
			line = find_window_file->trans_count-1;
	} else /* FUND_UP */ {
		line = find_window_file->transaction_window.entry_line-1;
		if (line < 0)
			line = 0;
	}

	/* Start the search. */

	line = find_from_line(&(find_window_file->find), FIND_NODIR, line);

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

static int find_from_line(find *new_params, int new_dir, int start)
{
	int		icon, line, transaction, direction;
	unsigned	test;
	osbool		match;
	wimp_pointer	pointer;
	char		buf1[64], buf2[64], ref[REF_FIELD_LEN+2], desc[DESCRIPT_FIELD_LEN+2];

	/* If new parameters are being supplied, take copies. */

	if (new_params != NULL) {
		find_params.date = new_params->date;
		find_params.from = new_params->from;
		find_params.from_rec = new_params->from_rec;
		find_params.to = new_params->to;
		find_params.to_rec = new_params->to_rec;
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
		snprintf(ref, REF_FIELD_LEN + 2, "*%s*", find_params.ref);
	else
		strcpy(ref, find_params.ref);

	if ((!find_params.whole_text) && (*(find_params.desc) != '\0'))
		snprintf(desc, DESCRIPT_FIELD_LEN + 2, "*%s*", find_params.desc);
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
			line = find_window_file->transaction_window.entry_line + 1;
			if (line >= find_window_file->trans_count)
				line = find_window_file->trans_count - 1;
		} else /* FIND_UP */ {
			line = find_window_file->transaction_window.entry_line - 1;
			if (line < 0)
				line = 0;
		}
	} else {
		line = start;
	}

	match = FALSE;
	icon = -1;

	while (line < find_window_file->trans_count && line >= 0 && !match) {
		/* Initialise the test result variable.  The tests all have a bit allocated.  For OR tests, these start unset and
		 * are set if a test passes; a non-zero value at the end indicates a match.  For AND tests, all the required bits
		 * are set at the start and cleared as tests match.  A zero value at the end indicates a match.
		 */

		test = 0;

		if (find_params.logic == FIND_AND) {
			if (find_params.date != NULL_DATE)
				test |= FIND_TEST_DATE;

			if (find_params.from != NULL_ACCOUNT)
				test |= FIND_TEST_FROM;

			if (find_params.to != NULL_ACCOUNT)
				test |= FIND_TEST_TO;

			if (find_params.amount != NULL_CURRENCY)
				test |= FIND_TEST_AMOUNT;

			if (*find_params.ref != '\0')
				test |= FIND_TEST_REF;

			if (*find_params.desc != '\0')
				test |= FIND_TEST_DESC;
		}

		/* Perform the tests.  Tests are carried out in reverse order, with the icon being set in each, such that the
		 * returned icon ends up in the left-most column.
		 */

		transaction = find_window_file->transactions[line].sort_index;

		if (*desc != '\0' && string_wildcard_compare(desc, find_window_file->transactions[transaction].description, !find_params.case_sensitive)) {
			test ^= FIND_TEST_DESC;
			icon = EDIT_ICON_DESCRIPT;
		}

		if (find_params.amount != NULL_CURRENCY && find_params.amount == find_window_file->transactions[transaction].amount) {
			test ^= FIND_TEST_AMOUNT;
			icon = EDIT_ICON_AMOUNT;
		}

		if (*ref != '\0' && string_wildcard_compare(ref, find_window_file->transactions[transaction].reference, !find_params.case_sensitive)) {
			test ^= FIND_TEST_REF;
			icon = EDIT_ICON_REF;
		}

		/* The following two tests check that a) an account has been specified, b) it is the same as the transaction and
		 * c) the two reconcile flags are set the same (if they are, the EOR operation cancels them out).
		 */

		if (find_params.to != NULL_ACCOUNT && find_params.to == find_window_file->transactions[transaction].to &&
				((find_params.to_rec ^ find_window_file->transactions[transaction].flags) & TRANS_REC_TO) == 0) {
			test ^= FIND_TEST_TO;
			icon = EDIT_ICON_TO;
		}

		if (find_params.from != NULL_ACCOUNT && find_params.from == find_window_file->transactions[transaction].from &&
				((find_params.from_rec ^ find_window_file->transactions[transaction].flags) & TRANS_REC_FROM) == 0) {
			test ^= FIND_TEST_FROM;
			icon = EDIT_ICON_FROM;
		}

		if (find_params.date != NULL_DATE && find_params.date == find_window_file->transactions[transaction].date) {
			test ^= FIND_TEST_DATE;
			icon = EDIT_ICON_DATE;
		}

		/* Check if the test passed or failed. */

		if ((find_params.logic == FIND_OR && test) || (find_params.logic == FIND_AND && !test)) {
			match = TRUE;
		} else {
			if (direction == FIND_UP)
				line--;
			else
				line++;
		}
	}

	if (!match) {
		error_msgs_report_info ("BadFind");
		return NULL_TRANSACTION;
	}

	wimp_close_window(find_window);

	place_transaction_edit_line(find_window_file, line);
	icons_put_caret_at_end(find_window_file->transaction_window.transaction_window, icon);
	find_transaction_edit_line(find_window_file);

	icons_copy_text(find_window_file->transaction_window.transaction_pane, column_group(TRANSACT_PANE_COL_MAP, icon), buf1);
	snprintf(buf2, sizeof(buf2), "%d", line);

	msgs_param_lookup("Found", icons_get_indirected_text_addr(find_result_window, FOUND_ICON_INFO), 64, buf1, buf2, NULL, NULL);

	if (!windows_get_open(find_result_window)) {
		wimp_get_pointer_info(&pointer);
		windows_open_centred_at_pointer(find_result_window, &pointer);
	} else {
		icons_redraw_group(find_result_window, 1, FOUND_ICON_INFO);
	}

	return line;
}


/**
 * Force the closure of the Find and Results windows if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void find_force_windows_closed(file_data *file)
{
	if (find_window_file == file && windows_get_open(find_window))
		close_dialogue_with_caret(find_window);

	if (find_window_file == file && windows_get_open(find_result_window))
		wimp_close_window(find_result_window);
}

