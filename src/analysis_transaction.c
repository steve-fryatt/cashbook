/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: analysis_transaction.c
 *
 * Analysis Transaction Report implementation.
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
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "analysis_transaction.h"

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
#define ANALYSIS_TRANS_OPEMPTY 41


/**
 * Transaction Report Template structure.
 */

struct analysis_transaction_report {
	date_t				date_from;
	date_t				date_to;
	osbool				budget;

	osbool				group;
	int				period;
	enum date_period		period_unit;
	osbool				lock;

	int				from_count;
	int				to_count;
	acct_t				from[ANALYSIS_ACC_LIST_LEN];
	acct_t				to[ANALYSIS_ACC_LIST_LEN];
	char				ref[TRANSACT_REF_FIELD_LEN];
	char				desc[TRANSACT_DESCRIPT_FIELD_LEN];
	amt_t				amount_min;
	amt_t				amount_max;

	osbool				output_trans;
	osbool				output_summary;
	osbool				output_accsummary;
	osbool				output_empty;
};

/**
 * Transaction Report Instance data.
 */

struct analysis_transaction_block {
	/**
	 * The parent analysis report instance.
	 */

	struct analysis_block			*parent;

	/**
	 * The saved instance report settings.
	 */

	struct analysis_transaction_report	saved;
};


/**
 * The dialogue instance used for this report.
 */

static struct analysis_dialogue_block	*analysis_transaction_dialogue = NULL;

/* Static Function Prototypes. */

static void *analysis_transaction_create_instance(struct analysis_block *parent);
static void analysis_transaction_delete_instance(void *instance);
static void analysis_transaction_open_window(void *instance, wimp_pointer *pointer, template_t template, osbool restore);
static void analysis_transaction_rename_template(struct analysis_block *parent, template_t template, char *name);
static void analysis_transaction_fill_window(struct analysis_block *parent, wimp_w window, void *block);
static void analysis_transaction_process_window(struct analysis_block *parent, wimp_w window, void *block);
static void analysis_transaction_generate(struct analysis_block *parent, void *template, struct report *report, struct analysis_data_block *scratch, char *title);
static void analysis_transaction_remove_template(struct analysis_block *parent, template_t template);
static void analysis_transaction_remove_account(void *report, acct_t account);
static void analysis_transaction_copy_template(void *to, void *from);
static void analysis_transaction_write_file_block(void *block, FILE *out, char *name);
static void analysis_transaction_process_file_token(void *block, struct filing_block *in);

/* The Transaction Report definition. */

static struct analysis_report_details analysis_transaction_details = {
	"TRWinT", "TRTitle",
	analysis_transaction_create_instance,
	analysis_transaction_delete_instance,
	analysis_transaction_open_window,
	analysis_transaction_fill_window,
	analysis_transaction_process_window,
	analysis_transaction_generate,
	analysis_transaction_process_file_token,
	analysis_transaction_write_file_block,
	analysis_transaction_copy_template,
	analysis_transaction_rename_template,
	analysis_transaction_remove_account,
	analysis_transaction_remove_template
};

/* The Transaction Report Dialogue Icon Details. */

static struct analysis_dialogue_icon analysis_transaction_icon_list[] = {
	{ANALYSIS_DIALOGUE_ICON_GENERATE,					ANALYSIS_TRANS_OK,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_CANCEL,						ANALYSIS_TRANS_CANCEL,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_DELETE,						ANALYSIS_TRANS_DELETE,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_RENAME,						ANALYSIS_TRANS_RENAME,			ANALYSIS_DIALOGUE_NO_ICON},

	/* Budget Group. */

