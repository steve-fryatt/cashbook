/* CashBook - main.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <stdlib.h>
#include <string.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/uri.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/pdriver.h"
#include "oslib/help.h"

/* SF-Lib header files. */

#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/transfer.h"
#include "sflib/url.h"
#include "sflib/msgs.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/string.h"
#include "sflib/colpick.h"

/* Application header files */

#include "main.h"
#include "global.h"

#include "account.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "caret.h"
#include "choices.h"
#include "clipboard.h"
#include "continue.h"
#include "dataxfer.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "find.h"
#include "goto.h"
#include "ihelp.h"
#include "init.h"
#include "mainmenu.h"
#include "presets.h"
#include "printing.h"
#include "redraw.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"
#include "window.h"

/* ------------------------------------------------------------------------------------------------------------------ */

/* Declare the global variables that are used. */

global_windows          windows;
global_menus            menus;
int                     global_drag_type;

/* Cross file global variables */

wimp_t                     task_handle;
int                        quit_flag = FALSE;

/* ==================================================================================================================
 * Main function
 */

int main (int argc, char *argv[])

{
  extern wimp_t task_handle;


  quit_flag = initialise ();

  parse_command_line (argc, argv);

  poll_loop ();

  msgs_close_file ();
  wimp_close_down (task_handle);

  return (0);
}

/* ==================================================================================================================
 * Wimp_Poll loop
 */

int poll_loop (void)
{
  file_data         *file;
  os_t              poll_time;
  wimp_pointer      ptr;
  char              buffer[1024], *pathcopy;

  wimp_block        blk;
  extern int        quit_flag;
  extern int        global_drag_type;


  poll_time = os_read_monotonic_time ();

  while (!quit_flag)
  {
    switch (wimp_poll_idle (0, &blk, poll_time, NULL))
    {
      case wimp_NULL_REASON_CODE:
        update_files_for_new_date ();
        poll_time += 6000; /* Wait for a minute for the next Null poll */
        break;

      case wimp_REDRAW_WINDOW_REQUEST:
        if ((file = find_transaction_window_file_block (blk.redraw.w)) != NULL)
        {
          redraw_transaction_window (&(blk.redraw), file);
        }
        else if ((file = find_account_window_file_block (blk.redraw.w)) != NULL)
        {
          redraw_account_window (&(blk.redraw), file);
        }
        else if ((file = find_accview_window_file_block (blk.redraw.w)) != NULL)
        {
          redraw_accview_window (&(blk.redraw), file);
        }
        else if ((file = find_sorder_window_file_block (blk.redraw.w)) != NULL)
        {
          redraw_sorder_window (&(blk.redraw), file);
        }
        else if ((file = find_preset_window_file_block (blk.redraw.w)) != NULL)
        {
          redraw_preset_window (&(blk.redraw), file);
        }
        else if ((file = find_report_window_file_block (blk.redraw.w)) != NULL)
        {
          redraw_report_window (&(blk.redraw), file);
        }
        break;

      case wimp_OPEN_WINDOW_REQUEST:
        if ((file = find_transaction_window_file_block (blk.redraw.w)) != NULL)
        {
          minimise_transaction_window_extent (file);
        }
        wimp_open_window (&(blk.open));
        break;

      case wimp_CLOSE_WINDOW_REQUEST:
        if ((file = find_transaction_window_file_block (blk.close.w)) != NULL)
        {
          wimp_get_pointer_info (&ptr);

          /* If Adjust was clicked, find the pathname and open the parent directory. */

          if (ptr.buttons == wimp_CLICK_ADJUST && check_for_filepath (file))
          {
            pathcopy = (char *) malloc (strlen(file->filename)+1);

            if (pathcopy != NULL)
            {
              strcpy(pathcopy, file->filename);

              sprintf (buffer, "%%Filer_OpenDir %s", find_pathname (pathcopy));
              xos_cli (buffer);

              free (pathcopy);
            }
          }

          /* If it was NOT an Adjust click with Shift held down, close the file. */

          if (!((osbyte1(osbyte_IN_KEY, 0xfc, 0xff) == 0xff || osbyte1(osbyte_IN_KEY, 0xf9, 0xff) == 0xff) &&
                ptr.buttons == wimp_CLICK_ADJUST))
          {
            delete_file (file);
          }
        }
        else if ((file = find_account_window_file_block (blk.close.w)) != NULL)
        {
          delete_accounts_window (file, find_accounts_window_type_from_handle (file, blk.close.w));
        }
        else if ((file = find_accview_window_file_block (blk.close.w)) != NULL)
        {
          delete_accview_window (file, find_accview_window_from_handle (file, blk.close.w));
        }
        else if ((file = find_sorder_window_file_block (blk.close.w)) != NULL)
        {
          delete_sorder_window (file);
        }
        else if ((file = find_preset_window_file_block (blk.close.w)) != NULL)
        {
          delete_preset_window (file);
        }
        else if ((file = find_report_window_file_block (blk.close.w)) != NULL)
        {
          delete_report_window (file, find_report_window_from_handle (file, blk.close.w));
        }
        else
        {
          wimp_close_window (blk.close.w);
        }
       break;

      case wimp_MOUSE_CLICK:
        mouse_click_handler (&(blk.pointer));
        break;

      case wimp_KEY_PRESSED:
        key_press_handler (&(blk.key));
        break;

      case wimp_MENU_SELECTION:
        menu_selection_handler (&(blk.selection));
        break;

      case wimp_USER_DRAG_BOX:
        switch (global_drag_type)
        {
          case SAVE_DRAG:
            terminate_user_drag (&(blk.dragged));
            break;

          case ACCOUNT_DRAG:
            terminate_account_drag (&(blk.dragged));
            break;

          case COLUMN_DRAG:
            terminate_column_width_drag (&(blk.dragged));
            break;
        }
        break;

      case wimp_SCROLL_REQUEST:
        scroll_request_handler (&(blk.scroll));
        break;

      case wimp_LOSE_CARET:
        refresh_transaction_edit_line_icons (blk.caret.w, -1, -1);
        break;

      case wimp_USER_MESSAGE:
      case wimp_USER_MESSAGE_RECORDED:
        user_message_handler (&(blk.message));
        break;

      case wimp_USER_MESSAGE_ACKNOWLEDGE:
        bounced_message_handler (&(blk.message));
        break;
    }
  }

  return 0;
}

/* ==================================================================================================================
 * Mouse click handler
 */

