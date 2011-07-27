/* CashBook - redraw.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"

/* SF-Lib header files. */

#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/debug.h"
#include "sflib/msgs.h"
#include "sflib/config.h"

/* Application header files */

#include "global.h"
#include "redraw.h"

#include "account.h"
#include "accview.h"
#include "conversion.h"
#include "date.h"
#include "file.h"
#include "transact.h"

/* ------------------------------------------------------------------------------------------------------------------
 * Global variables
 */

/* ==================================================================================================================
 * Transaction window redraw
 */

void redraw_transaction_window (wimp_draw *redraw, file_data *file)
{
  int                   ox, oy, top, base, y, i, t, shade_rec, shade_rec_col, icon_fg_col;
  char                  icon_buffer[DESCRIPT_FIELD_LEN], rec_char[REC_FIELD_LEN]; /* Assumes descript is longest. */
  osbool                more;

  extern global_windows windows;


  /* Perform the redraw if a window was found. */

  if (file != NULL)
  {
    more = wimp_redraw_window (redraw);

    ox = redraw->box.x0 - redraw->xscroll;
    oy = redraw->box.y1 - redraw->yscroll;

    msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);
    shade_rec = config_opt_read ("ShadeReconciled");
    shade_rec_col = config_int_read ("ShadeReconciledColour");

    /* Set the horizontal positions of the icons. */

    for (i=0; i < TRANSACT_COLUMNS; i++)
    {
      windows.transaction_window_def->icons[i].extent.x0 = file->transaction_window.column_position[i];
      windows.transaction_window_def->icons[i].extent.x1 = file->transaction_window.column_position[i] +
                                                           file->transaction_window.column_width[i];
      windows.transaction_window_def->icons[i].data.indirected_text.text = icon_buffer;
    }

    /* Perform the redraw. */

    while (more)
    {
      /* Calculate the rows to redraw. */

      top = (oy - redraw->clip.y1 - TRANSACT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
      if (top < 0)
      {
        top = 0;
      }

      base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2)
             + oy - redraw->clip.y0 - TRANSACT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

      /* Redraw the data into the window. */

      for (y = top; y <= base; y++)
      {
        /* Get the transaction number for the line. */

        t = (y < file->trans_count) ? file->transactions[y].sort_index : 0;

        /* work out the foreground colour for the line, based on whether the line is to be shaded or not. */

        if (shade_rec && (y < file->trans_count) &&
            ((file->transactions[t].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
        {
          icon_fg_col = (shade_rec_col << wimp_ICON_FG_COLOUR_SHIFT);
        }
        else
        {
          icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
        }

        /* Plot out the background with a filled grey rectangle. */

        wimp_set_colour (wimp_COLOUR_VERY_LIGHT_GREY);
        os_plot (os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - TRANSACT_TOOLBAR_HEIGHT);
        os_plot (os_PLOT_RECTANGLE + os_PLOT_TO,
                 ox + file->transaction_window.column_position[TRANSACT_COLUMNS-1]
                  + file->transaction_window.column_width[TRANSACT_COLUMNS-1],
                 oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - TRANSACT_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

        if (y != file->transaction_window.entry_line)
        {
          /* Date field */

          windows.transaction_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[0].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[0].flags |= icon_fg_col;

          if (y < file->trans_count)
          {
            convert_date_to_string (file->transactions[t].date, icon_buffer);
          }
          else
          {
            *icon_buffer = '\0';
          }
          wimp_plot_icon (&(windows.transaction_window_def->icons[0]));

          /* From field */

          windows.transaction_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[1].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[1].flags |= icon_fg_col;

          windows.transaction_window_def->icons[2].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[2].flags |= icon_fg_col;

          windows.transaction_window_def->icons[3].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[3].flags |= icon_fg_col;

          if (y < file->trans_count && file->transactions[t].from != NULL_ACCOUNT)
          {
            windows.transaction_window_def->icons[1].data.indirected_text.text =
               file->accounts[file->transactions[t].from].ident;
            windows.transaction_window_def->icons[2].data.indirected_text.text = icon_buffer;
            windows.transaction_window_def->icons[3].data.indirected_text.text =
               file->accounts[file->transactions[t].from].name;

            if (file->transactions[t].flags & TRANS_REC_FROM)
            {
              strcpy (icon_buffer, rec_char);
            }
            else
            {
              *icon_buffer = '\0';
            }
          }
          else
          {
            windows.transaction_window_def->icons[1].data.indirected_text.text = icon_buffer;
            windows.transaction_window_def->icons[2].data.indirected_text.text = icon_buffer;
            windows.transaction_window_def->icons[3].data.indirected_text.text = icon_buffer;
            *icon_buffer = '\0';
          }

          wimp_plot_icon (&(windows.transaction_window_def->icons[1]));
          wimp_plot_icon (&(windows.transaction_window_def->icons[2]));
          wimp_plot_icon (&(windows.transaction_window_def->icons[3]));

          /* To field */

          windows.transaction_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[6].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[6].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[4].flags |= icon_fg_col;

          windows.transaction_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[5].flags |= icon_fg_col;

          windows.transaction_window_def->icons[6].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[6].flags |= icon_fg_col;

          if (y < file->trans_count && file->transactions[t].to != NULL_ACCOUNT)
          {
            windows.transaction_window_def->icons[4].data.indirected_text.text =
               file->accounts[file->transactions[t].to].ident;
            windows.transaction_window_def->icons[5].data.indirected_text.text = icon_buffer;
            windows.transaction_window_def->icons[6].data.indirected_text.text =
               file->accounts[file->transactions[t].to].name;

            if (file->transactions[t].flags & TRANS_REC_TO)
            {
              strcpy (icon_buffer, rec_char);
            }
            else
            {
              *icon_buffer = '\0';
            }
          }
          else
          {
            windows.transaction_window_def->icons[4].data.indirected_text.text = icon_buffer;
            windows.transaction_window_def->icons[5].data.indirected_text.text = icon_buffer;
            windows.transaction_window_def->icons[6].data.indirected_text.text = icon_buffer;
            *icon_buffer = '\0';
          }

          wimp_plot_icon (&(windows.transaction_window_def->icons[4]));
          wimp_plot_icon (&(windows.transaction_window_def->icons[5]));
          wimp_plot_icon (&(windows.transaction_window_def->icons[6]));

          /* Reference field */

          windows.transaction_window_def->icons[7].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[7].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[7].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[7].flags |= icon_fg_col;

          if (y < file->trans_count)
          {
            windows.transaction_window_def->icons[7].data.indirected_text.text = file->transactions[t].reference;
          }
          else
          {
            windows.transaction_window_def->icons[7].data.indirected_text.text = icon_buffer;
            *icon_buffer = '\0';
          }
          wimp_plot_icon (&(windows.transaction_window_def->icons[7]));

          /* Amount field */

          windows.transaction_window_def->icons[8].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[8].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[8].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[8].flags |= icon_fg_col;

          if (y < file->trans_count)
          {
            convert_money_to_string (file->transactions[t].amount, icon_buffer);
          }
          else
          {
            *icon_buffer = '\0';
          }
          wimp_plot_icon (&(windows.transaction_window_def->icons[8]));

          /* Comments field */

          windows.transaction_window_def->icons[9].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.transaction_window_def->icons[9].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                               - TRANSACT_TOOLBAR_HEIGHT;

          windows.transaction_window_def->icons[9].flags &= ~wimp_ICON_FG_COLOUR;
          windows.transaction_window_def->icons[9].flags |= icon_fg_col;

          if (y < file->trans_count)
          {
            windows.transaction_window_def->icons[9].data.indirected_text.text = file->transactions[t].description;
          }
          else
          {
            windows.transaction_window_def->icons[9].data.indirected_text.text = icon_buffer;
            *icon_buffer = '\0';
          }
          wimp_plot_icon (&(windows.transaction_window_def->icons[9]));
        }
      }

      more = wimp_get_rectangle (redraw);
    }
  }
}

