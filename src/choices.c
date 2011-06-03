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
                     config_opt_read ("GlobalClipboardSupport"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_RO5KEYS,
                     config_opt_read ("IyonixKeys"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_REMEMBERDIALOGUE,
                     config_opt_read ("RememberValues"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_TERRITORYDATE,
                     config_opt_read ("TerritoryDates"));

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEIN), "%s",
           config_str_read ("DateSepIn"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEOUT), "%s",
           config_str_read ("DateSepOut"));

  /* Set the currency pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_SHOWZERO,
                     config_opt_read ("PrintZeros"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM,
                     config_opt_read ("TerritoryCurrency"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGMINUS,
                     !config_opt_read ("BracketNegatives"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGBRACE,
                     config_opt_read ("BracketNegatives"));

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPLACE), "%d",
           config_int_read ("DecimalPlaces"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPOINT), "%s",
           config_str_read ("DecimalPoint"));

  set_icons_shaded_when_radio_on (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM, 10,
                                  CHOICE_ICON_FORMATFRAME, CHOICE_ICON_FORMATLABEL,
                                  CHOICE_ICON_DECIMALPLACELABEL, CHOICE_ICON_DECIMALPLACE,
                                  CHOICE_ICON_DECIMALPOINTLABEL, CHOICE_ICON_DECIMALPOINT,
                                  CHOICE_ICON_NEGFRAME, CHOICE_ICON_NEGLABEL,
                                  CHOICE_ICON_NEGMINUS, CHOICE_ICON_NEGBRACE);

  /* Set the standing order pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SORTAFTERSO,
                     config_opt_read ("SortAfterSOrders"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_AUTOSORTSO,
                     config_opt_read ("AutoSortSOrders"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO,
                     config_opt_read ("TerritorySOrders"));

  for (i=0; i<7; i++)
  {
     set_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SOSUN+i,
                        config_int_read ("WeekendDays") & (1 << i));
  }

  set_icons_shaded_when_radio_on (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO, 9,
                                  CHOICE_ICON_WEEKENDFRAME, CHOICE_ICON_WEEKENDLABEL,
                                  CHOICE_ICON_SOSUN, CHOICE_ICON_SOMON, CHOICE_ICON_SOTUE, CHOICE_ICON_SOWED,
                                  CHOICE_ICON_SOTHU, CHOICE_ICON_SOFRI, CHOICE_ICON_SOSAT);

  /* Set the printing pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_STANDARD,
                     !config_opt_read ("PrintText"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_PORTRAIT,
                     !config_opt_read ("PrintRotate"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_LANDSCAPE,
                     config_opt_read ("PrintRotate"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_SCALE,
                     config_opt_read ("PrintFitWidth"));

  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_FASTTEXT,
                     config_opt_read ("PrintText"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_TEXTFORMAT,
                     config_opt_read ("PrintTextFormat"));

  set_radio_icon_group_selected (windows.choices_pane[CHOICE_PANE_PRINT], config_int_read ("PrintMarginUnits"), 3,
                                 CHOICE_ICON_MMM, CHOICE_ICON_MCM, CHOICE_ICON_MINCH);

  switch (config_int_read ("PrintMarginUnits"))
  {
    case MARGIN_UNIT_MM:
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%.2f",
               (float) config_int_read ("PrintMarginTop") / UNIT_MM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%.2f",
               (float) config_int_read ("PrintMarginLeft") / UNIT_MM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%.2f",
               (float) config_int_read ("PrintMarginRight") / UNIT_MM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%.2f",
               (float) config_int_read ("PrintMarginBottom") / UNIT_MM_TO_MILLIPOINT);
      break;

    case MARGIN_UNIT_CM:
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%.2f",
               (float) config_int_read ("PrintMarginTop") / UNIT_CM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%.2f",
               (float) config_int_read ("PrintMarginLeft") / UNIT_CM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%.2f",
               (float) config_int_read ("PrintMarginRight") / UNIT_CM_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%.2f",
               (float) config_int_read ("PrintMarginBottom") / UNIT_CM_TO_MILLIPOINT);
      break;

    case MARGIN_UNIT_INCH:
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%.2f",
               (float) config_int_read ("PrintMarginTop") / UNIT_INCH_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%.2f",
               (float) config_int_read ("PrintMarginLeft") / UNIT_INCH_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%.2f",
               (float) config_int_read ("PrintMarginRight") / UNIT_INCH_TO_MILLIPOINT);
      sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%.2f",
               (float) config_int_read ("PrintMarginBottom") / UNIT_INCH_TO_MILLIPOINT);
      break;
  }

  /* Set the report pane up. */

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT), "%s",
           config_str_read ("ReportFontNormal"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT), "%s",
           config_str_read ("ReportFontBold"));

  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSIZE), "%d",
           config_int_read ("ReportFontSize"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSPACE), "%d",
           config_int_read ("ReportFontLinespace"));

  /* Set the transaction pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORT,
                     config_opt_read ("AutoSort"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_TRANSDEL,
                     config_opt_read ("AllowTransDelete"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HIGHLIGHT,
                     config_opt_read ("ShadeReconciled"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTCOL,
                   config_int_read ("ShadeReconciledColour"));
  sprintf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOCOMP), "%d",
           config_int_read ("MaxAutofillLen"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORTPRE,
                     config_opt_read ("AutoSortPresets"));

  /* Set the account pane up. */

  set_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHIGHLIGHT,
                     config_opt_read ("ShadeAccounts"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTCOL,
                   config_int_read ("ShadeAccountsColour"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHIGHLIGHT,
                     config_opt_read ("ShadeBudgeted"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTCOL,
                   config_int_read ("ShadeBudgetedColour"));
  set_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHIGHLIGHT,
                     config_opt_read ("ShadeOverdrawn"));
  set_colour_icon (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHILIGHTCOL,
           config_int_read ("ShadeOverdrawnColour"));
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Read the contents of the Choices window into the settings. */

void read_choices_window (void)
{
  int   i, ignore;
  float top, left, right, bottom;

  extern global_windows windows;

  /* Read the general pane. */

  config_opt_set ("GlobalClipboardSupport",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_CLIPBOARD));
  config_opt_set ("IyonixKeys",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_RO5KEYS));
  config_opt_set ("RememberValues",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_REMEMBERDIALOGUE));
  config_opt_set ("TerritoryDates",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_TERRITORYDATE));

  config_str_set ("DateSepIn",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEIN));
  config_str_set ("DateSepOut",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_GENERAL], CHOICE_ICON_DATEOUT));


  /* Read the currency pane. */

  config_opt_set ("PrintZeros",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_SHOWZERO));
  config_opt_set ("TerritoryCurrency",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_TERRITORYNUM));
  config_opt_set ("BracketNegatives",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_NEGBRACE));
  config_int_set ("DecimalPlaces",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPLACE)));
  config_str_set ("DecimalPoint",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_CURRENCY], CHOICE_ICON_DECIMALPOINT));

  /* Read the standing order pane.*/

  config_opt_set ("SortAfterSOrders",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SORTAFTERSO));
  config_opt_set ("AutoSortSOrders",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_AUTOSORTSO));
  config_opt_set ("TerritorySOrders",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_TERRITORYSO));

  ignore = 0;

  for (i=0; i<7; i++)
  {
    if (read_icon_selected (windows.choices_pane[CHOICE_PANE_SORDER], CHOICE_ICON_SOSUN+i))
    {
      ignore |= 1 << i;
    }
  }

  config_int_set ("WeekendDays", ignore);

  /* Read the printing pane. */

  config_opt_set ("PrintFitWidth",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_SCALE));
  config_opt_set ("PrintRotate",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_LANDSCAPE));
  config_opt_set ("PrintText",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_FASTTEXT));
  config_opt_set ("PrintTextFormat",
                  read_icon_selected (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_TEXTFORMAT));

  config_int_set ("PrintMarginUnits", read_radio_icon_group_selected (windows.choices_pane[CHOICE_PANE_PRINT], 3,
                  CHOICE_ICON_MMM, CHOICE_ICON_MCM, CHOICE_ICON_MINCH));

  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MTOP), "%f", &top);
  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MLEFT), "%f", &left);
  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MRIGHT), "%f", &right);
  sscanf (indirected_icon_text (windows.choices_pane[CHOICE_PANE_PRINT], CHOICE_ICON_MBOTTOM), "%f", &bottom);

  switch (config_int_read ("PrintMarginUnits"))
  {
    case MARGIN_UNIT_MM:
      config_int_set ("PrintMarginTop", (int) (top * UNIT_MM_TO_MILLIPOINT));
      config_int_set ("PrintMarginLeft", (int) (left * UNIT_MM_TO_MILLIPOINT));
      config_int_set ("PrintMarginRight", (int) (right * UNIT_MM_TO_MILLIPOINT));
      config_int_set ("PrintMarginBottom", (int) (bottom * UNIT_MM_TO_MILLIPOINT));
      break;

    case MARGIN_UNIT_CM:
      config_int_set ("PrintMarginTop", (int) (top * UNIT_CM_TO_MILLIPOINT));
      config_int_set ("PrintMarginLeft", (int) (left * UNIT_CM_TO_MILLIPOINT));
      config_int_set ("PrintMarginRight", (int) (right * UNIT_CM_TO_MILLIPOINT));
      config_int_set ("PrintMarginBottom", (int) (bottom * UNIT_CM_TO_MILLIPOINT));
     break;

    case MARGIN_UNIT_INCH:
      config_int_set ("PrintMarginTop", (int) (top * UNIT_INCH_TO_MILLIPOINT));
      config_int_set ("PrintMarginLeft", (int) (left * UNIT_INCH_TO_MILLIPOINT));
      config_int_set ("PrintMarginRight", (int) (right * UNIT_INCH_TO_MILLIPOINT));
      config_int_set ("PrintMarginBottom", (int) (bottom * UNIT_INCH_TO_MILLIPOINT));
      break;
  }

  /* Read the report pane. */

  config_str_set ("ReportFontNormal",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_NFONT));
  config_str_set ("ReportFontBold",
                  indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_BFONT));
  config_int_set ("ReportFontSize",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSIZE)));
  config_int_set ("ReportFontLinespace",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_REPORT], CHOICE_ICON_FONTSPACE)));

  /* Read the transaction pane. */

  config_opt_set ("AutoSort",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORT));
  config_opt_set ("AllowTransDelete",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_TRANSDEL));
  config_opt_set ("ShadeReconciled",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HIGHLIGHT));
  config_int_set ("ShadeReconciledColour",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_HILIGHTCOL)));
  config_int_set ("MaxAutofillLen",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOCOMP)));
   config_opt_set ("AutoSortPresets",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_TRANSACT], CHOICE_ICON_AUTOSORTPRE));

  /* Read the account pane. */

  config_opt_set ("ShadeAccounts",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHIGHLIGHT));
  config_int_set ("ShadeAccountsColour",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_AHILIGHTCOL)));
  config_opt_set ("ShadeBudgeted",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHIGHLIGHT));
  config_int_set ("ShadeBudgetedColour",
                  atoi (indirected_icon_text (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_SHILIGHTCOL)));
  config_opt_set ("ShadeOverdrawn",
                   read_icon_selected (windows.choices_pane[CHOICE_PANE_ACCOUNT], CHOICE_ICON_OHIGHLIGHT));
  config_int_set ("ShadeOverdrawnColour",
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
