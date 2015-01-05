/* Copyright 2003-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "transact.h"

/* ==================================================================================================================
 * Static constants
 */

#define DATE_SEP_LENGTH 11

/* ==================================================================================================================
 * Data structures
 */

enum date_period {
	PERIOD_NONE = 0,							/**< No period specified.				*/
	PERIOD_DAYS,								/**< Period specified in days.				*/
	PERIOD_MONTHS,								/**< Period specified in months.			*/
	PERIOD_YEARS								/**< Period specified in years.				*/
};

/* ==================================================================================================================
 * Function prototypes.
 */
/* Date conversion. */

void convert_date_to_string (date_t date, char *string);
date_t convert_string_to_date (char *string, date_t previous_date, int month_days);
void convert_date_to_month_string (date_t date, char *string);
void convert_date_to_year_string (date_t date, char *string);

date_t get_current_date (void);

date_t add_to_date (date_t date, int unit, int add);
int days_in_month (int month, int year);
int months_in_year (int year);
int day_of_week (unsigned int date);
int full_month (date_t start, date_t end);
int full_year (date_t start, date_t end);
int count_days (date_t start, date_t end);

date_t get_valid_date (date_t date, int direction);
date_t get_sorder_date (date_t date, enum transact_flags flags);

void set_weekend_days (void);
int read_weekend_days (void);

int is_numeric (char *str);

#endif
