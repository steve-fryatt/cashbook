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
 * \file: list_window.c
 *
 * Generic List Window implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "list_window.h"

#include "column.h"
#include "dialogue.h"
#include "edit.h"
#include "flexutils.h"
#include "print_dialogue.h"
#include "report.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "window.h"

/**
 * The screen offset at which to open new Parent List Windows, in OS Units.
 */

#define LIST_WINDOW_OPEN_OFFSET 48

/**
 * The maximum number of offsets to apply, before wrapping around.
 */

#define LIST_WINDOW_OPEN_OFFSET_LIMIT 8

/**
 * List Window Definition data structure.
 */

struct list_window_block {
	/**
	 * The window definition.
	 */
	struct list_window_definition	*definition;

	/**
	 * The definition for the main window.
	 */
	wimp_window			*window_def;

	/**
	 * The definition for the toolbar pane.
	 */
	wimp_window			*toolbar_def;

	/**
	 * The definition for the footer pane.
	 */
	wimp_window			*footer_def;

	/**
	 * The handle of the window menu.
	 */
	wimp_menu			*menu;

	/**
	 * The sort callback function details.
	 */

	struct sort_callback		sort_callbacks;

	/**
	 * The sort dialogue box instance.
	 */
	struct sort_dialogue_block	*sort_dialogue;
};

/**
 * List Window line redraw data.
 */

struct list_window_redraw {
	/**
	 * The index into the client data for a given line.
	 */
	int				index;
};

/**
 * List Window Instance data structure.
 */

struct list_window {
	/**
	 * The List Window definition owning this instance.
	 */
	struct list_window_block	*parent;

	/**
	 * The parent file for the instance.
	 */
	struct file_block		*file;

	/**
	 * Data provided by the client.
	 */
	void				*client_data;

	/**
	 * Wimp window handle for the main List Window.
	 */
	wimp_w				window;

	/**
	 * Indirected title data for the window.
	 */
	char				title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the List Window Toolbar pane.
	 */
	wimp_w				toolbar;

	/**
	 * Wimp window handle for the List Window Footer pane.
	 */
	wimp_w				footer;

	/**
	 * Instance handle for the window's edit line.
	 */
	struct edit_block		*edit_line;

	/**
	 * Instance handle for the window's column definitions.
	 */
	struct column_block		*columns;

	/**
	 * Instance handle for the window's sort code.
	 */
	struct sort_block		*sort;

	/**
	 * Indirected text data for the sort sprite icon.
	 */
	char				sort_sprite[COLUMN_SORT_SPRITE_LEN];

	/**
	 * Count of the number of populated display lines in the window.
	 */
	int				display_lines;

	/**
	 * The line containing the menu, or -1 for none.
	 */
	int				menu_line;

	/**
	 * Flex array holding the line data for the window.
	 */
	struct list_window_redraw	*line_data;

	/**
	 * The number of visible lines in the window, including blank lines.
	 */
	int					visible_lines;

	/**
	 * Pointer to the next list window instance in the list.
	 */
	struct list_window		*next;
};

/**
 * Linked list of dialogue instances.
 */

static struct list_window	*list_window_instance_list = NULL;

/**
 * Offset, in OS Units, at which to open the next parent window.
 */

static int			list_window_new_offset = 0;

/* Static Function Prototypes. */

static void list_window_delete(struct list_window *instance);
static void list_window_close_handler(wimp_close *close);
static void list_window_click_handler(wimp_pointer *pointer);
static void list_window_pane_click_handler(wimp_pointer *pointer);
static void list_window_scroll_handler(wimp_scroll *scroll);
static void list_window_redraw_handler(wimp_draw *redraw);
static void list_window_force_redraw(struct list_window *instance, int from, int to, wimp_i column);
static void list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

static void list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void list_window_menu_close_handler(wimp_w w, wimp_menu *menu);

static void list_window_adjust_columns(void *data, wimp_i group, int width);
static void list_window_adjust_sort_icon(struct list_window *instance);
static void list_window_adjust_sort_icon_data(struct list_window *instance, wimp_icon *icon);
static void list_window_set_extent(struct list_window *instance);
static void list_window_build_title(struct list_window *instance);

static int list_window_get_line_from_index(struct list_window *instance, int index);
static osbool list_window_process_sort_window(enum sort_type order, void *data);
static int list_window_sort_compare(enum sort_type type, int index1, int index2, void *data);
static void list_window_sort_swap(int index1, int index2, void *data);
static struct report *list_window_print(struct report *report, void *data, date_t from, date_t to);


/**
 * Test whether a line number is safe to look up in the redraw data array.
 */

#define list_window_line_valid(instance, line) (((line) >= 0) && ((line) < ((instance)->display_lines)))


void list_window_initialise(void)
{

}

/**
 * Create a new list window template block, and load the window template
 * definitions ready for use.
 * 
 * \param *definition		Pointer to the window definition block.
 * \param *sprites		Pointer to the application sprite area.
 * \returns			Pointer to the window block, or NULL on failure.
 */

struct list_window_block *list_window_create(struct list_window_definition *definition, osspriteop_area *sprites)
{
	struct list_window_block	*block;
	wimp_w				sort_window;

	if (definition == NULL)
		return NULL;

	block = heap_alloc(sizeof(struct list_window_block));
	if (block == NULL)
		return NULL;

	block->definition = definition;

	/* Load the main window template. */

	if (definition->main_template_name != NULL) {
		block->window_def = templates_load_window(definition->main_template_name);
		block->window_def->icon_count = 0;
	} else {
		block->window_def = NULL;
	}

	/* Load the toolbar pane template. */

	if (definition->toolbar_template_name != NULL) {
		block->toolbar_def = templates_load_window(definition->toolbar_template_name);
		block->toolbar_def->sprite_area = sprites;
	} else {
		block->toolbar_def = NULL;
	}

