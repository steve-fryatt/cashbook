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

struct list_window_instance {
	/**
	 * Wimp window handle for the main List Window.
	 */
	wimp_w					sorder_window;

	/**
	 * Indirected title data for the window.
	 */
	char					window_title[WINDOW_TITLE_LENGTH];

	/**
	 * Wimp window handle for the List Window Toolbar pane.
	 */
	wimp_w					sorder_pane;

	/**
	 * Instance handle for the window's column definitions.
	 */
	struct column_block			*columns;

};


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
