/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: printing.c
 *
 * Low-level printing implementation.
 */

/* ANSI C header files */

#include <string.h>

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
#include "sflib/msgs.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "printing.h"

#include "caret.h"
#include "dataxfer.h"
#include "date.h"
#include "ihelp.h"
#include "templates.h"

/* This code deals with a "RISC OS 2" subset of the printer driver protocol.  We can start print jobs off via
 * the correct set of codes, but all printing is done immediately and the queue mechanism is ignored.
 *
 * This really needs to be addressed in a future version.
 */

/* Simple print dialogue */

#define SIMPLE_PRINT_OK 0
#define SIMPLE_PRINT_CANCEL 1
#define SIMPLE_PRINT_STANDARD 2
#define SIMPLE_PRINT_PORTRAIT 3
#define SIMPLE_PRINT_LANDSCAPE 4
#define SIMPLE_PRINT_SCALE 5
#define SIMPLE_PRINT_FASTTEXT 6
#define SIMPLE_PRINT_TEXTFORMAT 7
#define SIMPLE_PRINT_PNUM 8

/* Date-range print dailogue */

#define DATE_PRINT_OK 0
#define DATE_PRINT_CANCEL 1
#define DATE_PRINT_STANDARD 2
#define DATE_PRINT_PORTRAIT 3
#define DATE_PRINT_LANDSCAPE 4
#define DATE_PRINT_SCALE 5
#define DATE_PRINT_FASTTEXT 6
#define DATE_PRINT_TEXTFORMAT 7
#define DATE_PRINT_FROM 9
#define DATE_PRINT_TO 11
#define DATE_PRINT_PNUM 12

/**
 * Print dialogue.
 */

struct printing {
	osbool		fit_width;						/**< TRUE to fit width in graphics mode; FALSE to print 100%.					*/
	osbool		page_numbers;						/**< TRUE to print page numbers; FALSE to omit them.						*/
	osbool		rotate;							/**< TRUE to rotate 90 degrees in graphics mode to print Landscape; FALSE to print Portrait.	*/
	osbool		text;							/**< TRUE to print in text mode; FALSE to print in graphics mode.				*/
	osbool		text_format;						/**< TRUE to print with styles in text mode; FALSE to print plain text.				*/

	date_t		from;							/**< The date to print from in ranged prints (Advanced Print dialogue only).			*/
	date_t		to;							/**< The date to print to in ranged prints (Advanced Print dialogue only).			*/
};

/**
 * Allow us to track which of the print windows is being referenced.
 */

enum printing_window {
	PRINTING_WINDOW_NONE,							/**< No printing window is open.								*/
	PRINTING_WINDOW_SIMPLE,							/**< The Simple Print window is open.								*/
	PRINTING_WINDOW_ADVANCED						/**< The Advanced Print window is open.								*/
};

/* Print protocol negotiations. */

static void			(*printing_callback_start) (char *) = NULL;	/**< Callback to launch the print process.							*/
static void			(*printing_callback_cancel) (void) = NULL;	/**< Callback to clean up if the process fails part-way.					*/

static osbool			printing_text_mode;				/**< TRUE if the current print job is in text mode.						*/


static wimp_w			printing_simple_window = NULL;			/**< The Simple Print window handle.								*/
static wimp_w			printing_advanced_window = NULL;		/**< The Advanced Print window handle.								*/

static enum printing_window	printing_window_open = PRINTING_WINDOW_NONE;	/**< Which of the two windows is open.								*/



/* Simple print window handling. */

static void			(*printing_simple_callback) (osbool, osbool, osbool, osbool, osbool);
static char			printing_simple_title_token[64];		/**< The message token for the Simple Print window title.					*/
static file_data		*printing_simple_file = NULL;			/**< The file currently owning the Simple Print window.						*/
static osbool			printing_simple_restore;			/**< The current restore setting for the Simple Print window.					*/

/* Date-range print window handling. */

