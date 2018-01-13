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
 * \file: report_font_dialogue.c
 *
 * High-level report font dialogue implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/font.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "report_font_dialogue.h"

#include "caret.h"
#include "fontlist.h"
#include "report.h"

/* Dialogue Icons. */

#define REPORT_FONT_DIALOGUE_OK 19
#define REPORT_FONT_DIALOGUE_CANCEL 18
#define REPORT_FONT_DIALOGUE_NFONT 1
#define REPORT_FONT_DIALOGUE_NFONTMENU 2
#define REPORT_FONT_DIALOGUE_BFONT 4
#define REPORT_FONT_DIALOGUE_BFONTMENU 5
#define REPORT_FONT_DIALOGUE_IFONT 7
#define REPORT_FONT_DIALOGUE_IFONTMENU 8
#define REPORT_FONT_DIALOGUE_BIFONT 10
#define REPORT_FONT_DIALOGUE_BIFONTMENU 11
#define REPORT_FONT_DIALOGUE_FONTSIZE 13
#define REPORT_FONT_DIALOGUE_FONTSPACE 16

/* Global variables. */


/**
 * The handle of the Report Format dialogue.
 */

static wimp_w			report_font_dialogue_window = NULL;

/**
 * The handle of the Font menu.
 */

static wimp_menu		*report_font_dialogue_font_menu = NULL;

/**
 * The pop-up icon which opened the font menu.
 */

static wimp_i			report_font_dialogue_font_icon = -1;

/**
 * The starting normal font name.
 */

static char			report_font_dialogue_initial_normal[font_NAME_LIMIT];

/**
 * The starting bold font name.
 */

static char			report_font_dialogue_initial_bold[font_NAME_LIMIT];

/**
 * The starting italic font name.
 */

static char			report_font_dialogue_initial_italic[font_NAME_LIMIT];

/**
 * The starting bold italic font name.
 */

static char			report_font_dialogue_initial_bold_italic[font_NAME_LIMIT];

/**
 * The staring font size.
 */

static int			report_font_dialogue_initial_size;

/**
 * The starting line spacing.
 */

static int			report_font_dialogue_initial_spacing;

/**
 * Callback function to return updated settings.
 */

static void			(*report_font_dialogue_callback)(struct report *, char *, char *, char *, char *, int, int);

/**
 * The report to which the currently open Report Format window belongs.
 */

struct report			*report_font_dialogue_report = NULL;


/* Static function prototypes. */

static void	report_font_dialogue_click_handler(wimp_pointer *pointer);
static osbool	report_font_dialogue_keypress_handler(wimp_key *key);
static void	report_font_dialogue_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void	report_font_dialogue_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void	report_font_dialogue_menu_close_handler(wimp_w w, wimp_menu *menu);
static void	report_font_dialogue_refresh(void);
static void	report_font_dialogue_fill(void);
static void	report_font_dialogue_process(void);


/**
 * Initialise the report format dialogue.
 */

void report_font_dialogue_initialise(void)
{
	report_font_dialogue_window = templates_create_window("RepFont");
	ihelp_add_window(report_font_dialogue_window, "RepFont", NULL);
	event_add_window_mouse_event(report_font_dialogue_window, report_font_dialogue_click_handler);
	event_add_window_key_event(report_font_dialogue_window, report_font_dialogue_keypress_handler);
	event_add_window_menu_prepare(report_font_dialogue_window, report_font_dialogue_menu_prepare_handler);
	event_add_window_menu_selection(report_font_dialogue_window, report_font_dialogue_menu_selection_handler);
	event_add_window_menu_close(report_font_dialogue_window, report_font_dialogue_menu_close_handler);
	event_add_window_icon_popup(report_font_dialogue_window, REPORT_FONT_DIALOGUE_NFONTMENU, report_font_dialogue_font_menu, -1, NULL);
	event_add_window_icon_popup(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BFONTMENU, report_font_dialogue_font_menu, -1, NULL);
	event_add_window_icon_popup(report_font_dialogue_window, REPORT_FONT_DIALOGUE_IFONTMENU, report_font_dialogue_font_menu, -1, NULL);
	event_add_window_icon_popup(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BIFONTMENU, report_font_dialogue_font_menu, -1, NULL);
}


/**
 * Open the Report Format dialogue for a given report view.
 *
 * \param *ptr			The current Wimp pointer position.
 * \param *report		The report to own the dialogue.
 * \param *callback		The callback function to use to return the results.
 * \param *normal		The initial normal font name.
 * \param *bold			The initial bold font name.
 * \param *italic		The initial italic font name.
 * \param *bold_italic		The initial bold-italic font name.
 * \param size			The initial font size.
 * \param spacing		The initial line spacing.
 */

void report_font_dialogue_open(wimp_pointer *ptr, struct report *report, void (*callback)(struct report *, char *, char *, char *, char *, int, int),
		char *normal, char *bold, char *italic, char *bold_italic, int size, int spacing)
{
	string_copy(report_font_dialogue_initial_normal, normal, font_NAME_LIMIT);
	string_copy(report_font_dialogue_initial_bold, bold, font_NAME_LIMIT);
	string_copy(report_font_dialogue_initial_italic, italic, font_NAME_LIMIT);
	string_copy(report_font_dialogue_initial_bold_italic, bold_italic, font_NAME_LIMIT);

	report_font_dialogue_initial_size = size;
	report_font_dialogue_initial_spacing = spacing;

	report_font_dialogue_callback = callback;
	report_font_dialogue_report = report;

	/* If the window is already open, another report format is being
	 * edited.  Assume the user wants to lose any unsaved data and
	 * just close the window.
	 *
	 * We don't use the close_dialogue_with_caret() as the caret is
	 * just moving from one dialogue to another.
	 */

	if (windows_get_open(report_font_dialogue_window))
		wimp_close_window(report_font_dialogue_window);

	/* Set the window contents up. */

	report_font_dialogue_fill();

	/* Open the window. */

	windows_open_centred_at_pointer(report_font_dialogue_window, ptr);
	place_dialogue_caret(report_font_dialogue_window, REPORT_FONT_DIALOGUE_FONTSIZE);
}


