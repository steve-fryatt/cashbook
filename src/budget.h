/* CashBook - budget.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_BUDGET
#define CASHBOOK_BUDGET


/**
 * Initialise the Budget module.
 */

void budget_initialise(void);


/**
 * Open the Budget dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 */

void budget_open_window(file_data *file, wimp_pointer *ptr);


/**
 * Force the closure of the Budget window if it is open and relates
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void budget_force_window_closed(file_data *file);

#endif

