/* CashBook - presets.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "presets.h"

#include "account.h"
#include "caret.h"
#include "conversion.h"
#include "date.h"
#include "file.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "report.h"
#include "window.h"

/* ==================================================================================================================
 * Global variables.
 */

static file_data *edit_preset_file = NULL;
static file_data *preset_print_file = NULL;
static file_data *sort_preset_window_file = NULL;

static int       edit_preset_no = -1;

static wimp_i preset_pane_sort_substitute_icon = PRESET_PANE_FROM;

/* ==================================================================================================================
 * Window creation and deletion
 */

/* Create and open a standing order window. */

void create_preset_window (file_data *file)
{
  int                   i, j, height;
  wimp_window_state     parent;
  os_error              *error;

  extern global_windows windows;


  /* Create or re-open the window. */

  if (file->preset_window.preset_window != NULL)
  {
    /* The window is open, so just bring it forward. */

    open_window (file->preset_window.preset_window);
  }
  else
  {
    #ifdef DEBUG
    debug_printf ("\\CCreating preset window");
    #endif

    /* Create the new window data and build the window. */

    *(file->preset_window.window_title) = '\0';
    windows.preset_window_def->title_data.indirected_text.text = file->preset_window.window_title;

    height =  (file->preset_count > MIN_PRESET_ENTRIES) ? file->preset_count : MIN_PRESET_ENTRIES;

    parent.w = file->transaction_window.transaction_pane;
    wimp_get_window_state (&parent);

    set_initial_window_area (windows.preset_window_def,
                             file->preset_window.column_position[PRESET_COLUMNS-1] +
                             file->preset_window.column_width[PRESET_COLUMNS-1],
                              ((ICON_HEIGHT+LINE_GUTTER) * height) + PRESET_TOOLBAR_HEIGHT,
                               parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
                               parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

    file->child_x_offset++;
    if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
    {
      file->child_x_offset = 0;
    }

    error = xwimp_create_window (windows.preset_window_def, &(file->preset_window.preset_window));
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
      return;
    }

    /* Create the toolbar. */

    place_window_as_toolbar (windows.preset_window_def, windows.preset_pane_def, PRESET_TOOLBAR_HEIGHT-4);

    #ifdef DEBUG
    debug_printf ("Window extents set...");
    #endif

    for (i=0, j=0; j < PRESET_COLUMNS; i++, j++)
    {
      windows.preset_pane_def->icons[i].extent.x0 = file->preset_window.column_position[j];

      j = rightmost_group_column (PRESET_PANE_COL_MAP, i);

      windows.preset_pane_def->icons[i].extent.x1 = file->preset_window.column_position[j] +
                                                    file->preset_window.column_width[j] +
                                                    COLUMN_HEADING_MARGIN;
    }

    windows.preset_pane_def->icons[PRESET_PANE_SORT_DIR_ICON].data.indirected_sprite.id =
                             (osspriteop_id) file->preset_window.sort_sprite;
    windows.preset_pane_def->icons[PRESET_PANE_SORT_DIR_ICON].data.indirected_sprite.area =
                             windows.preset_pane_def->sprite_area;

    update_preset_window_sort_icon (file, &(windows.preset_pane_def->icons[PRESET_PANE_SORT_DIR_ICON]));

    #ifdef DEBUG
    debug_printf ("Toolbar icons adjusted...");
    #endif

    error = xwimp_create_window (windows.preset_pane_def, &(file->preset_window.preset_pane));
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
      return;
    }

    /* Set the title */

    build_preset_window_title (file);

    /* Open the window. */

    add_ihelp_window (file->preset_window.preset_window , "Preset", decode_preset_window_help);
    add_ihelp_window (file->preset_window.preset_pane , "PresetTB", NULL);

    open_window (file->preset_window.preset_window);
    open_window_nested_as_toolbar (file->preset_window.preset_pane,
                                   file->preset_window.preset_window,
                                   PRESET_TOOLBAR_HEIGHT-4);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Close and delete the accounts window associated with the file block. */

void delete_preset_window (file_data *file)
{
  #ifdef DEBUG
  debug_printf ("\\RDeleting preset window");
  #endif

  if (file->preset_window.preset_window != NULL)
  {
    remove_ihelp_window (file->preset_window.preset_window);
    remove_ihelp_window (file->preset_window.preset_pane);

    wimp_delete_window (file->preset_window.preset_window);
    wimp_delete_window (file->preset_window.preset_pane);

    file->preset_window.preset_window = NULL;
    file->preset_window.preset_pane = NULL;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_preset_window_columns (file_data *file)
{
  int              i, j, new_extent;
  wimp_icon_state  icon;
  wimp_window_info window;


  /* Re-adjust the icons in the pane. */

  for (i=0, j=0; j < PRESET_COLUMNS; i++, j++)
  {
    icon.w = file->preset_window.preset_pane;
    icon.i = i;
    wimp_get_icon_state (&icon);

    icon.icon.extent.x0 = file->preset_window.column_position[j];

    j = rightmost_group_column (PRESET_PANE_COL_MAP, i);

    icon.icon.extent.x1 = file->preset_window.column_position[j] +
                          file->preset_window.column_width[j] + COLUMN_HEADING_MARGIN;

    wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                      icon.icon.extent.x1, icon.icon.extent.y1);

    new_extent = file->preset_window.column_position[PRESET_COLUMNS-1] +
                 file->preset_window.column_width[PRESET_COLUMNS-1];
  }

  adjust_preset_window_sort_icon (file);

  /* Replace the edit line to force a redraw and redraw the rest of the window. */

  force_visible_window_redraw (file->preset_window.preset_window);
  force_visible_window_redraw (file->preset_window.preset_pane);

  /* Set the horizontal extent of the window and pane. */

  window.w = file->preset_window.preset_pane;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  window.w = file->preset_window.preset_window;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  open_window (window.w);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_preset_window_sort_icon (file_data *file)
{
  wimp_icon_state icon;

  icon.w = file->preset_window.preset_pane;
  icon.i = PRESET_PANE_SORT_DIR_ICON;
  wimp_get_icon_state (&icon);

  update_preset_window_sort_icon (file, &(icon.icon));

  wimp_resize_icon (icon.w, icon.i, icon.icon.extent.x0, icon.icon.extent.y0,
                                    icon.icon.extent.x1, icon.icon.extent.y1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void update_preset_window_sort_icon (file_data *file, wimp_icon *icon)
{
  int  i, width, anchor;


  i = 0;

  if (file->preset_window.sort_order & SORT_ASCENDING)
  {
    strcpy (file->preset_window.sort_sprite, "sortarrd");
  }
  else if (file->preset_window.sort_order & SORT_DESCENDING)
  {
    strcpy (file->preset_window.sort_sprite, "sortarru");
  }

  switch (file->preset_window.sort_order & SORT_MASK)
  {
    case SORT_CHAR:
      i = 0;
      preset_pane_sort_substitute_icon = PRESET_PANE_KEY;
      break;

    case SORT_NAME:
      i = 1;
      preset_pane_sort_substitute_icon = PRESET_PANE_NAME;
      break;

    case SORT_FROM:
      i = 4;
      preset_pane_sort_substitute_icon = PRESET_PANE_FROM;
      break;

    case SORT_TO:
      i = 7;
      preset_pane_sort_substitute_icon = PRESET_PANE_TO;
      break;

    case SORT_AMOUNT:
      i = 8;
      preset_pane_sort_substitute_icon = PRESET_PANE_AMOUNT;
      break;

    case SORT_DESCRIPTION:
      i = 9;
      preset_pane_sort_substitute_icon = PRESET_PANE_DESCRIPTION;
      break;
  }

  width = icon->extent.x1 - icon->extent.x0;

  if ((file->preset_window.sort_order & SORT_MASK) == SORT_AMOUNT)
  {
    anchor = file->preset_window.column_position[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x0 = anchor + COLUMN_SORT_OFFSET;
    icon->extent.x1 = icon->extent.x0 + width;
  }
  else
  {
    anchor = file->preset_window.column_position[i] +
             file->preset_window.column_width[i] + COLUMN_HEADING_MARGIN;
    icon->extent.x1 = anchor - COLUMN_SORT_OFFSET;
    icon->extent.x0 = icon->extent.x1 - width;
  }
}

/* ==================================================================================================================
 * Sorting presets
 */

void sort_preset_window (file_data *file)
{
  int         sorted, reorder, gap, comb, temp, order;


  #ifdef DEBUG
  debug_printf("Sorting standing order window");
  #endif

  hourglass_on ();

  /* Sort the entries using a combsort.  This has the advantage over qsort () that the order of entries is only
   * affected if they are not equal and are in descending order.  Otherwise, the status quo is left.
   */

  gap = file->preset_count - 1;

  order = file->preset_window.sort_order;

  do
  {
    gap = (gap > 1) ? (gap * 10 / 13) : 1;
    if ((file->preset_count >= 12) && (gap == 9 || gap == 10))
    {
      gap = 11;
    }

    sorted = 1;
    for (comb = 0; (comb + gap) < file->preset_count; comb++)
    {
      switch (order)
      {
        case SORT_CHAR | SORT_ASCENDING:
          reorder = (file->presets[file->presets[comb+gap].sort_index].action_key <
                     file->presets[file->presets[comb].sort_index].action_key);
          break;

        case SORT_CHAR | SORT_DESCENDING:
          reorder = (file->presets[file->presets[comb+gap].sort_index].action_key >
                     file->presets[file->presets[comb].sort_index].action_key);
          break;

        case SORT_NAME | SORT_ASCENDING:
          reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].name,
                     file->presets[file->presets[comb].sort_index].name) < 0);
          break;

        case SORT_NAME | SORT_DESCENDING:
          reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].name,
                     file->presets[file->presets[comb].sort_index].name) > 0);
          break;

        case SORT_FROM | SORT_ASCENDING:
          reorder = (strcmp(find_account_name(file, file->presets[file->presets[comb+gap].sort_index].from),
                     find_account_name(file, file->presets[file->presets[comb].sort_index].from)) < 0);
          break;

        case SORT_FROM | SORT_DESCENDING:
          reorder = (strcmp(find_account_name(file, file->presets[file->presets[comb+gap].sort_index].from),
                     find_account_name(file, file->presets[file->presets[comb].sort_index].from)) > 0);
          break;

        case SORT_TO | SORT_ASCENDING:
          reorder = (strcmp(find_account_name(file, file->presets[file->presets[comb+gap].sort_index].to),
                     find_account_name(file, file->presets[file->presets[comb].sort_index].to)) < 0);
          break;

        case SORT_TO | SORT_DESCENDING:
          reorder = (strcmp(find_account_name(file, file->presets[file->presets[comb+gap].sort_index].to),
                     find_account_name(file, file->presets[file->presets[comb].sort_index].to)) > 0);
          break;

        case SORT_AMOUNT | SORT_ASCENDING:
          reorder = (file->presets[file->presets[comb+gap].sort_index].amount <
                     file->presets[file->presets[comb].sort_index].amount);
          break;

        case SORT_AMOUNT | SORT_DESCENDING:
          reorder = (file->presets[file->presets[comb+gap].sort_index].amount >
                     file->presets[file->presets[comb].sort_index].amount);
          break;

        case SORT_DESCRIPTION | SORT_ASCENDING:
          reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].description,
                     file->presets[file->presets[comb].sort_index].description) < 0);
          break;

        case SORT_DESCRIPTION | SORT_DESCENDING:
          reorder = (strcmp(file->presets[file->presets[comb+gap].sort_index].description,
                     file->presets[file->presets[comb].sort_index].description) > 0);
          break;

        default:
          reorder = 0;
          break;
      }

      if (reorder)
      {
        temp = file->presets[comb+gap].sort_index;
        file->presets[comb+gap].sort_index = file->presets[comb].sort_index;
        file->presets[comb].sort_index = temp;

        sorted = 0;
      }
    }
  }
  while (!sorted || gap != 1);

  force_preset_window_redraw (file, 0, file->preset_count - 1);

  hourglass_off ();
}