/**
 * Force the closure of the report format dialogue if it relates to a
 * given report instance.
 *
 * \param *report		The report to be closed.
 */

void report_font_dialogue_force_close(struct report *report)
{
	if (report_font_dialogue_report == report && windows_get_open(report_font_dialogue_window))
		close_dialogue_with_caret(report_font_dialogue_window);
}


/**
 * Process mouse clicks in the Report Format dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void report_font_dialogue_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case REPORT_FONT_DIALOGUE_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(report_font_dialogue_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			report_font_dialogue_refresh();
		break;

	case REPORT_FONT_DIALOGUE_OK:
		report_font_dialogue_process();
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(report_font_dialogue_window);
		break;
	}
}


/**
 * Process keypresses in the Report Format window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool report_font_dialogue_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		report_font_dialogue_process();
		close_dialogue_with_caret(report_font_dialogue_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(report_font_dialogue_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process menu prepare events in the Report Format window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void report_font_dialogue_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	report_font_dialogue_font_menu = fontlist_build();
	report_font_dialogue_font_icon = pointer->i;
	event_set_menu_block(report_font_dialogue_font_menu);
	ihelp_add_menu(report_font_dialogue_font_menu, "FontMenu");
}


/**
 * Process menu selection events in the Report Format window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void report_font_dialogue_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	char	*font;

	font = fontlist_decode(selection);
	if (font == NULL)
		return;

	switch (report_font_dialogue_font_icon) {
	case REPORT_FONT_DIALOGUE_NFONTMENU:
		icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_NFONT, "%s", font);
		wimp_set_icon_state(report_font_dialogue_window, REPORT_FONT_DIALOGUE_NFONT, 0, 0);
		break;

	case REPORT_FONT_DIALOGUE_BFONTMENU:
		icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BFONT, "%s", font);
		wimp_set_icon_state(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BFONT, 0, 0);
		break;

	case REPORT_FONT_DIALOGUE_IFONTMENU:
		icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_IFONT, "%s", font);
		wimp_set_icon_state(report_font_dialogue_window, REPORT_FONT_DIALOGUE_IFONT, 0, 0);
		break;

	case REPORT_FONT_DIALOGUE_BIFONTMENU:
		icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BIFONT, "%s", font);
		wimp_set_icon_state(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BIFONT, 0, 0);
		break;
	}

	heap_free(font);
}


/**
 * Process menu close events in the Report Format window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void report_font_dialogue_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	fontlist_destroy();
	report_font_dialogue_font_menu = NULL;
	report_font_dialogue_font_icon = -1;
	ihelp_remove_menu(report_font_dialogue_font_menu);
}


/**
 * Refresh the contents of the Report Format window.
 */

static void report_font_dialogue_refresh(void)
{
	report_font_dialogue_fill();
	icons_redraw_group(report_font_dialogue_window, 4, REPORT_FONT_DIALOGUE_NFONT, REPORT_FONT_DIALOGUE_BFONT,
			REPORT_FONT_DIALOGUE_FONTSIZE, REPORT_FONT_DIALOGUE_FONTSPACE);
	icons_replace_caret_in_window(report_font_dialogue_window);
}


/**
 * Update the contents of the Report Format window to reflect the current
 * settings of the given report.
 */

static void report_font_dialogue_fill(void)
{
	icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_NFONT, "%s", report_font_dialogue_initial_normal);
	icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BFONT, "%s", report_font_dialogue_initial_bold);
	icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_IFONT, "%s", report_font_dialogue_initial_italic);
	icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BIFONT, "%s", report_font_dialogue_initial_bold_italic);

	icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_FONTSIZE, "%d", report_font_dialogue_initial_size / 16);
	icons_printf(report_font_dialogue_window, REPORT_FONT_DIALOGUE_FONTSPACE, "%d", report_font_dialogue_initial_spacing);
}


/**
 * Take the contents of an updated report format window and process the data.
 */

static void report_font_dialogue_process(void)
{

	if (report_font_dialogue_callback == NULL)
		return;

	/* Extract the information. */

	icons_copy_text(report_font_dialogue_window, REPORT_FONT_DIALOGUE_NFONT, report_font_dialogue_initial_normal, font_NAME_LIMIT);
	icons_copy_text(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BFONT, report_font_dialogue_initial_bold, font_NAME_LIMIT);
	icons_copy_text(report_font_dialogue_window, REPORT_FONT_DIALOGUE_IFONT, report_font_dialogue_initial_italic, font_NAME_LIMIT);
	icons_copy_text(report_font_dialogue_window, REPORT_FONT_DIALOGUE_BIFONT, report_font_dialogue_initial_bold_italic, font_NAME_LIMIT);

	report_font_dialogue_initial_size = atoi(icons_get_indirected_text_addr(report_font_dialogue_window, REPORT_FONT_DIALOGUE_FONTSIZE)) * 16;
	report_font_dialogue_initial_spacing = atoi(icons_get_indirected_text_addr(report_font_dialogue_window, REPORT_FONT_DIALOGUE_FONTSPACE));

	/* Call the client back. */

	report_font_dialogue_callback(report_font_dialogue_report,
			report_font_dialogue_initial_normal, report_font_dialogue_initial_bold,
			report_font_dialogue_initial_italic, report_font_dialogue_initial_bold_italic,
			report_font_dialogue_initial_size, report_font_dialogue_initial_spacing);
}

