/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: account.h
 *
 * Account and account list implementation.
 */

#ifndef CASHBOOK_ACCOUNT
#define CASHBOOK_ACCOUNT

#include <stdio.h>

typedef int acct_t;
struct account_block;

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

/**
 * The length of an account name.
 */

#define ACCOUNT_NAME_LEN 32

/**
 * The length of an account ident.
 */

#define ACCOUNT_IDENT_LEN 5

/**
 * The length of a section name in an account list.
 */

#define ACCOUNT_SECTION_LEN 52

#define NULL_ACCOUNT ((acct_t) -1)

/**
 * Account types
 *
 * (bitwise allocation, to allow oring of values)
 */

enum account_type {
	ACCOUNT_NULL     = 0x0000,						/**< Unset account type.				*/
	ACCOUNT_FULL     = 0x0001,						/**< Bank account type.					*/
	ACCOUNT_IN       = 0x0100,						/**< Income enabled analysis header.			*/
	ACCOUNT_OUT      = 0x0200,						/**< Outgoing enabled analysis header.			*/
	ACCOUNT_FULL_IN  = 0x0101,						/**< Income analysis header and bank account combo.	*/
	ACCOUNT_FULL_OUT = 0x0201						/**< Outgoing analysis header and bank account combo.	*/
};

/**
 * Account window line types
 */

enum account_line_type {
	ACCOUNT_LINE_BLANK = 0,							/**< Blank, unset line type.				*/
	ACCOUNT_LINE_DATA,							/**< Data line type.					*/
	ACCOUNT_LINE_HEADER,							/**< Section Heading line type.				*/
	ACCOUNT_LINE_FOOTER							/**< Section Footer line type.				*/
};

struct account_window;

/**
 * Get an account field from an input file.
 */

#define account_get_account_field(in) ((acct_t) filing_get_int_field((in)))

/**
 * Get an account type field from an input file.
 */

#define account_get_account_type_field(in) ((enum account_type) filing_get_int_field((in)))

/**
 * Get an account line type field from an input file.
 */

#define account_get_account_line_type_field(in) ((enum account_line_type) filing_get_int_field((in)))

#include "currency.h"
#include "filing.h"
#include "global.h"
#include "transact.h"


/**
 * Initialise the account system.
 *
 * \param *sprites		The application sprite area.
 */

void account_initialise(osspriteop_area *sprites);


/**
 * Create a new Account window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct account_block *account_create_instance(struct file_block *file);


/**
 * Delete an Account window instance, and all of its data.
 *
 * \param *block		The instance to be deleted.
 */

void account_delete_instance(struct account_block *block);


/**
 * Create and open an Accounts window for the given file and account type.
 *
 * \param *file			The file to open a window for.
 * \param type			The type of account to open a window for.
 */

void account_open_window(struct file_block *file, enum account_type type);


/**
 * Recreate the titles of all the account list and account view
 * windows associated with a file.
 *
 * \param *file			The file to rebuild the titles for.
 */

void account_build_window_titles(struct file_block *file);


/**
 * Force the complete redraw of all the account windows.
 *
 * \param *file			The file owning the windows to redraw.
 */

void account_redraw_all(struct file_block *file);


/**
 * Open the Account Edit dialogue for a given account list window.
 *
 * If account == NULL_ACCOUNT, type determines the type of the new account
 * to be created.  Otherwise, type is ignored and the type derived from the
 * account data block.
 *
 * \param *file			The file to own the dialogue.
 * \param account		The account to edit, or NULL_ACCOUNT for add new.
 * \param type			The type of new account to create if account
 *				is NULL_ACCOUNT.
 * \param *ptr			The current Wimp pointer position.
 */

void account_open_edit_window(struct file_block *file, acct_t account, enum account_type type, wimp_pointer *ptr);


/**
 * Create a new account with null details.  Core details are set up, but some
 * values are zeroed and left to be set up later.
 *
 * \param *file			The file to add the account to.
 * \param *name			The name to give the account.
 * \param *ident		The ident to give the account.
 * \param type			The type of account to be created.
 * \return			The new account index, or NULL_ACCOUNT.
 */

acct_t account_add(struct file_block *file, char *name, char *ident, enum account_type type);


