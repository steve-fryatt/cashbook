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
 * \file: import_dialogue.c
 *
 * Import Complete dialogue implementation.
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
#include "import_dialogue.h"

#include "dialogue.h"

/* Window Icons. */

#define IMPORT_DIALOGUE_IMPORTED 0
#define IMPORT_DIALOGUE_REJECTED 2
#define IMPORT_DIALOGUE_CLOSE 5
#define IMPORT_DIALOGUE_VIEW_LOG 4

/* Global Variables */

/**
 * The handle of the Import Complete dialogue.
 */

static struct dialogue_block	*import_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*import_dialogue_callback)(void *, struct import_dialogue_data *);

/* Static Function Prototypes. */

static void import_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool import_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void import_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Import Complete Dialogue Icon Set.
 */

static struct dialogue_icon import_dialogue_icon_list[] = {
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_IMPORT_CLOSE,	IMPORT_DIALOGUE_CLOSE,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_IMPORT_VIEW_LOG,	IMPORT_DIALOGUE_VIEW_LOG,	DIALOGUE_NO_ICON},

	/* The imported and rejected fields. */

	{DIALOGUE_ICON_REFRESH,					IMPORT_DIALOGUE_IMPORTED,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,					IMPORT_DIALOGUE_REJECTED,	DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,					DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};


/**
 * The Import Complete Dialogue Definition.
 */

static struct dialogue_definition import_dialogue_definition = {
	"ImpComp",
	"ImpComp",
	import_dialogue_icon_list,
	DIALOGUE_GROUP_IMPORT,
	DIALOGUE_FLAGS_NONE,
	import_dialogue_fill,
	import_dialogue_process,
	import_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the Import Complete dialogue.
 */

void import_dialogue_initialise(void)
{
	import_dialogue = dialogue_create(&import_dialogue_definition);
}


/**
 * Open the Import Complete dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void import_dialogue_open(wimp_pointer *ptr, struct file_block *file,
		osbool (*callback)(void *, struct import_dialogue_data *), struct import_dialogue_data *content)
{
	if (content == NULL)
		return;

	import_dialogue_callback = callback;

	/* Open the window. */

	dialogue_open(import_dialogue, FALSE, file, file, ptr, content);
}


/**
 * Fill the Import Complete Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void import_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct import_dialogue_data *content = data;

	if (content == NULL)
		return;

	/* Set start date. */

	icons_printf(window, IMPORT_DIALOGUE_IMPORTED, "%d", content->imported);
	icons_printf(window, IMPORT_DIALOGUE_REJECTED, "%d", content->rejected);
}


/**
 * Process OK clicks in the Import Complete Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dialogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool import_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct import_dialogue_data	*content = data;

	if (content == NULL || import_dialogue_callback == NULL)
		return FALSE;

	if (type & DIALOGUE_ICON_IMPORT_CLOSE)
		content->action = IMPORT_DIALOGUE_ACTION_CLOSE;
	else if (type & DIALOGUE_ICON_IMPORT_VIEW_LOG)
		content->action = IMPORT_DIALOGUE_ACTION_VIEW_REPORT;

	/* Call the client back. */

	return import_dialogue_callback(parent, content);
}


/**
 * The Import Complete dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void import_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	import_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

