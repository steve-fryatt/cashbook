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
 * \file: date.c
 *
 * Date implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osword.h"
#include "oslib/os.h"
#include "oslib/osword.h"
#include "oslib/territory.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/config.h"

/* Application header files */

#include "global.h"
#include "date.h"

/* ==================================================================================================================
 * Global variables.
 */

int weekend_days;

char date_sep_out;
char date_sep_in[DATE_SEP_LENGTH];

/* ==================================================================================================================
 * Date conversion.
 *
 * This source file contains functions to handle the date system used by Accounts.  It allows a resolution of one
 * day, stored in a single unsigned integer.  The format used is:
 *
 *  0xYYYYMMDD
 *
 * which allows dates to be sorted simply by numerical order.  This allows years from 0 to 65536 to be stored...
 * A 'null date' is represented by 0xffffffff, causing empty entries to sort to the end of the file.
 */

void convert_date_to_string (date_t date, char *string)
{
  int day, month, year;


  if (date != NULL_DATE)
  {
    day   = (date & 0x000000ff);
    month = (date & 0x0000ff00) >> 8;
    year  = (date & 0xffff0000) >> 16;

    sprintf (string, "%02d%c%02d%c%04d", day, date_sep_out, month, date_sep_out, year);
  }
  else
  {
    *string = '\0';
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Convert a string to a date in internal format (0xYYYYMMDD).  If previous_date is not NULL_DATE, base any
 * missing fields on this.  If month_days==0, days are limited to the specified month, otherwise days are limited
 * to month_days.
 */

date_t convert_string_to_date (char *string, date_t previous_date, int month_days)
{
  int                       day, month, year, base_month, base_year, day_limit, month_limit, valid;
  date_t                    result;
  char                      *next, date[256];
  oswordreadclock_utc_block time;
  territory_ordinals        ordinals;


  #ifdef DEBUG
  debug_printf ("\\GConverting date");
  #endif

  /* Get the date to base an incomplete entry on.  If a previous date is supplied, use that, otherwise
   * get the current date from the system.
   */

  if (previous_date != NULL_DATE)
  {
    base_month = (previous_date & 0x0000ff00) >> 8;
    base_year  = (previous_date & 0xffff0000) >> 16;

    #ifdef DEBUG
    debug_printf ("Base dates from given date (year: %d, month: %d)", base_year, base_month);
    #endif
  }
  else
  {
    time.op = oswordreadclock_OP_UTC;
    oswordreadclock_utc (&time);
    territory_convert_time_to_ordinals (territory_CURRENT, (const os_date_and_time *) &(time.utc), &ordinals);

    base_year = ordinals.year;
    base_month = ordinals.month;

    #ifdef DEBUG
    debug_printf ("Base dates from RTC (year: %d, month: %d)", base_year, base_month);
    #endif
  }

  /* Parse the string. */

  strcpy (date, string);

  next = strtok (date, date_sep_in);

  if (next != NULL)
  {
    day = atoi (next);
    valid = is_numeric (next);

    next = strtok (NULL, date_sep_in);
    month = (next != NULL) ? atoi (next) : base_month;
    valid = valid && (next == NULL || is_numeric (next));

    next = strtok (NULL, date_sep_in);
    year = (next != NULL) ? atoi (next) : base_year;
    valid = valid && (next == NULL || is_numeric (next));

    if (valid)
    {
      #ifdef DEBUG
      debug_printf ("Read date as day %d, month %d, year %d", day, month, year);
      #endif

      if (year >= 0 && year < 80)
      {
        year += 2000;
      }
      else if (year >=80 && year <= 99)
      {
        year += 1900;
      }

      month_limit = months_in_year (year);

      if (month > month_limit)
      {
        month = month_limit;
      }

      day_limit = (month_days == 0) ? days_in_month (month, year) : month_days;

      if (day > day_limit)
      {
        day = day_limit;
      }

      result = (day & 0x00ff) + ((month & 0x00ff) << 8) + ((year & 0xffff) << 16);
    }
    else
    {
      result = NULL_DATE;
    }
  }
  else
  {
    result = NULL_DATE;
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void convert_date_to_month_string (date_t date, char *string)
{
  os_date_and_time   os_date;
  territory_ordinals ordinals;


  ordinals.centisecond = 0;
  ordinals.second = 0;
  ordinals.minute = 0;
  ordinals.hour = 12;
  ordinals.date = (date & 0x000000ff);
  ordinals.month = (date & 0x0000ff00) >> 8;
  ordinals.year = (date & 0xffff0000) >> 16;

  territory_convert_ordinals_to_time (territory_CURRENT, &os_date, &ordinals);

  /* NB: assumes that buffer is 20 chars long... */
  territory_convert_date_and_time (territory_CURRENT, (const os_date_and_time *) &os_date, string, 20, "%MO %CE%YR");
}

/* ------------------------------------------------------------------------------------------------------------------ */

void convert_date_to_year_string (date_t date, char *string)
{
  os_date_and_time   os_date;
  territory_ordinals ordinals;


  ordinals.centisecond = 0;
  ordinals.second = 0;
  ordinals.minute = 0;
  ordinals.hour = 12;
  ordinals.date = (date & 0x000000ff);
  ordinals.month = (date & 0x0000ff00) >> 8;
  ordinals.year = (date & 0xffff0000) >> 16;

  territory_convert_ordinals_to_time (territory_CURRENT, &os_date, &ordinals);

  /* NB: assumes that buffer is 20 chars long... */
  territory_convert_date_and_time (territory_CURRENT, (const os_date_and_time *) &os_date, string, 20, "%CE%YR");
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Get today's date as an integer in internal format. */

date_t get_current_date (void)
{
  oswordreadclock_utc_block time;
  territory_ordinals        ordinals;


  time.op = oswordreadclock_OP_UTC;
  oswordreadclock_utc (&time);
  territory_convert_time_to_ordinals (territory_CURRENT, (const os_date_and_time *) &(time.utc), &ordinals);

  return (ordinals.date & 0x00ff) + ((ordinals.month & 0x00ff) << 8) + ((ordinals.year & 0xffff) << 16);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Add a given period on to the specified date.  The date returned may not be valid, and will require further processing
 * to find a real date to enter a standing order on (applies to month period, where day may fall outside days in the
 * current month).
 */

date_t add_to_date (date_t date, int unit, int add)
{
  int    day, month, year;
  date_t result;

  if (date != NULL_DATE)
  {
    day   = (date & 0x000000ff);
    month = (date & 0x0000ff00) >> 8;
    year  = (date & 0xffff0000) >> 16;

    /* Add or subtract whole years.  No other processing is required. */

    if (unit == PERIOD_YEARS)
    {
      year += add;
    }

    /* Add or subtract months.  If the month ends up out of range (1-12), add or subtract years until it comes back
     * into range again.
     */

    else if (unit == PERIOD_MONTHS)
    {
      month += add;

      while (month > months_in_year (year))
      {
        year++;
        month -= months_in_year (year);
      }

      while (month <= 0)
      {
        year--;
        month += months_in_year (year);
      }
    }

    /* Add or subtract days.  If the days end up out of range for the current month, adjust the days and months
     * as required.  If this takes the months out of range, correct the years too.
     */

    else if (unit == PERIOD_DAYS)
    {
      day += add;

      while (day > days_in_month (month, year))
      {
        day -= days_in_month (month, year);
        month ++;

        if (month > months_in_year (year))
        {
          year++;
          month -= months_in_year (year);
        }
      }

      while (day <= 0)
      {
        month --;
        day += days_in_month (month, year);

        if (month <= 0)
        {
          year--;
          month += months_in_year (year);
        }
      }
    }

    /* Recompose the date from the component parts. */

    result = (day & 0x00ff) + ((month & 0x00ff) << 8) + ((year & 0xffff) << 16);
  }
  else
  {
    result = NULL_DATE;
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the number of days in the given month. */

int days_in_month (int month, int year)
{
  os_date_and_time   date;
  territory_ordinals ordinals;
  territory_calendar calendar;


  if (config_opt_read ("Territory_dates"))
  {
    ordinals.centisecond = 0;
    ordinals.second = 0;
    ordinals.minute = 0;
    ordinals.hour = 12;
    ordinals.date = 1;
    ordinals.month = month;
    ordinals.year = year;

    territory_convert_ordinals_to_time (territory_CURRENT, &date, &ordinals);

    territory_read_calendar_information (territory_CURRENT, (const os_date_and_time *) &date, &calendar);
  }
  else
  {
    calendar.month_count = 12;
    switch (month)
    {
      case 2:
        calendar.day_count = ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0))) ? 29 : 28;
        break;

      case 4:
      case 6:
      case 9:
      case 11:
        calendar.day_count = 30;
        break;

      default:
        calendar.day_count = 31;
        break;
    }
  }

  #ifdef DEBUG
  debug_printf ("%d days in month %d (year %d)", calendar.day_count, month, year);
  #endif

  return (calendar.day_count);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the number of months in the given month. */

int months_in_year (int year)
{
  os_date_and_time   date;
  territory_ordinals ordinals;
  territory_calendar calendar;


  if (config_opt_read ("Territory_dates"))
  {
    ordinals.centisecond = 0;
    ordinals.second = 0;
    ordinals.minute = 0;
    ordinals.hour = 12;
    ordinals.date = 1;
    ordinals.month = 1;
    ordinals.year = year;

    territory_convert_ordinals_to_time (territory_CURRENT, &date, &ordinals);

    territory_read_calendar_information (territory_CURRENT, (const os_date_and_time *) &date, &calendar);
  }
  else
  {
    calendar.month_count = 12;
  }

  #ifdef DEBUG
  debug_printf ("%d months in year %d", calendar.month_count, year);
  #endif

  return (calendar.month_count);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Use the Territory module to find the day number (1 == Sunday -> 7 == Saturday). */

int day_of_week (unsigned int date)
{
  int                result;
  territory_ordinals ordinals;
  os_date_and_time   time;

  if (date != NULL_DATE)
  {
    ordinals.centisecond = 0;
    ordinals.second = 0;
    ordinals.minute = 0;
    ordinals.hour = 0;
    ordinals.date = (date & 0x000000ff);
    ordinals.month = (date & 0x0000ff00) >> 8;
    ordinals.year = (date & 0xffff0000) >> 16;
    territory_convert_ordinals_to_time (territory_CURRENT, &time, (territory_ordinals const *) &ordinals);
    territory_convert_time_to_ordinals (territory_CURRENT, (os_date_and_time const *) &time, &ordinals);

    result = ordinals.weekday;
  }
  else
  {
    result = 0;
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return true if the given dates encompass a full month. */

int full_month (date_t start, date_t end)
{
  int    day1, month1, year1, day2, month2, year2, result;


  result = 0;

  if (start != NULL_DATE && end != NULL_DATE)
  {
    day1   = (start & 0x000000ff);
    month1 = (start & 0x0000ff00) >> 8;
    year1  = (start & 0xffff0000) >> 16;

    day2   = (end & 0x000000ff);
    month2 = (end & 0x0000ff00) >> 8;
    year2  = (end & 0xffff0000) >> 16;

    result = (day1 == 1 && day2 == days_in_month (month2, year2) && month1 == month2 && year1 == year2);
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return true if the given dates encompass a full year. */

int full_year (date_t start, date_t end)
{
  int    day1, month1, year1, day2, month2, year2, result;


  result = 0;

  if (start != NULL_DATE && end != NULL_DATE)
  {
    day1   = (start & 0x000000ff);
    month1 = (start & 0x0000ff00) >> 8;
    year1  = (start & 0xffff0000) >> 16;

    day2   = (end & 0x000000ff);
    month2 = (end & 0x0000ff00) >> 8;
    year2  = (end & 0xffff0000) >> 16;

    result = (day1 == 1 && day2 == days_in_month (month2, year2) &&
              month1 == 1 && month2 == months_in_year (year2) && year1 == year2);
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the number of days between the given dates (inclusive). */

int count_days (date_t start, date_t end)
{
  int    day1, month1, year1, day2, month2, year2, result;


  result = 0;

  if (start != NULL_DATE && end != NULL_DATE)
  {
    day1   = (start & 0x000000ff);
    month1 = (start & 0x0000ff00) >> 8;
    year1  = (start & 0xffff0000) >> 16;

    day2   = (end & 0x000000ff);
    month2 = (end & 0x0000ff00) >> 8;
    year2  = (end & 0xffff0000) >> 16;

    if (month1 == month2 && year1 == year2)
    {
      result = day2 - day1 + 1;
    }
    else
    {
      result = days_in_month (month1, year1) - day1 + 1;

      month1++;

      if (month1 > months_in_year (year1))
      {
        month1 = 1;
        year1++;
      }

      while ((year1 < year2 && month1 <= months_in_year(year1)) || (year1 == year2 && month1 < month2))
      {
        result += days_in_month (month1, year1);

        month1++;

        if (month1 > months_in_year (year1))
        {
          month1 = 1;
          year1++;
        }
      }

      result += day2;
    }
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Make the date valid, by adjusting the day so that it is valid for the month.  If direction is <0, the day is
 * pulled into range for the month and year; if it is >0, the date is moved on to the 1st of the next month.
 */

date_t get_valid_date (date_t date, int direction)
{
  int    day, month, year;
  date_t result;


  if (date != NULL_DATE)
  {
    day   = (date & 0x000000ff);
    month = (date & 0x0000ff00) >> 8;
    year  = (date & 0xffff0000) >> 16;

    /* Handle cases where the day is greater than those in the given month. */

    if (direction < 0 && day > days_in_month (month, year))
    {
      day = days_in_month (month, year);
    }

    else if (direction > 0 && day > days_in_month (month, year))
    {
      day = 1;
      month += 1;

      if (month > months_in_year (year))
      {
        month = 1;
        year += 1;
      }
    }

    /* Handle cases where the day is less than 1. */

    else if (direction < 0 && day < 1)
    {
      month -= 1;

      if (month < 1)
      {
        year -= 1;
        month = months_in_year (year);
      }

      day = days_in_month (month, year);
    }

    else if (direction > 0 && day < 1)
    {
      day = 1;
    }

    result = (day & 0x00ff) + ((month & 0x00ff) << 8) + ((year & 0xffff) << 16);
  }
  else
  {
    result = NULL_DATE;
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take a raw standing order date and pull it in to a valid date for the order to be processed.  First the day is
 * brought into range for the current month, then weekends are skipped if necessary.
 */

date_t get_sorder_date (date_t date, int flags)
{
  int    weekday, move, weekends, shift;
  date_t result;

  if (date != NULL_DATE)
  {
    /* Take the date and move it into a valid position in the current month. */

    result = get_valid_date (date, -1);

    /* Correct for weekends, if necessary. */

    if (flags & TRANS_SKIP_FORWARD)
    {
      move = -1;
    }
    else if (flags & TRANS_SKIP_BACKWARD)
    {
      move = 1;
    }
    else
    {
      move = 0;
    }

    if (move != 0)
    {
      shift = 0;
      weekends = read_weekend_days ();
      weekday = day_of_week (result);

      /* While the weekend bit is set for the current weekday, move in the specified direction and try again. */

      while ((1 << (weekday-1)) & weekends)
      {
        shift += move;
        weekday += move;

        if (weekday > 7)
        {
          weekday = 1;
        }
        if (weekday < 1)
        {
          weekday = 7;
        }
      }

      /* If a shift was required, add the necessary days on to the base date. */

      if (shift)
      {
        result = add_to_date (result, PERIOD_DAYS, shift);
      }
    }
  }
  else
  {
    result = NULL_DATE;
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Set the weekend days option following a change in the configuration. */

void set_weekend_days (void)
{
  int                       i;
  territory_calendar        calendar;
  oswordreadclock_utc_block clock;

  clock.op = oswordreadclock_OP_UTC;
  oswordreadclock_utc (&clock);
  territory_read_calendar_information (territory_CURRENT, (const os_date_and_time *) &(clock.utc), &calendar);

  if (config_opt_read ("TerritorySOrders"))
  {
    weekend_days = 0;

    #ifdef DEBUG
    debug_printf ("Working days %d to %d", calendar.first_working_day, calendar.last_working_day);
    #endif

    for (i = 1; i < calendar.first_working_day; i++)
    {
      weekend_days |= 1 << (i-1);
      #ifdef DEBUG
      debug_printf ("Adding weekend day %d", i);
      #endif
    }

    for (i = calendar.last_working_day + 1; i <= 7; i++)
    {
      weekend_days |= 1 << (i-1);
      #ifdef DEBUG
      debug_printf ("Adding weekend day %d", i);
      #endif
    }

    #ifdef DEBUG
    debug_printf ("Resulting weekends 0x%x", weekend_days);
    #endif
  }
  else
  {
    /* Use the value as set in the Choices window. */

    weekend_days = config_int_read ("WeekendDays");
  }

  /* Set the other date info. */

  date_sep_out = *config_str_read ("DateSepOut");
  strcpy (date_sep_in, config_str_read ("DateSepIn"));
}

/* ------------------------------------------------------------------------------------------------------------------ */

int read_weekend_days (void)
{
  return (weekend_days);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int is_numeric (char *str)
{
  while (*str != '\0' && isdigit(*str))
  {
    str++;
  }

  return (*str == '\0');
}
