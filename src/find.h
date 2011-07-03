/* CashBook - find.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_FIND
#define CASHBOOK_FIND

/**
 * Initialise the Find module.
 */

void find_initialise(void);


/**
 * Open the Find dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void find_open_window(file_data *file, wimp_pointer *ptr, osbool restore);


/**
 * Force the closure of the Find and Results windows if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void find_force_windows_closed(file_data *file);

#endif

