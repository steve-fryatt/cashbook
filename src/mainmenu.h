/* CashBook - amenu.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_AMENU
#define CASHBOOK_AMENU

#include "account.h"

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

/* Menu IDs */

#define MENU_ID_ACCOUNT    3
#define MENU_ID_DATE       4
#define MENU_ID_REFDESC    11

/* Date menu */

#define DATE_MENU_TODAY 0

/* RefDesc menu */

#define REFDESC_MENU_CHEQUE 0

/* General definitions */

#define ACCOUNT_MENU_TITLE_LEN 32


#define REFDESC_MENU_BLOCKSIZE 50

/* ==================================================================================================================
 * Type definitions
 */




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









/* Account menu */

void open_account_menu (file_data *file, enum account_menu_type type, int line,
                        wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer);

/* Date menu */

void open_date_menu(file_data *file, int line, wimp_pointer *pointer);

/* RefDesc Menu */

void open_refdesc_menu (file_data *file, int menu_type, int line, wimp_pointer *pointer);

#endif