	/* Load the footer pane template. */

	if (definition->footer_template_name != NULL) {
		block->footer_def = templates_load_window(definition->footer_template_name);
	} else {
		block->footer_def = NULL;
	}

	/* Load the window menu template. */

	if (definition->menu_template_name != NULL) {
		block->menu = templates_get_menu(definition->menu_template_name);
		ihelp_add_menu(block->menu, definition->menu_help);
	} else {
		block->menu = NULL;
	}

	/* Set up the sort callbacks. */

	block->sort_callbacks.compare = list_window_sort_compare;
	block->sort_callbacks.swap = list_window_sort_swap;

	/* Set up the sort dialogue. */

	sort_window = templates_create_window(definition->sort_template_name);
	ihelp_add_window(sort_window, definition->sort_help, NULL);
	block->sort_dialogue = sort_dialogue_create(sort_window, definition->sort_columns, definition->sort_directions,
			definition->sort_icon_ok, definition->sort_icon_cancel, list_window_process_sort_window);

	return block;
}


/**
 * Create a new List Window instance.
 *
 * \param *parent		The List Window to own the instance.
 * \param *file			The file to which the instance belongs.
 * \param *data			Data pointer to be passed to callback functions.
 * \return			Pointer to the new instance, or NULL.
 */

struct list_window *list_window_create_instance(struct list_window_block *parent, struct file_block *file, void *data)
{
	struct list_window	*new;

	new = heap_alloc(sizeof(struct list_window));
	if (new == NULL)
		return NULL;

	new->parent = parent;
	new->file = file;
	new->client_data = data;
	new->next = NULL;

	new->window = NULL;
	new->toolbar = NULL;
	new->footer = NULL;
	new->edit_line = NULL;
	new->columns = NULL;
	new->sort = NULL;

	new->display_lines = 0;
	new->visible_lines = 0;
	new->line_data = NULL;
	new->menu_line = -1;

	/* Initialise the window columns. */

	new-> columns = column_create_instance(parent->definition->column_count, parent->definition->column_map,
			parent->definition->column_extra, parent->definition->sort_dir_icon);
	if (new->columns == NULL) {
		list_window_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, config_str_read(parent->definition->column_limits));
	column_init_window(new->columns, 0, FALSE, config_str_read(parent->definition->column_widths));

	/* Initialise the window sort. */

	new->sort = sort_create_instance(SORT_CHAR | SORT_ASCENDING, SORT_NONE, &(parent->sort_callbacks), new);
	if (new->sort == NULL) {
		list_window_delete_instance(new);
		return NULL;
	}

	/* Set the initial line data block up */

	if (!flexutils_initialise((void **) &(new->line_data))) {
		list_window_delete_instance(new);
		return NULL;
	}

	/* Link the instance in to the linked list. */

	new->next = list_window_instance_list;
	list_window_instance_list = new;

	return new;
}


/**
 * Delete a List Window instance.
 *
 * \param *instance		The List Window instance to delete.
 */

void list_window_delete_instance(struct list_window *instance)
{
	struct list_window **list;

	if (instance == NULL)
		return;

	if (instance->line_data != NULL)
		flexutils_free((void **) &(instance->line_data));

	column_delete_instance(instance->columns);
	sort_delete_instance(instance->sort);

	list_window_delete(instance);

	/* De-link the instance from the list of instances. */

	list = &list_window_instance_list;

	while (*list != NULL && *list != instance)
		list = &((*list)->next);
	
	if (*list != NULL)
		*list = instance->next;

	/* Delete the instance memory. */

	heap_free(instance);
}


/**
 * Force complete redraw operations for all of the list window
 * instances belonging to a file.
 * 
 * \param *file			The file to be redrawn.
 */

void list_window_redraw_file(struct file_block *file)
{
	struct list_window *list = list_window_instance_list;

	while (list != NULL) {
		if (list->file == file)
			list_window_redraw(list, LIST_WINDOW_NULL_INDEX, 0);

		list = list->next;
	}
}


/**
 * Rebuild the titles of all list window instances beloinging
 * to a file.
 * 
 * \param *file			The file to be updated.
 * \param osbool		TRUE to redraw just the parent; FALSE for all.
 */

void list_window_rebuild_file_titles(struct file_block *file, osbool parent)
{
	struct list_window *list = list_window_instance_list;

	while (list != NULL) {
		if (list->file == file)
			list_window_build_title(list);

		list = list->next;
	}
}


/**
 * Get the window state of the parent window belonging to
 * the specified file.
 *
 * \param *file			The file containing the window.
 * \param *state		The structure to hold the window state.
 * \return			Pointer to an error block, or NULL on success.
 */

os_error *list_window_get_state(struct file_block *file, wimp_window_state *state)
{
	struct list_window *list = list_window_instance_list;

	while (list != NULL) {
		if (list->file == file && list->parent != NULL || list->parent->definition != NULL) {
			if (list->parent->definition->flags & LIST_WINDOW_FLAGS_PARENT) {
				state->w = list->window;
				return xwimp_get_window_state(state);
			}
		}

		list = list->next;
	}

	return NULL;
}


