/* CashBook - analysis.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* ANSI C header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/debug.h"

/* Application header files */

#include "global.h"
#include "analysis.h"

#include "account.h"
#include "caret.h"
#include "conversion.h"
#include "date.h"
#include "file.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "report.h"
#include "templates.h"
#include "transact.h"


enum analysis_save_mode {
	ANALYSIS_SAVE_MODE_NONE = 0,
	ANALYSIS_SAVE_MODE_SAVE,
	ANALYSIS_SAVE_MODE_RENAME
};


struct analysis_template_link
{
	char			name[SAVED_REPORT_NAME_LEN+3];			/**< The name as it appears in the menu (+3 for ellipsis...)	*/
	int			template;					/**< Link to the associated report template.			*/
};



/* Report windows. */

static wimp_w			analysis_transaction_window = NULL;		/**< The handle of the Transaction Report window.		*/
static file_data		*analysis_transaction_file = NULL;		/**< The file currently owning the transaction dialogue.	*/
static osbool			analysis_transaction_restore = FALSE;		/**< The restore setting for the current Transaction dialogue.	*/

static wimp_w			analysis_unreconciled_window = NULL;		/**< The handle of the Unreconciled Report window.		*/
static file_data		*analysis_unreconciled_file = NULL;		/**< The file currently owning the unreconciled dialogue.	*/
static osbool			analysis_unreconciled_restore = FALSE;		/**< The restore setting for the current Unreconciled dialogue.	*/

static wimp_w			analysis_cashflow_window = NULL;		/**< The handle of the Cashflow Report window.			*/
static file_data		*analysis_cashflow_file = NULL;			/**< The file currently owning the cashflow dialogue.		*/
static osbool			analysis_cashflow_restore = FALSE;		/**< The restore setting for the current Cashflow dialogue.	*/

static wimp_w			analysis_balance_window = NULL;			/**< The handle of the Balance Report window.			*/
static file_data		*analysis_balance_file = NULL;			/**< The file currently owning the balance dialogue.		*/
static osbool			analysis_balance_restore = FALSE;		/**< The restore setting for the current Balance dialogue.	*/

/* Date period management. */

static date_t			analysis_period_start;				/**< The start of the current reporting period.			*/
static date_t			analysis_period_end;				/**< The end of the current reporting period.			*/
static int			analysis_period_length;
static int			analysis_period_unit;
static osbool			analysis_period_lock;
static osbool			analysis_period_first = TRUE;

/* Save/Rename Reports. */

static wimp_w			analysis_save_window = NULL;			/**< The handle of the Save/Rename window.			*/

static file_data *save_report_file = NULL;
static report_data *save_report_report = NULL;
static int save_report_template = NULL_TEMPLATE;

static enum analysis_save_mode	analysis_save_mode = 0;


/* Template List Menu. */

static wimp_menu			*analysis_template_menu = NULL;		/**< The saved template menu block.				*/
static struct analysis_template_link	*analysis_template_menu_link = NULL;	/**< Links for the template menu.				*/
static char				*analysis_template_menu_title = NULL;	/**< The menu title buffer.					*/


static trans_rep trans_rep_settings;
static unrec_rep unrec_rep_settings;
static cashflow_rep cashflow_rep_settings;
static balance_rep balance_rep_settings;

static int trans_rep_template = NULL_TEMPLATE;
static int unrec_rep_template = NULL_TEMPLATE;
static int cashflow_rep_template = NULL_TEMPLATE;
static int balance_rep_template = NULL_TEMPLATE;

static saved_report saved_report_template;

static acct_t wildcard_account_list = NULL_ACCOUNT; /* Pass a pointer to this to set all accounts. */







static void		analysis_transaction_click_handler(wimp_pointer *pointer);
static osbool		analysis_transaction_keypress_handler(wimp_key *key);
static void		analysis_refresh_transaction_window(void);
static void		analysis_fill_transaction_window(file_data *file, osbool restore);
static osbool		analysis_process_transaction_window(void);
static osbool		analysis_delete_transaction_window(void);
static void		analysis_generate_transaction_report(file_data *file);

static void		analysis_unreconciled_click_handler(wimp_pointer *pointer);
static osbool		analysis_unreconciled_keypress_handler(wimp_key *key);
static void		analysis_refresh_unreconciled_window(void);
static void		analysis_fill_unreconciled_window(file_data *file, osbool restore);
static osbool		analysis_process_unreconciled_window(void);
static osbool		analysis_delete_unreconciled_window(void);
static void		analysis_generate_unreconciled_report(file_data *file);

static void		analysis_cashflow_click_handler(wimp_pointer *pointer);
static osbool		analysis_cashflow_keypress_handler(wimp_key *key);
static void		analysis_refresh_cashflow_window(void);
static void		analysis_fill_cashflow_window(file_data *file, osbool restore);
static osbool		analysis_process_cashflow_window(void);
static osbool		analysis_delete_cashflow_window(void);
static void		analysis_generate_cashflow_report(file_data *file);

static void		analysis_balance_click_handler(wimp_pointer *pointer);
static osbool		analysis_balance_keypress_handler(wimp_key *key);
static void		analysis_refresh_balance_window(void);
static void		analysis_fill_balance_window(file_data *file, osbool restore);
static osbool		analysis_process_balance_window(void);
static osbool		analysis_delete_balance_window(void);
static void		analysis_generate_balance_report(file_data *file);

static void		analysis_find_date_range(file_data *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget);
static void		analysis_initialise_date_period(date_t start, date_t end, int period, int unit, osbool lock);
static osbool		analysis_get_next_date_period(date_t *next_start, date_t *next_end, char *date_text, size_t date_len);




static int		analysis_remove_account_from_list(file_data *file, acct_t account, acct_t *array, int *count);
static void		analysis_clear_account_report_flags(file_data *file);
static void		analysis_set_account_report_flags_from_list(file_data *file, unsigned type, unsigned flags, acct_t *array, int count);
static int		analysis_account_idents_to_list(file_data *file, unsigned type, char *list, acct_t *array);
static void		analysis_account_list_to_idents(file_data *file, char *list, acct_t *array, int len);




static void		analysis_open_rename_window(file_data *file, int template, wimp_pointer *ptr);
static void		analysis_save_click_handler(wimp_pointer *pointer);
static osbool		analysis_save_keypress_handler(wimp_key *key);
static void		analysis_save_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void		analysis_save_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void		analysis_save_menu_close_handler(wimp_w w, wimp_menu *menu);
static void		analysis_refresh_save_window(void);
static void		analysis_fill_save_window(report_data *report);
static void		analysis_fill_rename_window(file_data *file, int template);
static osbool		analysis_process_save_window(void);

static int		analysis_template_menu_compare_entries(const void *va, const void *vb);








static void		analysis_copy_transaction_template(trans_rep *to, trans_rep *from);
static void		analysis_copy_unreconciled_template(unrec_rep *to, unrec_rep *from);
static void		analysis_copy_cashflow_template(cashflow_rep *to, cashflow_rep *from);
static void		analysis_copy_balance_template(balance_rep *to, balance_rep *from);



/**
 * Initialise the Analysis module and all its dialogue boxes.
 */

void analysis_initialise(void)
{
	analysis_transaction_window = templates_create_window("TransRep");
	ihelp_add_window(analysis_transaction_window, "TransRep", NULL);
	event_add_window_mouse_event(analysis_transaction_window, analysis_transaction_click_handler);
	event_add_window_key_event(analysis_transaction_window, analysis_transaction_keypress_handler);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PDAYS);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS);
	event_add_window_icon_radio(analysis_transaction_window, ANALYSIS_TRANS_PYEARS);

	analysis_unreconciled_window = templates_create_window("UnrecRep");
	ihelp_add_window(analysis_unreconciled_window, "UnrecRep", NULL);
	event_add_window_mouse_event(analysis_unreconciled_window, analysis_unreconciled_click_handler);
	event_add_window_key_event(analysis_unreconciled_window, analysis_unreconciled_keypress_handler);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS);
	event_add_window_icon_radio(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS);

	analysis_cashflow_window = templates_create_window("CashFlwRep");
	ihelp_add_window(analysis_cashflow_window, "CashFlwRep", NULL);
	event_add_window_mouse_event(analysis_cashflow_window, analysis_cashflow_click_handler);
	event_add_window_key_event(analysis_cashflow_window, analysis_cashflow_keypress_handler);
	event_add_window_icon_radio(analysis_cashflow_window, ANALYSIS_CASHFLOW_PDAYS);
	event_add_window_icon_radio(analysis_cashflow_window, ANALYSIS_CASHFLOW_PMONTHS);
	event_add_window_icon_radio(analysis_cashflow_window, ANALYSIS_CASHFLOW_PYEARS);

	analysis_balance_window = templates_create_window("BalanceRep");
	ihelp_add_window(analysis_balance_window, "BalanceRep", NULL);
	event_add_window_mouse_event(analysis_balance_window, analysis_balance_click_handler);
	event_add_window_key_event(analysis_balance_window, analysis_balance_keypress_handler);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PDAYS);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS);
	event_add_window_icon_radio(analysis_balance_window, ANALYSIS_BALANCE_PYEARS);

	analysis_save_window = templates_create_window("SaveRepTemp");
	ihelp_add_window(analysis_save_window, "SaveRepTemp", NULL);
	event_add_window_mouse_event(analysis_save_window, analysis_save_click_handler);
	event_add_window_key_event(analysis_save_window, analysis_save_keypress_handler);
	event_add_window_menu_prepare(analysis_save_window, analysis_save_menu_prepare_handler);
	event_add_window_menu_selection(analysis_save_window, analysis_save_menu_selection_handler);
	event_add_window_menu_close(analysis_save_window, analysis_save_menu_close_handler);
	event_add_window_icon_popup(analysis_save_window, ANALYSIS_SAVE_NAMEPOPUP, NULL, -1);
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

