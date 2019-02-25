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
 * \file: dialogue.c
 *
 * Dialogue box implementation.
 */

/* ANSI C header files */

#include <string.h>

/* SF-Lib header files. */

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
	struct file_block			*file;			/**< The current parent file for the dialogue.			*/
	void					*parent;		/**< The parent object pointer for the dialogue, or NULL.	*/
	wimp_w					window;			/**< The Wimp window handle of the dialogue.			*/
	void					*client_data;		/**< Context data supplied by the client.			*/
	osbool					restore;		/**< TRUE if the current dialogue should restore.		*/

	struct dialogue_block			*next;			/**< Pointer to the next box in the list, or NULL.		*/
};

/**
 * The linked list of dialogue boxes.
 */

static struct dialogue_block *dialogue_list = NULL;

/**
 * The icon which is currently the target of a pop-up menu.
 */

static struct dialogue_icon *dialogue_menu_target = NULL;

/* Static Function Prototypes. */

static void dialogue_click_handler(wimp_pointer *pointer);
static osbool dialogue_keypress_handler(wimp_key *key);
static enum account_type dialogue_convert_account_type(enum dialogue_icon_type icon);
static enum account_menu_type dialogue_convert_account_menu_type(enum dialogue_icon_type icon);
static void dialogue_menu_prepare_handler(wimp_w window, wimp_menu *menu, wimp_pointer *pointer);
static void dialogue_menu_selection_handler(wimp_w window, wimp_menu *menu, wimp_selection *selection);
static void dialogue_menu_close_handler(wimp_w window, wimp_menu *menu);
static void dialogue_close_window(struct dialogue_block *dialogue);
static osbool dialogue_process(struct dialogue_block *dialogue, wimp_pointer *pointer, struct dialogue_icon *icon);
static void dialogue_fill(struct dialogue_block *dialogue);
static void dialogue_place_caret(struct dialogue_block *dialogue);
static void dialogue_shade_icons(struct dialogue_block *dialogue, wimp_i target);
static void dialogue_hide_icons(struct dialogue_block *dialogue, enum dialogue_icon_type type, osbool hide);
static void dialogue_register_icon_handlers(struct dialogue_block *dialogue);
static struct dialogue_icon *dialogue_find_icon(struct dialogue_block *dialogue, wimp_i icon);


/**
 * Initialise the dialogue handler.
 */

void dialogue_initialise(void)
{
	dialogue_lookup_initialise();
}


/**
 * Create a new dialogue window instance.
 *
 * \param *definition		The dialogue definition from the client.
 * \return			Pointer to the dialogue structure, or NULL on failure.
 */

struct dialogue_block *dialogue_create(struct dialogue_definition *definition)
{
	struct dialogue_block	*new;

	if (definition == NULL)
		return NULL;

	/* Create the instance. */

	new = heap_alloc(sizeof(struct dialogue_block));
	if (new == NULL)
		return NULL;

	new->definition = definition;
	new->file = NULL;
	new->parent = NULL;
	new->client_data = NULL;
	new->restore = FALSE;

	/* Create the dialogue window. */

	new->window = templates_create_window(definition->template_name);
	if (new->window == NULL) {
		heap_free(new);
		return NULL;
	}

	ihelp_add_window(new->window, definition->ihelp_token, NULL);
	event_add_window_user_data(new->window, new);
	event_add_window_mouse_event(new->window, dialogue_click_handler);
	event_add_window_key_event(new->window, dialogue_keypress_handler);
	event_add_window_menu_prepare(new->window, dialogue_menu_prepare_handler);
	event_add_window_menu_selection(new->window, dialogue_menu_selection_handler);
	event_add_window_menu_close(new->window, dialogue_menu_close_handler);

	dialogue_register_icon_handlers(new);

	/* Link the dialogue into the list. */

	new->next = dialogue_list;
	dialogue_list = new;

	return new;
}


/**
 * Close any open dialogues which relate to a given file or parent object.
 *
 * \param *file			If not NULL, the file to close dialogues
 *				for.
 * \param *parent		If not NULL, the parent object to close
 *				dialogues for.
 */

void dialogue_force_all_closed(struct file_block *file, void *parent)
{
	struct dialogue_block *dialogue = dialogue_list;

	debug_printf("\\GRunning dialogue close check for file=0x%x and parent=0x%x.", file, parent);

	while (dialogue != NULL) {
		debug_printf("Checking to close %s dialogue.", dialogue->definition->template_name);
		dialogue_close(dialogue, file, parent);
		dialogue = dialogue->next;
	}

	debug_printf("\\LFinished dialogue close check for file=0x%x and parent=0x%x.", file, parent);
}


