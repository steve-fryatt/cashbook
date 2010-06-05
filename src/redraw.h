/* CashBook - redraw.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_REDRAW
#define _ACCOUNTS_REDRAW

#include "oslib/wimp.h"

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

/* ------------------------------------------------------------------------------------------------------------------
 * File data structures.
 */


/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Transaction window redraw */

void redraw_transaction_window (wimp_draw *redraw, file_data *file);

/* Account window redraw */

void redraw_account_window (wimp_draw *redraw, file_data *file);

/* Account view window redraw */

void redraw_accview_window (wimp_draw *redraw, file_data *file);

/* Standing order window redraw */

void redraw_sorder_window (wimp_draw *redraw, file_data *file);

/* Preset window redraw */

void redraw_preset_window (wimp_draw *redraw, file_data *file);

#endif
