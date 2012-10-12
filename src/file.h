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
 * \file: file.h
 *
 * Search file record creation, maipulation and deletion.
 */

#ifndef CASHBOOK_FILE
#define CASHBOOK_FILE

/* ==================================================================================================================
 * Static constants
 */


/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* File initialisation and deletion */

file_data *build_new_file_block (void);
void create_new_file (void);
void delete_file (file_data *file);

/* Finding files */

file_data *find_transaction_window_file_block (wimp_w window);
file_data *find_transaction_pane_file_block (wimp_w window);

/* Saved files and data integrity */

int  check_for_unsaved_files (void);
void set_file_data_integrity(file_data *data, osbool unsafe);
int check_for_filepath (file_data *file);

/* Default filenames. */

char *make_file_pathname (file_data *file, char *path, int len);
char *make_file_leafname (file_data *file, char *leaf, int len);
char *generate_default_title (file_data *file, char *name, int length);

/* General file redraw. */

void redraw_all_files (void);
void redraw_file_windows (file_data *file);

/* A change of date. */

void update_files_for_new_date (void);

#endif
