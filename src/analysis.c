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
 * \file: analysis.c
 *
 * Analysis interface implementation.
 */

/* ANSI C header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "analysis.h"

#include "account.h"
#include "account_menu.h"
#include "budget.h"
#include "caret.h"
#include "currency.h"
#include "date.h"
#include "file.h"
#include "flexutils.h"
#include "report.h"
#include "transact.h"

#define ANALYSIS_MENU_TITLE_LEN 32

/* Transaction Report window. */

#define ANALYSIS_TRANS_OK 1
#define ANALYSIS_TRANS_CANCEL 0
#define ANALYSIS_TRANS_DELETE 39
#define ANALYSIS_TRANS_RENAME 40

#define ANALYSIS_TRANS_DATEFROM 5
#define ANALYSIS_TRANS_DATETO 7
#define ANALYSIS_TRANS_DATEFROMTXT 4
#define ANALYSIS_TRANS_DATETOTXT 6
#define ANALYSIS_TRANS_BUDGET 8
#define ANALYSIS_TRANS_GROUP 11
#define ANALYSIS_TRANS_PERIOD 13
#define ANALYSIS_TRANS_PTEXT 12
#define ANALYSIS_TRANS_PDAYS 14
#define ANALYSIS_TRANS_PMONTHS 15
#define ANALYSIS_TRANS_PYEARS 16
#define ANALYSIS_TRANS_LOCK 17

#define ANALYSIS_TRANS_FROMSPEC 21
#define ANALYSIS_TRANS_FROMSPECPOPUP 22
#define ANALYSIS_TRANS_TOSPEC 24
#define ANALYSIS_TRANS_TOSPECPOPUP 25
#define ANALYSIS_TRANS_REFSPEC 27
#define ANALYSIS_TRANS_AMTLOSPEC 29
#define ANALYSIS_TRANS_AMTHISPEC 31
#define ANALYSIS_TRANS_DESCSPEC 33

#define ANALYSIS_TRANS_OPTRANS 36
#define ANALYSIS_TRANS_OPSUMMARY 37
#define ANALYSIS_TRANS_OPACCSUMMARY 38

/* Unreconciled Report window. */

#define ANALYSIS_UNREC_OK 0
#define ANALYSIS_UNREC_CANCEL 1
#define ANALYSIS_UNREC_DELETE 28
#define ANALYSIS_UNREC_RENAME 29

#define ANALYSIS_UNREC_DATEFROMTXT 4
#define ANALYSIS_UNREC_DATEFROM 5
#define ANALYSIS_UNREC_DATETOTXT 6
#define ANALYSIS_UNREC_DATETO 7
#define ANALYSIS_UNREC_BUDGET 8

#define ANALYSIS_UNREC_GROUP 11
#define ANALYSIS_UNREC_GROUPACC 12
#define ANALYSIS_UNREC_GROUPDATE 13
#define ANALYSIS_UNREC_PERIOD 15
#define ANALYSIS_UNREC_PTEXT 14
#define ANALYSIS_UNREC_PDAYS 16
#define ANALYSIS_UNREC_PMONTHS 17
#define ANALYSIS_UNREC_PYEARS 18
#define ANALYSIS_UNREC_LOCK 19

#define ANALYSIS_UNREC_FROMSPEC 23
#define ANALYSIS_UNREC_FROMSPECPOPUP 24
#define ANALYSIS_UNREC_TOSPEC 26
#define ANALYSIS_UNREC_TOSPECPOPUP 27

/* Cashflow Report window. */

#define ANALYSIS_CASHFLOW_OK 0
#define ANALYSIS_CASHFLOW_CANCEL 1
#define ANALYSIS_CASHFLOW_DELETE 31
#define ANALYSIS_CASHFLOW_RENAME 32

#define ANALYSIS_CASHFLOW_DATEFROMTXT 4
#define ANALYSIS_CASHFLOW_DATEFROM 5
#define ANALYSIS_CASHFLOW_DATETOTXT 6
#define ANALYSIS_CASHFLOW_DATETO 7
#define ANALYSIS_CASHFLOW_BUDGET 8

#define ANALYSIS_CASHFLOW_GROUP 11
#define ANALYSIS_CASHFLOW_PERIOD 13
#define ANALYSIS_CASHFLOW_PTEXT 12
#define ANALYSIS_CASHFLOW_PDAYS 14
#define ANALYSIS_CASHFLOW_PMONTHS 15
#define ANALYSIS_CASHFLOW_PYEARS 16
#define ANALYSIS_CASHFLOW_LOCK 17
#define ANALYSIS_CASHFLOW_EMPTY 18

#define ANALYSIS_CASHFLOW_ACCOUNTS 22
#define ANALYSIS_CASHFLOW_ACCOUNTSPOPUP 23
#define ANALYSIS_CASHFLOW_INCOMING 25
#define ANALYSIS_CASHFLOW_INCOMINGPOPUP 26
#define ANALYSIS_CASHFLOW_OUTGOING 28
#define ANALYSIS_CASHFLOW_OUTGOINGPOPUP 29
#define ANALYSIS_CASHFLOW_TABULAR 30

/* Balance Report window. */

#define ANALYSIS_BALANCE_OK 0
#define ANALYSIS_BALANCE_CANCEL 1
#define ANALYSIS_BALANCE_DELETE 30
#define ANALYSIS_BALANCE_RENAME 31

#define ANALYSIS_BALANCE_DATEFROMTXT 4
#define ANALYSIS_BALANCE_DATEFROM 5
#define ANALYSIS_BALANCE_DATETOTXT 6
#define ANALYSIS_BALANCE_DATETO 7
#define ANALYSIS_BALANCE_BUDGET 8

#define ANALYSIS_BALANCE_GROUP 11
#define ANALYSIS_BALANCE_PERIOD 13
#define ANALYSIS_BALANCE_PTEXT 12
#define ANALYSIS_BALANCE_PDAYS 14
#define ANALYSIS_BALANCE_PMONTHS 15
#define ANALYSIS_BALANCE_PYEARS 16
#define ANALYSIS_BALANCE_LOCK 17

#define ANALYSIS_BALANCE_ACCOUNTS 21
#define ANALYSIS_BALANCE_ACCOUNTSPOPUP 22
#define ANALYSIS_BALANCE_INCOMING 24
#define ANALYSIS_BALANCE_INCOMINGPOPUP 25
#define ANALYSIS_BALANCE_OUTGOING 27
#define ANALYSIS_BALANCE_OUTGOINGPOPUP 28
#define ANALYSIS_BALANCE_TABULAR 29

#define ANALYSIS_SAVE_OK 4
#define ANALYSIS_SAVE_CANCEL 3
#define ANALYSIS_SAVE_NAME 1
#define ANALYSIS_SAVE_NAMEPOPUP 2

#define ANALYSIS_LOOKUP_IDENT 0
#define ANALYSIS_LOOKUP_REC 1
#define ANALYSIS_LOOKUP_NAME 2
#define ANALYSIS_LOOKUP_CANCEL 3
#define ANALYSIS_LOOKUP_OK 4

/* Report dialogues. */
/* SPEC_LEN is the length of the text fields; LIST_LEN is the number of accounts that can be stored. */

#define ANALYSIS_ACC_SPEC_LEN 128
#define ANALYSIS_ACC_LIST_LEN 64

/* Saved Report templates */

#define ANALYSIS_SAVED_NAME_LEN 32


/**
 * Transaction Report dialogue.
 */

struct trans_rep {
	date_t				date_from;
	date_t				date_to;
  int          budget;

  int          group;
	int          period;
	enum date_period		period_unit;
  int          lock;

	int				from_count;
	int				to_count;
	acct_t				from[ANALYSIS_ACC_LIST_LEN];
	acct_t				to[ANALYSIS_ACC_LIST_LEN];
	char				ref[TRANSACT_REF_FIELD_LEN];
	char				desc[TRANSACT_DESCRIPT_FIELD_LEN];
	amt_t				amount_min;
	amt_t				amount_max;

  int          output_trans;
  int          output_summary;
  int          output_accsummary;
};

/* Unreconciled Report dialogue. */

struct unrec_rep {
	date_t				date_from;
	date_t				date_to;
  int          budget;

  int          group;
	int				period;
	enum date_period		period_unit;
  int          lock;

	int				from_count;
	int				to_count;
	acct_t				from[ANALYSIS_ACC_LIST_LEN];
	acct_t				to[ANALYSIS_ACC_LIST_LEN];
};

/* Cashflow Report dialogue. */

struct cashflow_rep {
	date_t				date_from;
	date_t				date_to;
  int          budget;

  int          group;
	int				period;
	enum date_period		period_unit;
  int          lock;
  int          empty;

	int				accounts_count;
	int				incoming_count;
	int				outgoing_count;
	acct_t				accounts[ANALYSIS_ACC_LIST_LEN];
	acct_t				incoming[ANALYSIS_ACC_LIST_LEN];
	acct_t				outgoing[ANALYSIS_ACC_LIST_LEN];

  int          tabular;
};

/* Balance Report dialogue. */

struct balance_rep {
	date_t				date_from;
	date_t				date_to;
  int          budget;

  int          group;
	int				period;
	enum date_period		period_unit;
  int          lock;

	int				accounts_count;
	int				incoming_count;
	int				outgoing_count;
	acct_t				accounts[ANALYSIS_ACC_LIST_LEN];
	acct_t				incoming[ANALYSIS_ACC_LIST_LEN];
	acct_t				outgoing[ANALYSIS_ACC_LIST_LEN];

  int          tabular;
};

enum analysis_save_mode {
	ANALYSIS_SAVE_MODE_NONE = 0,						/**< The Save/Rename dialogue isn't used.					*/
	ANALYSIS_SAVE_MODE_SAVE,						/**< The Save/Rename dialogue is in Save mode.					*/
	ANALYSIS_SAVE_MODE_RENAME						/**< The Save/Rename dialogue is in Rename mode.				*/
};

enum analysis_flags {
	ANALYSIS_REPORT_NONE = 0,
	ANALYSIS_REPORT_FROM = 0x0001,
	ANALYSIS_REPORT_TO = 0x0002,
	ANALYSIS_REPORT_INCLUDE = 0x0004
};

/**
 * Saved Report Types.
 */

enum analysis_report_type {
	REPORT_TYPE_NONE = 0,							/**< No report.									*/
	REPORT_TYPE_TRANS = 1,							/**< Transaction report.							*/
	REPORT_TYPE_UNREC = 2,							/**< Unreconciled transaction report.						*/
	REPORT_TYPE_CASHFLOW = 3,						/**< Cashflow report.								*/
	REPORT_TYPE_BALANCE = 4							/**< Balance report.								*/
};

/**
 * Saved Report Data Union.
 */

union analysis_report_block {
	struct trans_rep		transaction;
	struct unrec_rep		unreconciled;
	struct cashflow_rep		cashflow;
	struct balance_rep		balance;
};

/**
 * Saved Report.
 */

struct analysis_report {
	struct file_block		*file;					/**< The file to which the template belongs.					*/
	char				name[ANALYSIS_SAVED_NAME_LEN];		/**< The name of the saved report template.					*/
	enum analysis_report_type	type;					/**< The type of the template.							*/

	union analysis_report_block	data;					/**< The template-type-specific data.						*/
};

/**
 * Template Menu Entries
 */

struct analysis_report_link
{
	char				name[ANALYSIS_SAVED_NAME_LEN + 3];	/**< The name as it appears in the menu (+3 for ellipsis...)			*/
	int				template;				/**< Index link to the associated report template in the saved report array.	*/
};

/**
 * Analysis Scratch Data
 *
 * Data associated with an individual account during report generation.
 */

struct analysis_data
{
	int				report_total;				/**< Running total for the account.						*/
	int				report_balance;				/**< Balance for the account.							*/
	enum analysis_flags		report_flags;				/**< Flags associated with the account.						*/
};

/* Report windows. */

static wimp_w			analysis_transaction_window = NULL;		/**< The handle of the Transaction Report window.				*/
static struct file_block	*analysis_transaction_file = NULL;		/**< The file currently owning the transaction dialogue.			*/
static osbool			analysis_transaction_restore = FALSE;		/**< The restore setting for the current Transaction dialogue.			*/
static struct trans_rep		analysis_transaction_settings;			/**< Saved initial settings for the Transaction dialogue.			*/
static int			analysis_transaction_template = NULL_TEMPLATE;	/**< The template which applies to the Transaction dialogue.			*/

static wimp_w			analysis_unreconciled_window = NULL;		/**< The handle of the Unreconciled Report window.				*/
static struct file_block	*analysis_unreconciled_file = NULL;		/**< The file currently owning the unreconciled dialogue.			*/
static osbool			analysis_unreconciled_restore = FALSE;		/**< The restore setting for the current Unreconciled dialogue.			*/
static struct unrec_rep		analysis_unreconciled_settings;			/**< Saved initial settings for the Unreconciled dialogue.			*/
static int			analysis_unreconciled_template = NULL_TEMPLATE;	/**< The template which applies to the Unreconciled dialogue.			*/

static wimp_w			analysis_cashflow_window = NULL;		/**< The handle of the Cashflow Report window.					*/
static struct file_block	*analysis_cashflow_file = NULL;			/**< The file currently owning the cashflow dialogue.				*/
static osbool			analysis_cashflow_restore = FALSE;		/**< The restore setting for the current Cashflow dialogue.			*/
static struct cashflow_rep	analysis_cashflow_settings;			/**< Saved initial settings for the Cashflow dialogue.				*/
static int			analysis_cashflow_template = NULL_TEMPLATE;	/**< The template which applies to the Cashflow dialogue.			*/

static wimp_w			analysis_balance_window = NULL;			/**< The handle of the Balance Report window.					*/
static struct file_block	*analysis_balance_file = NULL;			/**< The file currently owning the balance dialogue.				*/
static osbool			analysis_balance_restore = FALSE;		/**< The restore setting for the current Balance dialogue.			*/
static struct balance_rep	analysis_balance_settings;			/**< Saved initial settings for the Balance dialogue.				*/
static int			analysis_balance_template = NULL_TEMPLATE;	/**< The template which applies to the Balance dialogue.			*/

static struct analysis_report	analysis_report_template;			/**< New report settings for passing to the Report module.			*/

static acct_t			analysis_wildcard_account_list = NULL_ACCOUNT;	/**< Pass a pointer to this to set all accounts.				*/

/* Account Lookup Window. */

static wimp_w			analysis_lookup_window = NULL;			/**< The handle of the Account Lookup window.					*/
static struct file_block	*analysis_lookup_file = NULL;			/**< The file currently owning the Account Lookup window.			*/
static enum account_type	analysis_lookup_type = ACCOUNT_NULL;		/**< The type(s) of account to be looked up in the window.			*/
static wimp_w			analysis_lookup_parent;				/**< The window currently owning the Account Lookup window.			*/
static wimp_i			analysis_lookup_icon;				/**< The icon to which the lookup results should be inserted.			*/

/* Date period management. */

static date_t			analysis_period_start;				/**< The start of the current reporting period.					*/
static date_t			analysis_period_end;				/**< The end of the current reporting period.					*/
static int			analysis_period_length;
static enum date_period		analysis_period_unit;
static osbool			analysis_period_lock;
static osbool			analysis_period_first = TRUE;

/* Save/Rename Reports. */

static wimp_w			analysis_save_window = NULL;			/**< The handle of the Save/Rename window.					*/
static struct file_block	*analysis_save_file = NULL;			/**< The file currently owning the Save/Rename window.				*/
static struct analysis_report	*analysis_save_report = NULL;			/**< The report currently owning the Save/Rename window.			*/
static int			analysis_save_template = NULL_TEMPLATE;		/**< The template currently owning the Save/Rename window.			*/
static enum analysis_save_mode	analysis_save_mode = ANALYSIS_SAVE_MODE_NONE;	/**< The current mode of the Save/Rename window.				*/

/* Template List Menu. */

static wimp_menu			*analysis_template_menu = NULL;		/**< The saved template menu block.						*/
static struct analysis_report_link	*analysis_template_menu_link = NULL;	/**< Links for the template menu.						*/
static char				*analysis_template_menu_title = NULL;	/**< The menu title buffer.							*/



static void		analysis_transaction_click_handler(wimp_pointer *pointer);
static osbool		analysis_transaction_keypress_handler(wimp_key *key);
static void		analysis_refresh_transaction_window(void);
static void		analysis_fill_transaction_window(struct file_block *file, osbool restore);
static osbool		analysis_process_transaction_window(void);
static osbool		analysis_delete_transaction_window(void);
static void		analysis_generate_transaction_report(struct file_block *file);

static void		analysis_unreconciled_click_handler(wimp_pointer *pointer);
static osbool		analysis_unreconciled_keypress_handler(wimp_key *key);
static void		analysis_refresh_unreconciled_window(void);
static void		analysis_fill_unreconciled_window(struct file_block *file, osbool restore);
static osbool		analysis_process_unreconciled_window(void);
static osbool		analysis_delete_unreconciled_window(void);
static void		analysis_generate_unreconciled_report(struct file_block *file);

static void		analysis_cashflow_click_handler(wimp_pointer *pointer);
static osbool		analysis_cashflow_keypress_handler(wimp_key *key);
static void		analysis_refresh_cashflow_window(void);
static void		analysis_fill_cashflow_window(struct file_block *file, osbool restore);
static osbool		analysis_process_cashflow_window(void);
static osbool		analysis_delete_cashflow_window(void);
static void		analysis_generate_cashflow_report(struct file_block *file);

static void		analysis_balance_click_handler(wimp_pointer *pointer);
static osbool		analysis_balance_keypress_handler(wimp_key *key);
static void		analysis_refresh_balance_window(void);
static void		analysis_fill_balance_window(struct file_block *file, osbool restore);
static osbool		analysis_process_balance_window(void);
static osbool		analysis_delete_balance_window(void);
static void		analysis_generate_balance_report(struct file_block *file);

static void		analysis_open_account_lookup(struct file_block *file, wimp_w window, wimp_i icon, int account, enum account_type type);
static void		analysis_lookup_click_handler(wimp_pointer *pointer);
static osbool		analysis_lookup_keypress_handler(wimp_key *key);
static void		analysis_lookup_menu_closed(void);
static osbool		analysis_process_lookup_window(void);

static void		analysis_find_date_range(struct file_block *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget);
static void		analysis_initialise_date_period(date_t start, date_t end, int period, enum date_period unit, osbool lock);
static osbool		analysis_get_next_date_period(date_t *next_start, date_t *next_end, char *date_text, size_t date_len);

static void		analysis_remove_account_from_report_template(struct analysis_report *template, void *data);
static void		analysis_remove_account_from_template(struct analysis_report *template, acct_t account);
static int		analysis_remove_account_from_list(acct_t account, acct_t *array, int *count);
static void		analysis_clear_account_report_flags(struct file_block *file, struct analysis_data *data);
static void		analysis_set_account_report_flags_from_list(struct file_block *file, struct analysis_data *data, unsigned type, unsigned flags, acct_t *array, int count);
static int		analysis_account_idents_to_list(struct file_block *file, unsigned type, char *list, acct_t *array);
static void		analysis_account_list_to_idents(struct file_block *file, char *list, acct_t *array, int len);

static void		analysis_open_rename_window(struct file_block *file, int template, wimp_pointer *ptr);
static void		analysis_save_click_handler(wimp_pointer *pointer);
static osbool		analysis_save_keypress_handler(wimp_key *key);
static void		analysis_save_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void		analysis_save_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void		analysis_save_menu_close_handler(wimp_w w, wimp_menu *menu);
static void		analysis_refresh_save_window(void);
static void		analysis_fill_save_window(struct analysis_report *template);
static void		analysis_fill_rename_window(struct file_block *file, int template);
static osbool		analysis_process_save_window(void);

static int		analysis_template_menu_compare_entries(const void *va, const void *vb);

static void		analysis_force_close_report_rename_window(wimp_w window);

static int		analysis_get_template_from_name(struct file_block *file, char *name);
static void		analysis_store_template(struct file_block *file, struct analysis_report *report, int template);
static void		analysis_delete_template(struct file_block *file, int template);
static struct analysis_report	*analysis_create_template(struct analysis_report *base);

