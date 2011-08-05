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

static void      *transient_shared_data = NULL; /* A pointer to a block of shared data for transient menus. */

static file_data *main_menu_file = NULL; /* Point to the file block connected to the main menu. */
static int       main_menu_line = -1; /* Remember the line that a menu applies to. */
static int       main_menu_column = -1; /* Remember the column that a menu applies to. */




static wimp_w    account_menu_window = NULL;
static wimp_i    account_menu_name_icon = 0;
static wimp_i    account_menu_ident_icon = 0;
static wimp_i    account_menu_rec_icon = 0;

static date_menu_link     *date_menu = NULL; /* links from the date menu to presets. */

static acclist_menu_link  *account_link = NULL; /* Links from the menu to the accounts. */
static acclist_menu_group *account_group = NULL; /* Links from the parent menu to the submenus. */
static wimp_menu          *account_submenu = NULL; /* The account submenu block. */

static refdesc_menu_link  *refdesc_link = NULL;  /* Links from the refdesc menu to the entries. */
static int                refdesc_menu_type = 0; /* The type of reference or description menu open. */

static char               *account_title_buffer = NULL; /* Buffer for the accounts menu et al.. */

/* ==================================================================================================================
 * General
 */

/* Claim a block of memory as the shared transient block.  This is used by the various transient menus that
 * get built on the fly.
 *
 * Any prior claim is deallocated first.
 */

