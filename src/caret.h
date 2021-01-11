/* Copyright 2003-2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: caret.h
 *
 * Dialogue box caret history implementation.
 */

#ifndef CASHBOOK_CARET
#define CASHBOOK_CARET

/* ==================================================================================================================
 * Static constants
 */


/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

void place_dialogue_caret (wimp_w window, wimp_i icon);
void place_dialogue_caret_fallback (wimp_w window, int icons, ...);

void close_dialogue_with_caret (wimp_w window);

#endif
