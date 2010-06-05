/* CashBook - continue.c
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
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/errors.h"
#include "sflib/debug.h"

/* Application header files */

#include "global.h"
#include "continue.h"

#include "account.h"
#include "accview.h"
#include "calculation.h"
#include "caret.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "sorder.h"
#include "transact.h"

/* ==================================================================================================================
 * Global variables.
 */

static file_data *continue_window_file = NULL;
static int       continue_window_clear = 0;

/* ==================================================================================================================
 * Open the continue window.
 */

void open_continue_window (file_data *file, wimp_pointer *ptr, int clear)
{
  extern global_windows windows;


  /* If the window is already open, close it to start with. */

  if (window_is_open (windows.continuation))
  {
    wimp_close_window (windows.continuation);
  }

  /* Blank out the icon contents. */

  fill_continue_window (&(file->continuation), clear);

  /* Set the pointer up to find the window again and open the window. */

  continue_window_file = file;
  continue_window_clear = clear;

  open_window_centred_at_pointer (windows.continuation, ptr);
  place_dialogue_caret_fallback (windows.continuation, 1, CONTINUE_ICON_DATE);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Refresh the continue window icons. */

void refresh_continue_window (void)
{
  extern global_windows windows;

  fill_continue_window (&(continue_window_file->continuation), continue_window_clear);

  redraw_icons_in_window (windows.continuation, 1, CONTINUE_ICON_DATE);
  replace_caret_in_window (windows.continuation);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_continue_window (continuation *cont_data, int clear)
{
  extern global_windows windows;

  if (clear == 0)
  {
    set_icon_selected (windows.continuation, CONTINUE_ICON_TRANSACT, 1);
    set_icon_selected (windows.continuation, CONTINUE_ICON_ACCOUNTS, 0);
    set_icon_selected (windows.continuation, CONTINUE_ICON_HEADINGS, 0);
    set_icon_selected (windows.continuation, CONTINUE_ICON_SORDERS, 0);

    *indirected_icon_text (windows.continuation, CONTINUE_ICON_DATE) = '\0';
  }
  else
  {
    set_icon_selected (windows.continuation, CONTINUE_ICON_TRANSACT, cont_data->transactions);
    set_icon_selected (windows.continuation, CONTINUE_ICON_ACCOUNTS, cont_data->accounts);
    set_icon_selected (windows.continuation, CONTINUE_ICON_HEADINGS, cont_data->headings);
    set_icon_selected (windows.continuation, CONTINUE_ICON_SORDERS, cont_data->sorders);

    convert_date_to_string (cont_data->before, indirected_icon_text (windows.continuation, CONTINUE_ICON_DATE));
  }

  set_icons_shaded_when_radio_off (windows.continuation, CONTINUE_ICON_TRANSACT, 2,
                                   CONTINUE_ICON_DATE, CONTINUE_ICON_DATETEXT);
}

/* ==================================================================================================================
 * Perform a continue operation.
 */

int process_continue_window (void)
{
  extern global_windows windows;

  continue_window_file->continuation.transactions = read_icon_selected (windows.continuation, CONTINUE_ICON_TRANSACT);
  continue_window_file->continuation.accounts = read_icon_selected (windows.continuation, CONTINUE_ICON_ACCOUNTS);
  continue_window_file->continuation.headings = read_icon_selected (windows.continuation, CONTINUE_ICON_HEADINGS);
  continue_window_file->continuation.sorders = read_icon_selected (windows.continuation, CONTINUE_ICON_SORDERS);

  continue_window_file->continuation.before = convert_string_to_date (
                                                  indirected_icon_text (windows.continuation, CONTINUE_ICON_DATE),
                                                  NULL_DATE, 0);

  if (continue_window_file->modified == 1 &&
      wimp_msgtrans_question_report ("ContFileNotSaved", "ContFileNotSavedB") == 2)
  {
    return (1);
  }

  purge_file (continue_window_file, continue_window_file->continuation.transactions,
                                    continue_window_file->continuation.before,
                                    continue_window_file->continuation.accounts,
                                    continue_window_file->continuation.headings,
                                    continue_window_file->continuation.sorders);

  return (0);
}

/* ==================================================================================================================
 * Force the window to close.
 */

/* Force the closure of the continue window if the file disappears. */

void force_close_continue_window (file_data *file)
{
  extern global_windows windows;


  if (continue_window_file == file && window_is_open (windows.continuation))
  {
    close_dialogue_with_caret (windows.continuation);
  }
}

/* ==================================================================================================================
 * Purge a file of unwanted trasactions, accounts and standing orders.
 */

void purge_file (file_data *file, int transactions, date_t date, int accounts, int headings, int sorders)
{
  int i;


  hourglass_on ();

  /* Redraw the file now, so that the full extent of the original transactions is dealt with. */

  redraw_file_windows (file);

  #ifdef DEBUG
  debug_printf ("\\OPurging file");
  #endif

  /* Purge unused transactions from the file. */

  if (transactions)
  {
    for (i=0; i<file->trans_count; i++)
    {
      if ((file->transactions[i].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO) &&
          (date == NULL_DATE || file->transactions[i].date < date))
      {
        /* If the from and to accounts are full accounts, */

        if (file->transactions[i].from != NULL_ACCOUNT &&
            (file->accounts[file->transactions[i].from].type & ACCOUNT_FULL) != 0)
        {
          file->accounts[file->transactions[i].from].opening_balance -= file->transactions[i].amount;
        }

        if (file->transactions[i].to != NULL_ACCOUNT &&
            (file->accounts[file->transactions[i].to].type & ACCOUNT_FULL) != 0)
        {
          file->accounts[file->transactions[i].to].opening_balance += file->transactions[i].amount;
        }

        file->transactions[i].date = NULL_DATE;
        file->transactions[i].from = NULL_ACCOUNT;
        file->transactions[i].to = NULL_ACCOUNT;
        file->transactions[i].flags = 0;
        file->transactions[i].amount = NULL_CURRENCY;
        *file->transactions[i].reference = '\0';
        *file->transactions[i].description = '\0';

        file->sort_valid = 0;
      }
    }

    if (file->sort_valid == 0)
    {
      sort_transactions (file);
    }
    strip_blank_transactions (file);
  }

  /* Purge any unused standing orders from the file. */

  if (sorders)
  {
    for (i=0; i<file->sorder_count; i++)
    {
      if (file->sorders[i].adjusted_next_date == NULL_DATE)
      {
        if (!delete_sorder (file, i))
        {
          i--; /* Account for the record having been deleted. */
        }
      }
    }
  }

  /* Purge unused accounts and headings from the file. */


  if (accounts || headings)
  {
    for (i=0; i<file->account_count; i++)
    {
      if (!account_used_in_file (file, i) &&
          ((accounts && ((file->accounts[i].type & ACCOUNT_FULL) != 0)) ||
           (headings && ((file->accounts[i].type & (ACCOUNT_IN | ACCOUNT_OUT)) != 0))))
      {
        #ifdef DEBUG
        debug_printf ("Deleting account %d, type %x", i, file->accounts[i].type);
        #endif
        delete_account (file, i);
      }
    }
  }

  /* Recalculate the file and update the window. */

  rebuild_all_account_views (file);

  /* perform_full_recalculation (file); */

  *(file->filename) = '\0';
  build_transaction_window_title (file);
  set_file_data_integrity (file, 1);

  /* Put the caret into the first empty line. */

  scroll_transaction_window_to_end (file, -1);

  set_transaction_window_extent (file);

  place_transaction_edit_line (file, file->trans_count);
  put_caret_at_end (file->transaction_window.transaction_window, 0);
  find_transaction_edit_line (file);

  hourglass_off ();
}
