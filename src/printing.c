/* CashBook - printing.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/pdriver.h"
#include "oslib/os.h"
#include "oslib/osfile.h"

/* SF-Lib header files. */

#include "sflib/general.h"
#include "sflib/errors.h"
#include "sflib/debug.h"
#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/config.h"

/* Application header files */

#include "global.h"
#include "printing.h"

#include "caret.h"
#include "dataxfer.h"
#include "date.h"

/* This code deals with a "RISC OS 2" subset of the printer driver protocol.  We can start print jobs off via
 * the correct set of codes, but all printing is done immediately and the queue mechanism is ignored.
 *
 * This really needs to be addressed in a future version.
 */

/* ==================================================================================================================
 * Global variables.
 */

/* PRint protocol negotiations. */

static void (*external_print_function) (char *);
static void (*external_cancel_function) (void);

static int  print_mode_text;

/* Printer details update. */

static void (*update_printer_details) (void) = NULL;

/* Simple print window handling. */

static void (*simple_print_start) (int, int, int, int);

static char simple_print_title [64];

static file_data *simple_print_file = NULL;

static int  simple_print_clear;

/* Date-range print window handling. */

static void (*date_print_start) (int, int, int, int, date_t, date_t);

static char date_print_title [64];

static file_data *date_print_file = NULL;

static int  date_print_clear;

/* ==================================================================================================================
 * Printer Protocol handling
 */

/* Send a Message_PrintSave to start the printing process off with the printer driver. */

