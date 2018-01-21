/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: choices.c
 *
 * Choices dialogue implementation.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>

/* Acorn C Header files. */


/* OSLib Header files. */

#include "oslib/wimp.h"

/* SF-Lib Header files. */

#include "sflib/colpick.h"
#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files. */

#include "global.h"
#include "choices.h"

#include "caret.h"
#include "currency.h"
#include "date.h"
#include "file.h"
#include "fontlist.h"


/* Pane numbers */

#define CHOICES_PANES 7

#define CHOICE_PANE_GENERAL 0
#define CHOICE_PANE_CURRENCY 1
#define CHOICE_PANE_TRANSACT 2
#define CHOICE_PANE_ACCOUNT 3
#define CHOICE_PANE_SORDER 4
#define CHOICE_PANE_PRINT 5
#define CHOICE_PANE_REPORT 6

/* Main window icons. */

#define CHOICE_ICON_APPLY 0
#define CHOICE_ICON_SAVE 1
#define CHOICE_ICON_CANCEL 2
#define CHOICE_ICON_DEFAULT 3
#define CHOICE_ICON_PANE 4
#define CHOICE_ICON_SELECT 5

/* General pane icons. */

#define CHOICE_ICON_REMEMBERDIALOGUE 0
#define CHOICE_ICON_CLIPBOARD 4
#define CHOICE_ICON_RO5KEYS 3
#define CHOICE_ICON_TDRAG 15
#define CHOICE_ICON_DATEFORMAT 8
#define CHOICE_ICON_DATEFORMAT_POP 9
#define CHOICE_ICON_DATEIN 11
#define CHOICE_ICON_DATEOUT 13
#define CHOICE_ICON_TERRITORYDATE 14

/* Currency pane icons. */

#define CHOICE_ICON_SHOWZERO 0
#define CHOICE_ICON_TERRITORYNUM 1
#define CHOICE_ICON_FORMATFRAME 2
#define CHOICE_ICON_FORMATLABEL 3
#define CHOICE_ICON_DECIMALPLACELABEL 4
#define CHOICE_ICON_DECIMALPLACE 5
#define CHOICE_ICON_DECIMALPOINTLABEL 6
#define CHOICE_ICON_DECIMALPOINT 7
#define CHOICE_ICON_NEGFRAME 8
#define CHOICE_ICON_NEGLABEL 9
#define CHOICE_ICON_NEGMINUS 10
#define CHOICE_ICON_NEGBRACE 11

/* Standing order pane icons. */

#define CHOICE_ICON_SORTAFTERSO 0
#define CHOICE_ICON_TERRITORYSO 1
#define CHOICE_ICON_WEEKENDFRAME 2
#define CHOICE_ICON_WEEKENDLABEL 3
#define CHOICE_ICON_SOSUN 4
#define CHOICE_ICON_SOMON 5
#define CHOICE_ICON_SOTUE 6
#define CHOICE_ICON_SOWED 7
#define CHOICE_ICON_SOTHU 8
#define CHOICE_ICON_SOFRI 9
#define CHOICE_ICON_SOSAT 10
#define CHOICE_ICON_AUTOSORTSO 11

/* Print pane icons. */

#define CHOICE_ICON_STANDARD 0
#define CHOICE_ICON_PORTRAIT 1
#define CHOICE_ICON_LANDSCAPE 2
#define CHOICE_ICON_SCALE 3
#define CHOICE_ICON_MTOP 7
#define CHOICE_ICON_MLEFT 8
#define CHOICE_ICON_MRIGHT 11
#define CHOICE_ICON_MBOTTOM 13
#define CHOICE_ICON_MINCH 14
#define CHOICE_ICON_MCM 15
#define CHOICE_ICON_MMM 16
#define CHOICE_ICON_FASTTEXT 17
#define CHOICE_ICON_TEXTFORMAT 18
#define CHOICE_ICON_PNUM 19
#define CHOICE_ICON_GUTTER 21

/* Report pane icons. */

#define CHOICE_ICON_REPORT_SHOWPAGE 0
#define CHOICE_ICON_NFONT 4
#define CHOICE_ICON_NFONTMENU 5
#define CHOICE_ICON_BFONT 7
#define CHOICE_ICON_BFONTMENU 8
#define CHOICE_ICON_IFONT 10
#define CHOICE_ICON_IFONTMENU 11
#define CHOICE_ICON_BIFONT 13
#define CHOICE_ICON_BIFONTMENU 14
#define CHOICE_ICON_FONTSIZE 16
#define CHOICE_ICON_FONTSPACE 19
#define CHOICE_ICON_REPORT_PORTRAIT 23
#define CHOICE_ICON_REPORT_LANDSCAPE 24
#define CHOICE_ICON_REPORT_SCALE 25
#define CHOICE_ICON_REPORT_TITLE 28
#define CHOICE_ICON_REPORT_GRID 29
#define CHOICE_ICON_REPORT_PAGENUM 30

