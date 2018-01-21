/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: analysis_cashflow.c
 *
 * Analysis Cashflow Report implementation.
 */

/* ANSI C header files */

#include <string.h>

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"

/* Application header files */

#include "global.h"
#include "analysis_cashflow.h"

#include "account.h"
#include "analysis.h"
#include "analysis_data.h"
#include "analysis_dialogue.h"
#include "analysis_period.h"
#include "analysis_template.h"
#include "currency.h"
#include "date.h"
#include "file.h"
#include "report.h"
#include "stringbuild.h"
#include "transact.h"

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


/**
 * Cashflow Report Template structure.
 */

struct analysis_cashflow_report {
	date_t				date_from;
	date_t				date_to;
	osbool				budget;

	osbool				group;
	int				period;
	enum date_period		period_unit;
	osbool				lock;
	osbool				empty;

	int				accounts_count;
	int				incoming_count;
	int				outgoing_count;
	acct_t				accounts[ANALYSIS_ACC_LIST_LEN];
	acct_t				incoming[ANALYSIS_ACC_LIST_LEN];
	acct_t				outgoing[ANALYSIS_ACC_LIST_LEN];

	osbool				tabular;
};

/**
 * Cashflow Report Instance data.
 */

struct analysis_cashflow_block {
	/**
	 * The parent analysis report instance.
	 */

	struct analysis_block			*parent;

	/**
	 * The saved instance report settings.
	 */

	struct analysis_cashflow_report		saved;
};

/**
 * The dialogue instance used for this report.
 */

static struct analysis_dialogue_block	*analysis_cashflow_dialogue = NULL;

/* Static Function Prototypes. */

static void *analysis_cashflow_create_instance(struct analysis_block *parent);
static void analysis_cashflow_delete_instance(void *instance);
static void analysis_cashflow_open_window(void *instance, wimp_pointer *pointer, template_t template, osbool restore);
static void analysis_cashflow_rename_template(struct analysis_block *parent, template_t template, char *name);
static void analysis_cashflow_fill_window(struct analysis_block *parent, wimp_w window, void *block);
static void analysis_cashflow_process_window(struct analysis_block *parent, wimp_w window, void *block);
static void analysis_cashflow_generate(struct analysis_block *parent, void *template, struct report *report, struct analysis_data_block *scratch, char *title);
static void analysis_cashflow_remove_template(struct analysis_block *parent, template_t template);
static void analysis_cashflow_remove_account(void *report, acct_t account);
static void analysis_cashflow_copy_template(void *to, void *from);
static void analysis_cashflow_write_file_block(void *block, FILE *out, char *name);
static void analysis_cashflow_process_file_token(void *block, struct filing_block *in);

/* The Cashflow Report definition. */

static struct analysis_report_details analysis_cashflow_details = {
	"CRWinT", "CRTitle",
	analysis_cashflow_create_instance,
	analysis_cashflow_delete_instance,
	analysis_cashflow_open_window,
	analysis_cashflow_fill_window,
	analysis_cashflow_process_window,
	analysis_cashflow_generate,
	analysis_cashflow_process_file_token,
	analysis_cashflow_write_file_block,
	analysis_cashflow_copy_template,
	analysis_cashflow_rename_template,
	analysis_cashflow_remove_account,
	analysis_cashflow_remove_template
};

/* The Cashflow Report Dialogue Icon Details. */

static struct analysis_dialogue_icon analysis_cashflow_icon_list[] = {
	{ANALYSIS_DIALOGUE_ICON_GENERATE,					ANALYSIS_CASHFLOW_OK,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_CANCEL,						ANALYSIS_CASHFLOW_CANCEL,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_DELETE,						ANALYSIS_CASHFLOW_DELETE,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_RENAME,						ANALYSIS_CASHFLOW_RENAME,		ANALYSIS_DIALOGUE_NO_ICON},

	/* Budget Group. */