void mouse_click_handler (wimp_pointer *pointer)
{
  int       result;
  file_data *file;

  extern global_windows windows;


  /* Iconbar icon. */

  if (pointer->w == wimp_ICON_BAR)
  {
    switch ((int) pointer->buttons)
    {
      case wimp_CLICK_SELECT:
        create_new_file ();
        break;

      case wimp_CLICK_MENU:
        open_iconbar_menu (pointer);
        break;
    }
  }

  /* Program information window. */

  else if (pointer->w == windows.prog_info)
  {
    char temp_buf[256];

    switch ((int) pointer->i)
    {
      case 8: /* Website. */
        msgs_lookup ("SupportURL:http://www.stevefryatt.org.uk/software/", temp_buf, sizeof (temp_buf));
        launch_url (temp_buf);
        if (pointer->buttons == wimp_CLICK_SELECT)
        {
          wimp_create_menu ((wimp_menu *) -1, 0, 0);
        }
        break;
    }
  }

  /* Import complete window. */

  else if (pointer->w == windows.import_comp)
  {
    switch ((int) pointer->i)
    {
       case ICOMP_ICON_CLOSE:
         close_import_complete_dialogue (FALSE);
         break;

       case ICOMP_ICON_LOG:
         close_import_complete_dialogue (TRUE);
         break;
    }
  }

  /* Choices window. */

  else if (pointer->w == windows.choices)
  {
    switch ((int) pointer->i)
    {
      case CHOICE_ICON_APPLY:
        read_choices_window ();
        if (pointer->buttons == wimp_CLICK_SELECT)
        {
          close_choices_window ();
        }
        break;

      case CHOICE_ICON_SAVE: /* Save */
        read_choices_window ();
        save_configuration ();
        if (pointer->buttons == wimp_CLICK_SELECT)
        {
          close_choices_window ();
        }
        break;

      case CHOICE_ICON_CANCEL: /* Cancel */
        if (pointer->buttons == wimp_CLICK_SELECT) /* Close window. */
        {
          close_choices_window ();
        }
        else if (pointer->buttons == wimp_CLICK_ADJUST) /* Reset window contents. */
        {
          set_choices_window ();
          redraw_choices_window ();
          replace_caret_in_window (windows.choices_pane[CHOICE_PANE_CURRENCY]);
        }
        break;

      case CHOICE_ICON_DEFAULT: /* Default */
        if (pointer->buttons == wimp_CLICK_SELECT)
        {
          restore_default_configuration ();
          redraw_choices_window ();
        }
        break;

      default:
        if (pointer->i >= CHOICE_ICON_SELECT && pointer->i < (CHOICE_ICON_SELECT + CHOICES_PANES))
        {
          change_choices_pane (pointer->i - CHOICE_ICON_SELECT);
          set_icon_selected (windows.choices, pointer->i, 1);
        }
        break;
    }
  }

  else if (pointer->w == windows.choices_pane[CHOICE_PANE_CURRENCY])
  {
    if (pointer->i == CHOICE_ICON_TERRITORYNUM)
    {
      set_icons_shaded_when_radio_on (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM, 10,
                                      CHOICE_ICON_FORMATFRAME, CHOICE_ICON_FORMATLABEL,
                                      CHOICE_ICON_DECIMALPLACELABEL, CHOICE_ICON_DECIMALPLACE,
                                      CHOICE_ICON_DECIMALPOINTLABEL, CHOICE_ICON_DECIMALPOINT,
                                      CHOICE_ICON_NEGFRAME, CHOICE_ICON_NEGLABEL,
                                      CHOICE_ICON_NEGMINUS, CHOICE_ICON_NEGBRACE);
      replace_caret_in_window (windows.choices_pane[CHOICE_PANE_CURRENCY]);
    }
  }

  else if (pointer->w == windows.choices_pane[CHOICE_PANE_SORDER])
  {
    if (pointer->i == CHOICE_ICON_TERRITORYSO)
    {
      set_icons_shaded_when_radio_on (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO, 9,
                                      CHOICE_ICON_WEEKENDFRAME, CHOICE_ICON_WEEKENDLABEL,
                                      CHOICE_ICON_SOSUN, CHOICE_ICON_SOMON, CHOICE_ICON_SOTUE, CHOICE_ICON_SOWED,
                                      CHOICE_ICON_SOTHU, CHOICE_ICON_SOFRI, CHOICE_ICON_SOSAT);
    }
  }

  else if (pointer->w == windows.choices_pane[CHOICE_PANE_PRINT])
  {
    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == CHOICE_ICON_STANDARD || pointer->i == CHOICE_ICON_FASTTEXT ||
         pointer->i == CHOICE_ICON_PORTRAIT || pointer->i == CHOICE_ICON_LANDSCAPE ||
         pointer->i == CHOICE_ICON_MINCH || pointer->i == CHOICE_ICON_MCM ||
         pointer->i == CHOICE_ICON_MMM))
    {
      set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], pointer->i, 1);
    }
  }

  else if (pointer->w == windows.choices_pane[CHOICE_PANE_REPORT])
  {
    if (pointer->i == CHOICE_ICON_NFONTMENU)
    {
      open_font_list_menu (pointer);
    }

    else if (pointer->i == CHOICE_ICON_BFONTMENU)
    {
      open_font_list_menu (pointer);
    }
  }

  else if (pointer->w == windows.choices_pane[CHOICE_PANE_TRANSACT])
  {
    if (pointer->i == CHOICE_ICON_HILIGHTMEN)
    {
      open_simple_colour_window (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTCOL);
      create_popup_menu ((wimp_menu *) windows.colours, pointer);
    }
  }

  else if (pointer->w == windows.choices_pane[CHOICE_PANE_ACCOUNT])
  {
    if (pointer->i == CHOICE_ICON_AHILIGHTMEN)
    {
      open_simple_colour_window (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTCOL);
      create_popup_menu ((wimp_menu *) windows.colours, pointer);
    }

    else if (pointer->i == CHOICE_ICON_SHILIGHTMEN)
    {
      open_simple_colour_window (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTCOL);
      create_popup_menu ((wimp_menu *) windows.colours, pointer);
    }

    else if (pointer->i == CHOICE_ICON_OHILIGHTMEN)
    {
      open_simple_colour_window (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHILIGHTCOL);
      create_popup_menu ((wimp_menu *) windows.colours, pointer);
    }
  }

  /* Save window. */

  else if (pointer->w == windows.save_as)
  {
    if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == 0) /* 'Save' button */
    {
      immediate_window_save ();
    }

    if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == 1) /* 'Cancel' button */
    {
      wimp_create_menu (NULL, 0, 0);
    }

    if (pointer->buttons == wimp_DRAG_SELECT && pointer->i == 3) /* File icon */
    {
      start_save_window_drag ();
    }
  }

  /* Edit Account Window. */

  else if (pointer->w == windows.edit_acct)
  {
    if (pointer->i == ACCT_EDIT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_acct);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_account_edit_window ();
      }
    }

    if (pointer->i == ACCT_EDIT_OK) /* 'OK' button */
    {
      if (!process_account_edit_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_acct);
      }
    }

    if (pointer->i == ACCT_EDIT_DELETE)
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !delete_account_from_edit_window ())
      {
        close_dialogue_with_caret (windows.edit_acct);
      }
    }
  }

  /* Edit Heading Window. */

  else if (pointer->w == windows.edit_hdr)
  {
    if (pointer->i == HEAD_EDIT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_hdr);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_heading_edit_window ();
      }
    }

    if (pointer->i == HEAD_EDIT_OK) /* 'OK' button */
    {
      if (!process_heading_edit_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_hdr);
      }
    }

    if (pointer->i == HEAD_EDIT_DELETE)
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !delete_account_from_edit_window ())
      {
        close_dialogue_with_caret (windows.edit_hdr);
      }
    }


    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == HEAD_EDIT_INCOMING || pointer->i == HEAD_EDIT_OUTGOING)) /* Radio icons */
    {
      set_icon_selected (windows.edit_hdr, pointer->i, 1);
    }
  }

  /* Edit Section Window. */

  else if (pointer->w == windows.edit_sect)
  {
    if (pointer->i == SECTION_EDIT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_sect);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_section_edit_window ();
      }
    }

    if (pointer->i == SECTION_EDIT_OK) /* 'OK' button */
    {
      if (!process_section_edit_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_sect);
      }
    }

    if (pointer->i == SECTION_EDIT_DELETE)
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !delete_section_from_edit_window ())
      {
        close_dialogue_with_caret (windows.edit_sect);
      }
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == SECTION_EDIT_HEADER || pointer->i == SECTION_EDIT_FOOTER)) /* Radio icons */
    {
      set_icon_selected (windows.edit_sect, pointer->i, 1);
    }
  }

  /* Edit Standing Order Window. */

  else if (pointer->w == windows.edit_sorder)
  {
    if (pointer->i == SORDER_EDIT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_sorder);
      }
      else
      {
        refresh_sorder_edit_window ();
      }
    }

    if (pointer->i == SORDER_EDIT_OK) /* 'OK' button */
    {
      if (!process_sorder_edit_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_sorder);
      }
    }

    if (pointer->i == SORDER_EDIT_STOP) /* Stop button */
    {
      result = stop_sorder_from_edit_window ();

      if (!result && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_sorder);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_sorder_edit_window ();
      }
    }

    if (pointer->i == SORDER_EDIT_DELETE) /* 'Delete' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !delete_sorder_from_edit_window ())
      {
        close_dialogue_with_caret (windows.edit_sorder);
      }
    }

    if (pointer->i == SORDER_EDIT_AVOID) /* Avoid radio icon */
    {
      set_icons_shaded_when_radio_off (windows.edit_sorder, SORDER_EDIT_AVOID, 2,
                                       SORDER_EDIT_SKIPFWD, SORDER_EDIT_SKIPBACK);
    }

    if (pointer->i == SORDER_EDIT_FIRSTSW) /* First amount radio icon */
    {
      set_icons_shaded_when_radio_off (windows.edit_sorder, SORDER_EDIT_FIRSTSW, 1,
                                       SORDER_EDIT_FIRST);
      replace_caret_in_window (windows.edit_sorder);
    }

    if (pointer->i == SORDER_EDIT_LASTSW) /* Last amount radio icon */
    {
      set_icons_shaded_when_radio_off (windows.edit_sorder, SORDER_EDIT_LASTSW, 1,
                                       SORDER_EDIT_LAST);
      replace_caret_in_window (windows.edit_sorder);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == SORDER_EDIT_FMNAME || pointer->i == SORDER_EDIT_TONAME))
    {
      open_sorder_edit_account_menu (pointer);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == SORDER_EDIT_FMREC || pointer->i == SORDER_EDIT_TOREC))
    {
      toggle_sorder_edit_reconcile_fields (pointer);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == SORDER_EDIT_PERDAYS || pointer->i == SORDER_EDIT_PERMONTHS ||
         pointer->i == SORDER_EDIT_PERYEARS || pointer->i == SORDER_EDIT_SKIPFWD ||
         pointer->i == SORDER_EDIT_SKIPBACK)) /* Radio icons */
    {
      set_icon_selected (windows.edit_sorder  , pointer->i, 1);
    }
  }

  /* Edit Preset Window. */

  else if (pointer->w == windows.edit_preset)
  {
    if (pointer->i == PRESET_EDIT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_preset);
      }
      else
      {
        refresh_preset_edit_window ();
      }
    }

    if (pointer->i == PRESET_EDIT_OK) /* 'OK' button */
    {
      if (!process_preset_edit_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.edit_preset);
      }
    }

    if (pointer->i == PRESET_EDIT_DELETE) /* 'Delete' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !delete_preset_from_edit_window ())
      {
        close_dialogue_with_caret (windows.edit_preset);
      }
    }

    if (pointer->i == PRESET_EDIT_TODAY) /* Today radio icon */
    {
      set_icons_shaded_when_radio_on (windows.edit_preset, PRESET_EDIT_TODAY, 1,
                                       PRESET_EDIT_DATE);
      replace_caret_in_window (windows.edit_preset);
    }

    if (pointer->i == PRESET_EDIT_CHEQUE) /* Cheque radio icon */
    {
      set_icons_shaded_when_radio_on (windows.edit_preset, PRESET_EDIT_CHEQUE, 1,
                                       PRESET_EDIT_REF);
      replace_caret_in_window (windows.edit_preset);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == PRESET_EDIT_FMNAME || pointer->i == PRESET_EDIT_TONAME))
    {
      open_preset_edit_account_menu (pointer);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == PRESET_EDIT_FMREC || pointer->i == PRESET_EDIT_TOREC))
    {
      toggle_preset_edit_reconcile_fields (pointer);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == PRESET_EDIT_CARETDATE || pointer->i == PRESET_EDIT_CARETFROM ||
         pointer->i == PRESET_EDIT_CARETTO || pointer->i == PRESET_EDIT_CARETREF ||
         pointer->i == PRESET_EDIT_CARETAMOUNT || pointer->i == PRESET_EDIT_CARETDESC)) /* Radio icons */
    {
      set_icon_selected (windows.edit_preset, pointer->i, 1);
    }
  }

  /* Find transaction window. */

  else if (pointer->w == windows.find)
  {
    if (pointer->i == FIND_ICON_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.find);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_find_window ();
      }
    }

    if (pointer->i == FIND_ICON_OK) /* 'OK' button */
    {
      if (!process_find_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.find);
      }
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == FIND_ICON_FMNAME || pointer->i == FIND_ICON_TONAME))
    {
      open_find_account_menu (pointer);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == FIND_ICON_FMREC || pointer->i == FIND_ICON_TOREC))
    {
      toggle_find_reconcile_fields (pointer);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == FIND_ICON_AND || pointer->i == FIND_ICON_OR ||
         pointer->i == FIND_ICON_START || pointer->i == FIND_ICON_DOWN ||
         pointer->i == FIND_ICON_END || pointer->i == FIND_ICON_UP)) /* Radio icons */
    {
      set_icon_selected (windows.find, pointer->i, 1);
    }
  }

  /* Found transaction window. */

  else if (pointer->w == windows.found)
  {
    if (pointer->i == FOUND_ICON_CANCEL && pointer->buttons == wimp_CLICK_SELECT)
    {
      wimp_close_window (windows.found);
    }
    else if (pointer->i == FOUND_ICON_PREVIOUS && pointer->buttons == wimp_CLICK_SELECT)
    {
      if (find_from_line (NULL, FIND_UP, NULL_TRANSACTION) == NULL_TRANSACTION)
      {
        wimp_close_window (windows.found);
      }
    }
    else if (pointer->i == FOUND_ICON_NEXT && pointer->buttons == wimp_CLICK_SELECT)
    {
      if (find_from_line (NULL, FIND_DOWN, NULL_TRANSACTION) == NULL_TRANSACTION)
      {
        wimp_close_window (windows.found);
      }
    }
    else if (pointer->i == FOUND_ICON_NEW && pointer->buttons == wimp_CLICK_SELECT)
    {
      reopen_find_window (pointer);
    }
  }

  /* Goto transaction window. */

  else if (pointer->w == windows.go_to)
  {
    if (pointer->i == GOTO_ICON_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.go_to);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_goto_window ();
      }
    }

    if (pointer->i == GOTO_ICON_OK) /* 'OK' button */
    {
      if (!process_goto_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.go_to);
      }
    }

    if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == GOTO_ICON_NUMBER || pointer->i == GOTO_ICON_DATE)) /* Radio icons */
    {
      set_icon_selected (windows.go_to, pointer->i, 1);
    }
  }

  /* Continuation window. */

  else if (pointer->w == windows.continuation)
  {
    if (pointer->i == CONTINUE_ICON_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.continuation);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_continue_window ();
      }
    }

    else if (pointer->i == CONTINUE_ICON_OK) /* 'OK' button */
    {
      if (!process_continue_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.continuation);
      }
    }

    else if (pointer->i == CONTINUE_ICON_TRANSACT)
    {
      set_icons_shaded_when_radio_off (windows.continuation, CONTINUE_ICON_TRANSACT, 2,
                                       CONTINUE_ICON_DATE, CONTINUE_ICON_DATETEXT);
      replace_caret_in_window (windows.continuation);
    }
  }

  /* Budget  window. */

  else if (pointer->w == windows.budget)
  {
    if (pointer->i == BUDGET_ICON_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.budget);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_budget_window ();
      }
    }

    if (pointer->i == BUDGET_ICON_OK) /* 'OK' button */
    {
      if (!process_budget_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.budget);
      }
    }
  }

  /* Report format window. */

  else if (pointer->w == windows.report_format)
  {
    if (pointer->i == REPORT_FORMAT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.report_format);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_report_format_window ();
      }
    }

    if (pointer->i == REPORT_FORMAT_OK) /* 'OK' button */
    {
      if (!process_report_format_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.report_format);
      }
    }

    if (pointer->i == REPORT_FORMAT_NFONTMENU)
    {
      open_font_list_menu (pointer);
    }

    else if (pointer->i == REPORT_FORMAT_BFONTMENU)
    {
      open_font_list_menu (pointer);
    }

  }

  /* Simple print window. */

  else if (pointer->w == windows.simple_print)
  {
    if (pointer->i == SIMPLE_PRINT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.simple_print);
        deregister_printer_update_handler (refresh_simple_print_window);

      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_simple_print_window ();
      }
    }

    else if (pointer->i == SIMPLE_PRINT_OK) /* 'OK' button */
    {
      process_simple_print_window ();
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.simple_print);
        deregister_printer_update_handler (refresh_simple_print_window);
      }
    }

    else if (pointer->i == SIMPLE_PRINT_STANDARD || pointer->i == SIMPLE_PRINT_FASTTEXT)
    {
      if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        set_icon_selected (windows.simple_print, pointer->i, 1);
      }

      set_icons_shaded_when_radio_off (windows.simple_print, SIMPLE_PRINT_STANDARD, 3,
                                       SIMPLE_PRINT_PORTRAIT, SIMPLE_PRINT_LANDSCAPE, SIMPLE_PRINT_SCALE);
      set_icons_shaded_when_radio_off (windows.simple_print, SIMPLE_PRINT_FASTTEXT, 1,
                                       SIMPLE_PRINT_TEXTFORMAT);
    }

    else if (pointer->i == SIMPLE_PRINT_PORTRAIT || pointer->i == SIMPLE_PRINT_LANDSCAPE)
    {
      if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        set_icon_selected (windows.simple_print, pointer->i, 1);
      }
    }
  }

  /* Date-range print window. */

  else if (pointer->w == windows.date_print)
  {
    if (pointer->i == DATE_PRINT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.date_print);
        deregister_printer_update_handler (refresh_date_print_window);

      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_date_print_window ();
      }
    }

    else if (pointer->i == DATE_PRINT_OK) /* 'OK' button */
    {
      process_date_print_window ();
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.date_print);
        deregister_printer_update_handler (refresh_date_print_window);
      }
    }

    else if (pointer->i == DATE_PRINT_STANDARD || pointer->i == DATE_PRINT_FASTTEXT)
    {
      if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        set_icon_selected (windows.date_print, pointer->i, 1);
      }

      set_icons_shaded_when_radio_off (windows.date_print, DATE_PRINT_STANDARD, 3,
                                       DATE_PRINT_PORTRAIT, DATE_PRINT_LANDSCAPE, DATE_PRINT_SCALE);
      set_icons_shaded_when_radio_off (windows.date_print, DATE_PRINT_FASTTEXT, 1,
                                       DATE_PRINT_TEXTFORMAT);
    }

    else if (pointer->i == DATE_PRINT_PORTRAIT || pointer->i == DATE_PRINT_LANDSCAPE)
    {
      if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        set_icon_selected (windows.date_print, pointer->i, 1);
      }
    }
  }

  /* Account name enrty window. */

  else if (pointer->w == windows.enter_acc)
  {
    if (pointer->i == ACC_NAME_ENTRY_OK)
    {
      process_account_lookup_window ();
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        wimp_create_menu ((wimp_menu *) -1, 0, 0);
      }
    }

    if (pointer->buttons == wimp_CLICK_ADJUST && pointer->i == ACC_NAME_ENTRY_NAME)
    {
      open_account_lookup_account_menu (pointer);
    }

    if (pointer->buttons == wimp_CLICK_ADJUST && pointer->i == ACC_NAME_ENTRY_REC)
    {
      toggle_account_lookup_reconcile_field (pointer);
    }

    if (pointer->i == ACC_NAME_ENTRY_CANCEL && pointer->buttons == wimp_CLICK_SELECT)
    {
      wimp_create_menu ((wimp_menu *) -1, 0, 0);
    }
  }

  /* Transaction report window. */

  else if (pointer->w == windows.trans_rep)
  {
    if (pointer->i == ANALYSIS_TRANS_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.trans_rep);
        analysis_force_close_report_rename_window (windows.trans_rep);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_trans_report_window ();
      }
    }

    else if (pointer->i == ANALYSIS_TRANS_OK) /* 'OK' button */
    {
      if (!process_trans_report_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.trans_rep);
        analysis_force_close_report_rename_window (windows.trans_rep);
      }
    }

    else if (pointer->i == ANALYSIS_TRANS_DELETE) /* 'Delete' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !analysis_delete_trans_report_window ())
      {
        close_dialogue_with_caret (windows.trans_rep);
      }
    }

    else if (pointer->i == ANALYSIS_TRANS_RENAME) /* 'Rename...' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        analysis_rename_trans_report_window (pointer);
      }
    }

    else if (pointer->i == ANALYSIS_TRANS_BUDGET)
    {
      set_icons_shaded_when_radio_on (windows.trans_rep, ANALYSIS_TRANS_BUDGET, 4,
                                      ANALYSIS_TRANS_DATEFROMTXT, ANALYSIS_TRANS_DATEFROM,
                                      ANALYSIS_TRANS_DATETOTXT, ANALYSIS_TRANS_DATETO);
      replace_caret_in_window (windows.trans_rep);
    }

    else if (pointer->i == ANALYSIS_TRANS_GROUP)
    {
      set_icons_shaded_when_radio_off (windows.trans_rep, ANALYSIS_TRANS_GROUP, 6,
                                  ANALYSIS_TRANS_PERIOD, ANALYSIS_TRANS_PTEXT,
                                  ANALYSIS_TRANS_PDAYS, ANALYSIS_TRANS_PMONTHS, ANALYSIS_TRANS_PYEARS,
                                  ANALYSIS_TRANS_LOCK);
      replace_caret_in_window (windows.trans_rep);
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_TRANS_PDAYS || pointer->i == ANALYSIS_TRANS_PMONTHS ||
         pointer->i == ANALYSIS_TRANS_PYEARS)) /* Radio icons */
    {
      set_icon_selected (windows.trans_rep, pointer->i, 1);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_TRANS_FROMSPECPOPUP)
    {
      open_trans_lookup_window (ANALYSIS_TRANS_FROMSPEC);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_TRANS_TOSPECPOPUP)
    {
      open_trans_lookup_window (ANALYSIS_TRANS_TOSPEC);
    }
  }

  /* Unreconciled report window. */

  else if (pointer->w == windows.unrec_rep)
  {
    if (pointer->i == ANALYSIS_UNREC_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.unrec_rep);
        analysis_force_close_report_rename_window (windows.unrec_rep);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_unrec_report_window ();
      }
    }

    else if (pointer->i == ANALYSIS_UNREC_OK) /* 'OK' button */
    {
      if (!process_unrec_report_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.unrec_rep);
        analysis_force_close_report_rename_window (windows.unrec_rep);
      }
    }

    else if (pointer->i == ANALYSIS_UNREC_DELETE) /* 'Delete' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !analysis_delete_unrec_report_window ())
      {
        close_dialogue_with_caret (windows.unrec_rep);
      }
    }

    else if (pointer->i == ANALYSIS_UNREC_RENAME) /* 'Rename...' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        analysis_rename_unrec_report_window (pointer);
      }
    }

    else if (pointer->i == ANALYSIS_UNREC_BUDGET)
    {
      set_icons_shaded_when_radio_on (windows.unrec_rep, ANALYSIS_UNREC_BUDGET, 4,
                                      ANALYSIS_UNREC_DATEFROMTXT, ANALYSIS_UNREC_DATEFROM,
                                      ANALYSIS_UNREC_DATETOTXT, ANALYSIS_UNREC_DATETO);
      replace_caret_in_window (windows.unrec_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUP)
    {
      set_icons_shaded_when_radio_off (windows.unrec_rep, ANALYSIS_UNREC_GROUP, 2,
                                       ANALYSIS_UNREC_GROUPACC, ANALYSIS_UNREC_GROUPDATE);

      set_icons_shaded (windows.unrec_rep,
                         !(read_icon_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP) &&
                           read_icon_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPDATE)),
                        6,
                        ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
                        ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);

      replace_caret_in_window (windows.unrec_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUPACC || pointer->i == ANALYSIS_UNREC_GROUPDATE)
    {
      set_icon_selected (windows.unrec_rep, pointer->i, 1);

      set_icons_shaded (windows.unrec_rep,
                         !(read_icon_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP) &&
                           read_icon_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPDATE)),
                        6,
                        ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
                        ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);

      replace_caret_in_window (windows.unrec_rep);
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_UNREC_PDAYS || pointer->i == ANALYSIS_UNREC_PMONTHS ||
         pointer->i == ANALYSIS_UNREC_PYEARS)) /* Radio icons */
    {
      set_icon_selected (windows.unrec_rep, pointer->i, 1);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_UNREC_FROMSPECPOPUP)
    {
      open_unrec_lookup_window (ANALYSIS_UNREC_FROMSPEC);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_UNREC_TOSPECPOPUP)
    {
      open_unrec_lookup_window (ANALYSIS_UNREC_TOSPEC);
    }
  }

  /* Cashflow report window. */

  else if (pointer->w == windows.cashflow_rep)
  {
    if (pointer->i == ANALYSIS_CASHFLOW_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.cashflow_rep);
        analysis_force_close_report_rename_window (windows.cashflow_rep);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_cashflow_report_window ();
      }
    }

    else if (pointer->i == ANALYSIS_CASHFLOW_OK) /* 'OK' button */
    {
      if (!process_cashflow_report_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.cashflow_rep);
        analysis_force_close_report_rename_window (windows.cashflow_rep);
      }
    }

    else if (pointer->i == ANALYSIS_CASHFLOW_DELETE) /* 'Delete' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !analysis_delete_cashflow_report_window ())
      {
        close_dialogue_with_caret (windows.cashflow_rep);
      }
    }

    else if (pointer->i == ANALYSIS_CASHFLOW_RENAME) /* 'Rename...' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        analysis_rename_cashflow_report_window (pointer);
      }
    }

    else if (pointer->i == ANALYSIS_CASHFLOW_BUDGET)
    {
      set_icons_shaded_when_radio_on (windows.cashflow_rep, ANALYSIS_CASHFLOW_BUDGET, 4,
                                      ANALYSIS_CASHFLOW_DATEFROMTXT, ANALYSIS_CASHFLOW_DATEFROM,
                                      ANALYSIS_CASHFLOW_DATETOTXT, ANALYSIS_CASHFLOW_DATETO);
      replace_caret_in_window (windows.cashflow_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUP)
    {
      set_icons_shaded_when_radio_off (windows.cashflow_rep, ANALYSIS_CASHFLOW_GROUP, 7,
                                       ANALYSIS_CASHFLOW_PERIOD, ANALYSIS_CASHFLOW_PTEXT, ANALYSIS_CASHFLOW_LOCK,
                                       ANALYSIS_CASHFLOW_PDAYS, ANALYSIS_CASHFLOW_PMONTHS, ANALYSIS_CASHFLOW_PYEARS,
                                       ANALYSIS_CASHFLOW_EMPTY);

      replace_caret_in_window (windows.cashflow_rep);
    }

     else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_CASHFLOW_PDAYS || pointer->i == ANALYSIS_CASHFLOW_PMONTHS ||
         pointer->i == ANALYSIS_CASHFLOW_PYEARS)) /* Radio icons */
    {
      set_icon_selected (windows.cashflow_rep, pointer->i, 1);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_CASHFLOW_ACCOUNTSPOPUP)
    {
      open_cashflow_lookup_window (ANALYSIS_CASHFLOW_ACCOUNTS);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_CASHFLOW_INCOMINGPOPUP)
    {
      open_cashflow_lookup_window (ANALYSIS_CASHFLOW_INCOMING);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_CASHFLOW_OUTGOINGPOPUP)
    {
      open_cashflow_lookup_window (ANALYSIS_CASHFLOW_OUTGOING);
    }
  }


  /* Balance report window. */

  else if (pointer->w == windows.balance_rep)
  {
    if (pointer->i == ANALYSIS_BALANCE_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.balance_rep);
        analysis_force_close_report_rename_window (windows.balance_rep);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_balance_report_window ();
      }
    }

    else if (pointer->i == ANALYSIS_BALANCE_OK) /* 'OK' button */
    {
      if (!process_balance_report_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.balance_rep);
        analysis_force_close_report_rename_window (windows.balance_rep);
      }
    }

    else if (pointer->i == ANALYSIS_BALANCE_DELETE) /* 'Delete' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT && !analysis_delete_balance_report_window ())
      {
        close_dialogue_with_caret (windows.balance_rep);
      }
    }

    else if (pointer->i == ANALYSIS_BALANCE_RENAME) /* 'Rename...' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        analysis_rename_balance_report_window (pointer);
      }
    }

    else if (pointer->i == ANALYSIS_BALANCE_BUDGET)
    {
      set_icons_shaded_when_radio_on (windows.balance_rep, ANALYSIS_BALANCE_BUDGET, 4,
                                      ANALYSIS_BALANCE_DATEFROMTXT, ANALYSIS_BALANCE_DATEFROM,
                                      ANALYSIS_BALANCE_DATETOTXT, ANALYSIS_BALANCE_DATETO);
      replace_caret_in_window (windows.balance_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUP)
    {
      set_icons_shaded_when_radio_off (windows.balance_rep, ANALYSIS_BALANCE_GROUP, 6,
                                       ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_PTEXT, ANALYSIS_BALANCE_LOCK,
                                       ANALYSIS_BALANCE_PDAYS, ANALYSIS_BALANCE_PMONTHS, ANALYSIS_BALANCE_PYEARS);

      replace_caret_in_window (windows.balance_rep);
    }

     else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_BALANCE_PDAYS || pointer->i == ANALYSIS_BALANCE_PMONTHS ||
         pointer->i == ANALYSIS_BALANCE_PYEARS)) /* Radio icons */
    {
      set_icon_selected (windows.balance_rep, pointer->i, 1);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_BALANCE_ACCOUNTSPOPUP)
    {
      open_balance_lookup_window (ANALYSIS_BALANCE_ACCOUNTS);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_BALANCE_INCOMINGPOPUP)
    {
      open_balance_lookup_window (ANALYSIS_BALANCE_INCOMING);
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_BALANCE_OUTGOINGPOPUP)
    {
      open_balance_lookup_window (ANALYSIS_BALANCE_OUTGOING);
    }
  }

  /* Save report template window. */

  else if (pointer->w == windows.save_rep)
  {
    if (pointer->i == ANALYSIS_SAVE_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.save_rep);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_save_report_window ();
      }
    }

    else if (pointer->i == ANALYSIS_SAVE_OK) /* 'OK' button */
    {
      if (!process_save_report_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.save_rep);
      }
    }

    else if (pointer->buttons == wimp_CLICK_SELECT && pointer->i == ANALYSIS_SAVE_NAMEPOPUP)
    {
      analysis_open_save_report_popup_menu (pointer);
    }
  }

  /* Colour pick window. */

  else if (pointer->w == windows.colours)
  {
    if (pointer->i >= 1)
    {
      select_simple_colour_window (pointer->i - 1);

      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        wimp_create_menu (NULL, 0, 0);
      }
    }
  }

  /* Transaction Sort Window */

  else if (pointer->w == windows.sort_trans)
  {
    if (pointer->i == TRANS_SORT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_trans);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_transaction_sort_window ();
      }
    }

    else if (pointer->i == TRANS_SORT_OK) /* 'OK' button */
    {
      if (!process_transaction_sort_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_trans);
      }
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == TRANS_SORT_DATE || pointer->i == TRANS_SORT_FROM || pointer->i == TRANS_SORT_TO ||
         pointer->i == TRANS_SORT_REFERENCE || pointer->i == TRANS_SORT_AMOUNT ||
         pointer->i == TRANS_SORT_DESCRIPTION || pointer->i == TRANS_SORT_ASCENDING ||
         pointer->i == TRANS_SORT_DESCENDING)) /* Radio icons */
    {
      set_icon_selected (windows.sort_trans, pointer->i, 1);
    }
  }

  /* AccView Sort Window */

  else if (pointer->w == windows.sort_accview)
  {
    if (pointer->i == ACCVIEW_SORT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_accview);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_accview_sort_window ();
      }
    }

    else if (pointer->i == ACCVIEW_SORT_OK) /* 'OK' button */
    {
      if (!process_accview_sort_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_accview);
      }
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ACCVIEW_SORT_DATE || pointer->i == ACCVIEW_SORT_FROMTO ||
         pointer->i == ACCVIEW_SORT_REFERENCE || pointer->i == ACCVIEW_SORT_PAYMENTS ||
         pointer->i == ACCVIEW_SORT_RECEIPTS || pointer->i == ACCVIEW_SORT_BALANCE ||
         pointer->i == ACCVIEW_SORT_DESCRIPTION || pointer->i == ACCVIEW_SORT_ASCENDING ||
         pointer->i == ACCVIEW_SORT_DESCENDING)) /* Radio icons */
    {
      set_icon_selected (windows.sort_accview, pointer->i, 1);
    }
  }

  /* SOrder Sort Window */

  else if (pointer->w == windows.sort_sorder)
  {
    if (pointer->i == SORDER_SORT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_sorder);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_sorder_sort_window ();
      }
    }

    else if (pointer->i == SORDER_SORT_OK) /* 'OK' button */
    {
      if (!process_sorder_sort_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_sorder);
      }
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == SORDER_SORT_FROM || pointer->i == SORDER_SORT_TO ||
         pointer->i == SORDER_SORT_AMOUNT || pointer->i == SORDER_SORT_DESCRIPTION ||
         pointer->i == SORDER_SORT_NEXTDATE || pointer->i == SORDER_SORT_LEFT ||
         pointer->i == SORDER_SORT_ASCENDING || pointer->i == SORDER_SORT_DESCENDING)) /* Radio icons */
    {
      set_icon_selected (windows.sort_sorder, pointer->i, 1);
    }
  }

  /* Preset Sort Window */

  else if (pointer->w == windows.sort_preset)
  {
    if (pointer->i == PRESET_SORT_CANCEL) /* 'Cancel' button */
    {
      if (pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_preset);
      }
      else if (pointer->buttons == wimp_CLICK_ADJUST)
      {
        refresh_preset_sort_window ();
      }
    }

    else if (pointer->i == PRESET_SORT_OK) /* 'OK' button */
    {
      if (!process_preset_sort_window () && pointer->buttons == wimp_CLICK_SELECT)
      {
        close_dialogue_with_caret (windows.sort_preset);
      }
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == PRESET_SORT_FROM || pointer->i == PRESET_SORT_TO ||
         pointer->i == PRESET_SORT_AMOUNT || pointer->i == PRESET_SORT_DESCRIPTION ||
         pointer->i == PRESET_SORT_KEY || pointer->i == PRESET_SORT_NAME ||
         pointer->i == PRESET_SORT_ASCENDING || pointer->i == PRESET_SORT_DESCENDING)) /* Radio icons */
    {
      set_icon_selected (windows.sort_preset, pointer->i, 1);
    }
  }


  /* Look for transaction windows. */

  else if ((file = find_transaction_window_file_block (pointer->w)) != NULL)
  {
    transaction_window_click (file, pointer);
  }

  /* Look for transaction toolbars. */

  else if ((file = find_transaction_pane_file_block (pointer->w)) != NULL)
  {
    transaction_pane_click (file, pointer);
  }

  /* Look for account windows. */

  else if ((file = find_account_window_file_block (pointer->w)) != NULL)
  {
    account_window_click (file, pointer);
  }

  /* Look for account window toolbars. */

  else if ((file = find_account_pane_file_block (pointer->w)) != NULL)
  {
    account_pane_click (file, pointer);
  }

  /* Look for account view windows. */

  else if ((file = find_accview_window_file_block (pointer->w)) != NULL)
  {
    accview_window_click (file, pointer);
  }

  /* Look for account view window toolbars. */

  else if ((file = find_accview_pane_file_block (pointer->w)) != NULL)
  {
    accview_pane_click (file, pointer);
  }

  /* Look for standing order windows. */

  else if ((file = find_sorder_window_file_block (pointer->w)) != NULL)
  {
    sorder_window_click (file, pointer);
  }

  /* Look for standing order window toolbars. */

  else if ((file = find_sorder_pane_file_block (pointer->w)) != NULL)
  {
    sorder_pane_click (file, pointer);
  }

  /* Look for preset windows. */

  else if ((file = find_preset_window_file_block (pointer->w)) != NULL)
  {
    preset_window_click (file, pointer);
  }

  /* Look for preset window toolbars. */

  else if ((file = find_preset_pane_file_block (pointer->w)) != NULL)
  {
    preset_pane_click (file, pointer);
  }

  /* Look for report windows. */

  else if ((file = find_report_window_file_block (pointer->w)) != NULL)
  {
    report_window_click (file, pointer);
  }
}