static void			(*printing_advanced_callback) (osbool, osbool, osbool, osbool, osbool, date_t, date_t);
static char			printing_advanced_title_token[64];		/**< The message token for the Advanced Print window title.					*/
static file_data		*printing_advanced_file = NULL;			/**< The file currently owning the Advanced Print window.					*/
static osbool			printing_advanced_restore;			/**< The current restore setting for the Advanced Print window.					*/



static osbool		printing_handle_bounced_message_print_save(wimp_message *message);
static osbool		printing_handle_message_print_error(wimp_message *message);
static osbool		printing_handle_message_print_file(wimp_message *message);
static osbool		printing_handle_message_set_printer(wimp_message *message);

static void		printing_simple_click_handler(wimp_pointer *pointer);
static osbool		printing_simple_keypress_handler(wimp_key *key);
static void		printing_refresh_simple_window(void);
static void		printing_fill_simple_window(struct printing *print_data, osbool restore);
static void		printing_process_simple_window(void);

static void		printing_advanced_click_handler(wimp_pointer *pointer);
static osbool		printing_advanced_keypress_handler(wimp_key *key);
static void		printing_refresh_advanced_window(void);
static void		printing_fill_advanced_window(struct printing *print_data, osbool restore);
static void		printing_process_advanced_window(void);


/**
 * Initialise the printing system.
 */

void printing_initialise(void)
{
	printing_simple_window = templates_create_window("SimplePrint");
	ihelp_add_window(printing_simple_window, "SimplePrint", NULL);
	event_add_window_mouse_event(printing_simple_window, printing_simple_click_handler);
	event_add_window_key_event(printing_simple_window, printing_simple_keypress_handler);
	event_add_window_icon_radio(printing_simple_window, SIMPLE_PRINT_STANDARD, FALSE);
	event_add_window_icon_radio(printing_simple_window, SIMPLE_PRINT_FASTTEXT, FALSE);
	event_add_window_icon_radio(printing_simple_window, SIMPLE_PRINT_PORTRAIT, TRUE);
	event_add_window_icon_radio(printing_simple_window, SIMPLE_PRINT_LANDSCAPE, TRUE);

	printing_advanced_window = templates_create_window("DatePrint");
	ihelp_add_window(printing_advanced_window, "DatePrint", NULL);
	event_add_window_mouse_event(printing_advanced_window, printing_advanced_click_handler);
	event_add_window_key_event(printing_advanced_window, printing_advanced_keypress_handler);
	event_add_window_icon_radio(printing_advanced_window, DATE_PRINT_STANDARD, FALSE);
	event_add_window_icon_radio(printing_advanced_window, DATE_PRINT_FASTTEXT, FALSE);
	event_add_window_icon_radio(printing_advanced_window, DATE_PRINT_PORTRAIT, TRUE);
	event_add_window_icon_radio(printing_advanced_window, DATE_PRINT_LANDSCAPE, TRUE);

	/* Register the Wimp message handlers. */

	event_add_message_handler(message_PRINT_INIT, EVENT_MESSAGE_INCOMING, printing_handle_message_set_printer);
	event_add_message_handler(message_SET_PRINTER, EVENT_MESSAGE_INCOMING, printing_handle_message_set_printer);
	event_add_message_handler(message_PRINT_ERROR, EVENT_MESSAGE_INCOMING, printing_handle_message_print_error);
	event_add_message_handler(message_PRINT_FILE, EVENT_MESSAGE_INCOMING, printing_handle_message_print_file);
	event_add_message_handler(message_PRINT_SAVE, EVENT_MESSAGE_ACKNOWLEDGE, printing_handle_bounced_message_print_save);
}


/**
 * Construct new printing data block for a file, and return a pointer to the
 * resulting block. The block will be allocated with heap_alloc(), and should
 * be freed after use with heap_free().
 *
 * \return		Pointer to the new data block, or NULL on error.
 */

struct printing *printing_create(void)
{
	struct printing		*new;

