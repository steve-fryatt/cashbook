/* CashBook - main.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_MAIN
#define CASHBOOK_MAIN

#include "oslib/wimp.h"

/**
 * Application-wide global variables.
 */

extern wimp_t		main_task_handle;
extern osbool		main_quit_flag;


/**
 * Main code entry point.
 */

int main(int argc, char *argv[]);

#endif

