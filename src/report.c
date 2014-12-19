/* Copyright 2003-2013, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: report.c
 *
 * High-level report generator implementation.
 */

/* ANSI C header files */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/font.h"
#include "oslib/hourglass.h"
#include "oslib/osfile.h"
#include "oslib/pdriver.h"
#include "oslib/os.h"
#include "oslib/osfind.h"
#include "oslib/colourtrans.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "report.h"

#include "analysis.h"
#include "caret.h"
#include "file.h"
#include "filing.h"
#include "fontlist.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "saveas.h"
#include "templates.h"
#include "window.h"


/* Report format dialogue. */

#define REPORT_FORMAT_OK 13
#define REPORT_FORMAT_CANCEL 12
#define REPORT_FORMAT_NFONT 1
#define REPORT_FORMAT_NFONTMENU 2
#define REPORT_FORMAT_BFONT 4
#define REPORT_FORMAT_BFONTMENU 5
#define REPORT_FORMAT_FONTSIZE 7
#define REPORT_FORMAT_FONTSPACE 10

/* Report view menu */

#define REPVIEW_MENU_FORMAT 0
#define REPVIEW_MENU_SAVETEXT 1
#define REPVIEW_MENU_EXPCSV 2
#define REPVIEW_MENU_EXPTSV 3
#define REPVIEW_MENU_PRINT 4
#define REPVIEW_MENU_TEMPLATE 5

enum report_page_area {
	REPORT_PAGE_NONE   = 0,
	REPORT_PAGE_BODY   = 1,
	REPORT_PAGE_HEADER = 2,
	REPORT_PAGE_FOOTER = 4
};

struct report_print_pagination {
	int		header_line;						/**< A line to repeat as a header at the top of the page, or -1 for none.			*/
	int		first_line;						/**< The first non-repeated line on the page from the report document.				*/
	int		line_count;						/**< The total line count on the page, including a repeated header.				*/
};

/* Report status flags. */

enum report_status {
	REPORT_STATUS_NONE = 0x00,						/**< No status flags set.				*/
	REPORT_STATUS_MEMERR = 0x01,						/**< A memory allocation error has occurred, so stop allowing writes. */
	REPORT_STATUS_CLOSED = 0x02						/**< The report has been closed to writing.		*/
};

struct report {
	file_data		*file;						/**< The file that the report belongs to.		*/

	wimp_w			window;
	char			window_title[256];

	/* Report status flags. */

	enum report_status	flags;
	int			print_pending;

	/* Tab details */

	int			font_width[REPORT_TAB_BARS][REPORT_TAB_STOPS];	/**< Column widths in OS units for outline fonts.	*/
	int			text_width[REPORT_TAB_BARS][REPORT_TAB_STOPS];	/**< Column widths in characters for ASCII text.	*/

	int			font_tab[REPORT_TAB_BARS][REPORT_TAB_STOPS];	/**< Tab stops in OS units for outline fonts.		*/

	/* Font data */

	char			font_normal[MAX_REP_FONT_NAME];			/**< Name of 'normal' outline font.			*/
	char			font_bold[MAX_REP_FONT_NAME];			/**< Name of bold outline font.				*/
	int			font_size;					/**< Font size in 1/16 points.				*/
	int			line_spacing;					/**< Line spacing in percent.				*/

	/* Report content */

	int			lines;						/**< The number of lines in the report.			*/
	int			max_lines;					/**< The size of the line pointer block.		*/

	int			width;						/**< The displayed width of the report data.		*/
	int			height;						/**< The displayed height of the report data.		*/

	int			block_size;					/**< The size of the data block.			*/
	int			data_size;					/**< The size of the data in the block.			*/

	char			*data;						/**< The data block itself (flex block).		*/
	int			*line_ptr;					/**< The line pointer block (flex block).		*/

	/* Report template details. */

	struct analysis_report	*template;					/**< Pointer to an analysis report template.		*/

	/* Pointer to the next report in the list. */

	struct report		*next;
};


struct report			*report_format_report = NULL;			/**< The report to which the currently open Report Format window belongs.			*/
struct report			*report_print_report = NULL;			/**< The report to which the currently open Report Print dialogie belongs.			*/

static osbool			report_print_opt_text;				/**< TRUE if the current report is to be printed text format; FALSE to print graphically.	*/
static osbool			report_print_opt_textformat;			/**< TRUE if text formatting should be applied to the current text format print; else FALSE.	*/
static osbool			report_print_opt_fitwidth;			/**< TRUE if the graphics format print should be fitted to page width; else FALSE.		*/
static osbool			report_print_opt_rotate;			/**< TRUE if the graphics format print should be rotated to Landscape; FALSE for Portrait.	*/
static osbool			report_print_opt_pagenum;			/**< TRUE if the graphics format print should include page numbers; else FALSE.			*/

wimp_window			*report_window_def = NULL;			/**< The definition for the Report View window.							*/

static wimp_menu		*report_view_menu = NULL;			/**< The Report View window menu handle.							*/

static wimp_w			report_format_window = NULL;			/**< Window handle of the Report Format window.							*/

static wimp_menu		*report_format_font_menu = NULL;		/**< The font menu handle.									*/
static wimp_i			report_format_font_icon = -1;			/**< The pop-up icon which opened the font menu.						*/

static struct saveas_block	*report_saveas_text = NULL;			/**< The Save Text saveas data handle.								*/
static struct saveas_block	*report_saveas_csv = NULL;			/**< The Save CSV saveas data handle.								*/
static struct saveas_block	*report_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.								*/


static int			report_reflow_content(struct report *report);
static osbool			report_find_fonts(struct report *report, font_f *normal, font_f *bold);

static void			report_view_close_window_handler(wimp_close *close);
static void			report_view_redraw_handler(wimp_draw *redraw);
static void			report_view_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void			report_view_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void			report_view_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void			report_view_redraw_handler(wimp_draw *redraw);

static void			report_open_format_window(struct report *report, wimp_pointer *ptr);
static void			report_format_click_handler(wimp_pointer *pointer);
static osbool			report_format_keypress_handler(wimp_key *key);
static void			report_format_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void			report_format_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void			report_format_menu_close_handler(wimp_w w, wimp_menu *menu);
static void			report_refresh_format_window(void);
static void			report_fill_format_window(struct report *report);
static void			report_process_format_window(void);

static void			report_open_print_window(struct report *report, wimp_pointer *ptr, osbool restore);
static void			report_print_window_closed(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum);

static osbool			report_save_text(char *filename, osbool selection, void *data);
static osbool			report_save_csv(char *filename, osbool selection, void *data);
static osbool			report_save_tsv(char *filename, osbool selection, void *data);
static void			report_export_text(struct report *report, char *filename, osbool formatting);
static void			report_export_delimited(struct report *report, char *filename, enum filing_delimit_type format, int filetype);

static void			report_start_print_job(char *filename);
static void			report_cancel_print_job(void);
static void			report_print_as_graphic(struct report *report, osbool fit_width, osbool rotate, osbool pagenum);
static void			report_handle_print_error(os_error *error, os_fw file, font_f f1, font_f f2);
static os_error			*report_plot_line(struct report *report, unsigned int line, int x, int y, font_f normal, font_f bold);
static enum report_page_area	report_get_page_areas(osbool rotate, os_box *body, os_box *header, os_box *footer, unsigned header_size, unsigned footer_size);


/**
 * Initialise the reporting system.
 *
 * \param *sprites		The application sprite area.
 */

void report_initialise(osspriteop_area *sprites)
{
	/* Report Format Window. */

	report_format_window = templates_create_window("RepFormat");
	ihelp_add_window (report_format_window, "RepFormat", NULL);
	event_add_window_mouse_event(report_format_window, report_format_click_handler);
	event_add_window_key_event(report_format_window, report_format_keypress_handler);
	event_add_window_menu_prepare(report_format_window, report_format_menu_prepare_handler);
	event_add_window_menu_selection(report_format_window, report_format_menu_selection_handler);
	event_add_window_menu_close(report_format_window, report_format_menu_close_handler);
	event_add_window_icon_popup(report_format_window, REPORT_FORMAT_NFONTMENU, report_format_font_menu, -1, NULL);
	event_add_window_icon_popup(report_format_window, REPORT_FORMAT_BFONTMENU, report_format_font_menu, -1, NULL);

	/* Report View Window. */

	report_window_def = templates_load_window("Report");
	report_window_def->sprite_area = sprites;

	report_view_menu = templates_get_menu(TEMPLATES_MENU_REPORT);

	/* Save dialogue boxes. */

	report_saveas_text = saveas_create_dialogue(FALSE, "file_fff", report_save_text);
	report_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", report_save_csv);
	report_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", report_save_tsv);
}


