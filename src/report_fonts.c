/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
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
 * \file: report_fonts.c
 *
 * Handle fonts for a report.
 */

/* ANSI C Header files. */

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

/* Acorn C Header files. */

/* SFLib Header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* OSLib Header files. */

#include "oslib/colourtrans.h"
#include "oslib/font.h"
#include "oslib/pdriver.h"
#include "oslib/types.h"

/* Application header files. */

#include "report_fonts.h"

#include "report_cell.h"

/**
 * A single font definition.
 */

struct report_fonts_face {
	char		name[font_NAME_LIMIT];				/**< The name of the font.						*/
	char		*default_name;					/**< The name of the default fallback font.				*/
	font_f		handle;						/**< The handle of the font, if open.					*/
	osbool		open;						/**< TRUE if the font handle is valid; otherwise FALSE.			*/
};

/**
 * A Report Fonts instance data block.
 */

struct report_fonts_block {
	struct report_fonts_face	normal;				/**< The normal font face.						*/
	struct report_fonts_face	bold;				/**< The bold font face.						*/
	struct report_fonts_face	italic;				/**< The italic font face.						*/
	struct report_fonts_face	bold_italic;			/**< The bold-italic font face.						*/

	int				size;				/**< Font size in 1/16 points.						*/
	int				linespace;			/**< Line spacing as a % of font size.					*/

	int				max_height;			/**< The maximum font height encountered, in millipoints.		*/
	int				max_descender;			/**< The maximum font descender encountered, in millipoints.		*/
};

/**
 * The size of buffer allocated to strings for painting.
 */

#define REPORT_FONTS_BUFFER_SIZE 1010

/**
 * The number of bytes at the start of the buffer used for control.
 */

#define REPORT_FONTS_BUFFER_PREFIX 3

/**
 * The buffer to use when modifying text for painting.
 */

static char report_fonts_buffer[REPORT_FONTS_BUFFER_SIZE];

/**
 * The coordinate buffer to use when scanning strings.
 */

static font_scan_block report_fonts_scan_block;

/* Static Function Prototypes. */

static font_f report_fonts_get_handle(struct report_fonts_block *handle, enum report_cell_flags flags);
static void report_fonts_initialise_face(struct report_fonts_face *face, char *default_name);
static void report_fonts_set_face(struct report_fonts_face *face, char *name);
static void report_fonts_get_face(struct report_fonts_face *face, char *name, size_t length);
static os_error *report_fonts_find_face(struct report_fonts_face *face, int size);
static void report_fonts_lose_face(struct report_fonts_face *face);
static os_error *report_fonts_declare_face(struct report_fonts_face *face);
static os_error *report_fonts_set_face_colour(struct report_fonts_face *face, os_colour fill, os_colour bg_hint);


/**
 * Initialise the Report Fonts module.
 */

void report_fonts_initialise(void)
{
	report_fonts_buffer[0] = font_COMMAND_UNDERLINE;
	report_fonts_buffer[1] = 230;
	report_fonts_buffer[2] = 18;

	report_fonts_scan_block.space.x = 0;
	report_fonts_scan_block.space.y = 0;

	report_fonts_scan_block.letter.x = 0;
	report_fonts_scan_block.letter.y = 0;

	report_fonts_scan_block.split_char = -1;
}


/**
 * Initialise a Report Fonts block.
 *
 * \return			The block handle, or NULL on failure.
 */

struct report_fonts_block *report_fonts_create(void)
{
	struct report_fonts_block	*new;

	new = heap_alloc(sizeof(struct report_fonts_block));
	if (new == NULL)
		return NULL;

	/* Set up the font faces. */

	report_fonts_initialise_face(&(new->normal), "Homerton.Medium");
	report_fonts_initialise_face(&(new->bold), "Homerton.Bold");
	report_fonts_initialise_face(&(new->italic), "Homerton.Medium.Oblique");
	report_fonts_initialise_face(&(new->bold_italic), "Homerton.Bold.Oblique");

	new->size = 12 * 16;
	new->linespace = 130;
	new->max_height = 0;
	new->max_descender = 0;

	return new;
}


/**
 * Destroy a Report Fonts instance, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_fonts_destroy(struct report_fonts_block *handle)
{
	if (handle == NULL)
		return;

	heap_free(handle);
}


/**
 * Set the names of some or all of the font faces in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to be updated.
 * \param *normal		Pointer to the name of the normal font face,
 *				or NULL to leave as-is.
 * \param *bold			Pointer to the name of the bold font face,
 *				or NULL to leave as-is.
 * \param *italic		Pointer to the name of the italic font face,
 *				or NULL to leave as-is.
 * \param *bold_italic		Pointer to the name of the bold-italic font
 *				face, or NULL to leave as-is.
 */

