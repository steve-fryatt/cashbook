/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: analysis_unreconciled.c
 *
 * Analysis Unreconciled Report implementation.
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
#include "analysis_unreconciled.h"

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


/**
 * Unreconciled Report Template structure.
 */

struct analysis_unreconciled_report {
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
};

/**
 * Unreconciled Report Instance data.
 */

struct analysis_unreconciled_block {
	/**
	 * The parent analysis report instance.
	 */

	struct analysis_block			*parent;

	/**
	 * The saved instance report settings.
	 */

	struct analysis_unreconciled_report	saved;
};


/**
 * The dialogue instance used for this report.
 */

static struct analysis_dialogue_block	*analysis_unreconciled_dialogue = NULL;

/* Static Function Prototypes. */

static void *analysis_unreconciled_create_instance(struct analysis_block *parent);
static void analysis_unreconciled_delete_instance(void *instance);
static void analysis_unreconciled_open_window(void *instance, wimp_pointer *pointer, template_t template, osbool restore);
static void analysis_unreconciled_rename_template(struct analysis_block *parent, template_t template, char *name);
static void analysis_unreconciled_fill_window(struct analysis_block *parent, wimp_w window, void *block);
static void analysis_unreconciled_process_window(struct analysis_block *parent, wimp_w window, void *block);
static void analysis_unreconciled_generate(struct analysis_block *parent, void *template, struct report *report, struct analysis_data_block *scratch, char *title);
static void analysis_unreconciled_remove_template(struct analysis_block *parent, template_t template);
static void analysis_unreconciled_remove_account(void *report, acct_t account);
static void analysis_unreconciled_copy_template(void *to, void *from);
static void analysis_unreconciled_write_file_block(void *block, FILE *out, char *name);
static void analysis_unreconciled_process_file_token(void *block, struct filing_block *in);

/* The Unreconciled Report definition. */

static struct analysis_report_details analysis_unreconciled_details = {
	"URWinT", "URTitle",
	analysis_unreconciled_create_instance,
	analysis_unreconciled_delete_instance,
	analysis_unreconciled_open_window,
	analysis_unreconciled_fill_window,
	analysis_unreconciled_process_window,
	analysis_unreconciled_generate,
	analysis_unreconciled_process_file_token,
	analysis_unreconciled_write_file_block,
	analysis_unreconciled_copy_template,
	analysis_unreconciled_rename_template,
	analysis_unreconciled_remove_account,
	analysis_unreconciled_remove_template
};

/* The Unreconciled Report Dialogue Icon Details. */

static struct dialogue_icon analysis_unreconciled_icon_list[] = {
	{DIALOGUE_ICON_OK,									ANALYSIS_UNREC_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,									ANALYSIS_UNREC_CANCEL,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_ANALYSIS_DELETE,					ANALYSIS_UNREC_DELETE,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_ANALYSIS_RENAME,					ANALYSIS_UNREC_RENAME,		DIALOGUE_NO_ICON},

	/* Budget Group. */