/* ==================================================================================================================
 * Keypress handler
 */

void key_press_handler (wimp_key *key)
{
  file_data *file;

  extern global_windows windows;


  /* Save window */

  if (key->w == windows.save_as)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        immediate_window_save ();
        break;

      case wimp_KEY_ESCAPE:
        wimp_create_menu (NULL, 0, 0);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Edit Account Window. */

  else if (key->w == windows.edit_acct)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_account_edit_window ())
        {
          close_dialogue_with_caret (windows.edit_acct);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.edit_acct);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Edit Heading Window. */

  else if (key->w == windows.edit_hdr)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_heading_edit_window ())
        {
          close_dialogue_with_caret (windows.edit_hdr);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.edit_hdr);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Edit standing order window. */

  else if (key->w == windows.edit_sorder)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_sorder_edit_window ())
        {
          close_dialogue_with_caret (windows.edit_sorder);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.edit_sorder);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }

    if (key->i == SORDER_EDIT_FMIDENT || key->i == SORDER_EDIT_TOIDENT)
    {
      update_sorder_edit_account_fields (key);
    }
  }

  /* Edit preset window. */

  else if (key->w == windows.edit_preset)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_preset_edit_window ())
        {
          close_dialogue_with_caret (windows.edit_preset);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.edit_preset);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }

    if (key->i == PRESET_EDIT_FMIDENT || key->i == PRESET_EDIT_TOIDENT)
    {
      update_preset_edit_account_fields (key);
    }
  }

  /* Edit Section Window. */

  else if (key->w == windows.edit_sect)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_section_edit_window ())
        {
          close_dialogue_with_caret (windows.edit_sect);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.edit_sect);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Find transaction window. */

  else if (key->w == windows.find)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_find_window ())
        {
          close_dialogue_with_caret (windows.find);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.find);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }

    if (key->i == FIND_ICON_FMIDENT || key->i == FIND_ICON_TOIDENT)
    {
      update_find_account_fields (key);
    }
  }

  /* Goto transaction window. */

  else if (key->w == windows.go_to)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_goto_window ())
        {
          close_dialogue_with_caret (windows.go_to);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.go_to);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Continuation window. */

  else if (key->w == windows.continuation)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_continue_window ())
        {
          close_dialogue_with_caret (windows.continuation);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.continuation);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Budget  window. */

  else if (key->w == windows.budget)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_budget_window ())
        {
          close_dialogue_with_caret (windows.budget);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.budget);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Report format window. */

  else if (key->w == windows.report_format)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_report_format_window ())
        {
          close_dialogue_with_caret (windows.report_format);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.report_format);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Simple print window. */

  else if (key->w == windows.simple_print)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        process_simple_print_window ();
        close_dialogue_with_caret (windows.simple_print);
        deregister_printer_update_handler (refresh_simple_print_window);
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.simple_print);
        deregister_printer_update_handler (refresh_simple_print_window);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Date-range print window. */

  else if (key->w == windows.date_print)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        process_date_print_window ();
        close_dialogue_with_caret (windows.date_print);
        deregister_printer_update_handler (refresh_date_print_window);
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.date_print);
        deregister_printer_update_handler (refresh_date_print_window);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Enter account name window. */

  else if (key->w == windows.enter_acc)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_account_lookup_window ())
        {
          wimp_create_menu ((wimp_menu *) -1, 0, 0);
        }
        break;

      default:
        wimp_process_key (key->c);
        break;
    }

    if (key->i == ACC_NAME_ENTRY_IDENT)
    {
      update_account_lookup_window (key);
    }

  }

  /* Transaction report window. */

  else if (key->w == windows.trans_rep)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_trans_report_window ())
        {
          close_dialogue_with_caret (windows.trans_rep);
          analysis_force_close_report_rename_window (windows.trans_rep);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.trans_rep);
        analysis_force_close_report_rename_window (windows.trans_rep);
        break;

      case wimp_KEY_F1:
        if (key->i == ANALYSIS_TRANS_FROMSPEC || key->i == ANALYSIS_TRANS_TOSPEC)
        {
          open_trans_lookup_window (key->i);
        }
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Unreconciled report window. */

  else if (key->w == windows.unrec_rep)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_unrec_report_window ())
        {
          close_dialogue_with_caret (windows.unrec_rep);
          analysis_force_close_report_rename_window (windows.unrec_rep);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.unrec_rep);
        analysis_force_close_report_rename_window (windows.unrec_rep);
        break;

      case wimp_KEY_F1:
        if (key->i == ANALYSIS_UNREC_FROMSPEC || key->i == ANALYSIS_UNREC_TOSPEC)
        {
          open_unrec_lookup_window (key->i);
        }
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Cashflow report window. */

  else if (key->w == windows.cashflow_rep)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_cashflow_report_window ())
        {
          close_dialogue_with_caret (windows.cashflow_rep);
          analysis_force_close_report_rename_window (windows.cashflow_rep);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.cashflow_rep);
        analysis_force_close_report_rename_window (windows.cashflow_rep);
        break;

      case wimp_KEY_F1:
        if (key->i == ANALYSIS_CASHFLOW_ACCOUNTS ||
            key->i == ANALYSIS_CASHFLOW_INCOMING ||
            key->i == ANALYSIS_CASHFLOW_OUTGOING)
        {
          open_cashflow_lookup_window (key->i);
        }
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Balance report window. */

  else if (key->w == windows.balance_rep)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_balance_report_window ())
        {
          close_dialogue_with_caret (windows.balance_rep);
          analysis_force_close_report_rename_window (windows.balance_rep);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.balance_rep);
        analysis_force_close_report_rename_window (windows.balance_rep);
        break;

      case wimp_KEY_F1:
        if (key->i == ANALYSIS_BALANCE_ACCOUNTS ||
            key->i == ANALYSIS_BALANCE_INCOMING ||
            key->i == ANALYSIS_BALANCE_OUTGOING)
        {
          open_balance_lookup_window (key->i);
        }
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Save report template window. */

  else if (key->w == windows.save_rep)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_save_report_window ())
        {
          close_dialogue_with_caret (windows.save_rep);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.save_rep);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Choices window. */

  else if (key->w == windows.choices ||
           key->w == windows.choices_pane[CHOICE_PANE_GENERAL] ||
           key->w == windows.choices_pane[CHOICE_PANE_CURRENCY] ||
           key->w == windows.choices_pane[CHOICE_PANE_SORDER] ||
           key->w == windows.choices_pane[CHOICE_PANE_PRINT] ||
           key->w == windows.choices_pane[CHOICE_PANE_REPORT])
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        read_choices_window ();
        close_choices_window ();
        break;

      case wimp_KEY_ESCAPE:
        close_choices_window ();
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Transaction Sort Window */

  else if (key->w == windows.sort_trans)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_transaction_sort_window ())
        {
          close_dialogue_with_caret (windows.sort_trans);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.sort_trans);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* AccView Sort Window */

  else if (key->w == windows.sort_accview)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_accview_sort_window ())
        {
          close_dialogue_with_caret (windows.sort_accview);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.sort_accview);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* SOrder Sort Window */

  else if (key->w == windows.sort_sorder)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_sorder_sort_window ())
        {
          close_dialogue_with_caret (windows.sort_sorder);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.sort_sorder);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }

  /* Preset Sort Window */

  else if (key->w == windows.sort_preset)
  {
    switch (key->c)
    {
      case wimp_KEY_RETURN:
        if (!process_preset_sort_window ())
        {
          close_dialogue_with_caret (windows.sort_preset);
        }
        break;

      case wimp_KEY_ESCAPE:
        close_dialogue_with_caret (windows.sort_preset);
        break;

      default:
        wimp_process_key (key->c);
        break;
    }
  }


  /* Look for transaction windows. */

  else if ((file = find_transaction_window_file_block (key->w)) != NULL)
  {
    transaction_window_keypress (file, key);
  }
}

