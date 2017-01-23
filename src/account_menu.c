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

#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/config.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "account_menu.h"

#include "account.h"
#include "amenu.h"
#include "transact.h"


/**
 * The length of the menu title buffer.
 */

#define ACCOUNT_MENU_TITLE_LEN 32

/* Data Structures. */

struct account_menu_link {
	char		name[ACCOUNT_NAME_LEN];
	acct_t		account;
};

struct account_menu_group {
	char		name[ACCOUNT_SECTION_LEN];
	int		entry;
	int		start_line;
};

/* Global Variables. */

/**
 * The type of menu currently open.
 */

static enum account_menu_type			account_menu_active_type = ACCOUNT_MENU_NONE;

/**
 * Pointer to the file currently owning the menu.
 */

static struct file_block	*account_menu_file = NULL;

/**
 * The window line to which the menu currently applies, or -1 if attached to an icon.
 */

static int			account_menu_line = -1;

/**
 * The menu block.
 */

static wimp_menu			*refdesc_menu = NULL;

/**
 * The associated menu entry data.
 */

static struct refdesc_menu_link		*refdesc_menu_entry_link = NULL;

/**
 * Memory to hold the indirected menu title.
 */

static char				refdesc_menu_title[REFDESC_MENU_TITLE_LEN];






static wimp_menu			*account_complete_menu = NULL;		/**< The Account Complete menu block.					*/
static struct account_menu_group	*account_complete_menu_group = NULL;	/**< Groups for the Account Complete menu.				*/
static wimp_menu			*account_complete_submenu = NULL;	/**< The Account Complete menu block.					*/
static struct account_menu_link		*account_complete_submenu_link = NULL;	/**< Links for the Account Complete menu.				*/
static char				*account_complete_menu_title = NULL;	/**< Account Complete menu title buffer.				*/
static struct file_block		*account_complete_menu_file = NULL;	/**< The file to which the Account Complete menu is currently attached.	*/



static wimp_w			account_menu_window = NULL;
static wimp_i			account_menu_name_icon = 0;
static wimp_i			account_menu_ident_icon = 0;
static wimp_i			account_menu_rec_icon = 0;





static void decode_account_menu(wimp_selection *selection);
static void account_menu_submenu_message(wimp_message_menu_warning *submenu);
static void account_menu_closed_message (void);








void account_menu_open(struct file_block *file, enum account_menu_type type, int line, wimp_pointer *pointer)
{
	wimp_menu		*menu;

	menu = account_complete_menu_build(file, type);

	if (menu == NULL)
		return;

	amenu_open(menu, "AccountMenu", pointer, NULL, account_menu_submenu_message, decode_account_menu, account_menu_closed_message);

	account_menu_file = file;
	account_menu_active_type = type;

	account_menu_line = line;

	account_menu_window = NULL;
	account_menu_name_icon = wimp_ICON_WINDOW;
	account_menu_ident_icon = wimp_ICON_WINDOW;
	account_menu_rec_icon = wimp_ICON_WINDOW;
}


void account_menu_open_icon(struct file_block *file, enum account_menu_type type,
		wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer)
{
	wimp_menu		*menu;

	menu = account_complete_menu_build(file, type);

	if (menu == NULL)
		return;

	amenu_open(menu, "AccountMenu", pointer, NULL, account_menu_submenu_message, decode_account_menu, account_menu_closed_message);

	account_menu_file = file;
	account_menu_active_type = type;

	account_menu_line = -1;

	account_menu_window = window;
	account_menu_name_icon = icon_n;
	account_menu_ident_icon = icon_i;
	account_menu_rec_icon = icon_r;
}















/* ==================================================================================================================
 * Account menu -- List of accounts and headings to select from
 */


/* ------------------------------------------------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------------------------------------------------ */

