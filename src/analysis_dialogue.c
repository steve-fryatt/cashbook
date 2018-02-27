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
#include "analysis_dialogue.h"

#include "account.h"
#include "account_menu.h"
#include "analysis.h"
#include "analysis_lookup.h"
#include "analysis_template.h"
#include "analysis_template_save.h"
#include "caret.h"
#include "dialogue.h"


/**
 * An analysis dialogue definition.
 */

struct analysis_dialogue_block {
	struct dialogue_block			*dialogue;		/**< The dialogue instance.					*/
	struct analysis_dialogue_definition	*definition;		/**< The dialogue definition from the client.			*/
	struct analysis_block			*parent;		/**< The parent analysis instance.				*/
	template_t				template;		/**< The template associated with the dialogue.			*/
//	wimp_w					window;			/**< The Wimp window handle of the dialogue.			*/
	osbool					restore;		/**< The restore state for the dialogue.			*/
	void					*dialogue_settings;	/**< The settings block associated with the dialogue.		*/
	void					*file_settings;		/**< The settings block associated with the file instance.	*/
};

/* Static Function Prototypes. */

static void analysis_dialogue_click_handler(wimp_pointer *pointer);
static osbool analysis_dialogue_keypress_handler(wimp_key *key);
static osbool analysis_dialogue_process(struct analysis_dialogue_block *dialogue);
static osbool analysis_dialogue_delete(struct analysis_dialogue_block *dialogue);
static void analysis_dialogue_refresh(struct analysis_dialogue_block *dialogue);
static void analysis_dialogue_fill(struct analysis_dialogue_block *dialogue);
static void analysis_dialogue_place_caret(struct analysis_dialogue_block *dialogue);
static void analysis_dialogue_shade_icons(struct analysis_dialogue_block *dialogue, wimp_i target);
static void analysis_dialogue_hide_icons(struct analysis_dialogue_block *dialogue, enum analysis_dialogue_icon_type type, osbool hide);
static void analysis_dialogue_register_radio_icons(struct analysis_dialogue_block *dialogue);
static struct analysis_dialogue_icon *analysis_dialogue_find_icon(struct analysis_dialogue_block *dialogue, wimp_i icon);

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

	new->dialogue_settings = NULL;
	new->file_settings = NULL;
	new->definition = definition;
	new->parent = NULL;
	new->template = NULL_TEMPLATE;

	/* Claim a local template store to hold the live dialogue contents. */

	new->dialogue_settings = heap_alloc(definition->block_size);
	if (new->dialogue_settings == NULL) {
		heap_free(new);
		return NULL;
	}

	/* Create the dialogue window. */

	new->dialogue = dialogue_initialise(&(definition->dialogue));
	if (new->dialogue == NULL) {
		heap_free(new);
		return NULL;
	}

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
 *				These are assumed to belong to the file instance, and will
 *				be updated if the Generate button is clicked.
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

	/* If the window is already open, report is being edited.  Assume the user wants to lose
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
		report_details->copy_template(dialogue->dialogue_settings, analysis_template_get_data(template_block));
		dialogue->template = template;

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(dialogue->window),
				windows_get_indirected_title_length(dialogue->window),
				analysis_template_get_name(template_block, NULL, 0), NULL, NULL, NULL);

		/* If we use a template, we always want to reset to the template! */

		restore = TRUE;
	} else {
		report_details->copy_template(dialogue->dialogue_settings, settings);
		dialogue->template = NULL_TEMPLATE;

		msgs_lookup(dialogue->definition->title_token, windows_get_indirected_title_addr(dialogue->window),
				windows_get_indirected_title_length(dialogue->window));
	}

	/* Set the pointers up so we can find this lot again and open the window. */

	dialogue->parent = parent;
	dialogue->restore = restore;
	dialogue->file_settings = settings;

	/* Set the window contents up. */

	analysis_dialogue_hide_icons(dialogue, ANALYSIS_DIALOGUE_ICON_DELETE | ANALYSIS_DIALOGUE_ICON_RENAME, template_block == NULL);

	analysis_dialogue_fill(dialogue);

	windows_open_centred_at_pointer(dialogue->window, pointer);
	analysis_dialogue_place_caret(dialogue);
}


