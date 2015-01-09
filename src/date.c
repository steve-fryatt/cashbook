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
 * \file: date.c
 *
 * Date implementation.
 *
 * This source file contains functions to handle the date system used by
 * CashBook.  It allows a resolution of one day, stored in a single unsigned
 * integer.  The format used is:
 *
 *  0xYYYYMMDD
 *
 * which allows dates to be sorted simply by numerical order and years from
 * 0 to 65536 to be stored...
 *
 * A NULL_DATE is represented by 0xffffffff, causing empty entries to sort
 * to the end of the file.
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

#include "date.h"

#include "transact.h"


/**
 * The size of the input date separator list.
 */

#define DATE_SEP_LENGTH 11


#define DATE_FIELD_DAY (0x000000ff)
#define DATE_FIELD_MONTH (0x0000ff00)
#define DATE_FIELD_YEAR (0xffff0000)

#define DATE_SHIFT_DAY 0
#define DATE_SHIFT_MONTH 8
#define DATE_SHIFT_YEAR 16

#define date_get_day_from_date(date) (int) ((date) & DATE_FIELD_DAY)
#define date_get_month_from_date(date) (int) (((date) & DATE_FIELD_MONTH) >> DATE_SHIFT_MONTH)
#define date_get_year_from_date(date) (int) (((date) & DATE_FIELD_YEAR) >> DATE_SHIFT_YEAR)
#define date_combine_parts(day, month, year) (date_t) (((day) & DATE_FIELD_DAY) + (((month) << DATE_SHIFT_MONTH) & DATE_FIELD_MONTH) + (((year) << DATE_SHIFT_YEAR) & DATE_FIELD_YEAR))


/* ==================================================================================================================
 * Global variables.
 */

static enum date_days		date_weekend_days;				/**< A bitmask containing the days that form the weeekend.			*/
static char			date_sep_out;					/**< The character used to separate dates when displaying them.			*/
static char			date_sep_in[DATE_SEP_LENGTH];			/**< A list of the characters usable as separators when entering dates.		*/



static osbool		date_is_string_numeric(char *string);

/**
 * Initialise, or re-initialise, the date module.
 *
 * NB: This may be called multiple times, to re-initialise the date module
 * when the application choices are changed.
 */

void date_initialise(void)
{
	int				i;
	territory_calendar		calendar;
	oswordreadclock_utc_block	clock;


	clock.op = oswordreadclock_OP_UTC;
	oswordreadclock_utc(&clock);
	territory_read_calendar_information(territory_CURRENT, (const os_date_and_time *) &(clock.utc), &calendar);

	if (config_opt_read("TerritorySOrders")) {
		/* Take the weekend days from the Territory system. */

		date_weekend_days = DATE_DAY_NONE;

#ifdef DEBUG
	debug_printf("Working days %d to %d", calendar.first_working_day, calendar.last_working_day);
#endif

	for (i = 1; i < calendar.first_working_day; i++) {
		date_weekend_days |= 1 << (i-1);
#ifdef DEBUG
		debug_printf("Adding weekend day %d", i);
#endif
	}

	for (i = calendar.last_working_day + 1; i <= 7; i++) {
		date_weekend_days |= 1 << (i - 1);
#ifdef DEBUG
		debug_printf("Adding weekend day %d", i);
#endif
	}

#ifdef DEBUG
		debug_printf("Resulting weekends 0x%x", date_weekend_days);
#endif
	} else {
		/* Use the weekend days as set in the Choices window. */

		date_weekend_days = (enum date_days) config_int_read("WeekendDays");
	}

	/* Set the date separators. */

	date_sep_out = *config_str_read ("DateSepOut");

	strncpy(date_sep_in, config_str_read("DateSepIn"), DATE_SEP_LENGTH);
	date_sep_in[DATE_SEP_LENGTH - 1] = '\0';
}


