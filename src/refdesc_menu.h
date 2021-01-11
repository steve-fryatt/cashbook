/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: refdesc_menu.h
 *
 * Reference and Descripton completion menu implementation.
 */

#ifndef CASHBOOK_REFDESC_MENU
#define CASHBOOK_REFDESC_MENU


/**
 * The different types of Reference or Description menu available.
 */

enum refdesc_menu_type {
	REFDESC_MENU_NONE = 0,			/**< No menu.					*/
	REFDESC_MENU_REFERENCE,			/**< A menu of Reference column completions.	*/
	REFDESC_MENU_DESCRIPTION		/**< A menu of Description column completions.	*/
};


/**
 * Create and open a Reference or Description completion menu over a line
 * in a transaction window.
 * 
 * \param *file			The file to which the menu will belong.
 * \param menu_type		The type of menu to be opened.
 * \param line			The line of the window over which the menu opened.
 * \param *pointer		Pointer to the Wimp pointer details.
 */

void refdesc_menu_open(struct file_block *file, enum refdesc_menu_type menu_type, int line, wimp_pointer *pointer);

#endif
