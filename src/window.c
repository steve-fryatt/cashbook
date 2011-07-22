/* CashBook - window.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osbyte.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/general.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "window.h"

//#include "edit.h"
//#include "file.h"

/* Set up the window extent and visible area.
 *
 * On entry, width and height are the work area extent.  x and y give the X and Y posotions of the top left of the
 * window, or -1 to default place the window on that axis.  yoff is a Y offset to apply, to allow for raked openings.
 */

void set_initial_window_area (wimp_window *window, int width, int height, int x, int y, int yoff)
{
  int limit, clear_iconbar, lower_limit;

  /* Set the extent of the window. */

  window->extent.x0 = 0;
  window->extent.x1 = width;
  window->extent.y0 = -height;
  window->extent.y1 = 0;

  /* Read CMOS RAM to see if the icon-bar is to be kept clear. */

  clear_iconbar = ((osbyte2 (osbyte_READ_CMOS, osbyte_CONFIGURE_NO_OBSCURE_ICON_BAR, 0) &
                  osbyte_CONFIGURE_NO_OBSCURE_ICON_BAR_MASK) != 0);

  /* Set up the X position. */

  limit = general_mode_width ();
  width = (width < (limit * X_WINDOW_PERCENT_LIMIT / 100)) ? width : (limit * X_WINDOW_PERCENT_LIMIT / 100);

  if (x > -1)
  {
    window->visible.x0 = x;
    window->visible.x1 = x + width;

    if (window->visible.x1 >= limit)
    {
      window->visible.x1 = limit - 1;
      window->visible.x0 = window->visible.x1 - width;
    }
  }
  else
  {
    window->visible.x0 = (limit - width) / 2;
    window->visible.x1 = window->visible.x0 + width;
  }

  /* Set up the Y position. */

  limit = general_mode_height ();                                  /* The Y screen size. */
  lower_limit = (clear_iconbar) ? sf_ICONBAR_HEIGHT : 0;   /* The lower usable bound if clear-ibar is set. */

  /* Calculate the maximum visible height possible.  This is as a percentage of the total screen height.  If the
   * height comes out as too high for practical use, it is reduced to the height - icon-bar clearance, with top and
   * bottom window furniture removed too.
   */

  height = (height < (limit * Y_WINDOW_PERCENT_LIMIT / 100)) ? height : (limit * Y_WINDOW_PERCENT_LIMIT / 100);
  if (height + (2 * sf_WINDOW_GADGET_HEIGHT) > limit - lower_limit)
  {
    height = limit - lower_limit - (2 * sf_WINDOW_GADGET_HEIGHT);
  }

  if (y > -1)
  {
    window->visible.y1 = y - yoff - sf_WINDOW_GADGET_HEIGHT;
    window->visible.y0 = y - yoff - sf_WINDOW_GADGET_HEIGHT - height;

    if (window->visible.y0 < lower_limit)
    {
      window->visible.y0 = lower_limit;
      window->visible.y1 = lower_limit + height;
    }
  }
  else
  {
    window->visible.y1 = (limit * Y_WINDOW_PERCENT_ORIGIN / 100) - yoff;
    window->visible.y0 = window->visible.y1 - height;

    if (window->visible.y0 < lower_limit + sf_WINDOW_GADGET_HEIGHT)
    {
      window->visible.y0 = lower_limit + sf_WINDOW_GADGET_HEIGHT ;
      window->visible.y1 = lower_limit + sf_WINDOW_GADGET_HEIGHT + height;
    }
  }
}

