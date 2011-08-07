/* CashBook - mainmenu.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_MAINMENU
#define CASHBOOK_MAINMENU

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




/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

char *mainmenu_get_current_menu_name (char *buffer);

/* Account menu */

void set_account_menu (file_data *file);
void open_account_menu (file_data *file, enum account_menu_type type, int line,
                        wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer);

void decode_account_menu (wimp_selection *selection, wimp_pointer *pointer);

void account_menu_submenu_message (wimp_full_message_menu_warning *submenu);
void account_menu_closed_message (void);


/* Date menu */

void open_date_menu(file_data *file, int line, wimp_pointer *pointer);
void decode_date_menu(wimp_selection *selection, wimp_pointer *pointer);
void date_menu_closed_message(void);

/* RefDesc Menu */

void open_refdesc_menu (file_data *file, int menu_type, int line, wimp_pointer *pointer);
void decode_refdesc_menu (wimp_selection *selection, wimp_pointer *pointer);
void refdesc_menu_closed_message(void);

#endif

