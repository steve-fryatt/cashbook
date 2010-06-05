/* CashBook - account.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/wimp.h"
#include "oslib/dragasprite.h"
#include "oslib/wimpspriteop.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/debug.h"
#include "sflib/windows.h"
#include "sflib/string.h"
#include "sflib/msgs.h"
#include "sflib/icons.h"
#include "sflib/menus.h"

/* Application header files */

#include "global.h"
#include "account.h"

#include "accview.h"
#include "analysis.h"
#include "calculation.h"
#include "conversion.h"
#include "caret.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"
#include "window.h"

/* ==================================================================================================================
 * Global variables.
 */

/* Account and heading editing constants. */

static file_data *edit_account_file = NULL;
static int       edit_account_no = -1;

/* Section heading editing constants. */

static file_data *edit_section_file = NULL;
static int       edit_section_entry = -1;
static int       edit_section_line = -1;

/* Printing constants */

static file_data *account_print_file = NULL;
static int       account_print_type = 0;


/* Account drag constants. */

static int       dragging_sprite;
static file_data *dragging_file;
static int       dragging_entry;
static int       dragging_start_line;

/* Account name lookups. */

static file_data *account_name_lookup_file;
static unsigned  account_name_lookup_flags;
static wimp_w    account_name_lookup_window;
static wimp_i    account_name_lookup_icon;

/* ==================================================================================================================
 * Window creation and deletion
 */

/* Create and open an accounts window associated with the file block. */

void create_accounts_window (file_data *file, int type)
{
  int               entry, i, j, tb_type, height;
  os_error          *error;
  wimp_window_state parent;

  extern            global_windows windows;


  /* Find the window block to use. */

  entry = find_accounts_window_entry_from_type (file, type);

  if (entry == -1)
  {
    return;
  }

  /* Create or re-open the window. */

  if (file->account_windows[entry].account_window != NULL)
  {
    /* The window is open, so just bring it forward. */

    open_window (file->account_windows[entry].account_window);
  }
  else
  {
    /* Set the main window extent and create it. */

    *(file->account_windows[entry].window_title) = '\0';
    windows.account_window_def->title_data.indirected_text.text = file->account_windows[entry].window_title;

    height =  (file->account_windows[entry].display_lines > MIN_ACCOUNT_ENTRIES) ?
               file->account_windows[entry].display_lines : MIN_ACCOUNT_ENTRIES;


    /* Find the position to open the window at. */

    parent.w = file->transaction_window.transaction_pane;
    wimp_get_window_state (&parent);

    set_initial_window_area (windows.account_window_def,
                             file->account_windows[entry].column_position[ACCOUNT_COLUMNS-1] +
                             file->account_windows[entry].column_width[ACCOUNT_COLUMNS-1],
                              ((ICON_HEIGHT+LINE_GUTTER) * height) +
                              (ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT + 2),
                               parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
                               parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

    file->child_x_offset++;
    if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
    {
      file->child_x_offset = 0;
    }

    error = xwimp_create_window (windows.account_window_def, &(file->account_windows[entry].account_window));
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
      wimp_info_report ("Main window");
      delete_accounts_window (file, type);
      return;
    }

    /* Create the toolbar pane. */

    tb_type = (type == ACCOUNT_FULL) ? 0 : 1; /* Use toolbar 0 if it's a full account, 1 otherwise. */

    place_window_as_toolbar (windows.account_window_def, windows.account_pane_def[tb_type], ACCOUNT_TOOLBAR_HEIGHT-4);

    for (i=0, j=0; j < ACCOUNT_COLUMNS; i++, j++)
    {
      windows.account_pane_def[tb_type]->icons[i].extent.x0 = file->account_windows[entry].column_position[j];

      j = rightmost_group_column (ACCOUNT_PANE_COL_MAP, i);

      windows.account_pane_def[tb_type]->icons[i].extent.x1 = file->account_windows[entry].column_position[j]
                                                              + file->account_windows[entry].column_width[j]
                                                              + COLUMN_HEADING_MARGIN;
    }

    error = xwimp_create_window (windows.account_pane_def[tb_type], &(file->account_windows[entry].account_pane));
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
      wimp_info_report ("Toolbar");
      delete_accounts_window (file, type);
      return;
    }

    /* Create the footer pane. */

    place_window_as_footer (windows.account_window_def, windows.account_footer_def, ACCOUNT_FOOTER_HEIGHT);

    for (i=0; i < ACCOUNT_NUM_COLUMNS; i++)
    {
      windows.account_footer_def->icons[i+1].data.indirected_text.text = file->account_windows[entry].footer_icon[i];
    }

    for (i=0, j=0; j < ACCOUNT_COLUMNS; i++, j++)
    {
      windows.account_footer_def->icons[i].extent.x0 = file->account_windows[entry].column_position[j];
      windows.account_footer_def->icons[i].extent.y0 = -ACCOUNT_FOOTER_HEIGHT;
      windows.account_footer_def->icons[i].extent.y1 = 0;

      j = rightmost_group_column (ACCOUNT_PANE_COL_MAP, i);

      windows.account_footer_def->icons[i].extent.x1 = file->account_windows[entry].column_position[j]
                                                       + file->account_windows[entry].column_width[j];
    }

    /* The following block is for debug. */

    if (windows.account_footer_def->icon_count != 5)
    {
      char buf[1024];

      sprintf (buf, "Footer bar; icons = %d", windows.account_footer_def->icon_count);
      wimp_info_report (buf);
    }

    error = xwimp_create_window (windows.account_footer_def, &(file->account_windows[entry].account_footer));
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
      wimp_info_report ("Footer bar");
      delete_accounts_window (file, type);
      return;
    }

    /* Set the title */

    build_account_window_title (file, entry);

    /* Open the window. */

    if (type == ACCOUNT_FULL)
    {
      add_ihelp_window (file->account_windows[entry].account_window , "AccList", decode_account_window_help);
      add_ihelp_window (file->account_windows[entry].account_pane , "AccListTB", NULL);
      add_ihelp_window (file->account_windows[entry].account_footer , "AccListFB", NULL);
    }
    else
    {
      add_ihelp_window (file->account_windows[entry].account_window , "HeadList", decode_account_window_help);
      add_ihelp_window (file->account_windows[entry].account_pane , "HeadListTB", NULL);
      add_ihelp_window (file->account_windows[entry].account_footer , "HeadListFB", NULL);
    }

    open_window (file->account_windows[entry].account_window);
    open_window_nested_as_toolbar (file->account_windows[entry].account_pane,
                                   file->account_windows[entry].account_window,
                                   ACCOUNT_TOOLBAR_HEIGHT-4);
    open_window_nested_as_footer (file->account_windows[entry].account_footer,
                                  file->account_windows[entry].account_window,
                                  ACCOUNT_FOOTER_HEIGHT);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Close and delete the accounts window associated with the file block. */

void delete_accounts_window (file_data *file, int type)
{
  int    entry;


  /* Find the window block to use. */


  entry = find_accounts_window_entry_from_type (file, type);

  #ifdef DEBUG
  debug_printf ("\\RDeleting accounts window");
  debug_printf ("Entry: %d", entry);
  #endif

  if (entry != -1)
  {
    /* Delete the window, if it exists. */

    if (file->account_windows[entry].account_window != NULL)
    {
      remove_ihelp_window (file->account_windows[entry].account_window);
      wimp_delete_window (file->account_windows[entry].account_window);
      file->account_windows[entry].account_window = NULL;
    }

    if (file->account_windows[entry].account_pane != NULL)
    {
      remove_ihelp_window (file->account_windows[entry].account_footer);
      wimp_delete_window (file->account_windows[entry].account_pane);
      file->account_windows[entry].account_pane = NULL;
    }

    if (file->account_windows[entry].account_footer != NULL)
    {
      remove_ihelp_window (file->account_windows[entry].account_footer);
      wimp_delete_window (file->account_windows[entry].account_footer);
      file->account_windows[entry].account_footer = NULL;
    }
  }
}
/* ------------------------------------------------------------------------------------------------------------------ */