void analysis_open_transaction_window(file_data *file, wimp_pointer *ptr, int template, osbool restore)
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
		analysis_copy_transaction_template(&(trans_rep_settings), &(file->saved_reports[template].data.transaction));
		trans_rep_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_transaction_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_transaction_template(&(trans_rep_settings), &(file->trans_rep));
		trans_rep_template = NULL_TEMPLATE;

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
		if (pointer->buttons == wimp_CLICK_SELECT && trans_rep_template >= 0 && trans_rep_template < analysis_transaction_file->saved_report_count)
			analysis_open_rename_window(analysis_transaction_file, trans_rep_template, pointer);
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
			open_account_lookup_window(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		break;

	case ANALYSIS_TRANS_TOSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			open_account_lookup_window(analysis_transaction_file, analysis_transaction_window,
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
			open_account_lookup_window(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		else if (key->i == ANALYSIS_TRANS_TOSPEC)
			open_account_lookup_window(analysis_transaction_file, analysis_transaction_window,
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

static void analysis_fill_transaction_window(file_data *file, osbool restore)
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

		convert_date_to_string(trans_rep_settings.date_from,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATEFROM));
		convert_date_to_string(trans_rep_settings.date_to,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATETO));

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_BUDGET, trans_rep_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_GROUP, trans_rep_settings.group);

		icons_printf(analysis_transaction_window, ANALYSIS_TRANS_PERIOD, "%d", trans_rep_settings.period);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PDAYS, trans_rep_settings.period_unit == PERIOD_DAYS);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS, trans_rep_settings.period_unit == PERIOD_MONTHS);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_PYEARS, trans_rep_settings.period_unit == PERIOD_YEARS);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_LOCK, trans_rep_settings.lock);

		/* Set the include icons. */

		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_FROMSPEC),
				trans_rep_settings.from, trans_rep_settings.from_count);
		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_TOSPEC),
				trans_rep_settings.to, trans_rep_settings.to_count);
		icons_strncpy(analysis_transaction_window, ANALYSIS_TRANS_REFSPEC, trans_rep_settings.ref);
		convert_money_to_string(trans_rep_settings.amount_min,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC));
		convert_money_to_string(trans_rep_settings.amount_max,
				icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC));
		icons_strncpy(analysis_transaction_window, ANALYSIS_TRANS_DESCSPEC, trans_rep_settings.desc);

		/* Set the output icons. */

		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPTRANS, trans_rep_settings.output_trans);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPSUMMARY, trans_rep_settings.output_summary);
		icons_set_selected(analysis_transaction_window, ANALYSIS_TRANS_OPACCSUMMARY, trans_rep_settings.output_accsummary);
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

	analysis_transaction_file->trans_rep.date_from =
			convert_string_to_date (icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATEFROM), NULL_DATE, 0);
	analysis_transaction_file->trans_rep.date_to =
			convert_string_to_date (icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DATETO), NULL_DATE, 0);
	analysis_transaction_file->trans_rep.budget = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_BUDGET);

	/* Read the grouping settings. */

	analysis_transaction_file->trans_rep.group = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_GROUP);
	analysis_transaction_file->trans_rep.period = atoi(icons_get_indirected_text_addr (analysis_transaction_window, ANALYSIS_TRANS_PERIOD));

	if (icons_get_selected	(analysis_transaction_window, ANALYSIS_TRANS_PDAYS))
		analysis_transaction_file->trans_rep.period_unit = PERIOD_DAYS;
	else if (icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_PMONTHS))
		analysis_transaction_file->trans_rep.period_unit = PERIOD_MONTHS;
	else if (icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_PYEARS))
		analysis_transaction_file->trans_rep.period_unit = PERIOD_YEARS;
	else
		analysis_transaction_file->trans_rep.period_unit = PERIOD_MONTHS;

	analysis_transaction_file->trans_rep.lock = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_LOCK);

	/* Read the account and heading settings. */

	analysis_transaction_file->trans_rep.from_count =
			analysis_account_idents_to_list(analysis_transaction_file, ACCOUNT_FULL | ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_FROMSPEC),
			analysis_transaction_file->trans_rep.from);
	analysis_transaction_file->trans_rep.to_count =
			analysis_account_idents_to_list(analysis_transaction_file, ACCOUNT_FULL | ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_TOSPEC),
			analysis_transaction_file->trans_rep.to);
	strcpy(analysis_transaction_file->trans_rep.ref,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_REFSPEC));
	strcpy(analysis_transaction_file->trans_rep.desc,
			icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_DESCSPEC));
	analysis_transaction_file->trans_rep.amount_min = (*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC) == '\0') ?
			NULL_CURRENCY : convert_string_to_money(icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTLOSPEC));

	analysis_transaction_file->trans_rep.amount_max = (*icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC) == '\0') ?
			NULL_CURRENCY : convert_string_to_money(icons_get_indirected_text_addr(analysis_transaction_window, ANALYSIS_TRANS_AMTHISPEC));

	/* Read the output options. */

	analysis_transaction_file->trans_rep.output_trans = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_OPTRANS);
 	analysis_transaction_file->trans_rep.output_summary = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_OPSUMMARY);
	analysis_transaction_file->trans_rep.output_accsummary = icons_get_selected(analysis_transaction_window, ANALYSIS_TRANS_OPACCSUMMARY);

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
	if (trans_rep_template >= 0 && trans_rep_template < analysis_transaction_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_saved_report_template(analysis_transaction_file, trans_rep_template);
		trans_rep_template = NULL_TEMPLATE;

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

static void analysis_generate_transaction_report(file_data *file)
{
	report_data	*report;
	int		i, found, total, unit, period, group, lock, output_trans, output_summary, output_accsummary,
			total_days, period_days, period_limit, entry, account;
	date_t		start_date, end_date, next_start, next_end;
	amt_t		min_amount, max_amount;
	char		line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
	char		*match_ref, *match_desc;

	hourglass_on();

	if (!(file->sort_valid))
		sort_transactions(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date,
			file->trans_rep.date_from, file->trans_rep.date_to, file->trans_rep.budget);

	total_days = count_days(start_date, end_date);

	/* Read the grouping settings. */

	group = file->trans_rep.group;
	unit = file->trans_rep.period_unit;
	lock = file->trans_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
	period = (group) ? file->trans_rep.period : 0;

	/* Read the include list. */

	analysis_clear_account_report_flags(file);

	if (file->trans_rep.from_count == 0 && file->trans_rep.to_count == 0) {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM,
				&wildcard_account_list, 1);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO,
				&wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM,
				file->trans_rep.from, file->trans_rep.from_count);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO,
				file->trans_rep.to, file->trans_rep.to_count);
	}

	min_amount = file->trans_rep.amount_min;
	max_amount = file->trans_rep.amount_max;

	match_ref = (*(file->trans_rep.ref) == '\0') ? NULL : file->trans_rep.ref;
	match_desc = (*(file->trans_rep.desc) == '\0') ? NULL : file->trans_rep.desc;

	/* Read the output options. */

	output_trans = file->trans_rep.output_trans;
	output_summary = file->trans_rep.output_summary;
	output_accsummary = file->trans_rep.output_accsummary;

	/* Open a new report for output. */

	analysis_copy_transaction_template(&(saved_report_template.data.transaction), &(file->trans_rep));
	if (trans_rep_template == NULL_TEMPLATE)
		*(saved_report_template.name) = '\0';
	else
		strcpy(saved_report_template.name, file->saved_reports[trans_rep_template].name);
	saved_report_template.type = REPORT_TYPE_TRANS;

	msgs_lookup("TRWinT", line, sizeof (line));
	report = report_open(file, line, &saved_report_template);

	if (report == NULL) {
		hourglass_off();
		return;
	}

	/* Output report heading */

	 make_file_leafname(file, b1, sizeof(b1));
	if (*saved_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof(line), saved_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("TRTitle", line, sizeof(line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	convert_date_to_string(start_date, b1);
	convert_date_to_string(end_date, b2);
	convert_date_to_string(get_current_date(), b3);
	msgs_param_lookup("TRHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	analysis_initialise_date_period(start_date, end_date, period, unit, lock);

	/* Initialise the heading remainder values for the report. */

	for (i=0; i < file->account_count; i++) {
		if (file->accounts[i].type & ACCOUNT_OUT)
			file->accounts[i].report_balance = file->accounts[i].budget_amount;
		else if (file->accounts[i].type & ACCOUNT_IN)
			file->accounts[i].report_balance = -file->accounts[i].budget_amount;
	}

	while (analysis_get_next_date_period(&next_start, &next_end, date_text, sizeof(date_text))) {

		/* Zero the heading totals for the report. */

		for (i=0; i < file->account_count; i++)
			file->accounts[i].report_total = 0;

		/* Scan through the transactions, adding the values up for those in range and outputting them to the screen. */

		found = 0;

		for (i=0; i < file->trans_count; i++) {
			if ((next_start == NULL_DATE || file->transactions[i].date >= next_start) &&
					(next_end == NULL_DATE || file->transactions[i].date <= next_end) &&
					(((file->transactions[i].from != NULL_ACCOUNT) &&
					((file->accounts[file->transactions[i].from].report_flags & REPORT_FROM) != 0)) ||
					((file->transactions[i].to != NULL_ACCOUNT) &&
					((file->accounts[file->transactions[i].to].report_flags & REPORT_TO) != 0))) &&
					((min_amount == NULL_CURRENCY) || (file->transactions[i].amount >= min_amount)) &&
					((max_amount == NULL_CURRENCY) || (file->transactions[i].amount <= max_amount)) &&
					((match_ref == NULL) || string_wildcard_compare(match_ref, file->transactions[i].reference, TRUE)) &&
					((match_desc == NULL) || string_wildcard_compare(match_desc, file->transactions[i].description, TRUE))) {
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

				if (file->transactions[i].from != NULL_ACCOUNT)
					file->accounts[file->transactions[i].from].report_total -= file->transactions[i].amount;

				if (file->transactions[i].to != NULL_ACCOUNT)
					file->accounts[file->transactions[i].to].report_total += file->transactions[i].amount;

				if (output_trans) {
					convert_date_to_string(file->transactions[i].date, b1);

					convert_money_to_string(file->transactions[i].amount, b2);

					sprintf(line, "%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s", b1,
							find_account_name (file, file->transactions[i].from),
							find_account_name (file, file->transactions[i].to),
							file->transactions[i].reference, b2, file->transactions[i].description);

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

			entry = find_accounts_window_entry_from_type(file, ACCOUNT_FULL);

			for (i=0; i < file->account_windows[entry].display_lines; i++) {
				if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
					account = file->account_windows[entry].line_data[i].account;

					if (file->accounts[account].report_total != 0) {
						total += file->accounts[account].report_total;
						convert_money_to_string(file->accounts[account].report_total, b1);
						sprintf(line, "\\i%s\\t\\d\\r%s", file->accounts[account].name, b1);
						report_write_line(report, 2, line);
					}
				}
			}

			msgs_lookup("TRTotal", b1, sizeof (b1));
			convert_money_to_string(total, b2);
			sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
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
			if (file->trans_rep.budget){
				msgs_lookup("TRSummExtra", b1, sizeof(b1));
				strcat(line, b1);
			}
			report_write_line(report, 2, line);

			entry = find_accounts_window_entry_from_type(file, ACCOUNT_OUT);

			for (i=0; i < file->account_windows[entry].display_lines; i++) {
				if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
					account = file->account_windows[entry].line_data[i].account;

					if (file->accounts[account].report_total != 0) {
						total += file->accounts[account].report_total;
						convert_money_to_string(file->accounts[account].report_total, b1);
						sprintf(line, "\\i%s\\t\\d\\r%s", file->accounts[account].name, b1);
						if (file->trans_rep.budget) {
							period_days = count_days(next_start, next_end);
							period_limit = file->accounts[account].budget_amount * period_days / total_days;
							convert_money_to_string(period_limit, b1);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							convert_money_to_string(period_limit - file->accounts[account].report_total, b1);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							file->accounts[i].report_balance -= file->accounts[account].report_total;
							convert_money_to_string(file->accounts[account].report_balance, b1);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
						}

						report_write_line(report, 2, line);
					}
				}
			}

			msgs_lookup("TRTotal", b1, sizeof(b1));
			convert_money_to_string(total, b2);
			sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
			report_write_line(report, 2, line);

			/* Summarise the incomings. */

			total = 0;

			report_write_line(report, 0, "");
			msgs_lookup("TRIncomings", b1, sizeof(b1));
			sprintf(line, "\\i%s", b1);
			if (file->trans_rep.budget) {
				msgs_lookup("TRSummExtra", b1, sizeof(b1));
				strcat(line, b1);
			}

			report_write_line(report, 2, line);

			entry = find_accounts_window_entry_from_type(file, ACCOUNT_IN);

			for (i=0; i < file->account_windows[entry].display_lines; i++) {
				if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
					account = file->account_windows[entry].line_data[i].account;

					if (file->accounts[account].report_total != 0) {
						total += file->accounts[account].report_total;
						convert_money_to_string(-file->accounts[account].report_total, b1);
						sprintf(line, "\\i%s\\t\\d\\r%s", file->accounts[account].name, b1);
						if (file->trans_rep.budget) {
							period_days = count_days(next_start, next_end);
							period_limit = file->accounts[account].budget_amount * period_days / total_days;
							convert_money_to_string(period_limit, b1);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							convert_money_to_string(period_limit - file->accounts[account].report_total, b1);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
							file->accounts[i].report_balance -= file->accounts[account].report_total;
							convert_money_to_string(file->accounts[account].report_balance, b1);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
						}

						report_write_line(report, 2, line);
					}
				}
			}

			msgs_lookup("TRTotal", b1, sizeof(b1));
			convert_money_to_string(-total, b2);
			sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
			report_write_line(report, 2, line);
		}
	}

	report_close(report);

	hourglass_off();
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

void analysis_open_unreconciled_window(file_data *file, wimp_pointer *ptr, int template, osbool restore)
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
		analysis_copy_unreconciled_template(&(unrec_rep_settings), &(file->saved_reports[template].data.unreconciled));
		unrec_rep_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_unreconciled_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_unreconciled_template(&(unrec_rep_settings), &(file->unrec_rep));
		unrec_rep_template = NULL_TEMPLATE;

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
		if (pointer->buttons == wimp_CLICK_SELECT && unrec_rep_template >= 0 && unrec_rep_template < analysis_unreconciled_file->saved_report_count)
			analysis_open_rename_window (analysis_unreconciled_file, unrec_rep_template, pointer);
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
			open_account_lookup_window (analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		break;

	case ANALYSIS_UNREC_TOSPECPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			open_account_lookup_window (analysis_unreconciled_file, analysis_unreconciled_window,
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
			open_account_lookup_window (analysis_unreconciled_file, analysis_unreconciled_window,
					ANALYSIS_UNREC_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		else if (key->i == ANALYSIS_UNREC_TOSPEC)
			open_account_lookup_window (analysis_unreconciled_file, analysis_unreconciled_window,
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

static void analysis_fill_unreconciled_window(file_data *file, osbool restore)
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

		convert_date_to_string(unrec_rep_settings.date_from,
				icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATEFROM));
		convert_date_to_string(unrec_rep_settings.date_to,
				icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATETO));

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_BUDGET, unrec_rep_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP, unrec_rep_settings.group);

		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPACC, unrec_rep_settings.period_unit == PERIOD_NONE);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPDATE, unrec_rep_settings.period_unit != PERIOD_NONE);

		icons_printf(analysis_unreconciled_window, ANALYSIS_UNREC_PERIOD, "%d", unrec_rep_settings.period);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS, unrec_rep_settings.period_unit == PERIOD_DAYS);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS,
				unrec_rep_settings.period_unit == PERIOD_MONTHS ||
				unrec_rep_settings.period_unit == PERIOD_NONE);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS, unrec_rep_settings.period_unit == PERIOD_YEARS);
		icons_set_selected(analysis_unreconciled_window, ANALYSIS_UNREC_LOCK, unrec_rep_settings.lock);

		/* Set the from and to spec fields. */

		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_FROMSPEC),
				unrec_rep_settings.from, unrec_rep_settings.from_count);
		analysis_account_list_to_idents(file, icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_TOSPEC),
				unrec_rep_settings.to, unrec_rep_settings.to_count);
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

	analysis_unreconciled_file->unrec_rep.date_from =
			convert_string_to_date(icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATEFROM), NULL_DATE, 0);
	analysis_unreconciled_file->unrec_rep.date_to =
			convert_string_to_date(icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_DATETO), NULL_DATE, 0);
	analysis_unreconciled_file->unrec_rep.budget = icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_BUDGET);

	/* Read the grouping settings. */

	analysis_unreconciled_file->unrec_rep.group = icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUP);
	analysis_unreconciled_file->unrec_rep.period = atoi(icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_PERIOD));

	if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_GROUPACC))
		analysis_unreconciled_file->unrec_rep.period_unit = PERIOD_NONE;
	else if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PDAYS))
		analysis_unreconciled_file->unrec_rep.period_unit = PERIOD_DAYS;
	else if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PMONTHS))
		analysis_unreconciled_file->unrec_rep.period_unit = PERIOD_MONTHS;
	else if (icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_PYEARS))
		analysis_unreconciled_file->unrec_rep.period_unit = PERIOD_YEARS;
	else
		analysis_unreconciled_file->unrec_rep.period_unit = PERIOD_MONTHS;

	analysis_unreconciled_file->unrec_rep.lock = icons_get_selected(analysis_unreconciled_window, ANALYSIS_UNREC_LOCK);

	/* Read the account and heading settings. */

	analysis_unreconciled_file->unrec_rep.from_count =
			analysis_account_idents_to_list(analysis_unreconciled_file, ACCOUNT_FULL | ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_FROMSPEC),
			analysis_unreconciled_file->unrec_rep.from);
	analysis_unreconciled_file->unrec_rep.to_count =
			analysis_account_idents_to_list(analysis_unreconciled_file, ACCOUNT_FULL | ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_unreconciled_window, ANALYSIS_UNREC_TOSPEC),
			analysis_unreconciled_file->unrec_rep.to);

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
	if (unrec_rep_template >= 0 && unrec_rep_template < analysis_unreconciled_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_saved_report_template(analysis_unreconciled_file, unrec_rep_template);
		unrec_rep_template = NULL_TEMPLATE;

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

static void analysis_generate_unreconciled_report(file_data *file)
{
	int		i, acc, found, unit, period, group, lock, tot_in, tot_out, entry;
	char		line[2048], b1[1024], b2[1024], b3[1024], date_text[1024],
			rec_char[REC_FIELD_LEN], r1[REC_FIELD_LEN], r2[REC_FIELD_LEN];
	date_t		start_date, end_date, next_start, next_end;
	report_data	*report;
	int		acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	hourglass_on();

	if (!(file->sort_valid))
		sort_transactions(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date, file->unrec_rep.date_from, file->unrec_rep.date_to, file->unrec_rep.budget);

	/* Read the grouping settings. */

	group = file->unrec_rep.group;
	unit = file->unrec_rep.period_unit;
	lock = file->unrec_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
	period = (group) ? file->unrec_rep.period : 0;

	/* Read the include list. */

	analysis_clear_account_report_flags(file);

	if (file->unrec_rep.from_count == 0 && file->unrec_rep.to_count == 0) {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM, &wildcard_account_list, 1);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO, &wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM, file->unrec_rep.from, file->unrec_rep.from_count);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO, file->unrec_rep.to, file->unrec_rep.to_count);
	}

	/* Start to output the report details. */

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	analysis_copy_unreconciled_template(&(saved_report_template.data.unreconciled), &(file->unrec_rep));
	if (unrec_rep_template == NULL_TEMPLATE)
		*(saved_report_template.name) = '\0';
	else
		strcpy(saved_report_template.name, file->saved_reports[unrec_rep_template].name);
	saved_report_template.type = REPORT_TYPE_UNREC;

	msgs_lookup("URWinT", line, sizeof(line));
	report = report_open(file, line, &saved_report_template);

	if (report == NULL) {
		hourglass_off();
		return;
	}

	/* Output report heading */

	make_file_leafname(file, b1, sizeof(b1));
	if (*saved_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof (line), saved_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("URTitle", line, sizeof (line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	convert_date_to_string(start_date, b1);
	convert_date_to_string(end_date, b2);
	convert_date_to_string(get_current_date(), b3);
	msgs_param_lookup("URHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	if (group && unit == PERIOD_NONE) {
		/* We are doing a grouped-by-account report.
		 *
		 * Step through the accounts in account list order, and run through all the transactions each time.  A
		 * transaction is added if it is unreconciled in the account concerned; transactions unreconciled in two
		 * accounts may therefore appear twice in the list.
		 */

		for (acc_group = 0; acc_group < groups; acc_group++) {
			entry = find_accounts_window_entry_from_type(file, sequence[acc_group]);

			for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++) {
				if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA) {
					acc = file->account_windows[entry].line_data[group_line].account;

					found = 0;
					tot_in = 0;
					tot_out = 0;

					for (i=0; i < file->trans_count; i++) {
						if ((start_date == NULL_DATE || file->transactions[i].date >= start_date) &&
								(end_date == NULL_DATE || file->transactions[i].date <= end_date) &&
								((file->transactions[i].from == acc && (file->accounts[acc].report_flags & REPORT_FROM) != 0 &&
								(file->transactions[i].flags & TRANS_REC_FROM) == 0) ||
								(file->transactions[i].to == acc && (file->accounts[acc].report_flags & REPORT_TO) != 0 &&
								(file->transactions[i].flags & TRANS_REC_TO) == 0))) {
							if (found == 0) {
								report_write_line(report, 0, "");

								if (group == TRUE) {
									sprintf(line, "\\u%s", find_account_name(file, acc));
									report_write_line(report, 0, line);
								}
								msgs_lookup("URHeadings", line, sizeof(line));
								report_write_line(report, 1, line);
							}

							found++;

							if (file->transactions[i].from == acc)
								tot_out -= file->transactions[i].amount;
							else if (file->transactions[i].to == acc)
								tot_in += file->transactions[i].amount;

							/* Output the transaction to the report. */

							strcpy(r1, (file->transactions[i].flags & TRANS_REC_FROM) ? rec_char : "");
							strcpy(r2, (file->transactions[i].flags & TRANS_REC_TO) ? rec_char : "");
							convert_date_to_string(file->transactions[i].date, b1);
							convert_money_to_string(file->transactions[i].amount, b2);

							sprintf(line, "%s\\t%s\\t%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s", b1,
									r1, find_account_name(file, file->transactions[i].from),
									r2, find_account_name(file, file->transactions[i].to),
									file->transactions[i].reference, b2, file->transactions[i].description);

							report_write_line(report, 1, line);
						}
					}

					if (found != 0) {
						report_write_line(report, 2, "");

						msgs_lookup("URTotalIn", b1, sizeof(b1));
						full_convert_money_to_string(tot_in, b2, TRUE);
						sprintf(line, "\\i%s\\t\\d\\r%s", b1, b2);
						report_write_line(report, 2, line);

						msgs_lookup("URTotalOut", b1, sizeof(b1));
						full_convert_money_to_string(tot_out, b2, TRUE);
						sprintf(line, "\\i%s\\t\\d\\r%s", b1, b2);
						report_write_line(report, 2, line);

						msgs_lookup("URTotal", b1, sizeof(b1));
						full_convert_money_to_string(tot_in+tot_out, b2, TRUE);
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

			for (i=0; i < file->trans_count; i++) {
				if ((next_start == NULL_DATE || file->transactions[i].date >= next_start) &&
						(next_end == NULL_DATE || file->transactions[i].date <= next_end) &&
						(((file->transactions[i].flags & TRANS_REC_FROM) == 0 &&
						(file->transactions[i].from != NULL_ACCOUNT) &&
						(file->accounts[file->transactions[i].from].report_flags & REPORT_FROM) != 0) ||
						((file->transactions[i].flags & TRANS_REC_TO) == 0 &&
						(file->transactions[i].to != NULL_ACCOUNT) &&
						(file->accounts[file->transactions[i].to].report_flags & REPORT_TO) != 0))) {
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

					strcpy(r1, (file->transactions[i].flags & TRANS_REC_FROM) ? rec_char : "");
					strcpy(r2, (file->transactions[i].flags & TRANS_REC_TO) ? rec_char : "");
					convert_date_to_string(file->transactions[i].date, b1);
					convert_money_to_string(file->transactions[i].amount, b2);

					sprintf(line, "%s\\t%s\\t%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s", b1,
							r1, find_account_name(file, file->transactions[i].from),
							r2, find_account_name(file, file->transactions[i].to),
							file->transactions[i].reference, b2, file->transactions[i].description);

					report_write_line(report, 1, line);
				}
			}
		}
	}

	report_close(report);

	hourglass_off();
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

void analysis_open_cashflow_window(file_data *file, wimp_pointer *ptr, int template, osbool restore)
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
		analysis_copy_cashflow_template(&(cashflow_rep_settings), &(file->saved_reports[template].data.cashflow));
		cashflow_rep_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_cashflow_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_cashflow_template(&(cashflow_rep_settings), &(file->cashflow_rep));
		cashflow_rep_template = NULL_TEMPLATE;

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
		if (pointer->buttons == wimp_CLICK_SELECT && cashflow_rep_template >= 0 && cashflow_rep_template < analysis_cashflow_file->saved_report_count)
			analysis_open_rename_window(analysis_cashflow_file, cashflow_rep_template, pointer);
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
			open_account_lookup_window(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		break;

	case ANALYSIS_CASHFLOW_INCOMINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			open_account_lookup_window(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		break;

	case ANALYSIS_CASHFLOW_OUTGOINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			open_account_lookup_window(analysis_cashflow_file, analysis_cashflow_window,
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
			open_account_lookup_window(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		else if (key->i == ANALYSIS_CASHFLOW_INCOMING)
			open_account_lookup_window(analysis_cashflow_file, analysis_cashflow_window,
					ANALYSIS_CASHFLOW_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		else if (key->i == ANALYSIS_CASHFLOW_OUTGOING)
			open_account_lookup_window(analysis_cashflow_file, analysis_cashflow_window,
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

static void analysis_fill_cashflow_window(file_data *file, osbool restore)
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

		convert_date_to_string(cashflow_rep_settings.date_from,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATEFROM));
		convert_date_to_string(cashflow_rep_settings.date_to,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATETO));
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_BUDGET, cashflow_rep_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_GROUP, cashflow_rep_settings.group);

		icons_printf(analysis_cashflow_window, ANALYSIS_CASHFLOW_PERIOD, "%d", cashflow_rep_settings.period);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PDAYS, cashflow_rep_settings.period_unit == PERIOD_DAYS);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PMONTHS, cashflow_rep_settings.period_unit == PERIOD_MONTHS);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PYEARS, cashflow_rep_settings.period_unit == PERIOD_YEARS);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_LOCK, cashflow_rep_settings.lock);
		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_EMPTY, cashflow_rep_settings.empty);

		/* Set the accounts and format details. */

		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_ACCOUNTS),
				cashflow_rep_settings.accounts, cashflow_rep_settings.accounts_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_INCOMING),
				cashflow_rep_settings.incoming, cashflow_rep_settings.incoming_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_OUTGOING),
				cashflow_rep_settings.outgoing, cashflow_rep_settings.outgoing_count);

		icons_set_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_TABULAR, cashflow_rep_settings.tabular);
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

	analysis_cashflow_file->cashflow_rep.date_from =
			convert_string_to_date(icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATEFROM), NULL_DATE, 0);
	analysis_cashflow_file->cashflow_rep.date_to =
			convert_string_to_date(icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_DATETO), NULL_DATE, 0);
	analysis_cashflow_file->cashflow_rep.budget = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_BUDGET);

	/* Read the grouping settings. */

	analysis_cashflow_file->cashflow_rep.group = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_GROUP);
	analysis_cashflow_file->cashflow_rep.period = atoi(icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_PERIOD));

	if (icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PDAYS))
		analysis_cashflow_file->cashflow_rep.period_unit = PERIOD_DAYS;
	else if (icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PMONTHS))
		analysis_cashflow_file->cashflow_rep.period_unit = PERIOD_MONTHS;
	else if (icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_PYEARS))
		analysis_cashflow_file->cashflow_rep.period_unit = PERIOD_YEARS;
	else
		analysis_cashflow_file->cashflow_rep.period_unit = PERIOD_MONTHS;

	analysis_cashflow_file->cashflow_rep.lock = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_LOCK);
	analysis_cashflow_file->cashflow_rep.empty = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_EMPTY);

	/* Read the account and heading settings. */

	analysis_cashflow_file->cashflow_rep.accounts_count =
			analysis_account_idents_to_list(analysis_cashflow_file, ACCOUNT_FULL,
			icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_ACCOUNTS),
			analysis_cashflow_file->cashflow_rep.accounts);
	analysis_cashflow_file->cashflow_rep.incoming_count =
			analysis_account_idents_to_list(analysis_cashflow_file, ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_INCOMING),
			analysis_cashflow_file->cashflow_rep.incoming);
	analysis_cashflow_file->cashflow_rep.outgoing_count =
			analysis_account_idents_to_list(analysis_cashflow_file, ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_cashflow_window, ANALYSIS_CASHFLOW_OUTGOING),
			analysis_cashflow_file->cashflow_rep.outgoing);

	analysis_cashflow_file->cashflow_rep.tabular = icons_get_selected(analysis_cashflow_window, ANALYSIS_CASHFLOW_TABULAR);

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
	if (cashflow_rep_template >= 0 && cashflow_rep_template < analysis_cashflow_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_saved_report_template(analysis_cashflow_file, cashflow_rep_template);
		cashflow_rep_template = NULL_TEMPLATE;

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

static void analysis_generate_cashflow_report(file_data *file)
{
	int		i, acc, items, found, unit, period, group, lock, tabular, show_blank, total;
	char		line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
	date_t		start_date, end_date, next_start, next_end;
	report_data	*report;
	int		entry, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	hourglass_on();

	if (!(file->sort_valid))
		sort_transactions(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date, file->cashflow_rep.date_from, file->cashflow_rep.date_to, file->cashflow_rep.budget);

	/* Read the grouping settings. */

	group = file->cashflow_rep.group;
	unit = file->cashflow_rep.period_unit;
	lock = file->cashflow_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
	period = (group) ? file->cashflow_rep.period : 0;
	show_blank = file->cashflow_rep.empty;

	/* Read the include list. */

	analysis_clear_account_report_flags(file);

	if (file->cashflow_rep.accounts_count == 0 && file->cashflow_rep.incoming_count == 0 &&
			file->cashflow_rep.outgoing_count == 0) {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT, REPORT_INCLUDE, &wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL, REPORT_INCLUDE, file->cashflow_rep.accounts, file->cashflow_rep.accounts_count);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_IN, REPORT_INCLUDE, file->cashflow_rep.incoming, file->cashflow_rep.incoming_count);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_OUT, REPORT_INCLUDE, file->cashflow_rep.outgoing, file->cashflow_rep.outgoing_count);
	}

	tabular = file->cashflow_rep.tabular;

	/* Count the number of accounts and headings to be included.  If this comes to more than the number of tab
	 * stops available (including 2 for account name and total), force the tabular format option off.
	 */

	items = 0;

	for (i=0; i < file->account_count; i++)
		if ((file->accounts[i].report_flags & REPORT_INCLUDE) != 0)
			items++;

	if ((items + 2) > REPORT_TAB_STOPS)
		tabular = FALSE;

	/* Start to output the report details. */

	analysis_copy_cashflow_template(&(saved_report_template.data.cashflow), &(file->cashflow_rep));
	if (cashflow_rep_template == NULL_TEMPLATE)
		*(saved_report_template.name) = '\0';
	else
		strcpy(saved_report_template.name, file->saved_reports[cashflow_rep_template].name);
	saved_report_template.type = REPORT_TYPE_CASHFLOW;

	msgs_lookup("CRWinT", line, sizeof(line));
	report = report_open(file, line, &saved_report_template);

	if (report == NULL) {
		hourglass_off();
		return;
	}

	/* Output report heading */

	make_file_leafname(file, b1, sizeof(b1));
	if (*saved_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof(line), saved_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("CRTitle", line, sizeof(line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	convert_date_to_string(start_date, b1);
	convert_date_to_string(end_date, b2);
	convert_date_to_string(get_current_date (), b3);
	msgs_param_lookup("CRHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	/* Start to output the report. */

	if (tabular) {
		report_write_line(report, 0, "");
		msgs_lookup("CRDate", b1, sizeof(b1));
		sprintf(line, "\\b%s", b1);

		for (acc_group = 0; acc_group < groups; acc_group++) {
			entry = find_accounts_window_entry_from_type(file, sequence[acc_group]);

			for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++) {
				if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA) {
					acc = file->account_windows[entry].line_data[group_line].account;

					if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0) {
						sprintf(b1, "\\t\\r\\b%s", file->accounts[acc].name);
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

		for (i=0; i < file->account_count; i++)
			file->accounts[i].report_total = 0;

		/* Scan through the transactions, adding the values up for those in range and outputting them to the screen. */

		found = 0;

		for (i=0; i < file->trans_count; i++) {
			if ((next_start == NULL_DATE || file->transactions[i].date >= next_start) &&
					(next_end == NULL_DATE || file->transactions[i].date <= next_end)) {
				if (file->transactions[i].from != NULL_ACCOUNT)
					file->accounts[file->transactions[i].from].report_total -= file->transactions[i].amount;

				if (file->transactions[i].to != NULL_ACCOUNT)
					file->accounts[file->transactions[i].to].report_total += file->transactions[i].amount;

				found++;
			}
		}

		/* Print the transaction summaries. */

		if (found > 0 || show_blank) {
			if (tabular) {
				strcpy(line, date_text);

				total = 0;

				for (acc_group = 0; acc_group < groups; acc_group++) {
					entry = find_accounts_window_entry_from_type(file, sequence[acc_group]);

					for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++) {
						if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA) {
							acc = file->account_windows[entry].line_data[group_line].account;

							if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0) {
								total += file->accounts[acc].report_total;
								full_convert_money_to_string(file->accounts[acc].report_total, b1, TRUE);
								sprintf(b2, "\\t\\d\\r%s", b1);
								strcat(line, b2);
							}
						}
					}
				}

				full_convert_money_to_string(total, b1, TRUE);
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
					entry = find_accounts_window_entry_from_type(file, sequence[acc_group]);

					for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++) {
						if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA) {
							acc = file->account_windows[entry].line_data[group_line].account;

							if (file->accounts[acc].report_total != 0 && (file->accounts[acc].report_flags & REPORT_INCLUDE) != 0) {
								total += file->accounts[acc].report_total;
								full_convert_money_to_string(file->accounts[acc].report_total, b1, TRUE);
								sprintf(line, "\\i%s\\t\\d\\r%s", file->accounts[acc].name, b1);
								report_write_line(report, 2, line);
							}
						}
					}
				}
				msgs_lookup("CRTotal", b1, sizeof(b1));
				full_convert_money_to_string(total, b2, TRUE);
				sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
				report_write_line(report, 2, line);
			}
		}
	}

	report_close(report);

	hourglass_off();
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

