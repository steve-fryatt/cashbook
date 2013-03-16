/* Copyright 2003-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: calculation.h
 *
 * Calculation implementation
 */

#ifndef CASHBOOK_CALCULATION
#define CASHBOOK_CALCULATION

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */


/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Full recalculation */

void perform_full_recalculation (file_data *file);

void recalculate_account_windows (file_data *file);

/* Partial calculation */

void remove_transaction_from_totals (file_data *file, int transaction);
void restore_transaction_to_totals (file_data *file, int transaction);

#endif
