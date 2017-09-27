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
 * \file: window.h
 *
 * Window support code.
 */

#ifndef CASHBOOK_WINDOW
#define CASHBOOK_WINDOW

#include "currency.h"
#include "date.h"
#include "interest.h"

/* ==================================================================================================================
 * Static constants
 */

/**
 * The length of a window title buffer.
 */

#define WINDOW_TITLE_LENGTH 256

#define X_WINDOW_PERCENT_LIMIT 98
#define Y_WINDOW_PERCENT_LIMIT 40
#define Y_WINDOW_PERCENT_ORIGIN 75

/**
 * The height of a row icon in a window table.
 */

#define WINDOW_ROW_ICON_HEIGHT 36

/**
 * The horizontal spacing between rows in a window table.
 */

#define WINDOW_ROW_GUTTER 4

#define AUTO_SCROLL_MARGIN 20

#define CHILD_WINDOW_OFFSET 12
#define CHILD_WINDOW_X_OFFSET 128
#define CHILD_WINDOW_X_OFFSET_LIMIT 4


/**
 * The horizontal scroll step size, in OS units.
 */

#define HORIZONTAL_SCROLL 16


#define WINDOW_ROW_HEIGHT (WINDOW_ROW_ICON_HEIGHT + WINDOW_ROW_GUTTER)

/**
 * Calculate the first row to be included in a redraw operation.
 */

#define WINDOW_REDRAW_TOP(toolbar, y) (((y) - (toolbar)) / WINDOW_ROW_HEIGHT)

/**
 * Calculate the last row to be included in a redraw operation.
 */

#define WINDOW_REDRAW_BASE(toolbar, y) (((WINDOW_ROW_HEIGHT * 1.5) + (y) - (toolbar)) / WINDOW_ROW_HEIGHT)

/**
 * Calculate the base of a row in a table view.
 */

#define WINDOW_ROW_BASE(toolbar, y) ((-((y) + 1) * WINDOW_ROW_HEIGHT) - (toolbar))

/**
 * Calculate the top of a row in a table view.
 */

#define WINDOW_ROW_TOP(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar))

/**
 * Calculate the base of an icon in a table view.
 */

#define WINDOW_ROW_Y0(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar) - WINDOW_ROW_ICON_HEIGHT)

/**
 * Calculate the top of an icon in a table view.
 */

#define WINDOW_ROW_Y1(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar))

/**
 * Calculate the raw row number based on a window mouse coordinate.
 */

#define WINDOW_ROW(toolbar, y) (((-(y)) - (toolbar)) / WINDOW_ROW_HEIGHT)

/**
 * Caluclate the position within a row, given a window mouse coordinate.
 */

#define WINDOW_ROW_Y_POS(toolbar, y) (((-(y)) - (toolbar)) % WINDOW_ROW_HEIGHT)

/* Return true or false if a ROW_Y_POS() value is above or below the icon
 * area of the row.
 */

#define WINDOW_ROW_BELOW(y) ((y) < WINDOW_ROW_GUTTER)
#define WINDOW_ROW_ABOVE(y) ((y) > WINDOW_ROW_HEIGHT)


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

void window_set_initial_area(wimp_window *window, int width, int height, int x, int y, int yoff);


/**
 * Process data from a scroll event, updating the window position in the
 * associated data block as required.
 *
 * \param *scroll		The scroll event data to be processed.
 * \param pane_szie		The size, in OS units, of any toolbar and footer panes.
 */

void window_process_scroll_effect(wimp_scroll *scroll, int pane_size);


/**
 * Set an extent for a table window.
 * 
 * \param window		The window to set the extent for.
 * \param lines			The number of lines to display in the new window.
 * \param pane_height		The height of any toolbar and footer panes.
 * \param width			The width of the window, in OS units.
 */

void window_set_extent(wimp_w window, int lines, int pane_height, int width);


/**
 * Calculate the row that the mouse was clicked over in the list window.
 *
 * \param *pointer		The relevant Wimp pointer data.
 * \param *state		The relevant Wimp window state.
 * \param toolbar_height	The height of the window's toolbar, in OS units.
 * \param max_lines		The maximum number of lines in the window, or -1 for no constraint.
 * \return			The row (from 0) or -1 if none.
 */

int window_calculate_click_row(os_coord *pos, wimp_window_state *state, int toolbar_height, int max_lines);


/**
 * Calculate a window's plot area from the readrw clip rectangle, and
 * plot the background colour into the window.
 *
 * \param *redraw		The Wimp Redraw data block.
 * \param toolbar_height	The height of the window's toolbar, in OS Units.
 * \param background		The Wimp colour to plot the background.
 * \param *top			Pointer to variable to take the first redraw
 *				line, or NULL.
 * \param *base			Pointer to variable to take the last redraw
 *				line, or NULL.
 */

void window_plot_background(wimp_draw *redraw, int toolbar_height, wimp_colour background, int *top, int *base);

/**
 * Initialise a window template for use by the icon plotting interface.
 * 
 * It is assumed that all of the icons in the template have valid indirection
 * data set up for them, including buffer sizes.
 *
 * \param *definition		Pointer to the window template.
 */

void window_set_icon_templates(wimp_window *definition);


/**
 * Plot an empty field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 */
 
void window_plot_empty_field(wimp_i field);


/**
 * Plot a text field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param *text			Pointer to the text to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_text_field(wimp_i field, char *text, wimp_colour colour);


/**
 * Plot an integer field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param number		The integer value to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_int_field(wimp_i field, int number, wimp_colour colour);


/**
 * Plot a single character field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param character		The single character to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_char_field(wimp_i field, char character, wimp_colour colour);


/**
 * Plot a reconciled flag field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param reconciled		The reconciled state (yes or no) to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_reconciled_field(wimp_i field, osbool reconciled, wimp_colour colour);


/**
 * Plot a date field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param date			The date to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_date_field(wimp_i field, date_t date, wimp_colour colour);


/**
 * Plot a currency field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param amount		The currency amount to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_currency_field(wimp_i field, amt_t amount, wimp_colour colour);


/**
 * Plot an interest rate field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param rate			The interest rate amount to be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_interest_rate_field(wimp_i field, rate_t rate, wimp_colour colour);


/**
 * Plot a message token field from the icon plotting template.
 *
 * \param field			The field icon to plot.
 * \param token			The token of the message be plotted in the field.
 * \param colour		The foreground colour to plot the icon text in.
 */
 
void window_plot_message_field(wimp_i field, char *token, wimp_colour colour);

#endif
