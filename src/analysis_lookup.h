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
 * \file: analysis_lookup.h
 *
 * Analysis account lookup dialogue interface.
 */

#ifndef CASHBOOK_ANALYSIS_LOOKUP
#define CASHBOOK_ANALYSIS_LOOKUP

#include "account.h"


/**
 * Initialise the Account Lookup dialogue.
 */

void analysis_lookup_initialise(void);


/**
 * Open the account lookup window as a menu, allowing an account to be
 * entered into an account list using a graphical interface.
 *
 * \param *file			The file to which the operation relates.
 * \param window		The window to own the lookup dialogue.
 * \param icon			The icon to own the lookup dialogue.
 * \param account		An account to seed the window, or NULL_ACCOUNT.
 * \param type			The types of account to be accepted.
 */

void analysis_lookup_open_window(struct file_block *file, wimp_w window, wimp_i icon, acct_t account, enum account_type type);

#endif
