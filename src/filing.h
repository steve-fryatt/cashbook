/* CashBook - filing.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_FILING
#define _ACCOUNTS_FILING

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define LOAD_SECT_NONE     0
#define LOAD_SECT_BUDGET   1
#define LOAD_SECT_ACCOUNTS 2
#define LOAD_SECT_ACCLIST  3
#define LOAD_SECT_TRANSACT 4
#define LOAD_SECT_SORDER   5
#define LOAD_SECT_PRESET   6
#define LOAD_SECT_REPORT   7

#define DELIMIT_TAB 1
#define DELIMIT_COMMA 2
#define DELIMIT_QUOTED_COMMA 3

#define DELIMIT_LAST 0x01
#define DELIMIT_NUM 0x02

#define ICOMP_ICON_IMPORTED 0
#define ICOMP_ICON_REJECTED 2
#define ICOMP_ICON_CLOSE 5
#define ICOMP_ICON_LOG 4

#define MAX_FILE_LINE_LEN 1024

/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Loading accounts files */

void load_transaction_file (char *filename);
char *next_plain_field (char *line, char sep);

/* Saving accounts files */

void save_transaction_file (file_data *file, char *filename);

/* Delimited file import */

void import_csv_file (file_data *file, char *filename);

void close_import_complete_dialogue (int show_log);
void force_close_import_window (file_data *file);

/* Delimited file export */

void export_delimited_file (file_data *file, char *filename, int format, int filetype);
void export_delimited_accounts_file (file_data *file, int entry, char *filename, int format, int filetype);
void export_delimited_account_file (file_data *file, int account, char *filename, int format, int filetype);
void export_delimited_sorder_file (file_data *file, char *filename, int format, int filetype);
void export_delimited_preset_file (file_data *file, char *filename, int format, int filetype);

/* Delimited string output */

int delimited_field_output (FILE *f, char *string, int format, int flags);

/* String processing */

char *unquote_string (char *string);
char *next_field (char *line, char sep);

#endif
