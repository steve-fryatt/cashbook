/* Copyright 2003-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "sflib/icons.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "goto.h"

#include "caret.h"
#include "date.h"
#include "edit.h"
#include "ihelp.h"
#include "templates.h"
#include "transact.h"


#define GOTO_ICON_OK 0
#define GOTO_ICON_CANCEL 1
#define GOTO_ICON_NUMBER_FIELD 3
#define GOTO_ICON_NUMBER 4
#define GOTO_ICON_DATE 5


static file_data	*goto_window_file = NULL;				/**< The file whoch currently owns the Goto dialogue.	*/
static osbool		goto_restore = FALSE;					/**< The restore setting for the current dialogue.	*/
static wimp_w		goto_window = NULL;					/**< The Goto dialogue handle.				*/


static void		goto_click_handler(wimp_pointer *pointer);
static osbool		goto_keypress_handler(wimp_key *key);
static void		goto_refresh_window(void);
static void		goto_fill_window(go_to *go_to_data, osbool restore);
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
 * Open the Goto dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void goto_open_window(file_data *file, wimp_pointer *ptr, osbool restore)
{
	/* If the window is already open, close it to start with. */

	if (windows_get_open(goto_window))
		wimp_close_window(goto_window);

	/* Blank out the icon contents. */

	goto_fill_window(&(file->go_to), restore);

	/* Set the pointer up to find the window again and open the window. */

	goto_window_file = file;
	goto_restore = restore;

	windows_open_centred_at_pointer(goto_window, ptr);
	place_dialogue_caret(goto_window, GOTO_ICON_NUMBER_FIELD);
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
			close_dialogue_with_caret(goto_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			goto_refresh_window();
		break;

	case GOTO_ICON_OK:
		if (goto_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(goto_window);
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
			close_dialogue_with_caret(goto_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(goto_window);
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
	goto_fill_window(&(goto_window_file->go_to), goto_restore);

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

static void goto_fill_window(go_to *go_to_data, osbool restore)
{
	if (!restore) {
		*icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD) = '\0';

		icons_set_selected(goto_window, GOTO_ICON_NUMBER, 0);
		icons_set_selected(goto_window, GOTO_ICON_DATE, 1);
	} else {
		if (go_to_data->data_type == GOTO_TYPE_LINE)
			icons_printf(goto_window, GOTO_ICON_NUMBER_FIELD, "%d", go_to_data->data);
		else if (go_to_data->data_type == GOTO_TYPE_DATE)
			convert_date_to_string((date_t) go_to_data->data, icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD));

		icons_set_selected(goto_window, GOTO_ICON_NUMBER, go_to_data->data_type == GOTO_TYPE_LINE);
		icons_set_selected(goto_window, GOTO_ICON_DATE, go_to_data->data_type == GOTO_TYPE_DATE);
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
	int		min, max, mid, line = 0;
	date_t		target;

	goto_window_file->go_to.data_type = (icons_get_selected(goto_window, GOTO_ICON_DATE)) ? GOTO_TYPE_DATE : GOTO_TYPE_LINE;

	if (goto_window_file->go_to.data_type == GOTO_TYPE_LINE) {
		/* Go to a plain transaction line number. */

		goto_window_file->go_to.data = atoi(icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD));

		if (goto_window_file->go_to.data <= 0 || goto_window_file->go_to.data > goto_window_file->trans_count ||
				strlen(icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD)) == 0) {
			error_msgs_report_info("BadGotoLine");
			return FALSE;
		}

		line = goto_window_file->go_to.data - 1;
	} else if (goto_window_file->go_to.data_type == GOTO_TYPE_DATE) {
		/* Go to a given date, or the nearest transaction. */

		if (goto_window_file->sort_valid == 0)
			sort_transactions(goto_window_file);

		target = convert_string_to_date(icons_get_indirected_text_addr(goto_window, GOTO_ICON_NUMBER_FIELD), NULL_DATE, 0);
		goto_window_file->go_to.data = (unsigned) target;

		/* Search through the array using a binary search: assumes that transactions are stored in date order. */

		min = 0;
		max = goto_window_file->trans_count - 1;

		while (min < max) {
			mid = (min + max)/2;

			if (target <= goto_window_file->transactions[mid].date)
				max = mid;
			else
				min = mid + 1;
		}

		line = locate_transaction_in_transact_window(goto_window_file, min);
	}

	place_transaction_edit_line(goto_window_file, line);
	icons_put_caret_at_end(goto_window_file->transaction_window.transaction_window, 0);
	find_transaction_edit_line(goto_window_file);

	return TRUE;
}


/**
 * Force the closure of the Goto window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void goto_force_window_closed(file_data *file)
{
	if (goto_window_file == file && windows_get_open(goto_window))
		close_dialogue_with_caret(goto_window);
}

