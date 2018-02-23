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

//#include "global.h"
#include "print_dialogue.h"

#include "caret.h"
#include "date.h"
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

/* Advanced Print Dialogue Icons */

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
 * Print dialogue.
 */

struct print_dialogue_block {
	struct file_block	*file;						/**< The file to which this dialogue instance belongs.						*/

	osbool			fit_width;					/**< TRUE to fit width in graphics mode; FALSE to print 100%.					*/
	osbool			title;						/**< TRUE to include the report title; FALSE to omit it.					*/
	osbool			page_numbers;					/**< TRUE to print page numbers; FALSE to omit them.						*/
	osbool			grid;						/**< TRUE to plot a grid around tables; FALSE to omit it.					*/
	osbool			rotate;						/**< TRUE to rotate 90 degrees in graphics mode to print Landscape; FALSE to print Portrait.	*/
	osbool			text;						/**< TRUE to print in text mode; FALSE to print in graphics mode.				*/
	osbool			text_format;					/**< TRUE to print with styles in text mode; FALSE to print plain text.				*/

	date_t			from;						/**< The date to print from in ranged prints (Advanced Print dialogue only).			*/
	date_t			to;						/**< The date to print to in ranged prints (Advanced Print dialogue only).			*/
};

/**
 * Allow us to track which of the print windows is being referenced.
 */

enum print_dialogue_type {
	PRINTING_DIALOGUE_TYPE_NONE,									/**< No printing window is open.						*/
	PRINTING_DIALOGUE_TYPE_SIMPLE,									/**< The Simple Print window is open.						*/
	PRINTING_DIALOGUE_TYPE_ADVANCED									/**< The Advanced Print window is open.						*/
};

/* The active dialogue. */

static enum print_dialogue_type		print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_NONE;	/**< Which of the two windows is open.						*/

/* Window Handles. */

static wimp_w				print_dialogue_window = NULL;				/**< The Advanced Print window handle.						*/

/* Global Variables. */

static struct print_dialogue_block	*print_dialogue_current_instance = NULL;			/**< The print dialogue instance currently owning the dialogue.			*/
static osbool				print_dialogue_current_restore = FALSE;				/**< The current restore setting for the print dialogue.			*/
static char				print_dialogue_window_title_token[PRINT_MAX_TOKEN_LEN];		/**< The message token for the Simple Print window title.			*/
static char				print_dialogue_report_title_token[PRINT_MAX_TOKEN_LEN];		/**< The message token for the Simple Print report title.			*/
static void				*print_dialogue_client_data = NULL;				/**< Client data to be passed to the callback function.				*/

/* Simple print window handling. */

static struct report*			(*print_dialogue_simple_callback) (struct report *, void *);

/* Date-range print window handling. */

static struct report*			(*print_dialogue_advanced_callback) (struct report *, void *, date_t, date_t);

/* Static Function Prototypes. */

static osbool		print_dialogue_handle_message_set_printer(wimp_message *message);

static osbool		print_dialogue_open(struct print_dialogue_block *instance, osbool restore, char *title, char *report, void *data);

static void		print_dialogue_click_handler(wimp_pointer *pointer);
static osbool		print_dialogue_keypress_handler(wimp_key *key);

static void		print_dialogue_refresh_window(void);
static void		print_dialogue_fill_window(struct print_dialogue_block *print_data, osbool simple, osbool restore);
static void		print_dialogue_process_window(osbool direct);

static struct report	*print_dialogue_create_report(void);
static void		print_dialogue_process_report(struct print_dialogue_block *instance, struct report *report, osbool direct);


/**
 * Initialise the printing system.
 */

void print_dialogue_initialise(void)
{
	print_dialogue_window = templates_create_window("Print");
	ihelp_add_window(print_dialogue_window, "Print", NULL);
	event_add_window_mouse_event(print_dialogue_window, print_dialogue_click_handler);
	event_add_window_key_event(print_dialogue_window, print_dialogue_keypress_handler);
	event_add_window_icon_radio(print_dialogue_window, PRINT_DIALOGUE_STANDARD, FALSE);
	event_add_window_icon_radio(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT, FALSE);
	event_add_window_icon_radio(print_dialogue_window, PRINT_DIALOGUE_PORTRAIT, TRUE);
	event_add_window_icon_radio(print_dialogue_window, PRINT_DIALOGUE_LANDSCAPE, TRUE);

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

	if (print == print_dialogue_current_instance) {
		if (windows_get_open(print_dialogue_window))
			close_dialogue_with_caret(print_dialogue_window);

		print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_NONE;
	}

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
	print_dialogue_refresh_window();

	return TRUE;
}


