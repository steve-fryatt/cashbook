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
 * \file: conversion.h
 *
 * String to data value conversions.
 */

#ifndef CASHBOOK_CONVERSION
#define CASHBOOK_CONVERSION

/* ------------------------------------------------------------------------------------------------------------------
 * Static constants
 */

#define MAX_DIGITS 9

/* ------------------------------------------------------------------------------------------------------------------
 * Function prototypes.
 */

/* Money conversion. */

void full_convert_money_to_string (amt_t value, char *string, int zeros);
void convert_money_to_string (amt_t value, char *string);
amt_t convert_string_to_money (char *string);

void set_up_money (void);

#endif
