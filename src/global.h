/* CashBook - global.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_GLOBAL
#define _ACCOUNTS_GLOBAL

#include "oslib/wimp.h"

/* ==================================================================================================================
 * Templates and resources.
 */

#define IND_DATA_SIZE 8000
#define MAX_LINE_LENGTH 1024

#define COLUMN_GUTTER 4
#define LINE_GUTTER 4
#define ICON_HEIGHT 36
#define HORIZONTAL_SCROLL 16
#define AUTO_SCROLL_MARGIN 20
#define COLUMN_HEADING_MARGIN 2
#define COLUMN_SORT_OFFSET 8

#define ACCOUNT_NAME_LEN 32
#define ACCOUNT_IDENT_LEN 5
#define ACCOUNT_NO_LEN 32
#define ACCOUNT_SRTCD_LEN 15
#define ACCOUNT_ADDR_LEN 64

#define ACCOUNT_ADDR_LINES 4

#define PRESET_NAME_LEN 32

/* The Description field below must be longer than the Refererence.*/

#define DATE_FIELD_LEN 11
#define REC_FIELD_LEN 2
#define REF_FIELD_LEN 13
#define AMOUNT_FIELD_LEN 15
#define DESCRIPT_FIELD_LEN 101

#define NULL_ACCOUNT (-1)
#define NULL_TRANS_FLAGS 0
#define NULL_DATE (date_t) 0xffffffff
#define NULL_CURRENCY 0
#define NULL_SORDER (-1)
#define NULL_PRESET (-1)
#define NULL_TRANSACTION (-1)
#define NULL_TEMPLATE (-1)

#define MIN_DATE 0x00640101
#define MAX_DATE 0x270f0c1f

/* Transaction window details */

#define TRANSACT_COLUMNS 10
#define TRANSACT_TOOLBAR_HEIGHT 132

#define MIN_TRANSACT_ENTRIES 10
#define MIN_TRANSACT_BLANK_LINES 1

#define TRANSACTION_WINDOW_OPEN_OFFSET 48
#define TRANSACTION_WINDOW_OFFSET_LIMIT 8

#define CHILD_WINDOW_OFFSET 12
#define CHILD_WINDOW_X_OFFSET 128
#define CHILD_WINDOW_X_OFFSET_LIMIT 4

/* Account window details */

#define ACCOUNT_WINDOWS 3

#define ACCOUNT_COLUMNS 6
#define ACCOUNT_NUM_COLUMNS 4
#define ACCOUNT_TOOLBAR_HEIGHT 132
#define ACCOUNT_FOOTER_HEIGHT 36

#define MIN_ACCOUNT_ENTRIES 10

#define ACCOUNT_SECTION_LEN 52

/* Account View window details */

#define ACCVIEW_COLUMNS 9
#define ACCVIEW_TOOLBAR_HEIGHT 132
#define MIN_ACCVIEW_ENTRIES 10

/* Standing order window details. */

#define SORDER_COLUMNS 10

#define SORDER_TOOLBAR_HEIGHT 132

#define MIN_SORDER_ENTRIES 10

/* Preset window details. */

#define PRESET_COLUMNS 10

#define PRESET_TOOLBAR_HEIGHT 132

#define MIN_PRESET_ENTRIES 10

/* Account types (bitwise allocation, to allow oring of values) */

#define ACCOUNT_NULL 0x0000 /* Unset account type */

#define ACCOUNT_FULL 0x0001 /* Bank account type */
#define ACCOUNT_IN   0x0100 /* Income enabled analysis header */
#define ACCOUNT_OUT  0x0200 /* Outgoing enabled analysis header */

/* Standing order_details */

#define PERIOD_NONE 0
#define PERIOD_DAYS 1
#define PERIOD_MONTHS 2
#define PERIOD_YEARS 3

/* Account window line types */

#define ACCOUNT_LINE_BLANK 0
#define ACCOUNT_LINE_DATA 1
#define ACCOUNT_LINE_HEADER 2
#define ACCOUNT_LINE_FOOTER 3

/* Transaction flags (bitwiase allocation) */