/**
 * Create a new report, and return a handle ready to write data to.
 *
 * \param *file			The file to which the report belongs.
 * \param *title		The window title for the report.
 * \param *template		Pointer to the template used for the report, or
 *				NULL for none.
 * \return			Report handle, or NULL on failure.
 */

struct report *report_open(file_data *file, char *title, struct analysis_report *template)
{
	struct report	*new;

	#ifdef DEBUG
	debug_printf("\\GOpening report");
	#endif

	new = heap_alloc(sizeof(struct report));

	if (new == NULL)
		return NULL;

	new->next = file->reports;
	file->reports = new;

	new->file = file;

	new->flags = REPORT_STATUS_NONE;
	new->print_pending = 0;

	new->lines = 0;
	new->data_size = 0;

	new->data = NULL;
	new->line_ptr = NULL;

	new->block_size = 0;
	new->max_lines = 0;

	new->window = NULL;
	strcpy(new->window_title, title);

	if (flex_alloc((flex_ptr) &(new->data), REPORT_BLOCK_SIZE))
		new->block_size = REPORT_BLOCK_SIZE;
	else
		new->flags |= REPORT_STATUS_MEMERR;

	if (flex_alloc((flex_ptr) &(new->line_ptr), REPORT_LINE_SIZE * sizeof(int)))
		new->max_lines = REPORT_LINE_SIZE;
	else
		new->flags |= REPORT_STATUS_MEMERR;

	new->template = template;

	return (new);
}


/**
 * Close off a report that has had data written to it, and open a window
 * to display it on screen.
 *
 * \param *report		The report handle.
 */