	{ANALYSIS_DIALOGUE_ICON_SHADE_TARGET,					ANALYSIS_TRANS_BUDGET,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON,					ANALYSIS_TRANS_DATEFROMTXT,		ANALYSIS_TRANS_BUDGET},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_TRANS_DATEFROM,		ANALYSIS_TRANS_BUDGET},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON,					ANALYSIS_TRANS_DATETOTXT,		ANALYSIS_TRANS_BUDGET},
	{ANALYSIS_DIALOGUE_ICON_SHADE_ON | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_TRANS_DATETO,			ANALYSIS_TRANS_BUDGET},

	/* Group Period. */

	{ANALYSIS_DIALOGUE_ICON_SHADE_TARGET,					ANALYSIS_TRANS_GROUP,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_TRANS_PERIOD,			ANALYSIS_TRANS_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF,					ANALYSIS_TRANS_PTEXT,			ANALYSIS_TRANS_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF,					ANALYSIS_TRANS_LOCK,			ANALYSIS_TRANS_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_RADIO,	ANALYSIS_TRANS_PDAYS,			ANALYSIS_TRANS_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_RADIO,	ANALYSIS_TRANS_PMONTHS,			ANALYSIS_TRANS_GROUP},
	{ANALYSIS_DIALOGUE_ICON_SHADE_OFF | ANALYSIS_DIALOGUE_ICON_RADIO,	ANALYSIS_TRANS_PYEARS,			ANALYSIS_TRANS_GROUP},

	/* Account Fields. */

	{ANALYSIS_DIALOGUE_ICON_POPUP_FROM | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_TRANS_FROMSPEC,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_POPUP_FROM,					ANALYSIS_TRANS_FROMSPECPOPUP,		ANALYSIS_TRANS_FROMSPEC},
	{ANALYSIS_DIALOGUE_ICON_POPUP_TO | ANALYSIS_DIALOGUE_ICON_REFRESH,	ANALYSIS_TRANS_TOSPEC,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_POPUP_TO,					ANALYSIS_TRANS_TOSPECPOPUP,		ANALYSIS_TRANS_TOSPEC},

	/* Filter Fields. */

	{ANALYSIS_DIALOGUE_ICON_REFRESH,					ANALYSIS_TRANS_REFSPEC,			ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_REFRESH,					ANALYSIS_TRANS_DESCSPEC,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_REFRESH,					ANALYSIS_TRANS_AMTLOSPEC,		ANALYSIS_DIALOGUE_NO_ICON},
	{ANALYSIS_DIALOGUE_ICON_REFRESH,					ANALYSIS_TRANS_AMTHISPEC,		ANALYSIS_DIALOGUE_NO_ICON},

	{ANALYSIS_DIALOGUE_ICON_END,						ANALYSIS_DIALOGUE_NO_ICON,		ANALYSIS_DIALOGUE_NO_ICON}
};

/* The Transaction Report Dialogue Definition. */

static struct analysis_dialogue_definition analysis_transaction_dialogue_definition = {
	REPORT_TYPE_TRANSACTION,
	sizeof(struct analysis_transaction_report),
	"TransRep",
	"TransRep",
	"TrnRepTitle",
	analysis_transaction_icon_list
};


/**
 * Initialise the Transaction analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_transaction_initialise(void)
{
	analysis_template_set_block_size(analysis_transaction_dialogue_definition.block_size);
	analysis_transaction_dialogue = analysis_dialogue_initialise(&analysis_transaction_dialogue_definition);

	return &analysis_transaction_details;
}


/**
 * Construct new transaction report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \param *parent	Pointer to the parent analysis instance.
 * \return		Pointer to the new data block, or NULL on error.
 */

static void *analysis_transaction_create_instance(struct analysis_block *parent)
{
	struct analysis_transaction_block	*new;

	new = heap_alloc(sizeof(struct analysis_transaction_block));
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
	new->saved.from_count = 0;
	new->saved.to_count = 0;
	*(new->saved.ref) = '\0';
	*(new->saved.desc) = '\0';
	new->saved.amount_min = NULL_CURRENCY;
	new->saved.amount_max = NULL_CURRENCY;
	new->saved.output_trans = TRUE;
	new->saved.output_summary = TRUE;
	new->saved.output_accsummary = TRUE;
	new->saved.output_empty = FALSE;

	return new;
}


