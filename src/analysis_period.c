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
 * \file: analysis_period.c
 *
 * Analysis Date Period implementation.
 */

/* ANSI C header files */

#include <stdlib.h>

/* OSLib header files */

#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/msgs.h"

/* Application header files */

#include "analysis_period.h"

#include "date.h"

/* Date period management. */

/**
 * The start date of the current reporting period.
 */

static date_t			analysis_period_start;

/**
 * The end date of the current reporting period.
 */

static date_t			analysis_period_end;

/**
 * The length of the current reporting period, in the given units.
 */

static int			analysis_period_length;

/**
 * The units being used for the length of the current reporting period.
 */

static enum date_period		analysis_period_unit;

/**
 * True to apply calendar lock to the reporting period.
 */

static osbool			analysis_period_lock;

/**
 * True if this is the first period in a locked iteration.
 */

static osbool			analysis_period_first = TRUE;


/**
 * Initialise the date period iteration.  Set the state machine so that
 * analysis_get_next_date_period() can be called to work through the report.
 *
 * \param start			The start date for the report period.
 * \param end			The end date for the report period.
 * \param period		The time period into which to divide the report.
 * \param unit			The unit of the divisor period.
 * \param lock			TRUE to apply calendar lock; otherwise FALSE.
 */

void analysis_period_initialise(date_t start, date_t end, int period, enum date_period unit, osbool lock)
{
	analysis_period_start = start;
	analysis_period_end = end;
	analysis_period_length = period;
	analysis_period_unit = unit;
	analysis_period_lock = lock;
	analysis_period_first = lock;
}


/**
 * Return the next date period from the sequence set up with
 * analysis_period_initialise(), for use by the report modules.
 *
 * \param *next_start		Return the next period's start date.
 * \param *next_end		Return the next period's end date.
 * \param *date_text		Pointer to a buffer to hold a textual name for the period.
 * \param date_len		The number of bytes in the name buffer.
 * \return			TRUE if a period was returned; FALSE if none left.
 */

osbool analysis_period_get_next_dates(date_t *next_start, date_t *next_end, char *date_text, size_t date_len)
{
	char		b1[1024], b2[1024];

	if (analysis_period_start > analysis_period_end)
		return FALSE;

	if (analysis_period_length > 0) {
		/* If the report is to be grouped, find the next_end date which falls at the end of the period.
		 *
		 * If first_lock is set, the report is locked to the calendar and this is the first iteration.  Therefore,
		 * the end date is found by adding the period-1 to the current date, then setting the DAYS or DAYS+MONTHS to
		 * maximum in the result.  This means that the first period will be no more than the specified period.
		 * The resulting date will later be fixed into a valid date, before it is used in anger.
		 *
		 * If first_lock is not set, the next_end is found by adding the group period to the start date and subtracting
		 * 1 from it.  By this point, locked reports will be period aligned anyway, so this should work OK.
		 */

		if (analysis_period_first) {
			*next_end = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length - 1);

			switch (analysis_period_unit) {
			case DATE_PERIOD_MONTHS:
				*next_end = (*next_end & 0xffffff00) | 0x001f; /* Maximise the days, so end of month. */
				break;

			case DATE_PERIOD_YEARS:
				*next_end = (*next_end & 0xffff0000) | 0x0c1f; /* Maximise the days and months, so end of year. */
				break;

			default:
				*next_end = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length) - 1;
				break;
			}
		} else {
			*next_end = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length) - 1;
		}

		/* Pull back into range isf we fall off the end. */

		if (*next_end > analysis_period_end)
			*next_end = analysis_period_end;
	} else {
		/* If the report is not to be grouped, the next_end date is just the end of the report period. */

		*next_end = analysis_period_end;
	}

	/* Get the real start and end dates for the period. */

	*next_start = date_find_valid_day(analysis_period_start, DATE_ADJUST_BACKWARD);
	*next_end = date_find_valid_day(*next_end, DATE_ADJUST_FORWARD);

	if (analysis_period_length > 0) {
		/* If the report is grouped, find the next start date by adding the period on to the current start date. */

		analysis_period_start = date_add_period(analysis_period_start, analysis_period_unit, analysis_period_length);

		if (analysis_period_first) {
			/* If the report is calendar locked, and this is the first iteration, reset the DAYS or DAYS+MONTHS
			 * to one so that the start date will be locked on to the calendar from now on.
			 */

			switch (analysis_period_unit) {
			case DATE_PERIOD_MONTHS:
				analysis_period_start = (analysis_period_start & 0xffffff00) | 0x0001; /* Set the days to one. */
				break;

			case DATE_PERIOD_YEARS:
				analysis_period_start = (analysis_period_start & 0xffff0000) | 0x0101; /* Set the days and months to one. */
				break;

			default:
				break;
			}

			analysis_period_first = FALSE;
		}
	} else {
		analysis_period_start = analysis_period_end+1;
	}

	/* Generate a date period title for the report section.
	 *
	 * If calendar locked, this will be of the form "June 2003", or "1998"; otherwise it will be of the form
	 * "<start date> - <end date>".
	 */

	*date_text = '\0';

	if (analysis_period_lock) {
		switch (analysis_period_unit) {
		case DATE_PERIOD_MONTHS:
			date_convert_to_month_string(*next_start, b1, sizeof(b1));

			if ((*next_start & 0xffffff00) == (*next_end & 0xffffff00)) {
				msgs_param_lookup("PRMonth", date_text, date_len, b1, NULL, NULL, NULL);
			} else {
				date_convert_to_month_string(*next_end, b2, sizeof(b2));
				msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
			}
			break;

		case DATE_PERIOD_YEARS:
			date_convert_to_year_string(*next_start, b1, sizeof(b1));

			if ((*next_start & 0xffff0000) == (*next_end & 0xffff0000)) {
				msgs_param_lookup("PRYear", date_text, date_len, b1, NULL, NULL, NULL);
			} else {
				date_convert_to_year_string(*next_end, b2, sizeof(b2));
				msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
			}
			break;

		default:
			break;
		}
	} else {
		if (*next_start == *next_end) {
			date_convert_to_string(*next_start, b1, sizeof(b1));
			msgs_param_lookup("PRDay", date_text, date_len, b1, NULL, NULL, NULL);
		} else {
			date_convert_to_string(*next_start, b1, sizeof(b1));
			date_convert_to_string(*next_end, b2, sizeof(b2));
			msgs_param_lookup("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
		}
	}

	return TRUE;
}
