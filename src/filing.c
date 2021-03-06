/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
#include "sflib/heap.h"
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
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "import_dialogue.h"
#include "interest.h"
#include "preset.h"
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

/**
 * Test for file load statuses which are considered OK for continuing.
 */

#define filing_load_status_is_ok(status) (((status) == FILING_STATUS_OK) || ((status) == FILING_STATUS_UNEXPECTED))


/* Static Function Prototypes. */

static void		filing_open_import_complete_window(struct file_block *file, wimp_pointer *ptr, int imported, int rejected);
static osbool		filing_process_import_complete_window(void *parent, struct import_dialogue_data *content);
static char		*filing_find_next_field(struct filing_block *in);


/**
 * Initialise the filing system.
 */

void filing_initialise(void)
{
	import_dialogue_initialise();
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
			} while (filing_get_next_token(&in));
		}
	} while (filing_load_status_is_ok(in.status) && in.result != sf_CONFIG_READ_EOF);

	fclose(in.handle);

	/* If the file format wasn't understood, get out now. */

	if (!filing_load_status_is_ok(in.status)) {
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

	string_copy(file->filename, filename, FILE_MAX_FILENAME);

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
	
	string_copy(file->filename, filename, FILE_MAX_FILENAME);

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
	FILE			*input;
	char			line[FILING_CSV_LINE_LENGTH], log[FILING_LOG_LINE_LENGTH], b1[FILING_TEMP_BUF_LENGTH], b2[FILING_TEMP_BUF_LENGTH],
						*date, *ref, *amount, *description, *ident, *name, *raw_from, *raw_to;
	int			from, to, import_count, reject_count;
	osbool			error;
	wimp_pointer		pointer;
	unsigned int		type;
	enum transact_flags	rec_from, rec_to;


	import_count = 0;
	reject_count = 0;

	hourglass_on();

	/* If there's an existing log, delete it. */

	if (file->import_report != NULL) {
		report_delete(file->import_report);
		file->import_report = NULL;
	}

	dialogue_force_group_closed(DIALOGUE_GROUP_IMPORT); // Could be done via parent once import has a class.

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

			name = ident + strcspn(ident, "#:�");
			*name++ = '\0';
			while (strchr("#:�", *name))
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

			name = ident + strcspn(ident, "#:�");
			*name++ = '\0';
			while (strchr("#:�", *name))
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

			string_printf(log, FILING_LOG_LINE_LENGTH, "%s\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'",
					b1, date, raw_from, raw_to, ref, amount, description);
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

	string_printf(b1, FILING_TEMP_BUF_LENGTH, "%d", import_count);
	string_printf(b2, FILING_TEMP_BUF_LENGTH, "%d", reject_count);

	msgs_param_lookup("IRTotals", log, FILING_LOG_LINE_LENGTH, b1, b2, NULL, NULL);
	report_write_line(file->import_report, 0, log);

	wimp_get_pointer_info(&pointer);
	filing_open_import_complete_window(file, &pointer, import_count, reject_count);

	hourglass_off();
}


/**
 * Open the Import Result dialogue for a given import process.
 *
 * \param *file			The file to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void filing_open_import_complete_window(struct file_block *file, wimp_pointer *ptr, int imported, int rejected)
{
	struct import_dialogue_data *content = NULL;

	if (file == NULL)
		return;

	/* Open the dialogue box. */

	content = heap_alloc(sizeof(struct import_dialogue_data));
	if (content == NULL)
		return;

	content->action = IMPORT_DIALOGUE_ACTION_NONE;
	content->imported = imported;
	content->rejected = rejected;

	import_dialogue_open(ptr, file, filing_process_import_complete_window, content);
}


/**
 * Handle the closure of the Import Result dialogue, either opening or
 * deleting the log report. Once the window is closed, we no longer need
 * to track the report, so the pointer can be set to NULL.
 *
 * \param *parent		The parent file for the dialogue.
 * \param *content		The dialogue content.
 * \return			TRUE on success.
 */

