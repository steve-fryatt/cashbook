/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "sorder_list_window.h"

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
#include "list_window.h"
#include "print_dialogue.h"
#include "sorder.h"
#include "sorder_dialogue.h"
#include "sorder_full_report.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
#include "transact.h"
#include "window.h"

/* Standing Order List Window icons. */

#define SORDER_LIST_WINDOW_FROM 0
#define SORDER_LIST_WINDOW_FROM_REC 1
#define SORDER_LIST_WINDOW_FROM_NAME 2
#define SORDER_LIST_WINDOW_TO 3
#define SORDER_LIST_WINDOW_TO_REC 4
#define SORDER_LIST_WINDOW_TO_NAME 5
#define SORDER_LIST_WINDOW_AMOUNT 6
#define SORDER_LIST_WINDOW_DESCRIPTION 7
#define SORDER_LIST_WINDOW_NEXTDATE 8
#define SORDER_LIST_WINDOW_LEFT 9

/* Standing Order List Window Toolbar icons. */

#define SORDER_LIST_WINDOW_PANE_FROM 0
#define SORDER_LIST_WINDOW_PANE_TO 1
#define SORDER_LIST_WINDOW_PANE_AMOUNT 2
#define SORDER_LIST_WINDOW_PANE_DESCRIPTION 3
#define SORDER_LIST_WINDOW_PANE_NEXTDATE 4
#define SORDER_LIST_WINDOW_PANE_LEFT 5

#define SORDER_LIST_WINDOW_PANE_PARENT 6
#define SORDER_LIST_WINDOW_PANE_ADDSORDER 7
#define SORDER_LIST_WINDOW_PANE_PRINT 8
#define SORDER_LIST_WINDOW_PANE_SORT 9

#define SORDER_LIST_WINDOW_PANE_SORT_DIR_ICON 10

/* Standing Order List Window Menu entries. */

#define SORDER_LIST_WINDOW_MENU_SORT 0
#define SORDER_LIST_WINDOW_MENU_EDIT 1
#define SORDER_LIST_WINDOW_MENU_NEWSORDER 2
#define SORDER_LIST_WINDOW_MENU_EXPCSV 3
#define SORDER_LIST_WINDOW_MENU_EXPTSV 4
#define SORDER_LIST_WINDOW_MENU_PRINT 5
#define SORDER_LIST_WINDOW_MENU_FULLREP 6

/* Standing Order Sort Window icons. */

#define SORDER_LIST_WINDOW_SORT_OK 2
#define SORDER_LIST_WINDOW_SORT_CANCEL 3
#define SORDER_LIST_WINDOW_SORT_FROM 4
#define SORDER_LIST_WINDOW_SORT_TO 5
#define SORDER_LIST_WINDOW_SORT_AMOUNT 6
#define SORDER_LIST_WINDOW_SORT_DESCRIPTION 7
#define SORDER_LIST_WINDOW_SORT_NEXTDATE 8
#define SORDER_LIST_WINDOW_SORT_LEFT 9
#define SORDER_LIST_WINDOW_SORT_ASCENDING 10
#define SORDER_LIST_WINDOW_SORT_DESCENDING 11

/**
 * The minimum number of entries in the Standing Order List Window.
 */

#define SORDER_LIST_WINDOW_MIN_ENTRIES 10

/**
 * The height of the Standing Order List Window toolbar, in OS Units.
 */

#define SORDER_LIST_WINDOW_TOOLBAR_HEIGHT 132

/**
 * The number of draggable columns in the Standing Order List Window.
 */

#define SORDER_LIST_WINDOW_COLUMNS 10

/* The Standing Order List Window column map. */

