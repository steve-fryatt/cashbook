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
 * \file: stringbuild.h
 *
 * Compound String Builder Interface.
 */


#ifndef CASHBOOK_STRINGBUILD
#define CASHBOOK_STRINGBUILD

#include "currency.h"

/**
 * Initialise a new stringbuild session, using a supplied memory buffer to
 * construct the new line.
 *
 * \param *buffer		Pointer to a buffer to use for assembling the string.
 * \param length		The length of the supplied buffer.
 * \return			TRUE if successful; FALSE on error.
 */

osbool stringbuild_initialise(char *buffer, size_t length);


/**
 * Terminate a stringbuild session, resetting the pointers to prevent further
 * use of the existing buffer.
 */

void stringbuild_cancel(void);


/**
 * Clear the contents of a stringbuild settion, ready for a new string to be
 * assembled in the buffer.
 */

void stringbuild_reset(void);


/**
 * Terminate the current string, and return a pointer to it.
 *
 * \return			Pointer to the string, or NULL if there is none.
 */

char *stringbuild_get_line(void);


/**
 * Terminate the current string, and write it to the specified report.
 *
 * \param *report		The report to write to.
 * \param bar			The tab bar to use.
 */

void stringbuild_report_line(struct report *report, int tab_bar);


/**
 * Add a string to the end of the current line.
 *
 * \param *string		Pointer to the string to be added.
 */

void stringbuild_add_string(char *string);


/**
 * Add a string to the end of the current line, using the standard printf()
 * syntax and functionality.
 *
 * \param *cntrl_string		A standard printf() formatting string.
 * \param ...			Additional printf() parameters as required.
 * \return			The number of characters written, or <0 for error.
 */

int stringbuild_add_printf(char *cntrl_string, ...);


/**
 * Add a string looked up from the application messages to the end of the
 * current line.
 *
 * \param *token		Pointer to the token of the message to
 *				be added.
 */

void stringbuild_add_message(char *token);


/**
 * Add a currency value to the end of the current line.
 *
 * \param value			The value to be converted.
 * \param print_zeros		TRUE to convert zero values as 0; FALSE to
 *				return a blank string.
 */

void stringbuild_add_currency(amt_t value, osbool print_zeros);

#endif

