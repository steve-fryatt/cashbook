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
 * \file: account_account_dialogue.c
 *
 * Account Heading Edit dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "account_account_dialogue.h"

#include "account.h"
#include "account_idnum.h"
#include "caret.h"
#include "fontlist.h"

/* Edit section window. */

#define ACCOUNT_ACCOUNT_DIALOGUE_OK 2
#define ACCOUNT_ACCOUNT_DIALOGUE_CANCEL 3
#define ACCOUNT_ACCOUNT_DIALOGUE_DELETE 4

#define ACCOUNT_ACCOUNT_DIALOGUE_TITLE 0
#define ACCOUNT_ACCOUNT_DIALOGUE_HEADER 5
#define ACCOUNT_ACCOUNT_DIALOGUE_FOOTER 6

/* Accounr heading window. */

#define ACCT_EDIT_OK 0
#define ACCT_EDIT_CANCEL 1
#define ACCT_EDIT_DELETE 2

#define ACCT_EDIT_NAME 4
#define ACCT_EDIT_IDENT 6
#define ACCT_EDIT_CREDIT 8
#define ACCT_EDIT_BALANCE 10
#define ACCT_EDIT_PAYIN 12
#define ACCT_EDIT_CHEQUE 14
#define ACCT_EDIT_RATE 18
#define ACCT_EDIT_RATES 19
#define ACCT_EDIT_OFFSET_IDENT 21
#define ACCT_EDIT_OFFSET_REC 22
#define ACCT_EDIT_OFFSET_NAME 23
#define ACCT_EDIT_ACCNO 27
#define ACCT_EDIT_SRTCD 29
#define ACCT_EDIT_ADDR1 31
#define ACCT_EDIT_ADDR2 32
#define ACCT_EDIT_ADDR3 33
#define ACCT_EDIT_ADDR4 34


/* Global Variables */


/**
 * The handle of the Account Edit window.
 */

static wimp_w			account_account_dialogue_window = NULL;

/**
 * The starting name for the account.
 */

static char			account_account_dialogue_initial_name[ACCOUNT_NAME_LEN];

/**
 * The starting ident for the account.
 */

static char			account_account_dialogue_initial_ident[ACCOUNT_IDENT_LEN];

/**
 * The starting credit limit for the account.
 */

static amt_t			account_account_dialogue_initial_credit_limit;

/**
 * The starting opening balance for the account.
 */

static amt_t			account_account_dialogue_initial_opening_balance;

/**
 * The starting cheque number for the account.
 */

static struct account_idnum	account_account_dialogue_initial_cheque_number;

/**
 * The starting paying in for the account.
 */

static struct account_idnum	account_account_dialogue_initial_payin_number;

/**
 * The starting interest rate for the account.
 */

static rate_t			account_account_dialogue_initial_interest_rate;

/**
 * The starting offset account for the account.
 */

static acct_t			account_account_dialogue_initial_offset_against;

/**
 * The starting account number for the account.
 */

static char			account_account_dialogue_initial_account_num[ACCOUNT_NO_LEN];

/**
 * The starting sort code for the account.
 */

static char			account_account_dialogue_initial_sort_code[ACCOUNT_SRTCD_LEN];

/**
 * The starting address information for the account.
 */

static char			account_account_dialogue_initial_address[ACCOUNT_ADDR_LINES][ACCOUNT_ADDR_LEN];


/**
 * Callback function to return updated settings.
 */

static osbool			(*account_account_dialogue_update_callback)(struct account_block *, acc_t, char *, char *, amt_t, amt_t,
						struct account_idnum *, struct account_idnum *, acct_t, char *, char *, char **);

/**
 * Callback function to request the deletion of a heading.
 */

static osbool			(*account_account_dialogue_delete_callback)(struct account_block *, acct_t);

/**
 * The account list to which the currently open Heading Edit window belongs.
 */

static struct account_block	*account_account_dialogue_owner = NULL;

/**
 * The number of the heading being edited by the Heading Edit window.
 */

static acct_t			account_account_dialogue_account = NULL_ACCOUNT;

/* Static Function Prototypes. */

