/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/menus.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/windows.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "mainmenu.h"

#include "account.h"
#include "accview.h"
#include "amenu.h"
#include "analysis.h"
#include "budget.h"
#include "caret.h"
#include "choices.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "find.h"
#include "file.h"
#include "fontlist.h"
#include "goto.h"
#include "main.h"
#include "presets.h"
#include "purge.h"
#include "report.h"
#include "sorder.h"
#include "templates.h"
#include "transact.h"

/* ==================================================================================================================
 * Static global variables
 */

static struct file_block	*main_menu_file = NULL;				/* Point to the file block connected to the main menu.	*/
static int			main_menu_line = -1;				/* Remember the line that a menu applies to.		*/
static int			main_menu_column = -1;				/* Remember the column that a menu applies to.		*/

static wimp_w			account_menu_window = NULL;
static wimp_i			account_menu_name_icon = 0;
static wimp_i			account_menu_ident_icon = 0;
static wimp_i			account_menu_rec_icon = 0;


static int                refdesc_menu_type = 0; /* The type of reference or description menu open. */





static void decode_account_menu(wimp_selection *selection);
static void account_menu_submenu_message(wimp_message_menu_warning *submenu);
static void account_menu_closed_message (void);
static void decode_date_menu(wimp_selection *selection);
static void date_menu_closed_message(void);
static void refdesc_menu_prepare(void);
static void decode_refdesc_menu (wimp_selection *selection);
static void refdesc_menu_closed_message(void);


























/* ==================================================================================================================
 * Account menu -- List of accounts and headings to select from
 */


/* ------------------------------------------------------------------------------------------------------------------ */

void open_account_menu(struct file_block *file, enum account_menu_type type, int line,
		wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer)
{
	wimp_menu		*menu;

	menu = account_complete_menu_build(file, type);

	if (menu == NULL)
		return;

	templates_set_menu_token("AccountMenu");

	amenu_open(menu, pointer, NULL, account_menu_submenu_message, decode_account_menu, account_menu_closed_message);

	main_menu_file = file;
	main_menu_line = line;
	main_menu_column = type;

	account_menu_window = window;
	account_menu_name_icon = icon_n;
	account_menu_ident_icon = icon_i;
	account_menu_rec_icon = icon_r;
}

/* ------------------------------------------------------------------------------------------------------------------ */

static void decode_account_menu(wimp_selection *selection)
{
	int i, column, account;

	if (account_menu_window == NULL) {
		/* If the window is NULL, then the menu was opened over the transaction window. */

		/* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
		 * This really ought to be in edit.c!
		 */

		if (main_menu_line >= main_menu_file->trans_count && selection->items[0] != -1)
			for (i=main_menu_file->trans_count; i<=main_menu_line; i++)
				transact_add_raw_entry(main_menu_file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

		/* Again check that the transaction is in range.  If it isn't, the additions failed.
		 *
		 * Then change the transaction as instructed.
		 */

		if (main_menu_line < main_menu_file->trans_count && selection->items[1] != -1) {
			switch (main_menu_column) {
			case ACCOUNT_MENU_FROM:
				column = EDIT_ICON_FROM;
				break;

			case ACCOUNT_MENU_TO:
				column = EDIT_ICON_TO;
				break;

			default:
				column = -1;
				break;
			}

			edit_change_transaction_account(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line),
					column, account_complete_menu_decode(selection));
		}
	} else {
		/* If the window is not NULL, the menu was opened over a dialogue box. */

		if (selection->items[1] != -1) {
			account = account_complete_menu_decode(selection);

			fill_account_field(main_menu_file, account, !(account_get_type(main_menu_file, account) & ACCOUNT_FULL),
					account_menu_window, account_menu_ident_icon, account_menu_name_icon, account_menu_rec_icon);

			wimp_set_icon_state(account_menu_window, account_menu_ident_icon, 0, 0);
			wimp_set_icon_state(account_menu_window, account_menu_name_icon, 0, 0);
			wimp_set_icon_state(account_menu_window, account_menu_rec_icon, 0, 0);

			icons_replace_caret_in_window(account_menu_window);
		}
	}
}

/* ------------------------------------------------------------------------------------------------------------------ */

static void account_menu_submenu_message(wimp_message_menu_warning *submenu)
{
	wimp_menu *menu_block;

	if (submenu->selection.items[0] == -1 || submenu->selection.items[1] != -1)
		return;

	menu_block = account_complete_submenu_build(submenu);
	wimp_create_sub_menu(menu_block, submenu->pos.x, submenu->pos.y);
}

/* ------------------------------------------------------------------------------------------------------------------ */