	new = heap_alloc(sizeof(struct printing));
	if (new == NULL)
		return NULL;

	new->fit_width = config_opt_read("PrintFitWidth");
	new->page_numbers = config_opt_read("PrintPageNumbers");
	new->rotate = config_opt_read("PrintRotate");
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

void printing_delete(struct printing *print)
{
	if (print != NULL)
		heap_free(print);
}


/**
 * Send a Message_PrintSave to start the printing process off with the
 * RISC OS printer driver.
 *
 * \param *callback_print	Callback function to start the printing once
 *				negotiations have completed successfully.
 * \param *callback_cancel	Callback function to terminate printing
 *				if things fail at any stage.
 * \param *text_print		TRUE to print as text; FALSE to print in
 *				graphics mode.
 * \return			TRUE if process started OK; FALSE on error.
 */

osbool printing_send_start_print_save(void (*callback_print) (char *), void (*callback_cancel) (void), osbool text_print)
{
	wimp_full_message_data_xfer	datasave;
	os_error			*error;

	#ifdef DEBUG
	debug_printf("Sending Message_PrintFile");
	#endif

	printing_callback_start = callback_print;
	printing_text_mode = text_print;

	/* Set up and send Message_PrintSave. */

	datasave.size = WORDALIGN(45 + strlen (""));
	datasave.your_ref = 0;
	datasave.action = message_PRINT_SAVE;

	datasave.w = NULL;
	datasave.i = 0;
	datasave.pos.x = 0;
	datasave.pos.y = 0;
	datasave.est_size = 0;
	datasave.file_type = 0;
	*datasave.file_name = '\0';

	error = xwimp_send_message(wimp_USER_MESSAGE_RECORDED, (wimp_message *) &datasave, wimp_BROADCAST);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	return TRUE;
}


/**
 * Process a bounced Message_PrintSave.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool printing_handle_bounced_message_print_save(wimp_message *message)
{
	#ifdef DEBUG
	debug_printf("Message_PrintSave bounced");
	#endif

	if (!printing_text_mode)
		printing_callback_start("");
	else
		error_msgs_report_error("NoPManager");

	if (printing_callback_cancel != NULL)
		printing_callback_cancel();

	return TRUE;
}


/**
 * Process a Message_PrinterError.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool printing_handle_message_print_error(wimp_message *message)
{
	pdriver_full_message_print_error	*print_error = (pdriver_full_message_print_error *) message;

	#ifdef DEBUG
	debug_printf("Received Message_PrintError");
	#endif

	/* If the message block size is 20, this is a RISC OS 2 style Message_PrintBusy. */

	if (print_error->size == 20)
		error_msgs_report_error("PrintBusy");
	else
		error_report_error(print_error->errmess);

	if (printing_callback_cancel != NULL)
		printing_callback_cancel();

	return TRUE;
}


/**
 * Process a Message_PrintFile.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool printing_handle_message_print_file(wimp_message *message)
{
	char				filename[256];
	int				length;
	wimp_full_message_data_xfer	*print_file = (wimp_full_message_data_xfer *) message;
	os_error			*error;

	#ifdef DEBUG
	debug_printf("Received Message_PrintFile");
	#endif

	if (printing_text_mode) {
		/* Text mode printing.  Find the filename for the Print-temp file. */

		xos_read_var_val("Printer$Temp", filename, sizeof(filename), 0, os_VARTYPE_STRING, &length, NULL, NULL);
		*(filename+length) = '\0';

		/* Call the printing function with the PrintTemp filename. */

		printing_callback_start(filename);

		/* Set up the Message_DataLoad and send it to Printers.  File size and file type are read from the actual file
		 * on disc, using OS_File.
		 */

		print_file->your_ref = print_file->my_ref;
		print_file->action = message_DATA_LOAD;

		osfile_read_stamped_no_path(filename, NULL, NULL, &(print_file->est_size), NULL, &(print_file->file_type));
		strcpy(print_file->file_name, filename);

		print_file->size = WORDALIGN(45 + strlen (filename));

		error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) print_file, print_file->sender);
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
	} else {
		print_file->your_ref = print_file->my_ref;
		print_file->action = message_WILL_PRINT;

		error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) print_file, print_file->sender);
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		else
			printing_callback_start("");
	}

	return TRUE;
}


