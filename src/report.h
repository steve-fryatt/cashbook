/* CashBook - report.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_REPORT
#define _ACCOUNTS_REPORT

/* Report data block format consists of a series of lines as follows:
 *
 * <tab-bar-data><format flag>text[\t<format flag>text]\0
 *
 * The line pointer array consists of a series of offsets pointing to the tab-bar-data at the start of each line.
 *
 * The formatting flags indicate things like bold, italic, right align, etc for each individual field.  The
 * tab-bar-data is an integer showing which tab bar is to be used to space out the line.
 */

/* ==================================================================================================================
 * Static constants
 */

/* Report status flags. */

#define REPORT_STATUS_MEMERR 0x01 /* A memory allocation error has occurred, so stop allowing writes. */
#define REPORT_STATUS_CLOSED 0x02 /* The report has been closed to writing. */

/* Report formatting flags. */

#define REPORT_FLAG_BYTES 1 /* The length of the formatting flag data. */
#define REPORT_BAR_BYTES  1 /* The length of the tab bar data.         */

#define REPORT_FLAG_INDENT  0x01 /* The item is indented into the column. */
#define REPORT_FLAG_BOLD    0x02 /* The item is in bold. */
#define REPORT_FLAG_UNDER   0x04 /* The item is underlined. */
#define REPORT_FLAG_RIGHT   0x08 /* The item is right aligned. */
#define REPORT_FLAG_NUMERIC 0x10 /* The item is numeric. */
#define REPORT_FLAG_SPILL   0x20 /* The column is spill from adjacent columns on the left. */
#define REPORT_FLAG_HEADING 0x40 /* The row is a heading, to be repeated on page breaks. */
#define REPORT_FLAG_NOTNULL 0x80 /* Used to prevent the flag byte being a null terminator. */

/* Memory management */

#define REPORT_BLOCK_SIZE 10240
#define REPORT_LINE_SIZE 250

#define REPORT_MAX_LINE_LEN 1000

/* Layout details */

#define REPORT_BASELINE_OFFSET 4
#define REPORT_COLUMN_SPACE 20
#define REPORT_COLUMN_INDENT 40
#define REPORT_BOTTOM_MARGIN 4
#define REPORT_LEFT_MARGIN 4
#define REPORT_RIGHT_MARGIN 4
#define REPORT_MIN_WIDTH 1000
#define REPORT_MIN_HEIGHT 800

#define REPORT_TEXT_COLUMN_SPACE 1
#define REPORT_TEXT_COLUMN_INDENT 2


/* Aeport format dialogue. */

#define REPORT_FORMAT_OK 13
#define REPORT_FORMAT_CANCEL 12
#define REPORT_FORMAT_NFONT 1
#define REPORT_FORMAT_NFONTMENU 2
#define REPORT_FORMAT_BFONT 4
#define REPORT_FORMAT_BFONTMENU 5
#define REPORT_FORMAT_FONTSIZE 7
#define REPORT_FORMAT_FONTSPACE 10

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* Report creation and deletion. */

report_data *open_new_report (file_data *file, char *title, saved_report *template);
void close_report (file_data *file, report_data *report);
void close_and_print_report (file_data *file, report_data *report, int text, int textformat, int fitwidth, int rotate);
void delete_report (file_data *file, report_data *report);
void delete_report_window (file_data *file, report_data *report);

int write_report_line (report_data *report, int bar, char *text);

int find_report_fonts (report_data *report, font_f *normal, font_f *bold);
int format_report_columns (report_data *report);

int pending_print_reports (file_data *file);

/* Editing report format via the GUI. */

void open_report_format_window (file_data *file, report_data *report, wimp_pointer *ptr);
void refresh_report_format_window (void);
void fill_report_format_window (file_data *file, report_data *report);
int process_report_format_window (void);
void force_close_report_format_window (file_data *file);

/* Printing reports via the GUI. */

void open_report_print_window (file_data *file, report_data *report, wimp_pointer *ptr, int clear);
void report_print_window_closed (int text, int format, int scale, int rotate);

/* --- */

report_data *find_report_window_from_handle (file_data *file, wimp_w window);

/* Window handling. */

void report_window_click (file_data *file, wimp_pointer *pointer);

/* Window redraw. */

void redraw_report_window (wimp_draw *redraw, file_data *file);

/* Saving and export */

void save_report_text (file_data *file, report_data *report, char *filename, int formatting);
void export_delimited_report_file (file_data *file, report_data *report, char *filename, int format, int filetype);

/* Report printing */

void start_report_print (char *filename);
void cancel_report_print (void);

void print_report_graphic (file_data *file, report_data *report, int fit_width, int rotate);
void handle_print_error (os_fw file, os_error *error, font_f f1, font_f f2);

#endif
