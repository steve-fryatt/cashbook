/* CashBook - transact.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/heap.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/debug.h"

/* Application header files */

#include "global.h"
#include "transact.h"

#include "account.h"
#include "accview.h"
#include "calculation.h"
#include "caret.h"
#include "clipboard.h"
#include "conversion.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "fileinfo.h"
#include "filing.h"
#include "find.h"
#include "goto.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "presets.h"
#include "printing.h"
#include "report.h"
#include "sorder.h"
#include "window.h"

/* ==================================================================================================================
 * Global variables
 */

static int    new_transaction_window_offset = 0;
static wimp_i transaction_pane_sort_substitute_icon = TRANSACT_PANE_DATE;

file_data     *transact_print_file = NULL;
file_data     *sort_trans_window_file = NULL;

/* ==================================================================================================================
 * Window creation and deletion
 */

void create_transaction_window (file_data *file)
{
  int                   i, j, height;
  os_error              *error;

  extern global_windows windows;


  if (file->transaction_window.transaction_window != NULL)
  {
    /* The window is open, so just bring it forward. */

    windows_open (file->transaction_window.transaction_window);
  }
  else
  {
    /* Create the new window data and build the window. */

    *(file->transaction_window.window_title) = '\0';
    windows.transaction_window_def->title_data.indirected_text.text = file->transaction_window.window_title;

    file->transaction_window.display_lines = (file->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
                                             file->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;

    height =  file->transaction_window.display_lines;

    set_initial_window_area (windows.transaction_window_def,
                             file->transaction_window.column_position[TRANSACT_COLUMNS-1] +
                             file->transaction_window.column_width[TRANSACT_COLUMNS-1],
                              ((ICON_HEIGHT+LINE_GUTTER) * height) + TRANSACT_TOOLBAR_HEIGHT,
                               -1, -1, new_transaction_window_offset * TRANSACTION_WINDOW_OPEN_OFFSET);

    error = xwimp_create_window (windows.transaction_window_def, &(file->transaction_window.transaction_window));
    if (error != NULL)
    {
      error_report_os_error (error, wimp_ERROR_BOX_CANCEL_ICON);
      return;
    }

    new_transaction_window_offset++;
    if (new_transaction_window_offset >= TRANSACTION_WINDOW_OFFSET_LIMIT)
    {
      new_transaction_window_offset = 0;
    }

    /* Create the toolbar pane. */

    windows_place_as_toolbar (windows.transaction_window_def, windows.transaction_pane_def, TRANSACT_TOOLBAR_HEIGHT-4);

    for (i=0, j=0; j < TRANSACT_COLUMNS; i++, j++)
    {
      windows.transaction_pane_def->icons[i].extent.x0 = file->transaction_window.column_position[j];

      j = rightmost_group_column (TRANSACT_PANE_COL_MAP, i);

      windows.transaction_pane_def->icons[i].extent.x1 = file->transaction_window.column_position[j] +
                                                         file->transaction_window.column_width[j] +
                                                         COLUMN_HEADING_MARGIN;
    }

    windows.transaction_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
                                  (osspriteop_id) file->transaction_window.sort_sprite;
    windows.transaction_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
                                  windows.transaction_pane_def->sprite_area;

    update_transaction_window_sort_icon (file, &(windows.transaction_pane_def->icons[TRANSACT_PANE_SORT_DIR_ICON]));

    error = xwimp_create_window (windows.transaction_pane_def, &(file->transaction_window.transaction_pane));
    if (error != NULL)
    {
      error_report_os_error (error, wimp_ERROR_BOX_CANCEL_ICON);
      return;
    }

    /* Set the title */

    build_transaction_window_title (file);

    /* Update the toolbar */

    update_transaction_window_toolbar (file);

    /* Set the default values */

    file->transaction_window.entry_line = -1;
    file->transaction_window.display_lines = MIN_TRANSACT_ENTRIES;

    /* Open the window. */

    windows_open (file->transaction_window.transaction_window);
    windows_open_nested_as_toolbar (file->transaction_window.transaction_pane,
                                   file->transaction_window.transaction_window,
                                   TRANSACT_TOOLBAR_HEIGHT-4);

    ihelp_add_window (file->transaction_window.transaction_window , "Transact", decode_transact_window_help);
    ihelp_add_window (file->transaction_window.transaction_pane , "TransactTB", NULL);

    /* Put the caret into the first empty line. */

    place_transaction_edit_line (file, file->trans_count);
    icons_put_caret_at_end (file->transaction_window.transaction_window, 0);
    find_transaction_edit_line (file);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void delete_transaction_window (file_data *file)
{
  #ifdef DEBUG
  debug_printf ("\\RDeleting transaction window");
  #endif

  if (file->transaction_window.transaction_window != NULL)
  {
    ihelp_remove_window (file->transaction_window.transaction_window);
    ihelp_remove_window (file->transaction_window.transaction_pane);

    wimp_delete_window (file->transaction_window.transaction_window);
    wimp_delete_window (file->transaction_window.transaction_pane);

    file->transaction_window.transaction_window = NULL;
    file->transaction_window.transaction_pane = NULL;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_transaction_window_columns (file_data *file)
{
  int              i, j, new_extent;
  wimp_icon_state  icon;
  wimp_window_info window;
  wimp_caret       caret;


  /* Re-adjust the icons in the pane. */

  for (i=0, j=0; j < TRANSACT_COLUMNS; i++, j++)
  {
    icon.w = file->transaction_window.transaction_pane;
    icon.i = i;
    wimp_get_icon_state (&icon);

    icon.icon.extent.x0 = file->transaction_window.column_position[j];

    j = rightmost_group_column (TRANSACT_PANE_COL_MAP, i);

    icon.icon.extent.x1 = file->transaction_window.column_position[j] +
                          file->transaction_window.column_width[j] + COLUMN_HEADING_MARGIN;

    wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                      icon.icon.extent.x1, icon.icon.extent.y1);

    new_extent = file->transaction_window.column_position[TRANSACT_COLUMNS-1] +
                 file->transaction_window.column_width[TRANSACT_COLUMNS-1];
  }

  adjust_transaction_window_sort_icon (file);

  /* Replace the edit line to force a redraw and redraw the rest of the window. */

  wimp_get_caret_position (&caret);

  place_transaction_edit_line (file, file->transaction_window.entry_line);
  windows_redraw (file->transaction_window.transaction_window);
  windows_redraw (file->transaction_window.transaction_pane);

  if (file->transaction_window.transaction_window != NULL &&
      file->transaction_window.transaction_window == caret.w)
  {
    /* If the caret's position was in the current transaction window, we need to replace it in the same position
     * now, so that we don't lose input focus.
     */

    wimp_set_caret_position (caret.w, caret.i, 0, 0, -1, caret.index);
  }

  /* Set the horizontal extent of the window and pane. */

  window.w = file->transaction_window.transaction_pane;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  window.w = file->transaction_window.transaction_window;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  windows_open (window.w);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_transaction_window_sort_icon (file_data *file)
{
  wimp_icon_state icon;

  icon.w = file->transaction_window.transaction_pane;
  icon.i = TRANSACT_PANE_SORT_DIR_ICON;
  wimp_get_icon_state (&icon);

  update_transaction_window_sort_icon (file, &(icon.icon));

  wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                    icon.icon.extent.x1, icon.icon.extent.y1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void update_transaction_window_sort_icon (file_data *file, wimp_icon *icon)
{
  int              i, width, anchor;


  i = 0;

  if (file->transaction_window.sort_order & SORT_ASCENDING)
  {
    strcpy (file->transaction_window.sort_sprite, "sortarrd");
  }
  else if (file->transaction_window.sort_order & SORT_DESCENDING)
  {
    strcpy (file->transaction_window.sort_sprite, "sortarru");
  }

  switch (file->transaction_window.sort_order & SORT_MASK)
  {
    case SORT_DATE:
      i = 0;
      transaction_pane_sort_substitute_icon = TRANSACT_PANE_DATE;
      break;

    case SORT_FROM:
      i = 3;
      transaction_pane_sort_substitute_icon = TRANSACT_PANE_FROM;
      break;

    case SORT_TO:
      i = 6;
      transaction_pane_sort_substitute_icon = TRANSACT_PANE_TO;
      break;

    case SORT_REFERENCE:
      i = 7;
      transaction_pane_sort_substitute_icon = TRANSACT_PANE_REFERENCE;
      break;

    case SORT_AMOUNT:
      i = 8;
      transaction_pane_sort_substitute_icon = TRANSACT_PANE_AMOUNT;
      break;

    case SORT_DESCRIPTION:
      i = 9;
      transaction_pane_sort_substitute_icon = TRANSACT_PANE_DESCRIPTION;
      break;
  }

  width = icon->extent.x1 - icon->extent.x0;

  if ((file->transaction_window.sort_order & SORT_MASK) == SORT_AMOUNT)
  {
    anchor = file->transaction_window.column_position[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
    icon->extent.x1 = icon->extent.x0 + width;
  }
  else
  {
    anchor = file->transaction_window.column_position[i] +
             file->transaction_window.column_width[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
    icon->extent.x0 = icon->extent.x1 - width;
  }

}

/* ==================================================================================================================
 * Transaction handling
 */

/* Adds a new transaction to the end of the list. */

void add_raw_transaction (file_data *file, unsigned date, int from, int to, unsigned flags, int amount,
                          char *ref, char *description)
{
  int new;


  if (flex_extend ((flex_ptr) &(file->transactions), sizeof (transaction) * (file->trans_count+1)) == 1)
  {
    new = file->trans_count++;

    file->transactions[new].date = date;
    file->transactions[new].amount = amount;
    file->transactions[new].from = from;
    file->transactions[new].to = to;
    file->transactions[new].flags = flags;
    strcpy (file->transactions[new].reference, ref);
    strcpy (file->transactions[new].description, description);

    file->transactions[new].sort_index = new;

    set_file_data_integrity (file, 1);
    if (date != NULL_DATE)
    {
      file->sort_valid = 0;
    }
  }
  else
  {
    error_msgs_report_error ("NoMemNewTrans");
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return true if the transaction specified is completely empty. */

int is_transaction_blank (file_data *file, int transaction)
{
  return (file->transactions[transaction].date == NULL_DATE &&
          file->transactions[transaction].from == NULL_ACCOUNT &&
          file->transactions[transaction].to == NULL_ACCOUNT &&
          file->transactions[transaction].amount == NULL_CURRENCY &&
          *file->transactions[transaction].reference == '\0' &&
          *file->transactions[transaction].description == '\0');
}

/* ------------------------------------------------------------------------------------------------------------------ */


/* Strip blank transactions from the file.  This relies on the blank transactions being at the end, which relies
 * on a transaction list sort having occurred just before the function is called.
 */

void strip_blank_transactions (file_data *file)
{
  int i, j, found;


  i = file->trans_count - 1;

  while (is_transaction_blank (file, i))
  {
    /* Search the trasnaction sort index, looking for a line pointing to the blank.  If one is found, shuffle all
     * the following indexes up to compensate.
     */

    found = 0;

    for (j=0; j<i; j++)
    {
      if (file->transactions[j].sort_index == i)
      {
        found = 1;
      }

      if (found)
      {
        file->transactions[j].sort_index = file->transactions[j+1].sort_index;
      }
    }

    /* Remove the transaction. */

    i--;
  }

  /* Shuffle memory to reduce the transaction space. */

  if (i < file->trans_count-1)
  {
    file->trans_count = i+1;

    flex_extend ((flex_ptr) &(file->transactions), sizeof (transaction) * file->trans_count);
  }
}

/* ==================================================================================================================
 * Sorting transactions
 */

/* Sorts the transactions in the underlying file by date and amount.  This does not affect the view in the
 * transaction window -- to sort this, use sort_transaction_window ().  As a result, we do not need to look after the
 * location of things like the edit line; it does need to keep track of transactions[].sort_index, however.
 */

void sort_transactions (file_data *file)
{
  int         i, sorted, gap, comb;
  transaction temp;

  #ifdef DEBUG
  debug_printf("Sorting transactions");
  #endif

  hourglass_on ();

  /* Start by recording the order of the transactions on display in the main window, and also the order of
   * the transactions themselves.
   */

  for (i=0; i < file->trans_count; i++)
  {
    file->transactions[file->transactions[i].sort_index].saved_sort = i; /* Record transaction window lines. */
    file->transactions[i].sort_index = i;                                /* Record old transaction locations. */
  }

  /* Sort the entries using a combsort.  This has the advantage over qsort () that the order of entries is only
   * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
   */

  gap = file->trans_count - 1;

  do
  {
    gap = (gap > 1) ? (gap * 10 / 13) : 1;
    if ((file->trans_count >= 12) && (gap == 9 || gap == 10))
    {
      gap = 11;
    }

    sorted = 1;
    for (comb = 0; (comb + gap) < file->trans_count; comb++)
    {
      if (file->transactions[comb+gap].date < file->transactions[comb].date)
      {
        temp = file->transactions[comb+gap];
        file->transactions[comb+gap] = file->transactions[comb];
        file->transactions[comb] = temp;

        sorted = 0;
      }
    }
  }
  while (!sorted || gap != 1);

  /* Finally, restore the order of the transactions on display in the main window.
   */
  for (i=0; i < file->trans_count; i++)
  {
    file->transactions[file->transactions[i].sort_index].sort_workspace = i;
  }

  reindex_all_account_views (file);

  for (i=0; i < file->trans_count; i++)
  {
    file->transactions[file->transactions[i].saved_sort].sort_index = i;
  }

  file->sort_valid = 1;

  hourglass_off ();
}

/* ------------------------------------------------------------------------------------------------------------------ */

void sort_transaction_window (file_data *file)
{
  wimp_caret  caret;
  int         sorted, reorder, gap, comb, temp, order, edit_transaction;


  #ifdef DEBUG
  debug_printf("Sorting transaction window");
  #endif

  hourglass_on ();

  /* Find the caret position and edit line before sorting. */

  wimp_get_caret_position (&caret);
  edit_transaction = get_edit_line_transaction (file);

  /* Sort the entries using a combsort.  This has the advantage over qsort () that the order of entries is only
   * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
   */

  gap = file->trans_count - 1;

  order = file->transaction_window.sort_order;

  do
  {
    gap = (gap > 1) ? (gap * 10 / 13) : 1;
    if ((file->trans_count >= 12) && (gap == 9 || gap == 10))
    {
      gap = 11;
    }

    sorted = 1;
    for (comb = 0; (comb + gap) < file->trans_count; comb++)
    {
      switch (order)
      {
        case SORT_DATE | SORT_ASCENDING:
          reorder = (file->transactions[file->transactions[comb+gap].sort_index].date <
                     file->transactions[file->transactions[comb].sort_index].date);
          break;

        case SORT_DATE | SORT_DESCENDING:
          reorder = (file->transactions[file->transactions[comb+gap].sort_index].date >
                     file->transactions[file->transactions[comb].sort_index].date);
          break;

        case SORT_FROM | SORT_ASCENDING:
          reorder = (strcmp(find_account_name(file, file->transactions[file->transactions[comb+gap].sort_index].from),
                     find_account_name(file, file->transactions[file->transactions[comb].sort_index].from)) < 0);
          break;

        case SORT_FROM | SORT_DESCENDING:
          reorder = (strcmp(find_account_name(file, file->transactions[file->transactions[comb+gap].sort_index].from),
                     find_account_name(file, file->transactions[file->transactions[comb].sort_index].from)) > 0);
          break;

        case SORT_TO | SORT_ASCENDING:
          reorder = (strcmp(find_account_name(file, file->transactions[file->transactions[comb+gap].sort_index].to),
                     find_account_name(file, file->transactions[file->transactions[comb].sort_index].to)) < 0);
          break;

        case SORT_TO | SORT_DESCENDING:
          reorder = (strcmp(find_account_name(file, file->transactions[file->transactions[comb+gap].sort_index].to),
                     find_account_name(file, file->transactions[file->transactions[comb].sort_index].to)) > 0);
          break;

        case SORT_REFERENCE | SORT_ASCENDING:
          reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].reference,
                     file->transactions[file->transactions[comb].sort_index].reference) < 0);
          break;

        case SORT_REFERENCE | SORT_DESCENDING:
          reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].reference,
                     file->transactions[file->transactions[comb].sort_index].reference) > 0);
          break;

        case SORT_AMOUNT | SORT_ASCENDING:
          reorder = (file->transactions[file->transactions[comb+gap].sort_index].amount <
                     file->transactions[file->transactions[comb].sort_index].amount);
          break;

        case SORT_AMOUNT | SORT_DESCENDING:
          reorder = (file->transactions[file->transactions[comb+gap].sort_index].amount >
                     file->transactions[file->transactions[comb].sort_index].amount);
          break;

        case SORT_DESCRIPTION | SORT_ASCENDING:
          reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].description,
                     file->transactions[file->transactions[comb].sort_index].description) < 0);
          break;

        case SORT_DESCRIPTION | SORT_DESCENDING:
          reorder = (strcmp(file->transactions[file->transactions[comb+gap].sort_index].description,
                     file->transactions[file->transactions[comb].sort_index].description) > 0);
          break;

        default:
          reorder = 0;
          break;
      }

      if (reorder)
      {
        temp = file->transactions[comb+gap].sort_index;
        file->transactions[comb+gap].sort_index = file->transactions[comb].sort_index;
        file->transactions[comb].sort_index = temp;

        sorted = 0;
      }
    }
  }
  while (!sorted || gap != 1);

  /* Replace the edit line where we found it prior to the sort. */

  place_transaction_edit_line_transaction (file, edit_transaction);

  if (file->transaction_window.transaction_window != NULL &&
      file->transaction_window.transaction_window == caret.w)
  {
    /* If the caret's position was in the current transaction window, we need to replace it in the same position
     * now, so that we don't lose input focus.
     */

    wimp_set_caret_position (caret.w, caret.i, 0, 0, -1, caret.index);
  }

  force_transaction_window_redraw (file, 0, file->trans_count - 1);

  hourglass_off ();
}

/* ================================================================================================================== */

void open_transaction_sort_window (file_data *file, wimp_pointer *ptr)
{
  extern global_windows windows;

  /* If the window is open elsewhere, close it first. */

  if (windows_get_open (windows.sort_trans))
  {
    wimp_close_window (windows.sort_trans);
  }

  fill_transaction_sort_window (file->transaction_window.sort_order);

  sort_trans_window_file = file;

  windows_open_centred_at_pointer (windows.sort_trans, ptr);
  place_dialogue_caret (windows.sort_trans, wimp_ICON_WINDOW);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_transaction_sort_window (void)
{
  fill_transaction_sort_window (sort_trans_window_file->transaction_window.sort_order);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_transaction_sort_window (int sort_option)
{
  extern global_windows windows;

  icons_set_selected (windows.sort_trans, TRANS_SORT_DATE, (sort_option & SORT_MASK) == SORT_DATE);
  icons_set_selected (windows.sort_trans, TRANS_SORT_FROM, (sort_option & SORT_MASK) == SORT_FROM);
  icons_set_selected (windows.sort_trans, TRANS_SORT_TO, (sort_option & SORT_MASK) == SORT_TO);
  icons_set_selected (windows.sort_trans, TRANS_SORT_REFERENCE, (sort_option & SORT_MASK) == SORT_REFERENCE);
  icons_set_selected (windows.sort_trans, TRANS_SORT_AMOUNT, (sort_option & SORT_MASK) == SORT_AMOUNT);
  icons_set_selected (windows.sort_trans, TRANS_SORT_DESCRIPTION, (sort_option & SORT_MASK) == SORT_DESCRIPTION);

  icons_set_selected (windows.sort_trans, TRANS_SORT_ASCENDING, sort_option & SORT_ASCENDING);
  icons_set_selected (windows.sort_trans, TRANS_SORT_DESCENDING, sort_option & SORT_DESCENDING);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_transaction_sort_window (void)
{
  extern global_windows windows;

  sort_trans_window_file->transaction_window.sort_order = SORT_NONE;

  if (icons_get_selected (windows.sort_trans, TRANS_SORT_DATE))
  {
    sort_trans_window_file->transaction_window.sort_order = SORT_DATE;
  }
  else if (icons_get_selected (windows.sort_trans, TRANS_SORT_FROM))
  {
    sort_trans_window_file->transaction_window.sort_order = SORT_FROM;
  }
  else if (icons_get_selected (windows.sort_trans, TRANS_SORT_TO))
  {
    sort_trans_window_file->transaction_window.sort_order = SORT_TO;
  }
  else if (icons_get_selected (windows.sort_trans, TRANS_SORT_REFERENCE))
  {
    sort_trans_window_file->transaction_window.sort_order = SORT_REFERENCE;
  }
  else if (icons_get_selected (windows.sort_trans, TRANS_SORT_AMOUNT))
  {
    sort_trans_window_file->transaction_window.sort_order = SORT_AMOUNT;
  }
  else if (icons_get_selected (windows.sort_trans, TRANS_SORT_DESCRIPTION))
  {
    sort_trans_window_file->transaction_window.sort_order = SORT_DESCRIPTION;
  }

  if (sort_trans_window_file->transaction_window.sort_order != SORT_NONE)
  {
    if (icons_get_selected (windows.sort_trans, TRANS_SORT_ASCENDING))
    {
      sort_trans_window_file->transaction_window.sort_order |= SORT_ASCENDING;
    }
    else if (icons_get_selected (windows.sort_trans, TRANS_SORT_DESCENDING))
    {
      sort_trans_window_file->transaction_window.sort_order |= SORT_DESCENDING;
    }
  }

  adjust_transaction_window_sort_icon (sort_trans_window_file);
  windows_redraw (sort_trans_window_file->transaction_window.transaction_pane);
  sort_transaction_window (sort_trans_window_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the find window if the file disappears. */

void force_close_transaction_sort_window (file_data *file)
{
  extern global_windows windows;


  if (sort_trans_window_file == file && windows_get_open (windows.sort_trans))
  {
    close_dialogue_with_caret (windows.sort_trans);
  }
}

/* ==================================================================================================================
 * Finding transactions
 */

void find_next_reconcile_line (file_data *file, int set)
{
  int        line, found, account;
  wimp_caret caret;


  if (file->auto_reconcile)
  {
    line = file->transaction_window.entry_line;
    account = NULL_ACCOUNT;

    wimp_get_caret_position (&caret);

    if (caret.i == 1)
    {
      account = file->transactions[file->transactions[line].sort_index].from;
    }
    else if (caret.i == 4)
    {
      account = file->transactions[file->transactions[line].sort_index].to;
    }

    if (account != NULL_ACCOUNT)
    {
      line++;
      found = 0;

      while (line < file->trans_count && !found)
      {
        if (file->transactions[file->transactions[line].sort_index].from == account &&
            ((file->transactions[file->transactions[line].sort_index].flags & TRANS_REC_FROM) == set * TRANS_REC_FROM))
        {
          found = 1;
        }

        else if (file->transactions[file->transactions[line].sort_index].to == account &&
                 ((file->transactions[file->transactions[line].sort_index].flags & TRANS_REC_TO) == set * TRANS_REC_TO))
        {
          found = 4;
        }

        else
        {
          line++;
        }
      }

      if (found)
      {
        place_transaction_edit_line (file, line);
        icons_put_caret_at_end (file->transaction_window.transaction_window, found);
        find_transaction_edit_line (file);
      }
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the first blank line at the end of the transaction window */

int find_first_blank_line (file_data *file)
{
  int line;


  #ifdef DEBUG
  debug_printf ("\\DFinding first blank line");
  #endif

  line = file->trans_count;

  while (line > 0 && is_transaction_blank (file, file->transactions[line - 1].sort_index))
  {
    line--;

    #ifdef DEBUG
    debug_printf ("Stepping back up...");
    #endif
  }

  return (line);
}

/* ==================================================================================================================
 * Printing transactions via the GUI
 */

void open_transact_print_window (file_data *file, wimp_pointer *ptr, int clear)
{
  /* Set the pointers up so we can find this lot again and open the window. */

  transact_print_file = file;

  printing_open_advanced_window (file, ptr, clear, "PrintTransact", print_transact_window);
}

/* ==================================================================================================================
 * File and print output
 */

void print_transact_window(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum, date_t from, date_t to)
{
  report_data *report;
  int            i, t;
  char           line[4096], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];

  msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);
  msgs_lookup ("PrintTitleTransact", buffer, sizeof (buffer));
  report = report_open (transact_print_file, buffer, NULL);

  if (report != NULL)
  {
    hourglass_on ();

    /* Output the page title. */

    make_file_leafname (transact_print_file, numbuf1, sizeof (numbuf1));
    msgs_param_lookup ("TransTitle", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
    sprintf (line, "\\b\\u%s", buffer);
    report_write_line (report, 0, line);
    report_write_line (report, 0, "");

    /* Output the headings line, taking the text from the window icons. */

    *line = '\0';
    sprintf (buffer, "\\k\\b\\u%s\\t", icons_copy_text (transact_print_file->transaction_window.transaction_pane, 0, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t\\s\\t\\s\\t",
             icons_copy_text (transact_print_file->transaction_window.transaction_pane, 1, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t\\s\\t\\s\\t",
             icons_copy_text (transact_print_file->transaction_window.transaction_pane, 2, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icons_copy_text (transact_print_file->transaction_window.transaction_pane, 3, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\r\\u%s\\t",
             icons_copy_text (transact_print_file->transaction_window.transaction_pane, 4, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icons_copy_text (transact_print_file->transaction_window.transaction_pane, 5, numbuf1));
    strcat (line, buffer);

    report_write_line (report, 0, line);

    /* Output the transaction data as a set of delimited lines. */

    for (i=0; i < transact_print_file->trans_count; i++)
    {
      if ((from == NULL_DATE || transact_print_file->transactions[i].date >= from) &&
          (to == NULL_DATE || transact_print_file->transactions[i].date <= to))
      {
        *line = '\0';

        t = transact_print_file->transactions[i].sort_index;

        convert_date_to_string (transact_print_file->transactions[t].date, numbuf1);
        sprintf (buffer, "\\k%s\\t", numbuf1);
        strcat (line, buffer);

        sprintf (buffer, "%s\\t", find_account_ident (transact_print_file, transact_print_file->transactions[t].from));
        strcat (line, buffer);

        strcpy (numbuf1, (transact_print_file->transactions[t].flags & TRANS_REC_FROM) ? rec_char : "");
        sprintf (buffer, "%s\\t", numbuf1);
        strcat (line, buffer);

        sprintf (buffer, "%s\\t", find_account_name (transact_print_file, transact_print_file->transactions[t].from));
        strcat (line, buffer);

        sprintf (buffer, "%s\\t", find_account_ident (transact_print_file, transact_print_file->transactions[t].to));
        strcat (line, buffer);

        strcpy (numbuf1, (transact_print_file->transactions[t].flags & TRANS_REC_TO) ? rec_char : "");
        sprintf (buffer, "%s\\t", numbuf1);
        strcat (line, buffer);

        sprintf (buffer, "%s\\t", find_account_name (transact_print_file, transact_print_file->transactions[t].to));
        strcat (line, buffer);

        sprintf (buffer, "%s\\t", transact_print_file->transactions[t].reference);
        strcat (line, buffer);

        convert_money_to_string (transact_print_file->transactions[t].amount, numbuf1);
        sprintf (buffer, "\\r%s\\t", numbuf1);
        strcat (line, buffer);

        sprintf (buffer, "%s\\t", transact_print_file->transactions[t].description);
        strcat (line, buffer);

        report_write_line (report, 0, line);
      }
    }

    hourglass_off ();
  }
  else
  {
    error_msgs_report_error ("PrintMemFail");
  }

  report_close_and_print(report, text, format, scale, rotate, pagenum);
}

/* ==================================================================================================================
 * Transaction window handling.
 */

/* Deal with mouse clicks in the transaction window. */

void transaction_window_click (file_data *file, wimp_pointer *pointer)
{
  int                 line, transaction, xpos, column;
  wimp_window_state   window;
  wimp_pointer        ptr;


  /* Force a refresh of the current edit line, if there is one.  We avoid refreshing the icon where the mouse
   * was clicked.
   */

  refresh_transaction_edit_line_icons (NULL, -1, pointer->i);

  if (pointer->buttons == wimp_CLICK_SELECT)
  {
    if (pointer->i == wimp_ICON_WINDOW)
    {
      window.w = pointer->w;
      wimp_get_window_state (&window);

      line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - TRANSACT_TOOLBAR_HEIGHT)
             / (ICON_HEIGHT+LINE_GUTTER);

      if (line >= 0)
      {
        place_transaction_edit_line (file, line);

        /* Find the correct point for the caret and insert it. */

        wimp_get_pointer_info (&ptr);
        window.w = ptr.w;
        wimp_get_window_state (&window);

        if (ptr.i == EDIT_ICON_DATE || ptr.i == EDIT_ICON_FROM || ptr.i == EDIT_ICON_TO ||
            ptr.i == EDIT_ICON_REF || ptr.i == EDIT_ICON_AMOUNT || ptr.i == EDIT_ICON_DESCRIPT)
        {
          int xo, yo;

          xo = ptr.pos.x - window.visible.x0 + window.xscroll - 4;
          yo = ptr.pos.y - window.visible.y1 + window.yscroll - 4;
          wimp_set_caret_position (ptr.w, ptr.i, xo, yo, -1, -1);
        }
        else if (ptr.i == EDIT_ICON_FROM_REC || ptr.i == EDIT_ICON_FROM_NAME)
        {
          icons_put_caret_at_end (ptr.w, EDIT_ICON_FROM);
        }
        else if (ptr.i == EDIT_ICON_TO_REC || ptr.i == EDIT_ICON_TO_NAME)
        {
          icons_put_caret_at_end (ptr.w, EDIT_ICON_TO);
        }
      }
    }
    else if (pointer->i == EDIT_ICON_FROM_REC || pointer->i == EDIT_ICON_FROM_NAME)
    {
      icons_put_caret_at_end (pointer->w, EDIT_ICON_FROM);
    }
    else if (pointer->i == EDIT_ICON_TO_REC || pointer->i == EDIT_ICON_TO_NAME)
    {
      icons_put_caret_at_end (pointer->w, EDIT_ICON_TO);
    }
  }

  if (pointer->buttons == wimp_CLICK_ADJUST)
  {
    /* Adjust clicks don't care about icons, as we only need to know which line and column we're in. */

    window.w = pointer->w;
    wimp_get_window_state (&window);

    line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - TRANSACT_TOOLBAR_HEIGHT)
           / (ICON_HEIGHT+LINE_GUTTER);

    if (line >= 0)
    {
      /* If the line was in range, find the column that the click occurred in by scanning through the column
       * positions.
       */

      xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;

      for (column = 0;
           column < TRANSACT_COLUMNS &&
           xpos > (file->transaction_window.column_position[column] + file->transaction_window.column_width[column]);
           column++);

      #ifdef DEBUG
      debug_printf ("Adjust transaction window click (line %d, column %d, xpos %d)", line, column, xpos);
      #endif

      /* The first options can take place on any line in the window... */

      /* If the column is Date, open the date menu. */

      if (column == EDIT_ICON_DATE)
      {
        open_date_menu (file, line, pointer);
      }

      /* If the column is the From name, open the from account menu. */

      else if (column == EDIT_ICON_FROM_NAME)
      {
        open_account_menu (file, ACCOUNT_MENU_FROM, line, NULL, 0, 0, 0, pointer);
      }

      /* If the column is the To name, open the to account menu. */

      else if (column == EDIT_ICON_TO_NAME)
      {
        open_account_menu (file, ACCOUNT_MENU_TO, line, NULL, 0, 0, 0, pointer);
      }

      /* If the column is the Reference, open the to reference menu. */

      else if (column == EDIT_ICON_REF)
      {
        open_refdesc_menu (file, REFDESC_MENU_REFERENCE, line, pointer);
      }

      /* If the column is the Description, open the to description menu. */

      else if (column == EDIT_ICON_DESCRIPT)
      {
        open_refdesc_menu (file, REFDESC_MENU_DESCRIPTION, line, pointer);
      }

      /* ...while the rest have to occur over a transaction line. */

      else if (line < file->trans_count)
      {
        transaction = file->transactions[line].sort_index;

        /* If the column is the from reconcile flag, toggle its status. */

        if (column == EDIT_ICON_FROM_REC)
        {
          toggle_reconcile_flag (file, transaction, TRANS_REC_FROM);
        }

        /* If the column is the to reconcile flag, toggle its status. */

        else if (column == EDIT_ICON_TO_REC)
        {
          toggle_reconcile_flag (file, transaction, TRANS_REC_TO);
        }
      }
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_main_menu (file, pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Deal with clicks in the transaction toolbar. */

void transaction_pane_click (file_data *file, wimp_pointer *pointer)
{
  wimp_window_state     window;
  wimp_icon_state       icon;
  int                   ox;

  extern global_windows windows;


  /* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

  if (pointer->i == TRANSACT_PANE_SORT_DIR_ICON)
  {
    pointer->i = transaction_pane_sort_substitute_icon;
  }

  if (pointer->buttons == wimp_CLICK_SELECT)
  {
    switch (pointer->i)
    {
      case TRANSACT_PANE_SAVE:
        initialise_save_boxes (file, 0, 0);
        fill_save_as_window (file, SAVE_BOX_FILE);
        menus_create_standard_menu ((wimp_menu *) windows.save_as, pointer);
        break;

      case TRANSACT_PANE_PRINT:
        open_transact_print_window (file, pointer, config_opt_read ("RememberValues"));
        break;

      case TRANSACT_PANE_ACCOUNTS:
        create_accounts_window (file, ACCOUNT_FULL);
        break;

      case TRANSACT_PANE_VIEWACCT:
        open_accopen_menu (file, pointer);
        break;

      case TRANSACT_PANE_ADDACCT:
        open_account_edit_window (file, -1, ACCOUNT_FULL, pointer);
        break;

      case TRANSACT_PANE_IN:
        create_accounts_window (file, ACCOUNT_IN);
        break;

      case TRANSACT_PANE_OUT:
        create_accounts_window (file, ACCOUNT_OUT);
        break;

      case TRANSACT_PANE_ADDHEAD:
        open_account_edit_window (file, -1, ACCOUNT_IN, pointer);
        break;

      case TRANSACT_PANE_FIND:
        find_open_window (file, pointer, config_opt_read ("RememberValues"));
        break;

      case TRANSACT_PANE_GOTO:
        goto_open_window (file, pointer, config_opt_read ("RememberValues"));
        break;

      case TRANSACT_PANE_SORT:
        open_transaction_sort_window (file, pointer);
        break;

      case TRANSACT_PANE_RECONCILE:
        file->auto_reconcile = !file->auto_reconcile;
        break;

      case TRANSACT_PANE_SORDER:
        create_sorder_window (file);
        break;

      case TRANSACT_PANE_ADDSORDER:
        open_sorder_edit_window (file, NULL_SORDER, pointer);
        break;

      case TRANSACT_PANE_PRESET:
        preset_open_window (file);
        break;

      case TRANSACT_PANE_ADDPRESET:
        preset_open_edit_window (file, NULL_PRESET, pointer);
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_ADJUST)
  {
    switch (pointer->i)
    {
      case TRANSACT_PANE_SAVE:
        start_direct_menu_save (file);
        break;

      case TRANSACT_PANE_PRINT:
        open_transact_print_window (file, pointer, !config_opt_read ("RememberValues"));
        break;

      case TRANSACT_PANE_FIND:
        find_open_window (file, pointer, !config_opt_read ("RememberValues"));
        break;

      case TRANSACT_PANE_GOTO:
        goto_open_window (file, pointer, !config_opt_read ("RememberValues"));
        break;

      case TRANSACT_PANE_SORT:
        sort_transaction_window (file);
        break;

      case TRANSACT_PANE_RECONCILE:
        file->auto_reconcile = !file->auto_reconcile;
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_main_menu (file, pointer);
  }

  /* Process clicks on the column headings, for sorting the data.  This tests to see if the click was
   * outside of the column size drag hotspot before proceeding.
   */

  else if ((pointer->buttons == wimp_CLICK_SELECT * 256 || pointer->buttons == wimp_CLICK_ADJUST * 256) &&
            pointer->i != wimp_ICON_WINDOW)
  {
    window.w = pointer->w;
    wimp_get_window_state (&window);

    ox = window.visible.x0 - window.xscroll;

    icon.w = pointer->w;
    icon.i = pointer->i;
    wimp_get_icon_state (&icon);

    if (pointer->pos.x < (ox + icon.icon.extent.x1 - COLUMN_DRAG_HOTSPOT))
    {
      file->transaction_window.sort_order = SORT_NONE;

      switch (pointer->i)
      {
        case TRANSACT_PANE_DATE:
          file->transaction_window.sort_order = SORT_DATE;
          break;

        case TRANSACT_PANE_FROM:
          file->transaction_window.sort_order = SORT_FROM;
          break;

        case TRANSACT_PANE_TO:
          file->transaction_window.sort_order = SORT_TO;
          break;

        case TRANSACT_PANE_REFERENCE:
          file->transaction_window.sort_order = SORT_REFERENCE;
          break;

        case TRANSACT_PANE_AMOUNT:
          file->transaction_window.sort_order = SORT_AMOUNT;
          break;

        case TRANSACT_PANE_DESCRIPTION:
          file->transaction_window.sort_order = SORT_DESCRIPTION;
          break;
      }

      if (file->transaction_window.sort_order != SORT_NONE)
      {
        if (pointer->buttons == wimp_CLICK_SELECT * 256)
        {
          file->transaction_window.sort_order |= SORT_ASCENDING;
        }
        else
        {
          file->transaction_window.sort_order |= SORT_DESCENDING;
        }
      }

      adjust_transaction_window_sort_icon (file);
      windows_redraw (file->transaction_window.transaction_pane);
      sort_transaction_window (file);
    }
  }

  else if (pointer->buttons == wimp_DRAG_SELECT && pointer->i <= TRANSACT_PANE_DRAG_LIMIT)
  {
    start_column_width_drag (pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Deal with keypresses in the transaction window.  All hotkeys are handled, then any remaining presses are passed
 * on to the edit line handling code for attention.
 */

void transaction_window_keypress (file_data *file, wimp_key *key)
{
  wimp_pointer pointer;

  extern global_windows windows;


  /* The global clipboard keys. */

  if (key->c == 3) /* Ctrl- C */
  {
    copy_icon_to_clipboard (key);
  }
  else if (key->c == 18) /* Ctrl-R */
  {
    perform_full_recalculation (file);
  }
  else if (key->c == 22) /* Ctrl-V */
  {
    paste_clipboard_to_icon (key);
  }
  else if (key->c == 24) /* Ctrl-X */
  {
    cut_icon_to_clipboard (key);
  }

  /* Other keyboard shortcuts */

  else if (key->c == wimp_KEY_PRINT)
  {
    wimp_get_pointer_info (&pointer);
    open_transact_print_window (file, &pointer, config_opt_read ("RememberValues"));
  }

  else if (key->c == wimp_KEY_CONTROL + wimp_KEY_F1)
  {
    wimp_get_pointer_info (&pointer);
    fill_file_info_window (file);
    menus_create_standard_menu ((wimp_menu *) windows.file_info, &pointer);
  }

  else if (key->c == wimp_KEY_CONTROL + wimp_KEY_F2)
  {
    delete_file (file);
  }

  else if (key->c == wimp_KEY_F3)
  {
    wimp_get_pointer_info (&pointer);
    initialise_save_boxes (file, 0, 0);
    fill_save_as_window (file, SAVE_BOX_FILE);
    menus_create_standard_menu ((wimp_menu *) windows.save_as, &pointer);
  }

  else if (key->c == wimp_KEY_CONTROL + wimp_KEY_F3)
  {
    start_direct_menu_save (file);
  }

  else if (key->c == wimp_KEY_F4)
  {
    wimp_get_pointer_info (&pointer);
    find_open_window (file, &pointer, config_opt_read ("RememberValues"));
  }

  else if (key->c == wimp_KEY_F5)
  {
    wimp_get_pointer_info (&pointer);
    goto_open_window (file, &pointer, config_opt_read ("RememberValues"));
  }

  else if (key->c == wimp_KEY_F6)
  {
    wimp_get_pointer_info (&pointer);
    open_transaction_sort_window (file, &pointer);
  }

  else if (key->c == wimp_KEY_F9)
  {
    create_accounts_window (file, ACCOUNT_FULL);
  }

  else if (key->c == wimp_KEY_F10)
  {
    create_accounts_window (file, ACCOUNT_IN);
  }

  else if (key->c == wimp_KEY_F11)
  {
    create_accounts_window (file, ACCOUNT_OUT);
  }

  else if (key->c == wimp_KEY_PAGE_UP || key->c == wimp_KEY_PAGE_DOWN)
  {
    wimp_scroll scroll;

    /* Make up a Wimp_ScrollRequest block and pass it to the scroll request handler. */

    scroll.w = file->transaction_window.transaction_window;
    wimp_get_window_state ((wimp_window_state *) &scroll);

    scroll.xmin = wimp_SCROLL_NONE;
    scroll.ymin = (key->c == wimp_KEY_PAGE_UP) ? wimp_SCROLL_PAGE_UP : wimp_SCROLL_PAGE_DOWN;

    transaction_window_scroll_event (file, &scroll);
  }

  else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_UP) || key->c == wimp_KEY_HOME)
  {
    scroll_transaction_window_to_end (file, -1);
  }

  else if ((key->c == wimp_KEY_CONTROL + wimp_KEY_DOWN) ||
           (key->c == wimp_KEY_COPY && config_opt_read ("IyonixKeys")))
  {
    scroll_transaction_window_to_end (file, +1);
  }

  /* Pass any keys that are left on to the edit line handler. */

  else
  {
    process_transaction_edit_line_keypress (file, key);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Set the extent of the transaction window to reflect the number of lines displayed. */

void set_transaction_window_extent (file_data *file)
{
    wimp_window_state state;
    os_box            extent;
    int               visible_extent, new_extent, new_scroll;


    /* If the window display length is too small, extend it to one blank line after the data. */

    if (file->transaction_window.display_lines <= file->trans_count)
    {
      file->transaction_window.display_lines = file->trans_count + 1;
    }

    /* Work out the new extent. */

    new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * file->transaction_window.display_lines) - TRANSACT_TOOLBAR_HEIGHT;

    /* Get the current window details, and find the extent of the bottom of the visible area. */

    state.w = file->transaction_window.transaction_window;
    wimp_get_window_state (&state);

    visible_extent = state.yscroll + (state.visible.y0 - state.visible.y1);

    /* If the visible area falls outside the new window extent, then the window needs to be re-opened first. */

    if (new_extent > visible_extent)
    {
      /* Calculate the required new scroll offset.  If this is greater than zero, the current window is too
       * big and will need shrinking down.  Otherwise, just set the new scroll offset.
       */

      new_scroll = new_extent - (state.visible.y0 - state.visible.y1);

      if (new_scroll > 0)
      {
        state.visible.y0 += new_scroll;
        state.yscroll = 0;
      }
      else
      {
        state.yscroll = new_scroll;
      }

      wimp_open_window ((wimp_open *) &state);
    }

    /* Finally, call Wimp_SetExtent to update the extent, safe in the knowledge that the visible area will still
     * exist.
     */

    extent.x0 = 0;
    extent.y1 = 0;
    extent.x1 = file->transaction_window.column_position[TRANSACT_COLUMNS-1] +
                file->transaction_window.column_width[TRANSACT_COLUMNS-1];
    extent.y0 = new_extent;

    wimp_set_extent (file->transaction_window.transaction_window, &extent);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Try to minimise the extent of the transaction window, by removing redundant blank lines as they are scrolled
 * out of sight.
 */

void minimise_transaction_window_extent (file_data *file)
{
  int               height, last_visible_line, minimum_length;
  wimp_window_state window;


  window.w = file->transaction_window.transaction_window;
  wimp_get_window_state (&window);

  /* Calculate the height of the window and the last line that is visible. */

  height = (window.visible.y1 - window.visible.y0) - TRANSACT_TOOLBAR_HEIGHT;
  last_visible_line = (-window.yscroll + height) / (ICON_HEIGHT+LINE_GUTTER);

  /* Work out what the minimum length of the window needs to be, taking into account minimum window size, entries
   * and blank lines and the location of the edit line.
   */

  minimum_length = (file->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
                    file->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;

  if (file->transaction_window.entry_line >= minimum_length)
  {
    minimum_length = file->transaction_window.entry_line + 1;
  }

  if (last_visible_line > minimum_length)
  {
    minimum_length = last_visible_line;
  }

  /* Shrink the window. */

  if (file->transaction_window.display_lines > minimum_length)
  {
    file->transaction_window.display_lines = minimum_length;
    set_transaction_window_extent (file);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void build_transaction_window_title (file_data *file)
{
  make_file_pathname (file, file->transaction_window.window_title, sizeof (file->transaction_window.window_title));

  if (file->modified)
  {
    strcat (file->transaction_window.window_title, " *");
  }

  xwimp_force_redraw_title (file->transaction_window.transaction_window); /* Nested Wimp only! */
}

/* ------------------------------------------------------------------------------------------------------------------ */

void force_transaction_window_redraw (file_data *file, int from, int to)
{
  int              y0, y1;
  wimp_window_info window;

  window.w = file->transaction_window.transaction_window;
  if (xwimp_get_window_info_header_only (&window) == NULL)
  {
    y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - TRANSACT_TOOLBAR_HEIGHT;
    y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - TRANSACT_TOOLBAR_HEIGHT;

    wimp_force_redraw (file->transaction_window.transaction_window, window.extent.x0, y0, window.extent.x1, y1);
  }
  /* **** NB This doesn't redraw the edit line -- the icons need to be refreshed **** */
}

/* ------------------------------------------------------------------------------------------------------------------ */

void update_transaction_window_toolbar (file_data *file)
{
  icons_set_shaded (file->transaction_window.transaction_pane, TRANSACT_PANE_VIEWACCT,
                    count_accounts_in_file (file, ACCOUNT_FULL) == 0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle scroll events that occur in a transaction window */

void transaction_window_scroll_event (file_data *file, wimp_scroll *scroll)
{
  int width, height, line, error;


  /* Add in the X scroll offset. */

  width = scroll->visible.x1 - scroll->visible.x0;

  switch (scroll->xmin)
  {
    case wimp_SCROLL_COLUMN_LEFT:
      scroll->xscroll -= HORIZONTAL_SCROLL;
      break;

    case wimp_SCROLL_COLUMN_RIGHT:
      scroll->xscroll += HORIZONTAL_SCROLL;
      break;

    case wimp_SCROLL_PAGE_LEFT:
      scroll->xscroll -= width;
      break;

    case wimp_SCROLL_PAGE_RIGHT:
      scroll->xscroll += width;
      break;
  }

  /* Add in the Y scroll offset. */

  height = (scroll->visible.y1 - scroll->visible.y0) - TRANSACT_TOOLBAR_HEIGHT;

  switch (scroll->ymin)
  {
    case wimp_SCROLL_LINE_UP:
      scroll->yscroll += (ICON_HEIGHT + LINE_GUTTER);
      if ((error = ((scroll->yscroll) % (ICON_HEIGHT+LINE_GUTTER))))
      {
        scroll->yscroll -= (ICON_HEIGHT+LINE_GUTTER) + error;
      }
      break;

    case wimp_SCROLL_LINE_DOWN:
      scroll->yscroll -= (ICON_HEIGHT + LINE_GUTTER);
      if ((error = ((scroll->yscroll - height) % (ICON_HEIGHT+LINE_GUTTER))))
      {
        scroll->yscroll -= error;
      }

      /* Extend the window if necessary. */

      line = (-scroll->yscroll + height) / (ICON_HEIGHT+LINE_GUTTER);

      if (line > file->transaction_window.display_lines)
      {
        file->transaction_window.display_lines = line;
        set_transaction_window_extent (file);
      }
      break;

    case wimp_SCROLL_PAGE_UP:
      scroll->yscroll += height;
      if ((error = ((scroll->yscroll) % (ICON_HEIGHT+LINE_GUTTER))))
      {
        scroll->yscroll -= (ICON_HEIGHT+LINE_GUTTER) + error;
      }
      break;

    case wimp_SCROLL_PAGE_DOWN:
      scroll->yscroll -= height;
      if ((error = ((scroll->yscroll - height) % (ICON_HEIGHT+LINE_GUTTER))))
      {
        scroll->yscroll -= error;
      }
      break;
  }

  /* Re-open the window, then try and reduce the window extent if possible.
   *
   * It is assumed that the wimp will deal with out-of-bounds offsets for us.
   */

  wimp_open_window ((wimp_open *) scroll);
  minimise_transaction_window_extent (file);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Scroll the transaction window to the top or the end. */

void scroll_transaction_window_to_end (file_data *file, int dir)
{
  wimp_window_info window;


  window.w = file->transaction_window.transaction_window;
  wimp_get_window_info_header_only (&window);

  if (dir > 0)
  {
    window.yscroll = window.extent.y0 - (window.extent.y1 - window.extent.y0);
  }
  else if (dir < 0)
  {
    window.yscroll = window.extent.y1;
  }

  minimise_transaction_window_extent (file);
  wimp_open_window ((wimp_open *) &window);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the transaction number of the transaction in the centre (or nearest the centre) of the transactions
 * window which references the given account.
 *
 * First find the centre line, and see if that matches the account.  If so, return the transaction.  If not, search
 * outwards from that point towards the ends of the window, looking for a match.
 */

int find_transaction_window_centre (file_data *file, int account)
{
  wimp_window_state window;
  int               centre, height, result, i;


  window.w = file->transaction_window.transaction_window;
  wimp_get_window_state (&window);

  /* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
   */

  height = window.visible.y1 - window.visible.y0 - ICON_HEIGHT - LINE_GUTTER - TRANSACT_TOOLBAR_HEIGHT;

  /* Calculate the centre line in the window.  If this is greater than the number of actual tracsactions in the window,
   * reduce it accordingly.
   */

  centre = ((-window.yscroll + ICON_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER)) + ((height / 2) / (ICON_HEIGHT+LINE_GUTTER));

  if (centre >= file->trans_count)
  {
    centre = file->trans_count - 1;
  }

  if (centre > -1)
  {
    if (file->transactions[file->transactions[centre].sort_index].from == account ||
        file->transactions[file->transactions[centre].sort_index].to == account)
    {
      result = file->transactions[centre].sort_index;
    }
    else
    {
      i = 1;

      result = NULL_TRANSACTION;

      while (centre+i < file->trans_count || centre-i >= 0)
      {
        if (centre+i < file->trans_count
            && (file->transactions[file->transactions[centre+i].sort_index].from == account
            || file->transactions[file->transactions[centre+i].sort_index].to == account))
        {
          result = file->transactions[centre+i].sort_index;
          break;
        }

        if (centre-i >= 0
            && (file->transactions[file->transactions[centre-i].sort_index].from == account
            || file->transactions[file->transactions[centre-i].sort_index].to == account))
        {
          result = file->transactions[centre-i].sort_index;
          break;
        }

        i++;
      }
    }
  }
  else
  {
    result = NULL_TRANSACTION;
  }

  return (result);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_transact_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
  int                 column, xpos;
  wimp_window_state   window;
  file_data           *file;

  *buffer = '\0';

  file = find_transaction_window_file_block (w);

  if (file != NULL)
  {
    window.w = w;
    wimp_get_window_state (&window);

    xpos = (pos.x - window.visible.x0) + window.xscroll;

    for (column = 0;
         column < TRANSACT_COLUMNS &&
         xpos > (file->transaction_window.column_position[column] + file->transaction_window.column_width[column]);
         column++);

    sprintf (buffer, "Col%d", column);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find and return the line in the transaction window that points to the specified transaction. */

int locate_transaction_in_transact_window (file_data *file, int transaction)
{
  int        i, line;


  line = -1;

  for (i=0; i < file->trans_count; i++)
  {
    if (file->transactions[i].sort_index == transaction)
    {
      line = i;
      break;
    }
  }

  return (line);
}
