/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: main.c
 *
 * Application core and initialisation.
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
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/resources.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/tasks.h"
#include "sflib/templates.h"
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
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "file_info.h"
#include "filing.h"
#include "find.h"
#include "goto.h"
#include "iconbar.h"
#include "interest.h"
#include "presets.h"
#include "print_dialogue.h"
#include "print_protocol.h"
#include "purge.h"
#include "report.h"
#include "report_format_dialogue.h"
#include "sorder.h"
#include "transact.h"
#include "window.h"

/**
 * The size of buffer allocated to resource filename processing.
 */

#define MAIN_FILENAME_BUFFER_LEN 1024

/**
 * The size of buffer allocated to the task name.
 */

#define MAIN_TASKNAME_BUFFER_LEN 64

static void		main_poll_loop(void);
static void		main_initialise(void);
static void		main_parse_command_line(int argc, char *argv[]);
static osbool		main_message_quit(wimp_message *message);
static osbool		main_message_prequit(wimp_message *message);


/* Declare the global variables that are used. */

static struct dataxfer_memory	main_memory_handlers;

/* Cross file global variables */

wimp_t				main_task_handle;
osbool				main_quit_flag = FALSE;

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
				file_process_date_change();
				poll_time += 6000; /* Wait for a minute for the next Null poll */
				break;

			case wimp_OPEN_WINDOW_REQUEST:
				wimp_open_window(&(blk.open));
				break;

			case wimp_CLOSE_WINDOW_REQUEST:
				wimp_close_window(blk.close.w);
				break;

			case wimp_MENU_SELECTION:
				amenu_selection_handler(&(blk.selection));
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
	static char			task_name[MAIN_TASKNAME_BUFFER_LEN];
	char				resources[MAIN_FILENAME_BUFFER_LEN], res_temp[MAIN_FILENAME_BUFFER_LEN];
	osspriteop_area			*sprites;

	wimp_version_no			wimp_version;

	hourglass_on();

	strncpy(resources, "<CashBook$Dir>.Resources", MAIN_FILENAME_BUFFER_LEN);
	resources[MAIN_FILENAME_BUFFER_LEN - 1] = '\0';
	resources_find_path(resources, MAIN_FILENAME_BUFFER_LEN);

	/* Load the messages file. */

	snprintf(res_temp, MAIN_FILENAME_BUFFER_LEN, "%s.Messages", resources);
	res_temp[MAIN_FILENAME_BUFFER_LEN - 1] = '\0';
	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName", "TaskSpr", NULL);

	/* Initialise with the Wimp. */

	msgs_lookup("TaskName", task_name, MAIN_TASKNAME_BUFFER_LEN);
	main_task_handle = wimp_initialise(wimp_VERSION_RO38, task_name, NULL, &wimp_version);

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
	config_int_init("DateFormat", DATE_FORMAT_DMY);					/**< The default date format is DD-MM-YYYY.				*/
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
	config_int_init("WeekendDays", DATE_DAY_SATURDAY | DATE_DAY_SUNDAY);		/**< Manual set weekends for standing orders.				*/

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

	config_str_init("TransactCols", "100,180,88,32,362,88,32,362,200,176,808");
	config_str_init("LimTransactCols", "32,140,88,32,140,88,32,140,140,140,200");
	config_str_init("AccountCols", "88,362,176,176,176,176");
	config_str_init("LimAccountCols", "88,140,140,140,140,140");
	config_str_init("AccViewCols", "100,180,88,32,362,200,176,176,176,808");
	config_str_init("LimAccViewCols", "32,140,88,32,140,140,140,140,140,200");
	config_str_init("SOrderCols", "88,32,362,88,32,362,176,500,180,100");
	config_str_init("LimSOrderCols", "88,32,140,88,32,140,140,200,140,60");
	config_str_init("PresetCols", "120,500,88,32,362,88,32,362,176,500");
	config_str_init("LimPresetCols", "88,200,88,32,140,88,32,140,140,200");
	config_str_init("InterestCols", "180,176,176,808");
	config_str_init("LimInterestCols", "140,140,140,200");

	config_load();

	date_initialise();
	currency_initialise();

	/* Set up the dataxfer module's memory handlers, to use SFHeap. */

	main_memory_handlers.alloc = heap_alloc;
	main_memory_handlers.realloc = heap_extend;
	main_memory_handlers.free = heap_free;

	/* Load the window templates. */

	sprites = resources_load_user_sprite_area("<CashBook$Dir>.Sprites");
	if (sprites == NULL)
		error_msgs_report_fatal("NoSprites");

	snprintf(res_temp, MAIN_FILENAME_BUFFER_LEN, "%s.Menus", resources);
	res_temp[MAIN_FILENAME_BUFFER_LEN - 1] = '\0';
	templates_load_menus(res_temp);

	snprintf(res_temp, MAIN_FILENAME_BUFFER_LEN, "%s.Templates", resources);
	res_temp[MAIN_FILENAME_BUFFER_LEN - 1] = '\0';
	templates_open(res_temp);

	saveas_initialise("SaveAs", NULL);
	file_initialise();
	iconbar_initialise();
	choices_initialise();
	analysis_initialise();
	budget_initialise();
	find_initialise();
	goto_initialise();
	purge_initialise();
	interest_initialise(sprites);
	transact_initialise(sprites);
	account_initialise(sprites);
	accview_initialise(sprites);
	sorder_initialise(sprites);
	preset_initialise(sprites);
	file_info_initialise();
	filing_initialise();
	dataxfer_initialise(main_task_handle, &main_memory_handlers);
	clipboard_initialise();
	amenu_initialise();
	ihelp_initialise();
	url_initialise();
	print_dialogue_initialise();
	print_protocol_initialise();
	report_initialise(sprites);
	report_format_dialogue_initialise();

	templates_close();

	/* Initialise the file update mechanism: calling it now with no files loaded will force the date to be set up. */

	file_process_date_change();

	hourglass_off();
}


/**
 * Take the command line and parse it for useful arguments.
 *
 * \param argc			The number of parameters passed.
 * \param *argv[]		Pointer to the parameter array.
 */

static void main_parse_command_line(int argc, char *argv[])
{
	int	i;

	if (argc > 1) {
		for (i=1; i<argc; i++) {
			if (strcmp(argv[i], "-file") == 0 && i+1 < argc)
				filing_load_cashbook_file(argv[i+1]);
		}
	}
}


/**
 * Handle incoming Message_Quit.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool main_message_quit(wimp_message *message)
{
	main_quit_flag = TRUE;

	return TRUE;
}


/**
 * Handle incoming Message_PreQuit.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool main_message_prequit(wimp_message *message)
{
	if (!file_check_for_unsaved_data())
		return TRUE;

	message->your_ref = message->my_ref;
	wimp_send_message(wimp_USER_MESSAGE_ACKNOWLEDGE, message, message->sender);

	return TRUE;
}