static void			account_account_dialogue_click_handler(wimp_pointer *pointer);
static osbool			account_account_dialogue_keypress_handler(wimp_key *key);
static void			account_account_dialogue_refresh(void);
static void			account_account_dialogue_fill(void);
static osbool			account_account_dialogue_process(void);
static osbool			account_account_dialogue_delete(void);


/**
 * Initialise the Account Edit dialogue.
 */

void account_account_dialogue_initialise(void)
{
	account_account_dialogue_window = templates_create_window("EditAccount");
	ihelp_add_window(account_account_dialogue_window, "EditAccount", NULL);
	event_add_window_mouse_event(account_account_dialogue_window, account_account_dialogue_click_handler);
	event_add_window_key_event(account_account_dialogue_window, account_account_dialogue_keypress_handler);
}


/**
 * Open the Account Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param account		The account number of the account to be edited, or NULL_ACCOUNT.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *name			The initial name to use for the account.
 * \param *ident		The initial ident to use for the account.
 * \param credit_limit		The initial credit limit to use for the account.
 * \param opening_balance	The initial opening balance to use for the account.
 * \param *cheque_number	The initial cheque number to use for the account.
 * \param *payin_number		The initial paying in number to use for the account.
 * \param interest_rate		The initial interest rate to use for the account.
 * \param offset_against	The initial offset account to use for the account.
 * \param *account_num		The initial account number to use for the account.
 * \param *sort_code		The initial sort code to use for the account.
 * \param **address		The initial address details to use for the account, or NULL.
 */

void account_account_dialogue_open(wimp_pointer *ptr, struct account_block *owner, acc_t account,
		osbool (*update_callback)(struct account_block *, acc_t, char *, char *, amt_t, amt_t, struct account_idnum *, struct account_idnum *, acct_t, char *, char *, char **),
		osbool (*delete_callback)(struct account_block *, acc_t),
		char *name, char *ident, amt_t credit_limit, amt_t opening_balance,
		struct account_idnum *cheque_number, struct account_idnum *payin_number,
		rate_t interest_rate, acct_t offset_against,
		char *account_num, char *sort_code, char **address)
{
	int i;

	string_copy(account_account_dialogue_initial_name, name, ACCOUNT_NAME_LEN);
	string_copy(account_account_dialogue_initial_ident, ident, ACCOUNT_IDENT_LEN);
	string_copy(account_account_dialogue_initial_account_num, account_num, ACCOUNT_NO_LEN);
	string_copy(account_account_dialogue_initial_sort_code, sort_code, ACCOUNT_SRTCD_LEN);

	for (i = 0; i < ACCOUNT_ADDR_LINES; i++)
		string_copy(account_account_dialogue_initial_address[i], (address == NULL) ? "" : address[i], ACCOUNT_ADDR_LEN);

	account_idnum_copy(&account_account_dialogue_initial_cheque_number, cheque_number);
	account_idnum_copy(&account_account_dialogue_initial_payin_number, payin_number);

	account_account_dialogue_initial_credit_limit = credit_limit;
	account_account_dialogue_initial_opening_balance = opening_balance;
	account_account_dialogue_initial_interest_rate = interest_rate;
	account_account_dialogue_initial_offset_against = offset_against;

	account_account_dialogue_update_callback = update_callback;
	account_account_dialogue_delete_callback = delete_callback;
	account_account_dialogue_owner = owner;
	account_account_dialogue_account = account;

	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(account_account_dialogue_window))
		wimp_close_window(account_account_dialogue_window);

	/* Select the window to use and set the contents up. */

	account_account_dialogue_fill();

	if (line == -1) {
		windows_title_msgs_lookup(account_account_dialogue_window, "NewAcct");
		icons_msgs_lookup(account_account_dialogue_window, ACCOUNT_ACCOUNT_DIALOGUE_OK, "NewAcctAct");
	} else {
		windows_title_msgs_lookup(account_account_dialogue_window, "EditAcct");
		icons_msgs_lookup(account_account_dialogue_window, ACCOUNT_ACCOUNT_DIALOGUE_OK, "EditAcctAct");
	}

	/* Open the window. */

	windows_open_centred_at_pointer(account_account_dialogue_window, ptr);
	place_dialogue_caret(account_account_dialogue_window, ACCT_EDIT_NAME);
}


