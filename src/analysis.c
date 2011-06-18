/* CashBook - analysis.c
 *
 * (C) Stephen Fryatt, 2003
 */

/* ANSI C header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/debug.h"

/* Application header files */

#include "global.h"
#include "analysis.h"

#include "account.h"
#include "caret.h"
#include "conversion.h"
#include "date.h"
#include "file.h"
#include "ihelp.h"
#include "mainmenu.h"
#include "report.h"
#include "transact.h"

/* ==================================================================================================================
 * Global variables.
 */

/* Date period management. */

date_t period_start, period_end;
int    period_length, period_unit, period_lock, period_first = TRUE;

/* Transaction reports. */

static file_data *trans_rep_file = NULL;
static file_data *unrec_rep_file = NULL;
static file_data *cashflow_rep_file = NULL;
static file_data *balance_rep_file = NULL;

static file_data *save_report_file = NULL;
static report_data *save_report_report = NULL;
static int save_report_template = NULL_TEMPLATE;

static int save_report_mode = 0;

static int trans_rep_window_clear = 0;
static int unrec_rep_window_clear = 0;
static int cashflow_rep_window_clear = 0;
static int balance_rep_window_clear = 0;

static trans_rep trans_rep_settings;
static unrec_rep unrec_rep_settings;
static cashflow_rep cashflow_rep_settings;
static balance_rep balance_rep_settings;

static int trans_rep_template = NULL_TEMPLATE;
static int unrec_rep_template = NULL_TEMPLATE;
static int cashflow_rep_template = NULL_TEMPLATE;
static int balance_rep_template = NULL_TEMPLATE;

static saved_report saved_report_template;

static acct_t wildcard_account_list = NULL_ACCOUNT; /* Pass a pointer to this to set all accounts. */

/* ==================================================================================================================
 * Transaction reporting
 */

void generate_transaction_report (file_data *file)
{
  report_data *report;
  int         i, found, total, unit, period, group, lock, output_trans, output_summary, output_accsummary,
              total_days, period_days, period_limit, entry, account;
  date_t      start_date, end_date, next_start, next_end;
  amt_t       min_amount, max_amount;
  char        line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
  char        *match_ref, *match_desc;


  hourglass_on ();

  if (!(file->sort_valid))
  {
    sort_transactions (file);
  }

  /* Read the date settings. */

  find_date_range (file, &start_date, &end_date,
                   file->trans_rep.date_from, file->trans_rep.date_to, file->trans_rep.budget);

  total_days = count_days (start_date, end_date);

  /* Read the grouping settings. */

  group = file->trans_rep.group;
  unit = file->trans_rep.period_unit;
  lock = file->trans_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
  period = (group) ? file->trans_rep.period : 0;

  /* Read the include list. */

  clear_analysis_account_report_flags (file);

  if (file->trans_rep.from_count == 0 && file->trans_rep.to_count == 0)
  {
     set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM,
                                                  &wildcard_account_list, 1);
     set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO,
                                                  &wildcard_account_list, 1);
  }
  else
  {
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM,
                                                 file->trans_rep.from, file->trans_rep.from_count);
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO,
                                                 file->trans_rep.to, file->trans_rep.to_count);
  }

  min_amount = file->trans_rep.amount_min;
  max_amount = file->trans_rep.amount_max;

  match_ref = (*(file->trans_rep.ref) == '\0') ? NULL : file->trans_rep.ref;
  match_desc = (*(file->trans_rep.desc) == '\0') ? NULL : file->trans_rep.desc;

  /* Read the output options. */

  output_trans = file->trans_rep.output_trans;
  output_summary = file->trans_rep.output_summary;
  output_accsummary = file->trans_rep.output_accsummary;

  /* Open a new report for output. */

  analysis_copy_trans_report_template (&(saved_report_template.data.transaction), &(file->trans_rep));
  if (trans_rep_template == NULL_TEMPLATE)
  {
    *(saved_report_template.name) = '\0';
  }
  else
  {
    strcpy (saved_report_template.name, file->saved_reports[trans_rep_template].name);
  }
  saved_report_template.type = REPORT_TYPE_TRANS;

  msgs_lookup ("TRWinT", line, sizeof (line));
  report = open_new_report (file, line, &saved_report_template);

  if (report != NULL)
  {
    /* Output report heading */

    make_file_leafname (file, b1, sizeof (b1));
    if (*saved_report_template.name != '\0')
    {
      msgs_param_lookup ("GRTitle", line, sizeof (line), saved_report_template.name, b1, NULL, NULL);
    }
    else
    {
      msgs_param_lookup ("TRTitle", line, sizeof (line), b1, NULL, NULL, NULL);
    }
    write_report_line (report, 0, line);

    convert_date_to_string (start_date, b1);
    convert_date_to_string (end_date, b2);
    convert_date_to_string (get_current_date (), b3);
    msgs_param_lookup ("TRHeader", line, sizeof (line), b1, b2, b3, NULL);
    write_report_line (report, 0, line);

    initialise_date_period (start_date, end_date, period, unit, lock);

    /* Initialise the heading remainder values for the report. */

    for (i=0; i < file->account_count; i++)
    {
      if (file->accounts[i].type & ACCOUNT_OUT)
      {
        file->accounts[i].report_balance = file->accounts[i].budget_amount;
      }
      else if (file->accounts[i].type & ACCOUNT_IN)
      {
        file->accounts[i].report_balance = -file->accounts[i].budget_amount;
      }
    }

    while (get_next_date_period (&next_start, &next_end, date_text, sizeof (date_text)))
    {
        /* Zero the heading totals for the report. */

        for (i=0; i < file->account_count; i++)
        {
          file->accounts[i].report_total = 0;
        }

        /* Scan through the transactions, adding the values up for those in range and outputting them to the screen. */

        found = 0;

        for (i=0; i < file->trans_count; i++)
        {
          if ((next_start == NULL_DATE || file->transactions[i].date >= next_start) &&
              (next_end == NULL_DATE || file->transactions[i].date <= next_end) &&
              (((file->transactions[i].from != NULL_ACCOUNT) &&
                ((file->accounts[file->transactions[i].from].report_flags & REPORT_FROM) != 0)) ||
                ((file->transactions[i].to != NULL_ACCOUNT) &&
                 ((file->accounts[file->transactions[i].to].report_flags & REPORT_TO) != 0))) &&
                 ((min_amount == NULL_CURRENCY) || (file->transactions[i].amount >= min_amount)) &&
                 ((max_amount == NULL_CURRENCY) || (file->transactions[i].amount <= max_amount)) &&
                 ((match_ref == NULL) || string_wildcard_compare (match_ref, file->transactions[i].reference, TRUE)) &&
                 ((match_desc == NULL) || string_wildcard_compare (match_desc, file->transactions[i].description, TRUE)))
          {
            if (found == 0)
            {
              write_report_line (report, 0, "");

              if (group == TRUE)
              {
                sprintf (line, "\\u%s", date_text);
                write_report_line (report, 0, line);
              }
              if (output_trans)
              {
                msgs_lookup ("TRHeadings", line, sizeof (line));
                write_report_line (report, 1, line);
              }
            }

            found++;

            /* Update the totals and output the transaction to the report file. */

            if (file->transactions[i].from != NULL_ACCOUNT)
            {
              file->accounts[file->transactions[i].from].report_total -= file->transactions[i].amount;
            }

            if (file->transactions[i].to != NULL_ACCOUNT)
            {
              file->accounts[file->transactions[i].to].report_total += file->transactions[i].amount;
            }

            if (output_trans)
            {
              convert_date_to_string (file->transactions[i].date, b1);

              convert_money_to_string (file->transactions[i].amount, b2);

              sprintf (line, "%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s", b1,
                       find_account_name (file, file->transactions[i].from),
                       find_account_name (file, file->transactions[i].to),
                       file->transactions[i].reference, b2, file->transactions[i].description);

              write_report_line (report, 1, line);
            }
          }
        }

        /* Print the account summaries. */

        if (output_accsummary && found > 0)
        {
          /* Summarise the accounts. */

          total = 0;

          if (output_trans) /* Only output blank line if there are transactions above. */
          {
            write_report_line (report, 0, "");
          }
          msgs_lookup ("TRAccounts", b1, sizeof (b1));
          sprintf(line, "\\i%s", b1);
          write_report_line (report, 2, line);

          entry = find_accounts_window_entry_from_type (file, ACCOUNT_FULL);

          for (i=0; i < file->account_windows[entry].display_lines; i++)
          {
            if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA)
            {
              account = file->account_windows[entry].line_data[i].account;

              if (file->accounts[account].report_total != 0)
              {
                total += file->accounts[account].report_total;
                convert_money_to_string (file->accounts[account].report_total, b1);
                sprintf (line, "\\i%s\\t\\d\\r%s", file->accounts[account].name, b1);
                write_report_line (report, 2, line);
              }
            }
          }
          msgs_lookup ("TRTotal", b1, sizeof (b1));
          convert_money_to_string (total, b2);
          sprintf (line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
          write_report_line (report, 2, line);
        }


        /* Print the transaction summaries. */

        if (output_summary && found > 0)
        {
          /* Summarise the outgoings. */

          total = 0;

          if (output_trans || output_accsummary) /* Only output blank line if there is something above. */
          {
            write_report_line (report, 0, "");
          }
          msgs_lookup ("TROutgoings", b1, sizeof (b1));
          sprintf(line, "\\i%s", b1);
          if (file->trans_rep.budget)
          {
            msgs_lookup ("TRSummExtra", b1, sizeof (b1));
            strcat (line, b1);
          }
          write_report_line (report, 2, line);

          entry = find_accounts_window_entry_from_type (file, ACCOUNT_OUT);

          for (i=0; i < file->account_windows[entry].display_lines; i++)
          {
            if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA)
            {
              account = file->account_windows[entry].line_data[i].account;

              if (file->accounts[account].report_total != 0)
              {
                total += file->accounts[account].report_total;
                convert_money_to_string (file->accounts[account].report_total, b1);
                sprintf (line, "\\i%s\\t\\d\\r%s", file->accounts[account].name, b1);
                if (file->trans_rep.budget)
                {
                  period_days = count_days (next_start, next_end);
                  period_limit = file->accounts[account].budget_amount * period_days / total_days;
                  convert_money_to_string (period_limit, b1);
                  sprintf (b2, "\\t\\d\\r%s", b1);
                  strcat (line, b2);
                  convert_money_to_string (period_limit - file->accounts[account].report_total, b1);
                  sprintf (b2, "\\t\\d\\r%s", b1);
                  strcat (line, b2);
                  file->accounts[i].report_balance -= file->accounts[account].report_total;
                  convert_money_to_string (file->accounts[account].report_balance, b1);
                  sprintf (b2, "\\t\\d\\r%s", b1);
                  strcat (line, b2);
                }
                write_report_line (report, 2, line);
              }
            }
          }
          msgs_lookup ("TRTotal", b1, sizeof (b1));
          convert_money_to_string (total, b2);
          sprintf (line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
          write_report_line (report, 2, line);

          /* Summarise the incomings. */

          total = 0;

          write_report_line (report, 0, "");
          msgs_lookup ("TRIncomings", b1, sizeof (b1));
          sprintf(line, "\\i%s", b1);
          if (file->trans_rep.budget)
          {
            msgs_lookup ("TRSummExtra", b1, sizeof (b1));
            strcat (line, b1);
          }
          write_report_line (report, 2, line);

          entry = find_accounts_window_entry_from_type (file, ACCOUNT_IN);

          for (i=0; i < file->account_windows[entry].display_lines; i++)
          {
            if (file->account_windows[entry].line_data[i].type == ACCOUNT_LINE_DATA)
            {
              account = file->account_windows[entry].line_data[i].account;

              if (file->accounts[account].report_total != 0)
              {
                total += file->accounts[account].report_total;
                convert_money_to_string (-file->accounts[account].report_total, b1);
                sprintf (line, "\\i%s\\t\\d\\r%s", file->accounts[account].name, b1);
                if (file->trans_rep.budget)
                {
                  period_days = count_days (next_start, next_end);
                  period_limit = file->accounts[account].budget_amount * period_days / total_days;
                  convert_money_to_string (period_limit, b1);
                  sprintf (b2, "\\t\\d\\r%s", b1);
                  strcat (line, b2);
                  convert_money_to_string (period_limit - file->accounts[account].report_total, b1);
                  sprintf (b2, "\\t\\d\\r%s", b1);
                  strcat (line, b2);
                  file->accounts[i].report_balance -= file->accounts[account].report_total;
                  convert_money_to_string (file->accounts[account].report_balance, b1);
                  sprintf (b2, "\\t\\d\\r%s", b1);
                  strcat (line, b2);
                }
                write_report_line (report, 2, line);
              }
            }
          }
          msgs_lookup ("TRTotal", b1, sizeof (b1));
          convert_money_to_string (-total, b2);
          sprintf (line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
          write_report_line (report, 2, line);
        }
    }

    close_report (file, report);
  }

  hourglass_off ();
}

/* ================================================================================================================== */

