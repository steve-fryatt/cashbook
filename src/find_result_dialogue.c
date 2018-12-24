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

#include "sflib/heap.h"
#include "sflib/icons.h"

/* Application header files */

#include "global.h"
#include "find_result_dialogue.h"

#include "dialogue.h"

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

static osbool			(*find_result_dialogue_callback)(void *, struct find_result_dialogue_data *);


/* Static function prototypes. */

static void	find_result_dialogue_fill(wimp_w window, osbool restore, void *data);
static osbool	find_result_dialogue_process(wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void	find_result_dialogue_close(wimp_w window, void *data);

/**
 * The Find Results Dialogue Icon Set.
 */

static struct dialogue_icon find_result_dialogue_icon_list[] = {
	{DIALOGUE_ICON_CANCEL,		FIND_RESULT_DIALOGUE_ICON_CANCEL,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_FIND_PREVIOUS,	FIND_RESULT_DIALOGUE_ICON_PREVIOUS,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_FIND_NEXT,	FIND_RESULT_DIALOGUE_ICON_NEXT,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_FIND_NEW,	FIND_RESULT_DIALOGUE_ICON_NEW,		DIALOGUE_NO_ICON},

	/* The found info fields. */

	{DIALOGUE_ICON_SHADE_TARGET,	FIND_RESULT_DIALOGUE_ICON_INFO,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,		DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Find Results Dialogue Definition.
 */

static struct dialogue_definition find_result_dialogue_definition = {
	"Found",
	"Found",
	find_result_dialogue_icon_list,
	DIALOGUE_ICON_NONE,
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

void find_result_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(void *, struct find_result_dialogue_data *),
		struct find_result_dialogue_data *content)
{
	find_result_dialogue_callback = callback;

	/* Open the window. */

	dialogue_open(find_result_dialogue, FALSE, FALSE, file, owner, ptr, content);
}

/**
 * Fill the Find Result Dialogue with values.
 *
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	Unused restore flag.
 * \param *data		Client data pointer (unused).
 */

static void find_result_dialogue_fill(wimp_w window, osbool restore, void *data)
{
	struct find_result_dialogue_data *content = data;

	if (content == NULL)
		return;

	if (restore) {
		icons_set_selected(window, PURGE_DIALOGUE_ICON_TRANSACT, content->remove_transactions);
		icons_set_selected(window, PURGE_DIALOGUE_ICON_ACCOUNTS, content->remove_accounts);
		icons_set_selected(window, PURGE_DIALOGUE_ICON_HEADINGS, content->remove_headings);
		icons_set_selected(window, PURGE_DIALOGUE_ICON_SORDERS, content->remove_sorders);

		date_convert_to_string(content->keep_from, icons_get_indirected_text_addr(window, PURGE_DIALOGUE_ICON_DATE),
				icons_get_indirected_text_length(window, PURGE_DIALOGUE_ICON_DATE));
	} else {
		icons_set_selected(window, PURGE_DIALOGUE_ICON_TRANSACT, TRUE);
		icons_set_selected(window, PURGE_DIALOGUE_ICON_ACCOUNTS, FALSE);
		icons_set_selected(window, PURGE_DIALOGUE_ICON_HEADINGS, FALSE);
		icons_set_selected(window, PURGE_DIALOGUE_ICON_SORDERS, FALSE);

		*icons_get_indirected_text_addr(window, PURGE_DIALOGUE_ICON_DATE) = '\0';
	}
}

/**
 * Process OK clicks in the Find Result Dialogue.
 *
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dislogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool find_result_dialogue_process(wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct find_result_dialogue_data *content = data;

	if (find_result_dialogue_callback == NULL || content == NULL || parent == NULL)
		return TRUE;

	/* Extract the information. */

	content->remove_transactions = icons_get_selected(window, PURGE_DIALOGUE_ICON_TRANSACT);
	content->remove_accounts = icons_get_selected(window, PURGE_DIALOGUE_ICON_ACCOUNTS);
	content->remove_headings = icons_get_selected(window, PURGE_DIALOGUE_ICON_HEADINGS);
	content->remove_sorders = icons_get_selected(window, PURGE_DIALOGUE_ICON_SORDERS);

	content->keep_from = date_convert_from_string(icons_get_indirected_text_addr(window, PURGE_DIALOGUE_ICON_DATE), NULL_DATE, 0);

	/* Call the client back. */

	return find_result_dialogue_callback(parent, content);
}


/**
 * The Find Result dialogue has been closed.
 *
 * \param window	The handle of the dialogue box being closed.
 * \param *data		Client data pointer, to the dislogue data structure.
 */

static void find_result_dialogue_close(wimp_w window, void *data)
{
	find_result_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

