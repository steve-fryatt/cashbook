/* CashBook - choices.c
 * (c) Stephen Fryatt, 2003
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>

/* Acorn C Header files. */


/* OSLib Header files. */

#include "oslib/wimp.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/icons.h"
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/colpick.h"

/* Application header files. */

#include "global.h"
#include "choices.h"

#include "caret.h"
#include "conversion.h"
#include "date.h"
#include "file.h"

/* ==================================================================================================================
 * Global variables
 */

int choices_pane = 0;

/* ==================================================================================================================
 * Open and close the window
 */

void open_choices_window (wimp_pointer *pointer)
{
  int i;

  extern global_windows windows;

  choices_pane = CHOICE_PANE_GENERAL;

  for (i=0; i<CHOICES_PANES; i++)
  {
    set_icon_selected (windows.choices, CHOICE_ICON_SELECT + i, i == choices_pane);
  }

  set_choices_window ();

  open_pane_dialogue_centred_at_pointer (windows.choices, windows.choices_pane[choices_pane],
                                         CHOICE_ICON_PANE, 0, pointer);

  place_dialogue_caret_fallback (windows.choices_pane[CHOICE_PANE_GENERAL], 2,
                                 CHOICE_ICON_DATEIN, CHOICE_ICON_DATEOUT);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void close_choices_window (void)
{
  extern global_windows windows;


  close_dialogue_with_caret (windows.choices_pane[choices_pane]);
  close_dialogue_with_caret (windows.choices);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void change_choices_pane (int pane)
{
  int        i, old_pane;
  wimp_caret caret;

  extern global_windows windows;


  if (pane < CHOICES_PANES && window_is_open (windows.choices) && pane != choices_pane)
  {
    wimp_get_caret_position (&caret);

    old_pane = choices_pane;
    choices_pane = pane;

    for (i=0; i<CHOICES_PANES; i++)
    {
      set_icon_selected (windows.choices, CHOICE_ICON_SELECT + i, i == choices_pane);
    }

    open_pane_centred_in_icon (windows.choices, windows.choices_pane[pane], CHOICE_ICON_PANE, 0,
                               windows.choices_pane[old_pane]);

    wimp_close_window (windows.choices_pane[old_pane]);

    if (caret.w == windows.choices_pane[old_pane])
    {
      switch (pane)
      {
        case CHOICE_PANE_GENERAL:
          place_dialogue_caret_fallback (windows.choices_pane[CHOICE_PANE_GENERAL], 2,
                                         CHOICE_ICON_DATEIN, CHOICE_ICON_DATEOUT);
          break;

        case CHOICE_PANE_CURRENCY:
          place_dialogue_caret_fallback (windows.choices_pane[CHOICE_PANE_CURRENCY], 2,
                                         CHOICE_ICON_DECIMALPLACE, CHOICE_ICON_DECIMALPOINT);
          break;

        case CHOICE_PANE_SORDER:
          place_dialogue_caret (windows.choices_pane[CHOICE_PANE_SORDER], wimp_ICON_WINDOW);
          break;

        case CHOICE_PANE_REPORT:
          place_dialogue_caret_fallback (windows.choices_pane[CHOICE_PANE_REPORT], 2,
                                         CHOICE_ICON_FONTSIZE, CHOICE_ICON_FONTSPACE);
          break;

        case CHOICE_PANE_PRINT:
          place_dialogue_caret_fallback (windows.choices_pane[CHOICE_PANE_PRINT], 4,
                                         CHOICE_ICON_MTOP, CHOICE_ICON_MLEFT,
                                         CHOICE_ICON_MRIGHT, CHOICE_ICON_MBOTTOM);
          break;

        case CHOICE_PANE_TRANSACT:
          place_dialogue_caret_fallback (windows.choices_pane[CHOICE_PANE_TRANSACT], 1,
                                         CHOICE_ICON_AUTOCOMP);
          break;

        case CHOICE_PANE_ACCOUNT:
          place_dialogue_caret (windows.choices_pane[CHOICE_PANE_ACCOUNT], wimp_ICON_WINDOW);
          break;
      }
    }
  }
}

/* ==================================================================================================================
 * Set choices window contents
 */

/* Set the contents of the Choices window to reflect the current settings. */

void set_choices_window (void)
{
  int i;

  extern global_windows windows;

  /* Set the general pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_CLIPBOARD,
                     read_config_opt ("GlobalClipboardSupport"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_RO5KEYS,
                     read_config_opt ("IyonixKeys"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_REMEMBERDIALOGUE,
                     read_config_opt ("RememberValues"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_TERRITORYDATE,
                     read_config_opt ("TerritoryDates"));

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEIN), "%s",
           read_config_str ("DateSepIn"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEOUT), "%s",
           read_config_str ("DateSepOut"));

  /* Set the currency pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_SHOWZERO,
                     read_config_opt ("PrintZeros"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM,
                     read_config_opt ("TerritoryCurrency"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGMINUS,
                     !read_config_opt ("BracketNegatives"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGBRACE,
                     read_config_opt ("BracketNegatives"));

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPLACE), "%d",
           read_config_int ("DecimalPlaces"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPOINT), "%s",
           read_config_str ("DecimalPoint"));

  set_icons_shaded_when_radio_on (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM, 10,
                                  CHOICE_ICON_FORMATFRAME, CHOICE_ICON_FORMATLABEL,
                                  CHOICE_ICON_DECIMALPLACELABEL, CHOICE_ICON_DECIMALPLACE,
                                  CHOICE_ICON_DECIMALPOINTLABEL, CHOICE_ICON_DECIMALPOINT,
                                  CHOICE_ICON_NEGFRAME, CHOICE_ICON_NEGLABEL,
                                  CHOICE_ICON_NEGMINUS, CHOICE_ICON_NEGBRACE);

  /* Set the standing order pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SORTAFTERSO,
                     read_config_opt ("SortAfterSOrders"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_AUTOSORTSO,
                     read_config_opt ("AutoSortSOrders"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO,
                     read_config_opt ("TerritorySOrders"));

  for (i=0; i<7; i++)
  {
     set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SOSUN+i,
                        read_config_int ("WeekendDays") & (1 << i));
  }

  set_icons_shaded_when_radio_on (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO, 9,
                                  CHOICE_ICON_WEEKENDFRAME, CHOICE_ICON_WEEKENDLABEL,
                                  CHOICE_ICON_SOSUN, CHOICE_ICON_SOMON, CHOICE_ICON_SOTUE, CHOICE_ICON_SOWED,
                                  CHOICE_ICON_SOTHU, CHOICE_ICON_SOFRI, CHOICE_ICON_SOSAT);

  /* Set the printing pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_STANDARD,
                     !read_config_opt ("PrintText"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_PORTRAIT,
                     !read_config_opt ("PrintRotate"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_LANDSCAPE,
                     read_config_opt ("PrintRotate"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_SCALE,
                     read_config_opt ("PrintFitWidth"));

  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_FASTTEXT,
                     read_config_opt ("PrintText"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_TEXTFORMAT,
                     read_config_opt ("PrintTextFormat"));

  set_radio_icon_group_selected (windows.choices_pane[CHOICE_PANE_PRINT], read_config_int ("PrintMarginUnits"), 3,
                                 CHOICE_ICON_MMM, CHOICE_ICON_MCM, CHOICE_ICON_MINCH);

  switch (read_config_int ("PrintMarginUnits"))
  {
    case MARGIN_UNIT_MM:
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%.2f",
               (float) read_config_int ("PrintMarginTop") / UNIT_MM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%.2f",
               (float) read_config_int ("PrintMarginLeft") / UNIT_MM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%.2f",
               (float) read_config_int ("PrintMarginRight") / UNIT_MM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%.2f",
               (float) read_config_int ("PrintMarginBottom") / UNIT_MM_TO_MILLIPOINT);
      break;

    case MARGIN_UNIT_CM:
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%.2f",
               (float) read_config_int ("PrintMarginTop") / UNIT_CM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%.2f",
               (float) read_config_int ("PrintMarginLeft") / UNIT_CM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%.2f",
               (float) read_config_int ("PrintMarginRight") / UNIT_CM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%.2f",
               (float) read_config_int ("PrintMarginBottom") / UNIT_CM_TO_MILLIPOINT);
      break;

    case MARGIN_UNIT_INCH:
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%.2f",
               (float) read_config_int ("PrintMarginTop") / UNIT_INCH_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%.2f",
               (float) read_config_int ("PrintMarginLeft") / UNIT_INCH_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%.2f",
               (float) read_config_int ("PrintMarginRight") / UNIT_INCH_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%.2f",
               (float) read_config_int ("PrintMarginBottom") / UNIT_INCH_TO_MILLIPOINT);
      break;
  }

  /* Set the report pane up. */

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT), "%s",
           read_config_str ("ReportFontNormal"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT), "%s",
           read_config_str ("ReportFontBold"));

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSIZE), "%d",
           read_config_int ("ReportFontSize"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSPACE), "%d",
           read_config_int ("ReportFontLinespace"));

  /* Set the transaction pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORT,
                     read_config_opt ("AutoSort"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_TRANSDEL,
                     read_config_opt ("AllowTransDelete"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HIGHLIGHT,
                     read_config_opt ("ShadeReconciled"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTCOL,
                   read_config_int ("ShadeReconciledColour"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOCOMP), "%d",
           read_config_int ("MaxAutofillLen"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORTPRE,
                     read_config_opt ("AutoSortPresets"));

  /* Set the account pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHIGHLIGHT,
                     read_config_opt ("ShadeAccounts"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTCOL,
                   read_config_int ("ShadeAccountsColour"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHIGHLIGHT,
                     read_config_opt ("ShadeBudgeted"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTCOL,
                   read_config_int ("ShadeBudgetedColour"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHIGHLIGHT,
                     read_config_opt ("ShadeOverdrawn"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHILIGHTCOL,
           read_config_int ("ShadeOverdrawnColour"));
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Read the contents of the Choices window into the settings. */

