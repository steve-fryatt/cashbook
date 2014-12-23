/* Copyright 2003-2014, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: global.h
 *
 * Legacy global datastructure support.
 */

#ifndef CASHBOOK_GLOBAL
#define CASHBOOK_GLOBAL

#include "oslib/wimp.h"

/* ==================================================================================================================
 * Templates and resources.
 */

#define FILE_MAX_FILENAME 256

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

#define TRANSACT_COLUMNS 11
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

#define ACCVIEW_COLUMNS 10
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

enum account_type {
	ACCOUNT_NULL = 0x0000,							/**< Unset account type.				*/
	ACCOUNT_FULL = 0x0001,							/**< Bank account type.					*/
	ACCOUNT_IN   = 0x0100,							/**< Income enabled analysis header.			*/
	ACCOUNT_OUT  = 0x0200							/**< Outgoing enabled analysis header.			*/
};

/* Standing order_details */

enum date_period {
	PERIOD_NONE = 0,							/**< No period specified.				*/
	PERIOD_DAYS,								/**< Period specified in days.				*/
	PERIOD_MONTHS,								/**< Period specified in months.			*/
	PERIOD_YEARS								/**< Period specified in years.				*/
};

/* Account window line types */

enum account_line_type {
	ACCOUNT_LINE_BLANK = 0,							/**< Blank, unset line type.				*/
	ACCOUNT_LINE_DATA,							/**< Data line type.					*/
	ACCOUNT_LINE_HEADER,							/**< Section Heading line type.				*/
	ACCOUNT_LINE_FOOTER							/**< Section Footer line type.				*/
};

/* Transaction flags (bitwiase allocation) */

enum transact_flags {
	TRANS_FLAGS_NONE = 0,

	TRANS_REC_FROM = 0x0001,						/**< Reconciled from					*/
	TRANS_REC_TO = 0x0002,							/**< Reconciled to					*/

	/* The following flags are used by standing orders. */

	TRANS_SKIP_FORWARD = 0x0100,						/**< Move forward to avoid illegal days.		*/
	TRANS_SKIP_BACKWARD = 0x0200,						/**< Move backwards to avoid illegal days.		*/

	/* The following flags are used by presets. */

	TRANS_TAKE_TODAY = 0x1000,						/**< Always take today's date				*/
	TRANS_TAKE_CHEQUE = 0x2000						/**< Always take the next cheque number			*/
};


/* Report details. */

#define REPORT_TAB_STOPS 20
#define REPORT_TAB_BARS 5
#define MAX_REP_FONT_NAME 128

/* Report dialogues. */
/* SPEC_LEN is the length of the text fields; LIST_LEN is the number of accounts that can be stored. */

#define REPORT_ACC_SPEC_LEN 128
#define REPORT_ACC_LIST_LEN 64

/* Saved Report templates */

#define SAVED_REPORT_NAME_LEN 32

enum report_type {
	REPORT_TYPE_NONE = 0,							/**< No report.						*/
	REPORT_TYPE_TRANS = 1,							/**< Transaction report.				*/
	REPORT_TYPE_UNREC = 2,							/**< Unreconciled transaction report.			*/
	REPORT_TYPE_CASHFLOW = 3,						/**< Cashflow report.					*/
	REPORT_TYPE_BALANCE = 4							/**< Balance report.					*/
};

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
#define SORT_ROW         0x00006

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

typedef unsigned int	date_t;							/**< A date.							*/
typedef int		amt_t;							/**< An amount.							*/
typedef int		acct_t;							/**< An account number.						*/


/* \TODO -- These need to move into their modules once we've sorted the rest
 *          of this mess out.
 */

struct account;
struct sorder;
struct preset;
struct report;

struct accview_window;
struct account_redraw;

typedef struct file_data file_data;


/* ==================================================================================================================
 * File data structures.
 */

/* Transatcion data struct. */

struct transaction {
	date_t			date;
	enum transact_flags	flags;
	acct_t			from;
	acct_t			to;
	amt_t			amount;
	char			reference[REF_FIELD_LEN];
	char			description[DESCRIPT_FIELD_LEN];

