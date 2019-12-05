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
	 * Count of the number of populated display lines in the window.
	 */
	int				display_lines;


};

/* Static Function Prototypes. */

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

	return block;
}


/**
 * Create a new List Window instance.
 *
 * \param *parent		The List Window to own the instance.
 * \return			Pointer to the new instance, or NULL.
 */
struct list_window *list_window_create_instance(struct list_window_block *parent)
{
	struct list_window	*new;

	new = heap_alloc(sizeof(struct list_window));
	if (new == NULL)
		return NULL;

	new->parent = parent;

	new->window = NULL;
	new->toolbar = NULL;
	new->footer = NULL;
	new->columns = NULL;
	new->display_lines = 0;

	/* Initialise the window columns. */

	new-> columns = column_create_instance(parent->definition->column_count, parent->definition->column_map,
			parent->definition->column_extra, parent->definition->sort_dir_icon);
	if (new->columns == NULL) {
		list_window_delete_instance(new);
		return NULL;
	}

	column_set_minimum_widths(new->columns, parent->definition->column_limits);
	column_init_window(new->columns, 0, FALSE, parent->definition->column_widths);

	return new;
}


/**
 * Delete a List Window instance.
 *
 * \param *instance		The List Window instance to delete.
 */
void list_window_delete_instance(struct list_window *instance)
{
	if (instance == NULL)
		return;

	column_delete_instance(instance->columns);

	heap_free(instance);
}


osbool list_window_open(struct list_window *instance)
{
	int			height;
	os_error		*error;
	wimp_window_state	parent;
	struct file_block	*file;

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

	transact_get_window_state(file, &parent);

	window_set_initial_area(instance->parent->window_def, column_get_window_width(instance->columns),
			(height * WINDOW_ROW_HEIGHT) + instance->parent->definition->toolbar_height,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window(instance->parent->window_def, &(instance->window));
	if (error != NULL) {
		preset_list_window_delete(windat); // TODO
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
				(osspriteop_id) windat->sort_sprite;
		instance->parent->toolbar_def->icons[instance->parent->definition->sort_dir_icon].data.indirected_sprite.area =
				instance->parent->toolbar_def->sprite_area;
		instance->parent->toolbar_def->icons[instance->parent->definition->sort_dir_icon].data.indirected_sprite.size = COLUMN_SORT_SPRITE_LEN;

		preset_list_window_adjust_sort_icon_data(windat, &(instance->parent->toolbar_def->icons[instance->parent->definition->sort_dir_icon]));

		#ifdef DEBUG
		debug_printf ("Toolbar icons adjusted...");
		#endif

		error = xwimp_create_window(instance->parent->toolbar_def, &(windat->preset_pane));
		if (error != NULL) {
			preset_list_window_delete(windat); // TODO
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return FALSE;
		}
	}

	/* Set the title */

	preset_list_window_build_title(windat);

	/* Set up the interactive help. */

	if (instance->window != NULL)
		ihelp_add_window(instance->window , instance->parent->definition->window_help, list_window_decode_help);

	if (instance->toolbar != NULL)
		ihelp_add_window(instance->toolbar , instance->parent->definition->toolbar_help, NULL);

	if (instance->footer != NULL)
		ihelp_add_window(instance->footer , instance->parent->definition->footer_help, NULL);

	/* Open the window. */

	windows_open(windat->preset_window);
	windows_open_nested_as_toolbar(windat->preset_pane,
			windat->preset_window, instance->parent->definition->toolbar_height - 4, FALSE);

	/* Register event handlers for the two windows. */

	event_add_window_user_data(windat->preset_window, windat);
	event_add_window_menu(windat->preset_window, preset_list_window_menu);
	event_add_window_close_event(windat->preset_window, list_window_close_handler);
	event_add_window_mouse_event(windat->preset_window, list_window_click_handler);
	event_add_window_scroll_event(windat->preset_window, list_window_scroll_handler);
	event_add_window_redraw_event(windat->preset_window, list_window_redraw_handler);
	event_add_window_menu_prepare(windat->preset_window, list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->preset_window, list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->preset_window, list_window_menu_warning_handler);
	event_add_window_menu_close(windat->preset_window, list_window_menu_close_handler);

	event_add_window_user_data(windat->preset_pane, windat);
	event_add_window_menu(windat->preset_pane, preset_list_window_menu);
	event_add_window_mouse_event(windat->preset_pane, list_window_pane_click_handler);
	event_add_window_menu_prepare(windat->preset_pane, list_window_menu_prepare_handler);
	event_add_window_menu_selection(windat->preset_pane, list_window_menu_selection_handler);
	event_add_window_menu_warning(windat->preset_pane, list_window_menu_warning_handler);
	event_add_window_menu_close(windat->preset_pane, list_window_menu_close_handler);
}










static void list_window_close_handler(wimp_close *close)
{

}

static void list_window_click_handler(wimp_pointer *pointer)
{

}

static void list_window_pane_click_handler(wimp_pointer *pointer)
{

}

static void list_window_scroll_handler(wimp_scroll *scroll)
{

}

static void list_window_redraw_handler(wimp_draw *redraw)
{

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
