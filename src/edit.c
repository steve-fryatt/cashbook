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
 * \file: edit.c
 *
 * Transaction editing implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"

/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "edit.h"

#include "account.h"
#include "accview.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "presets.h"
#include "transact.h"
#include "window.h"

/**
 * The number of transfer blocks associated with an edit line instance, which determines
 * how many concurrent data get or put operations can be happening. This allows for
 * one operation to trigger another in a nested manner.
 */

#define EDIT_TRANSFER_BLOCKS 2

/**
 * The different types of icon which can form and edit line.
 */

enum edit_icon_type {
	EDIT_ICON_DISPLAY,						/**< A non-writable display field icon, which can not be edited.		*/
	EDIT_ICON_TEXT,							/**< A writable plain text field icon.						*/
	EDIT_ICON_CURRENCY,						/**< A writable currency field icon.						*/
	EDIT_ICON_DATE,							/**< A writable date field icon.						*/
	EDIT_ICON_ACCOUNT_NAME,						/**< A non-writable account field name icon.					*/
	EDIT_ICON_ACCOUNT_IDENT,					/**< A writable account field ident icon.					*/
	EDIT_ICON_ACCOUNT_REC						/**< A non-writable, clickable account field reconciliation icon.		*/
};

/**
 * Data relating to a text field.
 */

struct edit_field_text {
	int	sum;							/**< The sum (checksum/hash) of the field's text.				*/
};

/**
 * Data relating to a currency field.
 */

struct edit_field_currency {
	amt_t	amount;							/**< The currency amount held in the field.					*/
};

/**
 * Data relating to a date field.
 */

struct edit_field_date {
	date_t	date;							/**< The date held in the field.						*/
};

/**
 * Data relating to an account field.
 */

struct edit_field_account {
	acct_t	account;						/**< The account held in the field.						*/
	osbool	reconciled;						/**< TRUE if the account is reconciled; FALSE if it is not.			*/
};

/**
 * A field within an edit line.
 */

struct edit_field {
	struct edit_block	*instance;				/**< The parent edit line instance.						*/

	enum edit_field_type	type;					/**< The type of field.								*/

	struct edit_icon	*icon;					/**< The first icon in a linked list associated with the field.			*/

	union {
		struct edit_field_text		text;			/**< The data relating to a text field.						*/
		struct edit_field_currency	currency;		/**< The data relating to a currency field.					*/
		struct edit_field_date		date;			/**< The data relating to a date field.						*/
		struct edit_field_account	account;		/**< The data relating to an account field.					*/
	};

	struct edit_field	*next;					/**< Pointer to the next icon in the line, or NULL.				*/
};

/**
 * An icon within an edit line.
 */

struct edit_icon {
	struct edit_field	*parent;				/**< The parent field to which the icon belongs.				*/
	struct edit_icon	*sibling;				/**< Pointer to the next silbing icon in the field, if any (NULL if none).	*/

	enum edit_icon_type	type;					/**< The type of icon in an edit line context.					*/

	int			column;					/**< The column of the left-most part of the field.				*/

	wimp_i			icon;					/**< The wimp handle of the icon.						*/
	char			*buffer;				/**< Pointer to a buffer to hold the icon text.					*/
	size_t			length;					/**< The length of the text buffer.						*/

	struct edit_icon	*next;					/**< Pointer to the next field in the line, or NULL.				*/
};

/**
 * A data transfer block, holding the public facing data block and associated metadata.
 * Blocks are allocated to get and put operations on a first-come, first-served basis
 * to allow more than one operation to be running concurrently (such as if one operation
 * triggers a nested one).
 */

struct edit_transfer {
	osbool			free;					/**< TRUE if the block is free to be allocated; FALSE if it has been claimed.	*/
	struct edit_data	data;					/**< The data transfer block itself.						*/
};

/**
 * An edit line instance definition.
 */

struct edit_block {
	struct file_block	*file;					/**< The parent file.								*/

	wimp_window		*template;				/**< The template for the parent window.					*/
	wimp_w			parent;					/**< The parent window that the edit line belongs to.				*/
	struct column_block	*columns;				/**< The parent window column settings.						*/
	int			toolbar_height;				/**< The height of the toolbar in the parent window.				*/

	struct edit_field	*fields;				/**< The list of fields defined in the line, or NULL for none.			*/
	struct edit_icon	*icons;					/**< The list of icons defined in the line, or NULL for none.			*/

	void			*client_data;				/**< Client supplied data, for use in callbacks.				*/

	struct edit_transfer	transfer[EDIT_TRANSFER_BLOCKS];		/**< The structure used to transfer data to and from the client.		*/

	osbool			complete;				/**< TRUE if the instance is complete; FALSE if a memory allocation failed.	*/

	struct edit_callback	*callbacks;				/**< The client's callback function details.					*/

	int			edit_line;				/**< The line currently marked for entry, in terms of window lines, or -1.	*/
};


/* Global Variables. */

/**
 * Pointer to the instance currently holding the active edit line.
 */

static struct edit_block *edit_active_instance = NULL;

/* Static Function Prototypes. */

static osbool			edit_add_field_icon(struct edit_field *field, enum edit_icon_type type, wimp_i icon, char *buffer, size_t length, int column);
static osbool			edit_create_field_icon(struct edit_icon *icon, int line, wimp_colour colour);
static void			edit_move_caret_up_down(struct edit_block *instance, int direction);
static void			edit_move_caret_forward(struct edit_block *instance, wimp_key *key);
static void			edit_move_caret_back(struct edit_block *instance);
static void			edit_process_content_keypress(struct edit_block *instance, wimp_key *key);
static void			edit_process_text_field_keypress(struct edit_field *field, wimp_key *key);
static void			edit_process_date_field_keypress(struct edit_field *field, wimp_key *key);
static void			edit_process_currency_field_keypress(struct edit_field *field, wimp_key *key);
static void			edit_process_account_field_keypress(struct edit_field *field, wimp_key *key, enum account_type type);
static void			edit_get_field_content(struct edit_field *field, int line, osbool complete);
static void			edit_delete_line_content(struct edit_block *instance);
static void			edit_refresh_field_contents(struct edit_field *field);
static struct edit_data		*edit_get_field_transfer(struct edit_field *field, int line, osbool complete);
static void			edit_put_field_content(struct edit_field *field, int line, wimp_key_no key);
static osbool			edit_get_field_icons(struct edit_field *field, int icons, ...);
static osbool			edit_find_field_horizontally(struct edit_field *field);
static osbool			edit_callback_test_line(struct edit_block *instance, int line);
static osbool			edit_callback_place_line(struct edit_block *instance, int line);
static int			edit_callback_first_blank_line(struct edit_block *instance);
static osbool			edit_callback_auto_sort(struct edit_field *field);
static osbool			edit_callback_insert_preset(struct edit_block *instance, int line, wimp_key_no key);
static int			edit_sum_field_text(char *text, size_t length);
static struct edit_field	*edit_find_caret(struct edit_block *instance);
static struct edit_field	*edit_find_last_field(struct edit_field *field);
static struct edit_field	*edit_find_next_field(struct edit_field *field);
static struct edit_field	*edit_find_previous_field(struct edit_field *field);
static struct edit_field	*edit_find_first_field(struct edit_field *field);
static osbool			edit_is_field_writable(struct edit_field *field);
static void			edit_initialise_transfer_blocks(struct edit_block *instance);
static struct edit_data		*edit_get_transfer_block(struct edit_block *instance);
static void			edit_free_transfer_block(struct edit_block *instance, struct edit_data *transfer);


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
		struct edit_callback *callbacks, void *data)
{
	struct edit_block *new;

