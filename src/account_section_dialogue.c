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
 * \file: account_section_dialogue.c
 *
 * Account List Section Edit dialogue implementation.
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
#include "account_section_dialogue.h"

#include "account.h"
#include "caret.h"
#include "fontlist.h"

/* Edit section window. */

#define ACCOUNT_SECTION_DIALOGUE_OK 2
#define ACCOUNT_SECTION_DIALOGUE_CANCEL 3
#define ACCOUNT_SECTION_DIALOGUE_DELETE 4

#define ACCOUNT_SECTION_DIALOGUE_TITLE 0
#define ACCOUNT_SECTION_DIALOGUE_HEADER 5
#define ACCOUNT_SECTION_DIALOGUE_FOOTER 6


/* Global Variables */

/**
 * The handle of the Section Edit window.
 */

static wimp_w			account_section_dialogue_window = NULL;

/**
 * The starting name for the section.
 */

static char			account_section_dialogue_initial_name[ACCOUNT_SECTION_LEN];

/**
 * The starting type for the section.
 */

static enum account_line_type	account_section_dialogue_initial_type;

/**
 * Callback function to return updated settings.
 */

static osbool			(*account_section_dialogue_update_callback)(struct account_window *, int, char *, enum account_line_type);

/**
 * Callback function to request the deletion of a section.
 */

static osbool			(*account_section_dialogue_delete_callback)(struct account_window *, int);

/**
 * The account list to which the currently open Section Edit window belongs.
 */

static struct account_window	*account_section_dialogue_owner = NULL;

/**
 * The line in the account list being edited by the Section Edit window.
 */

static int			account_section_dialogue_line = -1;

/* Static Function Prototypes. */

static void			account_section_dialogue_click_handler(wimp_pointer *pointer);
static osbool			account_section_dialogue_keypress_handler(wimp_key *key);
static void			account_section_dialogue_refresh(void);
static void			account_section_dialogue_fill(void);
static osbool			account_section_dialogue_process(void);
static osbool			account_section_dialogue_delete(void);


/**
 * Initialise the account section edit dialogue.
 */

void account_section_dialogue_initialise(void)
{
	account_section_dialogue_window = templates_create_window("EditAccSect");
	ihelp_add_window(account_section_dialogue_window, "EditAccSect", NULL);
	event_add_window_mouse_event(account_section_dialogue_window, account_section_dialogue_click_handler);
	event_add_window_key_event(account_section_dialogue_window, account_section_dialogue_keypress_handler);
	event_add_window_icon_radio(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_HEADER, TRUE);
	event_add_window_icon_radio(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_FOOTER, TRUE);
}


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *window		The account list window to own the dialogue.
 * \param line			The line in the list associated with the dialogue, or -1.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *name			The initial name to use for the section.
 * \param type			The initial header/footer setting for the section.
 */

void account_section_dialogue_open(wimp_pointer *ptr, struct account_window *window, int line,
		osbool (*update_callback)(struct account_window *, int, char *, enum account_line_type),
		osbool (*delete_callback)(struct account_window *, int), char *name, enum account_line_type type)
{
	string_copy(account_section_dialogue_initial_name, name, ACCOUNT_SECTION_LEN);

	account_section_dialogue_initial_type = type;

	account_section_dialogue_update_callback = update_callback;
	account_section_dialogue_delete_callback = delete_callback;
	account_section_dialogue_owner = window;
	account_section_dialogue_line = line;

	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(account_section_dialogue_window))
		wimp_close_window(account_section_dialogue_window);

	/* Select the window to use and set the contents up. */

	account_section_dialogue_fill();

	if (line == -1) {
		windows_title_msgs_lookup(account_section_dialogue_window, "NewSect");
		icons_msgs_lookup(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_OK, "NewAcctAct");
	} else {
		windows_title_msgs_lookup(account_section_dialogue_window, "EditSect");
		icons_msgs_lookup(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_OK, "EditAcctAct");
	}

	/* Open the window. */

	windows_open_centred_at_pointer(account_section_dialogue_window, ptr);
	place_dialogue_caret(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_TITLE);
}