	{DIALOGUE_ICON_SHADE_TARGET,								ANALYSIS_UNREC_BUDGET,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_SHADE_ON,								ANALYSIS_UNREC_DATEFROMTXT,	ANALYSIS_UNREC_BUDGET},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_REFRESH,					ANALYSIS_UNREC_DATEFROM,	ANALYSIS_UNREC_BUDGET},
	{DIALOGUE_ICON_SHADE_ON,								ANALYSIS_UNREC_DATETOTXT,	ANALYSIS_UNREC_BUDGET},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_REFRESH,					ANALYSIS_UNREC_DATETO,		ANALYSIS_UNREC_BUDGET},

	/* Group Period. */

	{DIALOGUE_ICON_SHADE_TARGET,								ANALYSIS_UNREC_GROUP,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_REFRESH,					ANALYSIS_UNREC_PERIOD,		ANALYSIS_UNREC_GROUP},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPDATE},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPACC},
	{DIALOGUE_ICON_SHADE_OFF,								ANALYSIS_UNREC_PTEXT,		ANALYSIS_UNREC_GROUP},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPDATE},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPACC},
	{DIALOGUE_ICON_SHADE_OFF,								ANALYSIS_UNREC_LOCK,		ANALYSIS_UNREC_GROUP},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPDATE},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPACC},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_RADIO,						ANALYSIS_UNREC_PDAYS,		ANALYSIS_UNREC_GROUP},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPDATE},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPACC},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_RADIO,						ANALYSIS_UNREC_PMONTHS,		ANALYSIS_UNREC_GROUP},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPDATE},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPACC},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_RADIO,						ANALYSIS_UNREC_PYEARS,		ANALYSIS_UNREC_GROUP},
	{DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPDATE},
	{DIALOGUE_ICON_SHADE_ON | DIALOGUE_ICON_SHADE_OR,					DIALOGUE_NO_ICON,		ANALYSIS_UNREC_GROUPACC},

	/* Group Type. */

	{DIALOGUE_ICON_SHADE_TARGET | DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_RADIO_PASS,	ANALYSIS_UNREC_GROUPACC,	ANALYSIS_UNREC_GROUP},
	{DIALOGUE_ICON_SHADE_TARGET | DIALOGUE_ICON_SHADE_OFF | DIALOGUE_ICON_RADIO_PASS,	ANALYSIS_UNREC_GROUPDATE,	ANALYSIS_UNREC_GROUP},

	/* Account Fields. */

	{DIALOGUE_ICON_ACCOUNT_POPUP | DIALOGUE_ICON_TYPE_FROM | DIALOGUE_ICON_REFRESH,		ANALYSIS_UNREC_FROMSPEC,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACCOUNT_POPUP | DIALOGUE_ICON_TYPE_FROM,					ANALYSIS_UNREC_FROMSPECPOPUP,	ANALYSIS_UNREC_FROMSPEC},
	{DIALOGUE_ICON_ACCOUNT_POPUP | DIALOGUE_ICON_TYPE_TO | DIALOGUE_ICON_REFRESH,		ANALYSIS_UNREC_TOSPEC,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACCOUNT_POPUP | DIALOGUE_ICON_TYPE_TO,					ANALYSIS_UNREC_TOSPECPOPUP,	ANALYSIS_UNREC_TOSPEC},

	{DIALOGUE_ICON_END,									DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};

/* The Unreconciled Report Dialogue Definition. */

static struct analysis_dialogue_definition analysis_unreconciled_dialogue_definition = {
	REPORT_TYPE_UNRECONCILED,
	sizeof(struct analysis_unreconciled_report),
	"UrcRepTitle",
	{
		"UnrecRep",
		"UnrecRep",
		analysis_unreconciled_icon_list,
		DIALOGUE_GROUP_ANALYSIS,
		DIALOGUE_FLAGS_TAKE_FOCUS,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	}
};


/**
 * Initialise the Unreconciled Transactions analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_unreconciled_initialise(void)
{
	analysis_template_set_block_size(analysis_unreconciled_dialogue_definition.block_size);
	analysis_unreconciled_dialogue = analysis_dialogue_initialise(&analysis_unreconciled_dialogue_definition);

	return &analysis_unreconciled_details;
}


/**
 * Construct new unreconciled report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \param *parent	Pointer to the parent analysis instance.
 * \return		Pointer to the new data block, or NULL on error.
 */

static void *analysis_unreconciled_create_instance(struct analysis_block *parent)
{
	struct analysis_unreconciled_block	*new;

	new = heap_alloc(sizeof(struct analysis_unreconciled_block));
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

	return new;
}


/**
 * Delete an unreconciled report data block.
 *
 * \param *instance	Pointer to the report to delete.
 */

static void analysis_unreconciled_delete_instance(void *instance)
{
	struct analysis_unreconciled_block *report = instance;

	if (report == NULL)
		return;

	heap_free(report);
}