	{ANALYSIS_DIALOGUE_ICON_SHADE_TARGET,					ANALYSIS_CASHFLOW_BUDGET,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON,					ANALYSIS_CASHFLOW_DATEFROMTXT,		ANALYSIS_CASHFLOW_BUDGET},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_CASHFLOW_DATEFROM,		ANALYSIS_CASHFLOW_BUDGET},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON,					ANALYSIS_CASHFLOW_DATETOTXT,		ANALYSIS_CASHFLOW_BUDGET},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_CASHFLOW_DATETO,		ANALYSIS_CASHFLOW_BUDGET},

	/* Group Period. */

	{ANALYSIS_DIALOGUE_ICON_SHADE_TARGET,					ANALYSIS_CASHFLOW_GROUP,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_CASHFLOW_PERIOD,		ANALYSIS_CASHFLOW_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF,					ANALYSIS_CASHFLOW_PTEXT,		ANALYSIS_CASHFLOW_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF,					ANALYSIS_CASHFLOW_LOCK,			ANALYSIS_CASHFLOW_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_RADIO,	ANALYSIS_CASHFLOW_PDAYS,		ANALYSIS_CASHFLOW_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_RADIO,	ANALYSIS_CASHFLOW_PMONTHS,		ANALYSIS_CASHFLOW_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_RADIO,	ANALYSIS_CASHFLOW_PYEARS,		ANALYSIS_CASHFLOW_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF,					ANALYSIS_CASHFLOW_EMPTY,		ANALYSIS_CASHFLOW_GROUP},

	/* Account Fields. */

	{ANALYSIS_DIALOGUE_ICON_POPUP_FULL | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_CASHFLOW_ACCOUNTS,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_POPUP_FULL,					ANALYSIS_CASHFLOW_ACCOUNTSPOPUP,	ANALYSIS_CASHFLOW_ACCOUNTS},
	{ANALYSIS_DIALOGUE_ICON_POPUP_IN | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_CASHFLOW_INCOMING,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_POPUP_IN,					ANALYSIS_CASHFLOW_INCOMINGPOPUP,	ANALYSIS_CASHFLOW_INCOMING},
	{ANALYSIS_DIALOGUE_ICON_POPUP_OUT | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_CASHFLOW_OUTGOING,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_POPUP_OUT,					ANALYSIS_CASHFLOW_OUTGOINGPOPUP,	ANALYSIS_CASHFLOW_OUTGOING},

	{ANALYSIS_DIALOGUE_ICON_END,						ANALYSIS_DIALOGUE_NO_ICON,		ANALYSIS_DIALOGUE_NO_ICON}
};

/* The Cashflow Report Dialogue Definition. */

static struct analysis_dialogue_definition analysis_cashflow_dialogue_definition = {
	REPORT_TYPE_CASHFLOW,
	sizeof(struct analysis_cashflow_report),
	"CashFlwRep",
	"CashFlwRep",
	"CflRepTitle",
	analysis_cashflow_icon_list
};


/**
 * Initialise the Cashflow analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_cashflow_initialise(void)
{
	analysis_template_set_block_size(analysis_cashflow_dialogue_definition.block_size);
	analysis_cashflow_dialogue = analysis_dialogue_initialise(&analysis_cashflow_dialogue_definition);

	return &analysis_cashflow_details;
}


/**
 * Construct new cashflow report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \param *parent	Pointer to the parent analysis instance.
 * \return		Pointer to the new data block, or NULL on error.
 */

static void *analysis_cashflow_create_instance(struct analysis_block *parent)
{
	struct analysis_cashflow_block	*new;

	new = heap_alloc(sizeof(struct analysis_cashflow_block));
	if (new == NULL)
		return NULL;

	new->parent = parent;

	new->saved.date_from = NULL_DATE;
	new->saved.date_to = NULL_DATE;
	new->saved.budget = FALSE;
	new->saved.group = FALSE;
	new->saved.period = 1;
	new->saved.period_unit = DATE_PERIOD_MONTHS;
	new->saved.lock = FALSE;
	new->saved.empty = FALSE;
	new->saved.accounts_count = 0;
	new->saved.incoming_count = 0;
	new->saved.outgoing_count = 0;
	new->saved.tabular = FALSE;

	return new;
}


/**
 * Delete a cashflow report data block.
 *
 * \param *instance	Pointer to the report to delete.
 */

static void analysis_cashflow_delete_instance(void *instance)
{
	struct analysis_cashflow_block *report = instance;

	if (report == NULL)
		return;

	analysis_dialogue_close(analysis_cashflow_dialogue, report->parent);

	heap_free(report);
}


