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
 * \file: printing.h
 *
 * Print Dialogue Interface.
 */

#ifndef CASHBOOK_PRINTING
#define CASHBOOK_PRINTING


/**
 * Initialise the printing system.
 */

void printing_initialise(void);


/**
 * Construct new printing data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct printing *printing_create(void);


/**
 * Delete a printing data block.
 *
 * \param *print	Pointer to the printing window data to delete.
 */

void printing_delete(struct printing *print);


/**
 * Force the closure of any printing windows which are open and relate
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void printing_force_windows_closed(struct file_block *file);


/**
 * Open the Simple Print dialoge box, as used by a number of print routines.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 */

void printing_open_simple_window(struct file_block *file, wimp_pointer *ptr, osbool restore, char *title,
		void (callback) (osbool, osbool, osbool, osbool, osbool));


/**
 * Open the Simple Print dialoge box, as used by a number of print routines.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 */

void printing_open_advanced_window(struct file_block *file, wimp_pointer *ptr, osbool restore, char *title,
		void (callback) (osbool, osbool, osbool, osbool, osbool, date_t, date_t));

#endif