/**
 * Open the Unreconciled Report dialogue box.
 *
 * \param *instance	The unreconciled report instance to own the dialogue.
 * \param *pointer	The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

static void analysis_unreconciled_open_window(void *instance, wimp_pointer *pointer, template_t template, osbool restore)
{
	struct analysis_unreconciled_block *report = instance;

	if (report == NULL)
		return;

	analysis_dialogue_open(analysis_unreconciled_dialogue, report, report->parent, pointer, template, &(report->saved), restore);
}


/**
 * Handle the user renaming templates.
 *
 * \param *parent	The parent analysis report instance for the rename.
 * \param template	The template being renamed.
 * \param *name		The new name for the report.
 */

static void analysis_unreconciled_rename_template(struct analysis_block *parent, template_t template, char *name)
{
	if (parent == NULL || template == NULL_TEMPLATE || name == NULL)
		return;

	analysis_dialogue_rename_template(analysis_unreconciled_dialogue, parent, template, name);
}


/**
 * Fill the Unreconciled window with values.
 *
 * \param *parent		The parent analysis instance.
 * \param window		The handle of the window to be processed.
 * \param *block		The template data to put into the window, or
 *				NULL to use the defaults.
 */

static void analysis_unreconciled_fill_window(struct analysis_block *parent, wimp_w window, void *block)
{
	struct analysis_unreconciled_report *template = block;

	if (parent == NULL || window == NULL)
		return;

	if (template == NULL) {
		/* Set the period icons. */

		*icons_get_indirected_text_addr(window, ANALYSIS_UNREC_DATEFROM) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_UNREC_DATETO) = '\0';

		icons_set_selected(window, ANALYSIS_UNREC_BUDGET, FALSE);

		/* Set the grouping icons. */

		icons_set_selected(window, ANALYSIS_UNREC_GROUP, FALSE);

		icons_set_selected(window, ANALYSIS_UNREC_GROUPACC, TRUE);
		icons_set_selected(window, ANALYSIS_UNREC_GROUPDATE, FALSE);

		icons_strncpy(window, ANALYSIS_UNREC_PERIOD, "1");
		icons_set_selected(window, ANALYSIS_UNREC_PDAYS, FALSE);
		icons_set_selected(window, ANALYSIS_UNREC_PMONTHS, TRUE);
		icons_set_selected(window, ANALYSIS_UNREC_PYEARS, FALSE);
		icons_set_selected(window, ANALYSIS_UNREC_LOCK, FALSE);

		/* Set the from and to spec fields. */

		*icons_get_indirected_text_addr(window, ANALYSIS_UNREC_FROMSPEC) = '\0';
		*icons_get_indirected_text_addr(window, ANALYSIS_UNREC_TOSPEC) = '\0';
	} else {
		/* Set the period icons. */

		date_convert_to_string(template->date_from,
				icons_get_indirected_text_addr(window, ANALYSIS_UNREC_DATEFROM),
				icons_get_indirected_text_length(window, ANALYSIS_UNREC_DATEFROM));
		date_convert_to_string(template->date_to,
				icons_get_indirected_text_addr(window, ANALYSIS_UNREC_DATETO),
				icons_get_indirected_text_length(window, ANALYSIS_UNREC_DATETO));

		icons_set_selected(window, ANALYSIS_UNREC_BUDGET, template->budget);

		/* Set the grouping icons. */

		icons_set_selected(window, ANALYSIS_UNREC_GROUP, template->group);

		icons_set_selected(window, ANALYSIS_UNREC_GROUPACC, template->period_unit == DATE_PERIOD_NONE);
		icons_set_selected(window, ANALYSIS_UNREC_GROUPDATE, template->period_unit != DATE_PERIOD_NONE);

		icons_printf(window, ANALYSIS_UNREC_PERIOD, "%d", template->period);
		icons_set_selected(window, ANALYSIS_UNREC_PDAYS, template->period_unit == DATE_PERIOD_DAYS);
		icons_set_selected(window, ANALYSIS_UNREC_PMONTHS,
				template->period_unit == DATE_PERIOD_MONTHS ||
				template->period_unit == DATE_PERIOD_NONE);
		icons_set_selected(window, ANALYSIS_UNREC_PYEARS, template->period_unit == DATE_PERIOD_YEARS);
		icons_set_selected(window, ANALYSIS_UNREC_LOCK, template->lock);

		/* Set the from and to spec fields. */

		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(window, ANALYSIS_UNREC_FROMSPEC),
				icons_get_indirected_text_length(window, ANALYSIS_UNREC_FROMSPEC),
				template->from, template->from_count);
		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(window, ANALYSIS_UNREC_TOSPEC),
				icons_get_indirected_text_length(window, ANALYSIS_UNREC_TOSPEC),
				template->to, template->to_count);
	}
}


