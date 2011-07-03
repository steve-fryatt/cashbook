/* CashBook - goto.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_GOTO
#define CASHBOOK_GOTO

#define GOTO_TYPE_LINE 0
#define GOTO_TYPE_DATE 1

/**
 * Initialise the Goto module.
 */

void goto_initialise(void);


/**
 * Open the Goto dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void goto_open_window(file_data *file, wimp_pointer *ptr, osbool restore);


/**
 * Force the closure of the Goto window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void goto_force_window_closed(file_data *file);

#endif

