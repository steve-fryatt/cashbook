/* Copyright 2003-2012, Stephen Fryatt
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
 * \file: budget.c
 *
 * Budgeting and budget dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "budget.h"

#include "caret.h"
#include "calculation.h"
#include "date.h"
#include "file.h"
#include "ihelp.h"
#include "sorder.h"
#include "templates.h"


#define BUDGET_ICON_OK 0
#define BUDGET_ICON_CANCEL 1

#define BUDGET_ICON_START 5
#define BUDGET_ICON_FINISH 7
#define BUDGET_ICON_TRIAL 11
#define BUDGET_ICON_RESTRICT 13


static file_data	*budget_window_file = NULL;				/**< The file currently owning the Budget window.	*/
static wimp_w		budget_window = NULL;					/**< The Budget window handle.				*/


static void		budget_click_handler(wimp_pointer *pointer);
static osbool		budget_keypress_handler(wimp_key *key);
static void		budget_refresh_window(void);
static void		budget_fill_window(file_data *file);
static osbool		budget_process_window(void);


/**
 * Initialise the Budget module.
 */

void budget_initialise(void)
{
	budget_window = templates_create_window("Budget");
	ihelp_add_window(budget_window, "Budget", NULL);
	event_add_window_mouse_event(budget_window, budget_click_handler);
	event_add_window_key_event(budget_window, budget_keypress_handler);
}


/**
 * Open the Budget dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 */

void budget_open_window(file_data *file, wimp_pointer *ptr)
{
	/* If the window is already open, another account is being edited or created.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 */

	if (windows_get_open(budget_window))
		wimp_close_window(budget_window);

	/* Set the window contents up. */

	budget_fill_window(file);

	/* Set the pointers up so we can find this lot again and open the window. */

	budget_window_file = file;

	windows_open_centred_at_pointer(budget_window, ptr);
	place_dialogue_caret(budget_window, BUDGET_ICON_START);
}


/**
 * Process mouse clicks in the Budget dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void budget_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case BUDGET_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(budget_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			budget_refresh_window();
		break;

	case BUDGET_ICON_OK:
		if (budget_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(budget_window);
		break;
	}
}


/**
 * Process keypresses in the Budget window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool budget_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (budget_process_window())
			close_dialogue_with_caret(budget_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(budget_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Budget window.
 */

static void budget_refresh_window(void)
{
	budget_fill_window(budget_window_file);
	icons_redraw_group(budget_window, 3, BUDGET_ICON_START, BUDGET_ICON_FINISH, BUDGET_ICON_TRIAL);
	icons_replace_caret_in_window(budget_window);
}



/**
 * Fill the Budget window with values from the file.
 *
 * \param: *goto_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void budget_fill_window(file_data *file)
{
	convert_date_to_string(file->budget.start, icons_get_indirected_text_addr(budget_window, BUDGET_ICON_START));
	convert_date_to_string(file->budget.finish, icons_get_indirected_text_addr(budget_window, BUDGET_ICON_FINISH));

	icons_printf(budget_window, BUDGET_ICON_TRIAL, "%d", file->budget.sorder_trial);

	icons_set_selected(budget_window, BUDGET_ICON_RESTRICT, file->budget.limit_postdate);
}


/**
 * Process the contents of the Budget window, storing the details in the
 * owning file.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool budget_process_window(void)
{
	budget_window_file->budget.start =
			convert_string_to_date(icons_get_indirected_text_addr(budget_window, BUDGET_ICON_START), NULL_DATE, 0);
	budget_window_file->budget.finish =
			convert_string_to_date(icons_get_indirected_text_addr(budget_window, BUDGET_ICON_FINISH), NULL_DATE, 0);

	budget_window_file->budget.sorder_trial = atoi(icons_get_indirected_text_addr(budget_window, BUDGET_ICON_TRIAL));

	budget_window_file->budget.limit_postdate = icons_get_selected(budget_window, BUDGET_ICON_RESTRICT);

	/* Tidy up and redraw the windows */

	sorder_trial(budget_window_file);
	perform_full_recalculation(budget_window_file);
	set_file_data_integrity(budget_window_file, TRUE);
	redraw_file_windows(budget_window_file);

	return TRUE;
}


/**
 * Force the closure of the Budget window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void budget_force_window_closed (file_data *file)
{
	if (budget_window_file == file && windows_get_open(budget_window))
		close_dialogue_with_caret(budget_window);
}