void analysis_open_balance_window(file_data *file, wimp_pointer *ptr, int template, osbool restore)
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
		analysis_copy_balance_template(&(balance_rep_settings), &(file->saved_reports[template].data.balance));
		balance_rep_template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_balance_window), 50,
				file->saved_reports[template].name, NULL, NULL, NULL);

		restore = TRUE; /* If we use a template, we always want to reset to the template! */
	} else {
		analysis_copy_balance_template (&(balance_rep_settings), &(file->balance_rep));
		balance_rep_template = NULL_TEMPLATE;

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
		if (pointer->buttons == wimp_CLICK_SELECT && balance_rep_template >= 0 && balance_rep_template < analysis_balance_file->saved_report_count)
			analysis_open_rename_window(analysis_balance_file, balance_rep_template, pointer);
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
			open_account_lookup_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		break;

	case ANALYSIS_BALANCE_INCOMINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			open_account_lookup_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		break;

	case ANALYSIS_BALANCE_OUTGOINGPOPUP:
		if (pointer->buttons == wimp_CLICK_SELECT)
			open_account_lookup_window(analysis_balance_file, analysis_balance_window,
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
			open_account_lookup_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_ACCOUNTS, NULL_ACCOUNT, ACCOUNT_FULL);
		else if (key->i == ANALYSIS_BALANCE_INCOMING)
			open_account_lookup_window(analysis_balance_file, analysis_balance_window,
					ANALYSIS_BALANCE_INCOMING, NULL_ACCOUNT, ACCOUNT_IN);
		else if (key->i == ANALYSIS_BALANCE_OUTGOING)
			open_account_lookup_window(analysis_balance_file, analysis_balance_window,
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

static void analysis_fill_balance_window(file_data *file, osbool restore)
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

		convert_date_to_string(balance_rep_settings.date_from,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATEFROM));
		convert_date_to_string(balance_rep_settings.date_to,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATETO));
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_BUDGET, balance_rep_settings.budget);

		/* Set the grouping icons. */

		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_GROUP, balance_rep_settings.group);

		icons_printf(analysis_balance_window, ANALYSIS_BALANCE_PERIOD, "%d", balance_rep_settings.period);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PDAYS, balance_rep_settings.period_unit == PERIOD_DAYS);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS,
				balance_rep_settings.period_unit == PERIOD_MONTHS);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_PYEARS, balance_rep_settings.period_unit == PERIOD_YEARS);
		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_LOCK, balance_rep_settings.lock);

		/* Set the accounts and format details. */

		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_ACCOUNTS),
				balance_rep_settings.accounts, balance_rep_settings.accounts_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_INCOMING),
				balance_rep_settings.incoming, balance_rep_settings.incoming_count);
		analysis_account_list_to_idents(file,
				icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_OUTGOING),
				balance_rep_settings.outgoing, balance_rep_settings.outgoing_count);

		icons_set_selected(analysis_balance_window, ANALYSIS_BALANCE_TABULAR, balance_rep_settings.tabular);
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

	analysis_balance_file->balance_rep.date_from =
			convert_string_to_date (icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATEFROM), NULL_DATE, 0);
	analysis_balance_file->balance_rep.date_to =
			convert_string_to_date (icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_DATETO), NULL_DATE, 0);
	analysis_balance_file->balance_rep.budget = icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_BUDGET);

	/* Read the grouping settings. */

	analysis_balance_file->balance_rep.group = icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_GROUP);
	analysis_balance_file->balance_rep.period = atoi(icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_PERIOD));

	if (icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_PDAYS))
		analysis_balance_file->balance_rep.period_unit = PERIOD_DAYS;
	else if (icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_PMONTHS))
		analysis_balance_file->balance_rep.period_unit = PERIOD_MONTHS;
	else if (icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_PYEARS))
 		 analysis_balance_file->balance_rep.period_unit = PERIOD_YEARS;
	else
		analysis_balance_file->balance_rep.period_unit = PERIOD_MONTHS;

	analysis_balance_file->balance_rep.lock = icons_get_selected (analysis_balance_window, ANALYSIS_BALANCE_LOCK);

	/* Read the account and heading settings. */

	analysis_balance_file->balance_rep.accounts_count =
			analysis_account_idents_to_list(analysis_balance_file, ACCOUNT_FULL,
			icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_ACCOUNTS),
			analysis_balance_file->balance_rep.accounts);
	analysis_balance_file->balance_rep.incoming_count =
			analysis_account_idents_to_list(analysis_balance_file, ACCOUNT_IN,
			icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_INCOMING),
			analysis_balance_file->balance_rep.incoming);
	analysis_balance_file->balance_rep.outgoing_count =
			analysis_account_idents_to_list(analysis_balance_file, ACCOUNT_OUT,
			icons_get_indirected_text_addr(analysis_balance_window, ANALYSIS_BALANCE_OUTGOING),
			analysis_balance_file->balance_rep.outgoing);

	analysis_balance_file->balance_rep.tabular = icons_get_selected(analysis_balance_window, ANALYSIS_BALANCE_TABULAR);

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
	if (balance_rep_template >= 0 && balance_rep_template < analysis_balance_file->saved_report_count &&
			error_msgs_report_question("DeleteTemp", "DeleteTempB") == 1) {
		analysis_delete_saved_report_template(analysis_balance_file, balance_rep_template);
		balance_rep_template = NULL_TEMPLATE;

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

static void analysis_generate_balance_report(file_data *file)
{
	int		i, acc, items, unit, period, group, lock, tabular, total;
	char		line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
	date_t		start_date, end_date, next_start, next_end;
	report_data	*report;
	int		entry, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};

	hourglass_on();

	if (!(file->sort_valid))
		sort_transactions(file);

	/* Read the date settings. */

	analysis_find_date_range(file, &start_date, &end_date, file->balance_rep.date_from, file->balance_rep.date_to, file->balance_rep.budget);

	/* Read the grouping settings. */

	group = file->balance_rep.group;
	unit = file->balance_rep.period_unit;
	lock = file->balance_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
	period = (group) ? file->balance_rep.period : 0;

	/* Read the include list. */

	analysis_clear_account_report_flags(file);

	if (file->balance_rep.accounts_count == 0 && file->balance_rep.incoming_count == 0 && file->balance_rep.outgoing_count == 0) {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT, REPORT_INCLUDE, &wildcard_account_list, 1);
	} else {
		analysis_set_account_report_flags_from_list(file, ACCOUNT_FULL, REPORT_INCLUDE, file->balance_rep.accounts, file->balance_rep.accounts_count);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_IN, REPORT_INCLUDE, file->balance_rep.incoming, file->balance_rep.incoming_count);
		analysis_set_account_report_flags_from_list(file, ACCOUNT_OUT, REPORT_INCLUDE, file->balance_rep.outgoing, file->balance_rep.outgoing_count);
	}

	tabular = file->balance_rep.tabular;

	/* Count the number of accounts and headings to be included.  If this comes to more than the number of tab
	 * stops available (including 2 for account name and total), force the tabular format option off.
	 */

	items = 0;

	for (i=0; i < file->account_count; i++)
		if ((file->accounts[i].report_flags & REPORT_INCLUDE) != 0)
			items++;

	if ((items + 2) > REPORT_TAB_STOPS)
		tabular = FALSE;

	/* Start to output the report details. */

	analysis_copy_balance_template(&(saved_report_template.data.balance), &(file->balance_rep));
	if (balance_rep_template == NULL_TEMPLATE)
		*(saved_report_template.name) = '\0';
	else
		strcpy(saved_report_template.name, file->saved_reports[balance_rep_template].name);
	saved_report_template.type = REPORT_TYPE_BALANCE;

	msgs_lookup("BRWinT", line, sizeof(line));
	report = report_open(file, line, &saved_report_template);

	if (report == NULL) {
		hourglass_off();
		return;
	}

	/* Output report heading */

	make_file_leafname(file, b1, sizeof(b1));
	if (*saved_report_template.name != '\0')
		msgs_param_lookup("GRTitle", line, sizeof(line), saved_report_template.name, b1, NULL, NULL);
	else
		msgs_param_lookup("BRTitle", line, sizeof(line), b1, NULL, NULL, NULL);
	report_write_line(report, 0, line);

	convert_date_to_string(start_date, b1);
	convert_date_to_string(end_date, b2);
	convert_date_to_string(get_current_date(), b3);
	msgs_param_lookup("BRHeader", line, sizeof(line), b1, b2, b3, NULL);
	report_write_line(report, 0, line);

	/* Start to output the report. */

	if (tabular) {
		report_write_line(report, 0, "");
		msgs_lookup("BRDate", b1, sizeof(b1));
		sprintf(line, "\\b%s", b1);

		for (acc_group = 0; acc_group < groups; acc_group++) {
			entry = find_accounts_window_entry_from_type(file, sequence[acc_group]);

			for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++) {
				if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA) {
					acc = file->account_windows[entry].line_data[group_line].account;

					if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0) {
						sprintf(b1, "\\t\\r\\b%s", file->accounts[acc].name);
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

		for (i=0; i < file->account_count; i++)
			file->accounts[i].report_total = file->accounts[i].opening_balance;

		/* Scan through the transactions, adding the values up for those occurring before the end of the current
		 * period and outputting them to the screen.
		 */

		for (i=0; i < file->trans_count; i++) {
			if (next_end == NULL_DATE || file->transactions[i].date <= next_end) {
				if (file->transactions[i].from != NULL_ACCOUNT)
					file->accounts[file->transactions[i].from].report_total -= file->transactions[i].amount;

				if (file->transactions[i].to != NULL_ACCOUNT)
					file->accounts[file->transactions[i].to].report_total += file->transactions[i].amount;
			}
		}

		/* Print the transaction summaries. */

		if (tabular) {
			strcpy(line, date_text);

			total = 0;


			for (acc_group = 0; acc_group < groups; acc_group++) {
				entry = find_accounts_window_entry_from_type(file, sequence[acc_group]);

				for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++) {
					if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA) {
						acc = file->account_windows[entry].line_data[group_line].account;

						if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0) {
							total += file->accounts[acc].report_total;
							full_convert_money_to_string(file->accounts[acc].report_total, b1, TRUE);
							sprintf(b2, "\\t\\d\\r%s", b1);
							strcat(line, b2);
						}
					}
				}
			}
			full_convert_money_to_string(total, b1, TRUE);
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
				entry = find_accounts_window_entry_from_type(file, sequence[acc_group]);

				for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++) {
					if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA) {
						acc = file->account_windows[entry].line_data[group_line].account;

						if (file->accounts[acc].report_total != 0 && (file->accounts[acc].report_flags & REPORT_INCLUDE) != 0) {
							total += file->accounts[acc].report_total;
							full_convert_money_to_string(file->accounts[acc].report_total, b1, TRUE);
							sprintf(line, "\\i%s\\t\\d\\r%s", file->accounts[acc].name, b1);
							report_write_line(report, 2, line);
						}
					}
				}
			}
			msgs_lookup("BRTotal", b1, sizeof(b1));
			full_convert_money_to_string(total, b2, TRUE);
			sprintf(line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
			report_write_line(report, 2, line);
		}
	}

	report_close(report);

	hourglass_off();
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

