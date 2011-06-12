/* CashBook - budget.c
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

/* SF-Lib header files. */

#include "sflib/windows.h"
#include "sflib/icons.h"

/* Application header files */

#include "global.h"
#include "budget.h"

#include "caret.h"
#include "calculation.h"
#include "date.h"
#include "file.h"
#include "sorder.h"

/* ==================================================================================================================
 * Global variables.
 */

file_data *edit_budget_file = NULL;

/* ==================================================================================================================
 * Open the budget window.
 */

void open_budget_window (file_data *file, wimp_pointer *ptr)
{
  extern global_windows windows;


  /* If the window is already open, another account is being edited or created.  Assume the user wants to lose
   * any unsaved data and just close the window.
   */

  if (window_is_open (windows.budget))
  {
    wimp_close_window (windows.budget);
  }

  /* Set the window contents up. */

  fill_budget_window (file);

  /* Set the pointers up so we can find this lot again and open the window. */

  edit_budget_file = file;

  open_window_centred_at_pointer (windows.budget, ptr);
  place_dialogue_caret (windows.budget, BUDGET_ICON_START);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_budget_window (void)
{
  extern global_windows windows;

  fill_budget_window (edit_budget_file);
  icons_redraw_group (windows.budget, 3, BUDGET_ICON_START, BUDGET_ICON_FINISH, BUDGET_ICON_TRIAL);
  icons_replace_caret_in_window (windows.budget);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_budget_window (file_data *file)
{
  extern global_windows windows;


  convert_date_to_string (file->budget.start, icons_get_indirected_text_addr (windows.budget, BUDGET_ICON_START));
  convert_date_to_string (file->budget.finish, icons_get_indirected_text_addr (windows.budget, BUDGET_ICON_FINISH));

  sprintf (icons_get_indirected_text_addr (windows.budget, BUDGET_ICON_TRIAL), "%d", file->budget.sorder_trial);

  icons_set_selected (windows.budget, BUDGET_ICON_RESTRICT, file->budget.limit_postdate);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated edit heading window and process the data. */

int process_budget_window (void)
{
  extern global_windows windows;


  edit_budget_file->budget.start =
       convert_string_to_date (icons_get_indirected_text_addr (windows.budget, BUDGET_ICON_START), NULL_DATE, 0);
  edit_budget_file->budget.finish =
       convert_string_to_date (icons_get_indirected_text_addr (windows.budget, BUDGET_ICON_FINISH), NULL_DATE, 0);

  edit_budget_file->budget.sorder_trial = atoi (icons_get_indirected_text_addr (windows.budget, BUDGET_ICON_TRIAL));

  edit_budget_file->budget.limit_postdate = icons_get_selected (windows.budget, BUDGET_ICON_RESTRICT);

  /* Tidy up and redraw the windows */

  trial_standing_orders (edit_budget_file);
  perform_full_recalculation (edit_budget_file);
  set_file_data_integrity (edit_budget_file, 1);
  redraw_file_windows (edit_budget_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the budget window if the file disappears. */

void force_close_budget_window (file_data *file)
{
  extern global_windows windows;


  if (edit_budget_file == file && window_is_open (windows.budget))
  {
    close_dialogue_with_caret (windows.budget);
  }
}
