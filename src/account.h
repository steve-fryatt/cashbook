/* CashBook - account.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_ACCOUNT
#define CASHBOOK_ACCOUNT

#include <stdio.h>

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
 * Recreate the title of the specified Account window connected to the
 * given file.
 *
 * \param *file			The file to rebuild the title for.
 * \param entry			The entry of the window to to be updated.
 */

void account_build_window_title(file_data *file, int entry);


/**
 * Force a redraw of the Account List window, for the given range of
 * lines.
 *
 * \param *file			The file owning the window.
 * \param entry			The account list window to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

void account_force_window_redraw(file_data *file, int entry, int from, int to);





















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

wimp_menu *account_complete_submenu_build(wimp_message_menu_warning *submenu);


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



















int find_accounts_window_entry_from_type (file_data *file, enum account_type type);





























/**
 * Open the Account Edit dialogue for a given account list window.
 *
 * If account == NULL_ACCOUNT, type determines the type of the new account
 * to be created.  Otherwise, type is ignored and the type derived from the
 * account data block.
 *
 * \param *file			The file to own the dialogue.
 * \param account		The account to edit, or NULL_ACCOUNT for add new.
 * \param type			The type of new account to create if account
 *				is NULL_ACCOUNT.
 * \param *ptr			The current Wimp pointer position.
 */

void account_open_edit_window(file_data *file, acct_t account, enum account_type type, wimp_pointer *ptr);


/**
 * Force the closure of the Account, Heading and Section Edit windows if the
 * owning file disappears.
 *
 * \param *file			The file which has closed.
 */

void account_force_windows_closed(file_data *file);


/**
 * Create a new account with null details.  Core details are set up, but some
 * values are zeroed and left to be set up later.
 *
 * \param *file			The file to add the account to.
 * \param *name			The name to give the account.
 * \param *ident		The ident to give the account.
 * \param type			The type of account to be created.
 * \return			The new account index, or NULL_ACCOUNT.
 */

acct_t account_add(file_data *file, char *name, char *ident, enum account_type type);


/**
 * Delete an account from a file.
 *
 * \param *file			The file to act on.
 * \param account		The account to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

osbool account_delete(file_data *file, acct_t account);






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

int account_used_in_file(file_data *file, acct_t account);

int count_accounts_in_file (file_data *file, enum account_type type);

/* Account window handling */

void account_window_click (file_data *file, wimp_pointer *pointer);
void account_pane_click (file_data *file, wimp_pointer *pointer);





/**
 * Get the next cheque or paying in book number for a given combination of from and
 * to accounts, storing the value as a string in the buffer and incrementing the
 * next value for the chosen account as specified.
 *
 * \param *file			The file containing the accounts.
 * \param from_account		The account to transfer from.
 * \param to_account		The account to transfer to.
 * \param increment		The amount to increment the chosen number after
 *				use (0 for no increment).
 * \param *buffer		Pointer to the buffer to hold the number.
 * \param size			The size of the buffer.
 * \return			A pointer to the supplied buffer, containing the
 *				next available number.
 */

char *account_get_next_cheque_number(file_data *file, acct_t from_account, acct_t to_account, int increment, char *buffer, size_t size);





/**
 * Read account details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

int account_read_acct_file(file_data *file, FILE *in, char *section, char *token, char *value, osbool *unknown_data);


/**
 * Read account list details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *suffix		A string containing the trailing end of the section name.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

int account_read_list_file(file_data *file, FILE *in, char *section, char *token, char *value, char *suffix, osbool *unknown_data);

#endif

