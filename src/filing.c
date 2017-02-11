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
 * \file: filing.c
 *
 * Legacy file load and save routines.
 *
 * File Format 1.00
 * - Original format.
 *
 * File Format 1.01
 * - Add Row column to transaction and account view windows.
 *
 */

/* ANSI C header files */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/string.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "filing.h"

#include "account.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "interest.h"
#include "presets.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"
#include "window.h"

/* ==================================================================================================================
 * Global Variables
 */

/**
 * The current CashBook file format version.
 */

#define FILING_CURRENT_FORMAT 101

/**
 * The maximum Import CSV file line length.
 */

#define FILING_CSV_LINE_LENGTH 1024

/**
 * The maximum log output line length.
 */

#define FILING_LOG_LINE_LENGTH 1024

/**
 * The size of a temporary text buffer.
 */

#define FILING_TEMP_BUF_LENGTH 64


/**
 * Icons in the import report dialogue.
 */

#define FILING_ICON_IMPORTED 0
#define FILING_ICON_REJECTED 2
#define FILING_ICON_CLOSE 5
#define FILING_ICON_LOG 4

/**
 * The file load and save handle structure.
 */

struct filing_block {
	FILE			*handle;				/**< The handle of the input or output file.		*/
	char			section[FILING_MAX_FILE_LINE_LEN];
	char			*suffix;
	char			token[FILING_MAX_FILE_LINE_LEN];
	char			value[FILING_MAX_FILE_LINE_LEN];
	char			*field;
	int			format;
	enum config_read_status	result;
	enum filing_status	status;
};

/* Global Variables. */

/**
 * The handle of the Import Complete dialogue box.
 */

static wimp_w			filing_import_window = NULL;

/**
 * The file instance to which the Import Complete dialogue box belongs, or NULL.
 */

static struct file_block	*import_window_file = NULL;

/* Static Function Prototypes. */

static void		filing_import_complete_click_handler(wimp_pointer *pointer);
static void 		filing_close_import_complete(osbool show_log);
static char		*filing_find_next_field(struct filing_block *in);


/**
 * Initialise the filing system.
 */

void filing_initialise(void)
{
	filing_import_window = templates_create_window("ImpComp");
	ihelp_add_window(filing_import_window, "ImpComp", NULL);
	event_add_window_mouse_event(filing_import_window, filing_import_complete_click_handler);
}


/**
 * Load a CashBook file into memory, creating a new file instance and opening
 * a transaction window to display the contents.
 *
 * \param *filename		Pointer to the name of the file to be loaded.
 */