	new = heap_alloc(sizeof(struct edit_block));
	if (new == NULL)
		return NULL;

	new->file = file;
	new->template = template;
	new->parent = parent;
	new->columns = columns;
	new->toolbar_height = toolbar_height;
	new->client_data = data;
	new->complete = TRUE;
	new->callbacks = callbacks;

	edit_initialise_transfer_blocks(new);

	/* No fields are defined as yet. */

	new->fields = NULL;
	new->icons = NULL;

	new->edit_line = -1;

	return new;
}


/**
 * Delete an edit line instance, and free all of its resources.
 *
 * \param *instance		The instance handle to delete.
 */

void edit_delete_instance(struct edit_block *instance)
{
	struct edit_field	*currentf, *nextf;
	struct edit_icon	*currenti, *nexti;

	if (instance == NULL)
		return;

	// \TODO -- Should this also delete the icons?

	if (edit_active_instance == instance)
		edit_active_instance = NULL;

	/* Delete the field definitions. */

	currentf = instance->fields;

	while (currentf != NULL) {
		nextf = currentf->next;
		heap_free(currentf);
		currentf = nextf;
	}

	/* Delete the icon definitions. */

	currenti = instance->icons;

	while (currenti != NULL) {
		nexti = currenti->next;
		heap_free(currenti);
		currenti = nexti;
	}

	/* Delete the instance itself. */

	heap_free(instance);
}


/**
 * Return the complete state of the edit line instance.
 *
 * \param *instance		The instance to report on.
 * \return			TRUE if all memory allocations completed OK; FALSE if any
 * 				allocations failed.
 */

osbool edit_complete(struct edit_block *instance)
{
	if (instance == NULL)
		return FALSE;

	return instance->complete;
}


/**
 * Add a field to an edit line instance.
 * 
 * \param *instance		The instance to add the field to.
 * \param type			The type of field to add.
 * \param column		The column number of the left-most field.
 * \param ...			A list of the icons which apply to the field.
 * \return			True if the field was created OK; False on error.
 */

osbool edit_add_field(struct edit_block *instance, enum edit_field_type type, int column, ...)
{
	va_list			ap;
	struct edit_field	*field;

	if (instance == NULL || instance->complete == FALSE)
		return FALSE;

	/* Allocate storage for the field data. */

	field = heap_alloc(sizeof(struct edit_field));
	if (field == NULL) {
		instance->complete = FALSE;
		return FALSE;
	}

	/* Link the new field into the instance field list. */

	field->next = instance->fields;
	instance->fields = field;

	field->icon = NULL;

	field->instance = instance;

	/* Set up the field data. */

	field->type = type;

	va_start(ap, column);

	switch (type) {
	case EDIT_FIELD_DISPLAY:
		edit_add_field_icon(field, EDIT_ICON_DISPLAY, va_arg(ap, wimp_i), va_arg(ap, char *), va_arg(ap, size_t), column);
		break;
	case EDIT_FIELD_TEXT:
		edit_add_field_icon(field, EDIT_ICON_TEXT, va_arg(ap, wimp_i), va_arg(ap, char *), va_arg(ap, size_t), column);
		break;
	case EDIT_FIELD_CURRENCY:
		edit_add_field_icon(field, EDIT_ICON_CURRENCY, va_arg(ap, wimp_i), va_arg(ap, char *), va_arg(ap, size_t), column);
		break;
	case EDIT_FIELD_DATE:
		edit_add_field_icon(field, EDIT_ICON_DATE, va_arg(ap, wimp_i), va_arg(ap, char *), va_arg(ap, size_t), column);
		break;
	case EDIT_FIELD_ACCOUNT_IN:
	case EDIT_FIELD_ACCOUNT_OUT:
		edit_add_field_icon(field, EDIT_ICON_ACCOUNT_IDENT, va_arg(ap, wimp_i), va_arg(ap, char *), va_arg(ap, size_t), column);
		edit_add_field_icon(field, EDIT_ICON_ACCOUNT_REC, va_arg(ap, wimp_i), va_arg(ap, char *), va_arg(ap, size_t), column + 1);
		edit_add_field_icon(field, EDIT_ICON_ACCOUNT_NAME, va_arg(ap, wimp_i), va_arg(ap, char *), va_arg(ap, size_t), column + 2);
		break;
	}

	va_end(ap);

	return instance->complete;
}


/**
 * Add an icon definition to an edit line field. This does not create the
 * icon itself.
 * 
 * \param *field		The field to which the icon will belong.
 * \param type			The type of icon being created.
 * \param icon			The expected Wimp icon handle for the icon.
 * \param *buffer		Pointer to the buffer to hold the icon contents.
 * \param *length		The length of the supplied icon buffer.
 * \param column		The window column to which the icon belongs.
 * \return			TRUE on success; FALSE on error or failure.
 */

static osbool edit_add_field_icon(struct edit_field *field, enum edit_icon_type type, wimp_i icon, char *buffer, size_t length, int column)
{
	struct edit_icon	*new, *list;

	if (field == NULL || field->instance == NULL || field->instance->complete == FALSE)
		return FALSE;

	new = heap_alloc(sizeof(struct edit_icon));
	if (new == NULL) {
		field->instance->complete = FALSE;
		return FALSE;
	}

	new->type = type;
	new->parent = field;
	new->icon = icon;
	new->buffer = buffer;
	new->length = length;
	new->column = column;

	/* Link the icon to the end of it's parent's list of sibling icons. */

	new->sibling = NULL;

	if (field->icon == NULL) {
		field->icon = new;
	} else {
		list = field->icon;

		while (list->sibling != NULL)
			list = list->sibling;

		list->sibling = new;
	}

	/* Link the icon into the bar's icon list. If the existing icon list is empty,
	 * or the first icon has a higher icon handle than this one, put the new icon
	 * at the head of the list.
	 */

	if (field->instance->icons == NULL || field->instance->icons->icon > icon) {
		new->next = field->instance->icons;
		field->instance->icons = new;
		return FALSE;
	}

	/* Otherwise, insert the icon into the correct place in the list. */

	list = field->instance->icons;

	while (list->next != NULL && list->next->icon < icon)
		list = list->next;

	new->next = list->next;
	list->next = new;

	return TRUE;
}


