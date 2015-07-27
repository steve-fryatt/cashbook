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

struct file_block *build_new_file_block (void);
void create_new_file (void);
void delete_file (struct file_block *file);

/* Finding files */

struct file_block *find_transaction_window_file_block (wimp_w window);


/**
 * Check for unsaved files and for any pending print jobs which are
 * currently attached to open files, and warn the user if any are found.
 *
 * \return		TRUE if there is something that isn't saved; FALSE
 *			if there's nothing worth saving.
 */

osbool file_check_for_unsaved_data(void);


/**
 * Set the 'unsaved' state of a file.
 *
 * \param *file		The file to update.
 * \param unsafe	TRUE if the file has unsaved data; FALSE if not.
 */

void file_set_data_integrity(struct file_block *file, osbool unsafe);


/**
 * Check if the file has a full save path (ie. it has been saved before, or has
 * been loaded from disc).
 *
 * \param *file		The file to test.
 * \return		TRUE if there is a full filepath; FALSE if not.
 */

osbool file_check_for_filepath(struct file_block *file);


/**
 * Return a path-name string for the current file, using the <Untitled n>
 * format if the file hasn't been saved.
 *
 * \param *file		The file to build a pathname for.
 * \param *path		The buffer to return the pathname in.
 * \param len		The length of the supplied buffer.
 * \return		A pointer to the supplied buffer.
 */

char *file_get_pathname(struct file_block *file, char *path, int len);


/**
 * Return a leaf-name string for the current file, using the <Untitled n>
 * format if the file hasn't been saved.
 *
 * \param *file		The file to build a leafname for.
 * \param *leaf		The buffer to return the leafname in.
 * \param len		The length of the supplied buffer.
 * \return		A pointer to the supplied buffer.
 */

char *file_get_leafname(struct file_block *file, char *leaf, int len);


/**
 * Redraw the windows associated with all open files.
 */

void file_redraw_all(void);


/**
 * Redraw all the windows connected with a given file.
 *
 * \param *file		The file to redraw the windows for.
 */

void file_redraw_windows(struct file_block *file);


/**
 * Process any open files on a change of date: adding any new standing
 * orders and recalculate all the files on a change of day.
 */

void file_process_date_change(void);

#endif