void report_fonts_set_faces(struct report_fonts_block *handle, char *normal, char *bold, char *italic, char *bold_italic)
{
	if (handle == NULL)
		return;

	if (normal != NULL)
		report_fonts_set_face(&(handle->normal), normal);

	if (bold != NULL)
		report_fonts_set_face(&(handle->bold), bold);

	if (italic != NULL)
		report_fonts_set_face(&(handle->italic), italic);

	if (bold_italic != NULL)
		report_fonts_set_face(&(handle->bold_italic), bold_italic);
}


/**
 * Get the names of some or all of the font faces in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to be queried.
 * \param *normal		Pointer to a buffer to take the normal
 *				font name, or NULL.
 * \param *bold			Pointer to a buffer to take the bold
 *				font name, or NULL.
 * \param *italic		Pointer to a buffer to take the italic
 *				font name, or NULL.
 * \param *bold_italic		Pointer to a buffer to take the bold-italic
 *				font name, or NULL.
 * \param length		The length of the supplied buffers.
 */

void report_fonts_get_faces(struct report_fonts_block *handle, char *normal, char *bold, char *italic, char *bold_italic, size_t length)
{
	if (handle == NULL)
		return;

	if (normal != NULL)
		report_fonts_get_face(&(handle->normal), normal, length);

	if (bold != NULL)
		report_fonts_get_face(&(handle->bold), bold, length);

	if (italic != NULL)
		report_fonts_get_face(&(handle->italic), italic, length);

	if (bold_italic != NULL)
		report_fonts_get_face(&(handle->bold_italic), bold_italic, length);
}


/**
 * Set the size of the fonts used in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to be updated.
 * \param size			The new font size, in 16ths of a point.
 * \param linespace		The new line spacing, as a % of font size.
 */

void report_fonts_set_size(struct report_fonts_block *handle, int size, int linespace)
{
	if (handle == NULL)
		return;

	handle->size = size;
	handle->linespace = linespace;
}


/**
 * Get the size of the fonts used in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to be queried.
 * \param *size			Pointer to variable to take the font size,
 *				in 16ths of a point, or NULL.
 * \param *linespace		Pointer to variable to take the line spacing,
 *				as a % of font size, or NULL.
 */

void report_fonts_get_size(struct report_fonts_block *handle, int *size, int *linespace)
{
	if (handle == NULL)
		return;

	if (size != NULL)
		*size = handle->size;

	if (linespace != NULL)
		*linespace = handle->linespace;
}


/**
 * Return the required line spacing, in OS Units, for the fonts specified
 * in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to query.
 * \return			The required line space, in OS Units.
 */

int report_fonts_get_linespace(struct report_fonts_block *handle)
{
	int linespace;

	if (handle == NULL)
		return 0;

	font_convertto_os(1000 * (handle->size / 16) * handle->linespace / 100, 0, &linespace, NULL);

	return linespace;
}


/**
 * Find font handles for the faces used in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to initialise.
 * \return			Pointer to an error block on failure; NULL
 *				on success.
 */

os_error *report_fonts_find(struct report_fonts_block *handle)
{
	os_error	*error;

	if (handle == NULL)
		return NULL;

	handle->max_height = 0;
	handle->max_descender = 0;

	error = report_fonts_find_face(&(handle->normal), handle->size);
	if (error != NULL)
		return error;

	error = report_fonts_find_face(&(handle->bold), handle->size);
	if (error != NULL)
		return error;

	error = report_fonts_find_face(&(handle->italic), handle->size);
	if (error != NULL)
		return error;

	error = report_fonts_find_face(&(handle->bold_italic), handle->size);
	if (error != NULL)
		return error;

	return NULL;
}


/**
 * Return the greatest line and descender height encountered during any
 * call to report_fonts_get_string_width() since the last call to
 * report_fonts_find(), in OS Units.
 *
 * \param *handle		The Reports Fonts instance to query
 * \param *height		Pointer to variable to take the height.
 * \param *descender		Pointer to variable to take descender height.
 * \return			Pointer to an error block on failure; NULL
 *				on success.
 */

os_error *report_fonts_get_max_height(struct report_fonts_block *handle, int *height, int *descender)
{
	os_error	*error;

	if (handle == NULL)
		return NULL;

	if (height != NULL) {
		error = xfont_convertto_os(0, handle->max_height, NULL, height);
		if (error != NULL)
			return error;
	}

	if (descender != NULL) {
		error = xfont_convertto_os(0, -handle->max_descender, NULL, descender);
		if (error != NULL)
			return error;
	}

	return NULL;
}


/**
 * Release font handles for the faces used in a Report Fonts instance.
 *
 * \param *handle		The Reports Fonts instance to process.
 */

