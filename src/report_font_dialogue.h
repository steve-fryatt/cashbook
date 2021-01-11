/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: report_font_dialogue.h
 *
 * High-level report font dialogue interface.
 */

#ifndef CASHBOOK_REPORT_FONT_DIALOGUE
#define CASHBOOK_REPORT_FONT_DIALOGUE

/**
 * Initialise the report format dialogue.
 */

void report_font_dialogue_initialise(void);


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
		char *normal, char *bold, char *italic, char *bold_italic, int size, int spacing);


/**
 * Force the closure of the report format dialogue if it relates to a
 * given report instance.
 *
 * \param *report		The report to be closed.
 */

void report_font_dialogue_force_close(struct report *report);

#endif

