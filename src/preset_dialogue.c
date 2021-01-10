/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: preset_dialogue.c
 *
 * Preset Edit dialogue implementation.
 */

/* ANSI C header files */

#include <ctype.h>
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
#include "preset_dialogue.h"

//#include "account.h"
#include "dialogue.h"

/* Window Icons. */

#define PRESET_DIALOGUE_OK          0
#define PRESET_DIALOGUE_CANCEL      1
#define PRESET_DIALOGUE_DELETE      2

#define PRESET_DIALOGUE_NAME        4
#define PRESET_DIALOGUE_KEY         6
#define PRESET_DIALOGUE_DATE        10
#define PRESET_DIALOGUE_TODAY       11
#define PRESET_DIALOGUE_FMIDENT     14
#define PRESET_DIALOGUE_FMREC       15
#define PRESET_DIALOGUE_FMNAME      16
#define PRESET_DIALOGUE_TOIDENT     19
#define PRESET_DIALOGUE_TOREC       20
#define PRESET_DIALOGUE_TONAME      21
#define PRESET_DIALOGUE_REF         24
#define PRESET_DIALOGUE_CHEQUE      25
#define PRESET_DIALOGUE_AMOUNT      28
#define PRESET_DIALOGUE_DESC        31
#define PRESET_DIALOGUE_CARETDATE   12
#define PRESET_DIALOGUE_CARETFROM   17
#define PRESET_DIALOGUE_CARETTO     22
#define PRESET_DIALOGUE_CARETREF    26
#define PRESET_DIALOGUE_CARETAMOUNT 29
#define PRESET_DIALOGUE_CARETDESC   32


/* Global Variables */

/**
 * The handle of the Preset dialogue.
 */

static struct dialogue_block	*preset_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*preset_dialogue_callback)(void *, struct preset_dialogue_data *);

/* Static Function Prototypes. */

static void preset_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool preset_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void preset_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Preset Dialogue Icon Set.
 */

static struct dialogue_icon preset_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,									PRESET_DIALOGUE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,									PRESET_DIALOGUE_CANCEL	,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_EDIT_DELETE,					PRESET_DIALOGUE_DELETE,		DIALOGUE_NO_ICON},

	/* The name and key fields. */

	{DIALOGUE_ICON_REFRESH,									PRESET_DIALOGUE_NAME,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									PRESET_DIALOGUE_KEY,		DIALOGUE_NO_ICON},

	/* The caret target icons. */

	{DIALOGUE_ICON_RADIO,									PRESET_DIALOGUE_CARETDATE,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									PRESET_DIALOGUE_CARETFROM,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									PRESET_DIALOGUE_CARETTO,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									PRESET_DIALOGUE_CARETREF,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									PRESET_DIALOGUE_CARETAMOUNT,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									PRESET_DIALOGUE_CARETDESC,	DIALOGUE_NO_ICON},

	/* The details fields. */

	{DIALOGUE_ICON_SHADE_TARGET,								PRESET_DIALOGUE_TODAY,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_SHADE_ON,					PRESET_DIALOGUE_DATE,		PRESET_DIALOGUE_TODAY},

	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_IDENT | DIALOGUE_ICON_TYPE_FROM,		PRESET_DIALOGUE_FMIDENT,	PRESET_DIALOGUE_FMNAME},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_RECONCILE | DIALOGUE_ICON_TYPE_FROM,	PRESET_DIALOGUE_FMREC,		PRESET_DIALOGUE_FMIDENT},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_NAME | DIALOGUE_ICON_TYPE_FROM,		PRESET_DIALOGUE_FMNAME,		PRESET_DIALOGUE_FMREC},

	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_IDENT | DIALOGUE_ICON_TYPE_TO,		PRESET_DIALOGUE_TOIDENT,	PRESET_DIALOGUE_TONAME},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_RECONCILE | DIALOGUE_ICON_TYPE_TO,	PRESET_DIALOGUE_TOREC,		PRESET_DIALOGUE_TOIDENT},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_NAME | DIALOGUE_ICON_TYPE_TO,		PRESET_DIALOGUE_TONAME,		PRESET_DIALOGUE_TOREC},

	{DIALOGUE_ICON_SHADE_TARGET,								PRESET_DIALOGUE_CHEQUE,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_SHADE_ON,					PRESET_DIALOGUE_REF,		PRESET_DIALOGUE_CHEQUE},

	{DIALOGUE_ICON_REFRESH,									PRESET_DIALOGUE_AMOUNT,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_REFRESH,									PRESET_DIALOGUE_DESC,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,									DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};


