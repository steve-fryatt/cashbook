/* CashBook - dataxfer.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_DATAXFER
#define CASHBOOK_DATAXFER

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define SAVE_BOXES 14

#define SAVE_BOX_NONE (-1) /* This has to be -1, as the others are used as array indices. */
#define SAVE_BOX_FILE 0
#define SAVE_BOX_CSV  1
#define SAVE_BOX_TSV  2
#define SAVE_BOX_ACCCSV 3
#define SAVE_BOX_ACCTSV 4
#define SAVE_BOX_ACCVIEWCSV 5
#define SAVE_BOX_ACCVIEWTSV 6
#define SAVE_BOX_SORDERCSV 7
#define SAVE_BOX_SORDERTSV 8
#define SAVE_BOX_REPTEXT 9
#define SAVE_BOX_REPCSV 10
#define SAVE_BOX_REPTSV 11
#define SAVE_BOX_PRESETCSV 12
#define SAVE_BOX_PRESETTSV 13

#define CASHBOOK_FILE_TYPE 0x1ca
#define CSV_FILE_TYPE 0xdfe
#define TSV_FILE_TYPE 0xfff
#define TEXT_FILE_TYPE 0xfff
#define FANCYTEXT_FILE_TYPE 0xaf8



/**
 * Initialise the data transfer system.
 *
 * \param *sprites		The application sprite area.
 */

void dataxfer_initialise(void);


/* Initialise and prepare the save boxes. */

void initialise_save_boxes (file_data *file, int object, int delete_after);
void fill_save_as_window (file_data *file, int new_window);
void start_direct_menu_save (file_data *file);



/**
 * Open the Save As dialogue at the pointer.
 *
 * \param *pointer		The pointer location to open the dialogue.
 */

void dataxfer_open_saveas_window(wimp_pointer *pointer);


/* Save box drag handling. */

void start_save_window_drag (void);
void terminate_user_drag (wimp_dragged *drag);

int drag_end_save (char *filename);
int immediate_window_save (void);

/* Prepare file loading */

int initialise_data_load (wimp_message *message);
int drag_end_load (char *filename);

int start_data_open_load (wimp_message *message);

#endif