/* Transaction pane icons. */

#define CHOICE_ICON_AUTOSORT 0
#define CHOICE_ICON_HIGHLIGHT 3
#define CHOICE_ICON_HILIGHTCOL 6
#define CHOICE_ICON_HILIGHTMEN 4
#define CHOICE_ICON_TRANSDEL 8
#define CHOICE_ICON_AUTOCOMP 11
#define CHOICE_ICON_AUTOSORTPRE 9

/* Account pane icons. */

#define CHOICE_ICON_AHIGHLIGHT 2
#define CHOICE_ICON_AHILIGHTCOL 5
#define CHOICE_ICON_AHILIGHTMEN 3
#define CHOICE_ICON_SHIGHLIGHT 9
#define CHOICE_ICON_SHILIGHTCOL 13
#define CHOICE_ICON_SHILIGHTMEN 11
#define CHOICE_ICON_OHIGHLIGHT 14
#define CHOICE_ICON_OHILIGHTCOL 18
#define CHOICE_ICON_OHILIGHTMEN 16

/* unit conversion constants. */

#define UNIT_INCH_TO_MILLIPOINT 72000
#define UNIT_MM_TO_MILLIPOINT 2834
#define UNIT_CM_TO_MILLIPOINT 28346

#define MARGIN_UNIT_MM 0
#define MARGIN_UNIT_CM 1
#define MARGIN_UNIT_INCH 2

#define CHOICES_PANE_TOOLBAR_WIDTH 44


static int		choices_pane = 0;					/**< The choices pane that is currently displayed.	*/

static wimp_w		choices_window;						/**< The choices window handle.				*/
static wimp_w		choices_panes[CHOICES_PANES];				/**< The choices pane handles.				*/
static wimp_w		choices_colpick_window;					/**< The colour pick window.				*/

static wimp_menu	*choices_font_menu = NULL;				/**< The font menu handle.				*/
static wimp_i		choices_font_icon = -1;					/**< The pop-up icon which opened the font menu.	*/

/* Function Prototypes */

static void		choices_close_window(void);
static void		choices_click_handler(wimp_pointer *pointer);
static osbool		choices_keypress_handler(wimp_key *key);
static void		choices_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void		choices_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void		choices_menu_close_handler(wimp_w w, wimp_menu *menu);
static void		choices_change_pane(int pane);
static void		choices_set_window(void);
static void		choices_read_window(void);
static void		choices_redraw_window(void);
static void		choices_colpick_click_handler(wimp_pointer *pointer);


/**
 * Initialise the Choices module.
 */

