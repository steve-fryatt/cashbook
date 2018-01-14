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
#include "oslib/osfile.h"
#include "oslib/osfind.h"
#include "oslib/colourtrans.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "report.h"

#include "analysis.h"
#include "analysis_template_save.h"
#include "caret.h"
#include "file.h"
#include "filing.h"
#include "fontlist.h"
#include "flexutils.h"
#include "print_dialogue.h"
#include "print_protocol.h"
#include "report_cell.h"
#include "report_draw.h"
#include "report_fonts.h"
#include "report_font_dialogue.h"
#include "report_line.h"
#include "report_page.h"
#include "report_region.h"
#include "report_tabs.h"
#include "report_textdump.h"
#include "transact.h"
#include "window.h"

/* Report View Toolbar icons. */

#define REPORT_PANE_PARENT 0
#define REPORT_PANE_SAVE 1
#define REPORT_PANE_PRINT 2
#define REPORT_PANE_SHOW_PAGES 3
#define REPORT_PANE_PORTRAIT 4
#define REPORT_PANE_LANDSCAPE 5
#define REPORT_PANE_FIT_WIDTH 6
#define REPORT_PANE_SHOW_TITLE 7
#define REPORT_PANE_SHOW_GRID 8
#define REPORT_PANE_SHOW_PAGE_NUMBERS 9
#define REPORT_PANE_FORMAT 10

/* Report view menu */

#define REPVIEW_MENU_SHOWPAGES 0
#define REPVIEW_MENU_FORMAT 1
#define REPVIEW_MENU_INCLUDE 2
#define REPVIEW_MENU_LAYOUT 3
#define REPVIEW_MENU_SAVETEXT 4
#define REPVIEW_MENU_EXPCSV 5
#define REPVIEW_MENU_EXPTSV 6
#define REPVIEW_MENU_PRINT 7
#define REPVIEW_MENU_TEMPLATE 8

#define REPVIEW_MENU_INCLUDE_TITLE 0
#define REPVIEW_MENU_INCLUDE_PAGE_NUM 1
#define REPVIEW_MENU_INCLUDE_GRID 2

#define REPVIEW_MENU_LAYOUT_PORTRAIT 0
#define REPVIEW_MENU_LAYOUT_LANDSCAPE 1
#define REPVIEW_MENU_LAYOUT_FIT_WIDTH 2

/* Report export details */

#define REPORT_PRINT_TITLE_LENGTH 1024

#define REPORT_PRINT_BUFFER_LENGTH 10


/**
 * The maximum size of a page number, in characters.
 */

#define REPORT_MAX_PAGE_NUMBER_LEN 12


/**
 * The maximum length of a page number message, in characters.
 */

#define REPORT_MAX_PAGE_STRING_LEN 64

/**
 * The height of the Report window toolbar.
 */

#define REPORT_TOOLBAR_HEIGHT 78

/**
 * The margin around the outside of a print ractangle, in OS Units.
 */

#define REPORT_PRINT_RECTANGLE_MARGIN 2

#define REPORT_GRID_LINE_MARGIN 2


struct report_print_pagination {
	int		header_line;						/**< A line to repeat as a header at the top of the page, or -1 for none.			*/
	int		first_line;						/**< The first non-repeated line on the page from the report document.				*/
	int		line_count;						/**< The total line count on the page, including a repeated header.				*/
};

/**
 * An area of a report page body, for use while paginating the data.
 */

struct report_pagination_area {
	osbool		active;							/**< TRUE if the area is active; FALSE if it is to be skipped.				*/

	int		ypos_offset;						/**< An offset to apply to YPos values for lines, to bring them into region range.	*/

	int		height;							/**< The total height of the contained lines, in OS Units.				*/

	unsigned	top_line;						/**< The first line in the range.							*/
	unsigned	bottom_line;						/**< The last line in the range.							*/
};

/**
 * Report status flags.
 */

enum report_status {
	REPORT_STATUS_NONE = 0x00,						/**< No status flags set.					*/
	REPORT_STATUS_MEMERR = 0x01,						/**< A memory allocation error has occurred, so stop allowing writes. */
	REPORT_STATUS_CLOSED = 0x02						/**< The report has been closed to writing.			*/
};

/**
 * Report display option flags.
 */

enum report_display {
	REPORT_DISPLAY_NONE		= 0x0000,				/**< No flags are in use.					*/
	REPORT_DISPLAY_LANDSCAPE	= 0x0001,				/**< Use the page in landscape format.				*/
	REPORT_DISPLAY_FIT_WIDTH	= 0x0002,				/**< Scale the output to fit the width of a page.		*/
	REPORT_DISPLAY_PAGINATED	= 0x0004,				/**< Display the pagination of the report on screen.		*/
	REPORT_DISPLAY_SHOW_GRID	= 0x0008,				/**< Include a grid around tabular data.			*/
	REPORT_DISPLAY_SHOW_NUMBERS	= 0x0010,				/**< Show page numbers in the page footer.			*/
	REPORT_DISPLAY_SHOW_TITLE	= 0x0020				/**< Show the report title in the page header.			*/
};

struct report {
	struct file_block	*file;						/**< The file that the report belongs to.			*/

	wimp_w			window;
	char			window_title[WINDOW_TITLE_LENGTH];
	wimp_w			toolbar;

	/* Report status flags. */

	enum report_status	flags;
	int			print_pending;

	/* Tab details */

	struct report_tabs_block	*tabs;

	/* Font data */

	struct report_fonts_block	*fonts;

	/* Display options. */

	enum report_display	display;

	/* Report content */

	int			width;						/**< The displayed width of the report data in OS Units.	*/
	int			height;						/**< The displayed height of the report data in OS Units.	*/

	int			linespace;					/**< The height allocated to a line of text, in OS Units.	*/
	int			rulespace;					/**< The height allocated to a horizontal rule, in OS Units.	*/

	struct report_textdump_block	*content;
	struct report_cell_block	*cells;
	struct report_line_block	*lines;

	struct report_page_block	*pages;
	struct report_region_block	*regions;

	unsigned			page_title;				/**< Textdump offset for the page title, if any.		*/

	/* Report template details. */

	struct analysis_report	*template;					/**< Pointer to an analysis report template.			*/

	/* Pointer to the next report in the list. */

	struct report		*next;
};


struct report			*report_print_report = NULL;			/**< The report to which the currently open Report Print dialogie belongs.			*/

static osbool			report_print_opt_text;				/**< TRUE if the current report is to be printed text format; FALSE to print graphically.	*/
static osbool			report_print_opt_textformat;			/**< TRUE if text formatting should be applied to the current text format print; else FALSE.	*/
static osbool			report_print_opt_fitwidth;			/**< TRUE if the graphics format print should be fitted to page width; else FALSE.		*/
static osbool			report_print_opt_rotate;			/**< TRUE if the graphics format print should be rotated to Landscape; FALSE for Portrait.	*/
static osbool			report_print_opt_pagenum;			/**< TRUE if the graphics format print should include page numbers; else FALSE.			*/

wimp_window			*report_window_def = NULL;			/**< The definition for the Report View window.							*/
wimp_window			*report_toolbar_def = NULL;			/**< The definition for the Report View toolbar.						*/

static wimp_menu		*report_view_menu = NULL;			/**< The Report View window menu handle.							*/
static wimp_menu		*report_view_include_menu = NULL;		/**< The Report View Include submenu handle.							*/
static wimp_menu		*report_view_layout_menu = NULL;		/**< The Report View Layout submenu handle.							*/

static struct saveas_block	*report_saveas_text = NULL;			/**< The Save Text saveas data handle.								*/
static struct saveas_block	*report_saveas_csv = NULL;			/**< The Save CSV saveas data handle.								*/
static struct saveas_block	*report_saveas_tsv = NULL;			/**< The Save TSV saveas data handle.								*/



static void			report_close_and_calculate(struct report *report);
static osbool			report_add_cell(struct report *report, char *text, enum report_cell_flags flags, int tab_bar, int tab_stop, unsigned *first_cell_offset);


static void			report_reflow_content(struct report *report);

static void			report_view_close_window_handler(wimp_close *close);
static void			report_view_redraw_handler(wimp_draw *redraw);
static void			report_view_toolbar_prepare(struct report *report);
static void			report_view_toolbar_click_handler(wimp_pointer *pointer);
static void			report_view_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void			report_view_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void			report_view_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void			report_view_redraw_handler(wimp_draw *redraw);
static void			report_view_redraw_flat_handler(struct report *report, wimp_draw *redraw, int ox, int oy);
static void			report_view_redraw_page_handler(struct report *report, wimp_draw *redraw, int ox, int oy);



static void			report_open_format_window(struct report *report, wimp_pointer *ptr);
static void			report_process_format_window(struct report *report, char *normal, char *bold, char* italic, char *bold_italic, int size, int spacing);

static void			report_open_print_window(struct report *report, wimp_pointer *ptr, osbool restore);
static struct report		*report_print_window_closed(struct report *report, void *data);

static osbool			report_save_text(char *filename, osbool selection, void *data);
static osbool			report_save_csv(char *filename, osbool selection, void *data);
static osbool			report_save_tsv(char *filename, osbool selection, void *data);
static void			report_export_text(struct report *report, char *filename, osbool formatting);
static void			report_export_delimited(struct report *report, char *filename, enum filing_delimit_type format, int filetype);

static void			report_print(struct report *report, osbool text, osbool textformat, osbool fitwidth, osbool rotate, osbool pagenum);
static void			report_start_print_job(char *filename);
static void			report_cancel_print_job(void);
static void			report_print_as_graphic(struct report *report, osbool fit_width, osbool rotate, osbool pagenum);
static void			report_handle_print_error(os_error *error, os_fw file, struct report_fonts_block *fonts);


static os_error *report_plot_page(struct report *report, struct report_page_data *page, os_coord *origin, os_box *clip);
static os_error *report_plot_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip);
static os_error *report_plot_text_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip);
static os_error *report_plot_page_number_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip);
static os_error *report_plot_lines_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip);
static os_error *report_plot_line(struct report *report, unsigned int line, os_coord *origin, os_box *clip);
static os_error *report_plot_cell(struct report *report, os_box *outline, char *content, enum report_cell_flags flags);

static osbool report_handle_message_set_printer(wimp_message *message);
static void report_repaginate_all(struct file_block *file);
static void report_paginate(struct report *report);
static void report_add_page_row(struct report *report, struct report_page_layout *layout,
		struct report_pagination_area *main_area, struct report_pagination_area *repeat_area, int row);
