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
 * \file: account.c
 *
 * Account and account list implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"
#include "oslib/dragasprite.h"
#include "oslib/wimpspriteop.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "account.h"

#include "account_account_dialogue.h"
#include "account_heading_dialogue.h"
#include "account_idnum.h"
#include "account_menu.h"
#include "account_section_dialogue.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "interest.h"
#include "presets.h"
#include "print_dialogue.h"
#include "report.h"
#include "sorder.h"
#include "stringbuild.h"
#include "transact.h"
#include "window.h"


/**
 * Account data structure -- implementation.
 */

struct account {
	char			name[ACCOUNT_NAME_LEN];
	char			ident[ACCOUNT_IDENT_LEN];

	char			account_no[ACCOUNT_NO_LEN];
	char			sort_code[ACCOUNT_SRTCD_LEN];
	char			address[ACCOUNT_ADDR_LINES][ACCOUNT_ADDR_LEN];

	enum account_type	type;						/**< The type of account being defined.				*/
	//unsigned category;

	/* Pointer to account view window data. */

	struct accview_window	*account_view;

	/* Cheque and Paying In Numbers. */

	struct account_idnum	cheque_number;
	struct account_idnum	payin_number;

	/* Interest details. (the rates are held in the interest module). */

	acct_t			offset_against;					/* The account against which interest is offset, or NULL_ACCOUNT. */

	/* User-set values used for calculation. */

	amt_t			opening_balance;				/* The opening balance for accounts, from which everything else is calculated. */
	amt_t			credit_limit;					/* Credit limit for accounts. */
	amt_t			budget_amount;					/* Budgeted amount for headings. */

	/* Calculated values for both accounts and headings. */

	amt_t			statement_balance;				/* Reconciled statement balance. */
	amt_t			current_balance;				/* Balance up to today's date. */
	amt_t			future_balance;					/* Balance including all transactions. */
	amt_t			budget_balance;					/* Balance including all transactions betwen budget dates. */

	amt_t			sorder_trial;					/* Difference applied to account from standing orders in trial period. */

	/* Subsequent calculated values for accounts. */

	amt_t			trial_balance;					/* Balance including all transactions & standing order trial. */
	amt_t			available_balance;				/* Balance available, taking into account credit limit. */

	/* Subsequent calculated values for headings. */

	amt_t			budget_result;
};

struct account_block {
	struct file_block	*file;						/**< The file owning the block.					*/

	/* The account list windows. */

	struct account_window	account_windows[ACCOUNT_WINDOWS];

	/* Account Data. */

	struct account		*accounts;					/**< The account data for the defined accounts			*/
	int			account_count;					/**< The number of accounts defined in the file.		*/

	/* Recalculation data. */

	date_t			last_full_recalc;				/**< The last time a full recalculation was done on the file.	*/
};




static void			account_add_to_lists(struct file_block *file, acct_t account);
static int			account_add_list_display_line(struct file_block *file, int entry);
static int			account_find_window_entry_from_type(struct file_block *file, enum account_type type);
static osbool			account_used_in_file(struct account_block *instance, acct_t account);
static osbool			account_check_account(struct account_block *instance, acct_t account);


static void			account_recalculate_windows(struct file_block *file);

/**
 * Test whether an account number is safe to look up in the account data array.
 */

#define account_valid(windat, account) (((account) != NULL_ACCOUNT) && ((account) >= 0) && ((account) < ((windat)->account_count)))

/**
 * Initialise the account system.
 *
 * \param *sprites		The application sprite area.
 */

void account_initialise(osspriteop_area *sprites)
{
	account_list_window_initialise(sprites);
	account_account_dialogue_initialise();
	account_heading_dialogue_initialise();
	account_section_dialogue_initialise();
}


