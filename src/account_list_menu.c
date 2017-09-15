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
 * \file: account_menu.c
 *
 * Account completion menu interface.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/icons.h"
#include "sflib/heap.h"
#include "sflib/msgs.h"
#include "sflib/menus.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "account_list_menu.h"

#include "account.h"
#include "amenu.h"
#include "transact.h"


/**
 * The length of the menu title buffer.
 */

#define ACCOUNT_LIST_MENU_TITLE_LEN 32


/* Data Structures. */

struct account_list_menu_link {
	char		name[ACCOUNT_NAME_LEN];
	acct_t		account;
};


/* Clobal Variables. */

/**
 * Pointer to the file currently owning the menu.
 */

static struct file_block		*account_list_menu_file = NULL;

/**
 * The menu block.
 */

static wimp_menu			*account_list_menu = NULL;

/**
 * The associated menu entry data.
 */

static struct account_list_menu_link	*account_list_menu_entry_link = NULL;

/**
 * Memory to hold the indirected menu title.
 */

static char				account_list_menu_title[ACCOUNT_LIST_MENU_TITLE_LEN];



/**
 * Build an Account List menu for a file, and return the pointer.  This is a
 * list of Full Accounts, used for opening a Account List view.
 *
 * \param *file			The file to build the menu for.
 * \return			The created menu, or NULL for an error.
 */


wimp_menu *account_list_menu_build(struct file_block *file)
{
	int	i, line, accounts, display_lines, width;
	acct_t	account;
	char	*name;

	account_list_menu_destroy();

	if (file == NULL || file->accounts == NULL)
		return NULL;

	/* Find out how many accounts there are. */

	accounts = account_count_type_in_file(file, ACCOUNT_FULL);

	if (accounts == 0)
		return NULL;

	#ifdef DEBUG
	debug_printf("\\GBuilding account menu for %d accounts", accounts);
	#endif

	/* Claim enough memory to build the menu in. */

	account_list_menu = heap_alloc(28 + (24 * accounts));
	account_list_menu_entry_link = heap_alloc(sizeof(struct account_list_menu_link) * accounts);

	if (account_list_menu == NULL || account_list_menu_entry_link == NULL) {
		account_list_menu_destroy();
		return NULL;
	}

	account_list_menu_file = file;

	/* Populate the menu. */

	display_lines = account_get_list_length(file, ACCOUNT_FULL);

	line = 0;
	i = 0;
	width = 0;

	while (line < accounts && i < display_lines) {
		switch (account_get_list_entry_type(file, ACCOUNT_FULL, i)) {
		case ACCOUNT_LINE_DATA:
			account = account_get_list_entry_account(file, ACCOUNT_FULL, i);

			/* If the line is an account, add it to the manu... */

			if (account == NULL_ACCOUNT) {
				i++;
				continue;
			}

			name = account_get_name(file, account);
			if (name == NULL) {
				i++;
				continue;
			}

			/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
			 * well move while the menu is open.  The account number is also stored, to allow the account to be found.
			 */

			string_copy(account_list_menu_entry_link[line].name, name, ACCOUNT_NAME_LEN);
			account_list_menu_entry_link[line].account = account;

			if (strlen(account_list_menu_entry_link[line].name) > width)
				width = strlen(account_list_menu_entry_link[line].name);

			/* Set the menu and icon flags up. */

			account_list_menu->entries[line].menu_flags = 0;

			account_list_menu->entries[line].sub_menu = (wimp_menu *) -1;
			account_list_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			account_list_menu->entries[line].data.indirected_text.text = account_list_menu_entry_link[line].name;
			account_list_menu->entries[line].data.indirected_text.validation = NULL;
			account_list_menu->entries[line].data.indirected_text.size = ACCOUNT_NAME_LEN;

			#ifdef DEBUG
			debug_printf ("Line %d: '%s'", line, account_list_menu_entry_link[line].name);
			#endif

			line++;
			break;

		case ACCOUNT_LINE_HEADER:
			/* If the line is a header, and the menu has an item in it, add a separator... */

			if (line > 0)
				account_list_menu->entries[line - 1].menu_flags |= wimp_MENU_SEPARATE;
			break;

		case ACCOUNT_LINE_BLANK:
		case ACCOUNT_LINE_FOOTER:
			break;
		}

		i++;
	}

	account_list_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	msgs_lookup("ViewaccMenuTitle", account_list_menu_title, ACCOUNT_LIST_MENU_TITLE_LEN);
	account_list_menu->title_data.indirected_text.text = account_list_menu_title;
	account_list_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_list_menu->title_fg = wimp_COLOUR_BLACK;
	account_list_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_list_menu->work_fg = wimp_COLOUR_BLACK;
	account_list_menu->work_bg = wimp_COLOUR_WHITE;

	account_list_menu->width = (width + 1) * 16;
	account_list_menu->height = 44;
	account_list_menu->gap = 0;

	return account_list_menu;
}


/**
 * Destroy any Account List menu which is currently open.
 */

void account_list_menu_destroy(void)
{
	if (account_list_menu != NULL)
		heap_free(account_list_menu);

	if (account_list_menu_entry_link != NULL)
		heap_free(account_list_menu_entry_link);

	account_list_menu = NULL;
	account_list_menu_entry_link = NULL;
	*account_list_menu_title = '\0';
	account_list_menu_file = NULL;
}


/**
 * Prepare the Account List menu for opening or reopening, by ticking those
 * accounts which have Account List windows already open.
 */

void account_list_menu_prepare(void)
{
	int i = 0;

	if (account_list_menu == NULL || account_list_menu_entry_link == NULL || account_list_menu_file == NULL)
		return;

	do {
		if (account_get_accview(account_list_menu_file, account_list_menu_entry_link[i].account) != NULL)
			account_list_menu->entries[i].menu_flags |= wimp_MENU_TICKED;
		else
			account_list_menu->entries[i].menu_flags &= ~wimp_MENU_TICKED;
	} while ((account_list_menu->entries[i++].menu_flags & wimp_MENU_LAST) == 0);
}



/**
 * Decode a selection from the Account List menu, converting to an account
 * number.
 *
 * \param selection		The menu selection to decode.
 * \return			The account numer, or NULL_ACCOUNT.
 */

acct_t account_list_menu_decode(int selection)
{
	if (account_list_menu_entry_link == NULL || selection == -1)
		return NULL_ACCOUNT;

	return account_list_menu_entry_link[selection].account;
}
