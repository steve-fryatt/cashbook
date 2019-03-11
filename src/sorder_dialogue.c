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
 * \file: sorder_dialogue.c
 *
 * Standing Order Edit dialogue implementation.
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
#include "sorder_dialogue.h"

//#include "account.h"
#include "dialogue.h"

/* Window Icons. */

#define SORDER_DIALOGUE_OK 0
#define SORDER_DIALOGUE_CANCEL 1
#define SORDER_DIALOGUE_STOP 34
#define SORDER_DIALOGUE_DELETE 35

#define SORDER_DIALOGUE_START 3
#define SORDER_DIALOGUE_NUMBER 5
#define SORDER_DIALOGUE_PERIOD 7
#define SORDER_DIALOGUE_PERDAYS 8
#define SORDER_DIALOGUE_PERMONTHS 9
#define SORDER_DIALOGUE_PERYEARS 10
#define SORDER_DIALOGUE_AVOID 11
#define SORDER_DIALOGUE_SKIPFWD 12
#define SORDER_DIALOGUE_SKIPBACK 13
#define SORDER_DIALOGUE_FMIDENT 17
#define SORDER_DIALOGUE_FMREC 32
#define SORDER_DIALOGUE_FMNAME 18
#define SORDER_DIALOGUE_TOIDENT 20
#define SORDER_DIALOGUE_TOREC 33
#define SORDER_DIALOGUE_TONAME 21
#define SORDER_DIALOGUE_REF 23
#define SORDER_DIALOGUE_AMOUNT 25
#define SORDER_DIALOGUE_FIRSTSW 26
#define SORDER_DIALOGUE_FIRST 27
#define SORDER_DIALOGUE_LASTSW 28
#define SORDER_DIALOGUE_LAST 29
#define SORDER_DIALOGUE_DESC 31


/* Global Variables */

/**
 * The handle of the Standing Order dialogue.
 */

static struct dialogue_block	*sorder_dialogue = NULL;

/**
 * Callback function to return updated settings.
 */

static osbool			(*sorder_dialogue_callback)(void *, struct sorder_dialogue_data *);

/* Static Function Prototypes. */

static void sorder_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool sorder_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void sorder_dialogue_close(struct file_block *file, wimp_w window, void *data);

/**
 * The Standing Order Dialogue Icon Set.
 */

static struct dialogue_icon sorder_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,									SORDER_DIALOGUE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,									SORDER_DIALOGUE_CANCEL	,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_EDIT_STOP | DIALOGUE_ICON_ACTION_NO_CLOSE,	SORDER_DIALOGUE_STOP,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_EDIT_DELETE,					SORDER_DIALOGUE_DELETE,		DIALOGUE_NO_ICON},

	/* The date and number fields. */

	{DIALOGUE_ICON_REFRESH,									SORDER_DIALOGUE_START,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,									SORDER_DIALOGUE_NUMBER,		DIALOGUE_NO_ICON},

	/* The period icons. */

	{DIALOGUE_ICON_REFRESH,									SORDER_DIALOGUE_PERIOD,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									SORDER_DIALOGUE_PERDAYS,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									SORDER_DIALOGUE_PERMONTHS,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO,									SORDER_DIALOGUE_PERYEARS,	DIALOGUE_NO_ICON},

	/* The skip forward or back fields. */

	{DIALOGUE_ICON_SHADE_TARGET,								SORDER_DIALOGUE_AVOID,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO | DIALOGUE_ICON_SHADE_OFF,						SORDER_DIALOGUE_SKIPFWD,	SORDER_DIALOGUE_AVOID},
	{DIALOGUE_ICON_RADIO | DIALOGUE_ICON_SHADE_OFF,						SORDER_DIALOGUE_SKIPBACK,	SORDER_DIALOGUE_AVOID},

	/* The details fields. */

	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_IDENT | DIALOGUE_ICON_TYPE_FROM,		SORDER_DIALOGUE_FMIDENT,	SORDER_DIALOGUE_FMNAME},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_RECONCILE | DIALOGUE_ICON_TYPE_FROM,	SORDER_DIALOGUE_FMREC,		SORDER_DIALOGUE_FMIDENT},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_NAME | DIALOGUE_ICON_TYPE_FROM,		SORDER_DIALOGUE_FMNAME,		SORDER_DIALOGUE_FMREC},

	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_IDENT | DIALOGUE_ICON_TYPE_TO,		SORDER_DIALOGUE_TOIDENT,	SORDER_DIALOGUE_TONAME},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_RECONCILE | DIALOGUE_ICON_TYPE_TO,	SORDER_DIALOGUE_TOREC,		SORDER_DIALOGUE_TOIDENT},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_ACCOUNT_NAME | DIALOGUE_ICON_TYPE_TO,		SORDER_DIALOGUE_TONAME,		SORDER_DIALOGUE_TOREC},

	{DIALOGUE_ICON_REFRESH,									SORDER_DIALOGUE_REF,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_REFRESH,									SORDER_DIALOGUE_AMOUNT,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_SHADE_TARGET,								SORDER_DIALOGUE_FIRSTSW,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_SHADE_OFF,					SORDER_DIALOGUE_FIRST,		SORDER_DIALOGUE_FIRSTSW},

	{DIALOGUE_ICON_SHADE_TARGET,								SORDER_DIALOGUE_LASTSW,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH | DIALOGUE_ICON_SHADE_OFF,					SORDER_DIALOGUE_LAST,		SORDER_DIALOGUE_LASTSW},

	{DIALOGUE_ICON_REFRESH,									SORDER_DIALOGUE_DESC,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,									DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};