/**
 * Convert a date into a string in the format DD-MM-YYYY (where '-' is the
 * configured divider), placing the result into the supplied buffer.
 *
 * \param date			The date to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *date_convert_to_string(date_t date, char *buffer, size_t length)
{
	int	day, month, year;

	if (buffer == NULL || length <= 0)
		return NULL;

	/* If the value is NULL_DATE, blank the buffer and return. */

	if (date == NULL_DATE) {
		*buffer = '\0';
		return buffer;
	}

	/* split the date up and convert it to text. */

	day = date_get_day_from_date(date);
	month = date_get_month_from_date(date);
	year = date_get_year_from_date(date);

	snprintf(buffer, length, "%02d%c%02d%c%04d", day, date_sep_out, month, date_sep_out, year);
	buffer[length - 1] = '\0';

	return buffer;
}


/**
 * Convert a date into a month-and-year string in the format Month YYYY,
 * placing the result into the supplied buffer.
 *
 * \param date			The date to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *date_convert_to_month_string(date_t date, char *buffer, size_t length)
{
	os_date_and_time	os_date;
	territory_ordinals	ordinals;


	if (buffer == NULL || length <= 0)
		return NULL;

	/* If the value is NULL_DATE, blank the buffer and return. */

	if (date == NULL_DATE) {
		*buffer = '\0';
		return buffer;
	}

	ordinals.centisecond = 0;
	ordinals.second = 0;
	ordinals.minute = 0;
	ordinals.hour = 12;
	ordinals.date = date_get_day_from_date(date);
	ordinals.month = date_get_month_from_date(date);
	ordinals.year = date_get_year_from_date(date);

	territory_convert_ordinals_to_time(territory_CURRENT, &os_date, &ordinals);

	territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &os_date, buffer, length, "%MO %CE%YR");

	return buffer;
}


/**
 * Convert a date into a year string in the format YYYY, placing the result
 * into the supplied buffer.
 *
 * \param date			The date to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *date_convert_to_year_string(date_t date, char *buffer, size_t length)
{
	os_date_and_time	os_date;
	territory_ordinals	ordinals;


	if (buffer == NULL || length <= 0)
		return NULL;

	/* If the value is NULL_DATE, blank the buffer and return. */

	if (date == NULL_DATE) {
		*buffer = '\0';
		return buffer;
	}

	ordinals.centisecond = 0;
	ordinals.second = 0;
	ordinals.minute = 0;
	ordinals.hour = 12;
	ordinals.date = date_get_day_from_date(date);
	ordinals.month = date_get_month_from_date(date);
	ordinals.year = date_get_year_from_date(date);

	territory_convert_ordinals_to_time(territory_CURRENT, &os_date, &ordinals);

	territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &os_date, buffer, length, "%CE%YR");

	return buffer;
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
    valid = date_is_string_numeric (next);

    next = strtok (NULL, date_sep_in);
    month = (next != NULL) ? atoi (next) : base_month;
    valid = valid && (next == NULL || date_is_string_numeric (next));

    next = strtok (NULL, date_sep_in);
    year = (next != NULL) ? atoi (next) : base_year;
    valid = valid && (next == NULL || date_is_string_numeric (next));

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

    if (unit == DATE_PERIOD_YEARS)
    {
      year += add;
    }

    /* Add or subtract months.  If the month ends up out of range (1-12), add or subtract years until it comes back
     * into range again.
     */

    else if (unit == DATE_PERIOD_MONTHS)
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

    else if (unit == DATE_PERIOD_DAYS)
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

/**
 * Count the number of days (inclusive) between two dates.
 *
 * \param start		The start date.
 * \param end		The end date.
 * \return		The number of days (inclusive) between the start
 *			and end dates.
 */