/**
 * Open a new dialogue. Dialogues are attached to a file, and also to a
 * "parent object", which can be anything that the client for
 * dialogue_open() wishes to associate them with. If not NULL, they are
 * commonly pointers to instance blocks, such that a dialogue can be
 * associated with -- and closed on the demise of -- things such as
 * account or report views.
 * 
 * \param *dialogue		The dialogue instance to open.
 * \param hide			TRUE to hide the 'hidden' icons; FALSE
 *				to show them.
 * \param restore		TRUE to restore previous values; FALSE to
 *				use application defaults.
 * \param *file			The file to be the parent of the dialogue.
 * \param *parent		The parent object of the dialogue, or NULL.
 * \param *ptr			The current Wimp Pointer details.
 * \param *data			Data to pass to client callbacks.
 */

void dialogue_open(struct dialogue_block *dialogue, osbool hide, osbool restore, struct file_block *file, void *parent, wimp_pointer *pointer, void *data)
{
	if (dialogue == NULL || dialogue->definition == NULL || file == NULL || pointer == NULL)
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

	dialogue->file = file;
	dialogue->parent = parent;
	dialogue->client_data = data;
	dialogue->restore = restore;

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
 * \param *file			If not NULL, only close the dialogue if
 *				this is the parent file.
 * \param *parent		If not NULL, only close the dialogue if
 *				this is the parent object.
 */

void dialogue_close(struct dialogue_block *dialogue, struct file_block *file, void *parent)
{
	if (dialogue == NULL || (file!= NULL && dialogue->file != file) || (parent != NULL && dialogue->parent != parent))
		return;

	dialogue_close_window(dialogue);
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
 * Set the text in an icon or icons in a dialogue box, redrawing them if the
 * dialogue is currently open.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param type			The types of icon to update.
 * \param *token		The MessageTrans token for the new text.
 * \param *a			MessageTrans parameter A, or NULL.
 * \param *b			MessageTrans parameter B, or NULL.
 * \param *c			MessageTrans parameter C, or NULL.
 * \param *d			MessageTrans parameter D, or NULL.
 */

void dialogue_set_icon_text(struct dialogue_block *dialogue, enum dialogue_icon_type type, char *token, char *a, char *b, char *c, char *d)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon == DIALOGUE_NO_ICON) || !(icons[i].type & type))
			continue;

		icons_msgs_param_lookup(dialogue->window, icons[i].icon, token, a, b, c, d);

		if (windows_get_open(dialogue->window))
			xwimp_set_icon_state(dialogue->window, icons[i].icon, 0, 0);
	}
}


/**
 * Change the interactive help token modifier for a dialogue window.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param *modifier		The new modifier to apply.
 */

void dialogue_set_ihelp_modifier(struct dialogue_block *dialogue, char *modifier)
{
	if (dialogue == NULL || dialogue->window == NULL)
		return;

	ihelp_set_modifier(dialogue->window, modifier);
}


/**
 * Process mouse clicks in a dialogue instance's window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void dialogue_click_handler(wimp_pointer *pointer)
{
	struct dialogue_block	*windat;
	struct dialogue_icon	*icon, *next;
	enum account_type	account_type = ACCOUNT_NULL;
	enum account_menu_type	menu_type = ACCOUNT_MENU_NONE;

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
			dialogue_refresh(windat, FALSE);
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
	} else if (icon->type & DIALOGUE_ICON_ACCOUNT_POPUP) {
		if ((pointer->buttons == wimp_CLICK_SELECT) && (icon->target != DIALOGUE_NO_ICON)) {
			account_type = dialogue_convert_account_type(icon->type);

			if (account_type != ACCOUNT_NULL)
				dialogue_lookup_open_window(windat->file, windat->window,
						icon->target, NULL_ACCOUNT, account_type);
		}
	} else if (icon->type & DIALOGUE_ICON_ACCOUNT_RECONCILE) {
		if (pointer->buttons == wimp_CLICK_ADJUST)
			account_toggle_reconcile_icon(windat->window, icon->icon);
	} else if (icon->type & DIALOGUE_ICON_ACCOUNT_NAME) {
		menu_type = dialogue_convert_account_menu_type(icon->type);
		next = dialogue_find_icon(windat, icon->target);
		if ((menu_type != ACCOUNT_MENU_NONE) && (next != NULL) && (pointer->buttons == wimp_CLICK_ADJUST)) {
			account_menu_open_icon(windat->file, menu_type, NULL, windat->window,
					next->target, icon->icon, icon->target, pointer);
		}
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
	struct dialogue_icon	*icon, *next;
	enum account_type	account_type = ACCOUNT_NULL;

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
		if ((icon->type & DIALOGUE_ICON_ACCOUNT_POPUP) && (icon->target == DIALOGUE_NO_ICON)) {
			account_type = dialogue_convert_account_type(icon->type);

			if (account_type != ACCOUNT_NULL)
				dialogue_lookup_open_window(windat->file, windat->window,
						icon->icon, NULL_ACCOUNT, account_type);
		}
		break;

	default:
		if ((icon->type & DIALOGUE_ICON_ACCOUNT_IDENT) == 0)
			return FALSE;

		account_type = dialogue_convert_account_type(icon->type);
		next = dialogue_find_icon(windat, icon->target);

		if ((account_type != ACCOUNT_NULL) && (next != NULL)) {
			account_lookup_field(windat->file, key->c, account_type, NULL_ACCOUNT, NULL,
				windat->window, icon->icon, icon->target, next->target);
		}
		break;
	}

	return TRUE;
}


/**
 * Convert a dialogue icon type bitfield into an account type.
 *
 * \param icon		The icon bitfield to convert.
 * \return		The corresponding account type, or ACCOUNT_NULL.
 */