/**
 * Open the Cashflow Report dialogue box.
 *
 * \param *instance	The cashflow report instance to own the dialogue.
 * \param *pointer	The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

static void analysis_cashflow_open_window(void *instance, wimp_pointer *pointer, template_t template, osbool restore)
{
	struct analysis_cashflow_block *report = instance;

	if (report == NULL)
		return;

	analysis_dialogue_open(analysis_cashflow_dialogue, report->parent, pointer, template, &(report->saved), restore);
}


/**
 * Handle the user renaming templates.
 *
 * \param *parent	The parent analysis report instance for the rename.
 * \param template	The template being renamed.
 * \param *name		The new name for the report.
 */

static void analysis_cashflow_rename_template(struct analysis_block *parent, template_t template, char *name)
{
	if (parent == NULL || template == NULL_TEMPLATE || name == NULL)
		return;

	analysis_dialogue_rename_template(analysis_cashflow_dialogue, parent, template, name);
}


/**
 * Fill the Cashflow window with values.
 *
 * \param *parent		The parent analysis instance.
 * \param window		The handle of the window to be processed.
 * \param *block		The template data to put into the window, or
 *				NULL to use the defaults.
 */

static void analysis_cashflow_fill_window(struct analysis_block *parent, wimp_w window, void *block)
{
	struct analysis_cashflow_report *template = block;

	if (parent == NULL || window == NULL)
		return;

	if (template == NULL) {
		/* Set the period icons. */

		*icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_DATEFROM) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_DATETO) = '\0';

		icons_set_selected(window, ANALYSIS_CASHFLOW_BUDGET, 0);

		/* Set the grouping icons. */

		icons_set_selected(window, ANALYSIS_CASHFLOW_GROUP, 0);

		icons_strncpy(window, ANALYSIS_CASHFLOW_PERIOD, "1");
		icons_set_selected(window, ANALYSIS_CASHFLOW_PDAYS, 0);
		icons_set_selected(window, ANALYSIS_CASHFLOW_PMONTHS, 1);
		icons_set_selected(window, ANALYSIS_CASHFLOW_PYEARS, 0);
		icons_set_selected(window, ANALYSIS_CASHFLOW_LOCK, 0);
		icons_set_selected(window, ANALYSIS_CASHFLOW_EMPTY, 0);

		/* Set the accounts and format details. */

		*icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_ACCOUNTS) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_INCOMING) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_OUTGOING) = '\0';

		icons_set_selected(window, ANALYSIS_CASHFLOW_TABULAR, 0);
	} else {
		/* Set the period icons. */

		date_convert_to_string(template->date_from,
				icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_DATEFROM),
				icons_get_indirected_text_length(window, ANALYSIS_CASHFLOW_DATEFROM));
		date_convert_to_string(template->date_to,
				icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_DATETO),
				icons_get_indirected_text_length(window, ANALYSIS_CASHFLOW_DATETO));
		icons_set_selected(window, ANALYSIS_CASHFLOW_BUDGET, template->budget);

		/* Set the grouping icons. */

		icons_set_selected(window, ANALYSIS_CASHFLOW_GROUP, template->group);

		icons_printf(window, ANALYSIS_CASHFLOW_PERIOD, "%d", template->period);
		icons_set_selected(window, ANALYSIS_CASHFLOW_PDAYS, template->period_unit == DATE_PERIOD_DAYS);
		icons_set_selected(window, ANALYSIS_CASHFLOW_PMONTHS, template->period_unit == DATE_PERIOD_MONTHS);
		icons_set_selected(window, ANALYSIS_CASHFLOW_PYEARS, template->period_unit == DATE_PERIOD_YEARS);
		icons_set_selected(window, ANALYSIS_CASHFLOW_LOCK, template->lock);
		icons_set_selected(window, ANALYSIS_CASHFLOW_EMPTY, template->empty);

		/* Set the accounts and format details. */

		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_ACCOUNTS),
				icons_get_indirected_text_length(window, ANALYSIS_CASHFLOW_ACCOUNTS),
				template->accounts, template->accounts_count);
		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_INCOMING),
				icons_get_indirected_text_length(window, ANALYSIS_CASHFLOW_INCOMING),
				template->incoming, template->incoming_count);
		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_OUTGOING),
				icons_get_indirected_text_length(window, ANALYSIS_CASHFLOW_OUTGOING),
				template->outgoing, template->outgoing_count);

		icons_set_selected(window, ANALYSIS_CASHFLOW_TABULAR, template->tabular);
	}
}