/**
 * Force the closure of the account section edit dialogue if it relates
 * to a given accounts list instance.
 *
 * \param *parent		The parent of the dialogue to be closed,
 *				or NULL to force close.
 */

void account_section_dialogue_force_close(struct account_window *parent)
{
	if (account_section_dialogue_is_open(parent))
		close_dialogue_with_caret(account_section_dialogue_window);
}


/**
 * Check whether the Edit Section dialogue is open for a given accounts
 * list instance.
 *
 * \param *parent		The accounts list instance to check.
 * \return			TRUE if the dialogue is open; else FALSE.
 */

osbool account_section_dialogue_is_open(struct account_window *parent)
{
	return ((account_section_dialogue_owner == parent || parent == NULL) && windows_get_open(account_section_dialogue_window)) ? TRUE : FALSE;
}


/**
 * Process mouse clicks in the Section Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void account_section_dialogue_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ACCOUNT_SECTION_DIALOGUE_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_section_dialogue_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			account_section_dialogue_refresh();
		break;

	case ACCOUNT_SECTION_DIALOGUE_OK:
		if (account_section_dialogue_process() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(account_section_dialogue_window);
		break;

	case ACCOUNT_SECTION_DIALOGUE_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && account_section_dialogue_delete())
			close_dialogue_with_caret(account_section_dialogue_window);
		break;
	}
}


/**
 * Process keypresses in the Section Edit window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool account_section_dialogue_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (account_section_dialogue_process())
			close_dialogue_with_caret(account_section_dialogue_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(account_section_dialogue_window);
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

static void account_section_dialogue_refresh(void)
{
	account_section_dialogue_fill();
	icons_redraw_group(account_section_dialogue_window, 1, ACCOUNT_SECTION_DIALOGUE_TITLE);
	icons_replace_caret_in_window(account_section_dialogue_window);
}


/**
 * Update the contents of the Section Edit window to reflect the current
 * settings of the given file and section.
 */

static void account_section_dialogue_fill(void)
{
	icons_strncpy(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_TITLE, account_section_dialogue_initial_name);

	icons_set_selected(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_HEADER,
			(account_section_dialogue_initial_type == ACCOUNT_LINE_HEADER));
	icons_set_selected(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_FOOTER,
			(account_section_dialogue_initial_type == ACCOUNT_LINE_FOOTER));

	icons_set_deleted(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_DELETE,
			(account_section_dialogue_line == -1));
}


/**
 * Take the contents of an updated Section Edit window and process the data.
 *
 * \return			TRUE if the data was valid; FALSE otherwise.
 */

static osbool account_section_dialogue_process(void)
{
	if (account_section_dialogue_update_callback == NULL)
		return FALSE;

	/* Extract the information from the dialogue. */

	icons_copy_text(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_TITLE, account_section_dialogue_initial_name, ACCOUNT_SECTION_LEN);

	if (icons_get_selected(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_HEADER))
		account_section_dialogue_initial_type = ACCOUNT_LINE_HEADER;
	else if (icons_get_selected(account_section_dialogue_window, ACCOUNT_SECTION_DIALOGUE_FOOTER))
		account_section_dialogue_initial_type = ACCOUNT_LINE_FOOTER;
	else
		account_section_dialogue_initial_type = ACCOUNT_LINE_BLANK;

	/* Call the client back. */

	return account_section_dialogue_update_callback(account_section_dialogue_owner, account_section_dialogue_line,
			account_section_dialogue_initial_name, account_section_dialogue_initial_type);
}


/**
 * Delete the section associated with the currently open Section Edit
 * window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_section_dialogue_delete(void)
{
	if (account_section_dialogue_delete_callback == NULL)
		return FALSE;

	/* Check that the user really wishes to proceed. */

	if (error_msgs_report_question("DeleteSection", "DeleteSectionB") == 4)
		return FALSE;

	/* Call the client back. */

	return account_section_dialogue_delete_callback(account_section_dialogue_owner, account_section_dialogue_line);
}