void report_fonts_lose(struct report_fonts_block *handle)
{
	if (handle == NULL)
		return;

	report_fonts_lose_face(&(handle->normal));
	report_fonts_lose_face(&(handle->bold));
	report_fonts_lose_face(&(handle->italic));
	report_fonts_lose_face(&(handle->bold_italic));
}


/**
 * Return the width of a string in a given font, taking into account any
 * cell formatting which is applied.
 *
 * \param *handle		The Reports Fonts instance to use.
 * \param *text			Pointer to the text to measure.
 * \param flags			The cell formatting flags to be applied.
 * \param *width		Pointer to a variable to text width, in OS units, or NULL.
 * \return			A pointer to an OS Error block, or NULL on success.
 */

os_error *report_fonts_get_string_width(struct report_fonts_block *handle, char *text, enum report_cell_flags flags, int *width)
{
	os_error	*error;
	font_f		font;

	if (width != NULL)
		*width = 0;

	if (handle == NULL || width == NULL)
		return NULL;

	font = report_fonts_get_handle(handle, flags);
	if (font == font_SYSTEM)
		return NULL;

	error = xfont_scan_string(font, text, font_KERN | font_GIVEN_FONT | font_GIVEN_BLOCK | font_RETURN_BBOX,
			0x7fffffff, 0x7fffffff, &report_fonts_scan_block, NULL, 0, NULL, NULL, NULL, NULL);
	if (error != NULL)
		return error;

	if ((report_fonts_scan_block.bbox.y1 - report_fonts_scan_block.bbox.y0) > handle->max_height)
		handle->max_height = report_fonts_scan_block.bbox.y1 - report_fonts_scan_block.bbox.y0;

	if (report_fonts_scan_block.bbox.y0 < handle->max_descender)
		handle->max_descender = report_fonts_scan_block.bbox.y0;

	return xfont_convertto_os(report_fonts_scan_block.bbox.x1 - report_fonts_scan_block.bbox.x0, 0, width, NULL);
}


/**
 * Paint text in a given font, taking into account any cell formatting which
 * is applied.
 *
 * \param *handle		The Reports Fonts instance to use.
 * \param *text			Pointer to the text to paint.
 * \param x			The X position of the text, in OS Units.
 * \param y			The Y position of the text, in OS Units.
 * \param flags			The cell formatting flags to be applied.
 * \return			A pointer to an OS Error block, or NULL on success.
 */

os_error *report_fonts_paint_text(struct report_fonts_block *handle, char *text, int x, int y, enum report_cell_flags flags)
{
	font_f		font;

	if (handle == NULL)
		return NULL;

	font = report_fonts_get_handle(handle, flags);
	if (font == font_SYSTEM)
		return NULL;

	/* If we're underlining the text, copy it into a buffer which
	 * already has the "underline on" sequence at the start.
	 */

	if (flags & REPORT_CELL_FLAGS_UNDERLINE) {
		string_copy(report_fonts_buffer + REPORT_FONTS_BUFFER_PREFIX, text,
				REPORT_FONTS_BUFFER_SIZE - REPORT_FONTS_BUFFER_PREFIX);
		text = report_fonts_buffer;
	}

	return xfont_paint(font, text, font_OS_UNITS | font_KERN | font_GIVEN_FONT, x, y, NULL, NULL, 0);
}


/**
 * Return a suitable font handle from the available faces, based on the
 * supplied cell content formatting flags.
 *
 * \param *hamdle		The Reports Fonts instance to use.
 * \param flags			The cell formatting flags to be applied.
 * \return			The appropriate font handle, or font_SYSTEM.
 */

static font_f report_fonts_get_handle(struct report_fonts_block *handle, enum report_cell_flags flags)
{
	if (handle == NULL)
		return font_SYSTEM;

	if ((flags & REPORT_CELL_FLAGS_BOLD) && (flags & REPORT_CELL_FLAGS_ITALIC)) {
		if (handle->bold_italic.open == FALSE)
			return font_SYSTEM;

		return handle->bold_italic.handle;
	} else if (flags & REPORT_CELL_FLAGS_BOLD) {
		if (handle->bold.open == FALSE)
			return font_SYSTEM;

		return handle->bold.handle;
	} else if (flags & REPORT_CELL_FLAGS_ITALIC) {
		if (handle->italic.open == FALSE)
			return font_SYSTEM;

		return handle->italic.handle;
	} else {
		if (handle->normal.open == FALSE)
			return font_SYSTEM;

		return handle->normal.handle;
	}

	return font_SYSTEM;
}


/**
 * Declare the fonts in a Report Fonts instance to the printing system.
 *
 * \param *handle		The Report Fonts instance to declare.
 * \return			An OS Error block pointer, or NULL on success.
 */

