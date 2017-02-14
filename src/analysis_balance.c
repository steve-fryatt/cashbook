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
 * \file: analysis_balance.c
 *
 * Analysis Balance Report implementation.
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
#include "analysis_balance.h"

#include "account.h"
#include "account_menu.h"
#include "analysis.h"
#include "analysis_dialogue.h"
#include "analysis_lookup.h"
#include "analysis_template_save.h"
#include "budget.h"
#include "caret.h"
#include "currency.h"
#include "date.h"
#include "file.h"
#include "flexutils.h"
#include "report.h"
#include "transact.h"

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

static struct analysis_dialogue_block	*analysis_balance_dialogue = NULL;

static wimp_w			analysis_balance_window = NULL;			/**< The handle of the Balance Report window.					*/
static struct analysis_block	*analysis_balance_instance = NULL;		/**< The instance currently owning the report dialogue.				*/
//static struct file_block	*analysis_balance_file = NULL;			/**< The file currently owning the balance dialogue.				*/
static osbool			analysis_balance_restore = FALSE;		/**< The restore setting for the current Balance dialogue.			*/
static struct balance_rep	analysis_balance_settings;			/**< Saved initial settings for the Balance dialogue.				*/
static template_t		analysis_balance_template = NULL_TEMPLATE;	/**< The template which applies to the Balance dialogue.			*/

/* Static Function Prototypes. */

static void		analysis_balance_click_handler(wimp_pointer *pointer);
static osbool		analysis_balance_keypress_handler(wimp_key *key);
static void		analysis_refresh_balance_window(void);
static void		analysis_fill_balance_window(struct analysis_block *parent, osbool restore);
static osbool		analysis_process_balance_window(void);
static osbool		analysis_delete_balance_window(void);
static void		analysis_generate_balance_report(struct file_block *file);



/**
 * Initialise the Balance analysis report module.
 */

void analysis_balance_initialise(void)
{
	analysis_balance_window = templates_create_window("BalanceRep");
	ihelp_add_window(analysis_balance_window, "BalanceRep", NULL);
	event_add_window_mouse_event(analysis_balance_window, analysis_balance_click_handler);
	event_add_window_key_event(analysis_balance_window, analysis_balance_keypress_handler);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PDAYS, TRUE);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS, TRUE);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PYEARS, TRUE);
	analysis_balance_dialogue = analysis_dialogue_initialise("BalanceRep", "BalanceRep");
}