/**
 * Delete a transaction report data block.
 *
 * \param *instance	Pointer to the report to delete.
 */

static void analysis_transaction_delete_instance(void *instance)
{
	struct analysis_transaction_block *report = instance;

	if (report == NULL)
		return;

	analysis_dialogue_close(analysis_transaction_dialogue, report->parent);

	heap_free(report);
}


/**
 * Open the Transaction Report dialogue box.
 *
 * \param *instance	The transaction report instance to own the dialogue.
 * \param *pointer	The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

static void analysis_transaction_open_window(void *instance, wimp_pointer *pointer, template_t template, osbool restore)
{
	struct analysis_transaction_block *report = instance;

	if (report == NULL)
		return;

	analysis_dialogue_open(analysis_transaction_dialogue, report->parent, pointer, template, &(report->saved), restore);
}


/**
 * Handle the user renaming templates.
 *
 * \param *parent	The parent analysis report instance for the rename.
 * \param template	The template being renamed.
 * \param *name		The new name for the report.
 */

static void analysis_transaction_rename_template(struct analysis_block *parent, template_t template, char *name)
{
	if (parent == NULL || template == NULL_TEMPLATE || name == NULL)
		return;

	analysis_dialogue_rename_template(analysis_transaction_dialogue, parent, template, name);
}


/**
 * Fill the Transaction window with values.
 *
 * \param *parent		The parent analysis instance.
 * \param window		The handle of the window to be processed.
 * \param *block		The template data to put into the window, or
 *				NULL to use the defaults.
 */

static void analysis_transaction_fill_window(struct analysis_block *parent, wimp_w window, void *block)
{
	struct analysis_transaction_report *template = block;

	if (parent == NULL || window == NULL)
		return;

	if (template == NULL) {
		/* Set the period icons. */

		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_DATEFROM) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_DATETO) = '\0';

		icons_set_selected(window, ANALYSIS_TRANS_BUDGET, FALSE);

		/* Set the grouping icons. */

		icons_set_selected(window, ANALYSIS_TRANS_GROUP, FALSE);

		icons_strncpy(window, ANALYSIS_TRANS_PERIOD, "1");
		icons_set_selected(window, ANALYSIS_TRANS_PDAYS, FALSE);
		icons_set_selected(window, ANALYSIS_TRANS_PMONTHS, TRUE);
		icons_set_selected(window, ANALYSIS_TRANS_PYEARS, FALSE);
		icons_set_selected(window, ANALYSIS_TRANS_LOCK, FALSE);

		/* Set the include icons. */

		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_FROMSPEC) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_TOSPEC) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_REFSPEC) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTLOSPEC) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTHISPEC) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_DESCSPEC) = '\0';

		/* Set the output icons. */

		icons_set_selected(window, ANALYSIS_TRANS_OPTRANS, TRUE);
		icons_set_selected(window, ANALYSIS_TRANS_OPSUMMARY, TRUE);
		icons_set_selected(window, ANALYSIS_TRANS_OPACCSUMMARY, TRUE);
		icons_set_selected(window, ANALYSIS_TRANS_OPEMPTY, FALSE);
	} else {
		/* Set the period icons. */

		date_convert_to_string(template->date_from,
				icons_get_indirected_text_addr(window, ANALYSIS_TRANS_DATEFROM),
				icons_get_indirected_text_length(window, ANALYSIS_TRANS_DATEFROM));
		date_convert_to_string(template->date_to,
				icons_get_indirected_text_addr(window, ANALYSIS_TRANS_DATETO),
				icons_get_indirected_text_length(window, ANALYSIS_TRANS_DATETO));

		icons_set_selected(window, ANALYSIS_TRANS_BUDGET, template->budget);

		/* Set the grouping icons. */

		icons_set_selected(window, ANALYSIS_TRANS_GROUP, template->group);

		icons_printf(window, ANALYSIS_TRANS_PERIOD, "%d", template->period);
		icons_set_selected(window, ANALYSIS_TRANS_PDAYS, template->period_unit == DATE_PERIOD_DAYS);
		icons_set_selected(window, ANALYSIS_TRANS_PMONTHS, template->period_unit == DATE_PERIOD_MONTHS);
		icons_set_selected(window, ANALYSIS_TRANS_PYEARS, template->period_unit == DATE_PERIOD_YEARS);
		icons_set_selected(window, ANALYSIS_TRANS_LOCK, template->lock);

		/* Set the include icons. */

		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(window, ANALYSIS_TRANS_FROMSPEC),
				icons_get_indirected_text_length(window, ANALYSIS_TRANS_FROMSPEC),
				template->from, template->from_count);
		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(window, ANALYSIS_TRANS_TOSPEC),
				icons_get_indirected_text_length(window, ANALYSIS_TRANS_TOSPEC),
				template->to, template->to_count);
		icons_strncpy(window, ANALYSIS_TRANS_REFSPEC, template->ref);
		currency_convert_to_string(template->amount_min,
				icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTLOSPEC),
				icons_get_indirected_text_length(window, ANALYSIS_TRANS_AMTLOSPEC));
		currency_convert_to_string(template->amount_max,
				icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTHISPEC),
				icons_get_indirected_text_length(window, ANALYSIS_TRANS_AMTHISPEC));
		icons_strncpy(window, ANALYSIS_TRANS_DESCSPEC, template->desc);

		/* Set the output icons. */

		icons_set_selected(window, ANALYSIS_TRANS_OPTRANS, template->output_trans);
		icons_set_selected(window, ANALYSIS_TRANS_OPSUMMARY, template->output_summary);
		icons_set_selected(window, ANALYSIS_TRANS_OPACCSUMMARY, template->output_accsummary);
		icons_set_selected(window, ANALYSIS_TRANS_OPEMPTY, template->output_empty);
	}
}