/**
 * Create and open a List window for the given instance.
 *
 * \param *instance		The instance to open a window for.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool list_window_open(struct list_window *instance)
{
	int			height;
	os_error		*error;
	wimp_window_state	parent;

	if (instance == NULL || instance->parent == NULL)
		return FALSE;

	/* Create or re-open the window. */

	if (instance->window != NULL) {
		windows_open(instance->window);
		return TRUE;
	}

	#ifdef DEBUG
	debug_printf("\\CCreating list window");
	#endif

	/* Set the default values */

	instance->visible_lines = (instance->display_lines + instance->parent->definition->minimum_blank_lines > instance->parent->definition->minimum_entries) ?
			instance->display_lines + instance->parent->definition->minimum_blank_lines : instance->parent->definition->minimum_entries;

	/* Create the new window data and build the window. */

	*(instance->title) = '\0';
	instance->parent->window_def->title_data.indirected_text.text = instance->title;

	height = instance->visible_lines;

	if (instance->parent->definition->flags & LIST_WINDOW_FLAGS_PARENT) {
		window_set_initial_area(instance->parent->window_def, column_get_window_width(instance->columns),
				(height * WINDOW_ROW_HEIGHT) + instance->parent->definition->toolbar_height,
				-1, -1, list_window_new_offset * LIST_WINDOW_OPEN_OFFSET);

		list_window_new_offset++;
		if (list_window_new_offset >= LIST_WINDOW_OPEN_OFFSET_LIMIT)
			list_window_new_offset = 0;
	} else {
		list_window_get_state(instance->file, &parent);

		window_set_initial_area(instance->parent->window_def, column_get_window_width(instance->columns),
				(height * WINDOW_ROW_HEIGHT) + instance->parent->definition->toolbar_height,
				parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(instance->file),
				parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);
	}

	error = xwimp_create_window(instance->parent->window_def, &(instance->window));
	if (error != NULL) {
		list_window_delete(instance);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Create the toolbar. */

	if (instance->parent->toolbar_def != NULL) {
		windows_place_as_toolbar(instance->parent->window_def, instance->parent->toolbar_def,
				instance->parent->definition->toolbar_height - 4);

		#ifdef DEBUG
		debug_printf ("Window extents set...");
		#endif

		columns_place_heading_icons(instance->columns, instance->parent->toolbar_def);

		instance->parent->toolbar_def->icons[instance->parent->definition->sort_dir_icon].data.indirected_sprite.id =
				(osspriteop_id) instance->sort_sprite;
		instance->parent->toolbar_def->icons[instance->parent->definition->sort_dir_icon].data.indirected_sprite.area =
				instance->parent->toolbar_def->sprite_area;
		instance->parent->toolbar_def->icons[instance->parent->definition->sort_dir_icon].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

		list_window_adjust_sort_icon_data(instance, &(instance->parent->toolbar_def->icons[instance->parent->definition->sort_dir_icon]));

		#ifdef DEBUG
		debug_printf ("Toolbar icons adjusted...");
		#endif

		error = xwimp_create_window(instance->parent->toolbar_def, &(instance->toolbar));
		if (error != NULL) {
			list_window_delete(instance);
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return FALSE;
		}
	}

	/* Construct the edit line. */

	if (instance->parent->definition->flags & LIST_WINDOW_FLAGS_EDIT) {
		instance->edit_line = edit_create_instance(instance->file, instance->parent->window_def,
				instance->window, instance->columns,
				instance->parent->definition->toolbar_height,
				&transact_list_window_edit_callbacks, instance);

		if (instance->edit_line == NULL) {
			list_window_delete(instance);
			error_msgs_report_error("TransactNoMem");
			return;
		}
	}

	/* Set the title */

	list_window_build_title(instance);

	/* Set up the interactive help. */

	if (instance->window != NULL)
		ihelp_add_window(instance->window , instance->parent->definition->window_help, list_window_decode_help);

	if (instance->toolbar != NULL)
		ihelp_add_window(instance->toolbar , instance->parent->definition->toolbar_help, NULL);

	if (instance->footer != NULL)
		ihelp_add_window(instance->footer , instance->parent->definition->footer_help, NULL);

	/* Open the window. */

	windows_open(instance->window);

	if (instance->toolbar != NULL)
		windows_open_nested_as_toolbar(instance->toolbar, instance->window,
				instance->parent->definition->toolbar_height - 4, FALSE);

	/* Register event handlers for the main windows. */

	event_add_window_user_data(instance->window, instance);
	event_add_window_menu(instance->window, instance->parent->menu);
	event_add_window_close_event(instance->window, list_window_close_handler);
	event_add_window_mouse_event(instance->window, list_window_click_handler);
	event_add_window_scroll_event(instance->window, list_window_scroll_handler);
	event_add_window_redraw_event(instance->window, list_window_redraw_handler);
	event_add_window_menu_prepare(instance->window, list_window_menu_prepare_handler);
	event_add_window_menu_selection(instance->window, list_window_menu_selection_handler);
	event_add_window_menu_warning(instance->window, list_window_menu_warning_handler);
	event_add_window_menu_close(instance->window, list_window_menu_close_handler);

	/* Register event handlers for the toolbar pane. */

	if (instance->toolbar != NULL) {
		event_add_window_user_data(instance->toolbar, instance);
		event_add_window_menu(instance->toolbar, instance->parent->menu);
		event_add_window_mouse_event(instance->toolbar, list_window_pane_click_handler);
		event_add_window_menu_prepare(instance->toolbar, list_window_menu_prepare_handler);
		event_add_window_menu_selection(instance->toolbar, list_window_menu_selection_handler);
		event_add_window_menu_warning(instance->toolbar, list_window_menu_warning_handler);
		event_add_window_menu_close(instance->toolbar, list_window_menu_close_handler);
	}

	return TRUE;
}


/**
 * Close and delete a List Window associated with the given instance.
 * Note that this does NOT delete the instance itself; merely the Wimp
 * windows associated with it.
 *
 * \param *instance		The window to delete.
 */