#define TRANS_REC_FROM      0x0001 /* Reconciled from */
#define TRANS_REC_TO        0x0002 /* Reconciled to */

                                   /* The following flags are used by standing orders. */

#define TRANS_SKIP_FORWARD  0x0100 /* Move forward to avoid illegal days. */
#define TRANS_SKIP_BACKWARD 0x0200 /* Move backwards to avoid illegal days. */

                                   /* The following flags are used by presets. */

#define TRANS_TAKE_TODAY    0x1000 /* Always take today's date */
#define TRANS_TAKE_CHEQUE   0x2000 /* Always take the next cheque number */

/* Find dialogue settings. */

#define FIND_AND  1
#define FIND_OR   2

#define FIND_NODIR 0
#define FIND_START 1
#define FIND_END   2
#define FIND_UP    3
#define FIND_DOWN  4
#define FIND_NEXT  5
#define FIND_PREVIOUS 6

/* Drag types. */

#define SAVE_DRAG    1
#define ACCOUNT_DRAG 2
#define COLUMN_DRAG  3

/* Report details. */

#define REPORT_TAB_STOPS 20
#define REPORT_TAB_BARS 5
#define MAX_REP_FONT_NAME 128

/* Report account flags (bitwise allocation) */

#define REPORT_FROM      0x0001
#define REPORT_TO        0x0002
#define REPORT_INCLUDE   0x0004

/* Report dialogues. */
/* SPEC_LEN is the length of the text fields; LIST_LEN is the number of accounts that can be stored. */

#define REPORT_ACC_SPEC_LEN 128
#define REPORT_ACC_LIST_LEN 64

/* Saved Report templates */

#define SAVED_REPORT_NAME_LEN 32

#define REPORT_TYPE_NONE     0
#define REPORT_TYPE_TRANS    1
#define REPORT_TYPE_UNREC    2
#define REPORT_TYPE_CASHFLOW 3
#define REPORT_TYPE_BALANCE  4

/* Sort types. Types are shered between windows wherever possible.
 *
 * Ascending sorts contain 0x100000; descending 0x200000.
 */

#define SORT_NONE        0

#define SORT_ASCENDING   0x10000
#define SORT_DESCENDING  0x20000

#define SORT_MASK        0x0ffff

#define SORT_DATE        0x00001
#define SORT_FROM        0x00002
#define SORT_TO          0x00003
#define SORT_REFERENCE   0x00004
#define SORT_DESCRIPTION 0x00005

#define SORT_AMOUNT      0x00010

#define SORT_FROMTO      0x00100
#define SORT_PAYMENTS    0x00200
#define SORT_RECEIPTS    0x00300
#define SORT_BALANCE     0x00400

#define SORT_NEXTDATE    0x01000
#define SORT_LEFT        0x02000

#define SORT_CHAR        0x03000
#define SORT_NAME        0x04000

/* ==================================================================================================================
 * Basic types
 */

/* None of these are fully used as yet... */

typedef unsigned int date_t; /* A date */
typedef int          amt_t;  /* An amount. */
typedef unsigned int acct_t; /* An account number. */


/* \TODO -- These need to move into their modules once we've sorted the rest
 *          of this mess out.
 */

struct sorder;
struct preset;


/* ==================================================================================================================
 * Initial window data structures
 */

typedef struct accview_redraw
{
  int transaction; /* Pointer to the transaction entry. */
  int balance;     /* Running balance at this point. */

  /* Sort index entries.
   *
   * NB - These are unconnected to the rest of the redraw data, and are in effect a separate array that is used
   * for handling entries in the account view window.
   */

  int sort_index;  /* Point to another line, to allow the window to be sorted. */
}
accview_redraw;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Account view window data struct */

typedef struct accview_window
{
  /* Account window handle and title details. */

  wimp_w           accview_window;      /* Window handle of the account window */
  char             window_title[256];
  wimp_w           accview_pane;        /* Window handle of the account window toolbar pane */

  /* Display column details. */

  int              column_width[ACCVIEW_COLUMNS]; /* Array holding the column widths in the account window. */
  int              column_position[ACCVIEW_COLUMNS]; /* Array holding the column X-offsets in the acct window */

  /* Data parameters */

  int              display_lines;       /* Count of the lines in the window */
  accview_redraw   *line_data;          /* Pointer to array of line data for the redraw */

  int              sort_order;

  char             sort_sprite[12];    /* Space for the sort icon's indirected data. */
}
accview_window;