static void		analysis_copy_transaction_template(struct trans_rep *to, struct trans_rep *from);
static void		analysis_copy_unreconciled_template(struct unrec_rep *to, struct unrec_rep *from);
static void		analysis_copy_cashflow_template(struct cashflow_rep *to, struct cashflow_rep *from);
static void		analysis_copy_balance_template(struct balance_rep *to, struct balance_rep *from);



/**
 * Initialise the Analysis module and all its dialogue boxes.
 */

void analysis_initialise(void)
{
	analysis_transaction_window = templates_create_window("TransRep");
	ihelp_add_window(analysis_transaction_window, "TransRep", NULL);
	event_add_window_mouse_event(analysis_transaction_window, analysis_transaction_click_handler);
	event_add_window_key_event(analysis_transaction_window, analysis_transaction_keypress_handler);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PDAYS, TRUE);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS, TRUE);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PYEARS, TRUE);

	analysis_unreconciled_window = templates_create_window("UnrecRep");
	ihelp_add_window(analysis_unreconciled_window, "UnrecRep", NULL);
	event_add_window_mouse_event(analysis_unreconciled_window, analysis_unreconciled_click_handler);
	event_add_window_key_event(analysis_unreconciled_window, analysis_unreconciled_keypress_handler);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS, TRUE);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS, TRUE);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS, TRUE);

	analysis_cashflow_window = templates_create_window("CashFlwRep");
	ihelp_add_window(analysis_cashflow_window, "CashFlwRep", NULL);
	event_add_window_mouse_event(analysis_cashflow_window, analysis_cashflow_click_handler);
	event_add_window_key_event(analysis_cashflow_window, analysis_cashflow_keypress_handler);
	event_add_window_icon_radio(analysis_cashflow_window, ANALYSIS_CASHFLOW_PDAYS, TRUE);
	event_add_window_icon_radio(analysis_cashflow_window, ANALYSIS_CASHFLOW_PMONTHS, TRUE);
	event_add_window_icon_radio(analysis_cashflow_window, ANALYSIS_CASHFLOW_PYEARS, TRUE);

	analysis_balance_window = templates_create_window("BalanceRep");
	ihelp_add_window(analysis_balance_window, "BalanceRep", NULL);
	event_add_window_mouse_event(analysis_balance_window, analysis_balance_click_handler);
	event_add_window_key_event(analysis_balance_window, analysis_balance_keypress_handler);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PDAYS, TRUE);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS, TRUE);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PYEARS, TRUE);

	analysis_save_window = templates_create_window("SaveRepTemp");
	ihelp_add_window(analysis_save_window, "SaveRepTemp", NULL);
	event_add_window_mouse_event(analysis_save_window, analysis_save_click_handler);
	event_add_window_key_event(analysis_save_window, analysis_save_keypress_handler);
	event_add_window_menu_prepare(analysis_save_window, analysis_save_menu_prepare_handler);
	event_add_window_menu_selection(analysis_save_window, analysis_save_menu_selection_handler);
	event_add_window_menu_close(analysis_save_window, analysis_save_menu_close_handler);
	event_add_window_icon_popup(analysis_save_window, ANALYSIS_SAVE_NAMEPOPUP, NULL, -1, NULL);

	analysis_lookup_window = templates_create_window("AccEnter");
	ihelp_add_window(analysis_lookup_window, "AccEnter", NULL);
	event_add_window_mouse_event(analysis_lookup_window, analysis_lookup_click_handler);
	event_add_window_key_event(analysis_lookup_window, analysis_lookup_keypress_handler);
}


/**
 * Construct new transaction report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct trans_rep *analysis_create_transaction(void)
{
	struct trans_rep	*new;

	new = heap_alloc(sizeof(struct trans_rep));
	if (new == NULL)
		return NULL;

	new->date_from = NULL_DATE;
	new->date_to = NULL_DATE;
	new->budget = 0;
	new->group = 0;
	new->period = 1;
	new->period_unit = DATE_PERIOD_MONTHS;
	new->lock = 0;
	new->from_count = 0;
	new->to_count = 0;
	*(new->ref) = '\0';
	*(new->desc) = '\0';
	new->amount_min = NULL_CURRENCY;
	new->amount_max = NULL_CURRENCY;
	new->output_trans = 1;
	new->output_summary = 1;
	new->output_accsummary = 1;

	return new;
}


/**
 * Delete a transaction report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_transaction(struct trans_rep *report)
{
	if (report != NULL)
		heap_free(report);
}


/**
 * Open the Transaction Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_transaction_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore)
{
	osbool		template_mode;

	/* If the window is already open, another transction report is being edited.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_transaction_window))
		wimp_close_window(analysis_transaction_window);

	/* Copy the settings block contents into a static place that won't shift about on the flex heap
	 * while the dialogue is open.
	 */

	template_mode = (template >= 0 && template < file->saved_report_count);

	if (template_mode) {
		analysis_copy_transaction_template(&(analysis_transaction_settings), &(file->saved_reports[template].data.transaction));
		analysis_transaction_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_transaction_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_transaction_template(&(analysis_transaction_settings), file->trans_rep);
		analysis_transaction_template = NULL_TEMPLATE;

		msgs_lookup("TrnRepTitle", windows_get_indirected_title_addr(analysis_transaction_window), 40);
	}

	icons_set_deleted(analysis_transaction_window, ANALYSIS_TRANS_DELETE, !template_mode);
	icons_set_deleted(analysis_transaction_window, ANALYSIS_TRANS_RENAME, !template_mode);

	/* Set the window contents up. */

	analysis_fill_transaction_window(file, restore);

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_transaction_file = file;
	analysis_transaction_restore = restore;

	windows_open_centred_at_pointer(analysis_transaction_window, ptr);
	place_dialogue_caret_fallback(analysis_transaction_window, 4, ANALYSIS_TRANS_DATEFROM, ANALYSIS_TRANS_DATETO,
			ANALYSIS_TRANS_PERIOD, ANALYSIS_TRANS_FROMSPEC);
}


/**
 * Process mouse clicks in the Analysis Transaction dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_transaction_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ANALYSIS_TRANS_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_transaction_window);
			analysis_force_close_report_rename_window(analysis_transaction_window);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_transaction_window();
		}
		break;

	case ANALYSIS_TRANS_OK:
		if (analysis_process_transaction_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_transaction_window);
			analysis_force_close_report_rename_window(analysis_transaction_window);
		}
		break;

	case ANALYSIS_TRANS_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_transaction_window())
			close_dialogue_with_caret(analysis_transaction_window);
		break;

	case ANALYSIS_TRANS_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_transaction_template >= 0 && analysis_transaction_template < analysis_transaction_file->saved_report_count)
			analysis_open_rename_window(analysis_transaction_file, analysis_transaction_template, pointer);
		break;

	case ANALYSIS_TRANS_BUDGET:
		icons_set_group_shaded_when_on(analysis_transaction_window, ANALYSIS_TRANS_BUDGET, 4,
				ANALYSIS_TRANS_DATEFROMTXT, ANALYSIS_TRANS_DATEFROM,
				ANALYSIS_TRANS_DATETOTXT, ANALYSIS_TRANS_DATETO);
		icons_replace_caret_in_window(analysis_transaction_window);
		break;

	case ANALYSIS_TRANS_GROUP:
		icons_set_group_shaded_when_off(analysis_transaction_window, ANALYSIS_TRANS_GROUP, 6,
				ANALYSIS_TRANS_PERIOD, ANALYSIS_TRANS_PTEXT,
				ANALYSIS_TRANS_PDAYS, ANALYSIS_TRANS_PMONTHS, ANALYSIS_TRANS_PYEARS,
				ANALYSIS_TRANS_LOCK);
		icons_replace_caret_in_window(analysis_transaction_window);
		break;

	case ANALYSIS_TRANS_FROMSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		break;

	case ANALYSIS_TRANS_TOSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_TOSPEC, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
		break;
	}
}


/**
 * Process keypresses in the Analysis Transaction window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool analysis_transaction_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_process_transaction_window()) {
			close_dialogue_with_caret(analysis_transaction_window);
			analysis_force_close_report_rename_window(analysis_transaction_window);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_transaction_window);
		analysis_force_close_report_rename_window(analysis_transaction_window);
		break;

	case wimp_KEY_F1:
		if (key->i == ANALYSIS_TRANS_FROMSPEC)
			analysis_open_account_lookup(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		else if (key->i == ANALYSIS_TRANS_TOSPEC)
			analysis_open_account_lookup(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_TOSPEC, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Transaction window.
 */

static void analysis_refresh_transaction_window(void)
{
	analysis_fill_transaction_window(analysis_transaction_file, analysis_transaction_restore);
	icons_redraw_group(analysis_transaction_window, 9,
			ANALYSIS_TRANS_DATEFROM, ANALYSIS_TRANS_DATETO, ANALYSIS_TRANS_PERIOD,
			ANALYSIS_TRANS_FROMSPEC, ANALYSIS_TRANS_TOSPEC,
			ANALYSIS_TRANS_REFSPEC, ANALYSIS_TRANS_DESCSPEC,
			ANALYSIS_TRANS_AMTLOSPEC, ANALYSIS_TRANS_AMTHISPEC);

	icons_replace_caret_in_window(analysis_transaction_window);
}


/**
 * Fill the Transaction window with values.
 *
 * \param: *find_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void analysis_fill_transaction_window(struct file_block *file, osbool restore)
{
	if (!restore) {
		/* Set the period icons. */

		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATEFROM) = '\0';
		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATETO) = '\0';

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_BUDGET, 0);

		/* Set the grouping icons. */

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_GROUP, 0);

		icons_strncpy(analysis_transaction_window, ANALYSIS_TRANS_PERIOD, "1");
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PDAYS, 0);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS, 1);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PYEARS, 0);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_LOCK, 0);

		/* Set the include icons. */

		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_FROMSPEC) = '\0';
		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_TOSPEC) = '\0';
		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_REFSPEC) = '\0';
		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC) = '\0';
		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC) = '\0';
		*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DESCSPEC) = '\0';

		/* Set the output icons. */

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPTRANS, 1);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPSUMMARY, 1);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPACCSUMMARY, 1);
	} else {
		/* Set the period icons. */

		date_convert_to_string(analysis_transaction_settings.date_from,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATEFROM),
				icons_get_indirected_text_length(analysis_transaction_window, ANALYSIS_TRANS_DATEFROM));
		date_convert_to_string(analysis_transaction_settings.date_to,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATETO),
				icons_get_indirected_text_length(analysis_transaction_window, ANALYSIS_TRANS_DATETO));

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_BUDGET, analysis_transaction_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_GROUP, analysis_transaction_settings.group);

		icons_printf(analysis_transaction_window, ANALYSIS_TRANS_PERIOD, "%d", analysis_transaction_settings.period);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PDAYS, analysis_transaction_settings.period_unit == DATE_PERIOD_DAYS);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS, analysis_transaction_settings.period_unit == DATE_PERIOD_MONTHS);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PYEARS, analysis_transaction_settings.period_unit == DATE_PERIOD_YEARS);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_LOCK, analysis_transaction_settings.lock);

		/* Set the include icons. */

		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_FROMSPEC),
				analysis_transaction_settings.from, analysis_transaction_settings.from_count);
		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_TOSPEC),
				analysis_transaction_settings.to, analysis_transaction_settings.to_count);
		icons_strncpy(analysis_transaction_window, ANALYSIS_TRANS_REFSPEC, analysis_transaction_settings.ref);
		currency_convert_to_string(analysis_transaction_settings.amount_min,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC),
				icons_get_indirected_text_length(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC));
		currency_convert_to_string(analysis_transaction_settings.amount_max,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC),
				icons_get_indirected_text_length(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC));
		icons_strncpy(analysis_transaction_window, ANALYSIS_TRANS_DESCSPEC, analysis_transaction_settings.desc);

		/* Set the output icons. */

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPTRANS, analysis_transaction_settings.output_trans);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPSUMMARY, analysis_transaction_settings.output_summary);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPACCSUMMARY, analysis_transaction_settings.output_accsummary);
	}

	icons_set_group_shaded_when_on(analysis_transaction_window, ANALYSIS_TRANS_BUDGET, 4,
			ANALYSIS_TRANS_DATEFROMTXT, ANALYSIS_TRANS_DATEFROM,
			ANALYSIS_TRANS_DATETOTXT, ANALYSIS_TRANS_DATETO);

	icons_set_group_shaded_when_off(analysis_transaction_window, ANALYSIS_TRANS_GROUP, 6,
			ANALYSIS_TRANS_PERIOD, ANALYSIS_TRANS_PTEXT,
			ANALYSIS_TRANS_PDAYS, ANALYSIS_TRANS_PMONTHS, ANALYSIS_TRANS_PYEARS,
			ANALYSIS_TRANS_LOCK);
}


/**
 * Process the contents of the Transaction window, store the details and
 * generate a report from them.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool analysis_process_transaction_window(void)
{
	/* Read the date settings. */

	analysis_transaction_file->trans_rep->date_from =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATEFROM), NULL_DATE, 0);
	analysis_transaction_file->trans_rep->date_to =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATETO), NULL_DATE, 0);
	analysis_transaction_file->trans_rep->budget = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_BUDGET);

	/* Read the grouping settings. */

	analysis_transaction_file->trans_rep->group = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_GROUP);
	analysis_transaction_file->trans_rep->period = atoi(icons_get_indirected_text_addr (analysis_transaction_window, ANALYSIS_TRANS_PERIOD));

	if (icons_get_selected	(analysis_transaction_window, ANALYSIS_TRANS_PDAYS))
		analysis_transaction_file->trans_rep->period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS))
		analysis_transaction_file->trans_rep->period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_PYEARS))
		analysis_transaction_file->trans_rep->period_unit = DATE_PERIOD_YEARS;
	else
		analysis_transaction_file->trans_rep->period_unit = DATE_PERIOD_MONTHS;

	analysis_transaction_file->trans_rep->lock = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_LOCK);

	/* Read the account and heading settings. */

	analysis_transaction_file->trans_rep->from_count =
			analysis_account_idents_to_list(analysis_transaction_file, ACCOUNT_FULL | ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_FROMSPEC),
			analysis_transaction_file->trans_rep->from);
	analysis_transaction_file->trans_rep->to_count =
			analysis_account_idents_to_list(analysis_transaction_file, ACCOUNT_FULL | ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_TOSPEC),
			analysis_transaction_file->trans_rep->to);
	strcpy(analysis_transaction_file->trans_rep->ref,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_REFSPEC));
	strcpy(analysis_transaction_file->trans_rep->desc,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DESCSPEC));
	analysis_transaction_file->trans_rep->amount_min = (*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC) == '\0') ?
			NULL_CURRENCY : currency_convert_from_string(icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC));

	analysis_transaction_file->trans_rep->amount_max = (*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC) == '\0') ?
			NULL_CURRENCY : currency_convert_from_string(icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC));

	/* Read the output options. */

	analysis_transaction_file->trans_rep->output_trans = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_OPTRANS);
 	analysis_transaction_file->trans_rep->output_summary = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_OPSUMMARY);
	analysis_transaction_file->trans_rep->output_accsummary = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_OPACCSUMMARY);

	/* Run the report. */

	analysis_generate_transaction_report(analysis_transaction_file);

	return TRUE;
}


/**
 * Delete the template used in the current Transaction window.
 *
 * \return			TRUE if the template was deleted; else FALSE.
 */

static osbool analysis_delete_transaction_window(void)
{
	if (analysis_transaction_template >= 0 && analysis_transaction_template < analysis_transaction_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_template(analysis_transaction_file, analysis_transaction_template);
		analysis_transaction_template = NULL_TEMPLATE;

		return TRUE;
	} else {
		return FALSE;
	}
}


/**
 * Generate a transaction report based on the settings in the given file.
 *
 * \param *file			The file to generate the report for.
 */