void generate_unreconciled_report (file_data *file)
{
  int         i, acc, found, unit, period, group, lock, tot_in, tot_out, entry;
  char        line[2048], b1[1024], b2[1024], b3[1024], date_text[1024],
              rec_char[REC_FIELD_LEN], r1[REC_FIELD_LEN], r2[REC_FIELD_LEN];
  date_t      start_date, end_date, next_start, next_end;
  report_data *report;

  int         acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};


  hourglass_on ();

  if (!(file->sort_valid))
  {
    sort_transactions (file);
  }

  /* Read the date settings. */

  find_date_range (file, &start_date, &end_date,
                   file->unrec_rep.date_from, file->unrec_rep.date_to, file->unrec_rep.budget);

  /* Read the grouping settings. */

  group = file->unrec_rep.group;
  unit = file->unrec_rep.period_unit;
  lock = file->unrec_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
  period = (group) ? file->unrec_rep.period : 0;

  /* Read the include list. */

  clear_analysis_account_report_flags (file);

  if (file->unrec_rep.from_count == 0 && file->unrec_rep.to_count == 0)
  {
     set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM,
                                                  &wildcard_account_list, 1);
     set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO,
                                                  &wildcard_account_list, 1);
  }
  else
  {
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_IN, REPORT_FROM,
                                                 file->unrec_rep.from, file->unrec_rep.from_count);
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_OUT, REPORT_TO,
                                                 file->unrec_rep.to, file->unrec_rep.to_count);
  }

  /* Start to output the report details. */

  msgs_lookup ("RecChar", rec_char, REC_FIELD_LEN);

  analysis_copy_unrec_report_template (&(saved_report_template.data.unreconciled), &(file->unrec_rep));
  if (unrec_rep_template == NULL_TEMPLATE)
  {
    *(saved_report_template.name) = '\0';
  }
  else
  {
    strcpy (saved_report_template.name, file->saved_reports[unrec_rep_template].name);
  }
  saved_report_template.type = REPORT_TYPE_UNREC;

  msgs_lookup ("URWinT", line, sizeof (line));
  report = open_new_report (file, line, &saved_report_template);

  if (report != NULL)
  {
    /* Output report heading */

    make_file_leafname (file, b1, sizeof (b1));
    if (*saved_report_template.name != '\0')
    {
      msgs_param_lookup ("GRTitle", line, sizeof (line), saved_report_template.name, b1, NULL, NULL);
    }
    else
    {
      msgs_param_lookup ("URTitle", line, sizeof (line), b1, NULL, NULL, NULL);
    }
    write_report_line (report, 0, line);

    convert_date_to_string (start_date, b1);
    convert_date_to_string (end_date, b2);
    convert_date_to_string (get_current_date (), b3);
    msgs_param_lookup ("URHeader", line, sizeof (line), b1, b2, b3, NULL);
    write_report_line (report, 0, line);

    if (group && unit == PERIOD_NONE)
    {
      /* We are doing a grouped-by-account report.
       *
       * Step through the accounts in account list order, and run through all the transactions each time.  A
       * transaction is added if it is unreconciled in the account concerned; transactions unreconciled in two
       * accounts may therefore appear twice in the list.
       */

      for (acc_group = 0; acc_group < groups; acc_group++)
      {
        entry = find_accounts_window_entry_from_type (file, sequence[acc_group]);

        for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++)
        {
          if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA)
          {
            acc = file->account_windows[entry].line_data[group_line].account;

            found = 0;
            tot_in = 0;
            tot_out = 0;

            for (i=0; i < file->trans_count; i++)
            {
              if ((start_date == NULL_DATE || file->transactions[i].date >= start_date) &&
                  (end_date == NULL_DATE || file->transactions[i].date <= end_date) &&
                  ((file->transactions[i].from == acc && (file->accounts[acc].report_flags & REPORT_FROM) != 0 &&
                    (file->transactions[i].flags & TRANS_REC_FROM) == 0) ||
                   (file->transactions[i].to == acc && (file->accounts[acc].report_flags & REPORT_TO) != 0 &&
                    (file->transactions[i].flags & TRANS_REC_TO) == 0)))
              {
                if (found == 0)
                {
                  write_report_line (report, 0, "");

                  if (group == TRUE)
                  {
                    sprintf (line, "\\u%s", find_account_name (file, acc));
                    write_report_line (report, 0, line);
                  }
                  msgs_lookup ("URHeadings", line, sizeof (line));
                  write_report_line (report, 1, line);
                }

                found++;

                if (file->transactions[i].from == acc)
                {
                  tot_out -= file->transactions[i].amount;
                }

                else if (file->transactions[i].to == acc)
                {
                  tot_in += file->transactions[i].amount;
                }

                /* Output the transaction to the report. */

                strcpy (r1, (file->transactions[i].flags & TRANS_REC_FROM) ? rec_char : "");
                strcpy (r2, (file->transactions[i].flags & TRANS_REC_TO) ? rec_char : "");
                convert_date_to_string (file->transactions[i].date, b1);
                convert_money_to_string (file->transactions[i].amount, b2);

                sprintf (line, "%s\\t%s\\t%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s", b1,
                         r1, find_account_name (file, file->transactions[i].from),
                         r2, find_account_name (file, file->transactions[i].to),
                         file->transactions[i].reference, b2, file->transactions[i].description);

                write_report_line (report, 1, line);
              }
            }

            if (found != 0)
            {
              write_report_line (report, 2, "");

              msgs_lookup ("URTotalIn", b1, sizeof (b1));
              full_convert_money_to_string (tot_in, b2, TRUE);
              sprintf (line, "\\i%s\\t\\d\\r%s", b1, b2);
              write_report_line (report, 2, line);

              msgs_lookup ("URTotalOut", b1, sizeof (b1));
              full_convert_money_to_string (tot_out, b2, TRUE);
              sprintf (line, "\\i%s\\t\\d\\r%s", b1, b2);
              write_report_line (report, 2, line);

              msgs_lookup ("URTotal", b1, sizeof (b1));
              full_convert_money_to_string (tot_in+tot_out, b2, TRUE);
              sprintf (line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
              write_report_line (report, 2, line);
            }
          }
        }
      }
    }
    else
    {
      /* We are either doing a grouped-by-date report, or not grouping at all.
       *
       * For each date period, run through the transactions and output any which fall within it.
       */

      initialise_date_period (start_date, end_date, period, unit, lock);

      while (get_next_date_period (&next_start, &next_end, date_text, sizeof (date_text)))
      {
        found = 0;

        for (i=0; i < file->trans_count; i++)
        {
          if ((next_start == NULL_DATE || file->transactions[i].date >= next_start) &&
              (next_end == NULL_DATE || file->transactions[i].date <= next_end) &&
              (((file->transactions[i].flags & TRANS_REC_FROM) == 0 &&
                (file->transactions[i].from != NULL_ACCOUNT) &&
                (file->accounts[file->transactions[i].from].report_flags & REPORT_FROM) != 0) ||
               ((file->transactions[i].flags & TRANS_REC_TO) == 0 &&
                (file->transactions[i].to != NULL_ACCOUNT) &&
                (file->accounts[file->transactions[i].to].report_flags & REPORT_TO) != 0)))
          {
            if (found == 0)
            {
              write_report_line (report, 0, "");

              if (group == TRUE)
              {
                sprintf (line, "\\u%s", date_text);
                write_report_line (report, 0, line);
              }
              msgs_param_lookup ("URHeadings", line, sizeof (line), NULL, NULL, NULL, NULL);
              write_report_line (report, 1, line);
            }

            found++;

            /* Output the transaction to the report. */

            strcpy (r1, (file->transactions[i].flags & TRANS_REC_FROM) ? rec_char : "");
            strcpy (r2, (file->transactions[i].flags & TRANS_REC_TO) ? rec_char : "");
            convert_date_to_string (file->transactions[i].date, b1);
            convert_money_to_string (file->transactions[i].amount, b2);

            sprintf (line, "%s\\t%s\\t%s\\t%s\\t%s\\t%s\\t\\d\\r%s\\t%s", b1,
                     r1, find_account_name (file, file->transactions[i].from),
                     r2, find_account_name (file, file->transactions[i].to),
                     file->transactions[i].reference, b2, file->transactions[i].description);

            write_report_line (report, 1, line);
          }
        }
      }
    }

    close_report (file, report);
  }

  hourglass_off ();
}

/* ================================================================================================================== */

void generate_cashflow_report (file_data *file)
{
  int         i, acc, items, found, unit, period, group, lock, tabular, show_blank, total;
  char        line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
  date_t      start_date, end_date, next_start, next_end;
  report_data *report;
  int         entry, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};


  hourglass_on ();

  if (!(file->sort_valid))
  {
    sort_transactions (file);
  }

  /* Read the date settings. */

  find_date_range (file, &start_date, &end_date,
                   file->cashflow_rep.date_from, file->cashflow_rep.date_to, file->cashflow_rep.budget);

  /* Read the grouping settings. */

  group = file->cashflow_rep.group;
  unit = file->cashflow_rep.period_unit;
  lock = file->cashflow_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
  period = (group) ? file->cashflow_rep.period : 0;
  show_blank = file->cashflow_rep.empty;

  /* Read the include list. */

  clear_analysis_account_report_flags (file);

  if (file->cashflow_rep.accounts_count == 0 && file->cashflow_rep.incoming_count == 0 &&
      file->cashflow_rep.outgoing_count == 0)
  {
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT, REPORT_INCLUDE,
                                                 &wildcard_account_list, 1);
  }
  else
  {
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL, REPORT_INCLUDE,
                                                 file->cashflow_rep.accounts, file->cashflow_rep.accounts_count);
    set_analysis_account_report_flags_from_list (file, ACCOUNT_IN, REPORT_INCLUDE,
                                                 file->cashflow_rep.incoming, file->cashflow_rep.incoming_count);
    set_analysis_account_report_flags_from_list (file, ACCOUNT_OUT, REPORT_INCLUDE,
                                                 file->cashflow_rep.outgoing, file->cashflow_rep.outgoing_count);
  }

  tabular = file->cashflow_rep.tabular;

  /* Count the number of accounts and headings to be included.  If this comes to more than the number of tab
   * stops available (including 2 for account name and total), force the tabular format option off.
   */

  items = 0;

  for (i=0; i < file->account_count; i++)
  {
    if ((file->accounts[i].report_flags & REPORT_INCLUDE) != 0)
    {
      items++;
    }
  }

  if ((items + 2) > REPORT_TAB_STOPS)
  {
    tabular = FALSE;
  }

  /* Start to output the report details. */

  analysis_copy_cashflow_report_template (&(saved_report_template.data.cashflow), &(file->cashflow_rep));
  if (cashflow_rep_template == NULL_TEMPLATE)
  {
    *(saved_report_template.name) = '\0';
  }
  else
  {
    strcpy (saved_report_template.name, file->saved_reports[cashflow_rep_template].name);
  }
  saved_report_template.type = REPORT_TYPE_CASHFLOW;

  msgs_lookup ("CRWinT", line, sizeof (line));
  report = open_new_report (file, line, &saved_report_template);

  if (report != NULL)
  {
    /* Output report heading */

    make_file_leafname (file, b1, sizeof (b1));
    if (*saved_report_template.name != '\0')
    {
      msgs_param_lookup ("GRTitle", line, sizeof (line), saved_report_template.name, b1, NULL, NULL);
    }
    else
    {
      msgs_param_lookup ("CRTitle", line, sizeof (line), b1, NULL, NULL, NULL);
    }
    write_report_line (report, 0, line);

    convert_date_to_string (start_date, b1);
    convert_date_to_string (end_date, b2);
    convert_date_to_string (get_current_date (), b3);
    msgs_param_lookup ("CRHeader", line, sizeof (line), b1, b2, b3, NULL);
    write_report_line (report, 0, line);

    /* Start to output the report. */

    if (tabular)
    {
      write_report_line (report, 0, "");
      msgs_lookup ("CRDate", b1, sizeof (b1));
      sprintf (line, "\\b%s", b1);

      for (acc_group = 0; acc_group < groups; acc_group++)
      {
        entry = find_accounts_window_entry_from_type (file, sequence[acc_group]);

        for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++)
        {
          if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA)
          {
            acc = file->account_windows[entry].line_data[group_line].account;

            if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0)
            {
              sprintf (b1, "\\t\\r\\b%s", file->accounts[acc].name);
              strcat (line, b1);
            }
          }
        }
      }
      msgs_lookup ("CRTotal", b1, sizeof (b1));
      sprintf (b2, "\\t\\r\\b%s", b1);
      strcat (line, b2);

      write_report_line (report, 1, line);
    }

    initialise_date_period (start_date, end_date, period, unit, lock);

    while (get_next_date_period (&next_start, &next_end, date_text, sizeof (date_text)))
    {
      /* Zero the heading totals for the report. */

      for (i=0; i < file->account_count; i++)
      {
        file->accounts[i].report_total = 0;
      }

      /* Scan through the transactions, adding the values up for those in range and outputting them to the screen. */

      found = 0;

      for (i=0; i < file->trans_count; i++)
      {
        if ((next_start == NULL_DATE || file->transactions[i].date >= next_start) &&
           (next_end == NULL_DATE || file->transactions[i].date <= next_end))
        {
          if (file->transactions[i].from != NULL_ACCOUNT)
          {
            file->accounts[file->transactions[i].from].report_total -= file->transactions[i].amount;
          }

          if (file->transactions[i].to != NULL_ACCOUNT)
          {
            file->accounts[file->transactions[i].to].report_total += file->transactions[i].amount;
          }

          found++;
        }
      }

      /* Print the transaction summaries. */

      if (found > 0 || show_blank)
      {
        if (tabular)
        {
          strcpy (line, date_text);

          total = 0;

          for (acc_group = 0; acc_group < groups; acc_group++)
          {
            entry = find_accounts_window_entry_from_type (file, sequence[acc_group]);

            for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++)
            {
              if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA)
              {
                acc = file->account_windows[entry].line_data[group_line].account;

                if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0)
                {
                  total += file->accounts[acc].report_total;
                  full_convert_money_to_string (file->accounts[acc].report_total, b1, TRUE);
                  sprintf (b2, "\\t\\d\\r%s", b1);
                  strcat (line, b2);
                }
              }
            }
          }

          full_convert_money_to_string (total, b1, TRUE);
          sprintf (b2, "\\t\\d\\r%s", b1);
          strcat (line, b2);
          write_report_line (report, 1, line);
        }
        else
        {
          write_report_line (report, 0, "");
          if (group)
          {
            sprintf (line, "\\u%s", date_text);
            write_report_line (report, 0, line);
          }

          total = 0;

          for (acc_group = 0; acc_group < groups; acc_group++)
          {
            entry = find_accounts_window_entry_from_type (file, sequence[acc_group]);

            for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++)
            {
              if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA)
              {
                acc = file->account_windows[entry].line_data[group_line].account;

                if (file->accounts[acc].report_total != 0 && (file->accounts[acc].report_flags & REPORT_INCLUDE) != 0)
                {
                  total += file->accounts[acc].report_total;
                  full_convert_money_to_string (file->accounts[acc].report_total, b1, TRUE);
                  sprintf (line, "\\i%s\\t\\d\\r%s", file->accounts[acc].name, b1);
                  write_report_line (report, 2, line);
                }
              }
            }
          }
          msgs_lookup ("CRTotal", b1, sizeof (b1));
          full_convert_money_to_string (total, b2, TRUE);
          sprintf (line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
          write_report_line (report, 2, line);
        }
      }
    }

    close_report (file, report);
  }

  hourglass_off ();
}

