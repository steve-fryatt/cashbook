/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: dialogue.c
 *
 * Dialogue box implementation.
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
#include "dialogue.h"

#include "account.h"
#include "account_menu.h"
#include "caret.h"
#include "dialogue_lookup.h"


/**
 * A dialogue definition.
 */

struct dialogue_block {
	struct dialogue_definition		*definition;		/**< The dialogue definition from the client.			*/
	struct file_block			*parent;		/**< The current parent file for the dialogue.			*/
	wimp_w					window;			/**< The Wimp window handle of the dialogue.			*/
	void					*client_data;		/**< Context data supplied by the client.			*/
};

/* Static Function Prototypes. */

static void dialogue_click_handler(wimp_pointer *pointer);
static osbool dialogue_keypress_handler(wimp_key *key);
static void dialogue_close_window(struct dialogue_block *dialogue);
static osbool dialogue_process(struct dialogue_block *dialogue, wimp_pointer *pointer, struct dialogue_icon *icon);
static void dialogue_refresh(struct dialogue_block *dialogue);
static void dialogue_fill(struct dialogue_block *dialogue);
static void dialogue_place_caret(struct dialogue_block *dialogue);
static void dialogue_shade_icons(struct dialogue_block *dialogue, wimp_i target);
static void dialogue_hide_icons(struct dialogue_block *dialogue, enum dialogue_icon_type type, osbool hide);
static void dialogue_register_radio_icons(struct dialogue_block *dialogue);
static struct dialogue_icon *dialogue_find_icon(struct dialogue_block *dialogue, wimp_i icon);

/**
 * Initialise a new dialogue window instance.
 *
 * \param *definition		The dialogue definition from the client.
 * \return			Pointer to the dialogue structure, or NULL on failure.
 */

struct dialogue_block *dialogue_initialise(struct dialogue_definition *definition)
{
	struct dialogue_block	*new;

	if (definition == NULL)
		return NULL;

	/* Create the instance. */

	new = heap_alloc(sizeof(struct dialogue_block));
	if (new == NULL)
		return NULL;

	new->definition = definition;
	new->parent = NULL;
	new->client_data = NULL;

	/* Create the dialogue window. */

	new->window = templates_create_window(definition->template_name);
	ihelp_add_window(new->window, definition->ihelp_token, NULL);
	event_add_window_user_data(new->window, new);
	event_add_window_mouse_event(new->window, dialogue_click_handler);
	event_add_window_key_event(new->window, dialogue_keypress_handler);
	dialogue_register_radio_icons(new);

	return new;
}


/**
 * Open a new dialogue.
 * 
 * \param *dialogue		The dialogue instance to open.
 * \param hide			TRUE to hide the 'hidden' icons; FALSE
 *				to show them.
 * \param *parent		The file to be the parent of the dialogue.
 * \param *ptr			The current Wimp Pointer details.
 * \param *data			Data to pass to client callbacks.
 */

void dialogue_open(struct dialogue_block *dialogue, osbool hide, struct file_block *parent, wimp_pointer *pointer, void *data)
{
	if (dialogue == NULL || dialogue->definition == NULL || parent == NULL || pointer == NULL)
		return;

	/* If the window is already open, another instance is being edited.
	 * Assume that the user wants to lose any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret() as the caret is just moving
	 * from one dialogue to another.
	 */

	if (windows_get_open(dialogue->window))
		wimp_close_window(dialogue->window);

	/* Set the pointers up so we can find this lot again and open the window. */

	dialogue->parent = parent;
	dialogue->client_data = data;

	/* Set the window contents up. */

	if (dialogue->definition->hidden_icons != DIALOGUE_ICON_NONE)
		dialogue_hide_icons(dialogue, dialogue->definition->hidden_icons, hide);

	dialogue_fill(dialogue);

	windows_open_centred_at_pointer(dialogue->window, pointer);
	dialogue_place_caret(dialogue);
}


/**
 * Force a dialogue instance to close if it is currently open
 * on screen.
 *
 * \param *dialogue		The dialogue instance to close.
 * \param *parent		If not NULL, only close the dialogue if
 *				this is the parent file.
 */

void dialogue_close(struct dialogue_block *dialogue, struct file_block *parent)
{
	if (dialogue == NULL || dialogue->parent != parent)
		return;

	if (windows_get_open(dialogue->window))
		close_dialogue_with_caret(dialogue->window);
}


