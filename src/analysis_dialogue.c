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
 * \file: analysis_dialogue.c
 *
 * Analysis report dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>

/* SF-Lib header files. */

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
#include "analysis_dialogue.h"

#include "account.h"
#include "account_menu.h"
#include "analysis.h"
#include "analysis_template.h"
#include "analysis_template_save.h"
#include "caret.h"


/**
 * An analysis dialogue definition.
 */

struct analysis_dialogue_block {
	struct analysis_dialogue_definition	*definition;		/**< The dialogue definition from the client.		*/
	struct analysis_block			*parent;		/**< The parent analysis instance.			*/
	template_t				template;		/**< The template associated with the dialogue.		*/
	wimp_w					window;			/**< The Wimp window handle of the dialogue.		*/
	osbool					restore;		/**< The restore state for the dialogue.		*/
	void					*settings_block;	/**< The settings block associated with the dialogue.	*/
};

/* Static Function Prototypes. */

static void analysis_dialogue_click_handler(wimp_pointer *pointer);
static osbool analysis_dialogue_keypress_handler(wimp_key *key);

/**
 * Initialise a new analysis dialogue window instance.
 *
 * \param *definition		The dialogue definition from the client.
 * \return			Pointer to the dialogue structure, or NULL on failure.
 */

struct analysis_dialogue_block *analysis_dialogue_initialise(struct analysis_dialogue_definition *definition)
{
	struct analysis_dialogue_block	*new;

	if (definition == NULL)
		return NULL;

	/* Create the instance. */

	new = heap_alloc(sizeof(struct analysis_dialogue_block));
	if (new == NULL)
		return NULL;

	new->settings_block = NULL;
	new->definition = definition;
	new->parent = NULL;
	new->template = NULL_TEMPLATE;

	/* Claim a local template store to hold the live dialogue contents. */

	new->settings_block = heap_alloc(definition->block_size);
	if (new->settings_block == NULL)
		return NULL;

	/* Create the dialogue window. */

	new->window = templates_create_window(definition->template_name);
	ihelp_add_window(new->window, definition->ihelp_token, NULL);
	event_add_window_user_data(new->window, new);
	event_add_window_mouse_event(new->window, analysis_dialogue_click_handler);
	event_add_window_key_event(new->window, analysis_dialogue_keypress_handler);

	return new;
}


/**
 * Open a new analysis dialogue.
 * 
 * \param *dialogue		The analysis dialogue instance to open.
 * \param *parent		The analysis instance to be the parent.
 * \param *ptr			The current Wimp Pointer details.
 * \param template		The report template to use for the dialogue.
 * \param *settings		The dialogue settings to use when no template available.
 * \param restore		TRUE to retain the last settings for the file; FALSE to
 *				use the application defaults.
 */

void analysis_dialogue_open(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, wimp_pointer *pointer, template_t template, void *settings, osbool restore)
{
	struct analysis_report		*template_block;
	struct analysis_template_block	*templates;
	struct analysis_report_details	*report_details;

	if (dialogue == NULL || dialogue->definition == NULL || parent == NULL || pointer == NULL)
		return;

	templates = analysis_get_templates(parent);
	if (templates == NULL)
		return;

	report_details = analysis_get_report_details(dialogue->definition->type);
	if (report_details == NULL)
		return;

	/* If the window is already open, another balance report is being edited.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret() as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(dialogue->window))
		wimp_close_window(dialogue->window);

	/* Copy the settings block contents into a static place that won't shift about on the flex heap
	 * while the dialogue is open.
	 */

	template_block = analysis_template_get_report(templates, template);

	if (template_block != NULL) {
		report_details->copy_template(dialogue->settings_block, analysis_template_get_data(template_block));
		dialogue->template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(dialogue->window),
				windows_get_indirected_title_length(dialogue->window),
				analysis_template_get_name(template_block, NULL, 0), NULL, NULL, NULL);

		/* If we use a template, we always want to reset to the template! */

		restore = TRUE;
	} else {
		report_details->copy_template(dialogue->settings_block, settings);
		dialogue->template = NULL_TEMPLATE;

		msgs_lookup("BalRepTitle", windows_get_indirected_title_addr(dialogue->window),
				windows_get_indirected_title_length(dialogue->window));
	}