void choices_initialise(void)
{
	wimp_menu	*date_menu;

	choices_colpick_window = templates_create_window("Colours");
	ihelp_add_window(choices_colpick_window, "Colours", NULL);
	event_add_window_mouse_event(choices_colpick_window, choices_colpick_click_handler);

	choices_window = templates_create_window("Choices");
	ihelp_add_window(choices_window, "Choices", NULL);
	event_add_window_mouse_event(choices_window, choices_click_handler);
	event_add_window_key_event(choices_window, choices_keypress_handler);

	choices_panes[CHOICE_PANE_GENERAL] = templates_create_window("Choices0");
	ihelp_add_window(choices_panes[CHOICE_PANE_GENERAL], "Choices0", NULL);
	event_add_window_mouse_event(choices_panes[CHOICE_PANE_GENERAL], choices_click_handler);
	event_add_window_key_event(choices_panes[CHOICE_PANE_GENERAL], choices_keypress_handler);

	date_menu = templates_get_menu("ChoicesDateMenu");
	ihelp_add_menu(date_menu, "ChDateMenu");
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEFORMAT_POP, date_menu, CHOICE_ICON_DATEFORMAT, NULL);

	choices_panes[CHOICE_PANE_CURRENCY] = templates_create_window("Choices1");
	ihelp_add_window(choices_panes[CHOICE_PANE_CURRENCY], "Choices1", NULL);
	event_add_window_mouse_event(choices_panes[CHOICE_PANE_CURRENCY], choices_click_handler);
	event_add_window_key_event(choices_panes[CHOICE_PANE_CURRENCY], choices_keypress_handler);

	choices_panes[CHOICE_PANE_SORDER] = templates_create_window("Choices2");
	ihelp_add_window(choices_panes[CHOICE_PANE_SORDER], "Choices2", NULL);
	event_add_window_mouse_event(choices_panes[CHOICE_PANE_SORDER], choices_click_handler);
	event_add_window_key_event(choices_panes[CHOICE_PANE_SORDER], choices_keypress_handler);

	choices_panes[CHOICE_PANE_PRINT] = templates_create_window("Choices3");
	ihelp_add_window(choices_panes[CHOICE_PANE_PRINT], "Choices3", NULL);
	event_add_window_mouse_event(choices_panes[CHOICE_PANE_PRINT], choices_click_handler);
	event_add_window_key_event(choices_panes[CHOICE_PANE_PRINT], choices_keypress_handler);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_STANDARD, TRUE);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_FASTTEXT, TRUE);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_PORTRAIT, TRUE);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_LANDSCAPE, TRUE);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MINCH, TRUE);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MCM, TRUE);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MMM, TRUE);

	choices_panes[CHOICE_PANE_TRANSACT] = templates_create_window("Choices4");
	ihelp_add_window(choices_panes[CHOICE_PANE_TRANSACT], "Choices4", NULL);
	event_add_window_mouse_event(choices_panes[CHOICE_PANE_TRANSACT], choices_click_handler);
	event_add_window_key_event(choices_panes[CHOICE_PANE_TRANSACT], choices_keypress_handler);
	event_add_window_menu_prepare(choices_panes[CHOICE_PANE_TRANSACT], choices_menu_prepare_handler);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTMEN, (wimp_menu *) choices_colpick_window, -1, NULL);

	choices_panes[CHOICE_PANE_REPORT] = templates_create_window("Choices5");
	ihelp_add_window(choices_panes[CHOICE_PANE_REPORT], "Choices5", NULL);
	event_add_window_mouse_event(choices_panes[CHOICE_PANE_REPORT], choices_click_handler);
	event_add_window_key_event(choices_panes[CHOICE_PANE_REPORT], choices_keypress_handler);
	event_add_window_menu_prepare(choices_panes[CHOICE_PANE_REPORT], choices_menu_prepare_handler);
	event_add_window_menu_selection(choices_panes[CHOICE_PANE_REPORT], choices_menu_selection_handler);
	event_add_window_menu_close(choices_panes[CHOICE_PANE_REPORT], choices_menu_close_handler);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_NFONTMENU, choices_font_menu, -1, NULL);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BFONTMENU, choices_font_menu, -1, NULL);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_IFONTMENU, choices_font_menu, -1, NULL);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BIFONTMENU, choices_font_menu, -1, NULL);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_LANDSCAPE, TRUE);
	event_add_window_icon_radio(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_PORTRAIT, TRUE);

	choices_panes[CHOICE_PANE_ACCOUNT] = templates_create_window("Choices6");
	ihelp_add_window(choices_panes[CHOICE_PANE_ACCOUNT], "Choices6", NULL);
	event_add_window_mouse_event(choices_panes[CHOICE_PANE_ACCOUNT], choices_click_handler);
	event_add_window_key_event(choices_panes[CHOICE_PANE_ACCOUNT], choices_keypress_handler);
	event_add_window_menu_prepare(choices_panes[CHOICE_PANE_ACCOUNT], choices_menu_prepare_handler);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTMEN, (wimp_menu *) choices_colpick_window, -1, NULL);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTMEN, (wimp_menu *) choices_colpick_window, -1, NULL);
	event_add_window_icon_popup(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHILIGHTMEN, (wimp_menu *) choices_colpick_window, -1, NULL);
}


/**
 * Open the Choices window at the mouse pointer.
 *
 * \param *pointer		The details of the pointer to open the window at.
 */

void choices_open_window(wimp_pointer *pointer)
{
	int	i;

	choices_pane = CHOICE_PANE_GENERAL;

	for (i=0; i<CHOICES_PANES; i++)
		icons_set_selected(choices_window, CHOICE_ICON_SELECT + i, i == choices_pane);

	choices_set_window();

	windows_open_with_pane_centred_at_pointer(choices_window, choices_panes[choices_pane],
			CHOICE_ICON_PANE, CHOICES_PANE_TOOLBAR_WIDTH, pointer);

	place_dialogue_caret_fallback(choices_panes[choices_pane], 2,
			CHOICE_ICON_DATEIN, CHOICE_ICON_DATEOUT);
}


/**
 * Close the Choices window.
 */

static void choices_close_window(void)
{
	close_dialogue_with_caret(choices_panes[choices_pane]);
	close_dialogue_with_caret(choices_window);
}