	/* Sort index entries.
	 *
	 * NB - These are unconnected to the rest of the transaction data, and are in effect a separate array that is used
	 * for handling entries in the transaction window.
	 */

	int			sort_index;		/**< Point to another transaction, to allow the transaction window to be sorted. */
	int			saved_sort;		/**< Preserve the transaction window sort order across transaction data sorts. */
	int			sort_workspace;		/**< Workspace used by the sorting code. */
};

/* ==================================================================================================================
 * Window redraw data structures
 */

/* Account window line redraw data struct */

struct account_redraw {
	enum account_line_type	type;						/* Type of line (account, header, footer, blank, etc) */

	acct_t			account;					/* Number of account. */
	amt_t			total[ACCOUNT_COLUMNS-2];			/* Balance totals for section. */
	char			heading[ACCOUNT_SECTION_LEN];			/* Heading for section. */
};

/* ==================================================================================================================
 * Window data structures
 */

/* Account window data struct */

struct account_window {
	file_data		*file;						/**< The file owning the block (for reverse lookup).		*/
	int			entry;						/**< The array index of the block (for reverse lookup).		*/

	/* Account window handle and title details. */

	wimp_w			account_window;					/* Window handle of the account window */
	char			window_title[256];
	wimp_w			account_pane;					/* Window handle of the account window toolbar pane */
	wimp_w			account_footer;					/* Window handle of the account window footer pane */
	char			footer_icon[ACCOUNT_COLUMNS-2][AMOUNT_FIELD_LEN]; /* Indirected blocks for footer icons. */

	/* Display column details. */

	int			column_width[ACCOUNT_COLUMNS];			/* Array holding the column widths in the account window. */
	int			column_position[ACCOUNT_COLUMNS];		/* Array holding the column X-offsets in the acct window */

	/* Data parameters */

	enum account_type	type;						/* Type of accounts contained within the window */

	int			display_lines;					/* Count of the lines in the window */
	struct account_redraw	*line_data;					/* Pointer to array of line data for the redraw */
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Transaction window data struct */

struct transaction_window {
	file_data	*file;							/**< The file to which the window belongs.			*/

	/* Transactcion window handle and title details. */

	wimp_w		transaction_window;					/**< Window handle of the transaction window */
	char		window_title[256];
	wimp_w		transaction_pane;					/**< Window handle of the transaction window toolbar pane */

	/* Display column details. */

	int		column_width[TRANSACT_COLUMNS];				/**< Array holding the column widths in the transaction window. */
	int		column_position[TRANSACT_COLUMNS];			/**< Array holding the column X-offsets in the transact window. */

	/* Other window information. */

	int		display_lines;						/**< How many lines the current work area is formatted to display. */
	int		entry_line;						/**< The line currently marked for data entry, in terms of window lines. */
	int		sort_order;						/**< The order in which the window is sorted. */

	char		sort_sprite[12];					/**< Space for the sort icon's indirected data. */
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Standing order window data struct */

struct sorder_window {
	file_data	*file;							/**< The file to which the window belongs.			*/

	/* Transactcion window handle and title details. */

	wimp_w		sorder_window;						/**< Window handle of the standing order window.		*/
	char		window_title[256];
	wimp_w		sorder_pane;						/**< Window handle of the standing order window toolbar pane.	*/

	/* Display column details. */

	int		column_width[SORDER_COLUMNS];				/**< Array holding the column widths in the transaction window.	*/
	int		column_position[SORDER_COLUMNS];			/**< Array holding the column X-offsets in the transact window.	*/

	/* Other window details. */

	int		sort_order;						/**< The order in which the window is sorted.			*/

	char		sort_sprite[12];					/**< Space for the sort icon's indirected data.			*/
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Preset window data struct */

struct preset_window {
	file_data	*file;							/**< The file to which the window belongs.			*/

	/* Preset window handle and title details. */

	wimp_w		preset_window;						/**< Window handle of the preset window.			*/
	char		window_title[256];
	wimp_w		preset_pane;						/**< Window handle of the preset window toolbar pane.		*/

	/* Display column details. */

