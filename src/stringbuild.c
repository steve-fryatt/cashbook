/* Copyright 2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: stringbuild.c
 *
 * Compound String Builder Implementation.
 */

/* ANSI C header files */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* OSLib header files */

#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"

/* Application header files */

#include "global.h"
#include "stringbuild.h"

#include "currency.h"
#include "date.h"
#include "report.h"


/* Global Variables. */

/**
 * The buffer into which the string is being constructed, or NULL for none.
 */

static char *stringbuild_buffer = NULL;

/**
 * The first unused location in the buffer, or NULL if none.
 */

static char *stringbuild_ptr = NULL;

/**
 * The location of the final character in the buffer, or NULL if none.
 */

static char *stringbuild_end = NULL;

/**
 * Set TRUE if a line was too long for the buffer during the current session.
 */

static osbool stringbuild_too_long = FALSE;

/* Macros. */

/**
 * Check whether the current stringbuilder information is valid.
 */

#define stringbuild_valid() ((stringbuild_buffer != NULL) && (stringbuild_ptr != NULL) && (stringbuild_end != NULL))

/**
 * The number of bytes remainining in the buffer, excluding the final
 * terminating character (with a full buffer, *stringbuild_end would
 * point to the terminating \0 character).
 */

#define stringbuild_remaining() (stringbuild_end - stringbuild_ptr)

/**
 * Initialise a new stringbuild session, using a supplied memory buffer to
 * construct the new line.
 *
 * \param *buffer		Pointer to a buffer to use for assembling the string.
 * \param length		The length of the supplied buffer.
 * \return			TRUE if successful; FALSE on error.
 */

osbool stringbuild_initialise(char *buffer, size_t length)
{
	if (buffer == NULL || length <= 0) {
		stringbuild_reset();
		return FALSE;
	}

	stringbuild_buffer = buffer;
	stringbuild_ptr = buffer;
	stringbuild_end = buffer + length - 1;

	/* Terminate the string. */

	*stringbuild_end = '\0';

	/* Reset the too long flag. */

	stringbuild_too_long = FALSE;

	return TRUE;
}


/**
 * Terminate a stringbuild session, resetting the pointers to prevent further
 * use of the existing buffer.
 */

void stringbuild_cancel(void)
{
	stringbuild_buffer = NULL;
	stringbuild_ptr = NULL;
	stringbuild_end = NULL;

	if (stringbuild_too_long)
		error_msgs_report_error("StringTooLong");

	stringbuild_too_long = FALSE;
}


/**
 * Clear the contents of a stringbuild settion, ready for a new string to be
 * assembled in the buffer.
 */

void stringbuild_reset(void)
{
	stringbuild_ptr = stringbuild_buffer;
}


/**
 * Terminate the current string, and return a pointer to it.
 *
 * \return			Pointer to the string, or NULL if there is none.
 */

char *stringbuild_get_line(void)
{
	if (!stringbuild_valid())
		return NULL;

	/* If the buffer has overrun, do nothing. */

	if (stringbuild_too_long == TRUE)
		return NULL;

	/* Terminate the buffer and return the string it holds. */

	*stringbuild_ptr = '\0';

	return stringbuild_buffer;
}


/**
 * Terminate the current string, and write it to the specified report.
 *
 * \param *report		The report to write to.
 * \param bar			The tab bar to use.
 */

void stringbuild_report_line(struct report *report, int tab_bar)
{
	char *line;

	line = stringbuild_get_line();
	if (line != NULL)
		report_write_line(report, tab_bar, line);
}

#ifdef UNIT_TESTING

/**
 * Return the remaining count for use in Unit Testing.
 *
 * \return			The value of stringbuild_remaining().
 */

int stringbuild_get_remaining(void)
{
	return stringbuild_remaining();
}


/**
 * Return the too long status for use in Unit Testing.
 *
 * \return			The value of stringbuild_too_long.
 */

osbool stringbuild_get_too_long(void)
{
	return stringbuild_too_long;
}

#endif

/**
 * Add a string to the end of the current line.
 *
 * \param *string		Pointer to the string to be added.
 */

void stringbuild_add_string(char *string)
{
	if (string == NULL)
		return;

	size_t length = strlen(string);
	size_t chars_to_write = 0;

	if (length <= stringbuild_remaining()) {
		chars_to_write = length + 1;
	} else {
		chars_to_write = stringbuild_remaining() + 1;
		stringbuild_too_long = TRUE;
	}

	if (chars_to_write <= 1)
		return;

	strncpy(stringbuild_ptr, string, chars_to_write);
	stringbuild_ptr += (chars_to_write - 1);
}


