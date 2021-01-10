/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: analysis_balance.h
 *
 * Analysis Balance Report interface.
 */


#ifndef CASHBOOK_ANALYSIS_BALANCE
#define CASHBOOK_ANALYSIS_BALANCE

#include "account.h"
#include "analysis.h"

/**
 * A balance report instance within a file.
 */

struct analysis_balance_block;

/**
 * A balance report data set.
 */

struct analysis_balance_report;


/**
 * Initialise the Balance analysis report module.
 *
 * \return		Pointer to the report type record.
 */

struct analysis_report_details *analysis_balance_initialise(void);







/**
 * Remove any references to a report template.
 * 
 * \param *parent	The analysis instance being updated.
 * \param template	The template to be removed.
 */

//void analysis_balance_remove_template(struct analysis_block *parent, template_t template);

#endif