void filing_load_cashbook_file(char *filename)
{
	bits			load;
	struct file_block	*file;
	struct filing_block	in;

	#ifdef DEBUG
	debug_printf("\\BLoading accounts file");
	#endif

	file = build_new_file_block();

	if (file == NULL) {
		error_msgs_report_error("NoMemForLoad");
		return;
	}

	in.handle = fopen(filename, "r");

	if (in.handle == NULL) {
		delete_file(file);
		error_msgs_report_error("FileLoadFail");
		return;
	}

	hourglass_on();

	*in.section = '\0';
	*in.token = '\0';
	*in.value = '\0';
	in.field = in.section;
	in.suffix = NULL;

	in.format = 0;
	in.status = FILING_STATUS_OK;
	in.result = sf_CONFIG_READ_EOF;

	do {
		if (string_nocase_strcmp(in.section, "Budget") == 0)
			budget_read_file(file, &in);
		else if (string_nocase_strcmp(in.section, "Accounts") == 0)
			account_read_acct_file(file, &in);
		else if (string_nocase_strcmp(in.section, "AccountList") == 0)
			account_read_list_file(file, &in);
		else if (string_nocase_strcmp(in.section, "Interest") == 0)
			interest_read_file(file, &in);
		else if (string_nocase_strcmp(in.section, "Transactions") == 0)
			transact_read_file(file, &in);
		else if (string_nocase_strcmp(in.section, "StandingOrders") == 0)
			sorder_read_file(file, &in);
		else if (string_nocase_strcmp(in.section, "Presets") == 0)
			preset_read_file(file, &in);
		else if (string_nocase_strcmp(in.section, "Reports") == 0)
			analysis_read_file(file, &in);
		else {
			do {
				if (*in.section != '\0')
					in.status = FILING_STATUS_UNEXPECTED;

				/* Load in the file format, converting an n.nn number into an
				 * integer value (eg. 1.00 would become 100).  Supports 0.00 to 9.99.
				 */

				if (string_nocase_strcmp(in.token, "Format") == 0) {
					if (strlen(in.value) == 4 && isdigit(in.value[0]) && isdigit(in.value[2]) && isdigit(in.value[3]) && in.value[1] == '.') {
						in.value[1] = in.value[2];
						in.value[2] = in.value[3];
						in.value[3] = '\0';

						in.format = atoi(in.value);

						if (in.format > FILING_CURRENT_FORMAT)
							in.status = FILING_STATUS_VERSION;
					} else {
						in.status = FILING_STATUS_UNEXPECTED;
					}
				}

				in.result = config_read_token_pair(in.handle, in.token, in.value, in.section);
			} while ((in.status == FILING_STATUS_OK || in.status == FILING_STATUS_UNEXPECTED) && in.result != sf_CONFIG_READ_EOF && in.result != sf_CONFIG_READ_NEW_SECTION);
		}
	} while ((in.status == FILING_STATUS_OK || in.status == FILING_STATUS_UNEXPECTED) && in.result != sf_CONFIG_READ_EOF);

	fclose(in.handle);

	/* If the file format wasn't understood, get out now. */

	if (in.status == FILING_STATUS_VERSION || in.status == FILING_STATUS_MEMORY) {
		delete_file(file);
		hourglass_off();
		switch (in.status) {
		case FILING_STATUS_VERSION:
			error_msgs_report_error("UnknownFileFormat");
			break;
		case FILING_STATUS_MEMORY:
			error_msgs_report_error("NoMemNewFile");
			break;
		case FILING_STATUS_BAD_MEMORY:
			error_msgs_report_error("BadMemory");
			break;
		case FILING_STATUS_CORRUPT:
			error_msgs_report_error("CorruptFile");
			break;
		case FILING_STATUS_OK:
		case FILING_STATUS_UNEXPECTED:
			break;
		}
		return;
	}

	/* Get the datestamp of the file. */

	osfile_read_stamped(filename, &load, (bits *) file->datestamp, NULL, NULL, NULL);
	file->datestamp[4] = load & 0xff;

	/* Tidy up, create the transaction window and open it up. */

	strcpy(file->filename, filename);
	sorder_process(file);
	transact_sort_file_data(file);
	account_recalculate_all(file);
	transact_sort(file->transacts);
	sorder_sort(file->sorders);
	preset_sort(file->presets);
	transact_open_window(file); /* The window extent is set in this action. */

	hourglass_off();

	if (in.status == FILING_STATUS_UNEXPECTED)
		error_msgs_report_info("UnknownFileData");
}


/**
 * Save the data associated with a file block back to disc.
 *
 * \param *file			The file instance to be saved.
 * \param *filename		Pointer to the name of the file to save to.
 */

void filing_save_cashbook_file(struct file_block *file, char *filename)
{
	FILE	*out;
	bits	load;


	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Strip unused blank lines from the end of the file. */

	transact_strip_blanks_from_end(file);

	/* Output the file header. */

	fprintf(out, "# CashBook file\n");
	fprintf(out, "# Written by CashBook\n\n");

	fprintf(out, "Format: %1.2f\n", ((double) FILING_CURRENT_FORMAT) / 100.0);

	budget_write_file(file, out);
	account_write_file(file, out);
	interest_write_file(file, out);
	transact_write_file(file, out);
	sorder_write_file(file, out);
	preset_write_file(file, out);
	analysis_write_file(file, out);

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) dataxfer_TYPE_CASHBOOK);

	/* Get the datestamp of the file. */

	osfile_read_stamped(filename, &load, (bits *) file->datestamp, NULL, NULL, NULL);
	file->datestamp[4] = load & 0xff;

	/* Update the modified flag and filename for the file block and refresh the window title. */

	file_set_data_integrity(file, FALSE);
	strcpy(file->filename, filename);

	transact_build_window_title(file);
	account_build_window_titles(file);
	sorder_build_window_title(file);
	preset_build_window_title(file);
	interest_build_window_title(file);

	hourglass_off();
}