/**
 * Open the Simple Print dialoge box, as used by a number of print routines.
 *
 * \param *instance	The print dialogue instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The MessageTrans token for the dialogue title.
 * \param *report	The MessageTrans token for the report title, or NULL
 *			if the client doesn't need a report. 
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 * \param *data		Data to be passed to the callback function.
 */

void print_dialogue_open_simple(struct print_dialogue_block *instance, wimp_pointer *ptr, osbool restore, char *title, char *report,
		struct report* (callback) (struct report *, void *), void *data)
{
	if (instance == NULL)
		return;
	
	if (!print_dialogue_open(instance, restore, title, report, data))
		return;

	print_dialogue_simple_callback = callback;

	/* Set the window contents up. */

	print_dialogue_fill_window(instance, TRUE, restore);

	/* Open the window on screen. */

	windows_open_centred_at_pointer(print_dialogue_window, ptr);
	place_dialogue_caret(print_dialogue_window, wimp_ICON_WINDOW);

	print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_SIMPLE;
}


/**
 * Open the Advanced Print dialoge box, as used by a number of print routines.
 *
 * \param *instance	The print dialogue instance to own the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param *report	The MessageTrans token for the report title, or NULL
 *			if the client doesn't need a report. 
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 * \param *data		Data to be passed to the callback function.
 */

void print_dialogue_open_advanced(struct print_dialogue_block *instance, wimp_pointer *ptr, osbool restore, char *title, char *report,
		struct report* (callback) (struct report *, void *, date_t, date_t), void *data)
{
	if (instance == NULL)
		return;

	if (!print_dialogue_open(instance, restore, title, report, data))
		return;

	print_dialogue_advanced_callback = callback;

	/* Set the window contents up. */

	print_dialogue_fill_window(instance, FALSE, restore);

	/* Open the window on screen. */

	windows_open_centred_at_pointer(print_dialogue_window, ptr);
	place_dialogue_caret(print_dialogue_window, PRINT_DIALOGUE_RANGE_FROM);

	print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_ADVANCED;
}


/**
 * Prepare to open one of the print dialogue boxes.
 *
 * \param *instance	The print dialogue instance to own the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param *report	The MessageTrans token for the report title, or NULL
 *			if the client doesn't need a report.
 * \param *data		Data to be passed to the callback function.
 * \return		TRUE if successful; otherwise FALSE.
 */

static osbool print_dialogue_open(struct print_dialogue_block *instance, osbool restore, char *title, char *report, void *data)
{
	/* If the window is already open, another print job is being set up.
	 * Assume the user wants to lose any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret() as the caret is
	 * just moving from one dialogue to another.
	 */

	if (windows_get_open(print_dialogue_window))
		wimp_close_window(print_dialogue_window);

	print_dialogue_current_instance = instance;
	print_dialogue_current_restore = restore;
	print_dialogue_client_data = data;

	if (title != NULL)
		string_copy(print_dialogue_window_title_token, title, PRINT_MAX_TOKEN_LEN);
	else
		*print_dialogue_window_title_token = '\0';

	if (report != NULL)
		string_copy(print_dialogue_report_title_token, report, PRINT_MAX_TOKEN_LEN);
	else
		*print_dialogue_report_title_token = '\0';

	return TRUE;
}