static struct column_map sorder_list_window_columns[SORDER_LIST_WINDOW_COLUMNS] = {
	{SORDER_LIST_WINDOW_FROM, wimp_ICON_WINDOW, SORDER_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_LIST_WINDOW_FROM_REC, SORDER_LIST_WINDOW_FROM, SORDER_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_LIST_WINDOW_FROM_NAME, SORDER_LIST_WINDOW_FROM, SORDER_LIST_WINDOW_PANE_FROM, wimp_ICON_WINDOW, SORT_FROM},
	{SORDER_LIST_WINDOW_TO, wimp_ICON_WINDOW, SORDER_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_LIST_WINDOW_TO_REC, SORDER_LIST_WINDOW_TO, SORDER_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_LIST_WINDOW_TO_NAME, SORDER_LIST_WINDOW_TO, SORDER_LIST_WINDOW_PANE_TO, wimp_ICON_WINDOW, SORT_TO},
	{SORDER_LIST_WINDOW_AMOUNT, wimp_ICON_WINDOW, SORDER_LIST_WINDOW_PANE_AMOUNT, wimp_ICON_WINDOW, SORT_AMOUNT},
	{SORDER_LIST_WINDOW_DESCRIPTION, wimp_ICON_WINDOW, SORDER_LIST_WINDOW_PANE_DESCRIPTION, wimp_ICON_WINDOW, SORT_DESCRIPTION},
	{SORDER_LIST_WINDOW_NEXTDATE, wimp_ICON_WINDOW, SORDER_LIST_WINDOW_PANE_NEXTDATE, wimp_ICON_WINDOW, SORT_NEXTDATE},
	{SORDER_LIST_WINDOW_LEFT, wimp_ICON_WINDOW, SORDER_LIST_WINDOW_PANE_LEFT, wimp_ICON_WINDOW, SORT_LEFT}
};

/**
 * The Standing Order List Window Sort Dialogue column icons.
 */

static struct sort_dialogue_icon sorder_list_window_sort_columns[] = {
	{SORDER_LIST_WINDOW_SORT_FROM, SORT_FROM},
	{SORDER_LIST_WINDOW_SORT_TO, SORT_TO},
	{SORDER_LIST_WINDOW_SORT_AMOUNT, SORT_AMOUNT},
	{SORDER_LIST_WINDOW_SORT_DESCRIPTION, SORT_DESCRIPTION},
	{SORDER_LIST_WINDOW_SORT_NEXTDATE, SORT_NEXTDATE},
	{SORDER_LIST_WINDOW_SORT_LEFT, SORT_LEFT},
	{0, SORT_NONE}
};

/**
 * The Standing Order List Window Sort Dialogue direction icons.
 */

static struct sort_dialogue_icon sorder_list_window_sort_directions[] = {
	{SORDER_LIST_WINDOW_SORT_ASCENDING, SORT_ASCENDING},
	{SORDER_LIST_WINDOW_SORT_DESCENDING, SORT_DESCENDING},
	{0, SORT_NONE}
};

/**
 * The Standing Order List Window definition.
 */

