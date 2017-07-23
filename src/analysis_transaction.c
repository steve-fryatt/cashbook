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
 * \file: analysis_transaction.c
 *
 * Analysis Transaction Report implementation.
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
#include "analysis_transaction.h"

#include "account.h"
#include "account_menu.h"
#include "analysis.h"
#include "analysis_dialogue.h"
#include "analysis_lookup.h"
#include "analysis_template.h"
#include "analysis_template_save.h"
#include "budget.h"
#include "caret.h"
#include "currency.h"
#include "date.h"
#include "file.h"
#include "flexutils.h"
#include "report.h"
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


/**
 * Transaction Report dialogue.
 */

struct analysis_transaction_report {
	/**
	 * The parent analysis report instance.
	 */

	struct analysis_block		*parent;

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
};


static struct analysis_dialogue_block	*analysis_transaction_dialogue = NULL;

static wimp_w			analysis_transaction_window = NULL;		/**< The handle of the Transaction Report window.				*/
static struct analysis_block	*analysis_transcation_instance = NULL;		/**< The instance currently owning the report dialogue.				*/
//static struct file_block	*analysis_transaction_file = NULL;		/**< The file currently owning the transaction dialogue.			*/
static osbool			analysis_transaction_restore = FALSE;		/**< The restore setting for the current Transaction dialogue.			*/
static struct trans_rep		analysis_transaction_settings;			/**< Saved initial settings for the Transaction dialogue.			*/
static template_t		analysis_transaction_template = NULL_TEMPLATE;	/**< The template which applies to the Transaction dialogue.			*/

/* Static Function Prototypes. */
#if 0
static void		analysis_transaction_click_handler(wimp_pointer *pointer);
static osbool		analysis_transaction_keypress_handler(wimp_key *key);
static void		analysis_refresh_transaction_window(void);
static void		analysis_fill_transaction_window(struct file_block *file, osbool restore);
static osbool		analysis_process_transaction_window(void);
static osbool		analysis_delete_transaction_window(void);
static void		analysis_generate_transaction_report(struct file_block *file);
#endif

static void analysis_transaction_copy_template(struct trans_rep *to, struct trans_rep *from);
static void analysis_transaction_write_file_block(struct file_block *file, void *block, FILE *out, char *name);
static void analysis_transaction_process_file_token(struct file_block *file, void *block, struct filing_block *in);


static struct analysis_report_details analysis_transaction_details = {
	analysis_transaction_process_file_token,
	analysis_transaction_write_file_block,
	analysis_transaction_copy_template
};

