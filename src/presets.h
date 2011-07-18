/* CashBook - presets.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_PRESETS
#define CASHBOOK_PRESETS

/* ==================================================================================================================
 * Static constants
 */


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













void adjust_preset_window_columns (file_data *file);
void adjust_preset_window_sort_icon (file_data *file);
void update_preset_window_sort_icon (file_data *file, wimp_icon *icon);

/* Sorting presets */

void sort_preset_window (file_data *file);







/* Adding new presets. */

int add_preset (file_data *file);
int delete_preset (file_data *file, int preset_no);



/* Preset handling */

int find_preset_from_keypress (file_data *file, char key);


/* File and print output */

void print_preset_window(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);




#endif