static void analysis_generate_transaction_report(struct file_block *file)
{
	struct report		*report;
	struct analysis_data	*data = NULL;
	struct analysis_report	*template;
	int			i, found, total, unit, period, group, lock, output_trans, output_summary, output_accsummary,
				total_days, period_days, period_limit, entries, account;
	date_t			start_date, end_date, next_start, next_end, date;
	acct_t			from, to;
	amt_t			min_amount, max_amount, amount;
	char			line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
	char			*match_ref, *match_desc;

	if (file == NULL)
		return;

	if (!flexutils_allocate((void **) &data, sizeof(struct analysis_data), account_get_count(file))) {
		error_msgs_report_info("NoMemReport");
		return;
	}

	hourglass_on();

	transact_sort_file_data(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date,
			file->trans_rep->date_from, file->trans_rep->date_to, file->trans_rep->budget);

	total_days = date_count_days(start_date, end_date);

	/* Read the grouping settings. */

	group = file->trans_rep->group;
	unit = file->trans_rep->period_unit;
	lock = file->trans_rep->lock && (unit == DATE_PERIOD_MONTHS || unit == DATE_PERIOD_YEARS);
	period = (group) ? file->trans_rep->period : 0;

	/* Read the include list. */

	analysis_clear_account_report_flags(file, data);

	if (file->trans_rep->from_count == 0 && file->trans_rep->to_count == 0) {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_IN, ANALYSIS_REPORT_FROM,
				&analysis_wildcard_account_list, 1);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_OUT, ANALYSIS_REPORT_TO,
				&analysis_wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_IN, ANALYSIS_REPORT_FROM,
				file->trans_rep->from, file->trans_rep->from_count);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_OUT, ANALYSIS_REPORT_TO,
				file->trans_rep->to, file->trans_rep->to_count);
	}

	min_amount = file->trans_rep->amount_min;
	max_amount = file->trans_rep->amount_max;

	match_ref = (*(file->trans_rep->ref) == '\0') ? NULL : file->trans_rep->ref;
	match_desc = (*(file->trans_rep->desc) == '\0') ? NULL : file->trans_rep->desc;

	/* Read the output options. */

	output_trans = file->trans_rep->output_trans;
	output_summary = file->trans_rep->output_summary;
	output_accsummary = file->trans_rep->output_accsummary;

	/* Open a new report for output. */

	analysis_copy_transaction_template(&(analysis_report_template.data.transaction), file->trans_rep);
	if (analysis_transaction_template == NULL_TEMPLATE)
		*(analysis_report_template.name) = '\0';
	else
		strcpy(analysis_report_template.name, file->saved_reports[analysis_transaction_template].name);
	analysis_report_template.type = REPORT_TYPE_TRANS;
	analysis_report_template.file = file;

	template = analysis_create_template(&analysis_report_template);
	if (template == NULL) {
		if (data != NULL)
			flexutils_free((void **) &data);
		hourglass_off();
		error_msgs_report_info("NoMemReport");
		return;
	}

	msgs_lookup("TRWinT", line, sizeof (line));
	report = report_open(file, line, template);

	if (report == NULL) {
		if (data != NULL)
			flexutils_free((void **) &data);
		if (template != NULL)
			heap_free(template);
		hourglass_off();
		return;
	}

	/* Output report heading */

	file_get_leafname(file, b1, sizeof(b1));
	if (*analysis_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof(line), analysis_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("TRTitle", line, sizeof(line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	date_convert_to_string(start_date, b1, sizeof(b1));
	date_convert_to_string(end_date, b2, sizeof(b2));
	date_convert_to_string(date_today(), b3, sizeof(b3));
	msgs_param_lookup("TRHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	analysis_initialise_date_period(start_date, end_date, period, unit, lock);

	/* Initialise the heading remainder values for the report. */

	for (i = 0; i < account_get_count(file); i++) {
		enum account_type type = account_get_type(file, i);
	
		if (type & ACCOUNT_OUT)
			data[i].report_balance = account_get_budget_amount(file, i);
		else if (type & ACCOUNT_IN)
			data[i].report_balance = -account_get_budget_amount(file, i);
	}

	while (analysis_get_next_date_period(&next_start, &next_end, date_text, sizeof(date_text))) {

		/* Zero the heading totals for the report. */

		for (i = 0; i < account_get_count(file); i++)
			data[i].report_total = 0;

		/* Scan through the transactions, adding the values up for those in range and outputting them to the screen. */

		found = 0;

		for (i = 0; i < transact_get_count(file); i++) {
			date = transact_get_date(file, i);
			from = transact_get_from(file, i);
			to = transact_get_to(file, i);
			amount = transact_get_amount(file, i);
		
			if ((next_start == NULL_DATE || date >= next_start) &&
					(next_end == NULL_DATE || date <= next_end) &&
					(((from != NULL_ACCOUNT) &&
					((data[from].report_flags & ANALYSIS_REPORT_FROM) != 0)) ||
					((to != NULL_ACCOUNT) &&
					((data[to].report_flags & ANALYSIS_REPORT_TO) != 0))) &&
					((min_amount == NULL_CURRENCY) || (amount >= min_amount)) &&
					((max_amount == NULL_CURRENCY) || (amount <= max_amount)) &&
					((match_ref == NULL) || string_wildcard_compare(match_ref, transact_get_reference(file, i, NULL, 0), TRUE)) &&
					((match_desc == NULL) || string_wildcard_compare(match_desc, transact_get_description(file, i, NULL, 0), TRUE))) {
				if (found == 0) {
					report_write_line(report, 0, "");

					if (group == TRUE) {
						sprintf(line, "\\u%s", date_text);
						report_write_line(report, 0, line);
					}
					if (output_trans) {
						msgs_lookup("TRHeadings", line, sizeof(line));
						report_write_line(report, 1, line);
					}
				}

				found++;

				/* Update the totals and output the transaction to the report file. */

				if (from != NULL_ACCOUNT)
					data[from].report_total -= amount;

				if (to != NULL_ACCOUNT)
					data[to].report_total += amount;

				if (output_trans) {
					date_convert_to_string(date, b1, sizeof(b1));
					currency_convert_to_string(amount, b2, sizeof(b2));

					sprintf(line, "\\k\\d\\r%d\\t%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s",
							transact_get_transaction_number(i), b1,
							account_get_name(file, from),
							account_get_name(file, to),
							transact_get_reference(file, i, NULL, 0), b2,
							transact_get_description(file, i, NULL, 0));

					report_write_line(report, 1, line);
				}
			}
		}

		/* Print the account summaries. */

		if (output_accsummary && found > 0) {
			/* Summarise the accounts. */

			total = 0;

			if (output_trans) /* Only output blank line if there are transactions above. */
				report_write_line(report, 0, "");
			msgs_lookup("TRAccounts", b1, sizeof(b1));
			sprintf(line, "\\i%s", b1);
			report_write_line(report, 2, line);

			entries = account_get_list_length(file, ACCOUNT_FULL);

			for (i = 0; i < entries; i++) {
				if ((account = account_get_list_entry_account(file, ACCOUNT_FULL, i)) != NULL_ACCOUNT) {
					if (data[account].report_total != 0) {
						total += data[account].report_total;
						currency_convert_to_string(data[account].report_total, b1, sizeof(b1));
						sprintf(line, "\\k\\i%s\\t\\d\\r%s", account_get_name(file, account), b1);
						report_write_line(report, 2, line);
					}
				}
			}

			msgs_lookup("TRTotal", b1, sizeof (b1));
			currency_convert_to_string(total, b2, sizeof(b2));
			sprintf(line, "\\i\\k\\b%s\\t\\d\\r\\b%s", b1, b2);
			report_write_line(report, 2, line);
		}

		/* Print the transaction summaries. */

		if (output_summary && found > 0) {
			/* Summarise the outgoings. */

			total = 0;

			if (output_trans || output_accsummary) /* Only output blank line if there is something above. */
				report_write_line(report, 0, "");
			msgs_lookup("TROutgoings", b1, sizeof(b1));
			sprintf(line, "\\i%s", b1);
			if (file->trans_rep->budget){
				msgs_lookup("TRSummExtra", b1, sizeof(b1));
				strcat(line, b1);
			}
			report_write_line(report, 2, line);

			entries = account_get_list_length(file, ACCOUNT_OUT);

			for (i = 0; i < entries; i++) {
				if ((account = account_get_list_entry_account(file, ACCOUNT_OUT, i)) != NULL_ACCOUNT) {
					if (data[account].report_total != 0) {
						total += data[account].report_total;
						currency_convert_to_string(data[account].report_total, b1, sizeof(b1));
						sprintf(line, "\\i\\k%s\\t\\d\\r%s", account_get_name(file, account), b1);
						if (file->trans_rep->budget) {
							period_days = date_count_days(next_start, next_end);
							period_limit = account_get_budget_amount(file, account) * period_days / total_days;
							currency_convert_to_string(period_limit, b1, sizeof(b1));
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							currency_convert_to_string(period_limit - data[account].report_total, b1, sizeof(b1));
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							data[i].report_balance -= data[account].report_total;
							currency_convert_to_string(data[account].report_balance, b1, sizeof(b1));
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
						}

						report_write_line(report, 2, line);
					}
				}
			}

			msgs_lookup("TRTotal", b1, sizeof(b1));
			currency_convert_to_string(total, b2, sizeof(b2));
			sprintf(line, "\\i\\k\\b%s\\t\\d\\r\\b%s", b1, b2);
			report_write_line(report, 2, line);

			/* Summarise the incomings. */

			total = 0;

			report_write_line(report, 0, "");
			msgs_lookup("TRIncomings", b1, sizeof(b1));
			sprintf(line, "\\i%s", b1);
			if (file->trans_rep->budget) {
				msgs_lookup("TRSummExtra", b1, sizeof(b1));
				strcat(line, b1);
			}

			report_write_line(report, 2, line);

			entries = account_get_list_length(file, ACCOUNT_IN);

			for (i = 0; i < entries; i++) {
				if ((account = account_get_list_entry_account(file, ACCOUNT_IN, i)) != NULL_ACCOUNT) {
					if (data[account].report_total != 0) {
						total += data[account].report_total;
						currency_convert_to_string(-data[account].report_total, b1, sizeof(b1));
						sprintf(line, "\\i\\k%s\\t\\d\\r%s", account_get_name(file, account), b1);
						if (file->trans_rep->budget) {
							period_days = date_count_days(next_start, next_end);
							period_limit = account_get_budget_amount(file, account) * period_days / total_days;
							currency_convert_to_string(period_limit, b1, sizeof(b1));
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							currency_convert_to_string(period_limit - data[account].report_total, b1, sizeof(b1));
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							data[i].report_balance -= data[account].report_total;
							currency_convert_to_string(data[account].report_balance, b1, sizeof(b1));
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
						}

						report_write_line(report, 2, line);
					}
				}
			}

			msgs_lookup("TRTotal", b1, sizeof(b1));
			currency_convert_to_string(-total, b2, sizeof(b2));
			sprintf(line, "\\i\\k\\b%s\\t\\d\\r\\b%s", b1, b2);
			report_write_line(report, 2, line);
		}
	}

	report_close(report);

	if (data != NULL)
		flexutils_free((void **) &data);

	hourglass_off();
}


/**
 * Construct new unreconciled report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct unrec_rep *analysis_create_unreconciled(void)
{
	struct unrec_rep	*new;

	new = heap_alloc(sizeof(struct unrec_rep));
	if (new == NULL)
		return NULL;

	new->date_from = NULL_DATE;
	new->date_to = NULL_DATE;
	new->budget = 0;
	new->group = 0;
	new->period = 1;
	new->period_unit = DATE_PERIOD_MONTHS;
	new->lock = 0;
	new->from_count = 0;
	new->to_count = 0;

	return new;
}


/**
 * Delete an unreconciled report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_unreconciled(struct unrec_rep *report)
{
	if (report != NULL)
		heap_free(report);
}


/**
 * Open the Unreconciled Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_unreconciled_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore)
{
	osbool		template_mode;

	/* If the window is already open, another unreconciled report is being edited.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_unreconciled_window))
		wimp_close_window(analysis_unreconciled_window);

	/* Copy the settings block contents into a static place that won't shift about on the flex heap
	 * while the dialogue is open.
	 */

	template_mode = (template >= 0 && template < file->saved_report_count);

	if (template_mode) {
		analysis_copy_unreconciled_template(&(analysis_unreconciled_settings), &(file->saved_reports[template].data.unreconciled));
		analysis_unreconciled_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_unreconciled_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_unreconciled_template(&(analysis_unreconciled_settings), file->unrec_rep);
		analysis_unreconciled_template = NULL_TEMPLATE;

		msgs_lookup("UrcRepTitle", windows_get_indirected_title_addr(analysis_unreconciled_window), 40);
	}

	icons_set_deleted(analysis_unreconciled_window, ANALYSIS_UNREC_DELETE, !template_mode);
	icons_set_deleted(analysis_unreconciled_window, ANALYSIS_UNREC_RENAME, !template_mode);

	/* Set the window contents up. */

	analysis_fill_unreconciled_window(file, restore);

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_unreconciled_file = file;
	analysis_unreconciled_restore = restore;

	windows_open_centred_at_pointer(analysis_unreconciled_window, ptr);
	place_dialogue_caret_fallback(analysis_unreconciled_window, 4, ANALYSIS_UNREC_DATEFROM, ANALYSIS_UNREC_DATETO,
			ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_FROMSPEC);
}


/**
 * Process mouse clicks in the Analysis Unreconciled dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_unreconciled_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ANALYSIS_UNREC_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_unreconciled_window);
			analysis_force_close_report_rename_window(analysis_unreconciled_window);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_unreconciled_window();
		}
		break;

	case ANALYSIS_UNREC_OK:
		if (analysis_process_unreconciled_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_unreconciled_window);
			analysis_force_close_report_rename_window(analysis_unreconciled_window);
		}
		break;

	case ANALYSIS_UNREC_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_unreconciled_window())
			close_dialogue_with_caret(analysis_unreconciled_window);
		break;

	case ANALYSIS_UNREC_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_unreconciled_template >= 0 &&
				analysis_unreconciled_template < analysis_unreconciled_file->saved_report_count)
			analysis_open_rename_window (analysis_unreconciled_file, analysis_unreconciled_template, pointer);
		break;

	case ANALYSIS_UNREC_BUDGET:
		icons_set_group_shaded_when_on(analysis_unreconciled_window, ANALYSIS_UNREC_BUDGET, 4,
				ANALYSIS_UNREC_DATEFROMTXT, ANALYSIS_UNREC_DATEFROM,
				ANALYSIS_UNREC_DATETOTXT, ANALYSIS_UNREC_DATETO);
		icons_replace_caret_in_window(analysis_unreconciled_window);
		break;

	case ANALYSIS_UNREC_GROUP:
		icons_set_group_shaded_when_off(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP, 2,
				ANALYSIS_UNREC_GROUPACC, ANALYSIS_UNREC_GROUPDATE);

		icons_set_group_shaded(analysis_unreconciled_window,
				!(icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP) &&
				icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE)), 6,
				ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
				ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);

		icons_replace_caret_in_window(analysis_unreconciled_window);
		break;

	case ANALYSIS_UNREC_GROUPACC:
	case ANALYSIS_UNREC_GROUPDATE:
		icons_set_selected(analysis_unreconciled_window, pointer->i, 1);

		icons_set_group_shaded(analysis_unreconciled_window,
				!(icons_get_selected (analysis_unreconciled_window, ANALYSIS_UNREC_GROUP) &&
				icons_get_selected (analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE)), 6,
				ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
				ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);

		icons_replace_caret_in_window(analysis_unreconciled_window);
		break;

	case ANALYSIS_UNREC_FROMSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		break;

	case ANALYSIS_UNREC_TOSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_TOSPEC, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
		break;
	}
}


/**
 * Process keypresses in the Analysis Unreconciled window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool analysis_unreconciled_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_process_unreconciled_window()) {
			close_dialogue_with_caret(analysis_unreconciled_window);
			analysis_force_close_report_rename_window(analysis_unreconciled_window);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_unreconciled_window);
		analysis_force_close_report_rename_window(analysis_unreconciled_window);
		break;

	case wimp_KEY_F1:
		if (key->i == ANALYSIS_UNREC_FROMSPEC)
			analysis_open_account_lookup(analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		else if (key->i == ANALYSIS_UNREC_TOSPEC)
			analysis_open_account_lookup(analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_TOSPEC, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Unreconciled window.
 */

static void analysis_refresh_unreconciled_window(void)
{
	analysis_fill_unreconciled_window(analysis_unreconciled_file, analysis_unreconciled_restore);
	icons_redraw_group(analysis_unreconciled_window, 5,
			ANALYSIS_UNREC_DATEFROM, ANALYSIS_UNREC_DATETO, ANALYSIS_UNREC_PERIOD,
			ANALYSIS_UNREC_FROMSPEC, ANALYSIS_UNREC_TOSPEC);

	icons_replace_caret_in_window(analysis_unreconciled_window);
}


/**
 * Fill the Unreconciled window with values.
 *
 * \param: *find_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void analysis_fill_unreconciled_window(struct file_block *file, osbool restore)
{
	if (!restore) {
		/* Set the period icons. */

		*icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATEFROM) = '\0';
		*icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATETO) = '\0';

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_BUDGET, 0);

		/* Set the grouping icons. */

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP, 0);

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPACC, 1);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE, 0);

		icons_strncpy(analysis_unreconciled_window, ANALYSIS_UNREC_PERIOD, "1");
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS, 0);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS, 1);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS, 0);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_LOCK, 0);

		/* Set the from and to spec fields. */

		*icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_FROMSPEC) = '\0';
		*icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_TOSPEC) = '\0';
	} else {
		/* Set the period icons. */

		date_convert_to_string(analysis_unreconciled_settings.date_from,
				icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATEFROM),
				icons_get_indirected_text_length(analysis_unreconciled_window, ANALYSIS_UNREC_DATEFROM));
		date_convert_to_string(analysis_unreconciled_settings.date_to,
				icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATETO),
				icons_get_indirected_text_length(analysis_unreconciled_window, ANALYSIS_UNREC_DATETO));

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_BUDGET, analysis_unreconciled_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP, analysis_unreconciled_settings.group);

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPACC, analysis_unreconciled_settings.period_unit == DATE_PERIOD_NONE);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE, analysis_unreconciled_settings.period_unit != DATE_PERIOD_NONE);

		icons_printf(analysis_unreconciled_window, ANALYSIS_UNREC_PERIOD, "%d", analysis_unreconciled_settings.period);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS, analysis_unreconciled_settings.period_unit == DATE_PERIOD_DAYS);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS,
				analysis_unreconciled_settings.period_unit == DATE_PERIOD_MONTHS ||
				analysis_unreconciled_settings.period_unit == DATE_PERIOD_NONE);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS, analysis_unreconciled_settings.period_unit == DATE_PERIOD_YEARS);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_LOCK, analysis_unreconciled_settings.lock);

		/* Set the from and to spec fields. */

		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_FROMSPEC),
				analysis_unreconciled_settings.from, analysis_unreconciled_settings.from_count);
		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_TOSPEC),
				analysis_unreconciled_settings.to, analysis_unreconciled_settings.to_count);
	}

	icons_set_group_shaded_when_on(analysis_unreconciled_window, ANALYSIS_UNREC_BUDGET, 4,
			ANALYSIS_UNREC_DATEFROMTXT, ANALYSIS_UNREC_DATEFROM,
			ANALYSIS_UNREC_DATETOTXT, ANALYSIS_UNREC_DATETO);

	icons_set_group_shaded_when_off(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP, 2,
			ANALYSIS_UNREC_GROUPACC, ANALYSIS_UNREC_GROUPDATE);

	icons_set_group_shaded(analysis_unreconciled_window,
			!(icons_get_selected (analysis_unreconciled_window, ANALYSIS_UNREC_GROUP) &&
			icons_get_selected (analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE)), 6,
			ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
			ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);
}


/**
 * Process the contents of the Unreconciled window, store the details and
 * generate a report from them.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool analysis_process_unreconciled_window(void)
{
	/* Read the date settings. */

	analysis_unreconciled_file->unrec_rep->date_from =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATEFROM), NULL_DATE, 0);
	analysis_unreconciled_file->unrec_rep->date_to =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATETO), NULL_DATE, 0);
	analysis_unreconciled_file->unrec_rep->budget = icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_BUDGET);

	/* Read the grouping settings. */

	analysis_unreconciled_file->unrec_rep->group = icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP);
	analysis_unreconciled_file->unrec_rep->period = atoi(icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_PERIOD));

	if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPACC))
		analysis_unreconciled_file->unrec_rep->period_unit = DATE_PERIOD_NONE;
	else if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS))
		analysis_unreconciled_file->unrec_rep->period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS))
		analysis_unreconciled_file->unrec_rep->period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS))
		analysis_unreconciled_file->unrec_rep->period_unit = DATE_PERIOD_YEARS;
	else
		analysis_unreconciled_file->unrec_rep->period_unit = DATE_PERIOD_MONTHS;

	analysis_unreconciled_file->unrec_rep->lock = icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_LOCK);

	/* Read the account and heading settings. */

	analysis_unreconciled_file->unrec_rep->from_count =
			analysis_account_idents_to_list(analysis_unreconciled_file, ACCOUNT_FULL | ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_FROMSPEC),
			analysis_unreconciled_file->unrec_rep->from);
	analysis_unreconciled_file->unrec_rep->to_count =
			analysis_account_idents_to_list(analysis_unreconciled_file, ACCOUNT_FULL | ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_TOSPEC),
			analysis_unreconciled_file->unrec_rep->to);

	/* Run the report. */

	analysis_generate_unreconciled_report(analysis_unreconciled_file);

	return TRUE;
}


/**
 * Delete the template used in the current Transaction window.
 *
 * \return			TRUE if the template was deleted; else FALSE.
 */

static osbool analysis_delete_unreconciled_window(void)
{
	if (analysis_unreconciled_template >= 0 && analysis_unreconciled_template < analysis_unreconciled_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_template(analysis_unreconciled_file, analysis_unreconciled_template);
		analysis_unreconciled_template = NULL_TEMPLATE;

		return TRUE;
	} else {
		return FALSE;
	}
}


/**
 * Generate an unreconciled report based on the settings in the given file.
 *
 * \param *file			The file to generate the report for.
 */