/**
 * Process the contents of the Cashflow window,.
 *
 *
 * \param *parent		The parent analysis instance.
 * \param window		The handle of the window to be processed.
 * \param *block		The template to store the contents in.
 */

static void analysis_cashflow_process_window(struct analysis_block *parent, wimp_w window, void *block)
{
	struct analysis_cashflow_report *template = block;

	if (parent == NULL || template == NULL || window == NULL)
		return;

	/* Read the date settings. */

	template->date_from =
			date_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_DATEFROM), NULL_DATE, 0);
	template->date_to =
			date_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_DATETO), NULL_DATE, 0);
	template->budget = icons_get_selected(window, ANALYSIS_CASHFLOW_BUDGET);

	/* Read the grouping settings. */

	template->group = icons_get_selected(window, ANALYSIS_CASHFLOW_GROUP);
	template->period = atoi(icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_PERIOD));

	if (icons_get_selected(window, ANALYSIS_CASHFLOW_PDAYS))
		template->period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(window, ANALYSIS_CASHFLOW_PMONTHS))
		template->period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(window, ANALYSIS_CASHFLOW_PYEARS))
		template->period_unit = DATE_PERIOD_YEARS;
	else
		template->period_unit = DATE_PERIOD_MONTHS;

	template->lock = icons_get_selected(window, ANALYSIS_CASHFLOW_LOCK);
	template->empty = icons_get_selected(window, ANALYSIS_CASHFLOW_EMPTY);

	/* Read the account and heading settings. */

	template->accounts_count =
			analysis_account_idents_to_list(parent, ACCOUNT_FULL,
			icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_ACCOUNTS),
			template->accounts, ANALYSIS_ACC_LIST_LEN);
	template->incoming_count =
			analysis_account_idents_to_list(parent, ACCOUNT_IN,
			icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_INCOMING),
			template->incoming, ANALYSIS_ACC_LIST_LEN);
	template->outgoing_count =
			analysis_account_idents_to_list(parent, ACCOUNT_OUT,
			icons_get_indirected_text_addr(window, ANALYSIS_CASHFLOW_OUTGOING),
			template->outgoing, ANALYSIS_ACC_LIST_LEN);

	template->tabular = icons_get_selected(window, ANALYSIS_CASHFLOW_TABULAR);
}


/**
 * Generate a cashflow report.
 *
 * \param *parent		The parent analysis instance.
 * \param *template		The template data to use for the report.
 * \param *report		The report to write to.
 * \param *scratch		The scratch space to use to build the report.
 * \param *title		Pointer to the report title.
 */

