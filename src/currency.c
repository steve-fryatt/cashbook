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
 * \file: currency.c
 *
 * String to data value conversions.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/territory.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/config.h"

/* Application header files */

#include "currency.h"


/**
 * The maximum number of digits that can form a currency value. This includes
 * decimal places, so 1.23 would be 3 digits. The value is determined by the
 * size of the type used to hold an amt_t.
 */

#define CURRENCY_MAX_DIGITS 9

/**
 * The maximum string length that can be converted into an amount value, which
 * sets the size of the working buffer.
 */

#define CURRENCY_MAX_CONVERSION_LENGTH 256


/**
 * Global Variables
 */

static int	currency_decimal_places = 0;					/**< The number of decimal places used in currency representation.		*/
static char	currency_decimal_point = '.';					/**< The symbol used to represent the decimal point.				*/

static osbool	currency_print_zeros = FALSE;					/**< Should zero values be converted to digits or a blank space?		*/
static osbool	currency_bracket_negatives = FALSE;				/**< Should negative values be represented (1.23) instead of -1.23?		*/


/**
 * Initialise, or re-initialise, the currency module.
 *
 * NB: This may be called multiple times, to re-initialise the currency module
 * when the application choices are changed.
 */

void currency_initialise(void)
{
	currency_print_zeros = config_opt_read("PrintZeros");

	if (config_opt_read("TerritoryCurrency")) {
		currency_decimal_point = *territory_read_string_symbols(territory_CURRENT, territory_SYMBOL_CURRENCY_POINT);
		currency_decimal_places = territory_read_integer_symbols(territory_CURRENT, territory_SYMBOL_CURRENCY_PRECISION);
		currency_bracket_negatives = (territory_read_integer_symbols(territory_CURRENT, territory_SYMBOL_CURRENCY_NEGATIVE_FORMAT)
				== territory_SYMBOL_PARENTHESISED);
	} else {
		currency_decimal_point = *config_str_read("DecimalPoint");
		currency_decimal_places = config_int_read("DecimalPlaces");
		currency_bracket_negatives = config_opt_read("BracketNegatives");
	}
}

/* ==================================================================================================================
 * Money conversion.
 */

void full_convert_money_to_string (amt_t value, char *string, int zeros)
{
  int old_zero;

  old_zero = currency_print_zeros;
  currency_print_zeros = zeros;

  convert_money_to_string (value, string);

  currency_print_zeros = old_zero;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void convert_money_to_string (amt_t value, char *string)
{
  int  i, places;
  char *end, conversion[20];

  if (value != NULL_CURRENCY || currency_print_zeros)
  {
    /* Find the number of decimal places and set up a conversion string. Negative numbers need an additional place
     * in the format string, as the - sign takes up one of the 'digits'.
     */

    places = currency_decimal_places + 1;
    sprintf (conversion, "%%0%1dd", places + ((value < 0) ? 1 : 0));

    /* Print the number to the buffer and find the end. */

    sprintf (string, conversion, value);
    end = strchr (string, '\0');

    /* If there is a decimal point, shuffle the higher digits up one to make room and insert it. */

    if (places > 1)
    {
      for (i=1; i<=places; i++)
      {
        *(end + 1) = *end;
        end--;
      }

      *(++end) = currency_decimal_point;
    }

    /* If () is to be used for -ve numbers, replace the - */

    if (currency_bracket_negatives && *string == '-')
    {
      *string = '(';
      strcat (string, ")");
    }
  }
  else
  {
    *string = '\0';
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/**
 * Convert a string into a currency amount by brute force, based on the
 * configured settings, to ensure that accuracy is retained.
 *
 * \param *string		The string to be converted into a currency amount.
 * \return			The converted amount, or NULL_CURRENCY.
 */

amt_t currency_convert_from_string(char *string)
{
	int	result, decimal;
	osbool	negative;
	char	copy[CURRENCY_MAX_CONVERSION_LENGTH], *next;

	if (string == NULL || *string == '\0')
		return NULL_CURRENCY;

	/* Test for a negative value, by looking at the first character.
	 *
	 * \TODO -- Bracketed values should probably test the final
	 *          character as well?
	 */

	if (currency_bracket_negatives)
		negative = (*string == '(');
	else
		negative = (*string == '-');

	/* Take a copy of the string, with a leading zero so that values like
	 * .01 work OK. If the value is negative, then start one character in
	 * to skip the leading '-' or '('.
	 */

	snprintf(copy, CURRENCY_MAX_CONVERSION_LENGTH, "0%s", (negative) ? string + 1 : string);
	copy[CURRENCY_MAX_CONVERSION_LENGTH - 1] = '\0';

	/* Get the part of the string before the decimal place. If there isn't
	 * one, then return with an invalid amount.
	 */

	next = strtok(copy, ".");
	if (next == NULL)
		return NULL_CURRENCY;

	/* Take the value from before the DP and convert it to an integer.
	 *
	 * If the number is small enough to fit into the available digits, then
	 * the result is value * 10^decimal_places to shift the value up out
	 * of the way of the decimal part. If it isn't, then we just set the
	 * value to the maximum possible (10^decimal_places - 1).
	 *
	 * Note that we add 1 to the available digits, to cover leading 0 we
	 * added when copying the value.
	 */

	if ((strlen(next) + currency_decimal_places) <= CURRENCY_MAX_DIGITS + 1)
		result = atoi(next) * pow(10, currency_decimal_places);
	else
		result = (1 * pow(10, CURRENCY_MAX_DIGITS - currency_decimal_places) - 1) * pow(10, currency_decimal_places);

	/* Now see if there were any digits after the DP.  If there were,
	 * these are now processed.
	 */

	next = strtok(NULL, ".");
	if (currency_decimal_places > 0 && next != NULL) {
		/* If there were too many digits for the decimal part, truncate
		 * to the required number and lose the precision. No rounding
		 * is performed, so .119 would become .11 to 2dp. */

		if (strlen(next) > currency_decimal_places)
			next[currency_decimal_places] = '\0';

		/* Convert the required digits into an int. */

		decimal = atoi(next);

		/* If there weren't enough digits, multiply by the required
		 * power of 10 to shift the value into the correct place.
		 */

		if (strlen(next) < currency_decimal_places)
			decimal *= pow (10, currency_decimal_places - strlen(next));

		/* Add the decimal part on to the result. */

		result += decimal;
	}

	/* If the original value was negative, turn the converted value
	 * negative as well.
	 */

	if (negative)
		result = -result;

	/* Turn the converted value into an amount and return it. */

	return (amt_t) result;
}

