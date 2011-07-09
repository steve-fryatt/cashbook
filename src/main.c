/* CashBook - main.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* ANSI C header files */

#include <stdlib.h>
#include <string.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/help.h"
#include "oslib/hourglass.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/pdriver.h"
#include "oslib/uri.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/resources.h"
#include "sflib/string.h"
#include "sflib/tasks.h"
#include "sflib/transfer.h"
#include "sflib/url.h"
#include "sflib/windows.h"

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
#include "conversion.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "find.h"
#include "goto.h"
#include "iconbar.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "presets.h"
#include "printing.h"
#include "purge.h"
#include "redraw.h"
#include "report.h"
#include "sorder.h"
#include "templates.h"
#include "transact.h"
#include "window.h"

/* ------------------------------------------------------------------------------------------------------------------ */

static void		main_poll_loop(void);
static void		main_initialise(void);
static void		main_parse_command_line(int argc, char *argv[]);
static osbool		main_message_quit(wimp_message *message);
static osbool		main_message_prequit(wimp_message *message);




static void load_templates(global_windows *windows, osspriteop_area *sprites);
static void mouse_click_handler (wimp_pointer *);
static void key_press_handler (wimp_key *key);
static void menu_selection_handler (wimp_selection *);
static void scroll_request_handler (wimp_scroll *);
static void user_message_handler (wimp_message *);
static void bounced_message_handler (wimp_message *);





/* Declare the global variables that are used. */

global_windows          windows;
global_menus            menus;
int                     global_drag_type;

/* Cross file global variables */

wimp_t			main_task_handle;
osbool			main_quit_flag = FALSE;

/**
 * Main code entry point.
 */

int main(int argc, char *argv[])
{
	main_initialise();
	main_parse_command_line(argc, argv);

	main_poll_loop();

	msgs_terminate();
	wimp_close_down(main_task_handle);

	return 0;
}


/**
 * Wimp Poll loop.
 */

static void main_poll_loop(void)
{
	file_data	*file;
	os_t		poll_time;
	wimp_pointer	ptr;
	char		buffer[1024], *pathcopy;
	wimp_block	blk;
	wimp_event_no	reason;

	poll_time = os_read_monotonic_time();

	while (!main_quit_flag) {
		reason = wimp_poll_idle(0, &blk, poll_time, NULL);

		/* Events are passed to Event Lib first; only if this fails
		 * to handle them do they get passed on to the internal
		 * inline handlers shown here.
		 */

		if (!event_process_event(reason, &blk, 0)) {
			switch (reason) {
			case wimp_NULL_REASON_CODE:
				update_files_for_new_date();
				poll_time += 6000; /* Wait for a minute for the next Null poll */
				break;

			case wimp_REDRAW_WINDOW_REQUEST:
				if ((file = find_transaction_window_file_block(blk.redraw.w)) != NULL)
					redraw_transaction_window(&(blk.redraw), file);
				else if ((file = find_account_window_file_block(blk.redraw.w)) != NULL)
					redraw_account_window(&(blk.redraw), file);
				else if ((file = find_accview_window_file_block(blk.redraw.w)) != NULL)
					redraw_accview_window(&(blk.redraw), file);
				else if ((file = find_sorder_window_file_block(blk.redraw.w)) != NULL)
					redraw_sorder_window(&(blk.redraw), file);
				else if ((file = find_preset_window_file_block(blk.redraw.w)) != NULL)
					redraw_preset_window(&(blk.redraw), file);
				break;

			case wimp_OPEN_WINDOW_REQUEST:
				if ((file = find_transaction_window_file_block(blk.redraw.w)) != NULL)
					minimise_transaction_window_extent(file);
				wimp_open_window(&(blk.open));
				break;

			case wimp_CLOSE_WINDOW_REQUEST:
				if ((file = find_transaction_window_file_block(blk.close.w)) != NULL) {
					wimp_get_pointer_info(&ptr);

					/* If Adjust was clicked, find the pathname and open the parent directory. */

					if (ptr.buttons == wimp_CLICK_ADJUST && check_for_filepath(file)) {
						pathcopy = strdup(file->filename);
						if (pathcopy != NULL) {
							snprintf(buffer, sizeof(buffer), "%%Filer_OpenDir %s", string_find_pathname(pathcopy));
							xos_cli(buffer);
							free(pathcopy);
						}
					}

					/* If it was NOT an Adjust click with Shift held down, close the file. */

					if (!((osbyte1(osbyte_IN_KEY, 0xfc, 0xff) == 0xff || osbyte1(osbyte_IN_KEY, 0xf9, 0xff) == 0xff) &&
							ptr.buttons == wimp_CLICK_ADJUST))
						delete_file(file);
				} else if ((file = find_account_window_file_block(blk.close.w)) != NULL)
					delete_accounts_window(file, find_accounts_window_type_from_handle(file, blk.close.w));
				else if ((file = find_accview_window_file_block (blk.close.w)) != NULL)
					delete_accview_window(file, find_accview_window_from_handle(file, blk.close.w));
				else if ((file = find_sorder_window_file_block(blk.close.w)) != NULL)
					delete_sorder_window(file);
				else if ((file = find_preset_window_file_block(blk.close.w)) != NULL)
					delete_preset_window(file);
				else
					wimp_close_window(blk.close.w);
				break;

			case wimp_MOUSE_CLICK:
				mouse_click_handler(&(blk.pointer));
				break;

			case wimp_KEY_PRESSED:
				key_press_handler(&(blk.key));
				break;

			case wimp_MENU_SELECTION:
				menu_selection_handler(&(blk.selection));
				break;

			case wimp_USER_DRAG_BOX:
				switch (global_drag_type) {
				case SAVE_DRAG:
					terminate_user_drag(&(blk.dragged));
					break;

				case ACCOUNT_DRAG:
					terminate_account_drag(&(blk.dragged));
					break;

				case COLUMN_DRAG:
					terminate_column_width_drag(&(blk.dragged));
					break;
				}
				break;

			case wimp_SCROLL_REQUEST:
				scroll_request_handler(&(blk.scroll));
				break;

			case wimp_LOSE_CARET:
				refresh_transaction_edit_line_icons(blk.caret.w, -1, -1);
				break;

			case wimp_USER_MESSAGE:
			case wimp_USER_MESSAGE_RECORDED:
				user_message_handler(&(blk.message));
				break;

			case wimp_USER_MESSAGE_ACKNOWLEDGE:
				bounced_message_handler(&(blk.message));
				break;
			}
		}
	}
}


