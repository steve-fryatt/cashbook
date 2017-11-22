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


#define REPORT_DRAW_BUFFER_LENGTH 32

static bits report_draw_path[REPORT_DRAW_BUFFER_LENGTH];

static size_t report_draw_path_length = 0;


static osbool report_draw_add_move(int x, int y);
static osbool report_draw_add_line(int x, int y);
static osbool report_draw_close_subpath(void);
static osbool report_draw_end_path(void);
static draw_path_element *report_draw_get_new_element(size_t element_size);


os_error *report_draw_box(int x0, int y0, int x1, int y1)
{
	int i = 0;

	static const draw_line_style line_style = { draw_JOIN_MITRED,
			draw_CAP_BUTT, draw_CAP_BUTT, 0, 0x7fffffff,
			0, 0, 0, 0 };

	report_draw_path_length = 0;

	if (!report_draw_add_move(x0, y0))
		return NULL;

	if (!report_draw_add_line(x1, y0))
		return NULL;

	if (!report_draw_add_line(x1, y1))
		return NULL;

	if (!report_draw_add_line(x0, y1))
		return NULL;

	if (!report_draw_add_line(x0, y0))
		return NULL;

	if (!report_draw_close_subpath())
		return NULL;

	if (!report_draw_end_path())
		return NULL;

	for (i = 0; i < report_draw_path_length; i++)
		debug_printf("Draw Data at %d is %d", i, report_draw_path[i]);


	return xdraw_stroke((draw_path*) report_draw_path, draw_FILL_NONZERO, NULL, 0, 512, &line_style, NULL);
}


os_error *report_draw_line(int x0, int y0, int x1, int y1)
{
	report_draw_path_length = 0;

	if (!report_draw_add_move(x0, y0))
		return NULL;

	if (!report_draw_add_line(x1, y1))
		return NULL;

	if (!report_draw_end_path())
		return NULL;

	return xdraw_stroke((draw_path*) report_draw_path, draw_FILL_NONZERO, NULL, 0, 0, NULL, NULL);
}




static osbool report_draw_add_move(int x, int y)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(3);
	if (element == NULL)
		return FALSE;

	element->tag = draw_MOVE_TO;
	element->data.move_to.x = (x << 8);
	element->data.move_to.y = (y << 8);

	debug_printf("Writing to 0x%x", element);

	debug_printf("Adding move to %d, %d", x, y);

	return TRUE;
}


static osbool report_draw_add_line(int x, int y)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(3);
	if (element == NULL)
		return FALSE;

	element->tag = draw_LINE_TO;
	element->data.line_to.x = (x << 8);
	element->data.line_to.y = (y << 8);

	debug_printf("Writing to 0x%x", element);

	debug_printf("Adding line to %d, %d", x, y);

	return TRUE;
}

static osbool report_draw_close_subpath(void)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(1);
	if (element == NULL)
		return FALSE;

	element->tag = draw_CLOSE_LINE;

	debug_printf("Writing to 0x%x", element);

	debug_printf("Closing subpath");

	return TRUE;
}


static osbool report_draw_end_path(void)
{
	draw_path_element	*element;

	element = report_draw_get_new_element(2);
	if (element == NULL)
		return FALSE;

	element->tag = draw_END_PATH;
	element->data.end_path = 0;

	debug_printf("Writing to 0x%x", element);

	debug_printf("Closing path with %d bytes", 0);

	return TRUE;
}


static draw_path_element *report_draw_get_new_element(size_t element_size)
{
	draw_path_element	*element;

	if (report_draw_path_length + element_size > REPORT_DRAW_BUFFER_LENGTH)
		return NULL;

	element = (draw_path_element *) (report_draw_path + report_draw_path_length);
	report_draw_path_length += element_size;

	return element;
}

