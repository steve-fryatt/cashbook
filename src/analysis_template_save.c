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
 * \file: analysis_template_save.c
 *
 * Analysis Template save and rename dialogue implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>

/* OSLib header files */

//#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "analysis_template_save.h"

#include "analysis.h"
#include "analysis_template.h"
#include "analysis_template_menu.h"
#include "caret.h"

/* Dialogue Icons. */

#define ANALYSIS_SAVE_OK 4
#define ANALYSIS_SAVE_CANCEL 3
#define ANALYSIS_SAVE_NAME 1
#define ANALYSIS_SAVE_NAMEPOPUP 2

/**
 * The possible modes which the dialogue can be in.
 */

enum analysis_template_save_mode {
	ANALYSIS_SAVE_MODE_NONE = 0,					/**< The Save/Rename dialogue isn't used.					*/
	ANALYSIS_SAVE_MODE_SAVE,					/**< The Save/Rename dialogue is in Save mode.					*/
	ANALYSIS_SAVE_MODE_RENAME					/**< The Save/Rename dialogue is in Rename mode.				*/
};

/**
 * The handle of the Save/Rename window.
 */

static wimp_w					analysis_template_save_window = NULL;

/**
 * The current mode of the Save/Rename window.
 */

static enum analysis_template_save_mode		analysis_template_save_current_mode = ANALYSIS_SAVE_MODE_NONE;

/**
 * The saved template instance currently owning the Save/Rename window.
 */

static struct analysis_template_block		*analysis_template_save_parent = NULL;

/**
 * The report currently owning the Save/Rename window.
 */

static struct analysis_report			*analysis_template_save_report = NULL;

/**
 * The template currently owning the Save/Rename window.
 */

static int					analysis_template_save_template = NULL_TEMPLATE;

/**
 * The active Saved Template menu, if one is open.
 */

static wimp_menu				*analysis_template_save_menu = NULL;

/* Static Function Prototypes. */

static void		analysis_template_save_click_handler(wimp_pointer *pointer);
static osbool		analysis_template_save_keypress_handler(wimp_key *key);
static void		analysis_template_save_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void		analysis_template_save_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void		analysis_template_save_menu_close_handler(wimp_w w, wimp_menu *menu);
static void		analysis_template_save_refresh_window(void);
static void		analysis_template_save_fill_window(struct analysis_report *template);
static osbool		analysis_template_save_process_window(void);


/**
 * Initialise the Template Save and Template Rename dialogue.
 */

void analysis_template_save_initialise(void)
{
	analysis_template_save_window = templates_create_window("SaveRepTemp");
	ihelp_add_window(analysis_template_save_window, "SaveRepTemp", NULL);
	event_add_window_mouse_event(analysis_template_save_window, analysis_template_save_click_handler);
	event_add_window_key_event(analysis_template_save_window, analysis_template_save_keypress_handler);
	event_add_window_menu_prepare(analysis_template_save_window, analysis_template_save_menu_prepare_handler);
	event_add_window_menu_selection(analysis_template_save_window, analysis_template_save_menu_selection_handler);
	event_add_window_menu_close(analysis_template_save_window, analysis_template_save_menu_close_handler);
	event_add_window_icon_popup(analysis_template_save_window, ANALYSIS_SAVE_NAMEPOPUP, NULL, -1, NULL);
}


/**
 * Open the Save Template dialogue box.
 *
 * \param *template		The report template to be saved.
 * \param *ptr			The current Wimp Pointer details.
 */

void analysis_template_save_open_window(struct analysis_report *template, wimp_pointer *ptr)
{
	/* If the window is already open, another report is being saved.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_template_save_window))
		wimp_close_window(analysis_template_save_window);

	/* Set the window contents up. */

	msgs_lookup("SaveRepTitle", windows_get_indirected_title_addr(analysis_template_save_window),
			windows_get_indirected_title_length(analysis_template_save_window));
	msgs_lookup("SaveRepSave", icons_get_indirected_text_addr(analysis_template_save_window, ANALYSIS_SAVE_OK),
			icons_get_indirected_text_length(analysis_template_save_window, ANALYSIS_SAVE_OK));

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_template_save_parent = analysis_template_get_instance(template);
	analysis_template_save_report = template;
	analysis_template_save_current_mode = ANALYSIS_SAVE_MODE_SAVE;

	/* The popup can be shaded here, as the only way its state can be changed is if a report is added: which
	 * can only be done via this dialogue.  In the (unlikely) event that the Save dialogue is open when the last
	 * report is deleted, then the popup remains active but no memu will appear...
	 */

	icons_set_shaded(analysis_template_save_window, ANALYSIS_SAVE_NAMEPOPUP, analysis_template_get_count(analysis_template_save_parent) == 0);

	analysis_template_save_fill_window(template);

	ihelp_set_modifier(analysis_template_save_window, "Sav");


	windows_open_centred_at_pointer(analysis_template_save_window, ptr);
	place_dialogue_caret_fallback(analysis_template_save_window, 1, ANALYSIS_SAVE_NAME);
}


