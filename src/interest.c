/* Copyright 2016-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: interest.c
 *
 * Interest Rate manager implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"
#include "oslib/territory.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "interest.h"

#include "column.h"
#include "date.h"
#include "edit.h"
#include "window.h"



/**
 * Interest Rate instance data structure
 */

struct interest_block {
	struct file_block	*file;						/**< The file to which the instance belongs.				*/
};

/**
 * Initialise the transaction system.
 */

void interest_initialise(void)
{
}


/**
 * Create a new interest rate module instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct interest_block *interest_create_instance(struct file_block *file)
{
	struct interest_block	*new;

	new = heap_alloc(sizeof(struct interest_block));
	if (new == NULL)
		return NULL;

	/* Initialise the interest module. */

	new->file = file;

	return new;
}


/**
 * Delete an interest rate module instance, and all of its data.
 *
 * \param *instance		The instance to be deleted.
 */

void interest_delete_instance(struct interest_block *instance)
{
	if (instance == NULL)
		return;

	heap_free(instance);
}


/**
 * Return an interest rate for a given account on a given date. Returns
 * NULL_RATE on failure.
 *
 * \param *instance		The interest rate module instance to use.
 * \param account		The account to return an interest rate for.
 * \param date			The date to return the rate for.
 * \return			The interest rate on the date in question.
 */

rate_t interest_get_current_rate(struct interest_block *instance, acct_t account, date_t date)
{
	return NULL_RATE;
}


/**
 * Convert an interest rate into a string, placing the result into a
 * supplied buffer.
 *
 * \param rate			The rate to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *interest_convert_to_string(rate_t rate, char *buffer, size_t length)
{
	if (buffer == NULL || length == 0)
		return NULL;

	*buffer = '\0';

	return buffer;
}
