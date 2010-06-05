/* CashBook - init.c
 * (c) Stephen Fryatt, 2003
 */

/* ANSI C Header files. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Acorn C Header files. */

#include "flex.h"

/* OSLib Header files. */

#include "oslib/wimp.h"
#include "oslib/osbyte.h"
#include "oslib/osspriteop.h"
#include "oslib/uri.h"
#include "oslib/hourglass.h"
#include "oslib/pdriver.h"
#include "oslib/help.h"
#include "oslib/types.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/resources.h"
#include "sflib/menus.h"
#include "sflib/errors.h"
#include "sflib/url.h"
#include "sflib/heap.h"
#include "sflib/msgs.h"
#include "sflib/debug.h"
#include "sflib/tasks.h"
#include "sflib/windows.h"

/* Application header files. */

#include "global.h"
#include "init.h"

#include "choices.h"
#include "conversion.h"
#include "date.h"
#include "file.h"
#include "filing.h"
#include "ihelp.h"
#include "transact.h"

/* ================================================================================================================== */

/* Load templates into memory and either create windows or store definitions. */

void load_templates (char *template_file, global_windows *windows, osspriteop_area *sprites)
{
  wimp_window  *window_def;
  os_error     *error;


  error = xwimp_open_template (template_file);
  if (error != NULL)
  {
    wimp_program_report (error);
  }

  /* Program Info Window.
   *
   * Created now.
   */

  window_def = load_window_template ("ProgInfo");
  if (window_def != NULL)
  {
    windows->prog_info = wimp_create_window (window_def);
    add_ihelp_window (windows->prog_info, "ProgInfo", NULL);
    msgs_param_lookup ("Version",
                       window_def->icons[6].data.indirected_text.text, window_def->icons[6].data.indirected_text.size,
                       BUILD_DATE, NULL, NULL, NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* File Info Window.
   *
   * Created now.
   */

  window_def = load_window_template ("FileInfo");
  if (window_def != NULL)
  {
    windows->file_info = wimp_create_window (window_def);
    add_ihelp_window (windows->file_info, "FileInfo", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Import Complete Window.
   *
   * Created now.
   */

  window_def = load_window_template ("ImpComp");
  if (window_def != NULL)
  {
    windows->import_comp = wimp_create_window (window_def);
    add_ihelp_window (windows->import_comp, "ImpComp", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Save Window.
   *
   * Created now.
   */

  window_def = load_window_template ("SaveAs");
  if (window_def != NULL)
  {
    windows->save_as = wimp_create_window (window_def);
    add_ihelp_window (windows->save_as, "SaveAs", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Window.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices");
  if (window_def != NULL)
  {
    windows->choices = wimp_create_window (window_def);
    add_ihelp_window (windows->choices, "Choices", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Pane 0.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices0");
  if (window_def != NULL)
  {
    windows->choices_pane[CHOICE_PANE_GENERAL] = wimp_create_window (window_def);
    add_ihelp_window (windows->choices_pane[CHOICE_PANE_GENERAL], "Choices0", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Pane 1.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices1");
  if (window_def != NULL)
  {
    windows->choices_pane[CHOICE_PANE_CURRENCY] = wimp_create_window (window_def);
    add_ihelp_window (windows->choices_pane[CHOICE_PANE_CURRENCY], "Choices1", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Pane 2.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices2");
  if (window_def != NULL)
  {
    windows->choices_pane[CHOICE_PANE_SORDER] = wimp_create_window (window_def);
    add_ihelp_window (windows->choices_pane[CHOICE_PANE_SORDER], "Choices2", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Pane 3.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices3");
  if (window_def != NULL)
  {
    windows->choices_pane[CHOICE_PANE_PRINT] = wimp_create_window (window_def);
    add_ihelp_window (windows->choices_pane[CHOICE_PANE_PRINT], "Choices3", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Pane 4.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices4");
  if (window_def != NULL)
  {
    windows->choices_pane[CHOICE_PANE_TRANSACT] = wimp_create_window (window_def);
    add_ihelp_window (windows->choices_pane[CHOICE_PANE_TRANSACT], "Choices4", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Pane 5.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices5");
  if (window_def != NULL)
  {
    windows->choices_pane[CHOICE_PANE_REPORT] = wimp_create_window (window_def);
    add_ihelp_window (windows->choices_pane[CHOICE_PANE_REPORT], "Choices5", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Choices Pane 6.
   *
   * Created now.
   */

  window_def = load_window_template ("Choices6");
  if (window_def != NULL)
  {
    windows->choices_pane[CHOICE_PANE_ACCOUNT] = wimp_create_window (window_def);
    add_ihelp_window (windows->choices_pane[CHOICE_PANE_ACCOUNT], "Choices6", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Edit Account Window.
   *
   * Created now.
   */

  window_def = load_window_template ("EditAccount");
  if (window_def != NULL)
  {
    windows->edit_acct = wimp_create_window (window_def);
    add_ihelp_window (windows->edit_acct, "EditAccount", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Edit Heading Window.
   *
   * Created now.
   */

  window_def = load_window_template ("EditHeading");
  if (window_def != NULL)
  {
    windows->edit_hdr = wimp_create_window (window_def);
    add_ihelp_window (windows->edit_hdr, "EditHeading", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Edit Section Window.
   *
   * Created now.
   */

  window_def = load_window_template ("EditAccSect");
  if (window_def != NULL)
  {
    windows->edit_sect = wimp_create_window (window_def);
    add_ihelp_window (windows->edit_sect, "EditAccSect", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Edit Standing Order Window.
   *
   * Created now.
   */

  window_def = load_window_template ("EditSOrder");
  if (window_def != NULL)
  {
    windows->edit_sorder = wimp_create_window (window_def);
    add_ihelp_window (windows->edit_sorder, "EditSOrder", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Edit Preset Window.
   *
   * Created now.
   */

  window_def = load_window_template ("EditPreset");
  if (window_def != NULL)
  {
    windows->edit_preset = wimp_create_window (window_def);
    add_ihelp_window (windows->edit_preset, "EditPreset", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Goto Window.
   *
   * Created now.
   */

  window_def = load_window_template ("Goto");
  if (window_def != NULL)
  {
    windows->go_to = wimp_create_window (window_def);
    add_ihelp_window (windows->go_to, "Goto", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Find Window.
   *
   * Created now.
   */

  window_def = load_window_template ("Find");
  if (window_def != NULL)
  {
    windows->find = wimp_create_window (window_def);
    add_ihelp_window (windows->find, "Find", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Found Window.
   *
   * Created now.
   */

  window_def = load_window_template ("Found");
  if (window_def != NULL)
  {
    windows->found = wimp_create_window (window_def);
    add_ihelp_window (windows->found, "Found", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Budget Window.
   *
   * Created now.
   */

  window_def = load_window_template ("Budget");
  if (window_def != NULL)
  {
    windows->budget = wimp_create_window (window_def);
    add_ihelp_window (windows->budget, "Budget", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Report Format Window.
   *
   * Created now.
   */

  window_def = load_window_template ("RepFormat");
  if (window_def != NULL)
  {
    windows->report_format = wimp_create_window (window_def);
    add_ihelp_window (windows->report_format, "RepFormat", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Simple Print Window.
   *
   * Created now.
   */

  window_def = load_window_template ("SimplePrint");
  if (window_def != NULL)
  {
    windows->simple_print = wimp_create_window (window_def);
    add_ihelp_window (windows->simple_print, "SimplePrint", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Date Print Window.
   *
   * Created now.
   */

  window_def = load_window_template ("DatePrint");
  if (window_def != NULL)
  {
    windows->date_print = wimp_create_window (window_def);
    add_ihelp_window (windows->date_print, "DatePrint", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Transaction Report Window.
   *
   * Created now.
   */

  window_def = load_window_template ("TransRep");
  if (window_def != NULL)
  {
    windows->trans_rep = wimp_create_window (window_def);
    add_ihelp_window (windows->trans_rep, "TransRep", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Unreconciled Transaction Report Window.
   *
   * Created now.
   */

  window_def = load_window_template ("UnrecRep");
  if (window_def != NULL)
  {
    windows->unrec_rep = wimp_create_window (window_def);
    add_ihelp_window (windows->unrec_rep, "UnrecRep", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Cashflow Report Window.
   *
   * Created now.
   */

  window_def = load_window_template ("CashFlwRep");
  if (window_def != NULL)
  {
    windows->cashflow_rep = wimp_create_window (window_def);
    add_ihelp_window (windows->cashflow_rep, "CashFlwRep", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Balance Report Window.
   *
   * Created now.
   */

  window_def = load_window_template ("BalanceRep");
  if (window_def != NULL)
  {
    windows->balance_rep = wimp_create_window (window_def);
    add_ihelp_window (windows->balance_rep, "BalanceRep", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Account Name Enter Window.
   *
   * Created now.
   */

  window_def = load_window_template ("AccEnter");
  if (window_def != NULL)
  {
    windows->enter_acc = wimp_create_window (window_def);
    add_ihelp_window (windows->enter_acc, "AccEnter", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Purge File Window.
   *
   * Created now.
   */

  window_def = load_window_template ("Purge");
  if (window_def != NULL)
  {
    windows->continuation = wimp_create_window (window_def);
    add_ihelp_window (windows->continuation, "Purge", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Colours Window.
   *
   * Created now.
   */

  window_def = load_window_template ("Colours");
  if (window_def != NULL)
  {
    windows->colours = wimp_create_window (window_def);
    add_ihelp_window (windows->colours, "Colours", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Sort Transactions Window.
   *
   * Created now.
   */

  window_def = load_window_template ("SortTrans");
  if (window_def != NULL)
  {
    windows->sort_trans = wimp_create_window (window_def);
    add_ihelp_window (windows->sort_trans, "SortTrans", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Sort AccView Window.
   *
   * Created now.
   */

  window_def = load_window_template ("SortAccView");
  if (window_def != NULL)
  {
    windows->sort_accview = wimp_create_window (window_def);
    add_ihelp_window (windows->sort_accview, "SortAccView", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Sort SOrder Window.
   *
   * Created now.
   */

  window_def = load_window_template ("SortSOrder");
  if (window_def != NULL)
  {
    windows->sort_sorder = wimp_create_window (window_def);
    add_ihelp_window (windows->sort_sorder, "SortSOrder", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Sort Preset Window.
   *
   * Created now.
   */

  window_def = load_window_template ("SortPreset");
  if (window_def != NULL)
  {
    windows->sort_preset = wimp_create_window (window_def);
    add_ihelp_window (windows->sort_preset, "SortPreset", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Save Report Window.
   *
   * Created now.
   */

  window_def = load_window_template ("SaveRepTemp");
  if (window_def != NULL)
  {
    windows->save_rep = wimp_create_window (window_def);
    add_ihelp_window (windows->save_rep, "SaveRepTemp", NULL);
    free (window_def);
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }


  /* Transaction Window.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("Transact");
  if (window_def != NULL)
  {
    window_def->icon_count = 0;
    windows->transaction_window_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Transaction Pane.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("TransactTB");
  if (window_def != NULL)
  {
    window_def->sprite_area = sprites;
    windows->transaction_pane_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Account Window.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("Account");
  if (window_def != NULL)
  {
    window_def->icon_count = 0;
    windows->account_window_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Account Pane.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("AccountATB");
  if (window_def != NULL)
  {
    window_def->sprite_area = sprites;
    windows->account_pane_def[0] = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

   /* Account Footer.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("AccountTot");
  if (window_def != NULL)
  {
    windows->account_footer_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Heading Pane.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("AccountHTB");
  if (window_def != NULL)
  {
    window_def->sprite_area = sprites;
    windows->account_pane_def[1] = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Standing Order Window.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("SOrder");
  if (window_def != NULL)
  {
    window_def->icon_count = 0;
    windows->sorder_window_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Standing Order Pane.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("SOrderTB");
  if (window_def != NULL)
  {
    window_def->sprite_area = sprites;
    windows->sorder_pane_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Account View Window.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("AccView");
  if (window_def != NULL)
  {
    window_def->icon_count = 0;
    windows->accview_window_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Account View Pane.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("AccViewTB");
  if (window_def != NULL)
  {
    window_def->sprite_area = sprites;
    windows->accview_pane_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Preset Window.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("Preset");
  if (window_def != NULL)
  {
    window_def->icon_count = 0;
    windows->preset_window_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Preset Pane.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("PresetTB");
  if (window_def != NULL)
  {
    window_def->sprite_area = sprites;
    windows->preset_pane_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  /* Report Window.
   *
   * Definition loaded for future use.
   */

  window_def = load_window_template ("Report");
  if (window_def != NULL)
  {
    window_def->sprite_area = sprites;
    windows->report_window_def = window_def;
  }
  else
  {
    wimp_msgtrans_fatal_report ("BadTemplate");
  }

  wimp_close_template ();
}


/* ================================================================================================================== */

int initialise (void)
{
  static char                       task_name[255];
  char                              resources[255], res_temp[255];
  osspriteop_area                   *sprites;
  extern wimp_t                     task_handle;

  wimp_MESSAGE_LIST(20)             message_list;
  wimp_version_no                   wimp_version;
  wimp_icon_create                  icon_bar;

  wimp_menu                         *menu_list[20];
  menu_template                     menu_defs;

  osbool                            already_running;

  extern global_windows             windows;
  extern global_menus               menus;


  hourglass_on ();

  strcpy (resources, "<CashBook$Dir>.Resources");
  find_resource_path (resources, sizeof (resources));

  /* Load the messages file. */

  sprintf (res_temp, "%s.Messages", resources);
  msgs_init (res_temp);

  /* Initialise the error message system. */

  error_initialise ("TaskName", "TaskSpr", NULL);

  /* Initialise with the Wimp. */

  message_list.messages[0]=message_URI_RETURN_RESULT;
  message_list.messages[1]=message_ANT_OPEN_URL;
  message_list.messages[2]=message_CLAIM_ENTITY;
  message_list.messages[3]=message_DATA_REQUEST;
  message_list.messages[4]=message_DATA_SAVE;
  message_list.messages[5]=message_DATA_SAVE_ACK;
  message_list.messages[6]=message_DATA_LOAD;
  message_list.messages[7]=message_RAM_FETCH;
  message_list.messages[8]=message_RAM_TRANSMIT;
  message_list.messages[9]=message_DATA_OPEN;
  message_list.messages[10]=message_MENU_WARNING;
  message_list.messages[11]=message_MENUS_DELETED;
  message_list.messages[12]=message_PRE_QUIT;
  message_list.messages[13]=message_PRINT_SAVE;
  message_list.messages[14]=message_PRINT_ERROR;
  message_list.messages[15]=message_PRINT_FILE;
  message_list.messages[16]=message_PRINT_INIT;
  message_list.messages[17]=message_SET_PRINTER;
  message_list.messages[18]=message_HELP_REQUEST;
  message_list.messages[19]=message_QUIT;
  msgs_lookup ("TaskName", task_name, sizeof (task_name));
  task_handle = wimp_initialise (wimp_VERSION_RO38, task_name, (wimp_message_list *) &message_list, &wimp_version);

  already_running = test_for_duplicate_task (task_name, task_handle, "DupTask", "DupTaskB");

  /* Initialise the flex heap. */

  flex_init (task_name, 0, 0);
  heap_init ();

  /* Initialise the configuration. */

  initialise_configuration (task_name, "CashBook", "<CashBook$Dir>");

  init_config_opt ("IyonixKeys", (osbyte1 (osbyte_IN_KEY, 0, 0xff) == 0xaa)); /* True only on an Iyonix. */
  init_config_opt ("GlobalClipboardSupport", 1);

  init_config_opt ("RememberValues", 1);

  init_config_opt ("AllowTransDelete", 1);

  init_config_int ("MaxAutofillLen", 0);

  init_config_opt ("AutoSort", 1);

  init_config_opt ("ShadeReconciled", 0);
  init_config_int ("ShadeReconciledColour", 3);

  init_config_opt ("ShadeBudgeted", 0);
  init_config_int ("ShadeBudgetedColour", 3);

  init_config_opt ("ShadeOverdrawn", 0);
  init_config_int ("ShadeOverdrawnColour", 11);

  init_config_opt ("ShadeAccounts", 0);
  init_config_int ("ShadeAccountsColour", 11);

  init_config_opt ("TerritoryDates", 1);
  init_config_str ("DateSepIn", "-/\\.");
  init_config_str ("DateSepOut", "-");

  init_config_opt ("TerritoryCurrency", 1);
  init_config_opt ("PrintZeros", 0);
  init_config_opt ("BracketNegatives", 0);
  init_config_int ("DecimalPlaces", 2);
  init_config_str ("DecimalPoint", ".");

  init_config_opt ("SortAfterSOrders", 1);
  init_config_opt ("AutoSortSOrders", 1);
  init_config_opt ("TerritorySOrders", 1);
  init_config_int ("WeekendDays", 0x41);

  init_config_opt ("AutoSortPresets", 1);

  init_config_str ("ReportFontNormal", "Homerton.Medium");
  init_config_str ("ReportFontBold", "Homerton.Bold");
  init_config_int ("ReportFontSize", 12);
  init_config_int ("ReportFontLinespace", 130);

  init_config_opt ("PrintFitWidth", 1);
  init_config_opt ("PrintRotate", 0);
  init_config_opt ("PrintText", 0);
  init_config_opt ("PrintTextFormat", 1);

  init_config_int ("PrintMarginTop", 0);
  init_config_int ("PrintMarginLeft", 0);
  init_config_int ("PrintMarginRight", 0);
  init_config_int ("PrintMarginBottom", 0);
  init_config_int ("PrintMarginUnits", 0); /* 0 = mm, 1 = cm, 2 = inch */

  init_config_str ("TransactCols", "180,88,32,362,88,32,362,200,176,808");
  init_config_str ("LimTransactCols", "140,88,32,140,88,32,140,140,140,200");
  init_config_str ("AccountCols", "88,362,176,176,176,176");
  init_config_str ("LimAccountCols", "88,140,140,140,140,140");
  init_config_str ("AccViewCols", "180,88,32,362,200,176,176,176,808");
  init_config_str ("LimAccViewCols", "140,88,32,140,140,140,140,140,200");
  init_config_str ("SOrderCols", "88,32,362,88,32,362,176,500,180,100");
  init_config_str ("LimSOrderCols", "88,32,140,88,32,140,140,200,140,60");
  init_config_str ("PresetCols", "120,500,88,32,362,88,32,362,176,500");
  init_config_str ("LimPresetCols", "88,200,88,32,140,88,32,140,140,200");

  load_configuration ();

  set_weekend_days ();
  set_up_money ();

  /* Load the window templates. */

  sprites = load_user_sprite_area ("<CashBook$Dir>.Sprites");

  sprintf (res_temp, "%s.Templates", resources);
  load_templates (res_temp, &windows, sprites);

  /* Load the menu structure. */

  sprintf (res_temp, "%s.Menus", resources);
  menu_defs = load_menus(res_temp, NULL, menu_list);

  load_menus_dbox(menu_defs, "prog_info", windows.prog_info);
  load_menus_dbox(menu_defs, "file_info", windows.file_info);
  load_menus_dbox(menu_defs, "save_as", windows.save_as);

  menus.icon_bar        = menu_list[0];
  menus.main            = menu_list[1];
  menus.account_sub     = menu_list[3];
  menus.transaction_sub = menu_list[5];
  menus.analysis_sub    = menu_list[6];
  menus.acclist         = menu_list[7];
  menus.accview         = menu_list[8];
  menus.sorder          = menu_list[9];
  menus.preset          = menu_list[10];
  menus.reportview      = menu_list[11];

  menus.accopen         = NULL;
  menus.account         = NULL;
  menus.font_list       = NULL;

  /* Create an icon-bar icon. */

  icon_bar.w = wimp_ICON_BAR_RIGHT;
  icon_bar.icon.extent.x0 = 0;
  icon_bar.icon.extent.x1 = 68;
  icon_bar.icon.extent.y0 = 0;
  icon_bar.icon.extent.y1 = 69;
  icon_bar.icon.flags = wimp_ICON_SPRITE | (wimp_BUTTON_CLICK << wimp_ICON_BUTTON_TYPE_SHIFT);
  msgs_lookup ("TaskSpr", icon_bar.icon.data.sprite, osspriteop_NAME_LIMIT);
  wimp_create_icon (&icon_bar);

  /* Initialise the file update mechanism: calling it now with no files loaded will force the date to be set up. */

  update_files_for_new_date ();

  hourglass_off ();

  return (already_running);
}

/* ================================================================================================================== */

/* Take the command line and parse it for useful arguments. */

void parse_command_line (int argc, char *argv[])
{
  int i;

  if (argc > 1)
  {
    for (i=1; i<argc; i++)
    {
      if (strcmp (argv[i], "-file") == 0 && i+1 < argc)
      {
        load_transaction_file (argv[i+1]);
      }
    }
  }
}