/**
 * Create a new Account window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct account_block *account_create_instance(struct file_block *file)
{
	struct account_block	*new;
	int			i;
	osbool			mem_fail = FALSE;

	new = heap_alloc(sizeof(struct account_block));
	if (new == NULL)
		return NULL;

	/* Construct the new account block. */

	new->file = file;

	new->accounts = NULL;
	new->account_count = 0;

	new->last_full_recalc = NULL_DATE;

	/* Initialise the account and heading windows. */

	for (i = 0; i < ACCOUNT_WINDOWS; i++) {
		new->account_windows[i].instance = new;
		new->account_windows[i].entry = i;

		new->account_windows[i].account_window = NULL;
		new->account_windows[i].account_pane = NULL;
		new->account_windows[i].account_footer = NULL;
		new->account_windows[i].columns = NULL;

		new->account_windows[i].display_lines = 0;
		new->account_windows[i].line_data = NULL;

		/* Blank out the footer icons. */

		*new->account_windows[i].footer_icon[ACCOUNT_NUM_COLUMN_STATEMENT] = '\0';
		*new->account_windows[i].footer_icon[ACCOUNT_NUM_COLUMN_CURRENT] = '\0';
		*new->account_windows[i].footer_icon[ACCOUNT_NUM_COLUMN_FINAL] = '\0';
		*new->account_windows[i].footer_icon[ACCOUNT_NUM_COLUMN_BUDGET] = '\0';

		/* Set the individual windows to the type of account they will hold.
		 *
		 * \TODO -- Ick! Is there really no better way to do this? 
		 */

		switch (i) {
		case 0:
			new->account_windows[i].type = ACCOUNT_FULL;
			break;

		case 1:
			new->account_windows[i].type = ACCOUNT_IN;
			break;

		case 2:
			new->account_windows[i].type = ACCOUNT_OUT;
			break;
		}

		/* If there was a memory failure, we only want to initialise the block enough
		 * that it can be destroyed again. However, we do need to initialise all of
		 * the blocks so that everything is set to NULL that needs to be.
		 */

		if (mem_fail == TRUE)
			continue;

		new->account_windows[i].columns = column_create_instance(ACCOUNT_COLUMNS, account_columns, account_extra_columns, wimp_ICON_WINDOW);
		if (new->account_windows[i].columns == NULL) {
			mem_fail = TRUE;
			continue;
		}

		column_set_minimum_widths(new->account_windows[i].columns, config_str_read("LimAccountCols"));
		column_init_window(new->account_windows[i].columns, 0, FALSE, config_str_read("AccountCols"));

		/* Set the initial lines up */

		if (!flexutils_initialise((void **) &(new->account_windows[i].line_data))) {
			mem_fail = TRUE;
			continue;
		}
	}

	/* Set up the account data structures. */

	if (mem_fail || (!flexutils_initialise((void **) &(new->accounts)))) {
		account_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete an Account window instance, and all of its data.
 *
 * \param *block		The instance to be deleted.
 */

void account_delete_instance(struct account_block *block)
{
	int	i;
	acct_t	account;

	if (block == NULL)
		return;

	/* Step through the account list windows. */

	for (i = 0; i < ACCOUNT_WINDOWS; i++) {
		if (block->account_windows[i].line_data != NULL)
			flexutils_free((void **) &(block->account_windows[i].line_data));

		column_delete_instance(block->account_windows[i].columns);

		account_delete_window(&(block->account_windows[i]));
	}

	/* Step through the accounts and their account view windows. */

	for (account = 0; account < block->account_count; account++) {
		if (block->accounts[account].account_view != NULL) {
#ifdef DEBUG
			debug_printf("Account %d has a view to delete.", account);
#endif
			accview_delete_window(block->file, account);
		}
	}

	if (block->accounts != NULL)
		flexutils_free((void **) &(block->accounts));

	heap_free(block);
}


/**
 * Recreate the titles of all the account list and account view
 * windows associated with a file.
 *
 * \param *file			The file to rebuild the titles for.
 */

void account_build_window_titles(struct file_block *file)
{
	int	entry;
	acct_t	account;

	if (file == NULL || file->accounts == NULL)
		return;

	for (entry = 0; entry < ACCOUNT_WINDOWS; entry++)
		account_build_window_title(file, entry);

	for (account = 0; account < file->accounts->account_count; account++)
		accview_build_window_title(file, account);
}


/**
 * Force the complete redraw of all the account windows.
 *
 * \param *file			The file owning the windows to redraw.
 */

void account_redraw_all(struct file_block *file)
{
	int	i;

	if (file == NULL || file->accounts == NULL)
		return;

	for (i = 0; i < ACCOUNT_WINDOWS; i++)
		account_force_window_redraw(file, i, 0, file->accounts->account_windows[i].display_lines, wimp_ICON_WINDOW);
}


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

acct_t account_add(struct file_block *file, char *name, char *ident, enum account_type type)
{
	acct_t		new = NULL_ACCOUNT;
	int		i;

	if (file == NULL || file->accounts == NULL)
		return NULL_ACCOUNT;

	if (strcmp(ident, "") == 0) {
		error_msgs_report_error("BadAcctIdent");
		return new;
	}

	/* First, look for deleted accounts and re-use the first one found. */

	for (i = 0; i < file->accounts->account_count; i++) {
		if (file->accounts->accounts[i].type == ACCOUNT_NULL) {
			new = i;
			#ifdef DEBUG
			debug_printf("Found empty account: %d", new);
			#endif
			break;
		}
	}

	/* If that fails, create a new entry. */

	if (new == NULL_ACCOUNT) {
		if (flexutils_resize((void **) &(file->accounts->accounts), sizeof(struct account), file->accounts->account_count + 1)) {
			new = file->accounts->account_count++;
			#ifdef DEBUG
			debug_printf("Created new account: %d", new);
			#endif
		}
	}

	/* If a new account was created, fill it. */

	if (new == NULL_ACCOUNT) {
		error_msgs_report_error("NoMemNewAcct");
		return new;
	}

	strcpy(file->accounts->accounts[new].name, name);
	strcpy(file->accounts->accounts[new].ident, ident);
	file->accounts->accounts[new].type = type;
	file->accounts->accounts[new].opening_balance = 0;
	file->accounts->accounts[new].credit_limit = 0;
	file->accounts->accounts[new].budget_amount = 0;
	file->accounts->accounts[new].offset_against = NULL_ACCOUNT;
	account_idnum_initialise(&(file->accounts->accounts[new].cheque_number));
	account_idnum_initialise(&(file->accounts->accounts[new].payin_number));

	*file->accounts->accounts[new].account_no = '\0';
	*file->accounts->accounts[new].sort_code = '\0';
	for (i=0; i<ACCOUNT_ADDR_LINES; i++)
		*file->accounts->accounts[new].address[i] = '\0';

	file->accounts->accounts[new].account_view = NULL;

	account_add_to_lists(file, new);
	transact_update_toolbar(file);

	return new;
}


/**
 * Add an account to the approprite account lists, if it isn't already
 * in them.
 *
 * \param *file			The file containing the account.
 * \param account		The account to process.
 */

static void account_add_to_lists(struct file_block *file, acct_t account)
{
	int	entry, line;

	if (file == NULL || file->accounts == NULL || !account_valid(file->accounts, account))
		return;

	entry = account_find_window_entry_from_type(file, file->accounts->accounts[account].type);

	if (entry == -1)
		return;

	line = account_add_list_display_line(file, entry);

	if (line == -1) {
		error_msgs_report_error("NoMemLinkAcct");
		return;
	}

	file->accounts->account_windows[entry].line_data[line].type = ACCOUNT_LINE_DATA;
	file->accounts->account_windows[entry].line_data[line].account = account;

	/* If the target window is open, change the extent as necessary. */

	account_set_window_extent(file->accounts->account_windows + entry);
}


/**
 * Delete an account from a file.
 *
 * \param *file			The file to act on.
 * \param account		The account to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

osbool account_delete(struct file_block *file, acct_t account)
{
	int	i, j;

	if (file == NULL || file->accounts == NULL || !account_valid(file->accounts, account))
		return FALSE;

	/* Delete the entry from the listing windows. */

	#ifdef DEBUG
	debug_printf("Trying to delete account %d", account);
	#endif

	if (account_used_in_file(file->accounts, account))
		return FALSE;

	for (i = 0; i < ACCOUNT_WINDOWS; i++) {
		for (j = file->accounts->account_windows[i].display_lines-1; j >= 0; j--) {
			if (file->accounts->account_windows[i].line_data[j].type == ACCOUNT_LINE_DATA &&
					file->accounts->account_windows[i].line_data[j].account == account) {
				#ifdef DEBUG
				debug_printf("Deleting entry type %x", file->accounts->account_windows[i].line_data[j].type);
				#endif

				if (!flexutils_delete_object((void **) &(file->accounts->account_windows[i].line_data), sizeof(struct account_redraw), j)) {
					error_msgs_report_error("BadDelete");
					continue;
				}

				file->accounts->account_windows[i].display_lines--;
				j--; /* Take into account that the array has just shortened. */
			}
		}

		account_set_window_extent(file->accounts->account_windows + i);
		if (file->accounts->account_windows[i].account_window != NULL) {
			windows_open(file->accounts->account_windows[i].account_window);
			account_force_window_redraw(file, i, 0, file->accounts->account_windows[i].display_lines, wimp_ICON_WINDOW);
		}
	}

	/* Close the account view window. */

	if (file->accounts->accounts[account].account_view != NULL)
		accview_delete_window(file, account);

	/* Remove the account from any report templates. */

	analysis_remove_account_from_templates(file, account);

	/* Blank out the account. */

	file->accounts->accounts[account].type = ACCOUNT_NULL;

	/* Update the transaction window toolbar. */

	transact_update_toolbar(file);
	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Purge unused accounts from a file.
 *
 * \param *file			The file to purge.
 * \param accounts		TRUE to remove accounts; FALSE to ignore.
 * \param headings		TRUE to remove headings; FALSE to ignore.
 */

void account_purge(struct file_block *file, osbool accounts, osbool headings)
{
	acct_t	account;

	if (file == NULL || file->accounts == NULL)
		return;

	for (account = 0; account < file->accounts->account_count; account++) {
		if (!account_used_in_file(file->accounts, account) &&
				((accounts && ((account_get_type(file, account) & ACCOUNT_FULL) != 0)) ||
				(headings && ((account_get_type(file, account) & (ACCOUNT_IN | ACCOUNT_OUT)) != 0)))) {
#ifdef DEBUG
			debug_printf("Deleting account %d, type %x", account, file->accounts->accounts[account].type);
#endif
			account_delete(file, account);
		}
	}
}


/**
 * Find the number of accounts in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of accounts in the file.
 */

int account_get_count(struct file_block *file)
{
	return (file != NULL && file->accounts != NULL) ? file->accounts->account_count : 0;
}


/**
 * Find the account window entry index which corresponds to a given account type.
 *
 * \TODO -- Depending upon how we set the array indicies up, this could probably
 *          be replaced by a simple lookup?
 *
 * \param *file			The file to use.
 * \param type			The account type to find the entry for.
 * \return			The corresponding index, or -1 if not found.
 */

static int account_find_window_entry_from_type(struct file_block *file, enum account_type type)
{
	int	i, entry = -1;

	if (file == NULL || file->accounts == NULL)
		return entry;

	/* Find the window block to use. */

	for (i = 0; i < ACCOUNT_WINDOWS; i++)
		if (file->accounts->account_windows[i].type == type)
			entry = i;

	return entry;
}


/**
 * Find an account by looking up an ident string against accounts of a
 * given type.
 *
 * \param *file			The file containing the account.
 * \param *ident		The ident to look up.
 * \param type			The type(s) of account to include.
 * \return			The account number, or NULL_ACCOUNT if not found.
 */

acct_t account_find_by_ident(struct file_block *file, char *ident, enum account_type type)
{
	int account = 0;

	if (file == NULL || file->accounts == NULL)
		return NULL_ACCOUNT;

	while ((account < file->accounts->account_count) &&
			((string_nocase_strcmp(ident, file->accounts->accounts[account].ident) != 0) || ((file->accounts->accounts[account].type & type) == 0)))
		account++;

	if (account == file->accounts->account_count)
		account = NULL_ACCOUNT;

	return account;
}


/**
 * Return a pointer to a string repesenting the ident of an account, or ""
 * if the account is not valid.
 *
 * \param *file			The file containing the account.
 * \param account		The account to return an ident for.
 * \return			Pointer to the ident string, or "".
 */

char *account_get_ident(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return "";

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return "";

	return file->accounts->accounts[account].ident;
}


/**
 * Return a pointer to a string repesenting the name of an account, or ""
 * if the account is not valid.
 *
 * \param *file			The file containing the account.
 * \param account		The account to return an name for.
 * \return			Pointer to the name string, or "".
 */

char *account_get_name(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return "";

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return "";

	return file->accounts->accounts[account].name;
}


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

char *account_build_name_pair(struct file_block *file, acct_t account, char *buffer, size_t size)
{
	*buffer = '\0';

	if (file != NULL && file->accounts != NULL && account_valid(file->accounts, account) && file->accounts->accounts[account].type != ACCOUNT_NULL)
		snprintf(buffer, size, "%s:%s", account_get_ident(file, account), account_get_name(file, account));

	return buffer;
}


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
		osbool *reconciled, wimp_w window, wimp_i ident, wimp_i name, wimp_i rec)
{
	osbool	new_rec = FALSE;

	if (file == NULL || file->accounts == NULL)
		return NULL_ACCOUNT;

	/* If the character is an alphanumeric or a delete, look up the ident as it stends. */

	if (isalnum(key) || iscntrl(key)) {
		/* Look up the account number based on the ident. */

		account = account_find_by_ident(file, icons_get_indirected_text_addr(window, ident), type);

		/* Copy the corresponding name into the name field. */

		icons_strncpy(window, name, account_get_name(file, account));
		wimp_set_icon_state(window, name, 0, 0);

		/* Do the auto-reconciliation. */

		if (account != NULL_ACCOUNT && !(file->accounts->accounts[account].type & ACCOUNT_FULL)) {
			/* If the account exists, and it is a heading (ie. it isn't a full account), reconcile it... */

			new_rec = TRUE;
			icons_msgs_lookup(window, rec, "RecChar");
			wimp_set_icon_state(window, rec, 0, 0);
		} else {
			/* ...otherwise unreconcile it. */

			new_rec = FALSE;
			*icons_get_indirected_text_addr(window, rec) = '\0';
			wimp_set_icon_state(window, rec, 0, 0);
		}
	}

	/* If the key pressed was a reconcile one, set or clear the bit as required. */

	if (key == '+' || key == '=') {
		new_rec = TRUE;
		icons_msgs_lookup(window, rec, "RecChar");
		wimp_set_icon_state(window, rec, 0, 0);
	}

	if (key == '-' || key == '_') {
		new_rec = FALSE;
		*icons_get_indirected_text_addr(window, rec) = '\0';
		wimp_set_icon_state(window, rec, 0, 0);
	}

	/* Return the new reconciled state if applicable. */

	if (reconciled != NULL)
		*reconciled = new_rec;

	return account;
}


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
		wimp_w window, wimp_i ident, wimp_i name, wimp_i rec)
{
	icons_strncpy(window, name, account_get_name(file, account));
	icons_strncpy(window, ident, account_get_ident(file, account));

	if (reconciled)
		icons_msgs_lookup(window, rec, "RecChar");
	else
		*icons_get_indirected_text_addr(window, rec) = '\0';
}


