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
 * \file: report_draw.c
 *
 * Draw report objects to screen or paper implementation.
 */

/* ANSI C header files */

#include <stdlib.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/draw.h"
#include "oslib/os.h"

/* SF-Lib header files. */

/* Application header files */

#include "report_draw.h"


/**
 * The size of the Draw Path buffer, in words.
 */

#define REPORT_DRAW_BUFFER_LENGTH 32

/**
 * Buffer to hold the current Draw Path.
 */

static bits report_draw_path[REPORT_DRAW_BUFFER_LENGTH];

/**
 * Length of the current Draw Path, in words.
 */

static size_t report_draw_path_length = 0;

/**
 * The required line thickness, in Draw units.
 */

static int report_draw_line_thickness = 1024;

/* Static Function Prototypes. */

static osbool report_draw_add_move(int x, int y);
static osbool report_draw_add_line(int x, int y);
static osbool report_draw_close_subpath(void);
static osbool report_draw_end_path(void);
static draw_path_element *report_draw_get_new_element(size_t element_size);


/**
 * Set the line width for subsequent plot operations.
 *
 * \param width			The required width, in OS Units.
 */

void report_draw_set_line_width(int width)
{
	report_draw_line_thickness = (width << 8);
}


/**
 * Draw a rectangle on screen.
 *
 * \param *outline		The rectangle outline, in absolute OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *report_draw_box(os_box *outline)
{
	bits			dash_data[3];
	draw_dash_pattern	*dash_pattern = (draw_dash_pattern *) dash_data;

	if (outline == NULL)
		return NULL;

	dash_pattern->start = (4 << 8);
	dash_pattern->element_count = 1;
	dash_pattern->elements[0] = (4 << 8);

	static const draw_line_style line_style = { draw_JOIN_MITRED,
			draw_CAP_BUTT, draw_CAP_BUTT, 0, 0x7fffffff,
			0, 0, 0, 0 };

	report_draw_path_length = 0;

	if (!report_draw_add_move(outline->x0, outline->y0))
		return NULL;

	if (!report_draw_add_line(outline->x1, outline->y0))
		return NULL;

	if (!report_draw_add_line(outline->x1, outline->y1))
		return NULL;

	if (!report_draw_add_line(outline->x0, outline->y1))
		return NULL;

	if (!report_draw_add_line(outline->x0, outline->y0))
		return NULL;

	if (!report_draw_close_subpath())
		return NULL;

	if (!report_draw_end_path())
		return NULL;

	return xdraw_stroke((draw_path*) report_draw_path, draw_FILL_NONZERO, NULL,
			0, report_draw_line_thickness, &line_style, dash_pattern);
}


/**
 * Draw a line on screen.
 *
 * \param x0			The start X coordinate, in OS Units.
 * \param y0			The start Y coordinate, in OS Units.
 * \param x1			The end X coordinate, in OS Units.
 * \param y1			The end Y coordinate, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *report_draw_line(int x0, int y0, int x1, int y1)
{
	static const draw_line_style line_style = { draw_JOIN_MITRED,
			draw_CAP_SQUARE, draw_CAP_SQUARE, 0, 0x7fffffff,
			0, 0, 0, 0 };

	report_draw_path_length = 0;

	if (!report_draw_add_move(x0, y0))
		return NULL;

	if (!report_draw_add_line(x1, y1))
		return NULL;

	if (!report_draw_end_path())
		return NULL;

	return xdraw_stroke((draw_path*) report_draw_path, draw_FILL_NONZERO, NULL,
			0, report_draw_line_thickness, &line_style, NULL);
}


/**
 * Add a move to the current Draw Path.
 *
 * \param x			The X coordinate to move to, in OS Units.
 * \param y			The Y coordinate to move to, in OS Units.
 * \return			TRUE on success, FALSE on failure.
 */

static osbool report_draw_add_move(int x, int y)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(3);
	if (element == NULL)
		return FALSE;

	element->tag = draw_MOVE_TO;
	element->data.move_to.x = (x << 8);
	element->data.move_to.y = (y << 8);

	return TRUE;
}


/**
 * Add a line to the current Draw Path.
 *
 * \param x			The X coordinate to draw to, in OS Units.
 * \param y			The Y coordinate to draw to, in OS Units.
 * \return			TRUE on success, FALSE on failure.
 */

static osbool report_draw_add_line(int x, int y)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(3);
	if (element == NULL)
		return FALSE;

	element->tag = draw_LINE_TO;
	element->data.line_to.x = (x << 8);
	element->data.line_to.y = (y << 8);

	return TRUE;
}


/**
 * Close the current subpath in the Draw Path.
 *
 * \return			TRUE on success, FALSE on failure.
 */

static osbool report_draw_close_subpath(void)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(1);
	if (element == NULL)
		return FALSE;

	element->tag = draw_CLOSE_LINE;

	return TRUE;
}


/**
 * End the current Draw Path.
 *
 * \return			TRUE on success, FALSE on failure.
 */

static osbool report_draw_end_path(void)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(2);
	if (element == NULL)
		return FALSE;

	element->tag = draw_END_PATH;
	element->data.end_path = 0;

	return TRUE;
}


/**
 * Claim storage for a new Draw Path element from the data block.
 *
 * \param element_size		The required element size, in 32-bit words.
 * \return			Pointer to the new element, or NULL on failure.
 */

static draw_path_element *report_draw_get_new_element(size_t element_size)
{
	draw_path_element	*element;

	if (report_draw_path_length + element_size > REPORT_DRAW_BUFFER_LENGTH)
		return NULL;

	element = (draw_path_element *) (report_draw_path + report_draw_path_length);
	report_draw_path_length += element_size;

	return element;
}