/**
 * Set the window title for a dialogue box, redrawing it if the
 * dialogue is currently open.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param *token		The MessageTrans token for the new title.
 * \param *a			MessageTrans parameter A, or NULL.
 * \param *b			MessageTrans parameter B, or NULL.
 * \param *c			MessageTrans parameter C, or NULL.
 * \param *d			MessageTrans parameter D, or NULL.
 */

void dialogue_set_title(struct dialogue_block *dialogue, char *token, char *a, char *b, char *c, char *d)
{
	if (dialogue == NULL || dialogue->window == NULL)
		return;

	msgs_param_lookup(token, windows_get_indirected_title_addr(dialogue->window),
			windows_get_indirected_title_length(dialogue->window), a, b, c, d);

	if (windows_get_open(dialogue->window))
		xwimp_force_redraw_title(dialogue->window);
}


/**
 * Process mouse clicks in a dialogue instance's window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void dialogue_click_handler(wimp_pointer *pointer)
{
	struct dialogue_block	*windat;
	struct dialogue_icon	*icon;

	windat = event_get_window_user_data(pointer->w);
	if (pointer == NULL || windat == NULL || windat->definition == NULL)
		return;

	icon = dialogue_find_icon(windat, pointer->i);
	if (icon == NULL)
		return;

	if (icon->type & DIALOGUE_ICON_CANCEL) {
		if (pointer->buttons == wimp_CLICK_SELECT) {
			dialogue_close_window(windat);
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			dialogue_refresh(windat);
		}
	} else if (icon->type & DIALOGUE_ICON_OK) {
		if (dialogue_process(windat, pointer, icon) && pointer->buttons == wimp_CLICK_SELECT)
			dialogue_close_window(windat);
	} else if (icon->type & DIALOGUE_ICON_ACTION) {
		if (dialogue_process(windat, pointer, icon))
			dialogue_close_window(windat);
	} else if (icon->type & DIALOGUE_ICON_SHADE_TARGET) {
		dialogue_shade_icons(windat, pointer->i);
		icons_replace_caret_in_window(windat->window);
	} else if (icon->type & DIALOGUE_ICON_POPUP_FROM) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != DIALOGUE_NO_ICON))
			dialogue_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
	} else if (icon->type & DIALOGUE_ICON_POPUP_TO) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != DIALOGUE_NO_ICON))
			dialogue_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
	} else if (icon->type & DIALOGUE_ICON_POPUP_IN) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != DIALOGUE_NO_ICON))
			dialogue_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_IN);
	} else if (icon->type & DIALOGUE_ICON_POPUP_OUT) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != DIALOGUE_NO_ICON))
			dialogue_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_OUT);
	} else if (icon->type & DIALOGUE_ICON_POPUP_FULL) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != DIALOGUE_NO_ICON))
			dialogue_lookup_open_window(windat->parent, windat->window,
					icon->target, NULL_ACCOUNT, ACCOUNT_FULL);
	}
}


/**
 * Process keypresses in a dialogue instance's window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool dialogue_keypress_handler(wimp_key *key)
{
	struct dialogue_block	*windat;
	struct dialogue_icon	*icon;

	windat = event_get_window_user_data(key->w);
	if (key == NULL || windat == NULL || windat->definition == NULL)
		return FALSE;

	icon = dialogue_find_icon(windat, key->i);
	if (icon == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		if (dialogue_process(windat, NULL, icon))
			dialogue_close_window(windat);
		break;

	case wimp_KEY_ESCAPE:
		dialogue_close_window(windat);
		break;

	case wimp_KEY_F1:
		if (icon->type & DIALOGUE_ICON_POPUP_FROM) {
			if (icon->target == DIALOGUE_NO_ICON)
				dialogue_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_IN | ACCOUNT_FULL);
		} else if (icon->type & DIALOGUE_ICON_POPUP_TO) {
			if (icon->target == DIALOGUE_NO_ICON)
				dialogue_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_OUT | ACCOUNT_FULL);
		} else if (icon->type & DIALOGUE_ICON_POPUP_IN) {
			if (icon->target == DIALOGUE_NO_ICON)
				dialogue_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_IN);
		} else if (icon->type & DIALOGUE_ICON_POPUP_OUT) {
			if (icon->target == DIALOGUE_NO_ICON)
				dialogue_lookup_open_window(windat->parent, windat->window,
						key->i, NULL_ACCOUNT, ACCOUNT_OUT);
		} else if (icon->type & DIALOGUE_ICON_POPUP_FULL) {
			if (icon->target == DIALOGUE_NO_ICON)
				dialogue_lookup_open_window(windat->parent, windat->window,
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
 * Close a dialogue, warning the client that it has gone.
 *
 * \param *dialogue		The dialogue instance to close.
 */

