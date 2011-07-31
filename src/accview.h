/* CashBook - accview.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_ACCVIEW
#define CASHBOOK_ACCVIEW

#include "filing.h"

/* ==================================================================================================================
 * Static constants
 */
/* Toolbar icons */

#define ACCVIEW_PANE_DATE 0
#define ACCVIEW_PANE_FROMTO 1
#define ACCVIEW_PANE_REFERENCE 2
#define ACCVIEW_PANE_PAYMENTS 3
#define ACCVIEW_PANE_RECEIPTS 4
#define ACCVIEW_PANE_BALANCE 5
#define ACCVIEW_PANE_DESCRIPTION 6

#define ACCVIEW_PANE_PARENT 7
#define ACCVIEW_PANE_EDIT 8
#define ACCVIEW_PANE_GOTOEDIT 9
#define ACCVIEW_PANE_PRINT 10
#define ACCVIEW_PANE_SORT 11

#define ACCVIEW_PANE_SORT_DIR_ICON 12

#define ACCVIEW_COLUMN_RECONCILE 2

#define ACCVIEW_PANE_COL_MAP "0;1,2,3;4;5;6;7;8"

#define ACCVIEW_SORT_OK 2
#define ACCVIEW_SORT_CANCEL 3
#define ACCVIEW_SORT_DATE 4
#define ACCVIEW_SORT_FROMTO 5
#define ACCVIEW_SORT_REFERENCE 6
#define ACCVIEW_SORT_PAYMENTS 7
#define ACCVIEW_SORT_RECEIPTS 8
#define ACCVIEW_SORT_BALANCE 9
#define ACCVIEW_SORT_DESCRIPTION 10
#define ACCVIEW_SORT_ASCENDING 11
#define ACCVIEW_SORT_DESCENDING 12

/**
 * Initialise the account view system.
 *
 * \param *sprites		The application sprite area.
 */

void accview_initialise(osspriteop_area *sprites);


/**
 * Create and open a Account View window for the given file and account.
 *
 * \param *file			The file to open a window for.
 * \param account		The account to open a window for.
 */

void accview_open_window(file_data *file, acct_t account);


/**
 * Close and delete the Account View Window associated with the given
 * file block and account.
 *
 * \param *file			The file to use.
 * \param account		The account to close the window for.
 */

void accview_delete_window(file_data *file, acct_t account);


/**
 * Recreate the title of the specified Account View window connected to the
 * given file.
 *
 * \param *file			The file to rebuild the title for.
 * \param account		The account to rebilld the window title for.
 */

void accview_build_window_title(file_data *file, acct_t account);


/**
 * Force the closure of the Account List sort window if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void accview_force_windows_closed(file_data *file);


/**
 * Sort the account view list in a given file based on that file's sort setting.
 *
 * \param *file			The file to sort.
 * \param account		The account to sort.
 */

void accview_sort(file_data *file, int account);


/**
 * Rebuild a pre-existing account view from scratch, possibly becuase one of
 * the account's From/To entries has been changed, so all bets are off...
 * Delete the flex block and rebuild it, then resize the window and refresh the
 * whole thing.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be refreshed.
 */

void accview_rebuild(file_data *file, acct_t account);


/**
 * Recalculate the account view.  An amount entry or date has been changed, so
 * the number of transactions will remain the same.  Just re-fill the existing
 * flex block, then resize the window and refresh the whole thing.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be recalculated.
 * \param transaction		The transaction which has been changed.
 */

void accview_recalculate(file_data *file, acct_t account, int transaction);







int find_accview_line_from_transaction (file_data *file, int account, int transaction);

/* Account view creation. */

void refresh_account_view (file_data *file, int account, int transaction);

void reindex_all_account_views (file_data *file);
void redraw_all_account_views (file_data *file);
void recalculate_all_account_views (file_data *file);
void rebuild_all_account_views (file_data *file);



/* Account View window handling. */


int align_accview_with_transact_line (file_data *file, int account);
int align_accview_with_transact_y_offset (file_data *file, int account);
void align_accview_with_transact (file_data *file, int account);
void find_accview_line (file_data *file, int account, int line);



/**
 * Export the account view transaction data from a file into CSV or TSV format.
 *
 * \param *file			The file to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

void accview_export_delimited(file_data *file, acct_t account, char *filename, enum filing_delimit_type format, int filetype);

#endif

