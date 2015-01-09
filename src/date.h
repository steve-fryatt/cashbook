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




/* ==================================================================================================================
 * Static constants
 */


/* ==================================================================================================================
 * Data structures
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

enum date_period {
	DATE_PERIOD_NONE = 0,							/**< No period specified.					*/
	DATE_PERIOD_DAYS,							/**< Period specified in days.					*/
	DATE_PERIOD_MONTHS,							/**< Period specified in months.				*/
	DATE_PERIOD_YEARS							/**< Period specified in years.					*/
};

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




date_t convert_string_to_date (char *string, date_t previous_date, int month_days);

date_t get_current_date (void);

date_t add_to_date (date_t date, int unit, int add);
int days_in_month (int month, int year);
int months_in_year (int year);
int day_of_week (unsigned int date);
int full_month (date_t start, date_t end);
int full_year (date_t start, date_t end);

/**
 * Count the number of days (inclusive) between two dates.
 *
 * \param start		The start date.
 * \param end		The end date.
 * \return		The number of days (inclusive) between the start
 *			and end dates.
 */

int date_count_days(date_t start, date_t end);


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

#endif
