/* CashBook - mainmenu.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/windows.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "mainmenu.h"

#include "account.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "calculation.h"
#include "caret.h"
#include "choices.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "find.h"
#include "file.h"
#include "fileinfo.h"
#include "fontlist.h"
#include "goto.h"
#include "main.h"
#include "presets.h"
#include "purge.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"

/* ==================================================================================================================
 * Static global variables
 */

static file_data *main_menu_file = NULL; /* Point to the file block connected to the main menu. */
static int       main_menu_line = -1; /* Remember the line that a menu applies to. */
static int       main_menu_column = -1; /* Remember the column that a menu applies to. */

static wimp_w    account_menu_window = NULL;
static wimp_i    account_menu_name_icon = 0;
static wimp_i    account_menu_ident_icon = 0;
static wimp_i    account_menu_rec_icon = 0;


static int                refdesc_menu_type = 0; /* The type of reference or description menu open. */

/* ==================================================================================================================
 * General
 */


char *mainmenu_get_current_menu_name(char *buffer)
{
  extern global_menus menus;


  *buffer = '\0';

  if (menus.menu_id == MENU_ID_DATE)
  {
    strcpy (buffer, "DateMenu");
  }
  else if (menus.menu_id == MENU_ID_ACCOUNT)
  {
    strcpy (buffer, "AccountMenu");
  }
  else if (menus.menu_id == MENU_ID_REFDESC)
  {
    if (refdesc_menu_type == REFDESC_MENU_REFERENCE)
    {
      strcpy (buffer, "RefMenu");
    }
    else if (refdesc_menu_type == REFDESC_MENU_DESCRIPTION)
    {
      strcpy (buffer, "DescMenu");
    }
  }

  return (buffer);
}


/* ==================================================================================================================
 * Account menu -- List of accounts and headings to select from
 */