static struct list_window_definition sorder_list_window_definition = {
	"SOrder",					/**< The list window template name.		*/
	"SOrderTB",					/**< The list toolbar template name.		*/
	NULL,						/**< The list footer template name.		*/
	"SOrderMenu",					/**< The list menu template name.		*/
	SORDER_LIST_WINDOW_TOOLBAR_HEIGHT,		/**< The list toolbar height, in OS Units.	*/
	0,						/**< The list footer height, in OS Units.	*/
	sorder_list_window_columns,			/**< The window column definitions.		*/
	NULL,						/**< The window column extended definitions.	*/
	SORDER_LIST_WINDOW_COLUMNS,			/**< The number of column definitions.		*/
	"LimSOrderCols",				/**< The column width limit config token.	*/
	"SOrderCols",					/**< The column width config token.		*/
	SORDER_LIST_WINDOW_PANE_SORT_DIR_ICON,		/**< The toolbar icon used to show sort order.	*/
	"SortSOrder",					/**< The sort dialogue template name.		*/
	sorder_list_window_sort_columns,		/**< The sort dialogue column icons.		*/
	sorder_list_window_sort_directions,		/**< The sort dialogue direction icons.		*/
	SORDER_LIST_WINDOW_SORT_OK,			/**< The sort dialogue OK icon.			*/
	SORDER_LIST_WINDOW_SORT_CANCEL,			/**< The sort dialogue Cancel icon.		*/
	"SOrderTitle",					/*<< Window Title token.			*/
	"SOrder",					/**< Window Help token base.			*/
	"SOrderTB",					/**< Window Toolbar help token base.		*/
	NULL,						/**< Window Footer help token base.		*/
	"SOrderMenu",					/**< Window Menu help token base.		*/
	"SortSOrder",					/**< Sort dialogue help token base.		*/
	SORDER_LIST_WINDOW_MIN_ENTRIES,			/**< The minimum number of rows displayed.	*/
	0,						/**< The minimum number of blank lines.		*/
	"PrintSOrder",					/**< The print dialogue title token.		*/
	"PrintTitleSOrder",				/**< The print report title token.		*/
	FALSE,						/**< Should the print dialogue use dates?	*/
	LIST_WINDOW_FLAGS_NONE,				/**< The window flags.				*/

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


/**
 * Standing Order List Window instance data structure.
 */

struct sorder_list_window {
	/**
	 * The standing order instance owning the Standing Order List Window.
	 */
	struct sorder_block			*instance;

	/**
	 * The list window for the Preset List Window.
	 */
	struct list_window			*window;
};

/**
 * The Standing Order List Window base instance.
 */

static struct list_window_block		*sorder_list_window_block = NULL;

/**
 * The Save CSV saveas data handle.
 */

static struct saveas_block		*sorder_list_window_saveas_csv = NULL;

/**
 * The Save TSV saveas data handle.
 */

static struct saveas_block		*sorder_list_window_saveas_tsv = NULL;

/* Static Function Prototypes. */

static void sorder_list_window_click_handler(wimp_pointer *pointer, int index, struct file_block *file, void *data);
static void sorder_list_window_pane_click_handler(wimp_pointer *pointer, struct file_block *file, void *data);
static void sorder_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer, int index, struct file_block *file, void *data);
static void sorder_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection, wimp_pointer *pointer, int index, struct file_block *file, void *data);
static void sorder_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning, int index, struct file_block *file, void *data);
static void sorder_list_window_redraw_handler(int index, struct file_block *file, void *data, void *redraw);
static void sorder_list_window_open_print_window(struct sorder_list_window *windat, wimp_pointer *ptr, osbool restore);
static void sorder_list_window_print_field(struct file_block *file, wimp_i column, int sorder, char *rec_char);
static int sorder_list_window_sort_compare(enum sort_type type, int index1, int index2, struct file_block *file);
static osbool sorder_list_window_save_csv(char *filename, osbool selection, void *data);
static osbool sorder_list_window_save_tsv(char *filename, osbool selection, void *data);
static void sorder_list_window_export_delimited_line(FILE *out, enum filing_delimit_type format, struct file_block *file, int index);


/**
 * Initialise the Standing Order List Window system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_list_window_initialise(osspriteop_area *sprites)
{
	sorder_list_window_definition.callback_window_click_handler = sorder_list_window_click_handler;
	sorder_list_window_definition.callback_pane_click_handler = sorder_list_window_pane_click_handler;
	sorder_list_window_definition.callback_redraw_handler = sorder_list_window_redraw_handler;
	sorder_list_window_definition.callback_menu_prepare_handler = sorder_list_window_menu_prepare_handler;
	sorder_list_window_definition.callback_menu_selection_handler = sorder_list_window_menu_selection_handler;
	sorder_list_window_definition.callback_menu_warning_handler = sorder_list_window_menu_warning_handler;
	sorder_list_window_definition.callback_sort_compare = sorder_list_window_sort_compare;
	sorder_list_window_definition.callback_print_field = sorder_list_window_print_field;
	sorder_list_window_definition.callback_export_line = sorder_list_window_export_delimited_line;

	sorder_list_window_block = list_window_create(&sorder_list_window_definition, sprites);

	sorder_list_window_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", sorder_list_window_save_csv);
	sorder_list_window_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", sorder_list_window_save_tsv);
}


/**
 * Create a new Standing Order List Window instance.
 *
 * \param *parent		The parent sorder instance.
 * \return			Pointer to the new instance, or NULL.
 */