/**
 * Open the Rename Template dialogue box.
 *
 * \param *file			The file owning the template.
 * \param template_number	The template to be renamed.
 * \param *ptr			The current Wimp Pointer details.
 */

void analysis_template_save_open_rename_window(struct file_block *file, int template_number, wimp_pointer *ptr)
{
	struct analysis_report	*template;

	/* If the window is already open, another report is being saved.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(analysis_template_save_window))
		wimp_close_window(analysis_template_save_window);

	/* Set the window contents up. */

	msgs_lookup("RenRepTitle", windows_get_indirected_title_addr(analysis_template_save_window),
			windows_get_indirected_title_length(analysis_template_save_window));
	msgs_lookup("RenRepRen", icons_get_indirected_text_addr(analysis_template_save_window, ANALYSIS_SAVE_OK),
			icons_get_indirected_text_length(analysis_template_save_window, ANALYSIS_SAVE_OK));

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_template_save_parent = analysis_get_templates(file->analysis);
	analysis_template_save_template = template_number;
	analysis_template_save_current_mode = ANALYSIS_SAVE_MODE_RENAME;

	/* The popup can be shaded here, as the only way its state can be changed is if a report is added: which
	 * can only be done via this dialogue.  In the (unlikely) event that the Save dialogue is open when the last
	 * report is deleted, then the popup remains active but no menu will appear...
	 */

	icons_set_shaded(analysis_template_save_window, ANALYSIS_SAVE_NAMEPOPUP, analysis_template_get_count(analysis_template_save_parent) == 0);

	// \TODO -- The line above allows for no templates, but here we're assuming that the'res a template?!

	template = analysis_template_get_report(analysis_template_save_parent, template_number);
	analysis_template_save_fill_window(template);

	ihelp_set_modifier(analysis_template_save_window, "Ren");

	windows_open_centred_at_pointer(analysis_template_save_window, ptr);
	place_dialogue_caret_fallback(analysis_template_save_window, 1, ANALYSIS_SAVE_NAME);
}


/**
 * Process mouse clicks in the Save/Rename Report dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void analysis_template_save_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ANALYSIS_SAVE_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_template_save_window);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			analysis_template_save_refresh_window();
		}
		break;

	case ANALYSIS_SAVE_OK:
		if (analysis_template_save_process_window() && pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(analysis_template_save_window);
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

static osbool analysis_template_save_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		if (analysis_template_save_process_window()) {
			close_dialogue_with_caret(analysis_template_save_window);
		}
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(analysis_template_save_window);
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

static void analysis_template_save_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	analysis_template_save_menu = analysis_template_menu_build(analysis_template_get_file(analysis_template_save_parent), TRUE);
	if (analysis_template_save_menu == NULL)
		return;

	event_set_menu_block(analysis_template_save_menu);
	ihelp_add_menu(analysis_template_save_menu, "RepListMenu");
}


/**
 * Process menu selection events in the Save/Rename Report window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void analysis_template_save_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	template_t		template_number;
	struct analysis_report	*template;
	char			*name;

	if (selection->items[0] == -1 || analysis_template_save_parent == NULL || analysis_template_save_menu != NULL)
		return;

	template_number = analysis_template_menu_decode(selection->items[0]);
	if (template_number == NULL_TEMPLATE)
		return;

	template = analysis_template_get_report(analysis_template_save_parent, template_number);
	if (template == NULL)
		return;

	name = analysis_template_get_name(template, NULL, 0);
	if (name == NULL)
		return;

	icons_strncpy(analysis_template_save_window, ANALYSIS_SAVE_NAME, name);
	icons_redraw_group(analysis_template_save_window, 1, ANALYSIS_SAVE_NAME);
	icons_replace_caret_in_window(analysis_template_save_window);
}


/**
 * Process menu close events in the Save/Rename Report window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void analysis_template_save_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	if (analysis_template_save_menu != NULL)
		return;

	analysis_template_menu_destroy();
	ihelp_remove_menu(menu);
	analysis_template_save_menu = NULL;
}


/**
 * Refresh the contents of the current Save/Rename Report window.
 */