/**
 * Toggle the reconcile status shown in an icon.
 *
 * \param window	The window containing the icon.
 * \param icon		The icon to toggle.
 */

void account_toggle_reconcile_icon(wimp_w window, wimp_i icon)
{
	if (*icons_get_indirected_text_addr(window, icon) == '\0')
		icons_msgs_lookup(window, icon, "RecChar");
	else
		*icons_get_indirected_text_addr(window, icon) = '\0';

	wimp_set_icon_state(window, icon, 0, 0);
}


/**
 * Return the account view handle for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account to return the view of.
 * \return		The account's account view, or NULL.
 */

struct accview_window *account_get_accview(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return NULL;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return NULL;

	return file->accounts->accounts[account].account_view;
}


/**
 * Set the account view handle for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account to set the view of.
 * \param *view		The view handle, or NULL to clear.
 */

void account_set_accview(struct file_block *file, acct_t account, struct accview_window *view)
{
	if (file == NULL || file->accounts == NULL)
		return;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return;

	file->accounts->accounts[account].account_view = view;
}


/**
 * Return the type of an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the type.
 * \return		The account's type, or ACCOUNT_NULL.
 */

enum account_type account_get_type(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return ACCOUNT_NULL;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return ACCOUNT_NULL;

	return file->accounts->accounts[account].type;
}