/* ==================================================================================================================
 * File data structures.
 */

/* Transatcion data struct. */

typedef struct transaction
{
  unsigned date;
  unsigned flags;
  int      from;
  int      to;
  int      amount;
  char     reference[REF_FIELD_LEN];
  char     description[DESCRIPT_FIELD_LEN];

  /* Sort index entries.
   *
   * NB - These are unconnected to the rest of the transaction data, and are in effect a separate array that is used
   * for handling entries in the transaction window.
   */

  int      sort_index;       /* Point to another transaction, to allow the transaction window to be sorted. */
  int      saved_sort;       /* Preserve the transaction window sort order across transaction data sorts. */
  int      sort_workspace;   /* Workspace used by the sorting code. */
}
transaction;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Account data struct. */

typedef struct account
{
  char     name[ACCOUNT_NAME_LEN];
  char     ident[ACCOUNT_IDENT_LEN];

  char     account_no[ACCOUNT_NO_LEN];
  char     sort_code[ACCOUNT_SRTCD_LEN];
  char     address[ACCOUNT_ADDR_LINES][ACCOUNT_ADDR_LEN];

  int      type;
  unsigned category;

  /* Pointer to account view window data. */

  accview_window *account_view;

  /* Cheque tracking data. */

  unsigned next_payin_num;
  int      payin_num_width;

  unsigned next_cheque_num;
  int      cheque_num_width;

  /* User-set values used for calculation. */

  int opening_balance;     /* The opening balance for accounts, from which everything else is calculated. */
  int credit_limit;        /* Credit limit for accounts. */
  int budget_amount;       /* Budgeted amount for headings. */

  /* Calculated values for both accounts and headings. */

  int statement_balance;   /* Reconciled statement balance. */
  int current_balance;     /* Balance up to today's date. */
  int future_balance;      /* Balance including all transactions. */
  int budget_balance;      /* Balance including all transactions betwen budget dates. */

  int sorder_trial;        /* Difference applied to account from standing orders in trial period. */

  /* Subsequent calculated values for accounts. */

  int trial_balance;       /* Balance including all transactions & standing order trial. */
  int available_balance;   /* Balance available, taking into account credit limit. */

  /* Subsequent calculated values for headings. */

  int budget_result;

  /* Values used by reports. */

  int      report_total;
  int      report_balance;
  unsigned report_flags;
}
account;

/* ==================================================================================================================
 * Window redraw data structures
 */

/* Account window line redraw data struct */

typedef struct account_redraw
{
  int      type;                         /* Type of line (account, header, footer, blank, etc) */

  int      account;                      /* Number of account. */
  int      total[ACCOUNT_COLUMNS-2];     /* Balance totals for section. */
  char     heading[ACCOUNT_SECTION_LEN]; /* Heading for section. */
}
account_redraw;

/* ==================================================================================================================
 * Window data structures
 */

/* Account window data struct */

typedef struct account_window
{
  /* Account window handle and title details. */

  wimp_w           account_window;      /* Window handle of the account window */
  char             window_title[256];
  wimp_w           account_pane;        /* Window handle of the account window toolbar pane */
  wimp_w           account_footer;      /* Window handle of the account window footer pane */
  char             footer_icon[ACCOUNT_COLUMNS-2][AMOUNT_FIELD_LEN]; /* Indirected blocks for footer icons. */

  /* Display column details. */

  int              column_width[ACCOUNT_COLUMNS]; /* Array holding the column widths in the account window. */
  int              column_position[ACCOUNT_COLUMNS]; /* Array holding the column X-offsets in the acct window */

  /* Data parameters */

  int              type;                /* Type of accounts contained within the window */

  int              display_lines;       /* Count of the lines in the window */
  account_redraw   *line_data;          /* Pointer to array of line data for the redraw */
}
account_window;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Transaction window data struct */

