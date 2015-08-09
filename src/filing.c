/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
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
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "ihelp.h"
#include "presets.h"
#include "report.h"
#include "sorder.h"
#include "templates.h"
#include "transact.h"
#include "window.h"

/* ==================================================================================================================
 * Global Variables
 */

#define FILING_CURRENT_FORMAT 101

static struct file_block	*import_window_file = NULL;
static wimp_w			filing_import_window = NULL;



static void		filing_import_complete_click_handler(wimp_pointer *pointer);
static void 		filing_close_import_complete(osbool show_log);


/**
 * Initialise the filing system.
 */

void filing_initialise(void)
{
	filing_import_window = templates_create_window("ImpComp");
	ihelp_add_window(filing_import_window, "ImpComp", NULL);
	event_add_window_mouse_event(filing_import_window, filing_import_complete_click_handler);
}


/* ==================================================================================================================
 * Account file loading
 */

void load_transaction_file(char *filename)
{
	enum config_read_status	result = sf_CONFIG_READ_EOF;
	int			file_format = 0;
	char			section[MAX_FILE_LINE_LEN], token[MAX_FILE_LINE_LEN], value[MAX_FILE_LINE_LEN], *suffix;
	bits			load;
	struct file_block	*file;
	FILE			*in;
	osbool			unknown_data = FALSE, unknown_format = FALSE;

	#ifdef DEBUG
	debug_printf("\\BLoading accounts file");
	#endif

	file = build_new_file_block();

	if (file == NULL) {
		error_msgs_report_error("NoMemForLoad");
		return;
	}

	in = fopen(filename, "r");

	if (in == NULL) {
		delete_file(file);
		error_msgs_report_error("FileLoadFail");
		return;
	}

	hourglass_on();

	*section = '\0';
	*token = '\0';
	*value = '\0';

	do {
		next_field(section, ':');
		suffix = next_field(NULL, ':');

		if (string_nocase_strcmp(section, "Budget") == 0)
			result = budget_read_file(file, in, section, token, value, &unknown_data);
		else if (string_nocase_strcmp(section, "Accounts") == 0)
			result = account_read_acct_file(file, in, section, token, value, file_format, &unknown_data);
		else if (string_nocase_strcmp(section, "AccountList") == 0)
			result = account_read_list_file(file, in, section, token, value, suffix, &unknown_data);
		else if (string_nocase_strcmp(section, "Transactions") == 0)
			result = transact_read_file(file, in, section, token, value, file_format, &unknown_data);
		else if (string_nocase_strcmp(section, "StandingOrders") == 0)
			result = sorder_read_file(file, in, section, token, value, &unknown_data);
		else if (string_nocase_strcmp(section, "Presets") == 0)
			result = preset_read_file(file, in, section, token, value, &unknown_data);
		else if (string_nocase_strcmp(section, "Reports") == 0)
			result = analysis_read_file(file, in, section, token, value, &unknown_data);
		else {
			do {
				if (*section != '\0')
					unknown_data = TRUE;

				/* Load in the file format, converting an n.nn number into an
				 * integer value (eg. 1.00 would become 100).  Supports 0.00 to 9.99.
				 */

				if (string_nocase_strcmp(token, "Format") == 0) {
					if (strlen(value) == 4 && isdigit(value[0]) && isdigit(value[2]) && isdigit(value[3]) && value[1] == '.') {
						value[1] = value[2];
						value[2] = value[3];
						value[3] = '\0';

						file_format = atoi(value);

						if (file_format > FILING_CURRENT_FORMAT)
							unknown_format = TRUE;
					} else {
						unknown_data = TRUE;
					}
				}

				result = config_read_token_pair(in, token, value, section);
			} while (unknown_format == FALSE && result != sf_CONFIG_READ_EOF && result != sf_CONFIG_READ_NEW_SECTION);
		}
	} while (unknown_format == FALSE && result != sf_CONFIG_READ_EOF);

	fclose(in);

	/* If the file format wasn't understood, get out now. */

	if (unknown_format) {
		delete_file(file);
		hourglass_off();
		error_msgs_report_error("UnknownFileFormat");
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
	transact_sort(file);
	sorder_sort(file->sorders);
	preset_sort(file->presets);
	transact_open_window(file); /* The window extent is set in this action. */

	hourglass_off();

	if (unknown_data)
		error_msgs_report_info("UnknownFileData");
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

/* ==================================================================================================================
 * Account file saving
 */

void save_transaction_file(struct file_block *file, char *filename)
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
	transact_write_file(file, out);
	sorder_write_file(file, out);
	preset_write_file(file, out);
	analysis_write_file(file, out);

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) CASHBOOK_FILE_TYPE);

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

	hourglass_off();
}

/* ==================================================================================================================
 * Delimited file import
 */