/**
 * Process mouse clicks in the Choices dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void choices_click_handler(wimp_pointer *pointer)
{
	if (pointer->w == choices_window) {
		switch (pointer->i) {
		case CHOICE_ICON_APPLY:
			choices_read_window();
			if (pointer->buttons == wimp_CLICK_SELECT)
				choices_close_window();
			break;

		case CHOICE_ICON_SAVE:
			choices_read_window();
			config_save();
			if (pointer->buttons == wimp_CLICK_SELECT)
				choices_close_window();
			break;

		case CHOICE_ICON_CANCEL:
			if (pointer->buttons == wimp_CLICK_SELECT) {
				/* Close window. */
				choices_close_window();
			} else if (pointer->buttons == wimp_CLICK_ADJUST) {
				/* Reset window contents. */
				choices_set_window();
				choices_redraw_window();
				icons_replace_caret_in_window(choices_panes[CHOICE_PANE_CURRENCY]);
			}
			break;

		case CHOICE_ICON_DEFAULT:
			if (pointer->buttons == wimp_CLICK_SELECT) {
				config_restore_default();
				choices_redraw_window();
			}
			break;

		default:
			if (pointer->i >= CHOICE_ICON_SELECT && pointer->i < (CHOICE_ICON_SELECT + CHOICES_PANES)) {
				choices_change_pane(pointer->i - CHOICE_ICON_SELECT);
				icons_set_selected(choices_window, pointer->i, 1);
			}
			break;
		}
	} else if (pointer->w == choices_panes[CHOICE_PANE_CURRENCY]) {
		if (pointer->i == CHOICE_ICON_TERRITORYNUM) {
			icons_set_group_shaded_when_on(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM, 10,
					CHOICE_ICON_FORMATFRAME, CHOICE_ICON_FORMATLABEL,
					CHOICE_ICON_DECIMALPLACELABEL, CHOICE_ICON_DECIMALPLACE,
					CHOICE_ICON_DECIMALPOINTLABEL, CHOICE_ICON_DECIMALPOINT,
					CHOICE_ICON_NEGFRAME, CHOICE_ICON_NEGLABEL,
					CHOICE_ICON_NEGMINUS, CHOICE_ICON_NEGBRACE);
			icons_replace_caret_in_window(choices_panes[CHOICE_PANE_CURRENCY]);
		}
	} else if (pointer->w == choices_panes[CHOICE_PANE_SORDER]) {
		if (pointer->i == CHOICE_ICON_TERRITORYSO) {
			icons_set_group_shaded_when_on(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO, 9,
					CHOICE_ICON_WEEKENDFRAME, CHOICE_ICON_WEEKENDLABEL,
					CHOICE_ICON_SOSUN, CHOICE_ICON_SOMON, CHOICE_ICON_SOTUE, CHOICE_ICON_SOWED,
					CHOICE_ICON_SOTHU, CHOICE_ICON_SOFRI, CHOICE_ICON_SOSAT);
		}
	}
}


/**
 * Process keypresses in the Choices window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool choices_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		choices_read_window();
		choices_close_window();
		break;

	case wimp_KEY_ESCAPE:
		choices_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process menu prepare events in the Choices window.  These include popups of
 * the Colour Pick dialogue.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void choices_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	wimp_i		icon = wimp_WINDOW_BACK;

	if (menu == (wimp_menu *) choices_colpick_window) {
		if (w == choices_panes[CHOICE_PANE_TRANSACT] && pointer->i == CHOICE_ICON_HILIGHTMEN) {
			icon = CHOICE_ICON_HILIGHTCOL;
		} else if (w == choices_panes[CHOICE_PANE_ACCOUNT]) {
			switch (pointer->i) {
			case CHOICE_ICON_AHILIGHTMEN:
				icon = CHOICE_ICON_AHILIGHTCOL;
				break;

			case CHOICE_ICON_SHILIGHTMEN:
				icon = CHOICE_ICON_SHILIGHTCOL;
				break;

			case CHOICE_ICON_OHILIGHTMEN:
				icon = CHOICE_ICON_OHILIGHTCOL;
				break;
			}
		}

		colpick_open_window(w, icon);
	} else {
		choices_font_menu = fontlist_build();
		choices_font_icon = pointer->i;
		event_set_menu_block(choices_font_menu);
		ihelp_add_menu(choices_font_menu, "FontMenu");
	}
}


/**
 * Process menu selection events in the Choices window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void choices_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	char	*font;

	font = fontlist_decode(selection);
	if (font == NULL)
		return;

	switch (choices_font_icon) {
	case CHOICE_ICON_NFONTMENU:
		icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT, "%s", font);
		wimp_set_icon_state(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT, 0, 0);
		break;

	case CHOICE_ICON_BFONTMENU:
		icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT, "%s", font);
		wimp_set_icon_state(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT, 0, 0);
		break;

	case CHOICE_ICON_IFONTMENU:
		icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_IFONT, "%s", font);
		wimp_set_icon_state(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_IFONT, 0, 0);
		break;

	case CHOICE_ICON_BIFONTMENU:
		icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BIFONT, "%s", font);
		wimp_set_icon_state(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BIFONT, 0, 0);
		break;
	}

	heap_free(font);
}


/**
 * Process menu close events in the Choices window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void choices_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	fontlist_destroy();
	choices_font_menu = NULL;
	choices_font_icon = -1;
	ihelp_remove_menu(choices_font_menu);
}


/**
 * Switch the visible pane in the Choices dialogue.
 *
 * \param pane		The new pane to select.
 */