void report_close(struct report *report)
{
	int			linespace;
	wimp_window_state	parent;
	file_data		*file;

	#ifdef DEBUG
	debug_printf("\\GClosing report");
	#endif

	if (report == NULL || (report->flags & REPORT_STATUS_MEMERR)) {
		error_msgs_report_error("NoMemReport");
		report_delete(report);
		return;
	}

	file = report->file;

	/* Update the data block. */

	flex_extend((flex_ptr) &(report->data), report->data_size);
	flex_extend((flex_ptr) &(report->line_ptr), report->lines * sizeof(int));

	report->flags |= REPORT_STATUS_CLOSED;

	/* Set up the display details. */

	strcpy (report->font_normal, config_str_read("ReportFontNormal"));
	strcpy (report->font_bold, config_str_read("ReportFontBold"));
	report->font_size = config_int_read("ReportFontSize") * 16;
	report->line_spacing = config_int_read("ReportFontLinespace");
	report->width = report_reflow_content(report);

	/* Set up the window title */

	report_window_def->title_data.indirected_text.text = report->window_title;

	/* Size the window. */

	font_convertto_os(1000 * (report->font_size / 16) * report->line_spacing / 100, 0, &linespace, NULL);

	#ifdef DEBUG
	debug_printf("Report window width: %d", report->width);
	#endif

	/* Position the window and open it. */

	parent.w = file->transaction_window.transaction_pane;
	wimp_get_window_state(&parent);

	report->height = report->lines * linespace + REPORT_BOTTOM_MARGIN;

	set_initial_window_area(report_window_def,
			(report->width > REPORT_MIN_WIDTH) ? report->width : REPORT_MIN_WIDTH,
			(report->height > REPORT_MIN_HEIGHT) ? report->height : REPORT_MIN_HEIGHT,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	file->child_x_offset++;
	if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
		file->child_x_offset = 0;

	report->window = wimp_create_window(report_window_def);
	windows_open(report->window);
	ihelp_add_window(report->window, "Report", NULL);
	event_add_window_menu(report->window, report_view_menu);
	event_add_window_user_data(report->window, report);
	event_add_window_close_event(report->window, report_view_close_window_handler);
	event_add_window_redraw_event(report->window, report_view_redraw_handler);
	event_add_window_menu_prepare(report->window, report_view_menu_prepare_handler);
	event_add_window_menu_selection(report->window, report_view_menu_selection_handler);
	event_add_window_menu_warning(report->window, report_view_menu_warning_handler);
}


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

void report_close_and_print(struct report *report, osbool text, osbool textformat, osbool fitwidth, osbool rotate, osbool pagenum)
{
	int		linespace;

	#ifdef DEBUG
	debug_printf("\\GClosing report and starting printing");
	#endif

	if (report == NULL || (report->flags & REPORT_STATUS_MEMERR)) {
		error_msgs_report_error("NoMemReport");
		report_delete(report);
		return;
	}

	/* Update the data block. */

	flex_extend((flex_ptr) &(report->data), report->data_size);
	flex_extend((flex_ptr) &(report->line_ptr), report->lines * sizeof(int));

	report->flags |= REPORT_STATUS_CLOSED;

	/* Set up the display details. */

	strcpy (report->font_normal, config_str_read("ReportFontNormal"));
	strcpy (report->font_bold, config_str_read("ReportFontBold"));
	report->font_size = config_int_read("ReportFontSize") * 16;
	report->line_spacing = config_int_read("ReportFontLinespace");
	report->width = report_reflow_content(report);
	font_convertto_os(1000 * (report->font_size / 16) * report->line_spacing / 100, 0, &linespace, NULL);
	report->height = report->lines * linespace + REPORT_BOTTOM_MARGIN;

	/* There isn't a window. */

	report->window = NULL;

	/* Set up the details needed by the print system and go.
	 * This hijacks the same process used by the Report Print dialogue in process_report_print_window(), setting
	 * up the same variables and launching the same Wimp messages.
	 */

	report_print_report = report;

	report_print_opt_text = text;
	report_print_opt_textformat = textformat;
	report_print_opt_fitwidth = fitwidth;
	report_print_opt_rotate = rotate;
	report_print_opt_pagenum = pagenum;

	report_print_report->print_pending++;

	printing_send_start_print_save(report_start_print_job, report_cancel_print_job, report_print_opt_text);
}


/**
 * Delete a report block, along with any window and data associated with
 * it.
 *
 * \param *report		The report to delete.
 */

void report_delete(struct report *report)
{
	file_data	*file;
	struct report	**rep;

	#ifdef DEBUG
	debug_printf("\\RDeleting report");
	#endif

	if (report == NULL)
		return;

	file = report->file;

	if (report->window != NULL) {
		event_delete_window(report->window);
		wimp_delete_window(report->window);
		report->window = NULL;
	}

	/* Free the flex bloxks. */

	if (report->data != NULL)
		flex_free((flex_ptr) &(report->data));

	if (report->line_ptr != NULL)
		flex_free((flex_ptr) &(report->line_ptr));

	if (report->template != NULL)
		heap_free(report->template);

	/* Delink the block and delete it. */

	rep = &(file->reports);

	while (*rep != NULL && *rep != report)
		rep = &((*rep)->next);

	if (*rep != NULL)
		*rep = report->next;

	heap_free(report);
}


/**
 * Write a line of text to an open report.  Lines are NULL terminated, and can
 * contain the following inline commands:
 *
 *	\t - Tab (start a new column)
 *	\i - Indent the text in the current 'cell'
 *	\b - Format this 'cell' bold
 *	\u - Format this 'cell' underlined
 *	\d - This cell contains a number
 *	\r - Right-align the text in this 'cell'
 *	\s - Spill text from the previous cell into this one.
 *	\k - This line is part of a keep-together block: the first line is a
 *	     heading, repeated on subsequent pages.
 *
 * \param *report		The report to write to.
 * \param bar			The tab bar to use.
 * \param *text			The line of text to write.
 */

void report_write_line(struct report *report, int bar, char *text)
{
	int	len;
	char	*c, *copy, *flag;

	#ifdef DEBUG
	debug_printf("Print line: %s", text);
	#endif

	/* Parse the string */

	copy = malloc(strlen(text) + (REPORT_TAB_STOPS * REPORT_FLAG_BYTES) + 1);
	c = copy;

	bar = (bar >= 0 && bar < REPORT_TAB_BARS) ? bar : 0;

	flag = c++;
	*flag = REPORT_FLAG_NOTNULL;

	while (*text != '\0') {
		if (*text == '\\') {
			text++;

			switch (*text++) {
			case 't':
				*c++ = '\n';
				flag = c++;
				*flag = REPORT_FLAG_NOTNULL;
				break;

			case 'i':
				*flag |= REPORT_FLAG_INDENT;
				break;

			case 'b':
				*flag |= REPORT_FLAG_BOLD;
				break;

			case 'u':
				*flag |= REPORT_FLAG_UNDER;
				break;

			case 'r':
				*flag |= REPORT_FLAG_RIGHT;
				break;

			case 'd':
				*flag |= REPORT_FLAG_NUMERIC;
				break;

			case 's':
				*flag |= REPORT_FLAG_SPILL;
				break;

			case 'k':
				*flag |= REPORT_FLAG_KEEPTOGETHER;
				break;
			}
		} else {
			*c++ = *text++;
		}
	}

	*c = '\0';

	/* Now that we have a string containing /n for tabs and all the flag bytes in the correct places, allocate memory
	 * from the flex block and proceed to dump it in there.
	 */

	len = strlen(copy) + REPORT_BAR_BYTES + 1; /* Add in the terminator and the tab bar marker. */

	if (len > (report->block_size - report->data_size)) {
		#ifdef DEBUG
		debug_printf("Extending data block...");
		#endif

		if (flex_extend((flex_ptr) &(report->data), report->block_size + REPORT_BLOCK_SIZE))
			report->block_size += REPORT_BLOCK_SIZE;
		else
			report->flags |= REPORT_STATUS_MEMERR;
	}

	if (report->lines >= report->max_lines) {
		#ifdef DEBUG
		debug_printf("Extending line pointer block to %d...", ((report->max_lines + REPORT_LINE_SIZE) * sizeof (int)));
		#endif

		if (flex_extend((flex_ptr) &(report->line_ptr), ((report->max_lines + REPORT_LINE_SIZE) * sizeof (int))))
			report->max_lines += REPORT_LINE_SIZE;
		else
			report->flags |= REPORT_STATUS_MEMERR;
	}

	if ((report->flags & REPORT_STATUS_MEMERR) == 0 && (report->flags & REPORT_STATUS_CLOSED) == 0 &&
			len <= (report->block_size - report->data_size) && report->lines < report->max_lines) {
		*(report->data + report->data_size) = (char) bar;
		strcpy(report->data + report->data_size + REPORT_BAR_BYTES, copy);
		(report->line_ptr)[report->lines] = report->data_size;

		report->lines++;
		report->data_size += len;
	}

	free(copy);
}


/**
 * Reflow the text in a report to suit the current content and display settings.
 *
 * \param *report		The report to reflow.
 * \return			The required width, in OS units, of the window
 *				to contain the report.
 */

static int report_reflow_content(struct report *report)
{
	osbool		right[REPORT_TAB_BARS][REPORT_TAB_STOPS];
	int		width[REPORT_TAB_STOPS], t_width[REPORT_TAB_STOPS],
			width1[REPORT_TAB_BARS][REPORT_TAB_STOPS], width2[REPORT_TAB_BARS][REPORT_TAB_STOPS],
			t_width1[REPORT_TAB_BARS][REPORT_TAB_STOPS], t_width2[REPORT_TAB_BARS][REPORT_TAB_STOPS],
			i, j, line, bar, tab, total;
	char		*column, *flags;
	font_f		font, font_n, font_b;

	#ifdef DEBUG
	debug_printf("\\GFormatting report");
	#endif

	/* Reset the flags used to keep track of items. */

	for (i=0; i<REPORT_TAB_BARS; i++) {
		for (j=0; j<REPORT_TAB_STOPS; j++) {
			right[i][j]  = FALSE;		/* Flag to mark if anything in a column has been right-aligned. */

							/* These are in OS units, for on screen rendering. */

			width1[i][j] = 0;		/* Tally of the maximum widths of the columns. */
			width2[i][j] = 0;		/* Tally of the maximum widths of the columns, ignoring end column objects in each row. */

							/* These two are in characters, for ASCII formatting. */

			t_width1[i][j] = 0;		/* Tally of the maximum widths of the columns. */
			t_width2[i][j] = 0;		/* Tally of the maximum widths of the columns, ignoring end column objects in each row. */
		}
	}

	/* Find the font to be used by the report. */

	report_find_fonts(report, &font_n, &font_b);

	/* Work through the report, line by line, getting the maximum column widths. */

	for (line = 0; line < report->lines; line++) {
		tab = 0;
		column = report->data + report->line_ptr[line];
		bar = (int) *column;
		column += REPORT_BAR_BYTES;

		/* Process the next line. do {} while is used, as we don't know if the last tab has been reached until we get
		 * there.
		 */

		do {
			flags = column;
			column += REPORT_FLAG_BYTES;

			/* The flags that are looked at are bold, which affects the font, and indent, which affects the width.
			 * Underline and right don't affect the column width, so these are ignored with the formatting.
			 *
			 * Right flags are noted to allow the column widths and tab stops to be sorted out later.
			 */

			/* Outline font width. */

			font = (*flags & REPORT_FLAG_BOLD) ? font_b : font_n;

			font_scan_string(font, column, font_KERN | font_GIVEN_FONT, 0x7fffffff, 0x7fffffff,
					NULL, NULL, 0, NULL, &total, NULL, NULL);
			font_convertto_os(total, 0, &(width[tab]), NULL);

			/* ASCII text column width. */

			t_width[tab] = string_ctrl_strlen (column);

			/* If the column is indented, add the indent to the column widths. */

			if (*flags & REPORT_FLAG_INDENT) {
				width[tab] += REPORT_COLUMN_INDENT;
				t_width[tab] += REPORT_TEXT_COLUMN_INDENT;
			}

			/* If the column is right aligned, record the fact. */

			if (*flags & REPORT_FLAG_RIGHT)
				right[bar][tab] = TRUE;

			/* If the column is a spill column, the width is carried over from the width of the preceeding column, minus the
			 * inter-column gap.  The previous column is then zeroed.
			 */

			if ((*flags & REPORT_FLAG_SPILL) && tab > 0) {
				width[tab] = width[tab-1] - REPORT_COLUMN_SPACE;
				width[tab-1] = 0;

				t_width[tab] = t_width[tab-1] - REPORT_TEXT_COLUMN_SPACE;
				t_width[tab-1] = 0;
			}

			/* Skip past the text to the next \n or \0, then increment the tab stop. */

			while (*column != '\0' && *column != '\n')
				column++;
			column++;
			tab++;
		} while (tab < REPORT_TAB_STOPS && *(column - 1) != '\0');

		/* Update the tally of maximum column widths. */

		for (i=0; i<tab; i++) {
			if (width[i] > width1[bar][i]) /* All column widths are noted here... */
				width1[bar][i] = width[i];

			if (width[i] > width2[bar][i] && i < (tab-1)) /* ...but here only for non-end column (which can spill over). */
				width2[bar][i] = width[i];

			if (t_width[i] > t_width1[bar][i]) /* All column widths are noted here... */
				t_width1[bar][i] = t_width[i];

			if (t_width[i] > t_width2[bar][i] && i < (tab-1)) /* ...but here only those for non-end column. */
				t_width2[bar][i] = t_width[i];
		}
	}

	font_lose_font(font_n);
	font_lose_font(font_b);

	/* Go through the columns, storing the widths into the report data block.  If right alignment has been used, we
	 * must record the widest width; if not, we can get away with the widest non-end-column width.
	 */

	for (i=0; i<REPORT_TAB_BARS; i++) {
		for (j=0; j<REPORT_TAB_STOPS; j++) {
			report->font_width[i][j] = (right[i][j]) ? width1[i][j] : width2[i][j];
			report->text_width[i][j] = (right[i][j]) ? t_width1[i][j] : t_width2[i][j];
		}
	}

	/* Now set the tab stops up.  The first is at zero, then add each column width on and include the
	 * inter-column gap.
	 */

	for (i=0; i<REPORT_TAB_BARS; i++) {
		report->font_tab[i][0] = 0;

		#ifdef DEBUG
		debug_printf("Set tab bar %d (tab 0 = 0)", i);
		#endif

		for (j=1; j<REPORT_TAB_STOPS; j++) {
			report->font_tab[i][j] = report->font_tab[i][j-1] + report->font_width[i][j-1] + REPORT_COLUMN_SPACE;

			#ifdef DEBUG
			debug_printf ("tab %d = %d", j, report->font_tab[i][j]);
			#endif
		}
	}

	/* Finally, work out how wide the window needs to be.  This is done by taking each tab stop and adding on the
	 * widest entry in that column.
	 */

	total = 0;

	for (i=0; i<REPORT_TAB_BARS; i++) {
		for (j=0; j<REPORT_TAB_STOPS; j++) {
			if (width1[i][j] > 0 && report->font_tab[i][j] + width1[i][j] > total) {
				total = report->font_tab[i][j] + width1[i][j];
			}
		}
	}

	total += (REPORT_LEFT_MARGIN + REPORT_RIGHT_MARGIN);

	return total;
}


/**
 * Look up the fonts needed for a report, returning font handles for the two.
 * If the specified fonts don't exist then fall back to Homerton.
 *
 * \param *report		The report to look fonts up for.
 * \param *normal		Return the font handle for the normal font.
 * \param *bold			Return the font handle for the bold font.
 * \return			TRUE if successful; else FALSE.
 */

static osbool report_find_fonts(struct report *report, font_f *normal, font_f *bold)
{
	os_error	*error, *e1 = NULL, *e2 = NULL;

	if (normal != NULL) {
		error = xfont_find_font(report->font_normal, report->font_size, report->font_size, 0, 0, normal, NULL, NULL);
		if (error != NULL)
			e1 = xfont_find_font("Homerton.Medium", report->font_size, report->font_size, 0, 0, normal, NULL, NULL);
	}

	if (bold != NULL) {
		error = xfont_find_font (report->font_bold, report->font_size, report->font_size, 0, 0, bold, NULL, NULL);
		if (error != NULL)
			e2 = xfont_find_font ("Homerton.Bold", report->font_size, report->font_size, 0, 0, bold, NULL, NULL);
	}

	return (e1 != NULL || e2 != NULL) ? TRUE : FALSE;
}


/**
 * Test for any pending report print jobs on the specified file.
 *
 * \param *file			The file to test.
 * \return			TRUE if there are pending jobs; FALSE if not.
 */

osbool report_get_pending_print_jobs(file_data *file)
{
	struct report	*list;
	osbool		pending = FALSE;

	list = file->reports;

	while (list != NULL) {
		if (list->print_pending > 0)
			pending = TRUE;

		list = list->next;
	}

	return pending;
}


/**
 * Handle Close events on Report View windows, deleting the window and (if there
 * are no print jobs pending) its data block.
 *
 * \param *close		The Wimp Close data block.
 */

static void report_view_close_window_handler(wimp_close *close)
{
	struct report	*report;

	#ifdef DEBUG
	debug_printf ("\\RDeleting report window");
	#endif

	report = event_get_window_user_data(close->w);
	if (report == NULL)
		return;

	/* Close the window */

	if (report->window != NULL) {
		analysis_force_close_report_save_window(report->template);

		ihelp_remove_window(report->window);
		event_delete_window(report->window);
		wimp_delete_window(report->window);
		report->window = NULL;
	}

	if (report->print_pending == 0)
		report_delete(report);
}


/**
 * Process menu prepare events in the Report View window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void report_view_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct report	*report;

	report = event_get_window_user_data(w);

	if (report == NULL)
		return;

	saveas_initialise_dialogue(report_saveas_text, NULL, "DefRepFile", NULL, FALSE, FALSE, report);
	saveas_initialise_dialogue(report_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, report);
	saveas_initialise_dialogue(report_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, report);

	menus_shade_entry(report_view_menu, REPVIEW_MENU_TEMPLATE, report->template == NULL);
}


/**
 * Process menu selection events in the Report View window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void report_view_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct report	*report;
	wimp_pointer	pointer;

	report = event_get_window_user_data(w);

	if (report == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]){
	case REPVIEW_MENU_FORMAT:
		report_open_format_window(report, &pointer);
		break;

	case REPVIEW_MENU_PRINT:
		report_open_print_window(report, &pointer, config_opt_read("RememberValues"));
		break;

	case REPVIEW_MENU_TEMPLATE:
		analysis_open_save_window(report->template, &pointer);
		break;
	}
}


/**
 * Process submenu warning events in the Report View window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void report_view_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct report	*report;

	report = event_get_window_user_data(w);

	if (report == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\BReceived submenu warning message.");
	#endif

	switch (warning->selection.items[0]) {
	case REPVIEW_MENU_SAVETEXT:
		saveas_prepare_dialogue(report_saveas_text);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case REPVIEW_MENU_EXPCSV:
		saveas_prepare_dialogue(report_saveas_csv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;

	case REPVIEW_MENU_EXPTSV:
		saveas_prepare_dialogue(report_saveas_tsv);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process window redraw events in a Report View window.
 *
 * \param *redraw		The Wimp Redraw Event data block.
 */

static void report_view_redraw_handler(wimp_draw *redraw)
{
	int		ox, oy, top, base, y, linespace;
	font_f		font_n, font_b;
	struct report	*report;
	osbool		more;

	report = event_get_window_user_data(redraw->w);

	if (report == NULL)
		return;

	/* Find the required font, set it and calculate the font size from the linespacing in points. */

	report_find_fonts(report, &font_n, &font_b);

	font_convertto_os(1000 * (report->font_size / 16) * report->line_spacing / 100, 0, &linespace, NULL);

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		top = (oy - redraw->clip.y1) / linespace;
		if (top < 0)
			top = 0;

		base = (linespace + (linespace / 2) + oy - redraw->clip.y0 ) / linespace;

		wimp_set_font_colours(wimp_COLOUR_WHITE, wimp_COLOUR_BLACK);

		for (y = top; y < report->lines && y <= base; y++)
			report_plot_line(report, y, ox + REPORT_LEFT_MARGIN,
					oy - linespace * (y+1) + REPORT_BASELINE_OFFSET,
					font_n, font_b);

		more = wimp_get_rectangle(redraw);
	}

	font_lose_font(font_n);
	font_lose_font(font_b);
}