/**
 * The Standing Order Dialogue Definition.
 */

static struct dialogue_definition sorder_dialogue_definition = {
	"EditSOrder",
	"EditSOrder",
	sorder_dialogue_icon_list,
	DIALOGUE_GROUP_NONE,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	sorder_dialogue_fill,
	sorder_dialogue_process,
	sorder_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the Standing Order dialogue.
 */

void sorder_dialogue_initialise(void)
{
	sorder_dialogue = dialogue_create(&sorder_dialogue_definition);
}


/**
 * Open the Standing Order dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *owner		The account instance to own the dialogue.
 * \param *file			The file instance to own the dialogue.
 * \param *callback		The callback function to use to return new values.
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void sorder_dialogue_open(wimp_pointer *ptr, void *owner, struct file_block *file,
		osbool (*callback)(void *, struct sorder_dialogue_data *), struct sorder_dialogue_data *content)
{
	if (content == NULL)
		return;

	sorder_dialogue_callback = callback;

	/* Set up the dialogue title and action buttons. */

	if (content->sorder == NULL_SORDER) {
		dialogue_set_title(sorder_dialogue, "NewSO", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(sorder_dialogue, DIALOGUE_ICON_OK, "NewAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(sorder_dialogue, DIALOGUE_ICON_EDIT_DELETE | DIALOGUE_ICON_EDIT_STOP, TRUE);
	} else {
		dialogue_set_title(sorder_dialogue, "EditSO", NULL, NULL, NULL, NULL);
		dialogue_set_icon_text(sorder_dialogue, DIALOGUE_ICON_OK, "EditAcctAct", NULL, NULL, NULL, NULL);
		dialogue_set_hidden_icons(sorder_dialogue, DIALOGUE_ICON_EDIT_DELETE | DIALOGUE_ICON_EDIT_STOP, FALSE);
	}

	/* Open the window. */

	dialogue_open(sorder_dialogue, FALSE, file, owner, ptr, content);
}


/**
 * Fill the Standing Order Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void sorder_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct sorder_dialogue_data *content = data;

	if (content == NULL)
		return;

	/* Set start date. */

	date_convert_to_string(content->start_date,
			icons_get_indirected_text_addr(window, SORDER_DIALOGUE_START),
			icons_get_indirected_text_length(window, SORDER_DIALOGUE_START));

	/* Set number. */

	icons_printf(window, SORDER_DIALOGUE_NUMBER, "%d", content->number);

	/* Set period details. */

	icons_printf(window, SORDER_DIALOGUE_PERIOD, "%d", content->period);

	icons_set_selected(window, SORDER_DIALOGUE_PERDAYS,
			content->period_unit == DATE_PERIOD_DAYS);
	icons_set_selected(window, SORDER_DIALOGUE_PERMONTHS,
			content->period_unit == DATE_PERIOD_MONTHS);
	icons_set_selected(window, SORDER_DIALOGUE_PERYEARS,
			content->period_unit == DATE_PERIOD_YEARS);

	/* Set the ignore weekends details. */

	icons_set_selected(window, SORDER_DIALOGUE_AVOID,
			(content->flags & TRANS_SKIP_FORWARD ||
			content->flags & TRANS_SKIP_BACKWARD));

	icons_set_selected(window, SORDER_DIALOGUE_SKIPFWD, !(content->flags & TRANS_SKIP_BACKWARD));
	icons_set_selected(window, SORDER_DIALOGUE_SKIPBACK, (content->flags & TRANS_SKIP_BACKWARD));

	icons_set_shaded(window, SORDER_DIALOGUE_SKIPFWD,
			!(content->flags & TRANS_SKIP_FORWARD ||
			content->flags & TRANS_SKIP_BACKWARD));

	icons_set_shaded(window, SORDER_DIALOGUE_SKIPBACK,
			!(content->flags & TRANS_SKIP_FORWARD ||
			content->flags & TRANS_SKIP_BACKWARD));

	/* Fill in the from and to fields. */

	account_fill_field(file, content->from, content->flags & TRANS_REC_FROM,
			window, SORDER_DIALOGUE_FMIDENT, SORDER_DIALOGUE_FMNAME, SORDER_DIALOGUE_FMREC);

	account_fill_field(file, content->to, content->flags & TRANS_REC_TO,
			window, SORDER_DIALOGUE_TOIDENT, SORDER_DIALOGUE_TONAME, SORDER_DIALOGUE_TOREC);

	/* Fill in the reference field. */

	icons_strncpy(window, SORDER_DIALOGUE_REF, content->reference);

	/* Fill in the amount fields. */

	currency_convert_to_string(content->normal_amount,
			icons_get_indirected_text_addr(window, SORDER_DIALOGUE_AMOUNT),
			icons_get_indirected_text_length(window, SORDER_DIALOGUE_AMOUNT));


	currency_convert_to_string(content->first_amount,
			icons_get_indirected_text_addr(window, SORDER_DIALOGUE_FIRST),
			icons_get_indirected_text_length(window, SORDER_DIALOGUE_FIRST));

	icons_set_shaded(window, SORDER_DIALOGUE_FIRST,
			(content->first_amount == content->normal_amount));

	icons_set_selected(window, SORDER_DIALOGUE_FIRSTSW,
			(content->first_amount != content->normal_amount));

	currency_convert_to_string(content->last_amount,
			icons_get_indirected_text_addr(window, SORDER_DIALOGUE_LAST),
			icons_get_indirected_text_length(window, SORDER_DIALOGUE_LAST));

	icons_set_shaded(window, SORDER_DIALOGUE_LAST,
			(content->last_amount == content->normal_amount));

	icons_set_selected(window, SORDER_DIALOGUE_LASTSW,
			(content->last_amount != content->normal_amount));

	/* Fill in the description field. */

	icons_strncpy(window, SORDER_DIALOGUE_DESC, content->description);

	/* Shade icons as required for the edit mode.
	 * This assumes that none of the relevant icons get shaded for any other reason...
	 */

	icons_set_shaded(window, SORDER_DIALOGUE_START, content->active);
	icons_set_shaded(window, SORDER_DIALOGUE_PERIOD, content->active);
	icons_set_shaded(window, SORDER_DIALOGUE_PERDAYS, content->active);
	icons_set_shaded(window, SORDER_DIALOGUE_PERMONTHS, content->active);
	icons_set_shaded(window, SORDER_DIALOGUE_PERYEARS, content->active);

	icons_set_shaded(window, SORDER_DIALOGUE_STOP, (content->active == FALSE) && (content->sorder != NULL_SORDER));
}


/**
 * Process OK clicks in the Standing Order Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent goto instance.
 * \param *data		Client data pointer, to the dialogue data structure.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool sorder_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct sorder_dialogue_data	*content = data;
	date_t				start_date;
	enum date_period		period_unit;

	if (content == NULL || sorder_dialogue_callback == NULL)
		return FALSE;

	/* Extract the period details. */

