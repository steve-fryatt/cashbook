/* CashBook - mainmenu.c
 *
 * (C) Stephen Fryatt, 2003
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
#include "continue.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "find.h"
#include "file.h"
#include "fileinfo.h"
#include "goto.h"
#include "presets.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"

/* ==================================================================================================================
 * Static global variables
 */

static void      *transient_shared_data = NULL; /* A pointer to a block of shared data for transient menus. */

static file_data *main_menu_file = NULL; /* Point to the file block connected to the main menu. */
static int       acclist_menu_type = ACCOUNT_NULL; /* Remember the account type for an acclist menu. */
static int       accview_menu_account = NULL_ACCOUNT; /* Remember the account for an accview menu. */
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
static char               *replist_title_buffer = NULL; /* Buffer for the Replist menu (coexists with AccList). */

static report_data *report_menu_block = NULL;

static saved_report_menu_link *replist_link = NULL;

static byte      *font_buf1 = NULL;
static byte      *font_buf2 = NULL;
static wimp_w    font_window;
static wimp_i    font_icon;

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

char *get_current_menu_name (char *buffer)
{
  extern global_menus menus;


  *buffer = '\0';

  if (menus.menu_id == MENU_ID_MAIN)
  {
    strcpy (buffer, "MainMenu");
  }
  else if (menus.menu_id == MENU_ID_ICONBAR)
  {
    strcpy (buffer, "IconBarMenu");
  }
  else if (menus.menu_id == MENU_ID_ACCOPEN)
  {
    strcpy (buffer, "AccOpenMenu");
  }
  else if (menus.menu_id == MENU_ID_DATE)
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
  else if (menus.menu_id == MENU_ID_ACCLIST)
  {
    if (acclist_menu_type == ACCOUNT_FULL)
    {
      strcpy (buffer, "AccListMenu");
    }
    else if (acclist_menu_type == ACCOUNT_IN || acclist_menu_type == ACCOUNT_OUT)
    {
      strcpy (buffer, "HeadListMenu");
    }
  }
  else if (menus.menu_id == MENU_ID_ACCVIEW)
  {
    if (main_menu_file->accounts[accview_menu_account].type == ACCOUNT_FULL)
    {
      strcpy (buffer, "AccViewMenu");
    }
    else if (main_menu_file->accounts[accview_menu_account].type == ACCOUNT_IN ||
             main_menu_file->accounts[accview_menu_account].type == ACCOUNT_OUT)
    {
      strcpy (buffer, "HeadViewMenu");
    }
  }
  else if (menus.menu_id == MENU_ID_SORDER)
  {
    strcpy (buffer, "SOrderMenu");
  }
  else if (menus.menu_id == MENU_ID_PRESET)
  {
    strcpy (buffer, "PresetMenu");
  }
  else if (menus.menu_id == MENU_ID_REPORTVIEW)
  {
    strcpy (buffer, "ReportMenu");
  }
  else if (menus.menu_id == MENU_ID_REPLIST)
  {
    strcpy (buffer, "RepListMenu");
  }
  else if (menus.menu_id == MENU_ID_FONTLIST)
  {
    strcpy (buffer, "FontMenu");
  }

  return (buffer);
}

/* ==================================================================================================================
 * Main Menu
 */

/* Set and open the menu. */

