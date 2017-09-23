/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: account_section_dialogue.h
 *
 * Account List Section Edit dialogue interface.
 */

#ifndef CASHBOOK_ACCOUNT_SECTION_DIALOGUE
#define CASHBOOK_ACCOUNT_SECTION_DIALOGUE

#include "account_list_window.h"

/**
 * Initialise the account section edit dialogue.
 */

void account_section_dialogue_initialise(void);


/**
 * Open the Section Edit dialogue for a given account list window.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *window		The account list window to own the dialogue.
 * \param line			The line in the list associated with the dialogue, or -1.
 * \param *update_callback	The callback function to use to return new values.
 * \param *delete_callback	The callback function to use to request deletion.
 * \param *name			The initial name to use for the section.
 * \param type			The initial header/footer setting for the section.
 */

void account_section_dialogue_open(wimp_pointer *ptr, struct account_list_window *window, int line,
		osbool (*update_callback)(struct account_list_window *, int, char *, enum account_line_type),
		osbool (*delete_callback)(struct account_list_window *, int), char *name, enum account_line_type type);


/**
 * Force the closure of the account section edit dialogue if it relates
 * to a given accounts list instance.
 *
 * \param *parent		The parent of the dialogue to be closed,
 *				or NULL to force close.
 */

void account_section_dialogue_force_close(struct account_list_window *parent);


/**
 * Check whether the Edit Section dialogue is open for a given accounts
 * list instance.
 *
 * \param *parent		The accounts list instance to check.
 * \return			TRUE if the dialogue is open; else FALSE.
 */

osbool account_section_dialogue_is_open(struct account_list_window *parent);

#endif