/**
 * Return the opening balance for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the opening balance.
 * \return		The account's opening balance, or 0.
 */

amt_t account_get_opening_balance(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return 0;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return 0;

	return file->accounts->accounts[account].opening_balance;
}


/**
 * Adjust the opening balance for an account by adding or subtracting a
 * specified amount.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to alter the opening balance.
 * \param adjust	The amount to alter the opening balance by.
 */

void account_adjust_opening_balance(struct file_block *file, acct_t account, amt_t adjust)
{
	if (file == NULL || file->accounts == NULL)
		return;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return;

	file->accounts->accounts[account].opening_balance += adjust;
}


/**
 * Zero the standing order trial balances for all of the accounts in a file.
 *
 * \param *file		The file to zero.
 */

void account_zero_sorder_trial(struct file_block *file)
{
	if (file == NULL || file->accounts == NULL)
		return;

	acct_t	account;

	for (account = 0; account < file->accounts->account_count; account++)
		file->accounts->accounts[account].sorder_trial = 0;
}


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

void account_adjust_sorder_trial(struct file_block *file, acct_t account, amt_t adjust)
{
	if (file == NULL || file->accounts == NULL)
		return;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return;

	file->accounts->accounts[account].sorder_trial += adjust;
}


/**
 * Return the credit limit for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the opening balance.
 * \return		The account's opening balance, or 0.
 */

