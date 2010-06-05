/* CashBook - printing.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_PRINTING
#define _ACCOUNTS_PRINTING

/* ==================================================================================================================
 * Static constants
 */

/* Simple print dialogue */

#define SIMPLE_PRINT_OK 0
#define SIMPLE_PRINT_CANCEL 1
#define SIMPLE_PRINT_STANDARD 2
#define SIMPLE_PRINT_PORTRAIT 3
#define SIMPLE_PRINT_LANDSCAPE 4
#define SIMPLE_PRINT_SCALE 5
#define SIMPLE_PRINT_FASTTEXT 6
#define SIMPLE_PRINT_TEXTFORMAT 7

/* Date-range print dailogue */

#define DATE_PRINT_OK 0
#define DATE_PRINT_CANCEL 1
#define DATE_PRINT_STANDARD 2
#define DATE_PRINT_PORTRAIT 3
#define DATE_PRINT_LANDSCAPE 4
#define DATE_PRINT_SCALE 5
#define DATE_PRINT_FASTTEXT 6
#define DATE_PRINT_TEXTFORMAT 7
#define DATE_PRINT_FROM 9
#define DATE_PRINT_TO 11

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

/* Printer Protocol handling */

int send_start_print_save (void (*print_function) (char *), void (*cancel_function) (void), int text_print);
void handle_bounced_message_print_save (void);
void handle_message_print_error (wimp_message *message);
void handle_message_print_file (wimp_message *message);

/* Printer settings update */

void handle_message_set_printer (void);
void register_printer_update_handler (void (update_handler) (void));
void deregister_printer_update_handler (void (update_handler) (void));

/* Printing via the simple print GUI. */

void open_simple_print_window (file_data *file, wimp_pointer *ptr, int clear, char *title,
                               void (simple_print_start) (int, int, int, int));
void refresh_simple_print_window (void);
void fill_simple_print_window (print *print_data, int clear);
void process_simple_print_window (void);

void force_close_print_windows (file_data *file);

/* Printing via the date-range print GUI */

void open_date_print_window (file_data *file, wimp_pointer *ptr, int clear, char *title,
                             void (print_start) (int, int, int, int, date_t, date_t));
void refresh_date_print_window (void);
void fill_date_print_window (print *print_data, int clear);
void process_date_print_window (void);

#endif
