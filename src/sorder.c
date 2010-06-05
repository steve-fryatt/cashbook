/* CashBook - sorder.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/debug.h"
#include "sflib/windows.h"
#include "sflib/string.h"
#include "sflib/msgs.h"
#include "sflib/icons.h"
#include "sflib/config.h"

/* Application header files */

#include "global.h"
#include "sorder.h"

#include "account.h"
#include "accview.h"
#include "caret.h"
#include "conversion.h"
#include "calculation.h"
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

static file_data *edit_sorder_file = NULL;
static file_data *sorder_print_file = NULL;
static file_data *sort_sorder_window_file = NULL;

static int       edit_sorder_no = -1;

static wimp_i sorder_pane_sort_substitute_icon = SORDER_PANE_FROM;

/* ==================================================================================================================
 * Window creation and deletion
 */

/* Create and open a standing order window. */

void create_sorder_window (file_data *file)
{
  int                   i, j, height;
  wimp_window_state     parent;
  os_error              *error;

  extern global_windows windows;


  /* Create or re-open the window. */

  if (file->sorder_window.sorder_window != NULL)
  {
    /* The window is open, so just bring it forward. */

    open_window (file->sorder_window.sorder_window);
  }
  else
  {
    #ifdef DEBUG
    debug_printf ("\\CCreating standing order window");
    #endif

    /* Create the new window data and build the window. */

    *(file->sorder_window.window_title) = '\0';
    windows.sorder_window_def->title_data.indirected_text.text = file->sorder_window.window_title;

    height =  (file->sorder_count > MIN_SORDER_ENTRIES) ? file->sorder_count : MIN_SORDER_ENTRIES;

    parent.w = file->transaction_window.transaction_pane;
    wimp_get_window_state (&parent);

    set_initial_window_area (windows.sorder_window_def,
                             file->sorder_window.column_position[SORDER_COLUMNS-1] +
                             file->sorder_window.column_width[SORDER_COLUMNS-1],
                              ((ICON_HEIGHT+LINE_GUTTER) * height) + SORDER_TOOLBAR_HEIGHT,
                               parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
                               parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

    file->child_x_offset++;
    if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
    {
      file->child_x_offset = 0;
    }

    error = xwimp_create_window (windows.sorder_window_def, &(file->sorder_window.sorder_window));
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
      return;
    }

    /* Create the toolbar. */

    place_window_as_toolbar (windows.sorder_window_def, windows.sorder_pane_def, SORDER_TOOLBAR_HEIGHT-4);

    #ifdef DEBUG
    debug_printf ("Window extents set...");
    #endif

    for (i=0, j=0; j < SORDER_COLUMNS; i++, j++)
    {
      windows.sorder_pane_def->icons[i].extent.x0 = file->sorder_window.column_position[j];

      j = rightmost_group_column (SORDER_PANE_COL_MAP, i);

      windows.sorder_pane_def->icons[i].extent.x1 = file->sorder_window.column_position[j] +
                                                    file->sorder_window.column_width[j] +
                                                    COLUMN_HEADING_MARGIN;
    }

    windows.sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
                             (osspriteop_id) file->sorder_window.sort_sprite;
    windows.sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
                             windows.sorder_pane_def->sprite_area;

    update_sorder_window_sort_icon (file, &(windows.sorder_pane_def->icons[SORDER_PANE_SORT_DIR_ICON]));

    #ifdef DEBUG
    debug_printf ("Toolbar icons adjusted...");
    #endif

    error = xwimp_create_window (windows.sorder_pane_def, &(file->sorder_window.sorder_pane));
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
      return;
    }

    /* Set the title */

    build_sorder_window_title (file);

    /* Open the window. */

    add_ihelp_window (file->sorder_window.sorder_window , "SOrder", decode_sorder_window_help);
    add_ihelp_window (file->sorder_window.sorder_pane , "SOrderTB", NULL);

    open_window (file->sorder_window.sorder_window);
    open_window_nested_as_toolbar (file->sorder_window.sorder_pane,
                                   file->sorder_window.sorder_window,
                                   SORDER_TOOLBAR_HEIGHT-4);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Close and delete the accounts window associated with the file block. */

void delete_sorder_window (file_data *file)
{
  #ifdef DEBUG
  debug_printf ("\\RDeleting standing order window");
  #endif

  if (file->sorder_window.sorder_window != NULL)
  {
    remove_ihelp_window (file->sorder_window.sorder_window);
    remove_ihelp_window (file->sorder_window.sorder_pane);

    wimp_delete_window (file->sorder_window.sorder_window);
    wimp_delete_window (file->sorder_window.sorder_pane);

    file->sorder_window.sorder_window = NULL;
    file->sorder_window.sorder_pane = NULL;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_sorder_window_columns (file_data *file)
{
  int              i, j, new_extent;
  wimp_icon_state  icon;
  wimp_window_info window;


  /* Re-adjust the icons in the pane. */

  for (i=0, j=0; j < SORDER_COLUMNS; i++, j++)
  {
    icon.w = file->sorder_window.sorder_pane;
    icon.i = i;
    wimp_get_icon_state (&icon);

    icon.icon.extent.x0 = file->sorder_window.column_position[j];

    j = rightmost_group_column (SORDER_PANE_COL_MAP, i);

    icon.icon.extent.x1 = file->sorder_window.column_position[j] +
                          file->sorder_window.column_width[j] + COLUMN_HEADING_MARGIN;

    wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                      icon.icon.extent.x1, icon.icon.extent.y1);

    new_extent = file->sorder_window.column_position[SORDER_COLUMNS-1] +
                 file->sorder_window.column_width[SORDER_COLUMNS-1];
  }

  adjust_sorder_window_sort_icon (file);

  /* Replace the edit line to force a redraw and redraw the rest of the window. */

  force_visible_window_redraw (file->sorder_window.sorder_window);
  force_visible_window_redraw (file->sorder_window.sorder_pane);

  /* Set the horizontal extent of the window and pane. */

  window.w = file->sorder_window.sorder_pane;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  window.w = file->sorder_window.sorder_window;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  open_window (window.w);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_sorder_window_sort_icon (file_data *file)
{
  wimp_icon_state icon;

  icon.w = file->sorder_window.sorder_pane;
  icon.i = SORDER_PANE_SORT_DIR_ICON;
  wimp_get_icon_state (&icon);

  update_sorder_window_sort_icon (file, &(icon.icon));

  wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                    icon.icon.extent.x1, icon.icon.extent.y1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void update_sorder_window_sort_icon (file_data *file, wimp_icon *icon)
{
  int  i, width, anchor;


  i = 0;

  if (file->sorder_window.sort_order & SORT_ASCENDING)
  {
    strcpy (file->sorder_window.sort_sprite, "sortarrd");
  }
  else if (file->sorder_window.sort_order & SORT_DESCENDING)
  {
    strcpy (file->sorder_window.sort_sprite, "sortarru");
  }

  switch (file->sorder_window.sort_order & SORT_MASK)
  {
    case SORT_FROM:
      i = 2;
      sorder_pane_sort_substitute_icon = SORDER_PANE_FROM;
      break;

    case SORT_TO:
      i = 5;
      sorder_pane_sort_substitute_icon = SORDER_PANE_TO;
      break;

    case SORT_AMOUNT:
      i = 6;
      sorder_pane_sort_substitute_icon = SORDER_PANE_AMOUNT;
      break;

    case SORT_DESCRIPTION:
      i = 7;
      sorder_pane_sort_substitute_icon = SORDER_PANE_DESCRIPTION;
      break;

    case SORT_NEXTDATE:
      i = 8;
      sorder_pane_sort_substitute_icon = SORDER_PANE_NEXTDATE;
      break;

    case SORT_LEFT:
      i = 9;
      sorder_pane_sort_substitute_icon = SORDER_PANE_LEFT;
      break;
  }

  width = icon->extent.x1 - icon->extent.x0;

  if ((file->sorder_window.sort_order & SORT_MASK) == SORT_AMOUNT ||
      (file->sorder_window.sort_order & SORT_MASK) == SORT_LEFT)
  {
    anchor = file->sorder_window.column_position[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
    icon->extent.x1 = icon->extent.x0 + width;
  }
  else
  {
    anchor = file->sorder_window.column_position[i] +
             file->sorder_window.column_width[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
    icon->extent.x0 = icon->extent.x1 - width;
  }
}

/* ==================================================================================================================
 * Sorting standing orders
 */

void sort_sorder_window (file_data *file)
{
  int         sorted, reorder, gap, comb, temp, order;


  #ifdef DEBUG
  debug_printf("Sorting standing order window");
  #endif

  hourglass_on ();

  /* Sort the entries using a combsort.  This has the advantage over qsort () that the order of entries is only
   * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
   */

  gap = file->sorder_count - 1;

  order = file->sorder_window.sort_order;

  do
  {
    gap = (gap > 1) ? (gap * 10 / 13) : 1;
    if ((file->sorder_count >= 12) && (gap == 9 || gap == 10))
    {
      gap = 11;
    }

    sorted = 1;
    for (comb = 0; (comb + gap) < file->sorder_count; comb++)
    {
      switch (order)
      {
        case SORT_FROM | SORT_ASCENDING:
          reorder = (strcmp(find_account_name(file, file->sorders[file->sorders[comb+gap].sort_index].from),
                     find_account_name(file, file->sorders[file->sorders[comb].sort_index].from)) < 0);
          break;

        case SORT_FROM | SORT_DESCENDING:
          reorder = (strcmp(find_account_name(file, file->sorders[file->sorders[comb+gap].sort_index].from),
                     find_account_name(file, file->sorders[file->sorders[comb].sort_index].from)) > 0);
          break;

        case SORT_TO | SORT_ASCENDING:
          reorder = (strcmp(find_account_name(file, file->sorders[file->sorders[comb+gap].sort_index].to),
                     find_account_name(file, file->sorders[file->sorders[comb].sort_index].to)) < 0);
          break;

        case SORT_TO | SORT_DESCENDING:
          reorder = (strcmp(find_account_name(file, file->sorders[file->sorders[comb+gap].sort_index].to),
                     find_account_name(file, file->sorders[file->sorders[comb].sort_index].to)) > 0);
          break;

        case SORT_AMOUNT | SORT_ASCENDING:
          reorder = (file->sorders[file->sorders[comb+gap].sort_index].normal_amount <
                     file->sorders[file->sorders[comb].sort_index].normal_amount);
          break;

        case SORT_AMOUNT | SORT_DESCENDING:
          reorder = (file->sorders[file->sorders[comb+gap].sort_index].normal_amount >
                     file->sorders[file->sorders[comb].sort_index].normal_amount);
          break;

        case SORT_DESCRIPTION | SORT_ASCENDING:
          reorder = (strcmp(file->sorders[file->sorders[comb+gap].sort_index].description,
                     file->sorders[file->sorders[comb].sort_index].description) < 0);
          break;

        case SORT_DESCRIPTION | SORT_DESCENDING:
          reorder = (strcmp(file->sorders[file->sorders[comb+gap].sort_index].description,
                     file->sorders[file->sorders[comb].sort_index].description) > 0);
          break;

        case SORT_NEXTDATE | SORT_ASCENDING:
          reorder = (file->sorders[file->sorders[comb+gap].sort_index].adjusted_next_date >
                     file->sorders[file->sorders[comb].sort_index].adjusted_next_date);
          break;

        case SORT_NEXTDATE | SORT_DESCENDING:
          reorder = (file->sorders[file->sorders[comb+gap].sort_index].adjusted_next_date <
                     file->sorders[file->sorders[comb].sort_index].adjusted_next_date);
          break;

        case SORT_LEFT | SORT_ASCENDING:
          reorder = (file->sorders[file->sorders[comb+gap].sort_index].left <
                     file->sorders[file->sorders[comb].sort_index].left);
          break;

        case SORT_LEFT | SORT_DESCENDING:
          reorder = (file->sorders[file->sorders[comb+gap].sort_index].left >
                     file->sorders[file->sorders[comb].sort_index].left);
          break;

        default:
          reorder = 0;
          break;
      }

      if (reorder)
      {
        temp = file->sorders[comb+gap].sort_index;
        file->sorders[comb+gap].sort_index = file->sorders[comb].sort_index;
        file->sorders[comb].sort_index = temp;

        sorted = 0;
      }
    }
  }
  while (!sorted || gap != 1);

  force_sorder_window_redraw (file, 0, file->sorder_count - 1);

  hourglass_off ();
}

/* ================================================================================================================== */

void open_sorder_sort_window (file_data *file, wimp_pointer *ptr)
{
  extern global_windows windows;

  /* If the window is open elsewhere, close it first. */

  if (window_is_open (windows.sort_sorder))
  {
    wimp_close_window (windows.sort_sorder);
  }

  fill_sorder_sort_window (file->sorder_window.sort_order);

  sort_sorder_window_file = file;

  open_window_centred_at_pointer (windows.sort_sorder, ptr);
  place_dialogue_caret (windows.sort_sorder, wimp_ICON_WINDOW);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_sorder_sort_window (void)
{
  fill_sorder_sort_window (sort_sorder_window_file->sorder_window.sort_order);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_sorder_sort_window (int sort_option)
{
  extern global_windows windows;

  set_icon_selected (windows.sort_sorder, SORDER_SORT_FROM, (sort_option & SORT_MASK) == SORT_FROM);
  set_icon_selected (windows.sort_sorder, SORDER_SORT_TO, (sort_option & SORT_MASK) == SORT_TO);
  set_icon_selected (windows.sort_sorder, SORDER_SORT_AMOUNT, (sort_option & SORT_MASK) == SORT_AMOUNT);
  set_icon_selected (windows.sort_sorder, SORDER_SORT_DESCRIPTION, (sort_option & SORT_MASK) == SORT_DESCRIPTION);
  set_icon_selected (windows.sort_sorder, SORDER_SORT_NEXTDATE, (sort_option & SORT_MASK) == SORT_NEXTDATE);
  set_icon_selected (windows.sort_sorder, SORDER_SORT_LEFT, (sort_option & SORT_MASK) == SORT_LEFT);

  set_icon_selected (windows.sort_sorder, SORDER_SORT_ASCENDING, sort_option & SORT_ASCENDING);
  set_icon_selected (windows.sort_sorder, SORDER_SORT_DESCENDING, sort_option & SORT_DESCENDING);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_sorder_sort_window (void)
{
  extern global_windows windows;

  sort_sorder_window_file->sorder_window.sort_order = SORT_NONE;

  if (read_icon_selected (windows.sort_sorder, SORDER_SORT_FROM))
  {
    sort_sorder_window_file->sorder_window.sort_order = SORT_FROM;
  }
  else if (read_icon_selected (windows.sort_sorder, SORDER_SORT_TO))
  {
    sort_sorder_window_file->sorder_window.sort_order = SORT_TO;
  }
  else if (read_icon_selected (windows.sort_sorder, SORDER_SORT_AMOUNT))
  {
    sort_sorder_window_file->sorder_window.sort_order = SORT_AMOUNT;
  }
  else if (read_icon_selected (windows.sort_sorder, SORDER_SORT_DESCRIPTION))
  {
    sort_sorder_window_file->sorder_window.sort_order = SORT_DESCRIPTION;
  }
  else if (read_icon_selected (windows.sort_sorder, SORDER_SORT_NEXTDATE))
  {
    sort_sorder_window_file->sorder_window.sort_order = SORT_NEXTDATE;
  }
  else if (read_icon_selected (windows.sort_sorder, SORDER_SORT_LEFT))
  {
    sort_sorder_window_file->sorder_window.sort_order = SORT_LEFT;
  }

  if (sort_sorder_window_file->sorder_window.sort_order != SORT_NONE)
  {
    if (read_icon_selected (windows.sort_sorder, SORDER_SORT_ASCENDING))
    {
      sort_sorder_window_file->sorder_window.sort_order |= SORT_ASCENDING;
    }
    else if (read_icon_selected (windows.sort_sorder, SORDER_SORT_DESCENDING))
    {
      sort_sorder_window_file->sorder_window.sort_order |= SORT_DESCENDING;
    }
  }

  adjust_sorder_window_sort_icon (sort_sorder_window_file);
  force_visible_window_redraw (sort_sorder_window_file->sorder_window.sorder_pane);
  sort_sorder_window (sort_sorder_window_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the sort window if the file disappears. */

void force_close_sorder_sort_window (file_data *file)
{
  extern global_windows windows;


  if (sort_sorder_window_file == file && window_is_open (windows.sort_sorder))
  {
    close_dialogue_with_caret (windows.sort_sorder);
  }
}

/* ==================================================================================================================
 * Adding new standing orders
 */

/* Create a new standing with null details.  Values values are zeroed and left to be set up later. */

int add_sorder (file_data *file)
{
  int new;


  if (flex_extend ((flex_ptr) &(file->sorders), sizeof (sorder) * (file->sorder_count+1)) == 1)
  {
    new = file->sorder_count++;

    file->sorders[new].start_date = NULL_DATE;
    file->sorders[new].raw_next_date = NULL_DATE;
    file->sorders[new].adjusted_next_date = NULL_DATE;

    file->sorders[new].number = 0;
    file->sorders[new].left = 0;
    file->sorders[new].period = 0;
    file->sorders[new].period_unit = 0;

    file->sorders[new].flags = 0;

    file->sorders[new].from = NULL_ACCOUNT;
    file->sorders[new].to = NULL_ACCOUNT;
    file->sorders[new].normal_amount = NULL_CURRENCY;
    file->sorders[new].first_amount = NULL_CURRENCY;
    file->sorders[new].last_amount = NULL_CURRENCY;

    *file->sorders[new].reference = '\0';
    *file->sorders[new].description = '\0';

    file->sorders[new].sort_index = new;

    set_sorder_window_extent (file);
  }
  else
  {
    wimp_msgtrans_error_report ("NoMemNewSO");
    new = NULL_SORDER;
  }

  return (new);
}

/* ================================================================================================================== */

int delete_sorder (file_data *file, int sorder_no)
{
  int i, index;

  /* Find the index entry for the deleted order, and if it doesn't index itself, shuffle all the indexes along
   * so that they remain in the correct places. */

  for (i=0; i<file->sorder_count && file->sorders[i].sort_index != sorder_no; i++);

  if (file->sorders[i].sort_index == sorder_no && i != sorder_no)
  {
    index = i;

    if (index > sorder_no)
    {
      for (i=index; i>sorder_no; i--)
      {
        file->sorders[i].sort_index = file->sorders[i-1].sort_index;
      }
    }
    else
    {
      for (i=index; i<sorder_no; i++)
      {
        file->sorders[i].sort_index = file->sorders[i+1].sort_index;
      }
    }
  }

  /* Delete the order */

  flex_midextend ((flex_ptr) &(file->sorders), (sorder_no + 1) * sizeof (sorder), -sizeof (sorder));
  file->sorder_count--;

  /* Adjust the sort indexes that pointe to entries above the deleted one, by reducing any indexes that are
   * greater than the deleted entry by one.
   */

  for (i=0; i<file->sorder_count; i++)
  {
    if (file->sorders[i].sort_index > sorder_no)
    {
      file->sorders[i].sort_index = file->sorders[i].sort_index - 1;
    }
  }

  /* Update the main standing order display window. */

  set_sorder_window_extent (file);
  if (file->sorder_window.sorder_window != NULL)
  {
    open_window (file->sorder_window.sorder_window);
    if (read_config_opt ("AutoSortSOrders"))
    {
      sort_sorder_window (file);
      force_sorder_window_redraw (file, file->sorder_count, file->sorder_count);
    }
    else
    {
      force_sorder_window_redraw (file, 0, file->sorder_count);
    }
  }
  set_file_data_integrity (file, 1);

  return (0);
}

/* ==================================================================================================================
 * Editing standing orders via GUI.
 */

/* Open the standing order edit window. */

void open_sorder_edit_window (file_data *file, int sorder, wimp_pointer *ptr)
{
  int                   edit_mode;

  extern global_windows windows;


  /* If the window is already open, another standing is being edited or created.  Assume the user wants to lose
   * any unsaved data and just close the window.
   */

  if (window_is_open (windows.edit_sorder))
  {
    wimp_close_window (windows.edit_sorder);
  }

  /* Determine what can be edited.  If the order exists and there are more entries to be added, some bits can not
   * be changed.
   */

  edit_mode = (sorder != NULL_SORDER && file->sorders[sorder].adjusted_next_date != NULL_DATE);

  /* Set the contents of the window up. */

  if (sorder == NULL_SORDER)
  {
    msgs_lookup ("NewSO", indirected_window_title (windows.edit_sorder), 50);
    msgs_lookup ("NewAcctAct", indirected_icon_text (windows.edit_sorder, SORDER_EDIT_OK), 12);
  }
  else
  {
    msgs_lookup ("EditSO", indirected_window_title (windows.edit_sorder), 50);
    msgs_lookup ("EditAcctAct", indirected_icon_text (windows.edit_sorder, SORDER_EDIT_OK), 12);
  }

  fill_sorder_edit_window (file, sorder, edit_mode);

  /* Set the pointers up so we can find this lot again and open the window. */

  edit_sorder_file = file;
  edit_sorder_no = sorder;

  open_window_centred_at_pointer (windows.edit_sorder, ptr);
  place_dialogue_caret (windows.edit_sorder, edit_mode ? SORDER_EDIT_NUMBER : SORDER_EDIT_START);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_sorder_edit_window (void)
{
  extern global_windows windows;

  fill_sorder_edit_window (edit_sorder_file, edit_sorder_no,
                           edit_sorder_no != NULL_SORDER &&
                           edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date != NULL_DATE);
  redraw_icons_in_window (windows.edit_sorder, 14,
                          SORDER_EDIT_START, SORDER_EDIT_NUMBER, SORDER_EDIT_PERIOD,
                          SORDER_EDIT_FMIDENT, SORDER_EDIT_FMREC, SORDER_EDIT_FMNAME,
                          SORDER_EDIT_TOIDENT, SORDER_EDIT_TOREC, SORDER_EDIT_TONAME,
                          SORDER_EDIT_REF, SORDER_EDIT_AMOUNT, SORDER_EDIT_FIRST, SORDER_EDIT_LAST,
                          SORDER_EDIT_DESC);
  replace_caret_in_window (windows.edit_sorder);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_sorder_edit_window (file_data *file, int sorder, int edit_mode)
{
  extern global_windows windows;


  if (sorder == NULL_SORDER)
  {
    /* Set start date. */

    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_START) = '\0';

    /* Set number. */

    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_NUMBER) = '\0';

    /* Set period details. */

    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_PERIOD) = '\0';

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_PERDAYS, 1);
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_PERMONTHS, 0);
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_PERYEARS, 0);

    /* Set the ignore weekends details. */

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_AVOID, 0);

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_SKIPFWD, 1);
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_SKIPBACK, 0);

    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_SKIPFWD, 1);
    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_SKIPBACK, 1);

    /* Fill in the from and to fields. */

    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FMIDENT) = '\0';
    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FMNAME) = '\0';
    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FMREC) = '\0';

    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_TOIDENT) = '\0';
    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_TONAME) = '\0';
    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_TOREC) = '\0';

    /* Fill in the reference field. */

    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_REF) = '\0';

    /* Fill in the amount fields. */

    convert_money_to_string (0, indirected_icon_text (windows.edit_sorder, SORDER_EDIT_AMOUNT));

    convert_money_to_string (0, indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FIRST));
    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_FIRST, 1);
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_FIRSTSW, 0);

    convert_money_to_string (0, indirected_icon_text (windows.edit_sorder, SORDER_EDIT_LAST));
    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_LAST, 1);
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_LASTSW, 0);

    /* Fill in the description field. */

    *indirected_icon_text (windows.edit_sorder, SORDER_EDIT_DESC) = '\0';
  }
  else
  {
    /* Set start date. */

    convert_date_to_string (file->sorders[sorder].start_date,
                            indirected_icon_text (windows.edit_sorder, SORDER_EDIT_START));

    /* Set number. */

    sprintf (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_NUMBER), "%d",
             file->sorders[sorder].number);

    /* Set period details. */

    sprintf (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_PERIOD), "%d",
             file->sorders[sorder].period);

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_PERDAYS,
                       file->sorders[sorder].period_unit == PERIOD_DAYS);
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_PERMONTHS,
                       file->sorders[sorder].period_unit == PERIOD_MONTHS);
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_PERYEARS,
                       file->sorders[sorder].period_unit == PERIOD_YEARS);

    /* Set the ignore weekends details. */

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_AVOID,
                       (file->sorders[sorder].flags & TRANS_SKIP_FORWARD ||
                        file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_SKIPFWD, !(file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));
    set_icon_selected (windows.edit_sorder, SORDER_EDIT_SKIPBACK, (file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_SKIPFWD,
                     !(file->sorders[sorder].flags & TRANS_SKIP_FORWARD ||
                      file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_SKIPBACK,
                     !(file->sorders[sorder].flags & TRANS_SKIP_FORWARD ||
                      file->sorders[sorder].flags & TRANS_SKIP_BACKWARD));

    /* Fill in the from and to fields. */

    fill_account_field(file, file->sorders[sorder].from, file->sorders[sorder].flags & TRANS_REC_FROM,
                       windows.edit_sorder, SORDER_EDIT_FMIDENT, SORDER_EDIT_FMNAME, SORDER_EDIT_FMREC);

    fill_account_field(file, file->sorders[sorder].to, file->sorders[sorder].flags & TRANS_REC_TO,
                       windows.edit_sorder, SORDER_EDIT_TOIDENT, SORDER_EDIT_TONAME, SORDER_EDIT_TOREC);

    /* Fill in the reference field. */

    strcpy (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_REF), file->sorders[sorder].reference);

    /* Fill in the amount fields. */

    convert_money_to_string (file->sorders[sorder].normal_amount,
                             indirected_icon_text (windows.edit_sorder, SORDER_EDIT_AMOUNT));


    convert_money_to_string (file->sorders[sorder].first_amount,
                             indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FIRST));

    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_FIRST,
                     (file->sorders[sorder].first_amount == file->sorders[sorder].normal_amount));

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_FIRSTSW,
                       (file->sorders[sorder].first_amount != file->sorders[sorder].normal_amount));


    convert_money_to_string (file->sorders[sorder].last_amount,
                             indirected_icon_text (windows.edit_sorder, SORDER_EDIT_LAST));

    set_icon_shaded (windows.edit_sorder, SORDER_EDIT_LAST,
                     (file->sorders[sorder].last_amount == file->sorders[sorder].normal_amount));

    set_icon_selected (windows.edit_sorder, SORDER_EDIT_LASTSW,
                       (file->sorders[sorder].last_amount != file->sorders[sorder].normal_amount));

    /* Fill in the description field. */

    strcpy (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_DESC), file->sorders[sorder].description);
  }

  /* Shade icons as required for the edit mode.
   * This assumes that none of the relevant icons get shaded for any other reason...
   */

   set_icon_shaded (windows.edit_sorder, SORDER_EDIT_START, edit_mode);
   set_icon_shaded (windows.edit_sorder, SORDER_EDIT_PERIOD, edit_mode);
   set_icon_shaded (windows.edit_sorder, SORDER_EDIT_PERDAYS, edit_mode);
   set_icon_shaded (windows.edit_sorder, SORDER_EDIT_PERMONTHS, edit_mode);
   set_icon_shaded (windows.edit_sorder, SORDER_EDIT_PERYEARS, edit_mode);

   /* Detele the irrelevant action buttins for a new standing order. */

   set_icon_shaded (windows.edit_sorder, SORDER_EDIT_STOP, !edit_mode && sorder != NULL_SORDER);

   set_icon_deleted (windows.edit_sorder, SORDER_EDIT_STOP, sorder == NULL_SORDER);
   set_icon_deleted (windows.edit_sorder, SORDER_EDIT_DELETE, sorder == NULL_SORDER);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Update the account name fields in the standing order edit window. */