	int		column_width[PRESET_COLUMNS];				/**< Array holding the column widths in the transaction window.	*/
	int		column_position[PRESET_COLUMNS];			/**< Array holding the column X-offsets in the transact window.	*/

	/* Other window details. */

	int		sort_order;						/**< The order in which the window is sorted.			*/

	char		sort_sprite[12];					/**< Space for the sort icon's indirected data.			*/
};

/* ==================================================================================================================
 * Budgeting data
 */

/* Budget control data. */

struct budget {
	/* Budget date limits */

	date_t		start;							/**< The start date of the budget.					*/
	date_t		finish;							/**< The finish date of the budget.					*/

	/* Standing order trail limits. */

	int		sorder_trial;						/**< The number of days ahead to trial standing orders.			*/
	osbool		limit_postdate;						/**< TRUE to limit post-dated transactions to the SO trial period.	*/
};

/* ==================================================================================================================
 * Dialogue content data structures
 */

/* Goto dialogue data. */

union go_to_target {
	int			line;						/**< The transaction number if the tratget is a line.		*/
	date_t			date;						/**< The transaction date if the target is a date.		*/
};

enum go_to_type {
	GOTO_TYPE_LINE = 0,							/**< The goto target is given as a line number.			*/
	GOTO_TYPE_DATE = 1							/**< The goto target is given as a date.			*/
};

struct go_to{
	union go_to_target	target;						/**< The current goto target.					*/
	enum go_to_type		type;						/**< The type of target we're aiming for.			*/
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Find dialogue data. */

enum find_logic {
	FIND_AND = 1,
	FIND_OR = 2
};

enum find_direction {
	FIND_NODIR = 0,
	FIND_START = 1,
	FIND_END = 2,
	FIND_UP = 3,
	FIND_DOWN = 4,
	FIND_NEXT = 5,
	FIND_PREVIOUS = 6
};

struct find {
	date_t			date;
	acct_t			from;
  unsigned from_rec;
	acct_t			to;
  unsigned to_rec;
	amt_t			amount;
	char			ref[REF_FIELD_LEN];
	char			desc[DESCRIPT_FIELD_LEN];

	enum find_logic		logic;
	osbool			case_sensitive;
	osbool			whole_text;
	enum find_direction	direction;
};

/* ----------------------------------------------------------------------------------------------------------------- */

/* Print dialogue. */

struct printing {
	osbool		fit_width;						/**< TRUE to fit width in graphics mode; FALSE to print 100%.					*/
	osbool		page_numbers;						/**< TRUE to print page numbers; FALSE to omit them.						*/
	osbool		rotate;							/**< TRUE to rotate 90 degrees in graphics mode to print Landscape; FALSE to print Portrait.	*/
	osbool		text;							/**< TRUE to print in text mode; FALSE to print in graphics mode.				*/
	osbool		text_format;						/**< TRUE to print with styles in text mode; FALSE to print plain text.				*/

	date_t		from;							/**< The date to print from in ranged prints (Advanced Print dialogue only).			*/
	date_t		to;							/**< The date to print to in ranged prints (Advanced Print dialogue only).			*/
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Continuation dialogue. */

struct purge {
	osbool		transactions;						/**< TRUE to remove reconciled transactions, subject to the before constraint; else FALSE.	*/
	osbool		accounts;						/**< TRUE to remove unused accounts; else FALSE.						*/
	osbool		headings;						/**< TRUE to remove unused headings; else FALSE.						*/
	osbool		sorders;						/**< TRUE to remove completed standing orders; else FALSE.					*/

	date_t		before;							/**< The date before which reconciled transactions should be removed; NULL_DATE for none.	*/
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Transaction Report dialogue. */

struct trans_rep {
	date_t			date_from;
	date_t			date_to;
  int          budget;

  int          group;
	int          period;
	enum date_period	period_unit;
  int          lock;

	int			from_count;
	int			to_count;
	acct_t			from[REPORT_ACC_LIST_LEN];
	acct_t			to[REPORT_ACC_LIST_LEN];
	char			ref[REF_FIELD_LEN];
	char			desc[DESCRIPT_FIELD_LEN];
	amt_t			amount_min;
	amt_t			amount_max;