/* ================================================================================================================== */

void open_preset_sort_window (file_data *file, wimp_pointer *ptr)
{
  extern global_windows windows;

  /* If the window is open elsewhere, close it first. */

  if (window_is_open (windows.sort_preset))
  {
    wimp_close_window (windows.sort_preset);
  }

  fill_preset_sort_window (file->preset_window.sort_order);

  sort_preset_window_file = file;

  open_window_centred_at_pointer (windows.sort_preset, ptr);
  place_dialogue_caret (windows.sort_preset, wimp_ICON_WINDOW);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_preset_sort_window (void)
{
  fill_preset_sort_window (sort_preset_window_file->preset_window.sort_order);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_preset_sort_window (int sort_option)
{
  extern global_windows windows;

  set_icon_selected (windows.sort_preset, PRESET_SORT_FROM, (sort_option & SORT_MASK) == SORT_FROM);
  set_icon_selected (windows.sort_preset, PRESET_SORT_TO, (sort_option & SORT_MASK) == SORT_TO);
  set_icon_selected (windows.sort_preset, PRESET_SORT_AMOUNT, (sort_option & SORT_MASK) == SORT_AMOUNT);
  set_icon_selected (windows.sort_preset, PRESET_SORT_DESCRIPTION, (sort_option & SORT_MASK) == SORT_DESCRIPTION);
  set_icon_selected (windows.sort_preset, PRESET_SORT_KEY, (sort_option & SORT_MASK) == SORT_CHAR);
  set_icon_selected (windows.sort_preset, PRESET_SORT_NAME, (sort_option & SORT_MASK) == SORT_NAME);

  set_icon_selected (windows.sort_preset, PRESET_SORT_ASCENDING, sort_option & SORT_ASCENDING);
  set_icon_selected (windows.sort_preset, PRESET_SORT_DESCENDING, sort_option & SORT_DESCENDING);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_preset_sort_window (void)
{
  extern global_windows windows;

  sort_preset_window_file->preset_window.sort_order = SORT_NONE;

  if (read_icon_selected (windows.sort_preset, PRESET_SORT_FROM))
  {
    sort_preset_window_file->preset_window.sort_order = SORT_FROM;
  }
  else if (read_icon_selected (windows.sort_preset, PRESET_SORT_TO))
  {
    sort_preset_window_file->preset_window.sort_order = SORT_TO;
  }
  else if (read_icon_selected (windows.sort_preset, PRESET_SORT_AMOUNT))
  {
    sort_preset_window_file->preset_window.sort_order = SORT_AMOUNT;
  }
  else if (read_icon_selected (windows.sort_preset, PRESET_SORT_DESCRIPTION))
  {
    sort_preset_window_file->preset_window.sort_order = SORT_DESCRIPTION;
  }
  else if (read_icon_selected (windows.sort_preset, PRESET_SORT_KEY))
  {
    sort_preset_window_file->preset_window.sort_order = SORT_CHAR;
  }
  else if (read_icon_selected (windows.sort_preset, PRESET_SORT_NAME))
  {
    sort_preset_window_file->preset_window.sort_order = SORT_NAME;
  }

  if (sort_preset_window_file->preset_window.sort_order != SORT_NONE)
  {
    if (read_icon_selected (windows.sort_preset, PRESET_SORT_ASCENDING))
    {
      sort_preset_window_file->preset_window.sort_order |= SORT_ASCENDING;
    }
    else if (read_icon_selected (windows.sort_preset, PRESET_SORT_DESCENDING))
    {
      sort_preset_window_file->preset_window.sort_order |= SORT_DESCENDING;
    }
  }

  adjust_preset_window_sort_icon (sort_preset_window_file);
  force_visible_window_redraw (sort_preset_window_file->preset_window.preset_pane);
  sort_preset_window (sort_preset_window_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the sort window if the file disappears. */

void force_close_preset_sort_window (file_data *file)
{
  extern global_windows windows;


  if (sort_preset_window_file == file && window_is_open (windows.sort_preset))
  {
    close_dialogue_with_caret (windows.sort_preset);
  }
}

/* ==================================================================================================================
 * Adding new presets
 */

/* Create a new preset with null details.  Values values are zeroed and left to be set up later. */

int add_preset (file_data *file)
{
  int new;


  if (flex_extend ((flex_ptr) &(file->presets), sizeof (preset) * (file->preset_count+1)) == 1)
  {
    new = file->preset_count++;

    *file->presets[new].name = '\0';
    file->presets[new].action_key = 0;

    file->presets[new].flags = 0;

    file->presets[new].date = NULL_DATE;
    file->presets[new].from = NULL_ACCOUNT;
    file->presets[new].to = NULL_ACCOUNT;
    file->presets[new].amount = NULL_CURRENCY;

    *file->presets[new].reference = '\0';
    *file->presets[new].description = '\0';

    file->presets[new].sort_index = new;

    set_preset_window_extent (file);
  }
  else
  {
    wimp_msgtrans_error_report ("NoMemNewPreset");
    new = NULL_PRESET;
  }

  return (new);
}

/* ================================================================================================================== */

int delete_preset (file_data *file, int preset_no)
{
  int i, index;

  /* Find the index entry for the deleted preset, and if it doesn't index itself, shuffle all the indexes along
   * so that they remain in the correct places. */

  for (i=0; i<file->preset_count && file->presets[i].sort_index != preset_no; i++);

  if (file->presets[i].sort_index == preset_no && i != preset_no)
  {
    index = i;

    if (index > preset_no)
    {
      for (i=index; i>preset_no; i--)
      {
        file->presets[i].sort_index = file->presets[i-1].sort_index;
      }
    }
    else
    {
      for (i=index; i<preset_no; i++)
      {
        file->presets[i].sort_index = file->presets[i+1].sort_index;
      }
    }
  }

  /* Delete the preset */

  flex_midextend ((flex_ptr) &(file->presets), (preset_no + 1) * sizeof (preset), -sizeof (preset));
  file->preset_count--;

  /* Adjust the sort indexes that pointe to entries above the deleted one, by reducing any indexes that are
   * greater than the deleted entry by one.
   */

  for (i=0; i<file->preset_count; i++)
  {
    if (file->presets[i].sort_index > preset_no)
    {
      file->presets[i].sort_index = file->presets[i].sort_index - 1;
    }
  }

  /* Update the main preset display window. */

  set_preset_window_extent (file);
  if (file->preset_window.preset_window != NULL)
  {
    open_window (file->preset_window.preset_window);
    if (config_opt_read ("AutoSortPresets"))
    {
      sort_preset_window (file);
      force_preset_window_redraw (file, file->preset_count, file->preset_count);
    }
    else
    {
      force_preset_window_redraw (file, 0, file->preset_count);
    }
  }
  set_file_data_integrity (file, 1);

  return (0);
}

/* ==================================================================================================================
 * Editing presets via the GUI.
 */

/* Open the preset edit window. */

void open_preset_edit_window (file_data *file, int preset, wimp_pointer *ptr)
{
  extern global_windows windows;


  /* If the window is already open, another preset is being edited or created.  Assume the user wants to lose
   * any unsaved data and just close the window.
   */

  if (window_is_open (windows.edit_preset))
  {
    wimp_close_window (windows.edit_preset);
  }

  /* Set the contents of the window up. */

  if (preset == NULL_PRESET)
  {
    msgs_lookup ("NewPreset", indirected_window_title (windows.edit_preset), 50);
    msgs_lookup ("NewAcctAct", indirected_icon_text (windows.edit_preset, PRESET_EDIT_OK), 12);
  }
  else
  {
    msgs_lookup ("EditPreset", indirected_window_title (windows.edit_preset), 50);
    msgs_lookup ("EditAcctAct", indirected_icon_text (windows.edit_preset, PRESET_EDIT_OK), 12);
  }

  fill_preset_edit_window (file, preset);

  /* Set the pointers up so we can find this lot again and open the window. */

  edit_preset_file = file;
  edit_preset_no = preset;

  open_window_centred_at_pointer (windows.edit_preset, ptr);
  place_dialogue_caret (windows.edit_preset, PRESET_EDIT_NAME);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_preset_edit_window (void)
{
  extern global_windows windows;

  fill_preset_edit_window (edit_preset_file, edit_preset_no);
  redraw_icons_in_window (windows.edit_preset, 12,
                          PRESET_EDIT_NAME, PRESET_EDIT_KEY, PRESET_EDIT_DATE,
                          PRESET_EDIT_FMIDENT, PRESET_EDIT_FMREC, PRESET_EDIT_FMNAME,
                          PRESET_EDIT_TOIDENT, PRESET_EDIT_TOREC, PRESET_EDIT_TONAME,
                          PRESET_EDIT_REF, PRESET_EDIT_AMOUNT, PRESET_EDIT_DESC);
  replace_caret_in_window (windows.edit_preset);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_preset_edit_window (file_data *file, int preset)
{
  extern global_windows windows;

  if (preset == NULL_PRESET)
  {
    /* Set name and key. */

    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_NAME) = '\0';
    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_KEY) = '\0';

    /* Set date. */

    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_DATE) = '\0';
    set_icon_selected (windows.edit_preset, PRESET_EDIT_TODAY, 0);
    set_icon_shaded (windows.edit_preset, PRESET_EDIT_DATE, 0);

    /* Fill in the from and to fields. */

    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_FMIDENT) = '\0';
    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_FMNAME) = '\0';
    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_FMREC) = '\0';

    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_TOIDENT) = '\0';
    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_TONAME) = '\0';
    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_TOREC) = '\0';

    /* Fill in the reference field. */

    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_REF) = '\0';
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CHEQUE, 0);
    set_icon_shaded (windows.edit_preset, PRESET_EDIT_REF, 0);

    /* Fill in the amount fields. */

    convert_money_to_string (0, indirected_icon_text (windows.edit_preset, PRESET_EDIT_AMOUNT));

    /* Fill in the description field. */

    *indirected_icon_text (windows.edit_preset, PRESET_EDIT_DESC) = '\0';

    /* Set the caret location icons. */

    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETDATE, 1);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETFROM, 0);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETTO, 0);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETREF, 0);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETAMOUNT, 0);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETDESC, 0);
  }
  else
  {
    /* Set name and key. */

    strcpy (indirected_icon_text (windows.edit_preset, PRESET_EDIT_NAME), file->presets[preset].name);
    sprintf (indirected_icon_text (windows.edit_preset, PRESET_EDIT_KEY), "%c",
             file->presets[preset].action_key);

    /* Set date. */

    convert_date_to_string (file->presets[preset].date,
                            indirected_icon_text (windows.edit_preset, PRESET_EDIT_DATE));
    set_icon_selected (windows.edit_preset, PRESET_EDIT_TODAY, file->presets[preset].flags & TRANS_TAKE_TODAY);
    set_icon_shaded (windows.edit_preset, PRESET_EDIT_DATE, file->presets[preset].flags & TRANS_TAKE_TODAY);

    /* Fill in the from and to fields. */

    fill_account_field(file, file->presets[preset].from, file->presets[preset].flags & TRANS_REC_FROM,
                       windows.edit_preset, PRESET_EDIT_FMIDENT, PRESET_EDIT_FMNAME, PRESET_EDIT_FMREC);

    fill_account_field(file, file->presets[preset].to, file->presets[preset].flags & TRANS_REC_TO,
                       windows.edit_preset, PRESET_EDIT_TOIDENT, PRESET_EDIT_TONAME, PRESET_EDIT_TOREC);

    /* Fill in the reference field. */

    strcpy (indirected_icon_text (windows.edit_preset, PRESET_EDIT_REF), file->presets[preset].reference);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CHEQUE, file->presets[preset].flags & TRANS_TAKE_CHEQUE);
    set_icon_shaded (windows.edit_preset, PRESET_EDIT_REF, file->presets[preset].flags & TRANS_TAKE_CHEQUE);

    /* Fill in the amount fields. */

    convert_money_to_string (file->presets[preset].amount,
                             indirected_icon_text (windows.edit_preset, PRESET_EDIT_AMOUNT));

    /* Fill in the description field. */

    strcpy (indirected_icon_text (windows.edit_preset, PRESET_EDIT_DESC), file->presets[preset].description);

    /* Set the caret location icons. */

    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETDATE,
                       file->presets[preset].caret_target == PRESET_CARET_DATE);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETFROM,
                       file->presets[preset].caret_target == PRESET_CARET_FROM);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETTO,
                       file->presets[preset].caret_target == PRESET_CARET_TO);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETREF,
                       file->presets[preset].caret_target == PRESET_CARET_REFERENCE);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETAMOUNT,
                       file->presets[preset].caret_target == PRESET_CARET_AMOUNT);
    set_icon_selected (windows.edit_preset, PRESET_EDIT_CARETDESC,
                       file->presets[preset].caret_target == PRESET_CARET_DESCRIPTION);
  }

   /* Detele the irrelevant action buttons for a new preset. */

   set_icon_deleted (windows.edit_preset, PRESET_EDIT_DELETE, preset == NULL_PRESET);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Update the account name fields in the standing order edit window. */