/**
 * Process mouse clicks in the Advanced Print dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void print_dialogue_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case PRINT_DIALOGUE_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(print_dialogue_window);
			print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_NONE;
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			print_dialogue_refresh_window();
		}
		break;

	case PRINT_DIALOGUE_OK:
	case PRINT_DIALOGUE_REPORT:
		print_dialogue_process_window(pointer->i == PRINT_DIALOGUE_OK);
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(print_dialogue_window);
			print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_NONE;
		}
		break;

	case PRINT_DIALOGUE_STANDARD:
	case PRINT_DIALOGUE_FASTTEXT:
		icons_set_group_shaded(print_dialogue_window,
				icons_get_selected(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT) || (*print_dialogue_report_title_token == '\0'), 6,
				PRINT_DIALOGUE_PORTRAIT, PRINT_DIALOGUE_LANDSCAPE, PRINT_DIALOGUE_SCALE,
				PRINT_DIALOGUE_PNUM, PRINT_DIALOGUE_TITLE, PRINT_DIALOGUE_GRID);

		icons_set_group_shaded_when_off(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT, 1, PRINT_DIALOGUE_TEXTFORMAT);

		if (icons_get_shaded(print_dialogue_window, PRINT_DIALOGUE_TEXTFORMAT))
			icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_TEXTFORMAT, TRUE);
		break;
	}
}


/**
 * Process keypresses in one of the print dialogue boxes.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool print_dialogue_keypress_handler(wimp_key *key)
{
	if (key->w != print_dialogue_window)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		print_dialogue_process_window(TRUE);
		close_dialogue_with_caret(key->w);
		print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_NONE;
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret(key->w);
		print_dialogue_window_open = PRINTING_DIALOGUE_TYPE_NONE;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Advanced Print window.
 */

static void print_dialogue_refresh_window(void)
{
	if (print_dialogue_window_open != PRINTING_DIALOGUE_TYPE_ADVANCED)
		return;

	print_dialogue_fill_window(print_dialogue_current_instance, print_dialogue_window_open == PRINTING_DIALOGUE_TYPE_SIMPLE, print_dialogue_current_restore);
	icons_replace_caret_in_window(print_dialogue_window);
	xwimp_force_redraw_title(print_dialogue_window);
}


/**
 * Fill the Advanced Print window with values.
 *
 * \param *print_data		Saved settings to use if restore == FALSE.
 * \param simple		TRUE to configure a simple dialogue; FALSE for
 *				one with a date range.
 * \param restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void print_dialogue_fill_window(struct print_dialogue_block *print_data, osbool simple, osbool restore)
{
	char		*name, buffer[25];
	os_error	*error;

	error = xpdriver_info(NULL, NULL, NULL, NULL, &name, NULL, NULL, NULL);

	if (error != NULL) {
		msgs_lookup("NoPDriverT", buffer, sizeof(buffer));
		name = buffer;
	}

	windows_title_msgs_param_lookup(print_dialogue_window, print_dialogue_window_title_token, name, NULL, NULL, NULL);

	if (!restore) {
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_STANDARD, !config_opt_read("PrintText"));
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_PORTRAIT, !config_opt_read("ReportRotate"));
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_LANDSCAPE, config_opt_read("ReportRotate"));
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_SCALE, config_opt_read("ReportFitWidth"));
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_PNUM, config_opt_read("ReportShowPageNum"));
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_GRID, config_opt_read("ReportShowGrid"));
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_TITLE, config_opt_read("ReportShowTitle"));

		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT, config_opt_read("PrintText"));
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_TEXTFORMAT, config_opt_read("PrintTextFormat"));

		*icons_get_indirected_text_addr(print_dialogue_window, PRINT_DIALOGUE_RANGE_FROM) = '\0';
		*icons_get_indirected_text_addr(print_dialogue_window, PRINT_DIALOGUE_RANGE_TO) = '\0';
	} else {
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_STANDARD, !print_data->text);
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_PORTRAIT, !print_data->rotate);
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_LANDSCAPE, print_data->rotate);
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_SCALE, print_data->fit_width);
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_PNUM, print_data->page_numbers);
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_TITLE, print_data->title);
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_GRID, print_data->grid);

		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT, print_data->text);
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_TEXTFORMAT, print_data->text_format);

		date_convert_to_string(print_data->from, icons_get_indirected_text_addr(print_dialogue_window, PRINT_DIALOGUE_RANGE_FROM),
				icons_get_indirected_text_length(print_dialogue_window, PRINT_DIALOGUE_RANGE_FROM));
		date_convert_to_string(print_data->to, icons_get_indirected_text_addr(print_dialogue_window, PRINT_DIALOGUE_RANGE_TO),
				icons_get_indirected_text_length(print_dialogue_window, PRINT_DIALOGUE_RANGE_TO));
	}

	icons_set_group_shaded(print_dialogue_window, simple, 6,
			PRINT_DIALOGUE_RANGE_BOX, PRINT_DIALOGUE_RANGE_TITLE, PRINT_DIALOGUE_RANGE_LABEL1,
			PRINT_DIALOGUE_RANGE_LABEL2, PRINT_DIALOGUE_RANGE_FROM, PRINT_DIALOGUE_RANGE_TO);

	icons_set_group_shaded(print_dialogue_window,
			icons_get_selected(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT) || (*print_dialogue_report_title_token == '\0'), 6,
			PRINT_DIALOGUE_PORTRAIT, PRINT_DIALOGUE_LANDSCAPE, PRINT_DIALOGUE_SCALE,
			PRINT_DIALOGUE_PNUM, PRINT_DIALOGUE_TITLE, PRINT_DIALOGUE_GRID);

	icons_set_group_shaded_when_off(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT, 1, PRINT_DIALOGUE_TEXTFORMAT);

	if (icons_get_shaded(print_dialogue_window, PRINT_DIALOGUE_TEXTFORMAT))
		icons_set_selected(print_dialogue_window, PRINT_DIALOGUE_TEXTFORMAT, TRUE);

	icons_set_shaded(print_dialogue_window, PRINT_DIALOGUE_OK, error != NULL);

	icons_set_shaded(print_dialogue_window, PRINT_DIALOGUE_REPORT, *print_dialogue_report_title_token == '\0');
}


/**
 * Process the contents of the Advanced Print window, store the details and
 * call the supplied callback function with the details.
 *
 * \param direct		TRUE if the report should be printed direct.
 */

