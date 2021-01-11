/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: currency.h
 *
 * String to data value conversions.
 */

#ifndef CASHBOOK_CURRENCY
#define CASHBOOK_CURRENCY

/**
 * An amount of currency.
 */

typedef int		amt_t;


/**
 * Invalid or missing currency value.
 */

#define NULL_CURRENCY ((amt_t) 0)

/**
 * Get a currency field from an input file.
 */

#define currency_get_currency_field(in) ((amt_t) filing_get_int_field((in)))

/**
 * Initialise, or re-initialise, the currency module.
 */

void currency_initialise(void);


/**
 * Convert a currency amount into a string, placing the result into a
 * supplied buffer. The configured application setting for zero display
 * will be used.
 *
 * \param value			The value to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *currency_convert_to_string(amt_t value, char *buffer, size_t length);


/**
 * Convert a currency amount into a string, placing the result into a
 * supplied buffer.
 *
 * \param value			The value to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \param print_zeros		TRUE to convert zero values as 0; FALSE to
 *				return a blank string.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *currency_flexible_convert_to_string(amt_t value, char *buffer, size_t length, osbool print_zeros);


/**
 * Convert a string into a currency amount by brute force, based on the
 * configured settings.
 *
 * \param *string		The string to be converted into a currency amount.
 * \return			The converted amount, or NULL_CURRENCY.
 */

amt_t currency_convert_from_string(char *string);

#endif