/* ==================================================================================================================
 * Menu selection handler
 */

void menu_selection_handler (wimp_selection *selection)
{
  wimp_pointer          pointer;

  extern global_menus   menus;


  /* Store the mouse status before decoding the menu. */

  wimp_get_pointer_info (&pointer);

  /* Decode the icon-bar menu. */

  if (menus.menu_id == MENU_ID_ICONBAR)
  {
    decode_iconbar_menu (selection, &pointer);
  }

  /* Decode the main menu. */

  else if (menus.menu_id == MENU_ID_MAIN)
  {
    decode_main_menu (selection, &pointer);
  }

  /* Decode the account open menu. */

  else if (menus.menu_id == MENU_ID_ACCOPEN)
  {
    decode_accopen_menu (selection, &pointer);
  }

  /* Decode the account menu. */

  else if (menus.menu_id == MENU_ID_ACCOUNT)
  {
    decode_account_menu (selection, &pointer);
  }

  /* Decode the preset/date menu. */

  else if (menus.menu_id == MENU_ID_DATE)
  {
    decode_date_menu (selection, &pointer);
  }

  /* Decode the reference/description menu. */

  else if (menus.menu_id == MENU_ID_REFDESC)
  {
    decode_refdesc_menu (selection, &pointer);
  }

  /* Decode the account list menu. */

  else if (menus.menu_id == MENU_ID_ACCLIST)
  {
    decode_acclist_menu (selection, &pointer);
  }

  /* Decode the account view menu. */

  else if (menus.menu_id == MENU_ID_ACCVIEW)
  {
    decode_accview_menu (selection, &pointer);
  }

  /* Decode the standing order menu. */

  else if (menus.menu_id == MENU_ID_SORDER)
  {
    decode_sorder_menu (selection, &pointer);
  }

  /* Decode the preset window menu. */

  else if (menus.menu_id == MENU_ID_PRESET)
  {
    decode_preset_menu (selection, &pointer);
  }

  /* Decode the report view menu. */

  else if (menus.menu_id == MENU_ID_REPORTVIEW)
  {
    decode_reportview_menu (selection, &pointer);
  }

  /* Decode the report list menu when it appears apart from the main menu. */

  else if (menus.menu_id == MENU_ID_REPLIST)
  {
    mainmenu_decode_replist_menu (selection, &pointer);
  }

  /* Decode the transient font list menu. */

  else if (menus.menu_id == MENU_ID_FONTLIST)
  {
    decode_font_list_menu (selection, &pointer);
  }

  /* If Adjust was used, reopen the menu. */

  if (pointer.buttons == wimp_CLICK_ADJUST)
  {
    wimp_create_menu (menus.menu_up, 0, 0);
  }
}