static void list_window_delete(struct list_window *instance)
{
	if (instance == NULL)
		return;

	#ifdef DEBUG
	debug_printf ("\\RDeleting List window instance");
	#endif

	/* Delete the edit line, if it exists. */

	if (instance->edit_line != NULL) {
		edit_delete_instance(instance->edit_line);
		instance->edit_line = NULL;
	}

	/* Delete the main window, if it exists. */

	if (instance->window != NULL) {
		ihelp_remove_window(instance->window);
		event_delete_window(instance->window);
		wimp_delete_window(instance->window);
		instance->window = NULL;
	}

	/* Delete the toolbar pane, if it exists. */

	if (instance->toolbar != NULL) {
		ihelp_remove_window(instance->toolbar);
		event_delete_window(instance->toolbar);
		wimp_delete_window(instance->toolbar);
		instance->toolbar = NULL;
	}

	/* Delete the footer pane, if it exists. */

	if (instance->footer != NULL) {
		ihelp_remove_window(instance->footer);
		event_delete_window(instance->footer);
		wimp_delete_window(instance->footer);
		instance->footer = NULL;
	}

	/* Close any dialogues which belong to this window. */

	dialogue_force_all_closed(NULL, instance);
	sort_dialogue_close(instance->parent->sort_dialogue, instance);
}


/**
 * Handle Close events on List windows, deleting the window and
 * tidying up any associated objects.
 *
 * \param *close		The Wimp Close data block.
 */

static void list_window_close_handler(wimp_close *close)
{
	struct list_window *instance;

	#ifdef DEBUG
	debug_printf ("\\RClosing Preset List window");
	#endif

	instance = event_get_window_user_data(close->w);
	if (instance != NULL)
		list_window_delete(instance);

}


/**
 * Handle click events on List Windows.
 * 
 * \param *pointer		The Wimp Click data block.
 */

static void list_window_click_handler(wimp_pointer *pointer)
{
	struct list_window	*instance;
	int			line, index;
	wimp_window_state	window;

	instance = event_get_window_user_data(pointer->w);
	if (instance == NULL || instance->parent == NULL)
		return;

	/* Find the window type and get the line clicked on. */

	window.w = pointer->w;
	wimp_get_window_state(&window);

	line = window_calculate_click_row(&(pointer->pos), &window, instance->parent->definition->toolbar_height, instance->display_lines);

	if (line == -1)
		return;

	index = instance->line_data[line].index;

	/* Call the client's callback. */

	if (instance->parent->definition->callback_window_click_handler != NULL)
		instance->parent->definition->callback_window_click_handler(pointer, index, instance->file, instance->client_data);
}


/**
 * Handle click events on List Toolbar Panes.
 * 
 * \param *pointer		The Wimp Click data block.
 */

