/* Copyright 2016-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: interest.h
 *
 * Interest Rate manager Interface
 */

#ifndef CASHBOOK_INTEREST
#define CASHBOOK_INTEREST

/**
 * An interest rate representation.
 */

typedef int rate_t;

/**
 * Invalid or missing interest rate.
 */

#define NULL_RATE ((rate_t) -1)

/**
 * An interest rate management instance.
 */

struct interest_block;

#include "global.h"

#include "account.h"
#include "date.h"

/**
 * Initialise the transaction system.
 *
 * \param *sprites		The application sprite area.
 */

void interest_initialise(osspriteop_area *sprites);


/**
 * Create a new interest rate module instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct interest_block *interest_create_instance(struct file_block *file);


/**
 * Delete an interest rate module instance, and all of its data.
 *
 * \param *instance		The instance to be deleted.
 */

void interest_delete_instance(struct interest_block *instance);


/**
 * Return an interest rate for a given account on a given date. Returns
 * NULL_RATE on failure.
 *
 * \param *instance		The interest rate module instance to use.
 * \param account		The account to return an interest rate for.
 * \param date			The date to return the rate for.
 * \return			The interest rate on the date in question.
 */

rate_t interest_get_current_rate(struct interest_block *instance, acct_t account, date_t date);


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

char *interest_convert_to_string(rate_t rate, char *buffer, size_t length);

#endif
