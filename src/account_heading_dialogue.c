/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: account_heading_dialogue.c
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
#include "account_heading_dialogue.h"

#include "account.h"
#include "caret.h"

/* Window Icons. */

#define ACCOUNT_HEADING_DIALOGUE_OK 0
#define ACCOUNT_HEADING_DIALOGUE_CANCEL 1
#define ACCOUNT_HEADING_DIALOGUE_DELETE 2

#define ACCOUNT_HEADING_DIALOGUE_NAME 4
#define ACCOUNT_HEADING_DIALOGUE_IDENT 6
#define ACCOUNT_HEADING_DIALOGUE_INCOMING 7
#define ACCOUNT_HEADING_DIALOGUE_OUTGOING 8
#define ACCOUNT_HEADING_DIALOGUE_BUDGET 10


/* Global Variables */

/**
 * The handle of the Heading Edit window.
 */

static wimp_w			account_heading_dialogue_window = NULL;

/**
 * The starting name for the heading.
 */

static char			account_heading_dialogue_initial_name[ACCOUNT_NAME_LEN];

/**
 * The starting ident for the heading.
 */

static char			account_heading_dialogue_initial_ident[ACCOUNT_IDENT_LEN];

/**
 * The starting budget limit for the heading.
 */

static amt_t			account_heading_dialogue_initial_budget;

/**
 * The starting type for the heading.
 */

static enum account_type	account_heading_dialogue_initial_type;

/**
 * Callback function to return updated settings.
 */

static osbool			(*account_heading_dialogue_update_callback)(struct account_block *, acct_t, char *, char *, amt_t, enum account_type);

/**
 * Callback function to request the deletion of a heading.
 */

static osbool			(*account_heading_dialogue_delete_callback)(struct account_block *, acct_t);

/**
 * The account block to which the currently open Heading Edit window belongs.
 */

static struct account_block	*account_heading_dialogue_owner = NULL;

/**
 * The number of the heading being edited by the Heading Edit window.
 */

static acct_t			account_heading_dialogue_account = NULL_ACCOUNT;

/* Static Function Prototypes. */

static void			account_heading_dialogue_click_handler(wimp_pointer *pointer);
static osbool			account_heading_dialogue_keypress_handler(wimp_key *key);
static void			account_heading_dialogue_refresh(void);
static void			account_heading_dialogue_fill(void);
static osbool			account_heading_dialogue_process(void);
static osbool			account_heading_dialogue_delete(void);


/**
 * Initialise the Heading Edit dialogue.
 */

void account_heading_dialogue_initialise(void)
{
	account_heading_dialogue_window = templates_create_window("EditHeading");
	ihelp_add_window(account_heading_dialogue_window, "EditHeading", NULL);
	event_add_window_mouse_event(account_heading_dialogue_window, account_heading_dialogue_click_handler);
	event_add_window_key_event(account_heading_dialogue_window, account_heading_dialogue_keypress_handler);
	event_add_window_icon_radio(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_INCOMING, TRUE);
	event_add_window_icon_radio(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_OUTGOING, TRUE);
}


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param account		The account number of the heading to be edited, or NULL_ACCOUNT.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *name			The initial name to use for the heading.
 * \param *ident		The initial ident to use for the heading.
 * \param budget		The initial budget limit to use for the heading.
 * \param type			The initial incoming/outgoing type for the heading.
 */

void account_heading_dialogue_open(wimp_pointer *ptr, struct account_block *owner, acct_t account,
		osbool (*update_callback)(struct account_block *, acct_t, char *, char *, amt_t, enum account_type),
		osbool (*delete_callback)(struct account_block *, acct_t), char *name, char *ident, amt_t budget, enum account_type type)
{
	string_copy(account_heading_dialogue_initial_name, name, ACCOUNT_NAME_LEN);
	string_copy(account_heading_dialogue_initial_ident, ident, ACCOUNT_IDENT_LEN);

	account_heading_dialogue_initial_budget = budget;
	account_heading_dialogue_initial_type = type;

	account_heading_dialogue_update_callback = update_callback;
	account_heading_dialogue_delete_callback = delete_callback;
	account_heading_dialogue_owner = owner;
	account_heading_dialogue_account = account;

	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(account_heading_dialogue_window))
		wimp_close_window(account_heading_dialogue_window);

	/* Select the window to use and set the contents up. */

	account_heading_dialogue_fill();

	if (account == NULL_ACCOUNT) {
		windows_title_msgs_lookup(account_heading_dialogue_window, "NewHdr");
		icons_msgs_lookup(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_OK, "NewAcctAct");
	} else {
		windows_title_msgs_lookup(account_heading_dialogue_window, "EditHdr");
		icons_msgs_lookup(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_OK, "EditAcctAct");
	}

	/* Open the window. */

	windows_open_centred_at_pointer(account_heading_dialogue_window, ptr);
	place_dialogue_caret(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_NAME);
}