static void choices_change_pane(int pane)
{
	int		i, old_pane;
	wimp_caret	caret;

	if (pane >= CHOICES_PANES || !windows_get_open(choices_window) || pane == choices_pane)
		return;

	wimp_get_caret_position(&caret);

	old_pane = choices_pane;
	choices_pane = pane;

	for (i=0; i<CHOICES_PANES; i++)
		icons_set_selected (choices_window, CHOICE_ICON_SELECT + i, i == choices_pane);

	windows_open_pane_centred_in_icon(choices_window, choices_panes[pane], CHOICE_ICON_PANE,
			CHOICES_PANE_TOOLBAR_WIDTH, choices_panes[old_pane]);

	wimp_close_window(choices_panes[old_pane]);

	if (caret.w == choices_panes[old_pane]) {
		switch (pane) {
		case CHOICE_PANE_GENERAL:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_GENERAL], 2,
					CHOICE_ICON_DATEIN, CHOICE_ICON_DATEOUT);
			break;

		case CHOICE_PANE_CURRENCY:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_CURRENCY], 2,
					CHOICE_ICON_DECIMALPLACE, CHOICE_ICON_DECIMALPOINT);
			break;

		case CHOICE_PANE_SORDER:
			place_dialogue_caret(choices_panes[CHOICE_PANE_SORDER], wimp_ICON_WINDOW);
			break;

		case CHOICE_PANE_REPORT:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_REPORT], 2,
					CHOICE_ICON_FONTSIZE, CHOICE_ICON_FONTSPACE);
			break;

		case CHOICE_PANE_PRINT:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_PRINT], 4,
					CHOICE_ICON_MTOP, CHOICE_ICON_MLEFT,
					CHOICE_ICON_MRIGHT, CHOICE_ICON_MBOTTOM);
			break;

		case CHOICE_PANE_TRANSACT:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_TRANSACT], 1,
					CHOICE_ICON_AUTOCOMP);
			break;

		case CHOICE_PANE_ACCOUNT:
			place_dialogue_caret(choices_panes[CHOICE_PANE_ACCOUNT], wimp_ICON_WINDOW);
			break;
		}
	}
}


/**
 * Set the contents of the Choices window to reflect the current settings.
 */

