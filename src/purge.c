/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "purge.h"

#include "account.h"
#include "accview.h"
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


/**
 * Purge Dialogue data.
 */

struct purge {
	osbool			transactions;					/**< TRUE to remove reconciled transactions, subject to the before constraint; else FALSE.	*/
	osbool			accounts;					/**< TRUE to remove unused accounts; else FALSE.						*/
	osbool			headings;					/**< TRUE to remove unused headings; else FALSE.						*/
	osbool			sorders;					/**< TRUE to remove completed standing orders; else FALSE.					*/
	date_t			before;						/**< The date before which reconciled transactions should be removed; NULL_DATE for none.	*/
};

static struct file_block	*purge_window_file = NULL;			/**< The file which currently owns the Purge window.						*/
static osbool			purge_window_restore = 0;			/**< The current restore setting for the Purge window.						*/
static wimp_w			purge_window = NULL;				/**< The Purge window handle.									*/


static void		purge_click_handler(wimp_pointer *pointer);
static osbool		purge_keypress_handler(wimp_key *key);
static void		purge_refresh_window(void);
static void		purge_fill_window(struct purge *cont_data, osbool restore);
static osbool		purge_process_window(void);
static void		purge_file(struct file_block *file, osbool transactions, date_t date, osbool accounts, osbool headings, osbool sorders);


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
 * Construct new purge data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct purge *purge_create(void)
{
	struct purge	*new;

	new = heap_alloc(sizeof(struct purge));
	if (new == NULL)
		return NULL;

	new->transactions = TRUE;
	new->accounts = FALSE;
	new->headings = FALSE;
	new->sorders = FALSE;
	new->before = NULL_DATE;

	return new;
}


/**
 * Delete a purge data block.
 *
 * \param *purge	Pointer to the purge window data to delete.
 */

void purge_delete(struct purge *purge)
{
	if (purge != NULL)
		heap_free(purge);
}


/**
 * Open the Purge dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void purge_open_window(struct file_block *file, wimp_pointer *ptr, osbool restore)
{
	/* If the window is already open, close it to start with. */

	if (windows_get_open(purge_window))
		wimp_close_window(purge_window);

	/* Blank out the icon contents. */

	purge_fill_window(file->purge, restore);

	/* Set the pointer up to find the window again and open the window. */

	purge_window_file = file;
	purge_window_restore = restore;

	windows_open_centred_at_pointer(purge_window, ptr);
	place_dialogue_caret_fallback(purge_window, 1, PURGE_ICON_DATE);
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
	purge_fill_window(purge_window_file->purge, purge_window_restore);

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

static void purge_fill_window(struct purge *cont_data, osbool restore)
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

		date_convert_to_string(cont_data->before, icons_get_indirected_text_addr(purge_window, PURGE_ICON_DATE),
				icons_get_indirected_text_length(purge_window, PURGE_ICON_DATE));
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
	purge_window_file->purge->transactions = icons_get_selected(purge_window, PURGE_ICON_TRANSACT);
	purge_window_file->purge->accounts = icons_get_selected(purge_window, PURGE_ICON_ACCOUNTS);
	purge_window_file->purge->headings = icons_get_selected(purge_window, PURGE_ICON_HEADINGS);
	purge_window_file->purge->sorders = icons_get_selected(purge_window, PURGE_ICON_SORDERS);

	purge_window_file->purge->before =
			date_convert_from_string(icons_get_indirected_text_addr(purge_window, PURGE_ICON_DATE), NULL_DATE, 0);

	if (purge_window_file->modified == 1 && error_msgs_report_question("PurgeFileNotSaved", "PurgeFileNotSavedB") == 2)
		return FALSE;

	purge_file(purge_window_file, purge_window_file->purge->transactions,
		purge_window_file->purge->before,
		purge_window_file->purge->accounts,
		purge_window_file->purge->headings,
		purge_window_file->purge->sorders);

	return TRUE;
}


/**
 * Force the closure of the Purge window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void purge_force_window_closed(struct file_block *file)
{
	if (purge_window_file == file && windows_get_open(purge_window))
		close_dialogue_with_caret(purge_window);
}


/**
 * Purge unused components from a file.
 *
 * \TODO -- There might be an argument for moving a lot of the mechanics of this
 *          function into the transact and account modules, in the same way that
 *          standing orders are purged via sorder_purge()?
 *
 * \param *file			The file to be purged.
 * \param transactions		TRUE to purge transactions; FALSE to ignore.
 * \param cutoff		The cutoff transaction date, or NULL_DATE for all.
 * \param accounts		TRUE to purge accounts; FALSE to ignore.
 * \param headings		TRUE to purge headings; FALSE to ignore.
 * \param sorders		TRUE to purge standing orders; FALSE to ignore.
 */

static void purge_file(struct file_block *file, osbool transactions, date_t cutoff, osbool accounts, osbool headings, osbool sorders)
{
	int			i;
	enum transact_flags	flags;
	acct_t			from, to;
	date_t			date;
	amt_t			amount;

	hourglass_on();

	/* Redraw the file now, so that the full extent of the original data
	 * is included in the redraw.
	 */

	file_redraw_windows(file);

#ifdef DEBUG
	debug_printf("\\OPurging file");
#endif

	/* Purge unused transactions from the file. */

	if (transactions) {
		for (i=0; i<file->trans_count; i++) {
			date = transact_get_date(file, i);
			flags = transact_get_flags(file, i);
		
			if ((flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO) &&
					(cutoff == NULL_DATE || date < cutoff)) {
				from = transact_get_from(file, i);
				to = transact_get_to(file, i);
				amount = transact_get_amount(file, i);

				/* If the from and to accounts are full accounts, */

				if (from != NULL_ACCOUNT && (account_get_type(file, from) & ACCOUNT_FULL) != 0)
					account_adjust_opening_balance(file, from, -amount);

				if (to != NULL_ACCOUNT && (account_get_type(file, to) & ACCOUNT_FULL) != 0)
					account_adjust_opening_balance(file, to, +amount);

				transact_clear_raw_entry(file, i);
			}
		}

		if (file->sort_valid == 0)
			transact_sort_file_data(file);

		transact_strip_blanks_from_end(file);
	}

	/* Purge any unused standing orders from the file. */

	if (sorders)
		sorder_purge(file);

	/* Purge unused accounts and headings from the file. */

	if (accounts || headings) {
		for (i=0; i<file->account_count; i++) {
			if (!account_used_in_file(file, i) &&
					((accounts && ((account_get_type(file, i) & ACCOUNT_FULL) != 0)) ||
					(headings && ((account_get_type(file, i) & (ACCOUNT_IN | ACCOUNT_OUT)) != 0)))) {
#ifdef DEBUG
				debug_printf("Deleting account %d, type %x", i, file->accounts[i].type);
#endif
				account_delete(file, i);
			}
		}
	}

	/* Recalculate the file and update the window. */

	accview_rebuild_all(file);

	/* account_recalculate_all(file); */

	*(file->filename) = '\0';
	transact_build_window_title(file);
	file_set_data_integrity(file, TRUE);

	/* Put the caret into the first empty line. */

	transact_scroll_window_to_end(file, TRANSACT_SCROLL_HOME);

	transact_set_window_extent(file);

	transact_place_caret(file, file->trans_count, EDIT_ICON_DATE);

	hourglass_off();
}