/**
 * Process the contents of the Transaction window,.
 *
 *
 * \param *parent		The parent analysis instance.
 * \param window		The handle of the window to be processed.
 * \param *block		The template to store the contents in.
 */

static void analysis_transaction_process_window(struct analysis_block *parent, wimp_w window, void *block)
{
	struct analysis_transaction_report *template = block;

	if (parent == NULL || template == NULL || window == NULL)
		return;

	/* Read the date settings. */

	template->date_from =
			date_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_TRANS_DATEFROM), NULL_DATE, 0);
	template->date_to =
			date_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_TRANS_DATETO), NULL_DATE, 0);
	template->budget = icons_get_selected(window, ANALYSIS_TRANS_BUDGET);

	/* Read the grouping settings. */

	template->group = icons_get_selected(window, ANALYSIS_TRANS_GROUP);
	template->period = atoi(icons_get_indirected_text_addr (window, ANALYSIS_TRANS_PERIOD));

	if (icons_get_selected	(window, ANALYSIS_TRANS_PDAYS))
		template->period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(window, ANALYSIS_TRANS_PMONTHS))
		template->period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(window, ANALYSIS_TRANS_PYEARS))
		template->period_unit = DATE_PERIOD_YEARS;
	else
		template->period_unit = DATE_PERIOD_MONTHS;

	template->lock = icons_get_selected(window, ANALYSIS_TRANS_LOCK);

	/* Read the account and heading settings. */

	template->from_count =
			analysis_account_idents_to_list(parent, ACCOUNT_FULL | ACCOUNT_IN,
			icons_get_indirected_text_addr(window, ANALYSIS_TRANS_FROMSPEC),
			template->from, ANALYSIS_ACC_LIST_LEN);
	template->to_count =
			analysis_account_idents_to_list(parent, ACCOUNT_FULL | ACCOUNT_OUT,
			icons_get_indirected_text_addr(window, ANALYSIS_TRANS_TOSPEC),
			template->to, ANALYSIS_ACC_LIST_LEN);
	icons_copy_text(window, ANALYSIS_TRANS_REFSPEC, template->ref, TRANSACT_REF_FIELD_LEN);
	icons_copy_text(window, ANALYSIS_TRANS_DESCSPEC, template->desc, TRANSACT_DESCRIPT_FIELD_LEN);
	template->amount_min = (*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTLOSPEC) == '\0') ?
			NULL_CURRENCY : currency_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTLOSPEC));

	template->amount_max = (*icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTHISPEC) == '\0') ?
			NULL_CURRENCY : currency_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_TRANS_AMTHISPEC));

	/* Read the output options. */

	template->output_trans = icons_get_selected(window, ANALYSIS_TRANS_OPTRANS);
 	template->output_summary = icons_get_selected(window, ANALYSIS_TRANS_OPSUMMARY);
	template->output_accsummary = icons_get_selected(window, ANALYSIS_TRANS_OPACCSUMMARY);
	template->output_empty = icons_get_selected(window, ANALYSIS_TRANS_OPEMPTY);
}