static void analysis_generate_unreconciled_report(struct file_block *file)
{
	int			i, acc, found, unit, period, group, lock, tot_in, tot_out, entries;
	char			line[2048], b1[1024], b2[1024], b3[1024], date_text[1024],
				rec_char[REC_FIELD_LEN], r1[REC_FIELD_LEN], r2[REC_FIELD_LEN];
	date_t			start_date, end_date, next_start, next_end, date;
	acct_t			from, to;
	enum transact_flags	flags;
	amt_t			amount;
	struct report		*report;
	struct analysis_data	*data = NULL;
	struct analysis_report	*template;
	int			acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	if (file == NULL)
		return;

	if (!flexutils_allocate((void **) &data, sizeof(struct analysis_data), account_get_count(file))) {
		error_msgs_report_info("NoMemReport");
		return;
	}

	hourglass_on();

	transact_sort_file_data(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date, file->unrec_rep->date_from, file->unrec_rep->date_to, file->unrec_rep->budget);

	/* Read the grouping settings. */

	group = file->unrec_rep->group;
	unit = file->unrec_rep->period_unit;
	lock = file->unrec_rep->lock && (unit == DATE_PERIOD_MONTHS || unit == DATE_PERIOD_YEARS);
	period = (group) ? file->unrec_rep->period : 0;

	/* Read the include list. */

	analysis_clear_account_report_flags(file, data);

	if (file->unrec_rep->from_count == 0 && file->unrec_rep->to_count == 0) {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_IN, ANALYSIS_REPORT_FROM, &analysis_wildcard_account_list, 1);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_OUT, ANALYSIS_REPORT_TO, &analysis_wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_IN, ANALYSIS_REPORT_FROM, file->unrec_rep->from, file->unrec_rep->from_count);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_OUT, ANALYSIS_REPORT_TO, file->unrec_rep->to, file->unrec_rep->to_count);
	}

	/* Start to output the report details. */

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	analysis_copy_unreconciled_template(&(analysis_report_template.data.unreconciled), file->unrec_rep);
	if (analysis_unreconciled_template == NULL_TEMPLATE)
		*(analysis_report_template.name) = '\0';
	else
		strcpy(analysis_report_template.name, file->saved_reports[analysis_unreconciled_template].name);
	analysis_report_template.type = REPORT_TYPE_UNREC;
	analysis_report_template.file = file;

	template = analysis_create_template(&analysis_report_template);
	if (template == NULL) {
		if (data != NULL)
			flexutils_free((void **) &data);
		hourglass_off();
		error_msgs_report_info("NoMemReport");
		return;
	}

	msgs_lookup("URWinT", line, sizeof(line));
	report = report_open(file, line, template);

	if (report == NULL) {
		hourglass_off();
		return;
	}

	/* Output report heading */

	file_get_leafname(file, b1, sizeof(b1));
	if (*analysis_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof (line), analysis_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("URTitle", line, sizeof (line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	date_convert_to_string(start_date, b1, sizeof(b1));
	date_convert_to_string(end_date, b2, sizeof(b2));
	date_convert_to_string(date_today(), b3, sizeof(b3));
	msgs_param_lookup("URHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	if (group && unit == DATE_PERIOD_NONE) {
		/* We are doing a grouped-by-account report.
		 *
		 * Step through the accounts in account list order, and run through all the transactions each time.  A
		 * transaction is added if it is unreconciled in the account concerned; transactions unreconciled in two
		 * accounts may therefore appear twice in the list.
		 */

		for (acc_group = 0; acc_group < groups; acc_group++) {
			entries = account_get_list_length(file, sequence[acc_group]);

			for (group_line = 0; group_line < entries; group_line++) {
				if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
					found = 0;
					tot_in = 0;
					tot_out = 0;

					for (i = 0; i < transact_get_count(file); i++) {
						date = transact_get_date(file, i);
						from = transact_get_from(file, i);
						to = transact_get_to(file, i);
						flags = transact_get_flags(file, i);
						amount = transact_get_amount(file, i);

						if ((start_date == NULL_DATE || date >= start_date) &&
								(end_date == NULL_DATE || date <= end_date) &&
								((from == acc && (data[acc].report_flags & ANALYSIS_REPORT_FROM) != 0 &&
								(flags & TRANS_REC_FROM) == 0) ||
								(to == acc && (data[acc].report_flags & ANALYSIS_REPORT_TO) != 0 &&
								(flags & TRANS_REC_TO) == 0))) {
							if (found == 0) {
								report_write_line(report, 0, "");

								if (group == TRUE) {
									sprintf(line, "\\u%s", account_get_name(file, acc));
									report_write_line(report, 0, line);
								}
								msgs_lookup("URHeadings", line, sizeof(line));
								report_write_line(report, 1, line);
							}

							found++;

							if (from == acc)
								tot_out -= amount;
							else if (to == acc)
								tot_in += amount;

							/* Output the transaction to the report. */

							strcpy(r1, (flags & TRANS_REC_FROM) ? rec_char : "");
							strcpy(r2, (flags & TRANS_REC_TO) ? rec_char : "");
							date_convert_to_string(date, b1, sizeof(b1));
							currency_convert_to_string(amount, b2, sizeof(b2));

							sprintf(line, "\\k\\d\\r%d\\t%s\\t%s\\t%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s",
									transact_get_transaction_number(i), b1,
									r1, account_get_name(file, from),
									r2, account_get_name(file, to),
									transact_get_reference(file, i, NULL, 0), b2,
									transact_get_description(file, i, NULL, 0));

							report_write_line(report, 1, line);
						}
					}

					if (found != 0) {
						report_write_line(report, 2, "");

						msgs_lookup("URTotalIn", b1, sizeof(b1));
						currency_flexible_convert_to_string(tot_in, b2, sizeof(b2), TRUE);
						sprintf(line, "\\i%s\\t\\d\\r%s", b1, b2);
						report_write_line(report, 2, line);

						msgs_lookup("URTotalOut", b1, sizeof(b1));
						currency_flexible_convert_to_string(tot_out, b2, sizeof(b2), TRUE);
						sprintf(line, "\\i%s\\t\\d\\r%s", b1, b2);
						report_write_line(report, 2, line);

						msgs_lookup("URTotal", b1, sizeof(b1));
						currency_flexible_convert_to_string(tot_in+tot_out, b2, sizeof(b2), TRUE);
						sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
						report_write_line(report, 2, line);
					}
				}
			}
		}
	} else {
		/* We are either doing a grouped-by-date report, or not grouping at all.
		 *
		 * For each date period, run through the transactions and output any which fall within it.
		 */

		analysis_initialise_date_period(start_date, end_date, period, unit, lock);

		while (analysis_get_next_date_period(&next_start, &next_end, date_text, sizeof(date_text))) {
			found = 0;

			for (i = 0; i < transact_get_count(file); i++) {
				date = transact_get_date(file, i);
				from = transact_get_from(file, i);
				to = transact_get_to(file, i);
				flags = transact_get_flags(file, i);
				amount = transact_get_amount(file, i);

				if ((next_start == NULL_DATE || date >= next_start) &&
						(next_end == NULL_DATE || date <= next_end) &&
						(((flags & TRANS_REC_FROM) == 0 &&
						(from != NULL_ACCOUNT) &&
						(data[from].report_flags & ANALYSIS_REPORT_FROM) != 0) ||
						((flags & TRANS_REC_TO) == 0 &&
						(to != NULL_ACCOUNT) &&
						(data[to].report_flags & ANALYSIS_REPORT_TO) != 0))) {
					if (found == 0) {
						report_write_line(report, 0, "");

						if (group == TRUE) {
							sprintf(line, "\\u%s", date_text);
							report_write_line(report, 0, line);
						}
						msgs_param_lookup("URHeadings", line, sizeof(line), NULL, NULL, NULL, NULL);
						report_write_line(report, 1, line);
					}

					found++;

					/* Output the transaction to the report. */

					strcpy(r1, (flags & TRANS_REC_FROM) ? rec_char : "");
					strcpy(r2, (flags & TRANS_REC_TO) ? rec_char : "");
					date_convert_to_string(date, b1, sizeof(b1));
					currency_convert_to_string(amount, b2, sizeof(b2));

					sprintf(line, "\\k\\d\\r%d\\t%s\\t%s\\t%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s",
							 transact_get_transaction_number(i), b1,
							r1, account_get_name(file, from),
							r2, account_get_name(file, to),
							transact_get_reference(file, i, NULL, 0), b2,
							transact_get_description(file, i, NULL, 0));

					report_write_line(report, 1, line);
				}
			}
		}
	}

	report_close(report);

	if (data != NULL)
		flexutils_free((void **) &data);

	hourglass_off();
}


/**
 * Construct new cashflow report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct cashflow_rep *analysis_create_cashflow(void)
{
	struct cashflow_rep	*new;

	new = heap_alloc(sizeof(struct cashflow_rep));
	if (new == NULL)
		return NULL;

	new->date_from = NULL_DATE;
	new->date_to = NULL_DATE;
	new->budget = 0;
	new->group = 0;
	new->period = 1;
	new->period_unit = DATE_PERIOD_MONTHS;
	new->lock = 0;
	new->empty = 0;
	new->accounts_count = 0;
	new->incoming_count = 0;
	new->outgoing_count = 0;
	new->tabular = 0;

	return new;
}


/**
 * Delete a cashflow report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_cashflow(struct cashflow_rep *report)
{
	if (report != NULL)
		heap_free(report);
}


/**
 * Open the Cashflow Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_cashflow_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore)
{
	osbool		template_mode;

	/* If the window is already open, another cashflow report is being edited.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_cashflow_window))
		wimp_close_window(analysis_cashflow_window);

	/* Copy the settings block contents into a static place that won't shift about on the flex heap
	 * while the dialogue is open.
	 */

	template_mode = (template >= 0 && template < file->saved_report_count);

	if (template_mode) {
		analysis_copy_cashflow_template(&(analysis_cashflow_settings), &(file->saved_reports[template].data.cashflow));
		analysis_cashflow_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_cashflow_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_cashflow_template(&(analysis_cashflow_settings), file->cashflow_rep);
		analysis_cashflow_template = NULL_TEMPLATE;

		msgs_lookup ("CflRepTitle", windows_get_indirected_title_addr(analysis_cashflow_window), 40);
	}

	icons_set_deleted(analysis_cashflow_window, ANALYSIS_CASHFLOW_DELETE, !template_mode);
	icons_set_deleted(analysis_cashflow_window, ANALYSIS_CASHFLOW_RENAME, !template_mode);

	/* Set the window contents up. */

	analysis_fill_cashflow_window(file, restore);

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_cashflow_file = file;
	analysis_cashflow_restore = restore;

	windows_open_centred_at_pointer(analysis_cashflow_window, ptr);
	place_dialogue_caret_fallback(analysis_cashflow_window, 4, ANALYSIS_CASHFLOW_DATEFROM, ANALYSIS_CASHFLOW_DATETO,
			ANALYSIS_CASHFLOW_PERIOD, ANALYSIS_CASHFLOW_ACCOUNTS);
}


/**
 * Process mouse clicks in the Analysis Cashflow dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_cashflow_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ANALYSIS_CASHFLOW_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_cashflow_window);
			analysis_force_close_report_rename_window(analysis_cashflow_window);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_cashflow_window();
		}
		break;

	case ANALYSIS_CASHFLOW_OK:
		if (analysis_process_cashflow_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_cashflow_window);
			analysis_force_close_report_rename_window(analysis_cashflow_window);
		}
		break;

	case ANALYSIS_CASHFLOW_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_cashflow_window())
			close_dialogue_with_caret(analysis_cashflow_window);
		break;

	case ANALYSIS_CASHFLOW_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_cashflow_template >= 0 && analysis_cashflow_template < analysis_cashflow_file->saved_report_count)
			analysis_open_rename_window(analysis_cashflow_file, analysis_cashflow_template, pointer);
		break;

	case ANALYSIS_CASHFLOW_BUDGET:
		icons_set_group_shaded_when_on(analysis_cashflow_window, ANALYSIS_CASHFLOW_BUDGET, 4,
				ANALYSIS_CASHFLOW_DATEFROMTXT, ANALYSIS_CASHFLOW_DATEFROM,
				ANALYSIS_CASHFLOW_DATETOTXT, ANALYSIS_CASHFLOW_DATETO);
		icons_replace_caret_in_window(analysis_cashflow_window);
		break;

	case ANALYSIS_CASHFLOW_GROUP:
		icons_set_group_shaded_when_off(analysis_cashflow_window, ANALYSIS_CASHFLOW_GROUP, 7,
				ANALYSIS_CASHFLOW_PERIOD, ANALYSIS_CASHFLOW_PTEXT, ANALYSIS_CASHFLOW_LOCK,
				ANALYSIS_CASHFLOW_PDAYS, ANALYSIS_CASHFLOW_PMONTHS, ANALYSIS_CASHFLOW_PYEARS,
				ANALYSIS_CASHFLOW_EMPTY);

		icons_replace_caret_in_window(analysis_cashflow_window);
		break;

	case ANALYSIS_CASHFLOW_ACCOUNTSPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		break;

	case ANALYSIS_CASHFLOW_INCOMINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		break;

	case ANALYSIS_CASHFLOW_OUTGOINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_OUTGOING, NULL_ACCOUNT, ACCOUNT_OUT);
		break;
	}
}


/**
 * Process keypresses in the Analysis Cashflow window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool analysis_cashflow_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_process_cashflow_window()) {
			close_dialogue_with_caret(analysis_cashflow_window);
			analysis_force_close_report_rename_window(analysis_cashflow_window);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_cashflow_window);
		analysis_force_close_report_rename_window(analysis_cashflow_window);
		break;

	case wimp_KEY_F1:
		if (key->i == ANALYSIS_CASHFLOW_ACCOUNTS)
			analysis_open_account_lookup(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		else if (key->i == ANALYSIS_CASHFLOW_INCOMING)
			analysis_open_account_lookup(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		else if (key->i == ANALYSIS_CASHFLOW_OUTGOING)
			analysis_open_account_lookup(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_OUTGOING, NULL_ACCOUNT, ACCOUNT_OUT);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Cashflow window.
 */

static void analysis_refresh_cashflow_window(void)
{
	analysis_fill_cashflow_window(analysis_cashflow_file, analysis_cashflow_restore);
	icons_redraw_group(analysis_cashflow_window, 6,
			ANALYSIS_CASHFLOW_DATEFROM, ANALYSIS_CASHFLOW_DATETO, ANALYSIS_CASHFLOW_PERIOD,
			ANALYSIS_CASHFLOW_ACCOUNTS, ANALYSIS_CASHFLOW_INCOMING, ANALYSIS_CASHFLOW_OUTGOING);

	icons_replace_caret_in_window(analysis_cashflow_window);
}


/**
 * Fill the Cashflow window with values.
 *
 * \param: *find_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void analysis_fill_cashflow_window(struct file_block *file, osbool restore)
{
	if (!restore) {
		/* Set the period icons. */

		*icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATEFROM) = '\0';
		*icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATETO) = '\0';

		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_BUDGET, 0);

		/* Set the grouping icons. */

		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_GROUP, 0);

		icons_strncpy(analysis_cashflow_window, ANALYSIS_CASHFLOW_PERIOD, "1");
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PDAYS, 0);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PMONTHS, 1);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PYEARS, 0);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_LOCK, 0);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_EMPTY, 0);

		/* Set the accounts and format details. */

		*icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_ACCOUNTS) = '\0';
		*icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_INCOMING) = '\0';
		*icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_OUTGOING) = '\0';

		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_TABULAR, 0);
	} else {
		/* Set the period icons. */

		date_convert_to_string(analysis_cashflow_settings.date_from,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATEFROM),
				icons_get_indirected_text_length(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATEFROM));
		date_convert_to_string(analysis_cashflow_settings.date_to,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATETO),
				icons_get_indirected_text_length(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATETO));
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_BUDGET, analysis_cashflow_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_GROUP, analysis_cashflow_settings.group);

		icons_printf(analysis_cashflow_window, ANALYSIS_CASHFLOW_PERIOD, "%d", analysis_cashflow_settings.period);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PDAYS, analysis_cashflow_settings.period_unit == DATE_PERIOD_DAYS);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PMONTHS, analysis_cashflow_settings.period_unit == DATE_PERIOD_MONTHS);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PYEARS, analysis_cashflow_settings.period_unit == DATE_PERIOD_YEARS);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_LOCK, analysis_cashflow_settings.lock);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_EMPTY, analysis_cashflow_settings.empty);

		/* Set the accounts and format details. */

		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_ACCOUNTS),
				analysis_cashflow_settings.accounts, analysis_cashflow_settings.accounts_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_INCOMING),
				analysis_cashflow_settings.incoming, analysis_cashflow_settings.incoming_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_OUTGOING),
				analysis_cashflow_settings.outgoing, analysis_cashflow_settings.outgoing_count);

		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_TABULAR, analysis_cashflow_settings.tabular);
	}

	icons_set_group_shaded_when_on(analysis_cashflow_window, ANALYSIS_CASHFLOW_BUDGET, 4,
			ANALYSIS_CASHFLOW_DATEFROMTXT, ANALYSIS_CASHFLOW_DATEFROM,
			ANALYSIS_CASHFLOW_DATETOTXT, ANALYSIS_CASHFLOW_DATETO);

	icons_set_group_shaded_when_off(analysis_cashflow_window, ANALYSIS_CASHFLOW_GROUP, 7,
			ANALYSIS_CASHFLOW_PERIOD, ANALYSIS_CASHFLOW_PTEXT, ANALYSIS_CASHFLOW_LOCK,
			ANALYSIS_CASHFLOW_PDAYS, ANALYSIS_CASHFLOW_PMONTHS, ANALYSIS_CASHFLOW_PYEARS,
			ANALYSIS_CASHFLOW_EMPTY);
}


/**
 * Process the contents of the Cashflow window, store the details and
 * generate a report from them.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool analysis_process_cashflow_window(void)
{
	/* Read the date settings. */

	analysis_cashflow_file->cashflow_rep->date_from =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATEFROM), NULL_DATE, 0);
	analysis_cashflow_file->cashflow_rep->date_to =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATETO), NULL_DATE, 0);
	analysis_cashflow_file->cashflow_rep->budget = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_BUDGET);

	/* Read the grouping settings. */

	analysis_cashflow_file->cashflow_rep->group = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_GROUP);
	analysis_cashflow_file->cashflow_rep->period = atoi(icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_PERIOD));

	if (icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PDAYS))
		analysis_cashflow_file->cashflow_rep->period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PMONTHS))
		analysis_cashflow_file->cashflow_rep->period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PYEARS))
		analysis_cashflow_file->cashflow_rep->period_unit = DATE_PERIOD_YEARS;
	else
		analysis_cashflow_file->cashflow_rep->period_unit = DATE_PERIOD_MONTHS;

	analysis_cashflow_file->cashflow_rep->lock = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_LOCK);
	analysis_cashflow_file->cashflow_rep->empty = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_EMPTY);

	/* Read the account and heading settings. */

	analysis_cashflow_file->cashflow_rep->accounts_count =
			analysis_account_idents_to_list(analysis_cashflow_file, ACCOUNT_FULL,
			icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_ACCOUNTS),
			analysis_cashflow_file->cashflow_rep->accounts);
	analysis_cashflow_file->cashflow_rep->incoming_count =
			analysis_account_idents_to_list(analysis_cashflow_file, ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_INCOMING),
			analysis_cashflow_file->cashflow_rep->incoming);
	analysis_cashflow_file->cashflow_rep->outgoing_count =
			analysis_account_idents_to_list(analysis_cashflow_file, ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_OUTGOING),
			analysis_cashflow_file->cashflow_rep->outgoing);

	analysis_cashflow_file->cashflow_rep->tabular = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_TABULAR);

	/* Run the report. */

	analysis_generate_cashflow_report(analysis_cashflow_file);

	return TRUE;
}


/**
 * Delete the template used in the current Cashflow window.
 *
 * \return			TRUE if the template was deleted; else FALSE.
 */

static osbool analysis_delete_cashflow_window(void)
{
	if (analysis_cashflow_template >= 0 && analysis_cashflow_template < analysis_cashflow_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_template(analysis_cashflow_file, analysis_cashflow_template);
		analysis_cashflow_template = NULL_TEMPLATE;

		return TRUE;
	} else {
		return FALSE;
	}
}


/**
 * Generate a cashflow report based on the settings in the given file.
 *
 * \param *file			The file to generate the report for.
 */