amt_t account_get_credit_limit(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return 0;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return 0;

	return file->accounts->accounts[account].credit_limit;
}


/**
 * Return the budget amount for an account.
 *
 * \param *file		The file containing the account.
 * \param account	The account for which to return the budget amount.
 * \return		The account's budget amount, or 0.
 */

amt_t account_get_budget_amount(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return 0;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return 0;

	return file->accounts->accounts[account].budget_amount;
}


/**
 * Check if an account is used in anywhere in a file.
 *
 * \param *instance		The accounts instance to check.
 * \param account		The account to check for.
 * \return			TRUE if the account is found; else FALSE.
 */

static osbool account_used_in_file(struct account_block *instance, acct_t account)
{
	if (instance == NULL || instance->file == NULL)
		return FALSE;

	if (account_check_account(instance, account))
		return TRUE;

	if (transact_check_account(instance->file, account))
		return TRUE;

	if (sorder_check_account(instance->file, account))
		return TRUE;

	if (preset_check_account(instance->file, account))
		return TRUE;

	return FALSE;
}


/**
 * Check the accounts in a file to see if the given account is referenced
 * in any of them.
 *
 * \param *instance		The accounts instance to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

static osbool account_check_account(struct account_block *instance, acct_t account)
{
	int	i;

	if (instance == NULL || instance->accounts == NULL)
		return FALSE;

	/* Check to see if any other accounts offset to this one. */

	for (i = 0; i < instance->account_count; i++) {
		if (instance->accounts[i].offset_against == account)
			return TRUE;
	}

	return FALSE;
}


/**
 * Count the number of accounts of a given type in a file.
 *
 * \param *file			The account to use.
 * \param type			The type of account to count.
 * \return			The number of accounts found.
 */

int account_count_type_in_file(struct file_block *file, enum account_type type)
{
	int	i, accounts = 0;

	if (file == NULL || file->accounts == NULL)
		return accounts;

	for (i = 0; i < file->accounts->account_count; i++)
		if ((file->accounts->accounts[i].type & type) != 0)
			accounts++;

	return accounts;
}


/**
 * Test if an account supports insertion of cheque numbers.
 *
 * \param *file			The file containing the account.
 * \param account		The account to check.
 * \return			TRUE if cheque numbers are available; else FALSE.
 */

osbool account_cheque_number_available(struct file_block *file, acct_t account)
{
	if (file == NULL || file->accounts == NULL)
		return FALSE;

	if (!account_valid(file->accounts, account) || file->accounts->accounts[account].type == ACCOUNT_NULL)
		return FALSE;

	return account_idnum_active(&(file->accounts->accounts[account].cheque_number));
}

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

char *account_get_next_cheque_number(struct file_block *file, acct_t from_account, acct_t to_account, int increment, char *buffer, size_t size)
{
	osbool		from_ok, to_ok;

	if (file == NULL || file->accounts == NULL)
		return buffer;

	/* Check which accounts have active ID numbers. */

	from_ok = ((from_account != NULL_ACCOUNT) && account_idnum_active(&(file->accounts->accounts[from_account].cheque_number)));
	to_ok = ((to_account != NULL_ACCOUNT) && account_idnum_active(&(file->accounts->accounts[from_account].payin_number)));

	/* If both have, we need to ask the user which to use. */

	if (from_ok && to_ok) {
		if (error_msgs_param_report_question("ChqOrPayIn", "ChqOrPayInB",
				file->accounts->accounts[to_account].name,
				file->accounts->accounts[from_account].name, NULL, NULL) == 3)
			to_ok = FALSE;
		else
			from_ok = FALSE;
	}

	/* Now process the reference. */

	if (from_ok)
		account_idnum_get_next(&(file->accounts->accounts[from_account].cheque_number), buffer, size, increment);
	else if (to_ok)
		account_idnum_get_next(&(file->accounts->accounts[from_account].payin_number), buffer, size, increment);
	else if (buffer != NULL && size > 0)
		buffer[0] = '\0';

	return buffer;
}


/**
 * Fully recalculate all of the accounts in a file.
 *
 * \param *file		The file to recalculate.
 */