/**
 * Generate a transaction report.
 *
 * \param *parent		The parent analysis instance.
 * \param *template		The template data to use for the report.
 * \param *report		The report to write to.
 * \param *scratch		The scratch space to use to build the report.
 * \param *title		Pointer to the report title.
 */

static void analysis_transaction_generate(struct analysis_block *parent, void *template, struct report *report, struct analysis_data_block *scratch, char *title)
{
	struct analysis_transaction_report	*settings = template;
	struct file_block			*file;
	int					found, total, total_days, period_days, period_limit, entries, account;
	date_t					start_date, end_date, next_start, next_end, date;
	tran_t					i;
	acct_t					from, to;
	amt_t					min_amount, max_amount, amount;
	char					date_text[1024];
	char					*match_ref, *match_desc;

	if (parent == NULL || report == NULL || settings == NULL || scratch == NULL || title == NULL)
		return;

	file = analysis_get_file(parent);
	if (file == NULL)
		return;

	/* Read the include list. */

	if (settings->from_count == 0 && settings->to_count == 0) {
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL | ACCOUNT_IN, ANALYSIS_DATA_FROM, NULL, 1);
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL | ACCOUNT_OUT, ANALYSIS_DATA_TO, NULL, 1);
	} else {
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL | ACCOUNT_IN, ANALYSIS_DATA_FROM,
				settings->from, settings->from_count);
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL | ACCOUNT_OUT, ANALYSIS_DATA_TO,
				settings->to, settings->to_count);
	}

	min_amount = settings->amount_min;
	max_amount = settings->amount_max;

	match_ref = (*(settings->ref) == '\0') ? NULL : settings->ref;
	match_desc = (*(settings->desc) == '\0') ? NULL : settings->desc;

	/* Output report heading */

	report_write_line(report, 0, title);

	/* Read the date settings and output their details. */

	analysis_find_date_range(parent, &start_date, &end_date, settings->date_from, settings->date_to, settings->budget, report);

	total_days = date_count_days(start_date, end_date);

	/* Initialise the heading remainder values for the report. */

	analysis_data_initialise_balances(scratch);

	/* Process the report time groups. */

	analysis_period_initialise(start_date, end_date, settings->group, settings->period, settings->period_unit, settings->lock);

	while (analysis_period_get_next_dates(&next_start, &next_end, date_text, sizeof(date_text))) {
		analysis_data_zero_totals(scratch);

		/* Scan through the transactions, adding the values up for those in range and outputting them to the screen. */

		found = 0;

		for (i = 0; i < transact_get_count(file); i++) {
			date = transact_get_date(file, i);
			from = transact_get_from(file, i);
			to = transact_get_to(file, i);
			amount = transact_get_amount(file, i);
		
			if ((next_start == NULL_DATE || date >= next_start) &&
					(next_end == NULL_DATE || date <= next_end) &&
					(analysis_data_test_account(scratch, from, ANALYSIS_DATA_FROM) ||
							analysis_data_test_account(scratch, to, ANALYSIS_DATA_TO)) &&
					((min_amount == NULL_CURRENCY) || (amount >= min_amount)) &&
					((max_amount == NULL_CURRENCY) || (amount <= max_amount)) &&
					((match_ref == NULL) || string_wildcard_compare(match_ref, transact_get_reference(file, i, NULL, 0), TRUE)) &&
					((match_desc == NULL) || string_wildcard_compare(match_desc, transact_get_description(file, i, NULL, 0), TRUE))) {
				if (found == 0) {
					report_write_line(report, 0, "");

					if (settings->group == TRUE) {
						stringbuild_reset();
						stringbuild_add_printf("\\u%s", date_text);
						stringbuild_report_line(report, 0);
					}

					if (settings->output_trans) {
						stringbuild_reset();
						stringbuild_add_message("TRHeadings");
						stringbuild_report_line(report, 1);
					}
				}

				found++;

				/* Update the totals and output the transaction to the report file. */

				analysis_data_add_transaction(scratch, i);

				if (settings->output_trans) {
					stringbuild_reset();
					stringbuild_add_printf("\\k\\v\\d\\r%d\\t\\v\\c",
							transact_get_transaction_number(i));
					stringbuild_add_date(date);
					stringbuild_add_printf("\\t\\v%s\\t\\v%s\\t\\v%s\\t\\v\\d\\r",
							account_get_name(file, from),
							account_get_name(file, to),
							transact_get_reference(file, i, NULL, 0));
					stringbuild_add_currency(amount, TRUE);
					stringbuild_add_printf("\\t\\v%s",
							transact_get_description(file, i, NULL, 0));
					stringbuild_report_line(report, 1);
				}
			}
		}

		/* Print the account summaries. */

		if (settings->output_accsummary && found > 0) {
			/* Summarise the accounts. */

			total = 0;

			/* Only output blank line if there are transactions above. */

			if (settings->output_trans)
				report_write_line(report, 0, "");

			stringbuild_reset();
			stringbuild_add_string("\\i");
			stringbuild_add_message("TRAccounts");
			stringbuild_report_line(report, 2);

			entries = account_get_list_length(file, ACCOUNT_FULL);

			for (i = 0; i < entries; i++) {
				if ((account = account_get_list_entry_account(file, ACCOUNT_FULL, i)) != NULL_ACCOUNT) {
					amount = analysis_data_get_total(scratch, account);

					if ((amount != 0) || settings->output_empty) {
						total += amount;

						stringbuild_reset();
						stringbuild_add_printf("\\k\\i%s\\t\\d\\r", account_get_name(file, account));
						stringbuild_add_currency(amount, TRUE);
						stringbuild_report_line(report, 2);
					}
				}
			}

			stringbuild_reset();
			stringbuild_add_string("\\i\\k\\b");
			stringbuild_add_message("TRTotal");
			stringbuild_add_string("\\t\\d\\r\\b");
			stringbuild_add_currency(total, TRUE);
			stringbuild_report_line(report, 2);
		}

		/* Print the transaction summaries. */

		if (settings->output_summary && found > 0) {
			/* Summarise the outgoings. */

			total = 0;

			/* Only output blank line if there is something above. */

			if (settings->output_trans || settings->output_accsummary)
				report_write_line(report, 0, "");

			stringbuild_reset();
			stringbuild_add_string("\\i");
			stringbuild_add_message("TROutgoings");
			if (settings->budget)
				stringbuild_add_message("TRSummExtra");
			stringbuild_report_line(report, 2);

			entries = account_get_list_length(file, ACCOUNT_OUT);

			for (i = 0; i < entries; i++) {
				if ((account = account_get_list_entry_account(file, ACCOUNT_OUT, i)) != NULL_ACCOUNT) {
					amount = analysis_data_get_total(scratch, account);

					if ((amount != 0) || settings->output_empty) {
						total += amount;

						stringbuild_reset();
						stringbuild_add_printf("\\i\\k%s\\t\\d\\r", account_get_name(file, account));
						stringbuild_add_currency(amount, TRUE);

						if (settings->budget) {
							period_days = date_count_days(next_start, next_end);
							period_limit = account_get_budget_amount(file, account) * period_days / total_days;

							stringbuild_add_string("\\t\\d\\r");
							stringbuild_add_currency(period_limit, TRUE);

							stringbuild_add_string("\\t\\d\\r");
							stringbuild_add_currency(period_limit - amount, TRUE);

							stringbuild_add_string("\\t\\d\\r");
							stringbuild_add_currency(analysis_data_update_balance(scratch, amount), TRUE);
						}

						stringbuild_report_line(report, 2);
					}
				}
			}

			stringbuild_reset();
			stringbuild_add_string("\\i\\k\\b");
			stringbuild_add_message("TRTotal");
			stringbuild_add_string("\\t\\d\\r\\b");
			stringbuild_add_currency(total, TRUE);
			stringbuild_report_line(report, 2);

			/* Summarise the incomings. */

			total = 0;

			report_write_line(report, 0, "");

			stringbuild_reset();
			stringbuild_add_string("\\i");
			stringbuild_add_message("TRIncomings");
			if (settings->budget)
				stringbuild_add_message("TRSummExtra");
			stringbuild_report_line(report, 2);

			entries = account_get_list_length(file, ACCOUNT_IN);

			for (i = 0; i < entries; i++) {
				if ((account = account_get_list_entry_account(file, ACCOUNT_IN, i)) != NULL_ACCOUNT) {
					amount = analysis_data_get_total(scratch, account);

					if ((amount != 0) || settings->output_empty) {
						total += amount;

						stringbuild_reset();
						stringbuild_add_printf("\\i\\k%s\\t\\d\\r", account_get_name(file, account));
						stringbuild_add_currency(-amount, TRUE);

						if (settings->budget) {
							period_days = date_count_days(next_start, next_end);
							period_limit = account_get_budget_amount(file, account) * period_days / total_days;

							stringbuild_add_string("\\t\\d\\r");
							stringbuild_add_currency(period_limit, TRUE);

							stringbuild_add_string("\\t\\d\\r");
							stringbuild_add_currency(period_limit - amount, TRUE);

							stringbuild_add_string("\\t\\d\\r");
							stringbuild_add_currency(analysis_data_update_balance(scratch, amount), TRUE);
						}

						stringbuild_report_line(report, 2);
					}
				}
			}

			stringbuild_reset();
			stringbuild_add_string("\\i\\k\\b");
			stringbuild_add_message("TRTotal");
			stringbuild_add_string("\\t\\d\\r\\b");
			stringbuild_add_currency(-total, TRUE);
			stringbuild_report_line(report, 2);
		}
	}
}