/**
 * Import the contents of a CSV file into an existing file instance.
 *
 * \param *file			The file instance to take the CSV data.
 * \param *filename		Pointer to the name of the CSV file to process.
 */

void filing_import_csv_file(struct file_block *file, char *filename)
{
  FILE         *input;
  char         line[FILING_CSV_LINE_LENGTH], log[FILING_LOG_LINE_LENGTH], b1[FILING_TEMP_BUF_LENGTH], b2[FILING_TEMP_BUF_LENGTH],
               *date, *ref, *amount, *description, *ident, *name, *raw_from, *raw_to;
  int          from, to, import_count, reject_count;
	osbool		error;
  wimp_pointer pointer;
  unsigned int type;
  enum transact_flags rec_from, rec_to;


	import_window_file = file;

	import_count = 0;
	reject_count = 0;

	hourglass_on();

	/* If there's an existing log, delete it. */

	if (file->import_report != NULL) {
		report_delete(file->import_report);
		file->import_report = NULL;
	}

	if (windows_get_open(filing_import_window))
		wimp_close_window(filing_import_window);

	/* Open a log report for the process, and title it. */

	msgs_lookup("IRWinT", log, FILING_LOG_LINE_LENGTH);
	file->import_report = report_open(file, log, NULL);

	file_get_leafname(file, line, FILING_CSV_LINE_LENGTH);
	msgs_param_lookup("IRTitle", log, FILING_LOG_LINE_LENGTH, line, NULL, NULL, NULL);
	report_write_line(file->import_report, 0, log);
	msgs_param_lookup("IRImpFile", log, FILING_LOG_LINE_LENGTH, filename, NULL, NULL, NULL);
	report_write_line(file->import_report, 0, log);

	report_write_line(file->import_report, 0, "");

	msgs_lookup("IRHeadings", log, FILING_LOG_LINE_LENGTH);
	report_write_line(file->import_report, 0, log);

	input = fopen(filename, "r");

	if (input != NULL) {
		while (fgets(line, FILING_CSV_LINE_LENGTH, input) != NULL) {
			error = FALSE;

			/* Date */

			date = filing_read_delimited_field(line, DELIMIT_COMMA, DELIMIT_NONE);

			if (date_convert_from_string(date, NULL_DATE, 0) == NULL_DATE)
				error = TRUE;

			/* From */

			ident = filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);

			raw_from = ident;

			rec_from = (strchr(ident, '#') > 0) ? TRANS_REC_FROM : TRANS_FLAGS_NONE;

			name = ident + strcspn(ident, "#:");
			*name++ = '\0';
			while (strchr("#:", *name))
				name++;

			if (*ident == '\0') {
				error = TRUE;
				from = NULL_ACCOUNT;
			} else {
				type = isdigit(*ident) ? ACCOUNT_FULL : ACCOUNT_IN;
				from = account_find_by_ident(file, ident, type);

				if (from == -1)
					from = account_add(file, name, ident, type);
			}

			/* To */

			ident = filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);

			raw_to = ident;

			rec_to = (strchr(ident, '#') > 0) ? TRANS_REC_TO : TRANS_FLAGS_NONE;

			name = ident + strcspn(ident, "#:");
			*name++ = '\0';
			while (strchr("#:", *name))
				name++;

			if (*ident == '\0') {
				error = TRUE;
				to = NULL_ACCOUNT;
			} else {
				type = isdigit(*ident) ? ACCOUNT_FULL : ACCOUNT_OUT;
				to = account_find_by_ident(file, ident, type);

				if (to == -1)
					to = account_add(file, name, ident, type);
			}

			/* Ref */

			ref = filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);

			/* Amount */

			amount = filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);

			if (*amount == '\0')
				amount = filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);
			else
				filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);

			/* Skip Balance */

			filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);

			/* Description */

			description = filing_read_delimited_field(NULL, DELIMIT_COMMA, DELIMIT_NONE);

			/* Create a new transaction. */

			if (error == TRUE) {
				msgs_lookup("Rejected", b1, FILING_TEMP_BUF_LENGTH);
				reject_count++;
			} else {
				transact_add_raw_entry(file, date_convert_from_string(date, NULL_DATE, 0), from, to, rec_from | rec_to,
						currency_convert_from_string(amount), ref, description);
				msgs_lookup("Imported", b1, FILING_TEMP_BUF_LENGTH);

				import_count++;
			}

			snprintf(log, FILING_LOG_LINE_LENGTH, "%s\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'",
					b1, date, raw_from, raw_to, ref, amount, description);
			log[FILING_LOG_LINE_LENGTH - 1] = '\0';
			report_write_line(file->import_report, 0, log);
		}

		fclose(input);

		transact_set_window_extent(file);
		transact_sort_file_data(file);
		sorder_trial(file);
		account_recalculate_all(file);
		accview_rebuild_all(file);
		file_set_data_integrity(file, TRUE);

		transact_redraw_all(file);
	}

	/* Sort out the import results window. */

	report_write_line(file->import_report, 0, "");

	snprintf(b1, FILING_TEMP_BUF_LENGTH, "%d", import_count);
	b1[FILING_TEMP_BUF_LENGTH - 1] = '\0';

	snprintf(b2, FILING_TEMP_BUF_LENGTH, "%d", reject_count);
	b2[FILING_TEMP_BUF_LENGTH - 1] = '\0';

	msgs_param_lookup("IRTotals", log, FILING_LOG_LINE_LENGTH, b1, b2, NULL, NULL);
	report_write_line(file->import_report, 0, log);

	icons_printf(filing_import_window, FILING_ICON_IMPORTED, "%d", import_count);
	icons_printf(filing_import_window, FILING_ICON_REJECTED, "%d", reject_count);

	wimp_get_pointer_info(&pointer);
	windows_open_centred_at_pointer(filing_import_window, &pointer);

	hourglass_off();
}


