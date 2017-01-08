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
 * Sorting user interface
 */

#ifndef CASHBOOK_SORT
#define CASHBOOK_SORT

/**
 * A sort dialogue data handle.
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
 * A sort window icon definition, linking an icon handle to a
 * sort type definition.
 */

struct sort_icon {
	wimp_i		icon;				/**< The handle of the icon being defined.			*/
	enum sort_type	type;				/**< The sort type which applied to the icon.			*/
};


/**
 * Create a new Sort dialogue definition.
 *
 * \param window		The handle of the window to use for the dialogue.
 * \param columns[]		Pointer to a list of icons relating to sort columns.
 * \param directions[]		Pointer to a list of icons relating to sort directions.
 * \param ok			The icon handle of the OK icon.
 * \param cancel		The icon handle of the Cancel icon.
 * \param *callback		Pointer to a callback function to receive selections.
 * \return			Pointer to the newly created dialogue handle, or NULL on failure.
 */

struct sort_block *sort_create_dialogue(wimp_w window, struct sort_icon columns[], struct sort_icon directions[], wimp_i ok, wimp_i cancel,
		osbool (*callback)(enum sort_type order, void *data));


/**
 * Open an instance of a Sort dialogue box.
 *
 * \param *dialogue		The handle of the dialogue to be opened.
 * \param *ptr			Pointer to Wimp Pointer data giving the required
 *				dialogue position.
 * \param order			The sort order to use to open the dialogue.
 * \param *data			Pointer to client-specific data to be passed to the
 *				selection callback function.
 */

void sort_open_dialogue(struct sort_block *dialogue, wimp_pointer *ptr, enum sort_type order, void *data);


/**
 * Close a visible instance of a Sort dialogue box.
 *
 * \param *dialogue		The handle of the dialogue to be closed.
 * \param *data			If NULL, any dialogue instance will close; otherwise,
 *				the instance will close only if data matches the
 *				data supplied to sort_open_dialogue().
 */

void sort_close_dialogue(struct sort_block *dialogue, void *data);

#endif

