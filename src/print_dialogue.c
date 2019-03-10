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
 * \file: print_dialogue.c
 *
 * Print Dialogue Implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/pdriver.h"
#include "oslib/os.h"
#include "oslib/osfile.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "print_dialogue.h"

#include "caret.h"
#include "date.h"
#include "dialogue.h"
#include "report.h"
#include "stringbuild.h"


/**
 * The maximum space allocated for a print line.
 */

#define PRINT_MAX_LINE_LEN 4096

/**
 * The maximum space allocated for a print report title.
 */

#define PRINT_MAX_TITLE_LEN 256

/**
 * The maximum length of a message token.
 */

#define PRINT_MAX_TOKEN_LEN 64

/* Dialogue Icons */

#define PRINT_DIALOGUE_OK 19
#define PRINT_DIALOGUE_CANCEL 21
#define PRINT_DIALOGUE_REPORT 20
#define PRINT_DIALOGUE_STANDARD 8
#define PRINT_DIALOGUE_PORTRAIT 12
#define PRINT_DIALOGUE_LANDSCAPE 13
#define PRINT_DIALOGUE_SCALE 14
#define PRINT_DIALOGUE_FASTTEXT 9
#define PRINT_DIALOGUE_TEXTFORMAT 18
#define PRINT_DIALOGUE_RANGE_BOX 0
#define PRINT_DIALOGUE_RANGE_TITLE 1
#define PRINT_DIALOGUE_RANGE_LABEL1 2
#define PRINT_DIALOGUE_RANGE_FROM 3
#define PRINT_DIALOGUE_RANGE_LABEL2 4
#define PRINT_DIALOGUE_RANGE_TO 5
#define PRINT_DIALOGUE_PNUM 16
#define PRINT_DIALOGUE_TITLE 15
#define PRINT_DIALOGUE_GRID 17

/**
 * A Print Dialogue Instance Structure.
 */

struct print_dialogue_block {
	struct file_block	*file;			/**< The file to which this dialogue instance belongs.						*/

	osbool			fit_width;		/**< TRUE to fit width in graphics mode; FALSE to print 100%.					*/
	osbool			title;			/**< TRUE to include the report title; FALSE to omit it.					*/
	osbool			page_numbers;		/**< TRUE to print page numbers; FALSE to omit them.						*/
	osbool			grid;			/**< TRUE to plot a grid around tables; FALSE to omit it.					*/
	osbool			rotate;			/**< TRUE to rotate 90 degrees in graphics mode to print Landscape; FALSE to print Portrait.	*/
	osbool			text;			/**< TRUE to print in text mode; FALSE to print in graphics mode.				*/
	osbool			text_format;		/**< TRUE to print with styles in text mode; FALSE to print plain text.				*/

	date_t			from;			/**< The date to print from in ranged prints (Advanced Print dialogue only).			*/
	date_t			to;			/**< The date to print to in ranged prints (Advanced Print dialogue only).			*/
};


/* Global Variables. */

/**
 * The handle of the Print dialogue.
 */

static struct dialogue_block	*print_dialogue = NULL;

/**
 *TRUE if the selection of dates is allowed in the current dialogue box.
 */

static osbool			print_dialogue_include_dates = FALSE;

/**
 * The message token for the Print Dialogue title.
 */

static char			print_dialogue_window_title_token[PRINT_MAX_TOKEN_LEN];

/**
 * The message token for the Print Report title.
 */

static char			print_dialogue_report_title_token[PRINT_MAX_TOKEN_LEN];

/**
 * Client data to be passed to the callback function.
 */

static void			*print_dialogue_client_data = NULL;

/* The client callback to initiate a print job. */

static struct report*		(*print_dialogue_callback) (struct report *, void *, date_t, date_t);

/* Static Function Prototypes. */

static osbool		print_dialogue_handle_message_set_printer(wimp_message *message);
static void		print_dialogue_fill_window(struct file_block *file, wimp_w window, osbool restore, void *data);
static osbool		print_dialogue_process_window(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);
static void		print_dialogue_close(struct file_block *file, wimp_w window, void *data);
static struct report	*print_dialogue_create_report(struct print_dialogue_block *instance);
static void		print_dialogue_process_report(struct print_dialogue_block *instance, struct report *report, osbool direct);

