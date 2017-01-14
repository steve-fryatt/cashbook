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
 * \file: sort.c
 *
 * Sorting implementation.
 */

/* ANSI C header files */

#include <stdlib.h>

/* Acorn C header files */


/* OSLib header files */

//#include "oslib/wimp.h"

/* SF-Lib header files. */

//#include "sflib/event.h"
#include "sflib/heap.h"
//#include "sflib/icons.h"
//#include "sflib/windows.h"

/* Application header files */

#include "sort.h"

struct sort_block {
	enum sort_type		type;					/**< The sort settings for the instance			*/
};


/**
 * Create a new Sort instance.
 *
 * \param type			The initial sort type data for the instance.
 * \return			Pointer to the newly created instance, or NULL on failure.
 */

struct sort_block *sort_create_instance(enum sort_type type)
{
	struct sort_block	*new;

	new = heap_alloc(sizeof(struct sort_block));
	if (new == NULL)
		return NULL;

	new->type = type;

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