struct sorder_list_window *sorder_list_window_create_instance(struct sorder_block *parent)
{
	struct sorder_list_window *new;

	new = heap_alloc(sizeof(struct sorder_list_window));
	if (new == NULL)
		return NULL;

	new->instance = parent;
	new->window = NULL;

	/* Initialise the List Window. */

	new->window = list_window_create_instance(sorder_list_window_block, sorder_get_file(parent), new);
	if (new->window == NULL) {
		sorder_list_window_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a Standing Order List Window instance.
 *
 * \param *windat		The instance to be deleted.
 */

void sorder_list_window_delete_instance(struct sorder_list_window *windat)
{
	if (windat == NULL)
		return;

	list_window_delete_instance(windat->window);

	heap_free(windat);
}


/**
 * Create and open a Standing Order List window for the given instance.
 *
 * \param *windat		The instance to open a window for.
 */

void sorder_list_window_open(struct sorder_list_window *windat)
{
	if (windat == NULL)
		return;

	list_window_open(windat->window);
}


/**
 * Process mouse clicks in the Standing Order List window.
 *
 * \param *pointer		The mouse event block to handle.
 * \param index			The standing order under the pointer.
 * \param *file			The file owining the window.
 * \param *data			The Standing Order List Window instance.
 */

static void sorder_list_window_click_handler(wimp_pointer *pointer, int index, struct file_block *file, void *data)
{
	if (pointer->buttons == wimp_DOUBLE_SELECT)
		sorder_open_edit_window(file, index, pointer);
}


/**
 * Process mouse clicks in the Standing Order List pane.
 *
 * \param *pointer		The mouse event block to handle.
 * \param *file			The file owining the window.
 * \param *data			The Standing Order List Window instance.
 */

static void sorder_list_window_pane_click_handler(wimp_pointer *pointer, struct file_block *file, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL || windat->instance == NULL)
		return;

	if (pointer->buttons == wimp_CLICK_SELECT) {
		switch (pointer->i) {
		case SORDER_LIST_WINDOW_PANE_PARENT:
			transact_bring_window_to_top(file);
			break;

		case SORDER_LIST_WINDOW_PANE_PRINT:
			sorder_list_window_open_print_window(windat, pointer, config_opt_read("RememberValues"));
			break;

		case SORDER_LIST_WINDOW_PANE_ADDSORDER:
			sorder_open_edit_window(file, NULL_SORDER, pointer);
			break;

		case SORDER_LIST_WINDOW_PANE_SORT:
			list_window_open_sort_window(windat->window, pointer);
			break;
		}
	} else if (pointer->buttons == wimp_CLICK_ADJUST) {
		switch (pointer->i) {
		case SORDER_LIST_WINDOW_PANE_PRINT:
			sorder_list_window_open_print_window(windat, pointer, !config_opt_read("RememberValues"));
			break;

		case SORDER_LIST_WINDOW_PANE_SORT:
			sorder_list_window_sort(windat);
			break;
		}
	}
}


/**
 * Process menu prepare events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 * \param index		The index of the entry under the menu, or LIST_WINDOW_NULL_INDEX.
 * \param *file		The file owning the standing order list window.
 * \param *data		The standing order list window instance.
 */

static void sorder_list_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer, int index, struct file_block *file, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL)
		return;

	if (pointer != NULL) {
		saveas_initialise_dialogue(sorder_list_window_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, windat);
		saveas_initialise_dialogue(sorder_list_window_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, windat);
	}

	menus_shade_entry(menu, SORDER_LIST_WINDOW_MENU_EDIT, index == LIST_WINDOW_NULL_INDEX);
}


