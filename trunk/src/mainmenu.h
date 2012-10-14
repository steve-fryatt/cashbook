/* Copyright 2003-2012, Stephen Fryatt
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
 * \file: mainmenu.c
 *
 * Legacy central menu implementation.
 */

#ifndef CASHBOOK_MAINMENU
#define CASHBOOK_MAINMENU

#include "account.h"

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */


/* Date menu */

#define DATE_MENU_TODAY 0

/* RefDesc menu */

#define REFDESC_MENU_CHEQUE 0

/* General definitions */

#define ACCOUNT_MENU_TITLE_LEN 32


#define REFDESC_MENU_BLOCKSIZE 50

/* ==================================================================================================================
 * Type definitions
 */



/* Account menu */

void open_account_menu (file_data *file, enum account_menu_type type, int line,
                        wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer);

/* Date menu */

void open_date_menu(file_data *file, int line, wimp_pointer *pointer);

/* RefDesc Menu */

void open_refdesc_menu (file_data *file, int menu_type, int line, wimp_pointer *pointer);

#endif