typedef struct transaction_window
{
  /* Transactcion window handle and title details. */

  wimp_w           transaction_window;  /* Window handle of the transaction window */
  char             window_title[256];
  wimp_w           transaction_pane;    /* Window handle of the transaction window toolbar pane */

  /* Display column details. */

  int              column_width[TRANSACT_COLUMNS];  /* Array holding the column widths in the transaction window. */
  int              column_position[TRANSACT_COLUMNS]; /* Array holding the column X-offsets in the transact window. */

  /* Other window information. */

  int              display_lines;      /* How many lines the current work area is formatted to display. */
  int              entry_line;         /* The line currently marked for data entry, in terms of window lines. */
  int              sort_order;         /* The order in which the window is sorted. */

  char             sort_sprite[12];    /* Space for the sort icon's indirected data. */
}
transaction_window;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Standing order window data struct */

typedef struct sorder_window
{
  /* Transactcion window handle and title details. */

  wimp_w           sorder_window;  /* Window handle of the standing order window */
  char             window_title[256];
  wimp_w           sorder_pane;    /* Window handle of the standing order window toolbar pane */

  /* Display column details. */

  int              column_width[SORDER_COLUMNS];  /* Array holding the column widths in the transaction window. */
  int              column_position[SORDER_COLUMNS]; /* Array holding the column X-offsets in the transact window. */

  /* Other window details. */

  int              sort_order;         /* The order in which the window is sorted. */

  char             sort_sprite[12];    /* Space for the sort icon's indirected data. */
}
sorder_window;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Preset window data struct */

typedef struct preset_window
{
  /* Preset window handle and title details. */

  wimp_w           preset_window;  /* Window handle of the preset window */
  char             window_title[256];
  wimp_w           preset_pane;    /* Window handle of the preset window toolbar pane */

  /* Display column details. */

  int              column_width[PRESET_COLUMNS];  /* Array holding the column widths in the transaction window. */
  int              column_position[PRESET_COLUMNS]; /* Array holding the column X-offsets in the transact window. */

  /* Other window details. */

  int              sort_order;         /* The order in which the window is sorted. */

  char             sort_sprite[12];    /* Space for the sort icon's indirected data. */
}
preset_window;

/* ==================================================================================================================
 * Budgeting data
 */

/* Budget control data. */

typedef struct budget
{
  /* Budget date limits */

  unsigned start;
  unsigned finish;

  /* Standing order trail limits. */

  int sorder_trial;
  int limit_postdate;
}
budget;

/* ==================================================================================================================
 * Dialogue content data structures
 */

/* Goto dialogue data. */

typedef struct go_to
{
  unsigned data;
  int      data_type;
}
go_to;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find dialogue data. */

typedef struct find
{
  unsigned date;
  unsigned from;
  unsigned from_rec;
  unsigned to;
  unsigned to_rec;
  unsigned amount;
  char     ref[REF_FIELD_LEN];
  char     desc[DESCRIPT_FIELD_LEN];

  int      logic;
  int      case_sensitive;
  int      whole_text;
  int      direction;
}
find;

/* ----------------------------------------------------------------------------------------------------------------- */

/* Print dialogue. */

typedef struct print {
	osbool		fit_width;						/**< TRUE to fit width in graphics mode; FALSE to print 100%.					*/
	osbool		page_numbers;						/**< TRUE to print page numbers; FALSE to omit them.						*/
	osbool		rotate;							/**< TRUE to rotate 90 degrees in graphics mode to print Landscape; FALSE to print Portrait.	*/
	osbool		text;							/**< TRUE to print in text mode; FALSE to print in graphics mode.				*/
	osbool		text_format;						/**< TRUE to print with styles in text mode; FALSE to print plain text.				*/

	date_t		from;							/**< The date to print from in ranged prints (Advanced Print dialogue only).			*/
	date_t		to;							/**< The date to print to in ranged prints (Advanced Print dialogue only).			*/
} print;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Continuation dialogue. */

