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

/**
 * The length of a field in a delimited file export.
 */

#define FILING_DELIMITED_FIELD_LEN 256

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
	FILING_STATUS_BAD_MEMORY,						/**< Something went wrong with the memory allocation.				*/
	FILING_STATUS_CORRUPT							/**< The file contents appeared to be corrupt.					*/
};

struct filing_block;

#include "account.h"
#include "currency.h"
#include "date.h"
#include "file.h"

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


/**
 * Get the file format number of the a disc file. If the format token has
 * not been found, the format is returned as zero.
 * 
 * \param *in			The file to report on.
 * \return			The file version, or 0 if not known.
 */

int filing_get_format(struct filing_block *in);


/**
 * Move the file pointer on to the next token contained in the file.
 * 
 * \param *in			The file being loaded.
 * \return			TRUE if the token is in the current section;
 *				FALSE if there are no more tokens in the section.
 */

osbool filing_get_next_token(struct filing_block *in);


/**
 * Get the account type from the end of an account list section.
 *
 * \param *in			The file being loaded.
 * \return			The account type, or ACCOUT_NULL.
 */

enum account_type filing_get_account_type_suffix(struct filing_block *in);


/**
 * Test the name of the current token in a file.
 *
 * \param *in			The filing being loaded.
 * \param *token		Pointer to the token name to test against.
 * \return			TRUE if the token name matches; otherwise FALSE.
 */

osbool filing_test_token(struct filing_block *in, char *token);


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

char *filing_get_text_value(struct filing_block *in, char *buffer, size_t length);


/**
 * Return the boolean value of a token in a file, which will be in "Yes"
 * or "No" format.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or FALSE.
 */

osbool filing_get_opt_value(struct filing_block *in);


/**
 * Return the value of an integer field in a comma-separated token record.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or 0.
 */

int filing_get_int_field(struct filing_block *in);


/**
 * Return the value of an unsigned field in a comma-separated token record.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or 0.
 */

unsigned filing_get_unsigned_field(struct filing_block *in);


/**
 * Return the value of a char field in a comma-separated token record.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or \0.
 */

char filing_get_char_field(struct filing_block *in);


/**
 * Return the value of a boolean field in a comma-separated token record:
 * the value "1" is taken as TRUE, while all other values are FALSE.
 * The file's data is updated to identify the next field in the record. If
 * the field is missing or empty, the file is marked as corrupt.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or FALSE.
 */

osbool filing_get_opt_field(struct filing_block *in);


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

char *filing_get_text_field(struct filing_block *in, char *buffer, size_t length);


/**
 * Set the status of a file being loaded, to indicate problems that have
 * been encountered by the client modules.
 *
 * \param *in			The file being loaded.
 * \param status		The new status to set for the file.
 */

void filing_set_status(struct filing_block *in, enum filing_status status);

#endif
