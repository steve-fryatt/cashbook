/* Copyright 2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: flexutils.h
 *
 * Extensions to the Flex implementation
 */

#ifndef CASHBOOK_FLEXUTILS
#define CASHBOOK_FLEXUTILS

#include <stdlib.h>
#include "oslib/types.h"
#include "flex.h"


/**
 * Initialise a flex anchor with the minimum amount of memory necessary
 * to allow an allocation to take place. If the allocation fails, the
 * anchor is set to NULL.
 *
 * \param **anchor		The flex anchor to be initialised.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool flexutils_initialise(void **anchor);


/**
 * Free a non-NULL flex anchor, and set the anchor to NULL.
 *
 * \param **anchor		The flex anchor to be freed.
 */

void flexutils_free(void **anchor);

osbool flexutils_get_size(void **anchor, size_t block, size_t *size);

osbool flexutils_resize_block(void **anchor, size_t new_size);

#endif
