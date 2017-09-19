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
 * \file: account_idnum.c
 *
 * Cheque or PayIn ID Number implementation.
 */

/* ANSI C header files */

#include <stddef.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "account_idnum.h"


/**
 * The size of buffer allocated to writing printf() format strings.
 */

#define ACCOUNT_IDNUM_FORMAT_LENGTH 32


/**
 * Initialise a new Account ID Number instance.
 *
 * \param *block		The ID Number instance to initialise.
 */

void account_idnum_initialise(struct account_idnum *block)
{
	if (block == NULL)
		return;

	block->next_id = 0;
	block->width = 0;
}


/**
 * Set an Account ID Number instance using details copied from another
 * instance.
 *
 * \param *block		The ID Number instance to set.
 * \param *from			The ID Number instance to copy from.
 */

void account_idnum_copy(struct account_idnum *block, struct account_idnum *from)
{
	if (block == NULL)
		return;

	if (from == NULL) {
		account_idnum_initialise(block);
		return;
	}

	block->next_id = from->next_id;
	block->width = from->width;
}


/**
 * Set an Account ID Number instance using a textual number.
 *
 * \param *block		The ID Number instance to set.
 * \param *value		Pointer to a string representing the new value.
 */

void account_idnum_set_from_string(struct account_idnum *block, char *value)
{
	size_t len;

	if (block == NULL || value == NULL)
		return;

	len = strlen(value);

	if (len > 0) {
		block->width = len;
		block->next_id = atoi(value);
	} else {
		account_idnum_initialise(block);
	}
}


/**
 * Set the raw values in an Account ID Number instance directly.
 *
 * \param *block		The ID Number instance to set.
 * \param width			The width of the ID field, in digits.
 * \param next_id		The next ID value of the field.
 */

void account_idnum_set(struct account_idnum *block, int width, unsigned next_id)
{
	if (block == NULL)
		return;

	block->width = width;
	block->next_id = next_id;
}


/**
 * Read the raw values from an Account ID Number instance directly.
 *
 * \param *block		The ID Number instance to read.
 * \param *width		Pointer to a variable to take the width of
 *				the ID field, in digits.
 * \param *next_id		Pointer to a variable to take the next ID
 *				value of the field.
 */

void account_idnum_get(struct account_idnum *block, int *width, unsigned *next_id)
{
	if (block == NULL)
		return;

	if (width != NULL)
		*width = block->width;

	if (next_id != NULL)
		*next_id = block->next_id;
}


/**
 * Report whether an ID Number instance is active.
 *
 * \param *block		The ID Number instance to report on.
 * \return			TRUE if the instance is active; FALSE if not.
 */

osbool account_idnum_active(struct account_idnum *block)
{
	if (block == NULL)
		return FALSE;

	return (block->width > 0) ? TRUE : FALSE;
}


/**
 * Write the next ID number from the sequence into the supplied buffer,
 * and optionally increment the number by a specified amount.
 *
 * \param *block		The ID number instance to use.
 * \param *buffer		Pointer to the buffer to write the ID to.
 * \param length		The length of the supplied buffer.
 * \param increment		The amount to increment the ID by, or 0 for none.
 * \return			Pointer to the written number, or NULL.
 */

char *account_idnum_get_next(struct account_idnum *block, char *buffer, size_t length, unsigned increment)
{
	char		format[ACCOUNT_IDNUM_FORMAT_LENGTH];

	/* Check that there's a buffer for us to use */

	if (buffer == NULL || length == 0)
		return NULL;

	/* Set the buffer to empty, and check that we have an object. */

	buffer[0] = '\0';

	if (block == NULL || !account_idnum_active(block))
		return buffer;

	/* Generate the required ID number in the buffer. */

	string_printf(format, ACCOUNT_IDNUM_FORMAT_LENGTH, "%%0%dd", block->width);
	string_printf(buffer, length, format, block->next_id);

	block->next_id += increment;

	return buffer;
}

