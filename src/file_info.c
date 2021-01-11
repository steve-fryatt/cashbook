/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * OR CONDITIONS OF ANY KIND, either express or implied.a
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: file_info.c
 *
 * File Information window implementation.
 */

/* ANSI C header files */

//#include <string.h>
//#include <stdlib.h>
//#include <ctype.h>
//#include <assert.h>

/* OSLib header files */

#include "oslib/wimp.h"
//#include "oslib/os.h"
//#include "oslib/osbyte.h"
//#include "oslib/osfile.h"
//#include "oslib/hourglass.h"
//#include "oslib/osspriteop.h"
#include "oslib/territory.h"

/* SF-Lib header files. */

#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "file_info.h"

#include "account.h"
#include "presets.h"
#include "sorder.h"
#include "transact.h"

/* Window Icon Details. */

#define FILE_INFO_ICON_FILENAME 1
#define FILE_INFO_ICON_MODIFIED 3
#define FILE_INFO_ICON_DATE 5
#define FILE_INFO_ICON_ACCOUNTS 9
#define FILE_INFO_ICON_TRANSACT 11
#define FILE_INFO_ICON_HEADINGS 13
#define FILE_INFO_ICON_SORDERS 15
#define FILE_INFO_ICON_PRESETS 17


/**
 * The handle of the file info window.
 */

static wimp_w file_info_window = NULL;


/**
 * Initialise the file information dialogue.
 */

void file_info_initialise(void)
{
	file_info_window = templates_create_window("FileInfo");
	ihelp_add_window(file_info_window, "FileInfo", NULL);
	templates_link_menu_dialogue("file_info", file_info_window);
}


/**
 * Calculate the details of a file, and fill the file info dialogue.
 *
 * \param *file			The file to display data for.
 * \return			The handle of the window.
 */

wimp_w file_info_prepare_dialogue(struct file_block *file)
{
	file_get_pathname(file, icons_get_indirected_text_addr(file_info_window, FILE_INFO_ICON_FILENAME),
			icons_get_indirected_text_length(file_info_window, FILE_INFO_ICON_FILENAME));

	if (file_check_for_filepath(file))
		territory_convert_standard_date_and_time(territory_CURRENT, (os_date_and_time const *) file->datestamp,
				icons_get_indirected_text_addr(file_info_window, FILE_INFO_ICON_DATE),
				icons_get_indirected_text_length(file_info_window, FILE_INFO_ICON_DATE));
	else
		icons_msgs_lookup(file_info_window, FILE_INFO_ICON_DATE, "UnSaved");

	if (file_get_data_integrity(file))
		icons_msgs_lookup(file_info_window, FILE_INFO_ICON_MODIFIED, "Yes");
	else
		icons_msgs_lookup(file_info_window, FILE_INFO_ICON_MODIFIED, "No");

	icons_printf(file_info_window, FILE_INFO_ICON_TRANSACT, "%d", transact_get_count(file));
	icons_printf(file_info_window, FILE_INFO_ICON_SORDERS, "%d", sorder_get_count(file));
	icons_printf(file_info_window, FILE_INFO_ICON_PRESETS, "%d", preset_get_count(file));
	icons_printf(file_info_window, FILE_INFO_ICON_ACCOUNTS, "%d", account_count_type_in_file(file, ACCOUNT_FULL));
	icons_printf(file_info_window, FILE_INFO_ICON_HEADINGS, "%d", account_count_type_in_file(file, ACCOUNT_IN | ACCOUNT_OUT));

	return file_info_window;
}