/**
 * Process mouse clicks in the Import Complete dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void filing_import_complete_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case FILING_ICON_CLOSE:
		filing_close_import_complete(FALSE);
		break;

	case FILING_ICON_LOG:
		filing_close_import_complete(TRUE);
		break;
	}
}


/**
 * Close the import results dialogue, either opening or deleting the log report.
 * Once the window is closed, we no longer need to track the report, so the
 * pointer can be set to NULL.
 *
 * \param show_log		TRUE to open the import log; FALSE to dispose
 *				of it.
 */

static void filing_close_import_complete(osbool show_log)
{
	if (show_log)
		report_close(import_window_file->import_report);
	else
		report_delete(import_window_file->import_report);

	wimp_close_window(filing_import_window);
	import_window_file->import_report = NULL;
}


/**
 * Force the closure of the Import windows if the owning file disappears.
 * There's no need to delete any associated report, because it will be handled
 * via the Report module when the file disappears.
 *
 * \param *file			The file which has closed.
 */

void filing_force_windows_closed(struct file_block *file)
{
	if (import_window_file == file && windows_get_open(filing_import_window))
		wimp_close_window(filing_import_window);
}


/**
 * Output a text string to a file, treating it as a field in a delimited format
 * and applying the necessary quoting as required.
 *
 * \param *f			The file handle to write to.
 * \param *string		The string to write.
 * \param format		The file format to be written.
 * \param flags			Flags indicating addtional formatting to apply.
 * \return			0 on success.
 */

