/* CashBook - presets.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_PRESETS
#define CASHBOOK_PRESETS


#define PRESET_PANE_COL_MAP "0;1;2,3,4;5,6,7;8;9"
#define PRESET_PANE_SORT_DIR_ICON 10

/* Caret end locations */

#define PRESET_CARET_DATE 0
#define PRESET_CARET_FROM 1
#define PRESET_CARET_TO 2
#define PRESET_CARET_REFERENCE 3
#define PRESET_CARET_AMOUNT 4
#define PRESET_CARET_DESCRIPTION 5

/**
 * Initialise the preset system.
 *
 * \param *sprites		The application sprite area.
 */

void preset_initialise(osspriteop_area *sprites);


/**
 * Create and open a Preset List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void preset_open_window(file_data *file);


/**
 * Close and delete the Preset List Window associated with the given
 * file block.
 *
 * \param *file			The file to use.
 */

void preset_delete_window(file_data *file);


/**
 * Recreate the title of the Preset List window connected to the given file.
 *
 * \param *file			The file to rebuild the title for.
 */

void preset_build_window_title(file_data *file);


/**
 * Force a redraw of the Preset list window, for the given range of lines.
 *
 * \param *file			The file owning the window.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

void preset_force_window_redraw(file_data *file, int from, int to);


/**
 * Open the Preset Edit dialogue for a given preset list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void preset_open_edit_window(file_data *file, int preset, wimp_pointer *ptr);


/**
 * Force the closure of the Preset sort and edit windows if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void preset_force_windows_closed(file_data *file);


/**
 * Sort the presets in a given file based on that file's sort setting.
 *
 * \param *file			The file to sort.
 */

void preset_sort(file_data *file);


/**
 * Find a preset index based on its shortcut key.  If the key is '\0', or no
 * match is found, NULL_PRESET is returned.
 *
 * \param *file			The file to search in.
 * \param key			The shortcut key to search for.
 * \return			The matching preset index, or NULL_PRESET.
 */

int preset_find_from_keypress(file_data *file, char key);

#endif

