/* CashBook - mainmenu.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_MAINMENU
#define CASHBOOK_MAINMENU

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

/* Menu IDs */

#define MENU_ID_ACCOPEN    2
#define MENU_ID_ACCOUNT    3
#define MENU_ID_DATE       4
#define MENU_ID_REFDESC    11

#define REFDESC_MENU_REFERENCE   1
#define REFDESC_MENU_DESCRIPTION 2

/* Date menu */

#define DATE_MENU_TODAY 0

/* RefDesc menu */

#define REFDESC_MENU_CHEQUE 0

/* General definitions */

#define ACCOUNT_MENU_TITLE_LEN 32

#define ACCOUNT_MENU_FROM 1
#define ACCOUNT_MENU_TO 2
#define ACCOUNT_MENU_ACCOUNTS 3
#define ACCOUNT_MENU_INCOMING 4
#define ACCOUNT_MENU_OUTGOING 5

#define REFDESC_MENU_BLOCKSIZE 50

/* ==================================================================================================================
 * Type definitions
 */

typedef struct date_menu_link
{
  char name[PRESET_NAME_LEN];
  int  preset;
}
date_menu_link;

typedef struct acclist_menu_link
{
  char   name[ACCOUNT_NAME_LEN];
  acct_t account;
}
acclist_menu_link;

typedef struct acclist_menu_group
{
  char name[ACCOUNT_SECTION_LEN];
  int  entry;
  int  start_line;
}
acclist_menu_group;

typedef struct refdesc_menu_link
{
  char name[DESCRIPT_FIELD_LEN]; /* This assumes that references are always shorter than descriptions...*/
}
refdesc_menu_link;


/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

void *claim_transient_shared_memory (int amount);
void *extend_transient_shared_memory (int increase);
char *mainmenu_get_current_menu_name (char *buffer);

/* Main menu */


void decode_main_menu (wimp_selection *selection, wimp_pointer *pointer);

/* Account open menu. */

void set_accopen_menu (file_data *file);
void open_accopen_menu (file_data *file, wimp_pointer *pointer);

void decode_accopen_menu (wimp_selection *selection, wimp_pointer *pointer);

wimp_menu *build_accopen_menu (file_data *file);
acct_t decode_accopen_menu_item(int selection);

/* Account menu */

void set_account_menu (file_data *file);
void open_account_menu (file_data *file, int type, int line,
                        wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer);

void decode_account_menu (wimp_selection *selection, wimp_pointer *pointer);

void account_menu_submenu_message (wimp_full_message_menu_warning *submenu);
void account_menu_closed_message (wimp_full_message_menus_deleted *menu_del);

wimp_menu *build_account_menu (file_data *file, unsigned include, char *title);
wimp_menu *build_account_submenu (file_data *file, wimp_full_message_menu_warning *submenu);

/* Date menu */

void open_date_menu(file_data *file, int line, wimp_pointer *pointer);
void decode_date_menu(wimp_selection *selection, wimp_pointer *pointer);
void date_menu_closed_message(void);

/* RefDesc Menu */

void set_refdesc_menu (file_data *file, int menu_type, int line);
void open_refdesc_menu (file_data *file, int menu_type, int line, wimp_pointer *pointer);
void decode_refdesc_menu (wimp_selection *selection, wimp_pointer *pointer);
wimp_menu *build_refdesc_menu (file_data *file, int menu_type, int start_line);
void mainmenu_add_refdesc_menu_entry (refdesc_menu_link **entries, int *count, int *max, char *new);
int mainmenu_cmp_refdesc_menu_entries (const void *va, const void *vb);

#endif

