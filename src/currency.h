/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * Initialise, or re-initialise, the currency module.
 */

void currency_initialise(void);


/* Money conversion. */

void full_convert_money_to_string (amt_t value, char *string, int zeros);
void convert_money_to_string (amt_t value, char *string);


/**
 * Convert a string into a currency amount by brute force, based on the
 * configured settings.
 *
 * \param *string		The string to be converted into a currency amount.
 * \return			The converted amount, or NULL_CURRENCY.
 */

amt_t currency_convert_from_string(char *string);

#endif