static void list_window_pane_click_handler(wimp_pointer *pointer)
{
	struct list_window	*instance;
	wimp_window_state	window;
	wimp_icon_state		icon;
	int			ox;
	enum sort_type		sort_order;

	instance = event_get_window_user_data(pointer->w);
	if (instance == NULL || instance->parent == NULL)
		return;

	/* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

	column_update_heading_icon_click(instance->columns, pointer);

	/* Process toolbar clicks and column heading drags. */

	if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
		if ((instance->parent->definition->callback_pane_click_handler != NULL) &&
				(pointer->i != wimp_ICON_WINDOW))
			instance->parent->definition->callback_pane_click_handler(pointer, instance->file, instance->client_data);
	} else if ((pointer->buttons == wimp_CLICK_SELECT * 256 || pointer->buttons == wimp_CLICK_ADJUST * 256) &&
			pointer->i != wimp_ICON_WINDOW) {
		window.w = pointer->w;
		wimp_get_window_state(&window);

		ox = window.visible.x0 - window.xscroll;

		icon.w = pointer->w;
		icon.i = pointer->i;
		wimp_get_icon_state(&icon);

		if (pointer->pos.x < (ox + icon.icon.extent.x1 - COLUMN_DRAG_HOTSPOT)) {
			sort_order = column_get_sort_type_from_heading(instance->columns, pointer->i);

			if (sort_order != SORT_NONE) {
				sort_order |= (pointer->buttons == wimp_CLICK_SELECT * 256) ? SORT_ASCENDING : SORT_DESCENDING;

				sort_set_order(instance->sort, sort_order);
				list_window_adjust_sort_icon(instance);
				windows_redraw(instance->toolbar);
				list_window_sort(instance);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(instance->columns, pointer->i)) {
		column_set_minimum_widths(instance->columns, config_str_read(instance->parent->definition->column_limits));
		column_start_drag(instance->columns, pointer, instance, instance->toolbar, list_window_adjust_columns);
	}
}


/**
 * Process scroll events in the Preset List window.
 *
 * \param *scroll		The scroll event block to handle.
 */

static void list_window_scroll_handler(wimp_scroll *scroll)
{
	struct list_window	*instance;

	instance = event_get_window_user_data(scroll->w);
	if (instance == NULL || instance->parent == NULL)
		return;


	window_process_scroll_event(scroll, instance->parent->definition->toolbar_height);

	/* Re-open the window. It is assumed that the wimp will deal with out-of-bounds offsets for us. */

	wimp_open_window((wimp_open *) scroll);
}


/**
 * Process redraw events in a list window instance.
 *
 * \param *redraw		The draw event block to handle.
 */

static void list_window_redraw_handler(wimp_draw *redraw)
{
	struct list_window	*instance;
	int			top, base, y, select, index;
	char			icon_buffer[TRANSACT_DESCRIPT_FIELD_LEN]; /* Assumes descript is longest. */
	osbool			more;
	wimp_window		*window_def;

	instance = event_get_window_user_data(redraw->w);
	if (instance == NULL || instance->parent == NULL)
		return;

	window_def = instance->parent->window_def;
	if (window_def == NULL)
		return;

	/* Identify if there is a selected line to highlight. */

	if (redraw->w == event_get_current_menu_window())
		select = instance->menu_line;
	else
		select = -1;

	/* Set the horizontal positions of the icons. */

	columns_place_table_icons_horizontally(instance->columns, window_def, icon_buffer, TRANSACT_DESCRIPT_FIELD_LEN);

	window_set_icon_templates(window_def);

	/* Perform the redraw. */

	more = wimp_redraw_window(redraw);

	while (more) {
		window_plot_background(redraw, instance->parent->definition->toolbar_height, wimp_COLOUR_WHITE, select, &top, &base);

		/* Redraw the data into the window. */

		for (y = top; y <= base; y++) {
			index = (y < instance->display_lines) ? instance->line_data[y].index : 0;

			/* Place the icons in the current row. */

			columns_place_table_icons_vertically(instance->columns, window_def,
					WINDOW_ROW_Y0(instance->parent->definition->toolbar_height, y),
					WINDOW_ROW_Y1(instance->parent->definition->toolbar_height, y));

			/* If we're off the end of the data, plot a blank line and continue. */

			if (y >= instance->display_lines) {
				columns_plot_empty_table_icons(instance->columns);
				continue;
			}

			if (instance->parent->definition->callback_redraw_handler != NULL)
				instance->parent->definition->callback_redraw_handler(index, instance->file, instance->client_data);
		}

		more = wimp_get_rectangle(redraw);
	}
}


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

void list_window_redraw(struct list_window *instance, int index, int columns, ...)
{
	int	from, to, i;
	va_list	ap;

	if (instance == NULL)
		return;

	if (index != LIST_WINDOW_NULL_INDEX) {
		from = list_window_get_line_from_index(instance, index);
		to = from;
	} else {
		from = 0;
		to = instance->display_lines - 1;
	}

	if (columns == 0) {
		list_window_force_redraw(instance, from, to, wimp_ICON_WINDOW);
	} else {
		va_start(ap, columns);

		for (i = 0; i < columns; i++)
			list_window_force_redraw(instance, from, to, va_arg(ap, wimp_i));

		va_end(ap);
	}
}


/**
 * Force a redraw of a list window instance, for the given range of lines.
 *
 * \param *instance		The list window instance to be redrawn.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 * \param column		The column to be redrawn, or wimp_ICON_WINDOW for all.
 */

static void list_window_force_redraw(struct list_window *instance, int from, int to, wimp_i column)
{
	wimp_window_info	window;

	if (instance == NULL || instance->window == NULL)
		return;

	window.w = instance->window;
	if (xwimp_get_window_info_header_only(&window) != NULL)
		return;

	if (column != wimp_ICON_WINDOW) {
		window.extent.x0 = window.extent.x1;
		window.extent.x1 = 0;
		column_get_heading_xpos(instance->columns, column, &(window.extent.x0), &(window.extent.x1));
	}

	window.extent.y1 = WINDOW_ROW_TOP(instance->parent->definition->toolbar_height, from);
	window.extent.y0 = WINDOW_ROW_BASE(instance->parent->definition->toolbar_height, to);

	wimp_force_redraw(instance->window, window.extent.x0, window.extent.y0, window.extent.x1, window.extent.y1);
}


/**
 * Find the Wimp colour of a given line in a list window instance.
 *
 * \param *windat		The list window instance of interest.
 * \param line			The line of interest.
 * \return			The required Wimp colour, or Black on failure.
 */

static wimp_colour list_window_line_colour(struct list_window *instance, int line)
{
	int index;

	if (instance == NULL || instance->parent == NULL || instance->parent->definition == NULL ||
			instance->parent->definition->callback_get_colour == NULL ||
			instance ->line_data == NULL || !list_window_line_valid(instance, line))
		return wimp_COLOUR_BLACK;

	return instance->parent->definition->callback_get_colour(instance->line_data[line].index,
			instance->file, instance->client_data);
}


/**
 * Turn a mouse position over the a list window into an interactive help
 * token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int			xpos;
	wimp_i			icon;
	wimp_window_state	window;
	struct list_window	*instance;
	wimp_window		*window_def;

	*buffer = '\0';

	instance = event_get_window_user_data(w);
	if (instance == NULL || instance->parent == NULL)
		return;

	window_def = instance->parent->window_def;
	if (window_def == NULL)
		return;

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;

	icon = column_find_icon_from_xpos(instance->columns, xpos);
	if (icon == wimp_ICON_WINDOW)
		return;

	if (!icons_extract_validation_command(buffer, IHELP_INAME_LEN, window_def->icons[icon].data.indirected_text.validation, 'N'))
		string_printf(buffer, IHELP_INAME_LEN, "Col%d", icon);
}


/**
 * Process menu prepare events in a list window instance.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct list_window	*instance;
	int			line, index = LIST_WINDOW_NULL_INDEX;
	wimp_window_state	window;

	instance = event_get_window_user_data(w);
	if (instance == NULL || instance->parent == NULL)
		return;

	if (pointer != NULL) {
		instance->menu_line = -1;

		if (w == instance->window) {
			window.w = w;
			wimp_get_window_state(&window);

			line = window_calculate_click_row(&(pointer->pos), &window,
					instance->parent->definition->toolbar_height, instance->display_lines);

			if (line != -1) {
				instance->menu_line = line;

				if (line < instance->display_lines)
					index = instance->line_data[line].index;
			}
		}
	}

	if (instance->parent->definition->callback_menu_prepare_handler != NULL)
		instance->parent->definition->callback_menu_prepare_handler(w, menu, pointer, index,
				instance->file, instance->client_data);

	list_window_force_redraw(instance, instance->menu_line, instance->menu_line, wimp_ICON_WINDOW);
}


/**
 * Process menu prepare events in a list window instance.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 * \param index		The index of the entry under the menu, or LIST_WINDOW_NULL_INDEX.
 * \param *file		The file owning the preset list window.
 * \param *data		The preset list window instance.
 */

static void list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct list_window	*instance;
	wimp_pointer		pointer;
	int			index = LIST_WINDOW_NULL_INDEX;

	instance = event_get_window_user_data(w);
	if (instance == NULL || instance->parent == NULL)
		return;

	if (instance->parent->definition->callback_menu_selection_handler == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	if (instance->menu_line >= 0 && instance->menu_line < instance->display_lines)
		index = instance->line_data[instance->menu_line].index;

	instance->parent->definition->callback_menu_selection_handler(w, menu, selection,
			&pointer, index, instance->file, instance->client_data);
}


/**
 * Process submenu warning events in a list window instance.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct list_window	*instance;
	int			index = LIST_WINDOW_NULL_INDEX;

	instance = event_get_window_user_data(w);
	if (instance == NULL || instance->parent == NULL)
		return;

	if (instance->parent->definition->callback_menu_warning_handler == NULL)
		return;


	if (instance->menu_line >= 0 && instance->menu_line < instance->display_lines)
		index = instance->line_data[instance->menu_line].index;

	instance->parent->definition->callback_menu_warning_handler(w, menu, warning,
			index, instance->file, instance->client_data);
}


/**
 * Process menu close events in a list window instance.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void list_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	struct list_window	*instance;

	instance = event_get_window_user_data(w);
	if (instance == NULL)
		return;

	list_window_force_redraw(instance, instance->menu_line, instance->menu_line, wimp_ICON_WINDOW);

	instance->menu_line = -1;
}


/**
 * Callback handler for completing the drag of a column heading.
 *
 * \param *data			The window block for the origin of the drag.
 * \param group			The column group which has been dragged.
 * \param width			The new width for the group.
 */

static void list_window_adjust_columns(void *data, wimp_i group, int width)
{
	struct list_window	*instance = data;
	int			new_extent;
	wimp_window_info	window;
	wimp_caret		caret;

	if (instance == NULL || instance->parent == NULL)
		return;

	columns_update_dragged(instance->columns, instance->toolbar, NULL, group, width);

	new_extent = column_get_window_width(instance->columns);

	list_window_adjust_sort_icon(instance);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	if (instance->edit_line != NULL) {
		wimp_get_caret_position(&caret);
		edit_place_new_line(instance->edit_line, -1, wimp_COLOUR_BLACK);
	}

	windows_redraw(instance->window);

	if (instance->toolbar != NULL)
		windows_redraw(instance->toolbar);

	/* If the caret's position was in the current transaction window, we need to replace it in the same position
	 * now, so that we don't lose input focus.
	 */

	if (instance->edit_line != NULL && instance->window == caret.w)
		wimp_set_caret_position(caret.w, caret.i, 0, 0, -1, caret.index);

	/* Set the horizontal extent of the window and pane. */

	if (instance->toolbar != NULL) {
		window.w = instance->toolbar;
		wimp_get_window_info_header_only(&window);
		window.extent.x1 = window.extent.x0 + new_extent;
		wimp_set_extent(window.w, &(window.extent));
	}

	window.w = instance->window;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_extent;
	wimp_set_extent(window.w, &(window.extent));

	windows_open(window.w);

	file_set_data_integrity(instance->file, TRUE);
}


/**
 * Adjust the sort icon in a list window, to reflect the current column
 * heading positions.
 *
 * \param *instance		The window instance to be updated.
 */

static void list_window_adjust_sort_icon(struct list_window *instance)
{
	wimp_icon_state		icon;

	if (instance == NULL || instance->parent == NULL ||
			instance->parent->definition == NULL || instance->toolbar == NULL)
		return;

	icon.w = instance->toolbar;
	icon.i = instance->parent->definition->sort_dir_icon;
	wimp_get_icon_state(&icon);

	list_window_adjust_sort_icon_data(instance, &(icon.icon));

	wimp_resize_icon(icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
			icon.icon.extent.x1, icon.icon.extent.y1);
}


/**
 * Adjust an icon definition to match the current preset sort settings.
 *
 * \param *file			The preset window to be updated.
 * \param *icon			The icon to be updated.
 */

static void list_window_adjust_sort_icon_data(struct list_window *instance, wimp_icon *icon)
{
	enum sort_type	sort_order;

	if (instance == NULL || instance->parent == NULL || instance->parent->toolbar_def == NULL)
		return;

	sort_order = sort_get_order(instance->sort);

	column_update_sort_indicator(instance->columns, icon, instance->parent->toolbar_def, sort_order);
}


/**
 * Set the extent of the list window for the specified instance.
 *
 * \param *instance		The list window to update.
 */

static void list_window_set_extent(struct list_window *instance)
{
	int	lines;

	if (instance == NULL || instance->parent == NULL || instance->window == NULL)
		return;

	lines = (instance->display_lines > instance->parent->definition->minimum_entries) ?
			instance->display_lines : instance->parent->definition->minimum_entries;

	window_set_extent(instance->window, lines, instance->parent->definition->toolbar_height,
			column_get_window_width(instance->columns));
}


/**
 * Recreate the title of the given list window.
 *
 * \param *instance		The list window to rebuild the title for.
 */

static void list_window_build_title(struct list_window *instance)
{
	char name[WINDOW_TITLE_LENGTH];

	if (instance == NULL || instance->file == NULL)
		return;

	if (instance->parent->definition->window_title == NULL) {
		file_get_pathname(instance->file, instance->title, WINDOW_TITLE_LENGTH - 2);

		if (file_get_data_integrity(instance->file))
			strcat(instance->title, " *");
	} else {
		file_get_leafname(instance->file, name, WINDOW_TITLE_LENGTH);

		msgs_param_lookup(instance->parent->definition->window_title, instance->title,
				WINDOW_TITLE_LENGTH, name, NULL, NULL, NULL);
	}

	if (instance->window != NULL)
		wimp_force_redraw_title(instance->window);
}


/**
 * Initialise the contents of the list window, creating an entry for
 * each of the required entries.
 *
 * \param *instance		The list window instance to initialise.
 * \param entries		The number of entries to insert.
 * \return			TRUE on success; FALSE on failure.
 */

osbool list_window_initialise_entries(struct list_window *instance, int entries)
{
	int i;

	if (instance == NULL || instance->line_data == NULL)
		return FALSE;

	/* Extend the index array. */

	if (!flexutils_resize((void **) &(instance->line_data), sizeof(struct list_window_redraw), entries))
		return FALSE;

	instance->display_lines = entries;

	/* Initialise and sort the entries. */

	for (i = 0; i < entries; i++)
		instance->line_data[i].index = i;

	list_window_sort(instance);

	return TRUE;
}


/**
 * Add a new entry to a list window instance.
 *
 * \param *instance		The list window instance to add to.
 * \param entry			The index to add.
 * \param sort			TRUE to sort the entries after addition; otherwsie FALSE.
 * \return			TRUE on success; FALSE on failure.
 */

osbool list_window_add_entry(struct list_window *instance, int entry, osbool sort)
{
	if (instance == NULL || instance->line_data == NULL)
		return FALSE;

	/* Extend the index array. */

	if (!flexutils_resize((void **) &(instance->line_data), sizeof(struct list_window_redraw), instance->display_lines + 1))
		return FALSE;

	instance->display_lines++;

	/* Add the new entry, expand the window and sort the entries. */

	instance->line_data[instance->display_lines - 1].index = entry;

	list_window_set_extent(instance);

	if (sort)
		list_window_sort(instance);
	else
		list_window_force_redraw(instance, instance->display_lines - 1, instance->display_lines - 1, wimp_ICON_WINDOW);

	return TRUE;
}


/**
 * Remove an entry from a list window instance, and update the other entries to
 * allow for its deletion.
 *
 * \param *instance		The list window instance to remove from.
 * \param entry			The entry index to remove.
 * \param sort			TRUE to sort the entries after deletion; otherwsie FALSE.
 * \return			TRUE on success; FALSE on failure.
 */

osbool list_window_delete_entry(struct list_window *instance, int entry, osbool sort)
{
	int i = 0, delete = -1;

	if (instance == NULL || instance->line_data == NULL)
		return FALSE;

	/* Find the preset, and decrement any index entries above it. */

	for (i = 0; i < instance->display_lines; i++) {
		if (instance->line_data[i].index == entry)
			delete = i;
		else if (instance->line_data[i].index > entry)
			instance->line_data[i].index--;
	}

	/* Delete the index entry. */

	if (delete >= 0 && !flexutils_delete_object((void **) &(instance->line_data), sizeof(struct list_window_redraw), delete))
		return FALSE;

	instance->display_lines--;

	/* Update the preset window. */

	list_window_set_extent(instance);

	windows_open(instance->window);

	if (sort)
		list_window_sort(instance);
	else
		list_window_force_redraw(instance, delete, instance->display_lines, wimp_ICON_WINDOW);

	return TRUE;
}


/**
 * Find the display line in a list window which points to the specified
 * index under the applied sort.
 *
 * \param *instance		The list window instance to search.
 * \param index			The index to return the line for.
 * \return			The appropriate line, or -1 if not found.
 */

static int list_window_get_line_from_index(struct list_window *instance, int index)
{
	int	i;
	int	line = -1;

	if (instance == NULL || instance->line_data == NULL)
		return line;

	for (i = 0; i < instance->display_lines; i++) {
		if (instance->line_data[i].index == index) {
			line = i;
			break;
		}
	}

	return line;
}


/**
 * Find the index which corresponds to a display line in the specified
 * list window instance.
 *
 * \param *windat		The list window instance to search in.
 * \param line			The display line to return the index for.
 * \return			The appropriate index, or LIST_WINDOW_NULL_INDEX.
 */

int list_window_get_index_from_line(struct list_window *instance, int line)
{
	if (instance == NULL || instance->line_data == NULL || !list_window_line_valid(instance, line))
		return LIST_WINDOW_NULL_INDEX;

	return instance->line_data[line].index;
}


/**
 * Open the sort dialogue for a given list window instance.
 *
 * \param *file			The preset window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

void list_window_open_sort_window(struct list_window *instance, wimp_pointer *ptr)
{
	if (instance == NULL || ptr == NULL)
		return;

	sort_dialogue_open(instance->parent->sort_dialogue, ptr, sort_get_order(instance->sort), instance);
}


/**
 * Take the contents of an updated sort dialogue and process the data.
 *
 * \param order			The new sort order selected by the user.
 * \param *data			The preset window associated with the sort dialogue.
 * \return			TRUE if successful; else FALSE.
 */

static osbool list_window_process_sort_window(enum sort_type order, void *data)
{
	struct list_window *instance = data;

	if (instance == NULL)
		return FALSE;

	sort_set_order(instance->sort, order);

	list_window_adjust_sort_icon(instance);
	windows_redraw(instance->toolbar);
	list_window_sort(instance);

	return TRUE;
}


/**
 * Sort the entries in a given list window based on that instance's sort
 * setting.
 *
 * \param *instance		The list window instance to sort.
 */

void list_window_sort(struct list_window *instance)
{
	if (instance == NULL)
		return;

	#ifdef DEBUG
	debug_printf("Sorting list window");
	#endif

	hourglass_on();

	/* Run the sort. */

	sort_process(instance->sort, instance->display_lines);

	list_window_force_redraw(instance, 0, instance->display_lines - 1, wimp_ICON_WINDOW);

	hourglass_off();
}


/**
 * Compare two lines of a list window, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The number of the first line to be compared.
 * \param index2		The number of the second line to be compared.
 * \param *data			Client specific data, which is our window instance.
 * \return			The comparison result.
 */

static int list_window_sort_compare(enum sort_type type, int index1, int index2, void *data)
{
	struct list_window	*instance = data;

	if (instance == NULL || instance->parent == NULL || instance->line_data == NULL)
		return 0;

	if (instance->parent->definition->callback_sort_compare == NULL)
		return 0;

	return instance->parent->definition->callback_sort_compare(type,
			instance->line_data[index1].index, instance->line_data[index2].index, instance->file);
}


/**
 * Swap the sort index of two lines of a list window instance.
 *
 * \param index1		The index of the first line to be swapped.
 * \param index2		The index of the second line to be swapped.
 * \param *data			Client specific data, which is our instance block.
 */

static void list_window_sort_swap(int index1, int index2, void *data)
{
	struct list_window	*instance = data;
	int			temp;

	if (instance == NULL)
		return;

	temp = instance->line_data[index1].index;
	instance->line_data[index1].index = instance->line_data[index2].index;
	instance->line_data[index2].index = temp;
}


/**
 * Open the Print dialogue for a given list window instance.
 *
 * \param *instance		The list window instance to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to retain the previous settings; FALSE to
 *				return to defaults.
 */

void list_window_open_print_window(struct list_window *instance, wimp_pointer *ptr, osbool restore)
{
	if (instance == NULL || instance->file == NULL || instance->parent == NULL)
		return;

	print_dialogue_open(instance->file->print, ptr, instance->parent->definition->print_dates, restore,
			instance->parent->definition->print_title, instance->parent->definition->print_report_title,
			instance, list_window_print, instance);
}


/**
 * Send the contents of the list window to the printer, via the reporting
 * system.
 *
 * \param *report		The report handle to use for output.
 * \param *data			The list window instance to be printed.
 * \param from			The date to print from.
 * \param to			The date to print to.
 * \return			Pointer to the report, or NULL on failure.
 */

static struct report *list_window_print(struct report *report, void *data, date_t from, date_t to)
{
	struct list_window	*instance = data;
	int			line, column;
	char			rec_char[REC_FIELD_LEN];
	wimp_i			*columns;

	if (report == NULL || instance == NULL || instance->parent == NULL)
		return NULL;

	if (instance->parent->definition->callback_print_field == NULL)
		return NULL;

	columns = heap_alloc(sizeof(wimp_i) * instance->parent->definition->column_count);
	if (columns == NULL)
		return NULL;

	if (!column_get_icons(instance->columns, columns, instance->parent->definition->column_count, FALSE))
		return NULL;

	msgs_lookup("RecChar", rec_char, REC_FIELD_LEN);

	hourglass_on();

	/* Output the page title. */

	stringbuild_reset();

	stringbuild_add_string("\\b\\u");
	stringbuild_add_message_param("PresetTitle",
			file_get_leafname(instance->file, NULL, 0),
			NULL, NULL, NULL);

	stringbuild_report_line(report, 1);

	report_write_line(report, 1, "");

	/* Output the headings line, taking the text from the window icons. */

	stringbuild_reset();
	columns_print_heading_names(instance->columns, instance->toolbar);
	stringbuild_report_line(report, 0);

	/* Output the standing order data as a set of delimited lines. */

	for (line = 0; line < instance->display_lines; line++) {
		stringbuild_reset();

		for (column = 0; column < instance->parent->definition->column_count; column++) {
			if (column == 0)
				stringbuild_add_string("\\k");
			else
				stringbuild_add_string("\\t");

			instance->parent->definition->callback_print_field(instance->file,
					columns[column], instance->line_data[line].index, rec_char);
		}

		stringbuild_report_line(report, 0);
	}

	heap_free(columns);

	hourglass_off();

	return report;
}


/**
 * Export the data from a list window into CSV or TSV format.
 *
 * \param *instance		The list window instance to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

void list_window_export_delimited(struct list_window *instance, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE	*out;
	int	line;
	char	buffer[FILING_DELIMITED_FIELD_LEN];

	if (instance == NULL || instance->parent == NULL)
		return;

	if (instance->parent->definition->callback_export_line == NULL)
		return;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	/* Output the headings line, taking the text from the window icons. */

	columns_export_heading_names(instance->columns, instance->toolbar, out, format, buffer, FILING_DELIMITED_FIELD_LEN);

	/* Output the preset data as a set of delimited lines. */

	for (line = 0; line < instance->display_lines; line++)
		instance->parent->definition->callback_export_line(out, format, instance->file, instance->line_data[line].index);

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}