/**
 * Remove any references to a report template.
 * 
 * \param *parent	The analysis instance being updated.
 * \param template	The template to be removed.
 */

static void analysis_transaction_remove_template(struct analysis_block *parent, template_t template)
{
	analysis_dialogue_remove_template(analysis_transaction_dialogue, parent, template);
}


/**
 * Remove any references to an account if it appears within a
 * transaction report template.
 *
 * \param *report	The transaction report to be processed.
 * \param account	The account to be removed.
 */

static void analysis_transaction_remove_account(void *report, acct_t account)
{
	struct analysis_transaction_report *rep = report;

	if (rep == NULL)
		return;

	analysis_template_remove_account_from_list(account, rep->from, &(rep->from_count));
	analysis_template_remove_account_from_list(account, rep->to, &(rep->to_count));
}


/**
 * Copy a Transaction Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_transaction_copy_template(void *to, void *from)
{
	struct analysis_transaction_report	*a = from, *b = to;
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

	b->from_count = a->from_count;
	for (i = 0; i < a->from_count; i++)
		b->from[i] = a->from[i];

	b->to_count = a->to_count;
	for (i = 0; i < a->to_count; i++)
		b->to[i] = a->to[i];

	string_copy(b->ref, a->ref, TRANSACT_REF_FIELD_LEN);
	string_copy(b->desc, a->desc, TRANSACT_DESCRIPT_FIELD_LEN);
	b->amount_min = a->amount_min;
	b->amount_max = a->amount_max;

	b->output_trans = a->output_trans;
	b->output_summary = a->output_summary;
	b->output_accsummary = a->output_accsummary;
	b->output_empty = a->output_empty;
}


/**
 * Write a template to a saved cashbook file.
 *
 * \param *block		The saved report template block to write.
 * \param *out			The outgoing file handle.
 * \param *name			The name of the template.
 */