/**
 * Open the Report Format dialogue for a given report view.
 *
 * \param *report		The report to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void report_open_format_window(struct report *report, wimp_pointer *ptr)
{
	/* If the window is already open, another report format is being edited.  Assume the user wants to lose
	 * any unsaved data and just close the window.
	 *
	 * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
	 */

	if (windows_get_open(report_format_window))
		wimp_close_window(report_format_window);

	/* Set the window contents up. */

	report_fill_format_window(report);

	/* Set the pointers up so we can find this lot again and open the window. */

	report_format_report = report;

	windows_open_centred_at_pointer(report_format_window, ptr);
	place_dialogue_caret(report_format_window, REPORT_FORMAT_FONTSIZE);
}


/**
 * Process mouse clicks in the Report Format dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void report_format_click_handler(wimp_pointer *pointer)
{
	switch (pointer->i) {
	case REPORT_FORMAT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(report_format_window);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			report_refresh_format_window();
		break;

	case REPORT_FORMAT_OK:
		report_process_format_window();
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_dialogue_with_caret(report_format_window);
		break;
	}
}


/**
 * Process keypresses in the Report Format window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool report_format_keypress_handler(wimp_key *key)
{
	switch (key->c) {
	case wimp_KEY_RETURN:
		report_process_format_window();
		close_dialogue_with_caret(report_format_window);
		break;

	case wimp_KEY_ESCAPE:
		close_dialogue_with_caret (report_format_window);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process menu prepare events in the Report Format window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void report_format_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	report_format_font_menu = fontlist_build();
	report_format_font_icon = pointer->i;
	event_set_menu_block(report_format_font_menu);
	templates_set_menu(TEMPLATES_MENU_FONTS, report_format_font_menu);
}


/**
 * Process menu selection events in the Report Format window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void report_format_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	char	*font;

	font = fontlist_decode(selection);
	if (font == NULL)
		return;

	switch (report_format_font_icon) {
	case REPORT_FORMAT_NFONTMENU:
		icons_printf(report_format_window, REPORT_FORMAT_NFONT, "%s", font);
		wimp_set_icon_state(report_format_window, REPORT_FORMAT_NFONT, 0, 0);
		break;

	case REPORT_FORMAT_BFONTMENU:
		icons_printf(report_format_window, REPORT_FORMAT_BFONT, "%s", font);
		wimp_set_icon_state(report_format_window, REPORT_FORMAT_BFONT, 0, 0);
		break;
	}

	heap_free(font);
}


/**
 * Process menu close events in the Report Format window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void report_format_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	fontlist_destroy();
	report_format_font_menu = NULL;
	report_format_font_icon = -1;
}


/**
 * Refresh the contents of the Report Format window.
 */

static void report_refresh_format_window(void)
{
	report_fill_format_window(report_format_report);
	icons_redraw_group(report_format_window, 4, REPORT_FORMAT_NFONT, REPORT_FORMAT_BFONT,
			REPORT_FORMAT_FONTSIZE, REPORT_FORMAT_FONTSPACE);
	icons_replace_caret_in_window(report_format_window);
}


