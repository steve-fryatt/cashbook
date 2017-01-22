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
 * \file: refdesc_menu.c
 *
 * Reference and Descripton completion menu implementation.
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
#include "refdesc_menu.h"

#include "account.h"
#include "amenu.h"
#include "transact.h"

/**
 * Static entries in the menu.
 */

#define REFDESC_MENU_CHEQUE 0

/**
 * The number of items at a time for which data is allocated.
 */

#define REFDESC_MENU_BLOCKSIZE 50

/**
 * The length of the menu title buffer.
 */

#define REFDESC_MENU_TITLE_LEN 32

/* Data Structures */

struct refdesc_menu_link {
	char		name[TRANSACT_DESCRIPT_FIELD_LEN];		/**< Space for the menu entry.			*/
};


/* Global Variables. */

/**
 * The type of menu -- reference or description -- currently open.
 */

static enum refdesc_menu_type		refdesc_menu_active_type = REFDESC_MENU_NONE;

/**
 * Pointer to the file currently owning the menu.
 */

static struct file_block		*refdesc_menu_file = NULL;

/**
 * The window line to which the menu currently applies.
 */

static int				refdesc_menu_line = -1;	

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


/* Static Function Prototypes. */

static void		refdesc_menu_prepare(void);
static void		refdesc_menu_decode(wimp_selection *selection);
static wimp_menu	*refdesc_menu_build(struct file_block *file, enum refdesc_menu_type menu_type, int start_line);
static void		refdesc_menu_add_entry(struct refdesc_menu_link **entries, int *count, int *max, char *new);
static void		refdesc_menu_destroy(void);
static int		refdesc_menu_compare(const void *va, const void *vb);


/**
 * Create and open a Reference or Description completion menu over a line
 * in a transaction window.
 * 
 * \param *file			The file to which the menu will belong.
 * \param menu_type		The type of menu to be opened.
 * \param line			The line of the window over which the menu opened.
 * \param *pointer		Pointer to the Wimp pointer details.
 */

void refdesc_menu_open(struct file_block *file, enum refdesc_menu_type menu_type, int line, wimp_pointer *pointer)
{
	wimp_menu	*menu;
	char		*token = NULL;

	if (menu_type == REFDESC_MENU_NONE)
		return;

	menu = refdesc_menu_build(file, menu_type, line);

	if (menu == NULL)
		return;

	switch (menu_type) {
	case REFDESC_MENU_REFERENCE:
		token = "RefMenu";
		break;

	case REFDESC_MENU_DESCRIPTION:
		token = "DescMenu";
		break;
	case REFDESC_MENU_NONE:
		token = "";
		break;
	}

	amenu_open(menu, token, pointer, refdesc_menu_prepare, NULL, refdesc_menu_decode, refdesc_menu_destroy);
}


/**
 * Prepare the currently active Reference or Description menu for opening or
 * reopening, by shading lines which shouldn't be selectable.
 */

static void refdesc_menu_prepare(void)
{
	tran_t		transaction;
	acct_t		from = NULL_ACCOUNT, to = NULL_ACCOUNT;

	if (refdesc_menu == NULL || refdesc_menu_file == NULL || refdesc_menu_active_type != REFDESC_MENU_REFERENCE)
		return;

	transaction = transact_get_transaction_from_line(refdesc_menu_file, refdesc_menu_line);

	if (transaction != NULL_TRANSACTION) {
		from = transact_get_from(refdesc_menu_file, transaction);
		to = transact_get_to(refdesc_menu_file, transaction);
	}

	if ((transaction != NULL_TRANSACTION) && (from != NULL_ACCOUNT) && account_cheque_number_available(refdesc_menu_file, from))
		refdesc_menu->entries[REFDESC_MENU_CHEQUE].icon_flags &= ~wimp_ICON_SHADED;
	else
		refdesc_menu->entries[REFDESC_MENU_CHEQUE].icon_flags |= wimp_ICON_SHADED;
}


/**
 * Given a menu selection, decode and process the user's choice from a
 * Reference or Descripiion menu.
 * 
 * \param *selection		The selection from the menu.
 */