int send_start_print_save (void (*print_function) (char *), void (*cancel_function) (void), int text_print)
{
  wimp_full_message_data_xfer  datasave;
  os_error                     *error;


  #ifdef DEBUG
  debug_printf ("Sending Message_PrintFile");
  #endif

  external_print_function = print_function;
  print_mode_text         = text_print;

  /* Set up and send Message_PrintSave. */

  datasave.size = WORDALIGN(45 + strlen (""));
  datasave.your_ref = 0;
  datasave.action = message_PRINT_SAVE;

  datasave.w = NULL;
  datasave.i = 0;
  datasave.pos.x = 0;
  datasave.pos.y = 0;
  datasave.est_size = 0;
  datasave.file_type = 0;
  *datasave.file_name = '\0';

  error = xwimp_send_message (wimp_USER_MESSAGE_RECORDED, (wimp_message *) &datasave, wimp_BROADCAST);
  if (error != NULL)
  {
    wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
    return -1;
  }

  return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Deal with a bounced Message_PrintSave. */

void handle_bounced_message_print_save (void)
{
  #ifdef DEBUG
  debug_printf ("Message_PrintSave bounced");
  #endif

  if (!print_mode_text)
  {
    external_print_function ("");
  }
  else
  {
    wimp_msgtrans_error_report ("NoPManager");
  }

  if (external_cancel_function != NULL)
  {
    external_cancel_function ();
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Deal with a Message_PrintError. */

void handle_message_print_error (wimp_message *message)
{
  pdriver_full_message_print_error *print_error = (pdriver_full_message_print_error *) message;


  #ifdef DEBUG
  debug_printf ("Received Message_PrintError");
  #endif

  /* If the message block size is 20, this is a RISC OS 2 style Message_PrintBusy. */

  if (print_error->size == 20)
  {
    wimp_msgtrans_error_report ("PrintBusy");
  }
  else
  {
    wimp_error_report (print_error->errmess);
  }

  if (external_cancel_function != NULL)
  {
    external_cancel_function ();
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Deal with a Message_PrintFile. */

void handle_message_print_file (wimp_message *message)
{
  char filename[256];
  int  length;

  wimp_full_message_data_xfer  *print_file = (wimp_full_message_data_xfer *) message;
  os_error                     *error;


  #ifdef DEBUG
  debug_printf ("Received Message_PrintFile");
  #endif

  if (print_mode_text)
  {
    /* Text mode printing.  Find the filename for the Print-temp file. */

    xos_read_var_val ("Printer$Temp", filename, sizeof (filename), 0, os_VARTYPE_STRING, &length, NULL, NULL);
    *(filename+length) = '\0';

    /* Call the printing function with the PrintTemp filename. */

    external_print_function (filename);

    /* Set up the Message_DataLoad and send it to Printers.  File size and file type are read from the actual file
     * on disc, using OS_File.
     */

    print_file->your_ref = print_file->my_ref;
    print_file->action = message_DATA_LOAD;

    osfile_read_stamped_no_path (filename, NULL, NULL, &(print_file->est_size), NULL, &(print_file->file_type));
    strcpy (print_file->file_name, filename);

    print_file->size = WORDALIGN(45 + strlen (filename));

    error = xwimp_send_message (wimp_USER_MESSAGE, (wimp_message *) print_file, print_file->sender);
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
    }
  }
  else
  {
    print_file->your_ref = print_file->my_ref;
    print_file->action = message_WILL_PRINT;

    error = xwimp_send_message (wimp_USER_MESSAGE, (wimp_message *) print_file, print_file->sender);
    if (error != NULL)
    {
      wimp_os_error_report (error, wimp_ERROR_BOX_CANCEL_ICON);
    }
    else
    {
      external_print_function ("");
    }
  }
}

/* ==================================================================================================================
 * Printer settings update
 */

/* Call the registered printer details update handler function.  This will do things like updating the printer name in
 * a print dialogue box.
 */

void handle_message_set_printer (void)
{
  if (update_printer_details != NULL)
  {
    update_printer_details ();
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Register an update handler function. */

void register_printer_update_handler (void (update_handler) (void))
{
  if (update_printer_details == NULL)
  {
    update_printer_details = update_handler;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* De-register an update handler function. */

void deregister_printer_update_handler (void (update_handler) (void))
{
  if (update_printer_details == update_handler)
  {
    update_printer_details = NULL;
  }
}

/* ==================================================================================================================
 * Printing via the simple print GUI.
 */

/* Open the simple print dialoge box, as used by a number of print routines.  The file is taken, to allow the
 * dialogue to be closed, as is a window title token and a pointer to a function to call when the dialogue is
 * closed.
 */

void open_simple_print_window (file_data *file, wimp_pointer *ptr, int clear, char *title,
                               void (print_start) (int, int, int, int))
{
  extern global_windows windows;


  /* If the window is already open, another print job is being set up.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.simple_print))
  {
    wimp_close_window (windows.simple_print);
  }

  if (windows_get_open (windows.date_print))
  {
    wimp_close_window (windows.date_print);
  }

  simple_print_file = file;
  simple_print_start = print_start;
  simple_print_clear = clear;

  /* Set the window contents up. */

  strcpy (simple_print_title, title);
  fill_simple_print_window (&(file->print), clear);

  /* Open the window on screen. */

  windows_open_centred_at_pointer (windows.simple_print, ptr);
  place_dialogue_caret (windows.simple_print, wimp_ICON_WINDOW);

  register_printer_update_handler (refresh_simple_print_window);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_simple_print_window (void)
{
  extern global_windows windows;

  fill_simple_print_window (&(simple_print_file->print), simple_print_clear);
  icons_replace_caret_in_window (windows.simple_print);

  xwimp_force_redraw_title (windows.simple_print);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_simple_print_window (print *print_data, int clear)
{
  char     *name, buffer[25];
  os_error *error;

  extern global_windows windows;

  error = xpdriver_info (NULL, NULL, NULL, NULL, &name, NULL, NULL, NULL);

  if (error != NULL)
  {
    msgs_lookup ("NoPDriverT", buffer, sizeof (buffer));
    name = buffer;
  }

  msgs_param_lookup (simple_print_title, windows_get_indirected_title_addr (windows.simple_print), 64, name, NULL, NULL, NULL);

  if (clear == 0)
  {
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_STANDARD, !config_opt_read ("PrintText"));
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_PORTRAIT, !config_opt_read ("PrintRotate"));
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_LANDSCAPE, config_opt_read ("PrintRotate"));
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_SCALE, config_opt_read ("PrintFitWidth"));

    icons_set_selected (windows.simple_print, SIMPLE_PRINT_FASTTEXT, config_opt_read ("PrintText"));
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_TEXTFORMAT, config_opt_read ("PrintTextFormat"));
  }
  else
  {
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_STANDARD, !print_data->text);
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_PORTRAIT, !print_data->rotate);
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_LANDSCAPE, print_data->rotate);
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_SCALE, print_data->fit_width);

    icons_set_selected (windows.simple_print, SIMPLE_PRINT_FASTTEXT, print_data->text);
    icons_set_selected (windows.simple_print, SIMPLE_PRINT_TEXTFORMAT, print_data->text_format);
  }

  icons_set_group_shaded_when_off (windows.simple_print, SIMPLE_PRINT_STANDARD, 3,
                                   SIMPLE_PRINT_PORTRAIT, SIMPLE_PRINT_LANDSCAPE, SIMPLE_PRINT_SCALE);
  icons_set_group_shaded_when_off (windows.simple_print, SIMPLE_PRINT_FASTTEXT, 1,
                                   SIMPLE_PRINT_TEXTFORMAT);

  icons_set_shaded (windows.simple_print, SIMPLE_PRINT_OK, error != NULL);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated edit heading window and process the data. */

void process_simple_print_window (void)
{
  extern global_windows windows;


  #ifdef DEBUG
  debug_printf ("\\BProcessing the Simple Print window");
  #endif

  /* Extract the information and call the originator's print start function. */

  simple_print_file->print.fit_width = icons_get_selected (windows.simple_print, SIMPLE_PRINT_SCALE);
  simple_print_file->print.rotate = icons_get_selected (windows.simple_print, SIMPLE_PRINT_LANDSCAPE);
  simple_print_file->print.text = icons_get_selected (windows.simple_print, SIMPLE_PRINT_FASTTEXT);
  simple_print_file->print.text_format = icons_get_selected (windows.simple_print, SIMPLE_PRINT_TEXTFORMAT);

  simple_print_start (simple_print_file->print.text,
                      simple_print_file->print.text_format,
                      simple_print_file->print.fit_width,
                      simple_print_file->print.rotate);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the report format window if the file disappears. */

void force_close_print_windows (file_data *file)
{
  extern global_windows windows;


  if (simple_print_file == file && windows_get_open (windows.simple_print))
  {
    close_dialogue_with_caret (windows.simple_print);
  }

  if (date_print_file == file && windows_get_open (windows.date_print))
  {
    close_dialogue_with_caret (windows.date_print);
  }
}

/* ==================================================================================================================
 * Printing via the date-range print GUI.
 */

/* Open the date-range print dialoge box, as used by a number of print routines.  The file is taken, to allow the
 * dialogue to be closed, as is a window title token and a pointer to a function to call when the dialogue is
 * closed.
 */

void open_date_print_window (file_data *file, wimp_pointer *ptr, int clear, char *title,
                             void (print_start) (int, int, int, int, date_t, date_t))
{
  extern global_windows windows;


  /* If the window is already open, another print job is being set up.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.simple_print))
  {
    wimp_close_window (windows.simple_print);
  }

  if (windows_get_open (windows.date_print))
  {
    wimp_close_window (windows.date_print);
  }

  date_print_file = file;
  date_print_start = print_start;
  date_print_clear = clear;

  /* Set the window contents up. */

  strcpy (date_print_title, title);
  fill_date_print_window (&(file->print), clear);

  /* Open the window on screen. */

  windows_open_centred_at_pointer (windows.date_print, ptr);
  place_dialogue_caret (windows.date_print, DATE_PRINT_FROM);

  register_printer_update_handler (refresh_date_print_window);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_date_print_window (void)
{
  extern global_windows windows;

  fill_date_print_window (&(date_print_file->print), date_print_clear);
  icons_replace_caret_in_window (windows.date_print);

  xwimp_force_redraw_title (windows.date_print);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_date_print_window (print *print_data, int clear)
{
  char     *name, buffer[25];
  os_error *error;

  extern global_windows windows;

  error = xpdriver_info (NULL, NULL, NULL, NULL, &name, NULL, NULL, NULL);

  if (error != NULL)
  {
    msgs_lookup ("NoPDriverT", buffer, sizeof (buffer));
    name = buffer;
  }

  msgs_param_lookup (date_print_title, windows_get_indirected_title_addr (windows.date_print), 64, name, NULL, NULL, NULL);

  if (clear == 0)
  {
    icons_set_selected (windows.date_print, DATE_PRINT_STANDARD, !config_opt_read ("PrintText"));
    icons_set_selected (windows.date_print, DATE_PRINT_PORTRAIT, !config_opt_read ("PrintRotate"));
    icons_set_selected (windows.date_print, DATE_PRINT_LANDSCAPE, config_opt_read ("PrintRotate"));
    icons_set_selected (windows.date_print, DATE_PRINT_SCALE, config_opt_read ("PrintFitWidth"));

    icons_set_selected (windows.date_print, DATE_PRINT_FASTTEXT, config_opt_read ("PrintText"));
    icons_set_selected (windows.date_print, DATE_PRINT_TEXTFORMAT, config_opt_read ("PrintTextFormat"));

    *icons_get_indirected_text_addr (windows.date_print, DATE_PRINT_FROM) = '\0';
    *icons_get_indirected_text_addr (windows.date_print, DATE_PRINT_TO) = '\0';
  }
  else
  {
    icons_set_selected (windows.date_print, DATE_PRINT_STANDARD, !print_data->text);
    icons_set_selected (windows.date_print, DATE_PRINT_PORTRAIT, !print_data->rotate);
    icons_set_selected (windows.date_print, DATE_PRINT_LANDSCAPE, print_data->rotate);
    icons_set_selected (windows.date_print, DATE_PRINT_SCALE, print_data->fit_width);

    icons_set_selected (windows.date_print, DATE_PRINT_FASTTEXT, print_data->text);
    icons_set_selected (windows.date_print, DATE_PRINT_TEXTFORMAT, print_data->text_format);

    convert_date_to_string (print_data->from, icons_get_indirected_text_addr (windows.date_print, DATE_PRINT_FROM));
    convert_date_to_string (print_data->to, icons_get_indirected_text_addr (windows.date_print, DATE_PRINT_TO));
  }

  icons_set_group_shaded_when_off (windows.date_print, DATE_PRINT_STANDARD, 3,
                                   DATE_PRINT_PORTRAIT, DATE_PRINT_LANDSCAPE, DATE_PRINT_SCALE);
  icons_set_group_shaded_when_off (windows.date_print, DATE_PRINT_FASTTEXT, 1,
                                   DATE_PRINT_TEXTFORMAT);

  icons_set_shaded (windows.date_print, DATE_PRINT_OK, error != NULL);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated date-range print window and process the data. */

void process_date_print_window (void)
{
  extern global_windows windows;


  #ifdef DEBUG
  debug_printf ("\\BProcessing the Simple Print window");
  #endif

  /* Extract the information and call the originator's print start function. */

  date_print_file->print.fit_width = icons_get_selected (windows.date_print, DATE_PRINT_SCALE);
  date_print_file->print.rotate = icons_get_selected (windows.date_print, DATE_PRINT_LANDSCAPE);
  date_print_file->print.text = icons_get_selected (windows.date_print, DATE_PRINT_FASTTEXT);
  date_print_file->print.text_format = icons_get_selected (windows.date_print, DATE_PRINT_TEXTFORMAT);

  date_print_file->print.from = convert_string_to_date (icons_get_indirected_text_addr (windows.date_print, DATE_PRINT_FROM),
                                                        NULL_DATE, 0);
  date_print_file->print.to = convert_string_to_date (icons_get_indirected_text_addr (windows.date_print, DATE_PRINT_TO),
                                                      NULL_DATE, 0);

  date_print_start (date_print_file->print.text,
                    date_print_file->print.text_format,
                    date_print_file->print.fit_width,
                    date_print_file->print.rotate,
                    date_print_file->print.from,
                    date_print_file->print.to);
}