/**
 * Force an analysis dialogue instance to close if it is currently open
 * on screen.
 *
 * \param *dialogue		The dialogue instance to close.
 * \param *parent		If not NULL, only close the dialogue if
 *				this is the parent analysis instance.
 */

void analysis_dialogue_close(struct analysis_dialogue_block *dialogue, struct analysis_block *parent)
{
	if (dialogue == NULL || dialogue->parent != parent)
		return;

	if (windows_get_open(dialogue->window))
		close_dialogue_with_caret(dialogue->window);
}


/**
 * Update an analysis dialogue instance's template pointer if a template
 * is deleted from the parent analysis instance.
 *
 * \param *dialogue		The dialogue instance to process.
 * \param *parent		The analysis instance from which the template
 *				was deleted.
 * \param template		The template which was deleted.
 */

void analysis_dialogue_remove_template(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, template_t template)
{
	if (dialogue == NULL || dialogue->parent != parent)
		return;

	if (dialogue->template != NULL_TEMPLATE && dialogue->template > template)
		dialogue->template--;
}


/**
 * Tidy up after a template being renamed, by updating the window title
 * if the template belongs to this instance.
 *
 * \param *dialogue		The dialogue instance to check.
 * \param *parent		The parent analysis instance owning the
 *				renamed report.
 * \param template		The report being renamed.
 * \param *name			The new name for the report.
 */

void analysis_dialogue_rename_template(struct analysis_dialogue_block *dialogue, struct analysis_block *parent, template_t template, char *name)
{
	if (dialogue == NULL || dialogue->window == NULL || dialogue->parent != parent || dialogue->template != template)
		return;

	if (!windows_get_open(dialogue->window))
		return;

	msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(dialogue->window),
			windows_get_indirected_title_length(dialogue->window), name, NULL, NULL, NULL);
	xwimp_force_redraw_title(dialogue->window);
}


/**
 * Process mouse clicks in an analysis dialogue instance's window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_dialogue_click_handler(wimp_pointer *pointer)
{
	struct analysis_dialogue_block	*windat;
	struct analysis_dialogue_icon	*icon;

	windat = event_get_window_user_data(pointer->w);
	if (pointer == NULL || windat == NULL || windat->definition == NULL)
		return;

	icon = analysis_dialogue_find_icon(windat, pointer->i);
	if (icon == NULL)
		return;

	if (icon->type & ANALYSIS_DIALOGUE_ICON_CANCEL) {
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(windat->window);
			analysis_template_save_force_rename_close(windat->parent, windat->template);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_dialogue_refresh(windat);
		}
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_GENERATE) {
		if (analysis_dialogue_process(windat) && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(windat->window);
			analysis_template_save_force_rename_close(windat->parent, windat->template);
		}
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_DELETE) {
		if (pointer->buttons == wimp_CLICK_SELECT && analysis_dialogue_delete(windat))
			close_dialogue_with_caret(windat->window);
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_RENAME) {
		if (pointer->buttons == wimp_CLICK_SELECT && windat->template != NULL_TEMPLATE)
			analysis_template_save_open_rename_window(windat->parent, windat->template, pointer);
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_SHADE_TARGET) {
		analysis_dialogue_shade_icons(windat, pointer->i);
		icons_replace_caret_in_window(windat->window);
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_FROM) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != ANALYSIS_DIALOGUE_NO_ICON))
			analysis_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_TO) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != ANALYSIS_DIALOGUE_NO_ICON))
			analysis_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_IN) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != ANALYSIS_DIALOGUE_NO_ICON))
			analysis_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_IN);
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_OUT) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != ANALYSIS_DIALOGUE_NO_ICON))
			analysis_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_OUT);
	} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_FULL) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != ANALYSIS_DIALOGUE_NO_ICON))
			analysis_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_FULL);
	}
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
	struct analysis_dialogue_icon	*icon;

	windat = event_get_window_user_data(key->w);
	if (key == NULL || windat == NULL || windat->definition == NULL)
		return FALSE;

	icon = analysis_dialogue_find_icon(windat, key->i);
	if (icon == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_dialogue_process(windat)) {
			close_dialogue_with_caret(windat->window);
			analysis_template_save_force_rename_close(windat->parent, windat->template);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(windat->window);
		analysis_template_save_force_rename_close(windat->parent, windat->template);
		break;

	case wimp_KEY_F1:
		if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_FROM) {
			if (icon->target == ANALYSIS_DIALOGUE_NO_ICON)
				analysis_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_TO) {
			if (icon->target == ANALYSIS_DIALOGUE_NO_ICON)
				analysis_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
		} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_IN) {
			if (icon->target == ANALYSIS_DIALOGUE_NO_ICON)
				analysis_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_IN);
		} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_OUT) {
			if (icon->target == ANALYSIS_DIALOGUE_NO_ICON)
				analysis_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_OUT);
		} else if (icon->type & ANALYSIS_DIALOGUE_ICON_POPUP_FULL) {
			if (icon->target == ANALYSIS_DIALOGUE_NO_ICON)
				analysis_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_FULL);
		}
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process the contents of a dialogue and return it to the client.
 *
 * \param *dialogue		The dialogue instance to process.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool analysis_dialogue_process(struct analysis_dialogue_block *dialogue)
{
	struct analysis_report_details	*report_details;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->parent == NULL || dialogue->window == NULL)
		return FALSE;

	report_details = analysis_get_report_details(dialogue->definition->type);
	if (report_details == NULL)
		return FALSE;

	/* Request the client to read the data from the dialogue. */

	if (report_details->read_window != NULL)
		report_details->read_window(dialogue->parent, dialogue->window, dialogue->file_settings);

	/* Run the report itself */

	analysis_run_report(dialogue->parent, dialogue->definition->type, dialogue->file_settings, dialogue->template);

	return TRUE;
}


