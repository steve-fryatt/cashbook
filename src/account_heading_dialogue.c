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
 * \file: account_heading_dialogue.c
 *
 * Account Heading Edit dialogue implementation.
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
#include "account_heading_dialogue.h"

#include "account.h"
#include "dialogue.h"

/* Window Icons. */

#define ACCOUNT_HEADING_DIALOGUE_OK 0
#define ACCOUNT_HEADING_DIALOGUE_CANCEL 1
#define ACCOUNT_HEADING_DIALOGUE_DELETE 2

#define ACCOUNT_HEADING_DIALOGUE_NAME 4
#define ACCOUNT_HEADING_DIALOGUE_IDENT 6
#define ACCOUNT_HEADING_DIALOGUE_INCOMING 7
#define ACCOUNT_HEADING_DIALOGUE_OUTGOING 8
#define ACCOUNT_HEADING_DIALOGUE_BUDGET 10


/* Global Variables */

/**
 * The handle of the Heading Edit dialogue.
 */

static struct dialogue_block	*account_heading_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*account_heading_dialogue_update_callback)(void *, struct account_heading_dialogue_data *);

/**
 * Callback function to request the deletion of a heading.
 */

static osbool			(*account_heading_dialogue_delete_callback)(struct account_block *, acct_t);

/* Static Function Prototypes. */

static void account_heading_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool account_heading_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static osbool account_heading_dialogue_delete(void);
static void account_heading_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Heading Edit Dialogue Icon Set.
 */

static struct dialogue_icon account_heading_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,					ACCOUNT_HEADING_DIALOGUE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,					ACCOUNT_HEADING_DIALOGUE_CANCEL,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_EDIT_DELETE,	ACCOUNT_HEADING_DIALOGUE_DELETE,	DIALOGUE_NO_ICON},


	/* The title and ident fields. */

	{DIALOGUE_ICON_REFRESH,					ACCOUNT_HEADING_DIALOGUE_NAME,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,					ACCOUNT_HEADING_DIALOGUE_IDENT,		DIALOGUE_NO_ICON},

	/* The heading type icons. */

	{DIALOGUE_ICON_RADIO,					ACCOUNT_HEADING_DIALOGUE_INCOMING,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,					ACCOUNT_HEADING_DIALOGUE_OUTGOING,	DIALOGUE_NO_ICON},

	/* The budget field. */

	{DIALOGUE_ICON_REFRESH,					ACCOUNT_HEADING_DIALOGUE_BUDGET,	DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,					DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Heading Edit Dialogue Definition.
 */

static struct dialogue_definition account_heading_dialogue_definition = {
	"EditHeading",
	"EditHeading",
	account_heading_dialogue_icon_list,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	account_heading_dialogue_fill,
	account_heading_dialogue_process,
	account_heading_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the Heading Edit dialogue.
 */

void account_heading_dialogue_initialise(void)
{
	account_heading_dialogue = dialogue_create(&account_heading_dialogue_definition);
}


/**
 * Open the Heading Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void account_heading_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*update_callback)(void *, struct account_heading_dialogue_data *),
		osbool (*delete_callback)(struct account_block *, acct_t), struct account_heading_dialogue_data *content)
{
	if (content == NULL)
		return;

	account_heading_dialogue_update_callback = update_callback;
	account_heading_dialogue_delete_callback = delete_callback;

	/* Set up the dialogue title and action buttons. */

	if (content->account == NULL_ACCOUNT) {
		dialogue_set_title(account_heading_dialogue, "NewHdr", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(account_heading_dialogue, DIALOGUE_ICON_OK, "NewAcctAct", NULL, NULL, NULL, NULL);
	} else {
		dialogue_set_title(account_heading_dialogue, "EditHdr", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(account_heading_dialogue, DIALOGUE_ICON_OK, "EditAcctAct", NULL, NULL, NULL, NULL);
	}

	/* Open the window. */

	dialogue_open(account_heading_dialogue, FALSE, file, owner, ptr, content);
}


/**
 * Fill the Heading Edit Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void account_heading_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct account_heading_dialogue_data *content = data;

	if (content == NULL)
		return;

	icons_strncpy(window, ACCOUNT_HEADING_DIALOGUE_NAME, content->name);
	icons_strncpy(window, ACCOUNT_HEADING_DIALOGUE_IDENT, content->ident);

	currency_convert_to_string(content->budget,
			icons_get_indirected_text_addr(window, ACCOUNT_HEADING_DIALOGUE_BUDGET),
			icons_get_indirected_text_length(window, ACCOUNT_HEADING_DIALOGUE_BUDGET));

	icons_set_shaded(window, ACCOUNT_HEADING_DIALOGUE_INCOMING, (content->account != NULL_ACCOUNT));
	icons_set_selected(window, ACCOUNT_HEADING_DIALOGUE_INCOMING, (content->type & ACCOUNT_IN));

	icons_set_shaded(window, ACCOUNT_HEADING_DIALOGUE_OUTGOING, (content->account != NULL_ACCOUNT));
	icons_set_selected(window, ACCOUNT_HEADING_DIALOGUE_OUTGOING, (content->type & ACCOUNT_OUT));

	icons_set_deleted(window, ACCOUNT_HEADING_DIALOGUE_DELETE, (content->account == NULL_ACCOUNT));
}


/**
 * Process OK clicks in the Heading Edit Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dialogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool account_heading_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct account_heading_dialogue_data *content = data;

	if (content == NULL || account_heading_dialogue_update_callback == NULL)
		return FALSE;

	/* Extract the information from the dialogue. */

	icons_copy_text(window, ACCOUNT_HEADING_DIALOGUE_NAME, content->name, ACCOUNT_NAME_LEN);
	icons_copy_text(window, ACCOUNT_HEADING_DIALOGUE_IDENT, content->ident, ACCOUNT_IDENT_LEN);

	content->budget =
			currency_convert_from_string(icons_get_indirected_text_addr(window, ACCOUNT_HEADING_DIALOGUE_BUDGET));

	if (icons_get_selected(window, ACCOUNT_HEADING_DIALOGUE_INCOMING))
		content->type = ACCOUNT_IN;
	else if (icons_get_selected(window, ACCOUNT_HEADING_DIALOGUE_OUTGOING))
		content->type = ACCOUNT_OUT;
	else
		content->type = ACCOUNT_NULL;

	/* Call the client back. */

	return account_heading_dialogue_update_callback(parent, content);
}


/**
 * Delete the section associated with the currently open Section Edit
 * window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_heading_dialogue_delete(void)
{
	if (account_heading_dialogue_delete_callback == NULL)
		return FALSE;

	/* Check that the user really wishes to proceed. */

	if (error_msgs_report_question("DeleteAcct", "DeleteAcctB") == 4)
		return FALSE;

	/* Call the client back. */

//	return account_heading_dialogue_delete_callback(account_heading_dialogue_owner, account_heading_dialogue_account);
}


/**
 * The Edit Heading dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void account_heading_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	account_heading_dialogue_update_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