void import_csv_file (struct file_block *file, char *filename)
{
  FILE         *input;
  char         line[1024], log[1024], b1[64], b2[64],
               *date, *ref, *amount, *description, *dummy, *ident, *name, *raw_from, *raw_to;
  int          from, to, import_count, reject_count, error;
  wimp_pointer pointer;
  unsigned int type;
  enum transact_flags rec_from, rec_to;


  import_window_file = file;

  import_count = 0;
  reject_count = 0;

  hourglass_on ();

  /* If there's an existing log, delete it. */

  if (file->import_report != NULL)
  {
    report_delete(file->import_report);
    file->import_report = NULL;
  }

  if (windows_get_open (filing_import_window))
  {
    wimp_close_window (filing_import_window);
  }

  /* Open a log report for the process, and title it. */

  msgs_lookup("IRWinT", log, sizeof (log));
  file->import_report = report_open(file, log, NULL);

  file_get_leafname(file, line, sizeof (line));
  msgs_param_lookup("IRTitle", log, sizeof (log), line, NULL, NULL, NULL);
  report_write_line(file->import_report, 0, log);
  msgs_param_lookup("IRImpFile", log, sizeof (log), filename, NULL, NULL, NULL);
  report_write_line(file->import_report, 0, log);

  report_write_line(file->import_report, 0, "");

  msgs_lookup("IRHeadings", log, sizeof (log));
  report_write_line(file->import_report, 0, log);

  input = fopen (filename, "r");

  if (input != NULL)
  {
    while (fgets (line, sizeof (line), input) != NULL)
    {
      error = 0;

      /* Date */

      date = next_field (line, ',');
      date = unquote_string (date);

      if (date_convert_from_string(date, NULL_DATE, 0) == NULL_DATE)
      {
        error = 1;
      }

      /* From */

      ident = next_field (NULL, ',');
      ident = unquote_string (ident);

      raw_from = ident;

      rec_from = (strchr (ident, '#') > 0) ? TRANS_REC_FROM : TRANS_FLAGS_NONE;

      name = ident + strcspn (ident, "#:");
      *name++ = '\0';
      while (strchr ("#:", *name))
      {
        name++;
      }

      if (*ident == '\0')
      {
        error = 1;
        from = NULL_ACCOUNT;
      }
      else
      {
        type = isdigit (*ident) ? ACCOUNT_FULL : ACCOUNT_IN;
        from = account_find_by_ident(file, ident, type);

        if (from == -1)
        {
          from = account_add (file, name, ident, type);
        }
      }

      /* To */

      ident = next_field (NULL, ',');
      ident = unquote_string (ident);

      raw_to = ident;

      rec_to = (strchr (ident, '#') > 0) ? TRANS_REC_TO : TRANS_FLAGS_NONE;

      name = ident + strcspn (ident, "#:");
      *name++ = '\0';
      while (strchr ("#:", *name))
      {
        name++;
      }


      if (*ident == '\0')
      {
        error = 1;
        to = NULL_ACCOUNT;
      }
      else
      {
        type = isdigit(*ident) ? ACCOUNT_FULL : ACCOUNT_OUT;
        to = account_find_by_ident(file, ident, type);

        if (to == -1)
        {
          to = account_add(file, name, ident, type);
        }
      }

      /* Ref */

      ref = next_field(NULL, ',');
      ref = unquote_string(ref);

      /* Amount */

      amount = next_field(NULL, ',');

      if (*amount == '\0')
      {
        amount = next_field(NULL, ',');
      }
      else
      {
        dummy = next_field(NULL, ',');
      }

      amount = unquote_string(amount);

      /* Skip Balance */

      dummy = next_field(NULL, ',');

      /* Description */

      description = next_field(NULL, ',');
      description = unquote_string(description);

      /* Create a new transaction. */

      if (error == 1)
      {
        msgs_lookup("Rejected", b1, sizeof(b1));
        reject_count++;
      }
      else
      {
        transact_add_raw_entry(file, date_convert_from_string(date, NULL_DATE, 0), from, to, rec_from | rec_to,
                             currency_convert_from_string(amount), ref, description);
        msgs_lookup("Imported", b1, sizeof(b1));

        import_count++;
      }

      sprintf(log, "%s\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'",
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

  report_write_line (file->import_report, 0, "");

  sprintf (b1, "%d", import_count);
  sprintf (b2, "%d", reject_count);
  msgs_param_lookup ("IRTotals", log, sizeof (log), b1, b2, NULL, NULL);
  report_write_line (file->import_report, 0, log);

  sprintf (icons_get_indirected_text_addr (filing_import_window, ICOMP_ICON_IMPORTED), "%d", import_count);
  sprintf (icons_get_indirected_text_addr (filing_import_window, ICOMP_ICON_REJECTED), "%d", reject_count);

  wimp_get_pointer_info (&pointer);
  windows_open_centred_at_pointer (filing_import_window, &pointer);

  hourglass_off ();
}



/**
 * Process mouse clicks in the Import Complete dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void filing_import_complete_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case ICOMP_ICON_CLOSE:
		filing_close_import_complete(FALSE);
		break;

	case ICOMP_ICON_LOG:
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

/* ==================================================================================================================
 * Delimited file export
 */


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
		if (isspace(string[0]) || isspace (string[strlen(string)-1]))
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

/* ==================================================================================================================
 * String processing.
 */

char *unquote_string (char *string)
{
  string = string_strip_surrounding_whitespace (string);

  if (*string == '"' && *(strchr(string, '\0')-1) == '"')
  {
    string++;
    *(strchr(string, '\0')-1) = '\0';
  }

  return (string);
}

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