/**
 * Application initialisation.
 */

static void main_initialise(void)
{
	static char			task_name[255];
	char				resources[255], res_temp[255];
	osspriteop_area			*sprites;

	wimp_MESSAGE_LIST(20)		message_list;
	wimp_version_no			wimp_version;

	extern global_windows		windows;
	extern global_menus		menus;

	hourglass_on();

	strcpy(resources, "<CashBook$Dir>.Resources");
	resources_find_path(resources, sizeof(resources));

	/* Load the messages file. */

	snprintf(res_temp, sizeof(res_temp), "%s.Messages", resources);
	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName", "TaskSpr", NULL);

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

	msgs_lookup("TaskName", task_name, sizeof(task_name));
	main_task_handle = wimp_initialise(wimp_VERSION_RO38, task_name, (wimp_message_list *) &message_list, &wimp_version);

	if (tasks_test_for_duplicate(task_name, main_task_handle, "DupTask", "DupTaskB"))
		main_quit_flag = TRUE;

	event_add_message_handler(message_QUIT, EVENT_MESSAGE_INCOMING, main_message_quit);
	event_add_message_handler(message_PRE_QUIT, EVENT_MESSAGE_INCOMING, main_message_prequit);

	/* Initialise the flex heap. */

	flex_init(task_name, 0, 0);
	heap_initialise();

	/* Initialise the configuration. */

	config_initialise(task_name, "CashBook", "<CashBook$Dir>");

	config_opt_init("IyonixKeys", (osbyte1(osbyte_IN_KEY, 0, 0xff) == 0xaa)); /* True only on an Iyonix. */
	config_opt_init("GlobalClipboardSupport", TRUE);

	config_opt_init("RememberValues", TRUE);

	config_opt_init("AllowTransDelete", TRUE);

	config_int_init("MaxAutofillLen", 0);

	config_opt_init("AutoSort", TRUE);

	config_opt_init("ShadeReconciled", FALSE);
	config_int_init("ShadeReconciledColour", wimp_COLOUR_MID_LIGHT_GREY);

	config_opt_init("ShadeBudgeted", FALSE);
	config_int_init("ShadeBudgetedColour", wimp_COLOUR_MID_LIGHT_GREY);

	config_opt_init("ShadeOverdrawn", FALSE);
	config_int_init("ShadeOverdrawnColour", wimp_COLOUR_RED);

	config_opt_init("ShadeAccounts", FALSE);
	config_int_init("ShadeAccountsColour", wimp_COLOUR_RED);

	config_opt_init("TerritoryDates", TRUE);
	config_str_init("DateSepIn", "-/\\.");
	config_str_init("DateSepOut", "-");

	config_opt_init("TerritoryCurrency", TRUE);
	config_opt_init("PrintZeros", FALSE);
	config_opt_init("BracketNegatives", FALSE);
	config_int_init("DecimalPlaces", 2);
	config_str_init("DecimalPoint", ".");

	config_opt_init("SortAfterSOrders", TRUE);
	config_opt_init("AutoSortSOrders", TRUE);
	config_opt_init("TerritorySOrders", TRUE);
	config_int_init("WeekendDays", 0x41);

	config_opt_init("AutoSortPresets", TRUE);

	config_str_init("ReportFontNormal", "Homerton.Medium");
	config_str_init("ReportFontBold", "Homerton.Bold");
	config_int_init("ReportFontSize", 12);
	config_int_init("ReportFontLinespace", 130);

	config_opt_init("PrintFitWidth", TRUE);
	config_opt_init("PrintRotate", FALSE);
	config_opt_init("PrintText", FALSE);
	config_opt_init("PrintTextFormat", TRUE);

	config_int_init("PrintMarginTop", 0);
	config_int_init("PrintMarginLeft", 0);
	config_int_init("PrintMarginRight", 0);
	config_int_init("PrintMarginBottom", 0);
	config_int_init("PrintMarginUnits", 0); /* 0 = mm, 1 = cm, 2 = inch */

	config_str_init("TransactCols", "180,88,32,362,88,32,362,200,176,808");
	config_str_init("LimTransactCols", "140,88,32,140,88,32,140,140,140,200");
	config_str_init("AccountCols", "88,362,176,176,176,176");
	config_str_init("LimAccountCols", "88,140,140,140,140,140");
	config_str_init("AccViewCols", "180,88,32,362,200,176,176,176,808");
	config_str_init("LimAccViewCols", "140,88,32,140,140,140,140,140,200");
	config_str_init("SOrderCols", "88,32,362,88,32,362,176,500,180,100");
	config_str_init("LimSOrderCols", "88,32,140,88,32,140,140,200,140,60");
	config_str_init("PresetCols", "120,500,88,32,362,88,32,362,176,500");
	config_str_init("LimPresetCols", "88,200,88,32,140,88,32,140,140,200");

	config_load();

	set_weekend_days();
	set_up_money();

	/* Load the window templates. */

	sprites = resources_load_user_sprite_area("<CashBook$Dir>.Sprites");

	sprintf (res_temp, "%s.Menus", resources);
	templates_load_menus(res_temp);

	snprintf(res_temp, sizeof(res_temp), "%s.Templates", resources);
	templates_open(res_temp);

	load_templates(&windows, sprites);

	iconbar_initialise();
	choices_initialise();
	analysis_initialise();
	budget_initialise();
	find_initialise();
	goto_initialise();
	purge_initialise();

	ihelp_initialise();
	url_initialise();
	printing_initialise();
	report_initialise(sprites);

	templates_close();

	/* Initialise the legacy menu system. */

	templates_link_menu_dialogue("file_info", windows.file_info);
	templates_link_menu_dialogue("save_as", windows.save_as);

	menus.main            = templates_get_menu(TEMPLATES_MENU_MAIN);
	menus.account_sub     = templates_get_menu(TEMPLATES_MENU_MAIN_ACCOUNTS);
	menus.transaction_sub = templates_get_menu(TEMPLATES_MENU_MAIN_TRANSACTIONS);
	menus.analysis_sub    = templates_get_menu(TEMPLATES_MENU_MAIN_ANALYSIS);
	menus.acclist         = templates_get_menu(TEMPLATES_MENU_ACCLIST);
	menus.accview         = templates_get_menu(TEMPLATES_MENU_ACCVIEW);
	menus.sorder          = templates_get_menu(TEMPLATES_MENU_SORDER);
	menus.preset          = templates_get_menu(TEMPLATES_MENU_PRESET);

	menus.accopen         = NULL;
	menus.account         = NULL;

	/* Initialise the file update mechanism: calling it now with no files loaded will force the date to be set up. */

	update_files_for_new_date();

	hourglass_off();
}


