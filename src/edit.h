/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: edit.h
 *
 * Transaction editing implementation.
 */

#ifndef CASHBOOK_EDIT
#define CASHBOOK_EDIT

#include "account.h"
#include "transact.h"


/**
 * The different types of field available in an edit line.
 */

enum edit_field_type {
	EDIT_FIELD_NONE,					/**< No field.							*/
	EDIT_FIELD_DISPLAY,					/**< A display field which can not be edited.			*/
	EDIT_FIELD_TEXT,					/**< A plain text field.					*/
	EDIT_FIELD_CURRENCY,					/**< A currency field.						*/
	EDIT_FIELD_DATE,					/**< A date field.						*/
	EDIT_FIELD_ACCOUNT_IN,					/**< An incoming account field.					*/
	EDIT_FIELD_ACCOUNT_OUT					/**< An outgoing account field.					*/
};

/**
 * Data associated with a text field.
 */

struct edit_data_text {
	char					*text;		/**< Pointer to a buffer to hold the text value of the field.	*/
	size_t					length;		/**< The maximum length of the buffer.				*/
};

/**
 * Data associated with a currency field.
 */

struct edit_data_currency {
	amt_t					amount;		/**< The currency value of the field.				*/
};

/**
 * Data associated with a date field.
 */

struct edit_data_date {
	date_t					date;		/**< The date value of the field.				*/
};

/**
 * Data associated with an account field.
 */

struct edit_data_account {
	acct_t					account;	/**< The account value of the field.				*/
	osbool					reconciled;	/**< TRUE if the field has been reconciled; FALSE if not.	*/
};

/**
 * Block to hold data being transferred from the client to an edit line
 * field or back again.
 */

struct edit_data {
	int					line;		/**< The line containing the field to be transferred		*/
	wimp_i					icon;		/**< The handle of the first icon in the field.			*/
	enum edit_field_type			type;		/**< The type of field to be transferred			*/
	union {
		struct edit_data_text		display;	/**< The data relating to a display field.			*/
		struct edit_data_text		text;		/**< The data relating to a text field.				*/
		struct edit_data_currency	currency;	/**< The data relating to a currency field.			*/
		struct edit_data_date		date;		/**< The data relating to a date field.				*/
		struct edit_data_account	account;	/**< The data relating to an account field.			*/
	};
	wimp_key_no				key;		/**< The keypress resulting in the reported change, or \0.	*/
	void					*data;		/**< The client-supplied data pointer.				*/
};

/**
 * Field alignment options.
 */

enum edit_align {
	EDIT_ALIGN_NONE,					/**< Alignment not specified.					*/
	EDIT_ALIGN_LEFT,					/**< Align to the left-hand end of the field.			*/
	EDIT_ALIGN_CENTRE,					/**< Align to the centre of the field.				*/
	EDIT_ALIGN_RIGHT					/**< Align to the right-hand end of the field.			*/
};

/**
 * A set of callbacks which clients must either supply or set to NULL. These
 * will be used by the edit line to communicate with the client.
 */

struct edit_callback {
	/**
	 * Request new or updated field data from the client.
	 *
	 * \param *transfer		Pointer to the data transfer structure to take the data.
	 * \return			TRUE if data was returned; FALSE if not.
	 */

	osbool			(*get_field)(struct edit_data *transfer);

	/**
	 * Send changed field data back to the client.
	 *
	 * \param *transfer		Pointer to the data transfer structure containing the data.
	 * \return			TRUE if the new data was accepted; FALSE if not.
	 */

	osbool			(*put_field)(struct edit_data *transfer);

	/**
	 * Check with the client whether a line number is in range.
	 *
	 * \param line			The line number to be tested, in terms of current sort order.
	 * \param *data			Client-specific data.
	 * \return			TRUE if successful; FALSE on error.
	 */

	osbool			(*test_line)(int line, void *data);

	/**
	 * Request that the client places the edit line at a new location and
	 * then brings it into view.
	 *
 	 * \param line			The line number to place the edit line at.
	 * \param *data			Client-specific data.
	 * \return			TRUE if successful; FALSE on error.
	 */

	osbool			(*place_line)(int line, void *data);

	/**
	 * Request that the client adjusts the horizontal positioning of its
	 * window such that the supplied coordinates are in range. It should also
	 * ensure that the specified line is visible in the window.
	 *
	 * \param line			The line at which the edit line is located.
	 * \param xmin			The minimum X window coordinate, in OS units.
	 * \param xmax			The maximum X window coordinate, in OS units.
	 * \param target		The target alignment
	 * \param *data			Client-specific data.
	 * \return			TRUE if successful; FALSE on error.
	 */

	osbool			(*find_field)(int line, int xmin, int xmax, enum edit_align target, void *data);


	/**
	 * Request from the client the first blank line in the window, in terms
	 * of its current sort order.
	 *
	 * \param *data			Client-specific data.
	 * \return			The first blank line, or -1 on error.
	 */

	int			(*first_blank_line)(void *data);


	/**
	 * Give the client the chance to auto-sort on a Return keypress.
	 *
	 * \param icon			The handle of the first icon in the affected field.
	 * \param *data			Client-specific data.
	 * \return			TRUE on success; FALSE on failure.
	 */

	osbool			(*auto_sort)(wimp_i icon, void *data);


