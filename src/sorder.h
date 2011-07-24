/* CashBook - sorder.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_SORDER
#define CASHBOOK_SORDER

#include "filing.h"

#define SORDER_PANE_COL_MAP "0,1,2;3,4,5;6;7;8;9"
#define SORDER_PANE_SORT_DIR_ICON 10


/**
 * Initialise the standing order system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_initialise(osspriteop_area *sprites);


/**
 * Create and open a Standing Order List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void sorder_open_window(file_data *file);


/**
 * Close and delete the Standing order List Window associated with the given
 * file block.
 *
 * \param *file			The file to use.
 */

void sorder_delete_window(file_data *file);


/**
 * Recreate the title of the Standing Order List window connected to the given
 * file.
 *
 * \param *file			The file to rebuild the title for.
 */

void sorder_build_window_title(file_data *file);


/**
 * Force a redraw of the Standing Order list window, for the given range of
 * lines.
 *
 * \param *file			The file owning the window.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

void sorder_force_window_redraw(file_data *file, int from, int to);


/**
 * Open the Standing Order Edit dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void sorder_open_edit_window(file_data *file, int sorder, wimp_pointer *ptr);


/**
 * Force the closure of the Standing Order sort and edit windows if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void sorder_force_windows_closed(file_data *file);


/**
 * Sort the standing orders in a given file based on that file's sort setting.
 *
 * \param *file			The file to sort.
 */

void sorder_sort(file_data *file);


/**
 * Purge unused standing orders from a file.
 *
 * \param *file			The file to purge.
 */

void sorder_purge(file_data *file);


/**
 * Scan the standing orders in a file, adding transactions for any which have
 * fallen due.
 *
 * \param *file			The file to process.
 */

void sorder_process(file_data *file);


/**
 * Scan the standing orders in a file, and update the traial values to reflect
 * any pending transactions.
 *
 * \param *file			The file to scan.
 */

void sorder_trial(file_data *file);


/**
 * Generate a report detailing all of the standing orders in a file.
 *
 * \param *file			The file to report on.
 */

void sorder_full_report(file_data *file);


/**
 * Export the standing order data from a file into CSV or TSV format.
 *
 * \param *file			The file to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

void sorder_export_delimited(file_data *file, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Check the standing orders in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool sorder_check_account(file_data *file, int account);

#endif

