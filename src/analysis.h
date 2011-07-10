/* CashBook - analysis.h
 *
 * (c) Stephen Fryatt, 2003-2011
 */

#ifndef CASHBOOK_ANALYSIS
#define CASHBOOK_ANALYSIS

/* ==================================================================================================================
 * Static constants
 */

/* Transaction Report window. */

#define ANALYSIS_TRANS_OK 1
#define ANALYSIS_TRANS_CANCEL 0
#define ANALYSIS_TRANS_DELETE 39
#define ANALYSIS_TRANS_RENAME 40

#define ANALYSIS_TRANS_DATEFROM 5
#define ANALYSIS_TRANS_DATETO 7
#define ANALYSIS_TRANS_DATEFROMTXT 4
#define ANALYSIS_TRANS_DATETOTXT 6
#define ANALYSIS_TRANS_BUDGET 8
#define ANALYSIS_TRANS_GROUP 11
#define ANALYSIS_TRANS_PERIOD 13
#define ANALYSIS_TRANS_PTEXT 12
#define ANALYSIS_TRANS_PDAYS 14
#define ANALYSIS_TRANS_PMONTHS 15
#define ANALYSIS_TRANS_PYEARS 16
#define ANALYSIS_TRANS_LOCK 17

#define ANALYSIS_TRANS_FROMSPEC 21
#define ANALYSIS_TRANS_FROMSPECPOPUP 22
#define ANALYSIS_TRANS_TOSPEC 24
#define ANALYSIS_TRANS_TOSPECPOPUP 25
#define ANALYSIS_TRANS_REFSPEC 27
#define ANALYSIS_TRANS_AMTLOSPEC 29
#define ANALYSIS_TRANS_AMTHISPEC 31
#define ANALYSIS_TRANS_DESCSPEC 33

#define ANALYSIS_TRANS_OPTRANS 36
#define ANALYSIS_TRANS_OPSUMMARY 37
#define ANALYSIS_TRANS_OPACCSUMMARY 38

/* Unreconciled Report window. */

#define ANALYSIS_UNREC_OK 0
#define ANALYSIS_UNREC_CANCEL 1
#define ANALYSIS_UNREC_DELETE 28
#define ANALYSIS_UNREC_RENAME 29

#define ANALYSIS_UNREC_DATEFROMTXT 4
#define ANALYSIS_UNREC_DATEFROM 5
#define ANALYSIS_UNREC_DATETOTXT 6
#define ANALYSIS_UNREC_DATETO 7
#define ANALYSIS_UNREC_BUDGET 8

#define ANALYSIS_UNREC_GROUP 11
#define ANALYSIS_UNREC_GROUPACC 12
#define ANALYSIS_UNREC_GROUPDATE 13
#define ANALYSIS_UNREC_PERIOD 15
#define ANALYSIS_UNREC_PTEXT 14
#define ANALYSIS_UNREC_PDAYS 16
#define ANALYSIS_UNREC_PMONTHS 17
#define ANALYSIS_UNREC_PYEARS 18
#define ANALYSIS_UNREC_LOCK 19

#define ANALYSIS_UNREC_FROMSPEC 23
#define ANALYSIS_UNREC_FROMSPECPOPUP 24
#define ANALYSIS_UNREC_TOSPEC 26
#define ANALYSIS_UNREC_TOSPECPOPUP 27

/* Cashflow Report window. */

#define ANALYSIS_CASHFLOW_OK 0
#define ANALYSIS_CASHFLOW_CANCEL 1
#define ANALYSIS_CASHFLOW_DELETE 31
#define ANALYSIS_CASHFLOW_RENAME 32

#define ANALYSIS_CASHFLOW_DATEFROMTXT 4
#define ANALYSIS_CASHFLOW_DATEFROM 5
#define ANALYSIS_CASHFLOW_DATETOTXT 6
#define ANALYSIS_CASHFLOW_DATETO 7
#define ANALYSIS_CASHFLOW_BUDGET 8

#define ANALYSIS_CASHFLOW_GROUP 11
#define ANALYSIS_CASHFLOW_PERIOD 13
#define ANALYSIS_CASHFLOW_PTEXT 12
#define ANALYSIS_CASHFLOW_PDAYS 14
#define ANALYSIS_CASHFLOW_PMONTHS 15
#define ANALYSIS_CASHFLOW_PYEARS 16
#define ANALYSIS_CASHFLOW_LOCK 17
#define ANALYSIS_CASHFLOW_EMPTY 18

#define ANALYSIS_CASHFLOW_ACCOUNTS 22
#define ANALYSIS_CASHFLOW_ACCOUNTSPOPUP 23
#define ANALYSIS_CASHFLOW_INCOMING 25
#define ANALYSIS_CASHFLOW_INCOMINGPOPUP 26
#define ANALYSIS_CASHFLOW_OUTGOING 28
#define ANALYSIS_CASHFLOW_OUTGOINGPOPUP 29
#define ANALYSIS_CASHFLOW_TABULAR 30

/* Balance Report window. */