void set_main_menu (file_data *file)
{
  extern global_menus   menus;

  menus_tick_entry (menus.transaction_sub, MAIN_MENU_TRANS_RECONCILE, file->auto_reconcile);
  menus_shade_entry (menus.account_sub, MAIN_MENU_ACCOUNTS_VIEW, count_accounts_in_file (file, ACCOUNT_FULL) == 0);
  menus_shade_entry (menus.analysis_sub, MAIN_MENU_ANALYSIS_SAVEDREP, file->saved_report_count == 0);
  set_accopen_menu (file);
  mainmenu_set_replist_menu (file);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_main_menu (file_data *file, wimp_pointer *pointer)
{
  extern global_menus   menus;


  build_accopen_menu (file);
  mainmenu_build_replist_menu (file, 0);

  /* If the submenus concerned are greyed out, give them a valid submenu pointer so that the arrow shows. */

  if (file->account_count == 0)
  {
    menus.account_sub->entries[MAIN_MENU_ACCOUNTS_VIEW].sub_menu = menus.icon_bar;
  }
  if (file->saved_report_count == 0)
  {
    menus.analysis_sub->entries[MAIN_MENU_ANALYSIS_SAVEDREP].sub_menu = menus.icon_bar;
  }

  initialise_save_boxes (file, 0, 0);
  set_main_menu (file);

  menus.menu_up = menus_create_standard_menu (menus.main, pointer);
  menus.menu_id = MENU_ID_MAIN;
  main_menu_file = file;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_main_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  /* File submenu */

  if (selection->items[0] == MAIN_MENU_SUB_FILE)
  {
    if (selection->items[1] == MAIN_MENU_FILE_SAVE) /* Save */
    {
      start_direct_menu_save (main_menu_file);
    }

    else if (selection->items[1] == MAIN_MENU_FILE_CONTINUE)
    {
      open_continue_window (main_menu_file, pointer, config_opt_read ("RememberValues"));
    }

    else if (selection->items[1] == MAIN_MENU_FILE_PRINT)
    {
      open_transact_print_window (main_menu_file, pointer, config_opt_read ("RememberValues"));
    }
  }

  /* Account submenu */

  else if (selection->items[0] == MAIN_MENU_SUB_ACCOUNTS)
  {
    if (selection->items[1] == MAIN_MENU_ACCOUNTS_VIEW && selection->items[2] != -1) /* View */
    {
      create_accview_window (main_menu_file, account_link[selection->items[2]].account);
    }

    else if (selection->items[1] == MAIN_MENU_ACCOUNTS_LIST) /* List */
    {
      create_accounts_window (main_menu_file, ACCOUNT_FULL);
    }

    else if (selection->items[1] == MAIN_MENU_ACCOUNTS_NEW) /* New... */
    {
      open_account_edit_window (main_menu_file, -1, ACCOUNT_FULL, pointer);
    }
  }

   /* Headings submenu */

  else if (selection->items[0] == MAIN_MENU_SUB_HEADINGS)
  {
    if (selection->items[1] == MAIN_MENU_HEADINGS_LISTIN) /* Incoming */
    {
      create_accounts_window (main_menu_file, ACCOUNT_IN);
    }

    else if (selection->items[1] == MAIN_MENU_HEADINGS_LISTOUT) /* Outgoing */
    {
      create_accounts_window (main_menu_file, ACCOUNT_OUT);
    }

    else if (selection->items[1] == MAIN_MENU_HEADINGS_NEW) /* New... */
    {
      open_account_edit_window (main_menu_file, -1, ACCOUNT_IN, pointer);
    }
  }

   /* Transactions submenu */

  else if (selection->items[0] == MAIN_MENU_SUB_TRANS)
  {
    if (selection->items[1] == MAIN_MENU_TRANS_FIND)
    {
      open_find_window (main_menu_file, pointer, config_opt_read ("RememberValues"));
    }

    else if (selection->items[1] == MAIN_MENU_TRANS_GOTO) /* Goto */
    {
      open_goto_window (main_menu_file, pointer, config_opt_read ("RememberValues"));
    }

    else if (selection->items[1] == MAIN_MENU_TRANS_SORT) /* Sort */
    {
      open_transaction_sort_window (main_menu_file, pointer);
    }

    else if (selection->items[1] == MAIN_MENU_TRANS_AUTOVIEW) /* View SOs */
    {
      create_sorder_window (main_menu_file);
    }

    else if (selection->items[1] == MAIN_MENU_TRANS_AUTONEW) /* Add SOs */
    {
      open_sorder_edit_window (main_menu_file, NULL_SORDER, pointer);
    }

    else if (selection->items[1] == MAIN_MENU_TRANS_PRESET) /* View presets */
    {
      create_preset_window (main_menu_file);
    }

    else if (selection->items[1] == MAIN_MENU_TRANS_PRESETNEW) /* Add presets */
    {
      open_preset_edit_window (main_menu_file, NULL_PRESET, pointer);
    }

    else if (selection->items[1] == MAIN_MENU_TRANS_RECONCILE) /* Reconcile */
    {
      main_menu_file->auto_reconcile = !main_menu_file->auto_reconcile;
      set_icon_selected (main_menu_file->transaction_window.transaction_pane, TRANSACT_PANE_RECONCILE,
                         main_menu_file->auto_reconcile);
    }
  }

  /* Utilities submenu */

  else if (selection->items[0] == MAIN_MENU_SUB_UTILS)
  {
    if (selection->items[1] == MAIN_MENU_ANALYSIS_BUDGET)
    {
      open_budget_window (main_menu_file, pointer);
    }

    else if (selection->items[1] == MAIN_MENU_ANALYSIS_SAVEDREP && selection->items[2] != -1)
    {
      analysis_open_saved_report_dialogue (main_menu_file, pointer, replist_link[selection->items[2]].saved_report);
    }

    else if (selection->items[1] == MAIN_MENU_ANALYSIS_MONTHREP)
    {
      open_trans_report_window (main_menu_file, pointer, NULL_TEMPLATE, config_opt_read ("RememberValues"));
    }

    else if (selection->items[1] == MAIN_MENU_ANALYSIS_UNREC)
    {
      open_unrec_report_window (main_menu_file, pointer, NULL_TEMPLATE, config_opt_read ("RememberValues"));
    }

    else if (selection->items[1] == MAIN_MENU_ANALYSIS_CASHFLOW)
    {
      open_cashflow_report_window (main_menu_file, pointer, NULL_TEMPLATE, config_opt_read ("RememberValues"));
    }

    else if (selection->items[1] == MAIN_MENU_ANALYSIS_BALANCE)
    {
      open_balance_report_window (main_menu_file, pointer, NULL_TEMPLATE, config_opt_read ("RememberValues"));
    }

    else if (selection->items[1] == MAIN_MENU_ANALYSIS_SOREP)
    {
      generate_full_sorder_report (main_menu_file);
    }
  }

  set_main_menu (main_menu_file);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle submenu warnings. */

void main_menu_submenu_message (wimp_full_message_menu_warning *submenu)
{
  #ifdef DEBUG
  debug_reporter_text0 ("\\BReceived submenu warning message.");
  #endif

  switch (submenu->selection.items[0])
  {
    case MAIN_MENU_SUB_FILE: /* File submenu */
      switch (submenu->selection.items[1])
      {
        case MAIN_MENU_FILE_INFO: /* File info window */
          fill_file_info_window (main_menu_file);
          wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
          break;

        case MAIN_MENU_FILE_SAVE: /* File save window */
          fill_save_as_window (main_menu_file, SAVE_BOX_FILE);
          wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
          break;

        case MAIN_MENU_FILE_EXPCSV: /* CSV save window */
          fill_save_as_window (main_menu_file, SAVE_BOX_CSV);
          wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
          break;

        case MAIN_MENU_FILE_EXPTSV: /* TSV save window */
          fill_save_as_window (main_menu_file, SAVE_BOX_TSV);
          wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
          break;
      }
      break;
  }
}

/* ==================================================================================================================
 * Account open menu. -- A list of accounts only, to select a view from.
 */

void set_accopen_menu (file_data *file)
{
  int i;

  extern global_menus menus;

  if (menus.accopen != NULL)
  {
    i = 0;

    do
    {
      if (file->accounts[account_link[i].account].account_view != NULL)
      {
        menus.accopen->entries[i].menu_flags |= wimp_MENU_TICKED;
      }
      else
      {
        menus.accopen->entries[i].menu_flags &= ~wimp_MENU_TICKED;
      }
    }
    while ((menus.accopen->entries[i++].menu_flags & wimp_MENU_LAST) == 0);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_accopen_menu (file_data *file, wimp_pointer *pointer)
{
  extern global_menus   menus;

  build_accopen_menu (file);
  set_accopen_menu (file);

  menus.menu_up = menus_create_standard_menu (menus.accopen, pointer);
  menus.menu_id = MENU_ID_ACCOPEN;
  main_menu_file = file;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_accopen_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  /* File submenu */

  if (selection->items[0] != -1)
  {
    create_accview_window (main_menu_file, account_link[selection->items[0]].account);
  }

  set_accopen_menu (main_menu_file);
}

/* ------------------------------------------------------------------------------------------------------------------ */

wimp_menu *build_accopen_menu (file_data *file)
{
  int                 i, line, accounts, entry, width;
  void                *mem;

  extern global_menus menus;


  entry = find_accounts_window_entry_from_type (file, ACCOUNT_FULL);

  /* Find out how many accounts there are. */

  accounts = count_accounts_in_file (file, ACCOUNT_FULL);

  #ifdef DEBUG
  debug_printf ("\\GBuilding account menu for %d accounts", accounts);
  #endif

  /* Claim enough memory to build the menu in. */

  menus.accopen = NULL;
  account_link = NULL;

  if (accounts > 0)
  {
    mem = claim_transient_shared_memory (28 + (24 * accounts) + (sizeof (acclist_menu_link) * accounts));

    if (mem != NULL)
    {
      menus.accopen = mem;
      mem += 28 + (24 * accounts);

      account_link = mem;
    }
  }

  /* Populate the menu. */

  if (menus.accopen != NULL && account_link != NULL)
  {
    line = 0;
    i = 0;
    width = 0;

    while (line < accounts && i < file->account_windows[entry].display_lines)
    {
      /* If the line is an account, add it to the manu... */

      if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA)
      {
        /* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
         * well move while the menu is open.  The account number is also stored, to allow the account to be found.
         */

        strcpy (account_link[line].name, file->accounts[file->account_windows[entry].line_data[i].account].name);
        account_link[line].account = file->account_windows[entry].line_data[i].account;
        if (strlen (account_link[line].name) > width)
        {
          width = strlen (account_link[line].name);
        }

        /* Set the menu and icon flags up. */

        menus.accopen->entries[line].menu_flags = 0;

        menus.accopen->entries[line].sub_menu = (wimp_menu *) -1;
        menus.accopen->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                             wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                             wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

        /* Set the menu icon contents up. */

        menus.accopen->entries[line].data.indirected_text.text = account_link[line].name;
        menus.accopen->entries[line].data.indirected_text.validation = NULL;
        menus.accopen->entries[line].data.indirected_text.size = ACCOUNT_NAME_LEN;

        #ifdef DEBUG
        debug_printf ("Line %d: '%s'", line, account_link[line].name);
        #endif

        line++;
      }

      /* If the line is a header, and the menu has an item in it, add a separator... */

      else if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER && line > 0)
      {
        menus.accopen->entries[line-1].menu_flags |= wimp_MENU_SEPARATE;
      }

    i++;
    }

    menus.accopen->entries[line - 1].menu_flags |= wimp_MENU_LAST;

    if (account_title_buffer == NULL)
    {
      account_title_buffer = (char *) malloc (ACCOUNT_MENU_TITLE_LEN);
    }
    msgs_lookup ("ViewaccMenuTitle", account_title_buffer, ACCOUNT_MENU_TITLE_LEN);
    menus.accopen->title_data.indirected_text.text = account_title_buffer;
    menus.accopen->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
    menus.accopen->title_fg = wimp_COLOUR_BLACK;
    menus.accopen->title_bg = wimp_COLOUR_LIGHT_GREY;
    menus.accopen->work_fg = wimp_COLOUR_BLACK;
    menus.accopen->work_bg = wimp_COLOUR_WHITE;

    menus.accopen->width = (width + 1) * 16;
    menus.accopen->height = 44;
    menus.accopen->gap = 0;
  }

  menus.account_sub->entries[MAIN_MENU_ACCOUNTS_VIEW].sub_menu = menus.accopen;

  return (menus.accopen);
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

      replace_caret_in_window (account_menu_window);
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

void set_date_menu (file_data *file)
{
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_date_menu (file_data *file, int line, wimp_pointer *pointer)
{
  extern global_menus menus;


  build_date_menu (file);
  set_date_menu (file);

  menus.menu_up = menus_create_standard_menu (menus.date, pointer);
  menus.menu_id = MENU_ID_DATE;
  main_menu_file = file;
  main_menu_line = line;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_date_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  int i;


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

/* ------------------------------------------------------------------------------------------------------------------ */

wimp_menu *build_date_menu (file_data *file)
{
  int                 i, line, width, p;
  void                *mem;

  extern global_menus menus;



  /* Claim enough memory to build the menu in. */

  menus.date = NULL;
  date_menu = NULL;

  mem = claim_transient_shared_memory ((28 + 24 * (file->preset_count + 1)) +
                                       (sizeof (date_menu_link) * (file->preset_count + 1)));

  if (mem != NULL)
  {
    menus.date = (wimp_menu *) mem;
    date_menu = (date_menu_link *) (mem + (28 + 24 * (file->preset_count + 1)));
  }

  /* Populate the menu. */

  line = 0;

  if (menus.date != NULL && date_menu != NULL)
  {
    /* Set up the today's date field. */

    msgs_lookup ("DateMenuToday", date_menu[line].name, PRESET_NAME_LEN);
    date_menu[line].preset = NULL_PRESET;

    width = strlen (date_menu[line].name);

    /* Set the menu and icon flags up. */

    menus.date->entries[line].menu_flags = (file->preset_count > 0) ? wimp_MENU_SEPARATE : 0;

    menus.date->entries[line].sub_menu = (wimp_menu *) -1;
    menus.date->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                              wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                              wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

    /* Set the menu icon contents up. */

    menus.date->entries[line].data.indirected_text.text = date_menu[line].name;
    menus.date->entries[line].data.indirected_text.validation = NULL;
    menus.date->entries[line].data.indirected_text.size = PRESET_NAME_LEN;

    if (file->preset_count > 0)
    {
      for (i=0; i<file->preset_count; i++)
      {
        line++;

        p = file->presets[i].sort_index;

        strcpy (date_menu[line].name, file->presets[p].name);
        date_menu[line].preset = p;

        if (strlen (date_menu[line].name) > width)
        {
          width = strlen (date_menu[line].name);
        }

        /* Set the menu and icon flags up. */

        menus.date->entries[line].menu_flags = 0;

        menus.date->entries[line].sub_menu = (wimp_menu *) -1;
        menus.date->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                                  wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                                  wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

        /* Set the menu icon contents up. */

        menus.date->entries[line].data.indirected_text.text = date_menu[line].name;
        menus.date->entries[line].data.indirected_text.validation = NULL;
        menus.date->entries[line].data.indirected_text.size = PRESET_NAME_LEN;
      }
    }

    /* Finish off the menu, marking the last entry and filling in the header. */

    menus.date->entries[line].menu_flags |= wimp_MENU_LAST;

    if (account_title_buffer == NULL)
    {
      account_title_buffer = (char *) malloc (ACCOUNT_MENU_TITLE_LEN);
    }
    msgs_lookup ("DateMenuTitle", account_title_buffer, ACCOUNT_MENU_TITLE_LEN);
    menus.date->title_data.indirected_text.text = account_title_buffer;
    menus.date->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
    menus.date->title_fg = wimp_COLOUR_BLACK;
    menus.date->title_bg = wimp_COLOUR_LIGHT_GREY;
    menus.date->work_fg = wimp_COLOUR_BLACK;
    menus.date->work_bg = wimp_COLOUR_WHITE;

    menus.date->width = (width + 1) * 16;
    menus.date->height = 44;
    menus.date->gap = 0;
  }

  return (menus.date);
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
              (strstr_no_case (file->transactions[file->transactions[start_line+i].sort_index].reference,
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
              (strstr_no_case (file->transactions[file->transactions[start_line-i].sort_index].reference,
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
              (strstr_no_case (file->transactions[file->transactions[start_line+i].sort_index].description,
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
              (strstr_no_case (file->transactions[file->transactions[start_line-i].sort_index].description,
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
      if (strcmp_no_case ((*entries)[i].name, new) == 0)
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

  return (strcmp_no_case(a->name, b->name));
}

/* ==================================================================================================================
 * Iconbar menu
 */

/* Set and open the icon bar menu. */

void set_iconbar_menu (void)
{
  return;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_iconbar_menu (wimp_pointer *pointer)
{
  extern global_menus   menus;


  set_iconbar_menu ();

  menus.menu_up = menus_create_iconbar_menu (menus.icon_bar, pointer);
  menus.menu_id = MENU_ID_ICONBAR;
}


/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_iconbar_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  extern int            quit_flag;


  if (selection->items[0] == ICONBAR_MENU_HELP) /* Help */
  {
    os_cli ("%Filer_Run <CashBook$Dir>.!Help");
  }
  if (selection->items[0] == ICONBAR_MENU_CHOICES) /* Choices... */
  {
    open_choices_window (pointer);
  }
  else if (selection->items[0] == ICONBAR_MENU_QUIT) /* Quit */
  {
    if (!check_for_unsaved_files ())
    {
      quit_flag = TRUE;
    }
  }

  set_iconbar_menu ();
}

/* ==================================================================================================================
 * Account list menu
 */

/* Set and open the icon bar menu. */

void set_acclist_menu (int type, int line, int data)
{
  extern global_menus   menus;


  switch (type)
  {
    case ACCOUNT_FULL:
      msgs_lookup ("AcclistMenuTitleAcc", menus.acclist->title_data.text, 12);
      msgs_lookup ("AcclistMenuViewAcc", menus_get_indirected_text_addr (menus.acclist, ACCLIST_MENU_VIEWACCT), 20);
      msgs_lookup ("AcclistMenuEditAcc", menus_get_indirected_text_addr (menus.acclist, ACCLIST_MENU_EDITACCT), 20);
      msgs_lookup ("AcclistMenuNewAcc", menus_get_indirected_text_addr (menus.acclist, ACCLIST_MENU_NEWACCT), 20);
      break;

    case ACCOUNT_IN:
    case ACCOUNT_OUT:
      msgs_lookup ("AcclistMenuTitleHead", menus.acclist->title_data.text, 12);
      msgs_lookup ("AcclistMenuViewHead", menus_get_indirected_text_addr (menus.acclist, ACCLIST_MENU_VIEWACCT), 20);
      msgs_lookup ("AcclistMenuEditHead", menus_get_indirected_text_addr (menus.acclist, ACCLIST_MENU_EDITACCT), 20);
      msgs_lookup ("AcclistMenuNewHead", menus_get_indirected_text_addr (menus.acclist, ACCLIST_MENU_NEWACCT), 20);
      break;
  }

  menus_shade_entry (menus.acclist, ACCLIST_MENU_VIEWACCT, line == -1 || data != ACCOUNT_LINE_DATA);
  menus_shade_entry (menus.acclist, ACCLIST_MENU_EDITACCT, line == -1 || data != ACCOUNT_LINE_DATA);
  menus_shade_entry (menus.acclist, ACCLIST_MENU_EDITSECT, line == -1 ||
                   (data != ACCOUNT_LINE_HEADER && data != ACCOUNT_LINE_FOOTER));
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_acclist_menu (file_data *file, int type, int line, wimp_pointer *pointer)
{
  int entry, data;

  extern global_menus   menus;


  entry = find_accounts_window_entry_from_type (file, type);
  data = (line == -1) ? ACCOUNT_LINE_BLANK : file->account_windows[entry].line_data[line].type;

  initialise_save_boxes (file, find_accounts_window_entry_from_type (file, type), 0);
  set_acclist_menu (type, line, data);

  menus.menu_up = menus_create_standard_menu (menus.acclist, pointer);
  menus.menu_id = MENU_ID_ACCLIST;
  main_menu_file = file;
  main_menu_line = line;
  acclist_menu_type = type;
}


/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_acclist_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  int entry, data;

  entry = find_accounts_window_entry_from_type (main_menu_file, acclist_menu_type);
  data = (main_menu_line == -1) ?
          ACCOUNT_LINE_BLANK : main_menu_file->account_windows[entry].line_data[main_menu_line].type;

  if (selection->items[0] == ACCLIST_MENU_VIEWACCT)
  {
    create_accview_window (main_menu_file, main_menu_file->account_windows[entry].line_data[main_menu_line].account);
  }
  else if (selection->items[0] == ACCLIST_MENU_EDITACCT)
  {
    open_account_edit_window (main_menu_file, main_menu_file->account_windows[entry].line_data[main_menu_line].account,
                              ACCOUNT_NULL, pointer);
  }
  else if (selection->items[0] == ACCLIST_MENU_EDITSECT)
  {
    open_section_edit_window (main_menu_file, entry, main_menu_line, pointer);
  }
  else if (selection->items[0] == ACCLIST_MENU_NEWACCT)
  {
    open_account_edit_window (main_menu_file, -1, acclist_menu_type, pointer);
  }
  else if (selection->items[0] == ACCLIST_MENU_NEWHEADER)
  {
    open_section_edit_window (main_menu_file, find_accounts_window_entry_from_type (main_menu_file, acclist_menu_type),
                              -1, pointer);
  }
  else if (selection->items[0] == ACCLIST_MENU_PRINT)
  {
    open_account_print_window (main_menu_file, acclist_menu_type, pointer, config_opt_read ("RememberValues"));
  }

  set_acclist_menu (acclist_menu_type, main_menu_line, data);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle submenu warnings. */

void acclist_menu_submenu_message (wimp_full_message_menu_warning *submenu)
{
 #ifdef DEBUG
 debug_reporter_text0 ("\\BReceived submenu warning message.");
 #endif

 switch (submenu->selection.items[0])
 {
   case ACCLIST_MENU_EXPCSV: /* CSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_ACCCSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;

   case ACCLIST_MENU_EXPTSV: /* TSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_ACCTSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;
 }
}

/* ==================================================================================================================
 * Account view menu
 */

/* Set and open the menu. */

void set_accview_menu (int type, int line)
{
  extern global_menus   menus;


  switch (type)
  {
    case ACCOUNT_FULL:
      msgs_lookup ("AccviewMenuTitleAcc", menus.accview->title_data.text, 12);
      msgs_lookup ("AccviewMenuEditAcc", menus_get_indirected_text_addr (menus.accview, ACCVIEW_MENU_EDITACCT), 20);
      break;

    case ACCOUNT_IN:
    case ACCOUNT_OUT:
      msgs_lookup ("AccviewMenuTitleHead", menus.accview->title_data.text, 12);
      msgs_lookup ("AccviewMenuEditHead", menus_get_indirected_text_addr (menus.accview, ACCVIEW_MENU_EDITACCT), 20);
      break;
  }

  menus_shade_entry (menus.accview, ACCVIEW_MENU_FINDTRANS, line == -1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_accview_menu (file_data *file, int account, int line, wimp_pointer *pointer)
{
  extern global_menus   menus;


  initialise_save_boxes (file, account, 0);
  set_accview_menu (file->accounts[account].type, line);

  menus.menu_up = menus_create_standard_menu (menus.accview, pointer);
  menus.menu_id = MENU_ID_ACCVIEW;
  main_menu_file = file;
  main_menu_line = line;
  accview_menu_account = account;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_accview_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  if (selection->items[0] == ACCVIEW_MENU_FINDTRANS)
  {
    place_transaction_edit_line (main_menu_file,
                                 locate_transaction_in_transact_window (main_menu_file,
                                    (main_menu_file->accounts[accview_menu_account].account_view)->
                                     line_data[(main_menu_file->accounts[accview_menu_account].account_view)->
                                      line_data[main_menu_line].sort_index].transaction));
    put_caret_at_end (main_menu_file->transaction_window.transaction_window, 0);
    find_transaction_edit_line (main_menu_file);
  }
  if (selection->items[0] == ACCVIEW_MENU_GOTOTRANS)
  {
    align_accview_with_transact (main_menu_file, accview_menu_account);
  }
  if (selection->items[0] == ACCVIEW_MENU_SORT)
  {
    open_accview_sort_window (main_menu_file, accview_menu_account, pointer);
  }
  else if (selection->items[0] == ACCVIEW_MENU_EDITACCT)
  {
    open_account_edit_window (main_menu_file, accview_menu_account, -1, pointer);
  }
  else if (selection->items[0] == ACCVIEW_MENU_PRINT)
  {
    open_accview_print_window (main_menu_file, accview_menu_account, pointer, config_opt_read ("RememberValues"));
  }

  set_accview_menu (main_menu_file->accounts[accview_menu_account].type, main_menu_line);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle submenu warnings. */

void accview_menu_submenu_message (wimp_full_message_menu_warning *submenu)
{
 #ifdef DEBUG
 debug_reporter_text0 ("\\BReceived submenu warning message.");
 #endif

 switch (submenu->selection.items[0])
 {
   case ACCVIEW_MENU_EXPCSV: /* CSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_ACCVIEWCSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;

   case ACCVIEW_MENU_EXPTSV: /* TSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_ACCVIEWTSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;
 }
}

/* ==================================================================================================================
 * Standing order menu
 */

/* Set and open the menu. */

void set_sorder_menu (int line)
{
  extern global_menus   menus;

  menus_shade_entry (menus.sorder, SORDER_MENU_EDIT, line == -1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_sorder_menu (file_data *file, int line, wimp_pointer *pointer)
{
  extern global_menus   menus;


  initialise_save_boxes (file, 0, 0);
  set_sorder_menu (line);

  menus.menu_up = menus_create_standard_menu (menus.sorder, pointer);
  menus.menu_id = MENU_ID_SORDER;
  main_menu_file = file;
  main_menu_line = line;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_sorder_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  if (selection->items[0] == SORDER_MENU_SORT)
  {
    open_sorder_sort_window (main_menu_file, pointer);
  }
  else if (selection->items[0] == SORDER_MENU_EDIT)
  {
    if (main_menu_line != -1)
    {
      open_sorder_edit_window (main_menu_file, main_menu_file->sorders[main_menu_line].sort_index, pointer);
    }
  }
  else if (selection->items[0] == SORDER_MENU_NEWSORDER)
  {
    open_sorder_edit_window (main_menu_file, NULL_SORDER, pointer);
  }
  else if (selection->items[0] == SORDER_MENU_PRINT)
  {
    open_sorder_print_window (main_menu_file, pointer, config_opt_read ("RememberValues"));
  }
  else if (selection->items[0] == SORDER_MENU_FULLREP)
  {
    generate_full_sorder_report (main_menu_file);
  }

  set_sorder_menu (main_menu_line);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle submenu warnings. */

void sorder_menu_submenu_message (wimp_full_message_menu_warning *submenu)
{
 #ifdef DEBUG
 debug_reporter_text0 ("\\BReceived submenu warning message.");
 #endif

 switch (submenu->selection.items[0])
 {
   case SORDER_MENU_EXPCSV: /* CSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_SORDERCSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;

   case SORDER_MENU_EXPTSV: /* TSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_SORDERTSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;
 }
}

/* ==================================================================================================================
 * Preset menu
 */

/* Set and open the menu. */

void set_preset_menu (int line)
{
  extern global_menus   menus;

  menus_shade_entry (menus.preset, PRESET_MENU_EDIT, line == -1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_preset_menu (file_data *file, int line, wimp_pointer *pointer)
{
  extern global_menus   menus;


  initialise_save_boxes (file, 0, 0);
  set_preset_menu (line);

  menus.menu_up = menus_create_standard_menu (menus.preset, pointer);
  menus.menu_id = MENU_ID_PRESET;
  main_menu_file = file;
  main_menu_line = line;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_preset_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  if (selection->items[0] == PRESET_MENU_SORT)
  {
    open_preset_sort_window (main_menu_file, pointer);
  }
  else if (selection->items[0] == PRESET_MENU_EDIT)
  {
    if (main_menu_line != -1)
    {
      open_preset_edit_window (main_menu_file, main_menu_file->presets[main_menu_line].sort_index, pointer);
    }
  }
  else if (selection->items[0] == PRESET_MENU_NEWPRESET)
  {
    open_preset_edit_window (main_menu_file, NULL_PRESET, pointer);
  }
  else if (selection->items[0] == PRESET_MENU_PRINT)
  {
    open_preset_print_window (main_menu_file, pointer, config_opt_read ("RememberValues"));
  }

  set_preset_menu (main_menu_line);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle submenu warnings. */

void preset_menu_submenu_message (wimp_full_message_menu_warning *submenu)
{
 #ifdef DEBUG
 debug_reporter_text0 ("\\BReceived submenu warning message.");
 #endif

 switch (submenu->selection.items[0])
 {
   case PRESET_MENU_EXPCSV: /* CSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_PRESETCSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;

   case PRESET_MENU_EXPTSV: /* TSV save window */
     fill_save_as_window (main_menu_file, SAVE_BOX_PRESETTSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;
 }
}

/* ==================================================================================================================
 * Report view menu
 */

/* Set and open the menu. */

void set_reportview_menu (report_data *report)
{
  extern global_menus   menus;

  menus_shade_entry (menus.reportview, REPVIEW_MENU_TEMPLATE, report->template.type == REPORT_TYPE_NONE);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_reportview_menu (file_data *file, report_data *report, wimp_pointer *pointer)
{
  extern global_menus   menus;


  initialise_save_boxes (file, (int) report, 0);
  set_reportview_menu (report);

  menus.menu_up = menus_create_standard_menu (menus.reportview, pointer);
  menus.menu_id = MENU_ID_REPORTVIEW;
  main_menu_file = file;
  report_menu_block = report;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void decode_reportview_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  if (selection->items[0] == REPVIEW_MENU_FORMAT)
  {
    open_report_format_window (main_menu_file, report_menu_block, pointer);
  }
  else if (selection->items[0] == REPVIEW_MENU_PRINT)
  {
    open_report_print_window (main_menu_file, report_menu_block, pointer, config_opt_read ("RememberValues"));
  }
  else if (selection->items[0] == REPVIEW_MENU_TEMPLATE)
  {
    open_save_report_window (main_menu_file, report_menu_block, pointer);
  }

  set_reportview_menu (report_menu_block);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle submenu warnings. */

void reportview_menu_submenu_message (wimp_full_message_menu_warning *submenu)
{
 #ifdef DEBUG
 debug_reporter_text0 ("\\BReceived submenu warning message.");
 #endif

 switch (submenu->selection.items[0])
 {
   case REPVIEW_MENU_SAVETEXT:
     fill_save_as_window (main_menu_file, SAVE_BOX_REPTEXT);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;

   case REPVIEW_MENU_EXPCSV:
     fill_save_as_window (main_menu_file, SAVE_BOX_REPCSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;

   case REPVIEW_MENU_EXPTSV:
     fill_save_as_window (main_menu_file, SAVE_BOX_REPTSV);
     wimp_create_sub_menu (submenu->sub_menu, submenu->pos.x, submenu->pos.y);
     break;
 }
}

/* ==================================================================================================================
 * Saved Report menu. -- A list of saved reports, to select from.
 */

void mainmenu_set_replist_menu (file_data *file)
{
}

/* ------------------------------------------------------------------------------------------------------------------ */

void mainmenu_open_replist_menu (file_data *file, wimp_pointer *pointer)
{
  extern global_menus   menus;

  mainmenu_build_replist_menu (file, 1);
  mainmenu_set_replist_menu (file);

  menus.menu_up = menus_create_popup_menu (menus.replist, pointer);
  menus.menu_id = MENU_ID_REPLIST;
  main_menu_file = file;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Decode the menu selections. */

void mainmenu_decode_replist_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  extern global_windows windows;


  if (selection->items[0] != -1)
  {
    strcpy (indirected_icon_text (windows.save_rep, ANALYSIS_SAVE_NAME), replist_link[selection->items[0]].name);

    redraw_icons_in_window (windows.save_rep, 1, ANALYSIS_SAVE_NAME);
    replace_caret_in_window (windows.save_rep);
  }

  mainmenu_set_replist_menu (main_menu_file);
}

/* ------------------------------------------------------------------------------------------------------------------ */

wimp_menu *mainmenu_build_replist_menu (file_data *file, int standalone)
{
  int                 line, width;

  extern global_menus menus;


  /* Claim enough memory to build the menu in.   This can't use the shared allocation, as that is already in
   * use for the acclist menu when the main menu is open.
   */

  if (menus.replist != NULL)
  {
    heap_free (menus.replist);
  }
  if (replist_link != NULL)
  {
    heap_free (replist_link);
  }

  menus.replist = NULL;
  replist_link = NULL;

  if (file->saved_report_count > 0)
  {
    menus.replist = (wimp_menu *) heap_alloc (28 + 24 * file->saved_report_count);
    replist_link = (saved_report_menu_link *) heap_alloc (file->saved_report_count * sizeof (saved_report_menu_link));
  }

  /* Populate the menu. */

  if (menus.replist != NULL && replist_link != NULL)
  {
    width = 0;

    for (line = 0; line < file->saved_report_count; line++)
    {
      /* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
       * well move while the menu is open.  The account number is also stored, to allow the account to be found.
       */

      strcpy (replist_link[line].name, file->saved_reports[line].name);
      if (!standalone)
      {
        strcat(replist_link[line].name, "...");
      }
      replist_link[line].saved_report = line;
      if (strlen (replist_link[line].name) > width)
      {
        width = strlen (replist_link[line].name);
      }
    }

    qsort (replist_link, line, sizeof (saved_report_menu_link), mainmenu_cmp_replist_menu_entries);

    for (line = 0; line < file->saved_report_count; line++)
    {
      /* Set the menu and icon flags up. */

      menus.replist->entries[line].menu_flags = 0;

      menus.replist->entries[line].sub_menu = (wimp_menu *) -1;
      menus.replist->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
                                           wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
                                           wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

      /* Set the menu icon contents up. */

      menus.replist->entries[line].data.indirected_text.text = replist_link[line].name;
      menus.replist->entries[line].data.indirected_text.validation = NULL;
      menus.replist->entries[line].data.indirected_text.size = ACCOUNT_NAME_LEN;

      #ifdef DEBUG
      debug_printf ("Line %d: '%s'", line, replist_link[line].name);
      #endif
    }

    menus.replist->entries[line - 1].menu_flags |= wimp_MENU_LAST;

    if (replist_title_buffer == NULL)
    {
      replist_title_buffer = (char *) malloc (ACCOUNT_MENU_TITLE_LEN);
    }
    msgs_lookup ((standalone) ? "RepListMenuT2" : "RepListMenuT1", replist_title_buffer, ACCOUNT_MENU_TITLE_LEN);
    menus.replist->title_data.indirected_text.text = replist_title_buffer;
    menus.replist->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
    menus.replist->title_fg = wimp_COLOUR_BLACK;
    menus.replist->title_bg = wimp_COLOUR_LIGHT_GREY;
    menus.replist->work_fg = wimp_COLOUR_BLACK;
    menus.replist->work_bg = wimp_COLOUR_WHITE;

    menus.replist->width = (width + 1) * 16;
    menus.replist->height = 44;
    menus.replist->gap = 0;
  }

  menus.analysis_sub->entries[MAIN_MENU_ANALYSIS_SAVEDREP].sub_menu = menus.replist;

  return (menus.replist);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int mainmenu_cmp_replist_menu_entries (const void *va, const void *vb)
{
  saved_report_menu_link *a = (saved_report_menu_link *) va, *b = (saved_report_menu_link *) vb;

  return (strcmp_no_case(a->name, b->name));
}

/* ==================================================================================================================
 * Font list menu
 */

void open_font_list_menu (wimp_pointer *pointer)
{
  int size1, size2;

  extern global_menus   menus;

  font_list_fonts (0, font_RETURN_FONT_MENU, 0, 0, 0, 0, &size1, &size2);

  font_buf1 = heap_alloc (size1);
  font_buf2 = heap_alloc (size2);

  font_list_fonts (font_buf1, font_RETURN_FONT_MENU, size1, font_buf2, size2, 0, NULL, NULL);

  menus.font_list = (wimp_menu *) font_buf1;

  menus.menu_up = menus_create_popup_menu (menus.font_list, pointer);
  menus.menu_id = MENU_ID_FONTLIST;
  font_window = pointer->w;
  font_icon = pointer->i;
}

/* ------------------------------------------------------------------------------------------------------------------ */

void decode_font_list_menu (wimp_selection *selection, wimp_pointer *pointer)
{
  int  size;
  char *name, *sub, *c;

  extern global_windows windows;


  /* Decode the font menu. */

  font_decode_menu (0, font_buf1, (byte *) selection, 0, 0, NULL, &size);
  name = heap_alloc (size);
  font_decode_menu (0, font_buf1, (byte *) selection, (byte *) name, size, NULL, NULL);

  /* Extract the font name from the data returned from font_decode_menu (). */

  sub = strstr (name, "\\F");
  if (sub == NULL)
  {
    sub = name;
  }
  else
  {
    sub += 2;
  }

  if ((c = strchr (sub, '\\')) != NULL)
  {
    *c = '\0';
  }

  /* Update the correct icon. */

  if (font_window== windows.choices_pane[CHOICE_PANE_REPORT] && font_icon == CHOICE_ICON_NFONTMENU)
  {
    sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT), "%s", sub);
    wimp_set_icon_state (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT, 0, 0);
  }
  else if (font_window== windows.choices_pane[CHOICE_PANE_REPORT] && font_icon == CHOICE_ICON_BFONTMENU)
  {
    sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT), "%s", sub);
    wimp_set_icon_state (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT, 0, 0);
  }
  else if (font_window== windows.report_format && font_icon == REPORT_FORMAT_NFONTMENU)
  {
    sprintf (indirected_icon_text (windows.report_format, REPORT_FORMAT_NFONT), "%s", sub);
    wimp_set_icon_state (windows.report_format, REPORT_FORMAT_NFONT, 0, 0);
  }
  else if (font_window== windows.report_format && font_icon == REPORT_FORMAT_BFONTMENU)
  {
    sprintf (indirected_icon_text (windows.report_format, REPORT_FORMAT_BFONT), "%s", sub);
    wimp_set_icon_state (windows.report_format, REPORT_FORMAT_BFONT, 0, 0);
  }

  /* Free the name memory buffer. */

  heap_free (name);

  /* Clear the menu blocks if the menu is closed. */

  if (pointer->buttons != wimp_CLICK_ADJUST)
  {
    deallocate_font_list_menu ();
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void deallocate_font_list_menu (void)
{
  if (font_buf1 != NULL)
  {
    heap_free (font_buf1);
    font_buf1 = NULL;
  }

  if (font_buf2 != NULL)
  {
    heap_free (font_buf2);
    font_buf2 = NULL;
  }
}