/**
 * The Print Dialogue Icon Set.
 */

static struct dialogue_icon print_dialogue_icon_list[] = {
	{DIALOGUE_ICON_OK,					PRINT_DIALOGUE_OK,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_CANCEL,					PRINT_DIALOGUE_CANCEL,		DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_ACTION | DIALOGUE_ICON_PRINT_REPORT,	PRINT_DIALOGUE_REPORT,		DIALOGUE_NO_ICON},

	/* Range Group */

	{DIALOGUE_ICON_REFRESH,					PRINT_DIALOGUE_RANGE_FROM,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_REFRESH,					PRINT_DIALOGUE_RANGE_TO,	DIALOGUE_NO_ICON},

	/* Print Mode Group. */

	{DIALOGUE_ICON_RADIO_PASS | DIALOGUE_ICON_SHADE_TARGET,	PRINT_DIALOGUE_STANDARD,	DIALOGUE_NO_ICON},
	{DIALOGUE_ICON_RADIO_PASS | DIALOGUE_ICON_SHADE_TARGET,	PRINT_DIALOGUE_FASTTEXT,	DIALOGUE_NO_ICON},

	/* Formatting Group. */

	{DIALOGUE_ICON_RADIO | DIALOGUE_ICON_SHADE_OFF,		PRINT_DIALOGUE_PORTRAIT,	PRINT_DIALOGUE_STANDARD},
	{DIALOGUE_ICON_SHADE_OR | DIALOGUE_ICON_SHADE_ON,	PRINT_DIALOGUE_PORTRAIT,	PRINT_DIALOGUE_FASTTEXT},
	{DIALOGUE_ICON_RADIO | DIALOGUE_ICON_SHADE_OFF,		PRINT_DIALOGUE_LANDSCAPE,	PRINT_DIALOGUE_STANDARD},
	{DIALOGUE_ICON_SHADE_OR | DIALOGUE_ICON_SHADE_ON,	PRINT_DIALOGUE_LANDSCAPE,	PRINT_DIALOGUE_FASTTEXT},
	{DIALOGUE_ICON_SHADE_OFF,				PRINT_DIALOGUE_SCALE,		PRINT_DIALOGUE_STANDARD},
	{DIALOGUE_ICON_SHADE_OR | DIALOGUE_ICON_SHADE_ON,	PRINT_DIALOGUE_SCALE,		PRINT_DIALOGUE_FASTTEXT},
	{DIALOGUE_ICON_SHADE_OFF,				PRINT_DIALOGUE_TITLE,		PRINT_DIALOGUE_STANDARD},
	{DIALOGUE_ICON_SHADE_OR | DIALOGUE_ICON_SHADE_ON,	PRINT_DIALOGUE_TITLE,		PRINT_DIALOGUE_FASTTEXT},
	{DIALOGUE_ICON_SHADE_OFF,				PRINT_DIALOGUE_PNUM,		PRINT_DIALOGUE_STANDARD},
	{DIALOGUE_ICON_SHADE_OR | DIALOGUE_ICON_SHADE_ON,	PRINT_DIALOGUE_PNUM,		PRINT_DIALOGUE_FASTTEXT},
	{DIALOGUE_ICON_SHADE_OFF,				PRINT_DIALOGUE_GRID,		PRINT_DIALOGUE_STANDARD},
	{DIALOGUE_ICON_SHADE_OR | DIALOGUE_ICON_SHADE_ON,	PRINT_DIALOGUE_GRID,		PRINT_DIALOGUE_FASTTEXT},
	{DIALOGUE_ICON_SHADE_OFF,				PRINT_DIALOGUE_TEXTFORMAT,	PRINT_DIALOGUE_FASTTEXT},
	{DIALOGUE_ICON_SHADE_OR | DIALOGUE_ICON_SHADE_ON,	PRINT_DIALOGUE_TEXTFORMAT,	PRINT_DIALOGUE_STANDARD},

