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
 * \file: sort.h
 *
 * Sorting interface
 */

#ifndef CASHBOOK_SORT
#define CASHBOOK_SORT

/**
 * A sort instance handle.
 */

struct sort_block;


/**
 * Data sort types: values to indicate which field a window is sorted on. Types
 * are shared between windows wherever possible, to avoid duplicating field names.
 */

enum sort_type {
	SORT_NONE = 0,					/**< Apply no sort.						*/

	/* Bitfield masks. */

	SORT_MASK = 0x0ffff,				/**< Mask the sort type field from the value.			*/

	/* Sort directions. */

	SORT_ASCENDING = 0x10000,			/**< Sort in ascending order.					*/
	SORT_DESCENDING = 0x20000,			/**< Sort in descending order.					*/

	/* Sorts applying to different windows. */

	SORT_DATE = 0x00001,				/**< Sort on the Date column of a window.			*/
	SORT_FROM = 0x00002,				/**< Sort on the From Account column of a window.		*/
	SORT_TO = 0x00003,				/**< Sort on the To Account column of a window.			*/
	SORT_REFERENCE = 0x00004,			/**< Sort on the Reference column of a window.			*/
	SORT_DESCRIPTION = 0x00005,			/**< Sort on the Description column of a window.		*/
	SORT_ROW = 0x00006,				/**< Sort on the Row column of a window.			*/

	/* Sorts applying to Transaction windows. */

	SORT_AMOUNT = 0x00010,				/**< Sort on the Amount column of a window.			*/

	/* Sorts applying to Account View windows. */

	SORT_FROMTO = 0x00100,				/**< Sort on the From/To Account column of a window.		*/
	SORT_PAYMENTS = 0x00200,			/**< Sort on the Payments column of a window.			*/
	SORT_RECEIPTS = 0x00300,			/**< Sort on the Receipts column of a window.			*/
	SORT_BALANCE = 0x00400,				/**< Sort on the Balance column of a window.			*/

	/* Sorts applying to Standing Order windows. */

	SORT_NEXTDATE = 0x01000,			/**< Sort on the Next Date column of window.			*/
	SORT_LEFT = 0x02000,				/**< Sort on the Left column of a window.			*/

	/* Sorts applying to Preset windows. */

	SORT_CHAR = 0x03000,				/**< Sort on the Char column of a window.			*/
	SORT_NAME = 0x04000,				/**< Sort on the Name column of a window.			*/

	/* Sorts applying to Interest windows. */

	SORT_RATE = 0x05000				/**< Sort on the Rate column of a window.			*/
};


/**
 * A set of callbacks which clients must either supply or set to NULL. These
 * will be used by the edit line to communicate with the client.
 */

struct sort_callback {
	/**
	 * Request the client compare the data at two indexes.
	 * 
	 * \param *transfer		Pointer to the data transfer structure to take the data.
	 * \return			TRUE if data was returned; FALSE if not.
	 */

	int			(*compare)(enum sort_type type, int index1, int index2, void *data);

	void			(*swap)(int index1, int index2, void *data);
};


/**
 * Create a new Sort instance.
 *
 * \param type			The initial sort type data for the instance.
 * \param *callback		Pointer to the client-supplied sort callbacks.
 * \param *data			Pointer to the client data to be returned on all callbacks.
 * \return			Pointer to the newly created instance, or NULL on failure.
 */

struct sort_block *sort_create_instance(enum sort_type type, struct sort_callback *callback, void *data);


/**
 * Delete a Sort instance.
 *
 * \param *nstance		The instance to be deleted.
 */

void sort_delete_instance(struct sort_block *instance);


/**
 * Set the sort details -- field and direction -- of the instance.
 *
 * \param *instance		The instance to set.
 * \param order			The new sort details to be set.
 */

void sort_set_order(struct sort_block *instance, enum sort_type order);


/**
 * Get the current sort details -- field and direction -- from a
 * sort instance.
 *
 * \param *instance		The instance to interrogate.
 * \return			The sort order applied to the instance.
 */

enum sort_type sort_get_order(struct sort_block *instance);


/**
 * Copy the sort details -- field and direction -- from one sort instance
 * to another.
 *
 * \param *nstance		The instance to be updated with new details.
 * \param *source		The instance to copy the details from.
 */

void sort_copy_order(struct sort_block *instance, struct sort_block *source);


/**
 * Read the sort details encoded in a line of ASCII text, and use them to
 * update a sort instance.
 * 
 * \param *instance		The instance to be updated with the details.
 * \param *value		Pointer to the text containing the details.
 */

void sort_read_from_text(struct sort_block *instance, char *value);


/**
 * Write the sort details from an instance into an ASCII string.
 *
 * \param *instance		The instance to take the details from.
 * \param *buffer		Pointer to the buffer to take the string.
 * \param length		The length of the buffer supplied.
 */

void sort_write_as_text(struct sort_block *instance, char *buffer, size_t length);


/**
 * Perform a sort operation using the settings contained in a sort instance.
 *
 * \param *instance		The instance to use for the sorting.
 * \param items			The number of items which are to be sorted.
 */

void sort_process(struct sort_block *instance, size_t items);

#endif
