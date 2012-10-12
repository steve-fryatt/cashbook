/* Copyright 2003-2012, Stephen Fryatt
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
 * \file: dataxfer.h
 *
 * Save dialogues and data transfer implementation.
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

int drag_end_save (char *filename);
int immediate_window_save (void);

/* Prepare file loading */

int initialise_data_load (wimp_message *message);

#endif

