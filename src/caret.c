/* CashBook - caret.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <stdarg.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/icons.h"
#include "sflib/debug.h"

/* Application header files */

#include "global.h"
#include "caret.h"

#include "file.h"

/* ==================================================================================================================
 * Global variables.
 */

wimp_w    old_window = NULL;
wimp_i    old_icon   = 0;
int       old_index  = 0;

/* ==================================================================================================================
 *
 */

void place_dialogue_caret (wimp_w window, wimp_i icon)
{
  wimp_caret caret;


  wimp_get_caret_position (&caret);

  if (find_transaction_window_file_block (caret.w) != NULL)
  {
    old_window = caret.w;
    old_icon   = caret.i;
    old_index  = caret.index;
  }

  icons_put_caret_at_end (window, icon);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Try to place the caret into a sequence of writable icons, using the first not to be shaded.  If all are shaded,
 * place the caret into the work area.
 */


void place_dialogue_caret_fallback (wimp_w window, int icons, ...)
{
  int        i;
  wimp_i     icon;
  va_list    ap;
  wimp_caret caret;


  va_start (ap, icons);

  wimp_get_caret_position (&caret);

  if (find_transaction_window_file_block (caret.w) != NULL)
  {
    old_window = caret.w;
    old_icon   = caret.i;
    old_index  = caret.index;
  }

  i = 0;

  while (i < icons && i != -1)
  {
    i++;

    icon = va_arg (ap, wimp_i);

    if (!icons_get_shaded (window, icon))
    {
      icons_put_caret_at_end (window, icon);
      i = -1;
    }
  }

  if (i != -1)
  {
    icons_put_caret_at_end (window, wimp_ICON_WINDOW);
  }

  va_end (ap);
}

/* ================================================================================================================== */

void close_dialogue_with_caret (wimp_w window)
{
  wimp_caret caret;


  if (old_window != NULL)
  {
    wimp_get_caret_position (&caret);

    #ifdef DEBUG
    debug_printf ("Close dialogue %d (caret location %d)", window, caret.w);
    #endif

    if (caret.w == window && find_transaction_window_file_block (old_window) != NULL)
    {
      wimp_set_caret_position (old_window, old_icon, 0, 0, -1, old_index);

      old_window = NULL;
      old_icon   = 0;
      old_index  = 0;
    }
  }

  wimp_close_window (window);
}
