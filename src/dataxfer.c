/* CashBook - dataxfer.c
 *
 * (C) Stephen Fryatt, 2003
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

#include "sflib/msgs.h"
#include "sflib/errors.h"
#include "sflib/icons.h"
#include "sflib/debug.h"
#include "sflib/transfer.h"
#include "sflib/string.h"
#include "sflib/menus.h"

/* Application header files */

#include "global.h"
#include "dataxfer.h"

#include "account.h"
#include "calculation.h"
#include "file.h"
#include "filing.h"
#include "report.h"
#include "transact.h"

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
  extern global_windows windows;


  /* If a window has been opened already, remember the filename that was entered. */

  if (savebox_window != SAVE_BOX_NONE)
  {
    strcpy (savebox_filename[savebox_window], icons_get_indirected_text_addr (windows.save_as, 2));
  }

  /* Set up the box for the new dialogue. */

  #ifdef DEBUG
  debug_printf ("Filename: '%s'", savebox_filename[new_window]);
  debug_printf ("Sprite: '%s'", savebox_sprites[new_window]);
  #endif

  strcpy (icons_get_indirected_text_addr (windows.save_as, 2), savebox_filename[new_window]);
  strcpy (icons_get_indirected_text_addr (windows.save_as, 3), savebox_sprites[new_window]);

  savebox_window = new_window;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called to deal with File->Save in the menu being selected. */

void start_direct_menu_save (file_data *file)
{
  wimp_pointer pointer;

  extern global_windows windows;


  if (check_for_filepath (file))
  {
    save_transaction_file (file, file->filename);
  }
  else
  {
    wimp_get_pointer_info (&pointer);
    fill_save_as_window (file, SAVE_BOX_FILE);
    menus_create_standard_menu ((wimp_menu *) windows.save_as, &pointer);
  }
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

  extern global_windows windows;
  extern int global_drag_type;


  /* Get the basic information about the window and icon. */

  window.w = windows.save_as;
  wimp_get_window_state (&window);

  ox = window.visible.x0 - window.xscroll;
  oy = window.visible.y1 - window.yscroll;

  icon.w = windows.save_as;
  icon.i = 3;
  wimp_get_icon_state (&icon);


  /* Set up the drag parameters. */

  drag.w = windows.save_as;
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

  global_drag_type = SAVE_DRAG;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called from Wimp_Poll when a save drag has terminated. */

void terminate_user_drag (wimp_dragged *drag)
{
  wimp_pointer pointer;
  char         *leafname;
  int          filetype = 0;

  extern global_windows windows;


  if (dragging_sprite)
  {
    dragasprite_stop ();
  }

  leafname = find_leafname (icons_get_indirected_text_addr (windows.save_as, 2));

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
    send_start_data_save_function (pointer.w, pointer.i, pointer.pos, 0, drag_end_save, 0, filetype, leafname);
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
      export_delimited_file (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_TSV:
      export_delimited_file (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCCSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCTSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWCSV:
      export_delimited_account_file (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWTSV:
      export_delimited_account_file (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERCSV:
      export_delimited_sorder_file (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERTSV:
      export_delimited_sorder_file (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETCSV:
      export_delimited_preset_file (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETTSV:
      export_delimited_preset_file (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
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

  extern global_windows windows;


  #ifdef DEBUG
  debug_printf ("\\DSave with mouse-click or RETURN");
  #endif

  filename = icons_get_indirected_text_addr (windows.save_as, 2);

  /* Test if the filename is valid or not.  Exit if not. */

  if (strchr (filename, '.') == NULL)
  {
    wimp_msgtrans_info_report ("DragSave");
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
      export_delimited_file (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_TSV:
      export_delimited_file (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCCSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCTSV:
      export_delimited_accounts_file (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWCSV:
      export_delimited_account_file (saving_file, saving_object, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_ACCVIEWTSV:
      export_delimited_account_file (saving_file, saving_object, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERCSV:
      export_delimited_sorder_file (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_SORDERTSV:
      export_delimited_sorder_file (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETCSV:
      export_delimited_preset_file (saving_file, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
      break;

    case SAVE_BOX_PRESETTSV:
      export_delimited_preset_file (saving_file, filename, DELIMIT_TAB, TSV_FILE_TYPE);
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

int drag_end_load (char *filename)
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

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handle the receipt of a Message_DataLoad */

int start_data_open_load (wimp_message *message)
{
  wimp_full_message_data_xfer *xfer = (wimp_full_message_data_xfer *) message;
  os_error                    *error;


  switch (xfer->file_type)
  {
    case CASHBOOK_FILE_TYPE:
      xfer->your_ref = xfer->my_ref;
      xfer->action = message_DATA_LOAD_ACK;

      error = xwimp_send_message (wimp_USER_MESSAGE, (wimp_message *) xfer, xfer->sender);
      if (error != NULL)
      {
        wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
        return -1;
      }

      load_transaction_file (xfer->file_name);
      break;
  }

  return 0;
}