/**
 * Place a new edit line, removing any existing instance first since there can only
 * be one input focus.
 * 
 * \param *instance		The instance to place.
 * \param line			The line to place the instance in, or -1 to replace the
 *				line in its current location with the current colour.
 * \param colour		The colour to create the icon.
 */

void edit_place_new_line(struct edit_block *instance, int line, wimp_colour colour)
{
	struct edit_icon	*icon;
	struct edit_field	*field;
	wimp_icon_state		state;
	os_error		*error;

	if (instance == NULL)
		return;

	/* If no line is specified and the edit line is already in this instance,
	 * simply replace the line in the same location. This allows the icons to
	 * be adjusted for a change in column widths, for example.
	 */

	if (line == -1) {
		if (instance != edit_active_instance)
			return;

		/* The line doesn't move. */

		line = instance->edit_line;

		/* Get the foreground colour from the first icon in the line. */

		if (instance->icons != NULL) {
			state.w = instance->parent;
			state.i = instance->icons->icon;
			error = xwimp_get_icon_state(&state);
			if (error == NULL)
				colour = (state.icon.flags & wimp_ICON_FG_COLOUR) >> wimp_ICON_FG_COLOUR_SHIFT;
		}
	}

	/* Delete any active edit line icons. The assumption is that the data will
	 * be safe as it's always copied into memory as soon as a key is pressed
	 * in any of the writable icons...
	 */

	if (edit_active_instance != NULL) {
		icon = edit_active_instance->icons;

		while (icon != NULL) {
			wimp_delete_icon(edit_active_instance->parent, icon->icon);
			icon = icon->next;
		}

		edit_active_instance->edit_line = -1;
		edit_active_instance = NULL;
	}

	/* Set the active instance so that the icon creation knows that it's OK. */

	instance->edit_line = line;
	edit_active_instance = instance;

	/* Create the new edit line icons. */

	icon = instance->icons;

	while (icon != NULL) {
		if (!edit_create_field_icon(icon, line, colour)) {
			edit_active_instance->edit_line = -1;
			edit_active_instance = NULL;
			return;
		}
		icon = icon->next;
	}

	/* Get the field data. */

	field = instance->fields;

	while (field != NULL) {
		edit_get_field_content(field, line, FALSE);
		field = field->next;
	}
}


/**
 * Create a new edit line field icon. If a Wimp error occurs, an error box
 * will be displayed to the user before returning.
 *
 * \param *icon			The icon to be created.
 * \param line			The window line on which to create it.
 * \param colour		The colour to create the icon.
 * \return			TRUE if successful; FALSE on error.
 */

static osbool edit_create_field_icon(struct edit_icon *icon, int line, wimp_colour colour)
{
	wimp_icon_create	icon_block;
	wimp_i			handle;
	os_error		*error;

	if (icon == NULL || icon->parent == NULL || icon->parent->instance == NULL || icon->parent->instance->columns == NULL)
		return FALSE;

	if (icon->parent->instance != edit_active_instance)
		return FALSE;

	icon_block.w = icon->parent->instance->parent;
	memcpy(&(icon_block.icon), &(icon->parent->instance->template->icons[icon->icon]), sizeof(wimp_icon));

	icon_block.icon.data.indirected_text.text = icon->buffer;
	icon_block.icon.data.indirected_text.size = icon->length;

	icon_block.icon.extent.x0 = icon->parent->instance->columns->position[icon->column];
	icon_block.icon.extent.x1 = icon->parent->instance->columns->position[icon->column] + icon->parent->instance->columns->width[icon->column];
	icon_block.icon.extent.y0 = WINDOW_ROW_Y0(icon->parent->instance->toolbar_height, line);
	icon_block.icon.extent.y1 = WINDOW_ROW_Y1(icon->parent->instance->toolbar_height, line);

	icon_block.icon.flags &= ~wimp_ICON_FG_COLOUR;
	icon_block.icon.flags |= colour << wimp_ICON_FG_COLOUR_SHIFT;

	error = xwimp_create_icon(&icon_block, &handle);
	if (error != NULL) {
		error_msgs_param_report_error("EditFailIcon", error->errmess, NULL, NULL, NULL);
		return FALSE;
	}
	
	if (handle != icon->icon) {
		error_msgs_report_error("EditBadIconHandle");
		return FALSE;
	}

	return TRUE;
}


/**
 * Refresh the contents of an edit line, by restoring the memory contents back
 * into the icons.
 *
 * \param *instance		The instance to refresh, or NULL to refresh the
 *				currently active instance (if any).
 * \param only			If -1, refresh all fields in the line; otherwise,
 *				only refresh if the field's first icon handle matches.
 * \param avoid			If -1, refresh all fields in the line; otherwise, only
 *				refresh if the field's first icon handle does not match.
 */

void edit_refresh_line_contents(struct edit_block *instance, wimp_i only, wimp_i avoid)
{
	struct edit_field	*field;
	struct edit_icon	*icon;

	if (instance == NULL)
		instance = edit_active_instance;

	if (instance == NULL || instance->file == NULL || !edit_callback_test_line(instance, instance->edit_line))
		return;

	if (instance != edit_active_instance)
		return;

	/* Get the field data and update the icons. */

	field = instance->fields;

	while (field != NULL) {
		icon = field->icon;

		if ((icon != NULL) && (only == -1 || only == icon->icon) && (avoid != icon->icon)) {
			edit_get_field_content(field, instance->edit_line, FALSE);

			while (icon != NULL) {
				wimp_set_icon_state(instance->parent, icon->icon, 0, 0);
				icon = icon->sibling;
			}
		}

		field = field->next;
	}
}


/**
 * Get the line currently designated the edit line in a specific instance.
 *
 * \param *instance		The instance of interest.
 * \return			The edit line, counting from zero, or -1.
 */

int edit_get_line(struct edit_block *instance)
{
	if (instance == NULL)
		return -1;

	return instance->edit_line;
}


/**
 * Determine whether an edit line instance is the active one with icons present.
 *
 * \param *instance		The instance of interest.
 * \return			TRUE if the instance is active; otherwise FALSE.
 */

osbool edit_get_active(struct edit_block *instance)
{
	return (edit_active_instance == instance) ? TRUE : FALSE;
}