static void analysis_generate_cashflow_report(struct file_block *file)
{
	int			i, acc, items, found, unit, period, group, lock, tabular, show_blank, total;
	char			line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
	date_t			start_date, end_date, next_start, next_end, date;
	acct_t			from, to;
	amt_t			amount;
	struct report		*report;
	struct analysis_data	*data = NULL;
	struct analysis_report	*template;
	int			entries, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	if (file == NULL)
		return;

	if (!flexutils_allocate((void **) &data, sizeof(struct analysis_data), account_get_count(file))) {
		error_msgs_report_info("NoMemReport");
		return;
	}

	hourglass_on();

	transact_sort_file_data(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date, file->cashflow_rep->date_from, file->cashflow_rep->date_to, file->cashflow_rep->budget);

	/* Read the grouping settings. */

	group = file->cashflow_rep->group;
	unit = file->cashflow_rep->period_unit;
	lock = file->cashflow_rep->lock && (unit == DATE_PERIOD_MONTHS || unit == DATE_PERIOD_YEARS);
	period = (group) ? file->cashflow_rep->period : 0;
	show_blank = file->cashflow_rep->empty;

	/* Read the include list. */

	analysis_clear_account_report_flags(file, data);

	if (file->cashflow_rep->accounts_count == 0 && file->cashflow_rep->incoming_count == 0 &&
			file->cashflow_rep->outgoing_count == 0) {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT, ANALYSIS_REPORT_INCLUDE, &analysis_wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL, ANALYSIS_REPORT_INCLUDE, file->cashflow_rep->accounts, file->cashflow_rep->accounts_count);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_IN, ANALYSIS_REPORT_INCLUDE, file->cashflow_rep->incoming, file->cashflow_rep->incoming_count);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_OUT, ANALYSIS_REPORT_INCLUDE, file->cashflow_rep->outgoing, file->cashflow_rep->outgoing_count);
	}

	tabular = file->cashflow_rep->tabular;

	/* Count the number of accounts and headings to be included.  If this comes to more than the number of tab
	 * stops available (including 2 for account name and total), force the tabular format option off.
	 */

	items = 0;

	for (i = 0; i < account_get_count(file); i++)
		if ((data[i].report_flags & ANALYSIS_REPORT_INCLUDE) != 0)
			items++;

	if ((items + 2) > REPORT_TAB_STOPS)
		tabular = FALSE;

	/* Start to output the report details. */

	analysis_copy_cashflow_template(&(analysis_report_template.data.cashflow), file->cashflow_rep);
	if (analysis_cashflow_template == NULL_TEMPLATE)
		*(analysis_report_template.name) = '\0';
	else
		strcpy(analysis_report_template.name, file->saved_reports[analysis_cashflow_template].name);
	analysis_report_template.type = REPORT_TYPE_CASHFLOW;
	analysis_report_template.file = file;

	template = analysis_create_template(&analysis_report_template);
	if (template == NULL) {
		if (data != NULL)
			flexutils_free((void **) &data);
		hourglass_off();
		error_msgs_report_info("NoMemReport");
		return;
	}

	msgs_lookup("CRWinT", line, sizeof(line));
	report = report_open(file, line, template);

	if (report == NULL) {
		hourglass_off();
		return;
	}

	/* Output report heading */

	file_get_leafname(file, b1, sizeof(b1));
	if (*analysis_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof(line), analysis_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("CRTitle", line, sizeof(line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	date_convert_to_string(start_date, b1, sizeof(b1));
	date_convert_to_string(end_date, b2, sizeof(b2));
	date_convert_to_string(date_today (), b3, sizeof(b3));
	msgs_param_lookup("CRHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	/* Start to output the report. */

	if (tabular) {
		report_write_line(report, 0, "");
		msgs_lookup("CRDate", b1, sizeof(b1));
		sprintf(line, "\\k\\b%s", b1);

		for (acc_group = 0; acc_group < groups; acc_group++) {
			entries = account_get_list_length(file, sequence[acc_group]);

			for (group_line = 0; group_line < entries; group_line++) {
				if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
					if ((data[acc].report_flags & ANALYSIS_REPORT_INCLUDE) != 0) {
						sprintf(b1, "\\t\\r\\b%s", account_get_name(file, acc));
						strcat(line, b1);
					}
				}
			}
		}
		msgs_lookup("CRTotal", b1, sizeof(b1));
		sprintf(b2, "\\t\\r\\b%s", b1);
		strcat(line, b2);

		report_write_line(report, 1, line);
	}

	analysis_initialise_date_period(start_date, end_date, period, unit, lock);

	while (analysis_get_next_date_period(&next_start, &next_end, date_text, sizeof(date_text))) {
		/* Zero the heading totals for the report. */

		for (i = 0; i < account_get_count(file); i++)
			data[i].report_total = 0;

		/* Scan through the transactions, adding the values up for those in range and outputting them to the screen. */

		found = 0;

		for (i = 0; i < transact_get_count(file); i++) {
			date = transact_get_date(file, i);

			if ((next_start == NULL_DATE || date >= next_start) && (next_end == NULL_DATE || date <= next_end)) {
				from = transact_get_from(file, i);
				to = transact_get_to(file, i);
				amount = transact_get_amount(file, i);

				if (from != NULL_ACCOUNT)
					data[from].report_total -= amount;

				if (to != NULL_ACCOUNT)
					data[to].report_total += amount;

				found++;
			}
		}

		/* Print the transaction summaries. */

		if (found > 0 || show_blank) {
			if (tabular) {
				sprintf(line, "\\k%s", date_text);

				total = 0;

				for (acc_group = 0; acc_group < groups; acc_group++) {
					entries = account_get_list_length(file, sequence[acc_group]);

					for (group_line = 0; group_line < entries; group_line++) {
						if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
							if ((data[acc].report_flags & ANALYSIS_REPORT_INCLUDE) != 0) {
								total += data[acc].report_total;
								currency_flexible_convert_to_string(data[acc].report_total, b1, sizeof(b1), TRUE);
								sprintf(b2, "\\t\\d\\r%s", b1);
								strcat(line, b2);
							}
						}
					}
				}

				currency_flexible_convert_to_string(total, b1, sizeof(b1), TRUE);
				sprintf(b2, "\\t\\d\\r%s", b1);
				strcat(line, b2);
				report_write_line(report, 1, line);
			} else {
				report_write_line(report, 0, "");
				if (group) {
					sprintf(line, "\\u%s", date_text);
					report_write_line(report, 0, line);
				}

				total = 0;

				for (acc_group = 0; acc_group < groups; acc_group++) {
					entries = account_get_list_length(file, sequence[acc_group]);

					for (group_line = 0; group_line < entries; group_line++) {
						if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
							if (data[acc].report_total != 0 && (data[acc].report_flags & ANALYSIS_REPORT_INCLUDE) != 0) {
								total += data[acc].report_total;
								currency_flexible_convert_to_string(data[acc].report_total, b1, sizeof(b1), TRUE);
								sprintf(line, "\\i%s\\t\\d\\r%s", account_get_name(file, acc), b1);
								report_write_line(report, 2, line);
							}
						}
					}
				}
				msgs_lookup("CRTotal", b1, sizeof(b1));
				currency_flexible_convert_to_string(total, b2, sizeof(b2), TRUE);
				sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
				report_write_line(report, 2, line);
			}
		}
	}

	report_close(report);

	if (data != NULL)
		flexutils_free((void **) &data);

	hourglass_off();
}


/**
 * Construct new balance report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct balance_rep *analysis_create_balance(void)
{
	struct balance_rep	*new;

	new = heap_alloc(sizeof(struct balance_rep));
	if (new == NULL)
		return NULL;

	new->date_from = NULL_DATE;
	new->date_to = NULL_DATE;
	new->budget = 0;
	new->group = 0;
	new->period = 1;
	new->period_unit = DATE_PERIOD_MONTHS;
	new->lock = 0;
	new->accounts_count = 0;
	new->incoming_count = 0;
	new->outgoing_count = 0;
	new->tabular = 0;

	return new;
}


/**
 * Delete a balance report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_delete_balance(struct balance_rep *report)
{
	if (report != NULL)
		heap_free(report);
}


/**
 * Open the Balance Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_balance_window(struct file_block *file, wimp_pointer *ptr, int template, osbool restore)
{
	osbool		template_mode;

	/* If the window is already open, another balance report is being edited.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_balance_window))
		wimp_close_window(analysis_balance_window);

	/* Copy the settings block contents into a static place that won't shift about on the flex heap
	 * while the dialogue is open.
	 */

	template_mode = (template >= 0 && template < file->saved_report_count);

	if (template_mode) {
		analysis_copy_balance_template(&(analysis_balance_settings), &(file->saved_reports[template].data.balance));
		analysis_balance_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_balance_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_balance_template (&(analysis_balance_settings), file->balance_rep);
		analysis_balance_template = NULL_TEMPLATE;

		msgs_lookup("BalRepTitle", windows_get_indirected_title_addr(analysis_balance_window), 40);
	}

	icons_set_deleted(analysis_balance_window, ANALYSIS_BALANCE_DELETE, !template_mode);
	icons_set_deleted(analysis_balance_window, ANALYSIS_BALANCE_RENAME, !template_mode);

	/* Set the window contents up. */

	analysis_fill_balance_window(file, restore);

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_balance_file = file;
	analysis_balance_restore = restore;

	windows_open_centred_at_pointer(analysis_balance_window, ptr);
	place_dialogue_caret_fallback(analysis_balance_window, 4, ANALYSIS_BALANCE_DATEFROM, ANALYSIS_BALANCE_DATETO,
			ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_ACCOUNTS);
}


/**
 * Process mouse clicks in the Analysis Balance dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_balance_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ANALYSIS_BALANCE_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_balance_window);
			analysis_force_close_report_rename_window(analysis_balance_window);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_balance_window();
		}
		break;

	case ANALYSIS_BALANCE_OK:
		if (analysis_process_balance_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_balance_window);
			analysis_force_close_report_rename_window(analysis_balance_window);
		}
		break;

	case ANALYSIS_BALANCE_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_balance_window())
			close_dialogue_with_caret(analysis_balance_window);
		break;

	case ANALYSIS_BALANCE_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_balance_template >= 0 && analysis_balance_template < analysis_balance_file->saved_report_count)
			analysis_open_rename_window(analysis_balance_file, analysis_balance_template, pointer);
		break;

	case ANALYSIS_BALANCE_BUDGET:
		icons_set_group_shaded_when_on(analysis_balance_window, ANALYSIS_BALANCE_BUDGET, 4,
				ANALYSIS_BALANCE_DATEFROMTXT, ANALYSIS_BALANCE_DATEFROM,
				ANALYSIS_BALANCE_DATETOTXT, ANALYSIS_BALANCE_DATETO);
		icons_replace_caret_in_window(analysis_balance_window);
		break;

	case ANALYSIS_BALANCE_GROUP:
		icons_set_group_shaded_when_off(analysis_balance_window, ANALYSIS_BALANCE_GROUP, 6,
				ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_PTEXT, ANALYSIS_BALANCE_LOCK,
				ANALYSIS_BALANCE_PDAYS, ANALYSIS_BALANCE_PMONTHS, ANALYSIS_BALANCE_PYEARS);

		icons_replace_caret_in_window(analysis_balance_window);
		break;

	case ANALYSIS_BALANCE_ACCOUNTSPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		break;

	case ANALYSIS_BALANCE_INCOMINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		break;

	case ANALYSIS_BALANCE_OUTGOINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_open_account_lookup(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_OUTGOING, NULL_ACCOUNT, ACCOUNT_OUT);
		break;
	}
}


/**
 * Process keypresses in the Analysis Balance window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool analysis_balance_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_process_balance_window()) {
			close_dialogue_with_caret(analysis_balance_window);
			analysis_force_close_report_rename_window(analysis_balance_window);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_balance_window);
		analysis_force_close_report_rename_window(analysis_balance_window);
		break;

	case wimp_KEY_F1:
		if (key->i == ANALYSIS_BALANCE_ACCOUNTS)
			analysis_open_account_lookup(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		else if (key->i == ANALYSIS_BALANCE_INCOMING)
			analysis_open_account_lookup(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		else if (key->i == ANALYSIS_BALANCE_OUTGOING)
			analysis_open_account_lookup(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_OUTGOING, NULL_ACCOUNT, ACCOUNT_OUT);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Balance window.
 */

static void analysis_refresh_balance_window(void)
{
	analysis_fill_balance_window(analysis_balance_file, analysis_balance_restore);
	icons_redraw_group (analysis_balance_window, 6,
			ANALYSIS_BALANCE_DATEFROM, ANALYSIS_BALANCE_DATETO, ANALYSIS_BALANCE_PERIOD,
			ANALYSIS_BALANCE_ACCOUNTS, ANALYSIS_BALANCE_INCOMING, ANALYSIS_BALANCE_OUTGOING);

	icons_replace_caret_in_window(analysis_balance_window);
}


/**
 * Fill the Balance window with values.
 *
 * \param: *find_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void analysis_fill_balance_window(struct file_block *file, osbool restore)
{
	if (!restore) {
		/* Set the period icons. */

		*icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATEFROM) = '\0';
		*icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATETO) = '\0';

		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_BUDGET, 0);

		/* Set the grouping icons. */

		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_GROUP, 0);

		icons_strncpy(analysis_balance_window, ANALYSIS_BALANCE_PERIOD, "1");
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PDAYS, 0);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS, 1);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PYEARS, 0);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_LOCK, 0);

		/* Set the accounts and format details. */

		*icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_ACCOUNTS) = '\0';
		*icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_INCOMING) = '\0';
		*icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_OUTGOING) = '\0';

		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_TABULAR, 0);
	} else {
		/* Set the period icons. */

		date_convert_to_string(analysis_balance_settings.date_from,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATEFROM),
				icons_get_indirected_text_length(analysis_balance_window, ANALYSIS_BALANCE_DATEFROM));
		date_convert_to_string(analysis_balance_settings.date_to,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATETO),
				icons_get_indirected_text_length(analysis_balance_window, ANALYSIS_BALANCE_DATETO));
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_BUDGET, analysis_balance_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_GROUP, analysis_balance_settings.group);

		icons_printf(analysis_balance_window, ANALYSIS_BALANCE_PERIOD, "%d", analysis_balance_settings.period);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PDAYS, analysis_balance_settings.period_unit == DATE_PERIOD_DAYS);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS,
				analysis_balance_settings.period_unit == DATE_PERIOD_MONTHS);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PYEARS, analysis_balance_settings.period_unit == DATE_PERIOD_YEARS);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_LOCK, analysis_balance_settings.lock);

		/* Set the accounts and format details. */

		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_ACCOUNTS),
				analysis_balance_settings.accounts, analysis_balance_settings.accounts_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_INCOMING),
				analysis_balance_settings.incoming, analysis_balance_settings.incoming_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_OUTGOING),
				analysis_balance_settings.outgoing, analysis_balance_settings.outgoing_count);

		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_TABULAR, analysis_balance_settings.tabular);
	}

	icons_set_group_shaded_when_on (analysis_balance_window, ANALYSIS_BALANCE_BUDGET, 4,
			ANALYSIS_BALANCE_DATEFROMTXT, ANALYSIS_BALANCE_DATEFROM,
			ANALYSIS_BALANCE_DATETOTXT, ANALYSIS_BALANCE_DATETO);

	icons_set_group_shaded_when_off (analysis_balance_window, ANALYSIS_BALANCE_GROUP, 6,
			ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_PTEXT, ANALYSIS_BALANCE_LOCK,
			ANALYSIS_BALANCE_PDAYS, ANALYSIS_BALANCE_PMONTHS, ANALYSIS_BALANCE_PYEARS);
}


/**
 * Process the contents of the Balance window, store the details and
 * generate a report from them.
 *
 * \return			TRUE if the operation completed OK; FALSE if there
 *				was an error.
 */

static osbool analysis_process_balance_window(void)
{
	/* Read the date settings. */

	analysis_balance_file->balance_rep->date_from =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATEFROM), NULL_DATE, 0);
	analysis_balance_file->balance_rep->date_to =
			date_convert_from_string(icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATETO), NULL_DATE, 0);
	analysis_balance_file->balance_rep->budget = icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_BUDGET);

	/* Read the grouping settings. */

	analysis_balance_file->balance_rep->group = icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_GROUP);
	analysis_balance_file->balance_rep->period = atoi(icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_PERIOD));

	if (icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_PDAYS))
		analysis_balance_file->balance_rep->period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS))
		analysis_balance_file->balance_rep->period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_PYEARS))
 		 analysis_balance_file->balance_rep->period_unit = DATE_PERIOD_YEARS;
	else
		analysis_balance_file->balance_rep->period_unit = DATE_PERIOD_MONTHS;

	analysis_balance_file->balance_rep->lock = icons_get_selected (analysis_balance_window, ANALYSIS_BALANCE_LOCK);

	/* Read the account and heading settings. */

	analysis_balance_file->balance_rep->accounts_count =
			analysis_account_idents_to_list(analysis_balance_file, ACCOUNT_FULL,
			icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_ACCOUNTS),
			analysis_balance_file->balance_rep->accounts);
	analysis_balance_file->balance_rep->incoming_count =
			analysis_account_idents_to_list(analysis_balance_file, ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_INCOMING),
			analysis_balance_file->balance_rep->incoming);
	analysis_balance_file->balance_rep->outgoing_count =
			analysis_account_idents_to_list(analysis_balance_file, ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_OUTGOING),
			analysis_balance_file->balance_rep->outgoing);

	analysis_balance_file->balance_rep->tabular = icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_TABULAR);

	/* Run the report. */

	analysis_generate_balance_report(analysis_balance_file);

	return TRUE;
}


/**
 * Delete the template used in the current Balance window.
 *
 * \return			TRUE if the template was deleted; else FALSE.
 */

static osbool analysis_delete_balance_window(void)
{
	if (analysis_balance_template >= 0 && analysis_balance_template < analysis_balance_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_template(analysis_balance_file, analysis_balance_template);
		analysis_balance_template = NULL_TEMPLATE;

		return TRUE;
	} else {
		return FALSE;
	}
}


/**
 * Generate a balance report based on the settings in the given file.
 *
 * \param *file			The file to generate the report for.
 */

static void analysis_generate_balance_report(struct file_block *file)
{
	int			i, acc, items, unit, period, group, lock, tabular, total;
	char			line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
	date_t			start_date, end_date, next_start, next_end;
	acct_t			from, to;
	amt_t			amount;
	struct report		*report;
	struct analysis_data	*data = NULL;
	struct analysis_report	*template;
	int			entries, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	if (file == NULL)
		return;

	if (!flexutils_allocate((void **) &data, sizeof(struct analysis_data), account_get_count(file))) {
		error_msgs_report_info("NoMemReport");
		return;
	}

	hourglass_on();

	transact_sort_file_data(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date, file->balance_rep->date_from, file->balance_rep->date_to, file->balance_rep->budget);

	/* Read the grouping settings. */

	group = file->balance_rep->group;
	unit = file->balance_rep->period_unit;
	lock = file->balance_rep->lock && (unit == DATE_PERIOD_MONTHS || unit == DATE_PERIOD_YEARS);
	period = (group) ? file->balance_rep->period : 0;

	/* Read the include list. */

	analysis_clear_account_report_flags(file, data);

	if (file->balance_rep->accounts_count == 0 && file->balance_rep->incoming_count == 0 && file->balance_rep->outgoing_count == 0) {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT, ANALYSIS_REPORT_INCLUDE, &analysis_wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_FULL, ANALYSIS_REPORT_INCLUDE, file->balance_rep->accounts, file->balance_rep->accounts_count);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_IN, ANALYSIS_REPORT_INCLUDE, file->balance_rep->incoming, file->balance_rep->incoming_count);
		analysis_set_account_report_flags_from_list(file, data, ACCOUNT_OUT, ANALYSIS_REPORT_INCLUDE, file->balance_rep->outgoing, file->balance_rep->outgoing_count);
	}

	tabular = file->balance_rep->tabular;

	/* Count the number of accounts and headings to be included.  If this comes to more than the number of tab
	 * stops available (including 2 for account name and total), force the tabular format option off.
	 */

	items = 0;

	for (i = 0; i < account_get_count(file); i++)
		if ((data[i].report_flags & ANALYSIS_REPORT_INCLUDE) != 0)
			items++;

	if ((items + 2) > REPORT_TAB_STOPS)
		tabular = FALSE;

	/* Start to output the report details. */

	analysis_copy_balance_template(&(analysis_report_template.data.balance), file->balance_rep);
	if (analysis_balance_template == NULL_TEMPLATE)
		*(analysis_report_template.name) = '\0';
	else
		strcpy(analysis_report_template.name, file->saved_reports[analysis_balance_template].name);
	analysis_report_template.type = REPORT_TYPE_BALANCE;
	analysis_report_template.file = file;

	template = analysis_create_template(&analysis_report_template);
	if (template == NULL) {
		if (data != NULL)
			flexutils_free((void **) &data);
		hourglass_off();
		error_msgs_report_info("NoMemReport");
		return;
	}

	msgs_lookup("BRWinT", line, sizeof(line));
	report = report_open(file, line, template);

	if (report == NULL) {
		hourglass_off();
		return;
	}

	/* Output report heading */

	file_get_leafname(file, b1, sizeof(b1));
	if (*analysis_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof(line), analysis_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("BRTitle", line, sizeof(line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	date_convert_to_string(start_date, b1, sizeof(b1));
	date_convert_to_string(end_date, b2, sizeof(b2));
	date_convert_to_string(date_today(), b3, sizeof(b3));
	msgs_param_lookup("BRHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	/* Start to output the report. */

	if (tabular) {
		report_write_line(report, 0, "");
		msgs_lookup("BRDate", b1, sizeof(b1));
		sprintf(line, "\\k\\b%s", b1);

		for (acc_group = 0; acc_group < groups; acc_group++) {
			entries = account_get_list_length(file, sequence[acc_group]);

			for (group_line = 0; group_line < entries; group_line++) {
				if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
					if ((data[acc].report_flags & ANALYSIS_REPORT_INCLUDE) != 0) {
						sprintf(b1, "\\t\\r\\b%s", account_get_name(file, acc));
						strcat(line, b1);
					}
				}
			}
		}
		msgs_lookup("BRTotal", b1, sizeof(b1));
		sprintf(b2, "\\t\\r\\b%s", b1);
		strcat(line, b2);

		report_write_line(report, 1, line);
	}

	analysis_initialise_date_period(start_date, end_date, period, unit, lock);

	while (analysis_get_next_date_period(&next_start, &next_end, date_text, sizeof(date_text))) {
		/* Zero the heading totals for the report. */

		for (i = 0; i < account_get_count(file); i++)
			data[i].report_total = account_get_opening_balance(file, i);

		/* Scan through the transactions, adding the values up for those occurring before the end of the current
		 * period and outputting them to the screen.
		 */

		for (i = 0; i < transact_get_count(file); i++) {
			if (next_end == NULL_DATE || transact_get_date(file, i) <= next_end) {
				from = transact_get_from(file, i);
				to = transact_get_to(file, i);
				amount = transact_get_amount(file, i);
			
				if (from != NULL_ACCOUNT)
					data[from].report_total -= amount;

				if (to != NULL_ACCOUNT)
					data[to].report_total += amount;
			}
		}

		/* Print the transaction summaries. */

		if (tabular) {
			sprintf(line, "\\k%s", date_text);

			total = 0;


			for (acc_group = 0; acc_group < groups; acc_group++) {
				entries = account_get_list_length(file, sequence[acc_group]);

				for (group_line = 0; group_line < entries; group_line++) {
					if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
						if ((data[acc].report_flags & ANALYSIS_REPORT_INCLUDE) != 0) {
							total += data[acc].report_total;
							currency_flexible_convert_to_string(data[acc].report_total, b1, sizeof(b1), TRUE);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
						}
					}
				}
			}
			currency_flexible_convert_to_string(total, b1, sizeof(b1), TRUE);
			sprintf(b2, "\\t\\d\\r%s", b1);
			strcat(line, b2);
			report_write_line(report, 1, line);
		} else {
			report_write_line(report, 0, "");
			if (group) {
				sprintf(line, "\\u%s", date_text);
				report_write_line(report, 0, line);
			}

			total = 0;

			for (acc_group = 0; acc_group < groups; acc_group++) {
				entries = account_get_list_length(file, sequence[acc_group]);

				for (group_line = 0; group_line < entries; group_line++) {
					if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
						if (data[acc].report_total != 0 && (data[acc].report_flags & ANALYSIS_REPORT_INCLUDE) != 0) {
							total += data[acc].report_total;
							currency_flexible_convert_to_string(data[acc].report_total, b1, sizeof(b1), TRUE);
							sprintf(line, "\\i%s\\t\\d\\r%s", account_get_name(file, acc), b1);
							report_write_line(report, 2, line);
						}
					}
				}
			}
			msgs_lookup("BRTotal", b1, sizeof(b1));
			currency_flexible_convert_to_string(total, b2, sizeof(b2), TRUE);
			sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
			report_write_line(report, 2, line);
		}
	}

	report_close(report);

	if (data != NULL)
		flexutils_free((void **) &data);

	hourglass_off();
}


