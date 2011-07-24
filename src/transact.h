/* CashBook - transact.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_TRANSACT
#define CASHBOOK_TRANSACT

#include "oslib/wimp.h"

#include "filing.h"

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

/* Toolbar icons */

#define TRANSACT_PANE_DATE 0
#define TRANSACT_PANE_FROM 1
#define TRANSACT_PANE_TO 2
#define TRANSACT_PANE_REFERENCE 3
#define TRANSACT_PANE_AMOUNT 4
#define TRANSACT_PANE_DESCRIPTION 5

#define TRANSACT_PANE_SAVE 6
#define TRANSACT_PANE_PRINT 7
#define TRANSACT_PANE_ACCOUNTS 8
#define TRANSACT_PANE_VIEWACCT 19
#define TRANSACT_PANE_ADDACCT 9
#define TRANSACT_PANE_IN 10
#define TRANSACT_PANE_OUT 11
#define TRANSACT_PANE_ADDHEAD 12
#define TRANSACT_PANE_FIND 13
#define TRANSACT_PANE_GOTO 14
#define TRANSACT_PANE_SORT 15
#define TRANSACT_PANE_RECONCILE 16
#define TRANSACT_PANE_SORDER 17
#define TRANSACT_PANE_ADDSORDER 18
#define TRANSACT_PANE_PRESET 20
#define TRANSACT_PANE_ADDPRESET 21

#define TRANSACT_PANE_DRAG_LIMIT 5 /* The last icon to allow column drags on. */
#define TRANSACT_PANE_SORT_DIR_ICON 22

#define TRANSACT_PANE_COL_MAP "0;1,2,3;4,5,6;7;8;9"

#define TRANS_SORT_OK 2
#define TRANS_SORT_CANCEL 3
#define TRANS_SORT_DATE 4
#define TRANS_SORT_FROM 5
#define TRANS_SORT_TO 6
#define TRANS_SORT_REFERENCE 7
#define TRANS_SORT_AMOUNT 8
#define TRANS_SORT_DESCRIPTION 9
#define TRANS_SORT_ASCENDING 10
#define TRANS_SORT_DESCENDING 11

/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Window creation and deletion */

void create_transaction_window (file_data *file);
void delete_transaction_window (file_data *file);
void adjust_transaction_window_columns (file_data *file, int data, wimp_i icon, int width);
void adjust_transaction_window_sort_icon (file_data *file);
void update_transaction_window_sort_icon (file_data *file, wimp_icon *icon);

/* Transaction handling */

void add_raw_transaction (file_data *file, unsigned date, int from, int to, unsigned flags, int amount,
                          char *ref, char *description);
int is_transaction_blank (file_data *file, int line);
void strip_blank_transactions (file_data *file);

/* Sorting transactions */

void sort_transactions (file_data *file);
void sort_transaction_window (file_data *file);

void open_transaction_sort_window (file_data *file, wimp_pointer *ptr);
void refresh_transaction_sort_window (void);
void fill_transaction_sort_window (int sort_option);
int process_transaction_sort_window (void);
void force_close_transaction_sort_window (file_data *file);

/* Finding transactions */

void find_next_reconcile_line (file_data *file, int set);

int find_first_blank_line (file_data *file);

/* Printing transactions via the GUI */

void open_transact_print_window (file_data *file, wimp_pointer *ptr, int clear);

/* File and print output */

void print_transact_window(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum, date_t from, date_t to);

/* Transaction window handling */

void transaction_window_click (file_data *file, wimp_pointer *pointer);
void transaction_pane_click (file_data *file, wimp_pointer *pointer);
void transaction_window_keypress (file_data *file, wimp_key *key);
void set_transaction_window_extent (file_data *file);
void minimise_transaction_window_extent (file_data *file);
void build_transaction_window_title (file_data *file);
void force_transaction_window_redraw (file_data *file, int from, int to);
void update_transaction_window_toolbar (file_data *file);

void transaction_window_scroll_event (file_data *file, wimp_scroll *scroll);
void scroll_transaction_window_to_end (file_data *file, int dir);

int find_transaction_window_centre (file_data *file, int account);

void decode_transact_window_help (char *buffer, wimp_w window, wimp_i icon, os_coord pos, wimp_mouse_state buttons);

int locate_transaction_in_transact_window (file_data *file, int transaction);



/**
 * Save the transaction details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void transact_write_file(file_data *file, FILE *out);


/**
 * Read transaction details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

int transact_read_file(file_data *file, FILE *in, char *section, char *token, char *value, osbool *unknown_data);


/**
 * Export the transaction data from a file into CSV or TSV format.
 *
 * \param *file			The file to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

void transact_export_delimited(file_data *file, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Check the transactions in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool transact_check_account(file_data *file, int account);

#endif