/**
 * Change the foreground colour of the icons in the edit line. This will do
 * nothing if the instance does not have the active icons.
 *
 * \param *instance		The instance to be changed.
 * \param colour		The new colour of the line.
 */

void edit_set_line_colour(struct edit_block *instance, wimp_colour colour)
{
	struct edit_icon	*icon;
	os_error		*error;

	if (instance == NULL || instance != edit_active_instance || instance->edit_line == -1)
		return;

	/* Update the icon flags to change the colour. */

	icon = instance->icons;

	while (icon != NULL) {
		error = xwimp_set_icon_state(instance->parent, icon->icon, colour << wimp_ICON_FG_COLOUR_SHIFT, wimp_ICON_FG_COLOUR);

		if (error != NULL) {
			error_msgs_param_report_error("EditFailIcon", error->errmess, NULL, NULL, NULL);
			return;
		}

		icon = icon->next;
	}
}


/**
 * Process a keypress in an edit line icon.
 *
 * \param *instance		The edit line instance to take the keypress.
 * \param *key			The keypress event data to process.
 * \return			TRUE if the keypress was handled; FALSE if not.
 */

osbool edit_process_keypress(struct edit_block *instance, wimp_key *key)
{
	if (instance == NULL || instance->file == NULL || instance->parent != key->w || instance != edit_active_instance)
		return FALSE;

	if (key->c == wimp_KEY_CONTROL + wimp_KEY_F10) {
		/* Ctrl-F10 deletes the whole line. */

		edit_delete_line_content(instance);
	} else if (key->c == wimp_KEY_UP) {
		edit_move_caret_up_down(instance, -1);
	} else if (key->c == wimp_KEY_DOWN) {
		edit_move_caret_up_down(instance, +1);
	} else if (key->c == wimp_KEY_RETURN || key->c == wimp_KEY_TAB || key->c == wimp_KEY_TAB + wimp_KEY_CONTROL) {
		/* Return and Tab keys -- move the caret into the next icon,
		 * refreshing the icon it was moved from first.  Wrap at the end
		 * of a line.
		 */

		 edit_move_caret_forward(instance, key);
	} else if (key->c == (wimp_KEY_TAB + wimp_KEY_SHIFT)) {
		/* Shift-Tab key -- move the caret back to the previous icon,
		 * refreshing the icon moved from first.  Wrap up at the start
		 * of a line.
		 */

		edit_move_caret_back(instance);
	} else if (key->c == wimp_KEY_F12 ||
			key->c == (wimp_KEY_SHIFT + wimp_KEY_F12)  ||
			key->c == (wimp_KEY_CONTROL + wimp_KEY_F12) ||
			key->c == (wimp_KEY_SHIFT + wimp_KEY_CONTROL + wimp_KEY_F12)) {

		/* We don't want to claim any of the F12 combinations. */

		return FALSE;
	} else {
		/* Any unrecognised keys get passed on to the final stage. */

		edit_process_content_keypress(instance, key);
	}

	return TRUE;
}


/**
 * Move the edit line up or down a row.
 *
 * \param *instance		The edit line instance to be moved.
 * \param direction		The direction to move the line: positive for
 *				up, or negative for down.
 */

static void edit_move_caret_up_down(struct edit_block *instance, int direction)
{
	wimp_caret	caret;

	if (instance == NULL || instance != edit_active_instance)
		return;

	/* Establish the direction that we're moving in. */

	if (direction < 0) {
		direction = -1;

		if (instance->edit_line < 1)
			return;
	} else if (direction > 0) {
		direction = 1;
	} else {
		return;
	}

	wimp_get_caret_position(&caret);
	edit_refresh_line_contents(instance, caret.i, -1);
	edit_callback_place_line(instance, instance->edit_line + direction);
	wimp_set_caret_position(caret.w, caret.i, caret.pos.x, caret.pos.y + (direction * WINDOW_ROW_HEIGHT), -1, -1);
}


/**
 * Move the caret on to the next field in a line.
 * 
 * \param *instance		The edit line instance to be updated.
 * \param *key			The Wimp keypress data associated with the request.
 */

static void edit_move_caret_forward(struct edit_block *instance, wimp_key *key)
{
	struct edit_field	*field, *next;
	struct edit_data	*transfer;
	int			line;

	if (instance == NULL || instance != edit_active_instance)
		return;

	field = edit_find_caret(instance);
	if (field == NULL || field->icon == NULL)
		return;

	/* If Ctrl is pressed, copy the field contents down from the line above. */

	if ((osbyte1(osbyte_SCAN_KEYBOARD, 129, 0) == 0xff) && (instance->edit_line > 0)) {
		transfer = edit_get_field_transfer(field, instance->edit_line - 1, FALSE);

		if (transfer != NULL && instance->callbacks != NULL && instance->callbacks->put_field != NULL) {
			transfer->line = instance->edit_line;
			transfer->key = key->c;
			instance->callbacks->put_field(transfer);
			edit_free_transfer_block(instance, transfer);
		}
	}

	/* Refresh the icon that we're about to move from. */

	edit_refresh_line_contents(instance, field->icon->icon, -1);

	/* If Return was pressed, let the client run an auto-sort if it wishes. */

	if (key->c == wimp_KEY_RETURN)
		edit_callback_auto_sort(field);

	/* Find the next field and move in to it. */

	next = edit_find_next_field(field);

	if (next != NULL) {
		icons_put_caret_at_end(instance->parent, next->icon->icon);
		edit_find_field_horizontally(next);
	} else {
		next = edit_find_first_field(field);

		if (next != NULL) {
			line = (key->c == wimp_KEY_RETURN) ? edit_callback_first_blank_line(instance) : -1;
			if (line == -1)
				line = instance->edit_line + 1;

			edit_callback_place_line(instance, line);
			icons_put_caret_at_end(instance->parent, next->icon->icon);
			edit_find_field_horizontally(next);
		}
	}
}


/**
 * Move the caret back to the previous field in a line.
 * 
 * \param *instance		The edit line instance to be updated.
 */

static void edit_move_caret_back(struct edit_block *instance)
{
	struct edit_field	*field, *previous;

	if (instance == NULL || instance != edit_active_instance)
		return;

	field = edit_find_caret(instance);
	if (field == NULL || field->icon == NULL)
		return;

	edit_refresh_line_contents(instance, field->icon->icon, -1);

	previous = edit_find_previous_field(field);

	if (previous != NULL) {
		icons_put_caret_at_end(instance->parent, previous->icon->icon);
		edit_find_field_horizontally(previous);
	} else if (instance->edit_line > 0) {
		previous = edit_find_last_field(field);

		if (previous != NULL) {
			edit_callback_place_line(instance, instance->edit_line - 1);
			icons_put_caret_at_end(instance->parent, previous->icon->icon);
			edit_find_field_horizontally(previous);
		}
	}
}


