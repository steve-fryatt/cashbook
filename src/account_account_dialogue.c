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
 * \file: account_account_dialogue.c
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
#include "account_account_dialogue.h"

#include "account.h"
#include "account_idnum.h"
#include "dialogue.h"
#include "interest.h"

/* Window Icons. */

#define ACCOUNT_ACCOUNT_DIALOGUE_OK 0
#define ACCOUNT_ACCOUNT_DIALOGUE_CANCEL 1
#define ACCOUNT_ACCOUNT_DIALOGUE_DELETE 2

#define ACCOUNT_ACCOUNT_DIALOGUE_NAME 4
#define ACCOUNT_ACCOUNT_DIALOGUE_IDENT 6
#define ACCOUNT_ACCOUNT_DIALOGUE_CREDIT 8
#define ACCOUNT_ACCOUNT_DIALOGUE_BALANCE 10
#define ACCOUNT_ACCOUNT_DIALOGUE_PAYIN 12
#define ACCOUNT_ACCOUNT_DIALOGUE_CHEQUE 14
#define ACCOUNT_ACCOUNT_DIALOGUE_RATE 18
#define ACCOUNT_ACCOUNT_DIALOGUE_RATES 19
#define ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_IDENT 21
#define ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_REC 22
#define ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_NAME 23
#define ACCOUNT_ACCOUNT_DIALOGUE_ACCNO 27
#define ACCOUNT_ACCOUNT_DIALOGUE_SRTCD 29
#define ACCOUNT_ACCOUNT_DIALOGUE_ADDR1 31
#define ACCOUNT_ACCOUNT_DIALOGUE_ADDR2 32
#define ACCOUNT_ACCOUNT_DIALOGUE_ADDR3 33
#define ACCOUNT_ACCOUNT_DIALOGUE_ADDR4 34

/**
 * The number of address line icons.
 */

#define ACCOUNT_ACCOUNT_DIALOGUE_ADDR_LINES 4

/* Global Variables */

/**
 * The icons which make up the address field.
 */

static const wimp_i		account_account_dialogue_address_icons[] = {
		ACCOUNT_ACCOUNT_DIALOGUE_ADDR1,
		ACCOUNT_ACCOUNT_DIALOGUE_ADDR2,
		ACCOUNT_ACCOUNT_DIALOGUE_ADDR3,
		ACCOUNT_ACCOUNT_DIALOGUE_ADDR4,
};

/**
 * The handle of the Account Edit dialogue.
 */

static struct dialogue_block	*account_account_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*account_account_dialogue_update_callback)(void *, struct account_account_dialogue_data *);

/**
 * Callback function to request the deletion of a heading.
 */

static osbool			(*account_account_dialogue_delete_callback)(struct account_block *, acct_t);

/* Static Function Prototypes. */

static void account_account_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool account_account_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static osbool account_account_dialogue_delete(void);
static void account_account_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Account Edit Dialogue Icon Set.
 */

static struct dialogue_icon account_account_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,									ACCOUNT_ACCOUNT_DIALOGUE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,									ACCOUNT_ACCOUNT_DIALOGUE_CANCEL,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_EDIT_DELETE,					ACCOUNT_ACCOUNT_DIALOGUE_DELETE,	DIALOGUE_NO_ICON},

	/* The title and ident fields. */

	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_NAME,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_IDENT,		DIALOGUE_NO_ICON},

	/* The numeric fields. */

	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_CREDIT,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_BALANCE,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_PAYIN,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_CHEQUE,	DIALOGUE_NO_ICON},

	/* The interest fields. */

	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_RATE,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_EDIT_RATES,					ACCOUNT_ACCOUNT_DIALOGUE_RATES,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_IDENT | DIALOGUE_ICON_TYPE_TO,		ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_IDENT,	ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_NAME},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_RECONCILE | DIALOGUE_ICON_TYPE_TO,	ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_REC,	ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_IDENT},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_NAME | DIALOGUE_ICON_TYPE_TO,		ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_NAME,	ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_REC},

	/* The bank details fields. */

	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_ACCNO,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_SRTCD,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_ADDR1,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_ADDR2,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_ADDR3,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									ACCOUNT_ACCOUNT_DIALOGUE_ADDR4,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,									DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Account Edit Dialogue Definition.
 */

static struct dialogue_definition account_account_dialogue_definition = {
	"EditAccount",
	"EditAccount",
	account_account_dialogue_icon_list,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	account_account_dialogue_fill,
	account_account_dialogue_process,
	account_account_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the Account Edit dialogue.
 */

void account_account_dialogue_initialise(void)
{
	account_account_dialogue = dialogue_create(&account_account_dialogue_definition);
}


/**
 * Open the Account Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void account_account_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*update_callback)(void *, struct account_account_dialogue_data *),
		osbool (*delete_callback)(struct account_block *, acct_t), struct account_account_dialogue_data *content)
{
	if (content == NULL)
		return;

	/* Set up the dialogue title and action buttons. */

