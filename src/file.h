/* Copyright 2003-2013, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: file.h
 *
 * Search file record creation, maipulation and deletion.
 */

#ifndef CASHBOOK_FILE
#define CASHBOOK_FILE

/* ==================================================================================================================
 * Static constants
 */


#define CASHBOOK_FILE_TYPE 0x1ca
#define CSV_FILE_TYPE 0xdfe
#define TSV_FILE_TYPE 0xfff
#define TEXT_FILE_TYPE 0xfff
#define FANCYTEXT_FILE_TYPE 0xaf8

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/**
 * Initialise the overall file system.
 */

void file_initialise(void);


/* File initialisation and deletion */

file_data *build_new_file_block (void);
void create_new_file (void);
void delete_file (file_data *file);

/* Finding files */

file_data *find_transaction_window_file_block (wimp_w window);

/* Saved files and data integrity */

int  check_for_unsaved_files (void);


/**
 * Set the 'unsaved' state of a file.
 *
 * \param *file		The file to update.
 * \param unsafe	TRUE if the file has unsaved data; FALSE if not.
 */

void file_set_data_integrity(file_data *file, osbool unsafe);


/**
 * Check if the file has a full save path (ie. it has been saved before, or has
 * been loaded from disc).
 *
 * \param *file		The file to test.
 * \return		TRUE if there is a full filepath; FALSE if not.
 */

osbool file_check_for_filepath(file_data *file);


/**
 * Return a path-name string for the current file, using the <Untitled n>
 * format if the file hasn't been saved.
 *
 * \param *file		The file to build a pathname for.
 * \param *path		The buffer to return the pathname in.
 * \param len		The length of the supplied buffer.
 * \return		A pointer to the supplied buffer.
 */

char *file_get_pathname(file_data *file, char *path, int len);


/**
 * Return a leaf-name string for the current file, using the <Untitled n>
 * format if the file hasn't been saved.
 *
 * \param *file		The file to build a leafname for.
 * \param *leaf		The buffer to return the leafname in.
 * \param len		The length of the supplied buffer.
 * \return		A pointer to the supplied buffer.
 */

char *file_get_leafname(file_data *file, char *leaf, int len);




/* General file redraw. */

void redraw_all_files (void);
void redraw_file_windows (file_data *file);

/* A change of date. */

void update_files_for_new_date (void);

#endif