static void account_menu_closed_message(void)
{
	analysis_lookup_menu_closed();
	account_complete_menu_destroy();
	templates_set_menu_token("");
}

/* ------------------------------------------------------------------------------------------------------------------ */



/* ==================================================================================================================
 * Date menu -- List of presets to select from
 */


void open_date_menu(struct file_block *file, int line, wimp_pointer *pointer)
{
	wimp_menu	*menu;

	menu = preset_complete_menu_build(file);

	if (menu == NULL)
		return;

	templates_set_menu_token("DateMenu");

	amenu_open(menu, pointer, NULL, NULL, decode_date_menu, date_menu_closed_message);

	main_menu_file = file;
	main_menu_line = line;
}

/* ------------------------------------------------------------------------------------------------------------------ */

static void decode_date_menu(wimp_selection *selection)
{
	int i;

	if (main_menu_file == NULL)
		return;


	if (main_menu_file == NULL)
		return;

	/* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
	 * This really ought to be in edit.c!
	 */

	if (main_menu_line >= main_menu_file->trans_count && selection->items[0] != -1)
		for (i=main_menu_file->trans_count; i<=main_menu_line; i++)
			transact_add_raw_entry(main_menu_file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

	/* Again check that the transaction is in range.  If it isn't, the additions failed.
	 *
	 * Then change the transaction as instructed.
	 */

	if (main_menu_line < main_menu_file->trans_count && selection->items[0] != -1) {
		if (preset_complete_menu_decode(selection) == NULL_PRESET)
			edit_change_transaction_date(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line),
					date_today());
		else
			edit_insert_preset_into_transaction(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line),
					preset_complete_menu_decode(selection));
	}
}

static void date_menu_closed_message(void)
{
	preset_complete_menu_destroy();
	templates_set_menu_token("");
}


/* ==================================================================================================================
 * Ref Desc menu -- List of previous entries to choose from
 */


/* ------------------------------------------------------------------------------------------------------------------ */

void open_refdesc_menu (struct file_block *file, int menu_type, int line, wimp_pointer *pointer)
{
	wimp_menu	*menu;

	menu = transact_complete_menu_build(file, menu_type, line);

	if (menu == NULL)
		return;

	switch (menu_type) {
	case REFDESC_MENU_REFERENCE:
		templates_set_menu_token("RefMenu");
		break;

	case REFDESC_MENU_DESCRIPTION:
		templates_set_menu_token("DescMenu");
		break;
	}

	main_menu_file = file;
	main_menu_line = line;
	refdesc_menu_type = menu_type;

	amenu_open(menu, pointer, refdesc_menu_prepare, NULL, decode_refdesc_menu, refdesc_menu_closed_message);
}



static void refdesc_menu_prepare(void)
{
	transact_complete_menu_prepare(main_menu_line);
}

/* ------------------------------------------------------------------------------------------------------------------ */

static void decode_refdesc_menu (wimp_selection *selection)
{
	int		i;
	char		*field, cheque_buffer[REF_FIELD_LEN];

	if (main_menu_file == NULL)
		return;

	/* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
	 * This really ought to be in edit.c!
	 */

	if (main_menu_line >= main_menu_file->trans_count && selection->items[0] != -1)
		for (i=main_menu_file->trans_count; i<=main_menu_line; i++)
			transact_add_raw_entry(main_menu_file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

	/* Again check that the transaction is in range.  If it isn't, the additions failed.
	 *
	 * Then change the transaction as instructed.
	 */

	if (main_menu_line < main_menu_file->trans_count && selection->items[0] != -1) {
		field = transact_complete_menu_decode(selection);

		if (refdesc_menu_type == REFDESC_MENU_REFERENCE && field == NULL) {
			account_get_next_cheque_number(main_menu_file,
					transact_get_from(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line)),
					transact_get_to(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line)),
					1, cheque_buffer, sizeof(cheque_buffer));
			edit_change_transaction_refdesc(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line),
					EDIT_ICON_REF, cheque_buffer);
		} else if (refdesc_menu_type == REFDESC_MENU_REFERENCE && field != NULL) {
			edit_change_transaction_refdesc(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line),
					EDIT_ICON_REF, field);
		} else if (refdesc_menu_type == REFDESC_MENU_DESCRIPTION && field != NULL) {
			edit_change_transaction_refdesc(main_menu_file, transact_get_transaction_from_line(main_menu_file, main_menu_line),
					EDIT_ICON_DESCRIPT, field);
		}
	}
}

/* ------------------------------------------------------------------------------------------------------------------ */

static void refdesc_menu_closed_message(void)
{
	transact_complete_menu_destroy();
	templates_set_menu_token("");
}

