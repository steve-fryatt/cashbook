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
 * OR CONDITIONS OF ANY KIND, either express or implied.a
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: transact_list_window.c
 *
 * Transaction List Window implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

/* OSLib header files */

#include "oslib/dragasprite.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"
#include "oslib/territory.h"

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
#include "transact.h"

#include "account.h"
#include "account_list_menu.h"
#include "account_menu.h"
#include "accview.h"
#include "analysis.h"
#include "analysis_template_menu.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "file_info.h"
#include "filing.h"
#include "find.h"
#include "flexutils.h"
#include "goto.h"
#include "presets.h"
#include "preset_menu.h"
#include "print_dialogue.h"
#include "purge.h"
#include "refdesc_menu.h"
#include "report.h"
#include "sorder.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "transact.h"
#include "window.h"

