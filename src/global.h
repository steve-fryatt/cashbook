/* Copyright 2003-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "date.h"
#include "sort.h"

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

/* The Description field below must be longer than the Refererence.*/

#define REC_FIELD_LEN 2
#define REF_FIELD_LEN 13
#define AMOUNT_FIELD_LEN 15
#define DESCRIPT_FIELD_LEN 101

#define NULL_ACCOUNT (-1)
#define NULL_TRANSACTION ((int) (-1))
#define NULL_TEMPLATE ((int) (-1))

/* Transaction window details */

#define TRANSACT_COLUMNS 11
#define TRANSACT_TOOLBAR_HEIGHT 132

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


/* ==================================================================================================================
 * Basic types
 */

/* None of these are fully used as yet... */

typedef int		tran_t;							/**< A transaction number.					*/

/* \TODO -- These need to move into their modules once we've sorted the rest
 *          of this mess out.
 */

struct transact;
struct transact_window;
struct report;
struct budget_block;
struct find_block;
struct goto_block;
struct printing;
struct purge_block;
struct trans_rep;
struct unrec_rep;
struct cashflow_rep;
struct balance_rep;



/* ==================================================================================================================
 * Main file data structure
 */

/* File data struct. */

struct file_block
{
	/* File location */

	char				filename[FILE_MAX_FILENAME];		/**< The filename on disc; if "", it hasn't been saved before.	*/
	os_date_and_time		datestamp;				/**< The datestamp of when the file was last saved.		*/

	/* Details of the attached windows. */

	struct transact_window		*transaction_window;			/**< Data relating to the transaction module.			*/
	struct account_block		*accountz;				/**< Data relating to the account module.			*/
	struct sorder_block		*sorder_window;				/**< Data relating to the standing order module.		*/
	struct preset_block		*preset_window;				/**< Data relating to the preset module.			*/

	// \TODO -- accountz can be renamed accounts once the struct account block is inside accountz itself.

  /* Default display details for the accview windows. */

  int                accview_column_width[ACCVIEW_COLUMNS]; /* Base column widths in the account view windows. */
  int                accview_column_position[ACCVIEW_COLUMNS]; /* Base column positions in the account view windows. */
  int                accview_sort_order; /* Default sort order for the accview windows. */

  /* Array size counts. */

  int                trans_count;        /* The number of transactions defined in the file. */

  /* Account, transaction, standing order and preset data structures (pointers which become arrays). */

  struct transaction *transactions;

  /* Recalculation data. */

	date_t				last_full_recalc;			/* The last time a full recalculation was done on the file.	*/

	/* Budget data. */

	struct budget_block		*budget;				/**< The file's budgeting details.				*/

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

	/* Dialogue Content. */

	struct goto_block		*go_to;					/**< Data relating to the goto module.				*/
	struct find_block		*find;					/**< Data relating to the find module.				*/
	struct printing			*print;					/**< Data relating to the print dialogues.			*/
	struct purge_block		*purge;					/**< Data relating to the purge module.				*/

	/* Analysis Report Content. */

	struct trans_rep		*trans_rep;				/**< Data relating to the transaction report dialogue.		*/
	struct unrec_rep		*unrec_rep;				/**< Data relating to the unreconciled report dialogue.		*/
	struct cashflow_rep		*cashflow_rep;				/**< Data relating to the cashflow report dialogue.		*/
	struct balance_rep		*balance_rep;				/**< Data relating to the balance report dialogue.		*/

	/* Pointer to the next file in the list. */

	struct file_block		*next;
};

#endif

