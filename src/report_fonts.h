/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: report_fonts.h
 *
 * Handle fonts for a report.
 */

#ifndef CASHBOOK_REPORT_FONTS
#define CASHBOOK_REPORT_FONTS

#include <stdlib.h>
#include "oslib/types.h"
#include "report_cell.h"

/**
 * A Report Fonts instance handle.
 */

struct report_fonts_block;

/**
 * Initialise the Report Fonts module.
 */

void report_fonts_initialise(void);

/**
 * Initialise a Report Fonts block.
 *
 * \return			The block handle, or NULL on failure.
 */

struct report_fonts_block *report_fonts_create(void);


/**
 * Destroy a Report Fonts instance, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void report_fonts_destroy(struct report_fonts_block *handle);


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

void report_fonts_set_faces(struct report_fonts_block *handle, char *normal, char *bold, char *italic, char *bold_italic);


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

void report_fonts_get_faces(struct report_fonts_block *handle, char *normal, char *bold, char *italic, char *bold_italic, size_t length);


/**
 * Set the size of the fonts used in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to be updated.
 * \param size			The new font size, in 16ths of a point.
 * \param linespace		The new line spacing, as a % of font size.
 */

void report_fonts_set_size(struct report_fonts_block *handle, int size, int linespace);


/**
 * Get the size of the fonts used in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to be queried.
 * \param *size			Pointer to variable to take the font size,
 *				in 16ths of a point, or NULL.
 * \param *linespace		Pointer to variable to take the line spacing,
 *				as a % of font size, or NULL.
 */

void report_fonts_get_size(struct report_fonts_block *handle, int *size, int *linespace);


/**
 * Return the required line spacing, in OS Units, for the fonts specified
 * in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to query.
 * \return			The required line space, in OS Units.
 */

int report_fonts_get_linespace(struct report_fonts_block *handle);


/**
 * Find font handles for the faces used in a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to initialise.
 * \return			Pointer to an error block on failure; NULL
 *				on success.
 */

os_error *report_fonts_find(struct report_fonts_block *handle);


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

os_error *report_fonts_get_max_height(struct report_fonts_block *handle, int *height, int *descender);


/**
 * Release font handles for the faces used in a Report Fonts instance.
 *
 * \param *handle		The Reports Fonts instance to process.
 */

void report_fonts_lose(struct report_fonts_block *handle);


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

os_error *report_fonts_get_string_width(struct report_fonts_block *handle, char *text, enum report_cell_flags flags, int *width);


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

os_error *report_fonts_paint_text(struct report_fonts_block *handle, char *text, int x, int y, enum report_cell_flags flags);


/**
 * Declare the fonts in a Report Fonts instance to the printing system.
 *
 * \param *handle		The Report Fonts instance to declare.
 * \return			An OS Error block pointer, or NULL on success.
 */

os_error *report_fonts_declare(struct report_fonts_block *handle);


/**
 * Set the rendering colours for a Report Fonts instance.
 *
 * \param *handle		The Report Fonts instance to set.
 * \param fill			The required fill colour.
 * \param bg_hint		The anti-aliasing hint colour.
 * \return			A pointer to an OS Error block, or NULL on success.
 */

os_error *report_fonts_set_colour(struct report_fonts_block *handle, os_colour fill, os_colour bg_hint);

#endif

