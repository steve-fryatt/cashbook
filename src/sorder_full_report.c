/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: sorder_summary_report.c
 *
 * Standing Order Summary Report implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/msgs.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "sorder_full_report.h"

#include "account.h"
#include "currency.h"
#include "date.h"
#include "file.h"
#include "stringbuild.h"
#include "report.h"
#include "sorder.h"
#include "sorder_list_window.h"
#include "transact.h"


/* Buffers used in the standing order report. */

#define SORDER_REPORT_LINE_LENGTH 1024
#define SORDER_REPORT_BUF1_LENGTH 256
#define SORDER_REPORT_BUF2_LENGTH 32
#define SORDER_REPORT_BUF3_LENGTH 32


/**
 * Generate a report detailing all of the standing orders in a file.
 *
 * \param *file			The file to report on.
 */

void sorder_full_report(struct file_block *file)
{
	struct report		*report;
	int			i, sorder_count;
	sorder_t		sorder;
	amt_t			normal_amount, first_amount, last_amount;
	date_t			next_date;
	enum transact_flags	flags;
	char			line[SORDER_REPORT_LINE_LENGTH], numbuf1[SORDER_REPORT_BUF1_LENGTH], numbuf2[SORDER_REPORT_BUF2_LENGTH], numbuf3[SORDER_REPORT_BUF3_LENGTH];

	if (file == NULL || file->sorders == NULL)
		return;

	if (!stringbuild_initialise(line, SORDER_REPORT_LINE_LENGTH))
		return;

	msgs_lookup("SORWinT", line, SORDER_REPORT_LINE_LENGTH);
	report = report_open(file, line, NULL);

	if (report == NULL) {
		stringbuild_cancel();
		return;
	}

	hourglass_on();

	sorder_count = sorder_get_count(file);

	stringbuild_reset();
	stringbuild_add_message_param("SORTitle", file_get_leafname(file, NULL, 0), NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	stringbuild_reset();
	date_convert_to_string(date_today(), numbuf1, SORDER_REPORT_BUF1_LENGTH);
	stringbuild_add_message_param("SORHeader", numbuf1, NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	stringbuild_reset();
	string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", sorder_count);
	stringbuild_add_message_param("SORCount", numbuf1, NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	/* Output the data for each of the standing orders in turn. */

	for (i = 0; i < sorder_count; i++) {
		report_write_line(report, 0, ""); /* Separate each entry with a blank line. */

		sorder = sorder_get_sorder_from_line(file, i);

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", i + 1);
		stringbuild_add_message_param("SORNumber", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORFrom", account_get_name(file, sorder_get_from(file, sorder)), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORTo", account_get_name(file, sorder_get_to(file, sorder)), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORRef", sorder_get_reference(file, sorder, NULL, 0), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		normal_amount = sorder_get_amount(file, sorder, SORDER_AMOUNT_NORMAL);

		stringbuild_reset();
		currency_convert_to_string(normal_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORAmount", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		first_amount = sorder_get_amount(file, sorder, SORDER_AMOUNT_FIRST);

		if (normal_amount != first_amount) {
			stringbuild_reset();
			currency_convert_to_string(first_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
			stringbuild_add_message_param("SORFirst", numbuf1, NULL, NULL, NULL);
			stringbuild_report_line(report, 0);
		}

		last_amount = sorder_get_amount(file, sorder, SORDER_AMOUNT_LAST);

		if (normal_amount != last_amount) {
			stringbuild_reset();
			currency_convert_to_string(last_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
			stringbuild_add_message_param("SORFirst", numbuf1, NULL, NULL, NULL);
			stringbuild_report_line(report, 0);
		}

		stringbuild_reset();
		stringbuild_add_message_param("SORDesc", sorder_get_description(file, sorder, NULL, 0), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", sorder_get_transactions(file, sorder, SORDER_TRANSACTIONS_TOTAL));
		string_printf(numbuf2, SORDER_REPORT_BUF2_LENGTH, "%d", sorder_get_transactions(file, sorder, SORDER_TRANSACTIONS_DONE));
		string_printf(numbuf3, SORDER_REPORT_BUF3_LENGTH, "%d", sorder_get_transactions(file, sorder, SORDER_TRANSACTIONS_LEFT));
		stringbuild_add_message_param("SORCounts", numbuf1, numbuf2, numbuf3, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		date_convert_to_string(sorder_get_date(file, sorder, SORDER_DATE_START), numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORStart", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", sorder_get_period(file, sorder));
		switch (sorder_get_period_unit(file, sorder)) {
		case DATE_PERIOD_DAYS:
			msgs_lookup("SOrderDays", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		case DATE_PERIOD_MONTHS:
			msgs_lookup("SOrderMonths", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		case DATE_PERIOD_YEARS:
			msgs_lookup("SOrderYears", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		default:
			*numbuf2 = '\0';
			break;
		}
		stringbuild_add_message_param("SOREvery", numbuf1, numbuf2, NULL, NULL);
		stringbuild_report_line(report, 0);

		flags = sorder_get_flags(file, sorder);

		if (flags & TRANS_SKIP_FORWARD) {
			stringbuild_reset();
			stringbuild_add_message("SORAvoidFwd");
			stringbuild_report_line(report, 0);
		} else if (flags & TRANS_SKIP_BACKWARD) {
			stringbuild_reset();
			stringbuild_add_message("SORAvoidBack");
			stringbuild_report_line(report, 0);
		}

		next_date = sorder_get_date(file, sorder, SORDER_DATE_ADJUSTED_NEXT);

		stringbuild_reset();
		if (next_date != NULL_DATE)
			date_convert_to_string(next_date, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		else
			msgs_lookup("SOrderStopped", numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORNext", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);
	}

	/* Close the report. */

	stringbuild_cancel();

	report_close(report);

	hourglass_off();
}

