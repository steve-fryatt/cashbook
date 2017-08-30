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
 * \file: printing_protocol.h
 *
 * RISC OS Print Protocol Interface.
 */

#ifndef CASHBOOK_PRINTING_PROTOCOL
#define CASHBOOK_PRINTING_PROTOCOL


/**
 * Initialise the printing protocol system.
 */

void printing_protocol_initialise(void);


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

osbool printing_protocol_send_start_print_save(void (*callback_print) (char *), void (*callback_cancel) (void), osbool text_print);

#endif