static void print_dialogue_process_window(osbool direct)
{
	char		print_line[PRINT_MAX_LINE_LEN];
	struct report	*report_in, *report_out;

	#ifdef DEBUG
	debug_printf("\\BProcessing the Advanced Print window");
	#endif

	/* Extract the information and call the originator's print start function. */

	print_dialogue_current_instance->fit_width = icons_get_selected(print_dialogue_window, PRINT_DIALOGUE_SCALE);
	print_dialogue_current_instance->rotate = icons_get_selected(print_dialogue_window, PRINT_DIALOGUE_LANDSCAPE);
	print_dialogue_current_instance->text = icons_get_selected(print_dialogue_window, PRINT_DIALOGUE_FASTTEXT);
	print_dialogue_current_instance->text_format = icons_get_selected(print_dialogue_window, PRINT_DIALOGUE_TEXTFORMAT);
	print_dialogue_current_instance->page_numbers = icons_get_selected(print_dialogue_window, PRINT_DIALOGUE_PNUM);

	print_dialogue_current_instance->from = date_convert_from_string(icons_get_indirected_text_addr(print_dialogue_window, PRINT_DIALOGUE_RANGE_FROM),
			NULL_DATE, 0);
	print_dialogue_current_instance->to = date_convert_from_string(icons_get_indirected_text_addr(print_dialogue_window, PRINT_DIALOGUE_RANGE_TO),
			NULL_DATE, 0);

	if (!stringbuild_initialise(print_line, PRINT_MAX_LINE_LEN))
		return;

	report_in = print_dialogue_create_report();

	switch (print_dialogue_window_open) {
	case PRINTING_DIALOGUE_TYPE_SIMPLE:
		report_out = print_dialogue_simple_callback(report_in, print_dialogue_client_data);
		break;

	case PRINTING_DIALOGUE_TYPE_ADVANCED:
		report_out = print_dialogue_advanced_callback(report_in, print_dialogue_client_data,
				print_dialogue_current_instance->from,
				print_dialogue_current_instance->to);
		break;

	default:
		report_out = NULL;
		break;
	}

	stringbuild_cancel();

	if (report_in != NULL && report_out == NULL)
		report_delete(report_in);

	print_dialogue_process_report(print_dialogue_current_instance, report_out, direct);
}


/**
 * Construct a new report for the client to use, if one has been requested
 * by supplying a valid report title message token.
 *
 * \return			The new report handle, or NULL.
 */

static struct report *print_dialogue_create_report(void)
{
	struct report	*report;

	char report_title[PRINT_MAX_TITLE_LEN];

	if (print_dialogue_current_instance == NULL || *print_dialogue_report_title_token == '\0')
		return NULL;

	msgs_lookup(print_dialogue_report_title_token, report_title, PRINT_MAX_TITLE_LEN);
	report = report_open(print_dialogue_current_instance->file, report_title, NULL);

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