/**
 * The Preset Dialogue Definition.
 */

static struct dialogue_definition preset_dialogue_definition = {
	"EditPreset",
	"EditPreset",
	preset_dialogue_icon_list,
	DIALOGUE_GROUP_NONE,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	preset_dialogue_fill,
	preset_dialogue_process,
	preset_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the Preset dialogue.
 */

void preset_dialogue_initialise(void)
{
	preset_dialogue = dialogue_create(&preset_dialogue_definition);
}


/**
 * Open the Preset dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void preset_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*callback)(void *, struct preset_dialogue_data *), struct preset_dialogue_data *content)
{
	if (content == NULL)
		return;

	preset_dialogue_callback = callback;

	/* Set up the dialogue title and action buttons. */

	if (content->preset == NULL_PRESET) {
		dialogue_set_title(preset_dialogue, "NewPreset", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(preset_dialogue, DIALOGUE_ICON_OK, "NewAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(preset_dialogue, DIALOGUE_ICON_EDIT_DELETE, TRUE);
	} else {
		dialogue_set_title(preset_dialogue, "EditPreset", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(preset_dialogue, DIALOGUE_ICON_OK, "EditAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(preset_dialogue, DIALOGUE_ICON_EDIT_DELETE, FALSE);
	}

	/* Open the window. */

	dialogue_open(preset_dialogue, FALSE, file, owner, ptr, content);
}


/**
 * Fill the Preset Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void preset_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct preset_dialogue_data *content = data;

	if (content == NULL)
		return;

	/* Set name and key. */

	icons_strncpy(window, PRESET_DIALOGUE_NAME, content->name);
	icons_printf(window, PRESET_DIALOGUE_KEY, "%c", content->action_key);

	/* Set date. */

	date_convert_to_string(content->date,
			icons_get_indirected_text_addr(window, PRESET_DIALOGUE_DATE),
			icons_get_indirected_text_length(window, PRESET_DIALOGUE_DATE));
	icons_set_selected(window, PRESET_DIALOGUE_TODAY, content->flags & TRANS_TAKE_TODAY);
	icons_set_shaded(window, PRESET_DIALOGUE_DATE, content->flags & TRANS_TAKE_TODAY);

	/* Fill in the from and to fields. */

	account_fill_field(file, content->from, content->flags & TRANS_REC_FROM,
			window, PRESET_DIALOGUE_FMIDENT, PRESET_DIALOGUE_FMNAME, PRESET_DIALOGUE_FMREC);

	account_fill_field(file, content->to, content->flags & TRANS_REC_TO,
			window, PRESET_DIALOGUE_TOIDENT, PRESET_DIALOGUE_TONAME, PRESET_DIALOGUE_TOREC);

	/* Fill in the reference field. */

	icons_strncpy(window, PRESET_DIALOGUE_REF, content->reference);
	icons_set_selected(window, PRESET_DIALOGUE_CHEQUE, content->flags & TRANS_TAKE_CHEQUE);
	icons_set_shaded(window, PRESET_DIALOGUE_REF, content->flags & TRANS_TAKE_CHEQUE);

	/* Fill in the amount fields. */

	currency_convert_to_string(content->amount,
			icons_get_indirected_text_addr(window, PRESET_DIALOGUE_AMOUNT),
			icons_get_indirected_text_length(window, PRESET_DIALOGUE_AMOUNT));

	/* Fill in the description field. */

	icons_strncpy(window, PRESET_DIALOGUE_DESC, content->description);

	/* Set the caret location icons. */

	icons_set_selected(window, PRESET_DIALOGUE_CARETDATE,
			content->caret_target == PRESET_CARET_DATE);
	icons_set_selected(window, PRESET_DIALOGUE_CARETFROM,
			content->caret_target == PRESET_CARET_FROM);
	icons_set_selected(window, PRESET_DIALOGUE_CARETTO,
			content->caret_target == PRESET_CARET_TO);
	icons_set_selected(window, PRESET_DIALOGUE_CARETREF,
			content->caret_target == PRESET_CARET_REFERENCE);
	icons_set_selected(window, PRESET_DIALOGUE_CARETAMOUNT,
			content->caret_target == PRESET_CARET_AMOUNT);
	icons_set_selected(window, PRESET_DIALOGUE_CARETDESC,
			content->caret_target == PRESET_CARET_DESCRIPTION);
}


/**
 * Process OK clicks in the Preset Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dialogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool preset_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct preset_dialogue_data *content = data;

	if (content == NULL || preset_dialogue_callback == NULL)
		return FALSE;

	/* Extract the information from the dialogue. */

	if (type & DIALOGUE_ICON_OK)
		content->action = PRESET_DIALOGUE_ACTION_OK;
	else if (type & DIALOGUE_ICON_EDIT_DELETE)
		content->action = PRESET_DIALOGUE_ACTION_DELETE;

	/* Zero the flags and reset them as required. */

	content->flags = TRANS_FLAGS_NONE;

	/* Store the name. */

	icons_copy_text(window, PRESET_DIALOGUE_NAME, content->name, PRESET_NAME_LEN);

	/* Store the key. */

	content->action_key = toupper(*icons_get_indirected_text_addr(window, PRESET_DIALOGUE_KEY));

	/* Get the date and today settings. */

	content->date = date_convert_from_string(icons_get_indirected_text_addr(window, PRESET_DIALOGUE_DATE), NULL_DATE, 0);

	if (icons_get_selected(window, PRESET_DIALOGUE_TODAY))
		content->flags |= TRANS_TAKE_TODAY;

	/* Get the from and to fields. */

	content->from = account_find_by_ident(file,
			icons_get_indirected_text_addr(window, PRESET_DIALOGUE_FMIDENT), ACCOUNT_FULL | ACCOUNT_IN);

	content->to = account_find_by_ident(file,
			icons_get_indirected_text_addr(window, PRESET_DIALOGUE_TOIDENT), ACCOUNT_FULL | ACCOUNT_OUT);

	if (*icons_get_indirected_text_addr(window, PRESET_DIALOGUE_FMREC) != '\0')
		content->flags |= TRANS_REC_FROM;

	if (*icons_get_indirected_text_addr(window, PRESET_DIALOGUE_TOREC) != '\0')
		content->flags |= TRANS_REC_TO;

	/* Get the amounts. */

	content->amount = currency_convert_from_string(icons_get_indirected_text_addr(window, PRESET_DIALOGUE_AMOUNT));

	/* Store the reference. */

	icons_copy_text(window, PRESET_DIALOGUE_REF, content->reference, TRANSACT_REF_FIELD_LEN);

	if (icons_get_selected(window, PRESET_DIALOGUE_CHEQUE))
		content->flags |= TRANS_TAKE_CHEQUE;

	/* Store the description. */

	icons_copy_text(window, PRESET_DIALOGUE_DESC, content->description, TRANSACT_DESCRIPT_FIELD_LEN);

	/* Store the caret target. */

	if (icons_get_selected(window, PRESET_DIALOGUE_CARETFROM))
		content->caret_target = PRESET_CARET_FROM;
	else if (icons_get_selected(window, PRESET_DIALOGUE_CARETTO))
		content->caret_target = PRESET_CARET_TO;
	else if (icons_get_selected(window, PRESET_DIALOGUE_CARETREF))
		content->caret_target = PRESET_CARET_REFERENCE;
	else if (icons_get_selected(window, PRESET_DIALOGUE_CARETAMOUNT))
		content->caret_target = PRESET_CARET_AMOUNT;
	else if (icons_get_selected(window, PRESET_DIALOGUE_CARETDESC))
		content->caret_target = PRESET_CARET_DESCRIPTION;
	else
		content->caret_target = PRESET_CARET_DATE;

	/* Call the client back. */

	return preset_dialogue_callback(parent, content);
}


/**
 * The Edit Heading dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void preset_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	preset_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

