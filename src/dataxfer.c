/* CashBook - dataxfer.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/dragasprite.h"
#include "oslib/wimpspriteop.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/transfer.h"

/* Application header files */

#include "global.h"
#include "dataxfer.h"

#include "account.h"
#include "accview.h"
#include "calculation.h"
#include "file.h"
#include "filing.h"
#include "ihelp.h"
#include "main.h"
#include "presets.h"
#include "report.h"
#include "sorder.h"
#include "templates.h"
#include "transact.h"


#define DATAXFER_SAVEAS_OK 0
#define DATAXFER_SAVEAS_CANCEL 1
#define DATAXFER_SAVEAS_FILE 3



/* ==================================================================================================================
 * Static global variables
 */

static char      savebox_filename[SAVE_BOXES][256],
                 savebox_sprites[SAVE_BOXES][20];

static int       savebox_window,
                 dragging_sprite;

static file_data *saving_file;
static int       saving_object; /* Save specific data. */


static int       loading_filetype;
static file_data *loading_target;

static int       delete_file_after;



/* Save As Window. */

static wimp_w			dataxfer_saveas_window = NULL;			/**< The handle of the Save As window.			*/



static osbool			dataxfer_message_datasave(wimp_message *message);
static osbool			dataxfer_message_dataload(wimp_message *message);
static osbool			dataxfer_message_datasaveack(wimp_message *message);
static osbool			dataxfer_message_ramfetch(wimp_message *message);
static osbool			dataxfer_bounced_message_ramtransfer(wimp_message *message);
static osbool			dataxfer_bounced_message_ramfetch(wimp_message *message);


static void			dataxfer_saveas_click_handler(wimp_pointer *pointer);
static osbool			dataxfer_saveas_keypress_handler(wimp_key *key);



static osbool			dataxfer_message_dataopen(wimp_message *message);

static int			dataxfer_drag_end_load(char *filename);

static void			dataxfer_terminate_drag(wimp_dragged *drag, void *data);

/**
 * Initialise the data transfer system.
 *
 * \param *sprites		The application sprite area.
 */

void dataxfer_initialise(void)
{
	dataxfer_saveas_window = templates_create_window("SaveAs");
	ihelp_add_window(dataxfer_saveas_window, "SaveAs", NULL);
	event_add_window_mouse_event(dataxfer_saveas_window, dataxfer_saveas_click_handler);
	event_add_window_key_event(dataxfer_saveas_window, dataxfer_saveas_keypress_handler);
	templates_link_menu_dialogue("save_as", dataxfer_saveas_window);

	event_add_message_handler(message_DATA_SAVE, EVENT_MESSAGE_INCOMING, dataxfer_message_datasave);
	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, dataxfer_message_dataload);
	event_add_message_handler(message_DATA_SAVE_ACK, EVENT_MESSAGE_INCOMING, dataxfer_message_datasaveack);
	event_add_message_handler(message_RAM_FETCH, EVENT_MESSAGE_INCOMING, dataxfer_message_ramfetch);
	event_add_message_handler(message_DATA_OPEN, EVENT_MESSAGE_INCOMING, dataxfer_message_dataopen);

	event_add_message_handler(message_RAM_TRANSMIT, EVENT_MESSAGE_ACKNOWLEDGE, dataxfer_bounced_message_ramtransfer);
	event_add_message_handler(message_RAM_FETCH, EVENT_MESSAGE_ACKNOWLEDGE, dataxfer_bounced_message_ramfetch);
}


