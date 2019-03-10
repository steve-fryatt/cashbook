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
 * \file: find_result_dialogue.c
 *
 * High-level find results dialogue implementation.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "find_result_dialogue.h"

#include "dialogue.h"

#define FIND_RESULT_DIALOGUE_MESSAGE_LENGTH 64

/* Dialogue Icons. */

#define FIND_RESULT_DIALOGUE_ICON_CANCEL 1
#define FIND_RESULT_DIALOGUE_ICON_PREVIOUS 0
#define FIND_RESULT_DIALOGUE_ICON_NEXT 2
#define FIND_RESULT_DIALOGUE_ICON_NEW 3
#define FIND_RESULT_DIALOGUE_ICON_INFO 4

/* Global variables. */

/**
 * The handle of the Find Results dialogue.
 */

static struct dialogue_block	*find_result_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*find_result_dialogue_callback)(wimp_pointer *, void *, struct find_result_dialogue_data *);


/* Static function prototypes. */

static void	find_result_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool	find_result_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void	find_result_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Find Results Dialogue Icon Set.
 */

static struct dialogue_icon find_result_dialogue_icon_list[] = {
	{DIALOGUE_ICON_CANCEL,					FIND_RESULT_DIALOGUE_ICON_CANCEL,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_FIND_PREVIOUS,	FIND_RESULT_DIALOGUE_ICON_PREVIOUS,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_FIND_NEXT,	FIND_RESULT_DIALOGUE_ICON_NEXT,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_FIND_NEW,		FIND_RESULT_DIALOGUE_ICON_NEW,		DIALOGUE_NO_ICON},

	/* The found info fields. */

	{DIALOGUE_ICON_SHADE_TARGET,				FIND_RESULT_DIALOGUE_ICON_INFO,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,					DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Find Results Dialogue Definition.
 */

static struct dialogue_definition find_result_dialogue_definition = {
	"Found",
	"Found",
	find_result_dialogue_icon_list,
	DIALOGUE_GROUP_FIND,
	DIALOGUE_FLAGS_NONE,
	find_result_dialogue_fill,
	find_result_dialogue_process,
	find_result_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the find results dialogue.
 */

void find_result_dialogue_initialise(void)
{
	find_result_dialogue = dialogue_create(&find_result_dialogue_definition);
}

/**
 * Open the find result dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The find dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void find_result_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(wimp_pointer *, void *, struct find_result_dialogue_data *),
		struct find_result_dialogue_data *content)
{
	find_result_dialogue_callback = callback;

	/* Reset the action, ready for the next dialogue cycle. */

	if (content != NULL)
		content->action = FIND_RESULT_DIALOGUE_NONE;

	/* Open the window. */

	dialogue_open(find_result_dialogue, TRUE, file, owner, ptr, content);
}

/**
 * Fill the Find Result Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	Unused restore flag.
 * \param *data		Client data pointer (unused).
 */

static void find_result_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct find_result_dialogue_data	*content = data;
	char					buf1[FIND_RESULT_DIALOGUE_MESSAGE_LENGTH], buf2[FIND_RESULT_DIALOGUE_MESSAGE_LENGTH];

	if (content == NULL)
		return;

	if (restore) {
		transact_get_column_name(file, content->result, buf1, FIND_RESULT_DIALOGUE_MESSAGE_LENGTH);
		string_printf(buf2, FIND_RESULT_DIALOGUE_MESSAGE_LENGTH, "%d", transact_get_transaction_number(content->transaction));

		icons_msgs_param_lookup(window, FIND_RESULT_DIALOGUE_ICON_INFO, "Found", buf1, buf2, NULL, NULL);
	} else {
		*icons_get_indirected_text_addr(window, FIND_RESULT_DIALOGUE_ICON_INFO) = '\0';
	}
}

/**
 * Process OK clicks in the Find Result Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent find instance.
 * \param *data		Client data pointer, to the dislogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool find_result_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct find_result_dialogue_data *content = data;

	if (find_result_dialogue_callback == NULL || content == NULL || parent == NULL)
		return TRUE;

	/* Extract the information. */

	if ((type & DIALOGUE_ICON_FIND_PREVIOUS) && (pointer->buttons == wimp_CLICK_SELECT))
		content->action = FIND_RESULT_DIALOGUE_PREVIOUS;
	else if ((type & DIALOGUE_ICON_FIND_NEXT) && (pointer->buttons == wimp_CLICK_SELECT))
		content->action = FIND_RESULT_DIALOGUE_NEXT;
	else if ((type & DIALOGUE_ICON_FIND_NEW) && (pointer->buttons == wimp_CLICK_SELECT))
		content->action = FIND_RESULT_DIALOGUE_NEW;
	else
		content->action = FIND_RESULT_DIALOGUE_NONE;

	/* Call the client back. */

	return find_result_dialogue_callback(pointer, parent, content);
}


/**
 * The Find Result dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box being closed.
 * \param *data		Client data pointer, to the dislogue data structure.
 */

static void find_result_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	struct find_result_dialogue_data *content = data;

	find_result_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use, if the
	 * selected dialogue action was Cancel.
	 */

	if ((content != NULL) && (content->action == FIND_RESULT_DIALOGUE_NONE || content->action == FIND_RESULT_DIALOGUE_NEW))
		heap_free(content);
}