/* ================================================================================================================== */

void generate_balance_report (file_data *file)
{
  int         i, acc, items, unit, period, group, lock, tabular, total;
  char        line[2048], b1[1024], b2[1024], b3[1024], date_text[1024];
  date_t      start_date, end_date, next_start, next_end;
  report_data *report;
  int         entry, acc_group, group_line, groups = 3, sequence[]={ACCOUNT_FULL,ACCOUNT_IN,ACCOUNT_OUT};


  hourglass_on ();

  if (!(file->sort_valid))
  {
    sort_transactions (file);
  }

  /* Read the date settings. */

  find_date_range (file, &start_date, &end_date,
                   file->balance_rep.date_from, file->balance_rep.date_to, file->balance_rep.budget);

  /* Read the grouping settings. */

  group = file->balance_rep.group;
  unit = file->balance_rep.period_unit;
  lock = file->balance_rep.lock && (unit == PERIOD_MONTHS || unit == PERIOD_YEARS);
  period = (group) ? file->balance_rep.period : 0;

  /* Read the include list. */

  clear_analysis_account_report_flags (file);

  if (file->balance_rep.accounts_count == 0 && file->balance_rep.incoming_count == 0 &&
      file->balance_rep.outgoing_count == 0)
  {
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL | ACCOUNT_IN | ACCOUNT_OUT, REPORT_INCLUDE,
                                                 &wildcard_account_list, 1);
  }
  else
  {
    set_analysis_account_report_flags_from_list (file, ACCOUNT_FULL, REPORT_INCLUDE,
                                                 file->balance_rep.accounts, file->balance_rep.accounts_count);
    set_analysis_account_report_flags_from_list (file, ACCOUNT_IN, REPORT_INCLUDE,
                                                 file->balance_rep.incoming, file->balance_rep.incoming_count);
    set_analysis_account_report_flags_from_list (file, ACCOUNT_OUT, REPORT_INCLUDE,
                                                 file->balance_rep.outgoing, file->balance_rep.outgoing_count);
  }

  tabular = file->balance_rep.tabular;

  /* Count the number of accounts and headings to be included.  If this comes to more than the number of tab
   * stops available (including 2 for account name and total), force the tabular format option off.
   */

  items = 0;

  for (i=0; i < file->account_count; i++)
  {
    if ((file->accounts[i].report_flags & REPORT_INCLUDE) != 0)
    {
      items++;
    }
  }

  if ((items + 2) > REPORT_TAB_STOPS)
  {
    tabular = FALSE;
  }

  /* Start to output the report details. */

  analysis_copy_balance_report_template (&(saved_report_template.data.balance), &(file->balance_rep));
  if (balance_rep_template == NULL_TEMPLATE)
  {
    *(saved_report_template.name) = '\0';
  }
  else
  {
    strcpy (saved_report_template.name, file->saved_reports[balance_rep_template].name);
  }
  saved_report_template.type = REPORT_TYPE_BALANCE;

  msgs_lookup ("BRWinT", line, sizeof (line));
  report = open_new_report (file, line, &saved_report_template);

  if (report != NULL)
  {
    /* Output report heading */

    make_file_leafname (file, b1, sizeof (b1));
    if (*saved_report_template.name != '\0')
    {
      msgs_param_lookup ("GRTitle", line, sizeof (line), saved_report_template.name, b1, NULL, NULL);
    }
    else
    {
      msgs_param_lookup ("BRTitle", line, sizeof (line), b1, NULL, NULL, NULL);
    }
    write_report_line (report, 0, line);

    convert_date_to_string (start_date, b1);
    convert_date_to_string (end_date, b2);
    convert_date_to_string (get_current_date (), b3);
    msgs_param_lookup ("BRHeader", line, sizeof (line), b1, b2, b3, NULL);
    write_report_line (report, 0, line);

    /* Start to output the report. */

    if (tabular)
    {
      write_report_line (report, 0, "");
      msgs_lookup ("BRDate", b1, sizeof (b1));
      sprintf (line, "\\b%s", b1);

      for (acc_group = 0; acc_group < groups; acc_group++)
      {
        entry = find_accounts_window_entry_from_type (file, sequence[acc_group]);

        for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++)
        {
          if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA)
          {
            acc = file->account_windows[entry].line_data[group_line].account;

            if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0)
            {
              sprintf (b1, "\\t\\r\\b%s", file->accounts[acc].name);
              strcat (line, b1);
            }
          }
        }
      }
      msgs_lookup ("BRTotal", b1, sizeof (b1));
      sprintf (b2, "\\t\\r\\b%s", b1);
      strcat (line, b2);

      write_report_line (report, 1, line);
    }

    initialise_date_period (start_date, end_date, period, unit, lock);

    while (get_next_date_period (&next_start, &next_end, date_text, sizeof (date_text)))
    {
      /* Zero the heading totals for the report. */

      for (i=0; i < file->account_count; i++)
      {
        file->accounts[i].report_total = file->accounts[i].opening_balance;
      }

      /* Scan through the transactions, adding the values up for those occurring before the end of the current
       * period and outputting them to the screen.
       */

      for (i=0; i < file->trans_count; i++)
      {
        if (next_end == NULL_DATE || file->transactions[i].date <= next_end)
        {
          if (file->transactions[i].from != NULL_ACCOUNT)
          {
            file->accounts[file->transactions[i].from].report_total -= file->transactions[i].amount;
          }

          if (file->transactions[i].to != NULL_ACCOUNT)
          {
            file->accounts[file->transactions[i].to].report_total += file->transactions[i].amount;
          }
        }
      }

      /* Print the transaction summaries. */

      if (tabular)
      {
        strcpy (line, date_text);

        total = 0;


        for (acc_group = 0; acc_group < groups; acc_group++)
        {
          entry = find_accounts_window_entry_from_type (file, sequence[acc_group]);

          for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++)
          {
            if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA)
            {
              acc = file->account_windows[entry].line_data[group_line].account;

              if ((file->accounts[acc].report_flags & REPORT_INCLUDE) != 0)
              {
                total += file->accounts[acc].report_total;
                full_convert_money_to_string (file->accounts[acc].report_total, b1, TRUE);
                sprintf (b2, "\\t\\d\\r%s", b1);
                strcat (line, b2);
              }
            }
          }
        }
        full_convert_money_to_string (total, b1, TRUE);
        sprintf (b2, "\\t\\d\\r%s", b1);
        strcat (line, b2);
        write_report_line (report, 1, line);
      }
      else
      {
        write_report_line (report, 0, "");
        if (group)
        {
          sprintf (line, "\\u%s", date_text);
          write_report_line (report, 0, line);
        }

        total = 0;

        for (acc_group = 0; acc_group < groups; acc_group++)
        {
          entry = find_accounts_window_entry_from_type (file, sequence[acc_group]);

          for (group_line = 0; group_line < file->account_windows[entry].display_lines; group_line++)
          {
            if (file->account_windows[entry].line_data[group_line].type == ACCOUNT_LINE_DATA)
            {
              acc = file->account_windows[entry].line_data[group_line].account;

              if (file->accounts[acc].report_total != 0 && (file->accounts[acc].report_flags & REPORT_INCLUDE) != 0)
              {
                total += file->accounts[acc].report_total;
                full_convert_money_to_string (file->accounts[acc].report_total, b1, TRUE);
                sprintf (line, "\\i%s\\t\\d\\r%s", file->accounts[acc].name, b1);
                write_report_line (report, 2, line);
              }
            }
          }
        }
        msgs_lookup ("BRTotal", b1, sizeof (b1));
        full_convert_money_to_string (total, b2, TRUE);
        sprintf (line, "\\i\\b%s\\t\\d\\r\\b%s", b1, b2);
        write_report_line (report, 2, line);
      }
    }

    close_report (file, report);
  }

  hourglass_off ();
}

/* ==================================================================================================================
 * Date range manipulation.
 */

/* Get the range of dates to report over, based on the values entered and the file concerned.
 */