static int report_get_line_height(struct report *report, struct report_line_data *line_data);
static void report_set_display_option(struct report *report, enum report_display option, osbool state);
static void report_set_window_extent(struct report *report);
static osbool report_get_window_extent(struct report *report, int *x, int *y);



/**
 * Initialise the reporting system.
 *
 * \param *sprites		The application sprite area.
 */

void report_initialise(osspriteop_area *sprites)
{
	/* Report View Window. */

	report_window_def = templates_load_window("Report");
	report_window_def->sprite_area = sprites;

	report_toolbar_def = templates_load_window("ReportTB");
	report_toolbar_def->sprite_area = sprites;

	report_view_menu = templates_get_menu("ReportViewMenu");
	report_view_include_menu = templates_get_menu("ReportViewIncludeSubmenu");
	report_view_layout_menu = templates_get_menu("ReportViewLayoutSubmenu");
	ihelp_add_menu(report_view_menu, "ReportMenu");

	/* Save dialogue boxes. */

	report_saveas_text = saveas_create_dialogue(FALSE, "file_fff", report_save_text);
	report_saveas_csv = saveas_create_dialogue(FALSE, "file_dfe", report_save_csv);
	report_saveas_tsv = saveas_create_dialogue(FALSE, "file_fff", report_save_tsv);

	/* Initialise subsidiary parts of the report system. */

	report_font_dialogue_initialise();
	report_fonts_initialise();

	/* Register the Wimp message handlers. */

	event_add_message_handler(message_SET_PRINTER, EVENT_MESSAGE_INCOMING, report_handle_message_set_printer);
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

struct report *report_open(struct file_block *file, char *title, struct analysis_report *template)
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

	new->display = REPORT_DISPLAY_NONE;

	if (config_opt_read("ReportRotate"))
		new->display |= REPORT_DISPLAY_LANDSCAPE;
	if (config_opt_read("ReportShowPages"))
		new->display |= REPORT_DISPLAY_PAGINATED;
	if (config_opt_read("ReportFitWidth"))
		new->display |= REPORT_DISPLAY_FIT_WIDTH;
	if (config_opt_read("ReportShowTitle"))
		new->display |= REPORT_DISPLAY_SHOW_TITLE;
	if (config_opt_read("ReportShowPageNum"))
		new->display |= REPORT_DISPLAY_SHOW_NUMBERS;
	if (config_opt_read("ReportShowGrid"))
		new->display |= REPORT_DISPLAY_SHOW_GRID;

	new->page_title = REPORT_TEXTDUMP_NULL;

	new->tabs = report_tabs_create();
	if (new->tabs == NULL)
		new->flags |= REPORT_STATUS_MEMERR;

	new->fonts = report_fonts_create();
	if (new->fonts == NULL)
		new->flags |= REPORT_STATUS_MEMERR;

	new->content = report_textdump_create(0, 200, '\0');
	if (new->content == NULL)
		new->flags |= REPORT_STATUS_MEMERR;

	new->cells = report_cell_create(0);
	if (new->cells == NULL)
		new->flags |= REPORT_STATUS_MEMERR;

	new->lines = report_line_create(0);
	if (new->lines == NULL)
		new->flags |= REPORT_STATUS_MEMERR;

	new->pages = report_page_create(0);
	if (new->pages == NULL)
		new->flags |= REPORT_STATUS_MEMERR;

	new->regions = report_region_create(0);
	if (new->regions == NULL)
		new->flags |= REPORT_STATUS_MEMERR;

	new->window = NULL;
	string_copy(new->window_title, title, WINDOW_TITLE_LENGTH);
	new->toolbar = NULL;

	new->page_title = report_textdump_store(new->content, title);
	if (new->page_title == REPORT_TEXTDUMP_NULL)
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
	wimp_window_state	parent;
	int			xextent, yextent;
	os_error		*error;

	#ifdef DEBUG
	debug_printf("\\GClosing report");
	#endif

	if (report == NULL || (report->flags & REPORT_STATUS_MEMERR)) {
		error_msgs_report_error("NoMemReport");
		report_delete(report);
		return;
	}

	report_close_and_calculate(report);

	/* Set up the window title */

	report_window_def->title_data.indirected_text.text = report->window_title;

	#ifdef DEBUG
	debug_printf("Report window width: %d", report->width);
	#endif

	/* Position the report window. */

	transact_get_window_state(report->file, &parent);

	if (!report_get_window_extent(report, &xextent, &yextent)) {
		xextent = REPORT_MIN_WIDTH;
		yextent = REPORT_MIN_HEIGHT;
	}

	window_set_initial_area(report_window_def, xextent, yextent,
			parent.visible.x0 + CHILD_WINDOW_OFFSET + file_get_next_open_offset(report->file),
			parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

	error = xwimp_create_window(report_window_def, &(report->window));
	if (error != NULL) {
		report_delete(report);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Position the report toolbar pane. */

	windows_place_as_toolbar(report_window_def, report_toolbar_def, REPORT_TOOLBAR_HEIGHT - 4);

	error = xwimp_create_window(report_toolbar_def, &(report->toolbar));
	if (error != NULL) {
		report_delete(report);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	report_view_toolbar_prepare(report);

	/* Open the two windows. */

	ihelp_add_window(report->window, "Report", NULL);
	ihelp_add_window(report->toolbar, "ReportTB", NULL);

	windows_open(report->window);
	windows_open_nested_as_toolbar(report->toolbar, report->window,
			REPORT_TOOLBAR_HEIGHT - 4);

	/* Register event handles for the two windows. */

	event_add_window_user_data(report->window, report);
	event_add_window_menu(report->window, report_view_menu);
	event_add_window_close_event(report->window, report_view_close_window_handler);
	event_add_window_redraw_event(report->window, report_view_redraw_handler);
	event_add_window_menu_prepare(report->window, report_view_menu_prepare_handler);
	event_add_window_menu_selection(report->window, report_view_menu_selection_handler);
	event_add_window_menu_warning(report->window, report_view_menu_warning_handler);

	event_add_window_user_data(report->toolbar, report);
	event_add_window_menu(report->toolbar, report_view_menu);
	event_add_window_mouse_event(report->toolbar, report_view_toolbar_click_handler);
	event_add_window_menu_prepare(report->toolbar, report_view_menu_prepare_handler);
	event_add_window_menu_selection(report->toolbar, report_view_menu_selection_handler);
	event_add_window_menu_warning(report->toolbar, report_view_menu_warning_handler);

	event_add_window_icon_radio(report->toolbar, REPORT_PANE_PORTRAIT, FALSE);
	event_add_window_icon_radio(report->toolbar, REPORT_PANE_LANDSCAPE, FALSE);
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
	#ifdef DEBUG
	debug_printf("\\GClosing report and starting printing");
	#endif

	if (report == NULL || (report->flags & REPORT_STATUS_MEMERR)) {
		error_msgs_report_error("NoMemReport");
		report_delete(report);
		return;
	}

	report_close_and_calculate(report);

	if (!report_page_paginated(report->pages)) {
		error_msgs_report_error("PrintPgFail");
		report_delete(report);
		return;
	}

	report_print(report, text, textformat, fitwidth, rotate, pagenum);
}


/**
 * Close a report, if it is still open, reflow its contents and calculate
 * its dimensions.
 *
 * \param *report		The report to process.
 */

static void report_close_and_calculate(struct report *report)
{
	if (report == NULL || (report->flags & REPORT_STATUS_CLOSED))
		return;

	/* Mark the report as closed. */

	report->flags |= REPORT_STATUS_CLOSED;

	/* Update the data block to the required size. */

	report_textdump_close(report->content);
	report_cell_close(report->cells);
	report_line_close(report->lines);
	report_tabs_close(report->tabs);

	/* Set up the display details. */

	report_fonts_set_faces(report->fonts, config_str_read("ReportFontNormal"), config_str_read("ReportFontBold"), NULL, NULL);
	report_fonts_set_size(report->fonts, config_int_read("ReportFontSize") * 16, config_int_read("ReportFontLinespace"));

	report_reflow_content(report);

	/* Try to paginate the report. */

	report_paginate(report);
}


/**
 * Delete a report block, along with any window and data associated with
 * it.
 *
 * \param *report		The report to delete.
 */

void report_delete(struct report *report)
{
	struct file_block	*file;
	struct report		**rep;

	#ifdef DEBUG
	debug_printf("\\RDeleting report");
	#endif

	if (report == NULL)
		return;

	file = report->file;

	if (report->toolbar != NULL) {
		ihelp_remove_window(report->toolbar);
		event_delete_window(report->toolbar);
		wimp_delete_window(report->toolbar);
		report->toolbar = NULL;
	}

	if (report->window != NULL) {
		ihelp_remove_window(report->window);
		event_delete_window(report->window);
		wimp_delete_window(report->window);
		report->window = NULL;
	}

	/* Close any related dialogues. */

	report_font_dialogue_force_close(report);

	/* Free the flex blocks. */

	report_textdump_destroy(report->content);
	report_cell_destroy(report->cells);
	report_line_destroy(report->lines);
	report_fonts_destroy(report->fonts);
	report_tabs_destroy(report->tabs);
	report_page_destroy(report->pages);
	report_region_destroy(report->regions);

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
 * \param tab_bar		The tab bar to use.
 * \param *text			The line of text to write.
 */

void report_write_line(struct report *report, int tab_bar, char *text)
{
	char			*c, *copy;
	unsigned		first_cell_offset;
	int			tab_stop, cell_count;
	enum report_line_flags	line_flags;
	enum report_cell_flags	cell_flags;

	#ifdef DEBUG
	debug_printf("Print line: %s", text);
	#endif

	if ((report->flags & REPORT_STATUS_MEMERR) || (report->flags & REPORT_STATUS_CLOSED))
		return;

	/* Allocate a buffer to hold the copied string. */

	copy = malloc(strlen(text) + 1);
	if (copy == NULL) {
		report->flags |= REPORT_STATUS_MEMERR;
		return;
	}

	tab_stop = 0;
	cell_count = 0;

	line_flags = REPORT_LINE_FLAGS_NONE;
	cell_flags = REPORT_CELL_FLAGS_NONE;

	first_cell_offset = REPORT_CELL_NULL;

	c = copy;

	while ((*text != '\0') && !(report->flags & REPORT_STATUS_MEMERR)) {
		if (*text == '\\') {
			text++;

			switch (*text++) {
			case 'b':
				cell_flags |= REPORT_CELL_FLAGS_BOLD;
				break;

			case 'c':
				cell_flags |= REPORT_CELL_FLAGS_CENTRE;
				break;

			case 'd':
				cell_flags |= REPORT_CELL_FLAGS_NUMERIC;
				break;

			case 'i':
				cell_flags |= REPORT_CELL_FLAGS_INDENT;
				break;

			case 'k':
				line_flags |= REPORT_LINE_FLAGS_KEEP_TOGETHER;
				break;

			case 'o':
				cell_flags |= REPORT_CELL_FLAGS_ITALIC;
				break;

			case 'r':
				cell_flags |= REPORT_CELL_FLAGS_RIGHT;
				break;

			case 's':
				cell_flags |= REPORT_CELL_FLAGS_SPILL;
				break;

			case 't':
				*c++ = '\0';

				if (report_add_cell(report, copy, cell_flags, tab_bar, tab_stop, &first_cell_offset))
					cell_count++;

				tab_stop++;

				c = copy;
				cell_flags = REPORT_CELL_FLAGS_NONE;
				break;

			case 'u':
				cell_flags |= REPORT_CELL_FLAGS_UNDERLINE;
				break;
				
			case 'v':
				line_flags |= REPORT_LINE_FLAGS_RULE_BELOW;
				cell_flags |= REPORT_CELL_FLAGS_RULE_AFTER;
				break;
			}
		} else {
			*c++ = *text++;
		}
	}

	*c = '\0';

	if (report_add_cell(report, copy, cell_flags, tab_bar, tab_stop, &first_cell_offset))
		cell_count++;

	/* Store the line in the report. */

	if (!report_line_add(report->lines, first_cell_offset, cell_count, tab_bar, line_flags))
		report->flags |= REPORT_STATUS_MEMERR;

	free(copy);
}


/**
 * Add a cell to a report.
 *
 * \param *report		The report to add the cell to.
 * \param *text			Pointer to the text to be stored in the cell.
 * \param flags			The flags to be applied to the cell.
 * \param tab_bar		The tab bar to which the cell belongs.
 * \param tab_stop		The tab stop to which the cell belongs.
 * \param *first_cell_offset	Pointer to a variable holding the first cell offet,
 *				to be updated on return.
 * \return			TRUE if a cell was added; otherwise FALSE.
 */

static osbool report_add_cell(struct report *report, char *text, enum report_cell_flags flags, int tab_bar, int tab_stop, unsigned *first_cell_offset)
{
	unsigned			content_offset, cell_offset;
	enum report_tabs_stop_flags	tab_flags = REPORT_TABS_STOP_FLAGS_NONE;

	if (text != NULL && *text != '\0') {
		content_offset = report_textdump_store(report->content, text);
		if (content_offset == REPORT_TEXTDUMP_NULL) {
			report->flags |= REPORT_STATUS_MEMERR;
			return FALSE;
		}
	} else {
		content_offset = REPORT_TEXTDUMP_NULL;
	}

	if (content_offset != REPORT_TEXTDUMP_NULL || flags != REPORT_CELL_FLAGS_NONE) {
		cell_offset = report_cell_add(report->cells, content_offset, tab_stop, flags);
		if (cell_offset == REPORT_CELL_NULL) {
			report->flags |= REPORT_STATUS_MEMERR;
			return FALSE;
		}
	} else {
		cell_offset = REPORT_CELL_NULL;
	}

	if (*first_cell_offset == REPORT_CELL_NULL)
		*first_cell_offset = cell_offset;

	if (flags & REPORT_CELL_FLAGS_RULE_AFTER)
		tab_flags |= REPORT_TABS_STOP_FLAGS_RULE_AFTER;

	report_tabs_set_stop_flags(report->tabs, tab_bar, tab_stop, tab_flags);

	return (cell_offset == REPORT_CELL_NULL) ? FALSE : TRUE;
}


/**
 * Reflow the text in a report to suit the current content and display settings.
 *
 * \param *report		The report to reflow.
 * \return			The required width, in OS units, of the window
 *				to contain the report.
 */

static void report_reflow_content(struct report *report)
{
	int			font_width, text_width;
	int			line_space, rule_space, ypos;
	unsigned		line, cell;
	char			*content_base, *content;
	size_t			line_count;
	struct report_line_data	*line_data;
	struct report_cell_data	*cell_data;

	if (report == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\GFormatting report");
	#endif

	/* Reset the flags used to keep track of items. */

	report_tabs_reset_columns(report->tabs);

	/* Find the font to be used by the report. */

	report_fonts_find(report->fonts);

	line_space = report_fonts_get_linespace(report->fonts);

	content_base = report_textdump_get_base(report->content);

	/* Work through the report, line by line, calculating the column positions. */

	line_count = (content_base != NULL) ? report_line_get_count(report->lines) : 0;

	ypos = 0;

	for (line = 0; line < line_count; line++) {
		line_data = report_line_get_info(report->lines, line);
		if (line_data == NULL)
			continue;

		rule_space = 0;

		if (report->display & REPORT_DISPLAY_SHOW_GRID) {
			if (line_data->flags & REPORT_LINE_FLAGS_RULE_ABOVE)
				rule_space += 2 * REPORT_GRID_LINE_MARGIN;

			if (line_data->flags & REPORT_LINE_FLAGS_RULE_BELOW)
				rule_space += 2 * REPORT_GRID_LINE_MARGIN;
		}

		ypos += (line_space + rule_space);
		line_data->ypos = -ypos;

		if (!report_tabs_start_line_format(report->tabs, line_data->tab_bar)) {
			report->flags |= REPORT_STATUS_MEMERR;
			continue;
		}

		for (cell = 0; cell < line_data->cell_count; cell++) {
			cell_data = report_cell_get_info(report->cells, line_data->first_cell + cell);
			if (cell_data == NULL || (cell_data->offset == REPORT_TEXTDUMP_NULL && cell_data->flags == REPORT_CELL_FLAGS_NONE))
				continue;

			if (cell_data->flags & REPORT_CELL_FLAGS_SPILL) {
				font_width = REPORT_TABS_SPILL_WIDTH;
				text_width = REPORT_TABS_SPILL_WIDTH;
			} else if (cell_data->offset != REPORT_TEXTDUMP_NULL) {
				content = content_base + cell_data->offset;

				report_fonts_get_string_width(report->fonts, content, cell_data->flags, &font_width);
				text_width = string_ctrl_strlen(content);

				/* If the column is indented, add the indent to the column widths. */

				if (cell_data->flags & REPORT_CELL_FLAGS_INDENT) {
					font_width += REPORT_COLUMN_INDENT;
					text_width += REPORT_TEXT_COLUMN_INDENT;
				}
			} else {
				font_width = 0;
				text_width = 0;
			}

			report_tabs_set_cell_width(report->tabs, cell_data->tab_stop, font_width, text_width);
		}

		report_tabs_end_line_format(report->tabs);
	}

	/* Lose the fonts used in the report. */

	report_fonts_lose(report->fonts);

	/* Set the dimensions of the report. */

	report->width = report_tabs_calculate_columns(report->tabs, (report->display & REPORT_DISPLAY_SHOW_GRID) ? TRUE : FALSE);
	report->height = ypos;

	report->linespace = line_space;
	report->rulespace = 0;
}


/**
 * Test for any pending report print jobs on the specified file.
 *
 * \param *file			The file to test.
 * \return			TRUE if there are pending jobs; FALSE if not.
 */

osbool report_get_pending_print_jobs(struct file_block *file)
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
		analysis_template_save_force_template_close(report->template);

		ihelp_remove_window(report->window);
		event_delete_window(report->window);
		wimp_delete_window(report->window);
		report->window = NULL;
	}

	if (report->print_pending == 0)
		report_delete(report);
}


/**
 * Set the states of the icons in the Report View toolbar.
 */

static void report_view_toolbar_prepare(struct report *report)
{
	osbool paginated;

	if (report == NULL || report->toolbar == NULL)
		return;

	paginated = report_page_paginated(report->pages);

	icons_set_group_shaded(report->toolbar, !paginated, 6,
			REPORT_PANE_SHOW_PAGES, REPORT_PANE_PORTRAIT,
			REPORT_PANE_LANDSCAPE, REPORT_PANE_FIT_WIDTH,
			REPORT_PANE_SHOW_TITLE, REPORT_PANE_SHOW_PAGE_NUMBERS);

	icons_set_selected(report->toolbar, REPORT_PANE_SHOW_PAGES, paginated && (report->display & REPORT_DISPLAY_PAGINATED));
	icons_set_selected(report->toolbar, REPORT_PANE_PORTRAIT, (paginated == FALSE) || !(report->display & REPORT_DISPLAY_LANDSCAPE));
	icons_set_selected(report->toolbar, REPORT_PANE_LANDSCAPE, (paginated == TRUE) && (report->display & REPORT_DISPLAY_LANDSCAPE));
	icons_set_selected(report->toolbar, REPORT_PANE_FIT_WIDTH, paginated && (report->display & REPORT_DISPLAY_FIT_WIDTH));
	icons_set_selected(report->toolbar, REPORT_PANE_SHOW_TITLE, paginated && (report->display & REPORT_DISPLAY_SHOW_TITLE));
	icons_set_selected(report->toolbar, REPORT_PANE_SHOW_GRID, (report->display & REPORT_DISPLAY_SHOW_GRID) ? TRUE : FALSE);
	icons_set_selected(report->toolbar, REPORT_PANE_SHOW_PAGE_NUMBERS, paginated && (report->display & REPORT_DISPLAY_SHOW_NUMBERS));
}


/**
 * Process mouse clicks in the Report View pane.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void report_view_toolbar_click_handler(wimp_pointer *pointer)
{
	struct report		*report;

	report = event_get_window_user_data(pointer->w);
	if (report == NULL)
		return;

	/* Decode the mouse click. */

	switch (pointer->i) {
	case REPORT_PANE_PARENT:
		if (pointer->buttons == wimp_CLICK_SELECT)
			transact_bring_window_to_top(report->file);
		break;

	case REPORT_PANE_SAVE:
		break;

	case REPORT_PANE_PRINT:
		if (pointer->buttons == wimp_CLICK_SELECT)
			report_open_print_window(report, pointer, config_opt_read("RememberValues"));
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			report_open_print_window(report, pointer, !config_opt_read("RememberValues"));
		break;

	case REPORT_PANE_SHOW_PAGES:
		report_set_display_option(report, REPORT_DISPLAY_PAGINATED, icons_get_selected(report->toolbar, REPORT_PANE_SHOW_PAGES));
		break;

	case REPORT_PANE_PORTRAIT:
		report_set_display_option(report, REPORT_DISPLAY_LANDSCAPE, FALSE);
		break;

	case REPORT_PANE_LANDSCAPE:
		report_set_display_option(report, REPORT_DISPLAY_LANDSCAPE, TRUE);
		break;

	case REPORT_PANE_FIT_WIDTH:
		report_set_display_option(report, REPORT_DISPLAY_FIT_WIDTH, icons_get_selected(report->toolbar, REPORT_PANE_FIT_WIDTH));
		break;

	case REPORT_PANE_SHOW_TITLE:
		report_set_display_option(report, REPORT_DISPLAY_SHOW_TITLE, icons_get_selected(report->toolbar, REPORT_PANE_SHOW_TITLE));
		break;

	case REPORT_PANE_SHOW_GRID:
		report_set_display_option(report, REPORT_DISPLAY_SHOW_GRID, icons_get_selected(report->toolbar, REPORT_PANE_SHOW_GRID));
		break;

	case REPORT_PANE_SHOW_PAGE_NUMBERS:
		report_set_display_option(report, REPORT_DISPLAY_SHOW_NUMBERS, icons_get_selected(report->toolbar, REPORT_PANE_SHOW_PAGE_NUMBERS));
		break;

	case REPORT_PANE_FORMAT:
		report_open_format_window(report, pointer);
		break;
	}
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
	osbool		paginated;

	report = event_get_window_user_data(w);

	if (report == NULL)
		return;

	saveas_initialise_dialogue(report_saveas_text, NULL, "DefRepFile", NULL, FALSE, FALSE, report);
	saveas_initialise_dialogue(report_saveas_csv, NULL, "DefCSVFile", NULL, FALSE, FALSE, report);
	saveas_initialise_dialogue(report_saveas_tsv, NULL, "DefTSVFile", NULL, FALSE, FALSE, report);

	paginated = report_page_paginated(report->pages);

	menus_shade_entry(report_view_menu, REPVIEW_MENU_SHOWPAGES, paginated == FALSE);
	menus_shade_entry(report_view_menu, REPVIEW_MENU_LAYOUT, paginated == FALSE);
	menus_shade_entry(report_view_menu, REPVIEW_MENU_TEMPLATE, report->template == NULL);

	menus_shade_entry(report_view_include_menu, REPVIEW_MENU_INCLUDE_TITLE, paginated == FALSE);
	menus_shade_entry(report_view_include_menu, REPVIEW_MENU_INCLUDE_PAGE_NUM, paginated == FALSE);

	menus_shade_entry(report_view_layout_menu, REPVIEW_MENU_LAYOUT_PORTRAIT, paginated == FALSE);
	menus_shade_entry(report_view_layout_menu, REPVIEW_MENU_LAYOUT_LANDSCAPE, paginated == FALSE);
	menus_shade_entry(report_view_layout_menu, REPVIEW_MENU_LAYOUT_FIT_WIDTH, paginated == FALSE);

	menus_tick_entry(report_view_menu, REPVIEW_MENU_SHOWPAGES, (paginated == TRUE) && (report->display & REPORT_DISPLAY_PAGINATED));

	menus_tick_entry(report_view_include_menu, REPVIEW_MENU_INCLUDE_TITLE, paginated && (report->display & REPORT_DISPLAY_SHOW_TITLE));
	menus_tick_entry(report_view_include_menu, REPVIEW_MENU_INCLUDE_PAGE_NUM, paginated && (report->display & REPORT_DISPLAY_SHOW_NUMBERS));
	menus_tick_entry(report_view_include_menu, REPVIEW_MENU_INCLUDE_GRID, (report->display & REPORT_DISPLAY_SHOW_GRID) ? TRUE : FALSE);

	menus_tick_entry(report_view_layout_menu, REPVIEW_MENU_LAYOUT_PORTRAIT, (paginated == FALSE) || !(report->display & REPORT_DISPLAY_LANDSCAPE));
	menus_tick_entry(report_view_layout_menu, REPVIEW_MENU_LAYOUT_LANDSCAPE, (paginated == TRUE) && (report->display & REPORT_DISPLAY_LANDSCAPE));
	menus_tick_entry(report_view_layout_menu, REPVIEW_MENU_LAYOUT_FIT_WIDTH, paginated && (report->display & REPORT_DISPLAY_FIT_WIDTH));
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
	case REPVIEW_MENU_SHOWPAGES:
		report_set_display_option(report, REPORT_DISPLAY_PAGINATED, !(report->display & REPORT_DISPLAY_PAGINATED));
		break;

	case REPVIEW_MENU_FORMAT:
		report_open_format_window(report, &pointer);
		break;

	case REPVIEW_MENU_INCLUDE:
		switch(selection->items[1]) {
		case REPVIEW_MENU_INCLUDE_TITLE:
			report_set_display_option(report, REPORT_DISPLAY_SHOW_TITLE, !(report->display & REPORT_DISPLAY_SHOW_TITLE));
			break;

		case REPVIEW_MENU_INCLUDE_PAGE_NUM:
			report_set_display_option(report, REPORT_DISPLAY_SHOW_NUMBERS, !(report->display & REPORT_DISPLAY_SHOW_NUMBERS));
			break;

		case REPVIEW_MENU_INCLUDE_GRID:
			report_set_display_option(report, REPORT_DISPLAY_SHOW_GRID, !(report->display & REPORT_DISPLAY_SHOW_GRID));
			break;
		}
		break;

	case REPVIEW_MENU_LAYOUT:
		switch(selection->items[1]) {
		case REPVIEW_MENU_LAYOUT_PORTRAIT:
			report_set_display_option(report, REPORT_DISPLAY_LANDSCAPE, FALSE);
			break;

		case REPVIEW_MENU_LAYOUT_LANDSCAPE:
			report_set_display_option(report, REPORT_DISPLAY_LANDSCAPE, TRUE);
			break;

		case REPVIEW_MENU_LAYOUT_FIT_WIDTH:
			report_set_display_option(report, REPORT_DISPLAY_FIT_WIDTH, !(report->display & REPORT_DISPLAY_FIT_WIDTH));
			break;
		}
		break;

	case REPVIEW_MENU_PRINT:
		report_open_print_window(report, &pointer, config_opt_read("RememberValues"));
		break;

	case REPVIEW_MENU_TEMPLATE:
		analysis_template_save_open_window(report->template, &pointer);
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
	int		ox, oy;
	struct report	*report;
	osbool		paginated, more;

	report = event_get_window_user_data(redraw->w);

	if (report == NULL)
		return;

	/* Find the required font, set it and calculate the font size from the linespacing in points. */

	report_fonts_find(report->fonts);
	paginated = report_page_paginated(report->pages);

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll - REPORT_TOOLBAR_HEIGHT;

	while (more) {
		if (paginated && (report->display & REPORT_DISPLAY_PAGINATED))
			report_view_redraw_page_handler(report, redraw, ox, oy);
		else
			report_view_redraw_flat_handler(report, redraw, ox, oy);

		more = wimp_get_rectangle(redraw);
	}

	report_fonts_lose(report->fonts);
}


/**
 * Handle the redraw of a rectangle in the flat display mode.
 *
 * \param *report		The report to be redrawn.
 * \param *redraw		The Wimp Redraw Event data block.
 * \param ox			The X redraw origin.
 * \param oy			The Y redraw origin.
 */

static void report_view_redraw_flat_handler(struct report *report, wimp_draw *redraw, int ox, int oy)
{
	unsigned	top, base, y;
	size_t		line_count;
	os_coord	origin;
	os_error	*error;

	line_count = report_line_get_count(report->lines);

	top = report_line_find_from_ypos(report->lines, redraw->clip.y1 - oy);
	base = report_line_find_from_ypos(report->lines, redraw->clip.y0 - oy);

	/* Plot the background. */

	wimp_set_colour(wimp_COLOUR_WHITE);
	os_plot(os_MOVE_TO, redraw->clip.x0, redraw->clip.y1);
	os_plot(os_PLOT_RECTANGLE + os_PLOT_TO, redraw->clip.x1, redraw->clip.y0);

	/* Plot Text. */

	origin.x = ox + REPORT_LEFT_MARGIN;
	origin.y = oy - REPORT_TOP_MARGIN;

	for (y = top; y < line_count && y <= base; y++) {
		error = report_plot_line(report, y, &origin, &(redraw->clip));
		if (error != NULL)
			debug_printf("Redraw error: %s", error->errmess);
	}
}


/**
 * Handle the redraw of a rectangle in the page display mode.
 *
 * \param *report		The report to be redrawn.
 * \param *redraw		The Wimp Redraw Event data block.
 * \param ox			The X redraw origin.
 * \param oy			The Y redraw origin.
 */

static void report_view_redraw_page_handler(struct report *report, wimp_draw *redraw, int ox, int oy)
{
	int				x0, y0, x1, y1, x, y;
	unsigned			page;
	struct report_page_data		*data;
	os_box				outline;
	os_coord			origin;

	if (report == NULL || redraw == NULL)
		return;

	/* Identify the range of pages covered. */

	x0 = report_page_find_from_xpos(report->pages, redraw->clip.x0 - ox, FALSE);
	y0 = report_page_find_from_ypos(report->pages, redraw->clip.y1 - oy, FALSE);
	x1 = report_page_find_from_xpos(report->pages, redraw->clip.x1 - ox, TRUE);
	y1 = report_page_find_from_ypos(report->pages, redraw->clip.y0 - oy, TRUE);

	/* Plot the background. */

	wimp_set_colour(wimp_COLOUR_LIGHT_GREY);
	os_plot(os_MOVE_TO, redraw->clip.x0, redraw->clip.y1);
	os_plot(os_PLOT_RECTANGLE + os_PLOT_TO, redraw->clip.x1, redraw->clip.y0);

	/* Plot each of the pages which fall into the rectangle. */

	for (x = x0; x <= x1; x++) {
		for (y = y0; y <= y1; y++) {
			page = report_page_get_outline(report->pages, x, y, &outline);
			if (page == REPORT_PAGE_NONE)
				continue;

			/* Calculate the page origin point. */

			origin.x = ox + outline.x0;
			origin.y = oy + outline.y1;

			/* Plot the on-screen page background. */

			outline.x0 = (ox + outline.x0 > redraw->clip.x0) ? ox + outline.x0 : redraw->clip.x0;
			outline.y0 = (oy + outline.y0 > redraw->clip.y0) ? oy + outline.y0 : redraw->clip.y0;

			outline.x1 = (ox + outline.x1 < redraw->clip.x1) ? ox + outline.x1 : redraw->clip.x1;
			outline.y1 = (oy + outline.y1 < redraw->clip.y1) ? oy + outline.y1 : redraw->clip.y1;

			wimp_set_colour(wimp_COLOUR_WHITE);
			os_plot(os_MOVE_TO, outline.x0, outline.y1);
			os_plot(os_PLOT_RECTANGLE + os_PLOT_TO, outline.x1, outline.y0);

			/* Plot the page itself. */

			data = report_page_get_info(report->pages, page);
			if (data == NULL)
				continue;

			report_plot_page(report, data, &origin, &(redraw->clip));
		}
	}
}


/**
 * Force the readraw of all the open reports associated with a file.
 *
 * \param *file			The file on which to force a redraw.
 */

void report_redraw_all(struct file_block *file)
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
 * Open the Report Format dialogue for a given report view.
 *
 * \param *report		The report to own the dialogue.
 * \param *ptr			The current Wimp pointer position.
 */

static void report_open_format_window(struct report *report, wimp_pointer *ptr)
{
	char	normal[font_NAME_LIMIT], bold[font_NAME_LIMIT], italic[font_NAME_LIMIT], bold_italic[font_NAME_LIMIT];
	int	size, spacing;

	if (report == NULL || ptr == NULL)
		return;

	report_fonts_get_faces(report->fonts, normal, bold, italic, bold_italic, font_NAME_LIMIT);
	report_fonts_get_size(report->fonts, &size, &spacing);

	report_font_dialogue_open(ptr, report, report_process_format_window,
			normal, bold, italic, bold_italic, size, spacing);
}


/**
 * Take the contents of an updated report format window and process the data.
 */

static void report_process_format_window(struct report *report, char *normal, char *bold, char* italic, char *bold_italic, int size, int spacing)
{
	if (report == NULL)
		return;

	/* Extract the information. */

	report_fonts_set_faces(report->fonts, normal, bold, italic, bold_italic);
	report_fonts_set_size(report->fonts, size, spacing);

	/* Tidy up and redraw the windows */

	report_reflow_content(report);

	/* Calculate the new window extents. */

	report_set_window_extent(report);

	/* Redraw the window. */

	if (report->window != NULL)
		windows_redraw(report->window);
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
	if (report == NULL || report->file == NULL)
		return;

	print_dialogue_open_simple(report->file->print, ptr, restore, "PrintReport", NULL, report_print_window_closed, report);
}


/**
 * Callback from the printing module, when Print is selected in the simple print
 * dialogue.  Start the printing process proper.
 *
 * \param *report		Printing-system supplied report handle, which should be NULL.
 * \param *data			The report to be printed.
 * \return			Pointer to the report to print, or NULL on failure.
 */

static struct report *report_print_window_closed(struct report *report, void *data)
{
	#ifdef DEBUG
	debug_printf("Report print received data from simple print window");
	#endif

	if (report != NULL || data == NULL)
		return NULL;

	return data;
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

	report_export_delimited(report, filename, DELIMIT_QUOTED_COMMA, dataxfer_TYPE_CSV);
	
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
		
	report_export_delimited(report, filename, DELIMIT_TAB, dataxfer_TYPE_TSV);
	
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
	FILE			*out;
	int			line, cell, j, indent, width, column, escape;
	char			*content_base, *content;
	size_t			line_count;
	struct report_line_data	*line_data;
	struct report_cell_data	*cell_data;
	struct report_tabs_stop	*tab_stop;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	content_base = report_textdump_get_base(report->content);
	line_count = (content_base != NULL) ? report_line_get_count(report->lines) : 0;

	for (line = 0; line < line_count; line++) {
		line_data = report_line_get_info(report->lines, line);
		if (line_data == NULL)
			continue;

		column = 0;

		for (cell = 0; cell < line_data->cell_count; cell++) {
			cell_data = report_cell_get_info(report->cells, line_data->first_cell + cell);
			if (cell_data == NULL || cell_data->offset == REPORT_TEXTDUMP_NULL)
				continue;

			tab_stop = report_tabs_get_stop(report->tabs, line_data->tab_bar, cell_data->tab_stop);
			if (tab_stop == NULL)
				continue;

			/* Pad out with spaces to the desired column position. */

			while (column < tab_stop->text_left) {
				fputc(' ', out);
				column++;
			}

			content = content_base + cell_data->offset;

			escape = (cell_data->flags & REPORT_CELL_FLAGS_BOLD) ? 0x01 : 0x00;
			if (cell_data->flags & REPORT_CELL_FLAGS_UNDERLINE)
				escape |= 0x08;

			indent = (cell_data->flags & REPORT_CELL_FLAGS_INDENT) ? REPORT_TEXT_COLUMN_INDENT : 0;
			width = strlen(content);

			if (cell_data->flags & REPORT_CELL_FLAGS_RIGHT)
				indent = tab_stop->text_width - width;

			/* Output the indent spaces. */

			for (j = 0; j < indent; j++)
				fputc(' ', out);

			column += (indent + width);

			/* Output fancy text formatting codes (used when printing formatted text) */

			if (formatting && escape != 0) {
				fputc((char) 27, out);
				fputc((char) (0x80 | escape), out);
			}

			/* Output the actual field data. */

			fprintf(out, "%s", content);

			/* Output fancy text formatting codes (used when printing formatted text) */

			if (formatting && escape != 0) {
				fputc((char) 27, out);
				fputc((char) 0x80, out);
			}
		}

		fputc('\n', out);
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) (formatting) ? dataxfer_TYPE_FANCYTEXT : osfile_TYPE_TEXT);

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
	FILE				*out;
	int				line, cell;
	char				*content_base, *content;
	size_t				line_count;
	enum filing_delimit_flags	delimit_flags;
	struct report_line_data		*line_data;
	struct report_cell_data		*cell_data;

	out = fopen(filename, "w");

	if (out == NULL) {
		error_msgs_report_error("FileSaveFail");
		return;
	}

	hourglass_on();

	content_base = report_textdump_get_base(report->content);
	line_count = (content_base != NULL) ? report_line_get_count(report->lines) : 0;

	for (line = 0; line < line_count; line++) {
		line_data = report_line_get_info(report->lines, line);
		if (line_data == NULL)
			continue;

		for (cell = 0; cell < line_data->cell_count; cell++) {
			cell_data = report_cell_get_info(report->cells, line_data->first_cell + cell);
			if (cell_data == NULL)
				continue;

			content = content_base + cell_data->offset;

			/* Output the actual field data. */

			delimit_flags = ((cell + 1) >= line_data->cell_count) ? DELIMIT_LAST : 0;

			if (cell_data->flags & REPORT_CELL_FLAGS_NUMERIC)
				delimit_flags |= DELIMIT_NUM;

			filing_output_delimited_field(out, content, format, delimit_flags);
		}
	}

	/* Close the file and set the type correctly. */

	fclose(out);
	osfile_set_type(filename, (bits) filetype);

	hourglass_off();
}


/**
 * Send a report off to the printing system.
 *
 * \param *data			The report to be printed.
 * \param text			TRUE to use text mode printing.
 * \param textformat		TRUE to use formatted text mode printing.
 * \param fitwidth		TRUE to scale graphics printing to the page width.
 * \param rotate		TRUE to rotate graphics printing into Landscape mode.
 * \param pagenum		TRUE to include page numbers in graphics printing.
 */

static void report_print(struct report *report, osbool text, osbool textformat, osbool fitwidth, osbool rotate, osbool pagenum)
{
	report_print_report = report;

	/* Extract the information. */

	report_print_opt_text = text;
	report_print_opt_textformat = textformat;
	report_print_opt_fitwidth = fitwidth;
	report_print_opt_rotate = rotate;
	report_print_opt_pagenum = pagenum;

	/* Start the print dialogue process.
	 * This process is also used by the direct report print function
	 * report_close_and_print(), so the two probably can't co-exist.
	 */

	report_print_report->print_pending++;

	print_protocol_send_start_print_save(report_start_print_job, report_cancel_print_job, report_print_opt_text);
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
	os_coord			position, origin;
	os_box				redraw, rectangle;
	char				title[REPORT_PRINT_TITLE_LENGTH];
	pdriver_features		features;
	osbool				more;
	size_t				page_count;
	unsigned			page, region;
	os_hom_trfm			*scaling_matrix;
	struct report_page_data		*page_data;
	struct report_region_data	*region_data;

	/* If the report hasn't been paginated, we can't continue. */

	if (!report_page_paginated(report->pages))
		return;


	hourglass_on();

	/* Get the printer driver settings. */

	error = xpdriver_info(NULL, NULL, NULL, &features, NULL, NULL, NULL, NULL);
	if (error != NULL)
		return;

	/* Find the fonts we will use. */

	report_fonts_find(report->fonts);

	/* Open a printout file. */

	error = xosfind_openoutw(osfind_NO_PATH, "printer:", NULL, &out);
	if (error != NULL) {
		report_handle_print_error(error, out, report->fonts);
		return;
	}

	/* Start a print job. */

	msgs_param_lookup("PJobTitle", title, REPORT_PRINT_TITLE_LENGTH, report->window_title, NULL, NULL, NULL);
	error = xpdriver_select_jobw(out, title, NULL);
	if (error != NULL) {
		report_handle_print_error(error, out, report->fonts);
		return;
	}

	/* Declare the fonts we are using, if required. */

	if (features & pdriver_FEATURE_DECLARE_FONT) {
		error = report_fonts_declare(report->fonts);
		if (error != NULL) {
			report_handle_print_error(error, out, report->fonts);
			return;
		}
	}

	/* Begin the process of outputting the pages. */

	page_count = report_page_get_count(report->pages);

	for (page = 0; page < page_count; page++) {
		page_data = report_page_get_info(report->pages, page);
		if (page_data == NULL)
			continue;

		scaling_matrix = report_page_get_transform(report->pages);
		if (scaling_matrix == NULL)
			continue;

		for (region = page_data->first_region; region < page_data->first_region + page_data->region_count; region++) {
			region_data = report_region_get_info(report->regions, region);
			if (region_data == NULL)
				continue;

			/* Add a small margin around the region, to allow for scaling errors. */

			rectangle.x0 = region_data->position.x0 - REPORT_PRINT_RECTANGLE_MARGIN;
			rectangle.y0 = region_data->position.y0 - REPORT_PRINT_RECTANGLE_MARGIN;
			rectangle.x1 = region_data->position.x1 + REPORT_PRINT_RECTANGLE_MARGIN;
			rectangle.y1 = region_data->position.y1 + REPORT_PRINT_RECTANGLE_MARGIN;

			/* Calculate the position of the region's rectangle on screen, and pass
			 * the details to the printer driver.
			 */

			if (!report_page_calculate_position(report->pages, &rectangle,
					(report->display & REPORT_DISPLAY_LANDSCAPE) ? TRUE : FALSE, &position))
				continue;

			error = xpdriver_give_rectangle(region, &rectangle, scaling_matrix, &position, os_COLOUR_TRANSPARENT);
			if (error != NULL) {
				report_handle_print_error(error, out, report->fonts);
				return;
			}
		}

		error = xpdriver_draw_page(1, &redraw, 0, 0, &more, (int *) &region);
		if (error != NULL) {
			report_handle_print_error(error, out, report->fonts);
			return;
		}

		/* Perform the redraw. */

		origin.x = 0;
		origin.y = 0;

		while (more) {
			region_data = report_region_get_info(report->regions, region);
			if (region_data == NULL)
				continue;

			/* Plot the region. */

			error = report_plot_region(report, region_data, &origin, &redraw);
			if (error != NULL) {
				report_handle_print_error(error, out, report->fonts);
				return;
			}

			error = xpdriver_get_rectangle(&redraw, &more, (int *) &region);
			if (error != NULL) {
				report_handle_print_error(error, out, report->fonts);
				return;
			}
		}
	}

	/* Terminate the print job. */

	error = xpdriver_end_jobw(out);

	if (error != NULL) {
		report_handle_print_error(error, out, report->fonts);
		return;
	}

	/* Close the printout file. */

	xosfind_closew(out);

	report_fonts_lose(report->fonts);

	hourglass_off();
}


/**
 * Clean up after errors in the printing process.
 *
 * \param *error	The OS error block rescribing the error.
 * \param file		The print output file handle.
 * \param *fonts	The Report Fonts instance in use.
 */

static void report_handle_print_error(os_error *error, os_fw file, struct report_fonts_block *fonts)
{
	if (file != 0) {
		xpdriver_abort_jobw(file);
		xosfind_closew(file);
	}

	if (fonts != NULL)
		report_fonts_lose(fonts);

	hourglass_off();
	error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
}














/**
 * Plot the contents of a report page.
 *
 * \param *report		The report instance holding the page.
 * \param *page			Pointer to the page data.
 * \param *origin		Pointer to an OS Coord holding the page origin
 *				for the current target in OS Units.
 * \param *clip			The output clipping box on OS Units.
 * \return			Pointer to an error block on failure, or NULL.
 */

static os_error *report_plot_page(struct report *report, struct report_page_data *page, os_coord *origin, os_box *clip)
{
	unsigned			region;
	struct report_region_data	*data;
	os_error			*error = NULL;

	if (report == NULL || page == NULL || origin == NULL || clip == NULL)
		return NULL;

	/* If the page has no regions, there's nothing to do! */

	if (page->first_region == REPORT_REGION_NONE || page->region_count == 0)
		return NULL;

	/* Redraw all of the regions on the page. */

	for (region = 0; region < page->region_count; region++) {
		data = report_region_get_info(report->regions, page->first_region + region);
		if (data == NULL)
			continue;

		/* Plot the region. */

		error = report_plot_region(report, data, origin, clip);
		if (error != NULL)
			break;
	}

	return error;
}


/**
 * Plot the contents of a report region.
 *
 * \param *report		The report instance holding the region.
 * \param *page			Pointer to the region data.
 * \param *origin		Pointer to an OS Coord holding the region origin
 *				for the current target in OS Units.
 * \param *clip			The output clipping box on OS Units.
 * \return			Pointer to an error block on failure, or NULL.
 */

static os_error *report_plot_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip)
{
	os_error	*error = NULL;
	os_box		outline;

	if (report == NULL || region == NULL)
		return NULL;

	/* Check if the region falls into the redraw clip window. */

	if (origin->x + region->position.x0 > clip->x1 ||
			origin->x + region->position.x1 < clip->x0 ||
			origin->y + region->position.y0 > clip->y1 ||
			origin->y + region->position.y1 < clip->y0)
		return NULL;

	/* Draw a box around the region. This is for debugging only! */

	outline.x0 = origin->x + region->position.x0;
	outline.y0 = origin->y + region->position.y0;
	outline.x1 = origin->x + region->position.x1;
	outline.y1 = origin->y + region->position.y1;

	error = xcolourtrans_set_gcol(os_COLOUR_LIGHT_GREEN, colourtrans_SET_FG_GCOL, os_ACTION_OVERWRITE, NULL, NULL);
	if (error != NULL)
		return error;

	error = report_draw_box(&outline);
	if (error != NULL)
		return error;

	/* Replot the region. */

	switch (region->type) {
	case REPORT_REGION_TYPE_TEXT:
		error = report_plot_text_region(report, region, origin, clip);
		break;
	case REPORT_REGION_TYPE_PAGE_NUMBER:
		error = report_plot_page_number_region(report, region, origin, clip);
		break;
	case REPORT_REGION_TYPE_LINES:
		error = report_plot_lines_region(report, region, origin, clip);
		break;
	case REPORT_REGION_TYPE_NONE:
		break;
	}

	return error;
}