/**
 * Force the closure of the account section edit dialogue if it relates
 * to a given accounts list instance.
 *
 * \param *parent		The parent of the dialogue to be closed,
 *				or NULL to force close.
 */

void account_account_dialogue_force_close(struct account_window *parent)
{
	if (account_account_dialogue_is_open(parent))
		close_dialogue_with_caret(account_account_dialogue_window);
}


/**
 * Check whether the Edit Section dialogue is open for a given accounts
 * list instance.
 *
 * \param *parent		The accounts list instance to check.
 * \return			TRUE if the dialogue is open; else FALSE.
 */

osbool account_account_dialogue_is_open(struct account_window *parent)
{
	return ((account_account_dialogue_owner == parent || parent == NULL) && windows_get_open(account_account_dialogue_window)) ? TRUE : FALSE;
}


/**
 * Process mouse clicks in the Section Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_account_dialogue_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ACCT_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(account_account_dialogue_window);
	//		interest_delete_window(account_edit_owner->file->interest, account_edit_number);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			account_account_dialogue_refresh();
		}
		break;

	case ACCT_EDIT_OK:
		if (account_account_dialogue_process() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(account_account_dialogue_window);
	//		interest_delete_window(account_edit_owner->file->interest, account_edit_number);
		}
		break;

	case ACCT_EDIT_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && account_account_dialogue_delete()) {
			close_dialogue_with_caret(account_account_dialogue_window);
	//		interest_delete_window(account_edit_owner->file->interest, account_edit_number);
		}
		break;

	case ACCT_EDIT_RATES:
	//	if (pointer->buttons == wimp_CLICK_SELECT)
	//		interest_open_window(account_edit_owner->file->interest, account_edit_number);
		break;

	case ACCT_EDIT_OFFSET_NAME:
	//	if (pointer->buttons == wimp_CLICK_ADJUST)
	//		account_menu_open_icon(account_edit_owner->file, ACCOUNT_MENU_ACCOUNTS, NULL,
	//				account_acc_edit_window, ACCT_EDIT_OFFSET_IDENT, ACCT_EDIT_OFFSET_NAME, ACCT_EDIT_OFFSET_REC, pointer);
		break;
	}
}


/**
 * Process keypresses in the Section Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool account_account_dialogue_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (account_account_dialogue_process()) {
			close_dialogue_with_caret(account_account_dialogue_window);
	//		interest_delete_window(account_edit_owner->file->interest, account_edit_number);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(account_account_dialogue_window);
	//	interest_delete_window(account_edit_owner->file->interest, account_edit_number);
		break;

	default:
		if (key->i != ACCT_EDIT_OFFSET_IDENT)
			return FALSE;

	//	account_lookup_field(account_edit_owner->file, key->c, ACCOUNT_FULL, NULL_ACCOUNT, NULL,
	//			account_acc_edit_window, ACCT_EDIT_OFFSET_IDENT, ACCT_EDIT_OFFSET_NAME, ACCT_EDIT_OFFSET_REC);
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Section Edit window.
 */

static void account_account_dialogue_refresh(void)
{
	account_account_dialogue_fill();
	icons_redraw_group(account_account_dialogue_window, 10, ACCT_EDIT_NAME, ACCT_EDIT_IDENT, ACCT_EDIT_CREDIT, ACCT_EDIT_BALANCE,
			ACCT_EDIT_ACCNO, ACCT_EDIT_SRTCD,
			ACCT_EDIT_ADDR1, ACCT_EDIT_ADDR2, ACCT_EDIT_ADDR3, ACCT_EDIT_ADDR4);
	icons_replace_caret_in_window(account_account_dialogue_window);
}


/**
 * Update the contents of the Section Edit window to reflect the current
 * settings of the given file and section.
 */

