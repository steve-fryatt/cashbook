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
 * \file: file_info.h
 *
 * File Information window interface.
 */

#ifndef CASHBOOK_FILE_INFO
#define CASHBOOK_FILE_INFO

/**
 * Initialise the file information dialogue.
 */

void file_info_initialise(void);


/**
 * Calculate the details of a file, and fill the file info dialogue.
 *
 * \param *file			The file to display data for.
 * \return			The handle of the window.
 */

wimp_w file_info_prepare_dialogue(struct file_block *file);

#endif