/**
 * Plot a text region, containing a single line of static text.
 *
 * \param *report	The report to use.
 * \param *region	The region to plot.
 * \param *origin	The absolute origin to plot from, in OS Units.
 * \param *clip		The redraw clip region, in absolute OS Units.
 * \return		Pointer to an error block, or NULL for success.
 */

static os_error *report_plot_text_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip)
{
	os_box		position;
	char		*content_base;

	if (region == NULL || region->type != REPORT_REGION_TYPE_TEXT || region->data.text.content == REPORT_TEXTDUMP_NULL)
		return NULL;

	content_base = report_textdump_get_base(report->content);
	if (content_base == NULL)
		return NULL;

	position.x0 = origin->x + region->position.x0;
	position.x1 = origin->x + region->position.x1;
	position.y0 = origin->y + region->position.y0;
	position.y1 = origin->y + region->position.y1;

	return report_plot_cell(report, &position, content_base + region->data.text.content, REPORT_CELL_FLAGS_CENTRE);
}


/**
 * Plot a page number region, containing the single or double page numbering.
 *
 * \param *report	The report to use.
 * \param *region	The region to plot.
 * \param *origin	The absolute origin to plot from, in OS Units.
 * \param *clip		The redraw clip region, in absolute OS Units.
 * \return		Pointer to an error block, or NULL for success.
 */