	{DIALOGUE_ICON_END,					DIALOGUE_NO_ICON,		DIALOGUE_NO_ICON}
};

/**
 * The Print Dialogue Definition.
 */

static struct dialogue_definition print_dialogue_definition = {
	"Print",
	"Print",
	print_dialogue_icon_list,
	DIALOGUE_GROUP_NONE,
	DIALOGUE_FLAGS_TAKE_FOCUS,
	print_dialogue_fill_window,
	print_dialogue_process_window,
	print_dialogue_close,
	NULL,
	NULL,
	NULL
};


/**
 * Initialise the printing dialogue system.
 */

void print_dialogue_initialise(void)
{
	print_dialogue = dialogue_create(&print_dialogue_definition);

	/* Register the Wimp message handlers. */

	event_add_message_handler(message_PRINT_INIT, EVENT_MESSAGE_INCOMING, print_dialogue_handle_message_set_printer);
	event_add_message_handler(message_SET_PRINTER, EVENT_MESSAGE_INCOMING, print_dialogue_handle_message_set_printer);
}


/**
 * Construct new printing data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \param *file		The file to own the block.
 * \return		Pointer to the new data block, or NULL on error.
 */

struct print_dialogue_block *print_dialogue_create(struct file_block *file)
{
	struct print_dialogue_block		*new;

	new = heap_alloc(sizeof(struct print_dialogue_block));
	if (new == NULL)
		return NULL;

	new->file = file;

	new->fit_width = config_opt_read("ReportFitWidth");
	new->title = config_opt_read("ReportShowTitle");
	new->page_numbers = config_opt_read("ReportShowPageNum");
	new->grid = config_opt_read("ReportShowGrid");
	new->rotate = config_opt_read("ReportRotate");
	new->text = config_opt_read("PrintText");
	new->text_format = config_opt_read("PrintTextFormat");

	new->from = NULL_DATE;
	new->to = NULL_DATE;

	return new;
}


/**
 * Delete a printing data block.
 *
 * \param *print	Pointer to the printing window data to delete.
 */

void print_dialogue_delete(struct print_dialogue_block *print)
{
	if (print == NULL)
		return;

	/* Close any related dialogues. */

	dialogue_force_all_closed(NULL, print);

	/* Free the memory. */

	heap_free(print);
}


/**
 * Process a Message_SetPrinter or Message_PrintInit to call the registered
 * printer details update handler function.  This will do things like updating
 * the printer name in a print dialogue box.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool print_dialogue_handle_message_set_printer(wimp_message *message)
{
	dialogue_refresh(print_dialogue, TRUE);

	return TRUE;
}


/**
 * Open the Print dialoge box, as used by a number of print routines.
 *
 * \param *instance	The print dialogue instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param dates		TRUE to allow dates to be selected, FALSE to shade
 *			the associated fields out.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param *report	The MessageTrans token for the report title, or NULL
 *			if the client doesn't need a report. 
 * \param *parent	A parent for the dialogue, or NULL if there isn't
 *			a specific owner.
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 * \param *data		Data to be passed to the callback function.
 */

void print_dialogue_open(struct print_dialogue_block *instance, wimp_pointer *ptr, osbool dates, osbool restore, char *title, char *report,
		void *parent, struct report* (callback) (struct report *, void *, date_t, date_t), void *data)
{
	if (instance == NULL)
		return;

	print_dialogue_include_dates = dates;
	print_dialogue_client_data = data;

	if (title != NULL)
		string_copy(print_dialogue_window_title_token, title, PRINT_MAX_TOKEN_LEN);
	else
		*print_dialogue_window_title_token = '\0';

	if (report != NULL)
		string_copy(print_dialogue_report_title_token, report, PRINT_MAX_TOKEN_LEN);
	else
		*print_dialogue_report_title_token = '\0';

	dialogue_open(print_dialogue, restore, instance->file, parent, ptr, instance);

	print_dialogue_callback = callback;
}