/**
 * Take the command line and parse it for useful arguments.
 */

static void main_parse_command_line(int argc, char *argv[])
{
	int	i;

	if (argc > 1) {
		for (i=1; i<argc; i++) {
			if (strcmp(argv[i], "-file") == 0 && i+1 < argc)
				load_transaction_file(argv[i+1]);
		}
	}
}


/**
 * Handle incoming Message_Quit.
 */

static osbool main_message_quit(wimp_message *message)
{
	main_quit_flag = TRUE;

	return TRUE;
}


/**
 * Handle incoming Message_PreQuit.
 */

static osbool main_message_prequit(wimp_message *message)
{
	if (!check_for_unsaved_files())
		return TRUE;

	message->your_ref = message->my_ref;
	wimp_send_message(wimp_USER_MESSAGE_ACKNOWLEDGE, message, message->sender);

	return TRUE;
}






















/* ================================================================================================================== */

/* Load templates into memory and either create windows or store definitions. */

static void load_templates(global_windows *windows, osspriteop_area *sprites)
{
  wimp_window  *window_def;


  /* File Info Window.
   *
   * Created now.
   */

  windows->file_info = templates_create_window("FileInfo");
  ihelp_add_window (windows->file_info, "FileInfo", NULL);

  /* Import Complete Window.
   *
   * Created now.
   */

  windows->import_comp = templates_create_window("ImpComp");
  ihelp_add_window (windows->import_comp, "ImpComp", NULL);

  /* Save Window.
   *
   * Created now.
   */

  windows->save_as = templates_create_window("SaveAs");
  ihelp_add_window (windows->save_as, "SaveAs", NULL);

  /* Edit Account Window.
   *
   * Created now.
   */

    windows->edit_acct = templates_create_window("EditAccount");
    ihelp_add_window (windows->edit_acct, "EditAccount", NULL);

  /* Edit Heading Window.
   *
   * Created now.
   */

    windows->edit_hdr = templates_create_window("EditHeading");
    ihelp_add_window (windows->edit_hdr, "EditHeading", NULL);

  /* Edit Section Window.
   *
   * Created now.
   */

    windows->edit_sect = templates_create_window("EditAccSect");
    ihelp_add_window (windows->edit_sect, "EditAccSect", NULL);

  /* Edit Standing Order Window.
   *
   * Created now.
   */

    windows->edit_sorder = templates_create_window("EditSOrder");
    ihelp_add_window (windows->edit_sorder, "EditSOrder", NULL);

  /* Edit Preset Window.
   *
   * Created now.
   */

    windows->edit_preset = templates_create_window("EditPreset");
    ihelp_add_window (windows->edit_preset, "EditPreset", NULL);

   /* Transaction Report Window.
   *
   * Created now.
   */

    windows->trans_rep = templates_create_window("TransRep");
    ihelp_add_window (windows->trans_rep, "TransRep", NULL);

  /* Unreconciled Transaction Report Window.
   *
   * Created now.
   */

    windows->unrec_rep = templates_create_window("UnrecRep");
    ihelp_add_window (windows->unrec_rep, "UnrecRep", NULL);

  /* Cashflow Report Window.
   *
   * Created now.
   */

    windows->cashflow_rep = templates_create_window("CashFlwRep");
    ihelp_add_window (windows->cashflow_rep, "CashFlwRep", NULL);

  /* Balance Report Window.
   *
   * Created now.
   */

    windows->balance_rep = templates_create_window("BalanceRep");
    ihelp_add_window (windows->balance_rep, "BalanceRep", NULL);

  /* Account Name Enter Window.
   *
   * Created now.
   */

    windows->enter_acc = templates_create_window("AccEnter");
    ihelp_add_window (windows->enter_acc, "AccEnter", NULL);


  /* Sort Transactions Window.
   *
   * Created now.
   */

    windows->sort_trans = templates_create_window("SortTrans");
    ihelp_add_window (windows->sort_trans, "SortTrans", NULL);

  /* Sort AccView Window.
   *
   * Created now.
   */

     windows->sort_accview = templates_create_window("SortAccView");
    ihelp_add_window (windows->sort_accview, "SortAccView", NULL);

  /* Sort SOrder Window.
   *
   * Created now.
   */

    windows->sort_sorder = templates_create_window("SortSOrder");
    ihelp_add_window (windows->sort_sorder, "SortSOrder", NULL);

  /* Sort Preset Window.
   *
   * Created now.
   */

    windows->sort_preset = templates_create_window("SortPreset");
    ihelp_add_window (windows->sort_preset, "SortPreset", NULL);

  /* Save Report Window.
   *
   * Created now.
   */

    windows->save_rep = templates_create_window("SaveRepTemp");
    ihelp_add_window (windows->save_rep, "SaveRepTemp", NULL);


  /* Transaction Window.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("Transact");
    window_def->icon_count = 0;
    windows->transaction_window_def = window_def;

  /* Transaction Pane.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("TransactTB");
    window_def->sprite_area = sprites;
    windows->transaction_pane_def = window_def;

  /* Account Window.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("Account");
    window_def->icon_count = 0;
    windows->account_window_def = window_def;

  /* Account Pane.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("AccountATB");
    window_def->sprite_area = sprites;
    windows->account_pane_def[0] = window_def;

   /* Account Footer.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("AccountTot");
    windows->account_footer_def = window_def;

  /* Heading Pane.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("AccountHTB");
    window_def->sprite_area = sprites;
    windows->account_pane_def[1] = window_def;

  /* Standing Order Window.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("SOrder");
    window_def->icon_count = 0;
    windows->sorder_window_def = window_def;

  /* Standing Order Pane.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("SOrderTB");
    window_def->sprite_area = sprites;
    windows->sorder_pane_def = window_def;

  /* Account View Window.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("AccView");
    window_def->icon_count = 0;
    windows->accview_window_def = window_def;

  /* Account View Pane.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("AccViewTB");
    window_def->sprite_area = sprites;
    windows->accview_pane_def = window_def;

  /* Preset Window.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("Preset");
    window_def->icon_count = 0;
    windows->preset_window_def = window_def;

  /* Preset Pane.
   *
   * Definition loaded for future use.
   */

  window_def = templates_load_window("PresetTB");
    window_def->sprite_area = sprites;
    windows->preset_pane_def = window_def;
}




