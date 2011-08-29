/* CashBook - amenu.h
 *
 * (c) Stephen Fryatt, 2011
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
 * \param *pointer		The details of the position to open it.
 * \param *prepare		A handler to be called before (re-) opening.
 * \param *warning		A handler to be called on submenu warnings.
 * \param *selection		A handler to be called on selections.
 * \param *close		A handler to be called when the menu closes.
 */

void amenu_open(wimp_menu *menu, wimp_pointer *pointer, void (*prepare)(void), void (*warning)(wimp_message_menu_warning *), void (*selection)(wimp_selection *), void (*close)(void));


/**
 * Handle menu selection events from the Wimp.  This must be placed in the
 * Wimp_Poll loop, as EventLib doesn't provide a hook for menu selections.
 *
 * \param *selection		The menu selection block to be handled.
 */

void amenu_selection_handler(wimp_selection *selection);

#endif