static os_error *report_plot_page_number_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip)
{
	os_box		position;
	int		x_count, y_count;
	char		content[REPORT_MAX_PAGE_STRING_LEN];
	char		n1[REPORT_MAX_PAGE_NUMBER_LEN], n2[REPORT_MAX_PAGE_NUMBER_LEN];
	char		n3[REPORT_MAX_PAGE_NUMBER_LEN], n4[REPORT_MAX_PAGE_NUMBER_LEN];

	if (region == NULL || region->type != REPORT_REGION_TYPE_PAGE_NUMBER)
		return NULL;

	/* Identify the number of pages in the report layout. */

	if (!report_page_get_layout_pages(report->pages, &x_count, &y_count))
		return NULL;

	/* Generate the page number messages, including X pages if the report
	 * is more than 1 page wide.
	 */

	string_printf(n1, REPORT_MAX_PAGE_NUMBER_LEN, "%d", region->data.page_number.major);
	string_printf(n2, REPORT_MAX_PAGE_NUMBER_LEN, "%d", y_count);

	if (x_count > 1) {
		string_printf(n3, REPORT_MAX_PAGE_NUMBER_LEN, "%d", region->data.page_number.minor);
		string_printf(n4, REPORT_MAX_PAGE_NUMBER_LEN, "%d", x_count);

		msgs_param_lookup("Page2", content, REPORT_MAX_PAGE_STRING_LEN, n1, n3, n2, n4);
	} else {
		msgs_param_lookup("Page1", content, REPORT_MAX_PAGE_STRING_LEN, n1, n2, NULL, NULL);
	}

	/* Plot the page number information. */

	position.x0 = origin->x + region->position.x0;
	position.x1 = origin->x + region->position.x1;
	position.y0 = origin->y + region->position.y0;
	position.y1 = origin->y + region->position.y1;

	return report_plot_cell(report, &position, content, REPORT_CELL_FLAGS_CENTRE);
}


