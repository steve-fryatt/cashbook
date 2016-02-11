/* Copyright 2003-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: goto.c
 *
 * Transaction goto implementation.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "goto.h"

#include "caret.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "transact.h"


#define GOTO_ICON_OK 0
#define GOTO_ICON_CANCEL 1
#define GOTO_ICON_NUMBER_FIELD 3
#define GOTO_ICON_NUMBER 4
#define GOTO_ICON_DATE 5

/* Goto dialogue data. */

union go_to_target {
	int			line;						/**< The transaction number if the tratget is a line.		*/
	date_t			date;						/**< The transaction date if the target is a date.		*/
};

enum go_to_type {
	GOTO_TYPE_LINE = 0,							/**< The goto target is given as a line number.			*/
	GOTO_TYPE_DATE = 1							/**< The goto target is given as a date.			*/
};

struct goto_block {
	struct file_block	*file;						/**< The file owning the goto dialogue.				*/
	union go_to_target	target;						/**< The current goto target.					*/
	enum go_to_type		type;						/**< The type of target we're aiming for.			*/
};


static struct goto_block	*goto_window_owner = NULL;			/**< The file whoch currently owns the Goto dialogue.	*/
static osbool			goto_restore = FALSE;				/**< The restore setting for the current dialogue.	*/
static wimp_w			goto_window = NULL;				/**< The Goto dialogue handle.				*/


static void		goto_close_window(void);
static void		goto_click_handler(wimp_pointer *pointer);
static osbool		goto_keypress_handler(wimp_key *key);
static void		goto_refresh_window(void);
static void		goto_fill_window(struct goto_block *go_to_data, osbool restore);
static osbool		goto_process_window(void);


/**
 * Initialise the Goto module.
 */

void goto_initialise(void)
{
	goto_window = templates_create_window("Goto");
	ihelp_add_window(goto_window, "Goto", NULL);

	event_add_window_mouse_event(goto_window, goto_click_handler);
	event_add_window_key_event(goto_window, goto_keypress_handler);
	event_add_window_icon_radio(goto_window, GOTO_ICON_NUMBER, TRUE);
	event_add_window_icon_radio(goto_window, GOTO_ICON_DATE, TRUE);
}


/**
 * Construct new goto data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to which this instance belongs.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct goto_block *goto_create(struct file_block *file)
{
	struct goto_block	*new;

	new = heap_alloc(sizeof(struct goto_block));
	if (new == NULL)
		return NULL;

	new->file = file;
	new->target.date = NULL_DATE;
	new->type = GOTO_TYPE_DATE;

	return new;
}


/**
 * Delete a goto data block.
 *
 * \param *windat	Pointer to the goto window data to delete.
 */

void goto_delete(struct goto_block *windat)
{
	if (goto_window_owner == windat && windows_get_open(goto_window))
		goto_close_window();

	if (windat != NULL)
		heap_free(windat);
}


/**
 * Open the Goto dialogue box.
 *
 * \param *windat	The Goto instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void goto_open_window(struct goto_block *windat, wimp_pointer *ptr, osbool restore)
{
	if (windat == NULL || ptr == NULL)
		return;

	/* If the window is already open, close it to start with. */

	if (windows_get_open(goto_window))
		wimp_close_window(goto_window);

	/* Blank out the icon contents. */

	goto_fill_window(windat, restore);

	/* Set the pointer up to find the window again and open the window. */

	goto_window_owner = windat;
	goto_restore = restore;

	windows_open_centred_at_pointer(goto_window, ptr);
	place_dialogue_caret(goto_window, GOTO_ICON_NUMBER_FIELD);
}


/**
 * Close the Goto dialogue, and deregister it from its client.
 */

static void goto_close_window(void)
{
	close_dialogue_with_caret(goto_window);
	goto_window_owner = NULL;
}

/**
 * Process mouse clicks in the Goto dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void goto_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case GOTO_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			goto_close_window();
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			goto_refresh_window();
		break;

	case GOTO_ICON_OK:
		if (goto_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			goto_close_window();
		break;
	}
}


/**
 * Process keypresses in the Goto window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool goto_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (goto_process_window())
			goto_close_window();
		break;

	case wimp_KEY_ESCAPE:
		goto_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Goto window.
 */

static void goto_refresh_window(void)
{
	goto_fill_window(goto_window_owner, goto_restore);

	icons_redraw_group(goto_window, 1, GOTO_ICON_NUMBER_FIELD);
	icons_replace_caret_in_window(goto_window);
}


/**
 * Fill the Goto window with values.
 *
 * \param: *goto_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void goto_fill_window(struct goto_block *go_to_data, osbool restore)
{
	if (!restore) {
		*icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD) = '\0';

		icons_set_selected(goto_window, GOTO_ICON_NUMBER, FALSE);
		icons_set_selected(goto_window, GOTO_ICON_DATE, TRUE);
	} else {
		if (go_to_data->type == GOTO_TYPE_LINE)
			icons_printf(goto_window, GOTO_ICON_NUMBER_FIELD, "%d", go_to_data->target.line);
		else if (go_to_data->type == GOTO_TYPE_DATE)
			date_convert_to_string((date_t) go_to_data->target.date, icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD),
					icons_get_indirected_text_length(goto_window, GOTO_ICON_NUMBER_FIELD));

		icons_set_selected(goto_window, GOTO_ICON_NUMBER, go_to_data->type == GOTO_TYPE_LINE);
		icons_set_selected(goto_window, GOTO_ICON_DATE, go_to_data->type == GOTO_TYPE_DATE);
	}
}


/**
 * Process the contents of the Goto window, store the details and
 * perform a goto operation.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool goto_process_window(void)
{
	int		transaction, line = 0;

	goto_window_owner->type = (icons_get_selected(goto_window, GOTO_ICON_DATE)) ? GOTO_TYPE_DATE : GOTO_TYPE_LINE;

	if (goto_window_owner->type == GOTO_TYPE_LINE) {
		/* Go to a plain transaction line number. */

		goto_window_owner->target.line = atoi(icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD));

		if (goto_window_owner->target.line <= 0 || goto_window_owner->target.line > transact_get_count(goto_window_owner->file) ||
				strlen(icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD)) == 0) {
			error_msgs_report_info("BadGotoLine");
			return FALSE;
		}

		line = transact_get_line_from_transaction(goto_window_owner->file, transact_find_transaction_number(goto_window_owner->target.line));
	} else if (goto_window_owner->type == GOTO_TYPE_DATE) {
		/* Go to a given date, or the nearest transaction. */

		goto_window_owner->target.date = date_convert_from_string(icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD), NULL_DATE, 0);

		if (goto_window_owner->target.date == NULL_DATE) {
			error_msgs_report_info("BadGotoDate");
			return FALSE;
		}

		transaction = transact_find_date(goto_window_owner->file, goto_window_owner->target.date);

		if (transaction == NULL_TRANSACTION)
			return FALSE;

		line = transact_get_line_from_transaction(goto_window_owner->file, transaction);
	}

	transact_place_caret(goto_window_owner->file, line, EDIT_ICON_DATE);

	return TRUE;
}