/**
 * Fill the Print Dialogue with values.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param restore	TRUE if the dialogue should restore previous settings.
 * \param *data		Client data pointer, giving the dialogue instance.
 */

static void print_dialogue_fill_window(struct file_block *file, wimp_w window, osbool restore, void *data)
{
	char				*name, buffer[25];
	os_error			*error;
	struct print_dialogue_block	*instance = data;

	if (instance == NULL)
		return;

	/* Read the name of the currently selected printer. */

	error = xpdriver_info(NULL, NULL, NULL, NULL, &name, NULL, NULL, NULL);

	if (error != NULL) {
		msgs_lookup("NoPDriverT", buffer, sizeof(buffer));
		name = buffer;
	}

	dialogue_set_title(print_dialogue, print_dialogue_window_title_token, name, NULL, NULL, NULL);

	if (restore) {
		icons_set_selected(window, PRINT_DIALOGUE_STANDARD, !instance->text);
		icons_set_selected(window, PRINT_DIALOGUE_PORTRAIT, !instance->rotate);
		icons_set_selected(window, PRINT_DIALOGUE_LANDSCAPE, instance->rotate);
		icons_set_selected(window, PRINT_DIALOGUE_SCALE, instance->fit_width);
		icons_set_selected(window, PRINT_DIALOGUE_PNUM, instance->page_numbers);
		icons_set_selected(window, PRINT_DIALOGUE_TITLE, instance->title);
		icons_set_selected(window, PRINT_DIALOGUE_GRID, instance->grid);

		icons_set_selected(window, PRINT_DIALOGUE_FASTTEXT, instance->text);
		icons_set_selected(window, PRINT_DIALOGUE_TEXTFORMAT, instance->text_format);

		date_convert_to_string(instance->from, icons_get_indirected_text_addr(window, PRINT_DIALOGUE_RANGE_FROM),
				icons_get_indirected_text_length(window, PRINT_DIALOGUE_RANGE_FROM));
		date_convert_to_string(instance->to, icons_get_indirected_text_addr(window, PRINT_DIALOGUE_RANGE_TO),
				icons_get_indirected_text_length(window, PRINT_DIALOGUE_RANGE_TO));
	} else {
		icons_set_selected(window, PRINT_DIALOGUE_STANDARD, !config_opt_read("PrintText"));
		icons_set_selected(window, PRINT_DIALOGUE_PORTRAIT, !config_opt_read("ReportRotate"));
		icons_set_selected(window, PRINT_DIALOGUE_LANDSCAPE, config_opt_read("ReportRotate"));
		icons_set_selected(window, PRINT_DIALOGUE_SCALE, config_opt_read("ReportFitWidth"));
		icons_set_selected(window, PRINT_DIALOGUE_PNUM, config_opt_read("ReportShowPageNum"));
		icons_set_selected(window, PRINT_DIALOGUE_GRID, config_opt_read("ReportShowGrid"));
		icons_set_selected(window, PRINT_DIALOGUE_TITLE, config_opt_read("ReportShowTitle"));

		icons_set_selected(window, PRINT_DIALOGUE_FASTTEXT, config_opt_read("PrintText"));
		icons_set_selected(window, PRINT_DIALOGUE_TEXTFORMAT, config_opt_read("PrintTextFormat"));

		*icons_get_indirected_text_addr(window, PRINT_DIALOGUE_RANGE_FROM) = '\0';
		*icons_get_indirected_text_addr(window, PRINT_DIALOGUE_RANGE_TO) = '\0';
	}

	icons_set_group_shaded(window, !print_dialogue_include_dates, 6,
			PRINT_DIALOGUE_RANGE_BOX, PRINT_DIALOGUE_RANGE_TITLE, PRINT_DIALOGUE_RANGE_LABEL1,
			PRINT_DIALOGUE_RANGE_LABEL2, PRINT_DIALOGUE_RANGE_FROM, PRINT_DIALOGUE_RANGE_TO);

	icons_set_shaded(window, PRINT_DIALOGUE_OK, error != NULL);

	icons_set_shaded(window, PRINT_DIALOGUE_REPORT, *print_dialogue_report_title_token == '\0');
}