/**
 * Plot a lines region, containing one or more lines from a report.
 *
 * \param *report	The report to use.
 * \param *region	The region to plot.
 * \param *origin	The absolute origin to plot from, in OS Units.
 * \param *clip		The redraw clip region, in absolute OS Units.
 * \return		Pointer to an error block, or NULL for success.
 */

static os_error *report_plot_lines_region(struct report *report, struct report_region_data *region, os_coord *origin, os_box *clip)
{
	unsigned		top, base, y;
	size_t			line_count;
	os_coord		position;
	os_error		*error;
	struct report_line_data	*line_data;

	if (region == NULL || region->type != REPORT_REGION_TYPE_LINES)
		return NULL;

	/* Identify the position and extent of the text block. */

	line_data = report_line_get_info(report->lines, region->data.lines.first);
	if (line_data == NULL)
		return NULL;

	position.x = origin->x + region->position.x0;
	position.y = origin->y + region->position.y1 - (line_data->ypos + report_get_line_height(report, line_data));

	top = report_line_find_from_ypos(report->lines, clip->y1 - position.y);
	if (top < region->data.lines.first)
		top = region->data.lines.first;
	else if (top > region->data.lines.last)
		top = region->data.lines.last;

	base = report_line_find_from_ypos(report->lines, clip->y0 - position.y);
	if (base < region->data.lines.first)
		base = region->data.lines.first;
	else if (base > region->data.lines.last)
		base = region->data.lines.last;

	/* Plot Text. */

	line_count = report_line_get_count(report->lines);

	for (y = top; y < line_count && y <= base; y++) {
		error = report_plot_line(report, y, &position, clip);
		if (error != NULL)
			return error;
	}

	return NULL;
}


