/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/string.h"

/* Application header files */

#include "date.h"


/**
 * The size of the input date separator list.
 */

#define DATE_SEP_LENGTH 11


/**
 * The size of the date buffer used when converting from string.
 */

#define DATE_CONVERT_BUFFER_LEN 64

/**
 * The number of fields in a date (ie. day, month, year).
 */

#define DATE_FIELDS 3

/**
 * The definition of a date format.
 */

struct date_format_info {
	enum date_format	format;			/**< The format reference.												*/
	int			day_field;		/**< The field, from the left, holding the day value.									*/
	int			month_field;		/**< The field, from the left, holding the month value.									*/
	int			year_field;		/**< the field, from the left, holding the year value.									*/
	int			short_offset;		/**< The first offset to apply if fewer than DATE_FIELDS are present; add 1 for each missing field; -1 means no offset.	*/
};

/**
 * The date formats understood by the application.
 */

static struct date_format_info date_formats[] = {
	{DATE_FORMAT_DMY, 0, 1, 2, -1},
	{DATE_FORMAT_YMD, 2, 1, 0, 1},
	{DATE_FORMAT_MDY, 1, 0, 2, 0}
};

/**
 * A set of days, as used by OS and Territory SWIs.
 */

enum date_os_day {
	DATE_OS_DAY_NONE = 0,
	DATE_OS_DAY_SUNDAY = 1,
	DATE_OS_DAY_MONDAY = 2,
	DATE_OS_DAY_TUESDAY = 3,
	DATE_OS_DAY_WEDNESDAY = 4,
	DATE_OS_DAY_THURSDAY = 5,
	DATE_OS_DAY_FRIDAY = 6,
	DATE_OS_DAY_SATURDAY = 7
};


/**
 * The first day of the OS week.
 */

#define DATE_FIRST_OS_DAY 1


/**
 * The last day of the OS week.
 */

#define DATE_LAST_OS_DAY 7

/**
 * Details of the internal date implementation.
 */

/* Bitmasks to locate the day, month and year in the date_t type. */

#define DATE_FIELD_DAY (0x000000ff)
#define DATE_FIELD_MONTH (0x0000ff00)
#define DATE_FIELD_YEAR (0xffff0000)

/* Shifts to move the day, month and year into their correct places as above. */

#define DATE_SHIFT_DAY 0
#define DATE_SHIFT_MONTH 8
#define DATE_SHIFT_YEAR 16

/**
 * Extract int days, int months and int years from a date_t.
 */

#define date_get_day_from_date(date) (int) ((date) & DATE_FIELD_DAY)
#define date_get_month_from_date(date) (int) (((date) & DATE_FIELD_MONTH) >> DATE_SHIFT_MONTH)
#define date_get_year_from_date(date) (int) (((date) & DATE_FIELD_YEAR) >> DATE_SHIFT_YEAR)

/**
 * Convert an int day, int month and int year into a date_t.
 */

#define date_combine_parts(day, month, year) (date_t) (((day) & DATE_FIELD_DAY) + (((month) << DATE_SHIFT_MONTH) & DATE_FIELD_MONTH) + (((year) << DATE_SHIFT_YEAR) & DATE_FIELD_YEAR))


/**
 * Global Variables
 */

static enum date_days		date_weekend_days;				/**< A bitmask containing the days that form the weeekend.			*/
static char			date_sep_out;					/**< The character used to separate dates when displaying them.			*/
static char			date_sep_in[DATE_SEP_LENGTH];			/**< A list of the characters usable as separators when entering dates.		*/

/**
 * The date format currently being used by the application.
 */

static enum date_format		date_active_format;

/**
 * Static Function Protypes
 */

static int			date_days_in_month(int month, int year);
static int			date_months_in_year(int year);
static enum date_os_day		date_day_of_week(date_t date);
static osbool			date_is_string_numeric(char *string);


/**
 * Initialise, or re-initialise, the date module.
 *
 * NB: This may be called multiple times, to re-initialise the date module
 * when the application choices are changed.
 */

