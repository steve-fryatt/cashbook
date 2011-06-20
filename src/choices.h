/* CashBook - choices.h
 *
 * (C) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_CHOICES
#define CASHBOOK_CHOICES

#include "oslib/wimp.h"

/**
 * Initialise the Choices module.
 */

void choices_initialise(void);


/**
 * Open the Choices window at the mouse pointer.
 *
 * \param *pointer		The details of the pointer to open the window at.
 */

void choices_open_window(wimp_pointer *pointer);

#endif