  int          output_trans;
  int          output_summary;
  int          output_accsummary;
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Unreconciled Report dialogue. */

struct unrec_rep {
	date_t			date_from;
	date_t			date_to;
  int          budget;

  int          group;
	int			period;
	enum date_period	period_unit;
  int          lock;

	int			from_count;
	int			to_count;
	acct_t			from[REPORT_ACC_LIST_LEN];
	acct_t			to[REPORT_ACC_LIST_LEN];
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Cashflow Report dialogue. */

struct cashflow_rep {
	date_t			date_from;
	date_t			date_to;
  int          budget;

  int          group;
	int			period;
	enum date_period	period_unit;
  int          lock;
  int          empty;

	int			accounts_count;
	int			incoming_count;
	int			outgoing_count;
	acct_t			accounts[REPORT_ACC_LIST_LEN];
	acct_t			incoming[REPORT_ACC_LIST_LEN];
	acct_t			outgoing[REPORT_ACC_LIST_LEN];

  int          tabular;
};

/* ------------------------------------------------------------------------------------------------------------------ */

/* Balance Report dialogue. */

struct balance_rep {
	date_t			date_from;
	date_t			date_to;
  int          budget;

  int          group;
	int			period;
	enum date_period	period_unit;
  int          lock;

	int			accounts_count;
	int			incoming_count;
	int			outgoing_count;
	acct_t			accounts[REPORT_ACC_LIST_LEN];
	acct_t			incoming[REPORT_ACC_LIST_LEN];
	acct_t			outgoing[REPORT_ACC_LIST_LEN];

  int          tabular;
};

/* ==================================================================================================================
 * Main file data structure
 */

/* File data struct. */

struct file_data
{
	/* File location */

	char				filename[FILE_MAX_FILENAME];		/**< The filename on disc; if "", it hasn't been saved before.	*/
	os_date_and_time		datestamp;				/**< The datestamp of when the file was last saved.		*/

	/* Details of the attached windows. */

	struct transaction_window	transaction_window;			/**< Structure holding transaction window information.		*/
	struct account_window		account_windows[ACCOUNT_WINDOWS];	/**< Array holding account window information.			*/
	struct sorder_window		sorder_window;				/**< Structure holding standing order window information.	*/
	struct preset_window		preset_window;				/**< Structure holding preset window information.		*/

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

  struct account     *accounts;
  struct transaction *transactions;
  struct sorder      *sorders;
  struct preset      *presets;

  /* Recalculation data. */

	date_t				last_full_recalc;			/* The last time a full recalculation was done on the file.	*/

	/* Budget data. */

	struct budget			budget;					/**< The file's budgeting details.				*/

  /* Data integrity. */

  osbool             modified;           /* Flag to show if the file has been modified since the last save. */
  osbool             sort_valid;         /* Flag to show that the transaction data is sorted OK. */
  int                untitled_count;     /* Count to allow default title of the form <Untitled n>. */
  int                child_x_offset;     /* Count for child window opening offset. */
  osbool             auto_reconcile;     /* Flag to show if reconcile should jump to the next unreconcliled entry. */

  /* Report data structure */

	struct report			*reports;				/**< Pointer to a linked list of report structures.		*/

	/* Imports */

	struct report			*import_report;				/**< The current import log report (\TODO -- Import struct?).	*/

  /* Report templates */

  struct analysis_report *saved_reports;     /* A pointer to an array of saved report templates. */
  int                 saved_report_count; /* A count of how many reports are in the file. */

	/* Dialogue content. */

	struct go_to			go_to;					/**< Data relating to the goto module.				*/
	struct find			find;					/**< Data relating to the find module.				*/
	struct printing			print;					/**< Data relating to the print dialogues.			*/
	struct purge			purge;					/**< Data relating to the purge module.				*/
	struct trans_rep		trans_rep;
	struct unrec_rep		unrec_rep;
	struct cashflow_rep		cashflow_rep;
	struct balance_rep		balance_rep;

	/* Pointer to the next file in the list. */

	struct file_data		*next;
};

#endif