/**
 * Handle incoming Message_DataSave.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool dataxfer_message_datasave(wimp_message *message)
{
	if (message->your_ref != 0)
		return FALSE;

	if (initialise_data_load(message))
		transfer_load_reply_datasave_callback(message, dataxfer_drag_end_load);

	return TRUE;
}


/**
 * Handle incoming Message_DataLoad.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool dataxfer_message_dataload(wimp_message *message)
{
	if (message->your_ref != 0)
		return FALSE;

	if (initialise_data_load(message)) {
		transfer_load_start_direct_callback(message, dataxfer_drag_end_load);
		transfer_load_reply_dataload(message, NULL);
	}

	return TRUE;
}


/**
 * Handle Message_DataSaveAck.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool dataxfer_message_datasaveack(wimp_message *message)
{
	transfer_save_reply_datasaveack(message);

	return TRUE;
}


/**
 * Handle Message_RamFetch.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool dataxfer_message_ramfetch(wimp_message *message)
{
	transfer_save_reply_ramfetch(message, main_task_handle);

	return TRUE;
}


/**
 * Handle bounced Message_RamTransmit.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool dataxfer_bounced_message_ramtransfer(wimp_message *message)
{
	error_msgs_report_error("RAMXferFail");

	return TRUE;
}


/**
 * Handle bounced Message_RamFetch.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool dataxfer_bounced_message_ramfetch(wimp_message *message)
{
	transfer_load_bounced_ramfetch(message);

	return TRUE;
}





/* ==================================================================================================================
 * Initialise and prepare the save boxes.
 */

/* Called when the main menu is opened, to set up all the save boxes.  Can also be called before opening a save box
 * directly, say in response to F3.
 */