/**
 * Update the contents of the Report Format window to reflect the current
 * settings of the given report.
 *
 * \param *report		The report to use.
 */

static void report_fill_format_window(struct report *report)
{
	if (report == NULL)
		return;

	icons_printf(report_format_window, REPORT_FORMAT_NFONT, "%s", report->font_normal);
	icons_printf(report_format_window, REPORT_FORMAT_BFONT, "%s", report->font_bold);

	icons_printf(report_format_window, REPORT_FORMAT_FONTSIZE, "%d", report->font_size / 16);
	icons_printf(report_format_window, REPORT_FORMAT_FONTSPACE, "%d", report->line_spacing);
}


/**
 * Take the contents of an updated report format window and process the data.
 */

static void report_process_format_window(void)
{
	int			linespace, new_xextent, new_yextent, visible_xextent, visible_yextent, new_xscroll, new_yscroll;
	os_box			extent;
	wimp_window_state	state;

	/* Extract the information. */

	strcpy(report_format_report->font_normal, icons_get_indirected_text_addr(report_format_window, REPORT_FORMAT_NFONT));
	strcpy(report_format_report->font_bold, icons_get_indirected_text_addr(report_format_window, REPORT_FORMAT_BFONT));
	report_format_report->font_size = atoi (icons_get_indirected_text_addr(report_format_window, REPORT_FORMAT_FONTSIZE)) * 16;
	report_format_report->line_spacing = atoi(icons_get_indirected_text_addr(report_format_window, REPORT_FORMAT_FONTSPACE));

	/* Tidy up and redraw the windows */

	report_format_report->width = report_reflow_content(report_format_report);
	font_convertto_os(1000 * (report_format_report->font_size / 16) * report_format_report->line_spacing / 100, 0,
			&linespace, NULL);
	report_format_report->height = report_format_report->lines * linespace + REPORT_BOTTOM_MARGIN;

	/* Calculate the next window extents. */

	new_xextent = (report_format_report->width > REPORT_MIN_WIDTH) ? report_format_report->width : REPORT_MIN_WIDTH;
	new_yextent = (report_format_report->height > REPORT_MIN_HEIGHT) ? -report_format_report->height : -REPORT_MIN_HEIGHT;

	/* Get the current window details, and find the extent of the bottom and right of the visible area. */

	state.w = report_format_report->window;
	wimp_get_window_state(&state);

	visible_xextent = state.xscroll + (state.visible.x1 - state.visible.x0);
	visible_yextent = state.yscroll + (state.visible.y0 - state.visible.y1);

	/* If the visible area falls outside the new window extent, then the window needs to be re-opened first. */

	if (new_xextent < visible_xextent || new_yextent > visible_yextent) {
		/* Calculate the required new scroll offsets.
		 *
		 * Start with the x scroll.  If this is less than zero, the window is too wide and will need shrinking down.
		 * Otherwise, just set the new scroll offset.
		 */

		new_xscroll = new_xextent - (state.visible.x1 - state.visible.x0);

		if (new_xscroll < 0) {
			state.visible.x1 += new_xscroll;
			state.xscroll = 0;
		} else {
			state.xscroll = new_xscroll;
		}

		/* Now do the y scroll.  If this is greater than zero, the current window is too deep and will need
		 * shrinking down.  Otherwise, just set the new scroll offset.
		 */

		new_yscroll = new_yextent - (state.visible.y0 - state.visible.y1);

		if (new_yscroll > 0) {
			state.visible.y0 += new_yscroll;
			state.yscroll = 0;
		} else {
			state.yscroll = new_yscroll;
		}

		wimp_open_window((wimp_open *) &state);
	}

	/* Finally, call Wimp_SetExtent to update the extent, safe in the knowledge that the visible area will still
	 * exist.
	 */

	extent.x0 = 0;
	extent.x1 = new_xextent;
	extent.y1 = 0;
	extent.y0 = new_yextent;
	wimp_set_extent(report_format_report->window, &extent);

	windows_redraw(report_format_report->window);
}


/**
 * Force the readraw of all the open reports associated with a file.
 *
 * \param *file			The file on which to force a redraw.
 */

void report_redraw_all(file_data *file)
{
	struct report *report;

	if (file == NULL)
		return;

	report = file->reports;

	while (report != NULL) {
		if (report->window != NULL)
			windows_redraw(report->window);

		report = report->next;
	}
}


/**
 * Force the closure of any Report Format windows which are open and relate
 * to the given file.
 *
 * \param *file			The file data block of interest.
 */

void report_force_windows_closed(file_data *file)
{
	if (report_format_report != NULL && report_format_report->file == file && windows_get_open(report_format_window))
		close_dialogue_with_caret(report_format_window);
}


/**
 * Open the Report Print window via the global printing module.
 *
 * \param *report		The report to open the dialogue for.
 * \param *ptr			The Wimp Pointer data.
 * \param restore		TRUE to restore the previous print settings; FALSE
 *				to use the application defaults.
 */

static void report_open_print_window(struct report *report, wimp_pointer *ptr, osbool restore)
{
	if (report == NULL)
		return;

	report_print_report = report;
	printing_open_simple_window (report->file, ptr, restore, "PrintReport", report_print_window_closed);
}


/**
 * Callback from the printing module, when Print is selected in the simple print
 * dialogue.  Start the printing process proper.
 *
 * \param text			TRUE to use text mode printing.
 * \param format		TRUE to use formatted text mode printing.
 * \param scale			TRUE to scale graphics printing to the page width.
 * \param rotate		TRUE to rotate graphics printing into Landscape mode.
 * \param pagenum		TRUE to include page numbers in graphics printing.
 */

static void report_print_window_closed(osbool text, osbool format, osbool scale, osbool rotate, osbool pagenum)
{
	#ifdef DEBUG
	debug_printf("Report print received data from simple print window");
	#endif

	/* Extract the information. */

	report_print_opt_text = text;
	report_print_opt_textformat = format;
	report_print_opt_fitwidth = scale;
	report_print_opt_rotate = rotate;
	report_print_opt_pagenum = pagenum;

	/* Start the print dialogue process.
	 * This process is also used by the direct report print function
	 * report_close_and_print(), so the two probably can't co-exist.
	 */

	report_print_report->print_pending++;

	printing_send_start_print_save(report_start_print_job, report_cancel_print_job, report_print_opt_text);
}








/* ==================================================================================================================
 * Saving and export
 */


/**
 * Callback handler for saving a text version of a report.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the report block for the save target.
 */

static osbool report_save_text(char *filename, osbool selection, void *data)
{
	struct report *report = (struct report *) data;
	
	if (report == NULL || report->file == NULL)
		return FALSE;

	report_export_text(report, filename, FALSE);
	
	return TRUE;
}


/**
 * Callback handler for saving a CSV version of a report.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the report block for the save target.
 */

static osbool report_save_csv(char *filename, osbool selection, void *data)
{
	struct report *report = (struct report *) data;
	
	if (report == NULL || report->file == NULL)
		return FALSE;

	report_export_delimited(report, filename, DELIMIT_QUOTED_COMMA, CSV_FILE_TYPE);
	
	return TRUE;
}


/**
 * Callback handler for saving a TSV version of a report.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the report block for the save target.
 */

static osbool report_save_tsv(char *filename, osbool selection, void *data)
{
	struct report *report = (struct report *) data;
	
	if (report == NULL || report->file == NULL)
		return FALSE;
		
	report_export_delimited(report, filename, DELIMIT_TAB, TSV_FILE_TYPE);
	
	return TRUE;
}


/**
 * Export a report in plain ASCII text, optionally with fancy text formatting.
 *
 * \param *report		The report to export.
 * \param *filename		The filename to export to.
 * \param formatting		TRUE to include fancytext formmating; FALSE
 *				for plain text.
 */