/* ==================================================================================================================
 * Scroll request handler
 */

void scroll_request_handler (wimp_scroll *scroll)
{
  file_data *file;


  /* Check if the window is a transaction window. */

  if ((file = find_transaction_window_file_block (scroll->w)) != NULL)
  {
    transaction_window_scroll_event (file, scroll);
  }

  else if ((file = find_account_window_file_block (scroll->w)) != NULL)
  {
    account_window_scroll_event (file, scroll);
  }

  else if ((file = find_accview_window_file_block (scroll->w)) != NULL)
  {
    accview_window_scroll_event (file, scroll);
  }

  else if ((file = find_sorder_window_file_block (scroll->w)) != NULL)
  {
    sorder_window_scroll_event (file, scroll);
  }

  else if ((file = find_preset_window_file_block (scroll->w)) != NULL)
  {
    preset_window_scroll_event (file, scroll);
  }
}

/* ==================================================================================================================
 * User message handlers
 */

void user_message_handler (wimp_message *message)
{
  extern int          quit_flag;
  extern wimp_t       task_handle;
  extern global_menus menus;

  static char         *clipboard_data;
  int                 clipboard_size;


  switch (message->action)
  {
    case message_QUIT:
      quit_flag=TRUE;
      break;

    case message_PRE_QUIT:
      if (check_for_unsaved_files ())
      {
        message->your_ref = message->my_ref;
        wimp_send_message (wimp_USER_MESSAGE_ACKNOWLEDGE, message, message->sender);
      }
      break;

    case message_URI_RETURN_RESULT:
      url_bounce (message);
      break;

    case message_CLAIM_ENTITY:
      release_clipboard (message);
      break;

    case message_DATA_REQUEST:
      send_clipboard (message);
      break;

    case message_DATA_SAVE:
      if (message->your_ref != 0)
      {
        /* If your_ref != 0, we must have started the process so it must be the clipboard. */

        receive_reply_data_save_block (message, &clipboard_data);
      }
      else
      {
        /* Your_ref == 0, so this is a fresh approach and something wants us to load a file. */

        #ifdef DEBUG
        debug_reporter_text0 ("\\OLoading a file from another application");
        #endif

        if (initialise_data_load (message))
        {
          receive_reply_data_save_function (message, drag_end_load);
        }
      }
      break;

    case message_DATA_SAVE_ACK:
      send_reply_data_save_ack (message);
      break;

    case message_DATA_LOAD:
       if (message->your_ref != 0)
      {
        /* If your_ref != 0, there was a Message_DataSave before this.  clipboard_size will only return >0 if
         * the data is loaded automatically: since only the clipboard uses this, the following code is safe.
         * All other loads (data files, TSV and CSV) will have supplied a function to handle the physical loading
         * and clipboard_size will return as 0.
         */

        clipboard_size = receive_reply_data_load (message, NULL);
        if (clipboard_size > 0)
        {
          paste_received_clipboard (&clipboard_data, clipboard_size);
        }
      }
      else
      {
        #ifdef DEBUG
        debug_reporter_text0 ("\\OLoading a file from disc");
        #endif

        if (initialise_data_load (message))
        {
          receive_init_quick_data_load_function (message, drag_end_load);
          receive_reply_data_load (message, NULL);
        }
      }
      break;

    case message_RAM_FETCH:
      send_reply_ram_fetch (message, task_handle);
      break;

    case message_RAM_TRANSMIT:
      clipboard_size = recieve_reply_ram_transmit (message, NULL);
      if (clipboard_size > 0)
      {
        paste_received_clipboard (&clipboard_data, clipboard_size);
      }
      break;

    case message_DATA_OPEN:
      start_data_open_load (message);
      break;

    case message_PRINT_INIT:
    case message_SET_PRINTER:
      handle_message_set_printer ();
      break;

    case message_PRINT_ERROR:
      handle_message_print_error (message);
      break;

    case message_PRINT_FILE:
      handle_message_print_file (message);
      break;

    case message_HELP_REQUEST:
      send_reply_help_request (message);
      break;

    case message_MENUS_DELETED:
      if (menus.menu_id == MENU_ID_FONTLIST)
      {
        deallocate_font_list_menu ();
      }
      else if (menus.menu_id == MENU_ID_ACCOUNT)
      {
        account_menu_closed_message ((wimp_full_message_menus_deleted *) message);
      }
      break;

    case message_MENU_WARNING:
      if (menus.menu_id == MENU_ID_MAIN)
      {
        main_menu_submenu_message ((wimp_full_message_menu_warning *) message);
      }
      else if (menus.menu_id == MENU_ID_ACCLIST)
      {
        acclist_menu_submenu_message ((wimp_full_message_menu_warning *) message);
      }
      else if (menus.menu_id == MENU_ID_ACCVIEW)
      {
        accview_menu_submenu_message ((wimp_full_message_menu_warning *) message);
      }
      else if (menus.menu_id == MENU_ID_ACCOUNT)
      {
        account_menu_submenu_message ((wimp_full_message_menu_warning *) message);
      }
      else if (menus.menu_id == MENU_ID_SORDER)
      {
        sorder_menu_submenu_message ((wimp_full_message_menu_warning *) message);
      }
      else if (menus.menu_id == MENU_ID_PRESET)
      {
        preset_menu_submenu_message ((wimp_full_message_menu_warning *) message);
      }
      else if (menus.menu_id == MENU_ID_REPORTVIEW)
      {
        reportview_menu_submenu_message ((wimp_full_message_menu_warning *) message);
      }
      break;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void bounced_message_handler (wimp_message *message)
{
  switch (message->action)
  {
    case message_ANT_OPEN_URL:
      url_bounce (message);
      break;

    case message_RAM_TRANSMIT:
      wimp_msgtrans_error_report ("RAMXferFail");
      break;

    case message_RAM_FETCH:
      receive_bounced_ram_fetch (message);
      break;

    case message_PRINT_SAVE:
      handle_bounced_message_print_save ();
      break;
  }
}
