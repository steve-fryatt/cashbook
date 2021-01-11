/* Copyright 2011-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: amenu.h
 *
 * Structured handling of Adjust-Click Menus, for use by the various adjust-
 * click completion menus.
 */

#ifndef CASHBOOK_AMENU
#define CASHBOOK_AMENU

/**
 * Initialise the Adjust-Click Menu system.
 */

void amenu_initialise(void);


/**
 * Open an Adjust-Click Menu on screen, and set up the handlers to track its
 * progress.
 *
 * \param *menu			The menu to be opened.
 * \param *help_token		The interactive help token for the menu.
 * \param *pointer		The details of the position to open it.
 * \param *prepare		A handler to be called before (re-) opening.
 * \param *warning		A handler to be called on submenu warnings.
 * \param *selection		A handler to be called on selections.
 * \param *close		A handler to be called when the menu closes.
 */

void amenu_open(wimp_menu *menu, char *help_token, wimp_pointer *pointer,
		void (*prepare)(void), void (*warning)(wimp_message_menu_warning *), void (*selection)(wimp_selection *), void (*close)(void));


/**
 * Handle menu selection events from the Wimp.  This must be placed in the
 * Wimp_Poll loop, as EventLib doesn't provide a hook for menu selections.
 *
 * \param *selection		The menu selection block to be handled.
 */

void amenu_selection_handler(wimp_selection *selection);

#endif

