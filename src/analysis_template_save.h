/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: analysis_template_save.h
 *
 * Analysis Template save and rename dialogue interface.
 */

#ifndef CASHBOOK_ANALYSIS_TEMPLATE_SAVE
#define CASHBOOK_ANALYSIS_TEMPLATE_SAVE

#include "analysis.h"


/**
 * Initialise the Template Save and Template Rename dialogue.
 */

void analysis_template_save_initialise(void);


/**
 * Open the Save Template dialogue box.
 *
 * \param *template		The report template to be saved.
 * \param *ptr			The current Wimp Pointer details.
 */

void analysis_template_save_open_window(struct analysis_report *template, wimp_pointer *ptr);


/**
 * Open the Rename Template dialogue box.
 *
 * \param *file			The file owning the template.
 * \param template_number	The template to be renamed.
 * \param *ptr			The current Wimp Pointer details.
 */

void analysis_template_save_open_rename_window(struct file_block *file, int template_number, wimp_pointer *ptr);


/**
 * Report that a report template has been deleted, and adjust the
 * dialogue handle accordingly.
 *
 * \param *parent		The analysis instance from which the template has been deleted.
 * \param template		The deleted template ID.
 */

void analysis_template_save_delete_template(struct analysis_block *parent, template_t template);


/**
 * Force the closure of the Save / Rename Template dialogue if the file
 * owning it closes.
 *
 * \param *parent		The parent analysis instance.
 */

void analysis_template_save_force_close(struct analysis_block *parent);


/**
 * Force the closure of the Save Template window if it is open to save the
 * given template.
 *
 * \param *template		The template of interest.
 */

void analysis_template_save_force_template_close(struct analysis_report *template);


/**
 * Force the closure of the Rename Template window if it is open to rename the
 * given template.
 *
 * \param *parent		The analysis block in which the template is held.
 * \param template		The template which is to be closed.
 */

void analysis_template_save_force_rename_close(struct analysis_block *parent, template_t template);

#endif
