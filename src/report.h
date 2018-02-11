/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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

/**
 * The maximum length of a report line.
 */

#define REPORT_MAX_LINE_LEN 1000

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
 * \param title			TRUE to include a report title in graphics printing.
 * \param pagenum		TRUE to include page numbers in graphics printing.
 * \param grid			TRUE to include a grid in graphics printing.
 *				FALSE to print Portrait.
 */

void report_close_and_print(struct report *report, osbool text, osbool textformat, osbool fitwidth, osbool rotate, osbool title, osbool pagenum, osbool grid);


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

