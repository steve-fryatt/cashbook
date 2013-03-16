/* Copyright 2011-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: amenu.c
 *
 * Structured handling of Adjust-Click Menus, for use by the various adjust-
 * click completion menus.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/menus.h"
#include "sflib/event.h"

/* Application header files */

#include "amenu.h"

#include "templates.h"


static wimp_menu	*amenu_menu = NULL;						/**< The menu handle being processed.			*/
static void		(*amenu_callback_prepare)(void) = NULL;				/**< Callback handler for menu preparation.		*/
static void		(*amenu_callback_warning)(wimp_message_menu_warning *) = NULL;	/**< Callback handler for menu warnings.		*/
static void		(*amenu_callback_selection)(wimp_selection *) = NULL;		/**< Callback handler for menu selections.		*/
static void		(*amenu_callback_close)(void) = NULL;				/**< Callback handler for menu closure.			*/


static osbool		amenu_message_warning_handler(wimp_message *message);
static osbool		amenu_message_deleted_handler(wimp_message *message);
static void		amenu_close(void);


/**
 * Initialise the Adjust-Click Menu system.
 */

void amenu_initialise(void)
{
	event_add_message_handler(message_MENU_WARNING, EVENT_MESSAGE_INCOMING, amenu_message_warning_handler);
	event_add_message_handler(message_MENUS_DELETED, EVENT_MESSAGE_INCOMING, amenu_message_deleted_handler);
}


/**
 * Open an Adjust-Click Menu on screen, and set up the handlers to track its
 * progress.
 *
 * \param *menu			The menu to be opened.
 * \param *pointer		The details of the position to open it.
 * \param *prepare		A handler to be called before (re-) opening.
 * \param *warning		A handler to be called on submenu warnings.
 * \param *selection		A handler to be called on selections.
 * \param *close		A handler to be called when the menu closes.
 */

void amenu_open(wimp_menu *menu, wimp_pointer *pointer, void (*prepare)(void), void (*warning)(wimp_message_menu_warning *), void (*selection)(wimp_selection *), void (*close)(void))
{
	amenu_callback_prepare = prepare;
	amenu_callback_warning = warning;
	amenu_callback_selection = selection;
	amenu_callback_close = close;

	if (amenu_callback_prepare != NULL)
		amenu_callback_prepare();

	amenu_menu = menus_create_standard_menu(menu, pointer);

	templates_set_menu_handle(amenu_menu);
}


/**
 * Handle menu selection events from the Wimp.  This must be placed in the
 * Wimp_Poll loop, as EventLib doesn't provide a hook for menu selections.
 *
 * \param *selection		The menu selection block to be handled.
 */

void amenu_selection_handler(wimp_selection *selection)
{
	wimp_pointer		pointer;

	if (amenu_menu == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	if (amenu_callback_selection != NULL)
		amenu_callback_selection(selection);

	if (pointer.buttons == wimp_CLICK_ADJUST) {
		if (amenu_callback_prepare != NULL)
			amenu_callback_prepare();
		wimp_create_menu(amenu_menu, 0, 0);
	} else {
		amenu_close();
	}
}


/**
 * Message_MenuWarning handler.
 *
 * \param *message		The message data block.
 * \return			FALSE to pass the message on.
 */

static osbool amenu_message_warning_handler(wimp_message *message)
{
	if (amenu_menu == NULL)
		return FALSE;

	if (amenu_callback_warning != NULL)
		amenu_callback_warning((wimp_message_menu_warning *) &(message->data));

	return FALSE;
}


/**
 * Message_MenusDeleted handler.
 *
 * \param *message		The message data block.
 * \return			FALSE to pass the message on.
 */

static osbool amenu_message_deleted_handler(wimp_message *message)
{
	wimp_full_message_menus_deleted *menus_deleted = (wimp_full_message_menus_deleted *) message;

	if (amenu_menu == NULL || amenu_menu != menus_deleted->menu)
		return FALSE;

	amenu_close();

	return FALSE;
}


/**
 * Handle closure of an Adjust-Click Menu.
 */

static void amenu_close(void)
{
	if (amenu_callback_close != NULL)
		amenu_callback_close();

	amenu_menu = NULL;
	amenu_callback_prepare = NULL;
	amenu_callback_warning = NULL;
	amenu_callback_selection = NULL;
	amenu_callback_close = NULL;

	templates_set_menu_handle(NULL);
}

