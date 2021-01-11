/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: account_menu.h
 *
 * Account completion menu interface.
 */

#ifndef CASHBOOK_ACCOUNT_MENU
#define CASHBOOK_ACCOUNT_MENU

#include "account.h"

/**
 * The different types of Reference or Description menu available.
 */

enum account_menu_type{
	ACCOUNT_MENU_NONE = 0,
	ACCOUNT_MENU_FROM,
	ACCOUNT_MENU_TO,
	ACCOUNT_MENU_ACCOUNTS,
	ACCOUNT_MENU_INCOMING,
	ACCOUNT_MENU_OUTGOING
};




/**
 * Create and open an Account completion menu over a line in a transaction window.
 * 
 * \param *file			The file to which the menu will belong.
 * \param menu_type		The type of menu to be opened.
 * \param line			The line of the window over which the menu opened.
 * \param *pointer		Pointer to the Wimp pointer details.
 */

void account_menu_open(struct file_block *file, enum account_menu_type menu_type, int line, wimp_pointer *pointer);


/**
 * Create and open an Account completion menu over a set of account icons in
 * a dialogue box.
 * 
 * \param *file			The file to which the menu will belong.
 * \param menu_type		The type of menu to be opened.
 * \param *close_callback	Callback pointer to report closure of the menu, or NULL.
 * \param window		The window in which the target icons exist.
 * \param icon_i		The target ident field icon.
 * \param icon_n		The target name field icon.
 * \param icon_r		The target reconcile field icon.
 * \param *pointer		Pointer to the Wimp pointer details.
 */

void account_menu_open_icon(struct file_block *file, enum account_menu_type menu_type, void (*close_callback)(void),
		wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer);

#endif