static void refdesc_menu_decode(wimp_selection *selection)
{
	int		i;
	char		cheque_buffer[TRANSACT_REF_FIELD_LEN];

	if (refdesc_menu_file == NULL || refdesc_menu_entry_link == NULL || selection->items[0] == -1)
		return;

	/* Check that the line is in the range of transactions.  If not, add blank transactions to the file until it is.
	 * This really ought to be in edit.c!
	 */

	if (refdesc_menu_line >= transact_get_count(refdesc_menu_file) && selection->items[0] != -1) {
		for (i = transact_get_count(refdesc_menu_file); i <= refdesc_menu_line; i++)
			transact_add_raw_entry(refdesc_menu_file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, TRANS_FLAGS_NONE, NULL_CURRENCY, "", "");
	}

	/* Again check that the transaction is in range.  If it isn't, the additions failed. */

	if (refdesc_menu_line >= transact_get_count(refdesc_menu_file))
		return;

	if (refdesc_menu_active_type == REFDESC_MENU_REFERENCE && selection->items[0] == REFDESC_MENU_CHEQUE) {
		account_get_next_cheque_number(refdesc_menu_file,
				transact_get_from(refdesc_menu_file, transact_get_transaction_from_line(refdesc_menu_file, refdesc_menu_line)),
				transact_get_to(refdesc_menu_file, transact_get_transaction_from_line(refdesc_menu_file, refdesc_menu_line)),
				1, cheque_buffer, TRANSACT_REF_FIELD_LEN);
		transact_change_refdesc(refdesc_menu_file, transact_get_transaction_from_line(refdesc_menu_file, refdesc_menu_line),
				TRANSACT_FIELD_REF, cheque_buffer);
	} else if (refdesc_menu_active_type == REFDESC_MENU_REFERENCE) {
		transact_change_refdesc(refdesc_menu_file, transact_get_transaction_from_line(refdesc_menu_file, refdesc_menu_line),
				TRANSACT_FIELD_REF, refdesc_menu_entry_link[selection->items[0]].name);
	} else if (refdesc_menu_active_type == REFDESC_MENU_DESCRIPTION) {
		transact_change_refdesc(refdesc_menu_file, transact_get_transaction_from_line(refdesc_menu_file, refdesc_menu_line),
				TRANSACT_FIELD_DESC, refdesc_menu_entry_link[selection->items[0]].name);
	}
}


/**
 * Build a Reference or Drescription Complete menu for a given file.
 *
 * \param *file			The file to build the menu for.
 * \param type			The type of menu to build.
 * \param start_line		The line of the window to start from.
 * \return			The menu block, or NULL.
 */

