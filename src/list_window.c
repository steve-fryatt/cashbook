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

#include "flexutils.h"
#include "window.h"

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
	 * The Preset List Window Sort callbacks.
	 */

	struct sort_callback		sort_callbacks;
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
	 * Flex array holding the line data for the window.
	 */
	struct list_window_redraw	*line_data;

	/**
	 * Pointer to the next list window instance in the list.
	 */
	struct list_window		*next;
};

/**
 * Linked list of dialogue instances.
 */

static struct list_window *instance_list = NULL;

/* Static Function Prototypes. */

static void list_window_delete(struct list_window *instance);
static void list_window_close_handler(wimp_close *close);
static void list_window_click_handler(wimp_pointer *pointer);
static void list_window_pane_click_handler(wimp_pointer *pointer);
static void list_window_scroll_handler(wimp_scroll *scroll);
static void list_window_redraw_handler(wimp_draw *redraw);
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
	struct list_window_block *block;

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

	/* Set up the sort callbacks. */

	block->sort_callbacks.compare = NULL;
	block->sort_callbacks.swap = NULL;

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
	new->columns = NULL;
	new->sort = NULL;

	new->display_lines = 0;
	new->line_data = NULL;

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

	new->next = instance_list;
	instance_list = new;

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

	list = &instance_list;

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
	struct list_window *list = instance_list;

	while (list != NULL) {
	//	if (list->file == file)
	//		list_window_re()

		list = list->next;
	}
}


/**
 * Rebuild the titles of all list window instances beloinging
 * to a file.
 * 
 * \param *file			The file to be updated.
 */

void list_window_rebuild_file_titles(struct file_block *file)
{
	struct list_window *list = instance_list;

	while (list != NULL) {
		if (list->file == file)
			list_window_build_title(list);

		list = list->next;
	}
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
	debug_printf("\\CCreating preset window");
	#endif

	/* Create the new window data and build the window. */

	*(instance->title) = '\0';
	instance->parent->window_def->title_data.indirected_text.text = instance->title;

	height = (instance->display_lines > instance->parent->definition->minimum_entries) ?
			instance->display_lines : instance->parent->definition->minimum_entries;

	transact_get_window_state(instance->file, &parent);

	window_set_initial_area(instance->parent->window_def, column_get_window_width(instance->columns),
			(height * WINDOW_ROW_HEIGHT) + instance->parent->definition->toolbar_height,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(instance->file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

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
//	event_add_window_menu(instance->window, preset_list_window_menu);
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
//		event_add_window_menu(instance->toolbar, preset_list_window_menu);
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
// \TODO
//	dialogue_force_all_closed(NULL, instance);
//	sort_dialogue_close(preset_list_window_sort_dialogue, windat);

	/* Allow the client to tidy up if it needs to. */

	if (instance->parent->definition->callback_window_close_handler != NULL)
		instance->parent->definition->callback_window_close_handler(instance->client_data);
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
	//			list_window_sort(instance);
			}
		}
	} else if (pointer->buttons == wimp_DRAG_SELECT && column_is_heading_draggable(instance->columns, pointer->i)) {
		column_set_minimum_widths(instance->columns, config_str_read(instance->parent->definition->column_limits));
		column_start_drag(instance->columns, pointer, instance, instance->toolbar, list_window_adjust_columns);
	}
}

static void list_window_scroll_handler(wimp_scroll *scroll)
{

}

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

//	if (redraw->w == event_get_current_menu_window())
//		select = preset_list_window_menu_line;
//	else
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





static void list_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{

}

static void list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{

}

static void list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{

}

static void list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{

}

static void list_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{

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

	if (instance == NULL || instance->parent == NULL)
		return;

	columns_update_dragged(instance->columns, instance->toolbar, NULL, group, width);

	new_extent = column_get_window_width(instance->columns);

	list_window_adjust_sort_icon(instance);

	/* Replace the edit line to force a redraw and redraw the rest of the window. */

	windows_redraw(instance->window);

	if (instance->toolbar != NULL)
		windows_redraw(instance->toolbar);

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
	char			name[WINDOW_TITLE_LENGTH];

	if (instance == NULL || instance->file == NULL)
		return;

	file_get_leafname(instance->file, name, WINDOW_TITLE_LENGTH);

	msgs_param_lookup(instance->parent->definition->window_title, instance->title,
			WINDOW_TITLE_LENGTH, name, NULL, NULL, NULL);

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

//	preset_list_window_sort(windat);

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

//	if (sort)
//		preset_list_window_sort(windat);
//	else
//		preset_list_window_force_redraw(windat, windat->display_lines - 1, windat->display_lines - 1, wimp_ICON_WINDOW);

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

//	if (sort)
//		preset_list_window_sort(windat);
//	else
//		preset_list_window_force_redraw(windat, delete, windat->display_lines, wimp_ICON_WINDOW);

	return TRUE;
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
	if (instance == NULL)
		return;

	column_init_window(instance->columns, start, skip, columns);
}





wimp_window *list_window_get_window_def(struct list_window_block *block)
{
	if (block == NULL)
		return NULL;

	return block->window_def;
}

wimp_window *list_window_get_toolbar_def(struct list_window_block *block)
{
	if (block == NULL)
		return NULL;

	return block->toolbar_def;
}
