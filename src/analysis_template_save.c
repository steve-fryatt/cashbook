/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "dialogue.h"

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
 * The handle of the Save/Rename dialogue.
 */

static struct dialogue_block			*analysis_template_save_dialogue = NULL;

/**
 * The current mode of the Save/Rename dialogue.
 */

static enum analysis_template_save_mode		analysis_template_save_current_mode = ANALYSIS_SAVE_MODE_NONE;

/**
 * The saved template instance currently owning the Save/Rename dialogue.
 */

static struct analysis_template_block		*analysis_template_save_parent = NULL;

/**
 * The report currently owning the Save/Rename dialogue.
 */

static struct analysis_report			*analysis_template_save_report = NULL;

/**
 * The template currently owning the Save/Rename dialogue.
 */

static template_t				analysis_template_save_template = NULL_TEMPLATE;

/* Static Function Prototypes. */

static void		analysis_template_save_fill_window(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool		analysis_template_save_process_window(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void		analysis_template_save_window_close(struct file_block *file, wimp_w window, void *data);
static osbool		analysis_template_save_menu_prepare_handler(struct file_block *file, wimp_w window, wimp_i icon, struct dialogue_menu_data *menu, void *data);
static void		analysis_template_save_menu_selection_handler(struct file_block *file, wimp_w window, wimp_i icon, wimp_menu *menu, wimp_selection *selection, void *data);
static void		analysis_template_save_menu_close_handler(struct file_block *file, wimp_w window, wimp_menu *menu, void *data);

/**
 * The Save Template Dialogue Icon Set.
 */

static struct dialogue_icon analysis_template_save_icon_list[] = {
	{DIALOGUE_ICON_OK,	ANALYSIS_SAVE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,	ANALYSIS_SAVE_CANCEL,		DIALOGUE_NO_ICON},

	/* Saved Report Name Field. */

	{DIALOGUE_ICON_POPUP,	ANALYSIS_SAVE_NAMEPOPUP,	ANALYSIS_SAVE_NAME},
	{DIALOGUE_ICON_REFRESH,	ANALYSIS_SAVE_NAME,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,	DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};

/**
 * The Save Template Dialogue Definition.
 */

static struct dialogue_definition analysis_template_save_dialogue_definition = {
	"SaveRepTemp",
	"SaveRepTemp",
	analysis_template_save_icon_list,
	DIALOGUE_GROUP_NONE,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	analysis_template_save_fill_window,
	analysis_template_save_process_window,
	analysis_template_save_window_close,
	analysis_template_save_menu_prepare_handler,
	analysis_template_save_menu_selection_handler,
	analysis_template_save_menu_close_handler
};


/**
 * Initialise the Template Save and Template Rename dialogue.
 */

void analysis_template_save_initialise(void)
{
	analysis_template_save_dialogue = dialogue_create(&analysis_template_save_dialogue_definition);
}


/**
 * Open the Save Template dialogue box. When open, the dialogue's parent
 * object is the template handle of the template being renamed (ie. the
 * value passed as *template).
 *
 * \param *template		The report template to be saved.
 * \param *ptr			The current Wimp Pointer details.
 */

void analysis_template_save_open_window(struct analysis_report *template, wimp_pointer *ptr)
{
	/* Set the window contents up. */

	dialogue_set_title(analysis_template_save_dialogue, "SaveRepTitle", NULL, NULL, NULL, NULL);
	dialogue_set_icon_text(analysis_template_save_dialogue, DIALOGUE_ICON_OK, "SaveRepSave", NULL, NULL, NULL, NULL);
	dialogue_set_ihelp_modifier(analysis_template_save_dialogue, "Sav");

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_template_save_parent = analysis_template_get_instance(template);
	analysis_template_save_report = template;
	analysis_template_save_current_mode = ANALYSIS_SAVE_MODE_SAVE;

	/* Open the dialogue. */

	dialogue_open(analysis_template_save_dialogue, FALSE, analysis_template_get_file(analysis_template_save_parent), template, ptr, NULL);
}


/**
 * Open the Rename Template dialogue box. When open, the dialogue's parent
 * object is the global instance of the analysis dialogue which has opened
 * it. CashBook can only have one of each dialogue open at a time, so
 * there is no need to make this file-based.
 *
 * \param *parent		The analysis instance owning the template.
 * \param *dialogue		The analysis dialogue instance owning the template.
 * \param template_number	The template to be renamed.
 * \param *ptr			The current Wimp Pointer details.
 */

void analysis_template_save_open_rename_window(struct analysis_block *parent, void *dialogue, int template_number, wimp_pointer *ptr)
{
	/* Set the window contents up. */

	dialogue_set_title(analysis_template_save_dialogue, "RenRepTitle", NULL, NULL, NULL, NULL);
	dialogue_set_icon_text(analysis_template_save_dialogue, DIALOGUE_ICON_OK, "RenRepRen", NULL, NULL, NULL, NULL);
	dialogue_set_ihelp_modifier(analysis_template_save_dialogue, "Ren");

	/* Set the pointers up so we can find this lot again and open the window. */

	analysis_template_save_parent = analysis_get_templates(parent);
	analysis_template_save_template = template_number;
	analysis_template_save_current_mode = ANALYSIS_SAVE_MODE_RENAME;

	/* Open the dialogue. */

	dialogue_open(analysis_template_save_dialogue, FALSE, analysis_template_get_file(analysis_template_save_parent), dialogue, ptr, NULL);
}


/**
 * Fill the Save / Rename Template dialogue with values.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	Unused restore state flag.
 * \param *data		Client data pointer (unused).
 */

static void analysis_template_save_fill_window(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	struct analysis_report	*template = NULL;
	char			*name;

	/* Shade the template menu popup if there are no template names. */

	icons_set_shaded(window, ANALYSIS_SAVE_NAMEPOPUP, analysis_template_get_count(analysis_template_save_parent) == 0);

	/* Find the current template name, and insert it into the field. */

	switch (analysis_template_save_current_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		template = analysis_template_save_report;
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		template = analysis_template_get_report(analysis_template_save_parent, analysis_template_save_template);
		break;

	default:
		break;
	}

	if (template == NULL)
		return;

	name = analysis_template_get_name(template, NULL, 0);
	if (name == NULL)
		return;

	icons_strncpy(window, ANALYSIS_SAVE_NAME, name);
}


/**
 * Process OK clicks in the Save/Rename Template dialogue.  If it is a
 * real save, pass the call on to the store saved report function.  If it
 * is a rename, handle it directly here.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The dialogue parent object.
 * \param *data		Client data pointer (unused).
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool analysis_template_save_process_window(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	template_t	template;
	char		*name;

	if (*icons_get_indirected_text_addr(window, ANALYSIS_SAVE_NAME) == '\0')
		return FALSE;

	name = icons_get_indirected_text_addr(window, ANALYSIS_SAVE_NAME);

	template = analysis_template_get_from_name(analysis_template_save_parent, name);

	switch (analysis_template_save_current_mode) {
	case ANALYSIS_SAVE_MODE_SAVE:
		if (template != NULL_TEMPLATE && error_msgs_report_question("CheckTempOvr", "CheckTempOvrB") == 4)
			return FALSE;

		analysis_template_store(analysis_template_save_parent, analysis_template_save_report, template, name);
		break;

	case ANALYSIS_SAVE_MODE_RENAME:
		if (analysis_template_save_template == NULL_TEMPLATE)
			break;

		if (template != NULL_TEMPLATE && template != analysis_template_save_template) {
			error_msgs_report_error("TempExists");
			return FALSE;
		}

		analysis_template_rename(analysis_template_save_parent, analysis_template_save_template, name);
		break;

	default:
		break;
	}

	return TRUE;
}


/**
 * The Save / Rename Template dialogue has been closed.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer (unused).
 */

static void analysis_template_save_window_close(struct file_block *file, wimp_w window, void *data)
{
	analysis_template_save_current_mode = ANALYSIS_SAVE_MODE_NONE;
	analysis_template_save_parent = NULL;
	analysis_template_save_report = NULL;
	analysis_template_save_template = NULL_TEMPLATE;
}


/**
 * Process menu prepare events in the Save/Rename Template dialogue.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param window	The handle of the owning window.
 * \param icon		The target icon for the menu.
 * \param *menu		Pointer to struct to take the menu details.
 * \param *data		Client data pointer (unused).
 * \return		TRUE if the menu struct was updated; else FALSE.
 */

static osbool analysis_template_save_menu_prepare_handler(struct file_block *file, wimp_w window, wimp_i icon, struct dialogue_menu_data *menu, void *data)
{
	if (menu == NULL)
		return FALSE;

	menu->menu = analysis_template_menu_build(analysis_template_get_file(analysis_template_save_parent), TRUE);
	menu->help_token = "RepListMenu";

	return TRUE;
}


/**
 * Process menu selection events in the Save/Rename Template dialogue.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param w		The handle of the owning window.
 * \param icon		The target icon for the menu.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 * \param *data		Client data pointer (unused). 
 */

static void analysis_template_save_menu_selection_handler(struct file_block *file, wimp_w window, wimp_i icon, wimp_menu *menu, wimp_selection *selection, void *data)
{
	template_t		template_number;
	struct analysis_report	*template;
	char			*name;

	if (selection->items[0] == -1 || analysis_template_save_parent == NULL || menu == NULL || icon != ANALYSIS_SAVE_NAME)
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

	icons_strncpy(window, icon, name);
}


/**
 * Process menu close events in the Save/Rename Template dialogue.
 *
 * \param *file			The file instance associated with the dialogue.
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *data		Client data pointer (unused).
 */

static void analysis_template_save_menu_close_handler(struct file_block *file, wimp_w window, wimp_menu *menu, void *data)
{
	if (menu == NULL)
		return;

	analysis_template_menu_destroy();
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
	if (analysis_template_save_parent != analysis_get_templates(parent) ||
			analysis_template_save_template == NULL_TEMPLATE)
		return;

	if (analysis_template_save_template > template)
		analysis_template_save_template--;
	else if (analysis_template_save_template == template)
		analysis_template_save_template = NULL_TEMPLATE;
}

