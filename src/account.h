/* CashBook - account.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_ACCOUNT
#define CASHBOOK_ACCOUNT

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

enum account_menu_type{
	ACCOUNT_MENU_FROM = 1,
	ACCOUNT_MENU_TO,
	ACCOUNT_MENU_ACCOUNTS,
	ACCOUNT_MENU_INCOMING,
	ACCOUNT_MENU_OUTGOING
};

/**
 * Initialise the account system.
 *
 * \param *sprites		The application sprite area.
 */

void account_initialise(osspriteop_area *sprites);


/**
 * Create and open an Accounts window for the given file and account type.
 *
 * \param *file			The file to open a window for.
 * \param type			The type of account to open a window for.
 */

void account_open_window(file_data *file, enum account_type type);





















/**
 * Build an Account List menu for a file, and return the pointer.  This is a
 * list of Full Accounts, used for opening a Account List view.
 *
 * \param *file			The file to build the menu for.
 * \return			The created menu, or NULL for an error.
 */


wimp_menu *account_list_menu_build(file_data *file);


/**
 * Destroy any Account List menu which is currently open.
 */

void account_list_menu_destroy(void);


/**
 * Prepare the Account List menu for opening or reopening, by ticking those
 * accounts which have Account List windows already open.
 */

void account_list_menu_prepare(void);


/**
 * Decode a selection from the Account List menu, converting to an account
 * number.
 *
 * \param selection		The menu selection to decode.
 * \return			The account numer, or NULL_ACCOUNT.
 */

acct_t account_list_menu_decode(int selection);


/**
 * Build an Account Complete menu for a given file and account type.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \return			The menu block, or NULL.
 */

wimp_menu *account_complete_menu_build(file_data *file, enum account_menu_type type);


/**
 * Build a submenu for the Account Complete menu on the fly, using information
 * and memory allocated and assembled in account_complete_menu_build().
 *
 * The memory to hold the menu has been allocated and is pointed to by
 * account_complete_submenu and account_complete_submenu_link; if either of these
 *  are NULL, the fucntion must refuse to run.
 *
 * \param *submenu		The submenu warning message block to use.
 * \return			Pointer to the submenu block, or NULL on failure.
 */

wimp_menu *account_complete_submenu_build(wimp_full_message_menu_warning *submenu);


/**
 * Destroy any Account Complete menu which is currently open.
 */

void account_complete_menu_destroy(void);


/**
 * Decode a selection from the Account Complete menu, converting to an account
 * number.
 *
 * \param *selection		The menu selection to decode.
 * \return			The account numer, or NULL_ACCOUNT.
 */

acct_t account_complete_menu_decode(wimp_selection *selection);

















void redraw_account_window (wimp_draw *redraw, file_data *file);

/* Window creation and deletion */

void adjust_account_window_columns (void *data, wimp_i icon, int width);

int find_accounts_window_entry_from_type (file_data *file, enum account_type type);
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

void print_account_window(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);

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
