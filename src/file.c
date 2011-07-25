/* CashBook - file.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "file.h"

#include "account.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "clipboard.h"
#include "calculation.h"
#include "column.h"
#include "conversion.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "filing.h"
#include "find.h"
#include "goto.h"
#include "mainmenu.h"
#include "presets.h"
#include "printing.h"
#include "purge.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"

/* ==================================================================================================================
 * Global variables.
 */

/* The head of the linked list of file data structures. */

static file_data *file_list = NULL;

/* A count which is incremented to allow <Untitled n> window titles */

static int       untitled_count = 0;

/* The last date on which all the files were updated. */

static unsigned last_update_date = NULL_DATE;

/* ==================================================================================================================
 * File initialisation and deletion.
 */

/* Allocate memory for a file, initialise it and create the transaction window. */

file_data *build_new_file_block (void)
{
  file_data             *new;
  int                   i;


  /* Claim the memory required for the file descriptor block. */

  new = (file_data *) heap_alloc (sizeof (file_data));
  if (new == NULL)
  {
    error_msgs_report_error ("NoMemNewFile");
    return (new);
  }

  /* Initialise the transaction window. */

  new->transaction_window.transaction_window = NULL;
  new->transaction_window.transaction_pane = NULL;

  column_init_window (new->transaction_window.column_width, new->transaction_window.column_position,
                       TRANSACT_COLUMNS, config_str_read ("TransactCols"));

  new->transaction_window.sort_order = SORT_DATE | SORT_ASCENDING;

  /* Initialise the account and heading windows. */

  for (i=0; i<ACCOUNT_WINDOWS; i++)
  {
    new->account_windows[i].account_window = NULL;
    new->account_windows[i].account_pane = NULL;
    new->account_windows[i].account_footer = NULL;

    column_init_window (new->account_windows[i].column_width, new->account_windows[i].column_position,
                         ACCOUNT_COLUMNS, config_str_read ("AccountCols"));

    /* Blank out the footer icons. */

    *new->account_windows[i].footer_icon[0] = '\0';
    *new->account_windows[i].footer_icon[1] = '\0';
    *new->account_windows[i].footer_icon[2] = '\0';
    *new->account_windows[i].footer_icon[3] = '\0';

    /* Set the individual windows to the type of account they will hold. */

    switch (i)
    {
      case 0:
        new->account_windows[i].type = ACCOUNT_FULL;
        break;

      case 1:
        new->account_windows[i].type = ACCOUNT_IN;
        break;

      case 2:
        new->account_windows[i].type = ACCOUNT_OUT;
        break;
    }

    /* Set the initial lines up */

    new->account_windows[i].display_lines = 0;
    flex_alloc ((flex_ptr) &(new->account_windows[i].line_data), 4); /* Set up to an initial dummy amount. */
  }

  /* Initialise the account view window. */

  column_init_window (new->accview_column_width, new->accview_column_position,
                       ACCVIEW_COLUMNS, config_str_read ("AccViewCols"));

  new->accview_sort_order = SORT_DATE | SORT_ASCENDING;

  /* Initialise the standing order window. */

  new->sorder_window.sorder_window = NULL;
  new->sorder_window.sorder_pane = NULL;

  column_init_window (new->sorder_window.column_width, new->sorder_window.column_position,
                       SORDER_COLUMNS, config_str_read ("SOrderCols"));

  new->sorder_window.sort_order = SORT_NEXTDATE | SORT_DESCENDING;

  /* Initialise the preset window. */

  new->preset_window.preset_window = NULL;
  new->preset_window.preset_pane = NULL;

  column_init_window (new->preset_window.column_width, new->preset_window.column_position,
                       PRESET_COLUMNS, config_str_read ("PresetCols"));

  new->preset_window.sort_order = SORT_CHAR | SORT_ASCENDING;

  /* Set the filename and save status. */

  *(new->filename) = '\0';
  new->modified = 0;
  new->sort_valid = 1;
  new->untitled_count = ++untitled_count;
  new->child_x_offset = 0;
  new->auto_reconcile = 0;

  /* Set up the default initial values. */

  new->trans_count = 0;
  new->account_count = 0;
  new->sorder_count = 0;
  new->preset_count = 0;
  new->saved_report_count = 0;

  new->last_full_recalc = NULL_DATE;

  /* Set up the report data. */

  new->reports = NULL;

  new->import_report = NULL;

  /* Set up the budget details. */

  new->budget.start = NULL_DATE;
  new->budget.finish = NULL_DATE;

  new->budget.sorder_trial = 0;
  new->budget.limit_postdate = 0;

  /* Set up the dialogue defaults. */

  new->go_to.data = NULL_DATE;
  new->go_to.data_type = GOTO_TYPE_DATE;

  new->find.date = NULL_DATE;
  new->find.from = NULL_ACCOUNT;
  new->find.from_rec = 0;
  new->find.to = NULL_ACCOUNT;
  new->find.to_rec = 0;
  new->find.amount = NULL_CURRENCY;
  *(new->find.ref) = '\0';
  *(new->find.desc) = '\0';

  new->find.logic = FIND_OR;
  new->find.case_sensitive = 0;
  new->find.whole_text = 0;
  new->find.direction = FIND_START;

  new->print.fit_width = config_opt_read ("PrintFitWidth");
  new->print.page_numbers = config_opt_read ("PrintPageNumbers");
  new->print.rotate = config_opt_read ("PrintRotate");
  new->print.text = config_opt_read ("PrintText");
  new->print.text_format = config_opt_read ("PrintTextFormat");
  new->print.from = NULL_DATE;
  new->print.to = NULL_DATE;

  new->continuation.transactions = 1;
  new->continuation.accounts = 0;
  new->continuation.headings = 0;
  new->continuation.sorders = 0;
  new->continuation.before = NULL_DATE;

  new->trans_rep.date_from = NULL_DATE;
  new->trans_rep.date_to = NULL_DATE;
  new->trans_rep.budget = 0;
  new->trans_rep.group = 0;
  new->trans_rep.period = 1;
  new->trans_rep.period_unit = PERIOD_MONTHS;
  new->trans_rep.lock = 0;
  new->trans_rep.from_count = 0;
  new->trans_rep.to_count = 0;
  *(new->trans_rep.ref) = '\0';
  *(new->trans_rep.desc) = '\0';
  new->trans_rep.amount_min = NULL_CURRENCY;
  new->trans_rep.amount_max = NULL_CURRENCY;
  new->trans_rep.output_trans = 1;
  new->trans_rep.output_summary = 1;
  new->trans_rep.output_accsummary = 1;

  new->unrec_rep.date_from = NULL_DATE;
  new->unrec_rep.date_to = NULL_DATE;
  new->unrec_rep.budget = 0;
  new->unrec_rep.group = 0;
  new->unrec_rep.period = 1;
  new->unrec_rep.period_unit = PERIOD_MONTHS;
  new->unrec_rep.lock = 0;
  new->unrec_rep.from_count = 0;
  new->unrec_rep.to_count = 0;

  new->cashflow_rep.date_from = NULL_DATE;
  new->cashflow_rep.date_to = NULL_DATE;
  new->cashflow_rep.budget = 0;
  new->cashflow_rep.group = 0;
  new->cashflow_rep.period = 1;
  new->cashflow_rep.period_unit = PERIOD_MONTHS;
  new->cashflow_rep.lock = 0;
  new->cashflow_rep.empty = 0;
  new->cashflow_rep.accounts_count = 0;
  new->cashflow_rep.incoming_count = 0;
  new->cashflow_rep.outgoing_count = 0;
  new->cashflow_rep.tabular = 0;

  new->balance_rep.date_from = NULL_DATE;
  new->balance_rep.date_to = NULL_DATE;
  new->balance_rep.budget = 0;
  new->balance_rep.group = 0;
  new->balance_rep.period = 1;
  new->balance_rep.period_unit = PERIOD_MONTHS;
  new->balance_rep.lock = 0;
  new->balance_rep.accounts_count = 0;
  new->balance_rep.incoming_count = 0;
  new->balance_rep.outgoing_count = 0;
  new->balance_rep.tabular = 0;

  /* Set up the memory blocks. */

  flex_alloc ((flex_ptr) &(new->transactions), 4); /* Set up to an initial dummy amount. */
  flex_alloc ((flex_ptr) &(new->accounts), 4);     /* Set up to an initial dummy amount. */
  flex_alloc ((flex_ptr) &(new->sorders), 4);      /* Set up to an initial dummy amount. */
  flex_alloc ((flex_ptr) &(new->presets), 4);      /* Set up to an initial dummy amount. */
  flex_alloc ((flex_ptr) &(new->saved_reports), 4);      /* Set up to an initial dummy amount. */

  /* Link the file descriptor into the list. */

  new->next = file_list;
  file_list = new;

  return (new);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Create a new transaction file with window and open it. */

void create_new_file (void)
{
  file_data *file;


  /* Build a new file block. */

  file = build_new_file_block ();

  if (file != NULL)
  {
    create_transaction_window (file);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Delete a transaction file block with its window. */

void delete_file (file_data *file)
{
  file_data                **list;
  int                      i, button;
  wimp_pointer             pointer;

  extern global_windows windows;


  /* First check that the file is saved and, if not, prompt for deletion. */

  if (file->modified == 1 && (button = error_msgs_report_question ("FileNotSaved", "FileNotSavedB")) >= 2)
  {
    if (button == 3)
    {
      wimp_get_pointer_info (&pointer);
      initialise_save_boxes (file, 0, 1);
      fill_save_as_window (file, SAVE_BOX_FILE);
      menus_create_standard_menu ((wimp_menu *) windows.save_as, &pointer);
    }

    return;
  }

  /* If there are any reports in the file with pending print jobs, prompt for deletion. */

  if (report_get_pending_print_jobs (file) && error_msgs_report_question ("PendingPrints", "PendingPrintsB") == 2)
  {
    return;
  }

  /* Remove the edit line reference. */

  transaction_edit_line_block_deleted (file);

  /* Delete the windows. */

  delete_transaction_window (file);
  sorder_delete_window(file);
  preset_delete_window(file);

  for (i=0; i<ACCOUNT_WINDOWS; i++) /* Step through the account list windows. */
  {
    flex_free ((flex_ptr) &(file->account_windows[i].line_data));

    if (file->account_windows[i].account_window != NULL)
    {
      wimp_delete_window (file->account_windows[i].account_window);
    }

    if (file->account_windows[i].account_pane != NULL)
    {
      wimp_delete_window (file->account_windows[i].account_pane);
    }

    if (file->account_windows[i].account_footer != NULL)
    {
      wimp_delete_window (file->account_windows[i].account_footer);
    }
  }

  for (i=0; i<file->account_count; i++) /* Step through the accounts and their account view windows. */
  {
    if (file->accounts[i].account_view != NULL)
    {
      #ifdef DEBUG
      debug_printf ("Account %d has a view to delete.", i);
      #endif

      accview_delete_window(file, i);
    }
  }

  /* Delete any reports that are open. */

  while (file->reports != NULL)
  {
    report_delete(file->reports);
  }

  /* Do the same for any file-related dialogues that are open. */

  force_close_account_edit_window (file);
  force_close_section_edit_window (file);
  goto_force_window_closed(file);
  find_force_windows_closed(file);
  budget_force_window_closed(file);
  report_force_format_windows_closed(file);
  printing_force_windows_closed(file);
  analysis_force_windows_closed(file);
  purge_force_window_closed(file);
  force_close_transaction_sort_window (file);
  force_close_accview_sort_window (file);
  sorder_force_windows_closed(file);
  preset_force_windows_closed(file);
  force_close_import_window (file);

  /* Delink the block and delete it. */

  list = &file_list;

  while (*list != NULL && *list != file)
  {
    list = &((*list)->next);
  }

  if (*list != NULL)
  {
    *list = file->next;
  }

  flex_free ((flex_ptr) &(file->transactions));
  flex_free ((flex_ptr) &(file->accounts));
  flex_free ((flex_ptr) &(file->sorders));
  flex_free ((flex_ptr) &(file->presets));
  flex_free ((flex_ptr) &(file->saved_reports));

  heap_free (file);
}

/* ==================================================================================================================
 * Finding files.
 */

/* Find the file block that corresponds to a given transaction window handle. */

file_data *find_transaction_window_file_block (wimp_w window)
{
  file_data *list;


  list = file_list;

  while (list != NULL && list->transaction_window.transaction_window != window)
  {
    list = list->next;
  }

  /* If the window is not a transction window, this loop falls out with (list == NULL) ready to return the
   * NULL (not found) value.
   */

  return (list);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given transaction pane handle. */

file_data *find_transaction_pane_file_block (wimp_w window)
{
  file_data *list;


  list = file_list;

  while (list != NULL && list->transaction_window.transaction_pane != window)
  {
    list = list->next;
  }

  /* If the window is not a transction window, this loop falls out with (list == NULL) ready to return the
   * NULL (not found) value.
   */

  return (list);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given account window handle. */

file_data *find_account_window_file_block (wimp_w window)
{
  file_data *list, *found;
  int       i;


  list = file_list;
  found = NULL;

  while (list != NULL && found == NULL)
  {
    for (i=0; i<ACCOUNT_WINDOWS; i++)
    {
      if (list->account_windows[i].account_window == window)
      {
        found = list;
      }
    }

    list = list->next;
  }

  return (found);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given account pane handle. */

file_data *find_account_pane_file_block (wimp_w window)
{
  file_data *list, *found;
  int       i;


  list = file_list;
  found = NULL;

  while (list != NULL && found == NULL)
  {
    for (i=0; i<ACCOUNT_WINDOWS; i++)
    {
      if (list->account_windows[i].account_pane == window)
      {
        found = list;
      }
    }

    list = list->next;
  }

  return (found);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given standing order window handle. */

file_data *find_sorder_window_file_block (wimp_w window)
{
  file_data *list;


  list = file_list;

  while (list != NULL && list->sorder_window.sorder_window != window)
  {
    list = list->next;
  }

  /* If the window is not a transction window, this loop falls out with (list == NULL) ready to return the
   * NULL (not found) value.
   */

  return (list);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given standing pane handle. */

file_data *find_sorder_pane_file_block (wimp_w window)
{
  file_data *list;


  list = file_list;

  while (list != NULL && list->sorder_window.sorder_pane != window)
  {
    list = list->next;
  }

  /* If the window is not a transction window, this loop falls out with (list == NULL) ready to return the
   * NULL (not found) value.
   */

  return (list);
}


/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given preset pane handle. */

file_data *find_preset_pane_file_block (wimp_w window)
{
  file_data *list;


  list = file_list;

  while (list != NULL && list->preset_window.preset_pane != window)
  {
    list = list->next;
  }

  /* If the window is not a preset pane, this loop falls out with (list == NULL) ready to return the
   * NULL (not found) value.
   */

  return (list);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given account view window handle. */

file_data *find_accview_window_file_block (wimp_w window)
{
  file_data *list, *found;
  int       i;


  list = file_list;
  found = NULL;

  while (list != NULL && found == NULL)
  {
    for (i=0; i<list->account_count; i++)
    {
      if (list->accounts[i].account_view != NULL && (list->accounts[i].account_view)->accview_window == window)
      {
        found = list;
      }
    }

    list = list->next;
  }

  return (found);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the file block that corresponds to a given account view pane handle. */

file_data *find_accview_pane_file_block (wimp_w window)
{
  file_data *list, *found;
  int       i;


  list = file_list;
  found = NULL;

  while (list != NULL && found == NULL)
  {
    for (i=0; i<list->account_count; i++)
    {
      if (list->accounts[i].account_view != NULL && (list->accounts[i].account_view)->accview_pane == window)
      {
        found = list;
      }
    }

    list = list->next;
  }

  return (found);
}


/* ==================================================================================================================
 * Saved files and data integrity
 */

/* Check for unsaved files and for any pending print jobs. */

int check_for_unsaved_files (void)
{
  file_data *list = file_list;
  int       modified, pending;


  /* Search through all the loaded files to see if any are modified or any have pending print jobs attached. */

  modified = 0;
  pending = 0;

  while (list != NULL)
  {
    if (list->modified)
    {
      modified = 1;
    }

    if (report_get_pending_print_jobs (list))
    {
      pending = 1;
    }

    list = list->next;
  }

  /* If any were modified, allow the user to discard them. */

  if (modified)
  {
    if (error_msgs_report_question ("FilesNotSaved", "FilesNotSavedB") == 1)
    {
      modified = 0;
    }
  }

  /* If there were no unsaved files (or the use chose to discard them), warn of any pending print jobs.  This isn't
   * done if the process is aborted due to modified files to save 'dialogue box overload'. :-)
   */

  if (!modified && pending)
  {
    if (error_msgs_report_question ("FPendingPrints", "FPendingPrintsB") == 1)
    {
      pending = 0;
    }
  }

  /* Return true if anything needs rescuing. */

  return (modified || pending);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void set_file_data_integrity (file_data *file, int unsafe)
{
  if (file->modified != unsafe)
  {
    file->modified = unsafe;
    build_transaction_window_title (file);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return true if the file has a full save path (ie. it has been saved before, or loaded from disc). */

int check_for_filepath (file_data *file)
{
  return (*(file->filename) != '\0');
}

/* ==================================================================================================================
 * Default filenames
 */

/* Return a path-name string for the current file, using the <Untitled n> format if the file hasn't been saved. */

char *make_file_pathname (file_data *file, char *path, int len)
{

  if (*(file->filename) != '\0')
  {
    strcpy (path, file->filename);
  }
  else
  {
    generate_default_title (file, path, len);
  }

  return (path);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return a leaf-name string for the current file, using the <Untitled n> format if the file hasn't been saved. */

char *make_file_leafname (file_data *file, char *leaf, int len)
{

  if (*(file->filename) != '\0')
  {
    strcpy (leaf, string_find_leafname(file->filename));
  }
  else
  {
    generate_default_title (file, leaf, len);
  }

  return (leaf);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Build a title of the form <Untitled n> for the specified file block. */

char *generate_default_title (file_data *file, char *name, int len)
{
  char number[256];


  *name = '\0';

  sprintf (number, "%d", file->untitled_count);
  msgs_param_lookup ("DefTitle", name, len, number, NULL, NULL, NULL);

  return (name);
}

/* ==================================================================================================================
 * General file redraw.
 */

void redraw_all_files (void)
{
  file_data *list = file_list;

  /* Work through all the loaded files and force redraws of each one. */

  while (list != NULL)
  {
    redraw_file_windows (list);

    list = list->next;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Redraw all the windows connected with a given file. */

void redraw_file_windows (file_data *file)
{
  int i;
  report_data *rep_list;


  /* Redraw the transaction window. */

  force_transaction_window_redraw (file, 0, file->trans_count);
  refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, -1, -1);

  /* Redraw the account list windows. */

  for (i=0; i<ACCOUNT_WINDOWS; i++)
  {
    force_accounts_window_redraw (file, i, 0, file->account_windows[i].display_lines);
  }

  /* Redraw the account view windows. */

  redraw_all_account_views (file);

  /* Redraw the standing order window. */

  sorder_force_window_redraw (file, 0, file->sorder_count);

  preset_force_window_redraw(file, 0, file->preset_count);

  /* Redraw the report windows. */

  rep_list = file->reports;

  while (rep_list != NULL)
  {
    if (rep_list->window != NULL)
    {
        windows_redraw (rep_list->window);
    }
    rep_list = rep_list->next;
  }
}

/* ==================================================================================================================
 * A change of date.
 */

/* Add standing orders and recalculate all the files on a change of day. */

void update_files_for_new_date (void)
{
  unsigned  today;
  file_data *file;


  #ifdef DEBUG
  debug_printf ("\\YChecking for daily update");
  #endif

  /* Get today's date and check if the last update happened today. */

  today = get_current_date ();

  if (today != last_update_date)
  {
    hourglass_on ();

    file = file_list;

    while (file != NULL)
    {
      #ifdef DEBUG
      debug_printf ("Updateing file...");
      #endif

      sorder_process(file);
      perform_full_recalculation (file);
      set_transaction_window_extent (file);

      file = file->next;
    }

    /* Remember today so this doesn't happen again until tomorrow. */

    last_update_date = today;

    hourglass_off ();
  }
}