static void choices_set_window(void)
{
	int		day;
	enum date_days	ignore;

	/* Set the general pane up. */

	icons_set_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_CLIPBOARD,
			config_opt_read("GlobalClipboardSupport"));
	icons_set_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_RO5KEYS,
			config_opt_read("IyonixKeys"));
	icons_set_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_TDRAG,
			config_opt_read("TransDragDrop"));
	icons_set_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_REMEMBERDIALOGUE,
			config_opt_read("RememberValues"));
	icons_set_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_TERRITORYDATE,
			config_opt_read("TerritoryDates"));

	icons_printf(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEIN, "%s", config_str_read("DateSepIn"));
	icons_printf(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEOUT, "%s", config_str_read("DateSepOut"));

	event_set_window_icon_popup_selection(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEFORMAT_POP, config_int_read("DateFormat"));


	/* Set the currency pane up. */

	icons_set_selected(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_SHOWZERO,
			config_opt_read("PrintZeros"));
	icons_set_selected(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM,
			config_opt_read("TerritoryCurrency"));
	icons_set_selected(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGMINUS,
			!config_opt_read("BracketNegatives"));
	icons_set_selected(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGBRACE,
			config_opt_read("BracketNegatives"));

	icons_printf(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPLACE, "%d", config_int_read("DecimalPlaces"));
	icons_printf(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPOINT, "%s", config_str_read("DecimalPoint"));

	icons_set_group_shaded_when_on(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM, 10,
			CHOICE_ICON_FORMATFRAME, CHOICE_ICON_FORMATLABEL,
			CHOICE_ICON_DECIMALPLACELABEL, CHOICE_ICON_DECIMALPLACE,
			CHOICE_ICON_DECIMALPOINTLABEL, CHOICE_ICON_DECIMALPOINT,
			CHOICE_ICON_NEGFRAME, CHOICE_ICON_NEGLABEL,
			CHOICE_ICON_NEGMINUS, CHOICE_ICON_NEGBRACE);

	/* Set the standing order pane up. */

	icons_set_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_SORTAFTERSO,
			config_opt_read("SortAfterSOrders"));
	icons_set_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_AUTOSORTSO,
			config_opt_read("AutoSortSOrders"));
	icons_set_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO,
			config_opt_read("TerritorySOrders"));

	ignore = (enum date_days) config_int_read("WeekendDays");

	for (day = 0; day < 7; day++)
		icons_set_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_SOSUN + day,
				(ignore & date_convert_day_to_days(day + 1)) ? TRUE : FALSE);

	icons_set_group_shaded_when_on(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO, 9,
			CHOICE_ICON_WEEKENDFRAME, CHOICE_ICON_WEEKENDLABEL,
			CHOICE_ICON_SOSUN, CHOICE_ICON_SOMON, CHOICE_ICON_SOTUE, CHOICE_ICON_SOWED,
			CHOICE_ICON_SOTHU, CHOICE_ICON_SOFRI, CHOICE_ICON_SOSAT);

	/* Set the printing pane up. */

	icons_set_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_STANDARD,
			!config_opt_read("PrintText"));
	icons_set_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_PORTRAIT,
			!config_opt_read("PrintRotate"));
	icons_set_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_LANDSCAPE,
			config_opt_read("PrintRotate"));
	icons_set_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_SCALE,
			config_opt_read("PrintFitWidth"));
	icons_set_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_PNUM,
			config_opt_read("PrintPageNumbers"));

	icons_set_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_FASTTEXT,
			config_opt_read("PrintText"));
	icons_set_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_TEXTFORMAT,
			config_opt_read("PrintTextFormat"));

	icons_set_radio_group_selected(choices_panes[CHOICE_PANE_PRINT], config_int_read("PrintMarginUnits"), 3,
			CHOICE_ICON_MMM, CHOICE_ICON_MCM, CHOICE_ICON_MINCH);

	switch (config_int_read("PrintMarginUnits")) {
	case MARGIN_UNIT_MM:
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP, "%.2f",
				(float) config_int_read("PrintMarginTop") / UNIT_MM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT, "%.2f",
				(float) config_int_read("PrintMarginLeft") / UNIT_MM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT, "%.2f",
				(float) config_int_read("PrintMarginRight") / UNIT_MM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM, "%.2f",
				(float) config_int_read("PrintMarginBottom") / UNIT_MM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_GUTTER, "%.2f",
				(float) config_int_read("PrintMarginInternal") / UNIT_MM_TO_MILLIPOINT);
		break;

	case MARGIN_UNIT_CM:
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP, "%.2f",
				(float) config_int_read("PrintMarginTop") / UNIT_CM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT, "%.2f",
				(float) config_int_read("PrintMarginLeft") / UNIT_CM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT, "%.2f",
				(float) config_int_read("PrintMarginRight") / UNIT_CM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM, "%.2f",
				(float) config_int_read("PrintMarginBottom") / UNIT_CM_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_GUTTER, "%.2f",
				(float) config_int_read("PrintMarginInternal") / UNIT_CM_TO_MILLIPOINT);
		break;

	case MARGIN_UNIT_INCH:
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP, "%.2f",
				(float) config_int_read("PrintMarginTop") / UNIT_INCH_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT, "%.2f",
				(float) config_int_read("PrintMarginLeft") / UNIT_INCH_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT, "%.2f",
				(float) config_int_read("PrintMarginRight") / UNIT_INCH_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM, "%.2f",
				(float) config_int_read("PrintMarginBottom") / UNIT_INCH_TO_MILLIPOINT);
		icons_printf(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_GUTTER, "%.2f",
				(float) config_int_read("PrintMarginInternal") / UNIT_INCH_TO_MILLIPOINT);
		break;
	}

	/* Set the report pane up. */

	icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT, "%s",
			config_str_read("ReportFontNormal"));
	icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT, "%s",
			config_str_read("ReportFontBold"));
	icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_IFONT, "%s",
			config_str_read("ReportFontItalic"));
	icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BIFONT, "%s",
			config_str_read("ReportFontBoldItalic"));

	icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSIZE, "%d",
			config_int_read("ReportFontSize"));
	icons_printf(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSPACE, "%d",
			config_int_read("ReportFontLinespace"));

	icons_set_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_LANDSCAPE,
			config_opt_read("ReportRotate"));
	icons_set_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_PORTRAIT,
			!config_opt_read("ReportRotate"));
	icons_set_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_SCALE,
			config_opt_read("ReportFitWidth"));
	icons_set_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_TITLE,
			config_opt_read("ReportShowTitle"));
	icons_set_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_PAGENUM,
			config_opt_read("ReportShowPageNum"));
	icons_set_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_GRID,
			config_opt_read("ReportShowGrid"));
	icons_set_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_SHOWPAGE,
			config_opt_read("ReportShowPages"));


	/* Set the transaction pane up. */

	icons_set_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORT,
			config_opt_read("AutoSort"));
	icons_set_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_TRANSDEL,
			config_opt_read("AllowTransDelete"));
	icons_set_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_HIGHLIGHT,
			config_opt_read("ShadeReconciled"));
	colpick_set_icon_colour(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTCOL,
			config_int_read("ShadeReconciledColour"));
	icons_printf(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOCOMP, "%d",
			config_int_read("MaxAutofillLen"));
	icons_set_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORTPRE,
			config_opt_read("AutoSortPresets"));

	/* Set the account pane up. */

	icons_set_selected(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHIGHLIGHT,
			config_opt_read("ShadeAccounts"));
	colpick_set_icon_colour(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTCOL,
			config_int_read("ShadeAccountsColour"));
	icons_set_selected(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHIGHLIGHT,
			config_opt_read("ShadeBudgeted"));
	colpick_set_icon_colour(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTCOL,
			config_int_read("ShadeBudgetedColour"));
	icons_set_selected(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHIGHLIGHT,
			config_opt_read("ShadeOverdrawn"));
	colpick_set_icon_colour(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHILIGHTCOL,
			config_int_read("ShadeOverdrawnColour"));
}