static osbool filing_process_import_complete_window(void *parent, struct import_dialogue_data *content)
{
	struct file_block *file = parent;

	if (file == NULL || content == NULL)
		return FALSE;

	switch (content->action) {
	case IMPORT_DIALOGUE_ACTION_CLOSE:
		report_delete(file->import_report);
		break;
	
	case IMPORT_DIALOGUE_ACTION_VIEW_REPORT:
		report_close(file->import_report);
		break;

	default:
		return FALSE;
	}

	file->import_report = NULL;

	return TRUE;
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


/**
 * Get the file format number of the a disc file. If the format token has
 * not been found, the format is returned as zero.
 * 
 * \param *in			The file to report on.
 * \return			The file version, or 0 if not known.
 */

int filing_get_format(struct filing_block *in)
{
	if (in == NULL)
		return 0;

	return in->format;
}


/**
 * Move the file pointer on to the next token contained in the file.
 * 
 * \param *in			The file being loaded.
 * \return			TRUE if the token is in the current section;
 *				FALSE if there are no more tokens in the section.
 */

osbool filing_get_next_token(struct filing_block *in)
{
	char *separator;

	if (in == NULL || !filing_load_status_is_ok(in->status))
		return FALSE;

	in->result = config_read_token_pair(in->handle, in->token, in->value, in->section);

#ifdef DEBUG
	debug_printf("Read line: section=%s, token=%s, value=%s", in->section, in->token, in->value);
#endif

	if (in->result == sf_CONFIG_READ_NEW_SECTION) {
		separator = strchr(in->section, ':');
		if (separator != NULL) {
			*separator = '\0';
			in->suffix = separator + 1;
#ifdef DEBUG
			debug_printf("Split section: section=%s, suffix=%s", in->section, in->suffix);
#endif
		} else {
			in->suffix = NULL;
		}
	}

	in->field = in->value;

	return (in->result != sf_CONFIG_READ_EOF && in->result != sf_CONFIG_READ_NEW_SECTION) ? TRUE : FALSE;
}


/**
 * Get the account type from the end of an account list section.
 *
 * \param *in			The file being loaded.
 * \return			The account type, or ACCOUT_NULL.
 */

enum account_type filing_get_account_type_suffix(struct filing_block *in)
{
	if (in == NULL || in->suffix == NULL)
		return ACCOUNT_NULL;

	return strtoul(in->suffix, NULL, 16);
}


/**
 * Test the name of the current token in a file.
 *
 * \param *in			The filing being loaded.
 * \param *token		Pointer to the token name to test against.
 * \return			TRUE if the token name matches; otherwise FALSE.
 */

osbool filing_test_token(struct filing_block *in, char *token)
{
	if (in == NULL)
		return FALSE;

	return (string_nocase_strcmp(in->token, token) == 0) ? TRUE : FALSE;
}


/**
 * Get the textual value of a token in a file, either returning a pointer to
 * the volatile data in memory or copying it into a supplied buffer. If the
 * token value is too long to fit in the buffer, the file is reported as
 * being corrupt.
 *
 * \param *in			The file being loaded.
 * \param *buffer		Pointer to a buffer to take the string, or
 *				NULL to return a pointer to the string in
 *				volatile memory.
 * \param length		The length of the supplied buffer, or 0.
 * \return			Pointer to the value string, either in the
 *				supplied buffer or in volatile memory.
 */

char *filing_get_text_value(struct filing_block *in, char *buffer, size_t length)
{
	if (in == NULL)
		return NULL;

#ifdef DEBUG
	debug_printf("Return text value: %s", in->value);
#endif

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
#ifdef DEBUG
		debug_printf("Field is too long: original=%s, copied=%s", in->value, buffer);
#endif
	}

	return buffer;
}


/**
 * Return the boolean value of a token in a file, which will be in "Yes"
 * or "No" format.
 *
 * \param *in			The file being loaded.
 * \return			The boolean value, or FALSE.
 */

osbool filing_get_opt_value(struct filing_block *in)
{
	if (in == NULL || in->value == NULL)
		return FALSE;

	return config_read_opt_string(in->value);
}


/**
 * Return the value of an integer field in a comma-separated token record.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or 0.
 */

int filing_get_int_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return 0;
	}

	return strtoul(field, NULL, 16);
}


/**
 * Return the value of an unsigned field in a comma-separated token record.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or 0.
 */

unsigned filing_get_unsigned_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return 0;
	}

	return strtoul(field, NULL, 16);
}


/**
 * Return the value of a char field in a comma-separated token record.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or \0.
 */

char filing_get_char_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return '\0';
	}

	return strtoul(field, NULL, 16);
}


/**
 * Return the value of a boolean field in a comma-separated token record:
 * the value "1" is taken as TRUE, while all other values are FALSE.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or FALSE.
 */

osbool filing_get_opt_field(struct filing_block *in)
{
	char	*field = filing_find_next_field(in);

	if (field == NULL) {
		in->status = FILING_STATUS_CORRUPT;
		return FALSE;
	}

	return (*field == '1') ? TRUE : FALSE;
}


/**
 * Return the value of a text field in a comma-separated token record, 
 * ither returning a pointer to the volatile data in memory or copying
 * it into a supplied buffer. If the field is missing or empty, or the
 * token value is too long to fit in the buffer, the file is reported as
 * being corrupt.
 *
 * \param *in			The file being loaded.
 * \param *buffer		Pointer to a buffer to take the string, or
 *				NULL to return a pointer to the string in
 *				volatile memory.
 * \param length		The length of the supplied buffer, or 0.
 * \return			Pointer to the value string, either in the
 *				supplied buffer or in volatile memory, or
 *				NULL on failure.
 */
 
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
#ifdef DEBUG
		debug_printf("Field is too long: original=%s, copied=%s", field, buffer);
#endif
	}

	return buffer;
}


/**
 * Set the status of a file being loaded, to indicate problems that have
 * been encountered by the client modules.
 *
 * \param *in			The file being loaded.
 * \param status		The new status to set for the file.
 */

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

#ifdef DEBUG
	debug_printf("Split out next field: field=%s, tail=%s", start, in->field);
#endif

	return start;
}