/**
 * Process a Message_SetPrinter or Message_PrintInit to call the registered
 * printer details update handler function.  This will do things like updating
 * the printer name in a print dialogue box.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool printing_handle_message_set_printer(wimp_message *message)
{
	switch (printing_window_open) {
	case PRINTING_WINDOW_SIMPLE:
		printing_refresh_simple_window();
		break;

	case PRINTING_WINDOW_ADVANCED:
		printing_refresh_advanced_window();
		break;

	default:
		break;
	}

	return TRUE;
}


/* Force the closure of any printing windows which are open and relate
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void printing_force_windows_closed(file_data *file)
{
	if (printing_simple_file == file && windows_get_open(printing_simple_window)) {
		close_dialogue_with_caret(printing_simple_window);
		printing_window_open = PRINTING_WINDOW_NONE;
	}

	if (printing_advanced_file == file && windows_get_open (printing_advanced_window)) {
		close_dialogue_with_caret(printing_advanced_window);
		printing_window_open = PRINTING_WINDOW_NONE;
	}
}


/**
 * Open the Simple Print dialoge box, as used by a number of print routines.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 */

void printing_open_simple_window(file_data *file, wimp_pointer *ptr, osbool restore, char *title,
		void (callback) (osbool, osbool, osbool, osbool, osbool))
{
	/* If the window is already open, another print job is being set up.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret() as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(printing_simple_window))
		wimp_close_window(printing_simple_window);

	if (windows_get_open(printing_advanced_window))
		wimp_close_window(printing_advanced_window);

	printing_simple_file = file;
	printing_simple_callback = callback;
	printing_simple_restore = restore;

	/* Set the window contents up. */

	strcpy(printing_simple_title_token, title);
	printing_fill_simple_window(file->print, restore);

	/* Open the window on screen. */

	windows_open_centred_at_pointer(printing_simple_window, ptr);
	place_dialogue_caret(printing_simple_window, wimp_ICON_WINDOW);

	printing_window_open = PRINTING_WINDOW_SIMPLE;
}


/**
 * Process mouse clicks in the Simple Print dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void printing_simple_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case SIMPLE_PRINT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(printing_simple_window);
			printing_window_open = PRINTING_WINDOW_NONE;
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			printing_refresh_simple_window();
		}
		break;

	case SIMPLE_PRINT_OK:
		printing_process_simple_window();
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(printing_simple_window);
			printing_window_open = PRINTING_WINDOW_NONE;
		}
		break;

	case SIMPLE_PRINT_STANDARD:
	case SIMPLE_PRINT_FASTTEXT:
		icons_set_group_shaded_when_off(printing_simple_window, SIMPLE_PRINT_STANDARD, 4,
				SIMPLE_PRINT_PORTRAIT, SIMPLE_PRINT_LANDSCAPE, SIMPLE_PRINT_SCALE, SIMPLE_PRINT_PNUM);
		icons_set_group_shaded_when_off(printing_simple_window, SIMPLE_PRINT_FASTTEXT, 1,
				SIMPLE_PRINT_TEXTFORMAT);
		break;
	}
}


/**
 * Process keypresses in the Simple Print window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool printing_simple_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		printing_process_simple_window();
		close_dialogue_with_caret(printing_simple_window);
		printing_window_open = PRINTING_WINDOW_NONE;
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret (printing_simple_window);
		printing_window_open = PRINTING_WINDOW_NONE;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Refresh the contents of the current Simple Print window.
 */