/**
 * Save the list window details from a given instance to a CashBook file.
 * This assumes that the caller has already created a suitable section
 * in the file to be written.
 *
 * \param *instance		The window instance whose details to write.
 * \param *out			The file handle to write to.
 */

void list_window_write_file(struct list_window *instance, FILE *out)
{
	char	buffer[FILING_MAX_FILE_LINE_LEN];

	if (instance == NULL)
		return;

	if (instance->columns != NULL) {
		column_write_as_text(instance->columns, buffer, FILING_MAX_FILE_LINE_LEN);
		fprintf(out, "WinColumns: %s\n", buffer);
	}

	if (instance->sort != NULL) {
		sort_write_as_text(instance->sort, buffer, FILING_MAX_FILE_LINE_LEN);
		fprintf(out, "SortOrder: %s\n", buffer);
	}
}


/**
 * Process a WinColumns line from a file.
 *
 * \param *instance		The instance being read in to.
 * \param start			The first column to read in from the string.
 * \param skip			TRUE to ignore missing entries; FALSE to set to default.
 * \param *columns		The column text line.
 */

void list_window_read_file_wincolumns(struct list_window *instance, int start, osbool skip, char *columns)
{
	if (instance == NULL || instance->columns == NULL)
		return;

	column_init_window(instance->columns, start, skip, columns);
}


/**
 * Process a SortOrder line from a file.
 *
 * \param *instance		The instance being read in to.
 * \param *order		The order text line.
 */

void list_window_read_file_sortorder(struct list_window *instance, char *order)
{
	if (instance == NULL || instance->sort == NULL)
		return;

	sort_read_from_text(instance->sort, order);
}
