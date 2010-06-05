/* CashBook - find.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_FIND
#define _ACCOUNTS_FIND

/* ==================================================================================================================
 * Static constants
 */

#define FIND_ICON_OK 26
#define FIND_ICON_CANCEL 27

#define FIND_ICON_DATE 2
#define FIND_ICON_FMIDENT 4
#define FIND_ICON_FMREC 5
#define FIND_ICON_FMNAME 6
#define FIND_ICON_TOIDENT 8
#define FIND_ICON_TOREC 9
#define FIND_ICON_TONAME 10
#define FIND_ICON_REF 12
#define FIND_ICON_AMOUNT 14
#define FIND_ICON_DESC 16

#define FIND_ICON_AND 18
#define FIND_ICON_OR 19

#define FIND_ICON_CASE 17

#define FIND_ICON_START 22
#define FIND_ICON_DOWN 23
#define FIND_ICON_UP 25
#define FIND_ICON_END 24

#define FIND_TEST_DATE   0x01
#define FIND_TEST_FROM   0x02
#define FIND_TEST_TO     0x04
#define FIND_TEST_AMOUNT 0x08
#define FIND_TEST_REF    0x10
#define FIND_TEST_DESC   0x20

#define FOUND_ICON_CANCEL 1
#define FOUND_ICON_PREVIOUS 0
#define FOUND_ICON_NEXT 2
#define FOUND_ICON_NEW 3
#define FOUND_ICON_INFO 4

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* Open the find window. */

void open_find_window (file_data *file, wimp_pointer *ptr, int clear);
void reopen_find_window (wimp_pointer *ptr);
void refresh_find_window (void);
void fill_find_window (find *find_data, int clear);
void update_find_account_fields (wimp_key *key);
void open_find_account_menu (wimp_pointer *ptr);
void toggle_find_reconcile_fields (wimp_pointer *ptr);

/* Perform a find operation. */

int process_find_window (void);
int find_from_line (find *new_params, int new_dir, int line);

/* Force the window to close. */

void force_close_find_window (file_data *file);

#endif