void account_recalculate_all(struct file_block *file)
{
	acct_t			account, transaction_account;
	date_t			budget_start, budget_finish;
	amt_t			transaction_amount;
	enum transact_flags	transaction_flags;
	int			transaction;
	date_t			date, post_date, transaction_date;
	osbool			limit_postdated;

	if (file == NULL || file->accounts == NULL)
		return;

	hourglass_on();

	/* Initialise the accounts, based on the opening balances. */

	for (account = 0; account < file->accounts->account_count; account++) {
		file->accounts->accounts[account].statement_balance = file->accounts->accounts[account].opening_balance;
		file->accounts->accounts[account].current_balance = file->accounts->accounts[account].opening_balance;
		file->accounts->accounts[account].future_balance = file->accounts->accounts[account].opening_balance;
		file->accounts->accounts[account].budget_balance = 0; /* was file->accounts->accounts[account].opening_balance; */
	}

	date = date_today();
	post_date = date_add_period(date, DATE_PERIOD_DAYS, budget_get_sorder_trial(file));
	budget_get_dates(file, &budget_start, &budget_finish);
	limit_postdated = budget_get_limit_postdated(file);

	/* Add in the effects of each transaction */

	for (transaction = 0; transaction < transact_get_count(file); transaction++) {
		transaction_date = transact_get_date(file, transaction);
		transaction_amount = transact_get_amount(file, transaction);
		transaction_flags = transact_get_flags(file, transaction);

		if ((transaction_account = transact_get_from(file, transaction)) != NULL_ACCOUNT) {
			if (transaction_flags & TRANS_REC_FROM)
				file->accounts->accounts[transaction_account].statement_balance -= transaction_amount;

			if (transaction_date <= date)
				file->accounts->accounts[transaction_account].current_balance -= transaction_amount;

			if ((budget_start == NULL_DATE || transaction_date >= budget_start) &&
					(budget_finish == NULL_DATE || transaction_date <= budget_finish))
				file->accounts->accounts[transaction_account].budget_balance -= transaction_amount;

			if (!limit_postdated || transaction_date <= post_date)
				file->accounts->accounts[transaction_account].future_balance -= transaction_amount;
		}

		if ((transaction_account = transact_get_to(file, transaction)) != NULL_ACCOUNT) {
			if (transaction_flags & TRANS_REC_TO)
				file->accounts->accounts[transaction_account].statement_balance += transaction_amount;

			if (transaction_date <= date)
				file->accounts->accounts[transaction_account].current_balance += transaction_amount;

			if ((budget_start == NULL_DATE || transaction_date >= budget_start) &&
					(budget_finish == NULL_DATE || transaction_date <= budget_finish))
				file->accounts->accounts[transaction_account].budget_balance += transaction_amount;

			if (!limit_postdated || transaction_date <= post_date)
				file->accounts->accounts[transaction_account].future_balance += transaction_amount;
		}
	}

	/* Calculate the outstanding data for each account. */

	for (account = 0; account < file->accounts->account_count; account++) {
		file->accounts->accounts[account].available_balance = file->accounts->accounts[account].future_balance + file->accounts->accounts[account].credit_limit;
		file->accounts->accounts[account].trial_balance = file->accounts->accounts[account].available_balance + file->accounts->accounts[account].sorder_trial;
	}

	file->accounts->last_full_recalc = date;

	/* Calculate the accounts windows data and force a redraw of the windows that are open. */

	account_recalculate_windows(file);
	account_redraw_all(file);

	hourglass_off();
}


/**
 * Remove a transaction from all the calculated accounts, so that limited
 * changes can be made to its details. Once updated, it can be resored
 * using account_restore_transaction(). The changes can not affect the sort
 * order of the transactions in the file, or the restoration will be invalid.
 *
 * \param *file		The file containing the transaction.
 * \param transasction	The transaction to remove.
 */

void account_remove_transaction(struct file_block *file, tran_t transaction)
{
	date_t			budget_start, budget_finish, transaction_date;
	acct_t			transaction_account;
	enum transact_flags	transaction_flags;
	amt_t			transaction_amount;

	if (file == NULL || file->accounts == NULL || !transact_test_index_valid(file, transaction))
		return;

	budget_get_dates(file, &budget_start, &budget_finish);

	transaction_date = transact_get_date(file, transaction);
	transaction_flags = transact_get_flags(file, transaction);
	transaction_amount = transact_get_amount(file, transaction);

	/* Remove the current transaction from the fully-caluculated records. */

	if ((transaction_account = transact_get_from(file, transaction)) != NULL_ACCOUNT) {
		if (transaction_flags & TRANS_REC_FROM)
			file->accounts->accounts[transaction_account].statement_balance += transaction_amount;

		if (transaction_date <= file->accounts->last_full_recalc)
			file->accounts->accounts[transaction_account].current_balance += transaction_amount;

		if ((budget_start == NULL_DATE || transaction_date >= budget_start) &&
				(budget_finish == NULL_DATE || transaction_date <= budget_finish))
			file->accounts->accounts[transaction_account].budget_balance += transaction_amount;

		file->accounts->accounts[transaction_account].future_balance += transaction_amount;
		file->accounts->accounts[transaction_account].trial_balance += transaction_amount;
		file->accounts->accounts[transaction_account].available_balance += transaction_amount;
	}

	if ((transaction_account = transact_get_to(file, transaction)) != NULL_ACCOUNT) {
		if (transaction_flags & TRANS_REC_TO)
			file->accounts->accounts[transaction_account].statement_balance -= transaction_amount;

		if (transaction_date <= file->accounts->last_full_recalc)
			file->accounts->accounts[transaction_account].current_balance -= transaction_amount;

		if ((budget_start == NULL_DATE || transaction_date >= budget_start) &&
				(budget_finish == NULL_DATE || transaction_date <= budget_finish))
			file->accounts->accounts[transaction_account].budget_balance -= transaction_amount;

		file->accounts->accounts[transaction_account].future_balance -= transaction_amount;
		file->accounts->accounts[transaction_account].trial_balance -= transaction_amount;
		file->accounts->accounts[transaction_account].available_balance -= transaction_amount;
	}
}


/**
 * Restore a transaction previously removed by account_remove_transaction()
 * after changes have been made, recalculate the affected accounts and
 * refresh any affected displays. The changes can not affect the sort
 * order of the transactions in the file, or the restoration will be invalid.
 *
 * \param *file		The file containing the transaction.
 * \param transasction	The transaction to restore.
 */