void update_sorder_edit_account_fields (wimp_key *key)
{
  extern global_windows windows;


  if (key->i == SORDER_EDIT_FMIDENT)
  {
    lookup_account_field (edit_sorder_file, key->c, ACCOUNT_IN | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
                          windows.edit_sorder, SORDER_EDIT_FMIDENT, SORDER_EDIT_FMNAME, SORDER_EDIT_FMREC);
  }

  else if (key->i == SORDER_EDIT_TOIDENT)
  {
    lookup_account_field (edit_sorder_file, key->c, ACCOUNT_OUT | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
                          windows.edit_sorder, SORDER_EDIT_TOIDENT, SORDER_EDIT_TONAME, SORDER_EDIT_TOREC);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_sorder_edit_account_menu (wimp_pointer *ptr)
{
  extern global_windows windows;


  if (ptr->i == SORDER_EDIT_FMNAME)
  {
    open_account_menu (edit_sorder_file, ACCOUNT_MENU_FROM, 0,
                          windows.edit_sorder, SORDER_EDIT_FMIDENT, SORDER_EDIT_FMNAME, SORDER_EDIT_FMREC, ptr);
  }

  else if (ptr->i == SORDER_EDIT_TONAME)
  {
    open_account_menu (edit_sorder_file, ACCOUNT_MENU_TO, 0,
                          windows.edit_sorder, SORDER_EDIT_TOIDENT, SORDER_EDIT_TONAME, SORDER_EDIT_TOREC, ptr);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void toggle_sorder_edit_reconcile_fields (wimp_pointer *ptr)
{
  extern global_windows windows;


  if (ptr->i == SORDER_EDIT_FMREC)
  {
    toggle_account_reconcile_icon (windows.edit_sorder, SORDER_EDIT_FMREC);
  }

  else if (ptr->i == SORDER_EDIT_TOREC)
  {
    toggle_account_reconcile_icon (windows.edit_sorder, SORDER_EDIT_TOREC);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated edit standing order window and process the data. */

int process_sorder_edit_window (void)
{
  int                   done, new_period_unit;
  date_t                new_start_date;

  extern global_windows windows;


  /* Extract the start date from the dialogue.  Do this first so that we can test the value;
   * the info isn't stored until later.
   */

  if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_PERDAYS))
  {
    new_period_unit = PERIOD_DAYS;
  }
  else if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_PERMONTHS))
  {
    new_period_unit = PERIOD_MONTHS;
  }
  else if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_PERYEARS))
  {
    new_period_unit = PERIOD_YEARS;
  }
  else
  {
    new_period_unit = PERIOD_NONE;
  }

  /* If the period is months, 31 days are always allowed in the date conversion to allow for the longest months.
   * IF another period is used, the default of the number of days in the givem month is used.
   */

  new_start_date = convert_string_to_date (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_START), NULL_DATE,
                                           ((new_period_unit == PERIOD_MONTHS) ? 31 : 0));

  /* If the standing order doesn't exsit, create it.  If it does exist, validate any data that requires it. */

  if (edit_sorder_no == NULL_SORDER)
  {
    edit_sorder_no = add_sorder (edit_sorder_file);
    edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date = NULL_DATE; /* Set to allow editing. */

    done = 0;
  }
  else
  {
    done = edit_sorder_file->sorders[edit_sorder_no].number - edit_sorder_file->sorders[edit_sorder_no].left;
    if (atoi (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_NUMBER)) < done &&
        edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date != NULL_DATE)
    {
      wimp_msgtrans_error_report ("BadSONumber");
      return (1);
    }

    if (edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date == NULL_DATE &&
        edit_sorder_file->sorders[edit_sorder_no].start_date == new_start_date)
    {
      if (wimp_msgtrans_question_report ("CheckSODate", "CheckSODateB") == 2)
      {
        return (1);
      }
    }
  }

  /* If the standing order was created OK, store the rest of the data. */

  if (edit_sorder_no != NULL_SORDER)
  {
    /* Zero the flags and reset them as required. */

    edit_sorder_file->sorders[edit_sorder_no].flags = 0;

    /* Get the avoid mode. */

    if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_AVOID))
    {
      if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_SKIPFWD))
      {
        edit_sorder_file->sorders[edit_sorder_no].flags |= TRANS_SKIP_FORWARD;
      }
      else if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_SKIPBACK))
      {
        edit_sorder_file->sorders[edit_sorder_no].flags |= TRANS_SKIP_BACKWARD;
      }
    }

    /* If it's a new/finished order, get the start date and period and set up the date fields. */

    if (edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date == NULL_DATE)
    {
      edit_sorder_file->sorders[edit_sorder_no].period_unit = new_period_unit;

      edit_sorder_file->sorders[edit_sorder_no].start_date = new_start_date;

      edit_sorder_file->sorders[edit_sorder_no].raw_next_date =
                 edit_sorder_file->sorders[edit_sorder_no].start_date;

      edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date =
                 get_sorder_date (edit_sorder_file->sorders[edit_sorder_no].raw_next_date,
                                  edit_sorder_file->sorders[edit_sorder_no].flags);

      edit_sorder_file->sorders[edit_sorder_no].period =
                 atoi (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_PERIOD));

      done = 0;

    }

    /* Get the number of transactions. */

    edit_sorder_file->sorders[edit_sorder_no].number =
            atoi (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_NUMBER));

    edit_sorder_file->sorders[edit_sorder_no].left =
            edit_sorder_file->sorders[edit_sorder_no].number - done;

    if (edit_sorder_file->sorders[edit_sorder_no].left == 0)
    {
      edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date = NULL_DATE;
    }

    /* Get the from and to fields. */

    edit_sorder_file->sorders[edit_sorder_no].from =
          find_account (edit_sorder_file,
                        indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FMIDENT), ACCOUNT_FULL | ACCOUNT_IN);

    edit_sorder_file->sorders[edit_sorder_no].to =
          find_account (edit_sorder_file,
                        indirected_icon_text (windows.edit_sorder, SORDER_EDIT_TOIDENT), ACCOUNT_FULL | ACCOUNT_OUT);

    if (*indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FMREC) != '\0')
    {
      edit_sorder_file->sorders[edit_sorder_no].flags |= TRANS_REC_FROM;
    }

    if (*indirected_icon_text (windows.edit_sorder, SORDER_EDIT_TOREC) != '\0')
    {
      edit_sorder_file->sorders[edit_sorder_no].flags |= TRANS_REC_TO;
    }

    /* Get the amounts. */

    edit_sorder_file->sorders[edit_sorder_no].normal_amount =
          convert_string_to_money (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_AMOUNT));

    if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_FIRSTSW))
    {
      edit_sorder_file->sorders[edit_sorder_no].first_amount =
            convert_string_to_money (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_FIRST));
    }
    else
    {
      edit_sorder_file->sorders[edit_sorder_no].first_amount = edit_sorder_file->sorders[edit_sorder_no].normal_amount;
    }

    if (read_icon_selected (windows.edit_sorder, SORDER_EDIT_LASTSW))
    {
      edit_sorder_file->sorders[edit_sorder_no].last_amount =
            convert_string_to_money (indirected_icon_text (windows.edit_sorder, SORDER_EDIT_LAST));
    }
    else
    {
      edit_sorder_file->sorders[edit_sorder_no].last_amount = edit_sorder_file->sorders[edit_sorder_no].normal_amount;
    }

    /* Store the reference. */

    strcpy (edit_sorder_file->sorders[edit_sorder_no].reference,
            indirected_icon_text (windows.edit_sorder, SORDER_EDIT_REF));

    /* Store the description. */

    strcpy (edit_sorder_file->sorders[edit_sorder_no].description,
            indirected_icon_text (windows.edit_sorder, SORDER_EDIT_DESC));
  }

  if (read_config_opt ("AutoSortSOrders"))
  {
    sort_sorder_window (edit_sorder_file);
  }
  else
  {
    force_sorder_window_redraw (edit_sorder_file, edit_sorder_no, edit_sorder_no);
  }
  set_file_data_integrity (edit_sorder_file, 1);
  process_standing_orders (edit_sorder_file);
  perform_full_recalculation (edit_sorder_file);
  set_transaction_window_extent (edit_sorder_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Stop a standing order here and now.  Set the next dates to NULL and zero the left count. */

int stop_sorder_from_edit_window (void)
{
  if (wimp_msgtrans_question_report ("StopSOrder", "StopSOrderB") == 2)
  {
    return (1);
  }

  /* Stop the order */

  edit_sorder_file->sorders[edit_sorder_no].raw_next_date = NULL_DATE;
  edit_sorder_file->sorders[edit_sorder_no].adjusted_next_date = NULL_DATE;
  edit_sorder_file->sorders[edit_sorder_no].left = 0;

  /* Redraw the standing order edit window's contents. */

  refresh_sorder_edit_window ();

  /* Update the main standing order display window. */

  if (read_config_opt ("AutoSortSOrders"))
  {
    sort_sorder_window (edit_sorder_file);
  }
  else
  {
    force_sorder_window_redraw (edit_sorder_file, edit_sorder_no, edit_sorder_no);
  }
  set_file_data_integrity (edit_sorder_file, 1);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Delete a standing order here and now. */

int delete_sorder_from_edit_window (void)
{
  if (wimp_msgtrans_question_report ("DeleteSOrder", "DeleteSOrderB") == 2)
  {
    return (1);
  }

  return (delete_sorder (edit_sorder_file, edit_sorder_no));
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the standing order edit window if the file disappears. */

void force_close_sorder_edit_window (file_data *file)
{
  extern global_windows windows;


  if (edit_sorder_file == file && window_is_open (windows.edit_sorder))
  {
    close_dialogue_with_caret (windows.edit_sorder);
  }
}

/* ==================================================================================================================
 * Printing standing orders via the GUI.
 */

void open_sorder_print_window (file_data *file, wimp_pointer *ptr, int clear)
{
  /* Set the pointers up so we can find this lot again and open the window. */

  sorder_print_file = file;

  open_simple_print_window (file, ptr, clear, "PrintSOrder", print_sorder_window);
}

/* ==================================================================================================================
 * Standing order processing
 */

/* Called to add any outstanding standing orders to a file. */

void process_standing_orders (file_data *file)
{
  unsigned int today;
  int          order, changed, amount;
  char         ref[REF_FIELD_LEN], desc[DESCRIPT_FIELD_LEN];


  #ifdef DEBUG
  debug_printf ("\\YStanding Order processing");
  #endif

  today = get_current_date ();
  changed = 0;

  for (order=0; order<file->sorder_count; order++)
  {
    #ifdef DEBUG
    debug_printf ("Processing order %d...", order);
    #endif

    /* While the next date for the standing order is today or before today, process it. */

    while (file->sorders[order].adjusted_next_date!= NULL_DATE && file->sorders[order].adjusted_next_date <= today)
    {
      /* Action the standing order. */

      if (file->sorders[order].left == file->sorders[order].number)
      {
        amount = file->sorders[order].first_amount;
      }
      else if (file->sorders[order].left == 1)
      {
        amount = file->sorders[order].last_amount;
      }
      else
      {
        amount = file->sorders[order].normal_amount;
      }

      /* Reference and description are copied out of the block first as pointers are passed in to add_raw_transaction ()
       * and the act of adding the transaction will move the flex block and invalidate those pointers before they
       * get used.
       */

      strcpy (ref, file->sorders[order].reference);
      strcpy (desc, file->sorders[order].description);

      add_raw_transaction (file, file->sorders[order].adjusted_next_date,
                           file->sorders[order].from, file->sorders[order].to,
                           file->sorders[order].flags & (TRANS_REC_FROM | TRANS_REC_TO), amount, ref, desc);

      #ifdef DEBUG
      debug_printf ("Adding SO, ref '%s', desc '%s'...",
                    file->sorders[order].reference, file->sorders[order].description);
      #endif

      changed = 1;

      /* Decrement the outstanding orders. */

      file->sorders[order].left--;

      /* If there are outstanding orders to carry out, work out the next date and remember that. */

      if (file->sorders[order].left > 0)
      {
        file->sorders[order].raw_next_date = add_to_date (file->sorders[order].raw_next_date,
                                                          file->sorders[order].period_unit,
                                                          file->sorders[order].period);

        file->sorders[order].adjusted_next_date = get_sorder_date (file->sorders[order].raw_next_date,
                                                                   file->sorders[order].flags);
      }
      else
      {
        file->sorders[order].adjusted_next_date = NULL_DATE;
      }

      force_sorder_window_redraw (file, order, order);
    }
  }

  /* Update the trial values for the file. */

  trial_standing_orders (file);

  /* Refresh things if they have changed. */

  if (changed)
  {
    set_file_data_integrity (file, 1);
    file->sort_valid = 0;

    if (read_config_opt ("SortAfterSOrders"))
    {
      sort_transaction_window (file);
    }
    else
    {
      force_transaction_window_redraw (file, 0, file->trans_count - 1);
      refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, -1, -1);
    }

    if (read_config_opt ("AutoSortSOrders"))
    {
      sort_sorder_window (file);
    }

    rebuild_all_account_views (file);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called to update the standing order trial valus for a file. */

void trial_standing_orders (file_data *file)
{
  unsigned int trial_date, raw_next_date, adjusted_next_date;
  int          i, order, amount, left;


  #ifdef DEBUG
  debug_printf ("\\YStanding Order trialling");
  #endif

  /* Find the cutoff date for the trial. */

  trial_date = add_to_date (get_current_date (), PERIOD_DAYS, file->budget.sorder_trial);

  /* Zero the order trial values. */

  for (i=0; i<file->account_count; i++)
  {
    file->accounts[i].sorder_trial = 0;
  }

  /* Process the standing orders. */

  for (order=0; order<file->sorder_count; order++)
  {
    #ifdef DEBUG
    debug_printf ("Trialling order %d...", order);
    #endif

    /* While the next date for the standing order is today or before today, process it. */

    raw_next_date = file->sorders[order].raw_next_date;
    adjusted_next_date = file->sorders[order].adjusted_next_date;
    left = file->sorders[order].left;

    while (adjusted_next_date!= NULL_DATE && adjusted_next_date <= trial_date)
    {
      /* Action the standing order. */

      if (left == file->sorders[order].number)
      {
        amount = file->sorders[order].first_amount;
      }
      else if (file->sorders[order].left == 1)
      {
        amount = file->sorders[order].last_amount;
      }
      else
      {
        amount = file->sorders[order].normal_amount;
      }


      #ifdef DEBUG
      debug_printf ("Adding trial SO, ref '%s', desc '%s'...",
                    file->sorders[order].reference, file->sorders[order].description);
      #endif

      if (file->sorders[order].from != NULL_ACCOUNT)
      {
        file->accounts[file->sorders[order].from].sorder_trial -= amount;
      }

      if (file->sorders[order].to != NULL_ACCOUNT)
      {
        file->accounts[file->sorders[order].to].sorder_trial += amount;
      }

      /* Decrement the outstanding orders. */

      left--;

      /* If there are outstanding orders to carry out, work out the next date and remember that. */

      if (left > 0)
      {
        raw_next_date = add_to_date (raw_next_date, file->sorders[order].period_unit, file->sorders[order].period);

        adjusted_next_date = get_sorder_date (raw_next_date, file->sorders[order].flags);
      }
      else
      {
        adjusted_next_date = NULL_DATE;
      }
    }
  }
}

/* ==================================================================================================================
 * File and print output
 */

/* Print the standing order window by sending the data to a report. */

void print_sorder_window (int text, int format, int scale, int rotate)
{
  report_data *report;
  int            i, t;
  char           line[1024], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];
  sorder_window  *window;

  msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);
  msgs_lookup ("PrintTitleSOrder", buffer, sizeof (buffer));
  report = open_new_report (sorder_print_file, buffer, NULL);


  if (report != NULL)
  {
    hourglass_on ();

    window = &(sorder_print_file->sorder_window);

    /* Output the page title. */

    make_file_leafname (sorder_print_file, numbuf1, sizeof (numbuf1));
    msgs_param_lookup ("SOrderTitle", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
    sprintf (line, "\\b\\u%s", buffer);
    write_report_line (report, 0, line);
    write_report_line (report, 0, "");

    /* Output the headings line, taking the text from the window icons. */

    *line = '\0';
    sprintf (buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icon_text (window->sorder_pane, 0, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icon_text (window->sorder_pane, 1, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->sorder_pane, 2, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->sorder_pane, 3, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->sorder_pane, 4, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s", icon_text (window->sorder_pane, 5, numbuf1));
    strcat (line, buffer);

    write_report_line (report, 0, line);

    /* Output the standing order data as a set of delimited lines. */

    for (i=0; i < sorder_print_file->sorder_count; i++)
    {
      t = sorder_print_file->sorders[i].sort_index;

      *line = '\0';

      sprintf (buffer, "%s\\t", find_account_ident (sorder_print_file, sorder_print_file->sorders[t].from));
      strcat (line, buffer);

      strcpy (numbuf1, (sorder_print_file->sorders[t].flags & TRANS_REC_FROM) ? rec_char : "");
      sprintf (buffer, "%s\\t", numbuf1);
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", find_account_name (sorder_print_file, sorder_print_file->sorders[t].from));
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", find_account_ident (sorder_print_file, sorder_print_file->sorders[t].to));
      strcat (line, buffer);

      strcpy (numbuf1, (sorder_print_file->sorders[t].flags & TRANS_REC_TO) ? rec_char : "");
      sprintf (buffer, "%s\\t", numbuf1);
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", find_account_name (sorder_print_file, sorder_print_file->sorders[t].to));
      strcat (line, buffer);

      convert_money_to_string (sorder_print_file->sorders[t].normal_amount, numbuf1);
      sprintf (buffer, "\\r%s\\t", numbuf1);
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", sorder_print_file->sorders[t].description);
      strcat (line, buffer);

      if (sorder_print_file->sorders[t].adjusted_next_date != NULL_DATE)
      {
        convert_date_to_string (sorder_print_file->sorders[t].adjusted_next_date, numbuf1);
      }
      else
      {
        msgs_lookup ("SOrderStopped", numbuf1, sizeof (numbuf1));
      }
      sprintf (buffer, "%s\\t", numbuf1);
      strcat (line, buffer);

      sprintf (buffer, "\\r%d", sorder_print_file->sorders[t].left);
      strcat (line, buffer);

      write_report_line (report, 0, line);
    }

    hourglass_off ();
  }
  else
  {
    wimp_msgtrans_error_report ("PrintMemFail");
  }

  close_and_print_report (sorder_print_file, report, text, format, scale, rotate);
}

/* ==================================================================================================================
 * Report generation
 */

void generate_full_sorder_report (file_data *file)
{
  report_data    *report;
  int            i;
  char           line[1024], buffer[256], numbuf1[256], numbuf2[32], numbuf3[32];

  msgs_lookup ("SORWinT", buffer, sizeof (buffer));
  report = open_new_report (file, buffer, NULL);

  if (report != NULL)
  {
    hourglass_on ();

    make_file_leafname (file, numbuf1, sizeof (numbuf1));
    msgs_param_lookup ("SORTitle", line, sizeof (line), numbuf1, NULL, NULL, NULL);
    write_report_line (report, 0, line);

    convert_date_to_string (get_current_date (), numbuf1);
    msgs_param_lookup ("SORHeader", line, sizeof (line), numbuf1, NULL, NULL, NULL);
    write_report_line (report, 0, line);

    sprintf (numbuf1, "%d", file->sorder_count);
    msgs_param_lookup ("SORCount", line, sizeof (line), numbuf1, NULL, NULL, NULL);
    write_report_line (report, 0, line);

    /* Output the data for each of the standing orders in turn. */

    for (i=0; i < file->sorder_count; i++)
    {
      write_report_line (report, 0, ""); /* Separate each entry with a blank line. */

      sprintf (numbuf1, "%d", i+1);
      msgs_param_lookup ("SORNumber", line, sizeof (line), numbuf1, NULL, NULL, NULL);
      write_report_line (report, 0, line);

      msgs_param_lookup ("SORFrom", line, sizeof (line), find_account_name (file, file->sorders[i].from),
                         NULL, NULL, NULL);
      write_report_line (report, 0, line);

      msgs_param_lookup ("SORTo", line, sizeof (line), find_account_name (file, file->sorders[i].to),
                         NULL, NULL, NULL);
      write_report_line (report, 0, line);

      msgs_param_lookup ("SORRef", line, sizeof (line), file->sorders[i].reference, NULL, NULL, NULL);
      write_report_line (report, 0, line);

      convert_money_to_string (file->sorders[i].normal_amount, numbuf1);
      msgs_param_lookup ("SORAmount", line, sizeof (line), numbuf1, NULL, NULL, NULL);
      write_report_line (report, 0, line);

      if (file->sorders[i].normal_amount != file->sorders[i].first_amount)
      {
        convert_money_to_string (file->sorders[i].first_amount, numbuf1);
        msgs_param_lookup ("SORFirst", line, sizeof (line), numbuf1, NULL, NULL, NULL);
        write_report_line (report, 0, line);
      }

      if (file->sorders[i].normal_amount != file->sorders[i].last_amount)
      {
        convert_money_to_string (file->sorders[i].last_amount, numbuf1);
        msgs_param_lookup ("SORFirst", line, sizeof (line), numbuf1, NULL, NULL, NULL);
        write_report_line (report, 0, line);
      }

      msgs_param_lookup ("SORDesc", line, sizeof (line), file->sorders[i].description, NULL, NULL, NULL);
      write_report_line (report, 0, line);

      sprintf (numbuf1, "%d", file->sorders[i].number);
      sprintf (numbuf2, "%d", file->sorders[i].number - file->sorders[i].left);
      sprintf (numbuf3, "%d", file->sorders[i].left);
      msgs_param_lookup ("SORCounts", line, sizeof (line), numbuf1, numbuf2, numbuf3, NULL);
      write_report_line (report, 0, line);

      convert_date_to_string (file->sorders[i].start_date, numbuf1);
      msgs_param_lookup ("SORStart", line, sizeof (line), numbuf1, NULL, NULL, NULL);
      write_report_line (report, 0, line);

      sprintf (numbuf1, "%d", file->sorders[i].period);
      *numbuf2 = '\0';
      switch (file->sorders[i].period_unit)
      {
        case PERIOD_DAYS:
          msgs_lookup ("SOrderDays", numbuf2, sizeof (numbuf2));
          break;

        case PERIOD_MONTHS:
          msgs_lookup ("SOrderMonths", numbuf2, sizeof (numbuf2));
          break;

        case PERIOD_YEARS:
          msgs_lookup ("SOrderYears", numbuf2, sizeof (numbuf2));
          break;
      }
      msgs_param_lookup ("SOREvery", line, sizeof (line), numbuf1, numbuf2, NULL, NULL);
      write_report_line (report, 0, line);

      if (file->sorders[i].flags & TRANS_SKIP_FORWARD)
      {
        msgs_lookup ("SORAvoidFwd", line, sizeof (line));
        write_report_line (report, 0, line);
      }
      else if (file->sorders[i].flags & TRANS_SKIP_BACKWARD)
      {
        msgs_lookup ("SORAvoidBack", line, sizeof (line));
        write_report_line (report, 0, line);
      }

      if (file->sorders[i].adjusted_next_date != NULL_DATE)
      {
        convert_date_to_string (file->sorders[i].adjusted_next_date, numbuf1);
      }
      else
      {
        msgs_lookup ("SOrderStopped", numbuf1, sizeof (numbuf1));
      }
      msgs_param_lookup ("SORNext", line, sizeof (line), numbuf1, NULL, NULL, NULL);
      write_report_line (report, 0, line);
    }

    /* Close the report. */

    close_report (file, report);

    hourglass_off ();
  }
}

/* ==================================================================================================================
 * Standing order window handling
 */

void sorder_window_click (file_data *file, wimp_pointer *pointer)
{
  int               line;
  wimp_window_state window;


  /* Find the window type and get the line clicked on. */

  window.w = pointer->w;
  wimp_get_window_state (&window);

  line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - SORDER_TOOLBAR_HEIGHT)
         / (ICON_HEIGHT+LINE_GUTTER);

  if (line < 0 || line >= file->sorder_count)
  {
    line = -1;
  }

  /* Handle double-clicks, which will open an edit accout window. */

  if (pointer->buttons == wimp_DOUBLE_SELECT)
  {
    if (line != -1)
    {
      open_sorder_edit_window (file, file->sorders[line].sort_index, pointer);
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_sorder_menu (file, line, pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void sorder_pane_click (file_data *file, wimp_pointer *pointer)
{
  wimp_window_state     window;
  wimp_icon_state       icon;
  int                   ox;

  extern global_windows windows;


  /* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

  if (pointer->i == SORDER_PANE_SORT_DIR_ICON)
  {
    pointer->i = sorder_pane_sort_substitute_icon;
  }

  if (pointer->buttons == wimp_CLICK_SELECT)
  {
    switch (pointer->i)
    {
      case SORDER_PANE_PARENT:
        open_window (file->transaction_window.transaction_window);
        break;

      case SORDER_PANE_PRINT:
        open_sorder_print_window (file, pointer, read_config_opt ("RememberValues"));
        break;

      case SORDER_PANE_ADDSORDER:
        open_sorder_edit_window (file, NULL_SORDER, pointer);
        break;

      case SORDER_PANE_SORT:
        open_sorder_sort_window (file, pointer);
	break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_ADJUST)
  {
    switch (pointer->i)
    {
      case SORDER_PANE_PRINT:
        open_sorder_print_window (file, pointer, !read_config_opt ("RememberValues"));
        break;

      case SORDER_PANE_SORT:
        sort_sorder_window (file);
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_sorder_menu (file, -1, pointer);
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
      file->sorder_window.sort_order = SORT_NONE;

      switch (pointer->i)
      {
        case SORDER_PANE_FROM:
          file->sorder_window.sort_order = SORT_FROM;
          break;

        case SORDER_PANE_TO:
          file->sorder_window.sort_order = SORT_TO;
          break;

        case SORDER_PANE_AMOUNT:
          file->sorder_window.sort_order = SORT_AMOUNT;
          break;

        case SORDER_PANE_DESCRIPTION:
          file->sorder_window.sort_order = SORT_DESCRIPTION;
          break;

        case SORDER_PANE_NEXTDATE:
          file->sorder_window.sort_order = SORT_NEXTDATE;
          break;

        case SORDER_PANE_LEFT:
          file->sorder_window.sort_order = SORT_LEFT;
          break;
      }

      if (file->sorder_window.sort_order != SORT_NONE)
      {
        if (pointer->buttons == wimp_CLICK_SELECT * 256)
        {
          file->sorder_window.sort_order |= SORT_ASCENDING;
        }
        else
        {
          file->sorder_window.sort_order |= SORT_DESCENDING;
        }
      }

      adjust_sorder_window_sort_icon (file);
      force_visible_window_redraw (file->sorder_window.sorder_pane);
      sort_sorder_window (file);
    }
  }

  else if (pointer->buttons == wimp_DRAG_SELECT)
  {
    start_column_width_drag (pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Set the extent of the standing order window for the specified file. */

void set_sorder_window_extent (file_data *file)
{
  wimp_window_state state;
  os_box            extent;
  int               new_lines, visible_extent, new_extent, new_scroll;


  /* Set the extent. */

  if (file->sorder_window.sorder_window != NULL)
  {
    /* Get the number of rows to show in the window, and work out the window extent from this. */

    new_lines = (file->sorder_count > MIN_SORDER_ENTRIES) ? file->sorder_count : MIN_SORDER_ENTRIES;

    new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_lines) - SORDER_TOOLBAR_HEIGHT;

    /* Get the current window details, and find the extent of the bottom of the visible area. */

    state.w = file->sorder_window.sorder_window;
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
    extent.x1 = file->sorder_window.column_position[SORDER_COLUMNS-1] +
                file->sorder_window.column_width[SORDER_COLUMNS-1] + COLUMN_GUTTER;
    extent.y0 = new_extent;

    wimp_set_extent (file->sorder_window.sorder_window, &extent);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called to recreate the title of the accounts window connected to the data block. */

void build_sorder_window_title (file_data *file)
{
  char name[256];


  if (file->sorder_window.sorder_window != NULL)
  {
    make_file_leafname (file, name, sizeof (name));

    msgs_param_lookup ("SOrderTitle", file->sorder_window.window_title,
                       sizeof (file->sorder_window.window_title),
                       name, NULL, NULL, NULL);

    wimp_force_redraw_title (file->sorder_window.sorder_window); /* Nested Wimp only! */
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force a redraw of the standing order window, for the given range of lines. */

void force_sorder_window_redraw (file_data *file, int from, int to)
{
  int              y0, y1;
  wimp_window_info window;


  if (file->sorder_window.sorder_window != NULL)
  {
    window.w = file->sorder_window.sorder_window;
    wimp_get_window_info_header_only (&window);

    y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - SORDER_TOOLBAR_HEIGHT;
    y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - SORDER_TOOLBAR_HEIGHT;

    wimp_force_redraw (file->sorder_window.sorder_window, window.extent.x0, y0, window.extent.x1, y1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle scroll events that occur in a standing window */

void sorder_window_scroll_event (file_data *file, wimp_scroll *scroll)
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

  height = (scroll->visible.y1 - scroll->visible.y0) - SORDER_TOOLBAR_HEIGHT;

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

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_sorder_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
  int                 column, xpos;
  wimp_window_state   window;
  file_data           *file;

  *buffer = '\0';

  file = find_sorder_window_file_block (w);

  if (file != NULL)
  {
    window.w = w;
    wimp_get_window_state (&window);

    xpos = (pos.x - window.visible.x0) + window.xscroll;

    for (column = 0;
         column < SORDER_COLUMNS &&
         xpos > (file->sorder_window.column_position[column] + file->sorder_window.column_width[column]);
         column++);

    sprintf (buffer, "Col%d", column);
  }
}