/**
 * Process the contents of the Unreconciled window,.
 *
 *
 * \param *parent		The parent analysis instance.
 * \param window		The handle of the window to be processed.
 * \param *block		The template to store the contents in.
 */

static void analysis_unreconciled_process_window(struct analysis_block *parent, wimp_w window, void *block)
{
	struct analysis_unreconciled_report *template = block;

	if (parent == NULL || template == NULL || window == NULL)
		return;

	/* Read the date settings. */

	template->date_from =
			date_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_UNREC_DATEFROM), NULL_DATE, 0);
	template->date_to =
			date_convert_from_string(icons_get_indirected_text_addr(window, ANALYSIS_UNREC_DATETO), NULL_DATE, 0);
	template->budget = icons_get_selected(window, ANALYSIS_UNREC_BUDGET);

	/* Read the grouping settings. */

	template->group = icons_get_selected(window, ANALYSIS_UNREC_GROUP);
	template->period = atoi(icons_get_indirected_text_addr(window, ANALYSIS_UNREC_PERIOD));

	if (icons_get_selected(window, ANALYSIS_UNREC_GROUPACC))
		template->period_unit = DATE_PERIOD_NONE;
	else if (icons_get_selected(window, ANALYSIS_UNREC_PDAYS))
		template->period_unit = DATE_PERIOD_DAYS;
	else if (icons_get_selected(window, ANALYSIS_UNREC_PMONTHS))
		template->period_unit = DATE_PERIOD_MONTHS;
	else if (icons_get_selected(window, ANALYSIS_UNREC_PYEARS))
		template->period_unit = DATE_PERIOD_YEARS;
	else
		template->period_unit = DATE_PERIOD_MONTHS;

	template->lock = icons_get_selected(window, ANALYSIS_UNREC_LOCK);

	/* Read the account and heading settings. */

	template->from_count =
			analysis_account_idents_to_list(parent, ACCOUNT_FULL | ACCOUNT_IN,
			icons_get_indirected_text_addr(window, ANALYSIS_UNREC_FROMSPEC),
			template->from, ANALYSIS_ACC_LIST_LEN);
	template->to_count =
			analysis_account_idents_to_list(parent, ACCOUNT_FULL | ACCOUNT_OUT,
			icons_get_indirected_text_addr(window, ANALYSIS_UNREC_TOSPEC),
			template->to, ANALYSIS_ACC_LIST_LEN);
}


/**
 * Generate an unreconciled transaction report.
 *
 * \param *parent		The parent analysis instance.
 * \param *template		The template data to use for the report.
 * \param *report		The report to write to.
 * \param *scratch		The scratch space to use to build the report.
 * \param *title		Pointer to the report title.
 */