/**
 * Process menu selection events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 * \param *pointer	The pointer position.
 * \param index		The index of the entry under the menu, or LIST_WINDOW_NULL_INDEX.
 * \param *file		The file owning the standing order list window.
 * \param *data		The standing order list window instance.
 */

static void sorder_list_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection, wimp_pointer *pointer, int index, struct file_block *file, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL || windat->instance == NULL)
		return;

	switch (selection->items[0]){
	case SORDER_LIST_WINDOW_MENU_SORT:
		list_window_open_sort_window(windat->window, pointer);
		break;

	case SORDER_LIST_WINDOW_MENU_EDIT:
		if (index != LIST_WINDOW_NULL_INDEX)
			sorder_open_edit_window(file, index, pointer);
		break;

	case SORDER_LIST_WINDOW_MENU_NEWSORDER:
		sorder_open_edit_window(file, NULL_SORDER, pointer);
		break;

	case SORDER_LIST_WINDOW_MENU_PRINT:
		sorder_list_window_open_print_window(windat, pointer, config_opt_read("RememberValues"));
		break;

	case SORDER_LIST_WINDOW_MENU_FULLREP:
		sorder_full_report(file);
		break;
	}
}


/**
 * Process submenu warning events in the Standing Order List window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 * \param index		The index of the entry under the menu, or LIST_WINDOW_NULL_INDEX.
 * \param *file		The file owning the standing order list window.
 * \param *data		The standing order list window instance.
 */

