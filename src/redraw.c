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
    shade_rec = read_config_opt ("ShadeReconciled");
    shade_rec_col = read_config_int ("ShadeReconciledColour");

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

/* ==================================================================================================================
 * Account window redraw
 */

void redraw_account_window (wimp_draw *redraw, file_data *file)
{
  int                   ox, oy, top, base, y, i, entry, shade_overdrawn, shade_overdrawn_col, icon_fg_col;
  char                  icon_buffer1[AMOUNT_FIELD_LEN], icon_buffer2[AMOUNT_FIELD_LEN], icon_buffer3[AMOUNT_FIELD_LEN],
                        icon_buffer4[AMOUNT_FIELD_LEN];
  osbool                more;
  account_window        *window;

  extern global_windows windows;


  entry = find_accounts_window_entry_from_handle (file, redraw->w);

  /* Perform the redraw if a window was found. */

  if (entry != -1)
  {
    window = &(file->account_windows[entry]);

    shade_overdrawn = read_config_opt ("ShadeAccounts");
    shade_overdrawn_col = read_config_int ("ShadeAccountsColour");

    more = wimp_redraw_window (redraw);

    ox = redraw->box.x0 - redraw->xscroll;
    oy = redraw->box.y1 - redraw->yscroll;

    /* Set the horizontal positions of the icons for the account lines. */

    for (i=0; i < ACCOUNT_COLUMNS; i++)
    {
      windows.account_window_def->icons[i].extent.x0 = window->column_position[i];
      windows.account_window_def->icons[i].extent.x1 = window->column_position[i] + window->column_width[i];
    }

    /* Set the positions for the heading lines. */

    windows.account_window_def->icons[6].extent.x0 = window->column_position[0];
    windows.account_window_def->icons[6].extent.x1 = window->column_position[ACCOUNT_COLUMNS-1] +
                                                     window->column_width[ACCOUNT_COLUMNS-1];

    /* Set the positions for the footer lines. */

    windows.account_window_def->icons[7].extent.x0 = window->column_position[0];
    windows.account_window_def->icons[7].extent.x1 = window->column_position[1] +
                                                     window->column_width[1];

    windows.account_window_def->icons[8].extent.x0 = window->column_position[2];
    windows.account_window_def->icons[8].extent.x1 = window->column_position[2] +
                                                     window->column_width[2];

    windows.account_window_def->icons[9].extent.x0 = window->column_position[3];
    windows.account_window_def->icons[9].extent.x1 = window->column_position[3] +
                                                     window->column_width[3];

    windows.account_window_def->icons[10].extent.x0 = window->column_position[4];
    windows.account_window_def->icons[10].extent.x1 = window->column_position[4] +
                                                     window->column_width[4];

    windows.account_window_def->icons[11].extent.x0 = window->column_position[5];
    windows.account_window_def->icons[11].extent.x1 = window->column_position[ACCOUNT_COLUMNS-1] +
                                                     window->column_width[ACCOUNT_COLUMNS-1];

    /* The three numerical columns keep their icon buffers for the whole time, so set them up now. */

    windows.account_window_def->icons[2].data.indirected_text.text = icon_buffer1;
    windows.account_window_def->icons[3].data.indirected_text.text = icon_buffer2;
    windows.account_window_def->icons[4].data.indirected_text.text = icon_buffer3;
    windows.account_window_def->icons[5].data.indirected_text.text = icon_buffer4;

    windows.account_window_def->icons[8].data.indirected_text.text = icon_buffer1;
    windows.account_window_def->icons[9].data.indirected_text.text = icon_buffer2;
    windows.account_window_def->icons[10].data.indirected_text.text = icon_buffer3;
    windows.account_window_def->icons[11].data.indirected_text.text = icon_buffer4;

    /* Reset all the icon colours. */

    windows.account_window_def->icons[2].flags &= ~wimp_ICON_FG_COLOUR;
    windows.account_window_def->icons[2].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

    windows.account_window_def->icons[3].flags &= ~wimp_ICON_FG_COLOUR;
    windows.account_window_def->icons[3].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

    windows.account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
    windows.account_window_def->icons[4].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

    windows.account_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
    windows.account_window_def->icons[5].flags |= (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

    /* Perform the redraw. */

    while (more)
    {
      /* Calculate the rows to redraw. */

      top = (oy - redraw->clip.y1 - ACCOUNT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
      if (top < 0)
      {
        top = 0;
      }

      base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2)
             + oy - redraw->clip.y0 - ACCOUNT_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);


      /* Redraw the data into the window. */

      for (y = top; y <= base; y++)
      {
        /* Plot out the background with a filled white rectangle. */

        wimp_set_colour (wimp_COLOUR_WHITE);
        os_plot (os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCOUNT_TOOLBAR_HEIGHT);
        os_plot (os_PLOT_RECTANGLE + os_PLOT_TO,
                 ox + window->column_position[ACCOUNT_COLUMNS-1] + window->column_width[ACCOUNT_COLUMNS-1],
                 oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCOUNT_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

        if (y<window->display_lines && window->line_data[y].type == ACCOUNT_LINE_DATA)
        {
        /* Account field */

          windows.account_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[0].data.indirected_text.text =
             file->accounts[window->line_data[y].account].ident;
          windows.account_window_def->icons[1].data.indirected_text.text =
             file->accounts[window->line_data[y].account].name;

          wimp_plot_icon (&(windows.account_window_def->icons[0]));
          wimp_plot_icon (&(windows.account_window_def->icons[1]));

          /* Place the four numerical columns. */

          windows.account_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          /* Set the column data depending on the window type. */

          switch (window->type)
          {
            case ACCOUNT_FULL:
              convert_money_to_string (file->accounts[window->line_data[y].account].statement_balance, icon_buffer1);
              convert_money_to_string (file->accounts[window->line_data[y].account].current_balance, icon_buffer2);
              convert_money_to_string (file->accounts[window->line_data[y].account].trial_balance, icon_buffer3);
              convert_money_to_string (file->accounts[window->line_data[y].account].budget_balance, icon_buffer4);

              if (shade_overdrawn &&
                  (file->accounts[window->line_data[y].account].statement_balance <
                   -file->accounts[window->line_data[y].account].credit_limit))
              {
                icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
              }
              else
              {
                icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
              }
              windows.account_window_def->icons[2].flags &= ~wimp_ICON_FG_COLOUR;
              windows.account_window_def->icons[2].flags |= icon_fg_col;

              if (shade_overdrawn &&
                  (file->accounts[window->line_data[y].account].current_balance <
                   -file->accounts[window->line_data[y].account].credit_limit))
              {
                icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
              }
              else
              {
                icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
              }
              windows.account_window_def->icons[3].flags &= ~wimp_ICON_FG_COLOUR;
              windows.account_window_def->icons[3].flags |= icon_fg_col;

              if (shade_overdrawn &&
                  (file->accounts[window->line_data[y].account].trial_balance < 0))
              {
                icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
              }
              else
              {
                icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
              }
              windows.account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
              windows.account_window_def->icons[4].flags |= icon_fg_col;
              break;

            case ACCOUNT_IN:
              convert_money_to_string (-file->accounts[window->line_data[y].account].future_balance, icon_buffer1);
              convert_money_to_string (file->accounts[window->line_data[y].account].budget_amount, icon_buffer2);
              convert_money_to_string (-file->accounts[window->line_data[y].account].budget_balance, icon_buffer3);
              convert_money_to_string (file->accounts[window->line_data[y].account].budget_result, icon_buffer4);

              if (shade_overdrawn &&
                  (-file->accounts[window->line_data[y].account].budget_balance <
                   file->accounts[window->line_data[y].account].budget_amount))
              {
                icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
              }
              else
              {
                icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
              }
              windows.account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
              windows.account_window_def->icons[4].flags |= icon_fg_col;
              windows.account_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
              windows.account_window_def->icons[5].flags |= icon_fg_col;
              break;

            case ACCOUNT_OUT:
              convert_money_to_string (file->accounts[window->line_data[y].account].future_balance, icon_buffer1);
              convert_money_to_string (file->accounts[window->line_data[y].account].budget_amount, icon_buffer2);
              convert_money_to_string (file->accounts[window->line_data[y].account].budget_balance, icon_buffer3);
              convert_money_to_string (file->accounts[window->line_data[y].account].budget_result, icon_buffer4);

              if (shade_overdrawn &&
                  (file->accounts[window->line_data[y].account].budget_balance >
                   file->accounts[window->line_data[y].account].budget_amount))
              {
                icon_fg_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
              }
              else
              {
                icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
              }

              windows.account_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
              windows.account_window_def->icons[4].flags |= icon_fg_col;
              windows.account_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
              windows.account_window_def->icons[5].flags |= icon_fg_col;
              break;
          }

          /* Plot the three icons. */

          wimp_plot_icon (&(windows.account_window_def->icons[2]));
          wimp_plot_icon (&(windows.account_window_def->icons[3]));
          wimp_plot_icon (&(windows.account_window_def->icons[4]));
          wimp_plot_icon (&(windows.account_window_def->icons[5]));
        }
        else if (y<window->display_lines && window->line_data[y].type == ACCOUNT_LINE_HEADER)
        {
          /* Block header line */

          windows.account_window_def->icons[6].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[6].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[6].data.indirected_text.text = window->line_data[y].heading;

          wimp_plot_icon (&(windows.account_window_def->icons[6]));
        }
        else if (y<window->display_lines && window->line_data[y].type == ACCOUNT_LINE_FOOTER)
        {
          /* Block footer line */

          windows.account_window_def->icons[7].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[7].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[8].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[8].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[9].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[9].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[10].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[10].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[11].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[11].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[7].data.indirected_text.text = window->line_data[y].heading;
          convert_money_to_string (window->line_data[y].total[0], icon_buffer1);
          convert_money_to_string (window->line_data[y].total[1], icon_buffer2);
          convert_money_to_string (window->line_data[y].total[2], icon_buffer3);
          convert_money_to_string (window->line_data[y].total[3], icon_buffer4);

          wimp_plot_icon (&(windows.account_window_def->icons[7]));
          wimp_plot_icon (&(windows.account_window_def->icons[8]));
          wimp_plot_icon (&(windows.account_window_def->icons[9]));
          wimp_plot_icon (&(windows.account_window_def->icons[10]));
          wimp_plot_icon (&(windows.account_window_def->icons[11]));
        }
        else
        {
          /* Blank line */
          windows.account_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT - ICON_HEIGHT;
          windows.account_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                           - ACCOUNT_TOOLBAR_HEIGHT;

          windows.account_window_def->icons[0].data.indirected_text.text = icon_buffer1;
          windows.account_window_def->icons[1].data.indirected_text.text = icon_buffer1;
          *icon_buffer1 = '\0';
          *icon_buffer2 = '\0';
          *icon_buffer3 = '\0';
          *icon_buffer4 = '\0';

          wimp_plot_icon (&(windows.account_window_def->icons[0]));
          wimp_plot_icon (&(windows.account_window_def->icons[1]));
          wimp_plot_icon (&(windows.account_window_def->icons[2]));
          wimp_plot_icon (&(windows.account_window_def->icons[3]));
          wimp_plot_icon (&(windows.account_window_def->icons[4]));
          wimp_plot_icon (&(windows.account_window_def->icons[5]));
        }
      }

      more = wimp_get_rectangle (redraw);
    }
  }
}


/* ==================================================================================================================
 * Account view window redraw
 */

void redraw_accview_window (wimp_draw *redraw, file_data *file)
{
  int                   ox, oy, top, base, y, i, account, transaction, shade_budget, shade_budget_col,
                        shade_overdrawn, shade_overdrawn_col, icon_fg_col, icon_fg_balance_col;
  char                  icon_buffer[DESCRIPT_FIELD_LEN], rec_char[REC_FIELD_LEN]; /* Assumes descript is longest. */
  osbool                more;
  accview_window        *window;

  extern global_windows windows;


  account = find_accview_window_from_handle (file, redraw->w);

  /* Perform the redraw if a window was found. */

  if (account != NULL_ACCOUNT)
  {
    window = file->accounts[account].account_view;

    shade_budget = (file->accounts[account].type & (ACCOUNT_IN | ACCOUNT_OUT)) && read_config_opt ("ShadeBudgeted") &&
                   (file->budget.start != NULL_DATE || file->budget.finish != NULL_DATE);
    shade_budget_col = read_config_int ("ShadeBudgetedColour");

    shade_overdrawn = (file->accounts[account].type & ACCOUNT_FULL) && read_config_opt ("ShadeOverdrawn");
    shade_overdrawn_col = read_config_int ("ShadeOverdrawnColour");

    more = wimp_redraw_window (redraw);

    ox = redraw->box.x0 - redraw->xscroll;
    oy = redraw->box.y1 - redraw->yscroll;

    msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);

    /* Set the horizontal positions of the icons for the account lines. */

    for (i=0; i < ACCVIEW_COLUMNS; i++)
    {
      windows.accview_window_def->icons[i].extent.x0 = window->column_position[i];
      windows.accview_window_def->icons[i].extent.x1 = window->column_position[i] + window->column_width[i];
      windows.accview_window_def->icons[i].data.indirected_text.text = icon_buffer;
    }

    /* Perform the redraw. */

    while (more)
    {
      /* Calculate the rows to redraw. */

      top = (oy - redraw->clip.y1 - ACCVIEW_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
      if (top < 0)
      {
        top = 0;
      }

      base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2)
             + oy - redraw->clip.y0 - ACCVIEW_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);


      /* Redraw the data into the window. */

      for (y = top; y <= base; y++)
      {
        /* Plot out the background with a filled white rectangle. */

        wimp_set_colour (wimp_COLOUR_WHITE);
        os_plot (os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT);
        os_plot (os_PLOT_RECTANGLE + os_PLOT_TO,
                 ox + window->column_position[ACCVIEW_COLUMNS-1] + window->column_width[ACCVIEW_COLUMNS-1],
                 oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - ACCVIEW_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

        /* Find the transaction that applies to this line. */

        transaction = (y < window->display_lines) ?
                         (window->line_data)[(window->line_data)[y].sort_index].transaction : 0;

        /* work out the foreground colour for the line, based on whether the line is to be shaded or not. */

        if (shade_budget && (y < window->display_lines) &&
            ((file->budget.start == NULL_DATE || file->transactions[transaction].date < file->budget.start) ||
             (file->budget.finish == NULL_DATE || file->transactions[transaction].date > file->budget.finish)))
        {
          icon_fg_col = (shade_budget_col << wimp_ICON_FG_COLOUR_SHIFT);
          icon_fg_balance_col = (shade_budget_col << wimp_ICON_FG_COLOUR_SHIFT);
        }
        else if (shade_overdrawn && (y < window->display_lines) &&
              ((window->line_data)[(window->line_data)[y].sort_index].balance < - file->accounts[account].credit_limit))
        {
          icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
          icon_fg_balance_col = (shade_overdrawn_col << wimp_ICON_FG_COLOUR_SHIFT);
        }
        else
        {
          icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
          icon_fg_balance_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);
        }

        *icon_buffer = '\0';

        /* Date field */

        windows.accview_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[0].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[0].flags |= icon_fg_col;

        if (y < window->display_lines)
        {
          convert_date_to_string (file->transactions[transaction].date, icon_buffer);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.accview_window_def->icons[0]));

        /* From / To field */

        windows.accview_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[1].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[1].flags |= icon_fg_col;

        windows.accview_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[2].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[2].flags |= icon_fg_col;

        windows.accview_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[3].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[3].flags |= icon_fg_col;

        if (y < window->display_lines && file->transactions[transaction].from == account &&
           file->transactions[transaction].to != NULL_ACCOUNT)
        {
          windows.accview_window_def->icons[1].data.indirected_text.text =
             file->accounts[file->transactions[transaction].to].ident;
          windows.accview_window_def->icons[2].data.indirected_text.text = icon_buffer;
          windows.accview_window_def->icons[3].data.indirected_text.text =
             file->accounts[file->transactions[transaction].to].name;

          if (file->transactions[transaction].flags & TRANS_REC_FROM)
          {
            strcpy (icon_buffer, rec_char);
          }
          else
          {
            *icon_buffer = '\0';
          }
        }
        else if (y < window->display_lines && file->transactions[transaction].to == account &&
           file->transactions[transaction].from != NULL_ACCOUNT)
        {
          windows.accview_window_def->icons[1].data.indirected_text.text =
             file->accounts[file->transactions[transaction].from].ident;
          windows.accview_window_def->icons[2].data.indirected_text.text = icon_buffer;
          windows.accview_window_def->icons[3].data.indirected_text.text =
             file->accounts[file->transactions[transaction].from].name;

          if (file->transactions[transaction].flags & TRANS_REC_TO)
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
          windows.accview_window_def->icons[1].data.indirected_text.text = icon_buffer;
          windows.accview_window_def->icons[2].data.indirected_text.text = icon_buffer;
          windows.accview_window_def->icons[3].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }

        wimp_plot_icon (&(windows.accview_window_def->icons[1]));
        wimp_plot_icon (&(windows.accview_window_def->icons[2]));
        wimp_plot_icon (&(windows.accview_window_def->icons[3]));

        /* Reference field */

        windows.accview_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[4].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[4].flags |= icon_fg_col;

        if (y < window->display_lines)
        {
          windows.accview_window_def->icons[4].data.indirected_text.text = file->transactions[transaction].reference;
        }
        else
        {
          windows.accview_window_def->icons[4].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.accview_window_def->icons[4]));

        /* Payments field */

        windows.accview_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[5].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[5].flags |= icon_fg_col;

        if (y < window->display_lines && file->transactions[transaction].from == account)
        {
          convert_money_to_string (file->transactions[transaction].amount, icon_buffer);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.accview_window_def->icons[5]));

        /* Receipts field */

        windows.accview_window_def->icons[6].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[6].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[6].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[6].flags |= icon_fg_col;

        if (y < window->display_lines && file->transactions[transaction].to == account)
        {
          convert_money_to_string (file->transactions[transaction].amount, icon_buffer);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.accview_window_def->icons[6]));

        /* Balance field */

        windows.accview_window_def->icons[7].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[7].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[7].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[7].flags |= icon_fg_balance_col;

        if (y < window->display_lines)
        {
          convert_money_to_string ((window->line_data)[(window->line_data)[y].sort_index].balance, icon_buffer);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.accview_window_def->icons[7]));

       /* Comments field */

        windows.accview_window_def->icons[8].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.accview_window_def->icons[8].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                         - ACCVIEW_TOOLBAR_HEIGHT;

	windows.accview_window_def->icons[8].flags &= ~wimp_ICON_FG_COLOUR;
	windows.accview_window_def->icons[8].flags |= icon_fg_col;

        if (y < window->display_lines)
        {
          windows.accview_window_def->icons[8].data.indirected_text.text = file->transactions[transaction].description;
        }
        else
        {
          windows.accview_window_def->icons[8].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.accview_window_def->icons[8]));
      }
      more = wimp_get_rectangle (redraw);
    }
  }
}

