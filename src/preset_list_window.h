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
 * \file: preset_list_window.h
 *
 * Transaction Preset List Window interface.
 */

#ifndef CASHBOOK_PRESET_LIST_WINDOW
#define CASHBOOK_PRESET_LIST_WINDOW

/**
 * Initialise the Preset List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void preset_list_window_initialise(osspriteop_area *sprites);


/**
 * Create a new Preset List Window instance.
 *
 * \param *parent		The parent presets instance.
 * \return			Pointer to the new instance, or NULL.
 */

struct preset_list_window *preset_list_window_create_instance(struct preset_block *parent);


/**
 * Destroy a Preset List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void preset_list_window_delete_instance(struct preset_list_window *windat);


/**
 * Create and open a Preset List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void preset_list_window_open(struct preset_list_window *windat);


/**
 * Recreate the title of the Preset List window connected to the given file.
 *
 * \param *windat		The preset window to rebuild the title for.
 */

void preset_list_window_build_title(struct preset_list_window *windat);


/**
 * Force the complete redraw of the Preset list window.
 *
 * \param *file			The file owning the window to redraw.
 */

void preset_list_window_redraw_all(struct preset_list_window *windat);


/**
 * Find the preset which corresponds to a display line in the specified
 * preset list window.
 *
 * \param *file			The preset list window to search in.
 * \param line			The display line to return the preset for.
 * \return			The appropriate transaction, or NULL_PRESET.
 */

preset_t preset_list_window_get_preset_from_line(struct preset_list_window *windat, int line);

#endif