static enum account_type dialogue_convert_account_type(enum dialogue_icon_type icon)
{
	if (icon & DIALOGUE_ICON_TYPE_FROM)
		return ACCOUNT_IN | ACCOUNT_FULL;
	else if (icon & DIALOGUE_ICON_TYPE_TO)
		return ACCOUNT_OUT | ACCOUNT_FULL;
	else if (icon & DIALOGUE_ICON_TYPE_IN)
		return ACCOUNT_IN;
	else if (icon & DIALOGUE_ICON_TYPE_OUT)
		return ACCOUNT_OUT;
	else if (icon & DIALOGUE_ICON_TYPE_FULL)
		return ACCOUNT_FULL;
	else
		return ACCOUNT_NULL;
}


/**
 * Convert a dialogue icon type bitfield into an account menu type.
 *
 * \param icon		The icon bitfield to convert.
 * \return		The corresponding account menu type, or
 *			ACCOUNT_MENU_NONE.
 */

static enum account_menu_type dialogue_convert_account_menu_type(enum dialogue_icon_type icon)
{
	if (icon & DIALOGUE_ICON_TYPE_FROM)
		return ACCOUNT_MENU_FROM;
	else if (icon & DIALOGUE_ICON_TYPE_TO)
		return ACCOUNT_MENU_TO;
	else if (icon & DIALOGUE_ICON_TYPE_IN)
		return ACCOUNT_MENU_INCOMING;
	else if (icon & DIALOGUE_ICON_TYPE_OUT)
		return ACCOUNT_MENU_OUTGOING;
	else if (icon & DIALOGUE_ICON_TYPE_FULL)
		return ACCOUNT_MENU_ACCOUNTS;
	else
		return ACCOUNT_MENU_NONE;
}


/**
 * Process menu prepare events in a dialogue instance's window.
 *
 * \param window	The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void dialogue_menu_prepare_handler(wimp_w window, wimp_menu *menu, wimp_pointer *pointer)
{
	struct dialogue_block		*dialogue;
	struct dialogue_menu_data	menu_data;

	dialogue = event_get_window_user_data(window);
	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->callback_menu_prepare == NULL)
		return;

	if (pointer != NULL)
		dialogue_menu_target = dialogue_find_icon(dialogue, pointer->i);

	if (dialogue_menu_target == NULL)
		return;

	if (!dialogue->definition->callback_menu_prepare(dialogue->file, window, dialogue_menu_target->target, &menu_data, dialogue->client_data))
		return;

	event_set_menu_block(menu_data.menu);
	ihelp_add_menu(menu_data.menu, menu_data.help_token);
}


/**
 * Process menu selection events in a dialogue instance's window.
 *
 * \param window	The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void dialogue_menu_selection_handler(wimp_w window, wimp_menu *menu, wimp_selection *selection)
{
	struct dialogue_block	*dialogue;

	if (dialogue_menu_target == NULL)
		return;

	dialogue = event_get_window_user_data(window);
	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->callback_menu_select == NULL)
		return;

	dialogue->definition->callback_menu_select(dialogue->file, window, dialogue_menu_target->target, menu, selection, dialogue->client_data);

	wimp_set_icon_state(window, dialogue_menu_target->target, 0, 0);
	icons_replace_caret_in_window(window);
}


/**
 * Process menu close events in a dialogue instance's window.
 *
 * \param window	The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void dialogue_menu_close_handler(wimp_w window, wimp_menu *menu)
{
	struct dialogue_block	*dialogue;

	if (dialogue_menu_target == NULL)
		return;

	dialogue = event_get_window_user_data(window);
	if (dialogue == NULL || dialogue->definition == NULL || dialogue->definition->callback_menu_close == NULL)
		return;

	dialogue->definition->callback_menu_close(dialogue->file, window, menu, dialogue->client_data);

	ihelp_remove_menu(menu);
	dialogue_menu_target = NULL;
}


/**
 * Close a dialogue, warning the client that it has gone.
 *
 * \param *dialogue		The dialogue instance to close.
 */