typedef struct continuation
{
  int          transactions;
  int          accounts;
  int          headings;
  int          sorders;

  date_t       before;
}
continuation;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Transaction Report dialogue. */

typedef struct trans_rep
{
  date_t       date_from;
  date_t       date_to;
  int          budget;

  int          group;
  int          period;
  int          period_unit;
  int          lock;

  int          from_count, to_count;
  acct_t       from[REPORT_ACC_LIST_LEN];
  acct_t       to[REPORT_ACC_LIST_LEN];
  char         ref[REF_FIELD_LEN];
  char         desc[DESCRIPT_FIELD_LEN];
  amt_t        amount_min;
  amt_t        amount_max;

  int          output_trans;
  int          output_summary;
  int          output_accsummary;
}
trans_rep;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Unreconciled Report dialogue. */

typedef struct unrec_rep
{
  date_t       date_from;
  date_t       date_to;
  int          budget;

  int          group;
  int          period;
  int          period_unit;
  int          lock;

  int          from_count, to_count;
  acct_t       from[REPORT_ACC_LIST_LEN];
  acct_t       to[REPORT_ACC_LIST_LEN];
}
unrec_rep;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Cashflow Report dialogue. */

typedef struct cashflow_rep
{
  date_t       date_from;
  date_t       date_to;
  int          budget;

  int          group;
  int          period;
  int          period_unit;
  int          lock;
  int          empty;

  int          accounts_count, incoming_count, outgoing_count;
  acct_t       accounts[REPORT_ACC_LIST_LEN];
  acct_t       incoming[REPORT_ACC_LIST_LEN];
  acct_t       outgoing[REPORT_ACC_LIST_LEN];

  int          tabular;
}
cashflow_rep;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Balance Report dialogue. */

typedef struct balance_rep
{
  date_t       date_from;
  date_t       date_to;
  int          budget;

  int          group;
  int          period;
  int          period_unit;
  int          lock;

  int          accounts_count, incoming_count, outgoing_count;
  acct_t       accounts[REPORT_ACC_LIST_LEN];
  acct_t       incoming[REPORT_ACC_LIST_LEN];
  acct_t       outgoing[REPORT_ACC_LIST_LEN];

  int          tabular;
}
balance_rep;

/* ==================================================================================================================
 * Saved Report structures
 */

typedef union report_block
{
  trans_rep    transaction;
  unrec_rep    unreconciled;
  cashflow_rep cashflow;
  balance_rep  balance;
}
report_block;


typedef struct saved_report
{
  char         name[SAVED_REPORT_NAME_LEN];
  int          type;

  report_block data;
}
saved_report;

/* ==================================================================================================================
 * Report data structures
 */

typedef struct file_data file_data;

typedef struct report_data
{
  file_data		*file;							/**< The file that the report belongs to.		*/

  wimp_w             window;
  char               window_title[256];

  /* Report status flags. */

  int                flags;
  int                print_pending;

  /* Tab details */

  int                font_width[REPORT_TAB_BARS][REPORT_TAB_STOPS]; /* Column widths in OS units for outline fonts. */
  int                text_width[REPORT_TAB_BARS][REPORT_TAB_STOPS]; /* Column widths in characters for ASCII text. */

  int                font_tab[REPORT_TAB_BARS][REPORT_TAB_STOPS]; /* Tab stops in OS units for outline fonts. */

  /* Font data */

  char               font_normal[MAX_REP_FONT_NAME]; /* Name of 'normal' outline font. */
  char               font_bold[MAX_REP_FONT_NAME];   /* Name of bold outline font. */
  int                font_size;                      /* Font size in 1/16 points. */
  int                line_spacing;                   /* Line spacing in percent. */

  /* Report content */

  int                lines;      /* The number of lines in the report. */
  int                max_lines;  /* The size of the line pointer block. */

  int                width;      /* The displayed width of the report data. */
  int                height;     /* The displayed height of the report data. */

  int                block_size; /* The size of the data block */
  int                data_size;  /* The size of the data in the block. */

  char               *data;      /* The data block itself (flex block). */
  int                *line_ptr;  /* The line pointer block (flex block). */

  /* Report template details. */

  saved_report       template;

  struct report_data *next;
}
report_data;