static void analysis_cashflow_generate(struct analysis_block *parent, void *template, struct report *report, struct analysis_data_block *scratch, char *title)
{
	struct analysis_cashflow_report		*settings = template;
	struct file_block			*file;

	osbool			group, lock, tabular;
	int			found, unit, period, show_blank;
	char			date_text[1024];
	date_t			start_date, end_date, next_start, next_end;
	acct_t			acc;
	amt_t			amount, total;
	int			entries, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	if (parent == NULL || report == NULL || settings == NULL || scratch == NULL || title == NULL)
		return;

	file = analysis_get_file(parent);
	if (file == NULL)
		return;

	/* Read the grouping settings. */

	group = settings->group;
	unit = settings->period_unit;
	lock = settings->lock && (unit == DATE_PERIOD_MONTHS || unit == DATE_PERIOD_YEARS);
	period = (group) ? settings->period : 0;
	show_blank = settings->empty;

	/* Read the include list. */

	if (settings->accounts_count == 0 && settings->incoming_count == 0 && settings->outgoing_count == 0) {
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT, ANALYSIS_DATA_INCLUDE, NULL, 1);
	} else {
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL, ANALYSIS_DATA_INCLUDE, settings->accounts, settings->accounts_count);
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_IN, ANALYSIS_DATA_INCLUDE, settings->incoming, settings->incoming_count);
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_OUT, ANALYSIS_DATA_INCLUDE, settings->outgoing, settings->outgoing_count);
	}

	tabular = settings->tabular;

	/* Output report heading */

	report_write_line(report, 0, title);

	/* Read the date settings and output their details. */

	analysis_find_date_range(parent, &start_date, &end_date, settings->date_from, settings->date_to, settings->budget, report);

	/* Start to output the report. */

	if (tabular) {
		report_write_line(report, 0, "");

		stringbuild_reset();

		stringbuild_add_string("\\k\\b");
		stringbuild_add_message("CRDate");

		for (acc_group = 0; acc_group < groups; acc_group++) {
			entries = account_get_list_length(file, sequence[acc_group]);

			for (group_line = 0; group_line < entries; group_line++) {
				if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
					if (analysis_data_test_account(scratch, acc, ANALYSIS_DATA_INCLUDE)) {
						stringbuild_add_printf("\\t\\r\\b%s", account_get_name(file, acc));
					}
				}
			}
		}
		stringbuild_add_string("\\t\\r\\b");
		stringbuild_add_message("CRTotal");

		stringbuild_report_line(report, 1);
	}

	/* Process the report time groups. */

	analysis_period_initialise(start_date, end_date, period, unit, lock);

	while (analysis_period_get_next_dates(&next_start, &next_end, date_text, sizeof(date_text))) {
		found = analysis_data_calculate_balances(scratch, next_start, next_end, FALSE);

		/* Print the transaction summaries. */

		if (found > 0 || show_blank) {
			if (tabular) {
				stringbuild_reset();

				stringbuild_add_printf("\\k%s", date_text);

				total = 0;

				for (acc_group = 0; acc_group < groups; acc_group++) {
					entries = account_get_list_length(file, sequence[acc_group]);

					for (group_line = 0; group_line < entries; group_line++) {
						if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
							if (analysis_data_test_account(scratch, acc, ANALYSIS_DATA_INCLUDE)) {
								amount = analysis_data_get_total(scratch, acc);

								total += amount;

								stringbuild_add_string("\\t\\d\\r");
								stringbuild_add_currency(amount, TRUE);
							}
						}
					}
				}

				stringbuild_add_string("\\t\\d\\r");
				stringbuild_add_currency(total, TRUE);
				stringbuild_report_line(report, 1);
			} else {
				report_write_line(report, 0, "");
				if (group) {
					stringbuild_reset();
					stringbuild_add_printf("\\u%s", date_text);
					stringbuild_report_line(report, 0);
				}

				total = 0;

				for (acc_group = 0; acc_group < groups; acc_group++) {
					entries = account_get_list_length(file, sequence[acc_group]);

					for (group_line = 0; group_line < entries; group_line++) {
						if ((acc = account_get_list_entry_account(file, sequence[acc_group], group_line)) != NULL_ACCOUNT) {
							amount = analysis_data_get_total(scratch, acc);

							if (amount != 0 && analysis_data_test_account(scratch, acc, ANALYSIS_DATA_INCLUDE)) {
								total += amount;

								stringbuild_reset();
								stringbuild_add_printf("\\i%s\\t\\d\\r", account_get_name(file, acc));
								stringbuild_add_currency(amount, TRUE);
								stringbuild_report_line(report, 2);
							}
						}
					}
				}
				stringbuild_reset();
				stringbuild_add_string("\\i\\b");
				stringbuild_add_message("CRTotal");
				stringbuild_add_string("\\t\\d\\r\\b");
				stringbuild_add_currency(total, TRUE);
				stringbuild_report_line(report, 2);
			}
		}
	}
}


/**
 * Remove any references to a report template.
 * 
 * \param *parent	The analysis instance being updated.
 * \param template	The template to be removed.
 */

static void analysis_cashflow_remove_template(struct analysis_block *parent, template_t template)
{
	analysis_dialogue_remove_template(analysis_cashflow_dialogue, parent, template);
}


/**
 * Remove any references to an account if it appears within a
 * cashflow report template.
 *
 * \param *report	The cashflow report to be processed.
 * \param account	The account to be removed.
 */

static void analysis_cashflow_remove_account(void *report, acct_t account)
{
	struct analysis_cashflow_report *rep = report;

	if (rep == NULL)
		return;

	analysis_template_remove_account_from_list(account, rep->accounts, &(rep->accounts_count));
	analysis_template_remove_account_from_list(account, rep->incoming, &(rep->incoming_count));
	analysis_template_remove_account_from_list(account, rep->outgoing, &(rep->outgoing_count));
}