/* ==================================================================================================================
 * Standing order window redraw
 */

void redraw_sorder_window (wimp_draw *redraw, file_data *file)
{
  int                   ox, oy, top, base, y, i, t;
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

    /* Set the horizontal positions of the icons. */

    for (i=0; i < SORDER_COLUMNS; i++)
    {
      windows.sorder_window_def->icons[i].extent.x0 = file->sorder_window.column_position[i];
      windows.sorder_window_def->icons[i].extent.x1 = file->sorder_window.column_position[i] +
                                                      file->sorder_window.column_width[i];
      windows.sorder_window_def->icons[i].data.indirected_text.text = icon_buffer;
    }

    /* Perform the redraw. */

    while (more)
    {
      /* Calculate the rows to redraw. */

      top = (oy - redraw->clip.y1 - SORDER_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
      if (top < 0)
      {
        top = 0;
      }

      base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2)
             + oy - redraw->clip.y0 - SORDER_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

      /* Redraw the data into the window. */

      for (y = top; y <= base; y++)
      {
        t = (y < file->sorder_count) ? file->sorders[y].sort_index : 0;

        /* Plot out the background with a filled white rectangle. */

        wimp_set_colour (wimp_COLOUR_WHITE);
        os_plot (os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - SORDER_TOOLBAR_HEIGHT);
        os_plot (os_PLOT_RECTANGLE + os_PLOT_TO,
                 ox + file->sorder_window.column_position[SORDER_COLUMNS-1]
                  + file->sorder_window.column_width[SORDER_COLUMNS-1],
                 oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - SORDER_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

        /* From field */

        windows.sorder_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT;

        windows.sorder_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT;

        windows.sorder_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT;


        if (y < file->sorder_count && file->sorders[t].from != NULL_ACCOUNT)
        {
          windows.sorder_window_def->icons[0].data.indirected_text.text =
             file->accounts[file->sorders[t].from].ident;
          windows.sorder_window_def->icons[1].data.indirected_text.text = icon_buffer;
          windows.sorder_window_def->icons[2].data.indirected_text.text =
             file->accounts[file->sorders[t].from].name;

          if (file->sorders[t].flags & TRANS_REC_FROM)
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
          windows.sorder_window_def->icons[0].data.indirected_text.text = icon_buffer;
          windows.sorder_window_def->icons[1].data.indirected_text.text = icon_buffer;
          windows.sorder_window_def->icons[2].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }

        wimp_plot_icon (&(windows.sorder_window_def->icons[0]));
        wimp_plot_icon (&(windows.sorder_window_def->icons[1]));
        wimp_plot_icon (&(windows.sorder_window_def->icons[2]));

        /* To field */

        windows.sorder_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT;

        windows.sorder_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT;

        windows.sorder_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - SORDER_TOOLBAR_HEIGHT;

        if (y < file->sorder_count && file->sorders[t].to != NULL_ACCOUNT)
        {
          windows.sorder_window_def->icons[3].data.indirected_text.text =
             file->accounts[file->sorders[t].to].ident;
          windows.sorder_window_def->icons[4].data.indirected_text.text = icon_buffer;
          windows.sorder_window_def->icons[5].data.indirected_text.text =
             file->accounts[file->sorders[t].to].name;

          if (file->sorders[t].flags & TRANS_REC_TO)
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
          windows.sorder_window_def->icons[3].data.indirected_text.text = icon_buffer;
          windows.sorder_window_def->icons[4].data.indirected_text.text = icon_buffer;
          windows.sorder_window_def->icons[5].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }

        wimp_plot_icon (&(windows.sorder_window_def->icons[3]));
        wimp_plot_icon (&(windows.sorder_window_def->icons[4]));
        wimp_plot_icon (&(windows.sorder_window_def->icons[5]));

        /* Amount field */

        windows.sorder_window_def->icons[6].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[6].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT;
        if (y < file->sorder_count)
        {
          convert_money_to_string (file->sorders[t].normal_amount, icon_buffer);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.sorder_window_def->icons[6]));

        /* Comments field */

        windows.sorder_window_def->icons[7].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[7].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT;
        if (y < file->sorder_count)
        {
          windows.sorder_window_def->icons[7].data.indirected_text.text = file->sorders[t].description;
        }
        else
        {
          windows.sorder_window_def->icons[7].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.sorder_window_def->icons[7]));

        /* Next date field */

        windows.sorder_window_def->icons[8].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[8].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT;
        if (y < file->sorder_count)
        {
          if (file->sorders[t].adjusted_next_date != NULL_DATE)
          {
            convert_date_to_string (file->sorders[t].adjusted_next_date, icon_buffer);
          }
          else
          {
            msgs_lookup ("SOrderStopped", icon_buffer, sizeof (icon_buffer));
          }
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.sorder_window_def->icons[8]));

        /* Left field */

        windows.sorder_window_def->icons[9].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.sorder_window_def->icons[9].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - SORDER_TOOLBAR_HEIGHT;
        if (y < file->sorder_count)
        {
          sprintf (icon_buffer, "%d", file->sorders[t].left);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.sorder_window_def->icons[9]));
      }

      more = wimp_get_rectangle (redraw);
    }
  }
}