static wimp_menu *refdesc_menu_build(struct file_block *file, enum refdesc_menu_type menu_type, int start_line)
{
	int		i, transaction_count, range, line, width, items, max_items, item_limit;
	tran_t		start_transaction, compare_transaction;
	osbool		no_original;
	char		*title_token;
	char		start_text[TRANSACT_DESCRIPT_FIELD_LEN], *compare_text;

	if (file == NULL || file->transacts == NULL)
		return NULL;

	hourglass_on();

	refdesc_menu_destroy();

	refdesc_menu_file = file;
	refdesc_menu_line = start_line;
	refdesc_menu_active_type = menu_type;

	/* Claim enough memory to build the menu in. Note that this might return NULL;
	 * we can cope with this and carry on anyway.
	 */

	max_items = REFDESC_MENU_BLOCKSIZE;
	refdesc_menu_entry_link = heap_alloc((sizeof(struct refdesc_menu_link) * max_items));

	items = 0;
	item_limit = config_int_read("MaxAutofillLen");

	/* In the Reference menu, the first item needs to be the Cheque No. entry, so insert that manually. */

	if (refdesc_menu_entry_link != NULL && menu_type == REFDESC_MENU_REFERENCE)
		msgs_lookup("RefMenuChq", refdesc_menu_entry_link[items++].name, TRANSACT_DESCRIPT_FIELD_LEN);

	/* Bring the start line into range for the current file.  no_original is set true if the line fell off the end
	 * of the file, as this needs to be a special case of "no text".  If not, lines off the end of the file will
	 * be matched against the final transaction as a result of pulling start_line into range.
	 */

	transaction_count = transact_get_count(file);

	if (start_line >= transaction_count) {
		start_line = transaction_count - 1;
		no_original = TRUE;
	} else {
		no_original = FALSE;
	}

	if (transaction_count > 0 && refdesc_menu_entry_link != NULL) {
		/* Find the largest range from the current line to one end of the transaction list. */

		range = ((transaction_count - start_line - 1) > start_line) ? (transaction_count - start_line - 1) : start_line;

		/* Work out from the line to the edges of the transaction window. For each transaction, check the entries
		 * and add them into the list.
		 */

		start_transaction = transact_get_transaction_from_line(file, start_line);

		if (menu_type == REFDESC_MENU_REFERENCE) {
			transact_get_reference(file, start_transaction, start_text, TRANSACT_DESCRIPT_FIELD_LEN);

			for (i=1; i<=range && (item_limit == 0 || items <= item_limit); i++) {
				if (start_line + i < transaction_count) {
					compare_transaction = transact_get_transaction_from_line(file, start_line + i);
					compare_text = transact_get_reference(file, compare_transaction, NULL, 0);

					if (no_original || (*start_text == '\0') || (string_nocase_strstr(compare_text, start_text) == compare_text))
						refdesc_menu_add_entry(&refdesc_menu_entry_link, &items, &max_items, compare_text);
				}
				if (start_line - i >= 0) {
					compare_transaction = transact_get_transaction_from_line(file, start_line - i);
					compare_text = transact_get_reference(file, compare_transaction, NULL, 0);

					if (no_original || (*start_text == '\0') || (string_nocase_strstr(compare_text, start_text) == compare_text))
						refdesc_menu_add_entry(&refdesc_menu_entry_link, &items, &max_items, compare_text);
				}
			}
		} else if (menu_type == REFDESC_MENU_DESCRIPTION) {
			transact_get_description(file, start_transaction, start_text, TRANSACT_DESCRIPT_FIELD_LEN);

			for (i=1; i<=range && (item_limit == 0 || items < item_limit); i++) {
				if (start_line + i < transaction_count) {
					compare_transaction = transact_get_transaction_from_line(file, start_line + i);
					compare_text = transact_get_description(file, compare_transaction, NULL, 0);

					if (no_original || (*start_text == '\0') || (string_nocase_strstr(compare_text, start_text) == compare_text))
						refdesc_menu_add_entry(&refdesc_menu_entry_link, &items, &max_items, compare_text);
				}
				if (start_line - i >= 0) {
					compare_transaction = transact_get_transaction_from_line(file, start_line - i);
					compare_text = transact_get_description(file, compare_transaction, NULL, 0);

					if (no_original || (*start_text == '\0') || (string_nocase_strstr(compare_text, start_text) == compare_text))
						refdesc_menu_add_entry(&refdesc_menu_entry_link, &items, &max_items, compare_text);
				}
			}
		}
	  }

	/* If there are items in the menu, claim the extra memory required to build the Wimp menu structure and
	 * set up the pointers.   If there are not, transact_complete_menu will remain NULL and the menu won't exist.
	 *
	 * transact_complete_menu_link may be NULL if memory allocation failed at any stage of the build process.
	 */

	if (refdesc_menu_entry_link == NULL || items == 0) {
		refdesc_menu_destroy();
		hourglass_off();
		return NULL;
	}

	refdesc_menu = heap_alloc(28 + (24 * max_items));

	if (refdesc_menu == NULL || refdesc_menu_title == NULL) {
		refdesc_menu_destroy();
		hourglass_off();
		return NULL;
	}

	/* Populate the menu. */

	line = 0;
	width = 0;

	if (menu_type == REFDESC_MENU_REFERENCE)
		qsort(refdesc_menu_entry_link + 1, items - 1, sizeof(struct refdesc_menu_link), refdesc_menu_compare);
	else
		qsort(refdesc_menu_entry_link, items, sizeof(struct refdesc_menu_link), refdesc_menu_compare);

	if (items > 0) {
		for (i=0; i < items; i++) {
			if (strlen(refdesc_menu_entry_link[line].name) > width)
				width = strlen(refdesc_menu_entry_link[line].name);

			/* Set the menu and icon flags up. */

			if (menu_type == REFDESC_MENU_REFERENCE && i == REFDESC_MENU_CHEQUE)
				refdesc_menu->entries[line].menu_flags = (items > 1) ? wimp_MENU_SEPARATE : 0;
			else
				refdesc_menu->entries[line].menu_flags = 0;

			refdesc_menu->entries[line].sub_menu = (wimp_menu *) -1;
			refdesc_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			refdesc_menu->entries[line].data.indirected_text.text = refdesc_menu_entry_link[line].name;
			refdesc_menu->entries[line].data.indirected_text.validation = NULL;
			refdesc_menu->entries[line].data.indirected_text.size = TRANSACT_DESCRIPT_FIELD_LEN;

			line++;
		}
	}

	/* Finish off the menu, marking the last entry and filling in the header. */

	refdesc_menu->entries[(line > 0) ? line-1 : 0].menu_flags |= wimp_MENU_LAST;

	switch (menu_type) {
	case REFDESC_MENU_REFERENCE:
		title_token = "RefMenuTitle";
		break;

	case REFDESC_MENU_DESCRIPTION:
	default:
		title_token = "DescMenuTitle";
		break;
	}

	msgs_lookup(title_token, refdesc_menu_title, REFDESC_MENU_TITLE_LEN);

	refdesc_menu->title_data.indirected_text.text = refdesc_menu_title;
	refdesc_menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	refdesc_menu->title_fg = wimp_COLOUR_BLACK;
	refdesc_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	refdesc_menu->work_fg = wimp_COLOUR_BLACK;
	refdesc_menu->work_bg = wimp_COLOUR_WHITE;

	refdesc_menu->width = (width + 1) * 16;
	refdesc_menu->height = 44;
	refdesc_menu->gap = 0;

	hourglass_off();

	return refdesc_menu;
}