/**
 * Force the closure of the account section edit dialogue if it relates
 * to a given accounts instance.
 *
 * \param *parent		The parent of the dialogue to be closed,
 *				or NULL to force close.
 */

void account_heading_dialogue_force_close(struct account_block *parent)
{
	if (account_heading_dialogue_is_open(parent))
		close_dialogue_with_caret(account_heading_dialogue_window);
}


/**
 * Check whether the Edit Section dialogue is open for a given accounts
 * instance.
 *
 * \param *parent		The accounts list instance to check.
 * \return			TRUE if the dialogue is open; else FALSE.
 */

osbool account_heading_dialogue_is_open(struct account_block *parent)
{
	return ((account_heading_dialogue_owner == parent || parent == NULL) && windows_get_open(account_heading_dialogue_window)) ? TRUE : FALSE;
}


/**
 * Process mouse clicks in the Section Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_heading_dialogue_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ACCOUNT_HEADING_DIALOGUE_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_heading_dialogue_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			account_heading_dialogue_refresh();
		break;

	case ACCOUNT_HEADING_DIALOGUE_OK:
		if (account_heading_dialogue_process() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_heading_dialogue_window);
		break;

	case ACCOUNT_HEADING_DIALOGUE_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && account_heading_dialogue_delete())
			close_dialogue_with_caret(account_heading_dialogue_window);
		break;
	}
}


/**
 * Process keypresses in the Section Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool account_heading_dialogue_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (account_heading_dialogue_process())
			close_dialogue_with_caret(account_heading_dialogue_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(account_heading_dialogue_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the Section Edit window.
 */

static void account_heading_dialogue_refresh(void)
{
	account_heading_dialogue_fill();
	icons_redraw_group(account_heading_dialogue_window, 3, ACCOUNT_HEADING_DIALOGUE_NAME, ACCOUNT_HEADING_DIALOGUE_IDENT, ACCOUNT_HEADING_DIALOGUE_BUDGET);
	icons_replace_caret_in_window(account_heading_dialogue_window);
}


/**
 * Update the contents of the Section Edit window to reflect the current
 * settings of the given file and section.
 */

static void account_heading_dialogue_fill(void)
{
	icons_strncpy(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_NAME, account_heading_dialogue_initial_name);
	icons_strncpy(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_IDENT, account_heading_dialogue_initial_ident);

	currency_convert_to_string(account_heading_dialogue_initial_budget,
			icons_get_indirected_text_addr(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_BUDGET),
			icons_get_indirected_text_length(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_BUDGET));

	icons_set_shaded(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_INCOMING, (account_heading_dialogue_account != NULL_ACCOUNT));
	icons_set_selected(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_INCOMING, (account_heading_dialogue_initial_type & ACCOUNT_IN));

	icons_set_shaded(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_OUTGOING, (account_heading_dialogue_account != NULL_ACCOUNT));
	icons_set_selected(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_OUTGOING, (account_heading_dialogue_initial_type & ACCOUNT_OUT));

	icons_set_deleted(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_DELETE, (account_heading_dialogue_account == NULL_ACCOUNT));
}


/**
 * Take the contents of an updated Section Edit window and process the data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_heading_dialogue_process(void)
{
	if (account_heading_dialogue_update_callback == NULL)
		return FALSE;

	/* Extract the information from the dialogue. */

	icons_copy_text(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_NAME, account_heading_dialogue_initial_name, ACCOUNT_NAME_LEN);
	icons_copy_text(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_IDENT, account_heading_dialogue_initial_ident, ACCOUNT_IDENT_LEN);

	account_heading_dialogue_initial_budget =
			currency_convert_from_string(icons_get_indirected_text_addr(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_BUDGET));

	if (icons_get_selected(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_INCOMING))
		account_heading_dialogue_initial_type = ACCOUNT_IN;
	else if (icons_get_selected(account_heading_dialogue_window, ACCOUNT_HEADING_DIALOGUE_OUTGOING))
		account_heading_dialogue_initial_type = ACCOUNT_OUT;
	else
		account_heading_dialogue_initial_type = ACCOUNT_NULL;

	/* Call the client back. */

	return account_heading_dialogue_update_callback(account_heading_dialogue_owner, account_heading_dialogue_account,
			account_heading_dialogue_initial_name, account_heading_dialogue_initial_ident,
			account_heading_dialogue_initial_budget, account_heading_dialogue_initial_type);
}


/**
 * Delete the section associated with the currently open Section Edit
 * window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_heading_dialogue_delete(void)
{
	if (account_heading_dialogue_delete_callback == NULL)
		return FALSE;

	/* Check that the user really wishes to proceed. */

	if (error_msgs_report_question("DeleteAcct", "DeleteAcctB") == 4)
		return FALSE;

	/* Call the client back. */

	return account_heading_dialogue_delete_callback(account_heading_dialogue_owner, account_heading_dialogue_account);
}