/**
 * Copy a Cashflow Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_cashflow_copy_template(void *to, void *from)
{
	struct analysis_cashflow_report		*a = from, *b = to;
	int					i;

	if (a == NULL || b == NULL)
		return;

	b->date_from = a->date_from;
	b->date_to = a->date_to;
	b->budget = a->budget;

	b->group = a->group;
	b->period = a->period;
	b->period_unit = a->period_unit;
	b->lock = a->lock;
	b->empty = a->empty;

	b->accounts_count = a->accounts_count;
	for (i = 0; i < a->accounts_count; i++)
		b->accounts[i] = a->accounts[i];

	b->incoming_count = a->incoming_count;
	for (i = 0; i < a->incoming_count; i++)
		b->incoming[i] = a->incoming[i];

	b->outgoing_count = a->outgoing_count;
	for (i = 0; i < a->outgoing_count; i++)
		b->outgoing[i] = a->outgoing[i];

	b->tabular = a->tabular;
}


/**
 * Write a template to a saved cashbook file.
 *
 * \param *block		The saved report template block to write.
 * \param *out			The outgoing file handle.
 * \param *name			The name of the template.
 */

static void analysis_cashflow_write_file_block(void *block, FILE *out, char *name)
{
	char				buffer[FILING_MAX_FILE_LINE_LEN];
	struct analysis_cashflow_report	*template = block;

	if (out == NULL || template == NULL)
		return;

	fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
			REPORT_TYPE_CASHFLOW,
			template->date_from,
			template->date_to,
			template->budget,
			template->group,
			template->period,
			template->period_unit,
			template->lock,
			template->tabular,
			template->empty);

	if (name != NULL && *name != '\0')
		config_write_token_pair(out, "Name", name);

	if (template->accounts_count > 0) {
		analysis_template_account_list_to_hex(buffer, FILING_MAX_FILE_LINE_LEN,
				template->accounts, template->accounts_count);
		config_write_token_pair(out, "Accounts", buffer);
	}

	if (template->incoming_count > 0) {
		analysis_template_account_list_to_hex(buffer, FILING_MAX_FILE_LINE_LEN,
				template->incoming, template->incoming_count);
		config_write_token_pair(out, "Incoming", buffer);
	}

	if (template->outgoing_count > 0) {
		analysis_template_account_list_to_hex(buffer, FILING_MAX_FILE_LINE_LEN,
				template->outgoing, template->outgoing_count);
		config_write_token_pair(out, "Outgoing", buffer);
	}
}


/**
 * Process a token from the saved report template section of a saved
 * cashbook file.
 *
 * \param *block		The saved report template block to populate.
 * \param *in			The incoming file handle.
 */

static void analysis_cashflow_process_file_token(void *block, struct filing_block *in)
{
	struct analysis_cashflow_report *template = block;

	if (in == NULL || template == NULL)
		return;

	if (filing_test_token(in, "@")) {
		template->date_from = date_get_date_field(in);
		template->date_to = date_get_date_field(in);
		template->budget = filing_get_opt_field(in);
		template->group = filing_get_opt_field(in);
		template->period = filing_get_int_field(in);
		template->period_unit = date_get_period_field(in);
		template->lock = filing_get_opt_field(in);
		template->tabular = filing_get_opt_field(in);
		template->empty = filing_get_opt_field(in);
		template->accounts_count = 0;
		template->incoming_count = 0;
		template->outgoing_count = 0;
	} else if (filing_test_token(in, "Accounts")) {
		template->accounts_count = analysis_template_account_hex_to_list(filing_get_text_value(in, NULL, 0), template->accounts);
	} else if (filing_test_token(in, "Incoming")) {
		template->incoming_count = analysis_template_account_hex_to_list(filing_get_text_value(in, NULL, 0), template->incoming);
	} else if (filing_test_token(in, "Outgoing")) {
		template->outgoing_count = analysis_template_account_hex_to_list(filing_get_text_value(in, NULL, 0), template->outgoing);
	} else {
		filing_set_status(in, FILING_STATUS_UNEXPECTED);
	}
}

