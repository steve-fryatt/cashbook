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
 * \file: account_section_dialogue.c
 *
 * Account List Section Edit dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "account_section_dialogue.h"

#include "account_list_window.h"
#include "dialogue.h"

/* Window Icon Details. */

#define ACCOUNT_SECTION_DIALOGUE_OK 2
#define ACCOUNT_SECTION_DIALOGUE_CANCEL 3
#define ACCOUNT_SECTION_DIALOGUE_DELETE 4

#define ACCOUNT_SECTION_DIALOGUE_TITLE 0
#define ACCOUNT_SECTION_DIALOGUE_HEADER 5
#define ACCOUNT_SECTION_DIALOGUE_FOOTER 6


/* Global Variables */

/**
 * The handle of the Section Edit dialogue.
 */

static struct dialogue_block	*account_section_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*account_section_dialogue_callback)(void *, struct account_section_dialogue_data *);

/* Static Function Prototypes. */

static void account_section_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool account_section_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void account_section_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Section Edit Dialogue Icon Set.
 */

static struct dialogue_icon account_section_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,					ACCOUNT_SECTION_DIALOGUE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,					ACCOUNT_SECTION_DIALOGUE_CANCEL,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_EDIT_DELETE,	ACCOUNT_SECTION_DIALOGUE_DELETE,	DIALOGUE_NO_ICON},

	/* The title field. */

	{DIALOGUE_ICON_REFRESH,					ACCOUNT_SECTION_DIALOGUE_TITLE,		DIALOGUE_NO_ICON},

	/* The heading type icons. */

	{DIALOGUE_ICON_RADIO,					ACCOUNT_SECTION_DIALOGUE_HEADER,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,					ACCOUNT_SECTION_DIALOGUE_FOOTER,	DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,					DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Section Edit Dialogue Definition.
 */

static struct dialogue_definition account_section_dialogue_definition = {
	"EditAccSect",
	"EditAccSect",
	account_section_dialogue_icon_list,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	account_section_dialogue_fill,
	account_section_dialogue_process,
	account_section_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the account section edit dialogue.
 */

void account_section_dialogue_initialise(void)
{
	account_section_dialogue = dialogue_create(&account_section_dialogue_definition);
}


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void account_section_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*callback)(void *, struct account_section_dialogue_data *), struct account_section_dialogue_data *content)
{
	if (content == NULL)
		return;

	account_section_dialogue_callback = callback;

	/* Set up the dialogue title and action buttons. */

	if (content->line == -1) {
		dialogue_set_title(account_section_dialogue, "NewSect", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(account_section_dialogue, DIALOGUE_ICON_OK, "NewAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(account_section_dialogue, DIALOGUE_ICON_EDIT_DELETE, TRUE);
	} else {
		dialogue_set_title(account_section_dialogue, "EditSect", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(account_section_dialogue, DIALOGUE_ICON_OK, "EditAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(account_section_dialogue, DIALOGUE_ICON_EDIT_DELETE, FALSE);
	}

	/* Open the window. */

	dialogue_open(account_section_dialogue, FALSE, file, owner, ptr, content);
}


/**
 * Fill the Section Edit Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void account_section_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct account_section_dialogue_data *content = data;

	if (content == NULL)
		return;

	icons_strncpy(window, ACCOUNT_SECTION_DIALOGUE_TITLE, content->name);

	icons_set_selected(window, ACCOUNT_SECTION_DIALOGUE_HEADER,
			(content->type == ACCOUNT_LINE_HEADER));
	icons_set_selected(window, ACCOUNT_SECTION_DIALOGUE_FOOTER,
			(content->type == ACCOUNT_LINE_FOOTER));
}


/**
 * Process OK clicks in the Section Edit Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dialogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool account_section_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct account_section_dialogue_data *content = data;

	if (content == NULL || account_section_dialogue_callback == NULL)
		return FALSE;

	/* Extract the information from the dialogue. */

	if (type & DIALOGUE_ICON_OK)
		content->action = ACCOUNT_SECTION_ACTION_OK;
	else if (type & DIALOGUE_ICON_EDIT_DELETE)
		content->action = ACCOUNT_SECTION_ACTION_DELETE;

	icons_copy_text(window, ACCOUNT_SECTION_DIALOGUE_TITLE, content->name, ACCOUNT_SECTION_LEN);

	if (icons_get_selected(window, ACCOUNT_SECTION_DIALOGUE_HEADER))
		content->type = ACCOUNT_LINE_HEADER;
	else if (icons_get_selected(window, ACCOUNT_SECTION_DIALOGUE_FOOTER))
		content->type = ACCOUNT_LINE_FOOTER;
	else
		content->type = ACCOUNT_LINE_BLANK;

	/* Call the client back. */

	return account_section_dialogue_callback(parent, content);
}


/**
 * The Edit Section dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void account_section_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	account_section_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