/**
 * Plot a line of a report to screen or the printer drivers.
 *
 * \param *report	The report to use.
 * \param line		The line to be printed.
 * \param *origin	The absolute origin to plot from, in OS Units.
 * \param *clip		The redraw clip region, in absolute OS Units.
 * \return		Pointer to an error block, or NULL for success.
 */

static os_error *report_plot_line(struct report *report, unsigned int line, os_coord *origin, os_box *clip)
{
	os_error		*error;
	int			cell, line_width, line_inset;
	char			*content_base;
	os_box			line_outline, cell_outline;
	struct report_line_data	*line_data;
	struct report_cell_data	*cell_data, *spill_cell_data;
	struct report_tabs_stop	*tab_stop;

	content_base = report_textdump_get_base(report->content);
	if (content_base == NULL)
		return NULL;

	line_data = report_line_get_info(report->lines, line);
	if (line_data == NULL)
		return NULL;

	if (!report_tabs_get_bar_width(report->tabs, line_data->tab_bar, &line_width, &line_inset))
		return NULL;

	line_outline.x0 = origin->x;
	line_outline.x1 = origin->x + line_width;

	line_outline.y0 = origin->y + line_data->ypos;
	cell_outline.y0 = line_outline.y0;

	if ((report->display & REPORT_DISPLAY_SHOW_GRID) && (line_data->flags & REPORT_LINE_FLAGS_RULE_BELOW))
		cell_outline.y0 += 2 * REPORT_GRID_LINE_MARGIN;

	cell_outline.y1 = cell_outline.y0 + report->linespace;
	line_outline.y1 = cell_outline.y1;

	if ((report->display & REPORT_DISPLAY_SHOW_GRID) && (line_data->flags & REPORT_LINE_FLAGS_RULE_ABOVE))
		line_outline.y1 += 2 * REPORT_GRID_LINE_MARGIN;

	if (line_outline.y0 > clip->y1 || line_outline.y1 < clip->y0)
		return NULL;

	/* Plot the grid above and below the line. */

	if (report->display & REPORT_DISPLAY_SHOW_GRID) {
		error = xcolourtrans_set_gcol(os_COLOUR_BLACK, colourtrans_SET_FG_GCOL, os_ACTION_OVERWRITE, NULL, NULL);
		if (error != NULL)
			return error;

		if (line_data->flags & REPORT_LINE_FLAGS_RULE_ABOVE)
			error = report_draw_line(line_outline.x0, cell_outline.y1 + REPORT_GRID_LINE_MARGIN,
					line_outline.x1, cell_outline.y1 + REPORT_GRID_LINE_MARGIN);

		if (line_data->flags & REPORT_LINE_FLAGS_RULE_BELOW) {
			error = report_draw_line(line_outline.x0, cell_outline.y0 - REPORT_GRID_LINE_MARGIN,
					line_outline.x1, cell_outline.y0 - REPORT_GRID_LINE_MARGIN);

			error = report_draw_line(line_outline.x0, cell_outline.y0 - REPORT_GRID_LINE_MARGIN,
					line_outline.x0, cell_outline.y1 + REPORT_GRID_LINE_MARGIN);

			error = report_draw_line(line_outline.x1, cell_outline.y0 - REPORT_GRID_LINE_MARGIN,
					line_outline.x1, cell_outline.y1 + REPORT_GRID_LINE_MARGIN);
		}
	}

	/* Plot the cells in the line. */

	for (cell = 0; cell < line_data->cell_count; cell++) {
		cell_data = report_cell_get_info(report->cells, line_data->first_cell + cell);
		if (cell_data == NULL || cell_data->offset == REPORT_TEXTDUMP_NULL)
			continue;

		tab_stop = report_tabs_get_stop(report->tabs, line_data->tab_bar, cell_data->tab_stop);
		if (tab_stop == NULL)
			continue;

		cell_outline.x0 = origin->x + line_inset + tab_stop->font_left;
		cell_outline.x1 = cell_outline.x0 + tab_stop->font_width;

		/* Merge in any spill cells which follow. */

		while (cell + 1 < line_data->cell_count) {
			spill_cell_data = report_cell_get_info(report->cells, line_data->first_cell + cell + 1);
			if (spill_cell_data == NULL || !(spill_cell_data->flags & REPORT_CELL_FLAGS_SPILL))
				break;

			tab_stop = report_tabs_get_stop(report->tabs, line_data->tab_bar, spill_cell_data->tab_stop);
			if (tab_stop == NULL)
				break;

			cell_outline.x1 = origin->x + line_inset + tab_stop->font_left + tab_stop->font_width;

			cell++;
		}

		if ((cell_outline.x0 > clip->x1) || ((cell_outline.x1 + line_inset) < clip->x0))
			continue;

		if ((report->display & REPORT_DISPLAY_SHOW_GRID) && (tab_stop->flags & REPORT_TABS_STOP_FLAGS_RULE_AFTER)) {
			error = xcolourtrans_set_gcol(os_COLOUR_BLACK, colourtrans_SET_FG_GCOL, os_ACTION_OVERWRITE, NULL, NULL);
			if (error != NULL)
				return error;

			error = report_draw_line(cell_outline.x1 + line_inset, cell_outline.y0 - REPORT_GRID_LINE_MARGIN,
					cell_outline.x1 + line_inset, cell_outline.y1 + REPORT_GRID_LINE_MARGIN);
		}

		error = report_plot_cell(report, &cell_outline, content_base + cell_data->offset, cell_data->flags);
		if (error != NULL)
			return error;
	}

	return NULL;
}


