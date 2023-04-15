/* Copyright 2003-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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

#define LIST_WINDOW_NULL_INDEX ((int) -1)

/**
 * Flags defining aspects of the list window's behaviour.
 */

enum list_window_flags {
	LIST_WINDOW_FLAGS_NONE = 0,		/**< No flags set.					*/
	LIST_WINDOW_FLAGS_PARENT = 1,		/**< The window is the main window of a file instance.	*/
	LIST_WINDOW_FLAGS_EDIT = 2,		/**< The window has an edit line.			*/
};

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
	 * The name of the template for the window menu.
	 */
	char				*menu_template_name;

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
	 * The name of the template for the sort dialogue.
	 */
	char				*sort_template_name;

	/**
	 * The sort dialogue column icons.
	 */
	struct sort_dialogue_icon	*sort_columns;

	/**
	 * The sort dialogue direction icons.
	 */
	struct sort_dialogue_icon	*sort_directions;

	/**
	 * The sort dialogue OK icon.
	 */
	wimp_i				sort_icon_ok;

	/**
	 * The sort dialogue Cancel icon.
	 */
	wimp_i				sort_icon_cancel;

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
	 * Base help token for the window menu.
	 */
	char				*menu_help;

	/**
	 * Base help token for the sort dialogue box.
	 */
	char				*sort_help;

	/**
	 * The minimum number of entries in the window.
	 */
	int				minimum_entries;

	/**
	 * The minimum number of lbank lines in the window.
	 */
	int				minimum_blank_lines;

	/**
	 * The print dialogue box token.
	 */
	char				*print_title;

	/**
	 * The print report title token.
	 */
	char				*print_report_title;

	/**
	 * TRUE if the print dialogue box should contain dates.
	 */
	osbool				print_dates;

	/**
	 * Flags relating to the window's behaviour.
	 */
	enum list_window_flags		flags;

	/**
	 * The callback handlers below this point are subject to implementation change!
	 * They also probably require a void *data pointer, for the client to pass the
	 * windat pointer in.
	 */

	void		(*callback_window_click_handler)(wimp_pointer *pointer, int index, struct file_block *file, void *data);

	void		(*callback_pane_click_handler)(wimp_pointer *pointer, struct file_block *file, void *data);

	void*		(*callback_redraw_prepare)(struct file_block *file, void *data);

	void		(*callback_redraw_finalise)(struct file_block *file, void *data, void *redraw);

	void		(*callback_redraw_handler)(int index, struct file_block *file, void *data, void *redraw);

	void		(*callback_scroll_handler)(wimp_scroll *scroll);

	void		(*callback_decode_help)(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

	void		(*callback_menu_prepare_handler)(wimp_w w, wimp_menu *menu, wimp_pointer *pointer, int index, struct file_block *file, void *data);

	void		(*callback_menu_selection_handler)(wimp_w w, wimp_menu *menu, wimp_selection *selection, wimp_pointer *pointer, int index, struct file_block *file, void *data);

	void		(*callback_menu_warning_handler)(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning, int index, struct file_block *file, void *data);

	int		(*callback_sort_compare)(enum sort_type type, int index1, int index2, struct file_block *file);

	osbool		(*callback_print_include)(struct file_block *file, int index, date_t from, date_t to);

	void		(*callback_print_field)(struct file_block *file, wimp_i column, int index, char *rec_char);

	void		(*callback_export_line)(FILE *out, enum filing_delimit_type format, struct file_block *file, int index);

	wimp_colour	(*callback_get_colour)(int index, struct file_block *file, void *data);

	osbool		(*callback_is_blank)(int index, struct file_block *file, void *data);
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


/**
 * Delete a List Window instance.
 *
 * \param *instance		The List Window instance to delete.
 */
void list_window_delete_instance(struct list_window *instance);


/**
 * Force complete redraw operations for all of the list window
 * instances belonging to a file.
 * 
 * \param *file			The file to be redrawn.
 */

void list_window_redraw_file(struct file_block *file);


/**
 * Rebuild the titles of all list window instances beloinging
 * to a file.
 * 
 * \param *file			The file to be updated.
 * \param osbool		TRUE to redraw just the parent; FALSE for all.
 */

void list_window_rebuild_file_titles(struct file_block *file, osbool parent);


/**
 * Get the window state of the parent window belonging to
 * the specified file.
 *
 * \param *file			The file containing the window.
 * \param *state		The structure to hold the window state.
 * \return			Pointer to an error block, or NULL on success.
 */

os_error *list_window_get_state(struct file_block *file, wimp_window_state *state);


/**
 * Set the extent of windows relating to the specified file.
 *
 * \param *file			The file containing the window.
 * \param osbool		TRUE to update just the parent; FALSE for all.
 */

os_error *list_window_set_file_extent(struct file_block *file, osbool parent);


/**
 * Create and open a List window for the given instance.
 *
 * \param *instance		The instance to open a window for.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool list_window_open(struct list_window *instance);


/**
 * Force the redraw of one or all of the lines in the given list window.
 *
 * \param *instance		The list window instance to be redrawn.
 * \param index			The index to redraw, or
 *				LIST_WINDOW_NULL_INDEX for all.
 * \param columns		The number of columns to be redrawn, or
 *				0 to redraw all.
 * \param ...			List of column icons if columns > 0.
 */

void list_window_redraw(struct list_window *instance, int index, int columns, ...);


/**
 * Find the display line number of the current entry line.
 *
 * \param *instance		The list window instance to interrogate.
 * \return			The display line number of the line with the caret.
 */

int list_window_get_caret_line(struct list_window *instance);


/**
 * Initialise the contents of the list window, creating an entry for
 * each of the required entries.
 *
 * \param *instance		The list window instance to initialise.
 * \param entries		The number of entries to insert.
 * \return			TRUE on success; FALSE on failure.
 */

osbool list_window_initialise_entries(struct list_window *instance, int entries);


/**
 * Add a new entry to a list window instance.
 *
 * \param *instance		The list window instance to add to.
 * \param entry			The index to add.
 * \param sort			TRUE to sort the entries after addition; otherwsie FALSE.
 * \return			TRUE on success; FALSE on failure.
 */

osbool list_window_add_entry(struct list_window *instance, int entry, osbool sort);


/**
 * Remove an entry from a list window instance, and update the other entries to
 * allow for its deletion.
 *
 * \param *instance		The list window instance to remove from.
 * \param entry			The entry index to remove.
 * \param sort			TRUE to sort the entries after deletion; otherwsie FALSE.
 * \return			TRUE on success; FALSE on failure.
 */

osbool list_window_delete_entry(struct list_window *instance, int entry, osbool sort);


/**
 * Find and return the line number of the first blank line in a list window,
 * based on display order and the client's interpretation of 'blank'.
 *
 * \param *instance		The list window instance to search.
 * \return			The first blank display line.
 */

int list_window_find_first_blank_line(struct list_window *instance);


/**
 * Find the display line in a list window which points to the specified
 * index under the applied sort.
 *
 * \param *instance		The list window instance to search.
 * \param index			The index to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

int list_window_get_line_from_index(struct list_window *instance, int index);


/**
 * Find the index which corresponds to a display line in the specified
 * list window instance.
 *
 * \param *windat		The list window instance to search in.
 * \param line			The display line to return the index for.
 * \return			The appropriate index, or LIST_WINDOW_NULL_INDEX.
 */

int list_window_get_index_from_line(struct list_window *instance, int line);


/**
 * Open the sort dialogue for a given list window instance.
 *
 * \param *file			The preset window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

void list_window_open_sort_window(struct list_window *instance, wimp_pointer *ptr);


/**
 * Sort the entries in a given list window based on that instance's sort
 * setting.
 *
 * \param *instance		The list window instance to sort.
 */

void list_window_sort(struct list_window *instance);


/**
 * Open the Print dialogue for a given list window instance.
 *
 * \param *instance		The list window instance to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to retain the previous settings; FALSE to
 *				return to defaults.
 */

void list_window_open_print_window(struct list_window *instance, wimp_pointer *ptr, osbool restore);


/**
 * Export the data from a list window into CSV or TSV format.
 *
 * \param *instance		The list window instance to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

void list_window_export_delimited(struct list_window *instance, char *filename, enum filing_delimit_type format, int filetype);


/**
 * Save the list window details from a given instance to a CashBook file.
 * This assumes that the caller has already created a suitable section
 * in the file to be written.
 *
 * \param *instance		The window instance whose details to write.
 * \param *out			The file handle to write to.
 */

void list_window_write_file(struct list_window *instance, FILE *out);


/**
 * Process a WinColumns line from a file.
 *
 * \param *instance		The instance being read in to.
 * \param start			The first column to read in from the string.
 * \param skip			TRUE to ignore missing entries; FALSE to set to default.
  * \param *columns		The column text line.
 */

void list_window_read_file_wincolumns(struct list_window *instance, int start, osbool skip, char *columns);


/**
 * Process a SortOrder line from a file.
 *
 * \param *instance		The instance being read in to.
 * \param *order		The order text line.
 */

void list_window_read_file_sortorder(struct list_window *instance, char *order);

#endif