/**
 * Delete the template associated with a dialogue.
 *
 * \param *dialogue		The dialogue instance to process.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool analysis_dialogue_delete(struct analysis_dialogue_block *dialogue)
{
	struct analysis_template_block	*templates;

	if (dialogue == NULL || dialogue->parent == NULL || dialogue->template == NULL_TEMPLATE)
		return FALSE;

	templates = analysis_get_templates(dialogue->parent);
	if (templates == NULL)
		return FALSE;

	if (error_msgs_report_question("DeleteTemp", "DeleteTempB") != 3)
		return FALSE;

	if (!analysis_template_delete(templates, dialogue->template))
		return FALSE;

	dialogue->template = NULL_TEMPLATE;

	return TRUE;
}


/**
 * Request the client to fill a dialogue, update the shaded icons and then
 * redraw any fields which require it.
 *
 * \param *dialogue		The dialogue instance to refresh.
 */

static void analysis_dialogue_refresh(struct analysis_dialogue_block *dialogue)
{
	int				i;
	struct analysis_dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	analysis_dialogue_fill(dialogue);

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & ANALYSIS_DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != ANALYSIS_DIALOGUE_NO_ICON) && (icons[i].type & ANALYSIS_DIALOGUE_ICON_REFRESH))
			wimp_set_icon_state(dialogue->window, icons[i].icon, 0, 0);
	}

	icons_replace_caret_in_window(dialogue->window);
}


/**
 * Request the client to fill a dialogue, and update the shaded icons
 * based on the end result.
 *
 * \param *dialogue		The dialogue instance to fill.
 */

static void analysis_dialogue_fill(struct analysis_dialogue_block *dialogue)
{
	struct analysis_report_details	*report_details;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->parent == NULL || dialogue->window == NULL)
		return;

	report_details = analysis_get_report_details(dialogue->definition->type);
	if (report_details == NULL)
		return;

	/* Request the client to fill the dialogue. */

	if (report_details->fill_window != NULL)
		report_details->fill_window(dialogue->parent, dialogue->window, (dialogue->restore) ? dialogue->dialogue_settings : NULL);

	/* Update any shaded icons after the update. */

	analysis_dialogue_shade_icons(dialogue, ANALYSIS_DIALOGUE_NO_ICON);
}


/**
 * Place the caret into the first available writable icon in a dialogue.
 *
 * \param *dialogue		The dialogue instance to place the caret
 *				in.
 */

static void analysis_dialogue_place_caret(struct analysis_dialogue_block *dialogue)
{
	int				i;
	struct analysis_dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & ANALYSIS_DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != ANALYSIS_DIALOGUE_NO_ICON) && (icons[i].type & ANALYSIS_DIALOGUE_ICON_REFRESH) && !icons_get_shaded(dialogue->window, icons[i].icon)) {
			place_dialogue_caret(dialogue->window, icons[i].icon);
			return;
		}
	}

	place_dialogue_caret(dialogue->window, wimp_ICON_WINDOW);
}


