/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: find.c
 *
 * Transaction contents search implementation.
 */

#ifndef CASHBOOK_FIND
#define CASHBOOK_FIND

#include "file.h"

struct find_block;

/**
 * Search logic options.
 */

enum find_logic {
	FIND_NOLOGIC = 0,							/**< No logic has been specified.					*/
	FIND_AND = 1,								/**< Search using AND logic to combine the fields.			*/
	FIND_OR = 2								/**< Search using OR logic to comnine the fields.			*/
};

/**
 * Search direction options.
 */

enum find_direction {
	FIND_NODIR = 0,								/**< No direction has been specified.					*/
	FIND_START = 1,								/**< Begin searching down from the start of the file.			*/
	FIND_END = 2,								/**< Begin searching up from the end of the file.			*/
	FIND_UP = 3,								/**< Continue searching up.						*/
	FIND_DOWN = 4,								/**< Continue searching down.						*/
	FIND_NEXT = 5,								/**< Find the next match in the current direction.			*/
	FIND_PREVIOUS = 6							/**< Find the previous match in the current direction.			*/
};

/**
 * Initialise the Find module.
 */

void find_initialise(void);


/**
 * Construct new find data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to which this instance belongs.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct find_block *find_create(struct file_block *file);


/**
 * Delete a find data block.
 *
 * \param *find		Pointer to the find window data to delete.
 */

void find_delete(struct find_block *find);


/**
 * Open the Find dialogue box.
 *
 * \param *windat	The Find instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void find_open_window(struct find_block *windat, wimp_pointer *ptr, osbool restore);

#endif

