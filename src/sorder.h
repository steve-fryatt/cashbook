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
 * \file: sorder.h
 *
 * Standing Order implementation.
 */

#ifndef CASHBOOK_SORDER
#define CASHBOOK_SORDER

#include "filing.h"

/* A standing order number. */

typedef int sorder_t;

struct sorder_block;

#define NULL_SORDER ((sorder_t) (-1))

/**
 * Initialise the standing order system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_initialise(osspriteop_area *sprites);


/**
 * Create a new Standing Order window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct sorder_block *sorder_create_instance(struct file_block *file);


/**
 * Delete a Standing Order instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void sorder_delete_instance(struct sorder_block *windat);


/**
 * Create and open a Standing Order List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void sorder_open_window(struct file_block *file);


/**
 * Recreate the title of the Standing Order List window connected to the given
 * file.
 *
 * \param *file			The file to rebuild the title for.
 */

void sorder_build_window_title(struct file_block *file);


/**
 * Force the complete redraw of the Standing Order list window.
 *
 * \param *file			The file owning the window to redraw.
 */

void sorder_redraw_all(struct file_block *file);


/**
 * Open the Standing Order Edit dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void sorder_open_edit_window(struct file_block *file, sorder_t sorder, wimp_pointer *ptr);


/**
 * Sort the standing orders in a given file based on that file's sort setting.
 *
 * \param *windat		The standing order instance to sort.
 */

void sorder_sort(struct sorder_block *windat);


/**
 * Purge unused standing orders from a file.
 *
 * \param *file			The file to purge.
 */

void sorder_purge(struct file_block *file);


/**
 * Find the number of standing orders in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of standing orders in the file.
 */

int sorder_get_count(struct file_block *file);


/**
 * Scan the standing orders in a file, adding transactions for any which have
 * fallen due.
 *
 * \param *file			The file to process.
 */

void sorder_process(struct file_block *file);


/**
 * Scan the standing orders in a file, and update the traial values to reflect
 * any pending transactions.
 *
 * \param *file			The file to scan.
 */

void sorder_trial(struct file_block *file);


/**
 * Generate a report detailing all of the standing orders in a file.
 *
 * \param *file			The file to report on.
 */

void sorder_full_report(struct file_block *file);


/**
 * Save the standing order details from a file to a CashBook file
 *
 * \param *file			The file to write.
 * \param *f			The file handle to write to.
 */

void sorder_write_file(struct file_block *file, FILE *f);


/**
 * Read standing order details from a CashBook file into a file block.
 *
 * \param *file			The file to read into.
 * \param *out			The file handle to read from.
 * \param *section		A string buffer to hold file section names.
 * \param *token		A string buffer to hold file token names.
 * \param *value		A string buffer to hold file token values.
 * \param *value		A string buffer to hold file token values.
 * \param *unknown_data		A boolean flag to be set if unknown data is encountered.
 */

enum osbool sorder_read_file(struct file_block *file, struct filing_block *in);


/**
 * Check the standing orders in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool sorder_check_account(struct file_block *file, int account);

#endif