/**
 * Process possible content-editing keypresses in the edit line.
 *
 * \param *instance		The edit line instance accepting the keypress.
 * \param *key			The Wimp's key event data block.
 */

static void edit_process_content_keypress(struct edit_block *instance, wimp_key *key)
{
	struct edit_field	*field;


	if (instance == NULL || instance->parent != key->w || instance != edit_active_instance)
		return;

	/* Find the field which owns the caret icon. If a field isn't found, return. */

	field = instance->fields;

	while (field != NULL) {
		if (field->icon != NULL && field->icon->icon == key->i)
			break;

		field = field->next;
	}

	if (field == NULL)
		return;

	/* Process the keypress for the field. */

	switch (field->type) {
	case EDIT_FIELD_TEXT:
		edit_process_text_field_keypress(field, key);
		break;
	case EDIT_FIELD_DATE:
		edit_process_date_field_keypress(field, key);
		break;
	case EDIT_FIELD_CURRENCY:
		edit_process_currency_field_keypress(field, key);
		break;
	case EDIT_FIELD_ACCOUNT_IN:
		edit_process_account_field_keypress(field, key, ACCOUNT_IN | ACCOUNT_FULL);
		break;
	case EDIT_FIELD_ACCOUNT_OUT:
		edit_process_account_field_keypress(field, key, ACCOUNT_OUT | ACCOUNT_FULL);
		break;
	case EDIT_FIELD_DISPLAY:
		break;
	}
}


/**
 * Process keypresses in a text field, passing any changes made back to
 * the client.
 *
 * \param *field		The field to take the keypress.
 * \param *key			The keypress data from the Wimp.
 */

static void edit_process_text_field_keypress(struct edit_field *field, wimp_key *key)
{
	int	new_sum;

	if (field == NULL || field->icon == NULL || key == NULL)
		return;

	/* If F1 is pressed, ask the client to help with an autocomplete. */

	if (key->c == wimp_KEY_F1) {
		edit_get_field_content(field, field->instance->edit_line, TRUE);
		wimp_set_icon_state(key->w, field->icon->icon, 0, 0);
		edit_put_field_content(field, field->instance->edit_line, key->c);
		icons_replace_caret_in_window(key->w);
	}

	/* Calculate the new sum from the field; if it's unchanged, exit. */

	new_sum = edit_sum_field_text(field->icon->buffer, field->icon->length);

	if (new_sum == field->text.sum)
		return;

	/* Save the new sum and tell the client. */

	field->text.sum = new_sum;

	edit_put_field_content(field, field->instance->edit_line, key->c);
}


/**
 * Process keypresses in a date field, passing any changes made back to
 * the client.
 *
 * \param *field		The field to take the keypress.
 * \param *key			The keypress data from the Wimp.
 */

static void edit_process_date_field_keypress(struct edit_field *field, wimp_key *key)
{
	struct edit_data	*transfer;
	date_t			new_date, previous_date;

	if (field == NULL || field->instance == NULL || field->icon == NULL || key == NULL)
		return;

	/* Alpha key presses in a date field are handled as preset shortcuts. */

	if (isalpha(key->c)) {
		edit_callback_insert_preset(field->instance, field->instance->edit_line, key->c);
		return;
	}

	/* If F1 is pressed, insert the current date. */

	if (key->c == wimp_KEY_F1) {
		date_convert_to_string(date_today(), field->icon->buffer, field->icon->length);
		wimp_set_icon_state(key->w, field->icon->icon, 0, 0);
		icons_replace_caret_in_window(key->w);
	}

	/* If there's a Get callback, use it to get the previous line's date for autocompletion. */

	transfer = edit_get_field_transfer(field, field->instance->edit_line - 1, FALSE);
	previous_date = (transfer != NULL) ? transfer->date.date : NULL_DATE;
	edit_free_transfer_block(field->instance, transfer);

	/* Calculate the date from the field text; if it's unchanced, exit. */

	new_date = date_convert_from_string(field->icon->buffer, previous_date, 0);

	if (new_date == field->date.date)
		return;

	/* Save the new date and tell the client. */

	field->date.date = new_date;

	edit_put_field_content(field, field->instance->edit_line, key->c);
}


/**
 * Process keypresses in a currency field, passing any changes made back to
 * the client.
 *
 * \param *field		The field to take the keypress.
 * \param *key			The keypress data from the Wimp.
 */

static void edit_process_currency_field_keypress(struct edit_field *field, wimp_key *key)
{
	amt_t	new_amount;

	if (field == NULL || field->icon == NULL || key == NULL)
		return;

	/* Calculate the new amount from the field text; if it's unchaned, exit. */

	new_amount = currency_convert_from_string(field->icon->buffer);

	if (new_amount == field->currency.amount)
		return;

	/* Save the new amount and tell the client. */

	field->currency.amount = new_amount;

	edit_put_field_content(field, field->instance->edit_line, key->c);
}


/**
 * Process keypresses in an account field, passing any changes made back to
 * the client.
 *
 * \param *field		The field to take the keypress.
 * \param *key			The keypress data from the Wimp.
 * \param type			The types of account applicable to the field.
 */

static void edit_process_account_field_keypress(struct edit_field *field, wimp_key *key, enum account_type type)
{
	acct_t	new_account;
	bool	new_reconciled;
	wimp_i	ident, reconcile, name;

	if (field == NULL || field->icon == NULL || key == NULL)
		return;

	/* Identify the field icon handles */

	if (!edit_get_field_icons(field, 3, &ident, &reconcile, &name))
		return;

	/* Look up the new account and reconciled status; if it's unchanged, exit. */

	new_account = account_lookup_field(field->instance->file, key->c, type, field->account.account, &new_reconciled,
			key->w, ident, name, reconcile);

	if (new_account == field->account.account && new_reconciled == field->account.reconciled)
		return;

	/* Save the new account and reconciled status, and tell the client. */

	field->account.account = new_account;
	field->account.reconciled = new_reconciled;

	edit_put_field_content(field, field->instance->edit_line, key->c);
}


/**
 * Get a field's contents for a given line from the client, and update the
 * icons within it. The icons are not redrawn if they already exist.
 *
 * \param *field		The field to be updated.
 * \param line			The window line to update for.
 * \param complete		TRUE to perform an autocomplete; FALSE for a get.
 */

