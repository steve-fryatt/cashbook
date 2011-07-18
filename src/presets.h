/* CashBook - presets.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_PRESETS
#define CASHBOOK_PRESETS

/* ==================================================================================================================
 * Static constants
 */

/* Preset Edit icons. */

#define PRESET_EDIT_OK          0
#define PRESET_EDIT_CANCEL      1
#define PRESET_EDIT_DELETE      2

#define PRESET_EDIT_NAME        4
#define PRESET_EDIT_KEY         6
#define PRESET_EDIT_DATE        10
#define PRESET_EDIT_TODAY       11
#define PRESET_EDIT_FMIDENT     14
#define PRESET_EDIT_FMREC       15
#define PRESET_EDIT_FMNAME      16
#define PRESET_EDIT_TOIDENT     19
#define PRESET_EDIT_TOREC       20
#define PRESET_EDIT_TONAME      21
#define PRESET_EDIT_REF         24
#define PRESET_EDIT_CHEQUE      25
#define PRESET_EDIT_AMOUNT      28
#define PRESET_EDIT_DESC        31
#define PRESET_EDIT_CARETDATE   12
#define PRESET_EDIT_CARETFROM   17
#define PRESET_EDIT_CARETTO     22
#define PRESET_EDIT_CARETREF    26
#define PRESET_EDIT_CARETAMOUNT 29
#define PRESET_EDIT_CARETDESC   32

/* Toolbar icons. */

#define PRESET_PANE_KEY 0
#define PRESET_PANE_NAME 1
#define PRESET_PANE_FROM 2
#define PRESET_PANE_TO 3
#define PRESET_PANE_AMOUNT 4
#define PRESET_PANE_DESCRIPTION 5

#define PRESET_PANE_PARENT 6
#define PRESET_PANE_ADDPRESET 7
#define PRESET_PANE_PRINT 8
#define PRESET_PANE_SORT 9

#define PRESET_PANE_COL_MAP "0;1;2,3,4;5,6,7;8;9"
#define PRESET_PANE_SORT_DIR_ICON 10

/* Preset Sort Window */

#define PRESET_SORT_OK 2
#define PRESET_SORT_CANCEL 3
#define PRESET_SORT_FROM 4
#define PRESET_SORT_TO 5
#define PRESET_SORT_AMOUNT 6
#define PRESET_SORT_DESCRIPTION 7
#define PRESET_SORT_KEY 8
#define PRESET_SORT_NAME 9
#define PRESET_SORT_ASCENDING 10
#define PRESET_SORT_DESCENDING 11

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











void adjust_preset_window_columns (file_data *file);
void adjust_preset_window_sort_icon (file_data *file);
void update_preset_window_sort_icon (file_data *file, wimp_icon *icon);

/* Sorting presets */

void sort_preset_window (file_data *file);

void open_preset_sort_window (file_data *file, wimp_pointer *ptr);
void fill_preset_sort_window (int sort_option);
void force_close_preset_sort_window (file_data *file);

/* Adding new presets. */

int add_preset (file_data *file);
int delete_preset (file_data *file, int preset_no);

/* Editing presets via the GUI */

void open_preset_edit_window (file_data *file, int preset, wimp_pointer *ptr);
void fill_preset_edit_window (file_data *file, int preset);
void update_preset_edit_account_fields (wimp_key *key);
void open_preset_edit_account_menu (wimp_pointer *ptr);
void toggle_preset_edit_reconcile_fields (wimp_pointer *ptr);
int delete_preset_from_edit_window (void);
void force_close_preset_edit_window (file_data *file);

/* Printing Presets via the GUI. */

void open_preset_print_window (file_data *file, wimp_pointer *ptr, int clear);

/* Preset handling */

int find_preset_from_keypress (file_data *file, char key);


/* File and print output */

void print_preset_window(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);

/* Preset window handling */

void set_preset_window_extent (file_data *file);
void build_preset_window_title (file_data *file);
void force_preset_window_redraw (file_data *file, int from, int to);
void decode_preset_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);



#endif