/* ==================================================================================================================
 * Mouse click handler
 */

static void mouse_click_handler (wimp_pointer *pointer)
{
  int       result;
  file_data *file;

  extern global_windows windows;


  /* Import complete window. */

  if (pointer->w == windows.import_comp)
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
      icons_set_selected (windows.edit_hdr, pointer->i, 1);
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
      icons_set_selected (windows.edit_sect, pointer->i, 1);
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
      icons_set_group_shaded_when_off (windows.edit_sorder, SORDER_EDIT_AVOID, 2,
                                       SORDER_EDIT_SKIPFWD, SORDER_EDIT_SKIPBACK);
    }

    if (pointer->i == SORDER_EDIT_FIRSTSW) /* First amount radio icon */
    {
      icons_set_group_shaded_when_off (windows.edit_sorder, SORDER_EDIT_FIRSTSW, 1,
                                       SORDER_EDIT_FIRST);
      icons_replace_caret_in_window (windows.edit_sorder);
    }

    if (pointer->i == SORDER_EDIT_LASTSW) /* Last amount radio icon */
    {
      icons_set_group_shaded_when_off (windows.edit_sorder, SORDER_EDIT_LASTSW, 1,
                                       SORDER_EDIT_LAST);
      icons_replace_caret_in_window (windows.edit_sorder);
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
      icons_set_selected (windows.edit_sorder  , pointer->i, 1);
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
      icons_set_group_shaded_when_on (windows.edit_preset, PRESET_EDIT_TODAY, 1,
                                       PRESET_EDIT_DATE);
      icons_replace_caret_in_window (windows.edit_preset);
    }

    if (pointer->i == PRESET_EDIT_CHEQUE) /* Cheque radio icon */
    {
      icons_set_group_shaded_when_on (windows.edit_preset, PRESET_EDIT_CHEQUE, 1,
                                       PRESET_EDIT_REF);
      icons_replace_caret_in_window (windows.edit_preset);
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
      icons_set_selected (windows.edit_preset, pointer->i, 1);
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
      icons_set_group_shaded_when_on (windows.trans_rep, ANALYSIS_TRANS_BUDGET, 4,
                                      ANALYSIS_TRANS_DATEFROMTXT, ANALYSIS_TRANS_DATEFROM,
                                      ANALYSIS_TRANS_DATETOTXT, ANALYSIS_TRANS_DATETO);
      icons_replace_caret_in_window (windows.trans_rep);
    }

    else if (pointer->i == ANALYSIS_TRANS_GROUP)
    {
      icons_set_group_shaded_when_off (windows.trans_rep, ANALYSIS_TRANS_GROUP, 6,
                                  ANALYSIS_TRANS_PERIOD, ANALYSIS_TRANS_PTEXT,
                                  ANALYSIS_TRANS_PDAYS, ANALYSIS_TRANS_PMONTHS, ANALYSIS_TRANS_PYEARS,
                                  ANALYSIS_TRANS_LOCK);
      icons_replace_caret_in_window (windows.trans_rep);
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_TRANS_PDAYS || pointer->i == ANALYSIS_TRANS_PMONTHS ||
         pointer->i == ANALYSIS_TRANS_PYEARS)) /* Radio icons */
    {
      icons_set_selected (windows.trans_rep, pointer->i, 1);
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
      icons_set_group_shaded_when_on (windows.unrec_rep, ANALYSIS_UNREC_BUDGET, 4,
                                      ANALYSIS_UNREC_DATEFROMTXT, ANALYSIS_UNREC_DATEFROM,
                                      ANALYSIS_UNREC_DATETOTXT, ANALYSIS_UNREC_DATETO);
      icons_replace_caret_in_window (windows.unrec_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUP)
    {
      icons_set_group_shaded_when_off (windows.unrec_rep, ANALYSIS_UNREC_GROUP, 2,
                                       ANALYSIS_UNREC_GROUPACC, ANALYSIS_UNREC_GROUPDATE);

      icons_set_group_shaded (windows.unrec_rep,
                         !(icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP) &&
                           icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPDATE)),
                        6,
                        ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
                        ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);

      icons_replace_caret_in_window (windows.unrec_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUPACC || pointer->i == ANALYSIS_UNREC_GROUPDATE)
    {
      icons_set_selected (windows.unrec_rep, pointer->i, 1);

      icons_set_group_shaded (windows.unrec_rep,
                         !(icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP) &&
                           icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPDATE)),
                        6,
                        ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
                        ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);

      icons_replace_caret_in_window (windows.unrec_rep);
    }

    else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_UNREC_PDAYS || pointer->i == ANALYSIS_UNREC_PMONTHS ||
         pointer->i == ANALYSIS_UNREC_PYEARS)) /* Radio icons */
    {
      icons_set_selected (windows.unrec_rep, pointer->i, 1);
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
      icons_set_group_shaded_when_on (windows.cashflow_rep, ANALYSIS_CASHFLOW_BUDGET, 4,
                                      ANALYSIS_CASHFLOW_DATEFROMTXT, ANALYSIS_CASHFLOW_DATEFROM,
                                      ANALYSIS_CASHFLOW_DATETOTXT, ANALYSIS_CASHFLOW_DATETO);
      icons_replace_caret_in_window (windows.cashflow_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUP)
    {
      icons_set_group_shaded_when_off (windows.cashflow_rep, ANALYSIS_CASHFLOW_GROUP, 7,
                                       ANALYSIS_CASHFLOW_PERIOD, ANALYSIS_CASHFLOW_PTEXT, ANALYSIS_CASHFLOW_LOCK,
                                       ANALYSIS_CASHFLOW_PDAYS, ANALYSIS_CASHFLOW_PMONTHS, ANALYSIS_CASHFLOW_PYEARS,
                                       ANALYSIS_CASHFLOW_EMPTY);

      icons_replace_caret_in_window (windows.cashflow_rep);
    }

     else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_CASHFLOW_PDAYS || pointer->i == ANALYSIS_CASHFLOW_PMONTHS ||
         pointer->i == ANALYSIS_CASHFLOW_PYEARS)) /* Radio icons */
    {
      icons_set_selected (windows.cashflow_rep, pointer->i, 1);
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
      icons_set_group_shaded_when_on (windows.balance_rep, ANALYSIS_BALANCE_BUDGET, 4,
                                      ANALYSIS_BALANCE_DATEFROMTXT, ANALYSIS_BALANCE_DATEFROM,
                                      ANALYSIS_BALANCE_DATETOTXT, ANALYSIS_BALANCE_DATETO);
      icons_replace_caret_in_window (windows.balance_rep);
    }

    else if (pointer->i == ANALYSIS_UNREC_GROUP)
    {
      icons_set_group_shaded_when_off (windows.balance_rep, ANALYSIS_BALANCE_GROUP, 6,
                                       ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_PTEXT, ANALYSIS_BALANCE_LOCK,
                                       ANALYSIS_BALANCE_PDAYS, ANALYSIS_BALANCE_PMONTHS, ANALYSIS_BALANCE_PYEARS);

      icons_replace_caret_in_window (windows.balance_rep);
    }

     else if (pointer->buttons == wimp_CLICK_ADJUST &&
        (pointer->i == ANALYSIS_BALANCE_PDAYS || pointer->i == ANALYSIS_BALANCE_PMONTHS ||
         pointer->i == ANALYSIS_BALANCE_PYEARS)) /* Radio icons */
    {
      icons_set_selected (windows.balance_rep, pointer->i, 1);
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
      icons_set_selected (windows.sort_trans, pointer->i, 1);
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
      icons_set_selected (windows.sort_accview, pointer->i, 1);
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
      icons_set_selected (windows.sort_sorder, pointer->i, 1);
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
      icons_set_selected (windows.sort_preset, pointer->i, 1);
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
}