static void account_account_dialogue_fill(void)
{
	int i;

	icons_strncpy(account_account_dialogue_window, ACCT_EDIT_NAME, account_account_dialogue_initial_name);
	icons_strncpy(account_account_dialogue_window, ACCT_EDIT_IDENT, account_account_dialogue_initial_ident);

	currency_convert_to_string(account_account_dialogue_initial_credit_limit,
			icons_get_indirected_text_addr(account_account_dialogue_window, ACCT_EDIT_CREDIT),
			icons_get_indirected_text_length(account_account_dialogue_window, ACCT_EDIT_CREDIT));

	currency_convert_to_string(account_account_dialogue_initial_opening_balance,
			icons_get_indirected_text_addr(account_account_dialogue_window, ACCT_EDIT_BALANCE),
			icons_get_indirected_text_length(account_account_dialogue_window, ACCT_EDIT_BALANCE));

	account_idnum_get_next(&account_account_dialogue_initial_cheque_number,
			icons_get_indirected_text_addr(account_account_dialogue_window, ACCT_EDIT_CHEQUE),
			icons_get_indirected_text_length(account_account_dialogue_window, ACCT_EDIT_CHEQUE), 0);

	account_idnum_get_next(&account_account_dialogue_initial_payin_number,
			icons_get_indirected_text_addr(account_account_dialogue_window, ACCT_EDIT_PAYIN),
			icons_get_indirected_text_length(account_account_dialogue_window, ACCT_EDIT_PAYIN), 0);

	interest_convert_to_string(account_account_dialogue_initial_interest_rate,
			icons_get_indirected_text_addr(account_account_dialogue_window, ACCT_EDIT_RATE),
			icons_get_indirected_text_length(account_account_dialogue_window, ACCT_EDIT_RATE));

//	account_fill_field(block->file, block->accounts[account].offset_against, FALSE, account_acc_edit_window,
//			ACCT_EDIT_OFFSET_IDENT, ACCT_EDIT_OFFSET_NAME, ACCT_EDIT_OFFSET_REC);

	icons_strncpy(account_account_dialogue_window, ACCT_EDIT_ACCNO, account_account_dialogue_initial_account_um);
	icons_strncpy(account_account_dialogue_window, ACCT_EDIT_SRTCD, account_account_dialogue_initial_sort_code);

	for (i = 0; i < ACCOUNT_ADDR_LINES; i++)
		icons_strncpy(account_account_dialogue_window, ACCT_EDIT_ADDR1 + i, account_account_dialogue_initial_address[i]);

	icons_set_deleted(account_account_dialogue_window, ACCT_EDIT_DELETE, (account_account_dialogue_account == NULL_ACCOUNT));
}


/**
 * Take the contents of an updated Section Edit window and process the data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_account_dialogue_process(void)
{
	if (account_account_dialogue_update_callback == NULL)
		return FALSE;

	/* Extract the information from the dialogue. */

	icons_copy_text(account_account_dialogue_window, ACCOUNT_ACCOUNT_DIALOGUE_TITLE, account_account_dialogue_initial_name, ACCOUNT_ACCOUNT_LEN);

	if (icons_get_selected(account_account_dialogue_window, ACCOUNT_ACCOUNT_DIALOGUE_HEADER))
		account_account_dialogue_initial_type = ACCOUNT_LINE_HEADER;
	else if (icons_get_selected(account_account_dialogue_window, ACCOUNT_ACCOUNT_DIALOGUE_FOOTER))
		account_account_dialogue_initial_type = ACCOUNT_LINE_FOOTER;
	else
		account_account_dialogue_initial_type = ACCOUNT_LINE_BLANK;

	/* Call the client back. */

	return account_account_dialogue_update_callback(account_account_dialogue_owner, account_account_dialogue_line,
			account_account_dialogue_initial_name, account_account_dialogue_initial_type);
}


/**
 * Delete the section associated with the currently open Section Edit
 * window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_account_dialogue_delete(void)
{
	if (account_account_dialogue_delete_callback == NULL)
		return FALSE;

	/* Check that the user really wishes to proceed. */

	if (error_msgs_report_question("DeleteAcct", "DeleteAcctB") == 4)
		return FALSE;

	/* Call the client back. */

	return account_account_dialogue_delete_callback(account_account_dialogue_owner, account_account_dialogue_account);
}

