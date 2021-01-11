/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: sort.c
 *
 * Sorting implementation.
 */

/* ANSI C header files */

#include <stdio.h>
#include <stdlib.h>

/* Acorn C header files */


/* OSLib header files */


/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "sort.h"

struct sort_block {
	enum sort_type		type;					/**< The sort settings for the instance					*/

	enum sort_type		fallback;				/**< The fallback sort type for the instance				*/

	/**
	 * The client callbacks and their associated data
	 */

	struct sort_callback	*callback;				/**< Pointer to the a structure holding the callback function details.	*/

	void			*data;					/**< Client-supplied data for passing to the callback functions.	*/
};


/**
 * Create a new Sort instance.
 *
 * \param initial			The initial sort type data for the instance.
 * \param fallback		The fallback sort type for the instance.
 * \param *callback		Pointer to the client-supplied sort callbacks.
 * \param *data			Pointer to the client data to be returned on all callbacks.
 * \return			Pointer to the newly created instance, or NULL on failure.
 */

struct sort_block *sort_create_instance(enum sort_type initial, enum sort_type fallback, struct sort_callback *callback, void *data)
{
	struct sort_block	*new;

	new = heap_alloc(sizeof(struct sort_block));
	if (new == NULL)
		return NULL;

	new->type = initial;
	new->fallback = fallback;
	new->callback = callback;
	new->data = data;

	return new;
}


/**
 * Delete a Sort instance.
 *
 * \param *nstance		The instance to be deleted.
 */

void sort_delete_instance(struct sort_block *instance)
{
	if (instance == NULL)
		return;

	heap_free(instance);
}


/**
 * Set the sort details -- field and direction -- of the instance.
 *
 * \param *instance		The instance to set.
 * \param order			The new sort details to be set.
 */

void sort_set_order(struct sort_block *instance, enum sort_type order)
{
	if (instance == NULL)
		return;

	instance->type = order;
}


/**
 * Get the current sort details -- field and direction -- from a
 * sort instance.
 *
 * \param *instance		The instance to interrogate.
 * \return			The sort order applied to the instance.
 */

enum sort_type sort_get_order(struct sort_block *instance)
{
	if (instance == NULL)
		return SORT_NONE;

	return instance->type;
}


/**
 * Copy the sort details -- field and direction -- from one sort instance
 * to another.
 *
 * \param *nstance		The instance to be updated with new details.
 * \param *source		The instance to copy the details from.
 */

void sort_copy_order(struct sort_block *instance, struct sort_block *source)
{
	if (instance == NULL || source == NULL)
		return;

	instance->type = source->type;
}


/**
 * Read the sort details encoded in a line of ASCII text, and use them to
 * update a sort instance.
 *
 * \param *instance		The instance to be updated with the details.
 * \param *value		Pointer to the text containing the details.
 */

void sort_read_from_text(struct sort_block *instance, char *value)
{
	if (instance == NULL || value == NULL)
		return;

	instance->type = strtoul(value, NULL, 16);
}


/**
 * Write the sort details from an instance into an ASCII string.
 *
 * \param *instance		The instance to take the details from.
 * \param *buffer		Pointer to the buffer to take the string.
 * \param length		The length of the buffer supplied.
 */

void sort_write_as_text(struct sort_block *instance, char *buffer, size_t length)
{
	if (instance == NULL || buffer == NULL || length <= 0)
		return;

	string_printf(buffer, length, "%x", instance->type);
}


/**
 * Perform a sort operation using the settings contained in a sort instance.
 *
 * \param *instance		The instance to use for the sorting.
 * \param items			The number of items which are to be sorted.
 */

void sort_process(struct sort_block *instance, size_t items)
{
	int		gap, comb, result;
	osbool		sorted, ascending, descending;
	enum sort_type	order, fallback;

	if (instance == NULL || instance->callback == NULL || instance->callback->compare == NULL || instance->callback->swap == NULL)
		return;

	/* Sort the entries using a combsort.  This has the advantage over qsort() that the order of entries is only
	 * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
	 */

	gap = items - 1;

	order = instance->type & SORT_MASK;
	fallback = instance->fallback & SORT_MASK;

	if (order == SORT_NONE)
		return;

	do {
		gap = (gap > 1) ? (gap * 10 / 13) : 1;
		if ((items >= 12) && (gap == 9 || gap == 10))
			gap = 11;

		sorted = TRUE;
		for (comb = 0; (comb + gap) < items; comb++) {
			result = instance->callback->compare(order, comb, comb + gap, instance->data);

			if (result != 0) {
				ascending = (instance->type & SORT_ASCENDING) ? TRUE : FALSE;
				descending = (instance->type & SORT_DESCENDING) ? TRUE : FALSE;
			} else if (fallback != SORT_NONE) {
				result = instance->callback->compare(fallback, comb, comb + gap, instance->data);

				ascending = (instance->fallback & SORT_ASCENDING) ? TRUE : FALSE;
				descending = (instance->fallback & SORT_DESCENDING) ? TRUE : FALSE;
			}

			if ((descending && (result < 0)) || (ascending && (result > 0))) {
				instance->callback->swap(comb + gap, comb, instance->data);
				sorted = FALSE;
			}
		}
	} while (!sorted || gap != 1);
}
