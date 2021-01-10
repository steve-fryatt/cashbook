/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: sort_dialogue.h
 *
 * Sorting user interface
 */

#ifndef CASHBOOK_SORT_DIALOGUE
#define CASHBOOK_SORT_DIALOGUE

#include "sort.h"

/**
 * A sort dialogue data handle.
 */

struct sort_dialogue_block;


/**
 * A sort window icon definition, linking an icon handle to a
 * sort type definition.
 */

struct sort_dialogue_icon {
	wimp_i		icon;				/**< The handle of the icon being defined.			*/
	enum sort_type	type;				/**< The sort type which applied to the icon.			*/
};


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
		osbool (*callback)(enum sort_type order, void *data));


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

void sort_dialogue_open(struct sort_dialogue_block *dialogue, wimp_pointer *ptr, enum sort_type order, void *data);


/**
 * Close a visible instance of a Sort dialogue box.
 *
 * \param *dialogue		The handle of the dialogue to be closed.
 * \param *data			If NULL, any dialogue instance will close; otherwise,
 *				the instance will close only if data matches the
 *				data supplied to sort_open_dialogue().
 */

void sort_dialogue_close(struct sort_dialogue_block *dialogue, void *data);

#endif
