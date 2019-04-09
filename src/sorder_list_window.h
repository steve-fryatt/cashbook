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
 * \file: sorder_list_window.h
 *
 * Standing Order List Window implementation.
 */

#ifndef CASHBOOK_SORDER_LIST_WINDOW
#define CASHBOOK_SORDER_LIST_WINDOW

#include "sorder.h"

/**
 * Initialise the Standing Order List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_list_window_initialise(osspriteop_area *sprites);


/**
 * Create a new Standing Order List Window instance.
 *
 * \param *parent		The parent sorder instance.
 * \return			Pointer to the new instance, or NULL.
 */

struct sorder_list_window *sorder_list_window_create_instance(struct sorder_block *parent);


/**
 * Destroy a Standing Order List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void sorder_list_window_delete_instance(struct sorder_list_window *windat);


/**
 * Create and open a Standing Order List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 * \param sorders		The number of standing orders to include.
 */

void sorder_list_window_open(struct sorder_list_window *windat, int sorders);


/**
 * Recreate the title of the given Standing Order List window.
 *
 * \param *windat		The standing order window to rebuild the
 *				title for.
 */

void sorder_list_window_build_title(struct sorder_list_window *windat);


/**
 * Force the redraw of one or all of the standing orders in the given
 * Standing Order list window.
 *
 * \param *windat		The standing order window to redraw.
 * \param sorder		The standing order to redraw, or NULL_SORDER for all.
 * \param stopped		TRUE to redraw just the active columns.
 */

void sorder_list_window_redraw(struct sorder_list_window *windat, sorder_t sorder, osbool stopped);


/**
 * Sort the standing orders in a given list window based on that instances's
 * sort setting.
 *
 * \param *windat		The standing order window instance to sort.
 */

void sorder_list_window_sort(struct sorder_list_window *windat);


/**
 * Initialise the contents of the standing order list window, creating an
 * entry for each of the required standing orders.
 *
 * \param *windat		The standing order list window instance to initialise.
 * \param sorders		The number of standing orders to insert.
 * \return			TRUE on success; FALSE on failure.
 */

osbool sorder_list_window_initialise_entries(struct sorder_list_window *windat, int sorders);


/**
 * Add a new standing order to an instance of the standing order list window.
 *
 * \param *windat		The standing order list window instance to add to.
 * \param sorder		The standing order index to add.
 * \return			TRUE on success; FALSE on failure.
 */

osbool sorder_list_window_add_sorder(struct sorder_list_window *windat, sorder_t sorder);


/**
 * Remove a standing order from an instance of the standing order list window,
 * and update the other entries to allow for its deletion.
 *
 * \param *windat		The standing order list window instance to remove from.
 * \param sorder		The standing order index to remove.
 * \return			TRUE on success; FALSE on failure.
 */

osbool sorder_list_window_delete_sorder(struct sorder_list_window *windat, sorder_t sorder);


/**
 * Save the standing order list window details from a window to a CashBook
 * file. This assumes that the caller has already created a suitable section
 * in the file to be written.
 *
 * \param *windat		The window whose details to write.
 * \param *out			The file handle to write to.
 */

void sorder_list_window_write_file(struct sorder_list_window *windat, FILE *out);


/**
 * Process a WinColumns line from the StandingOrders section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The column text line.
 */

void sorder_list_window_read_file_wincolumns(struct sorder_list_window *windat, char *columns);


/**
 * Process a SortOrder line from the StandingOrders section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The sort order text line.
 */

void sorder_list_window_read_file_sortorder(struct sorder_list_window *windat, char *order);

#endif

