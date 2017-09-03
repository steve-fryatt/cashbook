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
 * \file: print_dialogue.h
 *
 * Print Dialogue Interface.
 */

#ifndef CASHBOOK_PRINT_DIALOGUE
#define CASHBOOK_PRINT_DIALOGUE

#include "global.h"

/**
 * A Print Dialogue Definition.
 */

struct print_dialogue_block;

/**
 * Initialise the printing system.
 */

void print_dialogue_initialise(void);


/**
 * Construct new printing data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to own the block.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct print_dialogue_block *print_dialogue_create(struct file_block *file);


/**
 * Delete a printing data block.
 *
 * \param *print	Pointer to the printing window data to delete.
 */

void print_dialogue_delete(struct print_dialogue_block *print);


/**
 * Force the closure of any printing windows which are open and relate
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void print_dialogue_force_windows_closed(struct file_block *file);


/**
 * Open the Simple Print dialoge box, as used by a number of print routines.
 *
 * \param *instance	The print dialogue instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The MessageTrans token for the dialogue title.
 * \param *report	The MessageTrans token for the report title, or NULL
 *			if the client doesn't need a report. 
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 * \param *data		Data to be passed to the callback function.
 */

void print_dialogue_open_simple(struct print_dialogue_block *instance, wimp_pointer *ptr, osbool restore, char *title, char *report,
		void (callback) (struct report *, void *, osbool, osbool, osbool, osbool, osbool), void *data);


/**
 * Open the Advanced Print dialoge box, as used by a number of print routines.
 *
 * \param *instance	The print dialogue instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param *report	The MessageTrans token for the report title, or NULL
 *			if the client doesn't need a report. 
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 * \param *data		Data to be passed to the callback function.
 */

void print_dialogue_open_advanced(struct print_dialogue_block *instance, wimp_pointer *ptr, osbool restore, char *title, char *report,
		void (callback) (struct report *, void *, osbool, osbool, osbool, osbool, osbool, date_t, date_t), void *data);

#endif