	/**
	 * Request that the client provide an auto-complete for a given field.
	 *
	 * \param *transfer		Pointer to the data transfer structure to take the completion data.
	 * \return			TRUE if data was returned; FALSE if not.
	 */

	osbool			(*auto_complete)(struct edit_data *transfer);

	/**
	 * Request that the client insert a preset template into a given line.
	 *
	 * \param line			The line number at which to inset the preset.
	 * \param key			The Wimp key number triggering the preset.
	 * \param *data			Client-specific data.
	 * \return			TRUE if the preset was processed; FALSE if not.
	 */

	osbool			(*insert_preset)(int line, wimp_key_no key, void *data);
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
 * \param *callbacks		Pointer to the client's callback details.
 * \param *data			Client-specific data pointer, or NULL for none.
 * \return			The new instance handle, or NULL on failure.
 */

struct edit_block *edit_create_instance(struct file_block *file, wimp_window *template, wimp_w parent, struct column_block *columns, int toolbar_height,
		struct edit_callback *callbacks, void *data);

/**
 * Delete an edit line instance.
 *
 * \param *instance		The instance handle to delete.
 */

void edit_delete_instance(struct edit_block *instance);


/**
 * Return the complete state of the edit line instance.
 *
 * \param *instance		The instance to report on.
 * \return			TRUE if all memory allocations completed OK; FALSE if any
 * 				allocations failed.
 */

osbool edit_complete(struct edit_block *instance);


/**
 * Add a field to an edit line instance.
 *
 * \param *instance		The instance to add the field to.
 * \param type			The type of field to add.
 * \param ...			A list of the icons which apply to the field.
 * \return			True if the field was created OK; False on error.
 */

osbool edit_add_field(struct edit_block *instance, enum edit_field_type type, ...);


/**
 * Place a new edit line, removing any existing instance first since there can only
 * be one input focus.
 *
 * \param *instance		The instance to place.
 * \param line			The line to place the instance in, or -1 to replace the
 *				line in its current location with the current colour.
 * \param colour		The colour to create the icon.
 */

void edit_place_new_line(struct edit_block *instance, int line, wimp_colour colour);


/**
 * Refresh the contents of an edit line, by restoring the memory contents back
 * into the icons.
 *
 * \param *instance		The instance to refresh, or NULL to refresh the
 *				currently active instance (if any).
 * \param only			If wimp_ICON_WINDOW, refresh all fields in the line; otherwise,
 *				only refresh if the field's first icon handle matches.
 * \param avoid			If wimp_ICON_WINDOW, refresh all fields in the line; otherwise, only
 *				refresh if the field's first icon handle does not match.
 */

void edit_refresh_line_contents(struct edit_block *instance, wimp_i only, wimp_i avoid);


/**
 * Request a transfer block from an instance of the edit bar, to allow a
 * field in the bar to be updated. It is essential that the instance is the
 * currently active instance; if it is not, a block will not be set up.
 *
 * It is ESSENTIAL that, if the returned block is not NULL, then the block
 * is passed back to edit_submit_field_contents_update() to allow it to be
 * deallocated. If it is not, a memory leak will occur.
 *
 * \param *instance		The edit bar instance to be updated.
 * \param icon			An icon in the field to be updated.
 * \return			Pointer to a transfer block, or NULL on failure.
 */

struct edit_data *edit_request_field_contents_update(struct edit_block *instance, wimp_i icon);


/**
 * Return a transfer block, previously claimed by calling
 * edit_request_field_contents_update(), to update a field in an edit
 * bar instance and release the memory used by the block.
 *
 * \param *instance		The edit bar instance to be updated.
 * \param *transfer		The transfer block containing the update.
 * \param caret			TRUE to place the caret in the field; otherwise FALSE.
 * \return			TRUE if the update was successful; FALSE on failure.
 */

osbool edit_submit_field_contents_update(struct edit_block *instance, struct edit_data *transfer, osbool caret);


/**
 * Return the type of field represented in an instance by a given icon.
 *
 * \param *instance		The instance to query.
 * \param icon			The icon to look up.
 * \return			The field type, or FIELD_TYPE_NONE.
 */

enum edit_field_type edit_get_field_type(struct edit_block *instance, wimp_i icon);


/**
 * Get the line currently designated the edit line in a specific instance.
 *
 * \param *instance		The instance of interest.
 * \return			The edit line, counting from zero, or -1.
 */

int edit_get_line(struct edit_block *instance);


/**
 * Determine whether an edit line instance is the active one with icons present.
 *
 * \param *instance		The instance of interest.
 * \return			TRUE if the instance is active; otherwise FALSE.
 */

osbool edit_get_active(struct edit_block *instance);


/**
 * Change the foreground colour of the icons in the edit line. This will do
 * nothing if the instance does not have the active icons.
 *
 * \param *instance		The instance to be changed.
 * \param colour		The new colour of the line.
 */

void edit_set_line_colour(struct edit_block *instance, wimp_colour colour);


/**
 * Process a keypress in an edit line icon.
 *
 * \param *instance		The edit line instance to take the keypress.
 * \param *key			The keypress event data to process.
 * \return			TRUE if the keypress was handled; FALSE if not.
 */

osbool edit_process_keypress(struct edit_block *instance, wimp_key *key);

#endif