void date_initialise(void)
{
	int				day;
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

	for (day = DATE_FIRST_OS_DAY; day < calendar.first_working_day; day++) {
		date_weekend_days |= date_convert_day_to_days(day);
#ifdef DEBUG
		debug_printf("Adding weekend day %d", day);
#endif
	}

	for (day = calendar.last_working_day + 1; day <= DATE_LAST_OS_DAY; day++) {
		date_weekend_days |= date_convert_day_to_days(day);
#ifdef DEBUG
		debug_printf("Adding weekend day %d", day);
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

	date_sep_out = *config_str_read("DateSepOut");

	strncpy(date_sep_in, config_str_read("DateSepIn"), DATE_SEP_LENGTH);
	date_sep_in[DATE_SEP_LENGTH - 1] = '\0';

	/* Set the date format. */

	date_active_format = (enum date_format) config_int_read("DateFormat");
	if (date_active_format < 0 || date_active_format >= DATE_FORMATS)
		date_active_format = DATE_FORMAT_DMY;
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

	switch (date_active_format) {
	case DATE_FORMAT_DMY:
		snprintf(buffer, length, "%02d%c%02d%c%04d", day, date_sep_out, month, date_sep_out, year);
		break;
	case DATE_FORMAT_YMD:
		snprintf(buffer, length, "%04d%c%02d%c%02d", year, date_sep_out, month, date_sep_out, day);
		break;
	case DATE_FORMAT_MDY:
		snprintf(buffer, length, "%02d%c%02d%c%04d", month, date_sep_out, day, date_sep_out, year);
		break;
	}
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


/**
 * Convert a string into a date, where the string is in the format
 * "DD", "DD-MM" or "DD-MM-YYYY" and "-" is any of the configured date
 * separators. The missing fields are filled in from the base date, which
 * if not specified is the current date. The number of days allowed in a
 * month will normally be taken from the month and year found in the
 * string, but can be overridden to allow different numbers of dates (eg.
 * to allow dates with 31 days to be entered).
 *
 * \param *string		Pointer to the string to be converted.
 * \param base_date		The date to use as a base for any missing
 *				fields; NULL_DATE to use current date.
 * \param month_days		The number of days to allow in a the month;
 *				0 to use the month and year found in the
 *				string.
 * \return			The converted date, or NULL_DATE on error.
 */

date_t date_convert_from_string(char *string, date_t base_date, int month_days)
{
	int	field, offset, day, month, year, base_month, base_year, day_limit, month_limit;
	char	*next, date[DATE_CONVERT_BUFFER_LEN], *fields[DATE_FIELDS];


	if (string == NULL)
		return NULL_DATE;

#ifdef DEBUG
	debug_printf("\\GConverting date");
#endif

	/* Get the date to base an incomplete entry on.  If a previous date
	 * is supplied, use that, otherwise get the current date from the
	 * system.
	 */

	if (base_date != NULL_DATE)
		base_date = date_today();
 
	base_month = date_get_month_from_date(base_date);
	base_year  = date_get_year_from_date(base_date);

#ifdef DEBUG
	debug_printf("Base dates on year: %d, month: %d", base_year, base_month);
#endif

	/* Take a copy of the string, so that we can parse it with strtok(). */

	strncpy(date, string, DATE_CONVERT_BUFFER_LEN);
	date[DATE_CONVERT_BUFFER_LEN - 1] = '\0';

	/* Split the string up into fields at the separators. */

	next = strtok(date, date_sep_in);

	for (field = 0; next != NULL && field < DATE_FIELDS; field++) {
		fields[field] = next;
		next = strtok(NULL, date_sep_in);
	}

#ifdef DEBUG
	debug_printf("String processed, found %d fields with %s trailing", field, (next == NULL) ? "none" : "some");
#endif

	/* At the end, we should have at least one field, and there should be none left. */

	if (field == 0 || next != NULL)
		return NULL_DATE;

	/* Work out an offset to apply to field indexes to account for missing fields. */

	if ((field < DATE_FIELDS) && (date_formats[date_active_format].short_offset >= 0))
		offset = date_formats[date_active_format].short_offset + (DATE_FIELDS - field) - 1;
	else
		offset = 0;

	/* Get the day; if not numeric, the date is invalid. */

	if (!date_is_string_numeric(fields[date_formats[date_active_format].day_field - offset]))
		return NULL_DATE;

	day = atoi(fields[date_formats[date_active_format].day_field - offset]);

	/* Get the month. If not present, the base month is used; if not
	 * numeric, the date is invalid.
	 */

	if (field >= 2) {
		if (!date_is_string_numeric(fields[date_formats[date_active_format].month_field - offset]))
			return NULL_DATE;

		month = atoi(fields[date_formats[date_active_format].month_field - offset]);
	} else {
		month = base_month;
	}

	/* Get the year. If not present, the base year is used; if not
	 * numeric, the date is invalid.
	 */

	if (field >= 3) {
		if (!date_is_string_numeric(fields[date_formats[date_active_format].year_field - offset]))
			return NULL_DATE;

		year = atoi(fields[date_formats[date_active_format].year_field - offset]);
	} else {
		year = base_year;
	}

#ifdef DEBUG
	debug_printf("Read date as day %d, month %d, year %d", day, month, year);
#endif

	/* Years from 00-79 are converted to 2000-2079; years from 80-99 are
	 * converted to 1980-1999. ALl other years are left as they are
	 * entered, allowing anything from 100ad to be entered.
	 */

	if (year < 0)
		year = 0;

	if (year >= 0 && year < 80)
		year += 2000;
	else if (year >=80 && year <= 99)
		year += 1900;

	/* Check the month, and bring it into a valid range for the year
	 * given.
	 */

	if (month < 1)
		month = 1;

	month_limit = date_months_in_year(year);

	if (month > month_limit)
		month = month_limit;

	/* Check the day, and bring it into a valid range for the month and
	 * year given.
	 */

	if (day < 1)
		day = 1;

	day_limit = (month_days == 0) ? date_days_in_month(month, year) : month_days;

	if (day > day_limit)
		day = day_limit;

	return date_combine_parts(day, month, year);
}


/**
 * Add a specified period on to a date.
 *
 * When adding days or years, the resulting date will always be valid. When
 * adding months, the day will be retained and may therefore fall outside
 * the valid range for the end month; such dates should then be passed to
 * date_find_valid_day() to bring them into range.
 *
 * \param date			The date to add to or subtract from.
 * \param unit			The unit of the supplied period.
 * \param period		The period to add (when +ve) or subtract
 *				(when -ve) to the date.
 * \return			The modified date, or NULL_DATE on error.
 */

date_t date_add_period(date_t date, enum date_period unit, int period)
{
	int	day, month, year;

	if (date == NULL_DATE)
		return NULL_DATE;

	day = date_get_day_from_date(date);
	month = date_get_month_from_date(date);
	year = date_get_year_from_date(date);

	switch (unit) {
	case DATE_PERIOD_YEARS:

		/* Add or subtract whole years. No other processing is required. */

		year += period;
		break;

	case DATE_PERIOD_MONTHS:

		/* Add or subtract months. If the month ends up out of range,
		 * add or subtract years until it comes back into range again.
		 */

		month += period;

		while (month > date_months_in_year(year)) {
			year++;
			month -= date_months_in_year(year);
		}

		while (month <= 0) {
			year--;
			month += date_months_in_year(year);
		}
		break;

	case DATE_PERIOD_DAYS:

		/* Add or subtract days. If the days end up out of range
		 * for the current month, adjust the days and months as
		 * required. If this takes the months out of range, correct
		 * the years too.
		 */

		day += period;

		while (day > date_days_in_month(month, year)) {
			day -= date_days_in_month(month, year);
			month ++;

			if (month > date_months_in_year(year)) {
				year++;
				month -= date_months_in_year(year);
			}
		}

		while (day <= 0) {
			month --;
			day += date_days_in_month(month, year);

			if (month <= 0) {
				year--;
				month += date_months_in_year(year);
			}
		}
		break;

	case DATE_PERIOD_NONE:
		break;
	}

	/* Recompose the date from the component parts. */

	return date_combine_parts(day, month, year);
}


/**
 * Take a raw date (where the day can be in the range 1-31 regardless of the
 * month), and make it valid by either moving it forwards to the last valid
 * day in the month or pushing it back to the 1st of the following month.
 *
 * \param date			The raw date to be adjusted.
 * \param direction		The direction to adjust the date in.
 * \return			The adjusted date.
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

	if (direction == DATE_ADJUST_FORWARD && day > date_days_in_month(month, year)) {
		day = date_days_in_month(month, year);
	} else if (direction == DATE_ADJUST_BACKWARD && day > date_days_in_month(month, year)) {
		day = 1;
		month += 1;

		if (month > date_months_in_year(year)) {
			month = 1;
			year += 1;
		}
	} else if (direction == DATE_ADJUST_FORWARD && day < 1) {
		month -= 1;

		if (month < 1) {
			year -= 1;
			month = date_months_in_year(year);
		}

		day = date_days_in_month(month, year);
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
 * \param date			The raw date to be adjusted.
 * \param direction		The direction to adjust the date, or none.
 * \return			The adjusted date.
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
	weekday = date_day_of_week(result);

	/* While the weekend bit is set for the current weekday, move the
	 * date one day in the specified direction and try again.
	 */

	while (date_convert_day_to_days(weekday) & date_weekend_days) {
		shift += move;
		weekday += move;

		if (weekday > DATE_LAST_OS_DAY)
			weekday = DATE_FIRST_OS_DAY;

		if (weekday < DATE_FIRST_OS_DAY)
			weekday = DATE_LAST_OS_DAY;
	}

	/* If a shift is required, add the necessary days on to the base
	 * date.
	 */

	if (shift != 0)
		result = date_add_period(result, DATE_PERIOD_DAYS, shift);

	return result;
}


/**
 * Get the current system date.
 *
 * \return			The current system date.
 */

date_t date_today(void)
{
	oswordreadclock_utc_block	time;
	territory_ordinals		ordinals;


	time.op = oswordreadclock_OP_UTC;
	oswordreadclock_utc(&time);
	territory_convert_time_to_ordinals(territory_CURRENT, (const os_date_and_time *) &(time.utc), &ordinals);

	return date_combine_parts(ordinals.date, ordinals.month, ordinals.year);
}


/**
 * Find the number of days in a month in a given year. If the user has
 * configured to use the Territory Manager, this information will be taken
 * from the OS; otherwise it will be calculated directly.
 *
 * \param month			The month to return the day count for.
 * \param year			The year containing the month.
 * \return			The number of days in the given month.
 */

static int date_days_in_month(int month, int year)
{
	os_date_and_time	date;
	territory_ordinals	ordinals;
	territory_calendar	calendar;


	if (config_opt_read("Territory_dates")) {
		ordinals.centisecond = 0;
		ordinals.second = 0;
		ordinals.minute = 0;
		ordinals.hour = 12;
		ordinals.date = 1;
		ordinals.month = month;
		ordinals.year = year;

		territory_convert_ordinals_to_time(territory_CURRENT, &date, &ordinals);
		territory_read_calendar_information(territory_CURRENT, (const os_date_and_time *) &date, &calendar);
	} else {
		switch (month) {
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
	debug_printf("%d days in month %d (year %d)", calendar.day_count, month, year);
#endif

	return calendar.day_count;
}


/**
 * Find the number of months in a given year. If the user has configured to
 * use the Territory Manager, this information will be taken from the OS;
 * otherwise it will be calculated directly.
 *
 * \param year			The year to return the month count for.
 * \return			The number of months in the given year.
 */

static int date_months_in_year(int year)
{
	os_date_and_time	date;
	territory_ordinals	ordinals;
	territory_calendar	calendar;


	if (config_opt_read("Territory_dates")) {
		ordinals.centisecond = 0;
		ordinals.second = 0;
		ordinals.minute = 0;
		ordinals.hour = 12;
		ordinals.date = 1;
		ordinals.month = 1;
		ordinals.year = year;

		territory_convert_ordinals_to_time(territory_CURRENT, &date, &ordinals);
		territory_read_calendar_information(territory_CURRENT, (const os_date_and_time *) &date, &calendar);
	} else {
		calendar.month_count = 12;
	}

#ifdef DEBUG
	debug_printf("%d months in year %d", calendar.month_count, year);
#endif

	return calendar.month_count;
}


/**
 * Find the day of the week that a given date falls on, returning the day
 * in the form of an OS weekday value where 1 = Sunday -> 7 = Saturday.
 * If the user has configured to use the Territory Manager, this information
 * will be taken from the OS; otherwise it will be calculated directly.
 *
 * \param date			The date to find the weekday for.
 * \return			The weekday of the supplied date.
 */

static enum date_os_day date_day_of_week(date_t date)
{
	int			day, month, year;
	int			table[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	territory_ordinals	ordinals;
	os_date_and_time	time;

	if (date == NULL_DATE)
		return DATE_OS_DAY_NONE;

	if (config_opt_read("Territory_dates")) {
		ordinals.centisecond = 0;
		ordinals.second = 0;
		ordinals.minute = 0;
		ordinals.hour = 0;
		ordinals.date = date_get_day_from_date(date);
		ordinals.month = date_get_month_from_date(date);
		ordinals.year = date_get_year_from_date(date);
		
		territory_convert_ordinals_to_time(territory_CURRENT, &time, (territory_ordinals const *) &ordinals);
		territory_convert_time_to_ordinals(territory_CURRENT, (os_date_and_time const *) &time, &ordinals);
	} else {
		day = date_get_day_from_date(date);
		month = date_get_month_from_date(date);
		year = date_get_year_from_date(date);

		if (year < 1752 || month < 1 || month > 12)
			return DATE_OS_DAY_NONE;

		year -= month < 3;
		ordinals.weekday = ((year + year/4 - year/100 + year/400 + table[month - 1] + day) % 7) + 1;
	}

	return (enum date_os_day) ordinals.weekday;
}


/**
 * Test two dates to see if they encompass a full month.
 *
 * \param start			The start date.
 * \param end			The end date.
 * \return			TRUE if the dates are the first day and the
 *				last day of the same month; FALSE if not.
 */

osbool date_is_full_month(date_t start, date_t end)
{
	int	day1, month1, year1, day2, month2, year2;


	if (start == NULL_DATE || end == NULL_DATE)
		return FALSE;

	day1 = date_get_day_from_date(start);
	month1 = date_get_month_from_date(start);
	year1 = date_get_year_from_date(start);

	day2 = date_get_day_from_date(end);
	month2 = date_get_month_from_date(end);
	year2 = date_get_year_from_date(end);

	return (day1 == 1 && day2 == date_days_in_month(month2, year2) &&
			month1 == month2 && year1 == year2) ? TRUE : FALSE;
}


/**
 * Test two dates to see if they encompass a full year.
 *
 * \param start			The start date.
 * \param end			The end date.
 * \return			TRUE if the dates are the first day and the
 *				last day of the same year; FALSE if not.
 */

osbool date_is_full_year(date_t start, date_t end)
{
	int	day1, month1, year1, day2, month2, year2;


	if (start == NULL_DATE || end == NULL_DATE)
		return FALSE;

	day1 = date_get_day_from_date(start);
	month1 = date_get_month_from_date(start);
	year1 = date_get_year_from_date(start);

	day2 = date_get_day_from_date(end);
	month2 = date_get_month_from_date(end);
	year2 = date_get_year_from_date(end);

	return (day1 == 1 && day2 == date_days_in_month(month2, year2) &&
			month1 == 1 && month2 == date_months_in_year(year2) &&
			year1 == year2) ? TRUE : FALSE;
}


/**
 * Count the number of days (inclusive) between two dates.
 *
 * \param start			The start date.
 * \param end			The end date.
 * \return			The number of days (inclusive) between the
 *				start and end dates.
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

		days = date_days_in_month(month1, year1) - day1 + 1;

		month1++;

		if (month1 > date_months_in_year(year1)) {
			month1 = 1;
			year1++;
		}

		while ((year1 < year2 && month1 <= date_months_in_year(year1)) || (year1 == year2 && month1 < month2)) {
			days += date_days_in_month(month1, year1);

			month1++;

			if (month1 > date_months_in_year(year1)) {
				month1 = 1;
				year1++;
			}
		}

		days += day2;
	}

	return days;
}


/**
 * Test a string, to see if all its characters are numeric.
 *
 * \param *string		Pointer to the string to test.
 * \return			TRUE if all the characters are numeric;
 *				FALSE if not.
 */

static osbool date_is_string_numeric(char *string)
{
	if (string == NULL)
		return FALSE;

	while (*string != '\0' && isdigit(*string))
		string++;

	return (*string == '\0') ? TRUE : FALSE;
}

