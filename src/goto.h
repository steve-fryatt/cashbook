/* CashBook - goto.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_GOTO
#define _ACCOUNTS_GOTO

/* ==================================================================================================================
 * Static constants
 */

#define GOTO_ICON_OK 0
#define GOTO_ICON_CANCEL 1
#define GOTO_ICON_NUMBER_FIELD 3
#define GOTO_ICON_NUMBER 4
#define GOTO_ICON_DATE 5

#define GOTO_TYPE_LINE 0
#define GOTO_TYPE_DATE 1

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* Open the goto window. */

void open_goto_window (file_data *file, wimp_pointer *ptr, int clear);
void refresh_goto_window (void);
void fill_goto_window (go_to *go_to_data, int clear);

/* Perform a goto operation. */

int process_goto_window (void);

/* Force the window to close. */

void force_close_goto_window (file_data *file);

#endif
