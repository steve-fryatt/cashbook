/* CashBook - report.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

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

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/windows.h"
#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/icons.h"
#include "sflib/string.h"
#include "sflib/msgs.h"

/* Application header files */

#include "global.h"
#include "report.h"

#include "analysis.h"
#include "caret.h"
#include "dataxfer.h"
#include "filing.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "printing.h"
#include "window.h"

/* ==================================================================================================================
 * Global variables.
 */

file_data   *report_format_file = NULL;
report_data *report_format_report = NULL;;

file_data   *report_print_file = NULL;
report_data *report_print_report = NULL;

static int  print_opt_text;
static int  print_opt_textformat;
static int  print_opt_fitwidth;
static int  print_opt_rotate;

/* ==================================================================================================================
 * Report creation and deletion.
 */

/* Create a new report block, ready to write data to. */

report_data *open_new_report (file_data *file, char *title, saved_report *template)
{
  report_data       *new;


  #ifdef DEBUG
  debug_printf ("\\GOpening report");
  #endif

  new = heap_alloc (sizeof (report_data));

  if (new != NULL)
  {
    new->next = file->reports;
    file->reports = new;

    new->flags = 0;
    new->print_pending = 0;

    new->lines = 0;
    new->data_size = 0;

    new->data = NULL;
    new->line_ptr = NULL;

    new->block_size = 0;
    new->max_lines = 0;

    new->window = NULL;
    strcpy (new->window_title, title);

    if (flex_alloc ((flex_ptr) &(new->data), REPORT_BLOCK_SIZE))
    {
      new->block_size = REPORT_BLOCK_SIZE;
    }
    else
    {
      new->flags |= REPORT_STATUS_MEMERR;
    }

    if (flex_alloc ((flex_ptr) &(new->line_ptr), REPORT_LINE_SIZE * sizeof (int)))
    {
      new->max_lines = REPORT_LINE_SIZE;
    }
    else
    {
      new->flags |= REPORT_STATUS_MEMERR;
    }

    if (template != NULL)
    {
      analysis_copy_saved_report_template (&(new->template), template);
    }
    else
    {
      new->template.type = REPORT_TYPE_NONE;
    }
  }

  return (new);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Close off a report that has had data written to it, and open a window on it. */

void close_report (file_data *file, report_data *report)
{
  int                   linespace;
  wimp_window_state     parent;

  extern global_windows windows;


  #ifdef DEBUG
  debug_printf ("\\GClosing report");
  #endif

  if (file != NULL && report != NULL && (report->flags & REPORT_STATUS_MEMERR) == 0)
  {
    /* Update the data block. */

    flex_extend ((flex_ptr) &(report->data), report->data_size);
    flex_extend ((flex_ptr) &(report->line_ptr), report->lines * sizeof (int));

    report->flags |= REPORT_STATUS_CLOSED;

    /* Set up the display details. */

    strcpy (report->font_normal, config_str_read ("ReportFontNormal"));
    strcpy (report->font_bold, config_str_read ("ReportFontBold"));
    report->font_size = config_int_read ("ReportFontSize") * 16;
    report->line_spacing = config_int_read ("ReportFontLinespace");
    report->width = format_report_columns (report);

    /* Set up the window title */

    windows.report_window_def->title_data.indirected_text.text = report->window_title;

    /* Size the window. */

    font_convertto_os (1000 * (report->font_size / 16) * report->line_spacing / 100, 0, &linespace, NULL);

    #ifdef DEBUG
    debug_printf ("Report window width: %d", report->width);
    #endif

    /* Position the window and open it. */

    parent.w = file->transaction_window.transaction_pane;
    wimp_get_window_state (&parent);

    report->height = report->lines * linespace + REPORT_BOTTOM_MARGIN;

    set_initial_window_area (windows.report_window_def,
                             (report->width > REPORT_MIN_WIDTH) ? report->width : REPORT_MIN_WIDTH,
                             (report->height > REPORT_MIN_HEIGHT) ? report->height : REPORT_MIN_HEIGHT,
                             parent.visible.x0 + CHILD_WINDOW_OFFSET + file->child_x_offset * CHILD_WINDOW_X_OFFSET,
                             parent.visible.y0 - CHILD_WINDOW_OFFSET, 0);

    file->child_x_offset++;
    if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
    {
      file->child_x_offset = 0;
    }

    report->window = wimp_create_window (windows.report_window_def);
    windows_open (report->window);

    add_ihelp_window (report->window, "Report", NULL);
  }
  else
  {
    error_msgs_report_error ("NoMemReport");
    delete_report (file, report);
  }
}
/* ------------------------------------------------------------------------------------------------------------------ */

/* Close off a report that has had data written to it, and print it before deleting it. */

void close_and_print_report (file_data *file, report_data *report, int text, int textformat, int fitwidth, int rotate)
{
  int linespace;

  #ifdef DEBUG
  debug_printf ("\\GClosing report and starting printing");
  #endif

  if (file != NULL && report != NULL && (report->flags & REPORT_STATUS_MEMERR) == 0)
  {
    /* Update the data block. */

    flex_extend ((flex_ptr) &(report->data), report->data_size);
    flex_extend ((flex_ptr) &(report->line_ptr), report->lines * sizeof (int));

    report->flags |= REPORT_STATUS_CLOSED;

    /* Set up the display details. */

    strcpy (report->font_normal, config_str_read ("ReportFontNormal"));
    strcpy (report->font_bold, config_str_read ("ReportFontBold"));
    report->font_size = config_int_read ("ReportFontSize") * 16;
    report->line_spacing = config_int_read ("ReportFontLinespace");
    report->width = format_report_columns (report);
    font_convertto_os (1000 * (report->font_size / 16) * report->line_spacing / 100, 0, &linespace, NULL);
    report->height = report->lines * linespace + REPORT_BOTTOM_MARGIN;

    /* There isn't a window. */

    report->window = NULL;

    /* Set up the details needed by the print system and go.
     * This hijacks the same process used by the Report Print dialogue in process_report_print_window (), setting
     * up[ the same variables and launching the same Wimp messages.
     */

    report_print_file = file;
    report_print_report = report;

    print_opt_text = text;
    print_opt_textformat = textformat;
    print_opt_fitwidth = fitwidth;
    print_opt_rotate = rotate;

    report_print_report->print_pending++;

    send_start_print_save (start_report_print, cancel_report_print, print_opt_text);
  }
  else
  {
    error_msgs_report_error ("NoMemReport");
    delete_report (file, report);
  }
}

/* ================================================================================================================== */

/* Delete a report window (and, if there are no print jobs pending, it's data block). */

void delete_report_window (file_data *file, report_data *report)
{
  #ifdef DEBUG
  debug_printf ("\\RDeleting report window");
  #endif

  if (file != NULL && report != NULL)
  {
    /* Close the window */

    if (report->window != NULL);
    {
      analysis_force_close_report_save_window (file, report);

      remove_ihelp_window (report->window);

      wimp_delete_window (report->window);
      report->window = NULL;
    }

    if (report->print_pending == 0)
    {
      delete_report (file, report);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Delete a report block (and any associated window). */

void delete_report (file_data *file, report_data *report)
{
  report_data **rep;


  #ifdef DEBUG
  debug_printf ("\\RDeleting report");
  #endif

  if (file != NULL && report != NULL)
  {
    if (report->window != NULL)
    {
      wimp_delete_window (report->window);
      report->window = NULL;
    }

    /* Free the flex bloxks. */

    if (report->data != NULL)
    {
      flex_free ((flex_ptr) &(report->data));
    }

    if (report->line_ptr != NULL)
    {
      flex_free ((flex_ptr) &(report->line_ptr));
    }

    /* Delink the block and delete it. */

    rep = &(file->reports);

    while (*rep != NULL && *rep != report)
    {
      rep = &((*rep)->next);
    }

    if (*rep != NULL)
    {
      *rep = report->next;
    }

    heap_free (report);
  }
}

/* ================================================================================================================== */

/* Write a line to the given report. */

/* text can contain the following commands:
 *
 * \t - Tab (start a new column)
 * \i - Indent the text in the current 'cell'
 * \b - Format this 'cell' bold
 * \u - Format this 'cell' underlined
 * \d - This cell contains a number
 * \r - Right-align the text in this 'cell'
 * \s - Spill text from the previous cell into this one.
 * \h - This line is a heading.
 */

int write_report_line (report_data *report, int bar, char *text)
{
  int  len;
  char *c, *copy, *flag;


  #ifdef DEBUG
  debug_printf ("Print line: %s", text);
  #endif

  /* Parse the string */

  copy = malloc (strlen(text) + (REPORT_TAB_STOPS * REPORT_FLAG_BYTES) + 1);
  c = copy;

  bar = (bar >= 0 && bar < REPORT_TAB_BARS) ? bar : 0;

  flag = c++;
  *flag = REPORT_FLAG_NOTNULL;

  while (*text != '\0')
  {
    if (*text == '\\')
    {
      text++;

      switch (*text++)
      {
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

        case 'h':
          *flag |= REPORT_FLAG_HEADING;
          break;
      }
    }
    else
    {
      *c++ = *text++;
    }
  }

  *c = '\0';

  /* Now that we have a string containing /n for tabs and all the flag bytes in the correct places, allocate memory
   * from the flex block and proceed to dump it in there.
   */


  len = strlen(copy) + REPORT_BAR_BYTES + 1; /* Add in the terminator and the tab bar marker. */

  if (len > (report->block_size - report->data_size))
  {
    #ifdef DEBUG
    debug_printf ("Extending data block...");
    #endif

    if (flex_extend ((flex_ptr) &(report->data), report->block_size + REPORT_BLOCK_SIZE))
    {
      report->block_size += REPORT_BLOCK_SIZE;
    }
    else
    {
      report->flags |= REPORT_STATUS_MEMERR;
    }
  }

  if (report->lines >= report->max_lines)
  {
    #ifdef DEBUG
    debug_printf ("Extending line pointer block to %d...", ((report->max_lines + REPORT_LINE_SIZE) * sizeof (int)));
    #endif

    if (flex_extend ((flex_ptr) &(report->line_ptr), ((report->max_lines + REPORT_LINE_SIZE) * sizeof (int))))
    {
      report->max_lines += REPORT_LINE_SIZE;
    }
    else
    {
      report->flags |= REPORT_STATUS_MEMERR;
    }
  }

  if ((report->flags & REPORT_STATUS_MEMERR) == 0 && (report->flags & REPORT_STATUS_CLOSED) == 0 &&
      len <= (report->block_size - report->data_size) && report->lines < report->max_lines)
  {
    *(report->data + report->data_size) = (char) bar;
    strcpy (report->data + report->data_size + REPORT_BAR_BYTES, copy);
    (report->line_ptr)[report->lines] = report->data_size;

    report->lines++;
    report->data_size += len;
  }

  free (copy);

  return (0);
}

/* ================================================================================================================== */

int find_report_fonts (report_data *report, font_f *normal, font_f *bold)
{
  os_error *error, *e1 = NULL, *e2 = NULL;

  if (normal != NULL)
  {
    error = xfont_find_font (report->font_normal, report->font_size, report->font_size, 0, 0, normal, NULL, NULL);
    if (error != NULL)
    {
      e1 = xfont_find_font ("Homerton.Medium", report->font_size, report->font_size, 0, 0, normal, NULL, NULL);
    }
  }

  if (bold != NULL)
  {
    error = xfont_find_font (report->font_bold, report->font_size, report->font_size, 0, 0, bold, NULL, NULL);
    if (error != NULL)
    {
      e2 = xfont_find_font ("Homerton.Bold", report->font_size, report->font_size, 0, 0, bold, NULL, NULL);
    }
  }

  return (e1 != NULL || e2 != NULL);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int format_report_columns (report_data *report)
{
  int      width[REPORT_TAB_STOPS], t_width[REPORT_TAB_STOPS], right[REPORT_TAB_BARS][REPORT_TAB_STOPS],
           width1[REPORT_TAB_BARS][REPORT_TAB_STOPS], width2[REPORT_TAB_BARS][REPORT_TAB_STOPS],
           t_width1[REPORT_TAB_BARS][REPORT_TAB_STOPS], t_width2[REPORT_TAB_BARS][REPORT_TAB_STOPS],
           i, j, line, bar, tab, total;
  char     *column, *flags;
  font_f   font, font_n, font_b;


  #ifdef DEBUG
  debug_printf ("\\GFormatting report");
  #endif

  /* Reset the flags used to keep track of items. */

  for (i=0; i<REPORT_TAB_BARS; i++)
  {
    for (j=0; j<REPORT_TAB_STOPS; j++)
    {
      right[i][j]  = 0; /* Flag to mark if anything in a column has been right-aligned. */

      /* These are in OS units, for on screen rendering. */

      width1[i][j] = 0; /* Tally of the maximum widths of the columns. */
      width2[i][j] = 0; /* Tally of the maximum widths of the columns, ignoring end column objects in each row. */

      /* These two are in characters, for ASCII formatting. */

      t_width1[i][j] = 0; /* Tally of the maximum widths of the columns. */
      t_width2[i][j] = 0; /* Tally of the maximum widths of the columns, ignoring end column objects in each row. */
    }
  }

  /* Find the font to be used by the report. */

  find_report_fonts (report, &font_n, &font_b);

  /* Work through the report, line by line, getting the maximum column widths. */

  for (line = 0; line < report->lines; line++)
  {
    tab = 0;
    column = report->data + report->line_ptr[line];
    bar = (int) *column;
    column += REPORT_BAR_BYTES;

    /* Process the next line. do {} while is used, as we don't know if the last tab has been reached until we get
     * there.
     */

    do
    {
      flags = column;
      column += REPORT_FLAG_BYTES;

      /* The flags that are looked at are bold, which affects the font, and indent, which affects the width.
       * Underline and right don't affect the column width, so these are ignored with the formatting.
       *
       * Right flags are noted to allow the column widths and tab stops to be sorted out later.
       */

       /* Outline font width. */

      font = (*flags & REPORT_FLAG_BOLD) ? font_b : font_n;

      font_scan_string (font, column, font_KERN | font_GIVEN_FONT, 0x7fffffff, 0x7fffffff, NULL, NULL, 0, NULL,
                        &total, NULL, NULL);
      font_convertto_os (total, 0, &(width[tab]), NULL);

      /* ASCII text column width. */

      t_width[tab] = string_ctrl_strlen (column);

      /* If the column is indented, add the indent to the column widths. */

      if (*flags & REPORT_FLAG_INDENT)
      {
        width[tab] += REPORT_COLUMN_INDENT;
        t_width[tab] += REPORT_TEXT_COLUMN_INDENT;
      }

      /* If the column is right aligned, record the fact. */

      if (*flags & REPORT_FLAG_RIGHT)
      {
        right[bar][tab] = 1;
      }

      /* If the column is a spill column, the width is carried over from the width of the preceeding column, minus the
       * inter-column gap.  The previous column is then zeroed.
       */

      if ((*flags & REPORT_FLAG_SPILL) && tab > 0)
      {
        width[tab] = width[tab-1] - REPORT_COLUMN_SPACE;
        width[tab-1] = 0;

        t_width[tab] = t_width[tab-1] - REPORT_TEXT_COLUMN_SPACE;
        t_width[tab-1] = 0;
      }

      /* Skip past the text to the next \n or \0, then increment the tab stop. */

      while (*column != '\0' && *column != '\n')
      {
        column++;
      }
      column++;
      tab++;
    }
    while (tab < REPORT_TAB_STOPS && *(column - 1) != '\0');

    /* Update the tally of maximum column widths. */

    for (i=0; i<tab; i++)
    {
      if (width[i] > width1[bar][i]) /* All column widths are noted here... */
      {
        width1[bar][i] = width[i];
      }

      if (width[i] > width2[bar][i] && i < (tab-1)) /* ...but here only for non-end column (which can spill over). */
      {
        width2[bar][i] = width[i];
      }

      if (t_width[i] > t_width1[bar][i]) /* All column widths are noted here... */
      {
        t_width1[bar][i] = t_width[i];
      }

      if (t_width[i] > t_width2[bar][i] && i < (tab-1)) /* ...but here only those for non-end column. */
      {
        t_width2[bar][i] = t_width[i];
      }
    }
  }

  font_lose_font (font_n);
  font_lose_font (font_b);

  /* Go through the columns, storing the widths into the report data block.  If right alignment has been used, we
   * must record the widest width; if not, we can get away with the widest non-end-column width.
   */

  for (i=0; i<REPORT_TAB_BARS; i++)
  {
    for (j=0; j<REPORT_TAB_STOPS; j++)
    {
      report->font_width[i][j] = (right[i][j]) ? width1[i][j] : width2[i][j];
      report->text_width[i][j] = (right[i][j]) ? t_width1[i][j] : t_width2[i][j];
    }
  }

  /* Now set the tab stops up.  The first is at zero, then add each column width on and include the
   * inter-column gap.
   */

  for (i=0; i<REPORT_TAB_BARS; i++)
  {
    report->font_tab[i][0] = 0;

    #ifdef DEBUG
    debug_printf ("Set tab bar %d (tab 0 = 0)", i);
    #endif

    for (j=1; j<REPORT_TAB_STOPS; j++)
    {
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

  for (i=0; i<REPORT_TAB_BARS; i++)
  {
    for (j=0; j<REPORT_TAB_STOPS; j++)
    {
      if (width1[i][j] > 0 && report->font_tab[i][j] + width1[i][j] > total)
      {
        total = report->font_tab[i][j] + width1[i][j];
      }
    }
  }

  total += (REPORT_LEFT_MARGIN + REPORT_RIGHT_MARGIN);

  return (total);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return true if any reports in the file have print jobs pending. */

int pending_print_reports (file_data *file)
{
  report_data *list;
  int         pending;

  list = file->reports;
  pending = 0;

  while (list != NULL)
  {
    if (list->print_pending > 0)
    {
      pending = 1;
    }

    list = list->next;
  }

  return (pending);
}

/* ==================================================================================================================
 * Editing report format via the GUI.
 */

void open_report_format_window (file_data *file, report_data *report, wimp_pointer *ptr)
{
  extern global_windows windows;


  /* If the window is already open, another report format is being edited.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.report_format))
  {
    wimp_close_window (windows.report_format);
  }

  /* Set the window contents up. */

  fill_report_format_window (file, report);

  /* Set the pointers up so we can find this lot again and open the window. */

  report_format_file = file;
  report_format_report = report;

  windows_open_centred_at_pointer (windows.report_format, ptr);
  place_dialogue_caret (windows.report_format, REPORT_FORMAT_FONTSIZE);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_report_format_window (void)
{
  extern global_windows windows;

  fill_report_format_window (report_format_file, report_format_report);
  icons_redraw_group (windows.report_format, 4, REPORT_FORMAT_NFONT, REPORT_FORMAT_BFONT,
                          REPORT_FORMAT_FONTSIZE, REPORT_FORMAT_FONTSPACE);
  icons_replace_caret_in_window (windows.report_format);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_report_format_window (file_data *file, report_data *report)
{
  extern global_windows windows;


  sprintf (icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_NFONT), "%s", report->font_normal);
  sprintf (icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_BFONT), "%s", report->font_bold);

  sprintf (icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_FONTSIZE), "%d", report->font_size / 16);
  sprintf (icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_FONTSPACE), "%d", report->line_spacing);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take the contents of an updated report format window and process the data. */

int process_report_format_window (void)
{
  int               linespace, new_xextent, new_yextent, visible_xextent, visible_yextent, new_xscroll, new_yscroll;
  os_box            extent;
  wimp_window_state state;

  extern global_windows windows;


  /* Extract the information. */

  strcpy (report_format_report->font_normal, icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_NFONT));
  strcpy (report_format_report->font_bold, icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_BFONT));
  report_format_report->font_size = atoi (icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_FONTSIZE)) * 16;
  report_format_report->line_spacing = atoi (icons_get_indirected_text_addr (windows.report_format, REPORT_FORMAT_FONTSPACE));

  /* Tidy up and redraw the windows */

  report_format_report->width = format_report_columns (report_format_report);
  font_convertto_os (1000 * (report_format_report->font_size / 16) * report_format_report->line_spacing / 100, 0,
                     &linespace, NULL);
  report_format_report->height = report_format_report->lines * linespace + REPORT_BOTTOM_MARGIN;

  /* Calculate the next window extents. */

  new_xextent = (report_format_report->width > REPORT_MIN_WIDTH) ? report_format_report->width : REPORT_MIN_WIDTH;
  new_yextent = (report_format_report->height > REPORT_MIN_HEIGHT) ? -report_format_report->height : -REPORT_MIN_HEIGHT;

  /* Get the current window details, and find the extent of the bottom and right of the visible area. */

  state.w = report_format_report->window;
  wimp_get_window_state (&state);

  visible_xextent = state.xscroll + (state.visible.x1 - state.visible.x0);
  visible_yextent = state.yscroll + (state.visible.y0 - state.visible.y1);

  /* If the visible area falls outside the new window extent, then the window needs to be re-opened first. */

  if (new_xextent < visible_xextent || new_yextent > visible_yextent)
  {
    /* Calculate the required new scroll offsets.
     *
     * Start with the x scroll.  If this is less than zero, the window is too wide and will need shrinking down.
     * Otherwise, just set the new scroll offset.
     */

    new_xscroll = new_xextent - (state.visible.x1 - state.visible.x0);

    if (new_xscroll < 0)
    {
      state.visible.x1 += new_xscroll;
      state.xscroll = 0;
    }
    else
    {
      state.xscroll = new_xscroll;
    }

    /* Now do the y scroll.  If this is greater than zero, the current window is too deep and will need
     * shrinking down.  Otherwise, just set the new scroll offset.
     */

    new_yscroll = new_yextent - (state.visible.y0 - state.visible.y1);

    if (new_yscroll > 0)
    {
      state.visible.y0 += new_yscroll;
      state.yscroll = 0;
    }
    else
    {
      state.yscroll = new_yscroll;
    }

    wimp_open_window ((wimp_open *) &state);
  }

  /* Finally, call Wimp_SetExtent to update the extent, safe in the knowledge that the visible area will still
   * exist.
   */

  extent.x0 = 0;
  extent.x1 = new_xextent;
  extent.y1 = 0;
  extent.y0 = new_yextent;
  wimp_set_extent (report_format_report->window, &extent);

  windows_redraw (report_format_report->window);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the report format window if the file disappears. */

void force_close_report_format_window (file_data *file)
{
  extern global_windows windows;


  if (report_format_file == file && windows_get_open (windows.report_format))
  {
    close_dialogue_with_caret (windows.report_format);
  }
}

/* ==================================================================================================================
 * Printing reports via the GUI.
 */

void open_report_print_window (file_data *file, report_data *report, wimp_pointer *ptr, int clear)
{
  /* Set the pointers up so we can find this lot again and open the window. */

  report_print_file = file;
  report_print_report = report;

  open_simple_print_window (file, ptr, clear, "PrintReport", report_print_window_closed);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called when Print is selected in the simple print dialogue.  Start the printing process going. */

void report_print_window_closed (int text, int format, int scale, int rotate)
{
  #ifdef DEBUG
  debug_printf ("Report print received data from simple print window");
  #endif

  /* Extract the information. */

  print_opt_text = text;
  print_opt_textformat = format;
  print_opt_fitwidth = scale;
  print_opt_rotate = rotate;

  /* Start the print dialogue process.
   * This process is also used by the direct report print function close_and_print_report (), so the two probably
   * can't co-exist.
   */

  report_print_report->print_pending++;

  send_start_print_save (start_report_print, cancel_report_print, print_opt_text);
}

/* ================================================================================================================== */

/* Return the report block shown in the given window. */

report_data *find_report_window_from_handle (file_data *file, wimp_w window)
{
  report_data *rep, *found;

  rep = file->reports;
  found = NULL;

  while (rep != NULL && found == NULL)
  {
    if (rep->window == window)
    {
      found = rep;
    }

    rep = rep->next;
  }

  return (found);
}

/* ==================================================================================================================
 * Window handling
 */

void report_window_click (file_data *file, wimp_pointer *pointer)
{
  report_data *report;


  /* Find the window block. */

  report = find_report_window_from_handle (file, pointer->w);

  if (report != NULL)
  {
    if (pointer->buttons == wimp_CLICK_MENU)
    {
      open_reportview_menu (file, report, pointer);
    }
  }
}

/* ==================================================================================================================
 * Window redraw
 */

void redraw_report_window (wimp_draw *redraw, file_data *file)
{
  int         ox, oy, top, base, y, linespace, bar, tab, indent, total, width;
  font_f      font, font_n, font_b;
  report_data *report;
  osbool      more;

  char        *column, *paint, *flags, buffer[REPORT_MAX_LINE_LEN+10];


  report = find_report_window_from_handle (file, redraw->w);

  /* Perform the redraw if a window was found. */

  if (file != NULL && report != NULL)
  {
    /* Find the required font, set it and calculate the font size from the linespacing in points. */

    find_report_fonts (report, &font_n, &font_b);

    font_convertto_os (1000 * (report->font_size / 16) * report->line_spacing / 100, 0, &linespace, NULL);

    more = wimp_redraw_window (redraw);

    ox = redraw->box.x0 - redraw->xscroll;
    oy = redraw->box.y1 - redraw->yscroll;

    /* Perform the redraw. */

    while (more)
    {
      /* Calculate the rows to redraw. */

      top = (oy - redraw->clip.y1) / linespace;
      if (top < 0)
      {
        top = 0;
      }

      base = (linespace + (linespace / 2) + oy - redraw->clip.y0 ) / linespace;

      /* Redraw the data into the window. */

      for (y = top; y < report->lines && y <= base; y++)
      {
        tab = 0;
        column = report->data + report->line_ptr[y];
        bar = (int) *column;
        column += REPORT_BAR_BYTES;

        do
        {
          flags = column;
          column += REPORT_FLAG_BYTES;

          font = (*flags & REPORT_FLAG_BOLD) ? font_b : font_n;
          indent = (*flags & REPORT_FLAG_INDENT) ? REPORT_COLUMN_INDENT : 0;

          if (*flags & REPORT_FLAG_RIGHT)
          {
            font_scan_string (font, column, font_KERN | font_GIVEN_FONT, 0x7fffffff, 0x7fffffff, NULL, NULL, 0, NULL,
                              &total, NULL, NULL);
            font_convertto_os (total, 0, &width, NULL);
            indent = report->font_width[bar][tab] - width;
          }

          if (*flags & REPORT_FLAG_UNDER)
          {
            *(buffer+0) = font_COMMAND_UNDERLINE;
            *(buffer+1) = 230;
            *(buffer+2) = 18;
            strcpy (buffer+3, column);
            paint = buffer;
          }
          else
          {
            paint = column;
          }

          wimp_set_font_colours (wimp_COLOUR_WHITE, wimp_COLOUR_BLACK);
          font_paint (font, paint, font_OS_UNITS | font_KERN | font_GIVEN_FONT,
                      ox + REPORT_LEFT_MARGIN + report->font_tab[bar][tab] + indent,
                      oy - linespace * (y+1) + REPORT_BASELINE_OFFSET, NULL, NULL, 0);

          while (*column != '\0' && *column != '\n')
          {
            column++;
          }
          column++;
          tab++;
        }
        while (*(column - 1) != '\0' && tab < REPORT_TAB_STOPS);
      }

      more = wimp_get_rectangle (redraw);
    }

    font_lose_font (font_n);
    font_lose_font (font_b);
  }
}

/* ==================================================================================================================
 * Saving and export
 */

void save_report_text (file_data *file, report_data *report, char *filename, int formatting)
{
  FILE           *out;
  int            i, j, bar, tab, indent, width, overrun, escape;
  char           *column, *flags, buffer[256];

  out = fopen (filename, "w");

  if (out != NULL)
  {
    hourglass_on ();

    for (i = 0; i < report->lines; i++)
    {
      tab = 0;
      overrun = 0;
      column = report->data + report->line_ptr[i];
      bar = (int) *column;
      column += REPORT_BAR_BYTES;

      do
      {
        flags = column;
        column += REPORT_FLAG_BYTES;
        string_ctrl_strcpy (buffer, column);

        escape = (*flags & REPORT_FLAG_BOLD) ? 0x01 : 0x00;
        if (*flags & REPORT_FLAG_UNDER)
        {
          escape |= 0x08;
        }

        indent = (*flags & REPORT_FLAG_INDENT) ? REPORT_TEXT_COLUMN_INDENT : 0;
        width = strlen (buffer);

        if (*flags & REPORT_FLAG_RIGHT)
        {
          indent = report->text_width[bar][tab] - width;
        }

        /* Output the indent spaces. */

        for (j=0; j<indent; j++)
        {
          fputc (' ', out);
        }

        /* Output fancy text formatting codes (used when printing formatted text) */

        if (formatting && escape != 0)
        {
          fputc ((char) 27, out);
          fputc ((char) (0x80 | escape), out);
        }

        /* Output the actual field data. */

        fprintf (out, "%s", buffer);

        /* Output fancy text formatting codes (used when printing formatted text) */

        if (formatting && escape != 0)
        {
          fputc ((char) 27, out);
          fputc ((char) 0x80, out);
        }
        /* Find the next field. */

        while (*column != '\0' && *column != '\n')
        {
          column++;
        }

        /* If there is another field, pad out with spaces. */

        if (*column != '\0')
        {
          /* Check the actual width against that allocated.  If it is more, note the amount that spills into the next
           * column, taking into account the width of the inter column gap.
           */

          if ((width+indent) > report->text_width[bar][tab])
          {
            overrun += (width+indent) - report->text_width[bar][tab] - REPORT_TEXT_COLUMN_SPACE;
          }

          /* Pad out the required number of spaces, taking into account any overspill from earlier columns. */

          for (j=0; j<(report->text_width[bar][tab] - (width+indent) + REPORT_TEXT_COLUMN_SPACE - overrun); j++)
          {
            fputc (' ', out);
          }

          /* Reduce the overspill record by the amount of free space in this column. */

          if ((width+indent) < report->text_width[bar][tab])
          {
            overrun -= report->text_width[bar][tab] - (width+indent) + REPORT_TEXT_COLUMN_SPACE;
            if (overrun < 0)
            {
              overrun = 0;
            }
          }
        }

        column++;
        tab++;
      }
      while (*(column - 1) != '\0' && tab < REPORT_TAB_STOPS);

      fputc ('\n', out);
    }

    /* Close the file and set the type correctly. */

    fclose (out);
    osfile_set_type (filename, (bits) (formatting) ? FANCYTEXT_FILE_TYPE : TEXT_FILE_TYPE);

    hourglass_off ();
  }
  else
  {
    error_msgs_report_error ("FileSaveFail");
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void export_delimited_report_file (file_data *file, report_data *report, char *filename, int format, int filetype)
{
  FILE           *out;
  int            i, tab, delimit;
  char           *column, *flags, buffer[256];

  out = fopen (filename, "w");

  if (out != NULL)
  {
    hourglass_on ();

    for (i = 0; i < report->lines; i++)
    {
      tab = 0;
      column = report->data + report->line_ptr[i] + REPORT_BAR_BYTES;

      do
      {
        flags = column;
        column += REPORT_FLAG_BYTES;
        string_ctrl_strcpy (buffer, column);

        /* Find the next field. */

        while (*column != '\0' && *column != '\n')
        {
          column++;
        }

        /* Output the actual field data. */

        delimit = (*column == '\0') ? DELIMIT_LAST : 0;

        if (*flags & REPORT_FLAG_NUMERIC)
        {
          delimit |= DELIMIT_NUM;
        }

        delimited_field_output (out, buffer, format, delimit);

        column++;
        tab++;
      }
      while (*(column - 1) != '\0' && tab < REPORT_TAB_STOPS);
    }

    /* Close the file and set the type correctly. */

    fclose (out);
    osfile_set_type (filename, (bits) filetype);

    hourglass_off ();
  }
  else
  {
    error_msgs_report_error ("FileSaveFail");
  }
}

/* ==================================================================================================================
 * Report printing
 */

/* Called when the negotiations with the printer driver have been finished, to actually start the printing process. */

void start_report_print (char *filename)
{
  if (print_opt_text)
  {
    save_report_text (report_print_file, report_print_report, filename, print_opt_textformat);
  }
  else
  {
    print_report_graphic (report_print_file, report_print_report, print_opt_fitwidth, print_opt_rotate);
  }

  /* Tidy up afterwards.  If that was the last print job in progress and the window has already been closed (or if
   * there wasn't a window at all), delete the report data.
   */

  if (--report_print_report->print_pending <= 0 && report_print_report->window == NULL)
  {
    #ifdef DEBUG
    debug_printf ("Deleting report post printing...");
    #endif

    delete_report (report_print_file, report_print_report);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Called if the negotiations with the printer driver break down, to tidy up. */

void cancel_report_print (void)
{
  if (--report_print_report->print_pending <= 0 && report_print_report->window == NULL)
  {
    #ifdef DEBUG
    debug_printf ("Deleting report after a failed print...");
    #endif

    delete_report (report_print_file, report_print_report);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void print_report_graphic (file_data *file, report_data *report, int fit_width, int rotate)
{
  os_error         *error;
  os_fw            out = 0;
  os_box           p_rect, rect;
  os_hom_trfm      p_trfm;
  os_coord         p_pos;
  char             title[1024];
  pdriver_features features;
  font_f           font, font_n = 0, font_b = 0;
  osbool           more;
  int              page_xsize, page_ysize, page_left, page_right, page_top, page_bottom;
  int              margin_left, margin_right, margin_top, margin_bottom, margin_fail;
  int              top, base, y, linespace, bar, tab, indent, total, width;
  unsigned int     page_width, page_height, scale, page_xstart, page_ystart;
  char             *column, *paint, *flags, buffer[REPORT_MAX_LINE_LEN+10];

  hourglass_on ();

  /* Get the printer driver settings. */

  error = xpdriver_info (NULL, NULL, NULL, &features, NULL, NULL, NULL, NULL);

  if (error != NULL)
  {
    handle_print_error (out, error, font_n, font_b);
    return;
  }

  /* Find the fonts we will use. */

  find_report_fonts (report, &font_n, &font_b);
  font_convertto_os (1000 * (report->font_size / 16) * report->line_spacing / 100, 0, &linespace, NULL);

  /* Get the page dimensions, and set up the print margins.  If the margins are bigger than the print
   * borders, the print borders are increased to match.
   */

  error = xpdriver_page_size (&page_xsize, &page_ysize, &page_left, &page_bottom, &page_right, &page_top);

  if (error != NULL)
  {
    handle_print_error (out, error, font_n, font_b);
    return;
  }

  margin_fail = FALSE;

  margin_left = page_left;

  if (config_int_read ("PrintMarginLeft") > 0 && config_int_read ("PrintMarginLeft") > margin_left)
  {
    margin_left = config_int_read ("PrintMarginLeft");

    page_left = margin_left;
  }
  else
  {
    if (config_int_read ("PrintMarginLeft") > 0)
    {
      margin_fail = TRUE;
    }
  }

  margin_bottom = page_bottom;

  if (config_int_read ("PrintMarginBottom") > 0 && config_int_read ("PrintMarginBottom") > margin_bottom)
  {
    margin_bottom = config_int_read ("PrintMarginBottom");

    page_bottom = margin_bottom;
  }
  else
  {
    if (config_int_read ("PrintMarginBottom") > 0)
    {
      margin_fail = TRUE;
    }
  }

  margin_right = page_xsize - page_right;

  if (config_int_read ("PrintMarginRight") > 0 && config_int_read ("PrintMarginRight") > margin_right)
  {
    margin_right = config_int_read ("PrintMarginRight");

    page_right = page_xsize - margin_right;
  }
  else
  {
    if (config_int_read ("PrintMarginRight") > 0)
    {
      margin_fail = TRUE;
    }
  }

  margin_top = page_ysize - page_top;

  if (config_int_read ("PrintMarginTop") > 0 && config_int_read ("PrintMarginTop") > margin_top)
  {
    margin_top = config_int_read ("PrintMarginTop");

    page_top = page_ysize - margin_top;
  }
  else
  {
    if (config_int_read ("PrintMarginTop") > 0)
    {
      margin_fail = TRUE;
    }
  }

  if (margin_fail)
  {
    error_msgs_report_error ("BadPrintMargins");
  }

  /* Open a printout file. */

  error = xosfind_openoutw (osfind_NO_PATH, "printer:", NULL, &out);

  if (error != NULL)
  {
    handle_print_error (out, error, font_n, font_b);
    return;
  }

  /* Start a print job. */

  msgs_param_lookup ("PJobTitle", title, sizeof (title), report->window_title, NULL, NULL, NULL);
  error = xpdriver_select_jobw (out, title, NULL);

  if (error != NULL)
  {
    handle_print_error (out, error, font_n, font_b);
    return;
  }

  /* Declare the fonts we are using, if required. */

  if (features & pdriver_FEATURE_DECLARE_FONT)
  {
    xpdriver_declare_font (font_n, 0, pdriver_KERNED);
    xpdriver_declare_font (font_b, 0, pdriver_KERNED);
    xpdriver_declare_font (0, 0, 0);
  }

  /* Calculate the page size, positions, transformations etc. */

  /* The printable page width and height, in milli-points and then into OS units. */

  if (rotate)
  {
    page_width = page_top - page_bottom;
    page_height = page_right - page_left;
  }
  else
  {
    page_width = page_right - page_left;
    page_height = page_top - page_bottom;
  }

  error = xfont_convertto_os (page_width, page_height, (int *) &page_width, (int *) &page_height);

  if (error != NULL)
  {
    handle_print_error (out, error, font_n, font_b);
    return;
  }

  /* Scale is the scaling factor to get the width of the report to fit onto one page, if required. The scale is
   * never more than 1:1 (we never enlarge the print).
   */

  if (fit_width)
  {
    scale = (1 << 16) * page_width / report->width;
    scale = (scale > (1 << 16)) ? (1 << 16) : scale;
  }
  else
  {
    scale = 1 << 16;
  }

  /* The page width and page height now need to be worked out in terms of what we actually want to print.
   * If scaling is on, the width is the report width and the height is the true page height scaled up in
   * proportion; otherwise, these stay as the true printable area in OS units.
   */

  if (fit_width)
  {
    if (page_width < report->width)
    {
      page_height = page_height * report->width / page_width;
    }

    page_width = report->width;
  }

  /* Clip the page length to be an exect number of lines */

  page_height -= (page_height % linespace);

  /* Set up the transformation matrix scale the page and rotate it as required. */

  if (rotate)
  {
    p_trfm.entries[0][0] = 0;
    p_trfm.entries[0][1] = scale;
    p_trfm.entries[1][0] = -scale;
    p_trfm.entries[1][1] = 0;
  }
  else
  {
    p_trfm.entries[0][0] = scale;
    p_trfm.entries[0][1] = 0;
    p_trfm.entries[1][0] = 0;
    p_trfm.entries[1][1] = scale;
  }

  /* Loop through the pages down the report and across. */

  for (page_ystart = 0; page_ystart < report->height; page_ystart += page_height)
  {
    for (page_xstart = 0; page_xstart < report->width; page_xstart += page_width)
    {
      /* Calculate the area of the page to print and set up the print rectangle.  If the page is on the edge,
       * crop the area down to save memory.
       */

      p_rect.x0 = page_xstart;
      p_rect.x1 = (page_xstart + page_width <= report->width) ? page_xstart + page_width : report->width;
      p_rect.y1 = -page_ystart;


      /* The bottom y edge is done specially, because we also need to set the print position.  If the page is at the
       * edge, it is cropped down to save on memory.
       *
       * The page origin will depend on rotation and the amount of text on the page.  For a full page, the
       * origin is placed at one corner (either bottom left for a portrait, or bottom right for a landscape).
       * For part pages, the origin is shifted left or up by the proportion of the page dimension (in milli-points)
       * taken from the proportion of OS units used for layout.
       */

      if (page_ystart + page_height <= report->height)
      {
        p_rect.y0 = -page_ystart - page_height;

        if (rotate)
        {
          p_pos.x = page_right;
          p_pos.y = page_bottom;
        }
        else
        {
          p_pos.x = page_left;
          p_pos.y = page_bottom;
        }
      }
      else
      {
        p_rect.y0 = -report->height;

        if (rotate)
        {
          p_pos.x = page_right - (page_right - page_left) * (page_height + (p_rect.y0 - p_rect.y1)) / page_height;
          p_pos.y = page_bottom;
        }
        else
        {
          p_pos.x = page_left;
          p_pos.y = page_bottom + (page_top - page_bottom) * (page_height + (p_rect.y0 - p_rect.y1)) / page_height;
        }
      }


      /* Pass the page details to the printer driver and start to draw the page. */

      error = xpdriver_give_rectangle (0, &p_rect, &p_trfm, &p_pos, os_COLOUR_WHITE);

      if (error != NULL)
      {
        handle_print_error (out, error, font_n, font_b);
        return;
      }

      error = xpdriver_draw_page (1, &rect, 0, 0, &more, NULL);

      /* Perform the redraw. */

      while (more)
      {
        /* Calculate the rows to redraw. */

        top = -rect.y1 / linespace;
        if (top < 0)
        {
          top = 0;
        }

        base = (linespace + (linespace / 2) - rect.y0 ) / linespace;

        /* Redraw the data into the window. */

        for (y = top; y < report->lines && y <= base; y++)
        {
          tab = 0;
          column = report->data + report->line_ptr[y];
          bar = (int) *column;
          column += REPORT_BAR_BYTES;

          do
          {
            flags = column;
            column += REPORT_FLAG_BYTES;

            font = (*flags & REPORT_FLAG_BOLD) ? font_b : font_n;
            indent = (*flags & REPORT_FLAG_INDENT) ? REPORT_COLUMN_INDENT : 0;

            if (*flags & REPORT_FLAG_RIGHT)
            {
              error = xfont_scan_string (font, column, font_KERN | font_GIVEN_FONT, 0x7fffffff, 0x7fffffff, NULL, NULL,
                                         0, NULL, &total, NULL, NULL);

              if (error != NULL)
              {
                handle_print_error (out, error, font_n, font_b);
                return;
              }

              error = xfont_convertto_os (total, 0, &width, NULL);

              if (error != NULL)
              {
                handle_print_error (out, error, font_n, font_b);
                return;
              }

              indent = report->font_width[bar][tab] - width;
            }

            if (*flags & REPORT_FLAG_UNDER)
            {
              *(buffer+0) = font_COMMAND_UNDERLINE;
              *(buffer+1) = 230;
              *(buffer+2) = 18;
              strcpy (buffer+3, column);
              paint = buffer;
            }
            else
            {
              paint = column;
            }

            error = xcolourtrans_set_font_colours (font, os_COLOUR_WHITE, os_COLOUR_BLACK, 0, NULL, NULL, NULL);

            if (error != NULL)
            {
              handle_print_error (out, error, font_n, font_b);
              return;
            }

            error = xfont_paint (font, paint, font_OS_UNITS | font_KERN | font_GIVEN_FONT,
                        + REPORT_LEFT_MARGIN + report->font_tab[bar][tab] + indent,
                        - linespace * (y+1) + REPORT_BASELINE_OFFSET, NULL, NULL, 0);

            if (error != NULL)
            {
              handle_print_error (out, error, font_n, font_b);
              return;
            }

            while (*column != '\0' && *column != '\n')
            {
              column++;
            }
            column++;
            tab++;
          }
          while (*(column - 1) != '\0' && tab < REPORT_TAB_STOPS);
        }

        error = xpdriver_get_rectangle (&rect, &more, NULL);

        if (error != NULL)
        {
          handle_print_error (out, error, font_n, font_b);
          return;
        }
      }
    }
  }

  /* Terminate the print job. */

  error = xpdriver_end_jobw (out);

  if (error != NULL)
  {
    handle_print_error (out, error, font_n, font_b);
    return;
  }

  /* Close the printout file. */

  xosfind_closew (out);

  font_lose_font (font_n);
  font_lose_font (font_b);

  hourglass_off ();
}

/* ------------------------------------------------------------------------------------------------------------------ */

void handle_print_error (os_fw file, os_error *error, font_f f1, font_f f2)
{
  pdriver_abort_jobw (file);

  if (file != 0)
  {
    xpdriver_abort_jobw (file);
    xosfind_closew (file);
  }

  if (f1 != 0)
  {
    font_lose_font (f1);
  }

  if (f2 != 0)
  {
    font_lose_font (f2);
  }

  hourglass_off ();
  error_report_os_error (error, wimp_ERROR_BOX_CANCEL_ICON);
}
