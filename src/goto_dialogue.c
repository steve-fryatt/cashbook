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
 * \file: goto_dialogue.c
 *
 * High-level Goto dialogue implementation.
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
#include "goto_dialogue.h"

#include "dialogue.h"

/* Dialogue Icons. */

#define GOTO_DIALOGUE_ICON_OK 0
#define GOTO_DIALOGUE_ICON_CANCEL 1
#define GOTO_DIALOGUE_ICON_NUMBER_FIELD 3
#define GOTO_DIALOGUE_ICON_NUMBER 4
#define GOTO_DIALOGUE_ICON_DATE 5

/* Global variables. */

/**
 * The handle of the Goto dialogue.
 */

static struct dialogue_block	*goto_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*goto_dialogue_callback)(void *, struct goto_dialogue_data *);


/* Static function prototypes. */

static void	goto_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool	goto_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void	goto_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Goto Dialogue Icon Set.
 */

static struct dialogue_icon goto_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,	GOTO_DIALOGUE_ICON_OK,			DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,	GOTO_DIALOGUE_ICON_CANCEL,		DIALOGUE_NO_ICON},

	/* The number field. */

	{DIALOGUE_ICON_REFRESH,	GOTO_DIALOGUE_ICON_NUMBER_FIELD,	DIALOGUE_NO_ICON},

	/* The mode radio icons. */

	{DIALOGUE_ICON_RADIO,	GOTO_DIALOGUE_ICON_NUMBER,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,	GOTO_DIALOGUE_ICON_DATE,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,	DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Goto Dialogue Definition.
 */

static struct dialogue_definition goto_dialogue_definition = {
	"Goto",
	"Goto",
	goto_dialogue_icon_list,
	DIALOGUE_ICON_NONE,
	goto_dialogue_fill,
	goto_dialogue_process,
	goto_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the goto dialogue.
 */

void goto_dialogue_initialise(void)
{
	goto_dialogue = dialogue_create(&goto_dialogue_definition);
}

/**
 * Open the Goto dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current dialogue content, otherwise FALSE
 * \param *owner		The goto dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void goto_dialogue_open(wimp_pointer *ptr, osbool restore, void *owner, struct file_block *file, osbool (*callback)(void *, struct goto_dialogue_data *),
		struct goto_dialogue_data *content)
{
	goto_dialogue_callback = callback;

	/* Open the window. */

	dialogue_open(goto_dialogue, FALSE, restore, file, owner, ptr, content);
}

/**
 * Fill the Goto Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer (unused).
 */

static void goto_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct goto_dialogue_data *content = data;

	if (content == NULL)
		return;

	if (restore) {
		switch (content->type) {
		case GOTO_DIALOGUE_TYPE_LINE:
			icons_printf(window, GOTO_DIALOGUE_ICON_NUMBER_FIELD, "%d", content->target.line);
			break;
		
		case GOTO_DIALOGUE_TYPE_DATE:
			date_convert_to_string(content->target.date, icons_get_indirected_text_addr(window, GOTO_DIALOGUE_ICON_NUMBER_FIELD),
					icons_get_indirected_text_length(window, GOTO_DIALOGUE_ICON_NUMBER_FIELD));
			break;
		}

		icons_set_selected(window, GOTO_DIALOGUE_ICON_NUMBER, content->type == GOTO_DIALOGUE_TYPE_LINE);
		icons_set_selected(window, GOTO_DIALOGUE_ICON_DATE, content->type == GOTO_DIALOGUE_TYPE_DATE);
	} else {
		*icons_get_indirected_text_addr(window, GOTO_DIALOGUE_ICON_NUMBER_FIELD) = '\0';

		icons_set_selected(window, GOTO_DIALOGUE_ICON_NUMBER, FALSE);
		icons_set_selected(window, GOTO_DIALOGUE_ICON_DATE, TRUE);
	}
}


/**
 * Process OK clicks in the Goto Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dislogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool goto_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct goto_dialogue_data *content = data;

	if (goto_dialogue_callback == NULL || content == NULL || parent == NULL)
		return TRUE;

	/* Extract the information. */

	content->type = (icons_get_selected(window, GOTO_DIALOGUE_ICON_DATE)) ? GOTO_DIALOGUE_TYPE_DATE : GOTO_DIALOGUE_TYPE_LINE;

	switch (content->type) {
	case GOTO_DIALOGUE_TYPE_LINE:
		content->target.line = atoi(icons_get_indirected_text_addr(window, GOTO_DIALOGUE_ICON_NUMBER_FIELD));
		break;

	case GOTO_DIALOGUE_TYPE_DATE:
		content->target.date = date_convert_from_string(icons_get_indirected_text_addr(window, GOTO_DIALOGUE_ICON_NUMBER_FIELD), NULL_DATE, 0);
		break;
	}

	/* Call the client back. */

	return goto_dialogue_callback(parent, content);
}


/**
 * The Goto dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dislogue data structure.
 */

static void goto_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	goto_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