static void dialogue_close_window(struct dialogue_block *dialogue)
{
	if (dialogue == NULL || dialogue->window == NULL || windows_get_open(dialogue->window) == FALSE)
		return;

	if (dialogue->definition != NULL && dialogue->definition->callback_close != NULL)
		dialogue->definition->callback_close(dialogue->file, dialogue->window, dialogue->client_data);

	close_dialogue_with_caret(dialogue->window);

	dialogue->file = NULL;
	dialogue->parent = NULL;
	dialogue->client_data = NULL;

	debug_printf("\\ODialogue Closed!");
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
	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->callback_process == NULL || icon == NULL)
		return FALSE;

	return dialogue->definition->callback_process(dialogue->file, dialogue->window, pointer, icon->type, dialogue->parent, dialogue->client_data);
}


/**
 * Request the client to fill a dialogue, update the shaded icons and then
 * redraw any fields which require it. If the dialogue isn't open, nothing
 * will be done.
 *
 * \param *dialogue		The dialogue instance to refresh.
 * \param redraw_title		TRUE to force a redraw of the title bar.
 */

void dialogue_refresh(struct dialogue_block *dialogue, osbool redraw_title)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	if (!windows_get_open(dialogue->window))
		return;

	dialogue_fill(dialogue);

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_REFRESH))
			wimp_set_icon_state(dialogue->window, icons[i].icon, 0, 0);
	}

	icons_replace_caret_in_window(dialogue->window);

	if (redraw_title)
		xwimp_force_redraw_title(dialogue->window);
}


/**
 * Request the client to fill a dialogue, and update the shaded icons
 * based on the end result.
 *
 * \param *dialogue		The dialogue instance to fill.
 */

static void dialogue_fill(struct dialogue_block *dialogue)
{
	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->callback_fill == NULL)
		return;

	dialogue->definition->callback_fill(dialogue->file, dialogue->window, dialogue->restore, dialogue->client_data);

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
	wimp_icon_state		state;
	wimp_icon_flags		button_type;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;
	state.w = dialogue->window;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if (icons[i].icon == DIALOGUE_NO_ICON)
			continue;

		state.i = icons[i].icon;
		wimp_get_icon_state(&state);

		if ((state.icon.flags & wimp_ICON_SHADED) || !(state.icon.flags & wimp_ICON_INDIRECTED))
			continue;

		button_type = (state.icon.flags & wimp_ICON_BUTTON_TYPE) >> wimp_ICON_BUTTON_TYPE_SHIFT;

		if (button_type == wimp_BUTTON_WRITE_CLICK_DRAG || button_type == wimp_BUTTON_WRITABLE) {
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

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
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

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & type))
			icons_set_deleted(dialogue->window, icons[i].icon, hide);
	}
}


/**
 * Register any icons declared as requiring event handlers.
 *
 * \param *dialogue		The dialogue instance to register.
 */

static void dialogue_register_icon_handlers(struct dialogue_block *dialogue)
{
	int			i;
	struct dialogue_icon	*icons;

	if (dialogue == NULL || dialogue->window == NULL || dialogue->definition == NULL || dialogue->definition->icons == NULL)
		return;

	icons = dialogue->definition->icons;

	for (i = 0; (icons[i].type & DIALOGUE_ICON_END) == 0; i++) {
		if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_RADIO))
			event_add_window_icon_radio(dialogue->window, icons[i].icon, TRUE);
		else if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_RADIO_PASS))
			event_add_window_icon_radio(dialogue->window, icons[i].icon, FALSE);
		else if ((icons[i].icon != DIALOGUE_NO_ICON) && (icons[i].type & DIALOGUE_ICON_POPUP))
			event_add_window_icon_popup(dialogue->window, icons[i].icon, NULL, -1, NULL);
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

