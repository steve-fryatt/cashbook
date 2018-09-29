/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "goto_dialogue.h"
#include "transact.h"


/* Goto dialogue data. */


struct goto_block {
	struct file_block		*file;					/**< The file owning the goto dialogue.				*/
	union goto_dialogue_target	target;					/**< The current goto target.					*/
	enum goto_dialogue_type		type;					/**< The type of target we're aiming for.			*/
};


//static struct goto_block	*goto_window_owner = NULL;			/**< The file whoch currently owns the Goto dialogue.	*/
//static osbool			goto_restore = FALSE;				/**< The restore setting for the current dialogue.	*/
//static wimp_w			goto_window = NULL;				/**< The Goto dialogue handle.				*/


static osbool goto_process_window(void *owner, enum goto_dialogue_type type, int line, date_t date);


/**
 * Initialise the Goto module.
 */

void goto_initialise(void)
{
	goto_dialogue_initialise();
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

	goto_dialogue_open(ptr, restore, windat, windat->file, goto_process_window,
			windat->type, windat->target.line, windat->target.date);
}


/**
 * Process the contents of the Goto window, store the details and
 * perform a goto operation.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool goto_process_window(void *owner, enum goto_dialogue_type type, int transaction, date_t date)
{
	int			line = 0;
	struct goto_block	*windat = owner;

	if (windat == NULL)
		return FALSE;

	windat->type = type;

	switch (type) {
	case GOTO_TYPE_LINE:
		windat->target.line = transaction;

		if (transaction > transact_get_count(windat->file)) {
			error_msgs_report_info("BadGotoLine");
			return FALSE;
		}

		line = transact_get_line_from_transaction(windat->file, transact_find_transaction_number(windat->target.line));
		break;

	case GOTO_TYPE_DATE:
		windat->target.date = date;

		if (date == NULL_DATE) {
			error_msgs_report_info("BadGotoDate");
			return FALSE;
		}

		transaction = transact_find_date(windat->file, windat->target.date);

		if (transaction == NULL_TRANSACTION)
			return FALSE;

		line = transact_get_line_from_transaction(windat->file, transaction);
		break;
	}

	transact_place_caret(windat->file, line, TRANSACT_FIELD_DATE);

	return TRUE;
}