static void sorder_list_window_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning, int index, struct file_block *file, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL)
		return;

	switch (warning->selection.items[0]) {
	case SORDER_LIST_WINDOW_MENU_EXPCSV:
		saveas_prepare_dialogue(sorder_list_window_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case SORDER_LIST_WINDOW_MENU_EXPTSV:
		saveas_prepare_dialogue(sorder_list_window_saveas_tsv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process redraw events in the Standing Order List window.
 *
 * \param *index		The index of the item in the line to be redrawn.
 * \param *file			Pointer to the owning file instance.
 * \param *data			Pointer to the Standing Order List Window instance.
 * \param *redraw		Pointer to the redraw instance data.
 */

static void sorder_list_window_redraw_handler(int index, struct file_block *file, void *data, void *redraw)
{
	sorder_t			sorder = index;
	acct_t				account;
	date_t				next_date;
	enum transact_flags		flags;

	flags =  sorder_get_flags(file, sorder);

	/* From field */

	account = sorder_get_from(file, sorder);

	window_plot_text_field(SORDER_LIST_WINDOW_FROM, account_get_ident(file, account), wimp_COLOUR_BLACK);
	window_plot_reconciled_field(SORDER_LIST_WINDOW_FROM_REC, (flags & TRANS_REC_FROM), wimp_COLOUR_BLACK);
	window_plot_text_field(SORDER_LIST_WINDOW_FROM_NAME, account_get_name(file, account), wimp_COLOUR_BLACK);

	/* To field */

	account = sorder_get_to(file, sorder);

	window_plot_text_field(SORDER_LIST_WINDOW_TO, account_get_ident(file, account), wimp_COLOUR_BLACK);
	window_plot_reconciled_field(SORDER_LIST_WINDOW_TO_REC, (flags & TRANS_REC_TO), wimp_COLOUR_BLACK);
	window_plot_text_field(SORDER_LIST_WINDOW_TO_NAME, account_get_name(file, account), wimp_COLOUR_BLACK);

	/* Amount field */

	window_plot_currency_field(SORDER_LIST_WINDOW_AMOUNT, sorder_get_amount(file, sorder, SORDER_AMOUNT_NORMAL), wimp_COLOUR_BLACK);

	/* Description field */

	window_plot_text_field(SORDER_LIST_WINDOW_DESCRIPTION, sorder_get_description(file, sorder, NULL, 0), wimp_COLOUR_BLACK);

	/* Next date field */

	next_date = sorder_get_date(file, sorder, SORDER_DATE_ADJUSTED_NEXT);

	if (next_date != NULL_DATE)
		window_plot_date_field(SORDER_LIST_WINDOW_NEXTDATE, next_date, wimp_COLOUR_BLACK);
	else
		window_plot_message_field(SORDER_LIST_WINDOW_NEXTDATE, "SOrderStopped", wimp_COLOUR_BLACK);

	/* Left field */

	window_plot_int_field(SORDER_LIST_WINDOW_LEFT, sorder_get_transactions(file, sorder, SORDER_TRANSACTIONS_LEFT), wimp_COLOUR_BLACK);
}


/**
 * Force the redraw of one or all of the standing orders in the given
 * Standing Order list window.
 *
 * \param *windat		The standing order window to redraw.
 * \param sorder		The standing order to redraw, or NULL_SORDER for all.
 * \param stopped		TRUE to redraw just the active columns.
 */

void sorder_list_window_redraw(struct sorder_list_window *windat, sorder_t sorder, osbool stopped)
{
	if (windat == NULL)
		return;

	if (stopped) {
		list_window_redraw(windat->window, sorder, 2,
				SORDER_LIST_WINDOW_PANE_NEXTDATE, SORDER_LIST_WINDOW_PANE_LEFT);
	} else {
		list_window_redraw(windat->window, sorder, 0);
	}
}


/**
 * Find the standing order which corresponds to a display line in the specified
 * standing order list window.
 *
 * \param *windat		The standing order list window to search in.
 * \param line			The display line to return the standing order for.
 * \return			The appropriate transaction, or NULL_SORDER.
 */

sorder_t sorder_list_window_get_sorder_from_line(struct sorder_list_window *windat, int line)
{
	if (windat == NULL)
		return NULL_SORDER;

	return list_window_get_index_from_line(windat->window, line);
}


/**
 * Open the Standing Order Print dialogue for a given standing order list window.
 *
 * \param *file			The standing order window to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 * \param restore		TRUE to restore the current settings; else FALSE.
 */

static void sorder_list_window_open_print_window(struct sorder_list_window *windat, wimp_pointer *ptr, osbool restore)
{
	list_window_open_print_window(windat->window, ptr, restore);
}


/**
 * Send the contents of the Standing Order Window to the printer, via the reporting
 * system.
 *
 * \param *file			The file owning the standing order list.
 * \param column		The column to be output.
 * \param sorder		The standing order to be output.
 * \param *rec_char		A string to use as the reconcile character.
 */

static void sorder_list_window_print_field(struct file_block *file, wimp_i column, sorder_t sorder, char *rec_char)
{
	date_t next_date;

	switch (column) {
	case SORDER_LIST_WINDOW_FROM:
		stringbuild_add_string(account_get_ident(file, sorder_get_from(file, sorder)));
		break;
	case SORDER_LIST_WINDOW_FROM_REC:
		if (sorder_get_flags(file, sorder) & TRANS_REC_FROM)
			stringbuild_add_string(rec_char);
		break;
	case SORDER_LIST_WINDOW_FROM_NAME:
		stringbuild_add_string("\\v");
		stringbuild_add_string(account_get_name(file, sorder_get_from(file, sorder)));
		break;
	case SORDER_LIST_WINDOW_TO:
		stringbuild_add_string(account_get_ident(file, sorder_get_to(file, sorder)));
		break;
	case SORDER_LIST_WINDOW_TO_REC:
		if (sorder_get_flags(file, sorder) & TRANS_REC_TO)
			stringbuild_add_string(rec_char);
		break;
	case SORDER_LIST_WINDOW_TO_NAME:
		stringbuild_add_string("\\v");
		stringbuild_add_string(account_get_name(file, sorder_get_to(file, sorder)));
		break;
	case SORDER_LIST_WINDOW_AMOUNT:
		stringbuild_add_string("\\v\\d\\r");
		stringbuild_add_currency(sorder_get_amount(file, sorder, SORDER_AMOUNT_NORMAL), FALSE);
		break;
	case SORDER_LIST_WINDOW_DESCRIPTION:
		stringbuild_add_string("\\v");
		stringbuild_add_string(sorder_get_description(file, sorder, NULL, 0));
		break;
	case SORDER_LIST_WINDOW_NEXTDATE:
		stringbuild_add_string("\\v\\c");
		next_date = sorder_get_date(file, sorder, SORDER_DATE_ADJUSTED_NEXT);
		if (next_date != NULL_DATE)
			stringbuild_add_date(next_date);
		else
			stringbuild_add_message("SOrderStopped");
		break;
	case SORDER_LIST_WINDOW_LEFT:
		stringbuild_add_printf("\\v\\d\\r%d", sorder_get_transactions(file, sorder, SORDER_TRANSACTIONS_LEFT));
		break;
	default:
		stringbuild_add_string("\\s");
		break;
	}
}


/**
 * Sort the standing orders in a given list window based on that instances's
 * sort setting.
 *
 * \param *windat		The standing order window instance to sort.
 */

void sorder_list_window_sort(struct sorder_list_window *windat)
{
	if (windat == NULL)
		return;

	list_window_sort(windat->window);
}

/**
 * Compare two lines of a standing order list, returning the result of the
 * in terms of a positive value, zero or a negative value.
 *
 * \param type			The required column type of the comparison.
 * \param index1		The index of the first line to be compared.
 * \param index2		The index of the second line to be compared.
 * \param *file			The file relating to the data being sorted.
 * \return			The comparison result.
 */

static int sorder_list_window_sort_compare(enum sort_type type, int index1, int index2, struct file_block *file)
{
	switch (type) {
	case SORT_FROM:
		return strcmp(account_get_name(file, sorder_get_from(file, index1)),
				account_get_name(file, sorder_get_from(file, index2)));

	case SORT_TO:
		return strcmp(account_get_name(file, sorder_get_to(file, index1)),
				account_get_name(file, sorder_get_to(file, index2)));

	case SORT_AMOUNT:
		return (sorder_get_amount(file, index1, SORDER_AMOUNT_NORMAL) -
				sorder_get_amount(file, index2, SORDER_AMOUNT_NORMAL));

	case SORT_DESCRIPTION:
		return strcmp(sorder_get_description(file, index1, NULL, 0),
				sorder_get_description(file, index2, NULL, 0));

	case SORT_NEXTDATE:
		return ((sorder_get_date(file, index2, SORDER_DATE_ADJUSTED_NEXT) & DATE_SORT_MASK) -
				(sorder_get_date(file, index1, SORDER_DATE_ADJUSTED_NEXT) & DATE_SORT_MASK));

	case SORT_LEFT:
		return  (sorder_get_transactions(file, index1, SORDER_TRANSACTIONS_LEFT) -
				sorder_get_transactions(file, index2, SORDER_TRANSACTIONS_LEFT));

	default:
		return 0;
	}
}


/**
 * Initialise the contents of the standing order list window, creating an
 * entry for each of the required standing orders.
 *
 * \param *windat		The standing order list window instance to initialise.
 * \param sorders		The number of standing orders to insert.
 * \return			TRUE on success; FALSE on failure.
 */

osbool sorder_list_window_initialise_entries(struct sorder_list_window *windat, int sorders)
{
	if (windat == NULL)
		return FALSE;

	return list_window_initialise_entries(windat->window, sorders);
}


/**
 * Add a new standing order to an instance of the standing order list window.
 *
 * \param *windat		The standing order list window instance to add to.
 * \param sorder		The standing order index to add.
 * \return			TRUE on success; FALSE on failure.
 */

osbool sorder_list_window_add_sorder(struct sorder_list_window *windat, sorder_t sorder)
{
	if (windat == NULL)
		return FALSE;

	return list_window_add_entry(windat->window, sorder, config_opt_read("AutoSortSOrders"));
}


/**
 * Remove a standing order from an instance of the standing order list window,
 * and update the other entries to allow for its deletion.
 *
 * \param *windat		The standing order list window instance to remove from.
 * \param sorder		The standing order index to remove.
 * \return			TRUE on success; FALSE on failure.
 */

osbool sorder_list_window_delete_sorder(struct sorder_list_window *windat, sorder_t sorder)
{
	if (windat == NULL)
		return FALSE;

	return list_window_delete_entry(windat->window, sorder, config_opt_read("AutoSortSOrders"));
}


/**
 * Save the standing order list window details from a window to a CashBook
 * file. This assumes that the caller has already created a suitable section
 * in the file to be written.
 *
 * \param *windat		The window whose details to write.
 * \param *out			The file handle to write to.
 */

void sorder_list_window_write_file(struct sorder_list_window *windat, FILE *out)
{
	if (windat == NULL)
		return;

	list_window_write_file(windat->window, out);
}


/**
 * Process a WinColumns line from the StandingOrders section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The column text line.
 */

void sorder_list_window_read_file_wincolumns(struct sorder_list_window *windat, char *columns)
{
	if (windat == NULL)
		return;

	list_window_read_file_wincolumns(windat->window, 0, TRUE, columns);
}


/**
 * Process a SortOrder line from the StandingOrders section of a file.
 *
 * \param *windat		The window being read in to.
 * \param *columns		The sort order text line.
 */

void sorder_list_window_read_file_sortorder(struct sorder_list_window *windat, char *order)
{
	if (windat == NULL)
		return;

	list_window_read_file_sortorder(windat->window, order);
}


/**
 * Callback handler for saving a CSV version of the standing order data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool sorder_list_window_save_csv(char *filename, osbool selection, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	list_window_export_delimited(windat->window, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);

	return TRUE;
}


/**
 * Callback handler for saving a TSV version of the standing order data.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the window block for the save target.
 */

static osbool sorder_list_window_save_tsv(char *filename, osbool selection, void *data)
{
	struct sorder_list_window *windat = data;

	if (windat == NULL)
		return FALSE;

	list_window_export_delimited(windat->window, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);

	return TRUE;
}


/**
 * Export the standing order data from a file into CSV or TSV format.
 *
 * \param *windat		The standing order window to export from.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void sorder_list_window_export_delimited_line(FILE *out, enum filing_delimit_type format, struct file_block *file, int index)
{
	char	buffer[FILING_DELIMITED_FIELD_LEN];
	date_t	next_date;

	account_build_name_pair(file, sorder_get_from(file, index), buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

	account_build_name_pair(file, sorder_get_to(file, index), buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

	currency_convert_to_string(sorder_get_amount(file, index, SORDER_AMOUNT_NORMAL), buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NUM);

	filing_output_delimited_field(out, sorder_get_description(file, index, NULL, 0), format, DELIMIT_NONE);

	next_date = sorder_get_date(file, index, SORDER_DATE_ADJUSTED_NEXT);
	if (next_date != NULL_DATE)
		date_convert_to_string(next_date, buffer, FILING_DELIMITED_FIELD_LEN);
	else
		msgs_lookup("SOrderStopped", buffer, FILING_DELIMITED_FIELD_LEN);
	filing_output_delimited_field(out, buffer, format, DELIMIT_NONE);

	string_printf(buffer, FILING_DELIMITED_FIELD_LEN, "%d", sorder_get_transactions(file, index, SORDER_TRANSACTIONS_LEFT));
	filing_output_delimited_field(out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
}

