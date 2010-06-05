/* CashBook - date.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_DATE
#define _ACCOUNTS_DATE

/* ==================================================================================================================
 * Static constants
 */

#define DATE_SEP_LENGTH 11

/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */
/* Date conversion. */

void convert_date_to_string (date_t date, char *string);
date_t convert_string_to_date (char *string, date_t previous_date, int month_days);
void convert_date_to_month_string (date_t date, char *string);
void convert_date_to_year_string (date_t date, char *string);

date_t get_current_date (void);

date_t add_to_date (date_t date, int unit, int add);
int days_in_month (int month, int year);
int months_in_year (int year);
int day_of_week (unsigned int date);
int full_month (date_t start, date_t end);
int full_year (date_t start, date_t end);
int count_days (date_t start, date_t end);

date_t get_valid_date (date_t date, int direction);
date_t get_sorder_date (date_t date, int flags);

void set_weekend_days (void);
int read_weekend_days (void);

int is_numeric (char *str);

#endif