/**
 * Construct new balance report data block for a file, and return a pointer
 * to the resulting block. The block will be allocated with heap_alloc(), and
 * should be freed after use with heap_free().
 *
 * \param *parent	Pointer to the parent analysis instance.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct balance_rep *analysis_balance_create_instance(struct analysis_block *parent)
{
	struct balance_rep	*new;

	new = heap_alloc(sizeof(struct balance_rep));
	if (new == NULL)
		return NULL;

	new->parent = parent;

	new->date_from = NULL_DATE;
	new->date_to = NULL_DATE;
	new->budget = FALSE;
	new->group = FALSE;
	new->period = 1;
	new->period_unit = DATE_PERIOD_MONTHS;
	new->lock = FALSE;
	new->accounts_count = 0;
	new->incoming_count = 0;
	new->outgoing_count = 0;
	new->tabular = FALSE;

	return new;
}


/**
 * Delete a balance report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_balance_delete_instance(struct balance_rep *report)
{
	if (report != NULL)
		return;

	if ((report->parent == analysis_balance_instance) && windows_get_open(analysis_balance_window))
		close_dialogue_with_caret(analysis_balance_window);

	heap_free(report);
}


/**
 * Open the Balance Report dialogue box.
 *
 * \param *parent	The paranet analysis instance owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_balance_open_window(struct analysis_block *parent, wimp_pointer *ptr, int template, osbool restore)
{
	struct analysis_block		*template_block;
	union analysis_report_block	*content;

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

	template_block = analysis_get_template(parent, template);
	content = analysis_get_template_contents(template_block, REPORT_TYPE_BALANCE);

	if (content != NULL) {
		analysis_copy_balance_template(&(analysis_balance_settings), &(content->balance));
		analysis_balance_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_balance_window),
				windows_get_indirected_title_length(analysis_balance_window),
				analysis_get_template_name(template_block, NULL, 0), NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_balance_template(&(analysis_balance_settings), parent->balance_rep);
		analysis_balance_template = NULL_TEMPLATE;

		msgs_lookup("BalRepTitle", windows_get_indirected_title_addr(analysis_balance_window),
				windows_get_indirected_title_length(analysis_balance_window));
	}

	icons_set_deleted(analysis_balance_window, ANALYSIS_BALANCE_DELETE, content != NULL);
	icons_set_deleted(analysis_balance_window, ANALYSIS_BALANCE_RENAME, content != NULL);

	/* Set the window contents up. */

	analysis_fill_balance_window(parent, restore);

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_balance_instance = parent;
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
			analysis_template_save_force_rename_close(analysis_balance_file, analysis_balance_template);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_balance_window();
		}
		break;

	case ANALYSIS_BALANCE_OK:
		if (analysis_process_balance_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_balance_window);
			analysis_template_save_force_rename_close(analysis_balance_file, analysis_balance_template);
		}
		break;

	case ANALYSIS_BALANCE_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_balance_window())
			close_dialogue_with_caret(analysis_balance_window);
		break;

	case ANALYSIS_BALANCE_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_balance_template >= 0 && analysis_balance_template < analysis_balance_file->analysis->saved_report_count)
			analysis_template_save_open_rename_window(analysis_balance_file, analysis_balance_template, pointer);
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
			analysis_lookup_open_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		break;

	case ANALYSIS_BALANCE_INCOMINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_lookup_open_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		break;

	case ANALYSIS_BALANCE_OUTGOINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_lookup_open_window(analysis_balance_file, analysis_balance_window,
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
			analysis_template_save_force_rename_close(analysis_balance_file, analysis_balance_template);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_balance_window);
		analysis_template_save_force_rename_close(analysis_balance_file, analysis_balance_template);
		break;

	case wimp_KEY_F1:
		if (key->i == ANALYSIS_BALANCE_ACCOUNTS)
			analysis_lookup_open_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		else if (key->i == ANALYSIS_BALANCE_INCOMING)
			analysis_lookup_open_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		else if (key->i == ANALYSIS_BALANCE_OUTGOING)
			analysis_lookup_open_window(analysis_balance_file, analysis_balance_window,
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

static void analysis_fill_balance_window(struct analysis_block *parent, osbool restore)
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

		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_ACCOUNTS),
				analysis_balance_settings.accounts, analysis_balance_settings.accounts_count);
		analysis_account_list_to_idents(parent,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_INCOMING),
				analysis_balance_settings.incoming, analysis_balance_settings.incoming_count);
		analysis_account_list_to_idents(parent,
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
	if (analysis_balance_template >= 0 && analysis_balance_template < analysis_balance_file->analysis->saved_report_count &&
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
	osbool			group, lock, tabular;
	int			i, acc, items, unit, period, total;
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
		strcpy(analysis_report_template.name, file->analysis->saved_reports[analysis_balance_template].name);
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

	analysis_period_initialise(start_date, end_date, period, unit, lock);

	while (analysis_period_get_next_dates(&next_start, &next_end, date_text, sizeof(date_text))) {
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
 * Remove any references to an account if it appears within a
 * balance report template.
 *
 * \param *report	The transaction report to be processed.
 * \param account	The account to be removed.
 */

void analysis_balance_remove_account(struct balance_rep *report, acct_t account)
{
	if (report == NULL)
		return;

	analysis_remove_account_from_list(account, report->accounts, &(report->accounts_count));
	analysis_remove_account_from_list(account, report->incoming, &(report->incoming_count));
	analysis_remove_account_from_list(account, report->outgoing, &(report->outgoing_count));
}


/**
 * Remove any references to a report template.
 * 
 * \param template	The template to be removed.
 */

void analysis_balance_remove_template(template_t template)
{
	if (analysis_balance_template > template)
		analysis_balance_template--;
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