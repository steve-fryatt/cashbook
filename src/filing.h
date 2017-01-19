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
 * \file: filing.h
 *
 * Legacy file load and save routines.
 */

#ifndef CASHBOOK_FILING
#define CASHBOOK_FILING

#include <stdio.h>


/**
 * The maximum length of a line in a CashBook file.
 */

#define FILING_MAX_FILE_LINE_LEN 1024

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

enum filing_status {
	FILING_STATUS_OK,							/**< The operation is OK.							*/
	FILING_STATUS_VERSION,							/**< An unknown file version number has been found.				*/ 
	FILING_STATUS_UNEXPECTED,						/**< The operation has encountered unexpected file contents.			*/
	FILING_STATUS_MEMORY,							/**< The operation has run out of memory.					*/
	FILING_STATUS_BAD_MEMORY						/**< Something went wrong with the memory allocation.				*/
};

/* Function Prototypes. */

/**
 * Initialise the filing system.
 */

void filing_initialise(void);


/**
 * Load a CashBook file into memory, creating a new file instance and opening
 * a transaction window to display the contents.
 *
 * \param *filename		Pointer to the name of the file to be loaded.
 */

void filing_load_cashbook_file(char *filename);


/**
 * Save the data associated with a file block back to disc.
 *
 * \param *file			The file instance to be saved.
 * \param *filename		Pointer to the name of the file to save to.
 */

void filing_save_cashbook_file(struct file_block *file, char *filename);


/**
 * Import the contents of a CSV file into an existing file instance.
 *
 * \param *file			The file instance to take the CSV data.
 * \param *filename		Pointer to the name of the CSV file to process.
 */

void filing_import_csv_file(struct file_block *file, char *filename);


/**
 * Force the closure of the Import windows if the owning file disappears.
 * There's no need to delete any associated report, because it will be handled
 * via the Report module when the file disappears.
 *
 * \param *file			The file which has closed.
 */

void filing_force_windows_closed(struct file_block *file);


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

char *filing_read_delimited_field(char *line, enum filing_delimit_type format, enum filing_delimit_flags flags);


/* String processing */

char *next_field (char *line, char sep);
char *next_plain_field (char *line, char sep);
#endif
