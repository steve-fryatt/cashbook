/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: dialogue_lookup.c
 *
 * Dialogue account lookup dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>

/* SF-Lib header files. */

#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "dialogue_lookup.h"

#include "account.h"
#include "account_menu.h"

/* Dialogue Icons. */

#define DIALOGUE_LOOKUP_IDENT 0
#define DIALOGUE_LOOKUP_REC 1
#define DIALOGUE_LOOKUP_NAME 2
#define DIALOGUE_LOOKUP_CANCEL 3
#define DIALOGUE_LOOKUP_OK 4

/**
 * The handle of the Account Lookup window.
 */

static wimp_w			dialogue_lookup_window = NULL;

/**
 * The file currently owning the Account Lookup window.
 */

static struct file_block	*dialogue_lookup_file = NULL;

/**
 * The type(s) of account to be looked up in the window.
 */

static enum account_type	dialogue_lookup_type = ACCOUNT_NULL;

/**
 * The window currently owning the Account Lookup window.
 */

static wimp_w			dialogue_lookup_parent;

/**
 * The icon to which the lookup results should be inserted.
 */

static wimp_i			dialogue_lookup_icon;


/* Static Function Prototypes. */

static void		dialogue_lookup_click_handler(wimp_pointer *pointer);
static osbool		dialogue_lookup_keypress_handler(wimp_key *key);
static void		dialogue_lookup_menu_closed(void);
static osbool		dialogue_lookup_process_window(void);




/**
 * Initialise the Account Lookup dialogue.
 */

void dialogue_lookup_initialise(void)
{
	dialogue_lookup_window = templates_create_window("AccEnter");
	ihelp_add_window(dialogue_lookup_window, "AccEnter", NULL);
	event_add_window_mouse_event(dialogue_lookup_window, dialogue_lookup_click_handler);
	event_add_window_key_event(dialogue_lookup_window, dialogue_lookup_keypress_handler);
}


/**
 * Open the account lookup window as a menu, allowing an account to be
 * entered into an account list using a graphical interface.
 *
 * \param *file		The file instance to which the operation relates.
 * \param window		The window to own the lookup dialogue.
 * \param icon			The icon to own the lookup dialogue.
 * \param account		An account to seed the window, or NULL_ACCOUNT.
 * \param type			The types of account to be accepted.
 */

void dialogue_lookup_open_window(struct file_block *file, wimp_w window, wimp_i icon, acct_t account, enum account_type type)
{
	wimp_pointer		pointer;

	dialogue_lookup_file = file;
	dialogue_lookup_type = type;
	dialogue_lookup_parent = window;
	dialogue_lookup_icon = icon;

	account_fill_field(dialogue_lookup_file, account, FALSE, dialogue_lookup_window, DIALOGUE_LOOKUP_IDENT, DIALOGUE_LOOKUP_NAME, DIALOGUE_LOOKUP_REC);

	/* Set the window position and open it on screen. */

	pointer.w = window;
	pointer.i = icon;
	menus_create_popup_menu((wimp_menu *) dialogue_lookup_window, &pointer);
}


/**
 * Process mouse clicks in the Account Lookup dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void dialogue_lookup_click_handler(wimp_pointer *pointer)
{
	enum account_menu_type	type;
	wimp_window_state	window_state;

	switch (pointer->i) {
	case DIALOGUE_LOOKUP_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			wimp_create_menu((wimp_menu *) -1, 0, 0);
		break;

	case DIALOGUE_LOOKUP_OK:
		if (dialogue_lookup_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			wimp_create_menu((wimp_menu *) -1, 0, 0);
		break;

	case DIALOGUE_LOOKUP_NAME:
		if (pointer->buttons != wimp_CLICK_ADJUST)
			break;

		/* Change the lookup window from a menu to a static window, so
		 * that the lookup menu can be created.
		 */

		window_state.w = dialogue_lookup_window;
		wimp_get_window_state(&window_state);
		wimp_create_menu((wimp_menu *) -1, 0, 0);
		wimp_open_window((wimp_open *) &window_state);

		switch (dialogue_lookup_type) {
		case ACCOUNT_FULL | ACCOUNT_IN:
			type = ACCOUNT_MENU_FROM;
			break;

		case ACCOUNT_FULL | ACCOUNT_OUT:
			type = ACCOUNT_MENU_TO;
			break;

		case ACCOUNT_FULL:
			type = ACCOUNT_MENU_ACCOUNTS;
			break;

		case ACCOUNT_IN:
			type = ACCOUNT_MENU_INCOMING;
			break;

		case ACCOUNT_OUT:
			type = ACCOUNT_MENU_OUTGOING;
			break;

		default:
			type = ACCOUNT_MENU_FROM;
			break;
		}

		account_menu_open_icon(dialogue_lookup_file, type, dialogue_lookup_menu_closed,
				dialogue_lookup_window, DIALOGUE_LOOKUP_IDENT, DIALOGUE_LOOKUP_NAME, DIALOGUE_LOOKUP_REC, pointer);
		break;

	case DIALOGUE_LOOKUP_REC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			account_toggle_reconcile_icon(dialogue_lookup_window, DIALOGUE_LOOKUP_REC);
		break;
	}
}


