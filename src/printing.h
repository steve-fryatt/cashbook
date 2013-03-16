/* Copyright 2003-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * Low-level printing implementation.
 */

#ifndef CASHBOOK_PRINTING
#define CASHBOOK_PRINTING


/**
 * Initialise the printing system.
 */

void printing_initialise(void);


/**
 * Send a Message_PrintSave to start the printing process off with the
 * RISC OS printer driver.
 *
 * \param *callback_print	Callback function to start the printing once
 *				negotiations have completed successfully.
 * \param *callback_cancel	Callback function to terminate printing
 *				if things fail at any stage.
 * \param *text_print		TRUE to print as text; FALSE to print in
 *				graphics mode.
 * \return			TRUE if process started OK; FALSE on error.
 */

osbool printing_send_start_print_save(void (*callback_print) (char *), void (*callback_cancel) (void), osbool text_print);


/**
 * Force the closure of any printing windows which are open and relate
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void printing_force_windows_closed(file_data *file);


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

void printing_open_simple_window(file_data *file, wimp_pointer *ptr, osbool restore, char *title,
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

void printing_open_advanced_window(file_data *file, wimp_pointer *ptr, osbool restore, char *title,
		void (callback) (osbool, osbool, osbool, osbool, osbool, date_t, date_t));

#endif