/**
 * Add a reference or description text to the list menu.
 *
 * \param **entries		Pointer to the reference struct pointer, allowing
 *				it to be updated if the memory needs to grow.
 * \param *count		Pointer to the item count, for update.
 * \param *max			Pointer to the max item count, for update.
 * \param *new			The nex text item to be added.
 */

static void refdesc_menu_add_entry(struct refdesc_menu_link **entries, int *count, int *max, char *new)
{
	int		i;
	osbool		found = FALSE;

	if (*entries == NULL || new == NULL || *new == '\0')
		return;

	for (i = 0; (i < *count) && !found; i++)
		if (string_nocase_strcmp((*entries)[i].name, new) == 0)
			found = TRUE;

	if (!found && *count < (*max))
		strcpy((*entries)[(*count)++].name, new);

	/* Extend the block *after* the copy, in anticipation of the next call, because this could easily move the
	 * flex blocks around and that would invalidate the new pointer...
	 */

	if (*count >= (*max)) {
		*entries = heap_extend(*entries, sizeof(struct refdesc_menu_link) * (*max + REFDESC_MENU_BLOCKSIZE));
		*max += REFDESC_MENU_BLOCKSIZE;
	}
}


/**
 * Compare two menu entries, for qsort().
 *
 * \param *va			The first entry to compare.
 * \param *vb			The second entry to compare.
 * \return			The comparison result.
 */

static int refdesc_menu_compare(const void *va, const void *vb)
{
	struct refdesc_menu_link *a = (struct refdesc_menu_link *) va, *b = (struct refdesc_menu_link *) vb;

	return (string_nocase_strcmp(a->name, b->name));
}


/**
 * Destroy any Reference or Description Complete menu which is currently open.
 */

static void refdesc_menu_destroy(void)
{
	if (refdesc_menu != NULL)
		heap_free(refdesc_menu);

	if (refdesc_menu_entry_link != NULL)
		heap_free(refdesc_menu_entry_link);

	refdesc_menu = NULL;
	refdesc_menu_entry_link = NULL;
	refdesc_menu_file = NULL;
	refdesc_menu_active_type = REFDESC_MENU_NONE;
	*refdesc_menu_title = '\0';
}
