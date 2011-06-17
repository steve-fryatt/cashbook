/* CashBook - find.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/errors.h"
#include "sflib/debug.h"
#include "sflib/string.h"
#include "sflib/msgs.h"

/* Application header files */

#include "global.h"
#include "find.h"

#include "account.h"
#include "caret.h"
#include "conversion.h"
#include "date.h"
#include "edit.h"
#include "mainmenu.h"
#include "transact.h"
#include "window.h"

/* ==================================================================================================================
 * Global variables.
 */

static file_data *find_window_file = NULL;
static find       find_params;
static int        find_window_clear = 0;

/* ==================================================================================================================
 * Open the goto window.
 */

void open_find_window (file_data *file, wimp_pointer *ptr, int clear)
{
  extern global_windows windows;


  /* If either of the find/found windows are already open, close them to start with. */

  if (windows_get_open (windows.find))
  {
    wimp_close_window (windows.find);
  }

  if (windows_get_open (windows.found))
  {
    wimp_close_window (windows.found);
  }

  /* Blank out the icon contents. */

  fill_find_window (&(file->find), clear);

  /* Set the pointer up to find the window again and open the window. */

  find_window_file = file;
  find_window_clear = clear;

  windows_open_centred_at_pointer (windows.find, ptr);
  place_dialogue_caret (windows.find, FIND_ICON_DATE);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* re-open the find window, from the 'modify' icon in the found window, with the current search params. */

void reopen_find_window (wimp_pointer *ptr)
{
  extern global_windows windows;


  /* If either of the find/found windows are already open, close them to start with. */

  if (windows_get_open (windows.find))
  {
    wimp_close_window (windows.find);
  }

  if (windows_get_open (windows.found))
  {
    wimp_close_window (windows.found);
  }

  /* Blank out the icon contents. */

  fill_find_window (&find_params, 1);

  find_window_clear = 1;

  windows_open_centred_at_pointer (windows.find, ptr);
  place_dialogue_caret (windows.find, FIND_ICON_DATE);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Refresh the find window. */

void refresh_find_window (void)
{
  extern global_windows windows;


  fill_find_window (&(find_window_file->find), find_window_clear);
  icons_redraw_group (windows.find, 10, FIND_ICON_DATE,
                          FIND_ICON_FMIDENT, FIND_ICON_FMREC, FIND_ICON_FMNAME,
                          FIND_ICON_TOIDENT, FIND_ICON_TOREC, FIND_ICON_TONAME,
                          FIND_ICON_REF, FIND_ICON_AMOUNT, FIND_ICON_DESC);
  icons_replace_caret_in_window (windows.find);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_find_window (find *find_data, int clear)
{
  extern global_windows windows;


  if (clear == 0)
  {
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_DATE) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_FMIDENT) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_FMREC) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_FMNAME) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_TOIDENT) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_TOREC) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_TONAME) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_REF) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_AMOUNT) = '\0';
    *icons_get_indirected_text_addr (windows.find, FIND_ICON_DESC) = '\0';

    icons_set_selected (windows.find, FIND_ICON_AND, 1);
    icons_set_selected (windows.find, FIND_ICON_OR, 0);

    icons_set_selected (windows.find, FIND_ICON_START, 1);
    icons_set_selected (windows.find, FIND_ICON_DOWN, 0);
    icons_set_selected (windows.find, FIND_ICON_UP, 0);
    icons_set_selected (windows.find, FIND_ICON_END, 0);
    icons_set_selected (windows.find, FIND_ICON_CASE, 0);
  }
  else
  {
    convert_date_to_string (find_data->date, icons_get_indirected_text_addr (windows.find, FIND_ICON_DATE));

    fill_account_field(find_window_file, find_data->from, find_data->from_rec,
                       windows.find, FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC);

    fill_account_field(find_window_file, find_data->to, find_data->to_rec,
                       windows.find, FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC);

    strcpy (icons_get_indirected_text_addr (windows.find, FIND_ICON_REF), find_data->ref);
    convert_money_to_string (find_data->amount, icons_get_indirected_text_addr (windows.find, FIND_ICON_AMOUNT));
    strcpy (icons_get_indirected_text_addr (windows.find, FIND_ICON_DESC), find_data->desc);

    icons_set_selected (windows.find, FIND_ICON_AND, find_data->logic == FIND_AND);
    icons_set_selected (windows.find, FIND_ICON_OR, find_data->logic == FIND_OR);

    icons_set_selected (windows.find, FIND_ICON_START, find_data->direction == FIND_START);
    icons_set_selected (windows.find, FIND_ICON_DOWN, find_data->direction == FIND_DOWN);
    icons_set_selected (windows.find, FIND_ICON_UP, find_data->direction == FIND_UP);
    icons_set_selected (windows.find, FIND_ICON_END, find_data->direction == FIND_END);

    icons_set_selected (windows.find, FIND_ICON_CASE, find_data->case_sensitive);
    icons_set_selected (windows.find, FIND_ICON_WHOLE, find_data->whole_text);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Update the account name fields in the find transaction window. */

void update_find_account_fields (wimp_key *key)
{
  extern global_windows windows;


  if (key->i == FIND_ICON_FMIDENT)
  {
    lookup_account_field (find_window_file, key->c, ACCOUNT_IN | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
                          windows.find, FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC);
  }

  else if (key->i == FIND_ICON_TOIDENT)
  {
    lookup_account_field (find_window_file, key->c, ACCOUNT_OUT | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
                          windows.find, FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_find_account_menu (wimp_pointer *ptr)
{
  extern global_windows windows;


  if (ptr->i == FIND_ICON_FMNAME)
  {
    open_account_menu (find_window_file, ACCOUNT_MENU_FROM, 0,
                          windows.find, FIND_ICON_FMIDENT, FIND_ICON_FMNAME, FIND_ICON_FMREC, ptr);
  }

  else if (ptr->i == FIND_ICON_TONAME)
  {
    open_account_menu (find_window_file, ACCOUNT_MENU_TO, 0,
                          windows.find, FIND_ICON_TOIDENT, FIND_ICON_TONAME, FIND_ICON_TOREC, ptr);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void toggle_find_reconcile_fields (wimp_pointer *ptr)
{
  extern global_windows windows;


  if (ptr->i == FIND_ICON_FMREC)
  {
    toggle_account_reconcile_icon (windows.find, FIND_ICON_FMREC);
  }

  else if (ptr->i == FIND_ICON_TOREC)
  {
    toggle_account_reconcile_icon (windows.find, FIND_ICON_TOREC);
  }
}

/* ==================================================================================================================
 * Perform a find operation.
 */

int process_find_window (void)
{
  int                   line;

  extern global_windows windows;


  /* Get the window contents. */

  find_window_file->find.date = convert_string_to_date (icons_get_indirected_text_addr (windows.find, FIND_ICON_DATE),
                                                        NULL_DATE, 0);
  find_window_file->find.from = find_account (find_window_file, icons_get_indirected_text_addr (windows.find, FIND_ICON_FMIDENT),
                                              ACCOUNT_FULL | ACCOUNT_IN);
  find_window_file->find.from_rec = (*icons_get_indirected_text_addr (windows.find, FIND_ICON_FMREC) == '\0') ?
                                    0 : TRANS_REC_FROM;
  find_window_file->find.to = find_account (find_window_file, icons_get_indirected_text_addr (windows.find, FIND_ICON_TOIDENT),
                                            ACCOUNT_FULL | ACCOUNT_OUT);
  find_window_file->find.to_rec = (*icons_get_indirected_text_addr (windows.find, FIND_ICON_TOREC) == '\0') ? 0 : TRANS_REC_TO;
  find_window_file->find.amount = convert_string_to_money (icons_get_indirected_text_addr (windows.find, FIND_ICON_AMOUNT));
  strcpy (find_window_file->find.ref, icons_get_indirected_text_addr (windows.find, FIND_ICON_REF));
  strcpy (find_window_file->find.desc, icons_get_indirected_text_addr (windows.find, FIND_ICON_DESC));

  /* Read find logic. */

  if (icons_get_selected (windows.find, FIND_ICON_AND))
  {
    find_window_file->find.logic = FIND_AND;
  }
  else if (icons_get_selected (windows.find, FIND_ICON_OR))
  {
    find_window_file->find.logic = FIND_OR;
  }

  /* Read search direction. */

  if (icons_get_selected (windows.find, FIND_ICON_START))
  {
    find_window_file->find.direction = FIND_START;
  }
  else if (icons_get_selected (windows.find, FIND_ICON_END))
  {
    find_window_file->find.direction = FIND_END;
  }
  else if (icons_get_selected (windows.find, FIND_ICON_DOWN))
  {
    find_window_file->find.direction = FIND_DOWN;
  }
  else if (icons_get_selected (windows.find, FIND_ICON_UP))
  {
    find_window_file->find.direction = FIND_UP;
  }

  find_window_file->find.case_sensitive = icons_get_selected (windows.find, FIND_ICON_CASE);
  find_window_file->find.whole_text = icons_get_selected (windows.find, FIND_ICON_WHOLE);

  /* Get the start line. */

  if (find_window_file->find.direction == FIND_START)
  {
    line = 0;
  }
  else if (find_window_file->find.direction == FIND_END)
  {
    line = find_window_file->trans_count-1;
  }
  else if (find_window_file->find.direction == FIND_DOWN)
  {
    line = find_window_file->transaction_window.entry_line+1;
    if (line >= find_window_file->trans_count)
    {
      line = find_window_file->trans_count-1;
    }
  }
  else /* FIND_UP */
  {
    line = find_window_file->transaction_window.entry_line-1;
    if (line < 0)
    {
      line = 0;
    }
  }

  /* Start the search. */

  line = find_from_line (&(find_window_file->find), FIND_NODIR, line);

  if (line != NULL_TRANSACTION)
  {
    return (0);
  }
  else
  {
    return (1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

int find_from_line (find *new_params, int new_dir, int start)
{
  int                   match, icon, line, transaction, direction;
  unsigned              test;
  wimp_pointer          pointer;
  char                  buf1[64], buf2[64], ref[REF_FIELD_LEN+2], desc[DESCRIPT_FIELD_LEN+2];

  extern global_windows windows;


  /* If new parameters are being supplied, take copies. */

  if (new_params != NULL)
  {
    find_params.date = new_params->date;
    find_params.from = new_params->from;
    find_params.from_rec = new_params->from_rec;
    find_params.to = new_params->to;
    find_params.to_rec = new_params->to_rec;
    find_params.amount = new_params->amount;
    strcpy (find_params.ref, new_params->ref);
    strcpy (find_params.desc, new_params->desc);

    find_params.logic = new_params->logic;
    find_params.case_sensitive = new_params->case_sensitive;
    find_params.whole_text = new_params->whole_text;
    find_params.direction = new_params->direction;

    /* Start and End have served their purpose; they now need to convert into up and down. */

    if (find_params.direction == FIND_START)
    {
      find_params.direction = FIND_DOWN;
    }
    if (find_params.direction == FIND_END)
    {
      find_params.direction = FIND_UP;
    }
  }


  /* Take local copies of the two text fields, and add bracketing wildcards as necessary. */

  if ((!find_params.whole_text) && (*(find_params.ref) != '\0')) {
    snprintf(ref, REF_FIELD_LEN + 2, "*%s*", find_params.ref);
  } else {
    strcpy (ref, find_params.ref);
  }

  if ((!find_params.whole_text) && (*(find_params.desc) != '\0')) {
    snprintf(desc, DESCRIPT_FIELD_LEN + 2, "*%s*", find_params.desc);
  } else {
    strcpy (desc, find_params.desc);
  }

  /* If the search needs to change direction, do so now. */

  if (new_dir == FIND_NEXT || new_dir == FIND_PREVIOUS) {
    if (find_params.direction == FIND_UP)
      direction = (new_dir == FIND_NEXT) ? FIND_UP : FIND_DOWN;
    else
      direction = (new_dir == FIND_NEXT) ? FIND_DOWN : FIND_UP;
  } else {
    direction = find_params.direction;
  }

  /* If a new start line is being specified, take note, elseuse the current edit line. */

  if (start == NULL_TRANSACTION)
  {
    if (direction == FIND_DOWN)
    {
      line = find_window_file->transaction_window.entry_line + 1;
      if (line >= find_window_file->trans_count)
      {
        line = find_window_file->trans_count - 1;
      }
    }
    else /* FIND_UP */
    {
      line = find_window_file->transaction_window.entry_line - 1;
      if (line < 0)
      {
        line = 0;
      }
    }
  }
  else
  {
    line = start;
  }

  match = 0;
  icon = -1;

  while (line < find_window_file->trans_count && line >= 0 && !match)
  {
    /* Initialise the test result variable.  The tests all have a bit allocated.  For OR tests, these start unset and
     * are set if a test passes; a non-zero value at the end indicates a match.  For AND tests, all the required bits
     * are set at the start and cleared as tests match.  A zero value at the end indicates a match.
     */

    test = 0;

    if (find_params.logic == FIND_AND)
    {
      if (find_params.date != NULL_DATE)
      {
        test |= FIND_TEST_DATE;
      }

      if (find_params.from != NULL_ACCOUNT)
      {
        test |= FIND_TEST_FROM;
      }

      if (find_params.to != NULL_ACCOUNT)
      {
        test |= FIND_TEST_TO;
      }

      if (find_params.amount != NULL_CURRENCY)
      {
        test |= FIND_TEST_AMOUNT;
      }

      if (*find_params.ref != '\0')
      {
        test |= FIND_TEST_REF;
      }

      if (*find_params.desc != '\0')
      {
        test |= FIND_TEST_DESC;
      }
    }

    /* Perform the tests.  Tests are carried out in reverse order, with the icon being set in each, such that the
     * returned icon ends up in the left-most column.
     */

    transaction = find_window_file->transactions[line].sort_index;

    if (*desc != '\0' &&
        string_wildcard_compare (desc, find_window_file->transactions[transaction].description, !find_params.case_sensitive))
    {
      test ^= FIND_TEST_DESC;
      icon = EDIT_ICON_DESCRIPT;
    }

    if (find_params.amount != NULL_CURRENCY && find_params.amount == find_window_file->transactions[transaction].amount)
    {
      test ^= FIND_TEST_AMOUNT;
      icon = EDIT_ICON_AMOUNT;
    }

    if (*ref != '\0' &&
        string_wildcard_compare (ref, find_window_file->transactions[transaction].reference, !find_params.case_sensitive))
    {
      test ^= FIND_TEST_REF;
      icon = EDIT_ICON_REF;
    }

    /* The following two tests check that a) an account has been specified, b) it is the same as the transaction and
     * c) the two reconcile flags are set the same (if they are, the EOR operation cancels them out).
     */

    if (find_params.to != NULL_ACCOUNT && find_params.to == find_window_file->transactions[transaction].to
        && ((find_params.to_rec ^ find_window_file->transactions[transaction].flags) & TRANS_REC_TO) == 0)
    {
      test ^= FIND_TEST_TO;
      icon = EDIT_ICON_TO;
    }

    if (find_params.from != NULL_ACCOUNT && find_params.from == find_window_file->transactions[transaction].from
        && ((find_params.from_rec ^ find_window_file->transactions[transaction].flags) & TRANS_REC_FROM) == 0)
    {
      test ^= FIND_TEST_FROM;
      icon = EDIT_ICON_FROM;
    }

    if (find_params.date != NULL_DATE && find_params.date == find_window_file->transactions[transaction].date)
    {
      test ^= FIND_TEST_DATE;
      icon = EDIT_ICON_DATE;
    }

    /* Check if the test passed or failed. */

    if ((find_params.logic == FIND_OR && test) || (find_params.logic == FIND_AND && !test))
    {
      match = 1;
    }
    else
    {
      if (direction == FIND_UP)
      {
        line --;
      }
      else
      {
        line ++;
      }
    }
  }

  if (match)
  {
    wimp_close_window (windows.find);

    place_transaction_edit_line (find_window_file, line);
    icons_put_caret_at_end (find_window_file->transaction_window.transaction_window, icon);
    find_transaction_edit_line (find_window_file);

    icons_copy_text (find_window_file->transaction_window.transaction_pane, column_group (TRANSACT_PANE_COL_MAP,icon), buf1);
    sprintf (buf2, "%d", line);

    msgs_param_lookup("Found", icons_get_indirected_text_addr (windows.found, FOUND_ICON_INFO), 64, buf1, buf2, NULL, NULL);

    if (!windows_get_open (windows.found))
    {
      wimp_get_pointer_info (&pointer);
      windows_open_centred_at_pointer (windows.found, &pointer);
    }
    else
    {
      icons_redraw_group (windows.found, 1, FOUND_ICON_INFO);
    }

    return (line);
  }
  else
  {
    wimp_msgtrans_info_report ("BadFind");

    return (NULL_TRANSACTION);
  }
}

/* ==================================================================================================================
 * Force the window to close.
 */

/* Force the closure of the find or found windows if the file disappears. */

void force_close_find_window (file_data *file)
{
  extern global_windows windows;


  if (find_window_file == file && windows_get_open (windows.find))
  {
    close_dialogue_with_caret (windows.find);
  }

  if (find_window_file == file && windows_get_open (windows.found))
  {
    wimp_close_window (windows.found);
  }
}