int filing_output_delimited_field(FILE *f, char *string, enum filing_delimit_type format, enum filing_delimit_flags flags)
{
	int	i;
	osbool	quote = FALSE;

	/* Decide whether to enclose in quotes. */

	switch (format) {
	case DELIMIT_TAB:		/* Never quote. */
		quote = FALSE;
		break;

	case DELIMIT_COMMA:		/* Only quote if leading whitespace, trailing whitespace, or enclosed comma. */
		if (isspace(string[0]) || isspace(string[strlen(string)-1]))
			quote = TRUE;

		for (i = 0; string[i] != 0 && quote == 0; i++)
			if (string[i] == ',')
				quote = TRUE;
		break;

	case DELIMIT_QUOTED_COMMA:	/* Always quote. */
		quote = TRUE;
		break;
	}

	if (flags & DELIMIT_NUM)	/* Exception: numbers are never quoted. */
		quote = FALSE;

	/* Output the string. */

	if (quote)
		fprintf(f, "\"%s\"", string);
	else
		fprintf(f, "%s", string);

	/* Output the field separator. */

	if (flags & DELIMIT_LAST)
		fprintf(f, "\n");
	else if (format == DELIMIT_COMMA || format == DELIMIT_QUOTED_COMMA)
		fprintf(f, ",");
	else if (format == DELIMIT_TAB)
		fprintf(f, "\t");

	return 0;
}


/**
 * Read a field from a line of text in memory, treating it as a field in
 * delimited format and processing any quoting as necessary.
 * 
 * NB: This operation modifies the original data in memory.
 * 
 * \param *line			A line from the file to be processed, or NULL to continue
 *				on the same line as before.
 * \param format		The field format to be read.
 * \param flags			Flags indicating addtional formatting to apply.
 * \return			Pointer to the processed field contents, or NULL.
 */

char *filing_read_delimited_field(char *line, enum filing_delimit_type format, enum filing_delimit_flags flags)
{
	static char	*ptr = NULL;
	char		*field, *p1, *p2;
	char		separator;
	osbool		allow_quotes, quoted, found_quotes;
	osbool		remove_whitespace = FALSE;

	/* Initialise the line pointer if a new line has been supplied. */

	if (line != NULL)
		ptr = line;

	/* If there's no line pointer, exit immediately. */

	if (ptr == NULL)
		return NULL;

	/* Identify the field separator and any associated formatting. */

	switch (format) {
	case DELIMIT_COMMA:
	case DELIMIT_QUOTED_COMMA:
		separator = ',';
		allow_quotes = TRUE;
		remove_whitespace = TRUE;
		break;
	case DELIMIT_TAB:
		separator = '\t';
		allow_quotes = FALSE;
		remove_whitespace = FALSE;
		break;
	}

	quoted = FALSE;
	found_quotes = FALSE;

	/* If the field doesn't support leading whitespace, strip any off from the start. */

	if (remove_whitespace == TRUE) {
		while (*ptr != '\0' && isspace(*ptr))
			ptr++;
	}

	/* Scan through the line, looking for the end of the field while ignoring separators in quoted blocks. */

	field = ptr;

	while (*ptr != '\0' && ((*ptr != separator) || quoted)) {
		if (allow_quotes && *ptr == '"') {
			found_quotes = TRUE;
			quoted = !quoted;
		}

		ptr++;
	}

	/* If the field doesn't support trailing whitespace, strip any off from the end. */

	if (remove_whitespace == TRUE) {
		p1 = ptr;

		while (p1 > field && (p1 == '\0' || isspace(*p1)))
			p1--;

		if (p1 != ptr)
			*p1 = '\0';
	}

	/* Terminate the field at the separator, if it wasn't the last field in the line. */

	if (*ptr == separator) {
		*ptr = '\0';
		ptr++;
	}

	/* If quotes were found, strip them out. */

	if (found_quotes) {
		p1 = field;
		p2 = field;

		while (*p1 != '\0') {
			if (*p1 == '"') {
				if (*(p1 + 1) == '"') {
					/* Replace double quotes with singles. */
					p1 += 2;
					*p2++ = '"';
				} else {
					/* Step past individual quotes. */
					p1++;
				}
			} else {
				/* Copy non-quotes as they are. */
				*p2++ = *p1++;
			}
		}

		*p2 = '\0';
	}

	return field;
}


/* ==================================================================================================================
 * String processing.
 */

/* ------------------------------------------------------------------------------------------------------------------ */