static void printing_refresh_simple_window(void)
{
	printing_fill_simple_window(printing_simple_file->print, printing_simple_restore);
	icons_replace_caret_in_window(printing_simple_window);
	xwimp_force_redraw_title(printing_simple_window);
}


/**
 * Fill the Simple Print window with values.
 *
 * \param: *print_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void printing_fill_simple_window(struct printing *print_data, osbool restore)
{
	char		*name, buffer[25];
	os_error	*error;

	error = xpdriver_info(NULL, NULL, NULL, NULL, &name, NULL, NULL, NULL);

	if (error != NULL) {
		msgs_lookup("NoPDriverT", buffer, sizeof(buffer));
		name = buffer;
	}

	msgs_param_lookup(printing_simple_title_token, windows_get_indirected_title_addr(printing_simple_window), 64, name, NULL, NULL, NULL);

	if (!restore) {
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_STANDARD, !config_opt_read ("PrintText"));
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_PORTRAIT, !config_opt_read ("PrintRotate"));
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_LANDSCAPE, config_opt_read ("PrintRotate"));
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_SCALE, config_opt_read ("PrintFitWidth"));
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_PNUM, config_opt_read ("PrintPageNumbers"));

		icons_set_selected(printing_simple_window, SIMPLE_PRINT_FASTTEXT, config_opt_read ("PrintText"));
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_TEXTFORMAT, config_opt_read ("PrintTextFormat"));
	} else {
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_STANDARD, !print_data->text);
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_PORTRAIT, !print_data->rotate);
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_LANDSCAPE, print_data->rotate);
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_SCALE, print_data->fit_width);
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_PNUM, print_data->page_numbers);

		icons_set_selected(printing_simple_window, SIMPLE_PRINT_FASTTEXT, print_data->text);
		icons_set_selected(printing_simple_window, SIMPLE_PRINT_TEXTFORMAT, print_data->text_format);
	}

	icons_set_group_shaded_when_off(printing_simple_window, SIMPLE_PRINT_STANDARD, 3,
			SIMPLE_PRINT_PORTRAIT, SIMPLE_PRINT_LANDSCAPE, SIMPLE_PRINT_SCALE);
	icons_set_group_shaded_when_off(printing_simple_window, SIMPLE_PRINT_FASTTEXT, 1,
			SIMPLE_PRINT_TEXTFORMAT);

	icons_set_shaded(printing_simple_window, SIMPLE_PRINT_OK, error != NULL);
}


/**
 * Process the contents of the Simple Print window, store the details and
 * call the supplied callback function with the details.
 */

static void printing_process_simple_window(void)
{
	#ifdef DEBUG
	debug_printf("\\BProcessing the Simple Print window");
	#endif

	/* Extract the information and call the originator's print start function. */

	printing_simple_file->print->fit_width = icons_get_selected(printing_simple_window, SIMPLE_PRINT_SCALE);
	printing_simple_file->print->rotate = icons_get_selected(printing_simple_window, SIMPLE_PRINT_LANDSCAPE);
	printing_simple_file->print->text = icons_get_selected(printing_simple_window, SIMPLE_PRINT_FASTTEXT);
	printing_simple_file->print->text_format = icons_get_selected(printing_simple_window, SIMPLE_PRINT_TEXTFORMAT);
	printing_simple_file->print->page_numbers = icons_get_selected(printing_simple_window, SIMPLE_PRINT_PNUM);

	printing_simple_callback(printing_simple_file->print->text,
			printing_simple_file->print->text_format,
			printing_simple_file->print->fit_width,
			printing_simple_file->print->rotate,
			printing_simple_file->print->page_numbers);
}


/**
 * Open the Simple Print dialoge box, as used by a number of print routines.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 * \param *title	The Message Trans token for the dialogue title.
 * \param callback	The function to call when the user closes the dialogue
 *			in the affermative.
 */

