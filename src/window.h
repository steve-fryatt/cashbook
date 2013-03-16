/* Copyright 2003-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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

/* ==================================================================================================================
 * Static constants
 */

#define X_WINDOW_PERCENT_LIMIT 98
#define Y_WINDOW_PERCENT_LIMIT 40
#define Y_WINDOW_PERCENT_ORIGIN 75


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

void set_initial_window_area(wimp_window *window, int width, int height, int x, int y, int yoff);

#endif

