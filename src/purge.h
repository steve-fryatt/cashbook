/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: purge.h
 *
 * Transaction purge implementation.
 */

#ifndef CASHBOOK_PURGE
#define CASHBOOK_PURGE


/**
 * Initialise the Purge module.
 */

void purge_initialise(void);


/**
 * Construct new purge data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to which this instance belongs.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct purge_block *purge_create(struct file_block *file);


/**
 * Delete a purge data block.
 *
 * \param *purge	Pointer to the purge window data to delete.
 */

void purge_delete(struct purge_block *purge);


/**
 * Open the Purge dialogue box.
 *
 * \param *purge	The purge instance owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void purge_open_window(struct purge_block *purge, wimp_pointer *ptr, osbool restore);

#endif

