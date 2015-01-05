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

#include "global.h"
#include "currency.h"

/* ==================================================================================================================
 * Global variables
 */

int decimal_places = 0;
char decimal_point = '.';

int print_zeros = 0;
int bracket_negatives = 0;

/* ==================================================================================================================
 * Money conversion.
 */

void full_convert_money_to_string (amt_t value, char *string, int zeros)
{
  int old_zero;

  old_zero = print_zeros;
  print_zeros = zeros;

  convert_money_to_string (value, string);

  print_zeros = old_zero;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void convert_money_to_string (amt_t value, char *string)
{
  int  i, places;
  char *end, conversion[20];

  if (value != NULL_CURRENCY || print_zeros)
  {
    /* Find the number of decimal places and set up a conversion string. Negative numbers need an additional place
     * in the format string, as the - sign takes up one of the 'digits'.
     */

    places = decimal_places + 1;
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

      *(++end) = decimal_point;
    }

    /* If () is to be used for -ve numbers, replace the - */

    if (bracket_negatives && *string == '-')
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

/* Convert the string into an integer by brute force. */

amt_t convert_string_to_money (char *string)
{
  int  result, pence, negative;
  char copy[256], *next;


  if (bracket_negatives)
  {
    negative = (*string == '(');
  }
  else
  {
    negative = (*string == '-');
  }

  /* Take a copy of the string, with a leading zero so that values like ".01" work OK.
   * If the value is negative, then start one character in to skip the leading '-' or '('.
   */

  sprintf (copy, "0%s", (negative) ? string + 1 : string);

  /* Get the part of the string before the decimal place. */

  next = strtok (copy, ".");
  if (next != NULL)
  {
    /* Take the value from before the DP and convert it to an integer. */

    if ((strlen (next) + decimal_places) <= MAX_DIGITS + 1) /* Add 1 to cover leading 0 we added. */
    {
      result = atoi (next)*pow(10, decimal_places);
    }
    else
    {
      result = (1 * pow (10, MAX_DIGITS - decimal_places) - 1) * pow(10, decimal_places);
    }

    /* Now see if there were any digits after the DP.  If there were...*/

    next = strtok (NULL, ".");
    if (decimal_places > 0 && next != NULL)
    {
      /* If there were too many digits, truncate to the required number. */

      if (strlen (next) > decimal_places)
      {
        next[decimal_places] = '\0';
      }

      /* Convert the remaining digits into an int. */

      pence = atoi (next);

      /* If there weren't enough digits, multiply by the required power of 10. */

      if (strlen (next) < decimal_places)
      {
        pence *= pow (10, decimal_places - strlen (next));
      }

      /* Add the pence on to the result. */

      result += pence;
    }

    if (negative)
    {
      result = -result;
    }
  }
  else
  {
    result = NULL_CURRENCY;
  }

  return ((amt_t) result);
}

/* ==================================================================================================================
 * Set up date information
 */

void set_up_money (void)
{
  print_zeros = config_opt_read ("PrintZeros");

  if (config_opt_read ("TerritoryCurrency"))
  {
    decimal_point = *territory_read_string_symbols (territory_CURRENT, territory_SYMBOL_CURRENCY_POINT);
    decimal_places = territory_read_integer_symbols (territory_CURRENT, territory_SYMBOL_CURRENCY_PRECISION);
    bracket_negatives = (territory_read_integer_symbols (territory_CURRENT, territory_SYMBOL_CURRENCY_NEGATIVE_FORMAT)
                         == territory_SYMBOL_PARENTHESISED);
  }
  else
  {
    decimal_point = *config_str_read ("DecimalPoint");
    decimal_places = config_int_read ("DecimalPlaces");
    bracket_negatives = config_opt_read ("BracketNegatives");
  }
}
