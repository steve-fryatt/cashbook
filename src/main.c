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
#include "amenu.h"
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
static void user_message_handler (wimp_message *);
static void bounced_message_handler (wimp_message *);





/* Declare the global variables that are used. */

global_windows          windows;
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
	os_t		poll_time;
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

			case wimp_OPEN_WINDOW_REQUEST:
				wimp_open_window(&(blk.open));
				break;

			case wimp_CLOSE_WINDOW_REQUEST:
				wimp_close_window(blk.close.w);
				break;

			case wimp_MOUSE_CLICK:
				mouse_click_handler(&(blk.pointer));
				break;

			case wimp_KEY_PRESSED:
				key_press_handler(&(blk.key));
				break;

			case wimp_MENU_SELECTION:
				amenu_selection_handler(&(blk.selection));
				break;

			case wimp_USER_DRAG_BOX:
				switch (global_drag_type) {
				case SAVE_DRAG:
					terminate_user_drag(&(blk.dragged));
					break;

				case ACCOUNT_DRAG:
					terminate_account_drag(&(blk.dragged));
					break;
				}
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

	config_opt_init("IyonixKeys", (osbyte1(osbyte_IN_KEY, 0, 0xff) == 0xaa));	/**< Use RISC OS 5 Delete: default true only on an Iyonix.		*/
	config_opt_init("GlobalClipboardSupport", TRUE);				/**< Support the global clipboard in the transaction window.		*/

	config_opt_init("RememberValues", TRUE);					/**< Remember previous values in dialogue boxes.			*/

	config_opt_init("AllowTransDelete", TRUE);					/**< Enable the use of Ctrl-F10 to delete whole transactions.		*/

	config_int_init("MaxAutofillLen", 0);						/**< Maximum entries in Ref or Descript Complete Menus (0 = no limit).	*/

	config_opt_init("AutoSort", TRUE);						/**< Automatically sort transaction list display on entry.		*/

	config_opt_init("ShadeReconciled", FALSE);
	config_int_init("ShadeReconciledColour", wimp_COLOUR_MID_LIGHT_GREY);

	config_opt_init("ShadeBudgeted", FALSE);
	config_int_init("ShadeBudgetedColour", wimp_COLOUR_MID_LIGHT_GREY);

	config_opt_init("ShadeOverdrawn", FALSE);
	config_int_init("ShadeOverdrawnColour", wimp_COLOUR_RED);

	config_opt_init("ShadeAccounts", FALSE);
	config_int_init("ShadeAccountsColour", wimp_COLOUR_RED);

	config_opt_init("TerritoryDates", TRUE);					/**< Take date information from Territory module.			*/
	config_str_init("DateSepIn", "-/\\.");						/**< List of characters to be accepted as input date separators.	*/
	config_str_init("DateSepOut", "-");						/**< The character to use as output date separator.			*/

	config_opt_init("TerritoryCurrency", TRUE);					/**< Take currency information from the Territory module.		*/
	config_opt_init("PrintZeros", FALSE);						/**< Print zero values, instead of leaving cells blank.			*/
	config_opt_init("BracketNegatives", FALSE);					/**< Show negative values as "(1.00)" instead of "-1.00".		*/
	config_int_init("DecimalPlaces", 2);						/**< The number of decimal places in the local currency.		*/
	config_str_init("DecimalPoint", ".");						/**< The character to use for a decimal point.				*/

	config_opt_init("SortAfterSOrders", TRUE);					/**< Automatically sort transaction list view after adding SOs.		*/
	config_opt_init("AutoSortSOrders", TRUE);					/**< Automatically sort SO list on entry.				*/
	config_opt_init("TerritorySOrders", TRUE);					/**< Take weekend day info for SOs from Territory module.		*/
	config_int_init("WeekendDays", 0x41);						/**< Manual set weekends for SOs (bit 0 = Sunday; bit 6 = Saturday).	*/

	config_opt_init("AutoSortPresets", TRUE);					/**< Automatically sort presets on entry.				*/

	config_str_init("ReportFontNormal", "Homerton.Medium");				/**< Normal weight font name for reporting and printing.		*/
	config_str_init("ReportFontBold", "Homerton.Bold");				/**< Bold weight font name for reporting and printing.			*/
	config_int_init("ReportFontSize", 12);						/**< Report and print font size (points).				*/
	config_int_init("ReportFontLinespace", 130);					/**< Report and print linespacing (percent of font size).		*/

	config_opt_init("PrintFitWidth", TRUE);						/**< Fit printout to one page width when PrintText == FALSE.		*/
	config_opt_init("PrintRotate", FALSE);						/**< Print Landscape when PrintText == FALSE.				*/
	config_opt_init("PrintPageNumbers", TRUE);					/**< Include page numbers when PrintText == FALSE.			*/
	config_opt_init("PrintText", FALSE);						/**< Print in legacy text mode instead of graphics.			*/
	config_opt_init("PrintTextFormat", TRUE);					/**< Include Fancy Text formatting when PrintText == TRUE.		*/

	config_int_init("PrintMarginTop", 0);						/**< Paper top margin (millipoints).					*/
	config_int_init("PrintMarginLeft", 0);						/**< Paper left margin (millipoints).					*/
	config_int_init("PrintMarginRight", 0);						/**< Paper right margin (millipoints).					*/
	config_int_init("PrintMarginBottom", 0);					/**< Paper bottom margin (millipoints).					*/
	config_int_init("PrintMarginInternal", 18000);					/**< Header/Footer margin (millipoints).				*/
	config_int_init("PrintMarginUnits", 0);						/**< Page units used in Choices (0 = mm, 1 = cm, 2 = inch).		*/

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
	filing_initialise();

	transact_initialise(sprites);
	account_initialise(sprites);
	accview_initialise(sprites);
	sorder_initialise(sprites);
	preset_initialise(sprites);

	amenu_initialise();
	ihelp_initialise();
	url_initialise();
	printing_initialise();
	report_initialise(sprites);

	templates_close();

	/* Initialise the legacy menu system. */

	templates_link_menu_dialogue("file_info", windows.file_info);
	templates_link_menu_dialogue("save_as", windows.save_as);

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
  /* File Info Window.
   *
   * Created now.
   */

  windows->file_info = templates_create_window("FileInfo");
  ihelp_add_window (windows->file_info, "FileInfo", NULL);

  /* Save Window.
   *
   * Created now.
   */

  windows->save_as = templates_create_window("SaveAs");
  ihelp_add_window (windows->save_as, "SaveAs", NULL);

  /* Account Name Enter Window.
   *
   * Created now.
   */

    windows->enter_acc = templates_create_window("AccEnter");
    ihelp_add_window (windows->enter_acc, "AccEnter", NULL);
}




/* ==================================================================================================================
 * Mouse click handler
 */

static void mouse_click_handler (wimp_pointer *pointer)
{
  extern global_windows windows;

  /* Save window. */

  if (pointer->w == windows.save_as)
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
}

/* ==================================================================================================================
 * Keypress handler
 */

static void key_press_handler (wimp_key *key)
{
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
}



/* ==================================================================================================================
 * User message handlers
 */

static void user_message_handler (wimp_message *message)
{
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
