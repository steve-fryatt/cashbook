/* CashBook - purge.h
 *
 * (c) Stephen Fryatt, 2003-1011
 */

#ifndef CASHBOOK_PURGE
#define CASHBOOK_PURGE


/**
 * Initialise the Purge module.
 */

void purge_initialise(void);


/**
 * Open the Purge dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void purge_open_window(file_data *file, wimp_pointer *ptr, osbool restore);


/**
 * Force the closure of the Purge window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void purge_force_window_closed(file_data *file);

#endif