void update_preset_edit_account_fields (wimp_key *key)
{
  extern global_windows windows;


  if (key->i == PRESET_EDIT_FMIDENT)
  {
    lookup_account_field (edit_preset_file, key->c, ACCOUNT_IN | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
                          windows.edit_preset, PRESET_EDIT_FMIDENT, PRESET_EDIT_FMNAME, PRESET_EDIT_FMREC);
  }

  else if (key->i == PRESET_EDIT_TOIDENT)
  {
    lookup_account_field (edit_preset_file, key->c, ACCOUNT_OUT | ACCOUNT_FULL, NULL_ACCOUNT, NULL,
                          windows.edit_preset, PRESET_EDIT_TOIDENT, PRESET_EDIT_TONAME, PRESET_EDIT_TOREC);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_preset_edit_account_menu (wimp_pointer *ptr)
{
  extern global_windows windows;


  if (ptr->i == PRESET_EDIT_FMNAME)
  {
    open_account_menu (edit_preset_file, ACCOUNT_MENU_FROM, 0,
                          windows.edit_preset, PRESET_EDIT_FMIDENT, PRESET_EDIT_FMNAME, PRESET_EDIT_FMREC, ptr);
  }

  else if (ptr->i == PRESET_EDIT_TONAME)
  {
    open_account_menu (edit_preset_file, ACCOUNT_MENU_TO, 0,
                          windows.edit_preset, PRESET_EDIT_TOIDENT, PRESET_EDIT_TONAME, PRESET_EDIT_TOREC, ptr);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void toggle_preset_edit_reconcile_fields (wimp_pointer *ptr)
{
  extern global_windows windows;


  if (ptr->i == PRESET_EDIT_FMREC)
  {
    toggle_account_reconcile_icon (windows.edit_preset, PRESET_EDIT_FMREC);
  }

  else if (ptr->i == PRESET_EDIT_TOREC)
  {
    toggle_account_reconcile_icon (windows.edit_preset, PRESET_EDIT_TOREC);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated edit preset window and process the data. */

int process_preset_edit_window (void)
{
  char copyname[PRESET_NAME_LEN];
  int  check_key;

  extern global_windows windows;



  /* Test that the preset has been given a name, and reject the data if not. */

  ctrl_strcpy (copyname,indirected_icon_text (windows.edit_preset, PRESET_EDIT_NAME));

  if (*strip_surrounding_whitespace(copyname) == '\0')
  {
    wimp_msgtrans_error_report ("NoPresetName");
    return (1);
  }

  /* Test that the key, if any, is unique. */

  check_key =
      find_preset_from_keypress (edit_preset_file, *indirected_icon_text (windows.edit_preset, PRESET_EDIT_KEY));

  if (check_key != NULL_PRESET && check_key != edit_preset_no)
  {
    wimp_msgtrans_error_report ("BadPresetNo");
    return (1);
  }


  /* If the preset doesn't exsit, create it.  If it does exist, validate any data that requires it. */

  if (edit_preset_no == NULL_PRESET)
  {
    edit_preset_no = add_preset (edit_preset_file);
  }

  /* If the preset was created OK, store the rest of the data. */

  if (edit_preset_no != NULL_PRESET)
  {
    /* Zero the flags and reset them as required. */

    edit_preset_file->presets[edit_preset_no].flags = 0;

    /* Store the name. */

    strcpy (edit_preset_file->presets[edit_preset_no].name,
            indirected_icon_text (windows.edit_preset, PRESET_EDIT_NAME));

    /* Store the key. */

    edit_preset_file->presets[edit_preset_no].action_key =
            toupper (*indirected_icon_text (windows.edit_preset, PRESET_EDIT_KEY));

    /* Get the date and today settings. */

    edit_preset_file->presets[edit_preset_no].date =
           convert_string_to_date (indirected_icon_text (windows.edit_preset, PRESET_EDIT_DATE), NULL_DATE, 0);

    if (read_icon_selected (windows.edit_preset, PRESET_EDIT_TODAY))
    {
      edit_preset_file->presets[edit_preset_no].flags |= TRANS_TAKE_TODAY;
    }

    /* Get the from and to fields. */

    edit_preset_file->presets[edit_preset_no].from =
          find_account (edit_preset_file,
                        indirected_icon_text (windows.edit_preset, PRESET_EDIT_FMIDENT), ACCOUNT_FULL | ACCOUNT_IN);

    edit_preset_file->presets[edit_preset_no].to =
          find_account (edit_preset_file,
                        indirected_icon_text (windows.edit_preset, PRESET_EDIT_TOIDENT), ACCOUNT_FULL | ACCOUNT_OUT);

    if (*indirected_icon_text (windows.edit_preset, PRESET_EDIT_FMREC) != '\0')
    {
      edit_preset_file->presets[edit_preset_no].flags |= TRANS_REC_FROM;
    }

    if (*indirected_icon_text (windows.edit_preset, PRESET_EDIT_TOREC) != '\0')
    {
      edit_preset_file->presets[edit_preset_no].flags |= TRANS_REC_TO;
    }

    /* Get the amounts. */

    edit_preset_file->presets[edit_preset_no].amount =
          convert_string_to_money (indirected_icon_text (windows.edit_preset, PRESET_EDIT_AMOUNT));

    /* Store the reference. */

    strcpy (edit_preset_file->presets[edit_preset_no].reference,
            indirected_icon_text (windows.edit_preset, PRESET_EDIT_REF));

    if (read_icon_selected (windows.edit_preset, PRESET_EDIT_CHEQUE))
    {
      edit_preset_file->presets[edit_preset_no].flags |= TRANS_TAKE_CHEQUE;
    }

    /* Store the description. */

    strcpy (edit_preset_file->presets[edit_preset_no].description,
            indirected_icon_text (windows.edit_preset, PRESET_EDIT_DESC));

    /* Store the caret target. */

    edit_preset_file->presets[edit_preset_no].caret_target = PRESET_CARET_DATE;

    if (read_icon_selected (windows.edit_preset, PRESET_EDIT_CARETFROM))
    {
      edit_preset_file->presets[edit_preset_no].caret_target = PRESET_CARET_FROM;
    }
    else if (read_icon_selected (windows.edit_preset, PRESET_EDIT_CARETTO))
    {
      edit_preset_file->presets[edit_preset_no].caret_target = PRESET_CARET_TO;
    }
    else if (read_icon_selected (windows.edit_preset, PRESET_EDIT_CARETREF))
    {
      edit_preset_file->presets[edit_preset_no].caret_target = PRESET_CARET_REFERENCE;
    }
    else if (read_icon_selected (windows.edit_preset, PRESET_EDIT_CARETAMOUNT))
    {
      edit_preset_file->presets[edit_preset_no].caret_target = PRESET_CARET_AMOUNT;
    }
    else if (read_icon_selected (windows.edit_preset, PRESET_EDIT_CARETDESC))
    {
      edit_preset_file->presets[edit_preset_no].caret_target = PRESET_CARET_DESCRIPTION;
    }
  }

  if (config_opt_read ("AutoSortPresets"))
  {
    sort_preset_window (edit_preset_file);
  }
  else
  {
    force_preset_window_redraw (edit_preset_file, edit_preset_no, edit_preset_no);
  }

  set_file_data_integrity (edit_preset_file, 1);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Delete a preset here and now. */

int delete_preset_from_edit_window (void)
{
  if (wimp_msgtrans_question_report ("DeletePreset", "DeletePresetB") == 2)
  {
    return (1);
  }

  return (delete_preset (edit_preset_file, edit_preset_no));
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the standing order edit window if the file disappears. */

void force_close_preset_edit_window (file_data *file)
{
  extern global_windows windows;


  if (edit_preset_file == file && window_is_open (windows.edit_preset))
  {
    close_dialogue_with_caret (windows.edit_preset);
  }
}

/* ==================================================================================================================
 * Printing Presets via the GUI.
 */

void open_preset_print_window (file_data *file, wimp_pointer *ptr, int clear)
{
  /* Set the pointers up so we can find this lot again and open the window. */

  preset_print_file = file;

  open_simple_print_window (file, ptr, clear, "PrintPreset", print_preset_window);
}

/* ==================================================================================================================
 * Preset handling
 */

/* Find a preset based on the key pressed.  If the key is '\0', no search is made and no match is returned. */

int find_preset_from_keypress (file_data *file, char key)
{
  int preset;


  if (key != '\0')
  {
    preset = 0;

    while ((preset < file->preset_count) && (file->presets[preset].action_key != key))
    {
      preset++;
    }

    if (preset == file->preset_count)
    {
      preset = NULL_PRESET;
    }
  }
  else
  {
    preset = NULL_PRESET;
  }

  return (preset);
}

/* ==================================================================================================================
 * File and print output
 */

/* Print the standing order window by sending the data to a report. */

void print_preset_window (int text, int format, int scale, int rotate)
{
  report_data *report;
  int            i, t;
  char           line[1024], buffer[256], numbuf1[256], rec_char[REC_FIELD_LEN];
  preset_window  *window;

  msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);
  msgs_lookup ("PrintTitlePreset", buffer, sizeof (buffer));
  report = open_new_report (preset_print_file, buffer, NULL);


  if (report != NULL)
  {
    hourglass_on ();

    window = &(preset_print_file->preset_window);

    /* Output the page title. */

    make_file_leafname (preset_print_file, numbuf1, sizeof (numbuf1));
    msgs_param_lookup ("PresetTitle", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
    sprintf (line, "\\b\\u%s", buffer);
    write_report_line (report, 0, line);
    write_report_line (report, 0, "");

    /* Output the headings line, taking the text from the window icons. */

    *line = '\0';
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->preset_pane, 0, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->preset_pane, 1, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icon_text (window->preset_pane, 2, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t\\s\\t\\s\\t", icon_text (window->preset_pane, 3, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->preset_pane, 4, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u%s\\t", icon_text (window->preset_pane, 5, numbuf1));
    strcat (line, buffer);

    write_report_line (report, 0, line);

    /* Output the standing order data as a set of delimited lines. */

    for (i=0; i < preset_print_file->preset_count; i++)
    {
      t = preset_print_file->presets[i].sort_index;

      *line = '\0';

      /* The tab after the first field is in the second, as the %c can be zero in which case the
       * first string will end there and then.
       */

      sprintf (buffer, "%c", preset_print_file->presets[t].action_key);
      strcat (line, buffer);

      sprintf (buffer, "\\t%s\\t", preset_print_file->presets[t].name);
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", find_account_ident (preset_print_file, preset_print_file->presets[t].from));
      strcat (line, buffer);

      strcpy (numbuf1, (preset_print_file->presets[t].flags & TRANS_REC_FROM) ? rec_char : "");
      sprintf (buffer, "%s\\t", numbuf1);
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", find_account_name (preset_print_file, preset_print_file->presets[t].from));
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", find_account_ident (preset_print_file, preset_print_file->presets[t].to));
      strcat (line, buffer);

      strcpy (numbuf1, (preset_print_file->presets[t].flags & TRANS_REC_TO) ? rec_char : "");
      sprintf (buffer, "%s\\t", numbuf1);
      strcat (line, buffer);

      sprintf (buffer, "%s\\t", find_account_name (preset_print_file, preset_print_file->presets[t].to));
      strcat (line, buffer);

      convert_money_to_string (preset_print_file->presets[t].amount, numbuf1);
      sprintf (buffer, "\\r%s\\t", numbuf1);
      strcat (line, buffer);

      sprintf (buffer, "%s", preset_print_file->presets[t].description);
      strcat (line, buffer);

      write_report_line (report, 0, line);
    }

    hourglass_off ();
  }
  else
  {
    wimp_msgtrans_error_report ("PrintMemFail");
  }

  close_and_print_report (preset_print_file, report, text, format, scale, rotate);
}

/* ==================================================================================================================
 * Preset window handling
 */

void preset_window_click (file_data *file, wimp_pointer *pointer)
{
  int               line;
  wimp_window_state window;


  /* Find the window type and get the line clicked on. */

  window.w = pointer->w;
  wimp_get_window_state (&window);

  line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - PRESET_TOOLBAR_HEIGHT)
         / (ICON_HEIGHT+LINE_GUTTER);

  if (line < 0 || line >= file->preset_count)
  {
    line = -1;
  }

  /* Handle double-clicks, which will open an edit accout window. */

  if (pointer->buttons == wimp_DOUBLE_SELECT)
  {
    if (line != -1)
    {
      open_preset_edit_window (file, file->presets[line].sort_index, pointer);
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_preset_menu (file, line, pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void preset_pane_click (file_data *file, wimp_pointer *pointer)
{
  wimp_window_state     window;
  wimp_icon_state       icon;
  int                   ox;

  extern global_windows windows;


  /* If the click was on the sort indicator arrow, change the icon to be the icon below it. */

  if (pointer->i == PRESET_PANE_SORT_DIR_ICON)
  {
    pointer->i = preset_pane_sort_substitute_icon;
  }

  if (pointer->buttons == wimp_CLICK_SELECT)
  {
    switch (pointer->i)
    {
      case PRESET_PANE_PARENT:
        open_window (file->transaction_window.transaction_window);
        break;

      case PRESET_PANE_PRINT:
        open_preset_print_window (file, pointer, config_opt_read ("RememberValues"));
        break;

      case PRESET_PANE_ADDPRESET:
        open_preset_edit_window (file, NULL_PRESET, pointer);
        break;

      case PRESET_PANE_SORT:
        open_preset_sort_window (file, pointer);
	break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_ADJUST)
  {
    switch (pointer->i)
    {
      case PRESET_PANE_PRINT:
        open_preset_print_window (file, pointer, !config_opt_read ("RememberValues"));
        break;

      case PRESET_PANE_SORT:
        sort_preset_window (file);
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_preset_menu (file, -1, pointer);
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
      file->preset_window.sort_order = SORT_NONE;

      switch (pointer->i)
      {
        case PRESET_PANE_KEY:
          file->preset_window.sort_order = SORT_CHAR;
          break;

        case PRESET_PANE_NAME:
          file->preset_window.sort_order = SORT_NAME;
          break;

        case PRESET_PANE_FROM:
          file->preset_window.sort_order = SORT_FROM;
          break;

        case PRESET_PANE_TO:
          file->preset_window.sort_order = SORT_TO;
          break;

        case PRESET_PANE_AMOUNT:
          file->preset_window.sort_order = SORT_AMOUNT;
          break;

        case PRESET_PANE_DESCRIPTION:
          file->preset_window.sort_order = SORT_DESCRIPTION;
          break;
      }

      if (file->preset_window.sort_order != SORT_NONE)
      {
        if (pointer->buttons == wimp_CLICK_SELECT * 256)
        {
          file->preset_window.sort_order |= SORT_ASCENDING;
        }
        else
        {
          file->preset_window.sort_order |= SORT_DESCENDING;
        }
      }

      adjust_preset_window_sort_icon (file);
      force_visible_window_redraw (file->preset_window.preset_pane);
      sort_preset_window (file);
    }
  }

  else if (pointer->buttons == wimp_DRAG_SELECT)
  {
    start_column_width_drag (pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Set the extent of the preset window for the specified file. */

void set_preset_window_extent (file_data *file)
{
  wimp_window_state state;
  os_box            extent;
  int               new_lines, visible_extent, new_extent, new_scroll;


  /* Set the extent. */

  if (file->preset_window.preset_window != NULL)
  {
    /* Get the number of rows to show in the window, and work out the window extent from this. */

    new_lines = (file->preset_count > MIN_PRESET_ENTRIES) ? file->preset_count : MIN_PRESET_ENTRIES;

    new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_lines) - PRESET_TOOLBAR_HEIGHT;

    /* Get the current window details, and find the extent of the bottom of the visible area. */

    state.w = file->preset_window.preset_window;
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
    extent.x1 = file->preset_window.column_position[PRESET_COLUMNS-1] +
                file->preset_window.column_width[PRESET_COLUMNS-1] + COLUMN_GUTTER;
    extent.y0 = new_extent;

    wimp_set_extent (file->preset_window.preset_window, &extent);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called to recreate the title of the preset window connected to the data block. */

void build_preset_window_title (file_data *file)
{
  char name[256];


  if (file->preset_window.preset_window != NULL)
  {
    make_file_leafname (file, name, sizeof (name));

    msgs_param_lookup ("PresetTitle", file->preset_window.window_title,
                       sizeof (file->preset_window.window_title),
                       name, NULL, NULL, NULL);

    wimp_force_redraw_title (file->preset_window.preset_window); /* Nested Wimp only! */
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force a redraw of the preset window, for the given range of lines. */

void force_preset_window_redraw (file_data *file, int from, int to)
{
  int              y0, y1;
  wimp_window_info window;


  if (file->preset_window.preset_window != NULL)
  {
    window.w = file->preset_window.preset_window;
    wimp_get_window_info_header_only (&window);

    y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - PRESET_TOOLBAR_HEIGHT;
    y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - PRESET_TOOLBAR_HEIGHT;

    wimp_force_redraw (file->preset_window.preset_window, window.extent.x0, y0, window.extent.x1, y1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle scroll events that occur in a preset window */

void preset_window_scroll_event (file_data *file, wimp_scroll *scroll)
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

  height = (scroll->visible.y1 - scroll->visible.y0) - PRESET_TOOLBAR_HEIGHT;

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

void decode_preset_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
  int                 column, xpos;
  wimp_window_state   window;
  file_data           *file;

  *buffer = '\0';

  file = find_preset_window_file_block (w);

  if (file != NULL)
  {
    window.w = w;
    wimp_get_window_state (&window);

    xpos = (pos.x - window.visible.x0) + window.xscroll;

    for (column = 0;
         column < PRESET_COLUMNS &&
         xpos > (file->preset_window.column_position[column] + file->preset_window.column_width[column]);
         column++);

    sprintf (buffer, "Col%d", column);
  }
}