/**
 * Add a string to the end of the current line, using the standard printf()
 * syntax and functionality.
 *
 * \param *cntrl_string		A standard printf() formatting string.
 * \param ...			Additional printf() parameters as required.
 * \return			The number of characters written, or <0 for error.
 */

int stringbuild_add_printf(char *cntrl_string, ...)
{
	if (cntrl_string == NULL)
		return 0;

	int space_available = stringbuild_remaining() + 1;

	if (space_available <= 1) {
		stringbuild_too_long = TRUE;
		return 0;
	}

	va_list ap;

	va_start(ap, cntrl_string);
	int chars_written = vsnprintf(stringbuild_ptr, space_available, cntrl_string, ap);
	va_end(ap);

	if (chars_written < space_available) {
		stringbuild_ptr += chars_written;
	} else if (chars_written >= 0) {
		stringbuild_ptr += (space_available - 1);
		*stringbuild_end = '\0';
		stringbuild_too_long = TRUE;
	} else {
		*stringbuild_ptr = '\0';
	}

	return chars_written;
}


/**
 * Add a string looked up from the application messages to the end of the
 * current line.
 *
 * \param *token		Pointer to the token of the message to
 *				be added.
 */

void stringbuild_add_message(char *token)
{
	int space_available = stringbuild_remaining() + 1;

	if (space_available <= 1) {
		stringbuild_too_long = TRUE;
		return;
	}

	enum msgs_status result = msgs_lookup_result(token, stringbuild_ptr, space_available);

	if (result == MSGS_STATUS_ERROR)
		return;

	while (stringbuild_ptr < stringbuild_end && *stringbuild_ptr != '\0')
		stringbuild_ptr++;

	if (result == MSGS_STATUS_BUFFER_FULL)
		stringbuild_too_long = TRUE;
}


/**
 * Add a string looked up from the application messages to the end of the
 * current line, allowing for paramater substitution.
 *
 * \param *token		Pointer to the token of the message to
 *				be added.
 * \param *a			Pointer to parameter %0, or NULL.
 * \param *b			Pointer to parameter %1, or NULL.
 * \param *c			Pointer to parameter %2, or NULL.
 * \param *d			Pointer to parameter %3, or NULL.
 */

void stringbuild_add_message_param(char *token, char *a, char *b, char *c, char *d)
{
	int space_available = stringbuild_remaining() + 1;

	if (space_available <= 1) {
		stringbuild_too_long = TRUE;
		return;
	}

	enum msgs_status result = msgs_param_lookup_result(token, stringbuild_ptr, space_available, a, b, c, d);

	if (result == MSGS_STATUS_ERROR)
		return;

	while (stringbuild_ptr < stringbuild_end && *stringbuild_ptr != '\0')
		stringbuild_ptr++;

	if (result == MSGS_STATUS_BUFFER_FULL)
		stringbuild_too_long = TRUE;
}


/**
 * Add a currency value to the end of the current line.
 *
 * \param value			The value to be converted.
 * \param print_zeros		TRUE to convert zero values as 0; FALSE to
 *				return a blank string.
 */

void stringbuild_add_currency(amt_t value, osbool print_zeros)
{
//	if (stringbuild_remaining() <= 0)
//		return;
//
//	currency_flexible_convert_to_string(value, stringbuild_ptr, stringbuild_remaining(), print_zeros);
//
//	while (stringbuild_ptr < stringbuild_end && *stringbuild_ptr != '\0')
//		stringbuild_ptr++;
}


/**
 * Add a date value to the end of the current line.
 *
 * \param value			The date to be converted.
 */

void stringbuild_add_date(date_t date)
{
//	if (stringbuild_remaining() <= 0)
//		return;
//
//	date_convert_to_string(date, stringbuild_ptr, stringbuild_remaining());
//
//	while (stringbuild_ptr < stringbuild_end && *stringbuild_ptr != '\0')
//		stringbuild_ptr++;
}


/**
 * Add an icon's contents to the end of the current line.
 *
 * \param window		The window containing the icon.
 * \param icon			The icon to copy text from.
 */

void stringbuild_add_icon(wimp_w window, wimp_i icon)
{
//	if (stringbuild_remaining() <= 0)
//		return;
//
//	icons_copy_text(window, icon, stringbuild_ptr, stringbuild_remaining());
//
//	while (stringbuild_ptr < stringbuild_end && *stringbuild_ptr != '\0')
//		stringbuild_ptr++;
}
