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
 * \file: interest.h
 *
 * Interest Rate manager Interface
 */

#ifndef CASHBOOK_INTEREST
#define CASHBOOK_INTEREST

struct interest_block;

#include "global.h"

/**
 * Initialise the transaction system.
 */

void interest_initialise(void);


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

#endif
