/* CashBook - calculation.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/debug.h"

/* Application header files */

#include "global.h"
#include "calculation.h"

#include "account.h"
#include "conversion.h"
#include "date.h"

/* ==================================================================================================================
 * Full recalculation
 */

/* Fully recalculate the file, to use as a base for future calculations. */

void perform_full_recalculation (file_data *file)
{
  int      i;
  unsigned date, post_date;

  hourglass_on ();

  /* Initialise the accounts, based on the opening balances. */

  for (i=0; i<file->account_count; i++)
  {
    file->accounts[i].statement_balance = file->accounts[i].opening_balance;
    file->accounts[i].current_balance = file->accounts[i].opening_balance;
    file->accounts[i].future_balance = file->accounts[i].opening_balance;
    file->accounts[i].budget_balance = 0; /* was file->accounts[i].opening_balance; */
  }

  date = get_current_date ();
  post_date = add_to_date (date, PERIOD_DAYS, file->budget.sorder_trial);

  /* Add in the effects of each transaction */

  for (i=0; i<file->trans_count; i++)
  {
    if (file->transactions[i].from != NULL_ACCOUNT)
    {
      if (file->transactions[i].flags & TRANS_REC_FROM)
      {
        file->accounts[file->transactions[i].from].statement_balance -= file->transactions[i].amount;
      }

      if (file->transactions[i].date <= date)
      {
        file->accounts[file->transactions[i].from].current_balance -= file->transactions[i].amount;
      }

      if ((file->budget.start == NULL_DATE ||file->transactions[i].date >= file->budget.start) &&
          (file->budget.finish == NULL_DATE ||file->transactions[i].date <= file->budget.finish))
      {
        file->accounts[file->transactions[i].from].budget_balance -= file->transactions[i].amount;
      }

      if (!file->budget.limit_postdate || file->transactions[i].date <= post_date)
      {
        file->accounts[file->transactions[i].from].future_balance -= file->transactions[i].amount;
      }
    }

    if (file->transactions[i].to != NULL_ACCOUNT)
    {
      if (file->transactions[i].flags & TRANS_REC_TO)
      {
        file->accounts[file->transactions[i].to].statement_balance += file->transactions[i].amount;
      }

      if (file->transactions[i].date <= date)
      {
        file->accounts[file->transactions[i].to].current_balance += file->transactions[i].amount;
      }

      if ((file->budget.start == NULL_DATE ||file->transactions[i].date >= file->budget.start) &&
          (file->budget.finish == NULL_DATE ||file->transactions[i].date <= file->budget.finish))
      {
        file->accounts[file->transactions[i].to].budget_balance += file->transactions[i].amount;
      }

      if (!file->budget.limit_postdate || file->transactions[i].date <= post_date)
      {
        file->accounts[file->transactions[i].to].future_balance += file->transactions[i].amount;
      }
    }
  }

  /* Calculate the outstanding data for each account. */

  for (i=0; i<file->account_count; i++)
  {
    file->accounts[i].trial_balance = file->accounts[i].future_balance +
                                      file->accounts[i].sorder_trial +  + file->accounts[i].credit_limit;
    file->accounts[i].available_balance = file->accounts[i].future_balance + file->accounts[i].credit_limit;
  }

  file->last_full_recalc = date;

  /* Calculate the accounts windows data and force a redraw of the windows that are open. */

  recalculate_account_windows (file);

  for (i=0; i<ACCOUNT_WINDOWS; i++)
  {
    account_force_window_redraw (file, i, 0, file->account_windows[i].display_lines);
  }

  hourglass_off ();
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Calculate the extra data required to display in the accounts windows (totals, sub-totals, budgets, etc). */

void recalculate_account_windows (file_data *file)
{
  int entry, i, j, sub_total[ACCOUNT_COLUMNS-2], total[ACCOUNT_COLUMNS-2];


  /* Calculate the full accounts details. */

  entry = find_accounts_window_entry_from_type (file, ACCOUNT_FULL);

  for (i=0; i<ACCOUNT_COLUMNS-2; i++)
  {
    sub_total[i] = 0;
    total[i] = 0;
  }

  for (i=0; i<file->account_windows[entry].display_lines; i++)
  {
    switch (file->account_windows[entry].line_data[i].type)
    {
      case ACCOUNT_LINE_DATA:
        sub_total[0] += file->accounts[file->account_windows[entry].line_data[i].account].statement_balance;
        sub_total[1] += file->accounts[file->account_windows[entry].line_data[i].account].current_balance;
        sub_total[2] += file->accounts[file->account_windows[entry].line_data[i].account].trial_balance;
        sub_total[3] += file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;

        total[0] += file->accounts[file->account_windows[entry].line_data[i].account].statement_balance;
        total[1] += file->accounts[file->account_windows[entry].line_data[i].account].current_balance;
        total[2] += file->accounts[file->account_windows[entry].line_data[i].account].trial_balance;
        total[3] += file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;
        break;

      case ACCOUNT_LINE_HEADER:
        for (j=0; j<ACCOUNT_COLUMNS-2; j++)
        {
          sub_total[j] = 0;
        }
        break;

      case ACCOUNT_LINE_FOOTER:
        for (j=0; j<ACCOUNT_COLUMNS-2; j++)
        {
          file->account_windows[entry].line_data[i].total[j] = sub_total[j];
        }
        break;

      default:
        break;
    }
  }

  for (i=0; i<ACCOUNT_COLUMNS-2; i++)
  {
    convert_money_to_string (total[i], file->account_windows[entry].footer_icon[i]);
  }

  /* Calculate the incoming account details. */

  entry = find_accounts_window_entry_from_type (file, ACCOUNT_IN);

  for (i=0; i<ACCOUNT_COLUMNS-2; i++)
  {
    sub_total[i] = 0;
    total[i] = 0;
  }

  for (i=0; i<file->account_windows[entry].display_lines; i++)
  {
    switch (file->account_windows[entry].line_data[i].type)
    {
      case ACCOUNT_LINE_DATA:
        if (file->accounts[file->account_windows[entry].line_data[i].account].budget_amount != NULL_CURRENCY)
        {
          file->accounts[file->account_windows[entry].line_data[i].account].budget_result =
              -file->accounts[file->account_windows[entry].line_data[i].account].budget_amount -
              file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;
        }
        else
        {
          file->accounts[file->account_windows[entry].line_data[i].account].budget_result = NULL_CURRENCY;
        }

        sub_total[0] -= file->accounts[file->account_windows[entry].line_data[i].account].future_balance;
        sub_total[1] += file->accounts[file->account_windows[entry].line_data[i].account].budget_amount;
        sub_total[2] -= file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;
        sub_total[3] += file->accounts[file->account_windows[entry].line_data[i].account].budget_result;

        total[0] -= file->accounts[file->account_windows[entry].line_data[i].account].future_balance;
        total[1] += file->accounts[file->account_windows[entry].line_data[i].account].budget_amount;
        total[2] -= file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;
        total[3] += file->accounts[file->account_windows[entry].line_data[i].account].budget_result;
        break;

      case ACCOUNT_LINE_HEADER:
        for (j=0; j<ACCOUNT_COLUMNS-2; j++)
        {
          sub_total[j] = 0;
        }
        break;

      case ACCOUNT_LINE_FOOTER:
        for (j=0; j<ACCOUNT_COLUMNS-2; j++)
        {
          file->account_windows[entry].line_data[i].total[j] = sub_total[j];
        }
        break;

      default:
        break;
    }
  }


  for (i=0; i<ACCOUNT_COLUMNS-2; i++)
  {
    convert_money_to_string (total[i], file->account_windows[entry].footer_icon[i]);
  }

  /* Calculate the outgoing account details. */

  entry = find_accounts_window_entry_from_type (file, ACCOUNT_OUT);

  for (i=0; i<ACCOUNT_COLUMNS-2; i++)
  {
    sub_total[i] = 0;
    total[i] = 0;
  }

  for (i=0; i<file->account_windows[entry].display_lines; i++)
  {
    switch (file->account_windows[entry].line_data[i].type)
    {
      case ACCOUNT_LINE_DATA:
        if (file->accounts[file->account_windows[entry].line_data[i].account].budget_amount != NULL_CURRENCY)
        {
          file->accounts[file->account_windows[entry].line_data[i].account].budget_result =
              file->accounts[file->account_windows[entry].line_data[i].account].budget_amount -
              file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;
        }
        else
        {
          file->accounts[file->account_windows[entry].line_data[i].account].budget_result = NULL_CURRENCY;
        }

        sub_total[0] += file->accounts[file->account_windows[entry].line_data[i].account].future_balance;
        sub_total[1] += file->accounts[file->account_windows[entry].line_data[i].account].budget_amount;
        sub_total[2] += file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;
        sub_total[3] += file->accounts[file->account_windows[entry].line_data[i].account].budget_result;

        total[0] += file->accounts[file->account_windows[entry].line_data[i].account].future_balance;
        total[1] += file->accounts[file->account_windows[entry].line_data[i].account].budget_amount;
        total[2] += file->accounts[file->account_windows[entry].line_data[i].account].budget_balance;
        total[3] += file->accounts[file->account_windows[entry].line_data[i].account].budget_result;
        break;

      case ACCOUNT_LINE_HEADER:
        for (j=0; j<ACCOUNT_COLUMNS-2; j++)
        {
          sub_total[j] = 0;
        }
        break;

      case ACCOUNT_LINE_FOOTER:
        for (j=0; j<ACCOUNT_COLUMNS-2; j++)
        {
          file->account_windows[entry].line_data[i].total[j] = sub_total[j];
        }
        break;

      default:
        break;
    }
  }

  for (i=0; i<ACCOUNT_COLUMNS-2; i++)
  {
    convert_money_to_string (total[i], file->account_windows[entry].footer_icon[i]);
  }
}

/* ==================================================================================================================
 * Partial calculation
 */

/* Take the current transcation out of the results, before changing it. */

void remove_transaction_from_totals (file_data *file, int transaction)
{
  /* Remove the current transaction from the fully-caluculated records. */

  if (file->transactions[transaction].from != NULL_ACCOUNT)
  {
    if (file->transactions[transaction].flags & TRANS_REC_FROM)
    {
      file->accounts[file->transactions[transaction].from].statement_balance += file->transactions[transaction].amount;
    }

    if (file->transactions[transaction].date <= file->last_full_recalc)
    {
      file->accounts[file->transactions[transaction].from].current_balance += file->transactions[transaction].amount;
    }

    if ((file->budget.start == NULL_DATE ||file->transactions[transaction].date >= file->budget.start) &&
        (file->budget.finish == NULL_DATE ||file->transactions[transaction].date <= file->budget.finish))
    {
      file->accounts[file->transactions[transaction].from].budget_balance += file->transactions[transaction].amount;
    }

    file->accounts[file->transactions[transaction].from].future_balance += file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].from].trial_balance += file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].from].available_balance += file->transactions[transaction].amount;
  }

  if (file->transactions[transaction].to != NULL_ACCOUNT)
  {
    if (file->transactions[transaction].flags & TRANS_REC_TO)
    {
      file->accounts[file->transactions[transaction].to].statement_balance -= file->transactions[transaction].amount;
    }

    if (file->transactions[transaction].date <= file->last_full_recalc)
    {
      file->accounts[file->transactions[transaction].to].current_balance -= file->transactions[transaction].amount;
    }

    if ((file->budget.start == NULL_DATE ||file->transactions[transaction].date >= file->budget.start) &&
        (file->budget.finish == NULL_DATE ||file->transactions[transaction].date <= file->budget.finish))
    {
      file->accounts[file->transactions[transaction].to].budget_balance -= file->transactions[transaction].amount;
    }

    file->accounts[file->transactions[transaction].to].future_balance -= file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].to].trial_balance -= file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].to].available_balance -= file->transactions[transaction].amount;
  }
}


