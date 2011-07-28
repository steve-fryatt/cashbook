/* CashBook - edit.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_EDIT
#define _ACCOUNTS_EDIT

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define EDIT_ICON_DATE 0
#define EDIT_ICON_FROM 1
#define EDIT_ICON_FROM_REC 2
#define EDIT_ICON_FROM_NAME 3
#define EDIT_ICON_TO 4
#define EDIT_ICON_TO_REC 5
#define EDIT_ICON_TO_NAME 6
#define EDIT_ICON_REF 7
#define EDIT_ICON_AMOUNT 8
#define EDIT_ICON_DESCRIPT 9


extern wimp_window *edit_transact_window_def;

/* ------------------------------------------------------------------------------------------------------------------
 * Data structures
 */

/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Edit line creation */

void place_transaction_edit_line (file_data* file, int line);

/* Entry line deletion */

void transaction_edit_line_block_deleted (file_data *file);

/* Entry line operations */

void find_transaction_edit_line (file_data *file);
void find_transaction_edit_icon (file_data *file);
void refresh_transaction_edit_line_icons (wimp_w w, wimp_i only, wimp_i avoid);
void set_transaction_edit_line_shading (file_data *file);

int get_edit_line_transaction (file_data *file);
void place_transaction_edit_line_transaction (file_data *file, int transaction);

/* Transaction operations */

void toggle_reconcile_flag (file_data *file, int transaction, int change_flag);
void edit_change_transaction_date (file_data *file, int transaction, date_t new_date);
void edit_change_transaction_amount (file_data *file, int transaction, amt_t new_amount);
void edit_change_transaction_refdesc (file_data *file, int transaction, int change_icon, char *new_text);
void edit_change_transaction_account (file_data *file, int transaction, int change_icon, acct_t new_account);
void insert_transaction_preset_full (file_data *file, int transaction, int preset);
int insert_transaction_preset (file_data *file, int transaction, int preset);
wimp_i edit_convert_preset_icon_number (int caret);
void delete_edit_line_transaction (file_data *file);

/* Keypress handling */

osbool process_transaction_edit_line_keypress (file_data *file, wimp_key *key);
void process_transaction_edit_line_entry_keys (file_data *file, wimp_key *key);

/* Descript completion. */

char *find_complete_description (file_data *file, int line, char *buffer);

#endif
