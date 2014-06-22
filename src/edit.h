/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
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
 * \file: edit.h
 *
 * Transaction editing implementation.
 */

#ifndef CASHBOOK_EDIT
#define CASHBOOK_EDIT

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define EDIT_ICON_ROW 0
#define EDIT_ICON_DATE 1
#define EDIT_ICON_FROM 2
#define EDIT_ICON_FROM_REC 3
#define EDIT_ICON_FROM_NAME 4
#define EDIT_ICON_TO 5
#define EDIT_ICON_TO_REC 6
#define EDIT_ICON_TO_NAME 7
#define EDIT_ICON_REF 8
#define EDIT_ICON_AMOUNT 9
#define EDIT_ICON_DESCRIPT 10


extern wimp_window *edit_transact_window_def;


/**
 * Create an edit line at the specified point in the given file's transaction
 * window. Any existing edit line is deleted first.
 *
 * The caret isn't placed in this routine.  That is left up to the caller, so
 * that they can place it depending on their context.
 *
 * \param *file		The file to place the edit line in.
 * \param line		The line to place the edit line at.
 */

void edit_place_new_line(file_data *file, int line);


/**
 * Place a new edit line by raw transaction number.
 *
 * \param *file		The file to place the line in.
 * \param transaction	The transaction to place the line on.
 */

void edit_place_new_line_by_transaction(file_data *file, int transaction);


/**
 * Inform the edit line code that a file has been deleted: this removes any
 * references to the edit line if it is within that file's transaction window.
 * 
 * Note that it isn't possible to delete an edit line and its icons: it will
 * only be completely destroyed if the parent window is deleted.
 *
 * \param *file		The file to be deleted.
 */

void edit_file_deleted(file_data *file);


/**
 * Bring the edit line into view in the window in a vertical direction.
 *
 * \param *file		The file that we're interested in working on
 */

void edit_find_line_vertically(file_data *file);


/**
 * Refresh the contents of the edit line icons, copying the contents of memory
 * back into them.
 *
 * \param w		If NULL, refresh any window; otherwise, only refresh if
 *			the parent transaction window handle matches w.
 * \param i		If -1, refresh all icons in the line; otherwise, only
 *			refresh if the icon handle matches i.
 * \param avoid		If -1, refresh all icons in the line; otherwise, only
 *			refresh if the icon handle does not match avoid.
 */

void edit_refresh_line_content(wimp_w w, wimp_i only, wimp_i avoid);


/**
 * Get the underlying transaction number relating to the current edit line
 * position.
 *
 * \param *file		The file that we're interested in.
 * \return		The transaction number, or NULL_TRANSACTION if the
 *			line isn't in the specified file.
 */

int edit_get_line_transaction(file_data *file);


/**
 * Toggle the state of one of the reconciled flags for a transaction.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to edit.
 * \param change_flag	Indicate which reconciled flags to change.
 */

void edit_toggle_reconcile_flag(file_data *file, int transaction, int change_flag);


void edit_change_transaction_date(file_data *file, int transaction, date_t new_date);
void edit_change_transaction_refdesc(file_data *file, int transaction, int change_icon, char *new_text);
void edit_change_transaction_account(file_data *file, int transaction, int change_icon, acct_t new_account);
void insert_transaction_preset_full(file_data *file, int transaction, int preset);

/* Keypress handling */

osbool process_transaction_edit_line_keypress(file_data *file, wimp_key *key);

#endif