static void analysis_find_date_range(file_data *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, osbool budget)
{
	int		i;
	osbool		find_start, find_end;

	if (budget) {
		/* Get the start and end dates from the budget settings. */

		*start_date = file->budget.start;
		*end_date = file->budget.finish;
	} else {
		/* Get the start and end dates from the icon text. */

		*start_date = date1;
		*end_date = date2;
	}

	find_start = (*start_date == NULL_DATE);
	find_end = (*end_date == NULL_DATE);

	/* If either of the dates wasn't specified, we need to find the earliest and latest dates in the file. */

	if (find_start || find_end) {
		if (find_start)
			*start_date = (file->trans_count > 0) ? file->transactions[0].date : NULL_DATE;

		if (find_end)
			*end_date = (file->trans_count > 0) ? file->transactions[0].date : NULL_DATE;

		for (i=0; i<file->trans_count; i++) {
			if (find_start && file->transactions[i].date != NULL_DATE && file->transactions[i].date < *start_date)
				*start_date = file->transactions[i].date;

			if (find_end && file->transactions[i].date != NULL_DATE && file->transactions[i].date > *end_date)
				*end_date = file->transactions[i].date;
		}
	}

	if (*start_date == NULL_DATE)
		*start_date = MIN_DATE;

	if (*end_date == NULL_DATE)
		*end_date = MAX_DATE;
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

static void analysis_initialise_date_period(date_t start, date_t end, int period, int unit, osbool lock)
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
			*next_end = add_to_date(analysis_period_start, analysis_period_unit, analysis_period_length - 1);

			switch (analysis_period_unit) {
			case PERIOD_MONTHS:
				*next_end = (*next_end & 0xffffff00) | 0x001f; /* Maximise the days, so end of month. */
				break;

			case PERIOD_YEARS:
				*next_end = (*next_end & 0xffff0000) | 0x0c1f; /* Maximise the days and months, so end of year. */
				break;

			default:
				*next_end = add_to_date(analysis_period_start, analysis_period_unit, analysis_period_length) - 1;
				break;
			}
		} else {
			*next_end = add_to_date(analysis_period_start, analysis_period_unit, analysis_period_length) - 1;
		}

		/* Pull back into range isf we fall off the end. */

		if (*next_end > analysis_period_end)
			*next_end = analysis_period_end;
	} else {
		/* If the report is not to be grouped, the next_end date is just the end of the report period. */

		*next_end = analysis_period_end;
	}

	/* Get the real start and end dates for the period. */

	*next_start = get_valid_date(analysis_period_start, +1);
	*next_end = get_valid_date(*next_end, -1);

	if (analysis_period_length > 0) {
		/* If the report is grouped, find the next start date by adding the period on to the current start date. */

		analysis_period_start = add_to_date(analysis_period_start, analysis_period_unit, analysis_period_length);

		if (analysis_period_first) {
			/* If the report is calendar locked, and this is the first iteration, reset the DAYS or DAYS+MONTHS
			 * to one so that the start date will be locked on to the calendar from now on.
			 */

			switch (analysis_period_unit) {
			case PERIOD_MONTHS:
				analysis_period_start = (analysis_period_start & 0xffffff00) | 0x0001; /* Set the days to one. */
				break;

			case PERIOD_YEARS:
				analysis_period_start = (analysis_period_start & 0xffff0000) | 0x0101; /* Set the days and months to one. */
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
		case PERIOD_MONTHS:
			convert_date_to_month_string(*next_start, b1);

			if ((*next_start & 0xffffff00) == (*next_end & 0xffffff00)) {
				msgs_param_lookup("PRMonth", date_text, date_len, b1, NULL, NULL, NULL);
			} else {
				convert_date_to_month_string (*next_end, b2);
				msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
			}
			break;

		case PERIOD_YEARS:
			convert_date_to_year_string(*next_start, b1);

			if ((*next_start & 0xffff0000) == (*next_end & 0xffff0000)) {
				msgs_param_lookup("PRYear", date_text, date_len, b1, NULL, NULL, NULL);
			} else {
				convert_date_to_year_string(*next_end, b2);
				msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
			}
			break;
		}
	} else {
		if (*next_start == *next_end) {
			convert_date_to_string(*next_start, b1);
			msgs_param_lookup("PRDay", date_text, date_len, b1, NULL, NULL, NULL);
		} else {
			convert_date_to_string(*next_start, b1);
			convert_date_to_string(*next_end, b2);
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

void analysis_remove_account_from_templates(file_data *file, acct_t account)
{
	int		i;
	report_data	*report;

	/* Handle the dialogue boxes. */

	analysis_remove_account_from_list(file, account, file->trans_rep.from, &(file->trans_rep.from_count));
	analysis_remove_account_from_list(file, account, file->trans_rep.to, &(file->trans_rep.to_count));

	analysis_remove_account_from_list(file, account, file->unrec_rep.from, &(file->unrec_rep.from_count));
	analysis_remove_account_from_list(file, account, file->unrec_rep.to, &(file->unrec_rep.to_count));

	analysis_remove_account_from_list(file, account, file->cashflow_rep.accounts, &(file->cashflow_rep.accounts_count));
	analysis_remove_account_from_list(file, account, file->cashflow_rep.incoming, &(file->cashflow_rep.incoming_count));
	analysis_remove_account_from_list(file, account, file->cashflow_rep.outgoing, &(file->cashflow_rep.outgoing_count));

	analysis_remove_account_from_list(file, account, file->balance_rep.accounts, &(file->balance_rep.accounts_count));
	analysis_remove_account_from_list(file, account, file->balance_rep.incoming, &(file->balance_rep.incoming_count));
	analysis_remove_account_from_list(file, account, file->balance_rep.outgoing, &(file->balance_rep.outgoing_count));

	/* Now process any saved templates. */

	for (i=0; i<file->saved_report_count; i++) {
		switch (file->saved_reports[i].type) {
		case REPORT_TYPE_TRANS:
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.transaction.from,
					&(file->saved_reports[i].data.transaction.from_count));
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.transaction.to,
					&(file->saved_reports[i].data.transaction.to_count));
			break;

		case REPORT_TYPE_UNREC:
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.unreconciled.from,
					&(file->saved_reports[i].data.unreconciled.from_count));
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.unreconciled.to,
					&(file->saved_reports[i].data.unreconciled.to_count));
			break;

		case REPORT_TYPE_CASHFLOW:
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.cashflow.accounts,
					&(file->saved_reports[i].data.cashflow.accounts_count));
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.cashflow.incoming,
					&(file->saved_reports[i].data.cashflow.incoming_count));
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.cashflow.outgoing,
					&(file->saved_reports[i].data.cashflow.outgoing_count));
			break;

		case REPORT_TYPE_BALANCE:
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.balance.accounts,
					&(file->saved_reports[i].data.balance.accounts_count));
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.balance.incoming,
					&(file->saved_reports[i].data.balance.incoming_count));
			analysis_remove_account_from_list(file, account, file->saved_reports[i].data.balance.outgoing,
					&(file->saved_reports[i].data.balance.outgoing_count));
			break;
		}
	}

	/* Finally, work through any reports in the file. */

	report = file->reports;

	while (report != NULL) {
		switch (report->template.type) {
		case REPORT_TYPE_TRANS:
			analysis_remove_account_from_list(file, account, report->template.data.transaction.from,
					&(report->template.data.transaction.from_count));
			analysis_remove_account_from_list(file, account, report->template.data.transaction.to,
					&(report->template.data.transaction.to_count));
			break;

		case REPORT_TYPE_UNREC:
			analysis_remove_account_from_list(file, account, report->template.data.unreconciled.from,
					&(report->template.data.unreconciled.from_count));
			analysis_remove_account_from_list(file, account, report->template.data.unreconciled.to,
					&(report->template.data.unreconciled.to_count));
			break;

		case REPORT_TYPE_CASHFLOW:
			analysis_remove_account_from_list(file, account, report->template.data.cashflow.accounts,
					&(report->template.data.cashflow.accounts_count));
			analysis_remove_account_from_list(file, account, report->template.data.cashflow.incoming,
					&(report->template.data.cashflow.incoming_count));
			analysis_remove_account_from_list(file, account, report->template.data.cashflow.outgoing,
					&(report->template.data.cashflow.outgoing_count));
			break;
		case REPORT_TYPE_BALANCE:
			analysis_remove_account_from_list(file, account, report->template.data.balance.accounts,
					&(report->template.data.balance.accounts_count));
			analysis_remove_account_from_list(file, account, report->template.data.balance.incoming,
					&(report->template.data.balance.incoming_count));
			analysis_remove_account_from_list(file, account, report->template.data.balance.outgoing,
					&(report->template.data.balance.outgoing_count));
			break;
		}

		report = report->next;
	}
}


