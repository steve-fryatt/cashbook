/* CashBook - fileinfo.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/territory.h"
#include "oslib/os.h"

/* SF-Lib header files. */

#include "sflib/icons.h"
#include "sflib/msgs.h"

/* Application header files */

#include "global.h"
#include "fileinfo.h"

#include "account.h"
#include "file.h"

/* ==================================================================================================================
 * Global variables.
 */

/* ==================================================================================================================
 *
 */

void fill_file_info_window (file_data *file)
{
  extern global_windows windows;


  /* Now fill the window. */

  make_file_pathname (file, icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_FILENAME), 255);

  if (check_for_filepath (file))
  {
    territory_convert_standard_date_and_time (territory_CURRENT, (os_date_and_time const *) file->datestamp,
                                              icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_DATE), 30);
  }
  else
  {
    msgs_lookup ("UnSaved", icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_DATE), 30);
  }

  if (file->modified)
  {
    msgs_lookup ("Yes", icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_MODIFIED), 12);
  }
  else
  {
    msgs_lookup ("No", icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_MODIFIED), 12);
  }

  sprintf (icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_TRANSACT), "%d", file->trans_count);
  sprintf (icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_SORDERS), "%d", file->sorder_count);
  sprintf (icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_PRESETS), "%d", file->preset_count);
  sprintf (icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_ACCOUNTS), "%d",
           count_accounts_in_file (file, ACCOUNT_FULL));
  sprintf (icons_get_indirected_text_addr (windows.file_info, FILEINFO_ICON_HEADINGS), "%d",
           count_accounts_in_file (file, ACCOUNT_IN | ACCOUNT_OUT));
}
