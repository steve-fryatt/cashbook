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
 * \file: date.h
 *
 * Date implementation.
 */

#ifndef CASHBOOK_DATE
#define CASHBOOK_DATE

/**
 * A date.
 */

typedef unsigned int	date_t;

/**
 * Invalid or missing date value.
 */

#define NULL_DATE ((date_t) 0xffffffff)

/**
 * The minimum valid date.
 */

#define DATE_MIN ((date_t) 0x00640101)

/**
 * The maximum valid date.
 */

#define DATE_MAX ((date_t) 0x270f0c1f)

/**
 * Days of the week in the form of a bitfield, which can be added together
 * to represent groups of days.
 *
 * The bitfield values need to correspond to the entries in enum date_os_day.
 */

enum date_days {
	DATE_DAY_NONE = 0x00,
	DATE_DAY_SUNDAY = 0x01,
	DATE_DAY_MONDAY = 0x02,
	DATE_DAY_TUESDAY = 0x04,
	DATE_DAY_WEDNESDAY = 0x08,
	DATE_DAY_THURSDAY = 0x10,
	DATE_DAY_FRIDAY = 0x20,
	DATE_DAY_SATURDAY = 0x40
};

/**
 * Convert an enum date_os_day into an enum date_days, so that the day
 * number is converted into the corresponding bit in the bitfield.
 */

#define date_convert_day_to_days(day) (1 << ((day) - 1))

/**
 * Represent the units of numerical date periods.
 */

enum date_period {
	DATE_PERIOD_NONE = 0,							/**< No period specified.					*/
	DATE_PERIOD_DAYS,							/**< Period specified in days.					*/
	DATE_PERIOD_MONTHS,							/**< Period specified in months.				*/
	DATE_PERIOD_YEARS							/**< Period specified in years.					*/
};

/**
 * Represent the direction of date adjustments. These are given in the sense
 * of "moving a date forward" to make it occur sooner, and "putting a date
 * back" to make it happen later.
 */

enum date_adjust {
	DATE_ADJUST_NONE = 0,							/**< Make no adjustment to the date.				*/
	DATE_ADJUST_FORWARD,							/**< Adjust the date by pulling it earlier in the calendar.	*/
	DATE_ADJUST_BACKWARD							/**< Adjust the date by pushing it later in the calendar.	*/
};

/* ==================================================================================================================
 * Function prototypes.
 */

/**
 * Initialise, or re-initialise, the date module.
 */

void date_initialise(void);


/**
 * Convert a date into a string, placing the result into the supplied buffer.
 *
 * \param date			The date to be converted.
 * \param *buffer		Pointer to a buffer to take the conversion.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			A pointer to the supplied buffer, or NULL if
 *				the buffer's details were invalid.
 */

char *date_convert_to_string(date_t date, char *buffer, size_t length);


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

char *date_convert_to_month_string(date_t date, char *buffer, size_t length);


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

char *date_convert_to_year_string(date_t date, char *buffer, size_t length);


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

date_t date_convert_from_string(char *string, date_t base_date, int month_days);


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

date_t date_add_period(date_t date, enum date_period unit, int period);


/**
 * Take a raw date (where the day can be in the range 1-31 regardless of the
 * month), and make it valid by either moving it forwards to the last valid
 * day in the month or pushing it back to the 1st of the following month.
 *
 * \param date		The raw date to be adjusted.
 * \param direction	The direction to adjust the date in.
 * \return		The adjusted date.
 */

date_t date_find_valid_day(date_t date, enum date_adjust direction);


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

date_t date_find_working_day(date_t date, enum date_adjust direction);


/**
 * Get the current system date.
 *
 * \return			The current system date.
 */

date_t date_today(void);


/**
 * Test two dates to see if they encompass a full month.
 *
 * \param start			The start date.
 * \param end			The end date.
 * \return			TRUE if the dates are the first day and the
 *				last day of the same month; FALSE if not.
 */

osbool date_is_full_month(date_t start, date_t end);


/**
 * Test two dates to see if they encompass a full year.
 *
 * \param start			The start date.
 * \param end			The end date.
 * \return			TRUE if the dates are the first day and the
 *				last day of the same year; FALSE if not.
 */

osbool date_is_full_year(date_t start, date_t end);


/**
 * Count the number of days (inclusive) between two dates.
 *
 * \param start		The start date.
 * \param end		The end date.
 * \return		The number of days (inclusive) between the start
 *			and end dates.
 */

int date_count_days(date_t start, date_t end);

#endif