os_error *report_fonts_declare(struct report_fonts_block *handle)
{
	os_error	*error;

	if (handle == NULL)
		return NULL;

	error = report_fonts_declare_face(&(handle->normal));
	if (error != NULL)
		return error;

	error = report_fonts_declare_face(&(handle->bold));
	if (error != NULL)
		return error;

	error = report_fonts_declare_face(&(handle->italic));
	if (error != NULL)
		return error;

	error = report_fonts_declare_face(&(handle->bold_italic));
	if (error != NULL)
		return error;

	return xpdriver_declare_font(0, 0, pdriver_KERNED);
}


/**
 * Set the rendering colours for a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to set.
 * \param fill			The required fill colour.
 * \param bg_hint		The anti-aliasing hint colour.
 * \return			A pointer to an OS Error block, or NULL on success.
 */

os_error *report_fonts_set_colour(struct report_fonts_block *handle, os_colour fill, os_colour bg_hint)
{
	os_error	*error;

	if (handle == NULL)
		return NULL;

	error = report_fonts_set_face_colour(&(handle->normal), fill, bg_hint);
	if (error != NULL)
		return error;

	error = report_fonts_set_face_colour(&(handle->bold), fill, bg_hint);
	if (error != NULL)
		return error;

	error = report_fonts_set_face_colour(&(handle->italic), fill, bg_hint);
	if (error != NULL)
		return error;

	error = report_fonts_set_face_colour(&(handle->bold_italic), fill, bg_hint);
	if (error != NULL)
		return error;

	return NULL;
}

/**
 * Initialise a font face structure.
 *
 * \param *face			The face to be initialised.
 * \param *fallback		Pointer to the name of the default face.
 */

static void report_fonts_initialise_face(struct report_fonts_face *face, char *default_name)
{
	if (face == NULL)
		return;

	face->handle = 0;
	face->open = FALSE;
	face->default_name = default_name;

	report_fonts_set_face(face, face->default_name);
}


/**
 * Set the name of the font to be used for a font face.
 *
 * \param *face			The face to be updated.
 * \param *name			Pointer to the font name to use.
 */

static void report_fonts_set_face(struct report_fonts_face *face, char *name)
{
	if (face == NULL)
		return;

	string_copy(face->name, name, font_NAME_LIMIT);
}


/**
 * Get the name of the font to be used for a font face.
 *
 * \param *face			The face to be updated.
 * \param *name			Pointer to a buffer to take the name.
 * \param length		The size of the supplied buffer.
 */

static void report_fonts_get_face(struct report_fonts_face *face, char *name, size_t length)
{
	if (name == NULL || length == 0)
		return;

	if (face == NULL) {
		name[0] = '\0';
		return;
	}

	string_copy(name, face->name, length);
}


/**
 * Find a font handle for a given font face.
 *
 * \param *face			The face for which to find the handle.
 * \param size			The font size to open, in 16ths of a point.
 * \return			A pointer to an error on failure; NULL on success.
 */

static os_error *report_fonts_find_face(struct report_fonts_face *face, int size)
{
	os_error	*error;

	if (face == NULL || face->open == TRUE)
		return NULL;

	error = xfont_find_font(face->name, size, size, 0, 0, &face->handle, NULL, NULL);
	if (error != NULL)
		error = xfont_find_font(face->default_name, size, size, 0, 0, &face->handle, NULL, NULL);

	if (error == NULL)
		face->open = TRUE;

	return error;
}


/**
 * Release a font handle for a given font face.
 *
 * \param *face			The face for which to release the handle.
 */

static void report_fonts_lose_face(struct report_fonts_face *face)
{
	if (face == NULL || face->open == FALSE)
		return;

	font_lose_font(face->handle);

	face->open = FALSE;
}


/**
 * Declare a font to the printing system if required.
 *
 * \param *face			The face to declared.
 * \return			A pointer to an OS Error block, or NULL on success.
 */

static os_error *report_fonts_declare_face(struct report_fonts_face *face)
{
	if (face == NULL || face->open == FALSE)
		return NULL;

	return xpdriver_declare_font(face->handle, 0, pdriver_KERNED);
}


/**
 * Set the rendering colours for a font face.
 *
 * \param *face			The face to be set.
 * \param fill			The required fill colour.
 * \param bg_hint		The anti-aliasing hint colour.
 * \return			A pointer to an OS Error block, or NULL on success.
 */

static os_error *report_fonts_set_face_colour(struct report_fonts_face *face, os_colour fill, os_colour bg_hint)
{
	if (face == NULL || face->open == FALSE)
		return NULL;

	return xcolourtrans_set_font_colours(face->handle, bg_hint, fill, 14, NULL, NULL, NULL);
}

