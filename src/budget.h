/* CashBook - budget.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_BUDGET
#define _ACCOUNTS_BUDGET

/* ==================================================================================================================
 * Static constants
 */

#define BUDGET_ICON_OK 0
#define BUDGET_ICON_CANCEL 1

#define BUDGET_ICON_START 5
#define BUDGET_ICON_FINISH 7
#define BUDGET_ICON_TRIAL 11
#define BUDGET_ICON_RESTRICT 13

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* Open the budget window. */

void open_budget_window (file_data *file, wimp_pointer *ptr);
void refresh_budget_window (void);
void fill_budget_window (file_data *file);

/* Update the budget details. */

int process_budget_window (void);

/* Force the window to close. */

void force_close_budget_window (file_data *file);


#endif
