/* Copyright 2003-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: edit.h
 *
 * Transaction editing implementation.
 */

#ifndef CASHBOOK_EDIT
#define CASHBOOK_EDIT

#include "account.h"
#include "transact.h"

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define EDIT_ICON_ROW 0
#define EDIT_ICON_DATE 1
#define EDIT_ICON_FROM 2
#define EDIT_ICON_FROM_REC 3
#define EDIT_ICON_FROM_NAME 4
#define EDIT_ICON_TO 5
#define EDIT_ICON_TO_REC 6
#define EDIT_ICON_TO_NAME 7
#define EDIT_ICON_REF 8
#define EDIT_ICON_AMOUNT 9
#define EDIT_ICON_DESCRIPT 10

/**
 * The different types of field available in an edit line.
 */

enum edit_field_type {
	EDIT_FIELD_DISPLAY,		/**< A display field which can not be edited.	*/
	EDIT_FIELD_TEXT,		/**< A plain text field.			*/
	EDIT_FIELD_CURRENCY,		/**< A currency field.				*/
	EDIT_FIELD_DATE,		/**< A date field.				*/
	EDIT_FIELD_ACCOUNT,		/**< An account field.				*/
};

struct edit_data_text {
	char	*text;
	size_t	length;
};

struct edit_data_currency {
	amt_t	amount;
};

struct edit_data_date {
	date_t	date;
};

struct edit_data_account {
	acct_t	account;
	osbool	reconciled;
};

struct edit_data {
	int					line;		/**< The line that we're interested in.		*/
	enum edit_field_type			type;		/**< The type of field to require the data.	*/
	union {
		struct edit_data_text		display;	/**< The data relating to a display field.	*/
		struct edit_data_text		text;		/**< The data relating to a text field.		*/
		struct edit_data_currency	currency;	/**< The data relating to a currency field.	*/
		struct edit_data_date		date;		/**< The data relating to a date field.		*/
		struct edit_data_account	account;	/**< The data relating to an account field.	*/
	};
	void					*data;		/**< The client-supplied data pointer.		*/
};

/**
 * The edit line instance block.
 */

struct edit_block;


/**
 * Create a new edit line instance.
 *
 * \param *file			The parent file.
 * \param *template		Pointer to the window template definition to use for the edit icons.
 * \param parent		The window handle in which the edit line will reside.
 * \param *columns		The column settings relating to the instance's parent window.
 * \param toolbar_height	The height of the window's toolbar.
 * \param *data			Client-specific data pointer, or NULL for none.
 * \return			The new instance handle, or NULL on failure.
 */

struct edit_block *edit_create_instance(struct file_block *file, wimp_window *template, wimp_w parent, struct column_block *columns, int toolbar_height, void *data);


/**
 * Delete an edit line instance.
 *
 * \param *instance		The instance handle to delete.
 */

void edit_delete_instance(struct edit_block *instance);


/**
 * Add a field to an edit line instance.
 * 
 * \param *instance		The instance to add the field to.
 * \param type			The type of field to add.
 * \param column		The column number of the left-most field.
 * \param ...			A list of the icons which apply to the field.
 * \return			True if the field was created OK; False on error.
 */

osbool edit_add_field(struct edit_block *instance, enum edit_field_type type, int column,
		osbool (*get)(struct edit_data *), osbool (*put)(struct edit_data *), ...);


/**
 * Place a new edit line, removing any existing instance first since there can only
 * be one input focus.
 * 
 * \param *instance		The instance to place.
 * \param line			The line to place the instance in.
 */

void edit_place_new_line(struct edit_block *instance, int line);

#ifdef LOSE

/**
 * Create an edit line at the specified point in the given file's transaction
 * window. Any existing edit line is deleted first.
 *
 * The caret isn't placed in this routine.  That is left up to the caller, so
 * that they can place it depending on their context.
 *
 * \param *instance	The edit instance to process.
 * \param line		The line to place the edit line at.
 */

void void edit_place_new_line(struct edit_block *instance, int line);


/**
 * Place a new edit line by raw transaction number.
 *
 * \param *edit		The edit instance to process.
 * \param *file		The file to place the line in.
 * \param transaction	The transaction to place the line on.
 */

void edit_place_new_line_by_transaction(struct edit_block *edit, struct file_block *file, int transaction);


/**
 * Bring the edit line into view in the window in a vertical direction.
 *
 * \param *edit		The edit line instance that we're interested in working on
 */

void edit_find_line_vertically(struct edit_block *edit);


/**
 * Refresh the contents of the edit line icons, copying the contents of memory
 * back into them.
 *
 * \param w		If NULL, refresh any window; otherwise, only refresh if
 *			the parent transaction window handle matches w.
 * \param only		If -1, refresh all icons in the line; otherwise, only
 *			refresh if the icon handle matches i.
 * \param avoid		If -1, refresh all icons in the line; otherwise, only
 *			refresh if the icon handle does not match avoid.
 */

void edit_refresh_line_content(wimp_w w, wimp_i only, wimp_i avoid);


/**
 * Get the underlying transaction number relating to the current edit line
 * position.
 *
 * \param *file		The file that we're interested in.
 * \return		The transaction number, or NULL_TRANSACTION if the
 *			line isn't in the specified file.
 */

int edit_get_line_transaction(struct file_block *file);



/**
 * Insert a preset into a pre-existing transaction, taking care of updating all
 * the file data in a clean way.
 *
 * \param *file		The file to edit.
 * \param transaction	The transaction to update.
 * \param preset	The preset to insert into the transaction.
 */

void edit_insert_preset_into_transaction(struct file_block *file, tran_t transaction, int preset);


/**
 * Handle keypresses in an edit line (and hence a transaction window). Process
 * any function keys, then pass content keys on to the edit handler.
 *
 * \param *edit		The edit instance to process.
 * \param *file		The file to pass they keys to.
 * \param *key		The Wimp's key event block.
 * \return		TRUE if the key was handled; FALSE if not.
 */

osbool edit_process_keypress(struct edit_block *edit, struct file_block *file, wimp_key *key);
#endif
#endif