static void edit_get_field_content(struct edit_field *field, int line, osbool complete)
{
	struct edit_data	*transfer;
	struct edit_icon	*icon;

	if (field == NULL || field->instance == NULL || field->icon == NULL)
		return;

	/* Get the first icon block associated with the field. */

	icon = field->icon;

	/* Get a transfer block from the client for the field. */

	transfer = edit_get_field_transfer(field, line, complete);

	/* If no data was available, set the field to its defaults and exit. */

	if (transfer == NULL && complete == TRUE) {
		return;
	} else if (transfer == NULL) {
		switch (field->type) {
		case EDIT_FIELD_TEXT:
			field->text.sum = 0;
			break;
		case EDIT_FIELD_CURRENCY:
			field->currency.amount = NULL_CURRENCY;
			break;
		case EDIT_FIELD_DATE:
			field->date.date = NULL_DATE;
			break;
		case EDIT_FIELD_ACCOUNT_IN:
		case EDIT_FIELD_ACCOUNT_OUT:
			field->account.account = NULL_ACCOUNT;
			field->account.reconciled = FALSE;
			break;
		case EDIT_FIELD_DISPLAY:
			break;
		}

		while (icon != NULL) {
			if (icon->buffer != NULL && icon->length > 0)
				*(icon->buffer) = '\0';

			icon = icon->sibling;
		}

		return;
	}

	/* Something valid was returned in the data block, so update the field icons
	 * accordingly.
	 */

	switch (field->type) {
	case EDIT_FIELD_DISPLAY:
		break;
	case EDIT_FIELD_TEXT:
		field->text.sum = edit_sum_field_text(icon->buffer, icon->length);
		break;
	case EDIT_FIELD_CURRENCY:
		field->currency.amount = transfer->currency.amount;
		break;
	case EDIT_FIELD_DATE:
		field->date.date = transfer->date.date;
		break;
	case EDIT_FIELD_ACCOUNT_IN:
	case EDIT_FIELD_ACCOUNT_OUT:
		field->account.account = transfer->account.account;
		field->account.reconciled = transfer->account.reconciled;
		break;
	}

	edit_free_transfer_block(field->instance, transfer);

	edit_refresh_field_contents(field);
}


/**
 * If transaction deletion is enabled, delete the contents of the edit line
 * in the supplied instance field by field, writing the data back to the
 * client as we go. The underlying data will remain in place, but will be
 * blank.
 *
 * \param *instance		The edit line instance to delete the contents of.
 */

static void edit_delete_line_content(struct edit_block *instance)
{
	struct edit_field	*field;
	struct edit_icon	*icon;
	osbool			changed;

	if (instance == NULL || instance->file == NULL || instance != edit_active_instance)
		return;

	if (!config_opt_read("AllowTransDelete"))
		return;

	/* Get the field data and update the icons. */

	field = instance->fields;

	while (field != NULL) {
		icon = field->icon;

		changed = FALSE;

		switch (field->type) {
		case EDIT_FIELD_TEXT:
			if (icon == NULL || icon->length == 0 || *(icon->buffer) == '\0')
				break;

			*(icon->buffer) = '\0';
			field->text.sum = edit_sum_field_text(icon->buffer, icon->length);
			changed = TRUE;
			break;
		
		case EDIT_FIELD_DATE:
			if (field->date.date == NULL_DATE)
				break;

			field->date.date = NULL_DATE;
			changed = TRUE;
			break;
		
		case EDIT_FIELD_CURRENCY:
			if (field->currency.amount == NULL_CURRENCY)
				break;

			field->currency.amount = NULL_CURRENCY;
			changed = TRUE;
			break;
		
		case EDIT_FIELD_ACCOUNT_IN:
		case EDIT_FIELD_ACCOUNT_OUT:
			if (field->account.account == NULL_ACCOUNT && field->account.reconciled == FALSE)
				break;

			field->account.account = NULL_ACCOUNT;
			field->account.reconciled = FALSE;
			changed = TRUE;
			break;

		case EDIT_FIELD_DISPLAY:
			break;
		}

		if (changed == TRUE) {
			edit_put_field_content(field, instance->edit_line, wimp_KEY_CONTROL + wimp_KEY_F10);
			edit_refresh_field_contents(field);

			while (icon != NULL) {
				wimp_set_icon_state(instance->parent, icon->icon, 0, 0);
				icon = icon->sibling;
			}
		}

		field = field->next;
	}
}


/**
 * Refresh the icons in a field from the field's data, where this is held
 * in a separate copy from the field's icon buffer.
 * 
 * \param *field		The field to be updated.
 */

static void edit_refresh_field_contents(struct edit_field *field)
{
	wimp_i	name, ident, reconcile;

	if (field == NULL || field->icon == NULL)
		return;

	switch (field->type) {
	case EDIT_FIELD_DISPLAY:
	case EDIT_FIELD_TEXT:
		/* The supplied pointer was direct to the icon's buffer, so there's nothing to do. */
		break;
	case EDIT_FIELD_CURRENCY:
		currency_convert_to_string(field->currency.amount, field->icon->buffer, field->icon->length);
		break;
	case EDIT_FIELD_DATE:
		date_convert_to_string(field->date.date, field->icon->buffer, field->icon->length);
		break;
	case EDIT_FIELD_ACCOUNT_IN:
	case EDIT_FIELD_ACCOUNT_OUT:
		if (!edit_get_field_icons(field, 3, &ident, &reconcile, &name))
			break;
		account_fill_field(field->instance->file, field->account.account, field->account.reconciled,
				field->instance->parent, ident, name, reconcile);
		break;
	}
}


/**
 * Set up a transfer block and call the client's Get or Auto Complete callback
 * (if present) to request data for a field at a given line. The transfer block
 * is returned, or NULL if there was no callback or no valid data returned.
 *
 * \param *field		The field to request data for.
 * \param line			The line to request data for.
 * \param complete		TRUE to request an autocomplete; FALSE for a get.
 * \return			Pointer to the completed transfer block, or NULL.
 */

static struct edit_data *edit_get_field_transfer(struct edit_field *field, int line, osbool complete)
{
	struct edit_data	*transfer;
	osbool			(*callback)(struct edit_data *);


	if (field == NULL || field->instance == NULL || field->icon == NULL || field->instance->callbacks == NULL)
		return NULL;

	callback = (complete == TRUE) ? field->instance->callbacks->auto_complete : field->instance->callbacks->get_field;

	if (callback == NULL)
		return NULL;

	transfer = edit_get_transfer_block(field->instance);
	if (transfer == NULL)
		return NULL;

	transfer->type = field->type;
	transfer->line = line;
	transfer->icon = field->icon->icon;
	transfer->key = '\0';