static void decode_account_menu(wimp_selection *selection)
{
	int			i;
	acct_t			account;
	enum transact_field	target;

	if (account_menu_window == NULL) {
		/* If the window is NULL, then the menu was opened over the transaction window. */

		/* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
		 * This really ought to be in edit.c!
		 */

		if (account_menu_line >= transact_get_count(account_menu_file) && selection->items[0] != -1)
			for (i = transact_get_count(account_menu_file); i <= account_menu_line; i++)
				transact_add_raw_entry(account_menu_file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");

		/* Again check that the transaction is in range.  If it isn't, the additions failed.
		 *
		 * Then change the transaction as instructed.
		 */

		if (account_menu_line < transact_get_count(account_menu_file) && selection->items[1] != -1) {
			switch (account_menu_active_type) {
			case ACCOUNT_MENU_FROM:
				target = TRANSACT_FIELD_FROM;
				break;

			case ACCOUNT_MENU_TO:
				target = TRANSACT_FIELD_TO;
				break;

			default:
				target = TRANSACT_FIELD_NONE;
				break;
			}

			transact_change_account(account_menu_file, transact_get_transaction_from_line(account_menu_file, account_menu_line),
					target, account_complete_menu_decode(selection));
		}
	} else {
		/* If the window is not NULL, the menu was opened over a dialogue box. */

		if (selection->items[1] != -1) {
			account = account_complete_menu_decode(selection);

			account_fill_field(account_menu_file, account, !(account_get_type(account_menu_file, account) & ACCOUNT_FULL),
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
}

/* ------------------------------------------------------------------------------------------------------------------ */












/**
 * Build an Account Complete menu for a given file and account type.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \return			The menu block, or NULL.
 */

wimp_menu *account_complete_menu_build(struct file_block *file, enum account_menu_type type)
{
	int			i, group, line, entry, width, sublen;
	int			groups = 3, maxsublen = 0, headers = 0;
	enum account_type	include, sequence[] = {ACCOUNT_FULL, ACCOUNT_IN, ACCOUNT_OUT};
	osbool			shade;
	char			*title;

	if (file == NULL || file->accounts == NULL)
		return NULL;

	account_complete_menu_destroy();

	switch (type) {
	case ACCOUNT_MENU_FROM:
		include = ACCOUNT_FULL | ACCOUNT_IN;
		title = "ViewAccMenuTitleFrom";
		break;

	case ACCOUNT_MENU_TO:
		include = ACCOUNT_FULL | ACCOUNT_OUT;
		title = "ViewAccMenuTitleTo";
		break;

	case ACCOUNT_MENU_ACCOUNTS:
		include = ACCOUNT_FULL;
		title = "ViewAccMenuTitleAcc";
		break;

	case ACCOUNT_MENU_INCOMING:
		include = ACCOUNT_IN;
		title = "ViewAccMenuTitleIn";
		break;

	case ACCOUNT_MENU_OUTGOING:
		include = ACCOUNT_OUT;
		title = "ViewAccMenuTitleOut";
		break;

	default:
		include = ACCOUNT_NULL;
		title = "";
		break;
	}

	account_complete_menu_file = file;

	/* Find out how many accounts there are, by counting entries in the groups. */

	/* for each group that will be included in the menu, count through the window definition. */

	for (group = 0; group < groups; group++) {
		if (include & sequence[group]) {
			i = 0;
			sublen = 0;
			entry = account_find_window_entry_from_type(file, sequence[group]);

			while (i < file->accounts->account_windows[entry].display_lines) {
				if (file->accounts->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER) {
					/* If the line is a header, increment the header count, and start a new sub-menu. */

					if (sublen > maxsublen)
						maxsublen = sublen;

					sublen = 0;
					headers++;
				} else if (file->accounts->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
					/* Else if the line is an account entry, increment the submenu length count.  If the line is the first in the
					 * group, it must fall outwith any headers and so will require its own submenu.
					 */

					sublen++;

					if (i == 0)
						headers++;
				}

				i++;
			}

			if (sublen > maxsublen)
				maxsublen = sublen;
		}
	}

	#ifdef DEBUG
	debug_printf("\\GBuilding accounts menu for %d headers, maximum submenu of %d", headers, maxsublen);
	#endif

	if (headers == 0 || maxsublen == 0)
		return NULL;

	/* Claim enough memory to build the menu in. */

	account_complete_menu = heap_alloc(28 + (24 * headers));
	account_complete_menu_group = heap_alloc(headers * sizeof(struct account_menu_group));
	account_complete_submenu = heap_alloc(28 + (24 * maxsublen));
	account_complete_submenu_link = heap_alloc(maxsublen * sizeof(struct account_menu_link));
	account_complete_menu_title = heap_alloc(ACCOUNT_MENU_TITLE_LEN);

	if (account_complete_menu == NULL || account_complete_menu_group == NULL || account_complete_menu_title == NULL ||
			account_complete_submenu == NULL || account_complete_submenu_link == NULL) {
		account_complete_menu_destroy();
		return NULL;
	}

	/* Populate the menu. */

	line = 0;
	width = 0;
	shade = TRUE;

	for (group = 0; group < groups; group++) {
		if (include & sequence[group]) {
			i = 0;
			entry = account_find_window_entry_from_type(file, sequence[group]);

			/* Start the group with a separator if there are lines in the menu already. */

			if (line > 0)
				account_complete_menu->entries[line-1].menu_flags |= wimp_MENU_SEPARATE;

			while (i < file->accounts->account_windows[entry].display_lines) {
				/* If the line is a section header, add it to the menu... */

				if (line < headers && file->accounts->account_windows[entry].line_data[i].type == ACCOUNT_LINE_HEADER) {
					/* Test for i>0 because if this is the first line of a new entry, the last group of the last entry will
					 * already have been dealt with at the end of the main loop.  shade will be FALSE if there have been any
					 * ACCOUNT_LINE_DATA since the last ACCOUNT_LINE_HEADER.
					 */
					if (shade && line > 0 && i > 0)
						account_complete_menu->entries[line - 1].icon_flags |= wimp_ICON_SHADED;

					shade = TRUE;

					/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
					 * well move while the menu is open.
					 */

					strcpy(account_complete_menu_group[line].name, file->accounts->account_windows[entry].line_data[i].heading);
					if (strlen(account_complete_menu_group[line].name) > width)
						width = strlen(account_complete_menu_group[line].name);
					account_complete_menu_group[line].entry = entry;
					account_complete_menu_group[line].start_line = i+1;

					/* Set the menu and icon flags up. */

					account_complete_menu->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

					account_complete_menu->entries[line].sub_menu = account_complete_submenu;
					account_complete_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
							wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
							wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

					/* Set the menu icon contents up. */

					account_complete_menu->entries[line].data.indirected_text.text = account_complete_menu_group[line].name;
					account_complete_menu->entries[line].data.indirected_text.validation = NULL;
					account_complete_menu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

					line++;
				} else if (file->accounts->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
					shade = FALSE;

					/* If this is the first line of the list, and it's a data line, there is no group header and a default
					 * group will be required.
					 */

					if (i == 0 && line < headers) {
						switch (sequence[group]) {
						case ACCOUNT_FULL:
							msgs_lookup("ViewaccMenuAccs", account_complete_menu_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						case ACCOUNT_IN:
							msgs_lookup("ViewaccMenuHIn", account_complete_menu_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						case ACCOUNT_OUT:
							msgs_lookup("ViewaccMenuHOut", account_complete_menu_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						default:
							break;
						}
						if (strlen(account_complete_menu_group[line].name) > width)
							width = strlen(account_complete_menu_group[line].name);
						account_complete_menu_group[line].entry = entry;
						account_complete_menu_group[line].start_line = i;

						/* Set the menu and icon flags up. */

						account_complete_menu->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

						account_complete_menu->entries[line].sub_menu = account_complete_submenu;
						account_complete_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
								wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
								wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

						/* Set the menu icon contents up. */

						account_complete_menu->entries[line].data.indirected_text.text = account_complete_menu_group[line].name;
						account_complete_menu->entries[line].data.indirected_text.validation = NULL;
						account_complete_menu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

						line++;
					}
				}

				i++;
			}

			/* Update the maximum submenu length count again. */

			if (shade && line > 0)
				account_complete_menu->entries[line - 1].icon_flags |= wimp_ICON_SHADED;
		}
	}

	/* Finish off the menu, marking the last entry and filling in the header. */

	account_complete_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;
	account_complete_menu->entries[line - 1].menu_flags &= ~wimp_MENU_SEPARATE;

	msgs_lookup(title, account_complete_menu_title, ACCOUNT_MENU_TITLE_LEN);
	account_complete_menu->title_data.indirected_text.text = account_complete_menu_title;
	account_complete_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_complete_menu->title_fg = wimp_COLOUR_BLACK;
	account_complete_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_complete_menu->work_fg = wimp_COLOUR_BLACK;
	account_complete_menu->work_bg = wimp_COLOUR_WHITE;

	account_complete_menu->width = (width + 1) * 16;
	account_complete_menu->height = 44;
	account_complete_menu->gap = 0;

	return account_complete_menu;
}


/**
 * Build a submenu for the Account Complete menu on the fly, using information
 * and memory allocated and assembled in account_complete_menu_build().
 *
 * The memory to hold the menu has been allocated and is pointed to by
 * account_complete_submenu and account_complete_submenu_link; if either of these
 *  are NULL, the fucntion must refuse to run.
 *
 * \param *submenu		The submenu warning message block to use.
 * \return			Pointer to the submenu block, or NULL on failure.
 */

wimp_menu *account_complete_submenu_build(wimp_message_menu_warning *submenu)
{
	int		i, line = 0, entry, width = 0;

	if (account_complete_submenu == NULL || account_complete_submenu_link == NULL ||
			account_complete_menu_file == NULL || account_complete_menu_file->accounts == NULL)
		return NULL;

	entry = account_complete_menu_group[submenu->selection.items[0]].entry;
	i = account_complete_menu_group[submenu->selection.items[0]].start_line;

	while (i < account_complete_menu_file->accounts->account_windows[entry].display_lines &&
			account_complete_menu_file->accounts->account_windows[entry].line_data[i].type != ACCOUNT_LINE_HEADER) {
		/* If the line is an account entry, add it to the manu... */

		if (account_complete_menu_file->accounts->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA) {
			/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
			 * well move while the menu is open.
			 */

			strcpy(account_complete_submenu_link[line].name,
					account_complete_menu_file->accounts->accounts[account_complete_menu_file->accounts->account_windows[entry].line_data[i].account].name);
			if (strlen(account_complete_submenu_link[line].name) > width)
				width = strlen(account_complete_submenu_link[line].name);
			account_complete_submenu_link[line].account = account_complete_menu_file->accounts->account_windows[entry].line_data[i].account;

			/* Set the menu and icon flags up. */

			account_complete_submenu->entries[line].menu_flags = 0;

			account_complete_submenu->entries[line].sub_menu = (wimp_menu *) -1;
			account_complete_submenu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			account_complete_submenu->entries[line].data.indirected_text.text = account_complete_submenu_link[line].name;
			account_complete_submenu->entries[line].data.indirected_text.validation = NULL;
			account_complete_submenu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

			line++;
		}

		i++;
	}

	account_complete_submenu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	account_complete_submenu->title_data.indirected_text.text = account_complete_menu_group[submenu->selection.items[0]].name;
	account_complete_submenu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_complete_submenu->title_fg = wimp_COLOUR_BLACK;
	account_complete_submenu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_complete_submenu->work_fg = wimp_COLOUR_BLACK;
	account_complete_submenu->work_bg = wimp_COLOUR_WHITE;

	account_complete_submenu->width = (width + 1) * 16;
	account_complete_submenu->height = 44;
	account_complete_submenu->gap = 0;

	return account_complete_submenu;
}


/**
 * Destroy any Account Complete menu which is currently open.
 */

void account_complete_menu_destroy(void)
{
	if (account_complete_menu != NULL)
		heap_free(account_complete_menu);

	if (account_complete_menu_group != NULL)
		heap_free(account_complete_menu_group);

	if (account_complete_submenu != NULL)
		heap_free(account_complete_submenu);

	if (account_complete_submenu_link != NULL)
		heap_free(account_complete_submenu_link);

	if (account_complete_menu_title != NULL)
		heap_free(account_complete_menu_title);

	account_complete_menu = NULL;
	account_complete_menu_group = NULL;
	account_complete_submenu = NULL;
	account_complete_submenu_link = NULL;
	account_complete_menu_title = NULL;
	account_complete_menu_file = NULL;
}


/**
 * Decode a selection from the Account Complete menu, converting to an account
 * number.
 *
 * \param *selection		The menu selection to decode.
 * \return			The account numer, or NULL_ACCOUNT.
 */

acct_t account_complete_menu_decode(wimp_selection *selection)
{
	if (account_complete_submenu_link == NULL || selection == NULL ||
			selection->items[0] == -1 || selection->items[1] == -1)
		return NULL_ACCOUNT;

	return account_complete_submenu_link[selection->items[1]].account;
}
