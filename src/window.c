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
 * \file: window.c
 *
 * Window support code.
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osbyte.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/general.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "window.h"

#include "currency.h"
#include "date.h"

/**
 * The window template block currently being used for plotting icons.
 */

static wimp_window	*window_icon_templates = NULL;

/**
 * The character sequence used to indicate a reconciled account reference. */

static char		window_reconciled_symbol[REC_FIELD_LEN];


/**
 * Set up the extent and visible area of a window in its creation block so that
 * it can be passed to Wimp_CreateWindow.
 *
 * \param *window		The window creation data block to update.
 * \param width			The width of the work area (not visible area).
 * \param height		The height of the work area (not visible area).
 * \param x			X position of top-left of window (or -1 for default).
 * \param y			Y position of top-left of window (or -1 for default).
 * \param yoff			Y Offset to apply to enable raked openings.
 */

void set_initial_window_area(wimp_window *window, int width, int height, int x, int y, int yoff)
{
	int	limit, lower_limit;
	osbool	clear_iconbar;

	/* Set the extent of the window. */

	window->extent.x0 = 0;
	window->extent.x1 = width;
	window->extent.y0 = -height;
	window->extent.y1 = 0;

	/* Read CMOS RAM to see if the icon-bar is to be kept clear. */

	clear_iconbar = ((osbyte2 (osbyte_READ_CMOS, osbyte_CONFIGURE_NO_OBSCURE_ICON_BAR, 0) &
			osbyte_CONFIGURE_NO_OBSCURE_ICON_BAR_MASK) != 0);

	/* Set up the X position. */

	limit = general_mode_width();
	width = (width < (limit * X_WINDOW_PERCENT_LIMIT / 100)) ? width : (limit * X_WINDOW_PERCENT_LIMIT / 100);

	if (x > -1) {
		window->visible.x0 = x;
		window->visible.x1 = x + width;

		if (window->visible.x1 >= limit) {
			window->visible.x1 = limit - 1;
			window->visible.x0 = window->visible.x1 - width;
		}
	} else {
		window->visible.x0 = (limit - width) / 2;
		window->visible.x1 = window->visible.x0 + width;
	}

	/* Set up the Y position. */

	limit = general_mode_height();
	lower_limit = (clear_iconbar) ? sf_ICONBAR_HEIGHT : 0; /* The lower usable bound if clear-ibar is set. */

	/* Calculate the maximum visible height possible.  This is as a percentage of the total screen height.  If the
	 * height comes out as too high for practical use, it is reduced to the height - icon-bar clearance, with top and
	 * bottom window furniture removed too.
	 */

	height = (height < (limit * Y_WINDOW_PERCENT_LIMIT / 100)) ? height : (limit * Y_WINDOW_PERCENT_LIMIT / 100);
	if (height + (2 * sf_WINDOW_GADGET_HEIGHT) > limit - lower_limit)
		height = limit - lower_limit - (2 * sf_WINDOW_GADGET_HEIGHT);

	if (y > -1) {
		window->visible.y1 = y - yoff - sf_WINDOW_GADGET_HEIGHT;
		window->visible.y0 = y - yoff - sf_WINDOW_GADGET_HEIGHT - height;

		if (window->visible.y0 < lower_limit) {
			window->visible.y0 = lower_limit;
			window->visible.y1 = lower_limit + height;
		}
	} else {
		window->visible.y1 = (limit * Y_WINDOW_PERCENT_ORIGIN / 100) - yoff;
		window->visible.y0 = window->visible.y1 - height;

		if (window->visible.y0 < lower_limit + sf_WINDOW_GADGET_HEIGHT) {
			window->visible.y0 = lower_limit + sf_WINDOW_GADGET_HEIGHT ;
			window->visible.y1 = lower_limit + sf_WINDOW_GADGET_HEIGHT + height;
		}
	}
}





/**
 * Calculate the row that the mouse was clicked over in the list window.
 *
 * \param *pointer		The relevant Wimp pointer data.
 * \param *state		The relevant Wimp window state.
 * \param toolbar_height	The height of the window's toolbar, in OS units.
 * \param max_lines		The maximum number of lines in the window, or -1 for no constraint.
 * \return			The row (from 0) or -1 if none.
 */

