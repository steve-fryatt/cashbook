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
 * \file: preset_menu.c
 *
 * Preset completion menu implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/config.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "preset_menu.h"

#include "account.h"
#include "amenu.h"
#include "preset.h"
#include "transact.h"

/**
 * Static entries in the menu.
 */

#define PRESET_MENU_DATE 0

/**
 * The length of the menu title buffer.
 */

#define PRESET_MENU_TITLE_LEN 32

/* Data Structures. */

struct preset_menu_link
{
	char			name[PRESET_NAME_LEN];				/**< The name as it appears in the menu.				*/
	preset_t		preset;						/**< Link to the associated preset.					*/
};


/* Global Variables. */

/**
 * Pointer to the file currently owning the menu.
 */

static struct file_block		*preset_menu_file = NULL;

/**
 * The window line to which the menu currently applies.
 */

static int				preset_menu_line = -1;

/**
 * The menu block.
 */

static wimp_menu			*preset_menu = NULL;

/**
 * The associated menu entry data.
 */

static struct preset_menu_link		*preset_menu_entry_link = NULL;

/**
 * Memory to hold the indirected menu title.
 */

static char				preset_menu_title[PRESET_MENU_TITLE_LEN];


/* Static Function Prototypes. */

static void		preset_menu_decode(wimp_selection *selection);
static wimp_menu	*preset_menu_build(struct file_block *file);
static void		preset_menu_destroy(void);


/**
 * Create and open a Preset completion menu over a line in a transaction window.
 * 
 * \param *file			The file to which the menu will belong.
 * \param line			The line of the window over which the menu opened.
 * \param *pointer		Pointer to the Wimp pointer details.
 */

void preset_menu_open(struct file_block *file, int line, wimp_pointer *pointer)
{
	wimp_menu	*menu;

	menu = preset_menu_build(file);

	if (menu == NULL)
		return;

	amenu_open(menu, "DateMenu", pointer, NULL, NULL, preset_menu_decode, preset_menu_destroy);

	preset_menu_file = file;
	preset_menu_line = line;
}


/**
 * Given a menu selection, decode and process the user's choice from a
 * Preset completion menu.
 * 
 * \param *selection		The selection from the menu.
 */

static void preset_menu_decode(wimp_selection *selection)
{
	int i;

	if (preset_menu_file == NULL || preset_menu_entry_link == NULL || preset_menu_line == -1 || selection->items[0] == -1)
		return;

	/* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
	 * This really ought to be in edit.c!
	 */

	if (preset_menu_line >= transact_get_count(preset_menu_file)) {
		for (i = transact_get_count(preset_menu_file); i <= preset_menu_line; i++)
			transact_add_raw_entry(preset_menu_file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");
	}

	/* Again check that the transaction is in range.  If it isn't, the additions failed. */

	if (preset_menu_line >= transact_get_count(preset_menu_file))
		return;

//	if (selection->items[0] == PRESET_MENU_DATE)
//		transact_change_date(preset_menu_file, transact_get_transaction_from_line(preset_menu_file, preset_menu_line), date_today());
//	else
//\TODO		transact_insert_preset_into_line(preset_menu_file, preset_menu_line, preset_menu_entry_link[selection->items[0]].preset);
}


/**
 * Build a Preset Complete menu and return the pointer.
 *
 * \param *file			The file to build the menu for.
 * \return			The created menu, or NULL for an error.
 */

static wimp_menu *preset_menu_build(struct file_block *file)
{
	int		preset_count, line, width, i;
	preset_t	preset;
	char		*name;

	/* Claim enough memory to build the menu in. */

	preset_menu_destroy();

	if (file == NULL)
		return NULL;

	hourglass_on();

	preset_count = preset_get_count(file);

	preset_menu = heap_alloc(28 + 24 * (preset_count + 1));
	preset_menu_entry_link = heap_alloc(sizeof(struct preset_menu_link) * (preset_count + 1));

	if (preset_menu == NULL || preset_menu_entry_link == NULL) {
		preset_menu_destroy();
		hourglass_off();
		return NULL;
	}

	/* Populate the menu. */

	line = 0;

	/* Set up the today's date field. */

	msgs_lookup("DateMenuToday", preset_menu_entry_link[line].name, PRESET_NAME_LEN);
	preset_menu_entry_link[line].preset = NULL_PRESET;

	width = strlen(preset_menu_entry_link[line].name);

	/* Set the menu and icon flags up. */

	preset_menu->entries[line].menu_flags = (preset_count > 0) ? wimp_MENU_SEPARATE : 0;
	preset_menu->entries[line].sub_menu = (wimp_menu *) -1;
	preset_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
			wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
			wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

	/* Set the menu icon contents up. */

	preset_menu->entries[line].data.indirected_text.text = preset_menu_entry_link[line].name;
	preset_menu->entries[line].data.indirected_text.validation = NULL;
	preset_menu->entries[line].data.indirected_text.size = PRESET_NAME_LEN;

	if (preset_count > 0) {
		for (i = 0; i < preset_count; i++) {
			preset = preset_get_preset_from_line(file, i);
			if (preset == NULL_PRESET)
				continue;

			name = preset_get_name(file, preset, NULL, 0);
			if (name == NULL)
				continue;

			line++;

			string_copy(preset_menu_entry_link[line].name, name, PRESET_NAME_LEN);
			preset_menu_entry_link[line].preset = preset;

			if (strlen(preset_menu_entry_link[line].name) > width)
				width = strlen(preset_menu_entry_link[line].name);

			/* Set the menu and icon flags up. */

			preset_menu->entries[line].menu_flags = 0;

			preset_menu->entries[line].sub_menu = (wimp_menu *) -1;
			preset_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			preset_menu->entries[line].data.indirected_text.text = preset_menu_entry_link[line].name;
			preset_menu->entries[line].data.indirected_text.validation = NULL;
			preset_menu->entries[line].data.indirected_text.size = PRESET_NAME_LEN;
		}
	}

	/* Finish off the menu, marking the last entry and filling in the header. */

	preset_menu->entries[line].menu_flags |= wimp_MENU_LAST;

	msgs_lookup ("DateMenuTitle", preset_menu_title, PRESET_MENU_TITLE_LEN);
	preset_menu->title_data.indirected_text.text = preset_menu_title;
	preset_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	preset_menu->title_fg = wimp_COLOUR_BLACK;
	preset_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	preset_menu->work_fg = wimp_COLOUR_BLACK;
	preset_menu->work_bg = wimp_COLOUR_WHITE;

	preset_menu->width = (width + 1) * 16;
	preset_menu->height = 44;
	preset_menu->gap = 0;

	hourglass_off();

	return preset_menu;
}


/**
 * Destroy any Preset Complete menu which is currently open.
 */

static void preset_menu_destroy(void)
{
	if (preset_menu != NULL)
		heap_free(preset_menu);

	if (preset_menu_entry_link != NULL)
		heap_free(preset_menu_entry_link);

	preset_menu = NULL;
	preset_menu_entry_link = NULL;
	preset_menu_line = -1;
	*preset_menu_title = '\0';
}