void adjust_account_window_columns (file_data *file, int entry)
{
  int              i, j, new_extent;
  wimp_icon_state  icon1, icon2;
  wimp_window_info window;


  /* Re-adjust the icons in the pane. */

  for (i=0, j=0; j < ACCOUNT_COLUMNS; i++, j++)
  {
    icon1.w = file->account_windows[entry].account_pane;
    icon1.i = i;
    wimp_get_icon_state (&icon1);

    icon2.w = file->account_windows[entry].account_footer;
    icon2.i = i;
    wimp_get_icon_state (&icon2);

    icon1.icon.extent.x0 = file->account_windows[entry].column_position[j];

    icon2.icon.extent.x0 = file->account_windows[entry].column_position[j];

    j = rightmost_group_column (ACCOUNT_PANE_COL_MAP, i);

    icon1.icon.extent.x1 = file->account_windows[entry].column_position[j] +
                           file->account_windows[entry].column_width[j] + COLUMN_HEADING_MARGIN;

    icon2.icon.extent.x1 = file->account_windows[entry].column_position[j] +
                           file->account_windows[entry].column_width[j];

    wimp_resize_icon (icon1.w, icon1.i, icon1.icon.extent.x0, icon1.icon.extent.y0,
                                        icon1.icon.extent.x1, icon1.icon.extent.y1);

    wimp_resize_icon (icon2.w, icon2.i, icon2.icon.extent.x0, icon2.icon.extent.y0,
                                        icon2.icon.extent.x1, icon2.icon.extent.y1);

    new_extent = file->account_windows[entry].column_position[ACCOUNT_COLUMNS-1] +
                 file->account_windows[entry].column_width[ACCOUNT_COLUMNS-1];
  }

  /* Replace the edit line to force a redraw and redraw the rest of the window. */

  force_visible_window_redraw (file->account_windows[entry].account_window);
  force_visible_window_redraw (file->account_windows[entry].account_pane);
  force_visible_window_redraw (file->account_windows[entry].account_footer);

  /* Set the horizontal extent of the window and pane. */

  window.w = file->account_windows[entry].account_pane;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  window.w = file->account_windows[entry].account_footer;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  window.w = file->account_windows[entry].account_window;
  wimp_get_window_info_header_only (&window);
  window.extent.x1 = window.extent.x0 + new_extent;
  wimp_set_extent (window.w, &(window.extent));

  open_window (window.w);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the type of account stored in the given window. */

int find_accounts_window_type_from_handle (file_data *file, wimp_w window)
{
  int i, type;


  /* Find the window block. */

  type = ACCOUNT_NULL;

  for (i=0; i<ACCOUNT_WINDOWS; i++)
  {
    if (file->account_windows[i].account_window == window || file->account_windows[i].account_pane == window)
    {
      type = file->account_windows[i].type;
    }
  }

  return (type);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the entry in the window list that corresponds to the given account type. */

int find_accounts_window_entry_from_type (file_data *file, int type)
{
  int i, entry;


  /* Find the window block to use. */

  entry = -1;

  for (i=0; i<ACCOUNT_WINDOWS; i++)
  {
    if (file->account_windows[i].type == type)
    {
      entry = i;
    }
  }

  return (entry);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the entry in the window list that corresponds to the given window handle. */

int find_accounts_window_entry_from_handle (file_data *file, wimp_w window)
{
  int i, entry;


  /* Find the window block to use. */

  entry = -1;

  for (i=0; i<ACCOUNT_WINDOWS; i++)
  {
    if (file->account_windows[i].account_window == window || file->account_windows[i].account_pane == window)
    {
      entry = i;
    }
  }

  return (entry);
}

/* ==================================================================================================================
 * Adding new accounts
 */

/* Create a new account with the core details.  Some values are zeroed and left to be set up later. */

int add_account (file_data *file, char *name, char *ident, unsigned int type)
{
  int new, i;


  new = -1;

  if (strcmp (ident, "") == 0)
  {
    wimp_msgtrans_error_report ("BadAcctIdent");
    return (new);
  }

  /* First, look for deleted accounts and re-use the first one found. */

  for (i=0; i<file->account_count; i++)
  {
    if (file->accounts[i].type == ACCOUNT_NULL)
    {
      new = i;
      #ifdef DEBUG
      debug_printf ("Found empty account: %d", new);
      #endif
      break;
    }
  }

  /* If that fails, create a new entry. */

  if (new == -1)
  {
    if (flex_extend ((flex_ptr) &(file->accounts), sizeof (account) * (file->account_count+1)) == 1)
    {
      new = file->account_count++;
      #ifdef DEBUG
      debug_printf ("Created new account: %d", new);
      #endif
    }
  }

  /* If a new account was created, fill it. */

  if (new != -1)
  {
    strcpy (file->accounts[new].name, name);
    strcpy (file->accounts[new].ident, ident);
    file->accounts[new].type = type;
    file->accounts[new].opening_balance = 0;
    file->accounts[new].credit_limit = 0;
    file->accounts[new].budget_amount = 0;
    file->accounts[new].next_payin_num = 0;
    file->accounts[new].payin_num_width = 0;
    file->accounts[new].next_cheque_num = 0;
    file->accounts[new].cheque_num_width = 0;

    *file->accounts[new].account_no = '\0';
    *file->accounts[new].sort_code = '\0';
    for (i=0; i<ACCOUNT_ADDR_LINES; i++)
    {
      *file->accounts[new].address[i] = '\0';
    }

    file->accounts[new].account_view = NULL;

    add_account_to_lists (file, new);
    update_transaction_window_toolbar (file);
  }
  else
  {
    wimp_msgtrans_error_report ("NoMemNewAcct");
  }

  return (new);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void add_account_to_lists (file_data *file, int account)
{
  int entry, line;


  entry = find_accounts_window_entry_from_type (file, file->accounts[account].type);

  if (entry != -1)
  {
    line = add_display_line (file, entry);

    if (line != -1)
    {
      file->account_windows[entry].line_data[line].type = ACCOUNT_LINE_DATA;
      file->account_windows[entry].line_data[line].account = account;

      /* If the target window is open, change the extent as necessary. */

      set_accounts_window_extent (file, entry);
    }
    else
    {
      wimp_msgtrans_error_report ("NoMemLinkAcct");
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Create a new display line block at the end of the list, fill it with blank data and return the number. */

int add_display_line (file_data *file, int entry)
{
  int line;


  line = -1;

  if (flex_extend ((flex_ptr) &(file->account_windows[entry].line_data),
                   sizeof (account_redraw) * ((file->account_windows[entry].display_lines)+1)) == 1)
  {
    line = file->account_windows[entry].display_lines++;

    #ifdef DEBUG
    debug_printf ("Creating new display line %d", line);
    #endif

    file->account_windows[entry].line_data[line].type = ACCOUNT_LINE_BLANK;
    file->account_windows[entry].line_data[line].account = NULL_ACCOUNT;

    *file->account_windows[entry].line_data[line].heading = '\0';
  }

  return (line);
}

/* ================================================================================================================== */

int delete_account (file_data *file, int account)
{
  int i, j;

  /* Delete the entry from the listing windows. */

  #ifdef DEBUG
  debug_printf ("Trying to delete account %d", account);
  #endif


  if (!account_used_in_file (file, account))
  {
    for (i=0; i < ACCOUNT_WINDOWS; i++)
    {
      for (j=file->account_windows[i].display_lines-1; j >= 0; j--)
      {
        if (file->account_windows[i].line_data[j].type == ACCOUNT_LINE_DATA &&
            file->account_windows[i].line_data[j].account == account)
        {
          #ifdef DEBUG
          debug_printf ("Deleting entry type %x", file->account_windows[i].line_data[j].type);
          #endif

          flex_midextend ((flex_ptr) &(file->account_windows[i].line_data),
                          (j + 1) * sizeof (account_redraw), -sizeof (account_redraw));
          file->account_windows[i].display_lines--;
          j--; /* Take into account that the array has just shortened. */
        }
      }

      set_accounts_window_extent (file, i);
      if (file->account_windows[i].account_window != NULL)
      {
        open_window (file->account_windows[i].account_window);
        force_accounts_window_redraw (file, i, 0, file->account_windows[i].display_lines);
      }
    }

    /* Close the account view window. */

    if (file->accounts[account].account_view != NULL)
    {
      delete_accview_window(file, account);
    }

    /* Remove the account from any report templates. */

    analysis_remove_account_from_reports (file, account);

    /* Blank out the account. */

    file->accounts[account].type = ACCOUNT_NULL;

    /* Update the transaction window toolbar. */

    update_transaction_window_toolbar (file);

    set_file_data_integrity (file, 1);

    return (0);
  }
  else
  {
    return (1);
  }
}

/* ==================================================================================================================
 * Editing accounts and headings via GUI.
 */

/* Open the account edit window.
 * If account = NULL_ACCOUNT, type determines the type of the new account to be created.  Otherwise, type is ignored
 * and the type derived from the account data block.
 */

void open_account_edit_window (file_data *file, int account, int type, wimp_pointer *ptr)
{
  wimp_w                win = 0;

  extern global_windows windows;


  /* If the window is already open, another account is being edited or created.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (window_is_open (windows.edit_acct))
  {
    wimp_close_window (windows.edit_acct);
  }

  if (window_is_open (windows.edit_hdr))
  {
    wimp_close_window (windows.edit_hdr);
  }

  if (window_is_open (windows.edit_sect))
  {
    wimp_close_window (windows.edit_sect);
  }

  /* Select the window to use and set the contents up. */

  if (account == NULL_ACCOUNT)
  {
    if (type & ACCOUNT_FULL)
    {
      fill_account_edit_window (file, account);
      win = windows.edit_acct;

      msgs_lookup ("NewAcct", indirected_window_title (win), 50);
      msgs_lookup ("NewAcctAct", indirected_icon_text (win, ACCT_EDIT_OK), 12);
    }
    else if (type & ACCOUNT_IN || type & ACCOUNT_OUT)
    {
      fill_heading_edit_window (file, account, type);
      win = windows.edit_hdr;

      msgs_lookup ("NewHdr", indirected_window_title (win), 50);
      msgs_lookup ("NewAcctAct", indirected_icon_text (win, HEAD_EDIT_OK), 12);
    }
  }
  else
  {
    if (file->accounts[account].type & ACCOUNT_FULL)
    {
      fill_account_edit_window (file, account);
      win = windows.edit_acct;

      msgs_lookup ("EditAcct", indirected_window_title (win), 50);
      msgs_lookup ("EditAcctAct", indirected_icon_text (win, ACCT_EDIT_OK), 12);
    }
    else if (file->accounts[account].type & ACCOUNT_IN || file->accounts[account].type & ACCOUNT_OUT)
    {
      fill_heading_edit_window (file, account, type);
      win = windows.edit_hdr;

      msgs_lookup ("EditHdr", indirected_window_title (win), 50);
      msgs_lookup ("EditAcctAct", indirected_icon_text (win, HEAD_EDIT_OK), 12);
    }
  }

  /* Set the pointers up so we can find this lot again and open the window. */

  if (win != 0)
  {
    edit_account_file = file;
    edit_account_no = account;

    open_window_centred_at_pointer (win, ptr);
    if (win == windows.edit_acct)
    {
      place_dialogue_caret (win, ACCT_EDIT_NAME);
    }
    else
    {
      place_dialogue_caret (win, HEAD_EDIT_NAME);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_account_edit_window (void)
{
  extern global_windows windows;

  fill_account_edit_window (edit_account_file, edit_account_no);
  redraw_icons_in_window (windows.edit_acct, 10, ACCT_EDIT_NAME, ACCT_EDIT_IDENT, ACCT_EDIT_CREDIT, ACCT_EDIT_BALANCE,
                          ACCT_EDIT_ACCNO, ACCT_EDIT_SRTCD,
                          ACCT_EDIT_ADDR1, ACCT_EDIT_ADDR2, ACCT_EDIT_ADDR3, ACCT_EDIT_ADDR4);
  replace_caret_in_window (windows.edit_acct);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_heading_edit_window (void)
{
  extern global_windows windows;

  fill_heading_edit_window (edit_account_file, edit_account_no, ACCOUNT_NULL);
  redraw_icons_in_window (windows.edit_hdr, 3, HEAD_EDIT_NAME, HEAD_EDIT_IDENT, HEAD_EDIT_BUDGET);
  replace_caret_in_window (windows.edit_hdr);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_account_edit_window (file_data *file, int account)
{
  int i;

  extern global_windows windows;


  if (account == NULL_ACCOUNT)
  {
    *indirected_icon_text (windows.edit_acct, ACCT_EDIT_NAME) = '\0';
    *indirected_icon_text (windows.edit_acct, ACCT_EDIT_IDENT) = '\0';

    convert_money_to_string (0, indirected_icon_text (windows.edit_acct, ACCT_EDIT_CREDIT));
    convert_money_to_string (0, indirected_icon_text (windows.edit_acct, ACCT_EDIT_BALANCE));

    *indirected_icon_text (windows.edit_acct, ACCT_EDIT_PAYIN) = '\0';
    *indirected_icon_text (windows.edit_acct, ACCT_EDIT_CHEQUE) = '\0';

    *indirected_icon_text (windows.edit_acct, ACCT_EDIT_ACCNO) = '\0';
    *indirected_icon_text (windows.edit_acct, ACCT_EDIT_SRTCD) = '\0';

    for (i=ACCT_EDIT_ADDR1; i<(ACCT_EDIT_ADDR1+ACCOUNT_ADDR_LINES); i++)
    {
      *indirected_icon_text (windows.edit_acct, i) = '\0';
    }

    set_icon_deleted (windows.edit_acct, ACCT_EDIT_DELETE, 1);
  }
  else
  {
    strcpy (indirected_icon_text (windows.edit_acct, ACCT_EDIT_NAME), file->accounts[account].name);
    strcpy (indirected_icon_text (windows.edit_acct, ACCT_EDIT_IDENT), file->accounts[account].ident);

    convert_money_to_string (file->accounts[account].credit_limit,
                             indirected_icon_text (windows.edit_acct, ACCT_EDIT_CREDIT));
    convert_money_to_string (file->accounts[account].opening_balance,
                             indirected_icon_text (windows.edit_acct, ACCT_EDIT_BALANCE));

    get_next_cheque_number (file, NULL_ACCOUNT, account, 0, indirected_icon_text (windows.edit_acct, ACCT_EDIT_PAYIN));
    get_next_cheque_number (file, account, NULL_ACCOUNT, 0, indirected_icon_text (windows.edit_acct, ACCT_EDIT_CHEQUE));

    strcpy (indirected_icon_text (windows.edit_acct, ACCT_EDIT_ACCNO), file->accounts[account].account_no);
    strcpy (indirected_icon_text (windows.edit_acct, ACCT_EDIT_SRTCD), file->accounts[account].sort_code);

    for (i=ACCT_EDIT_ADDR1; i<(ACCT_EDIT_ADDR1+ACCOUNT_ADDR_LINES); i++)
    {
      strcpy (indirected_icon_text (windows.edit_acct, i), file->accounts[account].address[i-ACCT_EDIT_ADDR1]);
    }

    set_icon_deleted (windows.edit_acct, ACCT_EDIT_DELETE, 0);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_heading_edit_window (file_data *file, int account, int type)
{
  extern global_windows windows;


  if (account == NULL_ACCOUNT)
  {
    *indirected_icon_text (windows.edit_hdr, HEAD_EDIT_NAME) = '\0';
    *indirected_icon_text (windows.edit_hdr, HEAD_EDIT_IDENT) = '\0';

    convert_money_to_string (0, indirected_icon_text (windows.edit_hdr, HEAD_EDIT_BUDGET));

    set_icon_shaded (windows.edit_hdr, HEAD_EDIT_INCOMING, 0);
    set_icon_shaded (windows.edit_hdr, HEAD_EDIT_OUTGOING, 0);
    set_icon_selected (windows.edit_hdr, HEAD_EDIT_INCOMING, (type & ACCOUNT_IN) || (type == ACCOUNT_NULL));
    set_icon_selected (windows.edit_hdr, HEAD_EDIT_OUTGOING, (type & ACCOUNT_OUT));

    set_icon_deleted (windows.edit_hdr, HEAD_EDIT_DELETE, 1);
  }
  else
  {
    strcpy (indirected_icon_text (windows.edit_hdr, HEAD_EDIT_NAME), file->accounts[account].name);
    strcpy (indirected_icon_text (windows.edit_hdr, HEAD_EDIT_IDENT), file->accounts[account].ident);

    convert_money_to_string (file->accounts[account].budget_amount, indirected_icon_text (windows.edit_hdr, HEAD_EDIT_BUDGET));

    set_icon_shaded (windows.edit_hdr, HEAD_EDIT_INCOMING, 1);
    set_icon_shaded (windows.edit_hdr, HEAD_EDIT_OUTGOING, 1);
    set_icon_selected (windows.edit_hdr, HEAD_EDIT_INCOMING, (file->accounts[account].type & ACCOUNT_IN));
    set_icon_selected (windows.edit_hdr, HEAD_EDIT_OUTGOING, (file->accounts[account].type & ACCOUNT_OUT));

    set_icon_deleted (windows.edit_hdr, HEAD_EDIT_DELETE, 0);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated edit account window and process the data. */

int process_account_edit_window (void)
{
  int                   check_ident, i, len;

  extern global_windows windows;


  /* Check if the ident is valid.  It's an account, so check all the possibilities.   If it fails, exit with
   * an error.
   */

  check_ident = find_account (edit_account_file, indirected_icon_text (windows.edit_acct, ACCT_EDIT_IDENT),
                              ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT);

  if (check_ident != NULL_ACCOUNT && check_ident != edit_account_no)
  {
    wimp_msgtrans_error_report ("UsedAcctIdent");
    return (1);
  }

  /* If the account doesn't exsit, create it.  Otherwise, copy the standard fields back from the window into
   * memory.
   */

  if (edit_account_no == NULL_ACCOUNT)
  {
    edit_account_no = add_account (edit_account_file, indirected_icon_text (windows.edit_acct, ACCT_EDIT_NAME),
                                   indirected_icon_text (windows.edit_acct, ACCT_EDIT_IDENT), ACCOUNT_FULL);
  }
  else
  {
    strcpy (edit_account_file->accounts[edit_account_no].name,
            indirected_icon_text (windows.edit_acct, ACCT_EDIT_NAME));
    strcpy (edit_account_file->accounts[edit_account_no].ident,
            indirected_icon_text (windows.edit_acct, ACCT_EDIT_IDENT));
  }

  /* If the account was created OK, store the rest of the data. */

  if (edit_account_no != NULL_ACCOUNT)
  {
    edit_account_file->accounts[edit_account_no].opening_balance
          = convert_string_to_money (indirected_icon_text (windows.edit_acct, ACCT_EDIT_BALANCE));

    edit_account_file->accounts[edit_account_no].credit_limit
          = convert_string_to_money (indirected_icon_text (windows.edit_acct, ACCT_EDIT_CREDIT));

    len = strlen (indirected_icon_text (windows.edit_acct, ACCT_EDIT_PAYIN));
    if (len > 0)
    {
      edit_account_file->accounts[edit_account_no].payin_num_width = len;
      edit_account_file->accounts[edit_account_no].next_payin_num
          = atoi (indirected_icon_text (windows.edit_acct, ACCT_EDIT_PAYIN));
    }
    else
    {
      edit_account_file->accounts[edit_account_no].payin_num_width = 0;
      edit_account_file->accounts[edit_account_no].next_payin_num = 0;
    }

    len = strlen (indirected_icon_text (windows.edit_acct, ACCT_EDIT_CHEQUE));
    if (len > 0)
    {
      edit_account_file->accounts[edit_account_no].cheque_num_width = len;
      edit_account_file->accounts[edit_account_no].next_cheque_num
          = atoi (indirected_icon_text (windows.edit_acct, ACCT_EDIT_CHEQUE));
    }
    else
    {
      edit_account_file->accounts[edit_account_no].cheque_num_width = 0;
      edit_account_file->accounts[edit_account_no].next_cheque_num = 0;
    }

    strcpy (edit_account_file->accounts[edit_account_no].account_no,
            indirected_icon_text (windows.edit_acct, ACCT_EDIT_ACCNO));
    strcpy (edit_account_file->accounts[edit_account_no].sort_code,
            indirected_icon_text (windows.edit_acct, ACCT_EDIT_SRTCD));

    for (i=ACCT_EDIT_ADDR1; i<(ACCT_EDIT_ADDR1+ACCOUNT_ADDR_LINES); i++)
    {
      strcpy (edit_account_file->accounts[edit_account_no].address[i-ACCT_EDIT_ADDR1],
              indirected_icon_text (windows.edit_acct, i));
    }
  }
  else
  {
    return (1);
  }

  trial_standing_orders (edit_account_file);
  perform_full_recalculation (edit_account_file);
  recalculate_account_view (edit_account_file, edit_account_no, 0);
  force_transaction_window_redraw (edit_account_file, 0, edit_account_file->trans_count - 1);
  refresh_transaction_edit_line_icons (edit_account_file->transaction_window.transaction_window, -1, -1);
  redraw_all_account_views (edit_account_file);
  set_file_data_integrity (edit_account_file, 1);

  /* Tidy up and redraw the windows */

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated edit heading window and process the data. */

int process_heading_edit_window (void)
{
  int                   check_ident, type;

  extern global_windows windows;


  /* Check if the ident is valid.  It's a header, so check all full accounts and those headings in the same
   * category.  If it fails, exit with an error.
   */

  type = read_icon_selected (windows.edit_hdr, HEAD_EDIT_INCOMING) ? ACCOUNT_IN : ACCOUNT_OUT;

  check_ident = find_account (edit_account_file, indirected_icon_text (windows.edit_hdr, HEAD_EDIT_IDENT),
                              ACCOUNT_FULL | type);

  if (check_ident != NULL_ACCOUNT && check_ident != edit_account_no)
  {
    wimp_msgtrans_error_report ("UsedAcctIdent");
    return (1);
  }

  /* If the heading doesn't exsit, create it.  Otherwise, copy the standard fields back from the window into
   * memory.
   */

  if (edit_account_no == NULL_ACCOUNT)
  {
    edit_account_no = add_account (edit_account_file, indirected_icon_text (windows.edit_hdr, HEAD_EDIT_NAME),
                                   indirected_icon_text (windows.edit_hdr, HEAD_EDIT_IDENT), type);
  }
  else
  {
    strcpy (edit_account_file->accounts[edit_account_no].name,
            indirected_icon_text (windows.edit_hdr, HEAD_EDIT_NAME));
    strcpy (edit_account_file->accounts[edit_account_no].ident,
            indirected_icon_text (windows.edit_hdr, HEAD_EDIT_IDENT));
  }

  /* If the heading was created OK, store the rest of the data. */

  if (edit_account_no != NULL_ACCOUNT)
  {
    edit_account_file->accounts[edit_account_no].budget_amount
          = convert_string_to_money (indirected_icon_text (windows.edit_hdr, HEAD_EDIT_BUDGET));
  }
  else
  {
    return (1);
  }

  /* Tidy up and redraw the windows */

  perform_full_recalculation (edit_account_file);
  force_transaction_window_redraw (edit_account_file, 0, edit_account_file->trans_count - 1);
  refresh_transaction_edit_line_icons (edit_account_file->transaction_window.transaction_window, -1, -1);
  redraw_all_account_views (edit_account_file);
  set_file_data_integrity (edit_account_file, 1);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the account and heading edit windows if the file disappears. */

void force_close_account_edit_window (file_data *file)
{
  extern global_windows windows;


  if (edit_account_file == file)
  {
    if (window_is_open (windows.edit_acct))
    {
      close_dialogue_with_caret (windows.edit_acct);
    }

    if (window_is_open (windows.edit_hdr))
    {
      close_dialogue_with_caret (windows.edit_hdr);
    }
  }
}
/* ------------------------------------------------------------------------------------------------------------------ */

/* Delete a section here and now. */

int delete_account_from_edit_window (void)
{
  if (account_used_in_file (edit_account_file, edit_account_no))
  {
    wimp_msgtrans_info_report ("CantDelAcct");
    return (1);
  }

  if (wimp_msgtrans_question_report ("DeleteAcct", "DeleteAcctB") == 2)
  {
    return (1);
  }

  return (delete_account (edit_account_file, edit_account_no));
}


/* ==================================================================================================================
 * Editing section headings via the GUI.
 */

void open_section_edit_window (file_data *file, int entry, int line, wimp_pointer *ptr)
{
  extern global_windows windows;


  /* If the window is already open, another account is being edited or created.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (window_is_open (windows.edit_acct))
  {
    wimp_close_window (windows.edit_acct);
  }

  if (window_is_open (windows.edit_hdr))
  {
    wimp_close_window (windows.edit_hdr);
  }

  if (window_is_open (windows.edit_sect))
  {
    wimp_close_window (windows.edit_sect);
  }

  /* Select the window to use and set the contents up. */

  if (line == -1)
  {
    fill_section_edit_window (file, entry, line);

    msgs_lookup ("NewSect", indirected_window_title (windows.edit_sect), 50);
    msgs_lookup ("NewAcctAct", indirected_icon_text (windows.edit_sect, SECTION_EDIT_OK), 12);
  }
  else
  {
    fill_section_edit_window (file, entry, line);

    msgs_lookup ("EditSect", indirected_window_title (windows.edit_sect), 50);
    msgs_lookup ("EditAcctAct", indirected_icon_text (windows.edit_sect, SECTION_EDIT_OK), 12);
  }

  /* Set the pointers up so we can find this lot again and open the window. */

  edit_section_file = file;
  edit_section_entry = entry;
  edit_section_line = line;

  open_window_centred_at_pointer (windows.edit_sect, ptr);
  place_dialogue_caret (windows.edit_sect, SECTION_EDIT_TITLE);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_section_edit_window (void)
{
  extern global_windows windows;

  fill_section_edit_window (edit_section_file, edit_section_entry, edit_section_line);
  redraw_icons_in_window (windows.edit_sect, 1, SECTION_EDIT_TITLE);
  replace_caret_in_window (windows.edit_sect);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_section_edit_window (file_data *file, int entry, int line)
{
  extern global_windows windows;


  if (line == -1)
  {
    *indirected_icon_text (windows.edit_sect, SECTION_EDIT_TITLE) = '\0';

    set_icon_selected (windows.edit_sect, SECTION_EDIT_HEADER, 1);
    set_icon_selected (windows.edit_sect, SECTION_EDIT_FOOTER, 0);
  }
  else
  {
    strcpy (indirected_icon_text (windows.edit_sect, SECTION_EDIT_TITLE),
            file->account_windows[entry].line_data[line].heading);

    set_icon_selected (windows.edit_sect, SECTION_EDIT_HEADER,
                       (file->account_windows[entry].line_data[line].type == ACCOUNT_LINE_HEADER));
    set_icon_selected (windows.edit_sect, SECTION_EDIT_FOOTER,
                       (file->account_windows[entry].line_data[line].type == ACCOUNT_LINE_FOOTER));
  }

  set_icon_deleted (windows.edit_sect, SECTION_EDIT_DELETE, line == -1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated edit heading window and process the data. */

int process_section_edit_window (void)
{
  extern global_windows windows;


  /* If the section doesn't exsit, create it.  Otherwise, copy the standard fields back from the window into
   * memory.
   */

  if (edit_section_line == -1)
  {
    edit_section_line = add_display_line (edit_section_file, edit_section_entry);

    if (edit_section_line == -1)
    {
      wimp_msgtrans_error_report ("NoMemNewSect");

      return (1);
    }
  }

  strcpy (edit_section_file->account_windows[edit_section_entry].line_data[edit_section_line].heading,
          indirected_icon_text (windows.edit_sect, SECTION_EDIT_TITLE));

  if (read_icon_selected (windows.edit_sect, SECTION_EDIT_HEADER))
  {
    edit_section_file->account_windows[edit_section_entry].line_data[edit_section_line].type = ACCOUNT_LINE_HEADER;
  }
  else if (read_icon_selected (windows.edit_sect, SECTION_EDIT_FOOTER))
  {
    edit_section_file->account_windows[edit_section_entry].line_data[edit_section_line].type = ACCOUNT_LINE_FOOTER;
  }
  else
  {
    edit_section_file->account_windows[edit_section_entry].line_data[edit_section_line].type = ACCOUNT_LINE_BLANK;
  }

  /* Tidy up and redraw the windows */

  perform_full_recalculation (edit_section_file);
  set_accounts_window_extent (edit_section_file, edit_section_entry);
  open_window (edit_section_file->account_windows[edit_section_entry].account_window);
  force_accounts_window_redraw (edit_section_file, edit_section_entry,
                                0, edit_section_file->account_windows[edit_section_entry].display_lines);
  set_file_data_integrity (edit_section_file, 1);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the section edit window if the file disappears. */

void force_close_section_edit_window (file_data *file)
{
  extern global_windows windows;


  if (edit_section_file == file && window_is_open (windows.edit_sect))
  {
    close_dialogue_with_caret (windows.edit_sect);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Delete a section here and now. */

int delete_section_from_edit_window (void)
{
  if (wimp_msgtrans_question_report ("DeleteSection", "DeleteSectionB") == 2)
  {
    return (1);
  }

  /* Delete the heading */

  flex_midextend ((flex_ptr) &(edit_section_file->account_windows[edit_section_entry].line_data),
                  (edit_section_line + 1) * sizeof (account_redraw), -sizeof (account_redraw));
  edit_section_file->account_windows[edit_section_entry].display_lines--;

  /* Update the accounts display window. */

  set_accounts_window_extent (edit_section_file, edit_section_entry);
  open_window (edit_section_file->account_windows[edit_section_entry].account_window);
  force_accounts_window_redraw (edit_section_file, edit_section_entry,
                                0, edit_section_file->account_windows[edit_section_entry].display_lines);
  set_file_data_integrity (edit_section_file, 1);

  return (0);
}

/* ==================================================================================================================
 * Printing accounts via the GUI
 */

void open_account_print_window (file_data *file, int type, wimp_pointer *ptr, int clear)
{
  /* Set the pointers up so we can find this lot again and open the window. */

  account_print_file = file;
  account_print_type = type;

  if (type & ACCOUNT_FULL)
  {
    open_simple_print_window (file, ptr, clear, "PrintAcclistAcc", print_account_window);
  }
  else if (type & ACCOUNT_IN || type & ACCOUNT_OUT)
  {
    open_simple_print_window (file, ptr, clear, "PrintAcclistHead", print_account_window);
  }
}

/* ==================================================================================================================
 * Finding accounts
 */

int find_account (file_data *file, char *ident, unsigned int type)
{
  int account;


  account = 0;

  while ((account < file->account_count) &&
         ((strcmp_no_case (ident, file->accounts[account].ident) != 0) || ((file->accounts[account].type & type) == 0)))
  {
    account++;
  }

  if (account == file->account_count)
  {
    account = NULL_ACCOUNT;
  }

  return (account);
}

/* ------------------------------------------------------------------------------------------------------------------ */

char *find_account_ident (file_data *file, int account)
{
  char *ident;


  ident = (account != NULL_ACCOUNT) ? file->accounts[account].ident : "";

  return (ident);
}

/* ------------------------------------------------------------------------------------------------------------------ */

char *find_account_name (file_data *file, int account)
{
  char *name;


  name = (account != NULL_ACCOUNT) ? file->accounts[account].name : "";

  return (name);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Build an account ident:name pair string. */

char *build_account_name_pair (file_data *file, int account, char *buffer)
{
  *buffer = '\0';

  if (account != NULL_ACCOUNT)
  {
    sprintf (buffer, "%s:%s", find_account_ident (file, account), find_account_name (file, account));
  }

  return (buffer);
}

/* ================================================================================================================== */

int lookup_account_field (file_data *file, char key, int type, int account, int *reconciled,
                          wimp_w window, wimp_i ident, wimp_i name, wimp_i rec)
{
  int new_rec = 0;
  char *ident_ptr, *name_ptr, *rec_ptr;


  /* Look up the text fields for the icons. */

  ident_ptr = indirected_icon_text (window, ident);
  name_ptr = indirected_icon_text (window, name);
  rec_ptr = indirected_icon_text (window, rec);

  /* If the character is an alphanumeric or a delete, look up the ident as it stends. */

  if (isalnum (key) || iscntrl (key))
  {
    /* Look up the account number based on the ident. */

    account = find_account (file, ident_ptr, type);

    /* Copy the corresponding name into the name field. */

    strcpy (name_ptr, find_account_name (file, account));
    wimp_set_icon_state (window, name, 0, 0);

    /* Do the auto-reconciliation. */

    if (account != NULL_ACCOUNT && !(file->accounts[account].type & ACCOUNT_FULL))
    {
      /* If the account exists, and it is a heading (ie. it isn't a full account), reconcile it... */

      new_rec = 1;
      msgs_lookup ("RecChar", rec_ptr, REC_FIELD_LEN);
      wimp_set_icon_state (window, rec, 0, 0);
    }
    else
    {
      /* ...otherwise unreconcile it. */

      new_rec = 0;
      *rec_ptr = '\0';
      wimp_set_icon_state (window, rec, 0, 0);
    }
  }

  /* If the key pressed was a reconcile one, set or clear the bit as required. */

  if (key == '+' || key == '=')
  {
    new_rec = 1;
    msgs_lookup ("RecChar", rec_ptr, REC_FIELD_LEN);
    wimp_set_icon_state (window, rec, 0, 0);
  }

  if (key == '-' || key == '_')
  {
    new_rec = 0;
    *rec_ptr = '\0';
    wimp_set_icon_state (window, rec, 0, 0);
  }

  /* Return the new reconciled state if applicable. */

  if (reconciled != NULL)
  {
    *reconciled = new_rec;
  }

  return (account);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Fill three icons with account name, ident and reconciled status. */

void fill_account_field (file_data *file, acct_t account, int reconciled,
                         wimp_w window, wimp_i ident, wimp_i name, wimp_i rec_field)
{
  strcpy (indirected_icon_text (window, ident), find_account_ident (file, account));

  if (reconciled)
  {
    msgs_lookup ("RecChar", indirected_icon_text (window, rec_field), REC_FIELD_LEN);
  }
  else
  {
    *indirected_icon_text (window, rec_field) = '\0';
  }
  strcpy (indirected_icon_text (window, name), find_account_name (file, account));
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Toggle the reconcile status in an icon. */

void toggle_account_reconcile_icon (wimp_w window, wimp_i icon)
{
  if (*indirected_icon_text (window, icon) == '\0')
  {
    msgs_lookup ("RecChar", indirected_icon_text (window, icon), REC_FIELD_LEN);
  }
  else
  {
    *indirected_icon_text (window, icon) = '\0';
  }

  wimp_set_icon_state (window, icon, 0, 0);
}

/* ================================================================================================================== */

/* Open the account lookup window as a menu. */

void open_account_lookup_window (file_data *file, wimp_w window, wimp_i icon, int account, unsigned flags)
{
  wimp_pointer          pointer;

  extern global_windows windows;


  strcpy (indirected_icon_text (windows.enter_acc, ACC_NAME_ENTRY_IDENT), find_account_ident (file, account));
  strcpy (indirected_icon_text (windows.enter_acc, ACC_NAME_ENTRY_NAME), find_account_name (file, account));
  *indirected_icon_text (windows.enter_acc, ACC_NAME_ENTRY_REC) = '\0';

  account_name_lookup_file = file;
  account_name_lookup_flags = flags;
  account_name_lookup_window = window;
  account_name_lookup_icon = icon;

  /* Set the window position and open it on screen. */

  pointer.w = window;
  pointer.i = icon;

  create_popup_menu ((wimp_menu *) windows.enter_acc, &pointer);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Update the details in the account lookup window, following a keypress. */

void update_account_lookup_window (wimp_key *key)
{
  extern global_windows windows;


  if (key->i == ACC_NAME_ENTRY_IDENT)
  {
    lookup_account_field (account_name_lookup_file, key->c, account_name_lookup_flags, NULL_ACCOUNT, NULL,
                          windows.enter_acc, ACC_NAME_ENTRY_IDENT, ACC_NAME_ENTRY_NAME, ACC_NAME_ENTRY_REC);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_account_lookup_account_menu (wimp_pointer *ptr)
{
  int               type;
  wimp_window_state window_state;

  extern global_windows windows;


  if (ptr->i == ACC_NAME_ENTRY_NAME)
  {
    window_state.w = windows.enter_acc;
    wimp_get_window_state (&window_state);
    wimp_create_menu ((wimp_menu *) -1, 0, 0);
    wimp_open_window ((wimp_open *) &window_state);

    switch (account_name_lookup_flags)
    {
      case (ACCOUNT_FULL | ACCOUNT_IN):
        type = ACCOUNT_MENU_FROM;
        break;

      case (ACCOUNT_FULL | ACCOUNT_OUT):
        type = ACCOUNT_MENU_TO;
        break;

      case (ACCOUNT_FULL):
        type = ACCOUNT_MENU_ACCOUNTS;
        break;

      case (ACCOUNT_IN):
        type = ACCOUNT_MENU_INCOMING;
        break;

      case (ACCOUNT_OUT):
        type = ACCOUNT_MENU_OUTGOING;
        break;

      default:
        type = ACCOUNT_MENU_FROM;
        break;
    }

    open_account_menu (account_name_lookup_file, type, 0,
                          windows.enter_acc, ACC_NAME_ENTRY_IDENT, ACC_NAME_ENTRY_NAME, ACC_NAME_ENTRY_REC, ptr);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* This function is called whenever the account list menu closes.  If the enter account window is open,
 * this is converted back into a transient menu.
 */

void close_account_lookup_account_menu (void)
{
  wimp_window_state window_state;

  extern global_windows windows;


  if (window_is_open (windows.enter_acc))
  {
    window_state.w = windows.enter_acc;
    wimp_get_window_state (&window_state);
    wimp_close_window (windows.enter_acc);

    if (window_is_open (account_name_lookup_window))
    {
      wimp_create_menu ((wimp_menu *) -1, 0, 0);
      wimp_create_menu ((wimp_menu *) windows.enter_acc, window_state.visible.x0, window_state.visible.y1);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void toggle_account_lookup_reconcile_field (wimp_pointer *ptr)
{
  extern global_windows windows;


  if (ptr->i == ACC_NAME_ENTRY_REC)
  {
    toggle_account_reconcile_icon (windows.enter_acc, ACC_NAME_ENTRY_REC);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Process the account from the account lookup window, and put the ident into the parent icon. */

int process_account_lookup_window (void)
{
  int        account, index, max_len;
  char       ident[ACCOUNT_IDENT_LEN+1], *icon;
  wimp_caret caret;

  extern global_windows windows;


  /* Get the account number that was entered. */

  account = find_account (account_name_lookup_file, indirected_icon_text (windows.enter_acc,ACC_NAME_ENTRY_IDENT),
                                              account_name_lookup_flags);

  if (account != NULL_ACCOUNT)
  {
    /* Get the icon text, and the length of it. */

    icon = indirected_icon_text (account_name_lookup_window, account_name_lookup_icon);
    max_len = ctrl_strlen (icon);

    /* Check the caret position.  If it is in the target icon, move the insertion until it falls before a comma;
     * if not, place the index at the end of the text.
     */

    wimp_get_caret_position (&caret);
    if (caret.w == account_name_lookup_window && caret.i == account_name_lookup_icon)
    {
      index = caret.index;
      while (index < max_len && icon[index] != ',')
      {
        index++;
      }
    }
    else
    {
      index = max_len;
    }

    /* If the icon text is empty, the ident is inserted on its own.
     *
     * If there is text there, a comma is placed at the start or end depending on where the index falls in the
     * string.  If it falls anywhere but the end, the index is assumed to be after a comma such that an extra
     * comma is added after the ident to be inserted.
     */

    if (*icon == '\0')
    {
      sprintf(ident, "%s", find_account_ident (account_name_lookup_file, account));
    }
    else
    {
      if (index < max_len)
      {
        sprintf(ident, "%s,", find_account_ident (account_name_lookup_file, account));
      }
      else
      {
        sprintf(ident, ",%s", find_account_ident (account_name_lookup_file, account));
      }
    }

    insert_text_into_icon (account_name_lookup_window, account_name_lookup_icon, index, ident, strlen (ident));
    replace_caret_in_window (account_name_lookup_window);
  }

  return (0);
}

/* ================================================================================================================== */

/* Check if an account number is used in any transactions or standing orders in a file. */

int account_used_in_file (file_data *file, int account)
{
  int i, found;

  found = 0;
  i = 0;

  while (i<file->trans_count && !found)
  {
    if (file->transactions[i].from == account || file->transactions[i].to == account)
    {
      found = 1;
    }
    i++;
  }

  while (i<file->sorder_count && !found)
  {
    if (file->sorders[i].from == account || file->sorders[i].to == account)
    {
      found = 1;
    }
    i++;
  }

  while (i<file->preset_count && !found)
  {
    if (file->presets[i].from == account || file->presets[i].to == account)
    {
      found = 1;
    }
    i++;
  }

  return (found);
}

/* ================================================================================================================== */

int count_accounts_in_file (file_data *file, unsigned int type)
{
  int i, accounts;


  accounts = 0;

  for (i=0; i<file->account_count; i++)
  {
    if ((file->accounts[i].type & type) != 0)
    {
      accounts++;
    }
  }

  return (accounts);
}

/* ==================================================================================================================
 * File and print output
 */

/* Print the account window by sending the data to a report. */

void print_account_window (int text, int format, int scale, int rotate)
{
  report_data *report;
  int            i, entry;
  char           line[4096], buffer[256], numbuf1[64], numbuf2[64], numbuf3[64], numbuf4[64];
  account_window  *window;

  msgs_lookup ((account_print_type & ACCOUNT_FULL) ? "PrintTitleAcclistAcc" : "PrintTitleAcclistHead",
               buffer, sizeof (buffer));
  report = open_new_report (account_print_file, buffer, NULL);

  if (report != NULL)
  {
    hourglass_on ();

    entry = find_accounts_window_entry_from_type (account_print_file, account_print_type);
    window = &(account_print_file->account_windows[entry]);

    /* Output the page title. */

    make_file_leafname (account_print_file, numbuf1, sizeof (numbuf1));
    switch (window->type)
    {
      case ACCOUNT_FULL:
        msgs_param_lookup ("AcclistTitleAcc", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
        break;

      case ACCOUNT_IN:
        msgs_param_lookup ("AcclistTitleHIn", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
        break;

      case ACCOUNT_OUT:
        msgs_param_lookup ("AcclistTitleHOut", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
        break;
    }
    sprintf (line, "\\b\\u%s", buffer);
    write_report_line (report, 0, line);

    if (account_print_file->budget.start != NULL_DATE || account_print_file->budget.finish != NULL_DATE)
    {
      *line = '\0';
      msgs_lookup ("AcclistBudgetTitle", buffer, sizeof (buffer));
      strcat (line, buffer);

      if (account_print_file->budget.start != NULL_DATE)
      {
        convert_date_to_string (account_print_file->budget.start, numbuf1);
        msgs_param_lookup ("AcclistBudgetFrom", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
        strcat (line, buffer);
      }

      if (account_print_file->budget.finish != NULL_DATE)
      {
        convert_date_to_string (account_print_file->budget.finish, numbuf1);
        msgs_param_lookup ("AcclistBudgetTo", buffer, sizeof (buffer), numbuf1, NULL, NULL, NULL);
        strcat (line, buffer);
      }

      strcat (line, ".");

      write_report_line (report, 0, line);
    }

    write_report_line (report, 0, "");

    /* Output the headings line, taking the text from the window icons. */

    *line = '\0';
    sprintf (buffer, "\\b\\u%s\\t\\s\\t", icon_text (window->account_pane, 0, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->account_pane, 1, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->account_pane, 2, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s\\t", icon_text (window->account_pane, 3, numbuf1));
    strcat (line, buffer);
    sprintf (buffer, "\\b\\u\\r%s", icon_text (window->account_pane, 4, numbuf1));
    strcat (line, buffer);

    write_report_line (report, 0, line);

    /* Output the account data as a set of delimited lines. */
    /* Output the transaction data as a set of delimited lines. */

    for (i=0; i < window->display_lines; i++)
    {
      *line = '\0';

      if (window->line_data[i].type == ACCOUNT_LINE_DATA)
      {
        build_account_name_pair (account_print_file, window->line_data[i].account, buffer);

        switch (window->type)
        {
          case ACCOUNT_FULL:
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].statement_balance,
                                     numbuf1);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].current_balance,
                                     numbuf2);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].trial_balance,
                                     numbuf3);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].budget_balance,
                                     numbuf4);
            break;

          case ACCOUNT_IN:
            convert_money_to_string (-account_print_file->accounts[window->line_data[i].account].future_balance,
                                     numbuf1);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].budget_amount,
                                     numbuf2);
            convert_money_to_string (-account_print_file->accounts[window->line_data[i].account].budget_balance,
                                     numbuf3);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].budget_result,
                                     numbuf4);
            break;

          case ACCOUNT_OUT:
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].future_balance,
                                     numbuf1);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].budget_amount,
                                     numbuf2);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].budget_balance,
                                     numbuf3);
            convert_money_to_string (account_print_file->accounts[window->line_data[i].account].budget_result,
                                     numbuf4);
            break;
        }
        sprintf (line, "%s\\t%s\\t\\r%s\\t\\r%s\\t\\r%s\\t\\r%s",
                 find_account_ident (account_print_file, window->line_data[i].account),
                 find_account_name (account_print_file, window->line_data[i].account),
                 numbuf1, numbuf2, numbuf3, numbuf4);
      }
      else if (window->line_data[i].type == ACCOUNT_LINE_HEADER)
      {
        sprintf (line, "\\u%s", window->line_data[i].heading);
      }
      else if (window->line_data[i].type == ACCOUNT_LINE_FOOTER)
      {
        convert_money_to_string (window->line_data[i].total[0], numbuf1);
        convert_money_to_string (window->line_data[i].total[1], numbuf2);
        convert_money_to_string (window->line_data[i].total[2], numbuf3);
        convert_money_to_string (window->line_data[i].total[3], numbuf4);

        sprintf (line, "%s\\t\\s\\t\\r\\b%s\\t\\r\\b%s\\t\\r\\b%s\\t\\r\\b%s",
                 window->line_data[i].heading, numbuf1, numbuf2, numbuf3, numbuf4);
      }

      write_report_line (report, 0, line);
    }

    /* Output the grand total line, taking the text from the window icons. */

    icon_text (window->account_footer, 0, buffer);
    sprintf (line, "\\u%s\\t\\s\\t\\r%s\\t\\r%s\\t\\r%s\\t\\r%s", buffer,
             window->footer_icon[0], window->footer_icon[1], window->footer_icon[2], window->footer_icon[3]);
    write_report_line (report, 0, line);

    hourglass_off ();
  }
  else
  {
    wimp_msgtrans_error_report ("PrintMemFail");
  }

  close_and_print_report (account_print_file, report, text, format, scale, rotate);
}

/* ==================================================================================================================
 * Account window handling
 */

void account_window_click (file_data *file, wimp_pointer *pointer)
{
  int               line, entry;
  wimp_window_state window;


  /* Find the window type and get the line clicked on. */

  entry = find_accounts_window_entry_from_handle (file, pointer->w);

  window.w = pointer->w;
  wimp_get_window_state (&window);

  line = ((window.visible.y1 - pointer->pos.y) - window.yscroll - ACCOUNT_TOOLBAR_HEIGHT)
         / (ICON_HEIGHT+LINE_GUTTER);

  if (line < 0 || line >= file->account_windows[entry].display_lines)
  {
    line = -1;
  }

  /* Handle double-clicks, which will open a statement view or an edit accout window. */

  if (pointer->buttons == wimp_DOUBLE_SELECT && line != -1)
  {
    if (file->account_windows[entry].line_data[line].type == ACCOUNT_LINE_DATA)
    {
      create_accview_window (file, file->account_windows[entry].line_data[line].account);
    }
  }

  else if (pointer->buttons == wimp_DOUBLE_ADJUST && line != -1)
  {
    switch (file->account_windows[entry].line_data[line].type)
    {
      case ACCOUNT_LINE_DATA:
        open_account_edit_window (file, file->account_windows[entry].line_data[line].account, ACCOUNT_NULL, pointer);
        break;

      case ACCOUNT_LINE_HEADER:
      case ACCOUNT_LINE_FOOTER:
        open_section_edit_window (file, entry, line, pointer);
        break;
    }
  }

  else if (pointer->buttons == wimp_DRAG_SELECT && line != -1)
  {
    start_account_drag (file, entry, line);
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_acclist_menu (file, find_accounts_window_type_from_handle (file, pointer->w), line, pointer);
  }

}

/* ------------------------------------------------------------------------------------------------------------------ */

void account_pane_click (file_data *file, wimp_pointer *pointer)
{
  if (pointer->buttons == wimp_CLICK_SELECT)
  {
    switch (pointer->i)
    {
      case ACCOUNT_PANE_PARENT:
        open_window (file->transaction_window.transaction_window);
        break;

      case ACCOUNT_PANE_PRINT:
        open_account_print_window (file, find_accounts_window_type_from_handle (file, pointer->w), pointer,
                                   read_config_opt ("RememberValues"));
        break;

      case ACCOUNT_PANE_ADDACCT:
        open_account_edit_window (file, NULL_ACCOUNT,
                                  find_accounts_window_type_from_handle (file, pointer->w), pointer);
        break;

      case ACCOUNT_PANE_ADDSECT:
        open_section_edit_window (file, find_accounts_window_entry_from_handle (file, pointer->w), -1, pointer);
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_ADJUST)
  {
    switch (pointer->i)
    {
      case ACCOUNT_PANE_PRINT:
        open_account_print_window (file, find_accounts_window_type_from_handle (file, pointer->w), pointer,
                                   !read_config_opt ("RememberValues"));
        break;
    }
  }

  else if (pointer->buttons == wimp_CLICK_MENU)
  {
    open_acclist_menu (file, find_accounts_window_type_from_handle (file, pointer->w), -1, pointer);
  }

  else if (pointer->buttons == wimp_DRAG_SELECT)
  {
    start_column_width_drag (pointer);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */
/* Set the extent of the standing order window for the specified file. */

void set_accounts_window_extent (file_data *file, int entry)
{
  wimp_window_state state;
  os_box            extent;
  int               new_height, visible_extent, new_extent, new_scroll;


  /* Set the extent. */

  if (file->account_windows[entry].account_window != NULL)
  {
    /* Get the number of rows to show in the window, and work out the window extent from this. */

    new_height =  (file->account_windows[entry].display_lines > MIN_ACCOUNT_ENTRIES) ?
                   file->account_windows[entry].display_lines : MIN_ACCOUNT_ENTRIES;

    new_extent = (-(ICON_HEIGHT+LINE_GUTTER) * new_height) - (ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT + 2);

    /* Get the current window details, and find the extent of the bottom of the visible area. */

    state.w = file->account_windows[entry].account_window;
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
    extent.x1 = file->account_windows[entry].column_position[ACCOUNT_COLUMNS-1]
                + file->account_windows[entry].column_width[ACCOUNT_COLUMNS-1];

    extent.y0 = new_extent;
    extent.y1 = 0;

    wimp_set_extent (file->account_windows[entry].account_window, &extent);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called to recreate the title of the accounts window connected to the data block. */

void build_account_window_title (file_data *file, int entry)
{
  char name[256];


  if (file->account_windows[entry].account_window != NULL)
  {
    make_file_leafname (file, name, sizeof (name));

    switch (file->account_windows[entry].type)
    {
      case ACCOUNT_FULL:
        msgs_param_lookup ("AcclistTitleAcc", file->account_windows[entry].window_title,
                           sizeof (file->account_windows[entry].window_title),
                           name, NULL, NULL, NULL);
        break;

      case ACCOUNT_IN:
        msgs_param_lookup ("AcclistTitleHIn", file->account_windows[entry].window_title,
                           sizeof (file->account_windows[entry].window_title),
                           name, NULL, NULL, NULL);
        break;

      case ACCOUNT_OUT:
        msgs_param_lookup ("AcclistTitleHOut", file->account_windows[entry].window_title,
                           sizeof (file->account_windows[entry].window_title),
                           name, NULL, NULL, NULL);
        break;
    }

    wimp_force_redraw_title (file->account_windows[entry].account_window); /* Nested Wimp only! */
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void force_accounts_window_redraw (file_data *file, int entry, int from, int to)
{
  int              y0, y1;
  wimp_window_info window;


  if (file->account_windows[entry].account_window != NULL)
  {
    window.w = file->account_windows[entry].account_window;
    wimp_get_window_info_header_only (&window);

    y1 = -from * (ICON_HEIGHT+LINE_GUTTER) - ACCOUNT_TOOLBAR_HEIGHT;
    y0 = -(to + 1) * (ICON_HEIGHT+LINE_GUTTER) - ACCOUNT_TOOLBAR_HEIGHT;

    wimp_force_redraw (file->account_windows[entry].account_window, window.extent.x0, y0, window.extent.x1, y1);

    /* Force a redraw of the three total icons in the footer. */

    redraw_icons_in_window (file->account_windows[entry].account_footer, 4, 1, 2, 3, 4);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle scroll events that occur in a account window */

void account_window_scroll_event (file_data *file, wimp_scroll *scroll)
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

  height = (scroll->visible.y1 - scroll->visible.y0) - (ACCOUNT_TOOLBAR_HEIGHT + ACCOUNT_FOOTER_HEIGHT);

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

void decode_account_window_help (char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
  int                 entry, column, xpos;
  wimp_window_state   window;
  file_data           *file;

  *buffer = '\0';

  file = find_account_window_file_block (w);
  entry = find_accounts_window_entry_from_handle (file, w);

  if (file != NULL)
  {
    window.w = w;
    wimp_get_window_state (&window);

    xpos = (pos.x - window.visible.x0) + window.xscroll;

    for (column = 0;
         column < ACCOUNT_COLUMNS &&
         xpos > (file->account_windows[entry].column_position[column] +
                 file->account_windows[entry].column_width[column]);
         column++);

    sprintf (buffer, "Col%d", column);
  }
}

/* ==================================================================================================================
 * Account window dragging
 */

/* Start an account window drag, to re-order the entries in the window. */

void start_account_drag (file_data *file, int entry, int line)
{
  wimp_window_state     window;
  wimp_auto_scroll_info auto_scroll;
  wimp_drag             drag;
  int                   ox, oy;

  extern int global_drag_type;
  extern global_windows windows;


  /* The drag is not started if any of the account window edit dialogues are open, as these will have pointers into
   * the data which won't like that data moving beneath them.
   */

  if (!window_is_open (windows.edit_acct) && !window_is_open (windows.edit_hdr) && !window_is_open (windows.edit_sect))
  {
    /* Get the basic information about the window. */

    window.w = file->account_windows[entry].account_window;
    wimp_get_window_state (&window);

    ox = window.visible.x0 - window.xscroll;
    oy = window.visible.y1 - window.yscroll;

    /* Set up the drag parameters. */

    drag.w = file->account_windows[entry].account_window;
    drag.type = wimp_DRAG_USER_FIXED;

    drag.initial.x0 = ox;
    drag.initial.y0 = oy + -(line * (ICON_HEIGHT+LINE_GUTTER) + ACCOUNT_TOOLBAR_HEIGHT + ICON_HEIGHT);
    drag.initial.x1 = ox + (window.visible.x1 - window.visible.x0);
    drag.initial.y1 = oy + -(line * (ICON_HEIGHT+LINE_GUTTER) + ACCOUNT_TOOLBAR_HEIGHT);

    drag.bbox.x0 = window.visible.x0;
    drag.bbox.y0 = window.visible.y0;
    drag.bbox.x1 = window.visible.x1;
    drag.bbox.y1 = window.visible.y1;

    /* Read CMOS RAM to see if solid drags are required. */

    dragging_sprite = ((osbyte2 (osbyte_READ_CMOS, osbyte_CONFIGURE_DRAG_ASPRITE, 0) &
                       osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0);

    if (0 && dragging_sprite) /* This is never used, though it could be... */
    {
      dragasprite_start (dragasprite_HPOS_CENTRE | dragasprite_VPOS_CENTRE | dragasprite_NO_BOUND |
                         dragasprite_BOUND_POINTER | dragasprite_DROP_SHADOW, wimpspriteop_AREA,
                         "", &(drag.initial), &(drag.bbox));
    }
    else
    {
      wimp_drag_box (&drag);
    }

    /* Initialise the autoscroll. */

    if (xos_swi_number_from_string ("Wimp_AutoScroll", NULL) == NULL)
    {
      auto_scroll.w = file->account_windows[entry].account_window;
      auto_scroll.pause_zone_sizes.x0 = AUTO_SCROLL_MARGIN;
      auto_scroll.pause_zone_sizes.y0 = AUTO_SCROLL_MARGIN + ACCOUNT_FOOTER_HEIGHT;
      auto_scroll.pause_zone_sizes.x1 = AUTO_SCROLL_MARGIN;
      auto_scroll.pause_zone_sizes.y1 = AUTO_SCROLL_MARGIN + ACCOUNT_TOOLBAR_HEIGHT;
      auto_scroll.pause_duration = 0;
      auto_scroll.state_change = (void *) 1;

      wimp_auto_scroll (wimp_AUTO_SCROLL_ENABLE_HORIZONTAL | wimp_AUTO_SCROLL_ENABLE_VERTICAL, &auto_scroll);
    }

    global_drag_type = ACCOUNT_DRAG;

    dragging_file = file;
    dragging_start_line = line;
    dragging_entry = entry;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Terminate an account window drag and re-order the data. */

void terminate_account_drag (wimp_dragged *drag)
{
  wimp_pointer      pointer;
  wimp_window_state window;
  int               line;
  account_redraw    block;


  /* Terminate the drag and end the autoscroll. */

  if (xos_swi_number_from_string ("Wimp_AutoScroll", NULL) == NULL)
  {
    wimp_auto_scroll (0, NULL);
  }

  if (dragging_sprite)
  {
    dragasprite_stop ();
  }

  /* Get the line at which the drag ended. */

  wimp_get_pointer_info (&pointer);

  window.w = dragging_file->account_windows[dragging_entry].account_window;
  wimp_get_window_state (&window);

  line = ((window.visible.y1 - pointer.pos.y) - window.yscroll - ACCOUNT_TOOLBAR_HEIGHT)
         / (ICON_HEIGHT+LINE_GUTTER);

  if (line < 0)
  {
    line = 0;
  }
  if (line >= dragging_file->account_windows[dragging_entry].display_lines)
  {
    line = dragging_file->account_windows[dragging_entry].display_lines - 1;
  }

  /* Move the blocks around. */

  block =  dragging_file->account_windows[dragging_entry].line_data[dragging_start_line];

  if (line < dragging_start_line)
  {
    memmove (&(dragging_file->account_windows[dragging_entry].line_data[line+1]),
             &(dragging_file->account_windows[dragging_entry].line_data[line]),
             (dragging_start_line - line) * sizeof (account_redraw));

    dragging_file->account_windows[dragging_entry].line_data[line] = block;
  }
  else if (line > dragging_start_line)
  {
    memmove (&(dragging_file->account_windows[dragging_entry].line_data[dragging_start_line]),
             &(dragging_file->account_windows[dragging_entry].line_data[dragging_start_line+1]),
             (line - dragging_start_line) * sizeof (account_redraw));

    dragging_file->account_windows[dragging_entry].line_data[line] = block;
  }

  /* Tidy up and redraw the windows */

  perform_full_recalculation (dragging_file);
  set_file_data_integrity (dragging_file, 1);
  force_accounts_window_redraw (dragging_file, dragging_entry,
                                0, dragging_file->account_windows[dragging_entry].display_lines - 1);

  #ifdef DEBUG
  debug_printf ("Move account from line %d to line %d", dragging_start_line, line);
  #endif
}

/* ==================================================================================================================
 * Cheque number printing
 */

/* Find the next cheque number from one of the two accounts, and return it in the buffer.
 * At present, only the from account is handled -- this uses the cheque number.
 */

char *get_next_cheque_number (file_data *file, acct_t from_account, acct_t to_account, int increment, char *buffer)
{
  char   format[32], mbuf[1024], bbuf[128];
  int    from_ok, to_ok;

  /* Test which of the two accounts have an auto-reference attached.  If both do, the user needs to be asked which one
   * use in the transaction.
   */

  from_ok = (from_account != NULL_ACCOUNT && file->accounts[from_account].cheque_num_width > 0);
  to_ok = (to_account != NULL_ACCOUNT && file->accounts[to_account].payin_num_width > 0);

  if (from_ok && to_ok)
  {
    msgs_param_lookup ("ChqOrPayIn", mbuf, sizeof(mbuf),
                       file->accounts[to_account].name, file->accounts[from_account].name, NULL, NULL);
    msgs_lookup ("ChqOrPayInB", bbuf, sizeof(bbuf));

    if (wimp_question_report (mbuf, bbuf) == 1)
    {
      to_ok = 0;
    }
    else
    {
      from_ok = 0;
    }
  }

  /* Now process the reference. */

  if (from_ok)
  {
    sprintf (format, "%%0%dd", file->accounts[from_account].cheque_num_width);
    sprintf (buffer, format, file->accounts[from_account].next_cheque_num);
    file->accounts[from_account].next_cheque_num += increment;
  }
  else if (to_ok)
  {
    sprintf (format, "%%0%dd", file->accounts[to_account].payin_num_width);
    sprintf (buffer, format, file->accounts[to_account].next_payin_num);
    file->accounts[to_account].next_payin_num += increment;
  }
  else
  {
    *buffer = '\0';
  }

  return (buffer);
}
