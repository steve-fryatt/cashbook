/* Copyright 2003-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "column.h"
#include "date.h"
#include "sort.h"

/* ==================================================================================================================
 * Templates and resources.
 */

#define FILE_MAX_FILENAME 256

#define IND_DATA_SIZE 8000
#define MAX_LINE_LENGTH 1024

#define COLUMN_HEADING_MARGIN 2
#define COLUMN_SORT_OFFSET 8

#define ACCOUNT_NAME_LEN 32
#define ACCOUNT_IDENT_LEN 5
#define ACCOUNT_NO_LEN 32
#define ACCOUNT_SRTCD_LEN 15
#define ACCOUNT_ADDR_LEN 64

#define ACCOUNT_ADDR_LINES 4

#define REC_FIELD_LEN 2
#define AMOUNT_FIELD_LEN 15

#define NULL_ACCOUNT ((acct_t) -1)
#define NULL_TEMPLATE ((int) (-1))




/* ==================================================================================================================
 * Basic types
 */


/* \TODO -- These need to move into their modules once we've sorted the rest
 *          of this mess out.
 */

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

	struct transact_block		*transacts;				/**< Data relating to the transaction module.			*/
	struct account_block		*accounts;				/**< Data relating to the account module.			*/
	struct sorder_block		*sorders;				/**< Data relating to the standing order module.		*/
	struct preset_block		*presets;				/**< Data relating to the preset module.			*/

	/* Details of the shared account view system. */

	struct accview_block		*accviews;				/**< Data relating to the shared accview module.		*/

	/* The interest rate manager. */

	struct interest_block		*interest;				/**< Data relating to the interest rate manager.		*/

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

