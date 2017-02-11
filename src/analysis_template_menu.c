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
 * \file: analysis_template_menu.c
 *
 * Analysis Template menu implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/wimp.h"
//#include "oslib/os.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "analysis_template_menu.h"

#include "analysis.h"


/**
 * The length of the menu title buffer.
 */

#define ANALYSIS_TEMPLATE_MENU_TITLE_LEN 32

/**
 * The length of the ellipsis for the end of template names.
 */

#define ANALYSIS_TEMPLATE_MENU_ELLIPSIS_LEN 3

/* Data Structures. */

struct analysis_template_menu_link
{
	char					name[ANALYSIS_SAVED_NAME_LEN + ANALYSIS_TEMPLATE_MENU_ELLIPSIS_LEN];	/**< The name as it appears in the menu (+3 for ellipsis...)			*/
	int					template;				/**< Index link to the associated report template in the saved report array.	*/
};


/**
 * Pointer to the file currently owning the menu.
 */

static struct file_block			*analysis_template_menu_file = NULL;

/**
 * The menu block.
 */

static wimp_menu				*analysis_template_menu = NULL;

/**
 * The associated menu entry data.
 */

static struct analysis_template_menu_link	*analysis_template_menu_entry_link = NULL;

/**
 * Memory to hold the indirected menu title.
 */

static char					analysis_template_menu_title[ANALYSIS_TEMPLATE_MENU_TITLE_LEN];

/* Static Function Prototypes*/

static int		analysis_template_menu_compare_entries(const void *va, const void *vb);





/**
 * Build a Template List menu and return the pointer.
 *
 * \param *file			The file to build the menu for.
 * \param standalone		TRUE if the menu is standalone; FALSE for part of
 *				the main menu.
 * \return			The created menu, or NULL for an error.
 */

wimp_menu *analysis_template_menu_build(struct file_block *file, osbool standalone)
{
	int			line, width, template_count;
	struct analysis_report	*template;
	char			*name;

	/* Claim enough memory to build the menu in. */

	analysis_template_menu_destroy();

	template_count = analysis_get_template_count(file);

	if (template_count > 0) {
		analysis_template_menu = heap_alloc(28 + 24 * template_count);
		analysis_template_menu_entry_link = heap_alloc(template_count * sizeof(struct analysis_template_menu_link));
	}

	if (analysis_template_menu == NULL || analysis_template_menu_entry_link == NULL) {
		analysis_template_menu_destroy();
		return NULL;
	}

	/* Populate the menu. */

	width = 0;

	for (line = 0; line < template_count; line++) {
		/* Set up the link data.  A copy of the name is taken, because the original is in a flex block and could
		 * well move while the menu is open.  The account number is also stored, to allow the account to be found.
		 */

		template = analysis_get_template(file, line);
		if (template == NULL)
			continue;

		name = analysis_get_template_name(template, NULL, 0);
		if (name == NULL)
			continue;

		strncpy(analysis_template_menu_entry_link[line].name, name, ANALYSIS_SAVED_NAME_LEN);
		analysis_template_menu_entry_link[line].name[ANALYSIS_SAVED_NAME_LEN - 1] = '\0';
		if (!standalone) {
			strncat(analysis_template_menu_entry_link[line].name, "...", ANALYSIS_TEMPLATE_MENU_ELLIPSIS_LEN);
			analysis_template_menu_entry_link[line].name[ANALYSIS_SAVED_NAME_LEN + ANALYSIS_TEMPLATE_MENU_ELLIPSIS_LEN - 1] = '\0';
		}
		analysis_template_menu_entry_link[line].template = line;
		if (strlen(analysis_template_menu_entry_link[line].name) > width)
			width = strlen(analysis_template_menu_entry_link[line].name);
	}

	qsort(analysis_template_menu_entry_link, line, sizeof(struct analysis_template_menu_link), analysis_template_menu_compare_entries);

	for (line = 0; line < template_count; line++) {
		/* Set the menu and icon flags up. */

		analysis_template_menu->entries[line].menu_flags = 0;

		analysis_template_menu->entries[line].sub_menu = (wimp_menu *) -1;
		analysis_template_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
				wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
				wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

		/* Set the menu icon contents up. */

		analysis_template_menu->entries[line].data.indirected_text.text = analysis_template_menu_entry_link[line].name;
		analysis_template_menu->entries[line].data.indirected_text.validation = NULL;
		analysis_template_menu->entries[line].data.indirected_text.size = ACCOUNT_NAME_LEN;

		#ifdef DEBUG
		debug_printf("Line %d: '%s'", line, analysis_template_menu_entry_link[line].name);
		#endif
	}

	analysis_template_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	msgs_lookup((standalone) ? "RepListMenuT2" : "RepListMenuT1", analysis_template_menu_title, ANALYSIS_TEMPLATE_MENU_TITLE_LEN);
	analysis_template_menu->title_data.indirected_text.text = analysis_template_menu_title;
	analysis_template_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	analysis_template_menu->title_fg = wimp_COLOUR_BLACK;
	analysis_template_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	analysis_template_menu->work_fg = wimp_COLOUR_BLACK;
	analysis_template_menu->work_bg = wimp_COLOUR_WHITE;

	analysis_template_menu->width = (width + 1) * 16;
	analysis_template_menu->height = 44;
	analysis_template_menu->gap = 0;

	analysis_template_menu_file = file;

	return analysis_template_menu;
}


/**
 * Compare two Template List menu entries for the benefit of qsort().
 *
 * \param *va			The first item structure.
 * \param *vb			The second item structure.
 * \return			Comparison result.
 */

static int analysis_template_menu_compare_entries(const void *va, const void *vb)
{
	struct analysis_template_menu_link *a = (struct analysis_template_menu_link *) va;
	struct analysis_template_menu_link *b = (struct analysis_template_menu_link *) vb;

	return (string_nocase_strcmp(a->name, b->name));
}


/**
 * Given an index into the menu, return the template that it identifies.
 *
 * \param selection		The selection index to decode.
 * \return			The associated template, or NULL_TEMPLATE.
 */

template_t analysis_template_menu_decode(int selection)
{
	if (analysis_template_menu_entry_link == NULL || analysis_template_menu_file == NULL ||
			selection < 0 || selection >= analysis_get_template_count(analysis_template_menu_file))
		return NULL_TEMPLATE;

	return analysis_template_menu_entry_link[selection].template;
}

/**
 * Destroy any Template List menu which is currently open.
 */

void analysis_template_menu_destroy(void)
{
	if (analysis_template_menu != NULL)
		heap_free(analysis_template_menu);

	if (analysis_template_menu_entry_link != NULL)
		heap_free(analysis_template_menu_entry_link);

	analysis_template_menu_file = NULL;
	analysis_template_menu = NULL;
	analysis_template_menu_entry_link = NULL;
	*analysis_template_menu_title = '\0';
}
