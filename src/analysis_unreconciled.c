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
 * \file: analysis_unreconciled.c
 *
 * Analysis Unreconciled Report implementation.
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
#include "analysis_unreconciled.h"

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

/* Unreconciled Report dialogue. */

struct analysis_unreconciled_report {
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
};



static struct analysis_dialogue_block	*analysis_unreconciled_dialogue = NULL;

static wimp_w			analysis_unreconciled_window = NULL;		/**< The handle of the Unreconciled Report window.				*/
static struct analysis_block	*analysis_unreconciled_instance = NULL;		/**< The instance currently owning the report dialogue.				*/
//static struct file_block	*analysis_unreconciled_file = NULL;		/**< The file currently owning the unreconciled dialogue.			*/
static osbool			analysis_unreconciled_restore = FALSE;		/**< The restore setting for the current Unreconciled dialogue.			*/
static struct unrec_rep		analysis_unreconciled_settings;			/**< Saved initial settings for the Unreconciled dialogue.			*/
static template_t		analysis_unreconciled_template = NULL_TEMPLATE;	/**< The template which applies to the Unreconciled dialogue.			*/

/* Static Function Prototypes. */
#if 0
static void		analysis_unreconciled_click_handler(wimp_pointer *pointer);
static osbool		analysis_unreconciled_keypress_handler(wimp_key *key);
static void		analysis_refresh_unreconciled_window(void);
static void		analysis_fill_unreconciled_window(struct file_block *file, osbool restore);
static osbool		analysis_process_unreconciled_window(void);
static osbool		analysis_delete_unreconciled_window(void);
static void		analysis_generate_unreconciled_report(struct file_block *file);
#endif

static void analysis_unreconciled_copy_template(struct unrec_rep *to, struct unrec_rep *from);
static void analysis_unreconciled_write_file_block(void *block, FILE *out, char *name);
static void analysis_unreconciled_process_file_token(void *block, struct filing_block *in);

static struct analysis_report_details analysis_unreconciled_details = {
	analysis_unreconciled_process_file_token,
	analysis_unreconciled_write_file_block,
	analysis_unreconciled_copy_template
};

/**
 * Initialise the Unreconciled Transactions analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_unreconciled_initialise(void)
{
	analysis_template_set_block_size(sizeof(struct analysis_unreconciled_report));
	analysis_unreconciled_window = templates_create_window("UnrecRep");
	ihelp_add_window(analysis_unreconciled_window, "UnrecRep", NULL);
//	event_add_window_mouse_event(analysis_unreconciled_window, analysis_unreconciled_click_handler);
//	event_add_window_key_event(analysis_unreconciled_window, analysis_unreconciled_keypress_handler);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPACC, FALSE);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE, FALSE);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS, TRUE);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS, TRUE);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS, TRUE);
	analysis_unreconciled_dialogue = analysis_dialogue_initialise("UnrecRep", "UnrecRep");

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

struct analysis_unreconciled_report *analysis_unreconciled_create_instance(struct analysis_block *parent)
{
	struct analysis_unreconciled_report	*new;

	new = heap_alloc(sizeof(struct analysis_unreconciled_report));
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

	return new;
}


/**
 * Delete an unreconciled report data block.
 *
 * \param *report	Pointer to the report to delete.
 */

void analysis_unreconciled_delete_instance(struct analysis_unreconciled_report *report)
{
	if (report == NULL)
		return;

	if ((report->parent == analysis_unreconciled_instance) && windows_get_open(analysis_unreconciled_window))
		close_dialogue_with_caret(analysis_unreconciled_window);

	heap_free(report);
}










#if 0



/**
 * Open the Unreconciled Report dialogue box.
 *
 * \param *parent	The paranet analysis instance owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_unreconciled_open_window(struct analysis_block *parent, wimp_pointer *ptr, int template, osbool restore)
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

	template_mode = (template >= 0 && template < file->analysis->saved_report_count);

	if (template_mode) {
		analysis_copy_unreconciled_template(&(analysis_unreconciled_settings), &(file->analysis->saved_reports[template].data.unreconciled));
		analysis_unreconciled_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_unreconciled_window),
				windows_get_indirected_title_length(analysis_unreconciled_window),
				file->analysis->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_unreconciled_template(&(analysis_unreconciled_settings), file->unrec_rep);
		analysis_unreconciled_template = NULL_TEMPLATE;

		msgs_lookup("UrcRepTitle", windows_get_indirected_title_addr(analysis_unreconciled_window),
				windows_get_indirected_title_length(analysis_unreconciled_window));
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
			analysis_template_save_force_rename_close(analysis_unreconciled_file, analysis_unreconciled_template);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_refresh_unreconciled_window();
		}
		break;

	case ANALYSIS_UNREC_OK:
		if (analysis_process_unreconciled_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_unreconciled_window);
			analysis_template_save_force_rename_close(analysis_unreconciled_file, analysis_unreconciled_template);
		}
		break;

	case ANALYSIS_UNREC_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_unreconciled_window())
			close_dialogue_with_caret(analysis_unreconciled_window);
		break;

	case ANALYSIS_UNREC_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_unreconciled_template >= 0 &&
				analysis_unreconciled_template < analysis_unreconciled_file->analysis->saved_report_count)
			analysis_template_save_open_rename_window (analysis_unreconciled_file, analysis_unreconciled_template, pointer);
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
		icons_set_group_shaded(analysis_unreconciled_window,
				!(icons_get_selected (analysis_unreconciled_window, ANALYSIS_UNREC_GROUP) &&
				icons_get_selected (analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE)), 6,
				ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
				ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);

		icons_replace_caret_in_window(analysis_unreconciled_window);
		break;

	case ANALYSIS_UNREC_FROMSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_lookup_open_window(analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		break;

	case ANALYSIS_UNREC_TOSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			analysis_lookup_open_window(analysis_unreconciled_file, analysis_unreconciled_window,
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
			analysis_template_save_force_rename_close(analysis_unreconciled_file, analysis_unreconciled_template);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_unreconciled_window);
		analysis_template_save_force_rename_close(analysis_unreconciled_file, analysis_unreconciled_template);
		break;

	case wimp_KEY_F1:
		if (key->i == ANALYSIS_UNREC_FROMSPEC)
			analysis_lookup_open_window(analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		else if (key->i == ANALYSIS_UNREC_TOSPEC)
			analysis_lookup_open_window(analysis_unreconciled_file, analysis_unreconciled_window,
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
	if (analysis_unreconciled_template >= 0 && analysis_unreconciled_template < analysis_unreconciled_file->analysis->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 3) {
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
	osbool			group, lock;
	int			i, acc, found, unit, period, tot_in, tot_out, entries;
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
		strcpy(analysis_report_template.name, file->analysis->saved_reports[analysis_unreconciled_template].name);
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

		analysis_period_initialise(start_date, end_date, period, unit, lock);

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
 * Remove any references to an account if it appears within an
 * unreconciled transaction report template.
 *
 * \param *report	The transaction report to be processed.
 * \param account	The account to be removed.
 */

void analysis_unreconciled_remove_account(struct unrec_rep *report, acct_t account)
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

void analysis_unreconciled_remove_template(template_t template)
{
	if (analysis_unreconciled_template > template)
		analysis_unreconciled_template--;
}


#endif

/**
 * Copy a Unreconciled Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_unreconciled_copy_template(struct unrec_rep *to, struct unrec_rep *from)
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