/**
 * Open the account lookup window as a menu, allowing an account to be
 * entered into an account list using a graphical interface.
 *
 * \param *file			The file to which the operation relates.
 * \param window		The window to own the lookup dialogue.
 * \param icon			The icon to own the lookup dialogue.
 * \param account		An account to seed the window, or NULL_ACCOUNT.
 * \param type			The types of account to be accepted.
 */

static void analysis_open_account_lookup(struct file_block *file, wimp_w window, wimp_i icon, int account, enum account_type type)
{
	wimp_pointer		pointer;

	account_fill_field(file, account, FALSE, analysis_lookup_window, ANALYSIS_LOOKUP_IDENT, ANALYSIS_LOOKUP_NAME, ANALYSIS_LOOKUP_REC);

	analysis_lookup_file = file;
	analysis_lookup_type = type;
	analysis_lookup_parent = window;
	analysis_lookup_icon = icon;

	/* Set the window position and open it on screen. */

	pointer.w = window;
	pointer.i = icon;
	menus_create_popup_menu((wimp_menu *) analysis_lookup_window, &pointer);
}


/**
 * Process mouse clicks in the Account Lookup dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_lookup_click_handler(wimp_pointer *pointer)
{
	enum account_menu_type	type;
	wimp_window_state	window_state;

	switch (pointer->i) {
	case ANALYSIS_LOOKUP_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			wimp_create_menu((wimp_menu *) -1, 0, 0);
		break;

	case ANALYSIS_LOOKUP_OK:
		if (analysis_process_lookup_window() && pointer->buttons == wimp_CLICK_SELECT)
			wimp_create_menu((wimp_menu *) -1, 0, 0);
		break;

	case ANALYSIS_LOOKUP_NAME:
		if (pointer->buttons != wimp_CLICK_ADJUST)
			break;

		/* Change the lookup window from a menu to a static window, so
		 * that the lookup menu can be created.
		 */

		window_state.w = analysis_lookup_window;
		wimp_get_window_state(&window_state);
		wimp_create_menu((wimp_menu *) -1, 0, 0);
		wimp_open_window((wimp_open *) &window_state);

		switch (analysis_lookup_type) {
		case ACCOUNT_FULL | ACCOUNT_IN:
			type = ACCOUNT_MENU_FROM;
			break;

		case ACCOUNT_FULL | ACCOUNT_OUT:
			type = ACCOUNT_MENU_TO;
			break;

		case ACCOUNT_FULL:
			type = ACCOUNT_MENU_ACCOUNTS;
			break;

		case ACCOUNT_IN:
			type = ACCOUNT_MENU_INCOMING;
			break;

		case ACCOUNT_OUT:
			type = ACCOUNT_MENU_OUTGOING;
			break;

		default:
			type = ACCOUNT_MENU_FROM;
			break;
		}

		account_menu_open_icon(analysis_lookup_file, type, analysis_lookup_menu_closed,
				analysis_lookup_window, ANALYSIS_LOOKUP_IDENT, ANALYSIS_LOOKUP_NAME, ANALYSIS_LOOKUP_REC, pointer);
		break;

	case ANALYSIS_LOOKUP_REC:
		if (pointer->buttons == wimp_CLICK_ADJUST)
			account_toggle_reconcile_icon(analysis_lookup_window, ANALYSIS_LOOKUP_REC);
		break;
	}
}


/**
 * Process keypresses in the Account Lookup window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool analysis_lookup_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_process_lookup_window())
			wimp_create_menu((wimp_menu *) -1, 0, 0);
		break;

	default:
		if (key->i != ANALYSIS_LOOKUP_IDENT)
			return FALSE;

		if (key->i == ANALYSIS_LOOKUP_IDENT)
			account_lookup_field(analysis_lookup_file, key->c, analysis_lookup_type, NULL_ACCOUNT, NULL,
					analysis_lookup_window, ANALYSIS_LOOKUP_IDENT, ANALYSIS_LOOKUP_NAME, ANALYSIS_LOOKUP_REC);
 		break;
	}

	return TRUE;
}


/**
 * This function is called whenever the account list menu closes.  If the enter
 * account window is open, it is converted back into a transient menu.
 */

static void analysis_lookup_menu_closed(void)
{
	wimp_window_state	window_state;

	if (!windows_get_open(analysis_lookup_window))
		return;

	window_state.w = analysis_lookup_window;
	wimp_get_window_state (&window_state);
	wimp_close_window(analysis_lookup_window);

	if (!windows_get_open(analysis_lookup_parent))
		return;

	wimp_create_menu((wimp_menu *) -1, 0, 0);
	wimp_create_menu((wimp_menu *) analysis_lookup_window, window_state.visible.x0, window_state.visible.y1);
}


/**
 * Take the account from the account lookup window, and put the ident into the
 * parent icon.
 *
 * \return			TRUE if the content was processed; FALSE otherwise.
 */

static osbool analysis_process_lookup_window(void)
{
	int		index, max_len;
	acct_t		account;
	char		ident[ACCOUNT_IDENT_LEN+1], *icon;
	wimp_caret	caret;

	/* Get the account number that was entered. */

	account = account_find_by_ident(analysis_lookup_file, icons_get_indirected_text_addr(analysis_lookup_window, ANALYSIS_LOOKUP_IDENT),
			analysis_lookup_type);

	if (account == NULL_ACCOUNT)
		return TRUE;

	/* Get the icon text, and the length of it. */

	icon = icons_get_indirected_text_addr(analysis_lookup_parent, analysis_lookup_icon);
	max_len = string_ctrl_strlen(icon);

	/* Check the caret position.  If it is in the target icon, move the insertion until it falls before a comma;
	 * if not, place the index at the end of the text.
	 */

	wimp_get_caret_position(&caret);
	if (caret.w == analysis_lookup_parent && caret.i == analysis_lookup_icon) {
		index = caret.index;
		while (index < max_len && icon[index] != ',')
			index++;
	} else {
		index = max_len;
	}

	/* If the icon text is empty, the ident is inserted on its own.
	 *
	 * If there is text there, a comma is placed at the start or end depending on where the index falls in the
	 * string.  If it falls anywhere but the end, the index is assumed to be after a comma such that an extra
	 * comma is added after the ident to be inserted.
	 */

	if (*icon == '\0') {
		sprintf(ident, "%s", account_get_ident(analysis_lookup_file, account));
	} else {
		if (index < max_len)
			sprintf(ident, "%s,", account_get_ident(analysis_lookup_file, account));
		else
			sprintf(ident, ",%s", account_get_ident(analysis_lookup_file, account));
	}

	icons_insert_text(analysis_lookup_parent, analysis_lookup_icon, index, ident, strlen(ident));
	icons_replace_caret_in_window(analysis_lookup_parent);

	return TRUE;
}


/**
 * Establish and return the range of dates to report over, based on the values
 * in a dialogue box and the data in the file concerned.
 *
 * \param *file			The file to run the report on.
 * \param *start_date		Return the date to start the report from.
 * \param *end_date		Return the date to end the report at.
 * \param date1			The start date entered in the dialogue, or NULL_DATE.
 * \param date2			The end date entered in the dialogue, or NULL_DATE.
 * \param budget		TRUE to report on the budget period; else FALSE.
 */

static void analysis_find_date_range(struct file_block *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget)
{
	int		i, transactions;
	osbool		find_start, find_end;
	date_t		date;

	if (budget) {
		/* Get the start and end dates from the budget settings. */

		budget_get_dates(file, start_date, end_date);
	} else {
		/* Get the start and end dates from the icon text. */

		*start_date = date1;
		*end_date = date2;
	}

	find_start = (*start_date == NULL_DATE);
	find_end = (*end_date == NULL_DATE);

	/* If either of the dates wasn't specified, we need to find the earliest and latest dates in the file. */

	if (find_start || find_end) {
		transactions = transact_get_count(file);

		if (find_start)
			*start_date = (transactions > 0) ? transact_get_date(file, 0) : NULL_DATE;

		if (find_end)
			*end_date = (transactions > 0) ? transact_get_date(file, transactions - 1) : NULL_DATE;

		for (i = 0; i < transactions; i++) {
			date = transact_get_date(file, i);

			if (find_start && date != NULL_DATE && date < *start_date)
				*start_date = date;

			if (find_end && date != NULL_DATE && date > *end_date)
				*end_date = date;
		}
	}

	if (*start_date == NULL_DATE)
		*start_date = DATE_MIN;

	if (*end_date == NULL_DATE)
		*end_date = DATE_MAX;
}


/**
 * Initialise the date period iteration.  Set the state machine so that
 * analysis_get_next_date_period() can be called to work through the report.
 *
 * \param start			The start date for the report period.
 * \param end			The end date for the report period.
 * \param period		The time period into which to divide the report.
 * \param unit			The unit of the divisor period.
 * \param lock			TRUE to apply calendar lock; otherwise FALSE.
 */

static void analysis_initialise_date_period(date_t start, date_t end, int period, enum date_period unit, osbool lock)
{
	analysis_period_start = start;
	analysis_period_end = end;
	analysis_period_length = period;
	analysis_period_unit = unit;
	analysis_period_lock = lock;
	analysis_period_first = lock;
}


/**
 * Return the next date period from the sequence set up with
 * analysis_initialise_date_period(), for use by the report modules.
 *
 * \param *next_start		Return the next period's start date.
 * \param *next_end		Return the next period's end date.
 * \param *date_text		Pointer to a buffer to hold a textual name for the period.
 * \param date_len		The number of bytes in the name buffer.
 * \return			TRUE if a period was returned; FALSE if none left.
 */

static osbool analysis_get_next_date_period(date_t *next_start, date_t *next_end, char *date_text, size_t date_len)
{
	char		b1[1024], b2[1024];

	if (analysis_period_start > analysis_period_end)
		return FALSE;

	if (analysis_period_length > 0) {
		/* If the report is to be grouped, find the next_end date which falls at the end of the period.
		 *
		 * If first_lock is set, the report is locked to the calendar and this is the first iteration.  Therefore,
		 * the end date is found by adding the period-1 to the current date, then setting the DAYS or DAYS+MONTHS to
		 * maximum in the result.  This means that the first period will be no more than the specified period.
		 * The resulting date will later be fixed into a valid date, before it is used in anger.
		 *
		 * If first_lock is not set, the next_end is found by adding the group period to the start date and subtracting
		 * 1 from it.  By this point, locked reports will be period aligned anyway, so this should work OK.
		 */

		if (analysis_period_first) {
			*next_end = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length - 1);

			switch (analysis_period_unit) {
			case DATE_PERIOD_MONTHS:
				*next_end = (*next_end & 0xffffff00) | 0x001f; /* Maximise the days, so end of month. */
				break;

			case DATE_PERIOD_YEARS:
				*next_end = (*next_end & 0xffff0000) | 0x0c1f; /* Maximise the days and months, so end of year. */
				break;

			default:
				*next_end = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length) - 1;
				break;
			}
		} else {
			*next_end = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length) - 1;
		}

		/* Pull back into range isf we fall off the end. */

		if (*next_end > analysis_period_end)
			*next_end = analysis_period_end;
	} else {
		/* If the report is not to be grouped, the next_end date is just the end of the report period. */

		*next_end = analysis_period_end;
	}

	/* Get the real start and end dates for the period. */

	*next_start = date_find_valid_day(analysis_period_start, DATE_ADJUST_BACKWARD);
	*next_end = date_find_valid_day(*next_end, DATE_ADJUST_FORWARD);

	if (analysis_period_length > 0) {
		/* If the report is grouped, find the next start date by adding the period on to the current start date. */

		analysis_period_start = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length);

		if (analysis_period_first) {
			/* If the report is calendar locked, and this is the first iteration, reset the DAYS or DAYS+MONTHS
			 * to one so that the start date will be locked on to the calendar from now on.
			 */

			switch (analysis_period_unit) {
			case DATE_PERIOD_MONTHS:
				analysis_period_start = (analysis_period_start & 0xffffff00) | 0x0001; /* Set the days to one. */
				break;

			case DATE_PERIOD_YEARS:
				analysis_period_start = (analysis_period_start & 0xffff0000) | 0x0101; /* Set the days and months to one. */
				break;

			default:
				break;
			}

			analysis_period_first = FALSE;
		}
	} else {
		analysis_period_start = analysis_period_end+1;
	}

	/* Generate a date period title for the report section.
	 *
	 * If calendar locked, this will be of the form "June 2003", or "1998"; otherwise it will be of the form
	 * "<start date> - <end date>".
	 */

	*date_text = '\0';

	if (analysis_period_lock) {
		switch (analysis_period_unit) {
		case DATE_PERIOD_MONTHS:
			date_convert_to_month_string(*next_start, b1, sizeof(b1));

			if ((*next_start & 0xffffff00) == (*next_end & 0xffffff00)) {
				msgs_param_lookup("PRMonth", date_text, date_len, b1, NULL, NULL, NULL);
			} else {
				date_convert_to_month_string(*next_end, b2, sizeof(b2));
				msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
			}
			break;

		case DATE_PERIOD_YEARS:
			date_convert_to_year_string(*next_start, b1, sizeof(b1));

			if ((*next_start & 0xffff0000) == (*next_end & 0xffff0000)) {
				msgs_param_lookup("PRYear", date_text, date_len, b1, NULL, NULL, NULL);
			} else {
				date_convert_to_year_string(*next_end, b2, sizeof(b2));
				msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
			}
			break;

		default:
			break;
		}
	} else {
		if (*next_start == *next_end) {
			date_convert_to_string(*next_start, b1, sizeof(b1));
			msgs_param_lookup("PRDay", date_text, date_len, b1, NULL, NULL, NULL);
		} else {
			date_convert_to_string(*next_start, b1, sizeof(b1));
			date_convert_to_string(*next_end, b2, sizeof(b2));
			msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
		}
	}

	return TRUE;
}


/**
 * Remove an account from all of the report templates in a file (pending
 * deletion).
 *
 * \param *file			The file to process.
 * \param account		The account to remove.
 */

void analysis_remove_account_from_templates(struct file_block *file, acct_t account)
{
	int		i;

	/* Handle the dialogue boxes. */

	analysis_remove_account_from_list(account, file->trans_rep->from, &(file->trans_rep->from_count));
	analysis_remove_account_from_list(account, file->trans_rep->to, &(file->trans_rep->to_count));

	analysis_remove_account_from_list(account, file->unrec_rep->from, &(file->unrec_rep->from_count));
	analysis_remove_account_from_list(account, file->unrec_rep->to, &(file->unrec_rep->to_count));

	analysis_remove_account_from_list(account, file->cashflow_rep->accounts, &(file->cashflow_rep->accounts_count));
	analysis_remove_account_from_list(account, file->cashflow_rep->incoming, &(file->cashflow_rep->incoming_count));
	analysis_remove_account_from_list(account, file->cashflow_rep->outgoing, &(file->cashflow_rep->outgoing_count));

	analysis_remove_account_from_list(account, file->balance_rep->accounts, &(file->balance_rep->accounts_count));
	analysis_remove_account_from_list(account, file->balance_rep->incoming, &(file->balance_rep->incoming_count));
	analysis_remove_account_from_list(account, file->balance_rep->outgoing, &(file->balance_rep->outgoing_count));

	/* Now process any saved templates. */

	for (i=0; i<file->saved_report_count; i++) {
		analysis_remove_account_from_template(&(file->saved_reports[i]), account);
	}

	/* Finally, work through any reports in the file. */

	report_process_all_templates(file, analysis_remove_account_from_report_template, &account);
}


/**
 * Callback function to accept analysis templates and account numbers from
 * the report system, when removing an account from all open report templates.
 *
 * \param *template		The template to be processed.
 * \param *data			Our supplied data (pointer to the number of
 *				the account to be removed).
 */

static void analysis_remove_account_from_report_template(struct analysis_report *template, void *data)
{
	acct_t	*account = data;

	if (template == NULL || account == NULL)
		return;

	analysis_remove_account_from_template(template, *account);
}

/**
 * Remove any references to an account from an analysis template.
 *
 * \param *template		The template to process.
 * \param account		The account to be removed.
 */

static void analysis_remove_account_from_template(struct analysis_report *template, acct_t account)
{
	if (template == NULL)
		return;

	switch (template->type) {
	case REPORT_TYPE_TRANS:
		analysis_remove_account_from_list(account, template->data.transaction.from,
				&(template->data.transaction.from_count));
		analysis_remove_account_from_list(account, template->data.transaction.to,
				&(template->data.transaction.to_count));
		break;

	case REPORT_TYPE_UNREC:
		analysis_remove_account_from_list(account, template->data.unreconciled.from,
				&(template->data.unreconciled.from_count));
		analysis_remove_account_from_list(account, template->data.unreconciled.to,
				&(template->data.unreconciled.to_count));
		break;

	case REPORT_TYPE_CASHFLOW:
		analysis_remove_account_from_list(account, template->data.cashflow.accounts,
				&(template->data.cashflow.accounts_count));
		analysis_remove_account_from_list(account, template->data.cashflow.incoming,
				&(template->data.cashflow.incoming_count));
		analysis_remove_account_from_list(account, template->data.cashflow.outgoing,
				&(template->data.cashflow.outgoing_count));
		break;

	case REPORT_TYPE_BALANCE:
		analysis_remove_account_from_list(account, template->data.balance.accounts,
				&(template->data.balance.accounts_count));
		analysis_remove_account_from_list(account, template->data.balance.incoming,
				&(template->data.balance.incoming_count));
		analysis_remove_account_from_list(account, template->data.balance.outgoing,
				&(template->data.balance.outgoing_count));
		break;

	case REPORT_TYPE_NONE:
		break;
	}
}


