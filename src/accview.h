/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: accview.h
 *
 * Account statement view implementation
 */

#ifndef CASHBOOK_ACCVIEW
#define CASHBOOK_ACCVIEW

#include "account.h"
#include "filing.h"


/**
 * Initialise the account view system.
 *
 * \param *sprites		The application sprite area.
 */

void accview_initialise(osspriteop_area *sprites);


/**
 * Create a new global account view module instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct accview_block *accview_create_instance(struct file_block *file);


/**
 * Delete a global account view module instance, and all of its data.
 *
 * \param *instance		The instance to be deleted.
 */

void accview_delete_instance(struct accview_block *instance);


/**
 * Create and open a Account View window for the given file and account.
 *
 * \param *file			The file to open a window for.
 * \param account		The account to open a window for.
 */

void accview_open_window(struct file_block *file, acct_t account);


/**
 * Close and delete the Account View Window associated with the given
 * file block and account.
 *
 * \param *file			The file to use.
 * \param account		The account to close the window for.
 */

void accview_delete_window(struct file_block *file, acct_t account);


/**
 * Recreate the title of the specified Account View window connected to the
 * given file.
 *
 * \param *file			The file to rebuild the title for.
 * \param account		The account to rebilld the window title for.
 */

void accview_build_window_title(struct file_block *file, acct_t account);


/**
 * Sort the account view list in a given file based on that file's sort setting.
 *
 * \param *file			The file to sort.
 * \param account		The account to sort.
 */

void accview_sort(struct file_block *file, acct_t account);


/**
 * Rebuild a pre-existing account view from scratch, possibly becuase one of
 * the account's From/To entries has been changed, so all bets are off...
 * Delete the flex block and rebuild it, then resize the window and refresh the
 * whole thing.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be refreshed.
 */

void accview_rebuild(struct file_block *file, acct_t account);


/**
 * Recalculate the account view.  An amount entry or date has been changed, so
 * the number of transactions will remain the same.  Just re-fill the existing
 * flex block, then resize the window and refresh the whole thing.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be recalculated.
 * \param transaction		The transaction which has been changed.
 */

void accview_recalculate(struct file_block *file, acct_t account, int transaction);


/**
 * Redraw the line in an account view corresponding to the given transaction.
 * If the transaction does not feature in the account, nothing is done.
 *
 * \param *file			The file containing the account.
 * \param account		The account to be redrawn.
 * \param transaction		The transaction to be redrawn.
 */

void accview_redraw_transaction(struct file_block *file, acct_t account, int transaction);


/**
 * Re-index the account views in a file.  This can *only* be done after
 * transact_sort_file_data() has been called, as it requires data set up
 * in the transaction block by that call.
 *
 * \param *file			The file to reindex.
 */

void accview_reindex_all(struct file_block *file);


/**
 * Fully redraw all of the open account views in a file.
 *
 * \param *file			The file to be redrawn.
 */

void accview_redraw_all(struct file_block *file);


/**
 * Fully recalculate all of the open account views in a file.
 *
 * \param *file			The file to be recalculated.
 */

void accview_recalculate_all(struct file_block *file);


/**
 * Fully rebuild all of the open account views in a file.
 *
 * \param *file			The file to be rebuilt.
 */

void accview_rebuild_all(struct file_block *file);


/**
 * Save the accview details to a CashBook file.
 * 
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void accview_write_file(struct file_block *file, FILE *out);


/**
 * Process a WinColumns line from the Accounts section of a file.
 *
 * \param *file			The file being read in to.
 * \param format		The format of the disc file.
 * \param *columns		The column text line.
 */

void accview_read_file_wincolumns(struct file_block *file, int format, char *columns);


/**
 * Process a SortOrder line from the Accounts section of a file.
 *
 * \param *file			The file being read in to.
 * \param *columns		The sort order text line.
 */

void accview_read_file_sortorder(struct file_block *file, char *order);

#endif