void printing_open_advanced_window(file_data *file, wimp_pointer *ptr, osbool restore, char *title,
		void (callback) (osbool, osbool, osbool, osbool, osbool, date_t, date_t))
{
	/* If the window is already open, another print job is being set up.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(printing_simple_window))
		wimp_close_window(printing_simple_window);

	if (windows_get_open(printing_advanced_window))
		wimp_close_window(printing_advanced_window);

	printing_advanced_file = file;
	printing_advanced_callback = callback;
	printing_advanced_restore = restore;

	/* Set the window contents up. */

	strcpy(printing_advanced_title_token, title);
	printing_fill_advanced_window(file->print, restore);

	/* Open the window on screen. */

	windows_open_centred_at_pointer(printing_advanced_window, ptr);
	place_dialogue_caret(printing_advanced_window, DATE_PRINT_FROM);

	printing_window_open = PRINTING_WINDOW_ADVANCED;
}


/**
 * Process mouse clicks in the Advanced Print dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void printing_advanced_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case DATE_PRINT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(printing_advanced_window);
			printing_window_open = PRINTING_WINDOW_NONE;
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			printing_refresh_advanced_window();
		}
		break;

	case DATE_PRINT_OK:
		printing_process_advanced_window();
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_dialogue_with_caret(printing_advanced_window);
			printing_window_open = PRINTING_WINDOW_NONE;
		}
		break;

	case DATE_PRINT_STANDARD:
	case DATE_PRINT_FASTTEXT:
		icons_set_group_shaded_when_off (printing_advanced_window, DATE_PRINT_STANDARD, 4,
				DATE_PRINT_PORTRAIT, DATE_PRINT_LANDSCAPE, DATE_PRINT_SCALE, SIMPLE_PRINT_PNUM);
		icons_set_group_shaded_when_off (printing_advanced_window, DATE_PRINT_FASTTEXT, 1,
				DATE_PRINT_TEXTFORMAT);
		break;
	}
}


/**
 * Process keypresses in the Advanced Print window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool printing_advanced_keypress_handler(wimp_key *key)
{
	switch (key->c) {
		case wimp_KEY_RETURN:
		printing_process_advanced_window ();
		close_dialogue_with_caret (printing_advanced_window);
		printing_window_open = PRINTING_WINDOW_NONE;
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret (printing_advanced_window);
		printing_window_open = PRINTING_WINDOW_NONE;
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

static void printing_refresh_advanced_window(void)
{
	printing_fill_advanced_window(printing_advanced_file->print, printing_advanced_restore);
	icons_replace_caret_in_window(printing_advanced_window);
	xwimp_force_redraw_title(printing_advanced_window);
}


/**
 * Fill the Advanced Print window with values.
 *
 * \param: *print_data		Saved settings to use if restore == FALSE.
 * \param: restore		TRUE to keep the supplied settings; FALSE to
 *				use system defaults.
 */