void initialise_save_boxes (file_data *file, int object, int delete_after)
{
  /* Set the initial filenames up. */

  if (check_for_filepath (file))
  {
    strcpy (savebox_filename[SAVE_BOX_FILE], file->filename);
  }
  else
  {
    msgs_lookup ("DefTransFile", savebox_filename[SAVE_BOX_FILE], sizeof (savebox_filename[SAVE_BOX_FILE]));
  }

  msgs_lookup ("DefCSVFile", savebox_filename[SAVE_BOX_CSV], sizeof (savebox_filename[SAVE_BOX_CSV]));
  msgs_lookup ("DefTSVFile", savebox_filename[SAVE_BOX_TSV], sizeof (savebox_filename[SAVE_BOX_TSV]));
  msgs_lookup ("DefCSVFile", savebox_filename[SAVE_BOX_ACCCSV], sizeof (savebox_filename[SAVE_BOX_ACCCSV]));
  msgs_lookup ("DefTSVFile", savebox_filename[SAVE_BOX_ACCTSV], sizeof (savebox_filename[SAVE_BOX_ACCTSV]));
  msgs_lookup ("DefCSVFile", savebox_filename[SAVE_BOX_ACCVIEWCSV], sizeof (savebox_filename[SAVE_BOX_ACCVIEWCSV]));
  msgs_lookup ("DefTSVFile", savebox_filename[SAVE_BOX_ACCVIEWTSV], sizeof (savebox_filename[SAVE_BOX_ACCVIEWTSV]));
  msgs_lookup ("DefCSVFile", savebox_filename[SAVE_BOX_SORDERCSV], sizeof (savebox_filename[SAVE_BOX_SORDERCSV]));
  msgs_lookup ("DefTSVFile", savebox_filename[SAVE_BOX_SORDERTSV], sizeof (savebox_filename[SAVE_BOX_SORDERTSV]));
  msgs_lookup ("DefCSVFile", savebox_filename[SAVE_BOX_PRESETCSV], sizeof (savebox_filename[SAVE_BOX_PRESETCSV]));
  msgs_lookup ("DefTSVFile", savebox_filename[SAVE_BOX_PRESETTSV], sizeof (savebox_filename[SAVE_BOX_PRESETTSV]));
  msgs_lookup ("DefRepFile", savebox_filename[SAVE_BOX_REPTEXT], sizeof (savebox_filename[SAVE_BOX_REPTEXT]));
  msgs_lookup ("DefCSVFile", savebox_filename[SAVE_BOX_REPCSV], sizeof (savebox_filename[SAVE_BOX_REPCSV]));
  msgs_lookup ("DefTSVFile", savebox_filename[SAVE_BOX_REPTSV], sizeof (savebox_filename[SAVE_BOX_REPTSV]));

  /* Set the default sprites up. */

  strcpy (savebox_sprites[SAVE_BOX_FILE], "file_1ca");
  strcpy (savebox_sprites[SAVE_BOX_CSV], "file_dfe");
  strcpy (savebox_sprites[SAVE_BOX_TSV], "file_fff");
  strcpy (savebox_sprites[SAVE_BOX_ACCCSV], "file_dfe");
  strcpy (savebox_sprites[SAVE_BOX_ACCTSV], "file_fff");
  strcpy (savebox_sprites[SAVE_BOX_ACCVIEWCSV], "file_dfe");
  strcpy (savebox_sprites[SAVE_BOX_ACCVIEWTSV], "file_fff");
  strcpy (savebox_sprites[SAVE_BOX_SORDERCSV], "file_dfe");
  strcpy (savebox_sprites[SAVE_BOX_SORDERTSV], "file_fff");
  strcpy (savebox_sprites[SAVE_BOX_PRESETCSV], "file_dfe");
  strcpy (savebox_sprites[SAVE_BOX_PRESETTSV], "file_fff");
  strcpy (savebox_sprites[SAVE_BOX_REPTEXT], "file_fff");
  strcpy (savebox_sprites[SAVE_BOX_REPCSV], "file_dfe");
  strcpy (savebox_sprites[SAVE_BOX_REPTSV], "file_fff");

  savebox_window = SAVE_BOX_NONE;
  saving_file = file;
  saving_object = object;

  delete_file_after =  delete_after;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called after initialise_save_boxes(), before a save dialogue is opened. */

void fill_save_as_window (file_data *file, int new_window)
{
 /* If a window has been opened already, remember the filename that was entered. */

  if (savebox_window != SAVE_BOX_NONE)
  {
    strcpy (savebox_filename[savebox_window], icons_get_indirected_text_addr (dataxfer_saveas_window, 2));
  }

  /* Set up the box for the new dialogue. */

  #ifdef DEBUG
  debug_printf ("Filename: '%s'", savebox_filename[new_window]);
  debug_printf ("Sprite: '%s'", savebox_sprites[new_window]);
  #endif

  strcpy (icons_get_indirected_text_addr (dataxfer_saveas_window, 2), savebox_filename[new_window]);
  strcpy (icons_get_indirected_text_addr (dataxfer_saveas_window, 3), savebox_sprites[new_window]);

  savebox_window = new_window;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called to deal with File->Save in the menu being selected. */

void start_direct_menu_save (file_data *file)
{
  wimp_pointer pointer;


  if (check_for_filepath (file))
  {
    save_transaction_file (file, file->filename);
  }
  else
  {
    wimp_get_pointer_info (&pointer);
    fill_save_as_window (file, SAVE_BOX_FILE);
    menus_create_standard_menu ((wimp_menu *) dataxfer_saveas_window, &pointer);
  }
}


/**
 * Open the Save As dialogue at the pointer.
 *
 * \param *pointer		The pointer location to open the dialogue.
 */

void dataxfer_open_saveas_window(wimp_pointer *pointer)
{
	menus_create_standard_menu((wimp_menu *) dataxfer_saveas_window, pointer);
}


/**
 * Process mouse clicks in the Save As dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void dataxfer_saveas_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case DATAXFER_SAVEAS_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			wimp_create_menu(NULL, 0, 0);
		break;

	case DATAXFER_SAVEAS_OK:
		if (pointer->buttons == wimp_CLICK_SELECT)
			immediate_window_save();
		break;

	case DATAXFER_SAVEAS_FILE:
		if (pointer->buttons == wimp_CLICK_SELECT)
			start_save_window_drag();
		break;
	}
}


/**
 * Process keypresses in the Save As dialogue.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool dataxfer_saveas_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		immediate_window_save();
		break;

	case wimp_KEY_ESCAPE:
		wimp_create_menu(NULL, 0, 0);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}













/* ==================================================================================================================
 * Save box drag handling.
 */

/* Start dragging the icon from the save dialogue. */

void start_save_window_drag (void)
{
  wimp_window_state     window;
  wimp_icon_state       icon;
  wimp_drag             drag;
  int                   ox, oy;


  /* Get the basic information about the window and icon. */

  window.w = dataxfer_saveas_window;
  wimp_get_window_state (&window);

  ox = window.visible.x0 - window.xscroll;
  oy = window.visible.y1 - window.yscroll;

  icon.w = dataxfer_saveas_window;
  icon.i = 3;
  wimp_get_icon_state (&icon);


  /* Set up the drag parameters. */

  drag.w = dataxfer_saveas_window;
  drag.type = wimp_DRAG_USER_FIXED;

  drag.initial.x0 = ox + icon.icon.extent.x0;
  drag.initial.y0 = oy + icon.icon.extent.y0;
  drag.initial.x1 = ox + icon.icon.extent.x1;
  drag.initial.y1 = oy + icon.icon.extent.y1;

  drag.bbox.x0 = 0x80000000;
  drag.bbox.y0 = 0x80000000;
  drag.bbox.x1 = 0x7fffffff;
  drag.bbox.y1 = 0x7fffffff;


  /* Read CMOS RAM to see if solid drags are required. */

  dragging_sprite = ((osbyte2 (osbyte_READ_CMOS, osbyte_CONFIGURE_DRAG_ASPRITE, 0) &
                     osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0);

  if (dragging_sprite)
  {
    dragasprite_start (dragasprite_HPOS_CENTRE | dragasprite_VPOS_CENTRE | dragasprite_NO_BOUND |
                       dragasprite_BOUND_POINTER | dragasprite_DROP_SHADOW, wimpspriteop_AREA,
                       icon.icon.data.indirected_text.text, &(drag.initial), &(drag.bbox));
  }
  else
  {
    wimp_drag_box (&drag);
  }

  event_set_drag_handler(dataxfer_terminate_drag, NULL, NULL);
}


/**
 * Handle drag-end events relating to save icon dragging.
 *
 * \param *drag			The Wimp drag end data.
 * \param *data			Unused client data sent via Event Lib.
 */

static void dataxfer_terminate_drag(wimp_dragged *drag, void *data)
{
  wimp_pointer pointer;
  char         *leafname;
  int          filetype = 0;


  if (dragging_sprite)
  {
    dragasprite_stop ();
  }

  leafname = string_find_leafname (icons_get_indirected_text_addr (dataxfer_saveas_window, 2));

  #ifdef DEBUG
  debug_printf ("\\DBegin data transfer");
  debug_printf ("Leafname: %s", leafname);
  #endif

  switch (savebox_window)
  {
    case SAVE_BOX_FILE:
      filetype = CASHBOOK_FILE_TYPE;
      break;

    case SAVE_BOX_REPTEXT:
      filetype = TEXT_FILE_TYPE;
      break;

    case SAVE_BOX_CSV:
    case SAVE_BOX_ACCCSV:
    case SAVE_BOX_ACCVIEWCSV:
    case SAVE_BOX_SORDERCSV:
    case SAVE_BOX_PRESETCSV:
    case SAVE_BOX_REPCSV:
      filetype = CSV_FILE_TYPE;
      break;

    case SAVE_BOX_TSV:
    case SAVE_BOX_ACCTSV:
    case SAVE_BOX_ACCVIEWTSV:
    case SAVE_BOX_SORDERTSV:
    case SAVE_BOX_PRESETTSV:
    case SAVE_BOX_REPTSV:
      filetype = TSV_FILE_TYPE;
      break;
  }

  if (filetype != 0)
  {
    wimp_get_pointer_info (&pointer);
    transfer_save_start_callback (pointer.w, pointer.i, pointer.pos, 0, drag_end_save, 0, filetype, leafname);
  }
}

/* ==================================================================================================================
 * Handle the saving itself.
 */

/* Called by the SFLib Transfer library when a save location has been negotiated via the data transfer protocol. */

int drag_end_save (char *filename)
{
  #ifdef DEBUG
  debug_printf ("\\DSave at drag end");
  debug_printf ("Filename: %s", filename);
  #endif

  switch (savebox_window)
  {
    case SAVE_BOX_FILE:
      save_transaction_file (saving_file, filename);
      if (delete_file_after)
      {
        delete_file (saving_file);
      }
      break;

    case SAVE_BOX_CSV:
      transact_export_delimited (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_TSV:
      transact_export_delimited (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCCSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCTSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWCSV:
      accview_export_delimited (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWTSV:
      accview_export_delimited (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERCSV:
      sorder_export_delimited (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERTSV:
      sorder_export_delimited (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETCSV:
      preset_export_delimited (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETTSV:
      preset_export_delimited (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_REPTEXT:
      save_report_text (saving_file, (report_data *) saving_object, filename, 0);
      break;

    case SAVE_BOX_REPCSV:
      export_delimited_report_file (saving_file, (report_data *) saving_object, filename,
                                    DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_REPTSV:
      export_delimited_report_file (saving_file, (report_data *) saving_object, filename,
                                    DELIMIT_TAB, TSV_FILE_TYPE);
      break;
  }

  wimp_create_menu (NULL, 0, 0);

  return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called when OK is clicked or Return pressed in the save dialogue. */

int immediate_window_save (void)
{
  char *filename;


  #ifdef DEBUG
  debug_printf ("\\DSave with mouse-click or RETURN");
  #endif

  filename = icons_get_indirected_text_addr (dataxfer_saveas_window, 2);

  /* Test if the filename is valid or not.  Exit if not. */

  if (strchr (filename, '.') == NULL)
  {
    error_msgs_report_info ("DragSave");
    return 0;
  }

  switch (savebox_window)
  {
    case SAVE_BOX_FILE:
      save_transaction_file (saving_file, filename);
      if (delete_file_after)
      {
        delete_file (saving_file);
      }
      break;

    case SAVE_BOX_CSV:
      transact_export_delimited (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_TSV:
      transact_export_delimited (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCCSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCTSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWCSV:
      accview_export_delimited (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWTSV:
      accview_export_delimited (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERCSV:
      sorder_export_delimited (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERTSV:
      sorder_export_delimited (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETCSV:
      preset_export_delimited (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETTSV:
      preset_export_delimited (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_REPTEXT:
      save_report_text (saving_file, (report_data *) saving_object, filename, 0);
      break;

    case SAVE_BOX_REPCSV:
      export_delimited_report_file (saving_file, (report_data *) saving_object, filename,
                                    DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_REPTSV:
      export_delimited_report_file (saving_file, (report_data *) saving_object, filename,
                                    DELIMIT_TAB, TSV_FILE_TYPE);
      break;
  }

  wimp_create_menu (NULL, 0, 0);
  return 1;
}


/* ==================================================================================================================
 * Prepare file loading
 */

int initialise_data_load (wimp_message *message)
{
  wimp_full_message_data_xfer *xfer = (wimp_full_message_data_xfer *) message;
  file_data                   *target;


  loading_filetype = -1;

  switch (xfer->file_type)
  {
    case CASHBOOK_FILE_TYPE:
      if (xfer->w == wimp_ICON_BAR)
      {
        loading_filetype = CASHBOOK_FILE_TYPE;
      }
      break;

    case CSV_FILE_TYPE:
      if (((target = find_transaction_window_file_block (xfer->w)) != NULL) ||
          ((target = find_transaction_pane_file_block (xfer->w)) != NULL))
      {
          loading_filetype = CSV_FILE_TYPE;
          loading_target = target;
      }
      break;

    case TSV_FILE_TYPE:
      loading_filetype = TSV_FILE_TYPE;
      break;
  }

  return (loading_filetype != -1);
}

/* ------------------------------------------------------------------------------------------------------------------ */

static int dataxfer_drag_end_load(char *filename)
{
  #ifdef DEBUG
  debug_printf ("\\DLoad at drag end");
  debug_printf ("Filename: %s, type: %03x", filename, loading_filetype);
  #endif

  switch (loading_filetype)
  {
    case CASHBOOK_FILE_TYPE:
      load_transaction_file (filename);
      break;

    case CSV_FILE_TYPE:
      import_csv_file (loading_target, filename);
      break;
  }

  return 0;
}

/**
 * Handle incoming Message_DataLoad.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool dataxfer_message_dataopen(wimp_message *message)
{
	wimp_full_message_data_xfer	*xfer = (wimp_full_message_data_xfer *) message;
	os_error			*error;

	switch (xfer->file_type) {
	case CASHBOOK_FILE_TYPE:
		xfer->your_ref = xfer->my_ref;
		xfer->action = message_DATA_LOAD_ACK;

		error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) xfer, xfer->sender);
		if (error != NULL) {
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return FALSE;
		}

		load_transaction_file(xfer->file_name);
		return TRUE;
		break;
	}

	return FALSE;
}
