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

struct list_window_instance;

/**
 * Definition of a list window object.
 */

struct list_window_definition {
	/**
	 * The name of the template for the list window itself.
	 */
	char		*main_template_name;

	/**
	 * The name of the template for the toolbar pane.
	 */
	char		*toolbar_template_name;

	/**
	 * The name of the template for the footer pane.
	 */
	char		*footer_template_name;
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

wimp_window *list_window_get_window_def(struct list_window_block *block);
wimp_window *list_window_get_toolbar_def(struct list_window_block *block);

#endif