/* ------------------------------------------------------------------------------------------------------------------ */

/* After changing the current transaction, put it back into the records, recalculate the account viewers and refresh
 * things as required.
 */

void restore_transaction_to_totals (file_data *file, int transaction)
{
  int i;

  /* Restore the current transaction back into the fully-caluculated records. */

  if (file->transactions[transaction].from != NULL_ACCOUNT)
  {
    if (file->transactions[transaction].flags & TRANS_REC_FROM)
    {
      file->accounts[file->transactions[transaction].from].statement_balance -= file->transactions[transaction].amount;
    }

    if (file->transactions[transaction].date <= file->last_full_recalc)
    {
      file->accounts[file->transactions[transaction].from].current_balance -= file->transactions[transaction].amount;
    }

    if ((file->budget.start == NULL_DATE ||file->transactions[transaction].date >= file->budget.start) &&
        (file->budget.finish == NULL_DATE ||file->transactions[transaction].date <= file->budget.finish))
    {
      file->accounts[file->transactions[transaction].from].budget_balance -= file->transactions[transaction].amount;
    }

    file->accounts[file->transactions[transaction].from].future_balance -= file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].from].trial_balance -= file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].from].available_balance -= file->transactions[transaction].amount;
  }

  if (file->transactions[transaction].to != NULL_ACCOUNT)
  {
    if (file->transactions[transaction].flags & TRANS_REC_TO)
    {
      file->accounts[file->transactions[transaction].to].statement_balance += file->transactions[transaction].amount;
    }

    if (file->transactions[transaction].date <= file->last_full_recalc)
    {
      file->accounts[file->transactions[transaction].to].current_balance += file->transactions[transaction].amount;
    }

    if ((file->budget.start == NULL_DATE ||file->transactions[transaction].date >= file->budget.start) &&
        (file->budget.finish == NULL_DATE ||file->transactions[transaction].date <= file->budget.finish))
    {
      file->accounts[file->transactions[transaction].to].budget_balance += file->transactions[transaction].amount;
    }

    file->accounts[file->transactions[transaction].to].future_balance += file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].to].trial_balance += file->transactions[transaction].amount;
    file->accounts[file->transactions[transaction].to].available_balance += file->transactions[transaction].amount;
  }

  /* Calculate the accounts windows data and force a redraw of the windows that are open. */

  recalculate_account_windows (file);

  for (i=0; i<ACCOUNT_WINDOWS; i++)
  {
    account_force_window_redraw (file, i, 0, file->account_windows[i].display_lines);
  }
}