void set_account_menu (file_data *file)
{
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_account_menu (file_data *file, enum account_menu_type type, int line,
                        wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer)
{
  extern global_menus menus;

  menus.account = account_complete_menu_build(file, type);
  set_account_menu (file);

  menus.menu_up = menus_create_standard_menu (menus.account, pointer);
  menus.menu_id = MENU_ID_ACCOUNT;
  main_menu_file = file;
  main_menu_line = line;
  main_menu_column = type;

  account_menu_window = window;
  account_menu_name_icon = icon_n;
  account_menu_ident_icon = icon_i;
  account_menu_rec_icon = icon_r;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_account_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  int i, column, account;

  if (account_menu_window == NULL)
  {
    /* If the window is NULL, then the menu was opened over the transaction window. */

    /* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
     * This really ought to be in edit.c!
     */

    if (main_menu_line >= main_menu_file->trans_count && selection->items[0] != -1)
    {
      for (i=main_menu_file->trans_count; i<=main_menu_line; i++)
      {
        add_raw_transaction (main_menu_file,
                             NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, NULL_TRANS_FLAGS, NULL_CURRENCY, "", "");
      }
    }

    /* Again check that the transaction is in range.  If it isn't, the additions failed.
     *
     * Then change the transaction as instructed.
     */

    if (main_menu_line < main_menu_file->trans_count && selection->items[1] != -1)
    {
      switch (main_menu_column)
      {
        case ACCOUNT_MENU_FROM:
          column = EDIT_ICON_FROM;
          break;

        case ACCOUNT_MENU_TO:
          column = EDIT_ICON_TO;
          break;

        default:
          column = -1;
          break;
      }

      edit_change_transaction_account (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index, column,
                                       account_complete_menu_decode(selection));

      set_account_menu (main_menu_file);
    }
  }
  else
  {
    /* If the window is not NULL, the menu was opened over a dialogue box. */

    if (selection->items[1] != -1)
    {
      account = account_complete_menu_decode(selection);

      fill_account_field (main_menu_file, account, !(main_menu_file->accounts[account].type & ACCOUNT_FULL),
                          account_menu_window, account_menu_ident_icon, account_menu_name_icon, account_menu_rec_icon);

      wimp_set_icon_state (account_menu_window, account_menu_ident_icon, 0, 0);
      wimp_set_icon_state (account_menu_window, account_menu_name_icon, 0, 0);
      wimp_set_icon_state (account_menu_window, account_menu_rec_icon, 0, 0);

      icons_replace_caret_in_window (account_menu_window);
    }
  }

  if (pointer->buttons != wimp_CLICK_ADJUST)
  {
    close_account_lookup_account_menu ();
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void account_menu_submenu_message (wimp_full_message_menu_warning *submenu)
{
  wimp_menu *menu_block;

  if (submenu->selection.items[1] == -1)
  {
    menu_block = account_complete_submenu_build(submenu);
    wimp_create_sub_menu(menu_block, submenu->pos.x, submenu->pos.y);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void account_menu_closed_message (void)
{
  extern global_menus menus;
  extern global_windows windows;

  if (account_menu_window == windows.enter_acc)
  {
    close_account_lookup_account_menu ();
  }

  account_complete_menu_destroy();
}

/* ------------------------------------------------------------------------------------------------------------------ */



/* ==================================================================================================================
 * Date menu -- List of presets to select from
 */


void open_date_menu(file_data *file, int line, wimp_pointer *pointer)
{
	extern global_menus menus;

	menus.date = preset_complete_menu_build(file);

	if (menus.date == NULL)
		return;

	menus.menu_up = menus_create_standard_menu(menus.date, pointer);
	menus.menu_id = MENU_ID_DATE;
	main_menu_file = file;
	main_menu_line = line;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_date_menu(wimp_selection *selection, wimp_pointer *pointer)
{
	int i;

	if (main_menu_file == NULL)
		return;


  if (main_menu_file != NULL)
  {
    /* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
     * This really ought to be in edit.c!
     */

    if (main_menu_line >= main_menu_file->trans_count && selection->items[0] != -1)
    {
      for (i=main_menu_file->trans_count; i<=main_menu_line; i++)
      {
        add_raw_transaction (main_menu_file,
                             NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, NULL_TRANS_FLAGS, NULL_CURRENCY, "", "");
      }
    }

    /* Again check that the transaction is in range.  If it isn't, the additions failed.
     *
     * Then change the transaction as instructed.
     */

    if (main_menu_line < main_menu_file->trans_count && selection->items[0] != -1)
    {
      if (preset_complete_menu_decode(selection) == NULL_PRESET)
      {
        edit_change_transaction_date (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                      get_current_date ());

      }
      else
      {
        insert_transaction_preset_full (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        preset_complete_menu_decode(selection));
      }
    }
  }
}

void date_menu_closed_message(void)
{
	preset_complete_menu_destroy();
}


/* ==================================================================================================================
 * Ref Desc menu -- List of previous entries to choose from
 */


/* ------------------------------------------------------------------------------------------------------------------ */

void open_refdesc_menu (file_data *file, int menu_type, int line, wimp_pointer *pointer)
{
  extern global_menus menus;


  menus.refdesc = transact_complete_menu_build(file, menu_type, line);
  transact_complete_menu_prepare(line);

  menus.menu_up = menus_create_standard_menu (menus.refdesc, pointer);
  menus.menu_id = MENU_ID_REFDESC;
  main_menu_file = file;
  main_menu_line = line;
  refdesc_menu_type = menu_type;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_refdesc_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  int  i;
  char *field, cheque_buffer[REF_FIELD_LEN];


  if (main_menu_file != NULL)
  {
    /* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
     * This really ought to be in edit.c!
     */

    if (main_menu_line >= main_menu_file->trans_count && selection->items[0] != -1)
    {
      for (i=main_menu_file->trans_count; i<=main_menu_line; i++)
      {
        add_raw_transaction (main_menu_file,
                             NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, NULL_TRANS_FLAGS, NULL_CURRENCY, "", "");
      }
    }

    /* Again check that the transaction is in range.  If it isn't, the additions failed.
     *
     * Then change the transaction as instructed.
     */

    if (main_menu_line < main_menu_file->trans_count && selection->items[0] != -1)
    {
    	field = transact_complete_menu_decode(selection);

      if (refdesc_menu_type == REFDESC_MENU_REFERENCE && field == NULL)
      {
        get_next_cheque_number (main_menu_file,
                  main_menu_file->transactions[main_menu_file->transactions[main_menu_line].sort_index].from,
                  main_menu_file->transactions[main_menu_file->transactions[main_menu_line].sort_index].to,
                  1, cheque_buffer);
        edit_change_transaction_refdesc (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        EDIT_ICON_REF, cheque_buffer);
      }
      else if (refdesc_menu_type == REFDESC_MENU_REFERENCE && field != NULL)
      {
        edit_change_transaction_refdesc (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        EDIT_ICON_REF, field);

      }
      else if (refdesc_menu_type == REFDESC_MENU_DESCRIPTION && field != NULL)
      {
        edit_change_transaction_refdesc (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        EDIT_ICON_DESCRIPT, field);

      }
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refdesc_menu_closed_message(void)
{
	transact_complete_menu_destroy();
}

