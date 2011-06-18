/* CashBook - goto.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/errors.h"
#include "sflib/debug.h"

/* Application header files */

#include "global.h"
#include "goto.h"

#include "caret.h"
#include "date.h"
#include "edit.h"
#include "transact.h"

/* ==================================================================================================================
 * Global variables.
 */

static file_data *goto_window_file = NULL;
static int       goto_window_clear = 0;

/* ==================================================================================================================
 * Open the goto window.
 */

void open_goto_window (file_data *file, wimp_pointer *ptr, int clear)
{
  extern global_windows windows;


  /* If the window is already open, close it to start with. */

  if (windows_get_open (windows.go_to))
  {
    wimp_close_window (windows.go_to);
  }

  /* Blank out the icon contents. */

  fill_goto_window (&(file->go_to), clear);

  /* Set the pointer up to find the window again and open the window. */

  goto_window_file = file;
  goto_window_clear = clear;

  windows_open_centred_at_pointer (windows.go_to, ptr);
  place_dialogue_caret (windows.go_to, GOTO_ICON_NUMBER_FIELD);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Refresh the goto window icons. */

void refresh_goto_window (void)
{
  extern global_windows windows;

  fill_goto_window (&(goto_window_file->go_to), goto_window_clear);

  icons_redraw_group (windows.go_to, 1, GOTO_ICON_NUMBER_FIELD);
  icons_replace_caret_in_window (windows.go_to);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_goto_window (go_to *go_to_data, int clear)
{
  extern global_windows windows;


  if (clear == 0)
  {
    *icons_get_indirected_text_addr (windows.go_to, GOTO_ICON_NUMBER_FIELD) = '\0';

    icons_set_selected (windows.go_to, GOTO_ICON_NUMBER, 0);
    icons_set_selected (windows.go_to, GOTO_ICON_DATE, 1);
  }
  else
  {
    if (go_to_data->data_type == GOTO_TYPE_LINE)
    {
      sprintf (icons_get_indirected_text_addr (windows.go_to, GOTO_ICON_NUMBER_FIELD), "%d", go_to_data->data);
    }
    else if (go_to_data->data_type == GOTO_TYPE_DATE)
    {
      convert_date_to_string ((date_t) go_to_data->data, icons_get_indirected_text_addr (windows.go_to, GOTO_ICON_NUMBER_FIELD));
    }

    icons_set_selected (windows.go_to, GOTO_ICON_NUMBER, go_to_data->data_type == GOTO_TYPE_LINE);
    icons_set_selected (windows.go_to, GOTO_ICON_DATE, go_to_data->data_type == GOTO_TYPE_DATE);
  }
}

/* ==================================================================================================================
 * Perform a goto operation.
 */

int process_goto_window (void)
{
  int    min, max, mid, line = 0;
  date_t target;

  extern global_windows windows;

  goto_window_file->go_to.data_type = (icons_get_selected (windows.go_to, GOTO_ICON_DATE)) ?
                                       GOTO_TYPE_DATE : GOTO_TYPE_LINE;

  if (goto_window_file->go_to.data_type == GOTO_TYPE_LINE)
  {
    /* Go to a plain transaction line number. */

    goto_window_file->go_to.data = atoi (icons_get_indirected_text_addr (windows.go_to, GOTO_ICON_NUMBER_FIELD));

    if (goto_window_file->go_to.data <= 0 || goto_window_file->go_to.data > goto_window_file->trans_count ||
        strlen (icons_get_indirected_text_addr (windows.go_to, GOTO_ICON_NUMBER_FIELD)) == 0)
    {
      error_msgs_report_info ("BadGotoLine");

      return (1);
    }

    line = goto_window_file->go_to.data - 1;
  }

  else if (goto_window_file->go_to.data_type == GOTO_TYPE_DATE)
  {
    /* Go to a given date, or the nearest transaction. */

    if (goto_window_file->sort_valid == 0)
    {
      sort_transactions (goto_window_file);
    }

    target = convert_string_to_date (icons_get_indirected_text_addr (windows.go_to, GOTO_ICON_NUMBER_FIELD), NULL_DATE, 0);
    goto_window_file->go_to.data = (unsigned) target;

    /* Search through the array using a binary search: assumes that transactions are stored in date order. */

    min = 0;
    max = goto_window_file->trans_count - 1;

    while (min < max)
    {
      mid = (min + max)/2;

      if (target <= goto_window_file->transactions[mid].date)
      {
        max = mid;
      }
      else
      {
        min = mid + 1;
      }
    }

    line = locate_transaction_in_transact_window (goto_window_file, min);
  }

  place_transaction_edit_line (goto_window_file, line);
  icons_put_caret_at_end (goto_window_file->transaction_window.transaction_window, 0);
  find_transaction_edit_line (goto_window_file);

  return (0);
}

/* ==================================================================================================================
 * Force the window to close.
 */

/* Force the closure of the goto window if the file disappears. */

void force_close_goto_window (file_data *file)
{
  extern global_windows windows;


  if (goto_window_file == file && windows_get_open (windows.go_to))
  {
    close_dialogue_with_caret (windows.go_to);
  }
}
