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
 * \file: budget_dialogue.c
 *
 * High-level budget dialogue implementation.
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
#include "budget_dialogue.h"

#include "dialogue.h"

/* Dialogue Icons. */

#define BUDGET_DIALOGUE_ICON_OK 0
#define BUDGET_DIALOGUE_ICON_CANCEL 1
#define BUDGET_DIALOGUE_ICON_START 5
#define BUDGET_DIALOGUE_ICON_FINISH 7
#define BUDGET_DIALOGUE_ICON_TRIAL 11
#define BUDGET_DIALOGUE_ICON_RESTRICT 13

/* Global variables. */

/**
 * The handle of the Budget dialogue.
 */

static struct dialogue_block	*budget_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*budget_dialogue_callback)(void *, struct budget_dialogue_data *);


/* Static function prototypes. */

static void	budget_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool	budget_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void	budget_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Budget Dialogue Icon Set.
 */

static struct dialogue_icon budget_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,	BUDGET_DIALOGUE_ICON_OK,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,	BUDGET_DIALOGUE_ICON_CANCEL,	DIALOGUE_NO_ICON},

	/* The date fields. */

	{DIALOGUE_ICON_REFRESH,	BUDGET_DIALOGUE_ICON_START,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,	BUDGET_DIALOGUE_ICON_FINISH,	DIALOGUE_NO_ICON},

	/* The trial field. */

	{DIALOGUE_ICON_REFRESH,	BUDGET_DIALOGUE_ICON_TRIAL,	DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,	DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};

/**
 * The Budget Dialogue Definition.
 */

static struct dialogue_definition budget_dialogue_definition = {
	"Budget",
	"Budget",
	budget_dialogue_icon_list,
	DIALOGUE_GROUP_NONE,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	budget_dialogue_fill,
	budget_dialogue_process,
	budget_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the budget dialogue.
 */

void budget_dialogue_initialise(void)
{
	budget_dialogue = dialogue_create(&budget_dialogue_definition);
}

/**
 * Open the budget dialogue for a given transaction window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The budget dialogue instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *content		Pointer to a structure to hold the dialogue content.
 */

void budget_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file, osbool (*callback)(void *, struct budget_dialogue_data *),
		struct budget_dialogue_data *content)
{
	budget_dialogue_callback = callback;

	/* Open the window. */

	dialogue_open(budget_dialogue, FALSE, file, owner, ptr, content);
}

/**
 * Fill the Budget Dialogue with values.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	Unused restore flag.
 * \param *data		Client data pointer (unused).
 */

static void budget_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct budget_dialogue_data *content = data;

	if (content == NULL)
		return;

	date_convert_to_string(content->start, icons_get_indirected_text_addr(window, BUDGET_DIALOGUE_ICON_START),
			icons_get_indirected_text_length(window, BUDGET_DIALOGUE_ICON_START));
	date_convert_to_string(content->finish, icons_get_indirected_text_addr(window, BUDGET_DIALOGUE_ICON_FINISH),
			icons_get_indirected_text_length(window, BUDGET_DIALOGUE_ICON_FINISH));

	icons_printf(window, BUDGET_DIALOGUE_ICON_TRIAL, "%d", content->sorder_trial);

	icons_set_selected(window, BUDGET_DIALOGUE_ICON_RESTRICT, content->limit_postdate);
}


/**
 * Process OK clicks in the Budget Dialogue.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dislogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool budget_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct budget_dialogue_data *content = data;

	if (budget_dialogue_callback == NULL || content == NULL || parent == NULL)
		return TRUE;

	/* Extract the information. */

	content->start = date_convert_from_string(icons_get_indirected_text_addr(window, BUDGET_DIALOGUE_ICON_START), NULL_DATE, 0);
	content->finish = date_convert_from_string(icons_get_indirected_text_addr(window, BUDGET_DIALOGUE_ICON_FINISH), NULL_DATE, 0);

	content->sorder_trial = atoi(icons_get_indirected_text_addr(window, BUDGET_DIALOGUE_ICON_TRIAL));

	content->limit_postdate = icons_get_selected(window, BUDGET_DIALOGUE_ICON_RESTRICT);

	/* Call the client back. */

	return budget_dialogue_callback(parent, content);
}


/**
 * The Budget dialogue has been closed.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dislogue data structure.
 */

static void budget_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	budget_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