/**
 * Read the contents of the Choices window into the settings.
 */

static void choices_read_window(void)
{
	int		day;
	enum date_days	ignore;
	float		top, left, right, bottom, internal;

	/* Read the general pane. */

	config_opt_set("GlobalClipboardSupport",
			icons_get_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_CLIPBOARD));
	config_opt_set("IyonixKeys",
			icons_get_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_RO5KEYS));
	config_opt_set("TransDragDrop",
			icons_get_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_TDRAG));
	config_opt_set("RememberValues",
			icons_get_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_REMEMBERDIALOGUE));
	config_opt_set("TerritoryDates",
			icons_get_selected(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_TERRITORYDATE));

	config_str_set("DateSepIn",
			icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEIN));
	config_str_set("DateSepOut",
			icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEOUT));

	config_int_set("DateFormat",
		event_get_window_icon_popup_selection(choices_panes[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEFORMAT_POP));


	/* Read the currency pane. */

	config_opt_set("PrintZeros",
			icons_get_selected(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_SHOWZERO));
	config_opt_set("TerritoryCurrency",
			icons_get_selected(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM));
	config_opt_set("BracketNegatives",
			icons_get_selected(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGBRACE));
	config_int_set("DecimalPlaces",
			atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPLACE)));
	config_str_set("DecimalPoint",
			icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPOINT));

	/* Read the standing order pane.*/

	config_opt_set("SortAfterSOrders",
			icons_get_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_SORTAFTERSO));
	config_opt_set("AutoSortSOrders",
			icons_get_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_AUTOSORTSO));
	config_opt_set("TerritorySOrders",
			icons_get_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO));

	ignore = DATE_DAY_NONE;

	for (day = 0; day < 7; day++)
		if (icons_get_selected(choices_panes[CHOICE_PANE_SORDER], CHOICE_ICON_SOSUN + day))
			ignore |= date_convert_day_to_days(day + 1);

	config_int_set("WeekendDays", (int) ignore);

	/* Read the printing pane. */

	config_opt_set("PrintFitWidth",
			icons_get_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_SCALE));
	config_opt_set("PrintRotate",
			icons_get_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_LANDSCAPE));
	config_opt_set("PrintPageNumbers",
			icons_get_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_PNUM));
	config_opt_set("PrintText",
			icons_get_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_FASTTEXT));
	config_opt_set("PrintTextFormat",
			icons_get_selected(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_TEXTFORMAT));

	config_int_set("PrintMarginUnits", icons_get_radio_group_selected(choices_panes[CHOICE_PANE_PRINT], 3,
			CHOICE_ICON_MMM, CHOICE_ICON_MCM, CHOICE_ICON_MINCH));

	sscanf(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%f", &top);
	sscanf(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%f", &left);
	sscanf(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%f", &right);
	sscanf(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%f", &bottom);
	sscanf(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_PRINT], CHOICE_ICON_GUTTER), "%f", &internal);

	switch (config_int_read("PrintMarginUnits")) {
	case MARGIN_UNIT_MM:
		config_int_set("PrintMarginTop", (int) (top * UNIT_MM_TO_MILLIPOINT));
		config_int_set("PrintMarginLeft", (int) (left * UNIT_MM_TO_MILLIPOINT));
		config_int_set("PrintMarginRight", (int) (right * UNIT_MM_TO_MILLIPOINT));
		config_int_set("PrintMarginBottom", (int) (bottom * UNIT_MM_TO_MILLIPOINT));
		config_int_set("PrintMarginInternal", (int) (internal * UNIT_MM_TO_MILLIPOINT));
		break;

	case MARGIN_UNIT_CM:
		config_int_set("PrintMarginTop", (int) (top * UNIT_CM_TO_MILLIPOINT));
		config_int_set("PrintMarginLeft", (int) (left * UNIT_CM_TO_MILLIPOINT));
		config_int_set("PrintMarginRight", (int) (right * UNIT_CM_TO_MILLIPOINT));
		config_int_set("PrintMarginBottom", (int) (bottom * UNIT_CM_TO_MILLIPOINT));
		config_int_set("PrintMarginInternal", (int) (internal * UNIT_CM_TO_MILLIPOINT));
		break;

	case MARGIN_UNIT_INCH:
		config_int_set("PrintMarginTop", (int) (top * UNIT_INCH_TO_MILLIPOINT));
		config_int_set("PrintMarginLeft", (int) (left * UNIT_INCH_TO_MILLIPOINT));
		config_int_set("PrintMarginRight", (int) (right * UNIT_INCH_TO_MILLIPOINT));
		config_int_set("PrintMarginBottom", (int) (bottom * UNIT_INCH_TO_MILLIPOINT));
		config_int_set("PrintMarginInternal", (int) (internal * UNIT_INCH_TO_MILLIPOINT));
		break;
	}

	/* Read the report pane. */

	config_str_set("ReportFontNormal",
			icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT));
	config_str_set("ReportFontBold",
			icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT));
	config_str_set("ReportFontItalic",
			icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_IFONT));
	config_str_set("ReportFontBoldItalic",
			icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_BIFONT));
	config_int_set("ReportFontSize",
			atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSIZE)));
	config_int_set("ReportFontLinespace",
                  atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSPACE)));

	config_opt_set("ReportRotate",
			icons_get_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_LANDSCAPE));
	config_opt_set("ReportFitWidth",
			icons_get_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_SCALE));
	config_opt_set("ReportShowTitle",
			icons_get_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_TITLE));
	config_opt_set("ReportShowPageNum",
			icons_get_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_PAGENUM));
	config_opt_set("ReportShowGrid",
			icons_get_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_GRID));
	config_opt_set("ReportShowPages",
			icons_get_selected(choices_panes[CHOICE_PANE_REPORT], CHOICE_ICON_REPORT_SHOWPAGE));

	/* Read the transaction pane. */

	config_opt_set("AutoSort",
			icons_get_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORT));
	config_opt_set("AllowTransDelete",
			icons_get_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_TRANSDEL));
	config_opt_set("ShadeReconciled",
			icons_get_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_HIGHLIGHT));
	config_int_set("ShadeReconciledColour",
			atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTCOL)));
	config_int_set("MaxAutofillLen",
			atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOCOMP)));
	config_opt_set("AutoSortPresets",
			icons_get_selected(choices_panes[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORTPRE));

	/* Read the account pane. */

	config_opt_set("ShadeAccounts",
			icons_get_selected(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHIGHLIGHT));
	config_int_set("ShadeAccountsColour",
			atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTCOL)));
	config_opt_set("ShadeBudgeted",
			icons_get_selected(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHIGHLIGHT));
	config_int_set("ShadeBudgetedColour",
			atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTCOL)));
	config_opt_set("ShadeOverdrawn",
			icons_get_selected(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHIGHLIGHT));
	config_int_set("ShadeOverdrawnColour",
			atoi(icons_get_indirected_text_addr(choices_panes[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHILIGHTCOL)));

	/* Update stored data. */

	date_initialise();
	currency_initialise();

	/* Redraw windows as required. */

	file_process_all(file_redraw_windows);
}