char *next_field (char *line, char sep)
{
  static char *current;
  char        *start;
  int         quoted;

  if (line != NULL)
  {
    current = line;
  }

  quoted = 0;
  start = current;

  while (*current != '\0' && ((*current != sep) || quoted))
  {
    if (*current == '"')
    {
      quoted = !quoted;
    }
    current++;
  }

  if (*current == sep)
  {
    *current = '\0';
    current ++;
  }

  return (start);
}
/* ------------------------------------------------------------------------------------------------------------------ */

char *next_plain_field (char *line, char sep)
{
  static char *current;
  char        *start;

  if (line != NULL)
  {
    current = string_strip_surrounding_whitespace (line);
  }

  start = current;

  while (*current != '\0' && *current != sep)
  {
    current++;
  }

  if (*current == sep)
  {
    *current = '\0';
    current ++;
  }

  return (start);
}



int filing_get_format(struct filing_block *in)
{
	if (in == NULL)
		return 0;

	return in->format;
}

osbool filing_get_next_token(struct filing_block *in)
{
	char *separator;

	if (in == NULL)
		return FALSE;

	in->result = config_read_token_pair(in->handle, in->token, in->value, in->section);

	separator = strchr(in->section, ':');
	if (separator != NULL) {
		*separator = '\0';
		in->suffix = separator + 1;
	} else {
		in->suffix = NULL;
	}

	in->field = in->value;

	return (in->result != sf_CONFIG_READ_EOF && in->result != sf_CONFIG_READ_NEW_SECTION) ? TRUE : FALSE;
}


enum account_type filing_get_account_type_suffix(struct filing_block *in)
{
	if (in == NULL || in->suffix == NULL)
		return ACCOUNT_NULL;

	return strtoul(in->suffix, NULL, 16);
}

osbool filing_test_token(struct filing_block *in, char *token)
{
	if (in == NULL)
		return FALSE;

	return (string_nocase_strcmp(in->token, token) == 0) ? TRUE : FALSE;
}

char *filing_get_text_value(struct filing_block *in, char *buffer, size_t length)
{
	if (in == NULL)
		return NULL;

	if (buffer == NULL)
		return in->value;

	if (length == 0) {
		in->status = FILING_STATUS_BAD_MEMORY;
		return NULL;
	}

	buffer[length - 1] = '\0';
	strncpy(buffer, in->value, length);

	if (buffer[length - 1] != '\0') {
		in->status = FILING_STATUS_CORRUPT;
		buffer[length - 1] = '\0';
	}

	return buffer;
}

int filing_get_int_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return 0;
	}

	return strtoul(field, NULL, 16);
}

unsigned filing_get_unsigned_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return 0;
	}

	return strtoul(field, NULL, 16);
}

char filing_get_char_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return 0;
	}

	return strtoul(field, NULL, 16);
}

osbool filing_get_opt_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return FALSE;
	}

	return config_read_opt_string(field);
}

char *filing_get_text_field(struct filing_block *in, char *buffer, size_t length)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return NULL;
	}

	if (buffer == NULL)
		return field;

	if (length == 0) {
		in->status = FILING_STATUS_BAD_MEMORY;
		return NULL;
	}

	buffer[length - 1] = '\0';
	strncpy(buffer, field, length);

	if (buffer[length - 1] != '\0') {
		in->status = FILING_STATUS_CORRUPT;
		buffer[length - 1] = '\0';
	}

	return buffer;
}

void filing_set_status(struct filing_block *in, enum filing_status status)
{
	if (in == NULL)
		return;

	in->status = status;
}


/**
 * Return a pointer to the next comma-separated text field in the current
 * token value read from the input file.
 *
 * \param *in		The input file to read from.
 * \return		Pointer to the next NULL terminated field, or
 *			NULL if the field doesn't exist.
 */

static char *filing_find_next_field(struct filing_block *in)
{
	char		*start = NULL;
	osbool		quoted;

	if (in == NULL)
		return NULL;

	if (*in->field == '\0')
		return NULL;

	quoted = FALSE;
	start = in->field;

	while (*in->field != '\0' && ((*in->field != ',') || quoted)) {
		if (*in->field == '"')
			quoted = !quoted;

		in->field++;
	}

	if (*in->field == ',') {
		*in->field = '\0';
		in->field++;
	}

	return start;
}
