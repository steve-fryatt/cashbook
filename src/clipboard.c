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

#include "oslib/wimp.h"

/* OSLibSupport header files */

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/transfer.h"

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
static char		*clipboard_xfer = NULL;					/**< Clipboard data being transferred in from another client.		*/

static size_t		clipboard_send_data(void **data);
static osbool		clipboard_store_text(char *text, size_t len);
static osbool		clipboard_message_claimentity(wimp_message *message);
#if 0
static osbool		clipboard_message_datarequest(wimp_message *message);
static osbool		clipboard_message_datasave(wimp_message *message);
static osbool		clipboard_message_ramtransmit(wimp_message *message);
static osbool		clipboard_message_dataload(wimp_message *message);
static void		clipboard_paste_text(char **data, size_t data_size);
#endif


/**
 * Initialise the Clipboard module.
 */

void clipboard_initialise(void)
{
	event_add_message_handler(message_CLAIM_ENTITY, EVENT_MESSAGE_INCOMING, clipboard_message_claimentity);
	dataxfer_register_clipboard_provider(clipboard_send_data);
//	event_add_message_handler(message_DATA_REQUEST, EVENT_MESSAGE_INCOMING, clipboard_message_datarequest);
//	event_add_message_handler(message_RAM_TRANSMIT, EVENT_MESSAGE_INCOMING, clipboard_message_ramtransmit);
//	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, clipboard_message_dataload);
//	event_add_message_handler(message_DATA_SAVE, EVENT_MESSAGE_INCOMING, clipboard_message_datasave);
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


static size_t clipboard_send_data(void **data)
{
	/* If we don't own the clipboard, return no data. */

	if (clipboard_data == NULL)
		return 0;

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


static osbool clipboard_receive_data(void *data, size_t data_size, bits type, void *context)
{
	wimp_caret	caret;

	wimp_get_caret_position(&caret);
	icons_insert_text(caret.w, caret.i, caret.index, data, data_size);

	debug_printf("We have data!\n");

	heap_free(data);

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

#if 0
/**
 * Handle incoming Message_DataRequest, by sending the clipboard contents out
 * if we currently own it.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool clipboard_message_datarequest(wimp_message *message)
{
	wimp_full_message_data_request	*requestblock = (wimp_full_message_data_request *) message;

	/* Just return if we do not own the clipboard at present. */

	if (clipboard_data == NULL)
		return FALSE;

	/* Check that the message flags are correct. */

	if ((requestblock->flags & wimp_DATA_REQUEST_CLIPBOARD) == 0)
		return FALSE;

	transfer_save_start_block(requestblock->w, requestblock->i, requestblock->pos, requestblock->my_ref,
			&clipboard_data, clipboard_length, 0xfff, "CutText");

	return TRUE;
}


/**
 * Handle incoming Message_DataSave.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool clipboard_message_datasave(wimp_message *message)
{
	if (message->your_ref == 0)
		return FALSE;

	transfer_load_reply_datasave_block(message, &clipboard_xfer);

	return TRUE;
}


/**
 * Handle incoming Message_RamTransmit.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool clipboard_message_ramtransmit(wimp_message *message)
{
	int		size;

	size = transfer_load_reply_ramtransmit(message, NULL);
	if (size > 0)
		clipboard_paste_text(&clipboard_xfer, size);

	return TRUE;
}


/**
 * Handle incoming Message_DataLoad.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool clipboard_message_dataload(wimp_message *message)
{
	int		size;

	if (message->your_ref == 0)
		return FALSE;

	/* If your_ref != 0, there was a Message_DataSave before this.  clipboard_size will only return >0 if
	 * the data is loaded automatically: since only the clipboard uses this, the following code is safe.
	 * All other loads (data files, TSV and CSV) will have supplied a function to handle the physical loading
	 * and clipboard_size will return as 0.
	 */

	size = transfer_load_reply_dataload (message, NULL);
	if (size > 0)
		clipboard_paste_text(&clipboard_xfer, size);

	return TRUE;
}

/**
 * Paste a block of text, received into a flex block via the data transfer
 * system, into an icon.
 *
 * \param **data		The flex block pointer for the data.
 * \param data_size		The size of the data to paste.
 */

static void clipboard_paste_text(char **data, size_t data_size)
{
	wimp_caret	caret;

	wimp_get_caret_position(&caret);
	icons_insert_text(caret.w, caret.i, caret.index, *data, data_size);

	flex_free ((flex_ptr) data);
}
#endif

