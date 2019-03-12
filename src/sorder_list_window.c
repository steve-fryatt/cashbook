/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: sorder_list_window.c
 *
 * Standing Order List Window implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

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
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "sorder.h"

#include "account.h"
#include "account_menu.h"
#include "accview.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "print_dialogue.h"
#include "sorder_dialogue.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
#include "transact.h"
#include "window.h"

/* Standing Order List Window icons. */

#define SORDER_ICON_FROM 0
#define SORDER_ICON_FROM_REC 1
#define SORDER_ICON_FROM_NAME 2
#define SORDER_ICON_TO 3
#define SORDER_ICON_TO_REC 4
#define SORDER_ICON_TO_NAME 5
#define SORDER_ICON_AMOUNT 6
#define SORDER_ICON_DESCRIPTION 7
#define SORDER_ICON_NEXTDATE 8
#define SORDER_ICON_LEFT 9

/* Standing Order List Window Toolbar icons. */

#define SORDER_PANE_FROM 0
#define SORDER_PANE_TO 1
#define SORDER_PANE_AMOUNT 2
#define SORDER_PANE_DESCRIPTION 3
#define SORDER_PANE_NEXTDATE 4
#define SORDER_PANE_LEFT 5

#define SORDER_PANE_PARENT 6
#define SORDER_PANE_ADDSORDER 7
#define SORDER_PANE_PRINT 8
#define SORDER_PANE_SORT 9

#define SORDER_PANE_SORT_DIR_ICON 10

/* Standing Order List Window Menu entries. */

#define SORDER_MENU_SORT 0
#define SORDER_MENU_EDIT 1
#define SORDER_MENU_NEWSORDER 2
#define SORDER_MENU_EXPCSV 3
#define SORDER_MENU_EXPTSV 4
#define SORDER_MENU_PRINT 5
#define SORDER_MENU_FULLREP 6

/* Standing order window details. */

#define SORDER_COLUMNS 10
#define SORDER_TOOLBAR_HEIGHT 132
#define MIN_SORDER_ENTRIES 10

/* Standing Order Window column mapping. */

static struct column_map sorder_columns[SORDER_COLUMNS] = {
	{SORDER_ICON_FROM, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_FROM_REC, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_FROM_NAME, SORDER_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_ICON_TO, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_TO_REC, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_TO_NAME, SORDER_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_ICON_AMOUNT, SORDER_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{SORDER_ICON_DESCRIPTION, SORDER_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION},
	{SORDER_ICON_NEXTDATE, SORDER_PANE_NEXTDATE, wimp_ICON_WINDOW, SORT_NEXTDATE},
	{SORDER_ICON_LEFT, SORDER_PANE_LEFT, wimp_ICON_WINDOW, SORT_LEFT}
};