static void analysis_transaction_write_file_block(void *block, FILE *out, char *name)
{
	char					buffer[FILING_MAX_FILE_LINE_LEN];
	struct analysis_transaction_report	*template = block;

	if (out == NULL || template == NULL)
		return;

	fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
			REPORT_TYPE_TRANSACTION,
			template->date_from,
			template->date_to,
			template->budget,
			template->group,
			template->period,
			template->period_unit,
			template->lock,
			template->output_trans,
			template->output_summary,
			template->output_accsummary);

	if (name != NULL && *name != '\0')
		config_write_token_pair(out, "Name", name);

	if (template->from_count > 0) {
		analysis_template_account_list_to_hex(buffer, FILING_MAX_FILE_LINE_LEN,
				template->from, template->from_count);
		config_write_token_pair(out, "From", buffer);
	}

	if (template->to_count > 0) {
		analysis_template_account_list_to_hex(buffer, FILING_MAX_FILE_LINE_LEN,
				template->to, template->to_count);
		config_write_token_pair(out, "To", buffer);
	}

	if (*(template->ref) != '\0')
		config_write_token_pair(out, "Ref", template->ref);

	if (template->amount_min != NULL_CURRENCY ||
			template->amount_max != NULL_CURRENCY) {
		string_printf(buffer, FILING_MAX_FILE_LINE_LEN, "%x,%x", template->amount_min, template->amount_max);
		config_write_token_pair(out, "Amount", buffer);
	}

	if (*(template->desc) != '\0')
		config_write_token_pair(out, "Desc", template->desc);

	if (template->output_empty != FALSE)
		config_write_token_pair(out, "IncEmpty", config_return_opt_string(template->output_empty));
}


