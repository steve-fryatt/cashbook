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
 * \file: find_search_dialogue.c
 *
 * High-level find search dialogue implementation.
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
#include "find_search_dialogue.h"

#include "dialogue.h"

/* Dialogue Icons. */

#define FIND_SEARCH_DIALOGUE_ICON_OK 26
#define FIND_SEARCH_DIALOGUE_ICON_CANCEL 27

#define FIND_SEARCH_DIALOGUE_ICON_DATE 2
#define FIND_SEARCH_DIALOGUE_ICON_FMIDENT 4
#define FIND_SEARCH_DIALOGUE_ICON_FMREC 5
#define FIND_SEARCH_DIALOGUE_ICON_FMNAME 6
#define FIND_SEARCH_DIALOGUE_ICON_TOIDENT 8
#define FIND_SEARCH_DIALOGUE_ICON_TOREC 9
#define FIND_SEARCH_DIALOGUE_ICON_TONAME 10
#define FIND_SEARCH_DIALOGUE_ICON_REF 12
#define FIND_SEARCH_DIALOGUE_ICON_AMOUNT 14
#define FIND_SEARCH_DIALOGUE_ICON_DESC 16

#define FIND_SEARCH_DIALOGUE_ICON_AND 18
#define FIND_SEARCH_DIALOGUE_ICON_OR 19

#define FIND_SEARCH_DIALOGUE_ICON_CASE 17
#define FIND_SEARCH_DIALOGUE_ICON_WHOLE 28

#define FIND_SEARCH_DIALOGUE_ICON_START 22
#define FIND_SEARCH_DIALOGUE_ICON_DOWN 23
#define FIND_SEARCH_DIALOGUE_ICON_UP 25
#define FIND_SEARCH_DIALOGUE_ICON_END 24

/* Global variables. */

/**
 * The handle of the Find Search dialogue.
 */

static struct dialogue_block	*find_search_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*find_search_dialogue_callback)(void *, struct find_search_dialogue_data *);


/* Static function prototypes. */

static void	find_search_dialogue_fill(wimp_w window, osbool restore, void *data);
static osbool	find_search_dialogue_process(wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void	fins_search_dialogue_close(wimp_w window, void *data);

/**
 * The Fins Search Dialogue Icon Set.
 */

static struct dialogue_icon find_search_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,						FIND_SEARCH_DIALOGUE_ICON_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,						FIND_SEARCH_DIALOGUE_ICON_CANCEL,	DIALOGUE_NO_ICON},

	/* The search value fields. */

	{DIALOGUE_ICON_REFRESH,						FIND_SEARCH_DIALOGUE_ICON_DATE,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,						FIND_SEARCH_DIALOGUE_ICON_REF,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,						FIND_SEARCH_DIALOGUE_ICON_AMOUNT,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,						FIND_SEARCH_DIALOGUE_ICON_DESC,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_IDENT,		FIND_SEARCH_DIALOGUE_ICON_FMIDENT,	FIND_SEARCH_DIALOGUE_ICON_FMNAME},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_RECONCILE,	FIND_SEARCH_DIALOGUE_ICON_FMREC,	FIND_SEARCH_DIALOGUE_ICON_FMIDENT},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_NAME,		FIND_SEARCH_DIALOGUE_ICON_FMNAME,	FIND_SEARCH_DIALOGUE_ICON_FMREC},

	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_IDENT,		FIND_SEARCH_DIALOGUE_ICON_TOIDENT,	FIND_SEARCH_DIALOGUE_ICON_TONAME},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_RECONCILE,	FIND_SEARCH_DIALOGUE_ICON_TOREC,	FIND_SEARCH_DIALOGUE_ICON_TOIDENT},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_NAME,		FIND_SEARCH_DIALOGUE_ICON_TONAME,	FIND_SEARCH_DIALOGUE_ICON_TOREC},

	/* The search logic fields. */

	{DIALOGUE_ICON_RADIO,						FIND_SEARCH_DIALOGUE_ICON_AND,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,						FIND_SEARCH_DIALOGUE_ICON_OR,		DIALOGUE_NO_ICON},

	/* The search direction fields. */

	{DIALOGUE_ICON_RADIO,						FIND_SEARCH_DIALOGUE_ICON_START,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,						FIND_SEARCH_DIALOGUE_ICON_END,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,						FIND_SEARCH_DIALOGUE_ICON_UP,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,						FIND_SEARCH_DIALOGUE_ICON_DOWN,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,						DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Find Search Dialogue Definition.
 */

static struct dialogue_definition find_search_dialogue_definition = {
	"Find",
	"Find",
	find_search_dialogue_icon_list,
	DIALOGUE_ICON_NONE,
	find_search_dialogue_fill,
	find_search_dialogue_process,
	find_search_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the find search dialogue.
 */

void find_search_dialogue_initialise(void)
{
	find_search_dialogue = dialogue_create(&find_search_dialogue_definition);
}

/**
 * Open the find search dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The find dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void find_search_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(void *, struct find_search_dialogue_data *),
		struct find_search_dialogue_data *content)
{
	find_search_dialogue_callback = callback;

	/* Open the window. */

	dialogue_open(find_search_dialogue, FALSE, FALSE, file, owner, ptr, content);
}

/**
 * Fill the Find Search Dialogue with values.
 *
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	Unused restore flag.
 * \param *data		Client data pointer (unused).
 */

static void find_search_dialogue_fill(wimp_w window, osbool restore, void *data)
{
	struct find_search_dialogue_data *content = data;

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
 * Process OK clicks in the Find Search Dialogue.
 *
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dislogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool find_search_dialogue_process(wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct find_search_dialogue_data *content = data;

	if (find_search_dialogue_callback == NULL || content == NULL || parent == NULL)
		return TRUE;

	/* Extract the information. */

	content->remove_transactions = icons_get_selected(window, PURGE_DIALOGUE_ICON_TRANSACT);
	content->remove_accounts = icons_get_selected(window, PURGE_DIALOGUE_ICON_ACCOUNTS);
	content->remove_headings = icons_get_selected(window, PURGE_DIALOGUE_ICON_HEADINGS);
	content->remove_sorders = icons_get_selected(window, PURGE_DIALOGUE_ICON_SORDERS);

	content->keep_from = date_convert_from_string(icons_get_indirected_text_addr(window, PURGE_DIALOGUE_ICON_DATE), NULL_DATE, 0);

	/* Call the client back. */

	return find_search_dialogue_callback(parent, content);
}


/**
 * The Find Search dialogue has been closed.
 *
 * \param window	The handle of the dialogue box being closed.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void find_search_dialogue_close(wimp_w window, void *data)
{
	find_search_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