static void analysis_template_save_refresh_window(void)
{
	struct analysis_report	*template;

	switch (analysis_template_save_current_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		analysis_template_save_fill_window(analysis_template_save_report);
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		template = analysis_template_get_report(analysis_template_save_parent, analysis_template_save_template);
		analysis_template_save_fill_window(template);
		break;

	default:
		break;
	}

	icons_redraw_group(analysis_template_save_window, 1, ANALYSIS_SAVE_NAME);

	icons_replace_caret_in_window(analysis_template_save_window);
}


/**
 * Fill the Save / Rename Report Window with values.
 *
 * \param: *template		The report template to be used.
 */

static void analysis_template_save_fill_window(struct analysis_report *template)
{
	char	*name;

	name = analysis_template_get_name(template, NULL, 0);
	if (name == NULL)
		return;

	icons_strncpy(analysis_template_save_window, ANALYSIS_SAVE_NAME, name);
}



/**
 * Process OK clicks in the Save/Rename Template window.  If it is a real save,
 * pass the call on to the store saved report function.  If it is a rename,
 * handle it directly here.
 */

static osbool analysis_template_save_process_window(void)
{
	template_t	template;
	char		*name;

	if (*icons_get_indirected_text_addr(analysis_template_save_window, ANALYSIS_SAVE_NAME) == '\0')
		return FALSE;

	template = analysis_template_get_from_name(analysis_template_save_parent,
			icons_get_indirected_text_addr(analysis_template_save_window, ANALYSIS_SAVE_NAME));

	switch (analysis_template_save_current_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		if (template != NULL_TEMPLATE && error_msgs_report_question("CheckTempOvr", "CheckTempOvrB") == 4)
			return FALSE;

		analysis_template_store(analysis_template_save_parent, analysis_template_save_report, template,
				icons_get_indirected_text_addr(analysis_template_save_window, ANALYSIS_SAVE_NAME));
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		if (analysis_template_save_template == NULL_TEMPLATE)
			break;

		if (template != NULL_TEMPLATE && template != analysis_template_save_template) {
			error_msgs_report_error("TempExists");
			return FALSE;
		}

		name = icons_get_indirected_text_addr(analysis_template_save_window, ANALYSIS_SAVE_NAME);

		analysis_template_rename(analysis_template_save_parent, analysis_template_save_template, name);

		msgs_param_lookup("GenRepTitle", windows_get_indirected_title_addr(analysis_template_save_window), windows_get_indirected_title_length(analysis_template_save_window), name, NULL, NULL, NULL);
		xwimp_force_redraw_title(analysis_template_save_window);
		break;

	default:
		break;
	}

	return TRUE;
}


/**
 * Report that a report template has been deleted, and adjust the
 * dialogue handle accordingly.
 *
 * \param *parent		The analysis instance from which the template has been deleted.
 * \param template		The deleted template ID.
 */

void analysis_template_save_delete_template(struct analysis_block *parent, template_t template)
{
	if (analysis_template_save_parent != parent ||
			analysis_template_save_template == NULL_TEMPLATE)
		return;

	if (analysis_template_save_template > template)
		analysis_template_save_template--;
	else if (analysis_template_save_template == template)
		analysis_template_save_template = NULL_TEMPLATE;
}


/**
 * Force the closure of the Save / Rename Template dialogue if the file
 * owning it closes.
 *
 * \param *parent		The parent analysis instance.
 */

void analysis_template_save_force_close(struct analysis_block *parent)
{
	if (analysis_template_save_parent == parent &&
			windows_get_open(analysis_template_save_window))
		close_dialogue_with_caret(analysis_template_save_window);
}

/**
 * Force the closure of the Save Template window if it is open to save the
 * given template.
 *
 * \param *template		The template of interest.
 */

void analysis_template_save_force_template_close(struct analysis_report *template)
{
	if (!windows_get_open(analysis_template_save_window) || template == NULL ||
			analysis_template_save_current_mode != ANALYSIS_SAVE_MODE_SAVE ||
			analysis_template_save_report != template)
		return;

	close_dialogue_with_caret(analysis_template_save_window);
}


/**
 * Force the closure of the Rename Template window if it is open to rename the
 * given template.
 *
 * \param *parent		The analysis block in which the template is held.
 * \param template		The template which is to be closed.
 */

void analysis_template_save_force_rename_close(struct analysis_block *parent, template_t template)
{
	if (!windows_get_open(analysis_template_save_window) ||
			analysis_template_save_parent != parent ||
			analysis_template_save_current_mode != ANALYSIS_SAVE_MODE_RENAME ||
			analysis_template_save_template != NULL_TEMPLATE)
		return;

	close_dialogue_with_caret(analysis_template_save_window);
}
