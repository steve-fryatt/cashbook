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
 * \file: list_window.h
 *
 * Generic List Window implementation.
 */

#ifndef CASHBOOK_LIST_WINDOW
#define CASHBOOK_LIST_WINDOW

/**
 * A one-time window defintion object.
 */

struct list_window_block;

/**
 * A list window instance.
 */

struct list_window;

/**
 * Definition of a list window object.
 */

struct list_window_definition {
	/**
	 * The name of the template for the list window itself.
	 */
	char				*main_template_name;

	/**
	 * The name of the template for the toolbar pane.
	 */
	char				*toolbar_template_name;

	/**
	 * The name of the template for the footer pane.
	 */
	char				*footer_template_name;

	/**
	 * The height of the toolbar pane, in OS Units.
	 */
	int				toolbar_height;

	/**
	 * The height of the footer pane, in OS Units.
	 */
	int				footer_height;

	/**
	 * The window column map.
	 */
	struct column_map		*column_map;

	/**
	 * The window column extra data.
	 */
	struct column_extra		*column_extra;

	/**
	 * The number of columns in the window.
	 */
	int				column_count;

	/**
	 * The column limit settings token.
	 */
	char				*column_limits;

	/**
	 * The column width settings token.
	 */
	char				*column_widths;

	/**
	 * The icon used to show the sort direction.
	 */
	wimp_i				sort_dir_icon;

	/**
	 * The sort dialogue column icons.
	 */
	struct sort_dialogue_icon	*sort_columns;

	/**
	 * The sort dialogue direction icons.
	 */
	struct sort_dialogue_icon	*sort_directions;

	/**
	 * Token for the window title.
	 */
	char				*window_title;

	/**
	 * Base help token for the main window.
	 */
	char				*window_help;

	/**
	 * Base help token for the toolbar pane.
	 */
	char				*toolbar_help;

	/**
	 * Base help token for the footer pane.
	 */
	char				*footer_help;

	/**
	 * The minimum number of entries in the window.
	 */
	int				minimum_entries;


	/**
	 * The callback handlers below this point are subject to implementation change!
	 * They also probably require a void *data pointer, for the client to pass the
	 * windat pointer in.
	 */

	void		(*callback_window_close_handler)(void *data);

	void		(*callback_window_click_handler)(wimp_pointer *pointer);

	void		(*callback_pane_click_handler)(wimp_pointer *pointer);

	void		(*callback_redraw_handler)(wimp_draw *redraw, void *data);

	void		(*callback_scroll_handler)(wimp_scroll *scroll);

	void		(*callback_decode_help)(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

	void		(*callback_menu_prepare_handler)(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);

	void		(*callback_menu_selection_handler)(wimp_w w, wimp_menu *menu, wimp_selection *selection);

	void		(*callback_menu_warning_handler)(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);

	void		(*callback_menu_close_handler)(wimp_w w, wimp_menu *menu);
};


/**
 * Create a new list window template block, and load the window template
 * definitions ready for use.
 * 
 * \param *definition		Pointer to the window definition block.
 * \param *sprites		Pointer to the application sprite area.
 * \returns			Pointer to the window block, or NULL on failure.
 */

struct list_window_block *list_window_create(struct list_window_definition *definition, osspriteop_area *sprites);


/**
 * Create a new List Window instance.
 *
 * \param *parent		The List Window to own the instance.
 * \param *file			The file to which the instance belongs.
 * \param *data			Data pointer to be passed to callback functions.
 * \return			Pointer to the new instance, or NULL.
 */
struct list_window *list_window_create_instance(struct list_window_block *parent, struct file_block *file, void *data);

osbool list_window_open(struct list_window *instance);

/**
 * Delete a List Window instance.
 *
 * \param *instance		The List Window instance to delete.
 */
void list_window_delete_instance(struct list_window *instance);










/**
 * Process a WinColumns line from a file.
 *
 * \param *instance		The instance being read in to.
 * \param start			The first column to read in from the string.
 * \param skip			TRUE to ignore missing entries; FALSE to set to default.
  * \param *columns		The column text line.
 */

void list_window_read_file_wincolumns(struct list_window *instance, int start, osbool skip, char *columns);





wimp_window *list_window_get_window_def(struct list_window_block *block);
wimp_window *list_window_get_toolbar_def(struct list_window_block *block);

#endif