/**
 * Process the contents of the Advanced Print window, store the details and
 * call the supplied callback function with the details.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be processed.
 * \param *pointer	The Wimp pointer state.
 * \param type		The type of icon selected by the user.
 * \param *parent	The dialogue parent object.
 * \param *data		Client data pointer, giving the dialogue instance.
 * \return		TRUE if the dialogue should close; otherwise FALSE.
 */

static osbool print_dialogue_process_window(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data)
{
	char				print_line[PRINT_MAX_LINE_LEN];
	struct report			*report_in, *report_out;
	struct print_dialogue_block	*instance = data;

	if (instance == NULL)
		return FALSE;

	/* Extract the information and call the originator's print start function. */

	instance->fit_width = icons_get_selected(window, PRINT_DIALOGUE_SCALE);
	instance->rotate = icons_get_selected(window, PRINT_DIALOGUE_LANDSCAPE);
	instance->text = icons_get_selected(window, PRINT_DIALOGUE_FASTTEXT);
	instance->text_format = icons_get_selected(window, PRINT_DIALOGUE_TEXTFORMAT);
	instance->page_numbers = icons_get_selected(window, PRINT_DIALOGUE_PNUM);
	instance->title = icons_get_selected(window, PRINT_DIALOGUE_TITLE);
	instance->grid = icons_get_selected(window, PRINT_DIALOGUE_GRID);

	instance->from = date_convert_from_string(icons_get_indirected_text_addr(window, PRINT_DIALOGUE_RANGE_FROM),
			NULL_DATE, 0);
	instance->to = date_convert_from_string(icons_get_indirected_text_addr(window, PRINT_DIALOGUE_RANGE_TO),
			NULL_DATE, 0);

	if (!stringbuild_initialise(print_line, PRINT_MAX_LINE_LEN))
		return FALSE;

	report_in = print_dialogue_create_report(instance);

	report_out = print_dialogue_callback(report_in, print_dialogue_client_data, instance->from, instance->to);

	stringbuild_cancel();

	if (report_in != NULL && report_out == NULL)
		report_delete(report_in);

	print_dialogue_process_report(instance, report_out, type == DIALOGUE_ICON_OK);

	return TRUE;
}


/**
 * The Print dialogue has been closed.
 *
 * \param *file		The file instance associated with the dialogue.
 * \param window	The handle of the dialogue box to be filled.
 * \param *data		Client data pointer, giving the dialogue instance.
 */

static void print_dialogue_close(struct file_block *file, wimp_w window, void *data)
{
	print_dialogue_callback = NULL;
	print_dialogue_client_data = NULL;
}


/**
 * Construct a new report for the client to use, if one has been requested
 * by supplying a valid report title message token.
 *
 * \param *instance		The dialogue instance to be used.
 * \return			The new report handle, or NULL.
 */

static struct report *print_dialogue_create_report(struct print_dialogue_block *instance)
{
	struct report	*report;

	char report_title[PRINT_MAX_TITLE_LEN];

	if (instance == NULL || *print_dialogue_report_title_token == '\0')
		return NULL;

	msgs_lookup(print_dialogue_report_title_token, report_title, PRINT_MAX_TITLE_LEN);
	report = report_open(instance->file, report_title, NULL);

	if (report == NULL)
		error_msgs_report_error("PrintMemFail");

	return report;
}


/**
 * Process a report returned from a client.
 *
 * \param *instance		The dialogue instance in use.
 * \param *report		The report to be processed.
 * \param direct		TRUE if the report should be printed direct.
 */

static void print_dialogue_process_report(struct print_dialogue_block *instance, struct report *report, osbool direct)
{
	if (instance == NULL || report == NULL)
		return;

	report_set_options(report, instance->fit_width, instance->rotate,
				instance->title, instance->page_numbers, instance->grid);

	if (direct == TRUE)
		report_close_and_print(report, instance->text, instance->text_format);
	else
		report_close(report);
}

