/* CashBook - conversion.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_CONVERSION
#define _ACCOUNTS_CONVERSION

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
