/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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

#include "oslib/font.h"

/**
 * The font data held by the dialogue.
 */

struct report_font_dialogue_data {
	/**
	 * The normal font name.
	 */

	char	normal[font_NAME_LIMIT];

	/**
	 * The bold font name.
	 */

	char	bold[font_NAME_LIMIT];

	/**
	 * The italic font name.
	 */

	char	italic[font_NAME_LIMIT];

	/**
	 * The bold italic font name.
	 */

	char	bold_italic[font_NAME_LIMIT];

	/**
	 * The font size.
	 */

	int	size;

	/**
	 * The font line spacing.
	 */

	int	spacing;
};

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
 * \param *content		Pointer to structure to hold the dialogue content.
 */

void report_font_dialogue_open(wimp_pointer *ptr, struct report *report, void (*callback)(void *, struct report_font_dialogue_data *),
		struct report_font_dialogue_data *content);

#endif