void read_choices_window (void)
{
  int   i, ignore;
  float top, left, right, bottom;

  extern global_windows windows;

  /* Read the general pane. */

  set_config_opt ("GlobalClipboardSupport",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_CLIPBOARD));
  set_config_opt ("IyonixKeys",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_RO5KEYS));
  set_config_opt ("RememberValues",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_REMEMBERDIALOGUE));
  set_config_opt ("TerritoryDates",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_TERRITORYDATE));

  set_config_str ("DateSepIn",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEIN));
  set_config_str ("DateSepOut",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEOUT));


  /* Read the currency pane. */

  set_config_opt ("PrintZeros",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_SHOWZERO));
  set_config_opt ("TerritoryCurrency",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM));
  set_config_opt ("BracketNegatives",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGBRACE));
  set_config_int ("DecimalPlaces",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPLACE)));
  set_config_str ("DecimalPoint",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPOINT));

  /* Read the standing order pane.*/

  set_config_opt ("SortAfterSOrders",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SORTAFTERSO));
  set_config_opt ("AutoSortSOrders",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_AUTOSORTSO));
  set_config_opt ("TerritorySOrders",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO));

  ignore = 0;

  for (i=0; i<7; i++)
  {
    if (read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SOSUN+i))
    {
      ignore |= 1 << i;
    }
  }

  set_config_int ("WeekendDays", ignore);

  /* Read the printing pane. */

  set_config_opt ("PrintFitWidth",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_SCALE));
  set_config_opt ("PrintRotate",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_LANDSCAPE));
  set_config_opt ("PrintText",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_FASTTEXT));
  set_config_opt ("PrintTextFormat",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_TEXTFORMAT));

  set_config_int ("PrintMarginUnits", read_radio_icon_group_selected (windows.choices_pane[CHOICE_PANE_PRINT], 3,
                  CHOICE_ICON_MMM, CHOICE_ICON_MCM, CHOICE_ICON_MINCH));

  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%f", &top);
  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%f", &left);
  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%f", &right);
  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%f", &bottom);

  switch (read_config_int ("PrintMarginUnits"))
  {
    case MARGIN_UNIT_MM:
      set_config_int ("PrintMarginTop", (int) (top * UNIT_MM_TO_MILLIPOINT));
      set_config_int ("PrintMarginLeft", (int) (left * UNIT_MM_TO_MILLIPOINT));
      set_config_int ("PrintMarginRight", (int) (right * UNIT_MM_TO_MILLIPOINT));
      set_config_int ("PrintMarginBottom", (int) (bottom * UNIT_MM_TO_MILLIPOINT));
      break;

    case MARGIN_UNIT_CM:
      set_config_int ("PrintMarginTop", (int) (top * UNIT_CM_TO_MILLIPOINT));
      set_config_int ("PrintMarginLeft", (int) (left * UNIT_CM_TO_MILLIPOINT));
      set_config_int ("PrintMarginRight", (int) (right * UNIT_CM_TO_MILLIPOINT));
      set_config_int ("PrintMarginBottom", (int) (bottom * UNIT_CM_TO_MILLIPOINT));
     break;

    case MARGIN_UNIT_INCH:
      set_config_int ("PrintMarginTop", (int) (top * UNIT_INCH_TO_MILLIPOINT));
      set_config_int ("PrintMarginLeft", (int) (left * UNIT_INCH_TO_MILLIPOINT));
      set_config_int ("PrintMarginRight", (int) (right * UNIT_INCH_TO_MILLIPOINT));
      set_config_int ("PrintMarginBottom", (int) (bottom * UNIT_INCH_TO_MILLIPOINT));
      break;
  }

  /* Read the report pane. */

  set_config_str ("ReportFontNormal",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT));
  set_config_str ("ReportFontBold",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT));
  set_config_int ("ReportFontSize",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSIZE)));
  set_config_int ("ReportFontLinespace",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSPACE)));

  /* Read the transaction pane. */

  set_config_opt ("AutoSort",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORT));
  set_config_opt ("AllowTransDelete",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_TRANSDEL));
  set_config_opt ("ShadeReconciled",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HIGHLIGHT));
  set_config_int ("ShadeReconciledColour",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTCOL)));
  set_config_int ("MaxAutofillLen",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOCOMP)));
   set_config_opt ("AutoSortPresets",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORTPRE));

  /* Read the account pane. */

  set_config_opt ("ShadeAccounts",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHIGHLIGHT));
  set_config_int ("ShadeAccountsColour",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTCOL)));
  set_config_opt ("ShadeBudgeted",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHIGHLIGHT));
  set_config_int ("ShadeBudgetedColour",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTCOL)));
  set_config_opt ("ShadeOverdrawn",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHIGHLIGHT));
  set_config_int ("ShadeOverdrawnColour",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHILIGHTCOL)));

  /* Update stored data. */

  set_weekend_days ();
  set_up_money ();

  /* Redraw windows as required. */

  redraw_all_files ();
}