/**
 * Delete an account from a file.
 *
 * \param *file			The file to act on.
 * \param account		The account to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

osbool account_delete(struct file_block *file, acct_t account);


/**
 * Purge unused accounts from a file.
 *
 * \param *file			The file to purge.
 * \param accounts		TRUE to remove accounts; FALSE to ignore.
 * \param headings		TRUE to remove headings; FALSE to ignore.
 */

void account_purge(struct file_block *file, osbool accounts, osbool headings);


/**
 * Find the number of accounts in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of accounts in the file.
 */

int account_get_count(struct file_block *file);


/**
 * Find the number of entries in the account window of a given account type.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \return			The number of entries, or 0.
 */

int account_get_list_length(struct file_block *file, enum account_type type);


/**
 * Return the  type of a given line of an account list window.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \param line			The line to return the details for.
 * \return			The type of data on that line.
 */

enum account_line_type account_get_list_entry_type(struct file_block *file, enum account_type type, int line);


/**
 * Return the account on a given line of an account list window.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \param line			The line to return the details for.
 * \return			The account on that line, or NULL_ACCOUNT if the
 *				line isn't an account.
 */

acct_t account_get_list_entry_account(struct file_block *file, enum account_type type, int line);


/**
 * Return the text on a given line of an account list window.
 *
 * \param *file			The file to use.
 * \param type			The type of account window to query.
 * \param line			The line to return the details for.
 * \return			A volatile pointer to the text on the line,
 *				or NULL.
 */

char *account_get_list_entry_text(struct file_block *file, enum account_type type, int line);


/**
 * Find an account by looking up an ident string against accounts of a
 * given type.
 *
 * \param *file			The file containing the account.
 * \param *ident		The ident to look up.
 * \param type			The type(s) of account to include.
 * \return			The account number, or NULL_ACCOUNT if not found.
 */

acct_t account_find_by_ident(struct file_block *file, char *ident, enum account_type type);


/**
 * Return a pointer to a string repesenting the ident of an account, or ""
 * if the account is not valid.
 *
 * \param *file			The file containing the account.
 * \param account		The account to return an ident for.
 * \return			Pointer to the ident string, or "".
 */

char *account_get_ident(struct file_block *file, acct_t account);


/**
 * Return a pointer to a string repesenting the name of an account, or ""
 * if the account is not valid.
 *
 * \param *file			The file containing the account.
 * \param account		The account to return an name for.
 * \return			Pointer to the name string, or "".
 */

char *account_get_name(struct file_block *file, acct_t account);


/**
 * Build a textual "Ident:Account Name" pair for the given account, and
 * insert it into the supplied buffer.
 *
 * \param *file			The file containing the account.
 * \param account		The account to give a name to.
 * \param *buffer		The buffer to take the Ident:Name.
 * \param size			The number of bytes in the buffer.
 * \return			Pointer to the start of the ident.
 */

char *account_build_name_pair(struct file_block *file, acct_t account, char *buffer, size_t size);


/**
 * Take a keypress into an account ident field, and look it up against the
 * accounts list. Update the associated name and reconciled icons, and return
 * the matched account plus reconciled state.
 *
 * \param *file		The file containing the account.
 * \param key		The keypress to process.
 * \param type		The types of account that we're interested in.
 * \param account	The account that is currently in the field.
 * \param *reconciled	A pointer to a variable to take the new reconciled state
 *			on exit, or NULL to return none.
 * \param window	The window containing the icons.
 * \param ident		The icon holding the ident.
 * \param name		The icon holding the account name.
 * \param rec		The icon holding the reconciled state.
 * \return		The new account number.
 */

acct_t account_lookup_field(struct file_block *file, char key, enum account_type type, acct_t account,
		osbool *reconciled, wimp_w window, wimp_i ident, wimp_i name, wimp_i rec);


/**
 * Fill an account field (ident, reconciled and name icons) with the details
 * of an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account to be shown in the field.
 * \param reconciled	TRUE to show the account reconciled; FALSE to show unreconciled.
 * \param window	The window containing the icons.
 * \param ident		The icon holding the ident.
 * \param name		The icon holding the account name.
 * \param rec		The icon holding the reconciled state.
 */

void account_fill_field(struct file_block *file, acct_t account, osbool reconciled,
		wimp_w window, wimp_i ident, wimp_i name, wimp_i rec);


/**
 * Toggle the reconcile status shown in an icon.
 *
 * \param window	The window containing the icon.
 * \param icon		The icon to toggle.
 */

void account_toggle_reconcile_icon(wimp_w window, wimp_i icon);


