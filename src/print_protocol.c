/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: print_protocol.c
 *
 * RISC OS Print Protocol Implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/pdriver.h"
#include "oslib/osfile.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/string.h"

/* Application header files */

#include "print_protocol.h"

/* This code deals with a "RISC OS 2" subset of the printer driver protocol.  We can start print jobs off via
 * the correct set of codes, but all printing is done immediately and the queue mechanism is ignored.
 *
 * This really needs to be addressed in a future version.
 */


/* Print protocol negotiations. */

/**
 * Callback to launch the print process.
 */

static void			(*print_protocol_callback_start) (char *, void *) = NULL;

/**
 * Callback to clean up if the process fails part-way.
 */

static void			(*print_protocol_callback_cancel) (void *) = NULL;

/**
 * User data pointer to pass to the callback functions.
 */

static void			*print_protocol_data;

/**
 * TRUE if the current print job is in text mode.
 */

static osbool			print_protocol_text_mode;

/* Static Function Prototypes. */

static osbool		print_protocol_handle_bounced_message_print_save(wimp_message *message);
static osbool		print_protocol_handle_message_print_error(wimp_message *message);
static osbool		print_protocol_handle_message_print_file(wimp_message *message);


/**
 * Initialise the printing protocol system.
 */

void print_protocol_initialise(void)
{
	/* Register the Wimp message handlers. */

	event_add_message_handler(message_PRINT_ERROR, EVENT_MESSAGE_INCOMING, print_protocol_handle_message_print_error);
	event_add_message_handler(message_PRINT_FILE, EVENT_MESSAGE_INCOMING, print_protocol_handle_message_print_file);
	event_add_message_handler(message_PRINT_SAVE, EVENT_MESSAGE_ACKNOWLEDGE, print_protocol_handle_bounced_message_print_save);
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
 * \param *data			User data to pass to the callback functions.
 * \return			TRUE if process started OK; FALSE on error.
 */

osbool print_protocol_send_start_print_save(void (*callback_print) (char *, void *), void (*callback_cancel) (void *), osbool text_print, void *data)
{
	wimp_full_message_data_xfer	datasave;
	os_error			*error;

	#ifdef DEBUG
	debug_printf("Sending Message_PrintFile");
	#endif

	print_protocol_callback_start = callback_print;
	print_protocol_callback_cancel = callback_cancel;
	print_protocol_data = data;
	print_protocol_text_mode = text_print;

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

static osbool print_protocol_handle_bounced_message_print_save(wimp_message *message)
{
	#ifdef DEBUG
	debug_printf("Message_PrintSave bounced");
	#endif

	if (!print_protocol_text_mode)
		print_protocol_callback_start("", print_protocol_data);
	else
		error_msgs_report_error("NoPManager");

	if (print_protocol_callback_cancel != NULL)
		print_protocol_callback_cancel(print_protocol_data);

	return TRUE;
}


/**
 * Process a Message_PrinterError.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool print_protocol_handle_message_print_error(wimp_message *message)
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

	if (print_protocol_callback_cancel != NULL)
		print_protocol_callback_cancel(print_protocol_data);

	return TRUE;
}


/**
 * Process a Message_PrintFile.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool print_protocol_handle_message_print_file(wimp_message *message)
{
	char				filename[256];
	int				length;
	wimp_full_message_data_xfer	*print_file = (wimp_full_message_data_xfer *) message;
	os_error			*error;

	#ifdef DEBUG
	debug_printf("Received Message_PrintFile");
	#endif

	if (print_protocol_text_mode) {
		/* Text mode printing.  Find the filename for the Print-temp file. */

		xos_read_var_val("Printer$Temp", filename, sizeof(filename), 0, os_VARTYPE_STRING, &length, NULL, NULL);
		*(filename+length) = '\0';

		/* Call the printing function with the PrintTemp filename. */

		print_protocol_callback_start(filename, print_protocol_data);

		/* Set up the Message_DataLoad and send it to Printers.  File size and file type are read from the actual file
		 * on disc, using OS_File.
		 */

		print_file->your_ref = print_file->my_ref;
		print_file->action = message_DATA_LOAD;

		osfile_read_stamped_no_path(filename, NULL, NULL, &(print_file->est_size), NULL, &(print_file->file_type));
		string_copy(print_file->file_name, filename, 212);

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
			print_protocol_callback_start("", print_protocol_data);
	}

	return TRUE;
}

