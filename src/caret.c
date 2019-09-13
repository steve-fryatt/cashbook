/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: caret.c
 *
 * Dialogue box caret history implementation.
 */

/* ANSI C header files */

#include <stdarg.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/windows.h"

/* Application header files */

#include "caret.h"

/* ==================================================================================================================
 * Global variables.
 */

static wimp_w	caret_old_window = NULL;
static wimp_i	caret_old_icon = 0;
static int	caret_old_index = 0;

/* ==================================================================================================================
 *
 */

void place_dialogue_caret(wimp_w window, wimp_i icon)
{
	wimp_caret	caret;


	wimp_get_caret_position(&caret);

	if (event_get_window_user_data(caret.w) != NULL) {
		caret_old_window = caret.w;
		caret_old_icon = caret.i;
		caret_old_index = caret.index;
	}

	icons_put_caret_at_end(window, icon);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Try to place the caret into a sequence of writable icons, using the first not to be shaded.  If all are shaded,
 * place the caret into the work area.
 */


void place_dialogue_caret_fallback(wimp_w window, int icons, ...)
{
	int		i;
	wimp_i		icon;
	va_list		ap;
	wimp_caret	caret;


	va_start(ap, icons);

	wimp_get_caret_position(&caret);

	if (event_get_window_user_data(caret.w) != NULL) {
		caret_old_window = caret.w;
		caret_old_icon = caret.i;
		caret_old_index = caret.index;
	}

	i = 0;

	while (i < icons && i != -1) {
		i++;

		icon = va_arg(ap, wimp_i);

		if (!icons_get_shaded(window, icon)) {
			icons_put_caret_at_end(window, icon);
			i = -1;
		}
	}

	if (i != -1)
		icons_put_caret_at_end(window, wimp_ICON_WINDOW);

	va_end(ap);
}

/* ================================================================================================================== */

void close_dialogue_with_caret(wimp_w window)
{
	wimp_caret caret;

	if (caret_old_window != NULL) {
		wimp_get_caret_position(&caret);

		#ifdef DEBUG
		debug_printf("Close dialogue %d (caret location %d)", window, caret.w);
		#endif

		if (caret.w == window && windows_get_open(caret_old_window) && event_get_window_user_data(caret_old_window) != NULL) {
			wimp_set_caret_position(caret_old_window, caret_old_icon, 0, 0, -1, caret_old_index);

			caret_old_window = NULL;
			caret_old_icon = 0;
			caret_old_index = 0;
		}
	}

	wimp_close_window(window);
}