	switch (field->type) {
	case EDIT_FIELD_DISPLAY:
		transfer->display.text = field->icon->buffer;
		transfer->display.length = field->icon->length;
		break;
	case EDIT_FIELD_TEXT:
		transfer->text.text = field->icon->buffer;
		transfer->text.length = field->icon->length;
		break;
	case EDIT_FIELD_CURRENCY:
		transfer->currency.amount = field->currency.amount;
	case EDIT_FIELD_DATE:
		transfer->date.date = field->date.date;
	case EDIT_FIELD_ACCOUNT_IN:
	case EDIT_FIELD_ACCOUNT_OUT:
		transfer->account.account = field->account.account;
		transfer->account.reconciled = field->account.reconciled;
		break;
	}

	if (!callback(transfer)) {
		edit_free_transfer_block(field->instance, transfer);
		return NULL;
	}

	return transfer;
}


/**
 * Put a field's contents back to the client. The field is not updated or
 * redrawn.
 *
 * \param *field		The field to be updated.
 * \param line			The window line to update for.
 * \param key			The key number relating to the update.
 */

static void edit_put_field_content(struct edit_field *field, int line, wimp_key_no key)
{
	struct edit_data	*transfer;
	struct edit_icon	*icon;

	if (field == NULL || field->instance == NULL || field->icon == NULL)
		return;

	if (field->instance->callbacks == NULL || field->instance->callbacks->put_field == NULL)
		return;

	/* Get the first icon block associated with the field. */

	icon = field->icon;

	/* Initialise the transfer data block. */

	transfer = edit_get_transfer_block(field->instance);
	if (transfer == NULL)
		return;

	transfer->type = field->type;
	transfer->line = line;
	transfer->icon = icon->icon;
	transfer->key = key;

	switch (field->type) {
	case EDIT_FIELD_DISPLAY:
		transfer->display.text = icon->buffer;
		transfer->display.length = icon->length;
		break;
	case EDIT_FIELD_TEXT:
		transfer->text.text = icon->buffer;
		transfer->text.length = icon->length;
		break;
	case EDIT_FIELD_CURRENCY:
		transfer->currency.amount = field->currency.amount;
		break;
	case EDIT_FIELD_DATE:
		transfer->date.date = field->date.date;
		break;
	case EDIT_FIELD_ACCOUNT_IN:
	case EDIT_FIELD_ACCOUNT_OUT:
		transfer->account.account = field->account.account;
		transfer->account.reconciled = field->account.reconciled;
		break;
	}

	/* Call the Put callback. */

	field->instance->callbacks->put_field(transfer);

	edit_free_transfer_block(field->instance, transfer);
}


/**
 * Return a set of icon handles associated with an edit bar field.
 * 
 * \param *field		The field of interest.
 * \param icons			The number of icons that are expected.
 * \param ...			Pointers to icon handle variables for each icon.
 * \return			TRUE if successful; FALSE if something went wrong.
 */

static osbool edit_get_field_icons(struct edit_field *field, int icons, ...)
{
	va_list			ap;
	struct edit_icon	*icon;
	wimp_i			*handle;

	if (field == NULL || field->icon == NULL)
		return FALSE;

	icon = field->icon;

	va_start(ap, icons);

	while (icons > 0 && icon != NULL) {
		handle = va_arg(ap, wimp_i *);

		*handle = icon->icon;

		icon = icon->sibling;
		icons--;
	}

	va_end(ap);

	return (icon != NULL || icons != 0) ? FALSE : TRUE;
}


/**
 * Bring a field in an edit line into view horizontally.
 *
 * \param *field		The field to be brought into view.
 */

static osbool edit_find_field_horizontally(struct edit_field *field)
{
	int			xmin, xmax;
	enum edit_align		alignment;
	struct edit_icon	*icon;

	if (field == NULL || field->instance == NULL || field->instance->callbacks == NULL || field->instance->callbacks->find_field == NULL)
		return FALSE;

	/* Identify the first icon and set XMin and XMax to the windown borders. */

	icon = field->icon;
	if (icon == NULL)
		return FALSE;

	xmin = column_get_window_width(field->instance->columns);
	xmax = 0;

	alignment = (field->instance->template->icons[icon->icon].flags & wimp_ICON_RJUSTIFIED) ? EDIT_ALIGN_RIGHT : EDIT_ALIGN_LEFT;

	/* Now process the icons, expanding the horizontal extent as required. */

	while (icon != NULL) {
		column_get_xpos(field->instance->columns, icon->icon, &xmin, &xmax);

		icon = icon->sibling;
	}

	return field->instance->callbacks->find_field(field->instance->edit_line, xmin, xmax, alignment, field->instance->client_data);
}


/**
 * Contact the client to test if a given line in its window represents
 * valid data or not.
 *
 * \param *instance		The instance holding the line to be tested.
 * \param line			The line number to be tested.
 * \return			TRUE if the line is valid; FALSE if not, or on error.
 */

static osbool edit_callback_test_line(struct edit_block *instance, int line)
{
	if (instance == NULL || instance->callbacks == NULL || instance->callbacks->test_line == NULL)
		return FALSE;

	return instance->callbacks->test_line(line, instance->client_data);
};


/**
 * Contact the client to request that the edit line is placed at a given
 * location in the window related to the specified instance. The client will
 * be expected to make sure that the line is visible to the user.
 * 
 * \param *instance		The instance to take the line.
 * \param line			The line number at which to place the edit line.
 * \return			TRUE if the line was placed successfull; FALSE if not.
 */

static osbool edit_callback_place_line(struct edit_block *instance, int line)
{
	if (instance == NULL || instance->callbacks == NULL || instance->callbacks->place_line == NULL)
		return FALSE;

	return instance->callbacks->place_line(line, instance->client_data);
};


/**
 * Contact the client to request details of the first blank line in an edit
 * line instance's parent window.
 * 
 * \param *instance		The instance of interest.
 * \return			The line number indexed from 0, or -1 on failure.
 */

static int edit_callback_first_blank_line(struct edit_block *instance)
{
	if (instance == NULL || instance->callbacks == NULL || instance->callbacks->first_blank_line == NULL)
		return -1;

	return instance->callbacks->first_blank_line(instance->client_data);

}


/**
 * Contact the client to request that a field's parent window is sorted on
 * that field, if applicable.
 * 
 * \param *field		The field to request the sort on.
 * \return			TRUE if the request was successfully handled; FALSE if not.
 */

static osbool edit_callback_auto_sort(struct edit_field *field)
{
	if (field == NULL || field->icon == NULL || field->instance == NULL ||
			field->instance->callbacks == NULL || field->instance->callbacks->auto_sort == NULL)
		return FALSE;

	return field->instance->callbacks->auto_sort(field->icon->icon, field->instance->client_data);
};


/**
 * Contact the client to report that a transaction preset request has been
 * received for a given edit line instance. In practice, this means that an
 * alphabetic character was entered into a date field.
 * 
 * \param *instance		The instance receiving the request.
 * \param line			The line at which the request was received.
 * \param key			The keypress triggering the request, in upper or lower case.
 * \return			TRUE if the request was successfully handled; FALSE if not.
 */

