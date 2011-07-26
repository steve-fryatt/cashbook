/* CashBook - file.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_FILE
#define _ACCOUNTS_FILE

/* ==================================================================================================================
 * Static constants
 */


/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* File initialisation and deletion */

file_data *build_new_file_block (void);
void create_new_file (void);
void delete_file (file_data *file);

/* Finding files */

file_data *find_transaction_window_file_block (wimp_w window);
file_data *find_transaction_pane_file_block (wimp_w window);
file_data *find_account_window_file_block (wimp_w window);
file_data *find_account_pane_file_block (wimp_w window);

/* Saved files and data integrity */

int  check_for_unsaved_files (void);
void set_file_data_integrity (file_data *data, int unsafe);
int check_for_filepath (file_data *file);

/* Default filenames. */

char *make_file_pathname (file_data *file, char *path, int len);
char *make_file_leafname (file_data *file, char *leaf, int len);
char *generate_default_title (file_data *file, char *name, int length);

/* General file redraw. */

void redraw_all_files (void);
void redraw_file_windows (file_data *file);

/* A change of date. */

void update_files_for_new_date (void);

#endif