static void report_export_text(struct report *report, char *filename, osbool formatting)
{
	FILE	*out;
	int	i, j, bar, tab, indent, width, overrun, escape;
	char	*column, *flags, buffer[256];


	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	for (i = 0; i < report->lines; i++) {
		tab = 0;
		overrun = 0;
		column = report->data + report->line_ptr[i];
		bar = (int) *column;
		column += REPORT_BAR_BYTES;

		do {
			flags = column;
			column += REPORT_FLAG_BYTES;
			string_ctrl_strcpy(buffer, column);

			escape = (*flags & REPORT_FLAG_BOLD) ? 0x01 : 0x00;
			if (*flags & REPORT_FLAG_UNDER)
				escape |= 0x08;

			indent = (*flags & REPORT_FLAG_INDENT) ? REPORT_TEXT_COLUMN_INDENT : 0;
			width = strlen(buffer);

			if (*flags & REPORT_FLAG_RIGHT)
				indent = report->text_width[bar][tab] - width;

			/* Output the indent spaces. */

			for (j = 0; j < indent; j++)
				fputc(' ', out);

			/* Output fancy text formatting codes (used when printing formatted text) */

			if (formatting && escape != 0) {
				fputc((char) 27, out);
				fputc((char) (0x80 | escape), out);
			}

			/* Output the actual field data. */

			fprintf(out, "%s", buffer);

			/* Output fancy text formatting codes (used when printing formatted text) */

			if (formatting && escape != 0) {
				fputc((char) 27, out);
				fputc((char) 0x80, out);
			}

			/* Find the next field. */

			while (*column != '\0' && *column != '\n')
				column++;

			/* If there is another field, pad out with spaces. */

			if (*column != '\0') {
				/* Check the actual width against that allocated.  If it is more,
				 * note the amount that spills into the next column, taking into
				 * account the width of the inter column gap.
				 */

				if ((width+indent) > report->text_width[bar][tab])
					overrun += (width+indent) - report->text_width[bar][tab] - REPORT_TEXT_COLUMN_SPACE;

				/* Pad out the required number of spaces, taking into account any
				 * overspill from earlier columns.
				 */

				for (j = 0; j < (report->text_width[bar][tab] - (width+indent) + REPORT_TEXT_COLUMN_SPACE - overrun); j++)
					fputc(' ', out);

				/* Reduce the overspill record by the amount of free space in this column. */

				if ((width+indent) < report->text_width[bar][tab]) {
					overrun -= report->text_width[bar][tab] - (width+indent) + REPORT_TEXT_COLUMN_SPACE;
					if (overrun < 0)
						overrun = 0;
				}
			}

			column++;
			tab++;
		} while (*(column - 1) != '\0' && tab < REPORT_TAB_STOPS);

		fputc('\n', out);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) (formatting) ? FANCYTEXT_FILE_TYPE : TEXT_FILE_TYPE);

	hourglass_off();
}


/**
 * Export a report in CSV or TSV format.
 *
 * \param *report		The report to export.
 * \param *filename		The filename to export to.
 * \param format		The file format to be used.
 * \param filetype		The RISC OS filetype to save as.
 */