void account_restore_transaction(struct file_block *file, int transaction)
{
	int			i;
	date_t			budget_start, budget_finish, transaction_date;
	acct_t			transaction_account;
	enum transact_flags	transaction_flags;
	amt_t			transaction_amount;

	if (file == NULL || file->accounts == NULL || !transact_test_index_valid(file, transaction))
		return;

	budget_get_dates(file, &budget_start, &budget_finish);

	transaction_date = transact_get_date(file, transaction);
	transaction_flags = transact_get_flags(file, transaction);
	transaction_amount = transact_get_amount(file, transaction);

	/* Restore the current transaction back into the fully-caluculated records. */

	if ((transaction_account = transact_get_from(file, transaction)) != NULL_ACCOUNT) {
		if (transaction_flags & TRANS_REC_FROM)
			file->accounts->accounts[transaction_account].statement_balance -= transaction_amount;

		if (transaction_date <= file->accounts->last_full_recalc)
			file->accounts->accounts[transaction_account].current_balance -= transaction_amount;

		if ((budget_start == NULL_DATE || transaction_date >= budget_start) &&
				(budget_finish == NULL_DATE || transaction_date <= budget_finish))
			file->accounts->accounts[transaction_account].budget_balance -= transaction_amount;

		file->accounts->accounts[transaction_account].future_balance -= transaction_amount;
		file->accounts->accounts[transaction_account].trial_balance -= transaction_amount;
		file->accounts->accounts[transaction_account].available_balance -= transaction_amount;
	}

	if ((transaction_account = transact_get_to(file, transaction)) != NULL_ACCOUNT) {
		if (transaction_flags & TRANS_REC_TO)
			file->accounts->accounts[transaction_account].statement_balance += transaction_amount;

		if (transaction_date <= file->accounts->last_full_recalc)
			file->accounts->accounts[transaction_account].current_balance += transaction_amount;

		if ((budget_start == NULL_DATE || transaction_date >= budget_start) &&
				(budget_finish == NULL_DATE || transaction_date <= budget_finish))
			file->accounts->accounts[transaction_account].budget_balance += transaction_amount;

		file->accounts->accounts[transaction_account].future_balance += transaction_amount;
		file->accounts->accounts[transaction_account].trial_balance += transaction_amount;
		file->accounts->accounts[transaction_account].available_balance += transaction_amount;
	}

	/* Calculate the accounts windows data and force a redraw of the windows that are open. */

	account_recalculate_windows(file);

	for (i = 0; i < ACCOUNT_WINDOWS; i++)
		account_force_window_redraw(file, i, 0, file->accounts->account_windows[i].display_lines, wimp_ICON_WINDOW);
}



/**
 * Save the account and account list details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void account_write_file(struct file_block *file, FILE *out)
{
	int		i, j, width;
	unsigned	next_id;
	char		buffer[FILING_MAX_FILE_LINE_LEN];

	if (file == NULL || file->accounts == NULL)
		return;

	/* Output the account data. */

	fprintf(out, "\n[Accounts]\n");

	fprintf(out, "Entries: %x\n", file->accounts->account_count);

	/* \TODO -- This probably shouldn't be here, but in the accview module?
	 *
	 * This would require AccView to have its own file section.
	 */

	accview_write_file(file, out);

	for (i = 0; i < file->accounts->account_count; i++) {

		/* Deleted accounts are skipped, as these can be filled in at load. */

		if (file->accounts->accounts[i].type != ACCOUNT_NULL) {
			account_idnum_get(&(file->accounts->accounts[i].cheque_number), &width, &next_id);

			fprintf(out, "@: %x,%s,%x,%x,%x,%x,%x,%x\n",
					i, file->accounts->accounts[i].ident, file->accounts->accounts[i].type,
					file->accounts->accounts[i].opening_balance, file->accounts->accounts[i].credit_limit,
					file->accounts->accounts[i].budget_amount, next_id, width);

			account_idnum_get(&(file->accounts->accounts[i].payin_number), &width, &next_id);

			if (*(file->accounts->accounts[i].name) != '\0')
				config_write_token_pair(out, "Name", file->accounts->accounts[i].name);
			if (*(file->accounts->accounts[i].account_no) != '\0')
				config_write_token_pair(out, "AccNo", file->accounts->accounts[i].account_no);
			if (*(file->accounts->accounts[i].sort_code) != '\0')
				config_write_token_pair(out, "SortCode", file->accounts->accounts[i].sort_code);
			if (*(file->accounts->accounts[i].address[0]) != '\0')
				config_write_token_pair(out, "Addr0", file->accounts->accounts[i].address[0]);
			if (*(file->accounts->accounts[i].address[1]) != '\0')
				config_write_token_pair(out, "Addr1", file->accounts->accounts[i].address[1]);
			if (*(file->accounts->accounts[i].address[2]) != '\0')
				config_write_token_pair(out, "Addr2", file->accounts->accounts[i].address[2]);
			if (*(file->accounts->accounts[i].address[3]) != '\0')
				config_write_token_pair(out, "Addr3", file->accounts->accounts[i].address[3]);
			if (width != 0 || next_id != 0)
				fprintf(out, "PayIn: %x,%x\n", width, next_id);
			if (file->accounts->accounts[i].offset_against != NULL_ACCOUNT)
				fprintf(out, "Offset: %x\n", file->accounts->accounts[i].offset_against);
		}
	}

	/* Output the Accounts Windows data. */

	for (j = 0; j < ACCOUNT_WINDOWS; j++) {
		fprintf(out, "\n[AccountList:%x]\n", file->accounts->account_windows[j].type);

		fprintf(out, "Entries: %x\n", file->accounts->account_windows[j].display_lines);

		column_write_as_text(file->accounts->account_windows[j].columns, buffer, FILING_MAX_FILE_LINE_LEN);
		fprintf(out, "WinColumns: %s\n", buffer);

		for (i = 0; i < file->accounts->account_windows[j].display_lines; i++) {
			fprintf(out, "@: %x,%x\n", file->accounts->account_windows[j].line_data[i].type,
					file->accounts->account_windows[j].line_data[i].account);

			if ((file->accounts->account_windows[j].line_data[i].type == ACCOUNT_LINE_HEADER ||
					file->accounts->account_windows[j].line_data[i].type == ACCOUNT_LINE_FOOTER) &&
					*(file->accounts->account_windows[j].line_data[i].heading) != '\0')
				config_write_token_pair (out, "Heading", file->accounts->account_windows[j].line_data[i].heading);
		}
	}
}