int date_count_days(date_t start, date_t end)
{
	int	day1, month1, year1, day2, month2, year2, days = 0;


	if (start == NULL_DATE || end == NULL_DATE)
		return 0;

	day1 = date_get_day_from_date(start);
	month1 = date_get_month_from_date(start);
	year1 = date_get_year_from_date(start);

	day2 = date_get_day_from_date(end);
	month2 = date_get_month_from_date(end);
	year2 = date_get_year_from_date(end);

	if (month1 == month2 && year1 == year2) {
		/* If both dates are in the same month and year, the
		 * calculation is simple.
		 */

		days = day2 - day1 + 1;
	} else {
		/* Otherwise, we need to count through the days a month
		 * at a time.
		 */

		days = days_in_month(month1, year1) - day1 + 1;

		month1++;

		if (month1 > months_in_year(year1)) {
			month1 = 1;
			year1++;
		}

		while ((year1 < year2 && month1 <= months_in_year(year1)) || (year1 == year2 && month1 < month2)) {
			days += days_in_month(month1, year1);

			month1++;

			if (month1 > months_in_year(year1)) {
				month1 = 1;
				year1++;
			}
		}

		days += day2;
	}

	return days;
}


/**
 * Take a raw date (where the day can be in the range 1-31 regardless of the
 * month), and make it valid by either moving it forwards to the last valid
 * day in the month or pushing it back to the 1st of the following month.
 *
 * \param date		The raw date to be adjusted.
 * \param direction	The direction to adjust the date in.
 * \return		The adjusted date.
 */

date_t date_find_valid_day(date_t date, enum date_adjust direction)
{
	int	day, month, year;


	if (date == NULL_DATE)
		return NULL_DATE;

	/* Extract the component parts of the date. */

	day = date_get_day_from_date(date);
	month = date_get_month_from_date(date);
	year = date_get_year_from_date(date);

	/* Shuffle the dates around. */

	if (direction == DATE_ADJUST_FORWARD && day > days_in_month(month, year)) {
		day = days_in_month(month, year);
	} else if (direction == DATE_ADJUST_BACKWARD && day > days_in_month(month, year)) {
		day = 1;
		month += 1;

		if (month > months_in_year(year)) {
			month = 1;
			year += 1;
		}
	} else if (direction == DATE_ADJUST_FORWARD && day < 1) {
		month -= 1;

		if (month < 1) {
			year -= 1;
			month = months_in_year(year);
		}

		day = days_in_month(month, year);
	} else if (direction == DATE_ADJUST_BACKWARD && day < 1) {
		day = 1;
	}

	return date_combine_parts(day, month, year);
}


/**
 * Take a raw date (where the day can be in the range 1-31 regardless of the
 * month), and bring the day into a valid range for the current month. Then
 * adjust the date forward or backwards to ensure that it does not fall on
 * a weekend day.
 *
 * \param date		The raw date to be adjusted.
 * \param direction	The direction to adjust the date, or none.
 * \return		The adjusted date.
 */ 

date_t date_find_working_day(date_t date, enum date_adjust direction)
{
	int		weekday, move, shift;
	date_t		result;


	if (date == NULL_DATE)
		return NULL_DATE;
		

	/* Take the date and move it into a valid position in the current month. */

	result = date_find_valid_day(date, DATE_ADJUST_FORWARD);

	/* Correct for weekends, if necessary. */

	switch (direction) {
	case DATE_ADJUST_FORWARD:
		move = -1;
		break;
	case DATE_ADJUST_BACKWARD:
		move = 1;
		break;
	default:
		move = 0;
		break;
	}
	

	if (move == 0)
		return result;

	shift = 0;
	weekday = day_of_week(result);

	/* While the weekend bit is set for the current weekday, move the
	 * date one day in the specified direction and try again.
	 */

	while ((1 << (weekday-1)) & date_weekend_days) {
		shift += move;
		weekday += move;

		if (weekday > 7)
			weekday = 1;

		if (weekday < 1)
			weekday = 7;
	}

	/* If a shift is required, add the necessary days on to the base
	 * date.
	 */

	if (shift != 0)
		result = add_to_date(result, DATE_PERIOD_DAYS, shift);

	return result;
}


/**
 * Test a string, to see if all its characters are numeric.
 *
 * \param *string	Pointer to the string to test.
 * \return		TRUE if all the characters are numeric; FALSE if not.
 */

static osbool date_is_string_numeric(char *string)
{
	if (string == NULL)
		return FALSE;

	while (*string != '\0' && isdigit(*string))
		string++;

	return (*string == '\0') ? TRUE : FALSE;
}

