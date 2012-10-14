/* Copyright 2003-2012, Stephen Fryatt
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
 * \file: filing.h
 *
 * Legacy file load and save routines.
 */

#ifndef CASHBOOK_FILING
#define CASHBOOK_FILING

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define LOAD_SECT_NONE     0
#define LOAD_SECT_BUDGET   1
#define LOAD_SECT_ACCOUNTS 2
#define LOAD_SECT_ACCLIST  3
#define LOAD_SECT_TRANSACT 4
#define LOAD_SECT_SORDER   5
#define LOAD_SECT_PRESET   6
#define LOAD_SECT_REPORT   7

#define ICOMP_ICON_IMPORTED 0
#define ICOMP_ICON_REJECTED 2
#define ICOMP_ICON_CLOSE 5
#define ICOMP_ICON_LOG 4

#define MAX_FILE_LINE_LEN 1024

enum filing_delimit_type {
	DELIMIT_TAB,								/**< Fields delimited by tabs.							*/
	DELIMIT_COMMA,								/**< Fields delimited by commas; text quoted when whitespace requires.		*/
	DELIMIT_QUOTED_COMMA							/**< Fields delimited by commas; text always quoted.				*/
};

enum filing_delimit_flags {
	DELIMIT_NONE	= 0,							/**< Flags unset.								*/
	DELIMIT_LAST	= 0x01,							/**< Last field on the line (no delimiter follows).				*/
	DELIMIT_NUM	= 0x02							/**< Numeric field, so no quoting required.					*/
};

/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */



/**
 * Initialise the filing system.
 */

void filing_initialise(void);




int filing_budget_read_file(file_data *file, FILE *in, char *section, char *token, char *value, osbool *unknown_data);



/* Loading accounts files */

void load_transaction_file (char *filename);
char *next_plain_field (char *line, char sep);

/* Saving accounts files */

void save_transaction_file (file_data *file, char *filename);

/* Delimited file import */

void import_csv_file (file_data *file, char *filename);



/**
 * Force the closure of the Import windows if the owning file disappears.
 * There's no need to delete any associated report, because it will be handled
 * via the Report module when the file disappears.
 *
 * \param *file			The file which has closed.
 */

void filing_force_windows_closed(file_data *file);




/* Delimited file export */

void export_delimited_accounts_file (file_data *file, int entry, char *filename, int format, int filetype);


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

int filing_output_delimited_field(FILE *f, char *string, enum filing_delimit_type format, enum filing_delimit_flags flags);

/* String processing */

char *unquote_string (char *string);
char *next_field (char *line, char sep);

#endif