void *claim_transient_shared_memory (int amount)
{
  if (transient_shared_data != NULL)
  {
    heap_free (transient_shared_data);
    transient_shared_data = NULL;
  }

  transient_shared_data = (void *) heap_alloc (amount);

  return (transient_shared_data);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Extend the transient shared memory block, without deallocating first.  increase is the number of bytes to extend
 * the block by.
 */

void *extend_transient_shared_memory (int increase)
{
  if (transient_shared_data != NULL)
  {
    transient_shared_data = (void *) heap_extend (transient_shared_data, heap_size (transient_shared_data) + increase);
  }

  return (transient_shared_data);
}

/* ------------------------------------------------------------------------------------------------------------------ */

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

void open_account_menu (file_data *file, int type, int line,
                        wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer)
{
  unsigned include;
  char     *title;

  extern global_menus menus;


  switch (type)
  {
    case ACCOUNT_MENU_FROM:
      include = ACCOUNT_FULL | ACCOUNT_IN;
      title = "ViewAccMenuTitleFrom";
      break;

    case ACCOUNT_MENU_TO:
      include = ACCOUNT_FULL | ACCOUNT_OUT;
      title = "ViewAccMenuTitleTo";
      break;

    case ACCOUNT_MENU_ACCOUNTS:
      include = ACCOUNT_FULL;
      title = "ViewAccMenuTitleAcc";
      break;

    case ACCOUNT_MENU_INCOMING:
      include = ACCOUNT_IN;
      title = "ViewAccMenuTitleIn";
      break;

    case ACCOUNT_MENU_OUTGOING:
      include = ACCOUNT_OUT;
      title = "ViewAccMenuTitleOut";
      break;

    default:
      include = 0;
      title = "";
      break;
  }

  build_account_menu (file, include, title);
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
                                       account_link[selection->items[1]].account);

      set_account_menu (main_menu_file);
    }
  }
  else
  {
    /* If the window is not NULL, the menu was opened over a dialogue box. */

    if (selection->items[1] != -1)
    {
      account = account_link[selection->items[1]].account;

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
    menu_block = build_account_submenu (main_menu_file, submenu);
    wimp_create_sub_menu (menu_block, submenu->pos.x, submenu->pos.y);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void account_menu_closed_message (wimp_full_message_menus_deleted *menu_del)
{
  extern global_menus menus;
  extern global_windows windows;

  if (menu_del->menu == menus.account && account_menu_window == windows.enter_acc)
  {
    close_account_lookup_account_menu ();
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

wimp_menu *build_account_menu (file_data *file, unsigned include, char *title)
{
  int                 i, group, line, headers, entry, width, sublen, maxsublen, shade;
  int                 groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};
  void                *mem;

  extern global_menus menus;


  /* Find out how many accounts there are, by counting entries in the groups. */

  maxsublen = 0;
  headers = 0;

  /* for each group that will be included in the menu, count through the window definition. */

  for (group = 0; group < groups; group++)
  {
    if (include & sequence[group])
    {
      i = 0;
      sublen = 0;
      entry = find_accounts_window_entry_from_type (file, sequence[group]);

      while (i < file->account_windows[entry].display_lines)
      {
        /* If the line is a header, increment the header count, and start a new sub-menu. */

        if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER)
        {
          if (sublen > maxsublen)
          {
            maxsublen = sublen;
          }

          sublen = 0;
          headers++;
        }

        /* Else if the line is an account entry, increment the submenu length count.  If the line is the first in the
         * group, it must fall outwith any headers and so will require its own submenu.
         */

        else if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA)
        {
          sublen++;

          if (i == 0)
          {
            headers++;
          }
        }

      i++;
      }

      if (sublen > maxsublen)
      {
        maxsublen = sublen;
      }
    }
  }


  #ifdef DEBUG
  debug_printf ("\\GBuilding accounts menu for %d headers, maximum submenu of %d", headers, maxsublen);
  #endif

  /* Claim enough memory to build the menu in. */

  menus.account = NULL;
  account_group = NULL;
  account_submenu = NULL;
  account_link = NULL;

  mem = claim_transient_shared_memory (56 + 24 * (headers + maxsublen) +
                                       sizeof (acclist_menu_group) * headers +
                                       sizeof (acclist_menu_link) * maxsublen);

  if (mem != NULL)
  {
    if (headers > 0)
    {
      menus.account = (wimp_menu *) mem;
      mem += 28 + 24 * headers;

      account_group = (acclist_menu_group *) mem;
      mem += sizeof (acclist_menu_group) * headers;
    }

    if (maxsublen > 0)
    {
      account_submenu = (wimp_menu *) mem;
      mem += 28 + 24 * maxsublen;

      account_link = (acclist_menu_link *) mem;
    }
  }

  /* Populate the menu. */

  if (menus.account != NULL && account_group != NULL)
  {
    line = 0;
    width = 0;
    shade = 1;

    for (group = 0; group < groups; group++)
    {
      if (include & sequence[group])
      {
        i = 0;
        entry = find_accounts_window_entry_from_type (file, sequence[group]);

        /* Start the group with a separator if there are lines in the menu already. */

        if (line > 0)
        {
          menus.account->entries[line-1].menu_flags |= wimp_MENU_SEPARATE;
        }

        while (i < file->account_windows[entry].display_lines)
        {
          /* If the line is a section header, add it to the manu... */

          if (line < headers && file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER)
          {
            /* Test for i>0 because if this is the first line of a new entry, the last group of the last entry will
             * already have been dealt with at the end of the main loop.  shade will be 0 if there have been any
             * ACCOUNT_LINE_DATA since the last ACCOUNT_LINE_HEADER.
             */
            if (shade && line > 0 && i > 0)
            {
              menus.account->entries[line - 1].icon_flags |= wimp_ICON_SHADED;
            }
            shade = 1;

            /* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
             * well move while the menu is open.
             */

            strcpy (account_group[line].name, file->account_windows[entry].line_data[i].heading);
            if (strlen (account_group[line].name) > width)
            {
              width = strlen (account_group[line].name);
            }
            account_group[line].entry = entry;
            account_group[line].start_line = i+1;

            /* Set the menu and icon flags up. */

            menus.account->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

            menus.account->entries[line].sub_menu = account_submenu;
            menus.account->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                                      wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                                      wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

            /* Set the menu icon contents up. */

            menus.account->entries[line].data.indirected_text.text = account_group[line].name;
            menus.account->entries[line].data.indirected_text.validation = NULL;
            menus.account->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

            line++;
          }
          else if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA)
          {
            shade = 0;

            /* If this is the first line of the list, and it's a data line, there is no group header and a default
             * group will be required.
             */

            if (i == 0 && line < headers)
            {
              switch (sequence[group])
              {
                case ACCOUNT_FULL:
                  msgs_lookup ("ViewaccMenuAccs", account_group[line].name, ACCOUNT_SECTION_LEN);
                  break;

                case ACCOUNT_IN:
                  msgs_lookup ("ViewaccMenuHIn", account_group[line].name, ACCOUNT_SECTION_LEN);
                  break;

                case ACCOUNT_OUT:
                  msgs_lookup ("ViewaccMenuHOut", account_group[line].name, ACCOUNT_SECTION_LEN);
                  break;
              }
              if (strlen (account_group[line].name) > width)
              {
                width = strlen (account_group[line].name);
              }
              account_group[line].entry = entry;
              account_group[line].start_line = i;

              /* Set the menu and icon flags up. */

              menus.account->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

              menus.account->entries[line].sub_menu = account_submenu;
              menus.account->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                                        wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                                        wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

              /* Set the menu icon contents up. */

              menus.account->entries[line].data.indirected_text.text = account_group[line].name;
              menus.account->entries[line].data.indirected_text.validation = NULL;
              menus.account->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

              line++;
            }
          }

        i++;
        }

        /* Update the maximum submenu length count again. */

        if (shade && line > 0)
        {
          menus.account->entries[line - 1].icon_flags |= wimp_ICON_SHADED;
        }
      }
    }

    /* Finish off the menu, marking the last entry and filling in the header. */

    menus.account->entries[line - 1].menu_flags |= wimp_MENU_LAST;

    if (account_title_buffer == NULL)
    {
      account_title_buffer = (char *) malloc (ACCOUNT_MENU_TITLE_LEN);
    }
    msgs_lookup (title, account_title_buffer, ACCOUNT_MENU_TITLE_LEN);
    menus.account->title_data.indirected_text.text = account_title_buffer;
    menus.account->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
    menus.account->title_fg = wimp_COLOUR_BLACK;
    menus.account->title_bg = wimp_COLOUR_LIGHT_GREY;
    menus.account->work_fg = wimp_COLOUR_BLACK;
    menus.account->work_bg = wimp_COLOUR_WHITE;

    menus.account->width = (width + 1) * 16;
    menus.account->height = 44;
    menus.account->gap = 0;
  }

  return (menus.account);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Build a submenu for the account menu on the fly, using information and memory from build_account_menu().
 *
 * The memory to hold the menu has been allocated and is pointed to by account_submenu and account_link; if
 * either of these are NULL, the fucntion must refuse to run.
 */

wimp_menu *build_account_submenu (file_data *file, wimp_full_message_menu_warning *submenu)
{
  int                 i, line, entry, width;

  extern global_menus menus;


  if (account_submenu != NULL && account_link != NULL)
  {
    line = 0;
    width = 0;

    entry = account_group[submenu->selection.items[0]].entry;
    i = account_group[submenu->selection.items[0]].start_line;

    while (i < file->account_windows[entry].display_lines &&
           file->account_windows[entry].line_data[i].type != ACCOUNT_LINE_HEADER)
    {
      /* If the line is an account entry, add it to the manu... */

      if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA)
      {
        /* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
         * well move while the menu is open.
         */

        strcpy (account_link[line].name, file->accounts[file->account_windows[entry].line_data[i].account].name);
        if (strlen (account_link[line].name) > width)
        {
          width = strlen (account_link[line].name);
        }
        account_link[line].account = file->account_windows[entry].line_data[i].account;

        /* Set the menu and icon flags up. */

        account_submenu->entries[line].menu_flags = 0;

        account_submenu->entries[line].sub_menu = (wimp_menu *) -1;
        account_submenu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                                    wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                                    wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

        /* Set the menu icon contents up. */

        account_submenu->entries[line].data.indirected_text.text = account_link[line].name;
        account_submenu->entries[line].data.indirected_text.validation = NULL;
        account_submenu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

        line++;
      }

    i++;
    }

    account_submenu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

    account_submenu->title_data.indirected_text.text = account_group[submenu->selection.items[0]].name;
    account_submenu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
    account_submenu->title_fg = wimp_COLOUR_BLACK;
    account_submenu->title_bg = wimp_COLOUR_LIGHT_GREY;
    account_submenu->work_fg = wimp_COLOUR_BLACK;
    account_submenu->work_bg = wimp_COLOUR_WHITE;

    account_submenu->width = (width + 1) * 16;
    account_submenu->height = 44;
    account_submenu->gap = 0;
  }

  return (account_submenu);
}


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
      if (selection->items[0] == DATE_MENU_TODAY)
      {
        edit_change_transaction_date (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                      get_current_date ());

      }
      else if (date_menu[selection->items[0]].preset != NULL_PRESET)
      {
        insert_transaction_preset_full (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        date_menu[selection->items[0]].preset);
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

void set_refdesc_menu (file_data *file, int menu_type, int line)
{
  int account;

  extern global_menus menus;


  if (menus.refdesc != NULL && menu_type == REFDESC_MENU_REFERENCE)
  {
    if ((line < file->trans_count) &&
        ((account = file->transactions[file->transactions[line].sort_index].from) != NULL_ACCOUNT) &&
        (file->accounts[account].cheque_num_width > 0))
    {
      menus.refdesc->entries[0].icon_flags &= ~wimp_ICON_SHADED;
    }
    else
    {
      menus.refdesc->entries[0].icon_flags |= wimp_ICON_SHADED;
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_refdesc_menu (file_data *file, int menu_type, int line, wimp_pointer *pointer)
{
  extern global_menus menus;


  build_refdesc_menu (file, menu_type, line);
  set_refdesc_menu (file, menu_type, line);

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
  char cheque_buffer[REF_FIELD_LEN];


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
      if (refdesc_menu_type == REFDESC_MENU_REFERENCE && selection->items[0] == REFDESC_MENU_CHEQUE)
      {
        get_next_cheque_number (main_menu_file,
                  main_menu_file->transactions[main_menu_file->transactions[main_menu_line].sort_index].from,
                  main_menu_file->transactions[main_menu_file->transactions[main_menu_line].sort_index].to,
                  1, cheque_buffer);
        edit_change_transaction_refdesc (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        EDIT_ICON_REF, cheque_buffer);
      }
      else if (refdesc_menu_type == REFDESC_MENU_REFERENCE && selection->items[0] > REFDESC_MENU_CHEQUE)
      {
        edit_change_transaction_refdesc (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        EDIT_ICON_REF, refdesc_link[selection->items[0]].name);

      }
      else if (refdesc_menu_type == REFDESC_MENU_DESCRIPTION)
      {
        edit_change_transaction_refdesc (main_menu_file, main_menu_file->transactions[main_menu_line].sort_index,
                                        EDIT_ICON_DESCRIPT, refdesc_link[selection->items[0]].name);

      }
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

wimp_menu *build_refdesc_menu (file_data *file, int menu_type, int start_line)
{
  int                 i, range, line, width, items, max_items, item_limit, no_original;
  char                *title_token;

  extern global_menus menus;


  hourglass_on ();

  /* Claim enough memory to build the menu in. */

  menus.refdesc = NULL;
  refdesc_link = NULL;

  max_items = REFDESC_MENU_BLOCKSIZE;
  refdesc_link = (refdesc_menu_link *) claim_transient_shared_memory ((sizeof (refdesc_menu_link) * max_items));

  items = 0;

  item_limit = config_int_read("MaxAutofillLen");

  if (refdesc_link != NULL && menu_type == REFDESC_MENU_REFERENCE)
  {
    /* In the Reference menu, the first item needs to be the Cheque No. entry, so insert that manually. */

    msgs_lookup ("RefMenuChq", refdesc_link[items++].name, DESCRIPT_FIELD_LEN);
  }

  /* Bring the start line into range for the current file.  no_original is set true if the line fell off the end
   * of the file, as this needs to be a special case of "no text".  If not, lines off the end of the file will
   * be matched against the final transaction as a result of pulling start_line into range.
   */

  if (start_line >= file->trans_count)
  {
    start_line = file->trans_count - 1;
    no_original = 1;
  }
  else
  {
    no_original = 0;
  }

  if (file->trans_count > 0 && refdesc_link != NULL)
  {
    /* Find the largest range from the current line to one end of the transaction list. */

    range = ((file->trans_count - start_line - 1) > start_line) ? (file->trans_count - start_line - 1) : start_line;

    /* Work out from the line to the edges of the transaction window. For each transaction, check the entries
     * and add them into the list.
     */

    if (menu_type == REFDESC_MENU_REFERENCE)
    {
      for (i=1; i<=range && (item_limit == 0 || items <= item_limit); i++)
      {
        if (start_line+i < file->trans_count)
        {
          if (no_original || (*(file->transactions[file->transactions[start_line].sort_index].reference) == '\0') ||
              (string_nocase_strstr (file->transactions[file->transactions[start_line+i].sort_index].reference,
                               file->transactions[file->transactions[start_line].sort_index].reference) ==
                               file->transactions[file->transactions[start_line+i].sort_index].reference))
          {
            mainmenu_add_refdesc_menu_entry (&refdesc_link, &items, &max_items,
                               file->transactions[file->transactions[start_line+i].sort_index].reference);
          }
        }
        if (start_line-i >= 0)
        {
          if (no_original || (*(file->transactions[file->transactions[start_line].sort_index].reference) == '\0') ||
              (string_nocase_strstr (file->transactions[file->transactions[start_line-i].sort_index].reference,
                               file->transactions[file->transactions[start_line].sort_index].reference) ==
                               file->transactions[file->transactions[start_line-i].sort_index].reference))
          {
            mainmenu_add_refdesc_menu_entry (&refdesc_link, &items, &max_items,
                               file->transactions[file->transactions[start_line-i].sort_index].reference);
          }
        }
      }
    }
    else if (menu_type == REFDESC_MENU_DESCRIPTION)
    {
      for (i=1; i<=range && (item_limit == 0 || items < item_limit); i++)
      {
        if (start_line+i < file->trans_count)
        {
          if (no_original || (*(file->transactions[file->transactions[start_line].sort_index].description) == '\0') ||
              (string_nocase_strstr (file->transactions[file->transactions[start_line+i].sort_index].description,
                               file->transactions[file->transactions[start_line].sort_index].description) ==
                               file->transactions[file->transactions[start_line+i].sort_index].description))
          {
            mainmenu_add_refdesc_menu_entry (&refdesc_link, &items, &max_items,
                               file->transactions[file->transactions[start_line+i].sort_index].description);
          }
        }
        if (start_line-i >= 0)
        {
          if (no_original || (*(file->transactions[file->transactions[start_line].sort_index].description) == '\0') ||
              (string_nocase_strstr (file->transactions[file->transactions[start_line-i].sort_index].description,
                               file->transactions[file->transactions[start_line].sort_index].description) ==
                               file->transactions[file->transactions[start_line-i].sort_index].description))
          {
            mainmenu_add_refdesc_menu_entry (&refdesc_link, &items, &max_items,
                               file->transactions[file->transactions[start_line-i].sort_index].description);
          }
        }
      }
    }
  }

  /* If there are items in the menu, claim the extra memory required to build the Wimp menu structure and
   * set up the pointers.   If there are not, menus.refdesc will remain NULL and the menu won't exist.
   *
   * refdesc_link may be NULL if memory allocation failed at any stage of the build process.
   */

  if (refdesc_link != NULL && items > 0)
  {
    refdesc_link = extend_transient_shared_memory (28 + 24 * max_items);
    menus.refdesc = (wimp_menu *) (refdesc_link + max_items);
  }

  /* Populate the menu. */

  line = 0;
  width = 0;

  if (menus.refdesc != NULL && refdesc_link != NULL)
  {
    if (menu_type == REFDESC_MENU_REFERENCE)
    {
      qsort (refdesc_link + 1, items - 1, sizeof (refdesc_menu_link), mainmenu_cmp_refdesc_menu_entries);
    }
    else
    {
      qsort (refdesc_link, items, sizeof (refdesc_menu_link), mainmenu_cmp_refdesc_menu_entries);
    }

    if (items > 0)
    {
      for (i=0; i < items; i++)
      {
        if (strlen (refdesc_link[line].name) > width)
        {
          width = strlen (refdesc_link[line].name);
        }

        /* Set the menu and icon flags up. */

        if (menu_type == REFDESC_MENU_REFERENCE && i == REFDESC_MENU_CHEQUE)
        {
          menus.refdesc->entries[line].menu_flags = (items > 1) ? wimp_MENU_SEPARATE : 0;
        }
        else
        {
          menus.refdesc->entries[line].menu_flags = 0;
        }

        menus.refdesc->entries[line].sub_menu = (wimp_menu *) -1;
        menus.refdesc->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                                  wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                                  wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

        /* Set the menu icon contents up. */

        menus.refdesc->entries[line].data.indirected_text.text = refdesc_link[line].name;
        menus.refdesc->entries[line].data.indirected_text.validation = NULL;
        menus.refdesc->entries[line].data.indirected_text.size = DESCRIPT_FIELD_LEN;

        line++;
      }
    }

    /* Finish off the menu, marking the last entry and filling in the header. */

    menus.refdesc->entries[(line > 0) ? line-1 : 0].menu_flags |= wimp_MENU_LAST;

    if (account_title_buffer == NULL)
    {
      account_title_buffer = (char *) malloc (ACCOUNT_MENU_TITLE_LEN);
    }

    switch (menu_type)
    {
      case REFDESC_MENU_REFERENCE:
        title_token = "RefMenuTitle";
        break;

      case REFDESC_MENU_DESCRIPTION:
      default:
        title_token = "DescMenuTitle";
        break;
    }
    msgs_lookup (title_token, account_title_buffer, ACCOUNT_MENU_TITLE_LEN);

    menus.refdesc->title_data.indirected_text.text = account_title_buffer;
    menus.refdesc->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
    menus.refdesc->title_fg = wimp_COLOUR_BLACK;
    menus.refdesc->title_bg = wimp_COLOUR_LIGHT_GREY;
    menus.refdesc->work_fg = wimp_COLOUR_BLACK;
    menus.refdesc->work_bg = wimp_COLOUR_WHITE;

    menus.refdesc->width = (width + 1) * 16;
    menus.refdesc->height = 44;
    menus.refdesc->gap = 0;
  }

  hourglass_off ();

  return (menus.refdesc);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void mainmenu_add_refdesc_menu_entry (refdesc_menu_link **entries, int *count, int *max, char *new)
{
  int  found, i;

  found = 0;

  if (*entries != NULL && *new != '\0')
  {
    for (i=0; (i < *count) && (found == 0); i++)
    {
      if (string_nocase_strcmp ((*entries)[i].name, new) == 0)
      {
        found = 1;
      }
    }

    if (!found && *count < (*max))
    {
      strcpy((*entries)[(*count)++].name, new);
    }

    /* Extend the block *after* the copy, in anticipation of the next call, because this could easily move the
     * flex blocks around and that would invalidate the new pointer...
     */

    if (*count >= (*max))
    {
      *entries = extend_transient_shared_memory (sizeof (refdesc_menu_link) * REFDESC_MENU_BLOCKSIZE);
      *max += REFDESC_MENU_BLOCKSIZE;
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

int mainmenu_cmp_refdesc_menu_entries (const void *va, const void *vb)
{
  refdesc_menu_link *a = (refdesc_menu_link *) va, *b = (refdesc_menu_link *) vb;

  return (string_nocase_strcmp(a->name, b->name));
}

