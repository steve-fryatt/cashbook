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
 * \file: purge.c
 *
 * Transaction purge implementation.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "purge.h"

#include "account.h"
#include "accview.h"
#include "calculation.h"
#include "caret.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "ihelp.h"
#include "sorder.h"
#include "templates.h"
#include "transact.h"


#define PURGE_ICON_OK 6
#define PURGE_ICON_CANCEL 7

#define PURGE_ICON_TRANSACT 0
#define PURGE_ICON_ACCOUNTS 3
#define PURGE_ICON_HEADINGS 4
#define PURGE_ICON_SORDERS 5

#define PURGE_ICON_DATE 2
#define PURGE_ICON_DATETEXT 1


static file_data	*purge_window_file = NULL;				/**< The file which currently owns the Purge window.	*/
static osbool		purge_window_restore = 0;				/**< The current restore setting for the Purge window.	*/
static wimp_w		purge_window = NULL;					/**< The Purge window handle.				*/


static void		purge_click_handler(wimp_pointer *pointer);
static osbool		purge_keypress_handler(wimp_key *key);
static void		purge_refresh_window(void);
static void		purge_fill_window(continuation *cont_data, osbool restore);
static osbool		purge_process_window(void);
static void		purge_file(file_data *file, osbool transactions, date_t date, osbool accounts, osbool headings, osbool sorders);


/**
 * Initialise the Purge module.
 */

void purge_initialise(void)
{
	purge_window = templates_create_window("Purge");
	ihelp_add_window(purge_window, "Purge", NULL);
	event_add_window_mouse_event(purge_window, purge_click_handler);
	event_add_window_key_event(purge_window, purge_keypress_handler);
}


/**
 * Open the Purge dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void purge_open_window(file_data *file, wimp_pointer *ptr, osbool restore)
{
	/* If the window is already open, close it to start with. */

	if (windows_get_open(purge_window))
		wimp_close_window(purge_window);

	/* Blank out the icon contents. */

	purge_fill_window(&(file->continuation), restore);

	/* Set the pointer up to find the window again and open the window. */

	purge_window_file = file;
	purge_window_restore = restore;

	windows_open_centred_at_pointer (purge_window, ptr);
	place_dialogue_caret_fallback (purge_window, 1, PURGE_ICON_DATE);
}


/**
 * Process mouse clicks in the Purge dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void purge_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case PURGE_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(purge_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			purge_refresh_window();
		break;

	case PURGE_ICON_OK:
		if (purge_process_window() && pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(purge_window);
		break;

	case PURGE_ICON_TRANSACT:
		icons_set_group_shaded_when_off(purge_window, PURGE_ICON_TRANSACT, 2,
				PURGE_ICON_DATE, PURGE_ICON_DATETEXT);
		icons_replace_caret_in_window(purge_window);
		break;
	}
}


/**
 * Process keypresses in the Purge window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool purge_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (purge_process_window())
			close_dialogue_with_caret(purge_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(purge_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Purge window.
 */

static void purge_refresh_window(void)
{
	purge_fill_window(&(purge_window_file->continuation), purge_window_restore);

	icons_redraw_group(purge_window, 1, PURGE_ICON_DATE);
	icons_replace_caret_in_window(purge_window);
}


