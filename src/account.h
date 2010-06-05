/* CashBook - account.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_ACCOUNT
#define _ACCOUNTS_ACCOUNT

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

/* Toolbar icons */

#define ACCOUNT_PANE_PARENT 5
#define ACCOUNT_PANE_ADDACCT 6
#define ACCOUNT_PANE_ADDSECT 7
#define ACCOUNT_PANE_PRINT 8

#define ACCOUNT_PANE_COL_MAP "0,1;2;3;4;5"

/* Accounr heading window. */

#define ACCT_EDIT_OK 0
#define ACCT_EDIT_CANCEL 1
#define ACCT_EDIT_DELETE 2

#define ACCT_EDIT_NAME 4
#define ACCT_EDIT_IDENT 6
#define ACCT_EDIT_CREDIT 8
#define ACCT_EDIT_BALANCE 10
#define ACCT_EDIT_ACCNO 18
#define ACCT_EDIT_SRTCD 20
#define ACCT_EDIT_ADDR1 22
#define ACCT_EDIT_ADDR2 23
#define ACCT_EDIT_ADDR3 24
#define ACCT_EDIT_ADDR4 25
#define ACCT_EDIT_PAYIN 12
#define ACCT_EDIT_CHEQUE 14

/* Edit heading window. */

#define HEAD_EDIT_OK 0
#define HEAD_EDIT_CANCEL 1
#define HEAD_EDIT_DELETE 2

#define HEAD_EDIT_NAME 4
#define HEAD_EDIT_IDENT 6
#define HEAD_EDIT_INCOMING 7
#define HEAD_EDIT_OUTGOING 8
#define HEAD_EDIT_BUDGET 10

/* Edit section window. */

#define SECTION_EDIT_OK 2
#define SECTION_EDIT_CANCEL 3
#define SECTION_EDIT_DELETE 4

#define SECTION_EDIT_TITLE 0
#define SECTION_EDIT_HEADER 5
#define SECTION_EDIT_FOOTER 6

/* Account entry window. */

#define ACC_NAME_ENTRY_IDENT 0
#define ACC_NAME_ENTRY_REC 1
#define ACC_NAME_ENTRY_NAME 2
#define ACC_NAME_ENTRY_CANCEL 3
#define ACC_NAME_ENTRY_OK 4

/* ------------------------------------------------------------------------------------------------------------------
 * File data structures
 */



/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Window creation and deletion */

void create_accounts_window (file_data *file, int type);
void delete_accounts_window (file_data *file, int type);
void adjust_account_window_columns (file_data *file, int entry);

int find_accounts_window_type_from_handle (file_data *file, wimp_w window);
int find_accounts_window_entry_from_type (file_data *file, int type);
int find_accounts_window_entry_from_handle (file_data *file, wimp_w window);

/* Adding new accounts */

int add_account (file_data *file, char *name, char *ident, unsigned int type);
void add_account_to_lists (file_data *file, int account);
int add_display_line (file_data *file, int entry);

int delete_account (file_data *file, int account);

/* Editing accounts and headings via GUI */

void open_account_edit_window (file_data *file, int account, int type, wimp_pointer *ptr);

void refresh_account_edit_window (void);
void refresh_heading_edit_window (void);

void fill_account_edit_window (file_data *file, int account);
void fill_heading_edit_window (file_data *file, int account, int type);

int process_account_edit_window (void);
int process_heading_edit_window (void);

void force_close_account_edit_window (file_data *file);

int delete_account_from_edit_window (void);

/* Editing section headings via the GUI. */

void open_section_edit_window (file_data *file, int entry, int line, wimp_pointer *ptr);
void refresh_section_edit_window (void);
void fill_section_edit_window (file_data *file, int entry, int line);

int process_section_edit_window (void);

void force_close_section_edit_window (file_data *file);
int delete_section_from_edit_window (void);

/* Printing accounts via the GUI. */

void open_account_print_window (file_data *file, int type, wimp_pointer *ptr, int clear);

/* Finding accounts */

int find_account (file_data *file, char *ident, unsigned int type);
char *find_account_ident (file_data *file, int account);
char *find_account_name (file_data *file, int account);
char *build_account_name_pair (file_data *file, int account, char *buffer);

int lookup_account_field (file_data *file, char key, int type, int account, int *reconciled,
                          wimp_w window, wimp_i ident, wimp_i name, wimp_i rec);

void fill_account_field (file_data *file, acct_t account, int reconciled,
                         wimp_w window, wimp_i ident, wimp_i name, wimp_i rec_field);
void toggle_account_reconcile_icon (wimp_w window, wimp_i icon);

void open_account_lookup_window (file_data *file, wimp_w window, wimp_i icon, int account, unsigned flags);
void update_account_lookup_window (wimp_key *key);
void open_account_lookup_account_menu (wimp_pointer *ptr);
void close_account_lookup_account_menu (void);
void toggle_account_lookup_reconcile_field (wimp_pointer *ptr);
int process_account_lookup_window (void);

int account_used_in_file (file_data *file, int account);

int count_accounts_in_file (file_data *file, unsigned int type);

/* File and print output */

void print_account_window (int text, int format, int scale, int rotate);

/* Account window handling */

void account_window_click (file_data *file, wimp_pointer *pointer);
void account_pane_click (file_data *file, wimp_pointer *pointer);
void set_accounts_window_extent (file_data *file, int entry);
void build_account_window_title (file_data *file, int entry);
void force_accounts_window_redraw (file_data *file, int entry, int from, int to);
void account_window_scroll_event (file_data *file, wimp_scroll *scroll);
void decode_account_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

/* Account window dragging. */

void start_account_drag (file_data *file, int entry, int line);
void terminate_account_drag (wimp_dragged *drag);

/* Cheque number printing */

char *get_next_cheque_number (file_data *file, acct_t from_account, acct_t to_account, int increment, char *buffer);

#endif
