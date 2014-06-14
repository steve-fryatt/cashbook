/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: clipboard.c
 *
 * Global Clipboard implementation.
 */

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/osfile.h"
#include "oslib/wimp.h"

/* OSLibSupport header files */

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"

/* Application header files */

#include "dataxfer.h"
#include "global.h"
#include "main.h"
#include "clipboard.h"

/* ANSI C header files */

#include <string.h>

/* ------------------------------------------------------------------------------------------------------------------ */

/* Declare the global variables that are used. */

static char		*clipboard_data = NULL;					/**< Clipboard data help by CashBook, or NULL for none.			*/
static size_t		clipboard_length = 0;					/**< The length of the clipboard held by CashBook.			*/

static osbool		clipboard_receive_data(void *data, size_t data_size, bits type, void *context);
static osbool		clipboard_store_text(char *text, size_t len);
static osbool		clipboard_message_claimentity(wimp_message *message);
static size_t		clipboard_send_data(bits types[], bits *type, void **data);


/**
 * Initialise the Clipboard module.
 */

void clipboard_initialise(void)
{
	event_add_message_handler(message_CLAIM_ENTITY, EVENT_MESSAGE_INCOMING, clipboard_message_claimentity);
	dataxfer_register_clipboard_provider(clipboard_send_data);
}


/**
 * Copy the contents of an icon to the global clipboard, cliaming it in the
 * process if necessary.
 *
 * \param *key			A Wimp Key block, indicating the icon to copy.
 * \return			TRUE if successful; else FALSE.
 */

osbool clipboard_copy_from_icon(wimp_key *key)
{
	wimp_icon_state		icon;

	if (!config_opt_read("GlobalClipboardSupport"))
		return TRUE;

	icon.w = key->w;
	icon.i = key->i;
	wimp_get_icon_state(&icon);

	return clipboard_store_text(icon.icon.data.indirected_text.text, strlen(icon.icon.data.indirected_text.text));
}


/**
 * Cut the contents of an icon to the global clipboard, cliaming it in the
 * process if necessary.
 *
 * \param *key			A Wimp Key block, indicating the icon to cut.
 * \return			TRUE if successful; else FALSE.
 */

osbool clipboard_cut_from_icon(wimp_key *key)
{
	wimp_icon_state		icon;

	if (!config_opt_read("GlobalClipboardSupport"))
		return TRUE;

	icon.w = key->w;
	icon.i = key->i;
	wimp_get_icon_state(&icon);

	if (!clipboard_store_text(icon.icon.data.indirected_text.text, strlen(icon.icon.data.indirected_text.text)))
		return FALSE;

	*icon.icon.data.indirected_text.text = '\0';
	wimp_set_icon_state(key->w, key->i, 0, 0);
	icons_put_caret_at_end(key->w, key->i);

	return TRUE;
}


/**
 * Paste the contents of the global clipboard into an icon.  If we own the
 * clipboard, this is done immediately; otherwise the Wimp message dialogue
 * is started.
 *
 * \param *key			A Wimp Key block, indicating the icon to paste.
 * \return			TRUE if successful; else FALSE.
 */

osbool clipboard_paste_to_icon(wimp_key *key)
{
	bits	types[] = {0xfff, -1};

	/* Test to see if we own the clipboard ourselves.  If so, use it directly; if not, send out a
	* Message_DataRequest.
	*/

	if (!config_opt_read("GlobalClipboardSupport"))
		return TRUE;

	if (clipboard_data != NULL) {
		icons_insert_text(key->w, key->i, key->index, clipboard_data, clipboard_length);
	} else {
		if (!dataxfer_request_clipboard(key->w, key->i, key->pos, types, clipboard_receive_data, NULL))
			return FALSE;
	}

	return TRUE;
}


/**
 * Paste the contents of the global clipboard into an icon, having retrieved it
 * from the current clipboard owner.
 *
 * \param **data		The flex block pointer for the data.
 * \param data_size		The size of the data to paste.
 */

static osbool clipboard_receive_data(void *data, size_t data_size, bits type, void *context)
{
	wimp_caret	caret;

	wimp_get_caret_position(&caret);
	icons_insert_text(caret.w, caret.i, caret.index, data, data_size);

	heap_free(data);

	return TRUE;
}


/**
 * Store a piece of text on the clipboard, claiming it in the process.
 *
 * \param *text			The text to be stored.
 * \param len			The length of the text.
 * \return			TRUE if claimed; else FALSE.
 */

static osbool clipboard_store_text(char *text, size_t len)
{
	wimp_full_message_claim_entity	claimblock;
	os_error			*error;

	/* If we already have the clipboard, clear it first. */

	if (clipboard_data != NULL) {
		heap_free(clipboard_data);
		clipboard_data = NULL;
		clipboard_length = 0;
	}

	/* Record the details of the text in our own variables. */

	clipboard_data = heap_alloc(len);
	if (clipboard_data == NULL) {
		error_msgs_report_error("ClipAllocFail");
		return FALSE;
	}

	memcpy(clipboard_data, text, len);
	clipboard_length = len;

	/* Send out Message_CliamClipboard. */

	claimblock.size = 24;
	claimblock.your_ref = 0;
	claimblock.action = message_CLAIM_ENTITY;
	claimblock.flags = wimp_CLAIM_CLIPBOARD;

	error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) &claimblock, wimp_BROADCAST);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);

		heap_free(clipboard_data);
		clipboard_data = NULL;
		clipboard_length = 0;

		return FALSE;
	}

	return TRUE;
}


/**
 * Handle incoming Message_ClainEntity, by dropping the clipboard if we
 * currently own it.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool clipboard_message_claimentity(wimp_message *message)
{
	wimp_full_message_claim_entity	*claimblock = (wimp_full_message_claim_entity *) message;

	/* Unset the contents of the clipboard if the claim was for that. */

	if ((clipboard_data != NULL) && (claimblock->sender != main_task_handle) && (claimblock->flags & wimp_CLAIM_CLIPBOARD)) {
		heap_free(clipboard_data);
		clipboard_data = NULL;
		clipboard_length = 0;
	}

	return TRUE;
}


/**
 * Handle requests from other tasks for the clipboard data by checking to see
 * if we currently own it and whether any of the requested types are ones that
 * we can support. If we can supply the data, copy it into a heap block and
 * pass it to the dataxfer code to process.
 *
 * \param types[]		A list of acceptable filetypes for the data.
 * \param *type			Pointer to a variable to take the chosen type.
 * \param **data		Pointer to a pointer to take the address of the data.
 * \return			The number of bytes offered, or 0 if we can't help.
 */

static size_t clipboard_send_data(bits types[], bits *type, void **data)
{
	int	i = 0;

	/* If we don't own the clipboard, return no data. */

	if (clipboard_data == NULL || types == NULL || type == NULL || data == NULL)
		return 0;

	/* Check the list of acceptable types to see if there's one we
	 * like.
	 */

	while (types[i] != -1) {
		if (types[i] == osfile_TYPE_TEXT)
			break;
	}

	if (types[i] == -1)
		return 0;

	*type = osfile_TYPE_TEXT;

	/* Make a copy of the clipboard using the static heap known to
	 * dataxfer, then return a pointer. This will be freed by dataxfer
	 * once the transfer is complete.
	 */

	*data = heap_alloc(clipboard_length);
	if (*data == NULL)
		return 0;
	
	memcpy(*data, clipboard_data, clipboard_length);
	
	return clipboard_length;
}

