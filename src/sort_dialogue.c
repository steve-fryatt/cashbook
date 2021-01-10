/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: sort_dialogue.c
 *
 * Sorting user interface implementation.
 */

/* ANSI C header files */

#include <stdlib.h>

/* Acorn C header files */


/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/windows.h"

/* Application header files */

#include "sort_dialogue.h"

#include "caret.h"

/**
 * Sort Window data block
 */

struct sort_dialogue_block {
	/* Client data. */

	void				*data;						/**< Data belonging to the client for the current instance.	*/
	enum sort_type			order;						/**< The original sort order displayed in the dialogue.		*/

	osbool				(*callback)(enum sort_type order, void *data);	/**< Callback to receive new sort order details.		*/

	/* The sort dialogue handle. */

	wimp_w				window;						/**< The window handle of the sort dialogue.			*/

	/* The icons in the window. */

	wimp_i				icon_ok;					/**< The handle of the dialogue's OK icon.			*/
	wimp_i				icon_cancel;					/**< The handle of the dialogue's Cancel icon.			*/
	struct sort_dialogue_icon	*columns;					/**< A list of icons relating to the column choices.		*/
	struct sort_dialogue_icon	*directions;					/**< A list of icons relating to the direction choices.		*/
};


static void sort_dialogue_fill(struct sort_dialogue_block *dialogue);
static osbool sort_dialogue_process(struct sort_dialogue_block *dialogue);
static void sort_dialogue_force_close(struct sort_dialogue_block *dialogue);
static void sort_dialogue_click_handler(wimp_pointer *pointer);
static osbool sort_dialogue_keypress_handler(wimp_key *key);


/**
 * Create a new Sort dialogue definition.
 *
 * \param window		The handle of the window to use for the dialogue.
 * \param columns[]		Pointer to a list of icons relating to sort columns.
 * \param directions[]		Pointer to a list of icons relating to sort directions.
 * \param ok			The icon handle of the OK icon.
 * \param cancel		The icon handle of the Cancel icon.
 * \param *callback		Pointer to a callback function to receive selections.
 * \return			Pointer to the newly created dialogue handle, or NULL on failure.
 */

struct sort_dialogue_block *sort_dialogue_create(wimp_w window, struct sort_dialogue_icon columns[], struct sort_dialogue_icon directions[], wimp_i ok, wimp_i cancel,
		osbool (*callback)(enum sort_type order, void *data))
{
	struct sort_dialogue_block	*new;
	int				i;

	new = malloc(sizeof(struct sort_dialogue_block));
	if (new == NULL)
		return NULL;

	new->data = NULL;
	new->order = SORT_NONE;
	new->callback = callback;
	new->window = window;
	new->columns = columns;
	new->directions = directions;
	new->icon_ok = ok;
	new->icon_cancel = cancel;

	event_add_window_user_data(window, new);
	event_add_window_mouse_event(window, sort_dialogue_click_handler);
	event_add_window_key_event(window, sort_dialogue_keypress_handler);

	for (i = 0; columns[i].type != SORT_NONE; i++)
		event_add_window_icon_radio(window, columns[i].icon, TRUE);

	for (i = 0; directions[i].type != SORT_NONE; i++)
		event_add_window_icon_radio(window, directions[i].icon, TRUE);

	return new;
}


/**
 * Open an instance of a Sort dialogue box.
 *
 * \param *dialogue		The handle of the dialogue to be opened.
 * \param *ptr			Pointer to Wimp Pointer data giving the required
 *				dialogue position.
 * \param order			The sort order to use to open the dialogue.
 * \param *data			Pointer to client-specific data to be passed to the
 *				selection callback function.
 */

void sort_dialogue_open(struct sort_dialogue_block *dialogue, wimp_pointer *ptr, enum sort_type order, void *data)
{
	if (dialogue == NULL || ptr == NULL)
		return;

	if (windows_get_open(dialogue->window))
		wimp_close_window(dialogue->window);

	dialogue->data = data;
	dialogue->order = order;

	sort_dialogue_fill(dialogue);

	windows_open_centred_at_pointer(dialogue->window, ptr);
	place_dialogue_caret(dialogue->window, wimp_ICON_WINDOW);
}