void find_date_range (file_data *file, date_t *start_date, date_t *end_date, date_t date1, date_t date2, int budget)
{
  int i, find_start, find_end;


  if (budget)
  {
    /* Get the start and end dates from the budget settings. */

    *start_date = file->budget.start;
    *end_date = file->budget.finish;
  }
  else
  {
    /* Get the start and end dates from the icon text. */

    *start_date = date1;
    *end_date = date2;
  }

  find_start = (*start_date == NULL_DATE);
  find_end = (*end_date == NULL_DATE);

  /* If either of the dates wasn't specified, we need to find the earliest and latest dates in the file. */

  if (find_start || find_end)
  {
    if (find_start)
    {
      *start_date = (file->trans_count > 0) ? file->transactions[0].date : NULL_DATE;
    }

    if (find_end)
    {
      *end_date = (file->trans_count > 0) ? file->transactions[0].date : NULL_DATE;
    }

    for (i=0; i<file->trans_count; i++)
    {
      if (find_start && file->transactions[i].date != NULL_DATE && file->transactions[i].date < *start_date)
      {
        *start_date = file->transactions[i].date;
      }

      if (find_end && file->transactions[i].date != NULL_DATE && file->transactions[i].date > *end_date)
      {
        *end_date = file->transactions[i].date;
      }
    }
  }

  if (*start_date == NULL_DATE)
  {
    *start_date = MIN_DATE;
  }

  if (*end_date == NULL_DATE)
  {
    *end_date = MAX_DATE;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Initialise the date period function.  Set the various global variables up so that get_next_date_period ()
 * can be called.
 */

void initialise_date_period (date_t start, date_t end, int period, int unit, int lock)
{
  period_start = start;
  period_end = end;
  period_length = period;
  period_unit = unit;
  period_lock = lock;

  period_first = lock;
}

/* ------------------------------------------------------------------------------------------------------------------ */

int get_next_date_period (date_t *next_start, date_t *next_end, char *date_text, int date_len)
{
  char b1[1024], b2[1024];


  if (period_start > period_end)
  {
    return (0);
  }

  if (period_length > 0)
  {
    /* If the report is to be grouped, find the next_end date which falls at the end of the period.
     *
     * If first_lock is set, the report is locked to the calendar and this is the first iteration.  Therefore,
     * the end date is found by adding the period-1 to the current date, then setting the DAYS or DAYS+MONTHS to
     * maximum in the result.  This means that the first period will be no more than the specified period.
     * The resulting date will later be fixed into a valid date, before it is used in anger.
     *
     * If first_lock is not set, the next_end is found by adding the group period to the start date and subtracting
     * 1 from it.  By this point, locked reports will be period aligned anyway, so this should work OK.
     */

    if (period_first)
    {
      *next_end = add_to_date (period_start, period_unit, period_length - 1);

      switch (period_unit)
      {
        case PERIOD_MONTHS:
          *next_end = (*next_end & 0xffffff00) | 0x001f; /* Maximise the days, so end of month. */
          break;

        case PERIOD_YEARS:
          *next_end = (*next_end & 0xffff0000) | 0x0c1f; /* Maximise the days and months, so end of year. */
          break;

        default:
          *next_end = add_to_date (period_start, period_unit, period_length) - 1;
          break;
      }
    }
    else
    {
      *next_end = add_to_date (period_start, period_unit, period_length) - 1;
    }

    /* Pull back into range isf we fall off the end. */

    if (*next_end > period_end)
    {
      *next_end = period_end;
    }
  }
  else
  {
    /* If the report is not to be grouped, the next_end date is just the end of the report period. */

    *next_end = period_end;
  }

  /* Get the real start and end dates for the period. */

  *next_start = get_valid_date (period_start, +1);
  *next_end = get_valid_date (*next_end, -1);

  if (period_length > 0)
  {
    /* If the report is grouped, find the next start date by adding the period on to the current start date. */

    period_start = add_to_date (period_start, period_unit, period_length);

    if (period_first)
    {
      /* If the report is calendar locked, and this is the first iteration, reset the DAYS or DAYS+MONTHS
       * to one so that the start date will be locked on to the calendar from now on.
       */

      switch (period_unit)
      {
        case PERIOD_MONTHS:
          period_start = (period_start & 0xffffff00) | 0x0001; /* Set the days to one. */
          break;

        case PERIOD_YEARS:
          period_start = (period_start & 0xffff0000) | 0x0101; /* Set the days and months to one. */
          break;
      }

      period_first = FALSE;
    }
  }
  else
  {
    period_start = period_end+1;
  }

  /* Generate a date period title for the report section.
   *
   * If calendar locked, this will be of the form "June 2003", or "1998"; otherwise it will be of the form
   * "<start date> - <end date>".
   */

  *date_text = '\0';

  if (period_lock)
  {
    switch (period_unit)
    {
      case PERIOD_MONTHS:
        convert_date_to_month_string (*next_start, b1);

        if ((*next_start & 0xffffff00) == (*next_end & 0xffffff00))
        {
          msgs_param_lookup ("PRMonth", date_text, date_len, b1, NULL, NULL, NULL);
        }
        else
        {
          convert_date_to_month_string (*next_end, b2);
          msgs_param_lookup ("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
        }
        break;

      case PERIOD_YEARS:
        convert_date_to_year_string (*next_start, b1);

        if ((*next_start & 0xffff0000) == (*next_end & 0xffff0000))
        {
          msgs_param_lookup ("PRYear", date_text, date_len, b1, NULL, NULL, NULL);
        }
        else
        {
          convert_date_to_year_string (*next_end, b2);
          msgs_param_lookup ("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
        }
        break;
    }
  }
  else
  {
    if (*next_start == *next_end)
    {
      convert_date_to_string (*next_start, b1);
      msgs_param_lookup ("PRDay", date_text, date_len, b1, NULL, NULL, NULL);
    }
    else
    {
      convert_date_to_string (*next_start, b1);
      convert_date_to_string (*next_end, b2);
      msgs_param_lookup ("PRPeriod", date_text, date_len, b1, b2, NULL, NULL);
    }
  }

  return (1);
}

/* ==================================================================================================================
 * Account list manipulation.
 */

/* Remove an account from all of the report templates in a file. */

void analysis_remove_account_from_reports (file_data *file, acct_t account)
{
  int         i;
  report_data *report;

  /* Handle the dialogue boxes. */

  analysis_remove_account_from_list (file, account, file->trans_rep.from, &(file->trans_rep.from_count));
  analysis_remove_account_from_list (file, account, file->trans_rep.to, &(file->trans_rep.to_count));

  analysis_remove_account_from_list (file, account, file->unrec_rep.from, &(file->unrec_rep.from_count));
  analysis_remove_account_from_list (file, account, file->unrec_rep.to, &(file->unrec_rep.to_count));

  analysis_remove_account_from_list (file, account, file->cashflow_rep.accounts, &(file->cashflow_rep.accounts_count));
  analysis_remove_account_from_list (file, account, file->cashflow_rep.incoming, &(file->cashflow_rep.incoming_count));
  analysis_remove_account_from_list (file, account, file->cashflow_rep.outgoing, &(file->cashflow_rep.outgoing_count));

  analysis_remove_account_from_list (file, account, file->balance_rep.accounts, &(file->balance_rep.accounts_count));
  analysis_remove_account_from_list (file, account, file->balance_rep.incoming, &(file->balance_rep.incoming_count));
  analysis_remove_account_from_list (file, account, file->balance_rep.outgoing, &(file->balance_rep.outgoing_count));

  /* Now process any saved templates. */

  for (i=0; i<file->saved_report_count; i++)
  {
    switch (file->saved_reports[i].type)
    {
      case REPORT_TYPE_TRANS:
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.transaction.from,
                                           &(file->saved_reports[i].data.transaction.from_count));
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.transaction.to,
                                           &(file->saved_reports[i].data.transaction.to_count));
        break;
      case REPORT_TYPE_UNREC:
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.unreconciled.from,
                                           &(file->saved_reports[i].data.unreconciled.from_count));
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.unreconciled.to,
                                           &(file->saved_reports[i].data.unreconciled.to_count));
        break;
      case REPORT_TYPE_CASHFLOW:
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.cashflow.accounts,
                                           &(file->saved_reports[i].data.cashflow.accounts_count));
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.cashflow.incoming,
                                           &(file->saved_reports[i].data.cashflow.incoming_count));
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.cashflow.outgoing,
                                           &(file->saved_reports[i].data.cashflow.outgoing_count));
        break;
      case REPORT_TYPE_BALANCE:
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.balance.accounts,
                                           &(file->saved_reports[i].data.balance.accounts_count));
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.balance.incoming,
                                           &(file->saved_reports[i].data.balance.incoming_count));
        analysis_remove_account_from_list (file, account, file->saved_reports[i].data.balance.outgoing,
                                           &(file->saved_reports[i].data.balance.outgoing_count));
        break;
    }
  }

  /* Finally, work through any reports in the file. */

  report = file->reports;

  while (report != NULL)
  {
    switch (report->template.type)
    {
      case REPORT_TYPE_TRANS:
        analysis_remove_account_from_list (file, account, report->template.data.transaction.from,
                                           &(report->template.data.transaction.from_count));
        analysis_remove_account_from_list (file, account, report->template.data.transaction.to,
                                           &(report->template.data.transaction.to_count));
        break;
      case REPORT_TYPE_UNREC:
        analysis_remove_account_from_list (file, account, report->template.data.unreconciled.from,
                                           &(report->template.data.unreconciled.from_count));
        analysis_remove_account_from_list (file, account, report->template.data.unreconciled.to,
                                           &(report->template.data.unreconciled.to_count));
        break;
      case REPORT_TYPE_CASHFLOW:
        analysis_remove_account_from_list (file, account, report->template.data.cashflow.accounts,
                                           &(report->template.data.cashflow.accounts_count));
        analysis_remove_account_from_list (file, account, report->template.data.cashflow.incoming,
                                           &(report->template.data.cashflow.incoming_count));
        analysis_remove_account_from_list (file, account, report->template.data.cashflow.outgoing,
                                           &(report->template.data.cashflow.outgoing_count));
        break;
      case REPORT_TYPE_BALANCE:
        analysis_remove_account_from_list (file, account, report->template.data.balance.accounts,
                                           &(report->template.data.balance.accounts_count));
        analysis_remove_account_from_list (file, account, report->template.data.balance.incoming,
                                           &(report->template.data.balance.incoming_count));
        analysis_remove_account_from_list (file, account, report->template.data.balance.outgoing,
                                           &(report->template.data.balance.outgoing_count));
        break;
    }

    report = report->next;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */
/* Remove an account from an account list. */

int analysis_remove_account_from_list (file_data *file, acct_t account, acct_t *array, int *count)
{
  int i, j;

  i = 0;
  j = 0;

  while (i < *count && j < *count)
  {
    /* Skip j on until it finds an account that is to be left in. */
    while (j < *count && array[j] == account)
    {
      j++;
    }

    /* If pointers are different, and not pointing to the account, copy down the account. */
    if (i < j && i < *count && j < *count && array[j] != account)
    {
      array[i] = array[j];
    }

    /* Increment the pointers if necessary */
    if (array[i] != account)
    {
      i++;
      j++;
    }
  }

  *count = i;

  return (i);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Clear all the account report flags in a file. */

void clear_analysis_account_report_flags (file_data *file)
{
  int i;

  for (i=0; i < file->account_count; i++)
  {
    file->accounts[i].report_flags = 0;
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Set the specified report flags for all accounts that match the list given.
 *
 * The special account ident '*' means 'all'.
 */

void set_analysis_account_report_flags_from_list (file_data *file, unsigned type, unsigned flags,
                                                  acct_t *array, int count)
{
  int  account, i;


  for (i=0; i<count; i++)
  {
    account = array[i];

    if (account == NULL_ACCOUNT)
    {
      for (account=0; account < file->account_count; account++)
      {
        if ((file->accounts[account].type & type) != 0)
        {
          file->accounts[account].report_flags |= flags;
        }
      }
    }
    else
    {
      if (account < file->account_count)
      {
        file->accounts[account].report_flags |= flags;
      }
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Convert the account ident list into an array of account numbers.
 *
 * The special account ident '*' means 'all'.
 */

int analysis_convert_account_list_to_array (file_data *file, unsigned type, char *list, acct_t *array)
{
  char *copy, *ident;
  int  account, i;


  copy = (char *) malloc (strlen (list) + 1);
  i = 0;

  if (copy != NULL)
  {
    strcpy (copy, list);

    ident = strtok (copy, ",");

    while (ident != NULL && i < REPORT_ACC_LIST_LEN)
    {
      if (strcmp (ident, "*") == 0)
      {
        array[i++] = NULL_ACCOUNT;
      }
      else
      {
        account = find_account (file, ident, type);

        if (account != NULL_ACCOUNT)
        {
          array[i++] = account;
        }
      }

      ident = strtok (NULL, ",");
    }

    free (copy);
  }

  return (i);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take a comma-separated list of hex numbers, and turn them into an account list array. */

int analysis_convert_account_numbers_to_array (file_data *file, char *list, acct_t *array)
{
  char *copy, *value;
  int  i;


  copy = (char *) malloc (strlen (list) + 1);
  i = 0;

  if (copy != NULL)
  {
    strcpy (copy, list);

    value = strtok (copy, ",");

    while (value != NULL && i < REPORT_ACC_LIST_LEN)
    {
      array[i++] = strtoul (value, NULL, 16);

      value = strtok (NULL, ",");
    }

    free (copy);
  }

  return (i);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Create a string of comma separated hex numbers from an account list array. */

void analysis_convert_account_array_to_numbers (file_data *file, char *list, int size, acct_t *array, int len)
{
  char buffer[32];
  int  i;

  *list = '\0';

  for (i=0; i<len; i++)
  {
    sprintf (buffer, "%x", array[i]);

    if (strlen(list) > 0 && strlen(list)+1 < size)
    {
      strcat(list, ",");
    }

    if (strlen(list) + strlen(buffer) < size)
    {
      strcat(list, buffer);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Convert the account number list into a string of account idents.
 *
 * The special account ident '*' means 'all'.
 */

void analysis_convert_account_array_to_list (file_data *file, char *list, acct_t *array, int len)
{
  char buffer[ACCOUNT_IDENT_LEN];
  int  account, i;

  *list = '\0';

  for (i=0; i<len; i++)
  {
    account = array[i];

    if (account != NULL_ACCOUNT)
    {
      strcpy (buffer, find_account_ident (file, account));
    }
    else
    {
      strcpy (buffer, "*");
    }

    if (strlen(list) > 0 && strlen(list)+1 < REPORT_ACC_SPEC_LEN)
    {
      strcat(list, ",");
    }

    if (strlen(list) + strlen(buffer) < REPORT_ACC_SPEC_LEN)
    {
      strcat(list, buffer);
    }
  }
}

/* ==================================================================================================================
 * Editing Transaction Report via the GUI.
 */

void open_trans_report_window (file_data *file, wimp_pointer *ptr, int template, int clear)
{
  int    template_mode;
  extern global_windows windows;


  /* If the window is already open, another transction report is being edited.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.trans_rep))
  {
    wimp_close_window (windows.trans_rep);
  }

  /* Copy the settings block contents into a static place that won't shift about on the flex heap
   * while the dialogue is open.
   */

  template_mode = (template >= 0 && template < file->saved_report_count);

  if (template_mode)
  {
    analysis_copy_trans_report_template (&(trans_rep_settings), &(file->saved_reports[template].data.transaction));
    trans_rep_template = template;

    msgs_param_lookup ("GenRepTitle", windows_get_indirected_title_addr (windows.trans_rep), 50,
                       file->saved_reports[template].name, NULL, NULL, NULL);

    clear = 1; /* If we use a template, we always want to reset to the template! */
  }
  else
  {
    analysis_copy_trans_report_template (&(trans_rep_settings), &(file->trans_rep));
    trans_rep_template = NULL_TEMPLATE;

    msgs_lookup ("TrnRepTitle", windows_get_indirected_title_addr (windows.trans_rep), 40);
  }

  icons_set_deleted (windows.trans_rep, ANALYSIS_TRANS_DELETE, !template_mode);
  icons_set_deleted (windows.trans_rep, ANALYSIS_TRANS_RENAME, !template_mode);

  /* Set the window contents up. */

  fill_trans_report_window (file, clear);

  /* Set the pointers up so we can find this lot again and open the window. */

  trans_rep_file = file;
  trans_rep_window_clear = clear;

  windows_open_centred_at_pointer (windows.trans_rep, ptr);
  place_dialogue_caret_fallback (windows.trans_rep, 4, ANALYSIS_TRANS_DATEFROM, ANALYSIS_TRANS_DATETO,
                                 ANALYSIS_TRANS_PERIOD, ANALYSIS_TRANS_FROMSPEC);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_trans_report_window (void)
{
  extern global_windows windows;

  fill_trans_report_window (trans_rep_file, trans_rep_window_clear);
  icons_redraw_group (windows.trans_rep, 9,
                          ANALYSIS_TRANS_DATEFROM, ANALYSIS_TRANS_DATETO, ANALYSIS_TRANS_PERIOD,
                          ANALYSIS_TRANS_FROMSPEC, ANALYSIS_TRANS_TOSPEC,
                          ANALYSIS_TRANS_REFSPEC, ANALYSIS_TRANS_DESCSPEC,
                          ANALYSIS_TRANS_AMTLOSPEC, ANALYSIS_TRANS_AMTHISPEC);

  icons_replace_caret_in_window (windows.trans_rep);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_trans_report_window (file_data *file, int clear)
{
  extern global_windows windows;


  if (clear == 0)
  {
    /* Set the period icons. */

    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DATEFROM) = '\0';
    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DATETO) = '\0';

    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_BUDGET, 0);

    /* Set the grouping icons. */

    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_GROUP, 0);

    strcpy(icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_PERIOD), "1");
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_PDAYS, 0);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_PMONTHS, 1);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_PYEARS, 0);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_LOCK, 0);

    /* Set the include icons. */

    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_FROMSPEC) = '\0';
    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_TOSPEC) = '\0';
    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_REFSPEC) = '\0';
    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTLOSPEC) = '\0';
    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTHISPEC) = '\0';
    *icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DESCSPEC) = '\0';

    /* Set the output icons. */

    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_OPTRANS, 1);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_OPSUMMARY, 1);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_OPACCSUMMARY, 1);
  }
  else
  {
    /* Set the period icons. */

    convert_date_to_string(trans_rep_settings.date_from,
                           icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DATEFROM));
    convert_date_to_string(trans_rep_settings.date_to,
                           icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DATETO));

    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_BUDGET, trans_rep_settings.budget);

    /* Set the grouping icons. */

    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_GROUP, trans_rep_settings.group);

    sprintf(icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_PERIOD), "%d", trans_rep_settings.period);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_PDAYS, trans_rep_settings.period_unit == PERIOD_DAYS);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_PMONTHS, trans_rep_settings.period_unit == PERIOD_MONTHS);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_PYEARS, trans_rep_settings.period_unit == PERIOD_YEARS);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_LOCK, trans_rep_settings.lock);

    /* Set the include icons. */

    analysis_convert_account_array_to_list (file, icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_FROMSPEC),
                                            trans_rep_settings.from, trans_rep_settings.from_count);
    analysis_convert_account_array_to_list (file, icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_TOSPEC),
                                            trans_rep_settings.to, trans_rep_settings.to_count);
    strcpy(icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_REFSPEC), trans_rep_settings.ref);
    convert_money_to_string (trans_rep_settings.amount_min,
                             icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTLOSPEC));
    convert_money_to_string (trans_rep_settings.amount_max,
                             icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTHISPEC));
    strcpy(icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DESCSPEC), trans_rep_settings.desc);

    /* Set the output icons. */

    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_OPTRANS, trans_rep_settings.output_trans);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_OPSUMMARY, trans_rep_settings.output_summary);
    icons_set_selected (windows.trans_rep, ANALYSIS_TRANS_OPACCSUMMARY, trans_rep_settings.output_accsummary);
  }

  icons_set_group_shaded_when_on (windows.trans_rep, ANALYSIS_TRANS_BUDGET, 4,
                                  ANALYSIS_TRANS_DATEFROMTXT, ANALYSIS_TRANS_DATEFROM,
                                  ANALYSIS_TRANS_DATETOTXT, ANALYSIS_TRANS_DATETO);

  icons_set_group_shaded_when_off (windows.trans_rep, ANALYSIS_TRANS_GROUP, 6,
                                   ANALYSIS_TRANS_PERIOD, ANALYSIS_TRANS_PTEXT,
                                   ANALYSIS_TRANS_PDAYS, ANALYSIS_TRANS_PMONTHS, ANALYSIS_TRANS_PYEARS,
                                   ANALYSIS_TRANS_LOCK);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_trans_report_window (void)
{
  extern global_windows windows;


  /* Read the date settings. */

  trans_rep_file->trans_rep.date_from =
        convert_string_to_date (icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DATEFROM), NULL_DATE, 0);
  trans_rep_file->trans_rep.date_to =
        convert_string_to_date (icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DATETO), NULL_DATE, 0);
  trans_rep_file->trans_rep.budget = icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_BUDGET);

  /* Read the grouping settings. */

  trans_rep_file->trans_rep.group = icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_GROUP);
  trans_rep_file->trans_rep.period = atoi (icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_PERIOD));

  if (icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_PDAYS))
  {
    trans_rep_file->trans_rep.period_unit = PERIOD_DAYS;
  }
  else if (icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_PMONTHS))
  {
    trans_rep_file->trans_rep.period_unit = PERIOD_MONTHS;
  }
  else if (icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_PYEARS))
  {
    trans_rep_file->trans_rep.period_unit = PERIOD_YEARS;
  }
  else
  {
    trans_rep_file->trans_rep.period_unit = PERIOD_MONTHS;
  }

  trans_rep_file->trans_rep.lock = icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_LOCK);

  /* Read the account and heading settings. */

  trans_rep_file->trans_rep.from_count =
      analysis_convert_account_list_to_array (trans_rep_file, ACCOUNT_FULL | ACCOUNT_IN,
                                              icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_FROMSPEC),
                                              trans_rep_file->trans_rep.from);
  trans_rep_file->trans_rep.to_count =
      analysis_convert_account_list_to_array (trans_rep_file, ACCOUNT_FULL | ACCOUNT_OUT,
                                              icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_TOSPEC),
                                              trans_rep_file->trans_rep.to);
  strcpy(trans_rep_file->trans_rep.ref,
         icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_REFSPEC));
  strcpy(trans_rep_file->trans_rep.desc,
         icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_DESCSPEC));
  trans_rep_file->trans_rep.amount_min = (*icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTLOSPEC) == '\0') ?
           NULL_CURRENCY : convert_string_to_money (icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTLOSPEC));

  trans_rep_file->trans_rep.amount_max = (*icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTHISPEC) == '\0') ?
           NULL_CURRENCY : convert_string_to_money (icons_get_indirected_text_addr (windows.trans_rep, ANALYSIS_TRANS_AMTHISPEC));

  /* Read the output options. */

  trans_rep_file->trans_rep.output_trans = icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_OPTRANS);
  trans_rep_file->trans_rep.output_summary = icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_OPSUMMARY);
  trans_rep_file->trans_rep.output_accsummary = icons_get_selected (windows.trans_rep, ANALYSIS_TRANS_OPACCSUMMARY);

  /* Run the report. */

  generate_transaction_report (trans_rep_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_trans_lookup_window (wimp_i icon)
{
  unsigned              flags;

  extern global_windows windows;

  if (icon == ANALYSIS_TRANS_FROMSPEC)
  {
    flags = ACCOUNT_IN | ACCOUNT_FULL;
  }
  else if (icon == ANALYSIS_TRANS_TOSPEC)
  {
    flags = ACCOUNT_OUT | ACCOUNT_FULL;
  }
  else
  {
    flags = ACCOUNT_NULL;
  }

  open_account_lookup_window (trans_rep_file, windows.trans_rep, icon, NULL_ACCOUNT, flags);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int analysis_delete_trans_report_window (void)
{
  if (trans_rep_template >= 0 && trans_rep_template < trans_rep_file->saved_report_count &&
      error_msgs_report_question ("DeleteTemp", "DeleteTempB") == 1)
  {
    analysis_delete_saved_report_template (trans_rep_file, trans_rep_template);
    trans_rep_template = NULL_TEMPLATE;

    return (0);
  }
  else
  {
    return (1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_rename_trans_report_window (wimp_pointer *ptr)
{
  if (trans_rep_template >= 0 && trans_rep_template < trans_rep_file->saved_report_count)
  {
    analysis_open_rename_report_window (trans_rep_file, trans_rep_template, ptr);
  }
}

/* ==================================================================================================================
 * Editing Unreconciled Report via the GUI.
 */

void open_unrec_report_window (file_data *file, wimp_pointer *ptr, int template, int clear)
{
  int    template_mode;
  extern global_windows windows;


  /* If the window is already open, another unreconciled report is being edited.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.unrec_rep))
  {
    wimp_close_window (windows.unrec_rep);
  }

  /* Copy the settings block contents into a static place that won't shift about on the flex heap
   * while the dialogue is open.
   */

  template_mode = (template >= 0 && template < file->saved_report_count);

  if (template_mode)
  {
    analysis_copy_unrec_report_template (&(unrec_rep_settings), &(file->saved_reports[template].data.unreconciled));
    unrec_rep_template = template;

    msgs_param_lookup ("GenRepTitle", windows_get_indirected_title_addr (windows.unrec_rep), 50,
                       file->saved_reports[template].name, NULL, NULL, NULL);

    clear = 1; /* If we use a template, we always want to reset to the template! */
  }
  else
  {
    analysis_copy_unrec_report_template (&(unrec_rep_settings), &(file->unrec_rep));
    unrec_rep_template = NULL_TEMPLATE;

    msgs_lookup ("UrcRepTitle", windows_get_indirected_title_addr (windows.unrec_rep), 40);
  }

  icons_set_deleted (windows.unrec_rep, ANALYSIS_UNREC_DELETE, !template_mode);
  icons_set_deleted (windows.unrec_rep, ANALYSIS_UNREC_RENAME, !template_mode);

  /* Set the window contents up. */

  fill_unrec_report_window (file, clear);

  /* Set the pointers up so we can find this lot again and open the window. */

  unrec_rep_file = file;
  unrec_rep_window_clear = clear;

  windows_open_centred_at_pointer (windows.unrec_rep, ptr);
  place_dialogue_caret_fallback (windows.unrec_rep, 4, ANALYSIS_UNREC_DATEFROM, ANALYSIS_UNREC_DATETO,
                                 ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_FROMSPEC);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_unrec_report_window (void)
{
  extern global_windows windows;

  fill_unrec_report_window (unrec_rep_file, unrec_rep_window_clear);
  icons_redraw_group (windows.unrec_rep, 5,
                          ANALYSIS_UNREC_DATEFROM, ANALYSIS_UNREC_DATETO, ANALYSIS_UNREC_PERIOD,
                          ANALYSIS_UNREC_FROMSPEC, ANALYSIS_UNREC_TOSPEC);

  icons_replace_caret_in_window (windows.unrec_rep);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_unrec_report_window (file_data *file, int clear)
{
  extern global_windows windows;


  if (clear == 0)
  {
    /* Set the period icons. */

    *icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_DATEFROM) = '\0';
    *icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_DATETO) = '\0';

    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_BUDGET, 0);

    /* Set the grouping icons. */

    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP, 0);

    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPACC, 1);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPDATE, 0);

    strcpy(icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_PERIOD), "1");
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_PDAYS, 0);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_PMONTHS, 1);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_PYEARS, 0);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_LOCK, 0);

    /* Set the from and to spec fields. */

    *icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_FROMSPEC) = '\0';
    *icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_TOSPEC) = '\0';
  }
  else
  {
    /* Set the period icons. */

    convert_date_to_string(unrec_rep_settings.date_from,
                           icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_DATEFROM));
    convert_date_to_string(unrec_rep_settings.date_to,
                           icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_DATETO));

    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_BUDGET, unrec_rep_settings.budget);

    /* Set the grouping icons. */

    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP, unrec_rep_settings.group);

    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPACC, unrec_rep_settings.period_unit == PERIOD_NONE);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPDATE, unrec_rep_settings.period_unit != PERIOD_NONE);

    sprintf(icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_PERIOD), "%d", unrec_rep_settings.period);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_PDAYS, unrec_rep_settings.period_unit == PERIOD_DAYS);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_PMONTHS,
                       unrec_rep_settings.period_unit == PERIOD_MONTHS ||
                       unrec_rep_settings.period_unit == PERIOD_NONE);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_PYEARS, unrec_rep_settings.period_unit == PERIOD_YEARS);
    icons_set_selected (windows.unrec_rep, ANALYSIS_UNREC_LOCK, unrec_rep_settings.lock);

    /* Set the from and to spec fields. */

    analysis_convert_account_array_to_list (file, icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_FROMSPEC),
                                            unrec_rep_settings.from, unrec_rep_settings.from_count);
    analysis_convert_account_array_to_list (file, icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_TOSPEC),
                                            unrec_rep_settings.to, unrec_rep_settings.to_count);
  }

  icons_set_group_shaded_when_on (windows.unrec_rep, ANALYSIS_UNREC_BUDGET, 4,
                                  ANALYSIS_UNREC_DATEFROMTXT, ANALYSIS_UNREC_DATEFROM,
                                  ANALYSIS_UNREC_DATETOTXT, ANALYSIS_UNREC_DATETO);

  icons_set_group_shaded_when_off (windows.unrec_rep, ANALYSIS_UNREC_GROUP, 2,
                                   ANALYSIS_UNREC_GROUPACC, ANALYSIS_UNREC_GROUPDATE);

  icons_set_group_shaded (windows.unrec_rep,
                    !(icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP) &&
                      icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPDATE)),
                    6,
                    ANALYSIS_UNREC_PERIOD, ANALYSIS_UNREC_PTEXT, ANALYSIS_UNREC_LOCK,
                    ANALYSIS_UNREC_PDAYS, ANALYSIS_UNREC_PMONTHS, ANALYSIS_UNREC_PYEARS);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_unrec_report_window (void)
{
  extern global_windows windows;


  /* Read the date settings. */

  unrec_rep_file->unrec_rep.date_from =
        convert_string_to_date (icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_DATEFROM), NULL_DATE, 0);
  unrec_rep_file->unrec_rep.date_to =
        convert_string_to_date (icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_DATETO), NULL_DATE, 0);
  unrec_rep_file->unrec_rep.budget = icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_BUDGET);

  /* Read the grouping settings. */

  unrec_rep_file->unrec_rep.group = icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUP);
  unrec_rep_file->unrec_rep.period = atoi (icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_PERIOD));

  if (icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_GROUPACC))
  {
    unrec_rep_file->unrec_rep.period_unit = PERIOD_NONE;
  }
  else if (icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_PDAYS))
  {
    unrec_rep_file->unrec_rep.period_unit = PERIOD_DAYS;
  }
  else if (icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_PMONTHS))
  {
    unrec_rep_file->unrec_rep.period_unit = PERIOD_MONTHS;
  }
  else if (icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_PYEARS))
  {
    unrec_rep_file->unrec_rep.period_unit = PERIOD_YEARS;
  }
  else
  {
    unrec_rep_file->unrec_rep.period_unit = PERIOD_MONTHS;
  }

  unrec_rep_file->unrec_rep.lock = icons_get_selected (windows.unrec_rep, ANALYSIS_UNREC_LOCK);

  /* Read the account and heading settings. */

  unrec_rep_file->unrec_rep.from_count =
      analysis_convert_account_list_to_array (unrec_rep_file, ACCOUNT_FULL | ACCOUNT_IN,
                                              icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_FROMSPEC),
                                              unrec_rep_file->unrec_rep.from);
  unrec_rep_file->unrec_rep.to_count =
      analysis_convert_account_list_to_array (unrec_rep_file, ACCOUNT_FULL | ACCOUNT_OUT,
                                              icons_get_indirected_text_addr (windows.unrec_rep, ANALYSIS_UNREC_TOSPEC),
                                              unrec_rep_file->unrec_rep.to);

  /* Run the report. */

  generate_unreconciled_report (unrec_rep_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_unrec_lookup_window (wimp_i icon)
{
  unsigned              flags;

  extern global_windows windows;

  if (icon == ANALYSIS_UNREC_FROMSPEC)
  {
    flags = ACCOUNT_IN | ACCOUNT_FULL;
  }
  else if (icon == ANALYSIS_UNREC_TOSPEC)
  {
    flags = ACCOUNT_OUT | ACCOUNT_FULL;
  }
  else
  {
    flags = ACCOUNT_NULL;
  }

  open_account_lookup_window (unrec_rep_file, windows.unrec_rep, icon, NULL_ACCOUNT, flags);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int analysis_delete_unrec_report_window (void)
{
  if (unrec_rep_template >= 0 && unrec_rep_template < unrec_rep_file->saved_report_count &&
      error_msgs_report_question ("DeleteTemp", "DeleteTempB") == 1)
  {
    analysis_delete_saved_report_template (unrec_rep_file, unrec_rep_template);
    unrec_rep_template = NULL_TEMPLATE;

    return (0);
  }
  else
  {
    return (1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_rename_unrec_report_window (wimp_pointer *ptr)
{
  if (unrec_rep_template >= 0 && unrec_rep_template < unrec_rep_file->saved_report_count)
  {
    analysis_open_rename_report_window (unrec_rep_file, unrec_rep_template, ptr);
  }
}

/* ==================================================================================================================
 * Editing Cashflow Report via the GUI.
 */

void open_cashflow_report_window (file_data *file, wimp_pointer *ptr, int template, int clear)
{
  int    template_mode;
  extern global_windows windows;


  /* If the window is already open, another cashflow report is being edited.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.cashflow_rep))
  {
    wimp_close_window (windows.cashflow_rep);
  }

  /* Copy the settings block contents into a static place that won't shift about on the flex heap
   * while the dialogue is open.
   */

  template_mode = (template >= 0 && template < file->saved_report_count);

  if (template_mode)
  {
    analysis_copy_cashflow_report_template (&(cashflow_rep_settings), &(file->saved_reports[template].data.cashflow));
    cashflow_rep_template = template;

    msgs_param_lookup ("GenRepTitle", windows_get_indirected_title_addr (windows.cashflow_rep), 50,
                       file->saved_reports[template].name, NULL, NULL, NULL);

    clear = 1; /* If we use a template, we always want to reset to the template! */
  }
  else
  {
    analysis_copy_cashflow_report_template (&(cashflow_rep_settings), &(file->cashflow_rep));
    cashflow_rep_template = NULL_TEMPLATE;

    msgs_lookup ("CflRepTitle", windows_get_indirected_title_addr (windows.cashflow_rep), 40);
  }

  icons_set_deleted (windows.cashflow_rep, ANALYSIS_CASHFLOW_DELETE, !template_mode);
  icons_set_deleted (windows.cashflow_rep, ANALYSIS_CASHFLOW_RENAME, !template_mode);

  /* Set the window contents up. */

  fill_cashflow_report_window (file, clear);

  /* Set the pointers up so we can find this lot again and open the window. */

  cashflow_rep_file = file;
  cashflow_rep_window_clear = clear;

  windows_open_centred_at_pointer (windows.cashflow_rep, ptr);
  place_dialogue_caret_fallback (windows.cashflow_rep, 4, ANALYSIS_CASHFLOW_DATEFROM, ANALYSIS_CASHFLOW_DATETO,
                                 ANALYSIS_CASHFLOW_PERIOD, ANALYSIS_CASHFLOW_ACCOUNTS);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_cashflow_report_window (void)
{
  extern global_windows windows;

  fill_cashflow_report_window (cashflow_rep_file, cashflow_rep_window_clear);
  icons_redraw_group (windows.cashflow_rep, 6,
                          ANALYSIS_CASHFLOW_DATEFROM, ANALYSIS_CASHFLOW_DATETO, ANALYSIS_CASHFLOW_PERIOD,
                          ANALYSIS_CASHFLOW_ACCOUNTS, ANALYSIS_CASHFLOW_INCOMING, ANALYSIS_CASHFLOW_OUTGOING);

  icons_replace_caret_in_window (windows.cashflow_rep);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_cashflow_report_window (file_data *file, int clear)
{
  extern global_windows windows;


  if (clear == 0)
  {
    /* Set the period icons. */

    *icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_DATEFROM) = '\0';
    *icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_DATETO) = '\0';

    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_BUDGET, 0);

    /* Set the grouping icons. */

    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_GROUP, 0);

    strcpy(icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_PERIOD), "1");
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PDAYS, 0);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PMONTHS, 1);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PYEARS, 0);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_LOCK, 0);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_EMPTY, 0);

    /* Set the accounts and format details. */

    *icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_ACCOUNTS) = '\0';
    *icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_INCOMING) = '\0';
    *icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_OUTGOING) = '\0';

    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_TABULAR, 0);
  }
  else
  {
    /* Set the period icons. */

    convert_date_to_string (cashflow_rep_settings.date_from,
                            icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_DATEFROM));
    convert_date_to_string (cashflow_rep_settings.date_to,
                            icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_DATETO));
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_BUDGET, cashflow_rep_settings.budget);

    /* Set the grouping icons. */

    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_GROUP, cashflow_rep_settings.group);

    sprintf(icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_PERIOD), "%d", cashflow_rep_settings.period);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PDAYS, cashflow_rep_settings.period_unit == PERIOD_DAYS);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PMONTHS,
                       cashflow_rep_settings.period_unit == PERIOD_MONTHS);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PYEARS,
                       cashflow_rep_settings.period_unit == PERIOD_YEARS);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_LOCK, cashflow_rep_settings.lock);
    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_EMPTY, cashflow_rep_settings.empty);

    /* Set the accounts and format details. */

    analysis_convert_account_array_to_list (file,
                                            icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_ACCOUNTS),
                                            cashflow_rep_settings.accounts, cashflow_rep_settings.accounts_count);
    analysis_convert_account_array_to_list (file,
                                            icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_INCOMING),
                                            cashflow_rep_settings.incoming, cashflow_rep_settings.incoming_count);
    analysis_convert_account_array_to_list (file,
                                            icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_OUTGOING),
                                            cashflow_rep_settings.outgoing, cashflow_rep_settings.outgoing_count);

    icons_set_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_TABULAR, cashflow_rep_settings.tabular);
  }

  icons_set_group_shaded_when_on (windows.cashflow_rep, ANALYSIS_CASHFLOW_BUDGET, 4,
                                  ANALYSIS_CASHFLOW_DATEFROMTXT, ANALYSIS_CASHFLOW_DATEFROM,
                                  ANALYSIS_CASHFLOW_DATETOTXT, ANALYSIS_CASHFLOW_DATETO);

  icons_set_group_shaded_when_off (windows.cashflow_rep, ANALYSIS_CASHFLOW_GROUP, 7,
                                   ANALYSIS_CASHFLOW_PERIOD, ANALYSIS_CASHFLOW_PTEXT, ANALYSIS_CASHFLOW_LOCK,
                                   ANALYSIS_CASHFLOW_PDAYS, ANALYSIS_CASHFLOW_PMONTHS, ANALYSIS_CASHFLOW_PYEARS,
                                   ANALYSIS_CASHFLOW_EMPTY);

}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_cashflow_report_window (void)
{
  extern global_windows windows;


  /* Read the date settings. */

  cashflow_rep_file->cashflow_rep.date_from =
        convert_string_to_date (icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_DATEFROM), NULL_DATE, 0);
  cashflow_rep_file->cashflow_rep.date_to =
        convert_string_to_date (icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_DATETO), NULL_DATE, 0);
  cashflow_rep_file->cashflow_rep.budget = icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_BUDGET);

  /* Read the grouping settings. */

  cashflow_rep_file->cashflow_rep.group = icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_GROUP);
  cashflow_rep_file->cashflow_rep.period = atoi (icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_PERIOD));

  if (icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PDAYS))
  {
    cashflow_rep_file->cashflow_rep.period_unit = PERIOD_DAYS;
  }
  else if (icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PMONTHS))
  {
    cashflow_rep_file->cashflow_rep.period_unit = PERIOD_MONTHS;
  }
  else if (icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_PYEARS))
  {
    cashflow_rep_file->cashflow_rep.period_unit = PERIOD_YEARS;
  }
  else
  {
    cashflow_rep_file->cashflow_rep.period_unit = PERIOD_MONTHS;
  }

  cashflow_rep_file->cashflow_rep.lock = icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_LOCK);
  cashflow_rep_file->cashflow_rep.empty = icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_EMPTY);

  /* Read the account and heading settings. */

  cashflow_rep_file->cashflow_rep.accounts_count =
      analysis_convert_account_list_to_array (cashflow_rep_file, ACCOUNT_FULL,
                                              icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_ACCOUNTS),
                                              cashflow_rep_file->cashflow_rep.accounts);
  cashflow_rep_file->cashflow_rep.incoming_count =
      analysis_convert_account_list_to_array (cashflow_rep_file, ACCOUNT_IN,
                                              icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_INCOMING),
                                              cashflow_rep_file->cashflow_rep.incoming);
  cashflow_rep_file->cashflow_rep.outgoing_count =
      analysis_convert_account_list_to_array (cashflow_rep_file, ACCOUNT_OUT,
                                              icons_get_indirected_text_addr (windows.cashflow_rep, ANALYSIS_CASHFLOW_OUTGOING),
                                              cashflow_rep_file->cashflow_rep.outgoing);

  cashflow_rep_file->cashflow_rep.tabular = icons_get_selected (windows.cashflow_rep, ANALYSIS_CASHFLOW_TABULAR);

  /* Run the report. */

  generate_cashflow_report (cashflow_rep_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_cashflow_lookup_window (wimp_i icon)
{
  unsigned              flags;

  extern global_windows windows;

  if (icon == ANALYSIS_CASHFLOW_ACCOUNTS)
  {
    flags = ACCOUNT_FULL;
  }
  else if (icon == ANALYSIS_CASHFLOW_INCOMING)
  {
    flags = ACCOUNT_IN;
  }
  else if (icon == ANALYSIS_CASHFLOW_OUTGOING)
  {
    flags = ACCOUNT_OUT;
  }
  else
  {
    flags = ACCOUNT_NULL;
  }

  open_account_lookup_window (cashflow_rep_file, windows.cashflow_rep, icon, NULL_ACCOUNT, flags);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int analysis_delete_cashflow_report_window (void)
{
  if (cashflow_rep_template >= 0 && cashflow_rep_template < cashflow_rep_file->saved_report_count &&
      error_msgs_report_question ("DeleteTemp", "DeleteTempB") == 1)
  {
    analysis_delete_saved_report_template (cashflow_rep_file, cashflow_rep_template);
    cashflow_rep_template = NULL_TEMPLATE;

    return (0);
  }
  else
  {
    return (1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_rename_cashflow_report_window (wimp_pointer *ptr)
{
  if (cashflow_rep_template >= 0 && cashflow_rep_template < cashflow_rep_file->saved_report_count)
  {
    analysis_open_rename_report_window (cashflow_rep_file, cashflow_rep_template, ptr);
  }
}

/* ==================================================================================================================
 * Editing Balance Report via the GUI.
 */

void open_balance_report_window (file_data *file, wimp_pointer *ptr, int template, int clear)
{
  int    template_mode;
  extern global_windows windows;


  /* If the window is already open, another balance report is being edited.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.balance_rep))
  {
    wimp_close_window (windows.balance_rep);
  }

  /* Copy the settings block contents into a static place that won't shift about on the flex heap
   * while the dialogue is open.
   */

  template_mode = (template >= 0 && template < file->saved_report_count);

  if (template_mode)
  {
    analysis_copy_balance_report_template (&(balance_rep_settings), &(file->saved_reports[template].data.balance));
    balance_rep_template = template;

    msgs_param_lookup ("GenRepTitle", windows_get_indirected_title_addr (windows.balance_rep), 50,
                       file->saved_reports[template].name, NULL, NULL, NULL);

    clear = 1; /* If we use a template, we always want to reset to the template! */
  }
  else
  {
    analysis_copy_balance_report_template (&(balance_rep_settings), &(file->balance_rep));
    balance_rep_template = NULL_TEMPLATE;

    msgs_lookup ("BalRepTitle", windows_get_indirected_title_addr (windows.balance_rep), 40);
  }

  icons_set_deleted (windows.balance_rep, ANALYSIS_BALANCE_DELETE, !template_mode);
  icons_set_deleted (windows.balance_rep, ANALYSIS_BALANCE_RENAME, !template_mode);

  /* Set the window contents up. */

  fill_balance_report_window (file, clear);

  /* Set the pointers up so we can find this lot again and open the window. */

  balance_rep_file = file;
  balance_rep_window_clear = clear;

  windows_open_centred_at_pointer (windows.balance_rep, ptr);
  place_dialogue_caret_fallback (windows.balance_rep, 4, ANALYSIS_BALANCE_DATEFROM, ANALYSIS_BALANCE_DATETO,
                                 ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_ACCOUNTS);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_balance_report_window (void)
{
  extern global_windows windows;

  fill_balance_report_window (balance_rep_file, balance_rep_window_clear);
  icons_redraw_group (windows.balance_rep, 6,
                          ANALYSIS_BALANCE_DATEFROM, ANALYSIS_BALANCE_DATETO, ANALYSIS_BALANCE_PERIOD,
                          ANALYSIS_BALANCE_ACCOUNTS, ANALYSIS_BALANCE_INCOMING, ANALYSIS_BALANCE_OUTGOING);

  icons_replace_caret_in_window (windows.balance_rep);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_balance_report_window (file_data *file, int clear)
{
  extern global_windows windows;


  if (clear == 0)
  {
    /* Set the period icons. */

    *icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_DATEFROM) = '\0';
    *icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_DATETO) = '\0';

    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_BUDGET, 0);

    /* Set the grouping icons. */

    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_GROUP, 0);

    strcpy(icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_PERIOD), "1");
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_PDAYS, 0);
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_PMONTHS, 1);
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_PYEARS, 0);
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_LOCK, 0);

    /* Set the accounts and format details. */

    *icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_ACCOUNTS) = '\0';
    *icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_INCOMING) = '\0';
    *icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_OUTGOING) = '\0';

    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_TABULAR, 0);
  }
  else
  {
    /* Set the period icons. */

    convert_date_to_string (balance_rep_settings.date_from,
                            icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_DATEFROM));
    convert_date_to_string (balance_rep_settings.date_to,
                            icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_DATETO));
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_BUDGET, balance_rep_settings.budget);

    /* Set the grouping icons. */

    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_GROUP, balance_rep_settings.group);

    sprintf(icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_PERIOD), "%d", balance_rep_settings.period);
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_PDAYS, balance_rep_settings.period_unit == PERIOD_DAYS);
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_PMONTHS,
                       balance_rep_settings.period_unit == PERIOD_MONTHS);
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_PYEARS, balance_rep_settings.period_unit == PERIOD_YEARS);
    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_LOCK, balance_rep_settings.lock);

    /* Set the accounts and format details. */

    analysis_convert_account_array_to_list (file,
                                            icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_ACCOUNTS),
                                            balance_rep_settings.accounts, balance_rep_settings.accounts_count);
    analysis_convert_account_array_to_list (file,
                                            icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_INCOMING),
                                            balance_rep_settings.incoming, balance_rep_settings.incoming_count);
    analysis_convert_account_array_to_list (file,
                                            icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_OUTGOING),
                                            balance_rep_settings.outgoing, balance_rep_settings.outgoing_count);

    icons_set_selected (windows.balance_rep, ANALYSIS_BALANCE_TABULAR, balance_rep_settings.tabular);
  }

  icons_set_group_shaded_when_on (windows.balance_rep, ANALYSIS_BALANCE_BUDGET, 4,
                                  ANALYSIS_BALANCE_DATEFROMTXT, ANALYSIS_BALANCE_DATEFROM,
                                  ANALYSIS_BALANCE_DATETOTXT, ANALYSIS_BALANCE_DATETO);

  icons_set_group_shaded_when_off (windows.balance_rep, ANALYSIS_BALANCE_GROUP, 6,
                                   ANALYSIS_BALANCE_PERIOD, ANALYSIS_BALANCE_PTEXT, ANALYSIS_BALANCE_LOCK,
                                   ANALYSIS_BALANCE_PDAYS, ANALYSIS_BALANCE_PMONTHS, ANALYSIS_BALANCE_PYEARS);

}