int window_calculate_click_row(os_coord *pos, wimp_window_state *state, int toolbar_height, int max_lines)
{
	int		y, row_y_pos, row;

	y = pos->y - state->visible.y1 + state->yscroll;

	row = WINDOW_ROW(toolbar_height, y);
	row_y_pos = WINDOW_ROW_Y_POS(toolbar_height, y);

	if ((row < 0) || ((max_lines > 0) && (row >= max_lines)) || WINDOW_ROW_ABOVE(row_y_pos) || WINDOW_ROW_BELOW(row_y_pos))
		row = -1;

	return row;
}


/**
 * Initialise a window template for use by the icon plotting interface.
 * 
 * It is assumed that all of the icons in the template have valid indirection
 * data set up for them, including buffer sizes.
 *
 * \param *definition		Pointer to the window template.
 */

void window_set_icon_templates(wimp_window *definition)
{
	window_icon_templates = definition;
	msgs_lookup("RecChar", window_reconciled_symbol, REC_FIELD_LEN);
}


/**
 * Plot an empty field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 */
 
void window_plot_empty_field(wimp_i field)
{
	wimp_icon	*icon = window_icon_templates->icons + field;

	if (icon->data.indirected_text.text == NULL || icon->data.indirected_text.size < 0)
		return;

	*(icon->data.indirected_text.text) = '\0';

	wimp_plot_icon(icon);
}


/**
 * Plot a text field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param *text			Pointer to the text to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_text_field(wimp_i field, char *text, wimp_colour colour)
{
	wimp_icon	*icon = window_icon_templates->icons + field;
	char		*buffer;

	icon->flags &= ~wimp_ICON_FG_COLOUR;
	icon->flags |= (colour << wimp_ICON_FG_COLOUR_SHIFT);

	buffer = icon->data.indirected_text.text;
	icon->data.indirected_text.text = text;

	wimp_plot_icon(icon);

	icon->data.indirected_text.text = buffer;
}


/**
 * Plot an integer field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param number		The integer value to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_int_field(wimp_i field, int number, wimp_colour colour)
{
	wimp_icon	*icon = window_icon_templates->icons + field;

	icon->flags &= ~wimp_ICON_FG_COLOUR;
	icon->flags |= (colour << wimp_ICON_FG_COLOUR_SHIFT);

	snprintf(icon->data.indirected_text.text, icon->data.indirected_text.size, "%d", number);

	wimp_plot_icon(icon);
}


/**
 * Plot a single character field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param character		The single character to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_char_field(wimp_i field, char character, wimp_colour colour)
{
	wimp_icon	*icon = window_icon_templates->icons + field;

	icon->flags &= ~wimp_ICON_FG_COLOUR;
	icon->flags |= (colour << wimp_ICON_FG_COLOUR_SHIFT);

	snprintf(icon->data.indirected_text.text, icon->data.indirected_text.size, "%c", character);

	wimp_plot_icon(icon);
}


/**
 * Plot a reconciled flag field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param reconciled		The reconciled state (yes or no) to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_reconciled_field(wimp_i field, osbool reconciled, wimp_colour colour)
{
	wimp_icon	*icon = window_icon_templates->icons + field;

	icon->flags &= ~wimp_ICON_FG_COLOUR;
	icon->flags |= (colour << wimp_ICON_FG_COLOUR_SHIFT);

	if (reconciled)
		strncpy(icon->data.indirected_text.text, window_reconciled_symbol, icon->data.indirected_text.size);
	else
		*(icon->data.indirected_text.text) = '\0';

	wimp_plot_icon(icon);
}


/**
 * Plot a date field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param date			The date to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_date_field(wimp_i field, date_t date, wimp_colour colour)
{
	wimp_icon	*icon = window_icon_templates->icons + field;

	icon->flags &= ~wimp_ICON_FG_COLOUR;
	icon->flags |= (colour << wimp_ICON_FG_COLOUR_SHIFT);

	date_convert_to_string(date, icon->data.indirected_text.text, icon->data.indirected_text.size);

	wimp_plot_icon(icon);
}


/**
 * Plot a currency field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param amount		The currency amount to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_currency_field(wimp_i field, amt_t amount, wimp_colour colour)
{
	wimp_icon	*icon = window_icon_templates->icons + field;

	icon->flags &= ~wimp_ICON_FG_COLOUR;
	icon->flags |= (colour << wimp_ICON_FG_COLOUR_SHIFT);

	currency_convert_to_string(amount, icon->data.indirected_text.text, icon->data.indirected_text.size);

	wimp_plot_icon(icon);
}
