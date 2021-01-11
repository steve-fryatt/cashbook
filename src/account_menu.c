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
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
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

/**
 * The number of groups which will be included in the menu.
 */

#define ACCOUNT_MENU_GROUPS 3

/**
 * The groups to be included in the menu.
 */

enum account_type account_menu_sequence[] = {ACCOUNT_FULL, ACCOUNT_IN, ACCOUNT_OUT};


/* Data Structures. */

struct account_menu_link {
	char			name[ACCOUNT_NAME_LEN];
	acct_t			account;
};

struct account_menu_group {
	char			name[ACCOUNT_SECTION_LEN];
	enum account_type	type;
	int			start_line;
};

/* Global Variables. */

/**
 * The type of menu currently open.
 */

static enum account_menu_type		account_menu_active_type = ACCOUNT_MENU_NONE;

/**
 * Pointer to the file currently owning the menu.
 */

static struct file_block		*account_menu_file = NULL;

/**
 * The window line to which the menu currently applies, or -1 if attached to an icon.
 */

static int				account_menu_line = -1;

/**
 * The window to which the menu currently applies, or NULL if attached to a transaction window.
 */

static wimp_w			account_menu_window = NULL;

/**
 * The name icon to which the menu currently applies, or wimp_ICON_WINDOW if attached to a transaction window.
 */

static wimp_i			account_menu_name_icon = 0;

/**
 * The ident icon to which the menu currently applies, or wimp_ICON_WINDOW if attached to a transaction window.
 */
static wimp_i			account_menu_ident_icon = 0;

/**
 * The reconcile icon to which the menu currently applies, or wimp_ICON_WINDOW if attached to a transaction window.
 */
 
static wimp_i			account_menu_rec_icon = 0;

/**
 * The menu block.
 */

static wimp_menu			*account_menu = NULL;

/**
 * The sub-menu block.
 */

static wimp_menu			*account_menu_submenu = NULL;

/**
 * The associated group menu entry data.
 */

static struct account_menu_group	*account_menu_entry_group = NULL;

/**
 * The associated account submenu entry data.
 */

static struct account_menu_link		*account_menu_entry_link = NULL;

/**
 * Memory to hold the indirected menu title.
 */

static char				account_menu_title[ACCOUNT_MENU_TITLE_LEN];

/**
 * A callback function to report the closure of the menu to the client.
 */

static void				(*account_menu_close_callback)(void) = NULL;

/* Static Function Prototypes. */

static void		account_menu_submenu_message(wimp_message_menu_warning *submenu);
static void		account_menu_decode(wimp_selection *selection);
static wimp_menu	*account_menu_build(struct file_block *file, enum account_menu_type type);
static wimp_menu	*account_menu_build_submenu(wimp_message_menu_warning *submenu);
static void		account_menu_destroy(void);


/**
 * Create and open an Account completion menu over a line in a transaction window.
 * 
 * \param *file			The file to which the menu will belong.
 * \param menu_type		The type of menu to be opened.
 * \param line			The line of the window over which the menu opened.
 * \param *pointer		Pointer to the Wimp pointer details.
 */

void account_menu_open(struct file_block *file, enum account_menu_type menu_type, int line, wimp_pointer *pointer)
{
	wimp_menu		*menu;

	menu = account_menu_build(file, menu_type);

	if (menu == NULL)
		return;

	amenu_open(menu, "AccountMenu", pointer, NULL, account_menu_submenu_message, account_menu_decode, account_menu_destroy);

	account_menu_file = file;
	account_menu_active_type = menu_type;

	account_menu_line = line;

	account_menu_window = NULL;
	account_menu_name_icon = wimp_ICON_WINDOW;
	account_menu_ident_icon = wimp_ICON_WINDOW;
	account_menu_rec_icon = wimp_ICON_WINDOW;

	account_menu_close_callback = NULL;
}


/**
 * Create and open an Account completion menu over a set of account icons in
 * a dialogue box.
 * 
 * \param *file			The file to which the menu will belong.
 * \param menu_type		The type of menu to be opened.
 * \param *close_callback	Callback pointer to report closure of the menu, or NULL.
 * \param window		The window in which the target icons exist.
 * \param icon_i		The target ident field icon.
 * \param icon_n		The target name field icon.
 * \param icon_r		The target reconcile field icon.
 * \param *pointer		Pointer to the Wimp pointer details.
 */

