/* Copyright 2003-2012, Stephen Fryatt
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

/**
 * Initialise the Find module.
 */

void find_initialise(void);


/**
 * Open the Find dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void find_open_window(file_data *file, wimp_pointer *ptr, osbool restore);


/**
 * Force the closure of the Find and Results windows if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void find_force_windows_closed(file_data *file);

#endif

