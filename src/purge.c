/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: purge.c
 *
 * Transaction purge implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/heap.h"

/* Application header files */

#include "global.h"
#include "purge.h"

#include "account.h"
#include "accview.h"
#include "date.h"
#include "file.h"
#include "list_window.h"
#include "purge_dialogue.h"
#include "sorder.h"
#include "transact.h"

/**
 * Purge Dialogue data.
 */

struct purge_block {
	struct file_block	*file;						/**< The file to which this instance of the dialogue belongs.					*/
	osbool			transactions;					/**< TRUE to remove reconciled transactions, subject to the before constraint; else FALSE.	*/
	osbool			accounts;					/**< TRUE to remove unused accounts; else FALSE.						*/
	osbool			headings;					/**< TRUE to remove unused headings; else FALSE.						*/
	osbool			sorders;					/**< TRUE to remove completed standing orders; else FALSE.					*/
	date_t			before;						/**< The date before which reconciled transactions should be removed; NULL_DATE for none.	*/
};


static osbool		purge_process_window(void *owner, struct purge_dialogue_data *content);
static void		purge_file(struct file_block *file, osbool transactions, date_t date, osbool accounts, osbool headings, osbool sorders);


/**
 * Initialise the Purge module.
 */

void purge_initialise(void)
{
	purge_dialogue_initialise();
}


/**
 * Construct new purge data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to which this instance belongs.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct purge_block *purge_create(struct file_block *file)
{
	struct purge_block	*new;

	new = heap_alloc(sizeof(struct purge_block));
	if (new == NULL)
		return NULL;

	new->file = file;
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

void purge_delete(struct purge_block *purge)
{
	if (purge != NULL)
		heap_free(purge);
}


/**
 * Open the Purge dialogue box.
 *
 * \param *purge	The purge instance owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void purge_open_window(struct purge_block *purge, wimp_pointer *ptr, osbool restore)
{
	struct purge_dialogue_data *content = NULL;

	if (purge == NULL || ptr == NULL)
		return;

	content = heap_alloc(sizeof(struct purge_dialogue_data));
	if (content == NULL)
		return;

	content->remove_transactions = purge->transactions;
	content->remove_accounts = purge->accounts;
	content->remove_headings = purge->headings;
	content->remove_sorders = purge->sorders;
	content->keep_from = purge->before;

	purge_dialogue_open(ptr, restore, purge, purge->file, purge_process_window, content);
}


/**
 * Process the contents of the Purge window, store the details and
 * perform a goto operation.
 *
 * \param *owner		The purge instance currently owning the dialogue.
 * \param *content		The data from the dialogue which is to be processed.
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool purge_process_window(void *owner, struct purge_dialogue_data *content)
{
	struct purge_block *windat = owner;

	if (windat == NULL || content == NULL)
		return TRUE;

	if (file_get_data_integrity(windat->file) == TRUE &&
			error_msgs_report_question("PurgeFileNotSaved", "PurgeFileNotSavedB") == 4)
		return FALSE;

	windat->transactions = content->remove_transactions;
	windat->accounts = content->remove_accounts;
	windat->headings = content->remove_headings;
	windat->sorders = content->remove_sorders;
	windat->before = content->keep_from;

	purge_file(windat->file, windat->transactions, windat->before,
		windat->accounts, windat->headings, windat->sorders);

	return TRUE;
}


/**
 * Purge unused components from a file.
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
	hourglass_on();

	/* Redraw the file now, so that the full extent of the original data
	 * is included in the redraw.
	 */

	file_redraw_windows(file);

#ifdef DEBUG
	debug_printf("\\OPurging file");
#endif

	/* Purge unused transactions from the file. */

	if (transactions)
		transact_purge(file, cutoff);

	/* Purge any unused standing orders from the file. */

	if (sorders)
		sorder_purge(file);

	/* Purge unused accounts and headings from the file. */

	if (accounts || headings)
		account_purge(file, accounts, headings);

	/* Recalculate the file and update the window. */

	accview_rebuild_all(file);

	/* account_recalculate_all(file); */

	*(file->filename) = '\0';
	list_window_rebuild_file_titles(file, TRUE);
	file_set_data_integrity(file, TRUE);

	/* Put the caret into the first empty line. */

	transact_scroll_window_to_end(file, TRANSACT_SCROLL_HOME);

	list_window_set_file_extent(file, TRUE);

	transact_place_caret(file, transact_find_first_blank_line(file), TRANSACT_FIELD_DATE);

	hourglass_off();
}