/**
 * Close a visible instance of a Sort dialogue box.
 *
 * \param *dialogue		The handle of the dialogue to be closed.
 * \param *data			If NULL, any dialogue instance will close; otherwise,
 *				the instance will close only if data matches the
 *				data supplied to sort_open_dialogue().
 */

void sort_dialogue_close(struct sort_dialogue_block *dialogue, void *data)
{
	if (dialogue == NULL)
		return;

	if (data == NULL || dialogue->data == data)
		sort_dialogue_force_close(dialogue);
}


/**
 * Update the contents of a Sort dialogue to reflect the the specified
 * sort order settings.
 *
 * \param *dialogue		The handle of the dialogue to be filled.
 */

static void sort_dialogue_fill(struct sort_dialogue_block *dialogue)
{
	int	i;

	if (dialogue == NULL)
		return;

	for (i = 0; dialogue->columns[i].type != SORT_NONE; i++) {
		icons_set_selected(dialogue->window, dialogue->columns[i].icon,
				(dialogue->order & SORT_MASK) == dialogue->columns[i].type);
	}

	for (i = 0; dialogue->directions[i].type != SORT_NONE; i++) {
		icons_set_selected(dialogue->window, dialogue->directions[i].icon,
				(dialogue->order & dialogue->directions[i].type) == dialogue->directions[i].type);
	}
}


/**
 * Take the contents of an updated Sort dialogue and process the data,
 * passing it back to the client.
 *
 * \param *dialogue		The handle of the dialogue to be processed.
 * \return			TRUE if successful; else FALSE.
 */


static osbool sort_dialogue_process(struct sort_dialogue_block *dialogue)
{
	enum sort_type	order = SORT_NONE;
	int		i;

	if (dialogue == NULL || dialogue->callback == NULL)
		return FALSE;

	for (i = 0; dialogue->columns[i].type != SORT_NONE; i++) {
		if (icons_get_selected(dialogue->window, dialogue->columns[i].icon))
			order |= dialogue->columns[i].type;
	}

	if (order != SORT_NONE) {
		for (i = 0; dialogue->directions[i].type != SORT_NONE; i++) {
			if (icons_get_selected(dialogue->window, dialogue->directions[i].icon))
				order |= dialogue->directions[i].type;
		}
	}

	if (!dialogue->callback(order, dialogue->data))
		return FALSE;

	dialogue->order = order;

	return TRUE;
}


/**
 * Close a Sort dialogue box, clearing any client data associated
 * with it (for internal use).
 *
 * \param *dialogue		The handle of the dialogue to close.
 */

static void sort_dialogue_force_close(struct sort_dialogue_block *dialogue)
{
	if (dialogue == NULL)
		return;

	/* If the window isn't open, there's nothing to do. */

	if (!windows_get_open(dialogue->window))
		return;

	/* Close the window. */

	close_dialogue_with_caret(dialogue->window);

	/* Reset the client data. */

	dialogue->data = NULL;
	dialogue->order = SORT_NONE;
}


/**
 * Process mouse clicks in a Sort dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void sort_dialogue_click_handler(wimp_pointer *pointer)
{
	struct sort_dialogue_block	*dialogue;

	dialogue = event_get_window_user_data(pointer->w);
	if (dialogue == NULL)
		return;

	if (pointer->i == dialogue->icon_cancel) {
		if (pointer->buttons == wimp_CLICK_SELECT)
			sort_dialogue_force_close(dialogue);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			sort_dialogue_fill(dialogue);
	} else if (pointer->i == dialogue->icon_ok) {
		if (sort_dialogue_process(dialogue) && pointer->buttons == wimp_CLICK_SELECT)
			sort_dialogue_force_close(dialogue);
	}
}


/**
 * Process keypresses in a Sort dialogue.
 *
 * \param *key			The keypress event block to handle.
 * \return			TRUE if the event was handled; else FALSE.
 */

static osbool sort_dialogue_keypress_handler(wimp_key *key)
{
	struct sort_dialogue_block	*dialogue;

	dialogue = event_get_window_user_data(key->w);
	if (dialogue == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		if (sort_dialogue_process(dialogue))
			sort_dialogue_force_close(dialogue);
		break;

	case wimp_KEY_ESCAPE:
		sort_dialogue_force_close(dialogue);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}
