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

#define MENU_ID_MAIN       0
#define MENU_ID_ACCOPEN    2
#define MENU_ID_ACCOUNT    3
#define MENU_ID_DATE       4
#define MENU_ID_ACCLIST    5
#define MENU_ID_ACCVIEW    6
#define MENU_ID_SORDER     7
#define MENU_ID_PRESET     8
#define MENU_ID_REFDESC    11
#define MENU_ID_REPLIST    12

#define REFDESC_MENU_REFERENCE   1
#define REFDESC_MENU_DESCRIPTION 2

/* Main Menu */

#define MAIN_MENU_SUB_FILE 0
#define MAIN_MENU_SUB_ACCOUNTS 1
#define MAIN_MENU_SUB_HEADINGS 2
#define MAIN_MENU_SUB_TRANS 3
#define MAIN_MENU_SUB_UTILS 4

#define MAIN_MENU_FILE_INFO 0
#define MAIN_MENU_FILE_SAVE 1
#define MAIN_MENU_FILE_EXPCSV 2
#define MAIN_MENU_FILE_EXPTSV 3
#define MAIN_MENU_FILE_CONTINUE 4
#define MAIN_MENU_FILE_PRINT 5

#define MAIN_MENU_ACCOUNTS_VIEW 0
#define MAIN_MENU_ACCOUNTS_LIST 1
#define MAIN_MENU_ACCOUNTS_NEW 2

#define MAIN_MENU_HEADINGS_LISTIN 0
#define MAIN_MENU_HEADINGS_LISTOUT 1
#define MAIN_MENU_HEADINGS_NEW 2

#define MAIN_MENU_TRANS_FIND 0
#define MAIN_MENU_TRANS_GOTO 1
#define MAIN_MENU_TRANS_SORT 2
#define MAIN_MENU_TRANS_AUTOVIEW 3
#define MAIN_MENU_TRANS_AUTONEW 4
#define MAIN_MENU_TRANS_PRESET 5
#define MAIN_MENU_TRANS_PRESETNEW 6
#define MAIN_MENU_TRANS_RECONCILE 7

#define MAIN_MENU_ANALYSIS_BUDGET 0
#define MAIN_MENU_ANALYSIS_SAVEDREP 1
#define MAIN_MENU_ANALYSIS_MONTHREP 2
#define MAIN_MENU_ANALYSIS_UNREC 3
#define MAIN_MENU_ANALYSIS_CASHFLOW 4
#define MAIN_MENU_ANALYSIS_BALANCE 5
#define MAIN_MENU_ANALYSIS_SOREP 6

/* Date menu */

#define DATE_MENU_TODAY 0

/* RefDesc menu */

#define REFDESC_MENU_CHEQUE 0

/* AccList menu */

#define ACCLIST_MENU_VIEWACCT 0
#define ACCLIST_MENU_EDITACCT 1
#define ACCLIST_MENU_EDITSECT 2
#define ACCLIST_MENU_NEWACCT 3
#define ACCLIST_MENU_NEWHEADER 4
#define ACCLIST_MENU_EXPCSV 5
#define ACCLIST_MENU_EXPTSV 6
#define ACCLIST_MENU_PRINT 7

/* AccView menu */

#define ACCVIEW_MENU_FINDTRANS 0
#define ACCVIEW_MENU_GOTOTRANS 1
#define ACCVIEW_MENU_SORT 2
#define ACCVIEW_MENU_EDITACCT 3
#define ACCVIEW_MENU_EXPCSV 4
#define ACCVIEW_MENU_EXPTSV 5
#define ACCVIEW_MENU_PRINT 6

/* SOrder menu */

#define SORDER_MENU_SORT 0
#define SORDER_MENU_EDIT 1
#define SORDER_MENU_NEWSORDER 2
#define SORDER_MENU_EXPCSV 3
#define SORDER_MENU_EXPTSV 4
#define SORDER_MENU_PRINT 5
#define SORDER_MENU_FULLREP 6

/* Preset menu */

#define PRESET_MENU_SORT 0
#define PRESET_MENU_EDIT 1
#define PRESET_MENU_NEWPRESET 2
#define PRESET_MENU_EXPCSV 3
#define PRESET_MENU_EXPTSV 4
#define PRESET_MENU_PRINT 5


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

typedef struct saved_report_menu_link
{
  char name[SAVED_REPORT_NAME_LEN+3]; /* +3 to allow space for ellipsis... */
  int  saved_report;
}
saved_report_menu_link;

/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

void *claim_transient_shared_memory (int amount);
void *extend_transient_shared_memory (int increase);
char *mainmenu_get_current_menu_name (char *buffer);

/* Main menu */

void set_main_menu (file_data *file);
void open_main_menu (file_data *file, wimp_pointer *pointer);

void decode_main_menu (wimp_selection *selection, wimp_pointer *pointer);

void main_menu_submenu_message (wimp_full_message_menu_warning *submenu);

/* Account open menu. */

void set_accopen_menu (file_data *file);
void open_accopen_menu (file_data *file, wimp_pointer *pointer);

void decode_accopen_menu (wimp_selection *selection, wimp_pointer *pointer);

wimp_menu *build_accopen_menu (file_data *file);

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

void set_date_menu (file_data *file);
void open_date_menu (file_data *file, int line, wimp_pointer *pointer);
void decode_date_menu (wimp_selection *selection, wimp_pointer *pointer);
wimp_menu *build_date_menu (file_data *file);

/* RefDesc Menu */

void set_refdesc_menu (file_data *file, int menu_type, int line);
void open_refdesc_menu (file_data *file, int menu_type, int line, wimp_pointer *pointer);
void decode_refdesc_menu (wimp_selection *selection, wimp_pointer *pointer);
wimp_menu *build_refdesc_menu (file_data *file, int menu_type, int start_line);
void mainmenu_add_refdesc_menu_entry (refdesc_menu_link **entries, int *count, int *max, char *new);
int mainmenu_cmp_refdesc_menu_entries (const void *va, const void *vb);


/* Account list menu */

void set_acclist_menu (int type, int line, int data);
void open_acclist_menu (file_data *file, int type, int line, wimp_pointer *pointer);

void decode_acclist_menu (wimp_selection *selection, wimp_pointer *pointer);

void acclist_menu_submenu_message (wimp_full_message_menu_warning *submenu);

/* Account view menu */

void set_accview_menu (int type, int line);
void open_accview_menu (file_data *file, int account, int line, wimp_pointer *pointer);

void decode_accview_menu (wimp_selection *selection, wimp_pointer *pointer);

void accview_menu_submenu_message (wimp_full_message_menu_warning *submenu);

/* Standing order menu */

void set_sorder_menu (int line);
void open_sorder_menu (file_data *file, int line, wimp_pointer *pointer);

void decode_sorder_menu (wimp_selection *selection, wimp_pointer *pointer);

void sorder_menu_submenu_message (wimp_full_message_menu_warning *submenu);

/* Preset menu */

void set_preset_menu (int line);
void open_preset_menu (file_data *file, int line, wimp_pointer *pointer);
void decode_preset_menu (wimp_selection *selection, wimp_pointer *pointer);
void preset_menu_submenu_message (wimp_full_message_menu_warning *submenu);

/* Saved Report list menu */

void mainmenu_set_replist_menu (file_data *file);
void mainmenu_open_replist_menu (file_data *file, wimp_pointer *pointer);

void mainmenu_decode_replist_menu (wimp_selection *selection, wimp_pointer *pointer);

wimp_menu *mainmenu_build_replist_menu (file_data *file, int standalone);

int mainmenu_cmp_replist_menu_entries (const void *va, const void *vb);

#endif