/**
 * Fill the Purge window with values.
 *
 * \param: *goto_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void purge_fill_window(continuation *cont_data, osbool restore)
{
	if (!restore) {
		icons_set_selected(purge_window, PURGE_ICON_TRANSACT, TRUE);
		icons_set_selected(purge_window, PURGE_ICON_ACCOUNTS, FALSE);
		icons_set_selected(purge_window, PURGE_ICON_HEADINGS, FALSE);
		icons_set_selected(purge_window, PURGE_ICON_SORDERS, FALSE);

		*icons_get_indirected_text_addr(purge_window, PURGE_ICON_DATE) = '\0';
	} else {
		icons_set_selected(purge_window, PURGE_ICON_TRANSACT, cont_data->transactions);
		icons_set_selected(purge_window, PURGE_ICON_ACCOUNTS, cont_data->accounts);
		icons_set_selected(purge_window, PURGE_ICON_HEADINGS, cont_data->headings);
		icons_set_selected(purge_window, PURGE_ICON_SORDERS, cont_data->sorders);

		convert_date_to_string(cont_data->before, icons_get_indirected_text_addr(purge_window, PURGE_ICON_DATE));
	}

	icons_set_group_shaded_when_off (purge_window, PURGE_ICON_TRANSACT, 2,
			PURGE_ICON_DATE, PURGE_ICON_DATETEXT);
}


/**
 * Process the contents of the Purge window, store the details and
 * perform a goto operation.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool purge_process_window(void)
{
	purge_window_file->continuation.transactions = icons_get_selected(purge_window, PURGE_ICON_TRANSACT);
	purge_window_file->continuation.accounts = icons_get_selected(purge_window, PURGE_ICON_ACCOUNTS);
	purge_window_file->continuation.headings = icons_get_selected(purge_window, PURGE_ICON_HEADINGS);
	purge_window_file->continuation.sorders = icons_get_selected(purge_window, PURGE_ICON_SORDERS);

	purge_window_file->continuation.before =
			convert_string_to_date(icons_get_indirected_text_addr(purge_window, PURGE_ICON_DATE), NULL_DATE, 0);

	if (purge_window_file->modified == 1 && error_msgs_report_question("ContFileNotSaved", "ContFileNotSavedB") == 2)
		return FALSE;

	purge_file(purge_window_file, purge_window_file->continuation.transactions,
		purge_window_file->continuation.before,
		purge_window_file->continuation.accounts,
		purge_window_file->continuation.headings,
		purge_window_file->continuation.sorders);

	return TRUE;
}


/**
 * Force the closure of the Purge window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void purge_force_window_closed(file_data *file)
{
	if (purge_window_file == file && windows_get_open(purge_window))
		close_dialogue_with_caret(purge_window);
}


/**
 * Purge unused components from a file.
 *
 * \param *file			The file to be purged.
 * \param transactions		TRUE to purge transactions; FALSE to ignore.
 * \param date			The cutoff transaction date, or NULL_DATE for all.
 * \param accounts		TRUE to purge accounts; FALSE to ignore.
 * \param headings		TRUE to purge headings; FALSE to ignore.
 * \param sorders		TRUE to purge standing orders; FALSE to ignore.
 */

static void purge_file(file_data *file, osbool transactions, date_t date, osbool accounts, osbool headings, osbool sorders)
{
	int	i;

	hourglass_on();

	/* Redraw the file now, so that the full extent of the original data
	 * is included in the redraw.
	 */

	redraw_file_windows(file);

	#ifdef DEBUG
	debug_printf("\\OPurging file");
	#endif

	/* Purge unused transactions from the file. */

	if (transactions) {
		for (i=0; i<file->trans_count; i++) {
			if ((file->transactions[i].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO) &&
					(date == NULL_DATE || file->transactions[i].date < date)) {
				/* If the from and to accounts are full accounts, */

				if (file->transactions[i].from != NULL_ACCOUNT &&
						(file->accounts[file->transactions[i].from].type & ACCOUNT_FULL) != 0)
					file->accounts[file->transactions[i].from].opening_balance -= file->transactions[i].amount;

				if (file->transactions[i].to != NULL_ACCOUNT &&
						(file->accounts[file->transactions[i].to].type & ACCOUNT_FULL) != 0)
					file->accounts[file->transactions[i].to].opening_balance += file->transactions[i].amount;

				file->transactions[i].date = NULL_DATE;
				file->transactions[i].from = NULL_ACCOUNT;
				file->transactions[i].to = NULL_ACCOUNT;
				file->transactions[i].flags = 0;
				file->transactions[i].amount = NULL_CURRENCY;
				*file->transactions[i].reference = '\0';
				*file->transactions[i].description = '\0';

				file->sort_valid = 0;
			}
		}

		if (file->sort_valid == 0)
			sort_transactions(file);

		strip_blank_transactions(file);
	}

	/* Purge any unused standing orders from the file. */

	if (sorders)
		sorder_purge(file);

	/* Purge unused accounts and headings from the file. */

	if (accounts || headings) {
		for (i=0; i<file->account_count; i++) {
			if (!account_used_in_file(file, i) &&
					((accounts && ((file->accounts[i].type & ACCOUNT_FULL) != 0)) ||
					(headings && ((file->accounts[i].type & (ACCOUNT_IN | ACCOUNT_OUT)) != 0)))) {
				#ifdef DEBUG
				debug_printf("Deleting account %d, type %x", i, file->accounts[i].type);
				#endif
				account_delete(file, i);
			}
		}
	}

	/* Recalculate the file and update the window. */

	accview_rebuild_all(file);

	/* perform_full_recalculation (file); */

	*(file->filename) = '\0';
	build_transaction_window_title(file);
	set_file_data_integrity(file, 1);

	/* Put the caret into the first empty line. */

	scroll_transaction_window_to_end(file, -1);

	set_transaction_window_extent(file);

	place_transaction_edit_line(file, file->trans_count);
	icons_put_caret_at_end(file->transaction_window.transaction_window, 0);
	find_transaction_edit_line(file);

	hourglass_off();
}

