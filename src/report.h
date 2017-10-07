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
 * \file: report.h
 *
 * High-level report generator interface.
 */

#ifndef CASHBOOK_REPORT
#define CASHBOOK_REPORT

/* Report data block format consists of a series of lines as follows:
 *
 * <tab-bar-data><format flag>text[\t<format flag>text]\0
 *
 * The line pointer array consists of a series of offsets pointing to the tab-bar-data at the start of each line.
 *
 * The formatting flags indicate things like bold, italic, right align, etc for each individual field.  The
 * tab-bar-data is an integer showing which tab bar is to be used to space out the line.
 */

/* ==================================================================================================================
 * Static constants
 */


/* Report formatting flags. */

#define REPORT_FLAG_BYTES 1 /* The length of the formatting flag data. */

#define REPORT_FLAG_INDENT       0x01 /* The item is indented into the column. */
#define REPORT_FLAG_BOLD         0x02 /* The item is in bold. */
#define REPORT_FLAG_UNDER        0x04 /* The item is underlined. */
#define REPORT_FLAG_RIGHT        0x08 /* The item is right aligned. */
#define REPORT_FLAG_NUMERIC      0x10 /* The item is numeric. */
#define REPORT_FLAG_SPILL        0x20 /* The column is spill from adjacent columns on the left. */
#define REPORT_FLAG_KEEPTOGETHER 0x40 /* The row is part of a keep-together block, the first line of which is to be repeated on page breaks. */
#define REPORT_FLAG_NOTNULL      0x80 /* Used to prevent the flag byte being a null terminator. */


/**
 * The maximum length of a report line.
 */

#define REPORT_MAX_LINE_LEN 1000

/**
 * The maximum number of tabs stops in a tab bar.
 */

#define REPORT_TAB_STOPS 20

/**
 * The maximum length of a font name.
 */

#define REPORT_MAX_FONT_NAME 128

/* Layout details */

#define REPORT_BASELINE_OFFSET 4
#define REPORT_COLUMN_SPACE 20
#define REPORT_COLUMN_INDENT 40
#define REPORT_BOTTOM_MARGIN 4
#define REPORT_LEFT_MARGIN 4
#define REPORT_RIGHT_MARGIN 4
#define REPORT_RULE_SPACE 6
#define REPORT_MIN_WIDTH 1000
#define REPORT_MIN_HEIGHT 800

#define REPORT_TEXT_COLUMN_SPACE 1
#define REPORT_TEXT_COLUMN_INDENT 2



/**
 * Initialise the reporting system.
 *
 * \param *sprites		The application sprite area.
 */

void report_initialise(osspriteop_area *sprites);


/**
 * Create a new report, and return a handle ready to write data to.
 *
 * \param *file			The file to which the report belongs.
 * \param *title		The window title for the report.
 * \param *template		Pointer to the template used for the report, or
 *				NULL for none.
 * \return			Report handle, or NULL on failure.
 */

struct report *report_open(struct file_block *file, char *title, struct analysis_report *template);


/**
 * Close off a report that has had data written to it, and open a window
 * to display it on screen.
 *
 * \param *report		The report handle.
 */

void report_close(struct report *report);


/**
 * Close off a report that has had data written to it, and send it directly
 * to the printing system before deleting it.
 *
 * \param *report		The report handle.
 * \param text			TRUE to print in text mode; FALSE for graphics.
 * \param textformat		TRUE to apply formatting to text mode printing.
 * \param fitwidth		TRUE to fit graphics printing to page width.
 * \param rotate		TRUE to rotate grapchis printing to Landscape;
 * \param pagenum		TRUE to include page numbers in graphics printing.
 *				FALSE to print Portrait.
 */

void report_close_and_print(struct report *report, osbool text, osbool textformat, osbool fitwidth, osbool rotate, osbool pagenum);


/**
 * Delete a report block, along with any window and data associated with
 * it.
 *
 * \param *report		The report to delete.
 */

void report_delete(struct report *report);


/**
 * Write a lone of text to an open report.  Lines are NULL terminated, and can
 * contain the following inline commands:
 *
 *	\t - Tab (start a new column)
 *	\i - Indent the text in the current 'cell'
 *	\b - Format this 'cell' bold
 *	\u - Format this 'cell' underlined
 *	\d - This cell contains a number
 *	\r - Right-align the text in this 'cell'
 *	\s - Spill text from the previous cell into this one.
 *	\h - This line is a heading.
 *
 * \param *report		The report to write to.
 * \param bar			The tab bar to use.
 * \param *text			The line of text to write.
 */

void report_write_line(struct report *report, int bar, char *text);


/**
 * Test for any pending report print jobs on the specified file.
 *
 * \param *file			The file to test.
 * \return			TRUE if there are pending jobs; FALSE if not.
 */

osbool report_get_pending_print_jobs(struct file_block *file);


/**
 * Force the readraw of all the open reports associated with a file.
 *
 * \param *file			The file on which to force a redraw.
 */

void report_redraw_all(struct file_block *file);


/**
 * Call a callback function to process the template stored in every currently
 * open report.
 *
 * \param *file			The file to process.
 * \param *callback		The callback function to call.
 * \param *data			Data to pass to the callback function.
 */

void report_process_all_templates(struct file_block *file, void (*callback)(struct analysis_report *template, void *data), void *data);

#endif

