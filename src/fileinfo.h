/* CashBook - fileinfo.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_FILEINFO
#define _ACCOUNTS_FILEINFO

/* ==================================================================================================================
 * Static constants
 */

#define FILEINFO_ICON_FILENAME 1
#define FILEINFO_ICON_MODIFIED 3
#define FILEINFO_ICON_DATE 5
#define FILEINFO_ICON_ACCOUNTS 9
#define FILEINFO_ICON_TRANSACT 11
#define FILEINFO_ICON_HEADINGS 13
#define FILEINFO_ICON_SORDERS 15
#define FILEINFO_ICON_PRESETS 17

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

void fill_file_info_window (file_data *file);

#endif