/* ==================================================================================================================
 * Main file data structure
 */

/* File data struct. */

struct file_data
{
  /* File location */

  char               filename[256];      /* The filename on disc; if null, it hasn't been saved before */
  os_date_and_time   datestamp;          /* The datestamp of the file. */

  /* Details of the attached windows. */

  transaction_window transaction_window; /* Structure holding transaction window information */
  account_window     account_windows[ACCOUNT_WINDOWS]; /* Array holding account window information */
  sorder_window      sorder_window;      /* Structure holding standing order window information. */
  preset_window      preset_window;      /* Structure holding preset window information. */

  /* Default display details for the accview windows. */

  int                accview_column_width[ACCVIEW_COLUMNS]; /* Base column widths in the account view windows. */
  int                accview_column_position[ACCVIEW_COLUMNS]; /* Base column positions in the account view windows. */
  int                accview_sort_order; /* Default sort order for the accview windows. */

  /* Array size counts. */

  int                account_count;      /* The number of accounts defined in the file. */
  int                trans_count;        /* The number of transactions defined in the file. */
  int                sorder_count;       /* The number of standing orders defined in the file. */
  int                preset_count;       /* The number of presets defined in the file. */

  /* Account, transaction, standing order and preset data structures (pointers which become arrays). */

  account            *accounts;
  transaction        *transactions;
  struct sorder      *sorders;
  struct preset      *presets;

  /* Recalculation data. */

  unsigned           last_full_recalc;   /* The last time a full recalculation was done on the file. */
                                         /* Not sure if this is actually used! */
  /* Budget data. */

  budget             budget;

  /* Data integrity. */

  int                modified;           /* Flag to show if the file has been modified since the last save. */
  int                sort_valid;         /* Flag to show that the transaction data is sorted OK. */
  int                untitled_count;     /* Count to allow default title of the form <Untitled n>. */
  int                child_x_offset;     /* Count for child window opening offset. */
  int                auto_reconcile;     /* Flag to show if reconcile should jump to the next unreconcliled entry. */

  /* Report data structure */

  report_data        *reports;           /* Pointer to a linked list of report structures. */

  report_data        *import_report;     /* The current import log report. */

  /* Report templates */

  saved_report       *saved_reports;     /* A pointer to an array of saved report templates. */
  int                saved_report_count; /* A count of how many reports are in the file. */

  /* Dialogue content. */

  go_to              go_to;
  find               find;
  print              print;
  continuation       continuation;
  trans_rep          trans_rep;
  unrec_rep          unrec_rep;
  cashflow_rep       cashflow_rep;
  balance_rep        balance_rep;

  struct file_data   *next;
};

/* ==================================================================================================================
 * Window and menu handle structures.
 */

/* Windows */

typedef struct
{
  wimp_w      file_info;
  wimp_w      save_as;
  wimp_w      import_comp;
  wimp_w      enter_acc;
  wimp_w      edit_acct;
  wimp_w      edit_hdr;
  wimp_w      edit_sect;
//  wimp_w      edit_sorder;
  wimp_w      sort_trans;
  wimp_w      sort_accview;
//  wimp_w      sort_sorder;

  wimp_window *transaction_window_def;
  wimp_window *transaction_pane_def;
  wimp_window *account_window_def;
  wimp_window *account_pane_def[2];
  wimp_window *account_footer_def;
  wimp_window *accview_window_def;
  wimp_window *accview_pane_def;
}
global_windows;

/* ------------------------------------------------------------------------------------------------------------------ */

/* Menus */

typedef struct
{
  wimp_menu   *menu_up;
  int         menu_id;

  wimp_menu   *main;
  wimp_menu   *account_sub;
  wimp_menu   *transaction_sub;
  wimp_menu   *analysis_sub;

  wimp_menu   *accopen;

  wimp_menu   *date;
  wimp_menu   *account;
  wimp_menu   *refdesc;

  wimp_menu   *acclist;
  wimp_menu   *accview;
}
global_menus;

#endif