/**
 * Process a token from the saved report template section of a saved
 * cashbook file.
 *
 * \param *block		The saved report template block to populate.
 * \param *in			The incoming file handle.
 */

static void analysis_transaction_process_file_token(void *block, struct filing_block *in)
{
	struct analysis_transaction_report *template = block;

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
		template->output_trans = filing_get_opt_field(in);
		template->output_summary = filing_get_opt_field(in);
		template->output_accsummary = filing_get_opt_field(in);
		template->amount_min = NULL_CURRENCY;
		template->amount_max = NULL_CURRENCY;
		template->from_count = 0;
		template->to_count = 0;
		*(template->ref) = '\0';
		*(template->desc) = '\0';
	} else if (filing_test_token(in, "From")) {
		template->from_count = analysis_template_account_hex_to_list(filing_get_text_value(in, NULL, 0), template->from);
	} else if (filing_test_token(in, "To")) {
		template->to_count = analysis_template_account_hex_to_list(filing_get_text_value(in, NULL, 0), template->to);
	} else if (filing_test_token(in, "Ref")) {
		filing_get_text_value(in, template->ref, TRANSACT_REF_FIELD_LEN);
	} else if (filing_test_token(in, "Amount")) {
		template->amount_min = currency_get_currency_field(in);
		template->amount_max = currency_get_currency_field(in);
	} else if (filing_test_token(in, "Desc")) {
		filing_get_text_value(in, template->desc, TRANSACT_DESCRIPT_FIELD_LEN);
	} else if (filing_test_token(in, "IncEmpty")) {
		template->output_empty = filing_get_opt_value(in);
	} else {
		filing_set_status(in, FILING_STATUS_UNEXPECTED);
	}
}