/**
 * Plot a cell on screen.
 *
 * \param *report	The report to use.
 * \param *outline	The outline of the cell, in absolute OS Units.
 * \param *content	Pointer to the content of the cell.
 * \param flags		The cell flags to apply to the plotting.
 * \return		Pointer to an OS Error box, or NULL on success.
 */

static os_error *report_plot_cell(struct report *report, os_box *outline, char *content, enum report_cell_flags flags)
{
	os_error	*error;
	int		width, indent;

	if (report == NULL || outline == NULL)
		return NULL;

	/* Draw a box around the cell. This is for debugging only! */

//	error = xcolourtrans_set_gcol(os_COLOUR_RED, colourtrans_SET_FG_GCOL, os_ACTION_OVERWRITE, NULL, NULL);
//	if (error != NULL)
//		return error;

//	error = report_draw_box(outline);
//	if (error != NULL)
//		return error;

	/* If there's no text, there's nothing else to do. */

	if (content == NULL)
		return NULL;

	/* Calculate the required cell indent. */

	if (flags & REPORT_CELL_FLAGS_INDENT) {
		indent = REPORT_COLUMN_INDENT;
	} else if (flags & REPORT_CELL_FLAGS_RIGHT || flags & REPORT_CELL_FLAGS_CENTRE) {
		error = report_fonts_get_string_width(report->fonts, content, flags, &width);
		if (error != NULL)
			return error;

		if (flags & REPORT_CELL_FLAGS_CENTRE)
			indent = ((outline->x1 - outline->x0) - width) / 2; 
		else
			indent = (outline->x1 - outline->x0) - width;
	} else {
		indent = 0;
	}

	/* Set the font colour and plot the cell contents. */

	error = report_fonts_set_colour(report->fonts, os_COLOUR_BLACK, os_COLOUR_WHITE);
	if (error != NULL)
		return error;

	return report_fonts_paint_text(report->fonts, content,
			outline->x0 + indent,
			outline->y0 + REPORT_BASELINE_OFFSET, flags);
}


/**
 * Call a callback function to process the template stored in every currently
 * open report.
 *
 * \param *file			The file to process.
 * \param *callback		The callback function to call.
 * \param *data			Data to pass to the callback function.
 */

void report_process_all_templates(struct file_block *file, void (*callback)(struct analysis_report *template, void *data), void *data)
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



/**
 * Process a Message_SetPrinter, which requires us to repaginate any open
 * reports.
 *
 * \param *message		The Wimp message block.
 * \return			TRUE to claim the message.
 */

static osbool report_handle_message_set_printer(wimp_message *message)
{
	#ifdef DEBUG
	debug_printf("Message_SetPrinter received.");
	#endif

	hourglass_on();

	file_process_all(report_repaginate_all);

	hourglass_off();

	return TRUE;
}


/**
 * Repaginate all of the reports in a file.
 *
 * \param *file			The file to be processed.
 */

static void report_repaginate_all(struct file_block *file)
{
	struct report	*report;

	if (file == NULL)
		return;

	report = file->reports;

	while (report != NULL) {
		report_paginate(report);
		report_set_window_extent(report);
		report_view_toolbar_prepare(report);

		if (report->window != NULL)
			windows_redraw(report->window);

		report = report->next;
	}
}


/**
 * Repaginate a report.
 *
 * \param *report		The handle of the report to be repaginated.
 */

