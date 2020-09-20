/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: purge_dialogue.c
 *
 * High-level purge dialogue implementation.
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
#include "purge_dialogue.h"

#include "dialogue.h"

/* Dialogue Icons. */

#define PURGE_DIALOGUE_ICON_OK 6
#define PURGE_DIALOGUE_ICON_CANCEL 7

#define PURGE_DIALOGUE_ICON_TRANSACT 0
#define PURGE_DIALOGUE_ICON_ACCOUNTS 3
#define PURGE_DIALOGUE_ICON_HEADINGS 4
#define PURGE_DIALOGUE_ICON_SORDERS 5

#define PURGE_DIALOGUE_ICON_DATE 2
#define PURGE_DIALOGUE_ICON_DATETEXT 1

/* Global variables. */

/**
 * The handle of the Purge dialogue.
 */

static struct dialogue_block	*purge_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*purge_dialogue_callback)(void *, struct purge_dialogue_data *);


/* Static function prototypes. */

static void	purge_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool	purge_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void	purge_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Purge Dialogue Icon Set.
 */

static struct dialogue_icon purge_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,					PURGE_DIALOGUE_ICON_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,					PURGE_DIALOGUE_ICON_CANCEL,	DIALOGUE_NO_ICON},

	/* The transaction date fields. */

	{DIALOGUE_ICON_SHADE_TARGET,				PURGE_DIALOGUE_ICON_TRANSACT,	DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_REFRESH,	PURGE_DIALOGUE_ICON_DATE,	PURGE_DIALOGUE_ICON_TRANSACT},
	{DIALOGUE_ICON_SHADE_OFF,				PURGE_DIALOGUE_ICON_DATETEXT,	PURGE_DIALOGUE_ICON_TRANSACT},

	{DIALOGUE_ICON_END,					DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};

/**
 * The Purge Dialogue Definition.
 */

static struct dialogue_definition purge_dialogue_definition = {
	"Purge",
	"Purge",
	purge_dialogue_icon_list,
	DIALOGUE_GROUP_NONE,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	purge_dialogue_fill,
	purge_dialogue_process,
	purge_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the purge dialogue.
 */

void purge_dialogue_initialise(void)
{
	purge_dialogue = dialogue_create(&purge_dialogue_definition);
}

/**
 * Open the purge dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current dialogue content, otherwise FALSE
 * \param *owner		The purge dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void purge_dialogue_open(wimp_pointer *ptr, osbool restore, void *owner, struct file_block *file, osbool (*callback)(void *, struct purge_dialogue_data *),
		struct purge_dialogue_data *content)
{
	purge_dialogue_callback = callback;

	/* Open the window. */

	dialogue_open(purge_dialogue, restore, file, owner, ptr, content);
}

/**
 * Fill the Purge Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	Unused restore flag.
 * \param *data		Client data pointer (unused).
 */

static void purge_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct purge_dialogue_data *content = data;

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
 * Process OK clicks in the Purge Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dislogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool purge_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct purge_dialogue_data *content = data;

	if (purge_dialogue_callback == NULL || content == NULL || parent == NULL)
		return TRUE;

	/* Extract the information. */

	content->remove_transactions = icons_get_selected(window, PURGE_DIALOGUE_ICON_TRANSACT);
	content->remove_accounts = icons_get_selected(window, PURGE_DIALOGUE_ICON_ACCOUNTS);
	content->remove_headings = icons_get_selected(window, PURGE_DIALOGUE_ICON_HEADINGS);
	content->remove_sorders = icons_get_selected(window, PURGE_DIALOGUE_ICON_SORDERS);

	content->keep_from = date_convert_from_string(icons_get_indirected_text_addr(window, PURGE_DIALOGUE_ICON_DATE), NULL_DATE, 0);

	/* Call the client back. */

	return purge_dialogue_callback(parent, content);
}


/**
 * The Purge dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box being closed.
 * \param *data		Client data pointer, to the dislogue data structure.
 */

static void purge_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	purge_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