static osbool edit_callback_insert_preset(struct edit_block *instance, int line, wimp_key_no key)
{
	if (instance == NULL || instance->callbacks == NULL || instance->callbacks->insert_preset == NULL)
		return FALSE;

	return instance->callbacks->insert_preset(line, key, instance->client_data);

}


/**
 * Calculate the sum of the characters in a text buffer. This can be used
 * as a very simple checksum/hash for text fields, in order to allow a guess
 * to me made as to whether or not their content has changed.
 * Fields are assumed to contain only non-control characters, and will
 * terminate on the first byte below ASCII Space.
 *
 * \param *text			Pointer to the buffer to be summed.
 * \param length		The maximum length of the buffer.
 * \return			The calculated sum (0 on failure).
 */

static int edit_sum_field_text(char *text, size_t length)
{
	int	sum = 0;
	char	*end = text + length;

	while (text < end && *text >= os_VDU_SPACE)
		sum += *(text++);

	return sum;
}


/**
 * Find the field in an instance which contains the caret.
 * 
 * \param *instance		The edit line to search.
 * \return			The field containing the caret, or NULL.
 */

static struct edit_field *edit_find_caret(struct edit_block *instance)
{
	wimp_caret		caret;
	struct edit_icon	*icon;

	if (instance == NULL)
		return NULL;

	wimp_get_caret_position(&caret);

	icon = instance->icons;

	while (icon != NULL && icon->icon != caret.i)
		icon = icon->next;

	if (icon == NULL)
		return NULL;

	return icon->parent;
}


/**
 * Find the last writable field in an edit line containing a given field, in terms
 * of the user-facing order. This means that the field will be at the start of the
 * field list, as fields are linked in reverse order.
 * 
 * \param *field		The field to search from.
 * \return			The field before the given field, or NULL.
 */

static struct edit_field *edit_find_last_field(struct edit_field *field)
{
	struct edit_field	*last;

	if (field == NULL || field->instance == NULL)
		return NULL;

	last = field->instance->fields;

	while (last != NULL && !edit_is_field_writable(last))
		last = last->next;

	return last;
}


/**
 * Find the first writable field after a given field in an edit line, in terms
 * of the user-facing order. This means that the field will be earlier in the
 * field list, as fields are linked in reverse order.
 * 
 * \param *field		The field to search from.
 * \return			The field before the given field, or NULL.
 */

static struct edit_field *edit_find_next_field(struct edit_field *field)
{
	struct edit_field	*next, *next_writable;

	if (field == NULL || field->instance == NULL)
		return NULL;

	next = field->instance->fields;
	next_writable = NULL;

	while (next != NULL && next != field) {
		if (edit_is_field_writable(next))
			next_writable = next;

		next = next->next;
	}

	return next_writable;
}


/**
 * Find the first writable field before a given field in an edit line, in terms
 * of the user-facing order. This means that the field will be later in the
 * field list, as fields are linked in reverse order.
 * 
 * \param *field		The field to search from.
 * \return			The field before the given field, or NULL.
 */

static struct edit_field *edit_find_previous_field(struct edit_field *field)
{
	struct edit_field	*previous;

	if (field == NULL || field->instance == NULL)
		return NULL;

	previous = field->next;

	while (previous != NULL && !edit_is_field_writable(previous))
		previous = previous->next;

	return previous;
}


/**
 * Find the first writable field in an edit line containing a given field, in terms
 * of the user-facing order. This means that the field will be at the end of the
 * field list, as fields are linked in reverse order.
 * 
 * \param *field		The field to search from.
 * \return			The field before the given field, or NULL.
 */

static struct edit_field *edit_find_first_field(struct edit_field *field)
{
	struct edit_field	*first, *first_writable;

	if (field == NULL || field->instance == NULL)
		return NULL;

	first = field->next;
	first_writable = NULL;

	while (first != NULL && first->next != NULL) {
		if (edit_is_field_writable(first))
			first_writable = first;

		first = first->next;
	}

	return first_writable;
}


/**
 * Test to see if a given field has a writable icon in its first location. The
 * icon must be of type Writable Click/Drag.
 * 
 * \param *field		The field to test.
 * \return			TRUE if the field is writable; FALSE if not.
 */

static osbool edit_is_field_writable(struct edit_field *field)
{
	if (field == NULL || field->instance == NULL || field->icon == NULL)
		return FALSE;

	if (field->instance->template == NULL)
		return FALSE;

	return (((field->instance->template->icons[field->icon->icon].flags & wimp_ICON_BUTTON_TYPE) >> wimp_ICON_BUTTON_TYPE_SHIFT) ==
			wimp_BUTTON_WRITE_CLICK_DRAG) ? TRUE : FALSE;
}


/**
 * Initialise the data transfer blocks when a new instance is created.
 *
 * \param *instance		The instance to be initialised.
 */

static void edit_initialise_transfer_blocks(struct edit_block *instance)
{
	int	i;

	if (instance == NULL)
		return;

	for (i = 0; i < EDIT_TRANSFER_BLOCKS; i++) {
		instance->transfer[i].free = TRUE;
		instance->transfer[i].data.data = instance->client_data;
	}
}


/**
 * Claim a new transfer block from the current instance. Claimed blocks
 * must be freed after use.
 * 
 * \param *instance		The instance to claim from.
 * \return			Pointer to the block, or NULL on failure.
 */

static struct edit_data *edit_get_transfer_block(struct edit_block *instance)
{
	int			i;
	struct edit_data	*transfer = NULL;

	if (instance == NULL)
		return NULL;

	for (i = 0; i < EDIT_TRANSFER_BLOCKS; i++) {
		if (instance->transfer[i].free == TRUE) {
#ifdef DEBUG
			debug_printf("Allocating transfer block %d", i);
#endif
			instance->transfer[i].free = FALSE;
			transfer = &(instance->transfer[i].data);
			break;
		}
	}

	return transfer;
}


/**
 * Release a transfer block claimed from an edit line instance.
 * 
 * \param *instance		The instance owning the block to be freed.
 * \param *transfer		Pointer to the block to be freed.
 */

static void edit_free_transfer_block(struct edit_block *instance, struct edit_data *transfer)
{
	int			i;

	if (instance == NULL || transfer == NULL)
		return;

	for (i = 0; i < EDIT_TRANSFER_BLOCKS; i++) {
		if (transfer == &(instance->transfer[i].data)) {
#ifdef DEBUG
			debug_printf("Freeing transfer block %d", i);
			if (instance->transfer[i].free == TRUE)
				debug_printf("Block not in use!");
#endif

			instance->transfer[i].free = TRUE;
			break;
		}
	}
}