void account_menu_open_icon(struct file_block *file, enum account_menu_type menu_type, void (*close_callback)(void),
		wimp_w window, wimp_i icon_i, wimp_i icon_n, wimp_i icon_r, wimp_pointer *pointer)
{
	wimp_menu		*menu;

	menu = account_menu_build(file, menu_type);

	if (menu == NULL)
		return;

	amenu_open(menu, "AccountMenu", pointer, NULL, account_menu_submenu_message, account_menu_decode, account_menu_destroy);

	account_menu_file = file;
	account_menu_active_type = menu_type;

	account_menu_line = -1;

	account_menu_window = window;
	account_menu_name_icon = icon_n;
	account_menu_ident_icon = icon_i;
	account_menu_rec_icon = icon_r;

	account_menu_close_callback = close_callback;
}


/**
 * Process Submenu Warning messages for an Account completion menu.
 *
 * \param *submenu		The Submenu Warning message data to process.
 */

static void account_menu_submenu_message(wimp_message_menu_warning *submenu)
{
	wimp_menu *menu_block;

	if (submenu->selection.items[0] == -1 || submenu->selection.items[1] != -1)
		return;

	menu_block = account_menu_build_submenu(submenu);
	wimp_create_sub_menu(menu_block, submenu->pos.x, submenu->pos.y);
}


/**
 * Given a menu selection, decode and process the user's choice from an
 * Account completion menu.
 * 
 * \param *selection		The selection from the menu.
 */

static void account_menu_decode(wimp_selection *selection)
{
	int			i;
	acct_t			account;
	enum transact_field	target;


	if (account_menu_file == NULL || account_menu_entry_link == NULL || selection->items[0] == -1 || selection->items[1] == -1)
		return;

	if (account_menu_window == NULL && account_menu_line != -1) {
		/* This is over a line in a transaction window. */

		/* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
		 * This really ought to be in edit.c!
		 */

		if (account_menu_line >= transact_get_count(account_menu_file)) {
			for (i = transact_get_count(account_menu_file); i <= account_menu_line; i++)
				transact_add_raw_entry(account_menu_file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");
		}

		/* Again check that the transaction is in range.  If it isn't, the additions failed. */

		if (account_menu_line >= transact_get_count(account_menu_file))
			return;

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
				target, account_menu_entry_link[selection->items[1]].account);
	} else if (account_menu_window != NULL && account_menu_ident_icon != wimp_ICON_WINDOW && account_menu_name_icon != wimp_ICON_WINDOW && account_menu_rec_icon != wimp_ICON_WINDOW) {
		/* If the window is not NULL, the menu was opened over a dialogue box. */

		account = account_menu_entry_link[selection->items[1]].account;

		account_fill_field(account_menu_file, account, !(account_get_type(account_menu_file, account) & ACCOUNT_FULL),
				account_menu_window, account_menu_ident_icon, account_menu_name_icon, account_menu_rec_icon);

		wimp_set_icon_state(account_menu_window, account_menu_ident_icon, 0, 0);
		wimp_set_icon_state(account_menu_window, account_menu_name_icon, 0, 0);
		wimp_set_icon_state(account_menu_window, account_menu_rec_icon, 0, 0);

		icons_replace_caret_in_window(account_menu_window);
	}
}


/**
 * Build an Account Complete menu for a given file and account type.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \return			The menu block, or NULL.
 */

