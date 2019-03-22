/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * Return the file associated with a standing order instance.
 *
 * \param *instance		The standing order instance to query.
 * \return			The associated file, or NULL.
 */

struct file_block *sorder_get_file(struct sorder_block *instance);


/**
 * Return the from account of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the from account for.
 * \return			The from account of the standing order, or NULL_ACCOUNT.
 */

acct_t sorder_get_from(struct file_block *file, sorder_t sorder);


/**
 * Return the to account of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the to account for.
 * \return			The to account of the standing order, or NULL_ACCOUNT.
 */

acct_t sorder_get_to(struct file_block *file, sorder_t sorder);


/**
 * Return the standing order flags for a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the flags for.
 * \return			The flags of the standing order, or TRANS_FLAGS_NONE.
 */

enum transact_flags sorder_get_flags(struct file_block *file, sorder_t sorder);


/**
 * Return the to amount of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the amount for.
 * \return			The amount of the standing order, or NULL_CURRENCY.
 */

amt_t sorder_get_amount(struct file_block *file, sorder_t sorder);


/**
 * Return the next date of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the next date for.
 * \return			The next date of the standing order, or NULL_DATE.
 */

date_t sorder_get_next_date(struct file_block *file, sorder_t sorder);


/**
 * Return the remaining transactions for a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the remaining transactions for.
 * \return			The remaining transactions for the standing order, or 0.
 */

date_t sorder_get_remaining_transactions(struct file_block *file, sorder_t sorder);


/**
 * Return the description for a standing order.
 *
 * If a buffer is supplied, the description is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * description in the preset array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the description of.
 * \param *buffer		Pointer to a buffer to take the description, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting description string,
 *				either the supplied buffer or the original.
 */

char *sorder_get_description(struct file_block *file, sorder_t sorder, char *buffer, size_t length);


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
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sorder_read_file(struct file_block *file, struct filing_block *in);


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

