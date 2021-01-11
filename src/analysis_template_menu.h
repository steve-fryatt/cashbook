/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: analysis_template_menu.h
 *
 * Analysis Template menu interface.
 */

#ifndef CASHBOOK_ANALYSIS_TEMPLATE_MENU
#define CASHBOOK_ANALYSIS_TEMPLATE_MENU

#include "analysis.h"


/**
 * Build a Template List menu and return the pointer.
 *
 * \param *file			The file to build the menu for.
 * \param standalone		TRUE if the menu is standalone; FALSE for part of
 *				the main menu.
 * \return			The created menu, or NULL for an error.
 */

wimp_menu *analysis_template_menu_build(struct file_block *file, osbool standalone);


/**
 * Test whether the Template List menu contains any entries.
 *
 * \return			TRUE if the menu contains entries; else FALSE.
 */

osbool analysis_template_menu_contains_entries(void);


/**
 * Given an index into the menu, return the template that it identifies.
 *
 * \param selection		The selection index to decode.
 * \return			The associated template, or NULL_TEMPLATE.
 */

template_t analysis_template_menu_decode(int selection);


/**
 * Destroy any Template List menu which is currently open.
 */

void analysis_template_menu_destroy(void);



#endif