#define ANALYSIS_BALANCE_OK 0
#define ANALYSIS_BALANCE_CANCEL 1
#define ANALYSIS_BALANCE_DELETE 30
#define ANALYSIS_BALANCE_RENAME 31

#define ANALYSIS_BALANCE_DATEFROMTXT 4
#define ANALYSIS_BALANCE_DATEFROM 5
#define ANALYSIS_BALANCE_DATETOTXT 6
#define ANALYSIS_BALANCE_DATETO 7
#define ANALYSIS_BALANCE_BUDGET 8

#define ANALYSIS_BALANCE_GROUP 11
#define ANALYSIS_BALANCE_PERIOD 13
#define ANALYSIS_BALANCE_PTEXT 12
#define ANALYSIS_BALANCE_PDAYS 14
#define ANALYSIS_BALANCE_PMONTHS 15
#define ANALYSIS_BALANCE_PYEARS 16
#define ANALYSIS_BALANCE_LOCK 17

#define ANALYSIS_BALANCE_ACCOUNTS 21
#define ANALYSIS_BALANCE_ACCOUNTSPOPUP 22
#define ANALYSIS_BALANCE_INCOMING 24
#define ANALYSIS_BALANCE_INCOMINGPOPUP 25
#define ANALYSIS_BALANCE_OUTGOING 27
#define ANALYSIS_BALANCE_OUTGOINGPOPUP 28
#define ANALYSIS_BALANCE_TABULAR 29

#define ANALYSIS_SAVE_OK 4
#define ANALYSIS_SAVE_CANCEL 3
#define ANALYSIS_SAVE_NAME 1
#define ANALYSIS_SAVE_NAMEPOPUP 2

/* Save Report Dialoge Modes */

#define ANALYSIS_SAVE_MODE_SAVE   1
#define ANALYSIS_SAVE_MODE_RENAME 2


/**
 * Initialise the Analysis module and all its dialogue boxes.
 */

void analysis_initialise(void);


/**
 * Open the Transaction Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_transaction_window(file_data *file, wimp_pointer *ptr, int template, osbool restore);


/**
 * Open the Transaction Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_unreconciled_window(file_data *file, wimp_pointer *ptr, int template, osbool restore);


/**
 * Open the Cashflow Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_cashflow_window(file_data *file, wimp_pointer *ptr, int template, osbool restore);


/**
 * Open the Balance Report dialogue box.
 *
 * \param *file		The file owning the dialogue.
 * \param *ptr		The current Wimp Pointer details.
 * \param template	The report template to use for the dialogue.
 * \param restore	TRUE to retain the last settings for the file; FALSE to
 *			use the application defaults.
 */

void analysis_open_balance_window(file_data *file, wimp_pointer *ptr, int template, osbool restore);











/* Date range manipulation. */

void find_date_range (file_data *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, int budget);
void initialise_date_period (date_t start, date_t end, int period, int unit, int lock);
int get_next_date_period (date_t *next_start, date_t *next_end, char *date_text, int date_len);

/* Account list manipulation. */

void analysis_remove_account_from_reports (file_data *file, acct_t account);
int analysis_remove_account_from_list (file_data *file, acct_t account, acct_t *array, int *count);

void clear_analysis_account_report_flags (file_data *file);
void set_analysis_account_report_flags_from_list (file_data *file, unsigned type, unsigned flags,
                                                  acct_t *array, int count);
int analysis_convert_account_list_to_array (file_data *file, unsigned type, char *list, acct_t *array);
int analysis_convert_account_numbers_to_array (file_data *file, char *list, acct_t *array);
void analysis_convert_account_array_to_numbers (file_data *file, char *list, int size, acct_t *array, int len);
void analysis_convert_account_array_to_list (file_data *file, char *list, acct_t *array, int len);








/* Saving and Renaming Report Templates via the GUI. */

void open_save_report_window (file_data *file, report_data *report, wimp_pointer *ptr);
void analysis_open_rename_report_window (file_data *file, int template, wimp_pointer *ptr);
void refresh_save_report_window (void);
void fill_save_report_window (report_data *report);
void analysis_fill_rename_report_window (file_data *file, int template);
int process_save_report_window (void);
void analysis_open_save_report_popup_menu (wimp_pointer *ptr);

/* Force the closure of the report format windows if the file disappears. */

void force_close_report_windows (file_data *file);
void analysis_force_close_report_save_window (file_data *file, report_data *report);
void analysis_force_close_report_rename_window (wimp_w window);

/* Saved template handling. */

void analysis_open_saved_report_dialogue(file_data *file, wimp_pointer *ptr, int template);
int analysis_find_saved_report_template_from_name (file_data *file, char *name);
void analysis_store_saved_report_template (file_data *file, saved_report *report, int number);
void analysis_delete_saved_report_template (file_data *file, int template);
void analysis_copy_saved_report_template(saved_report *to, saved_report *from);
void analysis_copy_trans_report_template(trans_rep *to, trans_rep *from);
void analysis_copy_unrec_report_template(unrec_rep *to, unrec_rep *from);
void analysis_copy_cashflow_report_template(cashflow_rep *to, cashflow_rep *from);
void analysis_copy_balance_report_template(balance_rep *to, balance_rep *from);

#endif
