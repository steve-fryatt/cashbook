/* CashBook - accview.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/windows.h"
#include "sflib/errors.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/string.h"
#include "sflib/msgs.h"
#include "sflib/icons.h"

/* Application header files */

#include "global.h"
#include "accview.h"

#include "account.h"
#include "conversion.h"
#include "caret.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "report.h"
#include "transact.h"
#include "window.h"

/* ==================================================================================================================
 * Global variables.
 */

/* Print dialogue */

file_data     *sort_accview_window_file = NULL;
int           sort_accview_window_account = NULL_ACCOUNT;

file_data     *accview_print_file = NULL;
int           accview_print_account = NULL_ACCOUNT;

static wimp_i accview_pane_sort_substitute_icon = ACCVIEW_PANE_DATE;

/* ==================================================================================================================
 * Window creation and deletion
 */

/* Create and open an account view window associated with the file block. */

void create_accview_window (file_data *file, int account)
{
  int               i, j, height;
  os_error          *error;
  wimp_window_state parent;

  accview_window    *new;

  extern            global_windows windows;


  /* Create or re-open the window. */

  if (file->accounts[account].account_view != NULL)
  {
    /* The window is open, so just bring it forward. */

    open_window (file->accounts[account].account_view->accview_window);
  }
  else
  {
    if (!(file->sort_valid))
    {
      sort_transactions (file);
    }

    /* The block pointer is put into the new variable, as the file->accounts[account].account_view pointer may move
     * as a result of the flex heap shifting for heap_alloc ().
     */

    new = (accview_window *) heap_alloc (sizeof (accview_window));
    file->accounts[account].account_view = new;

    if (file->accounts[account].account_view == NULL)
    {
      wimp_msgtrans_info_report ("AccviewMemErr1");
    }
    else
    {
      #ifdef DEBUG
      debug_printf ("\\BCreate Account View for %d", account);
      #endif

      build_account_view (file, account);

      /* Set the main window extent and create it. */

      *((file->accounts[account].account_view)->window_title) = '\0';
      windows.accview_window_def->title_data.indirected_text.text =
                 (file->accounts[account].account_view)->window_title;

      for (i=0; i<ACCVIEW_COLUMNS; i++)
      {
        (file->accounts[account].account_view)->column_width[i] = file->accview_column_width[i];
        (file->accounts[account].account_view)->column_position[i] = file->accview_column_position[i];
      }

      height =  ((file->accounts[account].account_view)->display_lines > MIN_ACCVIEW_ENTRIES) ?
                 (file->accounts[account].account_view)->display_lines : MIN_ACCVIEW_ENTRIES;

      (file->accounts[account].account_view)->sort_order = file->accview_sort_order;

      /* Find the position to open the window at. */

      parent.w = file->transaction_window.transaction_pane;
      wimp_get_window_state (&parent);

      set_initial_window_area (windows.accview_window_def,
                               (file->accounts[account].account_view)->column_position[ACCVIEW_COLUMNS-1] +
                               (file->accounts[account].account_view)->column_width[ACCVIEW_COLUMNS-1],
                                ((ICON_HEIGHT+LINE_GUTTER) * height) + ACCVIEW_TOOLBAR_HEIGHT,
                                 parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
                                 parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

      file->child_x_offset++;
      if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
      {
        file->child_x_offset = 0;
      }

      /* Set the scroll offset to show the contents of the transaction window */

      windows.accview_window_def->yscroll = align_accview_with_transact_y_offset (file, account);

      error = xwimp_create_window (windows.accview_window_def,
                                   &((file->accounts[account].account_view)->accview_window));
      if (error != NULL)
      {
        wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
        return;
      }
      #ifdef DEBUG
      debug_printf ("Created window: %d, %d, %d, %d...", windows.accview_window_def->visible.x0,
       windows.accview_window_def->visible.x1, windows.accview_window_def->visible.y0,
        windows.accview_window_def->visible.y1);
      #endif

      /* Create the toolbar pane. */

      place_window_as_toolbar (windows.accview_window_def, windows.accview_pane_def, ACCVIEW_TOOLBAR_HEIGHT-4);

      for (i=0, j=0; j < ACCVIEW_COLUMNS; i++, j++)
      {
        windows.accview_pane_def->icons[i].extent.x0 = (file->accounts[account].account_view)->column_position[j];

        j = rightmost_group_column (ACCVIEW_PANE_COL_MAP, i);

        windows.accview_pane_def->icons[i].extent.x1 = (file->accounts[account].account_view)->column_position[j] +
                                                        (file->accounts[account].account_view)->column_width[j] +
                                                        COLUMN_HEADING_MARGIN;
      }

      windows.accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
                                     (osspriteop_id) (file->accounts[account].account_view)->sort_sprite;
      windows.accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
                                     windows.accview_pane_def->sprite_area;

      update_accview_window_sort_icon (file, account, &(windows.accview_pane_def->icons[ACCVIEW_PANE_SORT_DIR_ICON]));

      error = xwimp_create_window (windows.accview_pane_def, &((file->accounts[account].account_view)->accview_pane));
      if (error != NULL)
      {
        wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
        return;
      }

      /* Set the title */

      build_accview_window_title (file, account);

      /* Sort the window contents. */

      sort_accview_window (file, account);

      /* Open the window. */

      if (file->accounts[account].type == ACCOUNT_FULL)
      {
        add_ihelp_window ((file->accounts[account].account_view)->accview_window , "AccView",
                          decode_accview_window_help);
        add_ihelp_window ((file->accounts[account].account_view)->accview_pane , "AccViewTB", NULL);
      }
      else
      {
        add_ihelp_window ((file->accounts[account].account_view)->accview_window , "HeadView",
                          decode_accview_window_help);
        add_ihelp_window ((file->accounts[account].account_view)->accview_pane , "HeadViewTB", NULL);
      }

      open_window ((file->accounts[account].account_view)->accview_window);

      open_window_nested_as_toolbar ((file->accounts[account].account_view)->accview_pane,
                                     (file->accounts[account].account_view)->accview_window,
                                     ACCVIEW_TOOLBAR_HEIGHT-4);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Close and delete the account view window associated with the file block. */

void delete_accview_window (file_data *file, int account)
{
  #ifdef DEBUG
  debug_printf ("\\RDeleting account view window");
  debug_printf ("Account: %d", account);
  #endif

  if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
  {
    if ((file->accounts[account].account_view)->accview_window != NULL)
    {
      remove_ihelp_window ((file->accounts[account].account_view)->accview_window);
      wimp_delete_window ((file->accounts[account].account_view)->accview_window);
    }

    if (file->accounts[account].account_view->accview_pane != NULL)
    {
      remove_ihelp_window ((file->accounts[account].account_view)->accview_window);
      wimp_delete_window ((file->accounts[account].account_view)->accview_pane);
    }

    flex_free ((flex_ptr) &((file->accounts[account].account_view)->line_data));

    heap_free (file->accounts[account].account_view);
    file->accounts[account].account_view = NULL;
  }
}
/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_accview_window_columns (file_data *file, int account)
{
  int              i, j, new_extent;
  wimp_icon_state  icon;
  wimp_window_info window;


  /* Re-adjust the icons in the pane. */

  for (i=0, j=0; j < ACCVIEW_COLUMNS; i++, j++)
  {
    icon.w = (file->accounts[account].account_view)->accview_pane;
    icon.i = i;
    wimp_get_icon_state (&icon);

    icon.icon.extent.x0 = (file->accounts[account].account_view)->column_position[j];

    j = rightmost_group_column (ACCVIEW_PANE_COL_MAP, i);

    icon.icon.extent.x1 = (file->accounts[account].account_view)->column_position[j] +
                          (file->accounts[account].account_view)->column_width[j] + COLUMN_HEADING_MARGIN;

    wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                      icon.icon.extent.x1, icon.icon.extent.y1);

    new_extent = (file->accounts[account].account_view)->column_position[ACCVIEW_COLUMNS-1] +
                 (file->accounts[account].account_view)->column_width[ACCVIEW_COLUMNS-1];
  }

  adjust_accview_window_sort_icon (file, account);

  /* Replace the edit line to force a redraw and redraw the rest of the window. */

  force_visible_window_redraw ((file->accounts[account].account_view)->accview_window);
  force_visible_window_redraw ((file->accounts[account].account_view)->accview_pane);

  /* Set the horizontal extent of the window and pane. */

  window.w = (file->accounts[account].account_view)->accview_pane;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  window.w = (file->accounts[account].account_view)->accview_window;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  open_window (window.w);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_accview_window_sort_icon (file_data *file, int account)
{
  wimp_icon_state icon;

  icon.w = (file->accounts[account].account_view)->accview_pane;
  icon.i = ACCVIEW_PANE_SORT_DIR_ICON;
  wimp_get_icon_state (&icon);

  update_accview_window_sort_icon (file, account, &(icon.icon));

  wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                    icon.icon.extent.x1, icon.icon.extent.y1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void update_accview_window_sort_icon (file_data *file, int account, wimp_icon *icon)
{
  int              i, width, anchor;


  i = 0;

  if ((file->accounts[account].account_view)->sort_order & SORT_ASCENDING)
  {
    strcpy ((file->accounts[account].account_view)->sort_sprite, "sortarrd");
  }
  else if ((file->accounts[account].account_view)->sort_order & SORT_DESCENDING)
  {
    strcpy ((file->accounts[account].account_view)->sort_sprite, "sortarru");
  }

  switch ((file->accounts[account].account_view)->sort_order & SORT_MASK)
  {
    case SORT_DATE:
      i = 0;
      accview_pane_sort_substitute_icon = ACCVIEW_PANE_DATE;
      break;

    case SORT_FROMTO:
      i = 3;
      accview_pane_sort_substitute_icon = ACCVIEW_PANE_FROMTO;
      break;

    case SORT_REFERENCE:
      i = 4;
      accview_pane_sort_substitute_icon = ACCVIEW_PANE_REFERENCE;
      break;

    case SORT_PAYMENTS:
      i = 5;
      accview_pane_sort_substitute_icon = ACCVIEW_PANE_PAYMENTS;
      break;

    case SORT_RECEIPTS:
      i = 6;
      accview_pane_sort_substitute_icon = ACCVIEW_PANE_RECEIPTS;
      break;

    case SORT_BALANCE:
      i = 7;
      accview_pane_sort_substitute_icon = ACCVIEW_PANE_BALANCE;
      break;

    case SORT_DESCRIPTION:
      i = 8;
      accview_pane_sort_substitute_icon = ACCVIEW_PANE_DESCRIPTION;
      break;
  }

  width = icon->extent.x1 - icon->extent.x0;

  if (((file->accounts[account].account_view)->sort_order & SORT_MASK) == SORT_PAYMENTS ||
      ((file->accounts[account].account_view)->sort_order & SORT_MASK) == SORT_RECEIPTS ||
      ((file->accounts[account].account_view)->sort_order & SORT_MASK) == SORT_BALANCE)
  {
    anchor = (file->accounts[account].account_view)->column_position[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
    icon->extent.x1 = icon->extent.x0 + width;
  }
  else
  {
    anchor = (file->accounts[account].account_view)->column_position[i] +
             (file->accounts[account].account_view)->column_width[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
    icon->extent.x0 = icon->extent.x1 - width;
  }
}

/* ================================================================================================================== */

/* Return the account shown in the given window. */

int find_accview_window_from_handle (file_data *file, wimp_w window)
{
  int i, account;


  /* Find the window block. */

  account = NULL_ACCOUNT;

  for (i=0; i<file->account_count; i++)
  {
    if (file->accounts[i].account_view != NULL &&
        ((file->accounts[i].account_view)->accview_window == window ||
         (file->accounts[i].account_view)->accview_pane == window))
    {
      account = i;
    }
  }

  return (account);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find the line in the given account view window that corresponds to the specified transaction. */

int find_accview_line_from_transaction (file_data *file, int account, int transaction)
{
  int line, index, i;


  line = -1;
  index = -1;

  if (account != NULL_ACCOUNT &&
      file->accounts[account].account_view != NULL &&
      (file->accounts[account].account_view)->line_data != NULL)
  {
    for (i = 0; i < (file->accounts[account].account_view)->display_lines && line == -1; i++)
    {
      if (((file->accounts[account].account_view)->line_data)[i].transaction == transaction)
      {
        line = i;
      }
    }

    if (line != -1)
    {
      for (i = 0; i < (file->accounts[account].account_view)->display_lines && index == -1; i++)
      {
        if (((file->accounts[account].account_view)->line_data)[i].sort_index == line)
        {
          index = i;
        }
      }
    }
  }

  return (index);
}

/* ==================================================================================================================
 * Account View creation
 */

/* Build a redraw list for an account statement view from scratch.  Allocate a flex block big enough to take all the
 * transactions, fill it as required, then shrink it down again.
 */

int build_account_view (file_data *file, int account)
{
  int            i, lines;

  lines = 0;

  if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
  {
    #ifdef DEBUG
    debug_printf ("\\BBuilding account statement view");
    #endif

    if (flex_alloc ((flex_ptr)  &((file->accounts[account].account_view)->line_data),
                    file->trans_count * sizeof (accview_redraw)))
    {
      lines = calculate_account_view (file, account);

      flex_extend ((flex_ptr) &((file->accounts[account].account_view)->line_data),
                   lines * sizeof (accview_redraw));

      for (i=0; i<lines; i++)
      {
        (file->accounts[account].account_view)->line_data[i].sort_index = i;
      }
    }
    else
    {
      wimp_msgtrans_info_report ("AccviewMemErr2");
    }
  }

  return (lines);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Calculate the contents of an account view redraw block: entering transaction references and calculating a running
 * balance for the display.
 *
 * This relies on there being enough space in the block to take a line for every transaction.  If it is called for
 * an existing view, it relies on the number of lines not having changed!
 */

int calculate_account_view (file_data *file, int account)
{
  int lines, i, balance;

  lines = 0;

  if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
  {
    hourglass_on ();

    balance = file->accounts[account].opening_balance;

    for (i=0; i<file->trans_count; i++)
    {
      if (file->transactions[i].from == account || file->transactions[i].to == account)
      {
        ((file->accounts[account].account_view)->line_data)[lines].transaction = i;

        if (file->transactions[i].from == account)
        {
          balance -= file->transactions[i].amount;
        }
        else
        {
          balance += file->transactions[i].amount;
        }

        ((file->accounts[account].account_view)->line_data)[lines].balance = balance;

        lines++;
      }
    }

    (file->accounts[account].account_view)->display_lines = lines;

    hourglass_off ();
  }

  return (lines);
}

/* ================================================================================================================== */

/* Rebuild the account view from scratch.  An account From/To entry has been changed, so all bets are off...
 * Delete the flex block and rebuild it, then resize the window and refresh the whole thing.
 */

void rebuild_account_view (file_data *file, int account)
{
  if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
  {
    #ifdef DEBUG
    debug_printf ("\\BRebuilding account statement view");
    #endif

    if (!(file->sort_valid))
    {
      sort_transactions (file);
    }

    if ((file->accounts[account].account_view)->line_data != NULL)
    {
      flex_free ((flex_ptr) &((file->accounts[account].account_view)->line_data));
    }

    build_account_view (file, account);
    set_accview_window_extent (file, account);
    sort_accview_window (file, account);
    force_visible_window_redraw ((file->accounts[account].account_view)->accview_window);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Recalculate the account view.  An amount entry or date has been changed, so the number of transactions will
 * remain the same.  Just re-fill the existing flex block, then resize the window and refresh the whole thing.
 */

void recalculate_account_view (file_data *file, int account, int transaction)
{
  if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
  {
    #ifdef DEBUG
    debug_printf ("\\BRecalculating account statement view");
    #endif

    if (!(file->sort_valid))
    {
      sort_transactions (file);
    }

    calculate_account_view (file, account);
    force_accview_window_redraw (file, account,
                                 find_accview_line_from_transaction (file, account, transaction),
                                 (file->accounts[account].account_view)->display_lines-1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_account_view (file_data *file, int account, int transaction)
{
  int line;

  line = find_accview_line_from_transaction (file, account, transaction);

  if (line != -1)
  {
    force_accview_window_redraw (file, account, line, line);
  }
}

/* ================================================================================================================== */

/* Re-index the account views.  This can *only* be done after a sort_transactions() as it requires data set up
 * by that call in the transaction block.
 */

void reindex_all_account_views (file_data *file)
{
  int i, j, t;

  #ifdef DEBUG
  debug_printf ("Reindexing account views...");
  #endif

  for (i=0; i<file->account_count; i++)
  {
    if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->line_data != NULL)
    {
      for (j=0; j<(file->accounts[i].account_view)->display_lines; j++)
      {
        t = ((file->accounts[i].account_view)->line_data)[j].transaction;
        ((file->accounts[i].account_view)->line_data)[j].transaction = file->transactions[t].sort_workspace;
      }
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void redraw_all_account_views (file_data *file)
{
  int i;

  for (i=0; i<file->account_count; i++)
  {
    if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->accview_window != NULL)
    {
      force_visible_window_redraw ((file->accounts[i].account_view)->accview_window);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void recalculate_all_account_views (file_data *file)
{
  int i;

  for (i=0; i<file->account_count; i++)
  {
    if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->accview_window != NULL)
    {
      recalculate_account_view (file, i, 0);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void rebuild_all_account_views (file_data *file)
{
  int i;

  for (i=0; i<file->account_count; i++)
  {
    if (file->accounts[i].account_view != NULL && (file->accounts[i].account_view)->accview_window != NULL)
    {
      rebuild_account_view (file, i);
    }
  }
}
/* ==================================================================================================================
 * Sorting AccView Windows
 */

void sort_accview_window (file_data *file, int account)
{
  int            sorted, reorder, gap, comb, temp, order;
  accview_window *window;


  #ifdef DEBUG
  debug_printf("Sorting accview window");
  #endif

  if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
  {
    hourglass_on ();

    /* Find the address of the accview block now, to save indirecting to it every time. */

    window = file->accounts[account].account_view;

    /* Sort the entries using a combsort.  This has the advantage over qsort () that the order of entries is only
     * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
     */

    gap = window->display_lines - 1;

    order = window->sort_order;

    do
    {
      gap = (gap > 1) ? (gap * 10 / 13) : 1;
      if ((window->display_lines >= 12) && (gap == 9 || gap == 10))
      {
        gap = 11;
      }

      sorted = 1;
      for (comb = 0; (comb + gap) < window->display_lines; comb++)
      {
        switch (order)
        {
          case SORT_DATE | SORT_ASCENDING:
            reorder = (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].date <
                       file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].date);
            break;

          case SORT_DATE | SORT_DESCENDING:
            reorder = (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].date >
                       file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].date);
            break;

          case SORT_FROMTO | SORT_ASCENDING:
            reorder = (strcmp (
             find_account_name (file,
           (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
              (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].to) :
              (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from)),
             find_account_name (file,
             (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
              (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].to) :
              (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from))) < 0);
            break;

          case SORT_FROMTO | SORT_DESCENDING:
            reorder = (strcmp (
             find_account_name (file,
           (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
              (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].to) :
              (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from)),
             find_account_name (file,
             (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
              (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].to) :
              (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from))) > 0);
            break;

          case SORT_REFERENCE | SORT_ASCENDING:
            reorder = (strcmp(
                  file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].reference,
                  file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].reference) < 0);
            break;

          case SORT_REFERENCE | SORT_DESCENDING:
            reorder = (strcmp(
                  file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].reference,
                  file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].reference) > 0);
            break;

          case SORT_PAYMENTS | SORT_ASCENDING:
            reorder = (
          ((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
               (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount) : 0) <
              ((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
               (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount) : 0));
            break;

          case SORT_PAYMENTS | SORT_DESCENDING:
            reorder = (
          ((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
               (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount) : 0) >
              ((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
               (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount) : 0));
            break;

          case SORT_RECEIPTS | SORT_ASCENDING:
            reorder = (
          ((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
               0 : (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount)) <
              ((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
               0 : (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount)));
            break;

          case SORT_RECEIPTS | SORT_DESCENDING:
            reorder = (
          ((file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].from == account) ?
               0 : (file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].amount)) >
              ((file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].from == account) ?
               0 : (file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].amount)));
            break;

          case SORT_BALANCE | SORT_ASCENDING:
            reorder = (window->line_data[window->line_data[comb+gap].sort_index].balance <
                       window->line_data[window->line_data[comb].sort_index].balance);
           break;

          case SORT_BALANCE | SORT_DESCENDING:
            reorder = (window->line_data[window->line_data[comb+gap].sort_index].balance >
                       window->line_data[window->line_data[comb].sort_index].balance);
            break;

          case SORT_DESCRIPTION | SORT_ASCENDING:
            reorder = (strcmp(
                file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].description,
                file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].description) < 0);
            break;

          case SORT_DESCRIPTION | SORT_DESCENDING:
            reorder = (strcmp(
                file->transactions[window->line_data[window->line_data[comb+gap].sort_index].transaction].description,
                file->transactions[window->line_data[window->line_data[comb].sort_index].transaction].description) > 0);
            break;

          default:
            reorder = 0;
            break;
        }

        if (reorder)
        {
          temp = window->line_data[comb+gap].sort_index;
          window->line_data[comb+gap].sort_index = window->line_data[comb].sort_index;
          window->line_data[comb].sort_index = temp;

          sorted = 0;
        }
      }
    }
    while (!sorted || gap != 1);

    force_accview_window_redraw (file, account, 0, window->display_lines - 1);

    hourglass_off ();
  }
}

/* ================================================================================================================== */

void open_accview_sort_window (file_data *file, int account, wimp_pointer *ptr)
{
  extern global_windows windows;

  /* If the window is open elsewhere, close it first. */

  if (window_is_open (windows.sort_accview))
  {
    wimp_close_window (windows.sort_accview);
  }

  fill_accview_sort_window ((file->accounts[account].account_view)->sort_order);

  sort_accview_window_file = file;
  sort_accview_window_account = account;

  open_window_centred_at_pointer (windows.sort_accview, ptr);
  place_dialogue_caret (windows.sort_accview, wimp_ICON_WINDOW);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_accview_sort_window (void)
{
  fill_accview_sort_window ((sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_accview_sort_window (int sort_option)
{
  extern global_windows windows;

  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_DATE, (sort_option & SORT_MASK) == SORT_DATE);
  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_FROMTO, (sort_option & SORT_MASK) == SORT_FROMTO);
  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_REFERENCE, (sort_option & SORT_MASK) == SORT_REFERENCE);
  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_PAYMENTS, (sort_option & SORT_MASK) == SORT_PAYMENTS);
  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_RECEIPTS, (sort_option & SORT_MASK) == SORT_RECEIPTS);
  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_BALANCE, (sort_option & SORT_MASK) == SORT_BALANCE);
  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_DESCRIPTION, (sort_option & SORT_MASK) == SORT_DESCRIPTION);

  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_ASCENDING, sort_option & SORT_ASCENDING);
  set_icon_selected (windows.sort_accview, ACCVIEW_SORT_DESCENDING, sort_option & SORT_DESCENDING);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_accview_sort_window (void)
{
  extern global_windows windows;

  (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_NONE;

  if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_DATE))
  {
    (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_DATE;
  }
  else if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_FROMTO))
  {
    (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_FROMTO;
  }
  else if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_REFERENCE))
  {
    (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_REFERENCE;
  }
  else if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_PAYMENTS))
  {
    (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_PAYMENTS;
  }
  else if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_RECEIPTS))
  {
    (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_RECEIPTS;
  }
  else if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_BALANCE))
  {
    (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_BALANCE;
  }
  else if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_DESCRIPTION))
  {
    (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order = SORT_DESCRIPTION;
  }

  if ((sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order != SORT_NONE)
  {
    if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_ASCENDING))
    {
      (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order |= SORT_ASCENDING;
    }
    else if (read_icon_selected (windows.sort_accview, ACCVIEW_SORT_DESCENDING))
    {
      (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order |= SORT_DESCENDING;
    }
  }

  adjust_accview_window_sort_icon (sort_accview_window_file, sort_accview_window_account);
  force_visible_window_redraw ((sort_accview_window_file->
                                accounts[sort_accview_window_account].account_view)->accview_pane);
  sort_accview_window (sort_accview_window_file, sort_accview_window_account);

  sort_accview_window_file->accview_sort_order =
       (sort_accview_window_file->accounts[sort_accview_window_account].account_view)->sort_order;

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the find window if the file disappears. */

void force_close_accview_sort_window (file_data *file)
{
  extern global_windows windows;


  if (sort_accview_window_file == file && window_is_open (windows.sort_accview))
  {
    close_dialogue_with_caret (windows.sort_accview);
  }
}

/* ==================================================================================================================
 * Printing account views via the GUI
 */

void open_accview_print_window (file_data *file, int account, wimp_pointer *ptr, int clear)
{
  /* Set the pointers up so we can find this lot again and open the window. */

  accview_print_file = file;
  accview_print_account = account;

  open_date_print_window (file, ptr, clear, "PrintAccview", print_accview_window);
}

/* ==================================================================================================================
 * File and print output
 */

void print_accview_window (int text, int format, int scale, int rotate, date_t from, date_t to)
{
  report_data *report;
  int            i, transaction=0;
  char           line[4096], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];
  accview_window *window;

  msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);
  msgs_lookup ("PrintTitleAccview", buffer, sizeof (buffer));
  report = open_new_report (accview_print_file, buffer, NULL);

  if (report != NULL)
  {
    hourglass_on ();

    window = accview_print_file->accounts[accview_print_account].account_view;

    /* Output the page title. */

    make_file_leafname (accview_print_file, numbuf1, sizeof (numbuf1));
    msgs_param_lookup ("AccviewTitle", buffer, sizeof (buffer),
                       accview_print_file->accounts[accview_print_account].name, numbuf1, NULL, NULL);
    sprintf (line, "\\b\\u%s", buffer);
    write_report_line (report, 0, line);
    write_report_line (report, 0, "");

    /* Output the headings line, taking the text from the window icons. */

    *line = '\0';
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->accview_pane, 0, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icon_text (window->accview_pane, 1, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->accview_pane, 2, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->accview_pane, 3, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->accview_pane, 4, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->accview_pane, 5, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->accview_pane, 6, numbuf1));
    strcat (line, buffer);

    write_report_line (report, 0, line);

    /* Output the transaction data as a set of delimited lines. */

    for (i=0; i < window->display_lines; i++)
    {
      transaction = (window->line_data)[(window->line_data)[i].sort_index].transaction;

      if ((from == NULL_DATE || accview_print_file->transactions[transaction].date >= from) &&
          (to == NULL_DATE || accview_print_file->transactions[transaction].date <= to))
      {
        *line = '\0';

        convert_date_to_string (accview_print_file->transactions[transaction].date, numbuf1);
        sprintf (buffer, "%s\\t", numbuf1);
        strcat (line, buffer);

        if (accview_print_file->transactions[transaction].from == accview_print_account)
        {
          sprintf (buffer, "%s\\t",
                   find_account_ident (accview_print_file, accview_print_file->transactions[transaction].to));
          strcat (line, buffer);

          strcpy (numbuf1, (accview_print_file->transactions[transaction].flags & TRANS_REC_FROM) ? rec_char : "");
          sprintf (buffer, "%s\\t", numbuf1);
          strcat (line, buffer);

          sprintf (buffer, "%s\\t",
                   find_account_name (accview_print_file, accview_print_file->transactions[transaction].to));
          strcat (line, buffer);
        }
        else
        {
          sprintf (buffer, "%s\\t",
                   find_account_ident (accview_print_file, accview_print_file->transactions[transaction].from));
          strcat (line, buffer);

          strcpy (numbuf1, (accview_print_file->transactions[transaction].flags & TRANS_REC_TO) ? rec_char : "");
          sprintf (buffer, "%s\\t", numbuf1);
          strcat (line, buffer);

          sprintf (buffer, "%s\\t",
                   find_account_name (accview_print_file, accview_print_file->transactions[transaction].from));
          strcat (line, buffer);
        }

        sprintf (buffer, "%s\\t", accview_print_file->transactions[transaction].reference);
        strcat (line, buffer);

        if (accview_print_file->transactions[transaction].from == accview_print_account)
        {
          convert_money_to_string (accview_print_file->transactions[transaction].amount, numbuf1);
          sprintf (buffer, "\\r%s\\t\\r\\t", numbuf1);
        }
        else
        {
          convert_money_to_string (accview_print_file->transactions[transaction].amount, numbuf1);
          sprintf (buffer, "\\r\\t\\r%s\\t", numbuf1);
        }
        strcat (line, buffer);

        convert_money_to_string (window->line_data[i].balance, numbuf1);
        sprintf (buffer, "\\r%s\\t", numbuf1);
        strcat (line, buffer);

        sprintf (buffer, "%s\\t", accview_print_file->transactions[transaction].description);
        strcat (line, buffer);

        write_report_line (report, 0, line);
      }
    }

    hourglass_off ();
  }
  else
  {
    wimp_msgtrans_error_report ("PrintMemFail");
  }

  close_and_print_report (accview_print_file, report, text, format, scale, rotate);
}

/* ==================================================================================================================
 * Account View window handling
 */

void accview_window_click (file_data *file, wimp_pointer *pointer)
{
  int               line, column, xpos, transaction, account, toggle_flag;
  int               trans_col_from [] = {0,1,1,1,7,8,8,8,9}, trans_col_to [] = {0,4,4,4,7,8,8,8,9}, *trans_col;
  wimp_window_state window;
  accview_window    *accview;


  #ifdef DEBUG
  debug_printf ("Accview window click: %d", pointer->buttons);
  #endif

  /* Find the window's account, and get the line clicked on. */

  account = find_accview_window_from_handle (file, pointer->w);

  accview = file->accounts[account].account_view;

  window.w = pointer->w;
  wimp_get_window_state (&window);

  line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - ACCVIEW_TOOLBAR_HEIGHT)
         / (ICON_HEIGHT+LINE_GUTTER);

  if (line < 0 || line >= accview->display_lines)
  {
    line = -1;
  }

  /* If the line is a transaction, handle mouse clicks over it.  Menu clicks are ignored and dealt with in the
   * else clause.
   */

  if (line != -1 && pointer->buttons != wimp_CLICK_MENU)
  {
    transaction = accview->line_data[accview->line_data[line].sort_index].transaction;

    xpos = (pointer->pos.x - window.visible.x0) + window.xscroll;

    for (column = 0;
         column < ACCVIEW_COLUMNS && xpos > (accview->column_position[column] + accview->column_width[column]);
         column++);

    if (column != ACCVIEW_COLUMN_RECONCILE &&
        (pointer->buttons == wimp_DOUBLE_SELECT || pointer->buttons == wimp_DOUBLE_ADJUST))
    {
      /* Handle double-clicks, which will locate the transaction in the main window.  Clicks in the reconcile
       * column are not used, as these are used to toggle the reconcile flag.
       */

      trans_col =  (file->transactions[transaction].from == account) ? trans_col_to : trans_col_from;

      place_transaction_edit_line (file, locate_transaction_in_transact_window (file, transaction));
      put_caret_at_end (file->transaction_window.transaction_window, trans_col[column]);
      find_transaction_edit_line (file);

      if (pointer->buttons == wimp_DOUBLE_ADJUST)
      {
        open_window (file->transaction_window.transaction_window);
      }
    }

    else if (column == ACCVIEW_COLUMN_RECONCILE && pointer->buttons == wimp_SINGLE_ADJUST)
    {
      /* Handle adjust-clicks in the reconcile column, to toggle the status. */

      toggle_flag = (file->transactions[transaction].from == account) ? TRANS_REC_FROM : TRANS_REC_TO;

      toggle_reconcile_flag (file, transaction, toggle_flag);
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_accview_menu (file, account, line, pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void accview_pane_click (file_data *file, wimp_pointer *pointer)
{
  wimp_window_state     window;
  wimp_icon_state       icon;
  int                   account, ox;

  /* Find the window's account. */

  account = find_accview_window_from_handle (file, pointer->w);

  /* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

  if (pointer->i == ACCVIEW_PANE_SORT_DIR_ICON)
  {
    pointer->i = accview_pane_sort_substitute_icon;
  }

  /* Decode the mouse click. */

  if (pointer->buttons == wimp_CLICK_SELECT)
  {
    switch (pointer->i)
    {
      case ACCVIEW_PANE_PARENT:
        open_window (file->transaction_window.transaction_window);
        break;

      case ACCVIEW_PANE_PRINT:
        open_accview_print_window (file, account, pointer, read_config_opt ("RememberValues"));
        break;

      case ACCVIEW_PANE_EDIT:
        open_account_edit_window (file, account, -1, pointer);
        break;

      case ACCVIEW_PANE_GOTOEDIT:
        align_accview_with_transact (file, account);
        break;

      case ACCVIEW_PANE_SORT:
        open_accview_sort_window (file, account, pointer);
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_ADJUST)
  {
    switch (pointer->i)
    {
      case ACCVIEW_PANE_PRINT:
        open_accview_print_window (file, account, pointer, !read_config_opt ("RememberValues"));
        break;

      case ACCVIEW_PANE_SORT:
        sort_accview_window (file, account);
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_accview_menu (file, account, -1, pointer);
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
      (file->accounts[account].account_view)->sort_order = SORT_NONE;

      switch (pointer->i)
      {
        case ACCVIEW_PANE_DATE:
          (file->accounts[account].account_view)->sort_order = SORT_DATE;
          break;

        case ACCVIEW_PANE_FROMTO:
          (file->accounts[account].account_view)->sort_order = SORT_FROMTO;
          break;

        case ACCVIEW_PANE_REFERENCE:
          (file->accounts[account].account_view)->sort_order = SORT_REFERENCE;
          break;

        case ACCVIEW_PANE_PAYMENTS:
          (file->accounts[account].account_view)->sort_order = SORT_PAYMENTS;
          break;

        case ACCVIEW_PANE_RECEIPTS:
          (file->accounts[account].account_view)->sort_order = SORT_RECEIPTS;
          break;

        case ACCVIEW_PANE_BALANCE:
          (file->accounts[account].account_view)->sort_order = SORT_BALANCE;
          break;

        case ACCVIEW_PANE_DESCRIPTION:
          (file->accounts[account].account_view)->sort_order = SORT_DESCRIPTION;
          break;
      }

      if ((file->accounts[account].account_view)->sort_order != SORT_NONE)
      {
        if (pointer->buttons == wimp_CLICK_SELECT * 256)
        {
          (file->accounts[account].account_view)->sort_order |= SORT_ASCENDING;
        }
        else
        {
          (file->accounts[account].account_view)->sort_order |= SORT_DESCENDING;
        }
      }

      adjust_accview_window_sort_icon (file, account);
      force_visible_window_redraw ((file->accounts[account].account_view)->accview_pane);
      sort_accview_window (file, account);

      file->accview_sort_order = (file->accounts[account].account_view)->sort_order;
    }
  }

  else if (pointer->buttons == wimp_DRAG_SELECT)
  {
    start_column_width_drag (pointer);
  }
}
/* ------------------------------------------------------------------------------------------------------------------ */
/* Set the extent of the account view window for the specified file. */

void set_accview_window_extent (file_data *file, int account)
{
  wimp_window_state state;
  os_box            extent;
  int               new_height, visible_extent, new_extent, new_scroll;


  /* Set the extent. */

  if (account != NULL_ACCOUNT &&
      file->accounts[account].account_view != NULL && (file->accounts[account].account_view)->accview_window != NULL)
  {
    /* Get the number of rows to show in the window, and work out the window extent from this. */

    new_height =  ((file->accounts[account].account_view)->display_lines > MIN_ACCVIEW_ENTRIES) ?
                   (file->accounts[account].account_view)->display_lines : MIN_ACCVIEW_ENTRIES;

    new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_height) - ACCVIEW_TOOLBAR_HEIGHT;

    /* Get the current window details, and find the extent of the bottom of the visible area. */

    state.w = (file->accounts[account].account_view)->accview_window;
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
    extent.x1 = (file->accounts[account].account_view)->column_position[ACCVIEW_COLUMNS-1]
                + (file->accounts[account].account_view)->column_width[ACCVIEW_COLUMNS-1];

    extent.y0 = new_extent;
    extent.y1 = 0;

    wimp_set_extent ((file->accounts[account].account_view)->accview_window, &extent);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called to recreate the title of the account view window connected to the data block. */

void build_accview_window_title (file_data *file, int account)
{
  char name[256];


  if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
  {
    make_file_leafname (file, name, sizeof (name));

    msgs_param_lookup ("AccviewTitle", (file->accounts[account].account_view)->window_title,
                        sizeof ((file->accounts[account].account_view)->window_title),
                        file->accounts[account].name, name, NULL, NULL);

    wimp_force_redraw_title ((file->accounts[account].account_view)->accview_window); /* Nested Wimp only! */
  }
}
/* ------------------------------------------------------------------------------------------------------------------ */

void force_accview_window_redraw (file_data *file, int account, int from, int to)
{
  int              y0, y1;
  wimp_window_info window;


  if (account != NULL_ACCOUNT &&
      file->accounts[account].account_view != NULL && (file->accounts[account].account_view)->accview_window != NULL)
  {
    window.w = (file->accounts[account].account_view)->accview_window;
    wimp_get_window_info_header_only (&window);

    y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - ACCVIEW_TOOLBAR_HEIGHT;
    y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - ACCVIEW_TOOLBAR_HEIGHT;

    wimp_force_redraw ((file->accounts[account].account_view)->accview_window,
                       window.extent.x0, y0, window.extent.x1, y1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle scroll events that occur in an accavount view window */

void accview_window_scroll_event (file_data *file, wimp_scroll *scroll)
{
  int width, height, error;


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

  height = (scroll->visible.y1 - scroll->visible.y0) - ACCVIEW_TOOLBAR_HEIGHT;

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

  /* Re-open the window.
   *
   * It is assumed that the wimp will deal with out-of-bounds offsets for us.
   */

  wimp_open_window ((wimp_open *) scroll);
}

/* ==================================================================================================================
 * Aligning Account View window with Transaction Window.
 */

int align_accview_with_transact_line (file_data *file, int account)
{
  int            centre_transact, line = 0;
  accview_window *window;


  if (account != NULL_ACCOUNT)
  {
    window = file->accounts[account].account_view;

    if (window != NULL)
    {
      centre_transact = find_transaction_window_centre (file, account);
      line = find_accview_line_from_transaction (file, account, centre_transact);

      if (line == -1)
      {
        line = 0;
      }
    }
  }

  return (line);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int align_accview_with_transact_y_offset (file_data *file, int account)
{
  return (-align_accview_with_transact_line (file, account) * (ICON_HEIGHT + LINE_GUTTER));
}

/* ------------------------------------------------------------------------------------------------------------------ */

void align_accview_with_transact (file_data *file, int account)
{
  find_accview_line (file, account, align_accview_with_transact_line (file, account));
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Locate the given line in the account view window. */

void find_accview_line (file_data *file, int account, int line)
{
  wimp_window_state window;
  int               height, top, bottom;


  if (account != NULL_ACCOUNT &&
      file->accounts[account].account_view != NULL && (file->accounts[account].account_view)->accview_window != NULL)
  {
    window.w = (file->accounts[account].account_view)->accview_window;
    wimp_get_window_state (&window);

    /* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
     * This will allow the edit line to be aligned with the top or bottom of the window.
     */

    height = window.visible.y1 - window.visible.y0 - ICON_HEIGHT - LINE_GUTTER - ACCVIEW_TOOLBAR_HEIGHT;

    /* Calculate the top full line and bottom full line that are showing in the window.  Part lines don't
     * count and are discarded.
     */

    top = (-window.yscroll + ICON_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
    bottom = height / (ICON_HEIGHT+LINE_GUTTER) + top;

    /* If the edit line is above or below the visible area, bring it into range. */

    if (line < top)
    {
      window.yscroll = -(line * (ICON_HEIGHT+LINE_GUTTER));
      wimp_open_window ((wimp_open *) &window);
    }

    if (line > bottom)
    {
      window.yscroll = -(line * (ICON_HEIGHT+LINE_GUTTER) - height);
      wimp_open_window ((wimp_open *) &window);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_accview_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
  int                 account, column, xpos;
  wimp_window_state   window;
  file_data           *file;

  *buffer = '\0';

  file = find_accview_window_file_block (w);
  account = find_accview_window_from_handle (file, w);

  if (file != NULL && account != NULL_ACCOUNT)
  {
    window.w = w;
    wimp_get_window_state (&window);

    xpos = (pos.x - window.visible.x0) + window.xscroll;

    for (column = 0;
         column < TRANSACT_COLUMNS &&
         xpos > ((file->accounts[account].account_view)->column_position[column] +
                 (file->accounts[account].account_view)->column_width[column]);
         column++);

    sprintf (buffer, "Col%d", column);
  }
}
