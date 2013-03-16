/* Copyright 2011-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: fontlist.h
 *
 * Font menu implementation.
 */

#ifndef CASHBOOK_FONTLIST
#define CASHBOOK_FONTLIST


/**
 * Build a Font List menu and return the pointer.
 *
 * \return		The created menu, or NULL for an error.
 */

wimp_menu *fontlist_build(void);


/**
 * Destroy any Font List menu which is currently open.
 */

void fontlist_destroy(void);


/**
 * Decode a menu selection from the Font List menu.
 *
 * \param *selection		The Wimp menu selection data.
 * \return			A pointer to the selected font name, in a
 *				buffer which must be freed via heap_free()
 *				after use.
 */

char *fontlist_decode(wimp_selection *selection);

#endif

