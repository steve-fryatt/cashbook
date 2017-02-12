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

/* ==================================================================================================================
 * Templates and resources.
 */

#define FILE_MAX_FILENAME 256



#define REC_FIELD_LEN 2
#define AMOUNT_FIELD_LEN 15




/* ==================================================================================================================
 * Basic types
 */


/* \TODO -- These need to move into their modules once we've sorted the rest
 *          of this mess out.
 */

struct analysis_report;
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
	/* Data integrity. */

	osbool				modified;				/**< TRUE if the file has unsaved modifications.		*/
	int				untitled_count;				/**< Count to allow default title of the form <Untitled n>.	*/
	int				child_x_offset;				/**< Count for child window opening offset.			*/

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

	/* Budget data. */

	struct budget_block		*budget;				/**< The file's budgeting details.				*/

	/* Report data structure */

	struct report			*reports;				/**< Pointer to a linked list of report structures.		*/

	/* Imports */

	struct report			*import_report;				/**< The current import log report (\TODO -- Import struct?).	*/

	/* Analysis Reports */

	struct analysis_block		*analysis;				/**< Data relating to the analysis report module.		*/

	/* Dialogue Content. */

	struct goto_block		*go_to;					/**< Data relating to the goto module.				*/
	struct find_block		*find;					/**< Data relating to the find module.				*/
	struct printing			*print;					/**< Data relating to the print dialogues.			*/
	struct purge_block		*purge;					/**< Data relating to the purge module.				*/

	/* Pointer to the next file in the list. */

	struct file_block		*next;
};

#endif

