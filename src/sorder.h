/* CashBook - sorder.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_SORDER
#define CASHBOOK_SORDER

/* ==================================================================================================================
 * Static constants
 */

/* Standing order edit icons. */

#define SORDER_EDIT_OK 0
#define SORDER_EDIT_CANCEL 1
#define SORDER_EDIT_STOP 34
#define SORDER_EDIT_DELETE 35

#define SORDER_EDIT_START 3
#define SORDER_EDIT_NUMBER 5
#define SORDER_EDIT_PERIOD 7
#define SORDER_EDIT_PERDAYS 8
#define SORDER_EDIT_PERMONTHS 9
#define SORDER_EDIT_PERYEARS 10
#define SORDER_EDIT_AVOID 11
#define SORDER_EDIT_SKIPFWD 12
#define SORDER_EDIT_SKIPBACK 13
#define SORDER_EDIT_FMIDENT 17
#define SORDER_EDIT_FMREC 32
#define SORDER_EDIT_FMNAME 18
#define SORDER_EDIT_TOIDENT 20
#define SORDER_EDIT_TOREC 33
#define SORDER_EDIT_TONAME 21
#define SORDER_EDIT_REF 23
#define SORDER_EDIT_AMOUNT 25
#define SORDER_EDIT_FIRSTSW 26
#define SORDER_EDIT_FIRST 27
#define SORDER_EDIT_LASTSW 28
#define SORDER_EDIT_LAST 29
#define SORDER_EDIT_DESC 31


/* Toolbar icons. */

#define SORDER_PANE_FROM 0
#define SORDER_PANE_TO 1
#define SORDER_PANE_AMOUNT 2
#define SORDER_PANE_DESCRIPTION 3
#define SORDER_PANE_NEXTDATE 4
#define SORDER_PANE_LEFT 5

#define SORDER_PANE_PARENT 6
#define SORDER_PANE_ADDSORDER 7
#define SORDER_PANE_PRINT 8
#define SORDER_PANE_SORT 9

#define SORDER_PANE_COL_MAP "0,1,2;3,4,5;6;7;8;9"
#define SORDER_PANE_SORT_DIR_ICON 10

/* Standing Order Sort Window */

#define SORDER_SORT_OK 2
#define SORDER_SORT_CANCEL 3
#define SORDER_SORT_FROM 4
#define SORDER_SORT_TO 5
#define SORDER_SORT_AMOUNT 6
#define SORDER_SORT_DESCRIPTION 7
#define SORDER_SORT_NEXTDATE 8
#define SORDER_SORT_LEFT 9
#define SORDER_SORT_ASCENDING 10
#define SORDER_SORT_DESCENDING 11


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





void adjust_sorder_window_columns (file_data *file, int data, wimp_i icon, int width);
void adjust_sorder_window_sort_icon (file_data *file);
void update_sorder_window_sort_icon (file_data *file, wimp_icon *icon);

/* Sorting Standing Orders */

void sort_sorder_window (file_data *file);
void open_sorder_sort_window (file_data *file, wimp_pointer *ptr);
void refresh_sorder_sort_window (void);
void fill_sorder_sort_window (int sort_option);
int process_sorder_sort_window (void);
void force_close_sorder_sort_window (file_data *file);

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

void force_close_sorder_edit_window (file_data *file);

/* Printing standing orders via the GUI. */

void open_sorder_print_window (file_data *file, wimp_pointer *ptr, int clear);

/* Standing order processing. */

void process_standing_orders (file_data *file);
void trial_standing_orders (file_data *file);

/* File and print output. */

void print_sorder_window(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);

/* Report generation. */

void generate_full_sorder_report (file_data *file);

/* Standing order window handling */

void set_sorder_window_extent (file_data *file);
void build_sorder_window_title (file_data *file);
void force_sorder_window_redraw (file_data *file, int from, int to);

void decode_sorder_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);

#endif