/**
 * Update the shading of icons in a dialogue, based on the state of other
 * user selections.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param target		The target icon whose dependents are to be
 *				updated, or ANALYSIS_DIALOGUE_NO_ICON for all.
 */

static void analysis_dialogue_shade_icons(struct analysis_dialogue_block *dialogue, wimp_i target)
{
	int				i;
	struct analysis_dialogue_icon	*icons;
	osbool				include = FALSE;
	osbool				shaded = FALSE;
	wimp_i				icon = ANALYSIS_DIALOGUE_NO_ICON;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & ANALYSIS_DIALOGUE_ICON_END) == 0; i++) {
		if (icons[i].target == ANALYSIS_DIALOGUE_NO_ICON)
			continue;

		/* Reset the shaded state if this isn't an OR clause. */

		if (!(icons[i].type & ANALYSIS_DIALOGUE_ICON_SHADE_OR)) {
			icon = icons[i].icon;
			shaded = FALSE;
			include = FALSE;
		}

		if (target == ANALYSIS_DIALOGUE_NO_ICON || target == icons[i].target)
			include = TRUE;

		/* Update the state based on the icon. */

		if (icons[i].type & ANALYSIS_DIALOGUE_ICON_SHADE_ON) {
			shaded = shaded || icons_get_selected(dialogue->window, icons[i].target);
		} else if (icons[i].type & ANALYSIS_DIALOGUE_ICON_SHADE_OFF) {
			shaded = shaded || !icons_get_selected(dialogue->window, icons[i].target);
		} else {
			icon = ANALYSIS_DIALOGUE_NO_ICON;
			shaded = FALSE;
		}

		/* If the next icon isn't an AND clause, this is the end: update the icon. */

		if (!(icons[i + 1].type & ANALYSIS_DIALOGUE_ICON_SHADE_OR) && (icon != ANALYSIS_DIALOGUE_NO_ICON) && include)
			icons_set_shaded(dialogue->window, icon, shaded);
	}
}


/**
 * Set the hidden (deleted) state of any icons with the hide flag.
 *
 * \param *dialogue		The dialogue instance to process.
 * \param type			The type(s) of icon to be affected.
 * \param hide			TRUE to hide any affected icons; FALSE to show them.
 */

static void analysis_dialogue_hide_icons(struct analysis_dialogue_block *dialogue, enum analysis_dialogue_icon_type type, osbool hide)
{
	int				i;
	struct analysis_dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & ANALYSIS_DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != ANALYSIS_DIALOGUE_NO_ICON) && (icons[i].type & type))
			icons_set_deleted(dialogue->window, icons[i].icon, hide);
	}
}


/**
 * Register any icons declared as radio icons with the event handler.
 *
 * \param *dialogue		The dialogue instance to register.
 */

static void analysis_dialogue_register_radio_icons(struct analysis_dialogue_block *dialogue)
{
	int				i;
	struct analysis_dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & ANALYSIS_DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != ANALYSIS_DIALOGUE_NO_ICON) && (icons[i].type & ANALYSIS_DIALOGUE_ICON_RADIO))
			event_add_window_icon_radio(dialogue->window, icons[i].icon, TRUE);
		else if ((icons[i].icon != ANALYSIS_DIALOGUE_NO_ICON) && (icons[i].type & ANALYSIS_DIALOGUE_ICON_RADIO_PASS))
			event_add_window_icon_radio(dialogue->window, icons[i].icon, FALSE);
	}
}


/**
 * Find an icon within the dialogue definition, and return its details.
 *
 * \param *dialogue		The dialogue instance to search in.
 * \param icon			The icon to search for.
 * \return			The icon's definition, or NULL if not found.
 */

static struct analysis_dialogue_icon *analysis_dialogue_find_icon(struct analysis_dialogue_block *dialogue, wimp_i icon)
{
	int				i;
	struct analysis_dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return NULL;

	if (icon == wimp_ICON_WINDOW)
		return NULL;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & ANALYSIS_DIALOGUE_ICON_END) == 0; i++) {
		if (icons[i].icon == icon)
			return &(icons[i]);
	}

	return NULL;
}