/**
 * Remove any references to an account from an account list array.
 *
 * \param *file			The file to process.
 * \param account		The account to remove, if present.
 * \param *array		The account list array.
 * \param *count		Pointer to number of accounts in the array, which
 *				is updated before return.
 * \return			The new account count in the array.
 */

static int analysis_remove_account_from_list(file_data *file, acct_t account, acct_t *array, int *count)
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
 * \param *file			The file to clear.
 */

static void analysis_clear_account_report_flags(file_data *file)
{
	int	i;

	for (i=0; i < file->account_count; i++)
		file->accounts[i].report_flags = 0;
}


/**
 * Set the specified report flags for all accounts that match the list given.
 * The account NULL_ACCOUNT will set all the acounts that match the given type.
 *
 * \param *file			The file to process.
 * \param type			The type(s) of account to match for NULL_ACCOUNT.
 * \param flags			The report flags to set for matching accounts.
 * \param *array		The account list array to use.
 * \param count			The number of accounts in the account list.
 */

static void analysis_set_account_report_flags_from_list(file_data *file, unsigned type, unsigned flags, acct_t *array, int count)
{
	int	account, i;

	for (i=0; i<count; i++) {
		account = array[i];

		if (account == NULL_ACCOUNT) {
			/* 'Wildcard': set all the accounts which match the
			 * given account type.
			 */

			for (account=0; account < file->account_count; account++)
				if ((file->accounts[account].type & type) != 0)
					file->accounts[account].report_flags |= flags;
		} else {
			/* Set a specific account. */

			if (account < file->account_count)
				file->accounts[account].report_flags |= flags;
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
 *				with space for REPORT_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

static int analysis_account_idents_to_list(file_data *file, unsigned type, char *list, acct_t *array)
{
	char	*copy, *ident;
	int	account, i = 0;

	copy = strdup(list);

	if (copy == NULL)
		return 0;

	ident = strtok(copy, ",");

	while (ident != NULL && i < REPORT_ACC_LIST_LEN) {
		if (strcmp(ident, "*") == 0) {
			array[i++] = NULL_ACCOUNT;
		} else {
			account = find_account(file, ident, type);

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
 *				list, which must be REPORT_ACC_SPEC_LEN long.
 * \param *array		The account list array to be converted.
 * \param len			The number of accounts in the list.
 */

static void analysis_account_list_to_idents(file_data *file, char *list, acct_t *array, int len)
{
	char	buffer[ACCOUNT_IDENT_LEN];
	int	account, i;

	*list = '\0';

	for (i=0; i<len; i++) {
		account = array[i];

		if (account != NULL_ACCOUNT)
			strcpy(buffer, find_account_ident(file, account));
		else
			strcpy(buffer, "*");

		if (strlen(list) > 0 && strlen(list)+1 < REPORT_ACC_SPEC_LEN)
			strcat(list, ",");

		if (strlen(list) + strlen(buffer) < REPORT_ACC_SPEC_LEN)
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
 *				with space for REPORT_ACC_LIST_LEN entries.
 * \return			The number of entries added to the list.
 */

int analysis_account_hex_to_list(file_data *file, char *list, acct_t *array)
{
	char	*copy, *value;
	int	i = 0;

	copy = strdup(list);

	if (copy == NULL)
		return 0;

	value = strtok(copy, ",");

	while (value != NULL && i < REPORT_ACC_LIST_LEN) {
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

void analysis_account_list_to_hex(file_data *file, char *list, size_t size, acct_t *array, int len)
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
 * \param *report	The report to be saved into the template.
 * \param *ptr		The current Wimp Pointer details.
 */

void analysis_open_save_window(report_data *report, wimp_pointer *ptr)
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
	 * can only be done via this dislogue.  In the (unlikely) event that the Save dialogue is open when the last
	 * report is deleted, then the popup remains active but no memu will appear...
	 */

	icons_set_shaded(analysis_save_window, ANALYSIS_SAVE_NAMEPOPUP, report->file->saved_report_count == 0);

	analysis_fill_save_window(report);

	ihelp_set_modifier(analysis_save_window, "Sav");

	/* Set the pointers up so we can find this lot again and open the window. */

	save_report_file = report->file;
	save_report_report = report;
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

static void analysis_open_rename_window(file_data *file, int template, wimp_pointer *ptr)
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

	save_report_file = file;
	save_report_template = template;
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

	template_menu = analysis_template_menu_build(save_report_file, TRUE);
	event_set_menu_block(template_menu);
	templates_set_menu(TEMPLATES_MENU_TEMPLATES, template_menu);
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
}


/**
 * Refresh the contents of the current Save/Rename Report window.
 */

static void analysis_refresh_save_window(void)
{
	switch (analysis_save_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		analysis_fill_save_window(save_report_report);
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		analysis_fill_rename_window(save_report_file, save_report_template);
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
 * \param: *report		The report to be saved.
 */

static void analysis_fill_save_window(report_data *report)
{
	icons_strncpy(analysis_save_window, ANALYSIS_SAVE_NAME, report->template.name);
}


/**
 * Fill the Rename Template Window with values.
 *
 * \param: *file		The file containing the template.
 * \param: template		The template to be renamed.
 */

static void analysis_fill_rename_window(file_data *file, int template)
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

	template = analysis_find_saved_report_template_from_name(save_report_file,
			icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_NAME));

	switch (analysis_save_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		if (template != NULL_TEMPLATE && error_msgs_report_question("CheckTempOvr", "CheckTempOvrB") == 2)
			return FALSE;

		strcpy(save_report_report->template.name, icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_NAME));
		analysis_store_saved_report_template(save_report_file, &(save_report_report->template), template);
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		if (save_report_template != NULL_TEMPLATE) {
			if (template != NULL_TEMPLATE && template != save_report_template) {
				error_msgs_report_error("TempExists");
				return FALSE;
			}

			strcpy(save_report_file->saved_reports[save_report_template].name,
					icons_get_indirected_text_addr(analysis_save_window, ANALYSIS_SAVE_NAME));

			/* Update the window title of the parent report dialogue. */

			switch (save_report_file->saved_reports[save_report_template].type) {
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
						save_report_file->saved_reports[save_report_template].name, NULL, NULL, NULL);
						xwimp_force_redraw_title(w); /* Nested Wimp only! */
			}

			/* Mark the file has being modified. */

			set_file_data_integrity(save_report_file, 1);
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

wimp_menu *analysis_template_menu_build(file_data *file, osbool standalone)
{
	int	line, width;

	/* Claim enough memory to build the menu in. */

	analysis_template_menu_destroy();

	if (file->saved_report_count > 0) {
		analysis_template_menu = heap_alloc(28 + 24 * file->saved_report_count);
		analysis_template_menu_link = heap_alloc(file->saved_report_count * sizeof(struct analysis_template_link));
		analysis_template_menu_title = heap_alloc(ACCOUNT_MENU_TITLE_LEN);
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

	qsort(analysis_template_menu_link, line, sizeof(struct analysis_template_link), analysis_template_menu_compare_entries);

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

	msgs_lookup((standalone) ? "RepListMenuT2" : "RepListMenuT1", analysis_template_menu_title, ACCOUNT_MENU_TITLE_LEN);
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
	struct analysis_template_link *a = (struct analysis_template_link *) va;
	struct analysis_template_link *b = (struct analysis_template_link *) vb;

	return (string_nocase_strcmp(a->name, b->name));
}



















/* ==================================================================================================================
 * Force the closure of the report format window if the file disappears.
 */

void force_close_report_windows (file_data *file)
{
  if (analysis_transaction_file == file && windows_get_open (analysis_transaction_window))
  {
    close_dialogue_with_caret (analysis_transaction_window);
  }

  if (analysis_unreconciled_file == file && windows_get_open (analysis_unreconciled_window))
  {
    close_dialogue_with_caret (analysis_unreconciled_window);
  }

  if (analysis_cashflow_file == file && windows_get_open (analysis_cashflow_window))
  {
    close_dialogue_with_caret (analysis_cashflow_window);
  }

  if (analysis_balance_file == file && windows_get_open (analysis_balance_window))
  {
    close_dialogue_with_caret (analysis_balance_window);
  }

  if (save_report_file == file && windows_get_open (analysis_save_window))
  {
    close_dialogue_with_caret (analysis_save_window);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_force_close_report_save_window (file_data *file, report_data *report)
{
  if (analysis_save_mode == ANALYSIS_SAVE_MODE_SAVE &&
      save_report_file == file && save_report_report == report && windows_get_open (analysis_save_window))
  {
    close_dialogue_with_caret (analysis_save_window);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_force_close_report_rename_window (wimp_w window)
{
  if (windows_get_open (analysis_save_window) &&
      analysis_save_mode == ANALYSIS_SAVE_MODE_RENAME && save_report_template != NULL_TEMPLATE)
  {
    if ((window == analysis_transaction_window &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_TRANS) ||
        (window == analysis_unreconciled_window &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_UNREC) ||
        (window == analysis_cashflow_window &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_CASHFLOW) ||
        (window == analysis_balance_window &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_BALANCE))
    {
      close_dialogue_with_caret (analysis_save_window);
    }
  }
}

/* ==================================================================================================================
 * Saved template handling.
 */

/* Open a report from a saved template. */

void analysis_open_saved_report_dialogue(file_data *file, wimp_pointer *ptr, int selection)
{
	int template;

	if (analysis_template_menu_link == NULL || selection < 0 || selection >= file->saved_report_count)
		return;

	template = analysis_template_menu_link[selection].template;

  if (template >= 0 && template < file->saved_report_count)
  {
    switch (file->saved_reports[template].type)
    {
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
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */
/* Find a saved template based on its name. */

int analysis_find_saved_report_template_from_name (file_data *file, char *name)
{
  int i, found = -1;

  if (file != NULL && file->saved_report_count > 0)
  {
    for (i=0; i<file->saved_report_count && found == -1; i++)
    {
      if (string_nocase_strcmp (file->saved_reports[i].name, name) == 0)
      {
        found = i;
      }
    }
  }

  return (found);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_store_saved_report_template (file_data *file, saved_report *report, int number)
{

  if (number == NULL_TEMPLATE)
  {
    if (flex_extend ((flex_ptr) &(file->saved_reports), sizeof (saved_report) * (file->saved_report_count+1)) == 1)
    {
      number = file->saved_report_count++;
    }
    else
    {
      error_msgs_report_error ("NoMemNewTemp");
    }
  }

  if (number >= 0 && number < file->saved_report_count)
  {
    analysis_copy_template(&(file->saved_reports[number]), report);
    set_file_data_integrity (file, 1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_delete_saved_report_template (file_data *file, int template)
{
  int type;

  /* delete the specified template. */

  if (template >= 0 && template < file->saved_report_count)
  {
    type = file->saved_reports[template].type;

    /* first remove the template from the flex block. */

    flex_midextend ((flex_ptr) &(file->saved_reports), (template + 1) * sizeof (saved_report), -sizeof (saved_report));
    file->saved_report_count--;
    set_file_data_integrity (file, 1);

    /* If the rename template window is open for this template, close it now before the pointer is lost. */

    if (windows_get_open (analysis_save_window) && analysis_save_mode == ANALYSIS_SAVE_MODE_RENAME &&
        template == save_report_template)
    {
      close_dialogue_with_caret (analysis_save_window);
    }

    /* Now adjust any other template pointers, which may be pointing further up the array.
     * In addition, if the rename template pointer (save_report_template) is pointing to the deleted
     * item, it needs to be unset.
     */

    if (type != REPORT_TYPE_TRANS && trans_rep_template > template)
    {
      trans_rep_template--;
    }
    if (type != REPORT_TYPE_UNREC && unrec_rep_template > template)
    {
      unrec_rep_template--;
    }
    if (type != REPORT_TYPE_CASHFLOW && cashflow_rep_template > template)
    {
      cashflow_rep_template--;
    }
    if (type != REPORT_TYPE_BALANCE && balance_rep_template > template)
    {
      balance_rep_template--;
    }
    if (save_report_template > template)
    {
      save_report_template--;
    }
    else if (save_report_template == template)
    {
      save_report_template = NULL_TEMPLATE;
    }
  }
}













/**
 * Copy a Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

void analysis_copy_template(saved_report *to, saved_report *from)
{
	if (from == NULL || to == NULL)
		return;

	strcpy(to->name, from->name);
	to->type = from->type;

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
	}
}


/**
 * Copy a Transaction Report Template from one structure to another.
 *
 * \param *to			The template structure to take the copy.
 * \param *from			The template to be copied.
 */

static void analysis_copy_transaction_template(trans_rep *to, trans_rep *from)
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

static void analysis_copy_unreconciled_template(unrec_rep *to, unrec_rep *from)
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

static void analysis_copy_cashflow_template(cashflow_rep *to, cashflow_rep *from)
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

static void analysis_copy_balance_template(balance_rep *to, balance_rep *from)
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