	if (content->account == NULL_ACCOUNT) {
		dialogue_set_title(account_account_dialogue, "NewAcct", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(account_account_dialogue, DIALOGUE_ICON_OK, "NewAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(account_account_dialogue, DIALOGUE_ICON_EDIT_DELETE, TRUE);
	} else {
		dialogue_set_title(account_account_dialogue, "EditAcct", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(account_account_dialogue, DIALOGUE_ICON_OK, "EditAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(account_account_dialogue, DIALOGUE_ICON_EDIT_DELETE, FALSE);
	}

	/* Open the window. */

	dialogue_open(account_account_dialogue, FALSE, file, owner, ptr, content);
}


/**
 * Fill the Account Edit Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void account_account_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct account_account_dialogue_data	*content = data;
	int					i;

	if (content == NULL)
		return;

	icons_strncpy(window, ACCOUNT_ACCOUNT_DIALOGUE_NAME, content->name);
	icons_strncpy(window, ACCOUNT_ACCOUNT_DIALOGUE_IDENT, content->ident);

	currency_convert_to_string(content->credit_limit,
			icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_CREDIT),
			icons_get_indirected_text_length(window, ACCOUNT_ACCOUNT_DIALOGUE_CREDIT));

	currency_convert_to_string(content->opening_balance,
			icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_BALANCE),
			icons_get_indirected_text_length(window, ACCOUNT_ACCOUNT_DIALOGUE_BALANCE));

	account_idnum_get_next(&(content->cheque_number),
			icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_CHEQUE),
			icons_get_indirected_text_length(window, ACCOUNT_ACCOUNT_DIALOGUE_CHEQUE), 0);

	account_idnum_get_next(&(content->payin_number),
			icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_PAYIN),
			icons_get_indirected_text_length(window, ACCOUNT_ACCOUNT_DIALOGUE_PAYIN), 0);

	interest_convert_to_string(content->interest_rate,
			icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_RATE),
			icons_get_indirected_text_length(window, ACCOUNT_ACCOUNT_DIALOGUE_RATE));

//	account_fill_field(block->file, block->accounts[account].offset_against, FALSE, account_acc_edit_window,
//			ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_IDENT, ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_NAME, ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_REC);

	icons_strncpy(window, ACCOUNT_ACCOUNT_DIALOGUE_ACCNO, content->account_num);
	icons_strncpy(window, ACCOUNT_ACCOUNT_DIALOGUE_SRTCD, content->sort_code);

	for (i = 0; (i < ACCOUNT_ADDR_LINES) && (i < ACCOUNT_ACCOUNT_DIALOGUE_ADDR_LINES); i++)
		icons_strncpy(window, account_account_dialogue_address_icons[i], content->address[i]);
}


/**
 * Process OK clicks in the Account Edit Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dialogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool account_account_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct account_account_dialogue_data	*content = data;
	int					i;

	if (content == NULL || account_account_dialogue_update_callback == NULL)
		return FALSE;

	/* Extract the information from the dialogue. */

	icons_copy_text(window, ACCOUNT_ACCOUNT_DIALOGUE_NAME, content->name, ACCOUNT_NAME_LEN);
	icons_copy_text(window, ACCOUNT_ACCOUNT_DIALOGUE_IDENT, content->ident, ACCOUNT_IDENT_LEN);

	content->credit_limit = currency_convert_from_string(icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_CREDIT));

	content->opening_balance = currency_convert_from_string(icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_BALANCE));

	account_idnum_set_from_string(&(content->cheque_number), icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_CHEQUE));

	account_idnum_set_from_string(&(content->payin_number), icons_get_indirected_text_addr(window, ACCOUNT_ACCOUNT_DIALOGUE_PAYIN));

//	account_edit_owner->accounts[account_edit_number].offset_against = account_find_by_ident(account_edit_owner->file,
//			icons_get_indirected_text_addr(account_acc_edit_window, ACCOUNT_ACCOUNT_DIALOGUE_OFFSET_IDENT), ACCOUNT_FULL);

	icons_copy_text(window, ACCOUNT_ACCOUNT_DIALOGUE_ACCNO, content->account_num, ACCOUNT_NO_LEN);
	icons_copy_text(window, ACCOUNT_ACCOUNT_DIALOGUE_SRTCD, content->sort_code, ACCOUNT_SRTCD_LEN);

	for (i = 0; (i < ACCOUNT_ADDR_LINES) && (i < ACCOUNT_ACCOUNT_DIALOGUE_ADDR_LINES); i++)
		icons_copy_text(window, account_account_dialogue_address_icons[i], content->address[i], ACCOUNT_ADDR_LEN);

	/* Call the client back. */

	return account_account_dialogue_update_callback(parent, content);
}


/**
 * Delete the section associated with the currently open Section Edit
 * window.
 *
 * \return			TRUE if deleted; else FALSE.
 */

static osbool account_account_dialogue_delete(void)
{
	if (account_account_dialogue_delete_callback == NULL)
		return FALSE;

	/* Check that the user really wishes to proceed. */

	if (error_msgs_report_question("DeleteAcct", "DeleteAcctB") == 4)
		return FALSE;

	/* Call the client back. */

//	return account_account_dialogue_delete_callback(account_account_dialogue_owner, account_account_dialogue_account);
}


/**
 * The Edit Account dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void account_account_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	account_account_dialogue_update_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