/**
 * Return the account view handle for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account to return.
 * \return		The account's account view, or NULL.
 */

struct accview_window *account_get_accview(struct file_block *file, acct_t account);


/**
 * Set the account view handle for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account to set the view of.
 * \param *view		The view handle, or NULL to clear.
 */

void account_set_accview(struct file_block *file, acct_t account, struct accview_window *view);


/**
 * Return the type of an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the type.
 * \return		The account's type, or ACCOUNT_NULL.
 */

enum account_type account_get_type(struct file_block *file, acct_t account);


/**
 * Return the opening balance for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the opening balance.
 * \return		The account's opening balance, or 0.
 */

amt_t account_get_opening_balance(struct file_block *file, acct_t account);


/**
 * Adjust the opening balance for an account by adding or subtracting a
 * specified amount.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to alter the opening balance.
 * \param adjust	The amount to alter the opening balance by.
 */

void account_adjust_opening_balance(struct file_block *file, acct_t account, amt_t adjust);


/**
 * Return the credit limit for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the opening balance.
 * \return		The account's opening balance, or 0.
 */

amt_t account_get_credit_limit(struct file_block *file, acct_t account);


/**
 * Return the budget amount for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the budget amount.
 * \return		The account's budget amount, or 0.
 */

amt_t account_get_budget_amount(struct file_block *file, acct_t account);


/**
 * Zero the standing order trial balances for all of the accounts in a file.
 *
 * \param *file		The file to zero.
 */

void account_zero_sorder_trial(struct file_block *file);


/**
 * Adjust the standing order trial balance for an account by adding or
 * subtracting a specified amount.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to alter the standing order
 *			trial balance.
 * \param adjust	The amount to alter the standing order trial
 *			balance by.
 */

void account_adjust_sorder_trial(struct file_block *file, acct_t account, amt_t adjust);


/**
 * Count the number of accounts of a given type in a file.
 *
 * \param *file			The account to use.
 * \param type			The type of account to count.
 * \return			The number of accounts found.
 */

int account_count_type_in_file(struct file_block *file, enum account_type type);


/**
 * Test if an account supports insertion of cheque numbers.
 *
 * \param *file			The file containing the account.
 * \param account		The account to check.
 * \return			TRUE if cheque numbers are available; else FALSE.
 */

osbool account_cheque_number_available(struct file_block *file, acct_t account);


/**
 * Get the next cheque or paying in book number for a given combination of from and
 * to accounts, storing the value as a string in the buffer and incrementing the
 * next value for the chosen account as specified.
 *
 * \param *file			The file containing the accounts.
 * \param from_account		The account to transfer from.
 * \param to_account		The account to transfer to.
 * \param increment		The amount to increment the chosen number after
 *				use (0 for no increment).
 * \param *buffer		Pointer to the buffer to hold the number.
 * \param size			The size of the buffer.
 * \return			A pointer to the supplied buffer, containing the
 *				next available number.
 */

char *account_get_next_cheque_number(struct file_block *file, acct_t from_account, acct_t to_account, int increment, char *buffer, size_t size);


/**
 * Fully recalculate all of the accounts in a file.
 *
 * \param *file		The file to recalculate.
 */

void account_recalculate_all(struct file_block *file);


/**
 * Remove a transaction from all the calculated accounts, so that limited
 * changes can be made to its details. Once updated, it can be resored
 * using account_restore_transaction(). The changes can not affect the sort
 * order of the transactions in the file, or the restoration will be invalid.
 *
 * \param *file		The file containing the transaction.
 * \param transasction	The transaction to remove.
 */

void account_remove_transaction(struct file_block *file, tran_t transaction);


/**
 * Restore a transaction previously removed by account_remove_transaction()
 * after changes have been made, recalculate the affected accounts and
 * refresh any affected displays. The changes can not affect the sort
 * order of the transactions in the file, or the restoration will be invalid.
 *
 * \param *file		The file containing the transaction.
 * \param transasction	The transaction to restore.
 */

void account_restore_transaction(struct file_block *file, tran_t transaction);


/**
 * Save the account and account list details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void account_write_file(struct file_block *file, FILE *out);


/**
 * Read account details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool account_read_acct_file(struct file_block *file, struct filing_block *in);


/**
 * Read account list details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool account_read_list_file(struct file_block *file, struct filing_block *in);

#endif

