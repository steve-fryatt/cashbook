/* CashBook - continue.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_CONTINUE
#define _ACCOUNTS_CONTINUE

/* ==================================================================================================================
 * Static constants
 */

#define CONTINUE_ICON_OK 6
#define CONTINUE_ICON_CANCEL 7

#define CONTINUE_ICON_TRANSACT 0
#define CONTINUE_ICON_ACCOUNTS 3
#define CONTINUE_ICON_HEADINGS 4
#define CONTINUE_ICON_SORDERS 5

#define CONTINUE_ICON_DATE 2
#define CONTINUE_ICON_DATETEXT 1

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* Open the continue window. */

void open_continue_window (file_data *file, wimp_pointer *ptr, int clear);
void refresh_continue_window (void);
void fill_continue_window (continuation *cont_data, int clear);

/* Perform a continue operation. */

int process_continue_window (void);

/* Force the window to close. */

void force_close_continue_window (file_data *file);

/* Purge a file of unwanted trasactions, accounts and standing orders. */

void purge_file (file_data *file, int transactions, date_t date, int accounts, int headings, int sorders);

#endif