static void report_paginate(struct report *report)
{
	struct report_line_data		*line_data;
	struct report_page_layout	layout;
	struct report_pagination_area	repeat_area, main_area;
	int				pages, target_width, field_height, body_height, used_height, repeat_tab_bar;
	size_t				line_count;
	unsigned			line, repeat_line;

	if (report == NULL)
		return;

	/* Reset any existing pagination. */

	report_page_clear(report->pages);
	report_region_clear(report->regions);

	/* Identify the page size. If this fails, there's no point continuing. */

	target_width = (report->display & REPORT_DISPLAY_FIT_WIDTH) ? report->width : 0;

	field_height = report_fonts_get_linespace(report->fonts);
	if (field_height == 0)
		return;

	if (report_page_calculate_areas(report->pages, (report->display & REPORT_DISPLAY_LANDSCAPE) ? TRUE : FALSE, target_width,
			((report->page_title != REPORT_TEXTDUMP_NULL) && (report->display & REPORT_DISPLAY_SHOW_TITLE)) ? field_height : 0,
			(report->display & REPORT_DISPLAY_SHOW_NUMBERS) ? field_height : 0) != NULL)
		return;

	report_page_get_areas(report->pages, &layout);
	if (layout.areas == REPORT_PAGE_AREA_NONE)
		return;

	/* The Body Height is the vertical space, in OS Units, which is available
	 * to take output lines on the page.
	 */

	body_height = layout.body.y1 - layout.body.y0;

	/* Initialise areas to hold the main "new" lines on the page, and to hold
	 * any lines repeated at the top of new pages.
	 */

	main_area.active = TRUE;
	main_area.height = 0;
	main_area.ypos_offset = 0;
	main_area.top_line = 0;
	main_area.bottom_line = 0;

	repeat_area.active = FALSE;
	repeat_area.height = 0;
	repeat_area.ypos_offset = 0;
	repeat_area.top_line = 0;
	repeat_area.bottom_line = 0;

	repeat_line = REPORT_LINE_NONE;
	repeat_tab_bar = -1;
	pages = 1;
	used_height = 0;

	/* Process the lines in the report one at a time. */

	line_count = report_line_get_count(report->lines);

	for (line = 0; line < line_count; line++) {
		line_data = report_line_get_info(report->lines, line);
		if (line_data == NULL)
			continue;

		/* Process keep-together lines: if the Keep Together flag is set and there's
		 * currently no heading line set or the tab bar has changed, record a new
		 * heading line. Otherwise, if the Keep Together flag isn't set, make sure
		 * that there's no heading line.
		 */

		if ((line_data->flags & REPORT_LINE_FLAGS_KEEP_TOGETHER) &&
				((line_data->tab_bar != repeat_tab_bar) || (repeat_line == REPORT_LINE_NONE))) {
			repeat_line = line;
		} else if (!(line_data->flags & REPORT_LINE_FLAGS_KEEP_TOGETHER)) {
			repeat_line = REPORT_LINE_NONE;
		}

		repeat_tab_bar = line_data->tab_bar;

		/* If the line falls out of the visible area, package up the current page
		 * data and add it to the page output before continuing.
		 */

		if ((main_area.ypos_offset - line_data->ypos) > (body_height - used_height)) {
			report_add_page_row(report, &layout, &main_area, &repeat_area, pages);

			pages++;

			/* Get the previous line's data, and use it to calculate the
			 * starting position -- line and vertical offset in OS Units of
			 * all of the Y-Pos values -- for the new page.
			 */

			if (line > 0) {
				line_data = report_line_get_info(report->lines, line - 1);
				if (line_data == NULL)
					continue;

				main_area.ypos_offset = line_data->ypos;
			} else {
				main_area.ypos_offset = 0;
			}

			main_area.top_line = line;

			/* If there's a line currently being repeated at the top of the
			 * page, set up the repeat area and update the Used Height to
			 * reflect the space lost to "new" lines.
			 */

			if ((repeat_line != REPORT_LINE_NONE) && ((line_data = report_line_get_info(report->lines, repeat_line)) != NULL)) {
				repeat_area.active = TRUE;
				repeat_area.height = report_get_line_height(report, line_data);
				repeat_area.ypos_offset = line_data->ypos - repeat_area.height;
				repeat_area.top_line = repeat_line;
				repeat_area.bottom_line = repeat_line;
				used_height = repeat_area.height;
			} else {
				repeat_area.active = FALSE;
				used_height = 0;
			}

			/* Re-find the original page data for the remainder of the processing. */

			line_data = report_line_get_info(report->lines, line);
			if (line_data == NULL)
				continue;
		}

		/* Record the current line details. */

		main_area.bottom_line = line;
		main_area.height = main_area.ypos_offset - line_data->ypos;

		/* If this is the first line of the page, and there are no cells on it,
		 * skip it so that we don't have blank lines at the top of a page.
		 */

		if (main_area.top_line == line && line_data->cell_count == 0) {
			main_area.ypos_offset = line_data->ypos;
			main_area.top_line = line + 1;
		}
	}

	/* If there are any lines left to put out on a page, do so. */

	if (line > main_area.top_line)
		report_add_page_row(report, &layout, &main_area, &repeat_area, pages);

	/* Close the pages and regions off, to finish the pagination. */

	report_page_close(report->pages);
	report_region_close(report->regions);
}


/**
 * Add a row of pages to a report.
 *
 * \param *report	The report to be processed.
 * \param *layout	The page layout details for the report.
 * \param *main_area	Pointer to data for the "main" line area.
 * \param *repeat_area	Pointer to data for the "repeat" line area.
 * \param row		The row number, for page numbering purposes.
 */

static void report_add_page_row(struct report *report, struct report_page_layout *layout,
		struct report_pagination_area *main_area, struct report_pagination_area *repeat_area, int row)
{
	int column, regions;
	unsigned first_region, region;
	os_box position;

	report_page_new_row(report->pages);

	for (column = 0; column < 1; column++) {
		first_region = REPORT_REGION_NONE;
		regions = 0;

		/* Add the main page body: any repeated lines at the top, followed
		 * by the "main" report lines for the page below.
		 */

		if (layout->areas & REPORT_PAGE_AREA_BODY) {
			position.x0 = layout->body.x0;
			position.x1 = layout->body.x1;
			position.y1 = layout->body.y1;

			if (repeat_area->active == TRUE) {
				position.y0 = layout->body.y1 - repeat_area->height;

				region = report_region_add_lines(report->regions, &position, repeat_area->top_line, repeat_area->bottom_line);
				if (region == REPORT_REGION_NONE)
					continue;

				regions++;

				if (first_region == REPORT_REGION_NONE)
					first_region = region;

				position.y1 = position.y0;
			}

			position.y0 = position.y1 - main_area->height;

			region = report_region_add_lines(report->regions, &position, main_area->top_line, main_area->bottom_line);
			if (region == REPORT_REGION_NONE)
				continue;

			regions++;

			if (first_region == REPORT_REGION_NONE)
				first_region = region;
		}

		/* Add the page header, if there is one. */

		if (layout->areas & REPORT_PAGE_AREA_HEADER) {
			region = report_region_add_text(report->regions, &(layout->header), report->page_title);
			if (region == REPORT_REGION_NONE)
				continue;

			regions++;

			if (first_region == REPORT_REGION_NONE)
				first_region = region;
		}

		/* Add the page footer, if there is one. */

		if (layout->areas & REPORT_PAGE_AREA_FOOTER) {
			region = report_region_add_page_number(report->regions, &(layout->footer), row, column + 1);
			if (region == REPORT_REGION_NONE)
				continue;

			regions++;

			if (first_region == REPORT_REGION_NONE)
				first_region = region;
		}

		/* Add the page to the report. */

		report_page_add(report->pages, first_region, regions);
	}
}


/**
 * Return the height of a line in a report, from the ypos baseline to
 * the top of the outside bounding box.
 *
 * \param *report	The report containing the line.
 * \param line_data	The data for the line of interest.
 */

static int report_get_line_height(struct report *report, struct report_line_data *line_data)
{
	if (report == NULL || line_data == NULL)
		return 0;

	return report->linespace;
} 

/**
 * Change the state of a display option in a report, and refresh the
 * report display as a result.
 *
 * \param *report		The report to update.
 * \param option		The option to be toggled.
 * \param state			The new state for the option.
 */

static void report_set_display_option(struct report *report, enum report_display option, osbool state)
{
	if (report == NULL)
		return;

	if (state == TRUE)
		report->display = report->display | option;
	else
		report->display = report->display & (~option);

	if (option & REPORT_DISPLAY_SHOW_GRID)
		report_reflow_content(report);

	if (option & (REPORT_DISPLAY_LANDSCAPE | REPORT_DISPLAY_SHOW_TITLE |
			REPORT_DISPLAY_SHOW_GRID | REPORT_DISPLAY_SHOW_NUMBERS))
		report_paginate(report);

	report_set_window_extent(report);
	report_view_toolbar_prepare(report);

	if (report->window != NULL)
		windows_redraw(report->window);
}


/**
 * Update the extent of a report view window, to take account of its
 * current content and settings.
 *
 * \param *report		The report to update.
 */

static void report_set_window_extent(struct report *report)
{
	int			new_xextent, new_yextent, visible_xextent, visible_yextent, new_xscroll, new_yscroll;
	os_box			extent;
	wimp_window_state	state;
	wimp_window_info	window;

	if (report == NULL || report->window == NULL)
		return;

	if (!report_get_window_extent(report, &new_xextent, &new_yextent))
		return;

	/* Get the current window details, and find the extent of the bottom and right of the visible area. */

	state.w = report->window;
	wimp_get_window_state(&state);

	visible_xextent = state.xscroll + (state.visible.x1 - state.visible.x0);
	visible_yextent = (state.visible.y1 - state.visible.y0) - state.yscroll;

	/* If the visible area falls outside the new window extent, then the window needs to be re-opened first. */

	if (new_xextent < visible_xextent || new_yextent < visible_yextent) {
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

		/* Now do the y scroll.  If this is less than zero, the current window is too deep and will need
		 * shrinking down.  Otherwise, just set the new scroll offset.
		 */

		new_yscroll = new_yextent - (state.visible.y1 - state.visible.y0);

		if (new_yscroll < 0) {
			state.visible.y0 -= new_yscroll;
			state.yscroll = 0;
		} else {
			state.yscroll = -new_yscroll;
		}

		wimp_open_window((wimp_open *) &state);
	}

	/* Finally, call Wimp_SetExtent to update the extent, safe in the knowledge that the visible area will still
	 * exist.
	 */

	extent.x0 = 0;
	extent.x1 = new_xextent;
	extent.y1 = 0;
	extent.y0 = -new_yextent;
	wimp_set_extent(report->window, &extent);

	window.w = report->toolbar;
	wimp_get_window_info_header_only(&window);
	window.extent.x1 = window.extent.x0 + new_xextent;
	wimp_set_extent(window.w, &(window.extent));
}


/**
 * Calculate the required window dimensions for a report in OS Units,
 * taking into account the content and selected view mode.
 *
 * \param *report		The report to calculate for.
 * \param *x			Pointer to variable to take X dimension.
 * \param *y			Pointer to variable to take Y dimension.
 * \return			TRUE if values have been returned.
 */

static osbool report_get_window_extent(struct report *report, int *x, int *y)
{
	if (report == NULL)
		return FALSE;

	if (!(report->display & REPORT_DISPLAY_PAGINATED) || !report_page_get_layout_extent(report->pages, x, y)) {
		debug_printf("Failed to get paginated extent.");
		if (x != NULL) {
			int width = report->width + REPORT_LEFT_MARGIN + REPORT_RIGHT_MARGIN;
			*x = (width > REPORT_MIN_WIDTH) ? width : REPORT_MIN_WIDTH;
		}

		if (y != NULL) {
			int height = report->height + REPORT_TOP_MARGIN + REPORT_BOTTOM_MARGIN;
			*y = (height > REPORT_MIN_HEIGHT) ? height : REPORT_MIN_HEIGHT;
		}
	}

	if (y != NULL)
		*y = *y + REPORT_TOOLBAR_HEIGHT;

	return TRUE;
}