/**
 * Remove any references to an account from an account list array.
 *
 * \param account		The account to remove, if present.
 * \param *array		The account list array.
 * \param *count		Pointer to number of accounts in the array, which
 *				is updated before return.
 * \return			The new account count in the array.
 */

static int analysis_remove_account_from_list(acct_t account, acct_t *array, int *count)
{
	int	i = 0, j = 0;

	while (i < *count && j < *count) {
		/* Skip j on until it finds an account that is to be left in. */

		while (j < *count && array[j] == account)
			j++;

		/* If pointers are different, and not pointing to the account, copy down the account. */
		if (i < j && i < *count && j < *count && array[j] != account)
			array[i] = array[j];

		/* Increment the pointers if necessary */
		if (array[i] != account) {
			i++;
			j++;
		}
	}

	*count = i;

	return i;
}


/**
 * Clear all the account report flags in a file, to allow them to be re-set
 * for a new report.
 *
 * \param *file			The file to which the data belongs.
 * \param *data			The data to be cleared.
 */

static void analysis_clear_account_report_flags(struct file_block *file, struct analysis_data *data)
{
	int	i;

	if (file == NULL || data == NULL)
		return;

	for (i = 0; i < account_get_count(file); i++)
		data[i].report_flags = ANALYSIS_REPORT_NONE;
}


/**
 * Set the specified report flags for all accounts that match the list given.
 * The account NULL_ACCOUNT will set all the acounts that match the given type.
 *
 * \param *file			The file to process.
 * \param *data			The data to be set.
 * \param type			The type(s) of account to match for NULL_ACCOUNT.
 * \param flags			The report flags to set for matching accounts.
 * \param *array		The account list array to use.
 * \param count			The number of accounts in the account list.
 */

static void analysis_set_account_report_flags_from_list(struct file_block *file, struct analysis_data *data, unsigned type, unsigned flags, acct_t *array, int count)
{
	int	account, i;

	for (i=0; i<count; i++) {
		account = array[i];

		if (account == NULL_ACCOUNT) {
			/* 'Wildcard': set all the accounts which match the
			 * given account type.
			 */

			for (account = 0; account < account_get_count(file); account++)
				if ((account_get_type(file, account) & type) != 0)
					data[account].report_flags |= flags;
		} else {
			/* Set a specific account. */

			if (account < account_get_count(file))
				data[account].report_flags |= flags;
		}
	}
}


/**
 * Convert a textual comma-separated list of account idents into a numeric
 * account list array.  The special account ident '*' means 'all', and is
 * stored as the 'wildcard' value (in this context) NULL_ACCOUNT.
 *
 * \param *file			The file to process.
 * \param type			The type(s) of account to process.
 * \param *list			The textual account ident list to process.
 * \param *array		Pointer to memory to take the numeric list,
 *				with space for ANALYSIS_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

static int analysis_account_idents_to_list(struct file_block *file, unsigned type, char *list, acct_t *array)
{
	char	*copy, *ident;
	int	account, i = 0;

	copy = strdup(list);

	if (copy == NULL)
		return 0;

	ident = strtok(copy, ",");

	while (ident != NULL && i < ANALYSIS_ACC_LIST_LEN) {
		if (strcmp(ident, "*") == 0) {
			array[i++] = NULL_ACCOUNT;
		} else {
			account = account_find_by_ident(file, ident, type);

			if (account != NULL_ACCOUNT)
				array[i++] = account;
		}

		ident = strtok(NULL, ",");
	}

	free(copy);

	return i;
}


/* Convert a numeric account list array into a textual list of comma-separated
 * account idents.
 *
 * \param *file			The file to process.
 * \param *list			Pointer to the buffer to take the textual
 *				list, which must be ANALYSIS_ACC_SPEC_LEN long.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

static void analysis_account_list_to_idents(struct file_block *file, char *list, acct_t *array, int len)
{
	char	buffer[ACCOUNT_IDENT_LEN];
	int	account, i;

	*list = '\0';

	for (i=0; i<len; i++) {
		account = array[i];

		if (account != NULL_ACCOUNT)
			strcpy(buffer, account_get_ident(file, account));
		else
			strcpy(buffer, "*");

		if (strlen(list) > 0 && strlen(list)+1 < ANALYSIS_ACC_SPEC_LEN)
			strcat(list, ",");

		if (strlen(list) + strlen(buffer) < ANALYSIS_ACC_SPEC_LEN)
			strcat(list, buffer);
	}
}


/**
 * Convert a textual comma-separated list of hex numbers into a numeric
 * account list array.
 *
 * \param *file			The file to process.
 * \param *list			The textual hex number list to process.
 * \param *array		Pointer to memory to take the numeric list,
 *				with space for ANALYSIS_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

int analysis_account_hex_to_list(struct file_block *file, char *list, acct_t *array)
{
	char	*copy, *value;
	int	i = 0;

	copy = strdup(list);

	if (copy == NULL)
		return 0;

	value = strtok(copy, ",");

	while (value != NULL && i < ANALYSIS_ACC_LIST_LEN) {
		array[i++] = strtoul(value, NULL, 16);

		value = strtok(NULL, ",");
	}

	free(copy);

	return i;
}


/* Convert a numeric account list array into a textual list of comma-separated
 * hex values.
 *
 * \param *file			The file to process.
 * \param *list			Pointer to the buffer to take the textual list.
 * \param size			The size of the buffer.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

void analysis_account_list_to_hex(struct file_block *file, char *list, size_t size, acct_t *array, int len)
{
	char	buffer[32];
	int	i;

	*list = '\0';

	for (i=0; i<len; i++) {
		sprintf(buffer, "%x", array[i]);

		if (strlen(list) > 0 && strlen(list)+1 < size)
			strcat(list, ",");

		if (strlen(list) + strlen(buffer) < size)
			strcat(list, buffer);
	}
}


/**
 * Open the Save Template dialogue box.
 *
 * \param *template	The report template to be saved.
 * \param *ptr		The current Wimp Pointer details.
 */

void analysis_open_save_window(struct analysis_report *template, wimp_pointer *ptr)
{
	/* If the window is already open, another report is being saved.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_save_window))
		wimp_close_window(analysis_save_window);

	/* Set the window contents up. */

	msgs_lookup("SaveRepTitle", windows_get_indirected_title_addr(analysis_save_window), 20);
	msgs_lookup("SaveRepSave", icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_OK), 10);

	/* The popup can be shaded here, as the only way its state can be changed is if a report is added: which
	 * can only be done via this dialogue.  In the (unlikely) event that the Save dialogue is open when the last
	 * report is deleted, then the popup remains active but no memu will appear...
	 */

	icons_set_shaded(analysis_save_window, ANALYSIS_SAVE_NAMEPOPUP, template->file->saved_report_count == 0);

	analysis_fill_save_window(template);

	ihelp_set_modifier(analysis_save_window, "Sav");

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_save_file = template->file;
	analysis_save_report = template;
	analysis_save_mode = ANALYSIS_SAVE_MODE_SAVE;

	windows_open_centred_at_pointer(analysis_save_window, ptr);
	place_dialogue_caret_fallback(analysis_save_window, 1, ANALYSIS_SAVE_NAME);
}


/**
 * Open the Rename Template dialogue box.
 *
 * \param *file		The file owning the template.
 * \param template	The template to be renamed.
 * \param *ptr		The current Wimp Pointer details.
 */

static void analysis_open_rename_window(struct file_block *file, int template, wimp_pointer *ptr)
{
	/* If the window is already open, another report is being saved.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_save_window))
		wimp_close_window(analysis_save_window);

	/* Set the window contents up. */

	msgs_lookup ("RenRepTitle", windows_get_indirected_title_addr(analysis_save_window), 20);
	msgs_lookup ("RenRepRen", icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_OK), 10);

	/* The popup can be shaded here, as the only way its state can be changed is if a report is added: which
	 * can only be done via this dislogue.  In the (unlikely) event that the Save dialogue is open when the last
	 * report is deleted, then the popup remains active but no memu will appear...
	 */

	icons_set_shaded(analysis_save_window, ANALYSIS_SAVE_NAMEPOPUP, file->saved_report_count == 0);

	analysis_fill_rename_window(file, template);

	ihelp_set_modifier(analysis_save_window, "Ren");

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_save_file = file;
	analysis_save_template = template;
	analysis_save_mode = ANALYSIS_SAVE_MODE_RENAME;

	windows_open_centred_at_pointer(analysis_save_window, ptr);
	place_dialogue_caret_fallback(analysis_save_window, 1, ANALYSIS_SAVE_NAME);
}


/**
 * Process mouse clicks in the Save/Rename Report dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_save_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ANALYSIS_SAVE_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_save_window);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_save_window();
		}
		break;

	case ANALYSIS_SAVE_OK:
		if (analysis_process_save_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_save_window);
		}
		break;
	}
}


/**
 * Process keypresses in the Save/Rename Report window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool analysis_save_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_process_save_window()) {
			close_dialogue_with_caret(analysis_save_window);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_save_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process menu prepare events in the Save/Rename Report window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void analysis_save_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	wimp_menu	*template_menu;

	template_menu = analysis_template_menu_build(analysis_save_file, TRUE);
	event_set_menu_block(template_menu);
	ihelp_add_menu(template_menu, "RepListMenu");
}


/**
 * Process menu selection events in the Save/Rename Report window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void analysis_save_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	if (selection->items[0] == -1)
		return;

	icons_strncpy(analysis_save_window, ANALYSIS_SAVE_NAME, analysis_template_menu_link[selection->items[0]].name);
	icons_redraw_group(analysis_save_window, 1, ANALYSIS_SAVE_NAME);
	icons_replace_caret_in_window(analysis_save_window);
}


/**
 * Process menu close events in the Save/Rename Report window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void analysis_save_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	analysis_template_menu_destroy();
	ihelp_remove_menu(analysis_template_menu);
}


/**
 * Refresh the contents of the current Save/Rename Report window.
 */

static void analysis_refresh_save_window(void)
{
	switch (analysis_save_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		analysis_fill_save_window(analysis_save_report);
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		analysis_fill_rename_window(analysis_save_file, analysis_save_template);
		break;

	default:
		break;
	}

	icons_redraw_group(analysis_save_window, 1, ANALYSIS_SAVE_NAME);

	icons_replace_caret_in_window(analysis_save_window);
}


/**
 * Fill the Save Report Window with values.
 *
 * \param: *template		The report template to be saved.
 */

static void analysis_fill_save_window(struct analysis_report *template)
{
	icons_strncpy(analysis_save_window, ANALYSIS_SAVE_NAME, template->name);
}


/**
 * Fill the Rename Template Window with values.
 *
 * \param: *file		The file containing the template.
 * \param: template		The template to be renamed.
 */

static void analysis_fill_rename_window(struct file_block *file, int template)
{
	icons_strncpy(analysis_save_window, ANALYSIS_SAVE_NAME, file->saved_reports[template].name);
}


/**
 * Process OK clicks in the Save/Rename Template window.  If it is a real save,
 * pass the call on to the store saved report function.  If it is a rename,
 * handle it directly here.
 */

static osbool analysis_process_save_window(void)
{
	int		template;
	wimp_w		w;

	if (*icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_NAME) == '\0')
		return FALSE;

	template = analysis_get_template_from_name(analysis_save_file,
			icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_NAME));

	switch (analysis_save_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		if (template != NULL_TEMPLATE && error_msgs_report_question("CheckTempOvr", "CheckTempOvrB") == 2)
			return FALSE;

		strcpy(analysis_save_report->name, icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_NAME));
		analysis_store_template(analysis_save_file, analysis_save_report, template);
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		if (analysis_save_template != NULL_TEMPLATE) {
			if (template != NULL_TEMPLATE && template != analysis_save_template) {
				error_msgs_report_error("TempExists");
				return FALSE;
			}

			strcpy(analysis_save_file->saved_reports[analysis_save_template].name,
					icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_NAME));

			/* Update the window title of the parent report dialogue. */

			switch (analysis_save_file->saved_reports[analysis_save_template].type) {
			case REPORT_TYPE_TRANS:
				w = analysis_transaction_window;
				break;
			case REPORT_TYPE_UNREC:
				w = analysis_unreconciled_window;
				break;
			case REPORT_TYPE_CASHFLOW:
				w = analysis_cashflow_window;
				break;
			case REPORT_TYPE_BALANCE:
				w = analysis_balance_window;
				break;
			default:
				w = NULL;
				break;
			}

			if (w != NULL) {
				msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(w), 50,
						analysis_save_file->saved_reports[analysis_save_template].name, NULL, NULL, NULL);
						xwimp_force_redraw_title(w); /* Nested Wimp only! */
			}

			/* Mark the file has being modified. */

			file_set_data_integrity(analysis_save_file, TRUE);
		}
		break;

	default:
		break;
	}

	return TRUE;
}


/**
 * Build a Template List menu and return the pointer.
 *
 * \param *file			The file to build the menu for.
 * \param standalone		TRUE if the menu is standalone; FALSE for part of
 *				the main menu.
 * \return			The created menu, or NULL for an error.
 */

wimp_menu *analysis_template_menu_build(struct file_block *file, osbool standalone)
{
	int	line, width;

	/* Claim enough memory to build the menu in. */

	analysis_template_menu_destroy();

	if (file->saved_report_count > 0) {
		analysis_template_menu = heap_alloc(28 + 24 * file->saved_report_count);
		analysis_template_menu_link = heap_alloc(file->saved_report_count * sizeof(struct analysis_report_link));
		analysis_template_menu_title = heap_alloc(ANALYSIS_MENU_TITLE_LEN);
	}

	if (analysis_template_menu == NULL || analysis_template_menu_link == NULL || analysis_template_menu_title == NULL) {
		analysis_template_menu_destroy();
		return NULL;
	}

	/* Populate the menu. */

	width = 0;

	for (line = 0; line < file->saved_report_count; line++) {
		/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
		 * well move while the menu is open.  The account number is also stored, to allow the account to be found.
		 */

		strcpy(analysis_template_menu_link[line].name, file->saved_reports[line].name);
		if (!standalone)
			strcat(analysis_template_menu_link[line].name, "...");
		analysis_template_menu_link[line].template = line;
		if (strlen(analysis_template_menu_link[line].name) > width)
			width = strlen(analysis_template_menu_link[line].name);
	}

	qsort(analysis_template_menu_link, line, sizeof(struct analysis_report_link), analysis_template_menu_compare_entries);

	for (line = 0; line < file->saved_report_count; line++) {
		/* Set the menu and icon flags up. */

		analysis_template_menu->entries[line].menu_flags = 0;

		analysis_template_menu->entries[line].sub_menu = (wimp_menu *) -1;
		analysis_template_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
				wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
				wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

		/* Set the menu icon contents up. */

		analysis_template_menu->entries[line].data.indirected_text.text = analysis_template_menu_link[line].name;
		analysis_template_menu->entries[line].data.indirected_text.validation = NULL;
		analysis_template_menu->entries[line].data.indirected_text.size = ACCOUNT_NAME_LEN;

		#ifdef DEBUG
		debug_printf("Line %d: '%s'", line, analysis_template_menu_link[line].name);
		#endif
	}

	analysis_template_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	msgs_lookup((standalone) ? "RepListMenuT2" : "RepListMenuT1", analysis_template_menu_title, ANALYSIS_MENU_TITLE_LEN);
	analysis_template_menu->title_data.indirected_text.text = analysis_template_menu_title;
	analysis_template_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	analysis_template_menu->title_fg = wimp_COLOUR_BLACK;
	analysis_template_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	analysis_template_menu->work_fg = wimp_COLOUR_BLACK;
	analysis_template_menu->work_bg = wimp_COLOUR_WHITE;

	analysis_template_menu->width = (width + 1) * 16;
	analysis_template_menu->height = 44;
	analysis_template_menu->gap = 0;

	return analysis_template_menu;
}


/**
 * Destroy any Template List menu which is currently open.
 */

void analysis_template_menu_destroy(void)
{
	if (analysis_template_menu != NULL)
		heap_free(analysis_template_menu);

	if (analysis_template_menu_link != NULL)
		heap_free(analysis_template_menu_link);

	if (analysis_template_menu_title != NULL)
		heap_free(analysis_template_menu_title);

	analysis_template_menu = NULL;
	analysis_template_menu_link = NULL;
	analysis_template_menu_title = NULL;
}


/**
 * Compare two Template List menu entries for the benefit of qsort().
 *
 * \param *va			The first item structure.
 * \param *vb			The second item structure.
 * \return			Comparison result.
 */

static int analysis_template_menu_compare_entries(const void *va, const void *vb)
{
	struct analysis_report_link *a = (struct analysis_report_link *) va;
	struct analysis_report_link *b = (struct analysis_report_link *) vb;

	return (string_nocase_strcmp(a->name, b->name));
}


/**
 * Force the closure of any Analysis windows which are open and relate
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void analysis_force_windows_closed(struct file_block *file)
{
	if (analysis_transaction_file == file && windows_get_open(analysis_transaction_window))
		close_dialogue_with_caret(analysis_transaction_window);

	if (analysis_unreconciled_file == file && windows_get_open(analysis_unreconciled_window))
		close_dialogue_with_caret(analysis_unreconciled_window);

	if (analysis_cashflow_file == file && windows_get_open(analysis_cashflow_window))
		close_dialogue_with_caret(analysis_cashflow_window);

	if (analysis_balance_file == file && windows_get_open(analysis_balance_window))
		close_dialogue_with_caret(analysis_balance_window);

	if (analysis_save_file == file && windows_get_open(analysis_save_window))
		close_dialogue_with_caret(analysis_save_window);
}


/**
 * Force the closure of the Save Template window if it is open to save the
 * given template.
 *
 * \param *template			The template of interest.
 */

void analysis_force_close_report_save_window(struct analysis_report *template)
{
	if (analysis_save_mode == ANALYSIS_SAVE_MODE_SAVE &&
			analysis_save_file == template->file && analysis_save_report == template &&
			windows_get_open(analysis_save_window))
		close_dialogue_with_caret(analysis_save_window);
}


/**
 * Force the closure of the Rename Template window if it is open to rename the
 * given template.
 *
 * \param *report			The report of interest.
 */

static void analysis_force_close_report_rename_window(wimp_w window)
{
	if (!windows_get_open(analysis_save_window) ||
			analysis_save_mode != ANALYSIS_SAVE_MODE_RENAME ||
			analysis_save_template == NULL_TEMPLATE)
		return;

	if ((window == analysis_transaction_window &&
			analysis_save_file->saved_reports[analysis_save_template].type == REPORT_TYPE_TRANS) ||
			(window == analysis_unreconciled_window &&
			analysis_save_file->saved_reports[analysis_save_template].type == REPORT_TYPE_UNREC) ||
			(window == analysis_cashflow_window &&
			analysis_save_file->saved_reports[analysis_save_template].type == REPORT_TYPE_CASHFLOW) ||
			(window == analysis_balance_window &&
			analysis_save_file->saved_reports[analysis_save_template].type == REPORT_TYPE_BALANCE))
		close_dialogue_with_caret(analysis_save_window);
}


/* Open a report from a saved template, following its selection from the
 * template list menu.
 *
 * \param *file			The file owning the template.
 * \param *ptr			The Wimp pointer details.
 * \param selection		The menu selection entry.
 */

void analysis_open_template_from_menu(struct file_block *file, wimp_pointer *ptr, int selection)
{
	int template;

	if (analysis_template_menu_link == NULL || selection < 0 || selection >= file->saved_report_count)
		return;

	template = analysis_template_menu_link[selection].template;

	if (template < 0 || template >= file->saved_report_count)
		return;

	switch (file->saved_reports[template].type) {
	case REPORT_TYPE_TRANS:
		analysis_open_transaction_window(file, ptr, template, config_opt_read ("RememberValues"));
		break;

	case REPORT_TYPE_UNREC:
		analysis_open_unreconciled_window(file, ptr, template, config_opt_read ("RememberValues"));
		break;

	case REPORT_TYPE_CASHFLOW:
		analysis_open_cashflow_window(file, ptr, template, config_opt_read ("RememberValues"));
		break;

	case REPORT_TYPE_BALANCE:
		analysis_open_balance_window(file, ptr, template, config_opt_read ("RememberValues"));
		break;

	case REPORT_TYPE_NONE:
		break;
	}
}


