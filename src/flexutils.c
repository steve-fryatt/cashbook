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
 * \file: flexutils.c
 *
 * Extensions to the Flex implementation.
 */

/* ANSII Header Files. */

#include <stdlib.h>

/* Acorn C Header files. */

#include "flex.h"

/* OSLib Header files. */

#include "oslib/types.h"

/* SFLib Header files. */

#include "sflib/debug.h"

/* Application header files. */

#include "flexutils.h"

/**
 * The minimum block to allocate.
 */

#define FLEXUTILS_MIN_BLOCK 4

/**
 * The current allocation block size.
 */

static size_t flexutils_block_size = 0;

/**
 * Initialise a NULL flex anchor with the minimum amount of memory
 * necessary to allow an allocation to take place. If the allocation
 * fails, the anchor is set to NULL.
 *
 * \param **anchor		The flex anchor to be initialised.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool flexutils_initialise(void **anchor)
{
	if (anchor == NULL  || *anchor != NULL)
		return FALSE;

	if (flex_alloc((flex_ptr) anchor, FLEXUTILS_MIN_BLOCK) == 0) {
		*anchor = NULL;
		return FALSE;
	}

	return TRUE;
}


/**
 * Free a non-NULL flex anchor, and set the anchor to NULL.
 *
 * \param **anchor		The flex anchor to be freed.
 */

void flexutils_free(void **anchor)
{
	if (anchor == NULL || *anchor == NULL)
		return;

	flex_free((flex_ptr) anchor);
}


/**
 * Given a unit block size, work out how many objects will fit into a
 * given flex block. The call will fail if the block size doesn't
 * correspond to a round number of objects.
 * 
 * This call is used to start a sequence of allocations via
 * flexutils_resize_block(); the sequence ends on a call to 
 * flexutils_shrink_block().
 *
 * \param **anchor		The flex anchor to look at.
 * \param block			The size of a single object in the block.
 * \param *size			Pointer to an array to hold the number of blocks found.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_get_size(void **anchor, size_t block, size_t *size)
{
	size_t	bytes;

	if (anchor == NULL || *anchor == NULL || size == NULL)
		return FALSE;

	bytes = flex_size((flex_ptr) anchor);
	*size = bytes / block;

#ifdef DEBUG
	debug_printf("Requesting the current block size: %d bytes, %d blocks (%d bytes/block)", bytes, *size, block);
#endif

	if (((*size * block) != bytes) && (bytes != FLEXUTILS_MIN_BLOCK)) {
		*size = 0;
		return FALSE;
	}

	flexutils_block_size = block;

	return TRUE;
}


/**
 * Resize a flex block to hold a specified number of objects. The size of
 * an object is taken to be that supplied to a previous call to
 * flexutils_get_size().
 *
 * \param **anchor		The flex anchor to be resized.
 * \param new_size		The required number of objects.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_resize_block(void **anchor, size_t new_size)
{
	if (anchor == NULL || *anchor == NULL || flexutils_block_size == 0)
		return FALSE;

#ifdef DEBUG
	debug_printf("Requesting the current block re-size: %d bytes, %d blocks (%d bytes/block)", flexutils_block_size * new_size, new_size, flexutils_block_size);
#endif

	if (flex_extend((flex_ptr) anchor, flexutils_block_size * new_size) == 0)
		return FALSE;

	return TRUE;
}


/**
 * If required, shrink a flex block down so that it holds only the specified
 * number of objects. The size of an object is taken to be that supplied to
 * a previous call to flexutils_get_size(). At the end of this call, that
 * size is discarded: preventing any more calls to flexutils_resize_block().
 * 
 * \param **anchor		The flex anchor to be shrunk.
 * \param new_size		The maximum required number of objects.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_shrink_block(void **anchor, size_t new_size)
{
	size_t	block_size;

	if (anchor == NULL || *anchor == NULL || flexutils_block_size == 0)
		return FALSE;

#ifdef DEBUG
	debug_printf("Requesting the current block shrink to %d blocks", new_size);
#endif

	if (!flexutils_get_size(anchor, flexutils_block_size, &block_size)) {
		flexutils_block_size = 0;
		return FALSE;
	}

	flexutils_block_size = 0;

	if ((block_size > new_size) && (flex_extend((flex_ptr) anchor, flexutils_block_size * new_size)))
		return FALSE;

	return TRUE;
}