//	icons_set_deleted(dialogue->window, ANALYSIS_BALANCE_DELETE, template_block == NULL);
//	icons_set_deleted(dialogue->window, ANALYSIS_BALANCE_RENAME, template_block == NULL);

	/* Set the window contents up. */

	if (report_details->fill_window != NULL)
		report_details->fill_window(parent, dialogue->window, dialogue->settings_block);

	/* Set the pointers up so we can find this lot again and open the window. */

	dialogue->parent = parent;
	dialogue->restore = restore;

	windows_open_centred_at_pointer(dialogue->window, pointer);
//	place_dialogue_caret_fallback(dialogue->window, 4, ANALYSIS_BALANCE_DATEFROM, ANALYSIS_BALANCE_DATETO,
//			ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_ACCOUNTS);
}


/**
 * Force an analysis dialogue instance to close if it is currently open
 * on screen.
 *
 * \param* dialogue		The dialogue instance to close.
 */

void analysis_dialogue_close(struct analysis_dialogue_block *dialogue)
{
	if (dialogue == NULL)
		return;

	if (windows_get_open(dialogue->window))
		close_dialogue_with_caret(dialogue->window);
}


/**
 * Process mouse clicks in an analysis dialogue instance's window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_dialogue_click_handler(wimp_pointer *pointer)
{
	struct analysis_dialogue_block	*windat;

	windat = event_get_window_user_data(pointer->w);
	if (pointer == NULL || windat == NULL || windat->definition == NULL)
		return;

	if (pointer->i == windat->definition->cancel_button) {
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(windat->window);
			analysis_template_save_force_rename_close(windat->parent, windat->template);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
	//		analysis_refresh_transaction_window();
		}
	} else if (pointer->i == windat->definition->generate_button) {
		if (/*analysis_process_transaction_window() &&*/ pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(windat->window);
			analysis_template_save_force_rename_close(windat->parent, windat->template);
		}
	} else if (pointer->i == windat->definition->delete_button) {
	//	if (pointer->buttons == wimp_CLICK_SELECT && analysis_delete_transaction_window())
	//		close_dialogue_with_caret(analysis_transaction_window);
	} else if (pointer->i == windat->definition->rename_button) {
	//	if (pointer->buttons == wimp_CLICK_SELECT && analysis_transaction_template >= 0 && analysis_transaction_template < analysis_transaction_file->saved_report_count)
	//		analysis_template_save_open_rename_window(analysis_transaction_file, analysis_transaction_template, pointer);
	} 


#if 0
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
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_transaction_template >= 0 && analysis_transaction_template < analysis_transaction_file->saved_report_count)
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
#endif
}


/**
 * Process keypresses in an analysis dialogue instance's window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool analysis_dialogue_keypress_handler(wimp_key *key)
{
	struct analysis_dialogue_block	*windat;

	windat = event_get_window_user_data(key->w);
	if (key == NULL || windat == NULL || windat->definition == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
	//	if (analysis_process_transaction_window()) {
			close_dialogue_with_caret(windat->window);
			analysis_template_save_force_rename_close(windat->parent, windat->template);
	//	}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(windat->window);
		analysis_template_save_force_rename_close(windat->parent, windat->template);
		break;
#if 0
	case wimp_KEY_F1:
		if (key->i == ANALYSIS_TRANS_FROMSPEC)
			analysis_lookup_open_window(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_FROMSPEC, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		else if (key->i == ANALYSIS_TRANS_TOSPEC)
			analysis_lookup_open_window(analysis_transaction_file, analysis_transaction_window,
					ANALYSIS_TRANS_TOSPEC, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
		break;
#endif
	default:
		return FALSE;
		break;
	}

	return TRUE;
}