/**
 * Find a save template ID based on its name.
 *
 * \param *file			The file to search in.
 * \param *name			The name to search for.
 * \return			The matching template ID, or NULL_TEMPLATE.
 */

static int analysis_get_template_from_name(struct file_block *file, char *name)
{
	int	i, found = NULL_TEMPLATE;

	if (file != NULL && file->saved_report_count > 0)
		for (i=0; i<file->saved_report_count && found == -1; i++)
			if (string_nocase_strcmp (file->saved_reports[i].name, name) == 0)
				found = i;

	return found;
}


/**
 * Store a report's template into a file.
 *
 * \param *file			The file to work on.
 * \param *report		The report to take the template from.
 * \param template		The template pointer to save to, or
 *				NULL_TEMPLATE to add a new entry.
 */

static void analysis_store_template(struct file_block *file, struct analysis_report *report, int template)
{
	if (template == NULL_TEMPLATE) {
		if (!flexutils_resize((void **) &(file->saved_reports), sizeof(struct analysis_report), file->saved_report_count + 1)) {
			error_msgs_report_error("NoMemNewTemp");
			return;
		}

		template = file->saved_report_count++;
	}

	if (template < 0 || template >= file->saved_report_count)
		return;

	analysis_copy_template(&(file->saved_reports[template]), report);
	file_set_data_integrity(file, TRUE);
}


/**
 * Delete a saved report from the file, and adjust any other template pointers
 * which are currently in use.
 *
 * \param *file			The file to delete the template from.
 * \param template		The template to delete.
 */

static void analysis_delete_template(struct file_block *file, int template)
{
	enum analysis_report_type	type;

	/* Delete the specified template. */

	if (template < 0 || template >= file->saved_report_count)
		return;

	type = file->saved_reports[template].type;

	/* First remove the template from the flex block. */

	if (!flexutils_delete_object((void **) &(file->saved_reports), sizeof(struct analysis_report), template)) {
		error_msgs_report_error("BadDelete");
		return;
	}

	file->saved_report_count--;
	file_set_data_integrity(file, TRUE);

	/* If the rename template window is open for this template, close it now before the pointer is lost. */

	if (windows_get_open(analysis_save_window) && analysis_save_mode == ANALYSIS_SAVE_MODE_RENAME &&
			template == analysis_save_template)
		close_dialogue_with_caret(analysis_save_window);

	/* Now adjust any other template pointers for currently open report dialogues,
	 * which may be pointing further up the array.
	 * In addition, if the rename template pointer (analysis_save_template) is pointing to the deleted
	 * item, it needs to be unset.
	 */

	if (type != REPORT_TYPE_TRANS && analysis_transaction_template > template)
		analysis_transaction_template--;

	if (type != REPORT_TYPE_UNREC && analysis_unreconciled_template > template)
		analysis_unreconciled_template--;

	if (type != REPORT_TYPE_CASHFLOW && analysis_cashflow_template > template)
		analysis_cashflow_template--;

	if (type != REPORT_TYPE_BALANCE && analysis_balance_template > template)
		analysis_balance_template--;

	if (analysis_save_template > template)
		analysis_save_template--;
	else if (analysis_save_template == template)
		analysis_save_template = NULL_TEMPLATE;
}


/**
 * Create a new analysis template in the static heap, optionally copying the
 * contents from an existing template.
 *
 * \param *base			Pointer to a template to copy, or NULL to
 *				create an empty template.
 * \return			Pointer to the new template, or NULL on failure.
 */

static struct analysis_report *analysis_create_template(struct analysis_report *base)
{
	struct analysis_report	*new;
	
	new = heap_alloc(sizeof(struct analysis_report));
	if (new == NULL)
		return NULL;

	if (base == NULL) {
		*(new->name) = '\0';
		new->type = REPORT_TYPE_NONE;
		new->file = NULL;
	} else {
		analysis_copy_template(new, base);
	}

	return new;
}


/**
 * Copy a Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

void analysis_copy_template(struct analysis_report *to, struct analysis_report *from)
{
	if (from == NULL || to == NULL)
		return;

#ifdef DEBUG
	debug_printf("Copy template from 0x%x to 0x%x", from, to);
#endif

	strcpy(to->name, from->name);
	to->type = from->type;
	to->file = from->file;

	switch (from->type) {
	case REPORT_TYPE_TRANS:
		analysis_copy_transaction_template(&(to->data.transaction), &(from->data.transaction));
		break;

	case REPORT_TYPE_UNREC:
		analysis_copy_unreconciled_template(&(to->data.unreconciled), &(from->data.unreconciled));
		break;

	case REPORT_TYPE_CASHFLOW:
		analysis_copy_cashflow_template(&(to->data.cashflow), &(from->data.cashflow));
		break;

	case REPORT_TYPE_BALANCE:
		analysis_copy_balance_template(&(to->data.balance), &(from->data.balance));
		break;

	case REPORT_TYPE_NONE:
		break;
	}
}


/**
 * Copy a Transaction Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_copy_transaction_template(struct trans_rep *to, struct trans_rep *from)
{
	int	i;

	if (from == NULL || to == NULL)
		return;

	to->date_from = from->date_from;
	to->date_to = from->date_to;
	to->budget = from->budget;

	to->group = from->group;
	to->period = from->period;
	to->period_unit = from->period_unit;
	to->lock = from->lock;

	to->from_count = from->from_count;
	for (i=0; i<from->from_count; i++)
		to->from[i] = from->from[i];

	to->to_count = from->to_count;
	for (i=0; i<from->to_count; i++)
		to->to[i] = from->to[i];

	strcpy(to->ref, from->ref);
	strcpy(to->desc, from->desc);
	to->amount_min = from->amount_min;
	to->amount_max = from->amount_max;

	to->output_trans = from->output_trans;
	to->output_summary = from->output_summary;
	to->output_accsummary = from->output_accsummary;
}


/**
 * Copy a Unreconciled Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_copy_unreconciled_template(struct unrec_rep *to, struct unrec_rep *from)
{
	int	i;

	if (from == NULL || to == NULL)
		return;

	to->date_from = from->date_from;
	to->date_to = from->date_to;
	to->budget = from->budget;

	to->group = from->group;
	to->period = from->period;
	to->period_unit = from->period_unit;
	to->lock = from->lock;

	to->from_count = from->from_count;
	for (i=0; i<from->from_count; i++)
		to->from[i] = from->from[i];

	to->to_count = from->to_count;
	for (i=0; i<from->to_count; i++)
		to->to[i] = from->to[i];
}


/**
 * Copy a Cashflow Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_copy_cashflow_template(struct cashflow_rep *to, struct cashflow_rep *from)
{
	int	i;

	if (from == NULL || to == NULL)
		return;

	to->date_from = from->date_from;
	to->date_to = from->date_to;
	to->budget = from->budget;

	to->group = from->group;
	to->period = from->period;
	to->period_unit = from->period_unit;
	to->lock = from->lock;
	to->empty = from->empty;

	to->accounts_count = from->accounts_count;
	for (i=0; i<from->accounts_count; i++)
		to->accounts[i] = from->accounts[i];

	to->incoming_count = from->incoming_count;
	for (i=0; i<from->incoming_count; i++)
		to->incoming[i] = from->incoming[i];

	to->outgoing_count = from->outgoing_count;
	for (i=0; i<from->outgoing_count; i++)
		to->outgoing[i] = from->outgoing[i];

	to->tabular = from->tabular;
}


/**
 * Copy a Balance Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_copy_balance_template(struct balance_rep *to, struct balance_rep *from)
{
	int	i;

	if (from == NULL || to == NULL)
		return;

	to->date_from = from->date_from;
	to->date_to = from->date_to;
	to->budget = from->budget;

	to->group = from->group;
	to->period = from->period;
	to->period_unit = from->period_unit;
	to->lock = from->lock;

	to->accounts_count = from->accounts_count;
	for (i=0; i<from->accounts_count; i++)
		to->accounts[i] = from->accounts[i];
	to->incoming_count = from->incoming_count;

	for (i=0; i<from->incoming_count; i++)
		to->incoming[i] = from->incoming[i];

	to->outgoing_count = from->outgoing_count;
	for (i=0; i<from->outgoing_count; i++)
		to->outgoing[i] = from->outgoing[i];

	to->tabular = from->tabular;
}


/**
 * Save the Report Template details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void analysis_write_file(struct file_block *file, FILE *out)
{
	int	i;
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	fprintf(out, "\n[Reports]\n");

	fprintf(out, "Entries: %x\n", file->saved_report_count);

	for (i = 0; i < file->saved_report_count; i++) {
		switch(file->saved_reports[i].type) {
		case REPORT_TYPE_TRANS:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_TRANS,
					file->saved_reports[i].data.transaction.date_from,
					file->saved_reports[i].data.transaction.date_to,
					file->saved_reports[i].data.transaction.budget,
					file->saved_reports[i].data.transaction.group,
					file->saved_reports[i].data.transaction.period,
					file->saved_reports[i].data.transaction.period_unit,
					file->saved_reports[i].data.transaction.lock,
					file->saved_reports[i].data.transaction.output_trans,
					file->saved_reports[i].data.transaction.output_summary,
					file->saved_reports[i].data.transaction.output_accsummary);
			if (*(file->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->saved_reports[i].name);
			if (file->saved_reports[i].data.transaction.from_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.transaction.from,
						file->saved_reports[i].data.transaction.from_count);
				config_write_token_pair(out, "From", buffer);
			}
			if (file->saved_reports[i].data.transaction.to_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.transaction.to,
						file->saved_reports[i].data.transaction.to_count);
				config_write_token_pair(out, "To", buffer);
			}
			if (*(file->saved_reports[i].data.transaction.ref) != '\0')
				config_write_token_pair(out, "Ref", file->saved_reports[i].data.transaction.ref);
			if (file->saved_reports[i].data.transaction.amount_min != NULL_CURRENCY ||
					file->saved_reports[i].data.transaction.amount_max != NULL_CURRENCY) {
				sprintf(buffer, "%x,%x",
						file->saved_reports[i].data.transaction.amount_min,
						file->saved_reports[i].data.transaction.amount_max);
				config_write_token_pair(out, "Amount", buffer);
			}
			if (*(file->saved_reports[i].data.transaction.desc) != '\0')
				config_write_token_pair(out, "Desc", file->saved_reports[i].data.transaction.desc);
			break;

		case REPORT_TYPE_UNREC:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_UNREC,
					file->saved_reports[i].data.unreconciled.date_from,
					file->saved_reports[i].data.unreconciled.date_to,
					file->saved_reports[i].data.unreconciled.budget,
					file->saved_reports[i].data.unreconciled.group,
					file->saved_reports[i].data.unreconciled.period,
					file->saved_reports[i].data.unreconciled.period_unit,
					file->saved_reports[i].data.unreconciled.lock);
			if (*(file->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->saved_reports[i].name);
			if (file->saved_reports[i].data.unreconciled.from_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.unreconciled.from,
						file->saved_reports[i].data.unreconciled.from_count);
				config_write_token_pair(out, "From", buffer);
			}
			if (file->saved_reports[i].data.unreconciled.to_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.unreconciled.to,
						file->saved_reports[i].data.unreconciled.to_count);
				config_write_token_pair(out, "To", buffer);
			}
			break;

		case REPORT_TYPE_CASHFLOW:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_CASHFLOW,
					file->saved_reports[i].data.cashflow.date_from,
					file->saved_reports[i].data.cashflow.date_to,
					file->saved_reports[i].data.cashflow.budget,
					file->saved_reports[i].data.cashflow.group,
					file->saved_reports[i].data.cashflow.period,
					file->saved_reports[i].data.cashflow.period_unit,
					file->saved_reports[i].data.cashflow.lock,
					file->saved_reports[i].data.cashflow.tabular,
					file->saved_reports[i].data.cashflow.empty);
			if (*(file->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->saved_reports[i].name);
			if (file->saved_reports[i].data.cashflow.accounts_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.cashflow.accounts,
						file->saved_reports[i].data.cashflow.accounts_count);
				config_write_token_pair(out, "Accounts", buffer);
			}
			if (file->saved_reports[i].data.cashflow.incoming_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.cashflow.incoming,
						file->saved_reports[i].data.cashflow.incoming_count);
				config_write_token_pair(out, "Incoming", buffer);
			}
			if (file->saved_reports[i].data.cashflow.outgoing_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.cashflow.outgoing,
						file->saved_reports[i].data.cashflow.outgoing_count);
				config_write_token_pair(out, "Outgoing", buffer);
			}
			break;

		case REPORT_TYPE_BALANCE:
			fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x\n",
					REPORT_TYPE_BALANCE,
					file->saved_reports[i].data.balance.date_from,
					file->saved_reports[i].data.balance.date_to,
					file->saved_reports[i].data.balance.budget,
					file->saved_reports[i].data.balance.group,
					file->saved_reports[i].data.balance.period,
					file->saved_reports[i].data.balance.period_unit,
					file->saved_reports[i].data.balance.lock,
					file->saved_reports[i].data.balance.tabular);
			if (*(file->saved_reports[i].name) != '\0')
				config_write_token_pair(out, "Name", file->saved_reports[i].name);
			if (file->saved_reports[i].data.balance.accounts_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.balance.accounts,
						file->saved_reports[i].data.balance.accounts_count);
				config_write_token_pair(out, "Accounts", buffer);
			}
			if (file->saved_reports[i].data.balance.incoming_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.balance.incoming,
						file->saved_reports[i].data.balance.incoming_count);
				config_write_token_pair(out, "Incoming", buffer);
			}
			if (file->saved_reports[i].data.balance.outgoing_count > 0) {
				analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
						file->saved_reports[i].data.balance.outgoing,
						file->saved_reports[i].data.balance.outgoing_count);
				config_write_token_pair(out, "Outgoing", buffer);
			}
			break;

		case REPORT_TYPE_NONE:
			break;
		}
	}
}


/**
 * Read Report Template details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *load_status		Pointer to return the current status of the load operation.
 * \return			The state of the config read operation.
  */

enum config_read_status analysis_read_file(struct file_block *file, FILE *in, char *section, char *token, char *value, enum filing_status *load_status)
{
	enum config_read_status	result;
	size_t			block_size;
	int			i = -1;

#ifdef DEBUG
	debug_printf("\\GLoading Analysis Reports.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->saved_reports), sizeof(struct analysis_report), &block_size)) {
		*load_status = FILING_STATUS_BAD_MEMORY;
		return sf_CONFIG_READ_EOF;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (string_nocase_strcmp(token, "Entries") == 0) {
			block_size = strtoul(value, NULL, 16);
			if (block_size > file->saved_report_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize((void **) &(file->saved_reports), block_size)) {
					*load_status = FILING_STATUS_MEMORY;
					return sf_CONFIG_READ_EOF;
				}
			} else {
				block_size = file->saved_report_count;
			}
		} else if (string_nocase_strcmp(token, "@") == 0) {
			file->saved_report_count++;
			if (file->saved_report_count > block_size) {
				block_size = file->saved_report_count;
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
				if (!flexutils_load_resize((void **) &(file->saved_reports), block_size)) {
					*load_status = FILING_STATUS_MEMORY;
					return sf_CONFIG_READ_EOF;
				}
			}
			i = file->saved_report_count-1;
			file->saved_reports[i].type = strtoul(next_field(value, ','), NULL, 16);
			switch(file->saved_reports[i].type) {
			case REPORT_TYPE_TRANS:
				file->saved_reports[i].data.transaction.date_from = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.date_to = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.budget = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.group = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.period = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.period_unit = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.lock = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.output_trans = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.output_summary = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.output_accsummary = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.transaction.amount_min = NULL_CURRENCY;
				file->saved_reports[i].data.transaction.amount_max = NULL_CURRENCY;
				file->saved_reports[i].data.transaction.from_count = 0;
				file->saved_reports[i].data.transaction.to_count = 0;
				*(file->saved_reports[i].data.transaction.ref) = '\0';
				*(file->saved_reports[i].data.transaction.desc) = '\0';
				break;

			case REPORT_TYPE_UNREC:
				file->saved_reports[i].data.unreconciled.date_from = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.unreconciled.date_to = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.unreconciled.budget = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.unreconciled.group = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.unreconciled.period = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.unreconciled.period_unit = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.unreconciled.lock = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.unreconciled.from_count = 0;
				file->saved_reports[i].data.unreconciled.to_count = 0;
				break;

			case REPORT_TYPE_CASHFLOW:
				file->saved_reports[i].data.cashflow.date_from = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.date_to = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.budget = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.group = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.period = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.period_unit = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.lock = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.tabular = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.empty = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.cashflow.accounts_count = 0;
				file->saved_reports[i].data.cashflow.incoming_count = 0;
				file->saved_reports[i].data.cashflow.outgoing_count = 0;
				break;

			case REPORT_TYPE_BALANCE:
				file->saved_reports[i].data.balance.date_from = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.date_to = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.budget = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.group = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.period = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.period_unit = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.lock = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.tabular = strtoul(next_field(NULL, ','), NULL, 16);
				file->saved_reports[i].data.balance.accounts_count = 0;
				file->saved_reports[i].data.balance.incoming_count = 0;
				file->saved_reports[i].data.balance.outgoing_count = 0;
				break;

			case REPORT_TYPE_NONE:
				break;
			}
		} else if (i != -1 && string_nocase_strcmp(token, "Name") == 0) {
			strcpy(file->saved_reports[i].name, value);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_CASHFLOW &&
				string_nocase_strcmp(token, "Accounts") == 0) {
			file->saved_reports[i].data.cashflow.accounts_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.cashflow.accounts);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_CASHFLOW &&
				string_nocase_strcmp(token, "Incoming") == 0) {
			file->saved_reports[i].data.cashflow.incoming_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.cashflow.incoming);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_CASHFLOW &&
				string_nocase_strcmp(token, "Outgoing") == 0) {
			file->saved_reports[i].data.cashflow.outgoing_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.cashflow.outgoing);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_BALANCE &&
				string_nocase_strcmp(token, "Accounts") == 0) {
			file->saved_reports[i].data.balance.accounts_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.balance.accounts);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_BALANCE &&
				string_nocase_strcmp(token, "Incoming") == 0) {
			file->saved_reports[i].data.balance.incoming_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.balance.incoming);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_BALANCE &&
				string_nocase_strcmp(token, "Outgoing") == 0) {
			file->saved_reports[i].data.balance.outgoing_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.balance.outgoing);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
				string_nocase_strcmp(token, "From") == 0) {
			file->saved_reports[i].data.transaction.from_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.transaction.from);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
				string_nocase_strcmp(token, "To") == 0) {
			file->saved_reports[i].data.transaction.to_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.transaction.to);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_UNREC &&
				string_nocase_strcmp(token, "From") == 0) {
			file->saved_reports[i].data.unreconciled.from_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.unreconciled.from);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_UNREC &&
				string_nocase_strcmp(token, "To") == 0) {
			file->saved_reports[i].data.unreconciled.to_count =
					analysis_account_hex_to_list(file, value, file->saved_reports[i].data.unreconciled.to);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
				string_nocase_strcmp(token, "Ref") == 0) {
			strcpy(file->saved_reports[i].data.transaction.ref, value);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
				string_nocase_strcmp(token, "Amount") == 0) {
			file->saved_reports[i].data.transaction.amount_min = strtoul(next_field(value, ','), NULL, 16);
			file->saved_reports[i].data.transaction.amount_max = strtoul(next_field(NULL, ','), NULL, 16);
		} else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
				string_nocase_strcmp(token, "Desc") == 0) {
			strcpy(file->saved_reports[i].data.transaction.desc, value);
		} else {
			*load_status = FILING_STATUS_UNEXPECTED;
		}

		result = config_read_token_pair(in, token, value, section);
	} while (result != sf_CONFIG_READ_EOF && result != sf_CONFIG_READ_NEW_SECTION);

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink((void **) &(file->saved_reports), file->saved_report_count)) {
		*load_status = FILING_STATUS_BAD_MEMORY;
		return sf_CONFIG_READ_EOF;
	}

	return result;
}