/**
 * Initialise the Transaction analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_transaction_initialise(void)
{
	analysis_template_set_block_size(sizeof(struct analysis_transaction_report));
	analysis_transaction_window = templates_create_window("TransRep");
	ihelp_add_window(analysis_transaction_window, "TransRep", NULL);
//	event_add_window_mouse_event(analysis_transaction_window, analysis_transaction_click_handler);
//	event_add_window_key_event(analysis_transaction_window, analysis_transaction_keypress_handler);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PDAYS, TRUE);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS, TRUE);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PYEARS, TRUE);
	analysis_transaction_dialogue = analysis_dialogue_initialise("TransRep", "TransRep");

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

struct analysis_transaction_report *analysis_transaction_create_instance(struct analysis_block *parent)
{
	struct analysis_transaction_report	*new;

	new = heap_alloc(sizeof(struct analysis_transaction_report));
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
	new->from_count = 0;
	new->to_count = 0;
	*(new->ref) = '\0';
	*(new->desc) = '\0';
	new->amount_min = NULL_CURRENCY;
	new->amount_max = NULL_CURRENCY;
	new->output_trans = TRUE;
	new->output_summary = TRUE;
	new->output_accsummary = TRUE;

	return new;
}


/**
 * Delete a transaction report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_transaction_delete_instance(struct analysis_transaction_report *report)
{
	if (report == NULL)
		return;

	if ((report->parent == analysis_transcation_instance) && windows_get_open(analysis_transaction_window))
		close_dialogue_with_caret(analysis_transaction_window);

	heap_free(report);
}











#if 0




/**
 * Open the Transaction Report dialogue box.
 *
 * \param *parent	The paranet analysis instance owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_transaction_open_window(struct analysis_block *parent, wimp_pointer *ptr, int template, osbool restore)
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

	template_mode = (template >= 0 && template < file->analysis->saved_report_count);

	if (template_mode) {
		analysis_copy_transaction_template(&(analysis_transaction_settings), &(file->analysis->saved_reports[template].data.transaction));
		analysis_transaction_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_transaction_window),
				windows_get_indirected_title_length(analysis_transaction_window),
				file->analysis->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_transaction_template(&(analysis_transaction_settings), file->trans_rep);
		analysis_transaction_template = NULL_TEMPLATE;

		msgs_lookup("TrnRepTitle", windows_get_indirected_title_addr(analysis_transaction_window),
				windows_get_indirected_title_length(analysis_transaction_window));
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
			analysis_template_save_force_rename_close(analysis_transaction_file, analysis_transaction_template);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_transaction_window();
		}
		break;

	case ANALYSIS_TRANS_OK:
		if (analysis_process_transaction_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_transaction_window);
			analysis_template_save_force_rename_close(analysis_transaction_file, analysis_transaction_template);
		}
		break;

	case ANALYSIS_TRANS_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_transaction_window())
			close_dialogue_with_caret(analysis_transaction_window);
		break;

	case ANALYSIS_TRANS_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_transaction_template >= 0 && analysis_transaction_template < analysis_transaction_file->analysis->saved_report_count)
			analysis_template_save_open_rename_window(analysis_transaction_file, analysis_transaction_template, pointer);
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
			analysis_lookup_open_window(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		break;

	case ANALYSIS_TRANS_TOSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_lookup_open_window(analysis_transaction_file, analysis_transaction_window,
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
			analysis_template_save_force_rename_close(analysis_transaction_file, analysis_transaction_template);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_transaction_window);
		analysis_template_save_force_rename_close(analysis_transaction_file, analysis_transaction_template);
		break;

	case wimp_KEY_F1:
		if (key->i == ANALYSIS_TRANS_FROMSPEC)
			analysis_lookup_open_window(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		else if (key->i == ANALYSIS_TRANS_TOSPEC)
			analysis_lookup_open_window(analysis_transaction_file, analysis_transaction_window,
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
	if (analysis_transaction_template >= 0 && analysis_transaction_template < analysis_transaction_file->analysis->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 3) {
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
	osbool			group, lock, output_trans, output_summary, output_accsummary;
	int			i, found, total, unit, period,
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
		strcpy(analysis_report_template.name, file->analysis->saved_reports[analysis_transaction_template].name);
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

	analysis_period_initialise(start_date, end_date, period, unit, lock);

	/* Initialise the heading remainder values for the report. */

	for (i = 0; i < account_get_count(file); i++) {
		enum account_type type = account_get_type(file, i);
	
		if (type & ACCOUNT_OUT)
			data[i].report_balance = account_get_budget_amount(file, i);
		else if (type & ACCOUNT_IN)
			data[i].report_balance = -account_get_budget_amount(file, i);
	}

	while (analysis_period_get_next_dates(&next_start, &next_end, date_text, sizeof(date_text))) {

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
 * Remove any references to an account if it appears within a
 * transaction report template.
 *
 * \param *report	The transaction report to be processed.
 * \param account	The account to be removed.
 */

void analysis_transaction_remove_account(struct trans_rep *report, acct_t account)
{
	if (report == NULL)
		return;

	analysis_remove_account_from_list(account, report->from, &(report->from_count));
	analysis_remove_account_from_list(account, report->to, &(report->to_count));
}


/**
 * Remove any references to a report template.
 * 
 * \param template	The template to be removed.
 */

void analysis_transaction_remove_template(template_t template)
{
	if (analysis_transaction_template > template)
		analysis_transaction_template--;
}






#endif


/**
 * Copy a Transaction Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_transaction_copy_template(struct trans_rep *to, struct trans_rep *from)
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

	strcpy(b->ref, a->ref);
	strcpy(b->desc, a->desc);
	b->amount_min = a->amount_min;
	b->amount_max = a->amount_max;

	b->output_trans = a->output_trans;
	b->output_summary = a->output_summary;
	b->output_accsummary = a->output_accsummary;
}


/**
 * Write a template to a saved cashbook file.
 *
 * \param *file			The handle of the file owning the data.
 * \param *block		The saved report template block to write.
 * \param *out			The outgoing file handle.
 * \param *name			The name of the template.
 */

static void analysis_transaction_write_file_block(struct file_block *file, void *block, FILE *out, char *name)
{
	char					buffer[FILING_MAX_FILE_LINE_LEN];
	struct analysis_transaction_report	*template = block;

	if (out == NULL || template == NULL || file == NULL)
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
		analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
				template->from, template->from_count);
		config_write_token_pair(out, "From", buffer);
	}

	if (template->to_count > 0) {
		analysis_account_list_to_hex(file, buffer, FILING_MAX_FILE_LINE_LEN,
				template->to, template->to_count);
		config_write_token_pair(out, "To", buffer);
	}

	if (*(template->ref) != '\0')
		config_write_token_pair(out, "Ref", template->ref);

	if (template->amount_min != NULL_CURRENCY ||
			template->amount_max != NULL_CURRENCY) {
		sprintf(buffer, "%x,%x", template->amount_min, template->amount_max);
		config_write_token_pair(out, "Amount", buffer);
	}

	if (*(template->desc) != '\0')
		config_write_token_pair(out, "Desc", template->desc);
}


/**
 * Process a token from the saved report template section of a saved
 * cashbook file.
 *
 * \param *file			The handle of the file owning the data.
 * \param *block		The saved report template block to populate.
 * \param *in			The incoming file handle.
 */

static void analysis_transaction_process_file_token(struct file_block *file, void *block, struct filing_block *in)
{
	struct analysis_transaction_report *template = block;

	if (in == NULL || template == NULL || file == NULL)
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
		template->from_count = analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), template->from);
	} else if (filing_test_token(in, "To")) {
		template->to_count = analysis_account_hex_to_list(file, filing_get_text_value(in, NULL, 0), template->to);
	} else if (filing_test_token(in, "Ref")) {
		filing_get_text_value(in, template->ref, TRANSACT_REF_FIELD_LEN);
	} else if (filing_test_token(in, "Amount")) {
		template->amount_min = currency_get_currency_field(in);
		template->amount_max = currency_get_currency_field(in);
	} else if (filing_test_token(in, "Desc")) {
		filing_get_text_value(in, template->desc, TRANSACT_DESCRIPT_FIELD_LEN);
	} else {
		filing_set_status(in, FILING_STATUS_UNEXPECTED);
	}
}