	if (icons_get_selected(window, SORDER_DIALOGUE_PERDAYS))
		period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(window, SORDER_DIALOGUE_PERMONTHS))
		period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(window, SORDER_DIALOGUE_PERYEARS))
		period_unit = SORDER_DIALOGUE_PERYEARS;
	else
		period_unit = DATE_PERIOD_NONE;

	/* Extract the start date.
	 *
	 * If the period is months, 31 days are always allowed in the date
	 * conversion to allow for the longest months. If another period
	 * is chosen, the default of the number of days in the given month
	 * is used.
	 */

	start_date = date_convert_from_string(icons_get_indirected_text_addr(window, SORDER_DIALOGUE_START), NULL_DATE,
			((period_unit == DATE_PERIOD_MONTHS) ? 31 : 0));

	if (start_date == NULL_DATE) {
		// \TODO -- Error Bad Date!
		return FALSE;
	}

	/* Extract the information from the dialogue. */

	if (type & DIALOGUE_ICON_OK)
		content->action = SORDER_DIALOGUE_ACTION_OK;
	else if (type & DIALOGUE_ICON_EDIT_DELETE)
		content->action = SORDER_DIALOGUE_ACTION_DELETE;
	else if (type & DIALOGUE_ICON_EDIT_STOP)
		content->action = SORDER_DIALOGUE_ACTION_STOP;

	/* Zero the flags and reset them as required. */

	content->flags = TRANS_FLAGS_NONE;

	/* Extract the period details. */

	content->period_unit = period_unit;
	content->start_date = start_date;

	content->period = atoi(icons_get_indirected_text_addr(window, SORDER_DIALOGUE_PERIOD));

	/* Extract the number of transactions. */

	content->number = atoi(icons_get_indirected_text_addr(window, SORDER_DIALOGUE_NUMBER));

	/* Extract the avoid mode. */

	if (icons_get_selected(window, SORDER_DIALOGUE_AVOID)) {
		if (icons_get_selected(window, SORDER_DIALOGUE_SKIPFWD))
			content->flags |= TRANS_SKIP_FORWARD;
		else if (icons_get_selected(window, SORDER_DIALOGUE_SKIPBACK))
			content->flags |= TRANS_SKIP_BACKWARD;
	}
	
	/* Extract the from and to fields. */

	content->from = account_find_by_ident(file,
			icons_get_indirected_text_addr(window, SORDER_DIALOGUE_FMIDENT), ACCOUNT_FULL | ACCOUNT_IN);

	content->to = account_find_by_ident(file,
			icons_get_indirected_text_addr(window, SORDER_DIALOGUE_TOIDENT), ACCOUNT_FULL | ACCOUNT_OUT);

	if (*icons_get_indirected_text_addr(window, SORDER_DIALOGUE_FMREC) != '\0')
		content->flags |= TRANS_REC_FROM;

	if (*icons_get_indirected_text_addr(window, SORDER_DIALOGUE_TOREC) != '\0')
		content->flags |= TRANS_REC_TO;

	/* Extract the amounts. */

	content->normal_amount =
		currency_convert_from_string(icons_get_indirected_text_addr(window, SORDER_DIALOGUE_AMOUNT));

	if (icons_get_selected(window, SORDER_DIALOGUE_FIRSTSW))
		content->first_amount =
				currency_convert_from_string(icons_get_indirected_text_addr(window, SORDER_DIALOGUE_FIRST));
	else
		content->first_amount = content->normal_amount;

	if (icons_get_selected(window, SORDER_DIALOGUE_LASTSW))
		content->last_amount =
				currency_convert_from_string(icons_get_indirected_text_addr(window, SORDER_DIALOGUE_LAST));
	else
		content->last_amount = content->normal_amount;

	/* Store the reference. */

	icons_copy_text(window, SORDER_DIALOGUE_REF, content->reference, TRANSACT_REF_FIELD_LEN);

	/* Store the description. */

	icons_copy_text(window, SORDER_DIALOGUE_DESC, content->description, TRANSACT_DESCRIPT_FIELD_LEN);

	/* Call the client back. */

	return sorder_dialogue_callback(parent, content);
}


/**
 * The Standing Order dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, to the dialogue data structure.
 */

static void sorder_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	sorder_dialogue_callback = NULL;

	/* The client is assuming that we'll delete this after use. */

	if (data != NULL)
		heap_free(data);
}