/* ==================================================================================================================
 * Keypress handler
 */

static void key_press_handler (wimp_key *key)
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

static void menu_selection_handler (wimp_selection *selection)
{
  wimp_pointer          pointer;

  extern global_menus   menus;


  /* Store the mouse status before decoding the menu. */

  wimp_get_pointer_info (&pointer);

  /* Decode the main menu. */

  if (menus.menu_id == MENU_ID_MAIN)
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

  /* Decode the report list menu when it appears apart from the main menu. */

  else if (menus.menu_id == MENU_ID_REPLIST)
  {
    mainmenu_decode_replist_menu (selection, &pointer);
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

static void scroll_request_handler (wimp_scroll *scroll)
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

static void user_message_handler (wimp_message *message)
{
  extern int          main_quit_flag;
  extern global_menus menus;

  static char         *clipboard_data;
  int                 clipboard_size;


  switch (message->action)
  {
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

        transfer_load_reply_datasave_block (message, &clipboard_data);
      }
      else
      {
        /* Your_ref == 0, so this is a fresh approach and something wants us to load a file. */

        #ifdef DEBUG
        debug_reporter_text0 ("\\OLoading a file from another application");
        #endif

        if (initialise_data_load (message))
        {
          transfer_load_reply_datasave_callback (message, drag_end_load);
        }
      }
      break;

    case message_DATA_SAVE_ACK:
      transfer_save_reply_datasaveack (message);
      break;

    case message_DATA_LOAD:
       if (message->your_ref != 0)
      {
        /* If your_ref != 0, there was a Message_DataSave before this.  clipboard_size will only return >0 if
         * the data is loaded automatically: since only the clipboard uses this, the following code is safe.
         * All other loads (data files, TSV and CSV) will have supplied a function to handle the physical loading
         * and clipboard_size will return as 0.
         */

        clipboard_size = transfer_load_reply_dataload (message, NULL);
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
          transfer_load_start_direct_callback (message, drag_end_load);
          transfer_load_reply_dataload (message, NULL);
        }
      }
      break;

    case message_RAM_FETCH:
      transfer_save_reply_ramfetch (message, main_task_handle);
      break;

    case message_RAM_TRANSMIT:
      clipboard_size = transfer_load_reply_ramtransmit (message, NULL);
      if (clipboard_size > 0)
      {
        paste_received_clipboard (&clipboard_data, clipboard_size);
      }
      break;

    case message_DATA_OPEN:
      start_data_open_load (message);
      break;

    case message_MENUS_DELETED:
      if (menus.menu_id == MENU_ID_ACCOUNT)
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
      break;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

static void bounced_message_handler (wimp_message *message)
{
  switch (message->action)
  {
    case message_RAM_TRANSMIT:
      error_msgs_report_error ("RAMXferFail");
      break;

    case message_RAM_FETCH:
      transfer_load_bounced_ramfetch (message);
      break;
  }
}
