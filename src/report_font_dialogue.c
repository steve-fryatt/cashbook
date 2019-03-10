/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "dialogue.h"
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
 * The handle of the Report Font dialogue.
 */

static struct dialogue_block	*report_font_dialogue = NULL;

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


/* Static function prototypes. */

static void	report_font_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool	report_font_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void	report_font_dialogue_close(struct file_block *file, wimp_w window, void *data);
static osbool	report_font_dialogue_menu_prepare(struct file_block *file, wimp_w window, wimp_i icon, struct dialogue_menu_data *menu, void *data);
static void	report_font_dialogue_menu_selection(struct file_block *file, wimp_w window, wimp_i icon, wimp_menu *menu, wimp_selection *selection, void *data);
static void	report_font_dialogue_menu_close(struct file_block *file, wimp_w window, wimp_menu *menu, void *data);

/**
 * The Report Font Dialogue Icon Set.
 */

static struct dialogue_icon report_font_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,	REPORT_FONT_DIALOGUE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,	REPORT_FONT_DIALOGUE_CANCEL,		DIALOGUE_NO_ICON},

	/* The Font Name fields. */

	{DIALOGUE_ICON_POPUP,	REPORT_FONT_DIALOGUE_NFONTMENU,		REPORT_FONT_DIALOGUE_NFONT},
	{DIALOGUE_ICON_REFRESH,	REPORT_FONT_DIALOGUE_NFONT,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_POPUP,	REPORT_FONT_DIALOGUE_BFONTMENU,		REPORT_FONT_DIALOGUE_BFONT},
	{DIALOGUE_ICON_REFRESH,	REPORT_FONT_DIALOGUE_BFONT,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_POPUP,	REPORT_FONT_DIALOGUE_IFONTMENU,		REPORT_FONT_DIALOGUE_IFONT},
	{DIALOGUE_ICON_REFRESH,	REPORT_FONT_DIALOGUE_IFONT,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_POPUP,	REPORT_FONT_DIALOGUE_BIFONTMENU,	REPORT_FONT_DIALOGUE_BIFONT},
	{DIALOGUE_ICON_REFRESH,	REPORT_FONT_DIALOGUE_BIFONT,		DIALOGUE_NO_ICON},

	/* The Font Size and Line Space fields. */

	{DIALOGUE_ICON_REFRESH,	REPORT_FONT_DIALOGUE_FONTSIZE,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,	REPORT_FONT_DIALOGUE_FONTSPACE,		DIALOGUE_NO_ICON},

	{DIALOGUE_ICON_END,	DIALOGUE_NO_ICON,			DIALOGUE_NO_ICON}
};

/**
 * The Report Font Dialogue Definition.
 */

static struct dialogue_definition report_font_dialogue_definition = {
	"RepFont",
	"RepFont",
	report_font_dialogue_icon_list,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	report_font_dialogue_fill,
	report_font_dialogue_process,
	report_font_dialogue_close,
	report_font_dialogue_menu_prepare,
	report_font_dialogue_menu_selection,
	report_font_dialogue_menu_close
};


/**
 * Initialise the report format dialogue.
 */

void report_font_dialogue_initialise(void)
{
	report_font_dialogue = dialogue_create(&report_font_dialogue_definition);
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

	/* Open the window. */

	dialogue_open(report_font_dialogue, FALSE, report_get_file(report), report, ptr, NULL);
}


/**
 * Fill the Report Font Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	Unused restore state flag.
 * \param *data		Client data pointer (unused).
 */

static void report_font_dialogue_fill(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	icons_printf(window, REPORT_FONT_DIALOGUE_NFONT, "%s", report_font_dialogue_initial_normal);
	icons_printf(window, REPORT_FONT_DIALOGUE_BFONT, "%s", report_font_dialogue_initial_bold);
	icons_printf(window, REPORT_FONT_DIALOGUE_IFONT, "%s", report_font_dialogue_initial_italic);
	icons_printf(window, REPORT_FONT_DIALOGUE_BIFONT, "%s", report_font_dialogue_initial_bold_italic);

	icons_printf(window, REPORT_FONT_DIALOGUE_FONTSIZE, "%d", report_font_dialogue_initial_size / 16);
	icons_printf(window, REPORT_FONT_DIALOGUE_FONTSPACE, "%d", report_font_dialogue_initial_spacing);
}


/**
 * Process OK clicks in the Report Font Dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The parent report which owns the dialogue.
 * \param *data		Client data pointer (unused).
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool report_font_dialogue_process(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	struct report *report = parent;

	if (report_font_dialogue_callback == NULL || report == NULL)
		return TRUE;

	/* Extract the information. */

	icons_copy_text(window, REPORT_FONT_DIALOGUE_NFONT, report_font_dialogue_initial_normal, font_NAME_LIMIT);
	icons_copy_text(window, REPORT_FONT_DIALOGUE_BFONT, report_font_dialogue_initial_bold, font_NAME_LIMIT);
	icons_copy_text(window, REPORT_FONT_DIALOGUE_IFONT, report_font_dialogue_initial_italic, font_NAME_LIMIT);
	icons_copy_text(window, REPORT_FONT_DIALOGUE_BIFONT, report_font_dialogue_initial_bold_italic, font_NAME_LIMIT);

	report_font_dialogue_initial_size = atoi(icons_get_indirected_text_addr(window, REPORT_FONT_DIALOGUE_FONTSIZE)) * 16;
	report_font_dialogue_initial_spacing = atoi(icons_get_indirected_text_addr(window, REPORT_FONT_DIALOGUE_FONTSPACE));

	/* Call the client back. */

	report_font_dialogue_callback(report,
			report_font_dialogue_initial_normal, report_font_dialogue_initial_bold,
			report_font_dialogue_initial_italic, report_font_dialogue_initial_bold_italic,
			report_font_dialogue_initial_size, report_font_dialogue_initial_spacing);

	return TRUE;
}


/**
 * The Report Font dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer (unused).
 */

static void report_font_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	report_font_dialogue_callback = NULL;
}


/**
 * Process menu prepare events in the Report Font dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the owning window.
 * \param icon		The target icon for the menu.
 * \param *menu		Pointer to struct to take the menu details.
 * \param *data		Client data pointer (unused).
 * \return		TRUE if the menu struct was updated; else FALSE.
 */

static osbool report_font_dialogue_menu_prepare(struct file_block *file, wimp_w window, wimp_i icon, struct dialogue_menu_data *menu, void *data)
{
	if (menu == NULL)
		return FALSE;

	menu->menu = fontlist_build();
	menu->help_token = "FontMenu";

	return TRUE;
}


/**
 * Process menu selection events in the Report Font dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param w		The handle of the owning window.
 * \param icon		The target icon for the menu.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 * \param *data		Client data pointer (unused). 
 */

static void report_font_dialogue_menu_selection(struct file_block *file, wimp_w window, wimp_i icon, wimp_menu *menu, wimp_selection *selection, void *data)
{
	char	*font;

	font = fontlist_decode(selection);
	if (font == NULL)
		return;

	icons_printf(window, icon, "%s", font);

	heap_free(font);
}


/**
 * Process menu close events in the Report Font dialogue.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *data		Client data pointer (unused).
 */

static void report_font_dialogue_menu_close(struct file_block *file, wimp_w window, wimp_menu *menu, void *data)
{
	fontlist_destroy();
}