/* ==================================================================================================================
 * Preset window redraw
 */

void redraw_preset_window (wimp_draw *redraw, file_data *file)
{
  int                   ox, oy, top, base, y, i, t;
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

    /* Set the horizontal positions of the icons. */

    for (i=0; i < PRESET_COLUMNS; i++)
    {
      windows.preset_window_def->icons[i].extent.x0 = file->preset_window.column_position[i];
      windows.preset_window_def->icons[i].extent.x1 = file->preset_window.column_position[i] +
                                                      file->preset_window.column_width[i];
      windows.preset_window_def->icons[i].data.indirected_text.text = icon_buffer;
    }

    /* Perform the redraw. */

    while (more)
    {
      /* Calculate the rows to redraw. */

      top = (oy - redraw->clip.y1 - PRESET_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
      if (top < 0)
      {
        top = 0;
      }

      base = ((ICON_HEIGHT+LINE_GUTTER) + ((ICON_HEIGHT+LINE_GUTTER) / 2)
             + oy - redraw->clip.y0 - PRESET_TOOLBAR_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);

      /* Redraw the data into the window. */

      for (y = top; y <= base; y++)
      {
        t = (y < file->preset_count) ? file->presets[y].sort_index : 0;

        /* Plot out the background with a filled white rectangle. */

        wimp_set_colour (wimp_COLOUR_WHITE);
        os_plot (os_MOVE_TO, ox, oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - PRESET_TOOLBAR_HEIGHT);
        os_plot (os_PLOT_RECTANGLE + os_PLOT_TO,
                 ox + file->preset_window.column_position[PRESET_COLUMNS-1]
                  + file->preset_window.column_width[PRESET_COLUMNS-1],
                 oy - (y * (ICON_HEIGHT+LINE_GUTTER)) - PRESET_TOOLBAR_HEIGHT - (ICON_HEIGHT+LINE_GUTTER));

        /* Key field */

        windows.preset_window_def->icons[0].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[0].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT;
        if (y < file->preset_count)
        {
          sprintf(icon_buffer, "%c", file->presets[t].action_key);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.preset_window_def->icons[0]));

        /* Name field */

        windows.preset_window_def->icons[1].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[1].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT;
        if (y < file->preset_count)
        {
          windows.preset_window_def->icons[1].data.indirected_text.text = file->presets[t].name;
        }
        else
        {
          windows.preset_window_def->icons[1].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.preset_window_def->icons[1]));

        /* From field */

        windows.preset_window_def->icons[2].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[2].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT;

        windows.preset_window_def->icons[3].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[3].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT;

        windows.preset_window_def->icons[4].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[4].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT;


        if (y < file->preset_count && file->presets[t].from != NULL_ACCOUNT)
        {
          windows.preset_window_def->icons[2].data.indirected_text.text =
             file->accounts[file->presets[t].from].ident;
          windows.preset_window_def->icons[3].data.indirected_text.text = icon_buffer;
          windows.preset_window_def->icons[4].data.indirected_text.text =
             file->accounts[file->presets[t].from].name;

          if (file->presets[t].flags & TRANS_REC_FROM)
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
          windows.preset_window_def->icons[2].data.indirected_text.text = icon_buffer;
          windows.preset_window_def->icons[3].data.indirected_text.text = icon_buffer;
          windows.preset_window_def->icons[4].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }

        wimp_plot_icon (&(windows.preset_window_def->icons[2]));
        wimp_plot_icon (&(windows.preset_window_def->icons[3]));
        wimp_plot_icon (&(windows.preset_window_def->icons[4]));

        /* To field */

        windows.preset_window_def->icons[5].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[5].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT;

        windows.preset_window_def->icons[6].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[6].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT;

        windows.preset_window_def->icons[7].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[7].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                             - PRESET_TOOLBAR_HEIGHT;

        if (y < file->preset_count && file->presets[t].to != NULL_ACCOUNT)
        {
          windows.preset_window_def->icons[5].data.indirected_text.text =
             file->accounts[file->presets[t].to].ident;
          windows.preset_window_def->icons[6].data.indirected_text.text = icon_buffer;
          windows.preset_window_def->icons[7].data.indirected_text.text =
             file->accounts[file->presets[t].to].name;

          if (file->presets[t].flags & TRANS_REC_TO)
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
          windows.preset_window_def->icons[5].data.indirected_text.text = icon_buffer;
          windows.preset_window_def->icons[6].data.indirected_text.text = icon_buffer;
          windows.preset_window_def->icons[7].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }

        wimp_plot_icon (&(windows.preset_window_def->icons[5]));
        wimp_plot_icon (&(windows.preset_window_def->icons[6]));
        wimp_plot_icon (&(windows.preset_window_def->icons[7]));

        /* Amount field */

        windows.preset_window_def->icons[8].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[8].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT;
        if (y < file->preset_count)
        {
          convert_money_to_string (file->presets[t].amount, icon_buffer);
        }
        else
        {
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.preset_window_def->icons[8]));

        /* Comments field */

        windows.preset_window_def->icons[9].extent.y0 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT - ICON_HEIGHT;
        windows.preset_window_def->icons[9].extent.y1 = (-y * (ICON_HEIGHT+LINE_GUTTER))
                                                        - PRESET_TOOLBAR_HEIGHT;
        if (y < file->preset_count)
        {
          windows.preset_window_def->icons[9].data.indirected_text.text = file->presets[t].description;
        }
        else
        {
          windows.preset_window_def->icons[9].data.indirected_text.text = icon_buffer;
          *icon_buffer = '\0';
        }
        wimp_plot_icon (&(windows.preset_window_def->icons[9]));
      }

      more = wimp_get_rectangle (redraw);
    }
  }
}