/* ------------------------------------------------------------------------------------------------------------------ */

int process_balance_report_window (void)
{
  extern global_windows windows;


  /* Read the date settings. */

  balance_rep_file->balance_rep.date_from =
        convert_string_to_date (icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_DATEFROM), NULL_DATE, 0);
  balance_rep_file->balance_rep.date_to =
        convert_string_to_date (icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_DATETO), NULL_DATE, 0);
  balance_rep_file->balance_rep.budget = icons_get_selected (windows.balance_rep, ANALYSIS_BALANCE_BUDGET);

  /* Read the grouping settings. */

  balance_rep_file->balance_rep.group = icons_get_selected (windows.balance_rep, ANALYSIS_BALANCE_GROUP);
  balance_rep_file->balance_rep.period = atoi (icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_PERIOD));

  if (icons_get_selected (windows.balance_rep, ANALYSIS_BALANCE_PDAYS))
  {
    balance_rep_file->balance_rep.period_unit = PERIOD_DAYS;
  }
  else if (icons_get_selected (windows.balance_rep, ANALYSIS_BALANCE_PMONTHS))
  {
    balance_rep_file->balance_rep.period_unit = PERIOD_MONTHS;
  }
  else if (icons_get_selected (windows.balance_rep, ANALYSIS_BALANCE_PYEARS))
  {
    balance_rep_file->balance_rep.period_unit = PERIOD_YEARS;
  }
  else
  {
    balance_rep_file->balance_rep.period_unit = PERIOD_MONTHS;
  }

  balance_rep_file->balance_rep.lock = icons_get_selected (windows.balance_rep, ANALYSIS_BALANCE_LOCK);

  /* Read the account and heading settings. */

  balance_rep_file->balance_rep.accounts_count =
      analysis_convert_account_list_to_array (balance_rep_file, ACCOUNT_FULL,
                                              icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_ACCOUNTS),
                                              balance_rep_file->balance_rep.accounts);
  balance_rep_file->balance_rep.incoming_count =
      analysis_convert_account_list_to_array (balance_rep_file, ACCOUNT_IN,
                                              icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_INCOMING),
                                              balance_rep_file->balance_rep.incoming);
  balance_rep_file->balance_rep.outgoing_count =
      analysis_convert_account_list_to_array (balance_rep_file, ACCOUNT_OUT,
                                              icons_get_indirected_text_addr (windows.balance_rep, ANALYSIS_BALANCE_OUTGOING),
                                              balance_rep_file->balance_rep.outgoing);

  balance_rep_file->balance_rep.tabular = icons_get_selected (windows.balance_rep, ANALYSIS_BALANCE_TABULAR);

  /* Run the report. */

  generate_balance_report (balance_rep_file);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_balance_lookup_window (wimp_i icon)
{
  unsigned              flags;

  extern global_windows windows;

  if (icon == ANALYSIS_BALANCE_ACCOUNTS)
  {
    flags = ACCOUNT_FULL;
  }
  else if (icon == ANALYSIS_BALANCE_INCOMING)
  {
    flags = ACCOUNT_IN;
  }
  else if (icon == ANALYSIS_BALANCE_OUTGOING)
  {
    flags = ACCOUNT_OUT;
  }
  else
  {
    flags = ACCOUNT_NULL;
  }

  open_account_lookup_window (balance_rep_file, windows.balance_rep, icon, NULL_ACCOUNT, flags);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int analysis_delete_balance_report_window (void)
{
  if (balance_rep_template >= 0 && balance_rep_template < balance_rep_file->saved_report_count &&
      error_msgs_report_question ("DeleteTemp", "DeleteTempB") == 1)
  {
    analysis_delete_saved_report_template (balance_rep_file, balance_rep_template);
    balance_rep_template = NULL_TEMPLATE;

    return (0);
  }
  else
  {
    return (1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_rename_balance_report_window (wimp_pointer *ptr)
{
  if (balance_rep_template >= 0 && balance_rep_template < balance_rep_file->saved_report_count)
  {
    analysis_open_rename_report_window (balance_rep_file, balance_rep_template, ptr);
  }
}

/* ==================================================================================================================
 * Saving and Renaming Report Templates via the GUI.
 */

void open_save_report_window (file_data *file, report_data *report, wimp_pointer *ptr)
{
  extern global_windows windows;


  /* If the window is already open, another report is being saved.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.save_rep))
  {
    wimp_close_window (windows.save_rep);
  }

  /* Set the window contents up. */

  msgs_lookup ("SaveRepTitle", windows_get_indirected_title_addr (windows.save_rep), 20);
  msgs_lookup ("SaveRepSave", icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_OK), 10);

  /* The popup can be shaded here, as the only way its state can be changed is if a report is added: which
   * can only be done via this dislogue.  In the (unlikely) event that the Save dialogue is open when the last
   * report is deleted, then the popup remains active but no memu will appear...
   */

  icons_set_shaded (windows.save_rep, ANALYSIS_SAVE_NAMEPOPUP, file->saved_report_count == 0);

  fill_save_report_window (report);

  ihelp_set_modifier (windows.save_rep, "Sav");

  /* Set the pointers up so we can find this lot again and open the window. */

  save_report_file = file;
  save_report_report = report;
  save_report_mode = ANALYSIS_SAVE_MODE_SAVE;

  windows_open_centred_at_pointer (windows.save_rep, ptr);
  place_dialogue_caret_fallback (windows.save_rep, 1, ANALYSIS_SAVE_NAME);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_open_rename_report_window (file_data *file, int template, wimp_pointer *ptr)
{
  extern global_windows windows;


  /* If the window is already open, another report is being saved.  Assume the user wants to lose
   * any unsaved data and just close the window.
   *
   * We don't use the close_dialogue_with_caret () as the caret is just moving from one dialogue to another.
   */

  if (windows_get_open (windows.save_rep))
  {
    wimp_close_window (windows.save_rep);
  }

  /* Set the window contents up. */

  msgs_lookup ("RenRepTitle", windows_get_indirected_title_addr (windows.save_rep), 20);
  msgs_lookup ("RenRepRen", icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_OK), 10);

  /* The popup can be shaded here, as the only way its state can be changed is if a report is added: which
   * can only be done via this dislogue.  In the (unlikely) event that the Save dialogue is open when the last
   * report is deleted, then the popup remains active but no memu will appear...
   */

  icons_set_shaded (windows.save_rep, ANALYSIS_SAVE_NAMEPOPUP, file->saved_report_count == 0);

  analysis_fill_rename_report_window (file, template);

  ihelp_set_modifier (windows.save_rep, "Ren");

  /* Set the pointers up so we can find this lot again and open the window. */

  save_report_file = file;
  save_report_template = template;
  save_report_mode = ANALYSIS_SAVE_MODE_RENAME;

  windows_open_centred_at_pointer (windows.save_rep, ptr);
  place_dialogue_caret_fallback (windows.save_rep, 1, ANALYSIS_SAVE_NAME);
}


/* ------------------------------------------------------------------------------------------------------------------ */

void refresh_save_report_window (void)
{
  extern global_windows windows;

  switch (save_report_mode)
  {
    case ANALYSIS_SAVE_MODE_SAVE:
      fill_save_report_window (save_report_report);
      break;

    case ANALYSIS_SAVE_MODE_RENAME:
      analysis_fill_rename_report_window (save_report_file, save_report_template);
      break;
  }
  icons_redraw_group (windows.save_rep, 1, ANALYSIS_SAVE_NAME);

  icons_replace_caret_in_window (windows.save_rep);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void fill_save_report_window (report_data *report)
{
  extern global_windows windows;


  /* Set the name icons. */

  strcpy (icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_NAME), report->template.name);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_fill_rename_report_window (file_data *file, int template)
{
  extern global_windows windows;


  /* Set the name icons. */

  strcpy (icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_NAME), file->saved_reports[template].name);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Process OK clicks in the save report window.  If it is a real save, pass the call on to the store saved report
 * function.  If it is a rename, handle it directly here.
 */

int process_save_report_window (void)
{
  int                   template;
  wimp_w                w;


  extern global_windows windows;

  if (*icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_NAME) == '\0')
  {
    return (1);
  }

  template = analysis_find_saved_report_template_from_name (save_report_file,
                   icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_NAME));

  switch(save_report_mode)
  {
    case ANALYSIS_SAVE_MODE_SAVE:
      if (template != NULL_TEMPLATE && error_msgs_report_question ("CheckTempOvr", "CheckTempOvrB") == 2)
      {
        return (1);
      }

      strcpy(save_report_report->template.name, icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_NAME));

      analysis_store_saved_report_template (save_report_file, &(save_report_report->template), template);
      break;

    case ANALYSIS_SAVE_MODE_RENAME:
      if (save_report_template != NULL_TEMPLATE)
      {
        if (template != NULL_TEMPLATE && template != save_report_template)
        {
          error_msgs_report_error ("TempExists");
          return (1);
        }

        strcpy(save_report_file->saved_reports[save_report_template].name,
               icons_get_indirected_text_addr (windows.save_rep, ANALYSIS_SAVE_NAME));

        /* Update the window title of the parent report dialogue. */

        switch (save_report_file->saved_reports[save_report_template].type)
        {
          case REPORT_TYPE_TRANS:
            w = windows.trans_rep;
            break;
          case REPORT_TYPE_UNREC:
            w = windows.unrec_rep;
            break;
          case REPORT_TYPE_CASHFLOW:
            w = windows.cashflow_rep;
            break;
          case REPORT_TYPE_BALANCE:
            w = windows.balance_rep;
            break;
          default:
            w = NULL;
            break;
        }

        if (w != NULL)
        {
          msgs_param_lookup ("GenRepTitle", windows_get_indirected_title_addr (w), 50,
                             save_report_file->saved_reports[save_report_template].name, NULL, NULL, NULL);
          xwimp_force_redraw_title (w); /* Nested Wimp only! */
        }

        /* Mark the file has being modified. */

        set_file_data_integrity (save_report_file, 1);
      }
      break;
  }

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_open_save_report_popup_menu (wimp_pointer *ptr)
{
  mainmenu_open_replist_menu (save_report_file, ptr);
}

/* ==================================================================================================================
 * Force the closure of the report format window if the file disappears.
 */

void force_close_report_windows (file_data *file)
{
  extern global_windows windows;


  if (trans_rep_file == file && windows_get_open (windows.trans_rep))
  {
    close_dialogue_with_caret (windows.trans_rep);
  }

  if (unrec_rep_file == file && windows_get_open (windows.unrec_rep))
  {
    close_dialogue_with_caret (windows.unrec_rep);
  }

  if (cashflow_rep_file == file && windows_get_open (windows.cashflow_rep))
  {
    close_dialogue_with_caret (windows.cashflow_rep);
  }

  if (balance_rep_file == file && windows_get_open (windows.balance_rep))
  {
    close_dialogue_with_caret (windows.balance_rep);
  }

  if (save_report_file == file && windows_get_open (windows.save_rep))
  {
    close_dialogue_with_caret (windows.save_rep);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_force_close_report_save_window (file_data *file, report_data *report)
{
  extern global_windows windows;


  if (save_report_mode == ANALYSIS_SAVE_MODE_SAVE &&
      save_report_file == file && save_report_report == report && windows_get_open (windows.save_rep))
  {
    close_dialogue_with_caret (windows.save_rep);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_force_close_report_rename_window (wimp_w window)
{
  extern global_windows windows;


  if (windows_get_open (windows.save_rep) &&
      save_report_mode == ANALYSIS_SAVE_MODE_RENAME && save_report_template != NULL_TEMPLATE)
  {
    if ((window == windows.trans_rep &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_TRANS) ||
        (window == windows.unrec_rep &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_UNREC) ||
        (window == windows.cashflow_rep &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_CASHFLOW) ||
        (window == windows.balance_rep &&
         save_report_file->saved_reports[save_report_template].type == REPORT_TYPE_BALANCE))
    {
      close_dialogue_with_caret (windows.save_rep);
    }
  }
}

/* ==================================================================================================================
 * Saved template handling.
 */

/* Open a report from a saved template. */

void analysis_open_saved_report_dialogue(file_data *file, wimp_pointer *ptr, int template)
{
  if (template >= 0 && template < file->saved_report_count)
  {
    switch (file->saved_reports[template].type)
    {
      case REPORT_TYPE_TRANS:
        open_trans_report_window (file, ptr, template, config_opt_read ("RememberValues"));
        break;

      case REPORT_TYPE_UNREC:
        open_unrec_report_window (file, ptr, template, config_opt_read ("RememberValues"));
        break;

      case REPORT_TYPE_CASHFLOW:
        open_cashflow_report_window (file, ptr, template, config_opt_read ("RememberValues"));
        break;

      case REPORT_TYPE_BALANCE:
        open_balance_report_window (file, ptr, template, config_opt_read ("RememberValues"));
        break;
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */
/* Find a saved template based on its name. */

int analysis_find_saved_report_template_from_name (file_data *file, char *name)
{
  int i, found = -1;

  if (file != NULL && file->saved_report_count > 0)
  {
    for (i=0; i<file->saved_report_count && found == -1; i++)
    {
      if (string_nocase_strcmp (file->saved_reports[i].name, name) == 0)
      {
        found = i;
      }
    }
  }

  return (found);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_store_saved_report_template (file_data *file, saved_report *report, int number)
{

  if (number == NULL_TEMPLATE)
  {
    if (flex_extend ((flex_ptr) &(file->saved_reports), sizeof (saved_report) * (file->saved_report_count+1)) == 1)
    {
      number = file->saved_report_count++;
    }
    else
    {
      error_msgs_report_error ("NoMemNewTemp");
    }
  }

  if (number >= 0 && number < file->saved_report_count)
  {
    analysis_copy_saved_report_template(&(file->saved_reports[number]), report);
    set_file_data_integrity (file, 1);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void analysis_delete_saved_report_template (file_data *file, int template)
{
  int type;

  extern global_windows windows;


  /* delete the specified template. */

  if (template >= 0 && template < file->saved_report_count)
  {
    type = file->saved_reports[template].type;

    /* first remove the template from the flex block. */

    flex_midextend ((flex_ptr) &(file->saved_reports), (template + 1) * sizeof (saved_report), -sizeof (saved_report));
    file->saved_report_count--;
    set_file_data_integrity (file, 1);

    /* If the rename template window is open for this template, close it now before the pointer is lost. */

    if (windows_get_open (windows.save_rep) && save_report_mode == ANALYSIS_SAVE_MODE_RENAME &&
        template == save_report_template)
    {
      close_dialogue_with_caret (windows.save_rep);
    }

    /* Now adjust any other template pointers, which may be pointing further up the array.
     * In addition, if the rename template pointer (save_report_template) is pointing to the deleted
     * item, it needs to be unset.
     */

    if (type != REPORT_TYPE_TRANS && trans_rep_template > template)
    {
      trans_rep_template--;
    }
    if (type != REPORT_TYPE_UNREC && unrec_rep_template > template)
    {
      unrec_rep_template--;
    }
    if (type != REPORT_TYPE_CASHFLOW && cashflow_rep_template > template)
    {
      cashflow_rep_template--;
    }
    if (type != REPORT_TYPE_BALANCE && balance_rep_template > template)
    {
      balance_rep_template--;
    }
    if (save_report_template > template)
    {
      save_report_template--;
    }
    else if (save_report_template == template)
    {
      save_report_template = NULL_TEMPLATE;
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */
/* Copy a saved report template from one place to another. */

void analysis_copy_saved_report_template(saved_report *to, saved_report *from)
{
  if (from != NULL && to != NULL)
  {
    strcpy (to->name, from->name);
    to->type = from->type;

    switch (from->type)
    {
      case REPORT_TYPE_TRANS:
        analysis_copy_trans_report_template(&(to->data.transaction), &(from->data.transaction));
        break;

      case REPORT_TYPE_UNREC:
        analysis_copy_unrec_report_template(&(to->data.unreconciled), &(from->data.unreconciled));
        break;

      case REPORT_TYPE_CASHFLOW:
        analysis_copy_cashflow_report_template(&(to->data.cashflow), &(from->data.cashflow));
        break;

      case REPORT_TYPE_BALANCE:
        analysis_copy_balance_report_template(&(to->data.balance), &(from->data.balance));
        break;

    }
  }
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Copy a transation report definition from one place to another. */

void analysis_copy_trans_report_template(trans_rep *to, trans_rep *from)
{
  int i;

  if (from != NULL && to != NULL)
  {
    to->date_from = from->date_from;
    to->date_to = from->date_to;
    to->budget = from->budget;

    to->group = from->group;
    to->period = from->period;
    to->period_unit = from->period_unit;
    to->lock = from->lock;

    to->from_count = from->from_count;
    for (i=0; i<from->from_count; i++)
    {
      to->from[i] = from->from[i];
    }
    to->to_count = from->to_count;
    for (i=0; i<from->to_count; i++)
    {
      to->to[i] = from->to[i];
    }
    strcpy (to->ref, from->ref);
    strcpy (to->desc, from->desc);
    to->amount_min = from->amount_min;
    to->amount_max = from->amount_max;

    to->output_trans = from->output_trans;
    to->output_summary = from->output_summary;
    to->output_accsummary = from->output_accsummary;
  }
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Copy an unreconciled transaction report definition from one place to another. */

void analysis_copy_unrec_report_template(unrec_rep *to, unrec_rep *from)
{
  int i;

  if (from != NULL && to != NULL)
  {
    to->date_from = from->date_from;
    to->date_to = from->date_to;
    to->budget = from->budget;

    to->group = from->group;
    to->period = from->period;
    to->period_unit = from->period_unit;
    to->lock = from->lock;

    to->from_count = from->from_count;
    for (i=0; i<from->from_count; i++)
    {
      to->from[i] = from->from[i];
    }
    to->to_count = from->to_count;
    for (i=0; i<from->to_count; i++)
    {
      to->to[i] = from->to[i];
    }
  }
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Copy a cashflow report definition from one place to another. */

void analysis_copy_cashflow_report_template(cashflow_rep *to, cashflow_rep *from)
{
  int i;

  if (from != NULL && to != NULL)
  {
    to->date_from = from->date_from;
    to->date_to = from->date_to;
    to->budget = from->budget;

    to->group = from->group;
    to->period = from->period;
    to->period_unit = from->period_unit;
    to->lock = from->lock;
    to->empty = from->empty;

    to->accounts_count = from->accounts_count;
    for (i=0; i<from->accounts_count; i++)
    {
      to->accounts[i] = from->accounts[i];
    }
    to->incoming_count = from->incoming_count;
    for (i=0; i<from->incoming_count; i++)
    {
      to->incoming[i] = from->incoming[i];
    }
    to->outgoing_count = from->outgoing_count;
    for (i=0; i<from->outgoing_count; i++)
    {
      to->outgoing[i] = from->outgoing[i];
    }

    to->tabular = from->tabular;
  }
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Copy a balance report definition from one place to another. */

void analysis_copy_balance_report_template(balance_rep *to, balance_rep *from)
{
  int i;

  if (from != NULL && to != NULL)
  {
    to->date_from = from->date_from;
    to->date_to = from->date_to;
    to->budget = from->budget;

    to->group = from->group;
    to->period = from->period;
    to->period_unit = from->period_unit;
    to->lock = from->lock;

    to->accounts_count = from->accounts_count;
    for (i=0; i<from->accounts_count; i++)
    {
      to->accounts[i] = from->accounts[i];
    }
    to->incoming_count = from->incoming_count;
    for (i=0; i<from->incoming_count; i++)
    {
      to->incoming[i] = from->incoming[i];
    }
    to->outgoing_count = from->outgoing_count;
    for (i=0; i<from->outgoing_count; i++)
    {
      to->outgoing[i] = from->outgoing[i];
    }

    to->tabular = from->tabular;
  }
}
