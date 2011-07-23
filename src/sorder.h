/* CashBook - sorder.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_SORDER
#define CASHBOOK_SORDER


#define SORDER_PANE_COL_MAP "0,1,2;3,4,5;6;7;8;9"
#define SORDER_PANE_SORT_DIR_ICON 10


/**
 * Initialise the standing order system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_initialise(osspriteop_area *sprites);


/**
 * Create and open a Standing Order List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void sorder_open_window(file_data *file);


/**
 * Close and delete the Standing order List Window associated with the given
 * file block.
 *
 * \param *file			The file to use.
 */

void sorder_delete_window(file_data *file);


/**
 * Force a redraw of the Standing Order list window, for the given range of
 * lines.
 *
 * \param *file			The file owning the window.
 * \param from			The first line to redraw, inclusive.
 * \param to			The last line to redraw, inclusive.
 */

void sorder_force_window_redraw(file_data *file, int from, int to);


/**
 * Open the Standing Order Edit dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void sorder_open_edit_window(file_data *file, int sorder, wimp_pointer *ptr);


/**
 * Force the closure of the Standing Order sort and edit windows if the owning
 * file disappears.
 *
 * \param *file			The file which has closed.
 */

void sorder_force_windows_closed(file_data *file)





/* Sorting Standing Orders */

void sort_sorder_window (file_data *file);



/* Adding new standing orders */

int add_sorder (file_data *file);
int delete_sorder (file_data *file, int sorder_no);

/* Editing standing orders via GUI */

void open_sorder_edit_window (file_data *file, int sorder, wimp_pointer *ptr);
void fill_sorder_edit_window (file_data *date, int sorder, int edit_mode);

void refresh_sorder_edit_window ();

void update_sorder_edit_account_fields (wimp_key *key);
void open_sorder_edit_account_menu (wimp_pointer *ptr);
void toggle_sorder_edit_reconcile_fields (wimp_pointer *ptr);

int process_sorder_edit_window (void);
int stop_sorder_from_edit_window (void);
int delete_sorder_from_edit_window (void);


/* Standing order processing. */

void process_standing_orders (file_data *file);
void trial_standing_orders (file_data *file);


/* Report generation. */

void generate_full_sorder_report (file_data *file);

#endif

