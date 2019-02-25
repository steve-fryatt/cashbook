/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "sflib/errors.h"
#include "sflib/heap.h"

/* Application header files */

#include "global.h"
#include "goto.h"

#include "goto_dialogue.h"
#include "transact.h"


/**
 * Goto instance data.
 */

struct goto_block {
	/**
	 * The file owning the goto instance.
	 */
	struct file_block		*file;

	/**
	 * The most recent type of target held by the dialogue.
	 */
	enum goto_dialogue_type		type;

	/**
	 * The most recent target held by the dialogue.
	 */
	union goto_dialogue_target	target;
};

/* Static Function Prototypes. */

static osbool goto_process_window(void *owner, struct goto_dialogue_data *content);


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
	new->type = GOTO_DIALOGUE_TYPE_DATE;
	new->target.date = NULL_DATE;

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
	struct goto_dialogue_data *content = NULL;

	if (windat == NULL || ptr == NULL)
		return;

	content = heap_alloc(sizeof(struct goto_dialogue_data));
	if (content == NULL)
		return;

	content->type = windat->type;
	content->target = windat->target;

	goto_dialogue_open(ptr, restore, windat, windat->file, goto_process_window, content);
}


/**
 * Process the contents of the Goto window, store the details and
 * perform a goto operation.
 *
 * \param *owner		The goto instance currently owning the dialogue.
 * \param *content		The data from the dialogue which is to be processed.
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool goto_process_window(void *owner, struct goto_dialogue_data *content)
{
	int			transaction = 0, line = 0;
	struct goto_block	*windat = owner;

	if (windat == NULL || content == NULL)
		return FALSE;

	switch (content->type) {
	case GOTO_DIALOGUE_TYPE_LINE:
		if ((content->target.line <= 0) || (content->target.line > transact_get_count(windat->file))) {
			error_msgs_report_info("BadGotoLine");
			return FALSE;
		}

		windat->type = content->type;
		windat->target.line = content->target.line;

		line = transact_get_line_from_transaction(windat->file, transact_find_transaction_number(content->target.line));
		break;

	case GOTO_DIALOGUE_TYPE_DATE:
		if (content->target.date == NULL_DATE) {
			error_msgs_report_info("BadGotoDate");
			return FALSE;
		}

		windat->type = content->type;
		windat->target.date = content->target.date;

		transaction = transact_find_date(windat->file, content->target.date);

		if (transaction == NULL_TRANSACTION)
			return FALSE;

		line = transact_get_line_from_transaction(windat->file, transaction);
		break;
	}

	transact_place_caret(windat->file, line, TRANSACT_FIELD_DATE);

	return TRUE;
}