/**
 * Process keypresses in the Account Lookup window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool dialogue_lookup_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (dialogue_lookup_process_window())
			wimp_create_menu((wimp_menu *) -1, 0, 0);
		break;

	default:
		if (key->i != DIALOGUE_LOOKUP_IDENT)
			return FALSE;

		if (key->i == DIALOGUE_LOOKUP_IDENT)
			account_lookup_field(dialogue_lookup_file, key->c, dialogue_lookup_type, NULL_ACCOUNT, NULL,
					dialogue_lookup_window, DIALOGUE_LOOKUP_IDENT, DIALOGUE_LOOKUP_NAME, DIALOGUE_LOOKUP_REC);
 		break;
	}

	return TRUE;
}


/**
 * This function is called whenever the account list menu closes.  If the enter
 * account window is open, it is converted back into a transient menu.
 */

static void dialogue_lookup_menu_closed(void)
{
	wimp_window_state	window_state;

	if (!windows_get_open(dialogue_lookup_window))
		return;

	window_state.w = dialogue_lookup_window;
	wimp_get_window_state(&window_state);
	wimp_close_window(dialogue_lookup_window);

	if (!windows_get_open(dialogue_lookup_parent))
		return;

	wimp_create_menu((wimp_menu *) -1, 0, 0);
	wimp_create_menu((wimp_menu *) dialogue_lookup_window, window_state.visible.x0, window_state.visible.y1);
}


/**
 * Take the account from the account lookup window, and put the ident into the
 * parent icon.
 *
 * \return			TRUE if the content was processed; FALSE otherwise.
 */

static osbool dialogue_lookup_process_window(void)
{
	int		index, max_len;
	acct_t		account;
	char		ident[ACCOUNT_IDENT_LEN + 1], *icon;
	wimp_caret	caret;

	/* Get the account number that was entered. */

	account = account_find_by_ident(dialogue_lookup_file, icons_get_indirected_text_addr(dialogue_lookup_window, DIALOGUE_LOOKUP_IDENT),
			dialogue_lookup_type);

	if (account == NULL_ACCOUNT)
		return TRUE;

	/* Get the icon text, and the length of it. */

	icon = icons_get_indirected_text_addr(dialogue_lookup_parent, dialogue_lookup_icon);
	max_len = string_ctrl_strlen(icon);

	/* Check the caret position.  If it is in the target icon, move the insertion until it falls before a comma;
	 * if not, place the index at the end of the text.
	 */

	wimp_get_caret_position(&caret);
	if (caret.w == dialogue_lookup_parent && caret.i == dialogue_lookup_icon) {
		index = caret.index;
		while (index < max_len && icon[index] != ',')
			index++;
	} else {
		index = max_len;
	}

	/* If the icon text is empty, the ident is inserted on its own.
	 *
	 * If there is text there, a comma is placed at the start or end depending on where the index falls in the
	 * string.  If it falls anywhere but the end, the index is assumed to be after a comma such that an extra
	 * comma is added after the ident to be inserted.
	 */

	if (*icon == '\0') {
		string_printf(ident, ACCOUNT_IDENT_LEN + 1, "%s", account_get_ident(dialogue_lookup_file, account));
	} else {
		if (index < max_len)
			string_printf(ident, ACCOUNT_IDENT_LEN + 1, "%s,", account_get_ident(dialogue_lookup_file, account));
		else
			string_printf(ident, ACCOUNT_IDENT_LEN + 1, ",%s", account_get_ident(dialogue_lookup_file, account));
	}

	icons_insert_text(dialogue_lookup_parent, dialogue_lookup_icon, index, ident, strlen(ident));
	icons_replace_caret_in_window(dialogue_lookup_parent);

	return TRUE;
}