static void dialogue_close_window(struct dialogue_block *dialogue)
{
	if (dialogue == NULL)
		return;

	if (dialogue->definition->callback_close != NULL)
		dialogue->definition->callback_close(dialogue->window, dialogue->client_data);

	close_dialogue_with_caret(dialogue->window);

}


/**
 * Process the contents of a dialogue and return it to the client.
 *
 * \param *dialogue		The dialogue instance to process.
 * \param *pointer		Pointer to the pointer data, or NULL on keypress.
 * \param *icon			The dialogue icon details.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool dialogue_process(struct dialogue_block *dialogue, wimp_pointer *pointer, struct dialogue_icon *icon)
{
	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition->callback_process == NULL || icon == NULL)
		return FALSE;

	return dialogue->definition->callback_process(dialogue->window, pointer, icon->type, dialogue->client_data);
}


/**
 * Request the client to fill a dialogue, update the shaded icons and then
 * redraw any fields which require it.
 *
 * \param *dialogue		The dialogue instance to refresh.
 */

static void dialogue_refresh(struct dialogue_block *dialogue)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	dialogue_fill(dialogue);

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_REFRESH))
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

static void dialogue_fill(struct dialogue_block *dialogue)
{
	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition->callback_fill == NULL)
		return;

	dialogue->definition->callback_fill(dialogue->window, dialogue->client_data);

	/* Update any shaded icons after the update. */

	dialogue_shade_icons(dialogue, DIALOGUE_NO_ICON);
}


/**
 * Place the caret into the first available writable icon in a dialogue.
 *
 * \param *dialogue		The dialogue instance to place the caret
 *				in.
 */

static void dialogue_place_caret(struct dialogue_block *dialogue)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_REFRESH) && !icons_get_shaded(dialogue->window, icons[i].icon)) {
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

static void dialogue_shade_icons(struct dialogue_block *dialogue, wimp_i target)
{
	int			i;
	struct dialogue_icon	*icons;
	osbool			include = FALSE;
	osbool			shaded = FALSE;
	wimp_i			icon = DIALOGUE_NO_ICON;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if (icons[i].target == DIALOGUE_NO_ICON)
			continue;

		/* Reset the shaded state if this isn't an OR clause. */

		if (!(icons[i].type & DIALOGUE_ICON_SHADE_OR)) {
			icon = icons[i].icon;
			shaded = FALSE;
			include = FALSE;
		}

		if (target == DIALOGUE_NO_ICON || target == icons[i].target)
			include = TRUE;

		/* Update the state based on the icon. */

		if (icons[i].type & DIALOGUE_ICON_SHADE_ON) {
			shaded = shaded || icons_get_selected(dialogue->window, icons[i].target);
		} else if (icons[i].type & DIALOGUE_ICON_SHADE_OFF) {
			shaded = shaded || !icons_get_selected(dialogue->window, icons[i].target);
		} else {
			icon = DIALOGUE_NO_ICON;
			shaded = FALSE;
		}

		/* If the next icon isn't an AND clause, this is the end: update the icon. */

		if (!(icons[i + 1].type & DIALOGUE_ICON_SHADE_OR) && (icon != DIALOGUE_NO_ICON) && include)
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

static void dialogue_hide_icons(struct dialogue_block *dialogue, enum dialogue_icon_type type, osbool hide)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & type))
			icons_set_deleted(dialogue->window, icons[i].icon, hide);
	}
}


/**
 * Register any icons declared as radio icons with the event handler.
 *
 * \param *dialogue		The dialogue instance to register.
 */

static void dialogue_register_radio_icons(struct dialogue_block *dialogue)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_RADIO))
			event_add_window_icon_radio(dialogue->window, icons[i].icon, TRUE);
		else if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_RADIO_PASS))
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

static struct dialogue_icon *dialogue_find_icon(struct dialogue_block *dialogue, wimp_i icon)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return NULL;

	if (icon == wimp_ICON_WINDOW)
		return NULL;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if (icons[i].icon == icon)
			return &(icons[i]);
	}

	return NULL;
}