/* ================================================================================================================== */

void redraw_choices_window (void)
{
  extern global_windows windows;

  /* Redraw the contents of the Choices window, as required, and re-fresh the caret if necessary.
   */

  switch (choices_pane)
  {
    case CHOICE_PANE_GENERAL:
      redraw_icons_in_window (windows.choices_pane[CHOICE_PANE_GENERAL], 2,
                              CHOICE_ICON_DATEIN, CHOICE_ICON_DATEOUT);
      break;

    case CHOICE_PANE_CURRENCY:
      redraw_icons_in_window (windows.choices_pane[CHOICE_PANE_CURRENCY], 2,
                              CHOICE_ICON_DECIMALPLACE, CHOICE_ICON_DECIMALPOINT);
      break;

    case CHOICE_PANE_SORDER:
      /* No icons to refresh manually. */
      break;

    case CHOICE_PANE_PRINT:
      redraw_icons_in_window (windows.choices_pane[CHOICE_PANE_PRINT], 4,
                              CHOICE_ICON_MTOP, CHOICE_ICON_MLEFT, CHOICE_ICON_MRIGHT, CHOICE_ICON_MBOTTOM);
      break;

    case CHOICE_PANE_REPORT:
      redraw_icons_in_window (windows.choices_pane[CHOICE_PANE_REPORT], 4,
                              CHOICE_ICON_NFONT, CHOICE_ICON_BFONT, CHOICE_ICON_FONTSIZE, CHOICE_ICON_FONTSPACE);
      break;

    case CHOICE_PANE_TRANSACT:
      redraw_icons_in_window (windows.choices_pane[CHOICE_PANE_TRANSACT], 1,
                              CHOICE_ICON_HILIGHTCOL);
      break;

    case CHOICE_PANE_ACCOUNT:
      redraw_icons_in_window (windows.choices_pane[CHOICE_PANE_ACCOUNT], 3,
                              CHOICE_ICON_AHILIGHTCOL, CHOICE_ICON_SHILIGHTCOL, CHOICE_ICON_OHILIGHTCOL);
      break;
  }

  replace_caret_in_window (windows.choices_pane[choices_pane]);
}