/**
 * Force an update of all the icons in the Choices Window, and replace the
 * caret if required.
 */

static void choices_redraw_window(void)
{
	switch (choices_pane) {
	case CHOICE_PANE_GENERAL:
		icons_redraw_group(choices_panes[CHOICE_PANE_GENERAL], 2,
				CHOICE_ICON_DATEIN, CHOICE_ICON_DATEOUT);
	break;

	case CHOICE_PANE_CURRENCY:
		icons_redraw_group(choices_panes[CHOICE_PANE_CURRENCY], 2,
				CHOICE_ICON_DECIMALPLACE, CHOICE_ICON_DECIMALPOINT);
		break;

	case CHOICE_PANE_SORDER:
		/* No icons to refresh manually. */
		break;

	case CHOICE_PANE_PRINT:
		icons_redraw_group(choices_panes[CHOICE_PANE_PRINT], 4,
				CHOICE_ICON_MTOP, CHOICE_ICON_MLEFT, CHOICE_ICON_MRIGHT, CHOICE_ICON_MBOTTOM);
		break;

	case CHOICE_PANE_REPORT:
		icons_redraw_group(choices_panes[CHOICE_PANE_REPORT], 4,
				CHOICE_ICON_NFONT, CHOICE_ICON_BFONT, CHOICE_ICON_FONTSIZE, CHOICE_ICON_FONTSPACE);
		break;

	case CHOICE_PANE_TRANSACT:
		icons_redraw_group(choices_panes[CHOICE_PANE_TRANSACT], 1,
				CHOICE_ICON_HILIGHTCOL);
		break;

	case CHOICE_PANE_ACCOUNT:
		icons_redraw_group(choices_panes[CHOICE_PANE_ACCOUNT], 3,
				CHOICE_ICON_AHILIGHTCOL, CHOICE_ICON_SHILIGHTCOL, CHOICE_ICON_OHILIGHTCOL);
		break;
	}

	icons_replace_caret_in_window(choices_panes[choices_pane]);
}


/**
 * Process mouse clicks in the Colour Pick dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void choices_colpick_click_handler(wimp_pointer *pointer)
{
	if (pointer->i >= 1) {
		colpick_select_colour(pointer->i - 1);

		if (pointer->buttons == wimp_CLICK_SELECT)
			wimp_create_menu(NULL, 0, 0);
	}
}