static void analysis_unreconciled_generate(struct analysis_block *parent, void *template, struct report *report, struct analysis_data_block *scratch, char *title)
{
	struct analysis_unreconciled_report	*settings = template;
	struct file_block			*file;

	int			acc, found, entries;
	char			date_text[1024], rec_char[REC_FIELD_LEN];
	date_t			start_date, end_date, next_start, next_end, date;
	tran_t			i;
	acct_t			from, to;
	enum transact_flags	flags;
	amt_t			amount, total_in, total_out;
	int			acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

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
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL | ACCOUNT_IN, ANALYSIS_DATA_FROM, settings->from, settings->from_count);
		analysis_data_set_flags_from_account_list(scratch, ACCOUNT_FULL | ACCOUNT_OUT, ANALYSIS_DATA_TO, settings->to, settings->to_count);
	}

	/* Start to output the report details. */

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	/* Output report heading */

	report_write_line(report, 0, title);

	/* Read the date settings and output their details. */

	analysis_find_date_range(parent, &start_date, &end_date, settings->date_from, settings->date_to, settings->budget, report);

	/* Run the report itself. */

	if (settings->group && settings->period_unit == DATE_PERIOD_NONE) {
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
					total_in = 0;
					total_out = 0;

					for (i = 0; i < transact_get_count(file); i++) {
						date = transact_get_date(file, i);
						from = transact_get_from(file, i);
						to = transact_get_to(file, i);
						flags = transact_get_flags(file, i);
						amount = transact_get_amount(file, i);

						if ((start_date == NULL_DATE || date >= start_date) &&
								(end_date == NULL_DATE || date <= end_date) &&
								(((from == acc) && analysis_data_test_account(scratch, acc, ANALYSIS_DATA_FROM) &&
								(flags & TRANS_REC_FROM) == 0) ||
								((to == acc) && analysis_data_test_account(scratch, acc, ANALYSIS_DATA_TO) &&
								(flags & TRANS_REC_TO) == 0))) {
							if (found == 0) {
								report_write_line(report, 0, "");

								if (settings->group == TRUE) {
									stringbuild_reset();
									stringbuild_add_printf("\\u%s", account_get_name(file, acc));
									stringbuild_report_line(report, 0);
								}

								stringbuild_reset();
								stringbuild_add_message("URHeadings");
								stringbuild_report_line(report, 1);
							}

							found++;

							if (from == acc)
								total_out -= amount;
							else if (to == acc)
								total_in += amount;

							/* Output the transaction to the report. */

							stringbuild_reset();

							stringbuild_add_printf("\\k\\v\\d\\r%d\\t\\v\\c",
									transact_get_transaction_number(i));
							stringbuild_add_date(date);
							stringbuild_add_string("\\t");
							if (flags & TRANS_REC_FROM)
								stringbuild_add_string(rec_char);
							stringbuild_add_printf("\\t\\v%s\\t",
									account_get_name(file, from));
							if (flags & TRANS_REC_TO)
								stringbuild_add_string(rec_char);
							stringbuild_add_printf("\\t\\v%s\\t\\v%s\\t\\v\\d\\r",
								account_get_name(file, to),
								transact_get_reference(file, i, NULL, 0));
							stringbuild_add_currency(amount, TRUE);
							stringbuild_add_printf("\\t\\v%s",
									transact_get_description(file, i, NULL, 0));

							stringbuild_report_line(report, 1);
						}
					}

					if (found != 0) {
						report_write_line(report, 2, "");

						stringbuild_reset();
						stringbuild_add_string("\\i");
						stringbuild_add_message("URTotalIn");
						stringbuild_add_string("\\t\\d\\r");
						stringbuild_add_currency(total_in, TRUE);
						stringbuild_report_line(report, 2);

						stringbuild_reset();
						stringbuild_add_string("\\i");
						stringbuild_add_message("URTotalOut");
						stringbuild_add_string("\\t\\d\\r");
						stringbuild_add_currency(total_out, TRUE);
						stringbuild_report_line(report, 2);

						stringbuild_reset();
						stringbuild_add_string("\\i");
						stringbuild_add_message("URTotal");
						stringbuild_add_string("\\t\\d\\r");
						stringbuild_add_currency(total_in + total_out, TRUE);
						stringbuild_report_line(report, 2);
					}
				}
			}
		}
	} else {
		/* We are either doing a grouped-by-date report, or not grouping at all.
		 *
		 * For each date period, run through the transactions and output any which fall within it.
		 */

		analysis_period_initialise(start_date, end_date, settings->group, settings->period, settings->period_unit, settings->lock);

		while (analysis_period_get_next_dates(&next_start, &next_end, date_text, sizeof(date_text))) {
			found = 0;

			for (i = 0; i < transact_get_count(file); i++) {
				date = transact_get_date(file, i);
				from = transact_get_from(file, i);
				to = transact_get_to(file, i);
				flags = transact_get_flags(file, i);
				amount = transact_get_amount(file, i);

				if ((next_start == NULL_DATE || date >= next_start) &&
						(next_end == NULL_DATE || date <= next_end) &&
						((((flags & TRANS_REC_FROM) == 0) && analysis_data_test_account(scratch, from, ANALYSIS_DATA_FROM)) ||
						(((flags & TRANS_REC_TO) == 0) && analysis_data_test_account(scratch, to, ANALYSIS_DATA_TO)))) {
					if (found == 0) {
						report_write_line(report, 0, "");

						if (settings->group == TRUE) {
							stringbuild_reset();
							stringbuild_add_printf("\\u%s", date_text);
							stringbuild_report_line(report, 0);
						}

						stringbuild_reset();
						stringbuild_add_message("URHeadings");
						stringbuild_report_line(report, 1);
					}

					found++;

					/* Output the transaction to the report. */

					stringbuild_reset();

					stringbuild_add_printf("\\k\\v\\d\\r%d\\t\\v\\c",
							transact_get_transaction_number(i));
					stringbuild_add_date(date);
					stringbuild_add_string("\\t");
					if (flags & TRANS_REC_FROM)
						stringbuild_add_string(rec_char);
					stringbuild_add_printf("\\t\\v%s\\t",
							account_get_name(file, from));
					if (flags & TRANS_REC_TO)
						stringbuild_add_string(rec_char);
					stringbuild_add_printf("\\t\\v%s\\t\\v%s\\t\\v\\d\\r",
						account_get_name(file, to),
						transact_get_reference(file, i, NULL, 0));
					stringbuild_add_currency(amount, TRUE);
					stringbuild_add_printf("\\t\\v%s",
							transact_get_description(file, i, NULL, 0));

					stringbuild_report_line(report, 1);
				}
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

static void analysis_unreconciled_remove_template(struct analysis_block *parent, template_t template)
{
	analysis_dialogue_remove_template(analysis_unreconciled_dialogue, parent, template);
}


/**
 * Remove any references to an account if it appears within an
 * unreconciled transaction report template.
 *
 * \param *report	The report template to be processed.
 * \param account	The account to be removed.
 */

static void analysis_unreconciled_remove_account(void *report, acct_t account)
{
	struct analysis_unreconciled_report *rep = report;

	if (rep == NULL)
		return;

	analysis_template_remove_account_from_list(account, rep->from, &(rep->from_count));
	analysis_template_remove_account_from_list(account, rep->to, &(rep->to_count));
}


/**
 * Copy a Unreconciled Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_unreconciled_copy_template(void *to, void *from)
{
	struct analysis_unreconciled_report	*a = from, *b = to;
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
}


/**
 * Write a template to a saved cashbook file.
 *
 * \param *block		The saved report template block to write.
 * \param *out			The outgoing file handle.
 * \param *name			The name of the template.
 */

static void analysis_unreconciled_write_file_block(void *block, FILE *out, char *name)
{
	char					buffer[FILING_MAX_FILE_LINE_LEN];
	struct analysis_unreconciled_report	*template = block;

	if (out == NULL || template == NULL)
		return;

	fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x\n",
			REPORT_TYPE_UNRECONCILED,
			template->date_from,
			template->date_to,
			template->budget,
			template->group,
			template->period,
			template->period_unit,
			template->lock);

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
}


/**
 * Process a token from the saved report template section of a saved
 * cashbook file.
 *
 * \param *block		The saved report template block to populate.
 * \param *in			The incoming file handle.
 */

static void analysis_unreconciled_process_file_token(void *block, struct filing_block *in)
{
	struct analysis_unreconciled_report *template = block;

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
		template->from_count = 0;
		template->to_count = 0;
	} else if (filing_test_token(in, "From")) {
		template->from_count = analysis_template_account_hex_to_list(filing_get_text_value(in, NULL, 0), template->from);
	} else if (filing_test_token(in, "To")) {
		template->to_count = analysis_template_account_hex_to_list(filing_get_text_value(in, NULL, 0), template->to);
	} else {
		filing_set_status(in, FILING_STATUS_UNEXPECTED);
	}
}

