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
 * \file: account_idnum.h
 *
 * Cheque or PayIn ID Number interface.
 */

#ifndef CASHBOOK_ACCOUNT_IDNUM
#define CASHBOOK_ACCOUNT_IDNUM

#include <stddef.h>

/**
 * The Account ID Number Data Structure.
 *
 * This is define publicly to allow copies to be inlined into the Account
 * data structure. Clients are discouraged from making any assumptions
 * about the contents of the struct, however.
 */

struct account_idnum {
	unsigned	next_id;		/**< The next ID number in the sequence.	*/
	int		width;			/**< The display width of the ID number.	*/
}


/**
 * Initialise a new Account ID Number instance.
 *
 * \param *block		The ID number instance to initialise.
 */

void account_idnum_initialise(struct account_idnum *block);


/**
 * Set an Account ID Number instance using details copied from another
 * instance.
 *
 * \param *block		The ID Number instance to initialise.
 * \param *from			The ID Number instance to copy from.
 */

void account_idnum_copy(struct account_idnum *block, struct account_idnum *from);


/**
 * Set the raw values in an Account ID Number instance directly.
 *
 * \param *block		The ID Number instance to set.
 * \param width			The width of the ID field, in digits.
 * \param next_id		The next ID value of the field.
 */

void account_idnum_set(struct account_idnum *block, int width, unsigned next_id);


/**
 * Read the raw values from an Account ID Number instance directly.
 *
 * \param *block		The ID Number instance to read.
 * \param *width		Pointer to a variable to take the width of
 *				the ID field, in digits.
 * \param *next_id		Pointer to a variable to take the next ID
 *				value of the field.
 */

void account_idnum_get(struct account_idnum *block, int *width, unsigned *next_id);


/**
 * Report whether an ID Number instance is active.
 *
 * \param *block		The ID Number instance to report on.
 * \return			TRUE if the instance is active; FALSE if not.
 */

osbool account_idnum_active(struct account_idnum *block);


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

char *account_idnum_get_next(struct account_idnum *block, char *buffer, size_t length, unsigned increment);

#endif

