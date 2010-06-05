/* CashBook - calculation.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_CALCULATION
#define _ACCOUNTS_CALCULATION

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */


/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Full recalculation */

void perform_full_recalculation (file_data *file);

void recalculate_account_windows (file_data *file);

/* Partial calculation */

void remove_transaction_from_totals (file_data *file, int transaction);
void restore_transaction_to_totals (file_data *file, int transaction);

#endif