static void printing_fill_advanced_window(struct printing *print_data, osbool restore)
{
	char		*name, buffer[25];
	os_error	*error;

	error = xpdriver_info(NULL, NULL, NULL, NULL, &name, NULL, NULL, NULL);

	if (error != NULL) {
		msgs_lookup("NoPDriverT", buffer, sizeof(buffer));
		name = buffer;
	}

	msgs_param_lookup(printing_advanced_title_token, windows_get_indirected_title_addr(printing_advanced_window), 64, name, NULL, NULL, NULL);

	if (!restore) {
		icons_set_selected(printing_advanced_window, DATE_PRINT_STANDARD, !config_opt_read ("PrintText"));
		icons_set_selected(printing_advanced_window, DATE_PRINT_PORTRAIT, !config_opt_read ("PrintRotate"));
		icons_set_selected(printing_advanced_window, DATE_PRINT_LANDSCAPE, config_opt_read ("PrintRotate"));
		icons_set_selected(printing_advanced_window, DATE_PRINT_SCALE, config_opt_read ("PrintFitWidth"));
		icons_set_selected(printing_advanced_window, DATE_PRINT_PNUM, config_opt_read ("PrintPageNumbers"));

		icons_set_selected(printing_advanced_window, DATE_PRINT_FASTTEXT, config_opt_read ("PrintText"));
		icons_set_selected(printing_advanced_window, DATE_PRINT_TEXTFORMAT, config_opt_read ("PrintTextFormat"));

		*icons_get_indirected_text_addr(printing_advanced_window, DATE_PRINT_FROM) = '\0';
		*icons_get_indirected_text_addr(printing_advanced_window, DATE_PRINT_TO) = '\0';
	} else {
		icons_set_selected(printing_advanced_window, DATE_PRINT_STANDARD, !print_data->text);
		icons_set_selected(printing_advanced_window, DATE_PRINT_PORTRAIT, !print_data->rotate);
		icons_set_selected(printing_advanced_window, DATE_PRINT_LANDSCAPE, print_data->rotate);
		icons_set_selected(printing_advanced_window, DATE_PRINT_SCALE, print_data->fit_width);
		icons_set_selected(printing_advanced_window, DATE_PRINT_PNUM, print_data->page_numbers);

		icons_set_selected(printing_advanced_window, DATE_PRINT_FASTTEXT, print_data->text);
		icons_set_selected(printing_advanced_window, DATE_PRINT_TEXTFORMAT, print_data->text_format);

		date_convert_to_string(print_data->from, icons_get_indirected_text_addr(printing_advanced_window, DATE_PRINT_FROM),
				icons_get_indirected_text_length(printing_advanced_window, DATE_PRINT_FROM));
		date_convert_to_string(print_data->to, icons_get_indirected_text_addr(printing_advanced_window, DATE_PRINT_TO),
				icons_get_indirected_text_length(printing_advanced_window, DATE_PRINT_TO));
	}

	icons_set_group_shaded_when_off(printing_advanced_window, DATE_PRINT_STANDARD, 3,
			DATE_PRINT_PORTRAIT, DATE_PRINT_LANDSCAPE, DATE_PRINT_SCALE);
	icons_set_group_shaded_when_off(printing_advanced_window, DATE_PRINT_FASTTEXT, 1,
			DATE_PRINT_TEXTFORMAT);

	icons_set_shaded(printing_advanced_window, DATE_PRINT_OK, error != NULL);
}


/**
 * Process the contents of the Advanced Print window, store the details and
 * call the supplied callback function with the details.
 */

static void printing_process_advanced_window(void)
{
	#ifdef DEBUG
	debug_printf("\\BProcessing the Advanced Print window");
	#endif

	/* Extract the information and call the originator's print start function. */

	printing_advanced_file->print->fit_width = icons_get_selected(printing_advanced_window, DATE_PRINT_SCALE);
	printing_advanced_file->print->rotate = icons_get_selected(printing_advanced_window, DATE_PRINT_LANDSCAPE);
	printing_advanced_file->print->text = icons_get_selected(printing_advanced_window, DATE_PRINT_FASTTEXT);
	printing_advanced_file->print->text_format = icons_get_selected(printing_advanced_window, DATE_PRINT_TEXTFORMAT);
	printing_advanced_file->print->page_numbers = icons_get_selected(printing_advanced_window, DATE_PRINT_PNUM);

	printing_advanced_file->print->from = convert_string_to_date(icons_get_indirected_text_addr(printing_advanced_window, DATE_PRINT_FROM),
			NULL_DATE, 0);
	printing_advanced_file->print->to = convert_string_to_date(icons_get_indirected_text_addr(printing_advanced_window, DATE_PRINT_TO),
			NULL_DATE, 0);

	printing_advanced_callback(printing_advanced_file->print->text,
			printing_advanced_file->print->text_format,
			printing_advanced_file->print->fit_width,
			printing_advanced_file->print->rotate,
			printing_advanced_file->print->page_numbers,
			printing_advanced_file->print->from,
			printing_advanced_file->print->to);
}

