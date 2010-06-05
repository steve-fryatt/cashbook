/* CashBook - choices.h
 * (c) Stephen Fryatt, 2003
 *
 *
 *
 */

#ifndef _ACCOUNTS_CHOICES
#define _ACCOUNTS_CHOICES

/* Pane numbers */

#define CHOICE_PANE_GENERAL 0
#define CHOICE_PANE_CURRENCY 1
#define CHOICE_PANE_TRANSACT 2
#define CHOICE_PANE_ACCOUNT 3
#define CHOICE_PANE_SORDER 4
#define CHOICE_PANE_PRINT 5
#define CHOICE_PANE_REPORT 6

/* Main window icons. */

#define CHOICE_ICON_APPLY 0
#define CHOICE_ICON_SAVE 1
#define CHOICE_ICON_CANCEL 2
#define CHOICE_ICON_DEFAULT 3
#define CHOICE_ICON_PANE 4
#define CHOICE_ICON_SELECT 5

/* General pane icons. */

#define CHOICE_ICON_CLIPBOARD 4
#define CHOICE_ICON_RO5KEYS 3
#define CHOICE_ICON_REMEMBERDIALOGUE 0
#define CHOICE_ICON_DATEIN 8
#define CHOICE_ICON_DATEOUT 10
#define CHOICE_ICON_TERRITORYDATE 11

/* Currency pane icons. */

#define CHOICE_ICON_SHOWZERO 0
#define CHOICE_ICON_TERRITORYNUM 1
#define CHOICE_ICON_FORMATFRAME 2
#define CHOICE_ICON_FORMATLABEL 3
#define CHOICE_ICON_DECIMALPLACELABEL 4
#define CHOICE_ICON_DECIMALPLACE 5
#define CHOICE_ICON_DECIMALPOINTLABEL 6
#define CHOICE_ICON_DECIMALPOINT 7
#define CHOICE_ICON_NEGFRAME 8
#define CHOICE_ICON_NEGLABEL 9
#define CHOICE_ICON_NEGMINUS 10
#define CHOICE_ICON_NEGBRACE 11

/* Standing order pane icons. */

#define CHOICE_ICON_SORTAFTERSO 0
#define CHOICE_ICON_TERRITORYSO 1
#define CHOICE_ICON_WEEKENDFRAME 2
#define CHOICE_ICON_WEEKENDLABEL 3
#define CHOICE_ICON_SOSUN 4
#define CHOICE_ICON_SOMON 5
#define CHOICE_ICON_SOTUE 6
#define CHOICE_ICON_SOWED 7
#define CHOICE_ICON_SOTHU 8
#define CHOICE_ICON_SOFRI 9
#define CHOICE_ICON_SOSAT 10
#define CHOICE_ICON_AUTOSORTSO 11

/* Print pane icons. */

#define CHOICE_ICON_STANDARD 0
#define CHOICE_ICON_PORTRAIT 1
#define CHOICE_ICON_LANDSCAPE 2
#define CHOICE_ICON_SCALE 3
#define CHOICE_ICON_MTOP 7
#define CHOICE_ICON_MLEFT 8
#define CHOICE_ICON_MRIGHT 11
#define CHOICE_ICON_MBOTTOM 13
#define CHOICE_ICON_MINCH 14
#define CHOICE_ICON_MCM 15
#define CHOICE_ICON_MMM 16
#define CHOICE_ICON_FASTTEXT 17
#define CHOICE_ICON_TEXTFORMAT 18

/* Report pane icons. */

#define CHOICE_ICON_NFONT 3
#define CHOICE_ICON_NFONTMENU 4
#define CHOICE_ICON_BFONT 6
#define CHOICE_ICON_BFONTMENU 7
#define CHOICE_ICON_FONTSIZE 9
#define CHOICE_ICON_FONTSPACE 12

/* Transaction pane icons. */

#define CHOICE_ICON_AUTOSORT 0
#define CHOICE_ICON_HIGHLIGHT 3
#define CHOICE_ICON_HILIGHTCOL 6
#define CHOICE_ICON_HILIGHTMEN 4
#define CHOICE_ICON_TRANSDEL 8
#define CHOICE_ICON_AUTOCOMP 11
#define CHOICE_ICON_AUTOSORTPRE 9

/* Account pane icons. */

#define CHOICE_ICON_AHIGHLIGHT 2
#define CHOICE_ICON_AHILIGHTCOL 5
#define CHOICE_ICON_AHILIGHTMEN 3
#define CHOICE_ICON_SHIGHLIGHT 9
#define CHOICE_ICON_SHILIGHTCOL 13
#define CHOICE_ICON_SHILIGHTMEN 11
#define CHOICE_ICON_OHIGHLIGHT 14
#define CHOICE_ICON_OHILIGHTCOL 18
#define CHOICE_ICON_OHILIGHTMEN 16

/* unit conversion constants. */

#define UNIT_INCH_TO_MILLIPOINT 72000
#define UNIT_MM_TO_MILLIPOINT 2834
#define UNIT_CM_TO_MILLIPOINT 28346

#define MARGIN_UNIT_MM 0
#define MARGIN_UNIT_CM 1
#define MARGIN_UNIT_INCH 2

/* ------------------------------------------------------------------------------------------------------------------ */

void open_choices_window (wimp_pointer *pointer);
void close_choices_window (void);
void change_choices_pane (int pane);

void set_choices_window (void);
void read_choices_window (void);

void redraw_choices_window (void);

#endif