/**
 * Read account details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool account_read_acct_file(struct file_block *file, struct filing_block *in)
{
	size_t			block_size;
	acct_t			account = NULL_ACCOUNT;
	int			j;

	if (file == NULL || file->accounts == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Account Data.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->accounts->accounts), sizeof(struct account), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->accounts->account_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->accounts->account_count;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			accview_read_file_wincolumns(file, filing_get_format(in), filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "SortOrder")) {
			accview_read_file_sortorder(file, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			/* A new account.  Take the account number, and see if it falls within the current defined set of
			 * accounts (not the same thing as the pre-expanded account block).  If not, expand the acconut_count
			 * to the new account number and blank all the new entries.
			 */

			account = account_get_account_field(in);

			if (account >= file->accounts->account_count) {
				j = file->accounts->account_count;
				file->accounts->account_count = account + 1;

				#ifdef DEBUG
				debug_printf("Account range expanded to %d", account);
				#endif

				/* The block isn't big enough, so expand this to the required size. */

				if (file->accounts->account_count > block_size) {
					block_size = file->accounts->account_count;
					#ifdef DEBUG
					debug_printf("Section block expand to %d", block_size);
					#endif
					if (!flexutils_load_resize(block_size)) {
						filing_set_status(in, FILING_STATUS_MEMORY);
						return FALSE;
					}
				}

				/* Blank all the intervening entries. */

				while (j < file->accounts->account_count) {
					#ifdef DEBUG
					debug_printf("Blanking account entry %d", j);
					#endif

					*(file->accounts->accounts[j].name) = '\0';
					*(file->accounts->accounts[j].ident) = '\0';
					file->accounts->accounts[j].type = ACCOUNT_NULL;
					file->accounts->accounts[j].opening_balance = 0;
					file->accounts->accounts[j].credit_limit = 0;
					file->accounts->accounts[j].budget_amount = 0;
					file->accounts->accounts[j].offset_against = NULL_ACCOUNT;
					account_idnum_initialise(&(file->accounts->accounts[j].cheque_number));
					account_idnum_initialise(&(file->accounts->accounts[j].payin_number));

					file->accounts->accounts[j].account_view = NULL;

					*(file->accounts->accounts[j].name) = '\0';
					*(file->accounts->accounts[j].account_no) = '\0';
					*(file->accounts->accounts[j].sort_code) = '\0';
					*(file->accounts->accounts[j].address[0]) = '\0';
					*(file->accounts->accounts[j].address[1]) = '\0';
					*(file->accounts->accounts[j].address[2]) = '\0';
					*(file->accounts->accounts[j].address[3]) = '\0';

					j++;
				}
			}

			#ifdef DEBUG
			debug_printf("Loading account entry %d", account);
			#endif

			filing_get_text_field(in, file->accounts->accounts[account].ident, ACCOUNT_IDENT_LEN);
			file->accounts->accounts[account].type = account_get_account_type_field(in);
			file->accounts->accounts[account].opening_balance = currency_get_currency_field(in);
			file->accounts->accounts[account].credit_limit = currency_get_currency_field(in);
			file->accounts->accounts[account].budget_amount = currency_get_currency_field(in);
			account_idnum_set(&(file->accounts->accounts[account].cheque_number),
					filing_get_int_field(in), filing_get_unsigned_field(in));

			*(file->accounts->accounts[account].name) = '\0';
			*(file->accounts->accounts[account].account_no) = '\0';
			*(file->accounts->accounts[account].sort_code) = '\0';
			*(file->accounts->accounts[account].address[0]) = '\0';
			*(file->accounts->accounts[account].address[1]) = '\0';
			*(file->accounts->accounts[account].address[2]) = '\0';
			*(file->accounts->accounts[account].address[3]) = '\0';
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "Name")) {
			filing_get_text_value(in, file->accounts->accounts[account].name, ACCOUNT_NAME_LEN);
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "AccNo")) {
			filing_get_text_value(in, file->accounts->accounts[account].account_no, ACCOUNT_NO_LEN);
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "SortCode")) {
			filing_get_text_value(in, file->accounts->accounts[account].sort_code, ACCOUNT_SRTCD_LEN);
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "Addr0")) {
			filing_get_text_value(in, file->accounts->accounts[account].address[0], ACCOUNT_ADDR_LEN);
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "Addr1")) {
			filing_get_text_value(in, file->accounts->accounts[account].address[1], ACCOUNT_ADDR_LEN);
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "Addr2")) {
			filing_get_text_value(in, file->accounts->accounts[account].address[2], ACCOUNT_ADDR_LEN);
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "Addr3")) {
			filing_get_text_value(in, file->accounts->accounts[account].address[3], ACCOUNT_ADDR_LEN);
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "PayIn")) {
			account_idnum_set(&(file->accounts->accounts[account].payin_number),
					filing_get_int_field(in), filing_get_unsigned_field(in));
		} else if (account != NULL_ACCOUNT && filing_test_token(in, "Offset")) {
			file->accounts->accounts[account].offset_against = account_get_account_field(in);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(file->accounts->account_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
}


