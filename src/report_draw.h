/* Copyright 2017-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: report_draw.h
 *
 * Draw report objects to screen or paper interface.
 */

#ifndef CASHBOOK_REPORT_DRAW
#define CASHBOOK_REPORT_DRAW

/**
 * Set the line width for subsequent plot operations.
 *
 * \param width			The required width, in OS Units.
 */

void report_draw_set_line_width(int width);


/**
 * Draw a rectangle on screen.
 *
 * \param *outline		The rectangle outline, in absolute OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *report_draw_box(os_box *outline);


/**
 * Draw a line on screen.
 *
 * \param x0			The start X coordinate, in OS Units.
 * \param y0			The start Y coordinate, in OS Units.
 * \param x1			The end X coordinate, in OS Units.
 * \param y1			The end Y coordinate, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *report_draw_line(int x0, int y0, int x1, int y1);

#endif

