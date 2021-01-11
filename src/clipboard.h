/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: clipboard.h
 *
 * Global Clipboard implementation.
 */

#ifndef CASHBOOK_CLIPBOARD
#define CASHBOOK_CLIPBOARD

/**
 * Initialise the Clipboard module.
 */

void clipboard_initialise(void);


/**
 * Copy the contents of an icon to the global clipboard, cliaming it in the
 * process if necessary.
 *
 * \param *key			A Wimp Key block, indicating the icon to copy.
 * \return			TRUE if a value was copied; else FALSE.
 */

osbool clipboard_copy_from_icon(wimp_key *key);


/**
 * Cut the contents of an icon to the global clipboard, cliaming it in the
 * process if necessary.
 *
 * \param *key			A Wimp Key block, indicating the icon to cut.
 * \return			TRUE if a value was cut; else FALSE.
 */

osbool clipboard_cut_from_icon(wimp_key *key);

/**
 * Paste the contents of the global clipboard into an icon.  If we own the
 * clipboard, this is done immediately; otherwise the Wimp message dialogue
 * is started.
 *
 * \param *key			A Wimp Key block, indicating the icon to paste.
 * \param *callback		A callback function to be called after d delayed
 *				paste from another application.
 * \param *data			Data to be passed to the callback function.
 * \return			TRUE if a value was pasted immediately; else FALSE.
 */

osbool clipboard_paste_to_icon(wimp_key *key, void (*callback)(void *), void *data);

#endif