static void report_export_delimited(struct report *report, char *filename, enum filing_delimit_type format, int filetype)
{
	FILE	*out;
	int	i, tab, delimit;
	char	*column, *flags, buffer[256];

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

  
	hourglass_on();

	for (i = 0; i < report->lines; i++) {
		tab = 0;
		column = report->data + report->line_ptr[i] + REPORT_BAR_BYTES;

      do
      {
		flags = column;
		column += REPORT_FLAG_BYTES;
		string_ctrl_strcpy(buffer, column);

		/* Find the next field. */

		while (*column != '\0' && *column != '\n')
			column++;

		/* Output the actual field data. */

		delimit = (*column == '\0') ? DELIMIT_LAST : 0;

		if (*flags & REPORT_FLAG_NUMERIC)
			delimit |= DELIMIT_NUM;

		filing_output_delimited_field(out, buffer, format, delimit);

		column++;
		tab++;
		} while (*(column - 1) != '\0' && tab < REPORT_TAB_STOPS);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}


/**
 * Callback for when the negotiations with the printer driver have completed,
 * to actually start the printing process.
 *
 * \param *filename	The filename supplied by the printing system.
 */

static void report_start_print_job(char *filename)
{
	if (report_print_opt_text)
		report_export_text(report_print_report, filename, report_print_opt_textformat);
	else
		report_print_as_graphic(report_print_report, report_print_opt_fitwidth, report_print_opt_rotate, report_print_opt_pagenum);

	/* Tidy up afterwards.  If that was the last print job in progress and the window has already been closed (or if
	 * there wasn't a window at all), delete the report data.
	 */

	if (--report_print_report->print_pending <= 0 && report_print_report->window == NULL)
		report_delete(report_print_report);
}


/**
 * Callback for when negotiations with the printing system break down, to
 * tidy up after ourselves.
 */

static void report_cancel_print_job(void)
{
	if (--report_print_report->print_pending <= 0 && report_print_report->window == NULL)
		report_delete(report_print_report);

}


/**
 * Print a report via the graphical printing system.
 *
 * \param *report		The report to print.
 * \param fit_width		TRUE to fit to the width of the paper.
 * \param rotate		TRUE to rotate into Landscape format.
 * \param pagenum		TRUE to include page numbers.
 */

static void report_print_as_graphic(struct report *report, osbool fit_width, osbool rotate, osbool pagenum)
{
	os_error			*error;
	os_fw				out = 0;
	os_box				p_rect, f_rect, rect, body = {0, 0, 0, 0}, footer = {0, 0, 0, 0};
	os_hom_trfm			p_trfm;
	os_coord			p_pos, f_pos;
	char				title[1024], b0[10], b1[10], b2[10], b3[10];
	pdriver_features		features;
	font_f				font_n = 0, font_b = 0;
	osbool				more;
	int				width, offset;
	int				pages_x, pages_y, lines_per_page, trim, page_x, page_y, lines, header;
	int				top, base, y, linespace, bar;
	unsigned int			footer_width, footer_height, header_height, page_width, page_height, scale, page_xstart, line;
	double				scaling;
	struct report_print_pagination	*pages;
	enum report_page_area		areas, area;

	hourglass_on();

	/* Get the printer driver settings. */

	error = xpdriver_info (NULL, NULL, NULL, &features, NULL, NULL, NULL, NULL);
	if (error != NULL)
		return;

	/* Find the fonts we will use and size the header and footer accordingly. */

	report_find_fonts(report, &font_n, &font_b);
	font_convertto_os((1000 * report->font_size) * report->line_spacing / 1600, 0, &linespace, NULL);

	/* Establish the page dimensions. */

	header_height = 0;
	footer_height = (pagenum) ? (1000 * report->font_size) * report->line_spacing / 1600 : 0;

	areas = report_get_page_areas(rotate, &body, NULL, &footer, header_height, footer_height);

	/* Open a printout file. */

	error = xosfind_openoutw(osfind_NO_PATH, "printer:", NULL, &out);
	if (error != NULL) {
		report_handle_print_error(error, out, font_n, font_b);
		return;
	}

	/* Start a print job. */

	msgs_param_lookup("PJobTitle", title, sizeof(title), report->window_title, NULL, NULL, NULL);
	error = xpdriver_select_jobw(out, title, NULL);
	if (error != NULL) {
		report_handle_print_error(error, out, font_n, font_b);
		return;
	}

	/* Declare the fonts we are using, if required. */

	if (features & pdriver_FEATURE_DECLARE_FONT) {
		xpdriver_declare_font(font_n, 0, pdriver_KERNED);
		xpdriver_declare_font(font_b, 0, pdriver_KERNED);
		xpdriver_declare_font(0, 0, 0);
	}

	/* Calculate the page size, positions, transformations etc. */

	/* The printable page width and height, in milli-points and then into OS units. */

	page_width = abs(body.x1 - body.x0);
	page_height = abs(body.y1 - body.y0);

	error = xfont_convertto_os(page_width, page_height, (int *) &page_width, (int *) &page_height);
	if (error != NULL) {
		report_handle_print_error(error, out, font_n, font_b);
		return;
	}

	/* Scale is the scaling factor to get the width of the report to fit onto one page, if required. The scale is
	 * never more than 1:1 (we never enlarge the print).
	 */

	if (fit_width) {
		scale = (1 << 16) * page_width / report->width;
		scale = (scale > (1 << 16)) ? (1 << 16) : scale;
	} else {
		scale = 1 << 16;
	}

	/* The page width and page height now need to be worked out in terms of what we actually want to print.
	 * If scaling is on, the width is the report width and the height is the true page height scaled up in
	 * proportion; otherwise, these stay as the true printable area in OS units.
	 */

	scaling = 1;

	if (fit_width) {
		if (page_width < report->width) {
			page_height = page_height * report->width / page_width;
			scaling = (double) page_width / (double) report->width;
		}

		page_width = report->width;
	}

	/* \TODO -- Apply the header and footer here. */

	/* Clip the page length to be an exect number of lines */

	trim = (page_height % linespace);
	page_height -= trim;
	lines_per_page = page_height / linespace;

	/* Work out the number of pages.
	 *
	 * Start by deciding the basic number of pages based on usable page width and
	 * height compared to report width and height.
	 */

	pages_x = (int) ceil((double) report->width / (double) page_width);
	pages_y = (int) ceil((double) report->height / (double) page_height);

	/* Adjust the pages vertically to allow for the possibility that
	 * each page might have a header carried over from the previous page.
	 * This is just for allocating the pagination memory.
	 */

	while ((pages_y * lines_per_page) < (report->lines + pages_y))
		pages_y++;

	#ifdef DEBUG
	debug_printf("Pages required: x=%d, y=%d, lines per page=%d", pages_x, pages_y, lines_per_page);
	#endif

	pages = malloc(sizeof(struct report_print_pagination) * pages_y);

	page_y = 0;
	lines = 0;
	header = -1;
	bar = -1;

	/* Paginate the file.  Run through the file line by line, tracking whether
	 * we are in a keep-together block.  If we are when the page changes,
	 * remember the first line of the block to be the repeated header for
	 * the top of the next page.
	 *
	 * By the end of this, we know exactly how many pages will be required.
	 *
	 * \TODO -- There's no protection for running off the end of the
	 *          pagination array...
	 */

	for (y = 0; y < report->lines; y++) {
		if (lines <= 0) {
			#ifdef DEBUG
			debug_printf("Page %d starts at line %d, repeating heading from line %d", page_y, y, header);
			#endif

			pages[page_y].header_line = header;
			pages[page_y].first_line = y;
			page_y++;
			lines = lines_per_page;
			if (header != -1)
				lines--;
			pages[page_y - 1].line_count = (header == -1) ? 0 : 1;
		}

		if ((*(report->data + report->line_ptr[y] + REPORT_BAR_BYTES) & REPORT_FLAG_KEEPTOGETHER) &&
				((*(report->data + report->line_ptr[y]) == bar) || (header == -1))) {
			if (header == -1)
				header = y;
		} else {
			header = -1;
		}

		#ifdef DEBUG
		debug_printf("Line %d has header %d", y, header);
		#endif

		bar = *(report->data + report->line_ptr[y]);

		pages[page_y - 1].line_count++;
		lines --;
	}

	pages_y = page_y;

	if (pages_x == 1) {
		snprintf(b1, sizeof(b1), "%d", pages_y);
	} else {
		snprintf(b2, sizeof(b2), "%d", pages_x);
		snprintf(b3, sizeof(b3), "%d", pages_y);
	}

	/* Set up the transformation matrix scale the page and rotate it as required. */

	footer_width = footer.x1 - footer.x0;

	error = xfont_convertto_os(footer_width, footer_height, (int *) &footer_width, (int *) &footer_height);
	if (error != NULL) {
		report_handle_print_error(error, out, font_n, font_b);
		return;
	}

	/* If the page is wider than the footer, then make the footer wider
	 * as it will get scaled back in the transformation.
	 */

	if (fit_width && page_width > footer_width)
		footer_width = page_width;

	f_rect.x0 = 0;
	f_rect.x1 = footer_width;
	f_rect.y1 = footer_height;
	f_rect.y0 = 0;

	if (rotate) {
		p_trfm.entries[0][0] = 0;
		p_trfm.entries[0][1] = scale;
		p_trfm.entries[1][0] = -scale;
		p_trfm.entries[1][1] = 0;

		f_pos.x = footer.y0;
		f_pos.y = footer.x0;
	} else {
		p_trfm.entries[0][0] = scale;
		p_trfm.entries[0][1] = 0;
		p_trfm.entries[1][0] = 0;
		p_trfm.entries[1][1] = scale;

		f_pos.x = footer.x0;
		f_pos.y = footer.y0;
	}

	/* Loop through the pages down the report and across. */

	for (page_y = 0; page_y < pages_y; page_y++) {
		page_x = 0;

		for (page_xstart = 0; page_xstart < report->width; page_xstart += page_width) {
			/* Calculate the area of the page to print and set up the print rectangle.  If the page is on the edge,
			 * crop the area down to save memory.
			 */

			p_rect.x0 = page_xstart;
			p_rect.x1 = (page_xstart + page_width <= report->width) ? page_xstart + page_width : report->width;
			p_rect.y1 = (pages[page_y].header_line == -1) ? 0 : linespace;
			p_rect.y0 = p_rect.y1 - (pages[page_y].line_count * linespace);

			/* The bottom y edge is done specially, because we also need to set the print position.  If the page is at the
			 * edge, it is cropped down to save on memory.
			 *
			 * The page origin will depend on rotation and the amount of text on the page.  For a full page, the
			 * origin is placed at one corner (either bottom left for a portrait, or bottom right for a landscape).
			 * For part pages, the origin is shifted left or up by the proportion of the page dimension (in milli-points)
			 * taken from the proportion of OS units used for layout.
			 */

			if (rotate) {
				error = xfont_converttopoints((page_height + (p_rect.y0 - p_rect.y1) + trim) * scaling, 0, (int *) &offset, NULL);
				if (error != NULL) {
					report_handle_print_error(error, out, font_n, font_b);
					return;
				}

				p_pos.x = body.y0 - offset;
				p_pos.y = body.x0;
			} else {
				error = xfont_converttopoints((page_height + (p_rect.y0 - p_rect.y1) + trim) * scaling, 0, (int *) &offset, NULL);
				if (error != NULL) {
					report_handle_print_error(error, out, font_n, font_b);
					return;
				}

				p_pos.x = body.x0;
				p_pos.y = body.y0 + offset;
			}

			/* Pass the page details to the printer driver and start to draw the page. */

			error = xpdriver_give_rectangle(REPORT_PAGE_BODY, &p_rect, &p_trfm, &p_pos, os_COLOUR_WHITE);
			if (error != NULL) {
				report_handle_print_error(error, out, font_n, font_b);
				return;
			}

			if (areas & REPORT_PAGE_FOOTER) {
				error = xpdriver_give_rectangle(REPORT_PAGE_FOOTER, &f_rect, &p_trfm, &f_pos, os_COLOUR_WHITE);
				if (error != NULL) {
					report_handle_print_error(error, out, font_n, font_b);
					return;
				}
			}

			error = xpdriver_draw_page(1, &rect, 0, 0, &more, (int *) &area);

			/* Perform the redraw. */

			while (more) {
				switch (area) {
				case REPORT_PAGE_BODY:
					/* Calculate the rows to redraw. */

					top = -rect.y1 / linespace;
					if (top < -1)
						top = -1;
					base = (linespace + (linespace / 2) - rect.y0 ) / linespace;

					error = xcolourtrans_set_font_colours(font_n, os_COLOUR_WHITE, os_COLOUR_BLACK, 0, NULL, NULL, NULL);
					if (error != NULL) {
						report_handle_print_error(error, out, font_n, font_b);
						return;
					}

					error = xcolourtrans_set_font_colours(font_b, os_COLOUR_WHITE, os_COLOUR_BLACK, 0, NULL, NULL, NULL);
					if (error != NULL) {
						report_handle_print_error(error, out, font_n, font_b);
						return;
					}

					/* Redraw the data to the printer. */

					for (y = top; (pages[page_y].first_line + y) < report->lines && y <= base; y++) {
						if (y < 0)
							line = pages[page_y].header_line;
						else
							line = pages[page_y].first_line + y;
						error = report_plot_line(report, line, REPORT_LEFT_MARGIN,
								- linespace * (y+1) + REPORT_BASELINE_OFFSET, font_n, font_b);
						if (error != NULL) {
							report_handle_print_error(error, out, font_n, font_b);
							return;
						}
					}
					break;

				case REPORT_PAGE_FOOTER:
					snprintf(b0, sizeof(b0), "%d", page_y);
					if (pages_x == 1) {
						snprintf(b0, sizeof(b0), "%d", page_y + 1);
						msgs_param_lookup("Page1", title, sizeof(title), b0, b1, NULL, NULL);
					} else {
						snprintf(b0, sizeof(b0), "%d", page_x + 1);
						snprintf(b1, sizeof(b1), "%d", page_y + 1);
						msgs_param_lookup("Page2", title, sizeof(title), b0, b1, b2, b3);
					}

					error = xfont_scan_string(font_n, title, font_KERN | font_GIVEN_FONT,
							0x7fffffff, 0x7fffffff, NULL, NULL, 0, NULL, &width, NULL, NULL);
					if (error != NULL) {
						report_handle_print_error(error, out, font_n, font_b);
						return;
					}

					error = xfont_convertto_os(width, 0, &width, NULL);
					if (error != NULL) {
						report_handle_print_error(error, out, font_n, font_b);
						return;
					}

					error = xfont_paint(font_n, title, font_OS_UNITS | font_KERN | font_GIVEN_FONT,
							(footer_width - width) / 2, 4, NULL, NULL, 0);
					if (error != NULL) {
						report_handle_print_error(error, out, font_n, font_b);
						return;
					}
					break;

				default:
					break;
				}

				error = xpdriver_get_rectangle(&rect, &more, (int *) &area);
				if (error != NULL) {
					report_handle_print_error(error, out, font_n, font_b);
					return;
				}
			}

			page_x++;
		}
	}

	/* Terminate the print job. */

	error = xpdriver_end_jobw(out);

	if (error != NULL) {
		report_handle_print_error(error, out, font_n, font_b);
		return;
	}

	/* Close the printout file. */

	xosfind_closew(out);

	font_lose_font(font_n);
	font_lose_font(font_b);

	free(pages);

	hourglass_off();
}


/**
 * Clean up after errors in the printing process.
 *
 * \param *error	The OS error block rescribing the error.
 * \param file		The print output file handle.
 * \param f1		The first font handle.
 * \param f2		The second font handle.
 */

static void report_handle_print_error(os_error *error, os_fw file, font_f f1, font_f f2)
{
	if (file != 0) {
		xpdriver_abort_jobw(file);
		xosfind_closew(file);
	}

	if (f1 != 0)
		font_lose_font(f1);

	if (f2 != 0)
		font_lose_font(f2);

	hourglass_off();
	error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
}


/**
 * Plot a line of a report to screen or the printer drivers.
 *
 * \param *report	The report to use.
 * \param line		The line to be printed.
 * \param x		The x coordinate to plot at.
 * \param y		The y coordinate to plot at.
 * \param normal	The font handle for the normal font face.
 * \param bold		The font handle for the bold font face.
 * \return		Pointer to an error block, or NULL for success.
 */

static os_error *report_plot_line(struct report *report, unsigned int line, int x, int y, font_f normal, font_f bold)
{
	os_error	*error;
	font_f		font;
	int		bar, tab = 0, indent, total, width;
	char		*column, *paint, *flags, buffer[REPORT_MAX_LINE_LEN+10];


	column = report->data + report->line_ptr[line];
	bar = (int) *column;
	column += REPORT_BAR_BYTES;

	do {
		flags = column;
		column += REPORT_FLAG_BYTES;

		font = (*flags & REPORT_FLAG_BOLD) ? bold : normal;
		indent = (*flags & REPORT_FLAG_INDENT) ? REPORT_COLUMN_INDENT : 0;

		if (*flags & REPORT_FLAG_RIGHT) {
			error = xfont_scan_string(font, column, font_KERN | font_GIVEN_FONT,
					0x7fffffff, 0x7fffffff, NULL, NULL, 0, NULL, &total, NULL, NULL);
			if (error != NULL)
				return error;

			error = xfont_convertto_os(total, 0, &width, NULL);
			if (error != NULL)
				return error;

			indent = report->font_width[bar][tab] - width;
		}

		if (*flags & REPORT_FLAG_UNDER) {
			*(buffer+0) = font_COMMAND_UNDERLINE;
			*(buffer+1) = 230;
			*(buffer+2) = 18;
			strcpy (buffer+3, column);
			paint = buffer;
		} else {
			paint = column;
		}

		error = xfont_paint(font, paint, font_OS_UNITS | font_KERN | font_GIVEN_FONT,
				x + report->font_tab[bar][tab] + indent, y, NULL, NULL, 0);
		if (error != NULL)
			return error;

		while (*column != '\0' && *column != '\n')
			column++;
		column++;
		tab++;
	} while (*(column - 1) != '\0' && tab < REPORT_TAB_STOPS);

	return NULL;
}


/**
 * Read the current printer page size, and work out from the configured margins
 * where on the page the printed body, header and footer will go.
 *
 * \param rotate		TRUE to rotate the page to Landscape format; else FALSE.
 * \param *body			Structure to return the body area, or NULL for none.
 * \param *header		Structure to return the header area, or NULL for none.
 * \param *footer		Structure to return the footer area, or NULL for none.
 * \param header_size		The required height of the header, in millipoints.
 * \param footer_size		The required height of the footer, in millipoints.
 * \return			Flagword indicating which areas were set up.
 */

static enum report_page_area report_get_page_areas(osbool rotate, os_box *body, os_box *header, os_box *footer, unsigned header_size, unsigned footer_size)
{
	os_error			*error;
	osbool				margin_fail = FALSE;
	enum report_page_area		areas = REPORT_PAGE_NONE;
	int				page_xsize, page_ysize, page_left, page_right, page_top, page_bottom;
	int				margin_left, margin_right, margin_top, margin_bottom;

	/* Get the page dimensions, and set up the print margins.  If the margins are bigger than the print
	 * borders, the print borders are increased to match.
	 */

	error = xpdriver_page_size(&page_xsize, &page_ysize, &page_left, &page_bottom, &page_right, &page_top);
	if (error != NULL)
		return REPORT_PAGE_NONE;

	margin_left = page_left;

	if (config_int_read("PrintMarginLeft") > 0 && config_int_read("PrintMarginLeft") > margin_left) {
		margin_left = config_int_read("PrintMarginLeft");
		page_left = margin_left;
	} else if (config_int_read("PrintMarginLeft") > 0) {
		margin_fail = TRUE;
	}

	margin_bottom = page_bottom;

	if (config_int_read("PrintMarginBottom") > 0 && config_int_read("PrintMarginBottom") > margin_bottom) {
		margin_bottom = config_int_read("PrintMarginBottom");
		page_bottom = margin_bottom;
	} else if (config_int_read("PrintMarginBottom") > 0) {
		margin_fail = TRUE;
	}

	margin_right = page_xsize - page_right;

	if (config_int_read("PrintMarginRight") > 0 && config_int_read("PrintMarginRight") > margin_right) {
		margin_right = config_int_read("PrintMarginRight");
		page_right = page_xsize - margin_right;
	} else if (config_int_read("PrintMarginRight") > 0) {
		margin_fail = TRUE;
	}

	margin_top = page_ysize - page_top;

	if (config_int_read("PrintMarginTop") > 0 && config_int_read("PrintMarginTop") > margin_top) {
		margin_top = config_int_read("PrintMarginTop");
		page_top = page_ysize - margin_top;
	} else if (config_int_read("PrintMarginTop") > 0) {
		margin_fail = TRUE;
	}

	if (body != NULL) {
		areas |= REPORT_PAGE_BODY;

		if (rotate) {
			body->x0 = page_bottom;
			body->x1 = page_top;
			body->y0 = page_right;
			body->y1 = page_left;
		} else {
			body->x0 = page_left;
			body->x1 = page_right;
			body->y0 = page_bottom;
			body->y1 = page_top;
		}

		if (header != NULL && header_size > 0) {
			header->x0 = body->x0;
			header->x1 = body->x1;
			header->y1 = body->y1;

			header->y0 = header->y1 - (header_size * ((rotate) ? -1 : 1));
			body->y1 = header->y0 - (config_int_read("PrintMarginInternal") * ((rotate) ? -1 : 1));

			areas |= REPORT_PAGE_HEADER;
		}

		if (footer != NULL && footer_size > 0) {
			footer->x0 = body->x0;
			footer->x1 = body->x1;
			footer->y0 = body->y0;

			footer->y1 = footer->y0 + (footer_size * ((rotate) ? -1 : 1));
			body->y0 = footer->y1 + (config_int_read("PrintMarginInternal") * ((rotate) ? -1 : 1));

			areas |= REPORT_PAGE_FOOTER;
		}
	}

	if (margin_fail)
		error_msgs_report_error("BadPrintMargins");

	return areas;
}


/**
 * Call a callback function to process the template stored in every currently
 * open report.
 *
 * \param *file			The file to process.
 * \param *callback		The callback function to call.
 * \param *data			Data to pass to the callback function.
 */

void report_process_all_templates(file_data *file, void (*callback)(struct analysis_report *template, void *data), void *data)
{
	struct report	*report;

	if (file == NULL || callback == NULL)
		return;

	report = file->reports;

	while (report != NULL) {
		callback(report->template, data);

		report = report->next;
	}
}