static wimp_menu *account_menu_build(struct file_block *file, enum account_menu_type type)
{
	int			i, group, display_lines, line, width, sublen;
	int			maxsublen = 0, headers = 0;
	enum account_type	include;
	osbool			shade;
	char			*title, *name;

	if (file == NULL || file->accounts == NULL)
		return NULL;

	account_menu_destroy();

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

	account_menu_file = file;

	/* Find out how many accounts there are, by counting entries in the groups. */

	/* for each group that will be included in the menu, count through the window definition. */

	for (group = 0; group < ACCOUNT_MENU_GROUPS; group++) {
		if (include & account_menu_sequence[group]) {
			i = 0;
			sublen = 0;

			display_lines = account_get_list_length(file, account_menu_sequence[group]);

			while (i < display_lines) {
				switch (account_get_list_entry_type(file, account_menu_sequence[group], i)) {
				case ACCOUNT_LINE_HEADER:
					/* If the line is a header, increment the header count, and start a new sub-menu. */

					if (sublen > maxsublen)
						maxsublen = sublen;

					sublen = 0;
					headers++;
					break;
				case ACCOUNT_LINE_DATA:
					/* If the line is an account entry, increment the submenu length count. */

					sublen++;

					/* If the line is the first in the group, it must fall outwith any headers
					 * and so will require its own submenu.
					 */

					if (i == 0)
						headers++;
					break;

				case ACCOUNT_LINE_BLANK:
				case ACCOUNT_LINE_FOOTER:
					break;
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

	account_menu = heap_alloc(28 + (24 * headers));
	account_menu_entry_group = heap_alloc(headers * sizeof(struct account_menu_group));
	account_menu_submenu = heap_alloc(28 + (24 * maxsublen));
	account_menu_entry_link = heap_alloc(maxsublen * sizeof(struct account_menu_link));

	if (account_menu == NULL || account_menu_entry_group == NULL || account_menu_submenu == NULL || account_menu_entry_link == NULL) {
		account_menu_destroy();
		return NULL;
	}

	/* Populate the menu. */

	line = 0;
	width = 0;
	shade = TRUE;

	for (group = 0; group < ACCOUNT_MENU_GROUPS; group++) {
		if (include & account_menu_sequence[group]) {
			i = 0;

			display_lines = account_get_list_length(file, account_menu_sequence[group]);

			/* Start the group with a separator if there are lines in the menu already. */

			if (line > 0)
				account_menu->entries[line-1].menu_flags |= wimp_MENU_SEPARATE;

			while (i < display_lines) {
				switch (account_get_list_entry_type(file, account_menu_sequence[group], i)) {
				case ACCOUNT_LINE_HEADER:
					/* If the line is a section header, add it to the menu... */

					name = account_get_list_entry_text(file, account_menu_sequence[group], i);
					if (name == NULL) {
						i++;
						continue;
					}

					/* Test for i>0 because if this is the first line of a new entry, the last group of the last entry will
					 * already have been dealt with at the end of the main loop.  shade will be FALSE if there have been any
					 * ACCOUNT_LINE_DATA since the last ACCOUNT_LINE_HEADER.
					 */
					if (shade && line > 0 && i > 0)
						account_menu->entries[line - 1].icon_flags |= wimp_ICON_SHADED;

					shade = TRUE;

					/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
					 * well move while the menu is open.
					 */


					string_copy(account_menu_entry_group[line].name, name, ACCOUNT_SECTION_LEN);

					if (strlen(account_menu_entry_group[line].name) > width)
						width = strlen(account_menu_entry_group[line].name);

					account_menu_entry_group[line].type = account_menu_sequence[group];
					account_menu_entry_group[line].start_line = i + 1;

					/* Set the menu and icon flags up. */

					account_menu->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

					account_menu->entries[line].sub_menu = account_menu_submenu;
					account_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
							wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
							wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

					/* Set the menu icon contents up. */

					account_menu->entries[line].data.indirected_text.text = account_menu_entry_group[line].name;
					account_menu->entries[line].data.indirected_text.validation = NULL;
					account_menu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

					line++;
					break;

				case ACCOUNT_LINE_DATA:
					shade = FALSE;

					/* If this is the first line of the list, and it's a data line, there is no group header and a default
					 * group will be required.
					 */

					if (i == 0 && line < headers) {
						switch (account_menu_sequence[group]) {
						case ACCOUNT_FULL:
							msgs_lookup("ViewaccMenuAccs", account_menu_entry_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						case ACCOUNT_IN:
							msgs_lookup("ViewaccMenuHIn", account_menu_entry_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						case ACCOUNT_OUT:
							msgs_lookup("ViewaccMenuHOut", account_menu_entry_group[line].name, ACCOUNT_SECTION_LEN);
							break;

						default:
							break;
						}
						
						if (strlen(account_menu_entry_group[line].name) > width)
							width = strlen(account_menu_entry_group[line].name);
							
						account_menu_entry_group[line].type = account_menu_sequence[group];
						account_menu_entry_group[line].start_line = i;

						/* Set the menu and icon flags up. */

						account_menu->entries[line].menu_flags = wimp_MENU_GIVE_WARNING;

						account_menu->entries[line].sub_menu = account_menu_submenu;
						account_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
								wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
								wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

						/* Set the menu icon contents up. */

						account_menu->entries[line].data.indirected_text.text = account_menu_entry_group[line].name;
						account_menu->entries[line].data.indirected_text.validation = NULL;
						account_menu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

						line++;
					}
					break;

				case ACCOUNT_LINE_BLANK:
				case ACCOUNT_LINE_FOOTER:
					break;
				}

				i++;
			}

			/* Update the maximum submenu length count again. */

			if (shade && line > 0)
				account_menu->entries[line - 1].icon_flags |= wimp_ICON_SHADED;
		}
	}

	/* Finish off the menu, marking the last entry and filling in the header. */

	account_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;
	account_menu->entries[line - 1].menu_flags &= ~wimp_MENU_SEPARATE;

	msgs_lookup(title, account_menu_title, ACCOUNT_MENU_TITLE_LEN);
	account_menu->title_data.indirected_text.text = account_menu_title;
	account_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_menu->title_fg = wimp_COLOUR_BLACK;
	account_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_menu->work_fg = wimp_COLOUR_BLACK;
	account_menu->work_bg = wimp_COLOUR_WHITE;

	account_menu->width = (width + 1) * 16;
	account_menu->height = 44;
	account_menu->gap = 0;

	return account_menu;
}


/**
 * Build a submenu for the Account Complete menu on the fly, using information
 * and memory allocated and assembled in account_menu_build().
 *
 * The memory to hold the menu has been allocated and is pointed to by
 * account_complete_submenu and account_menu_entry_link; if either of these
 *  are NULL, the fucntion must refuse to run.
 *
 * \param *submenu		The submenu warning message block to use.
 * \return			Pointer to the submenu block, or NULL on failure.
 */

static wimp_menu *account_menu_build_submenu(wimp_message_menu_warning *submenu)
{
	int			i, display_lines, line = 0, width = 0;
	acct_t			account;
	char			*name;
	enum account_type	list_type;
	enum account_line_type	line_type;

	if (account_menu_submenu == NULL || account_menu_entry_link == NULL ||
			account_menu_file == NULL || account_menu_file->accounts == NULL)
		return NULL;

	list_type = account_menu_entry_group[submenu->selection.items[0]].type;
	i = account_menu_entry_group[submenu->selection.items[0]].start_line;

	display_lines = account_get_list_length(account_menu_file, list_type);

	while (i < display_lines && (line_type = account_get_list_entry_type(account_menu_file, list_type, i)) != ACCOUNT_LINE_HEADER) {
		/* If the line is an account entry, add it to the manu... */

		if (line_type == ACCOUNT_LINE_DATA) {
			/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
			 * well move while the menu is open.
			 */

			account = account_get_list_entry_account(account_menu_file, list_type, i);

			/* If the line is an account, add it to the manu... */

			if (account == NULL_ACCOUNT) {
				i++;
				continue;
			}

			name = account_get_name(account_menu_file, account);
			if (name == NULL) {
				i++;
				continue;
			}

			string_copy(account_menu_entry_link[line].name, name, ACCOUNT_NAME_LEN);

			if (strlen(account_menu_entry_link[line].name) > width)
				width = strlen(account_menu_entry_link[line].name);
			account_menu_entry_link[line].account = account;

			/* Set the menu and icon flags up. */

			account_menu_submenu->entries[line].menu_flags = 0;

			account_menu_submenu->entries[line].sub_menu = (wimp_menu *) -1;
			account_menu_submenu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			account_menu_submenu->entries[line].data.indirected_text.text = account_menu_entry_link[line].name;
			account_menu_submenu->entries[line].data.indirected_text.validation = NULL;
			account_menu_submenu->entries[line].data.indirected_text.size = ACCOUNT_SECTION_LEN;

			line++;
		}

		i++;
	}

	account_menu_submenu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	account_menu_submenu->title_data.indirected_text.text = account_menu_entry_group[submenu->selection.items[0]].name;
	account_menu_submenu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	account_menu_submenu->title_fg = wimp_COLOUR_BLACK;
	account_menu_submenu->title_bg = wimp_COLOUR_LIGHT_GREY;
	account_menu_submenu->work_fg = wimp_COLOUR_BLACK;
	account_menu_submenu->work_bg = wimp_COLOUR_WHITE;

	account_menu_submenu->width = (width + 1) * 16;
	account_menu_submenu->height = 44;
	account_menu_submenu->gap = 0;

	return account_menu_submenu;
}


/**
 * Destroy any Account Complete menu which is currently open.
 */

static void account_menu_destroy(void)
{
	if (account_menu_close_callback != NULL) {
		account_menu_close_callback();
		account_menu_close_callback = NULL;
	}

	if (account_menu != NULL)
		heap_free(account_menu);

	if (account_menu_entry_group != NULL)
		heap_free(account_menu_entry_group);

	if (account_menu_submenu != NULL)
		heap_free(account_menu_submenu);

	if (account_menu_entry_link != NULL)
		heap_free(account_menu_entry_link);

	account_menu = NULL;
	account_menu_entry_group = NULL;
	account_menu_submenu = NULL;
	account_menu_entry_link = NULL;
	*account_menu_title = '\0';
	account_menu_file = NULL;
}
