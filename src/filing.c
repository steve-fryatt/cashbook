/* CashBook - filing.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* ANSI C header files */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/string.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "filing.h"

#include "account.h"
#include "accview.h"
#include "analysis.h"
#include "calculation.h"
#include "column.h"
#include "conversion.h"
#include "dataxfer.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "presets.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"
#include "window.h"

/* ==================================================================================================================
 * Global Variables
 */

static file_data *import_window_file = NULL;


/* ==================================================================================================================
 * Account file loading
 */

void load_transaction_file (char *filename)
{
  int       i=0, j=0, type, entry=0, result, sect_num = LOAD_SECT_NONE, block_size=0, unknown_data=0;
  char      section[MAX_FILE_LINE_LEN], token[MAX_FILE_LINE_LEN], value[MAX_FILE_LINE_LEN], *sect_sup;
  bits      load;
  file_data *file;
  FILE      *in;


  #ifdef DEBUG
  debug_printf ("\\BLoading accounts file");
  #endif

  file = build_new_file_block ();

  if (file != NULL)
  {
    hourglass_on ();

    in = fopen (filename, "r");

    if (in != NULL)
    {
      /* Find the next data block that is in the file. */

      while ((result = config_read_token_pair(in, token, value, section)) != sf_READ_CONFIG_EOF)
      {
        if (result == sf_READ_CONFIG_NEW_SECTION)
        {
          /* If a new section is found, decode it into an integer for speed and split out any fields in the
           * section name.  Set up the current block size details and any section specific information ready
           * for use.
           */

          next_field (section, ':');
          sect_sup = next_field (NULL, ':');

          if (string_nocase_strcmp (section, "Budget") == 0)
          {
            sect_num = LOAD_SECT_BUDGET;

            #ifdef DEBUG
            debug_printf ("\\rStart Budget section");
            #endif
          }
          else if (string_nocase_strcmp (section, "Accounts") == 0)
          {
            sect_num = LOAD_SECT_ACCOUNTS;

            block_size = flex_size ((flex_ptr) &(file->accounts)) / sizeof (account);

            #ifdef DEBUG
            debug_printf ("\\rStart Account section (block size: %d)", block_size);
            #endif
          }
          else if (string_nocase_strcmp (section, "AccountList") == 0)
          {
            sect_num = LOAD_SECT_ACCLIST;

            type = strtoul (sect_sup, NULL, 16);
            entry = find_accounts_window_entry_from_type (file, type);

            block_size = flex_size ((flex_ptr) &(file->account_windows[entry].line_data)) / sizeof (account_redraw);

            #ifdef DEBUG
            debug_printf ("\\rStart AccountList section for entry %d (block size: %d)", entry, block_size);
            #endif
          }
          else if (string_nocase_strcmp (section, "Transactions") == 0)
          {
            sect_num = LOAD_SECT_TRANSACT;

            block_size = flex_size ((flex_ptr) &(file->transactions)) / sizeof (transaction);

            #ifdef DEBUG
            debug_printf ("\\rStart Transactions section (block size: %d)", block_size);
            #endif
          }
          else if (string_nocase_strcmp (section, "StandingOrders") == 0)
          {
            sect_num = LOAD_SECT_SORDER;

            block_size = flex_size ((flex_ptr) &(file->sorders)) / sizeof(struct sorder);

            #ifdef DEBUG
            debug_printf ("\\rStart StandingOrders section (block size: %d)", block_size);
            #endif
          }
          else if (string_nocase_strcmp (section, "Presets") == 0)
          {
            sect_num = LOAD_SECT_PRESET;

            block_size = flex_size ((flex_ptr) &(file->presets)) / sizeof(struct preset);

            #ifdef DEBUG
            debug_printf ("\\rStart Presets section (block size: %d)", block_size);
            #endif
          }
          else if (string_nocase_strcmp (section, "Reports") == 0)
          {
            sect_num = LOAD_SECT_REPORT;

            block_size = flex_size ((flex_ptr) &(file->saved_reports)) / sizeof (saved_report);

            #ifdef DEBUG
            debug_printf ("\\rStart Saved Reports section (block size: %d)", block_size);
            #endif
          }
          else
          {
            sect_num = LOAD_SECT_NONE;
            unknown_data = 1;
          }

          i = -1; /* Reset the array pointer, so an @ field must be found before more additional fields will load. */
        }

        /* Parse the token/value pairs read in from the file, depending on location. */

        switch (sect_num)
        {
          /* Load the Budget data block. */

          case LOAD_SECT_BUDGET:
            if (string_nocase_strcmp (token, "Start") == 0)
            {
              file->budget.start = strtoul (value, NULL, 16);
            }
            else if (string_nocase_strcmp (token, "Finish") == 0)
            {
              file->budget.finish = strtoul (value, NULL, 16);
            }
            else if (string_nocase_strcmp (token, "SOTrial") == 0)
            {
              file->budget.sorder_trial = strtoul (value, NULL, 16);
            }
            else if (string_nocase_strcmp (token, "RestrictPost") == 0)
            {
              file->budget.limit_postdate = (config_read_opt_string(value) == TRUE);
            }
            else
            {
              unknown_data = 1;
            }
            break;

          /* Load the Account data block. */

          case LOAD_SECT_ACCOUNTS:
            if (string_nocase_strcmp (token, "Entries") == 0)
            {
              block_size = strtoul (value, NULL, 16);
              if (block_size > file->account_count)
              {
                #ifdef DEBUG
                debug_printf ("Section block pre-expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->accounts), sizeof (account) * block_size);
              }
              else
              {
                block_size = file->account_count;
              }
            }
            else if (string_nocase_strcmp (token, "WinColumns") == 0)
            {
              column_init_window (file->accview_column_width, file->accview_column_position,
                                   ACCVIEW_COLUMNS, value);
            }
            else if (string_nocase_strcmp (token, "SortOrder") == 0)
            {
              file->accview_sort_order = strtoul (value, NULL, 16);
            }
            else if (string_nocase_strcmp (token, "@") == 0)
            {
              /* A new account.  Take the account number, and see if it falls within the current defined set of
               * accounts (not the same thing as the pre-expanded account block.  If not, expand the acconut_count
               * to the new account number and blank all the new entries.
               */

              i = strtoul (next_field (value, ','), NULL, 16);

              if (i >= file->account_count)
              {
                j = file->account_count;
                file->account_count = i+1;

                #ifdef DEBUG
                debug_printf ("Account range expanded to %d", i);
                #endif

                /* The block isn't big enough, so expand this to the required size. */

                if (file->account_count > block_size)
                {
                  block_size = file->account_count;
                  #ifdef DEBUG
                  debug_printf ("Section block expand to %d", block_size);
                  #endif
                  flex_extend ((flex_ptr) &(file->accounts), sizeof (account) * block_size);
                }

                /* Blank all the intervening entries. */

                while (j < file->account_count)
                {
                  #ifdef DEBUG
                  debug_printf ("Blanking account entry %d", j);
                  #endif

                  *(file->accounts[j].name) = '\0';
                  *(file->accounts[j].ident) = '\0';
                  file->accounts[j].type = ACCOUNT_NULL;
                  file->accounts[j].opening_balance = 0;
                  file->accounts[j].credit_limit = 0;
                  file->accounts[j].budget_amount = 0;
                  file->accounts[j].cheque_num_width = 0;
                  file->accounts[j].next_cheque_num = 0;
                  file->accounts[j].payin_num_width = 0;
                  file->accounts[j].next_payin_num = 0;

                  file->accounts[j].account_view = NULL;

                  *(file->accounts[j].name) = '\0';
                  *(file->accounts[j].account_no) = '\0';
                  *(file->accounts[j].sort_code) = '\0';
                  *(file->accounts[j].address[0]) = '\0';
                  *(file->accounts[j].address[1]) = '\0';
                  *(file->accounts[j].address[2]) = '\0';
                  *(file->accounts[j].address[3]) = '\0';

                  j++;
                }
              }

              #ifdef DEBUG
              debug_printf ("Loading account entry %d", i);
              #endif

              strcpy (file->accounts[i].ident, next_field (NULL, ','));
              file->accounts[i].type = strtoul (next_field (NULL, ','), NULL, 16);
              file->accounts[i].opening_balance = strtoul (next_field (NULL, ','), NULL, 16);
              file->accounts[i].credit_limit = strtoul (next_field (NULL, ','), NULL, 16);
              file->accounts[i].budget_amount = strtoul (next_field (NULL, ','), NULL, 16);
              file->accounts[i].cheque_num_width = strtoul (next_field (NULL, ','), NULL, 16);
              file->accounts[i].next_cheque_num = strtoul (next_field (NULL, ','), NULL, 16);

              *(file->accounts[i].name) = '\0';
              *(file->accounts[i].account_no) = '\0';
              *(file->accounts[i].sort_code) = '\0';
              *(file->accounts[i].address[0]) = '\0';
              *(file->accounts[i].address[1]) = '\0';
              *(file->accounts[i].address[2]) = '\0';
              *(file->accounts[i].address[3]) = '\0';
            }
            else if (i != -1 && string_nocase_strcmp (token, "Name") == 0)
            {
              strcpy (file->accounts[i].name, value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "AccNo") == 0)
            {
              strcpy (file->accounts[i].account_no, value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "SortCode") == 0)
            {
              strcpy (file->accounts[i].sort_code, value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Addr0") == 0)
            {
              strcpy (file->accounts[i].address[0], value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Addr1") == 0)
            {
              strcpy (file->accounts[i].address[1], value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Addr2") == 0)
            {
              strcpy (file->accounts[i].address[2], value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Addr3") == 0)
            {
              strcpy (file->accounts[i].address[3], value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "PayIn") == 0)
            {
              file->accounts[i].payin_num_width = strtoul (next_field (value, ','), NULL, 16);
              file->accounts[i].next_payin_num = strtoul (next_field (NULL, ','), NULL, 16);
            }
            else
            {
              unknown_data = 1;
            }
            break;

          /* Load the Account List data block. */

          case LOAD_SECT_ACCLIST:
            if (string_nocase_strcmp (token, "Entries") == 0)
            {
              block_size = strtoul (value, NULL, 16);
              if (block_size > file->account_windows[entry].display_lines)
              {
                #ifdef DEBUG
                debug_printf ("Section block pre-expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->account_windows[entry].line_data),
                             sizeof (account_redraw) * block_size);
              }
              else
              {
                block_size = file->account_windows[entry].display_lines;
              }
            }
            else if (string_nocase_strcmp (token, "WinColumns") == 0)
            {
              column_init_window (file->account_windows[entry].column_width,
                                   file->account_windows[entry].column_position,
                                   ACCOUNT_COLUMNS, value);
            }
            else if (string_nocase_strcmp (token, "@") == 0)
            {
              file->account_windows[entry].display_lines++;
              if (file->account_windows[entry].display_lines > block_size)
              {
                block_size = file->account_windows[entry].display_lines;
                #ifdef DEBUG
                debug_printf ("Section block expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->account_windows[entry].line_data),
                             sizeof (account_redraw) * block_size);
              }
              i = file->account_windows[entry].display_lines-1;
              *(file->account_windows[entry].line_data[i].heading) = '\0';
              file->account_windows[entry].line_data[i].type = strtoul (next_field (value, ','), NULL, 16);
              file->account_windows[entry].line_data[i].account = strtoul (next_field (NULL, ','), NULL, 16);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Heading") == 0)
            {
              strcpy (file->account_windows[entry].line_data[i].heading, value);
            }
            else
            {
              unknown_data = 1;
            }
            break;

          /* Load in Transactions data block. */

          case LOAD_SECT_TRANSACT:
            if (string_nocase_strcmp (token, "Entries") == 0)
            {
              block_size = strtoul (value, NULL, 16);
              if (block_size > file->trans_count)
              {
                #ifdef DEBUG
                debug_printf ("Section block pre-expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->transactions), sizeof (transaction) * block_size);
              }
              else
              {
                block_size = file->trans_count;
              }
            }
            else if (string_nocase_strcmp (token, "WinColumns") == 0)
            {
              column_init_window (file->transaction_window.column_width,
                                   file->transaction_window.column_position,
                                   TRANSACT_COLUMNS, value);
            }
            else if (string_nocase_strcmp (token, "SortOrder") == 0)
            {
              file->transaction_window.sort_order = strtoul (value, NULL, 16);
            }
            else if (string_nocase_strcmp (token, "@") == 0)
            {
              file->trans_count++;
              if (file->trans_count > block_size)
              {
                block_size = file->trans_count;
                #ifdef DEBUG
                debug_printf ("Section block expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->transactions), sizeof (transaction) * block_size);
              }
              i = file->trans_count-1;
              file->transactions[i].date = strtoul (next_field (value, ','), NULL, 16);
              file->transactions[i].flags = strtoul (next_field (NULL, ','), NULL, 16);
              file->transactions[i].from = strtoul (next_field (NULL, ','), NULL, 16);
              file->transactions[i].to = strtoul (next_field (NULL, ','), NULL, 16);
              file->transactions[i].amount = strtoul (next_field (NULL, ','), NULL, 16);

              *(file->transactions[i].reference) = '\0';
              *(file->transactions[i].description) = '\0';

              file->transactions[i].sort_index = i;
            }
            else if (i != -1 && string_nocase_strcmp (token, "Ref") == 0)
            {
              strcpy (file->transactions[i].reference, value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Desc") == 0)
            {
              strcpy (file->transactions[i].description, value);
            }
            else
            {
              unknown_data = 1;
            }
            break;

          /* Load in StandingOrders data block. */

          case LOAD_SECT_SORDER:
            if (string_nocase_strcmp (token, "Entries") == 0)
            {
              block_size = strtoul (value, NULL, 16);
              if (block_size > file->sorder_count)
              {
                #ifdef DEBUG
                debug_printf ("Section block pre-expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->sorders), sizeof(struct sorder) * block_size);
              }
              else
              {
                block_size = file->sorder_count;
              }
            }
            else if (string_nocase_strcmp (token, "WinColumns") == 0)
            {
              column_init_window (file->sorder_window.column_width,
                                   file->sorder_window.column_position,
                                   SORDER_COLUMNS, value);
            }
            else if (string_nocase_strcmp (token, "SortOrder") == 0)
            {
              file->sorder_window.sort_order = strtoul (value, NULL, 16);
            }
            else if (string_nocase_strcmp (token, "@") == 0)
            {
              file->sorder_count++;
              if (file->sorder_count > block_size)
              {
                block_size = file->sorder_count;
                #ifdef DEBUG
                debug_printf ("Section block expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->sorders), sizeof(struct sorder) * block_size);
              }
              i = file->sorder_count-1;
              file->sorders[i].start_date = strtoul (next_field (value, ','), NULL, 16);
              file->sorders[i].number = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].period = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].period_unit = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].raw_next_date = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].adjusted_next_date = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].left = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].flags = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].from = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].to = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].normal_amount = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].first_amount = strtoul (next_field (NULL, ','), NULL, 16);
              file->sorders[i].last_amount = strtoul (next_field (NULL, ','), NULL, 16);
              *(file->sorders[i].reference) = '\0';
              *(file->sorders[i].description) = '\0';
              file->sorders[i].sort_index = i;
            }
            else if (i != -1 && string_nocase_strcmp (token, "Ref") == 0)
            {
              strcpy (file->sorders[i].reference, value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Desc") == 0)
            {
              strcpy (file->sorders[i].description, value);
            }
            else
            {
              unknown_data = 1;
            }
            break;

          /* Load in Presets data block. */

          case LOAD_SECT_PRESET:
            if (string_nocase_strcmp (token, "Entries") == 0)
            {
              block_size = strtoul (value, NULL, 16);
              if (block_size > file->preset_count)
              {
                #ifdef DEBUG
                debug_printf ("Section block pre-expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->presets), sizeof(struct preset) * block_size);
              }
              else
              {
                block_size = file->preset_count;
              }
            }
            else if (string_nocase_strcmp (token, "WinColumns") == 0)
            {
              column_init_window (file->preset_window.column_width,
                                   file->preset_window.column_position,
                                   PRESET_COLUMNS, value);
            }
            else if (string_nocase_strcmp (token, "SortOrder") == 0)
            {
              file->preset_window.sort_order = strtoul (value, NULL, 16);
            }
            else if (string_nocase_strcmp (token, "@") == 0)
            {
              file->preset_count++;
              if (file->preset_count > block_size)
              {
                block_size = file->preset_count;
                #ifdef DEBUG
                debug_printf ("Section block expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->presets), sizeof(struct preset) * block_size);
              }
              i = file->preset_count-1;
              file->presets[i].action_key = strtoul (next_field (value, ','), NULL, 16);
              file->presets[i].caret_target = strtoul (next_field (NULL, ','), NULL, 16);
              file->presets[i].date = strtoul (next_field (NULL, ','), NULL, 16);
              file->presets[i].flags = strtoul (next_field (NULL, ','), NULL, 16);
              file->presets[i].from = strtoul (next_field (NULL, ','), NULL, 16);
              file->presets[i].to = strtoul (next_field (NULL, ','), NULL, 16);
              file->presets[i].amount = strtoul (next_field (NULL, ','), NULL, 16);
              *(file->presets[i].name) = '\0';
              *(file->presets[i].reference) = '\0';
              *(file->presets[i].description) = '\0';
              file->presets[i].sort_index = i;
            }
            else if (i != -1 && string_nocase_strcmp (token, "Name") == 0)
            {
              strcpy (file->presets[i].name, value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Ref") == 0)
            {
              strcpy (file->presets[i].reference, value);
            }
            else if (i != -1 && string_nocase_strcmp (token, "Desc") == 0)
            {
              strcpy (file->presets[i].description, value);
            }
            else
            {
              unknown_data = 1;
            }
            break;

          /* Load in Saved Reports data block. */

          case LOAD_SECT_REPORT:
            if (string_nocase_strcmp (token, "Entries") == 0)
            {
              block_size = strtoul (value, NULL, 16);
              if (block_size > file->saved_report_count)
              {
                #ifdef DEBUG
                debug_printf ("Section block pre-expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->saved_reports), sizeof (saved_report) * block_size);
              }
              else
              {
                block_size = file->saved_report_count;
              }
            }
            else if (string_nocase_strcmp (token, "@") == 0)
            {
              file->saved_report_count++;
              if (file->saved_report_count > block_size)
              {
                block_size = file->saved_report_count;
                #ifdef DEBUG
                debug_printf ("Section block expand to %d", block_size);
                #endif
                flex_extend ((flex_ptr) &(file->saved_reports), sizeof (saved_report) * block_size);
              }
              i = file->saved_report_count-1;
              file->saved_reports[i].type = strtoul (next_field (value, ','), NULL, 16);
              switch(file->saved_reports[i].type)
              {
                case REPORT_TYPE_TRANS:
                file->saved_reports[i].data.transaction.date_from = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.date_to = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.budget = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.group = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.period = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.period_unit = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.lock = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.output_trans = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.output_summary = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.output_accsummary = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.transaction.amount_min = NULL_CURRENCY;
                file->saved_reports[i].data.transaction.amount_max = NULL_CURRENCY;
                file->saved_reports[i].data.transaction.from_count = 0;
                file->saved_reports[i].data.transaction.to_count = 0;
                *(file->saved_reports[i].data.transaction.ref) = '\0';
                *(file->saved_reports[i].data.transaction.desc) = '\0';
                break;

                case REPORT_TYPE_UNREC:
                file->saved_reports[i].data.unreconciled.date_from = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.unreconciled.date_to = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.unreconciled.budget = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.unreconciled.group = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.unreconciled.period = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.unreconciled.period_unit = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.unreconciled.lock = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.unreconciled.from_count = 0;
                file->saved_reports[i].data.unreconciled.to_count = 0;
                break;

                case REPORT_TYPE_CASHFLOW:
                file->saved_reports[i].data.cashflow.date_from = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.date_to = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.budget = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.group = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.period = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.period_unit = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.lock = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.tabular = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.empty = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.cashflow.accounts_count = 0;
                file->saved_reports[i].data.cashflow.incoming_count = 0;
                file->saved_reports[i].data.cashflow.outgoing_count = 0;
                break;

                case REPORT_TYPE_BALANCE:
                file->saved_reports[i].data.balance.date_from = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.date_to = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.budget = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.group = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.period = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.period_unit = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.lock = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.tabular = strtoul (next_field (NULL, ','), NULL, 16);
                file->saved_reports[i].data.balance.accounts_count = 0;
                file->saved_reports[i].data.balance.incoming_count = 0;
                file->saved_reports[i].data.balance.outgoing_count = 0;
                break;
              }
            }
            else if (i != -1 && string_nocase_strcmp (token, "Name") == 0)
            {
              strcpy (file->saved_reports[i].name, value);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_CASHFLOW &&
                     string_nocase_strcmp (token, "Accounts") == 0)
            {
              file->saved_reports[i].data.cashflow.accounts_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.cashflow.accounts);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_CASHFLOW &&
                     string_nocase_strcmp (token, "Incoming") == 0)
            {
              file->saved_reports[i].data.cashflow.incoming_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.cashflow.incoming);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_CASHFLOW &&
                     string_nocase_strcmp (token, "Outgoing") == 0)
            {
              file->saved_reports[i].data.cashflow.outgoing_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.cashflow.outgoing);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_BALANCE &&
                     string_nocase_strcmp (token, "Accounts") == 0)
            {
              file->saved_reports[i].data.balance.accounts_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.balance.accounts);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_BALANCE &&
                     string_nocase_strcmp (token, "Incoming") == 0)
            {
              file->saved_reports[i].data.balance.incoming_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.balance.incoming);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_BALANCE &&
                     string_nocase_strcmp (token, "Outgoing") == 0)
            {
              file->saved_reports[i].data.balance.outgoing_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.balance.outgoing);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
                     string_nocase_strcmp (token, "From") == 0)
            {
              file->saved_reports[i].data.transaction.from_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.transaction.from);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
                     string_nocase_strcmp (token, "To") == 0)
            {
              file->saved_reports[i].data.transaction.to_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.transaction.to);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_UNREC &&
                     string_nocase_strcmp (token, "From") == 0)
            {
              file->saved_reports[i].data.unreconciled.from_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.unreconciled.from);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_UNREC &&
                     string_nocase_strcmp (token, "To") == 0)
            {
              file->saved_reports[i].data.unreconciled.to_count =
                analysis_account_hex_to_list (file, value, file->saved_reports[i].data.unreconciled.to);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
                     string_nocase_strcmp (token, "Ref") == 0)
            {
              strcpy (file->saved_reports[i].data.transaction.ref, value);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
                     string_nocase_strcmp (token, "Amount") == 0)
            {
              file->saved_reports[i].data.transaction.amount_min = strtoul (next_field (value, ','), NULL, 16);
              file->saved_reports[i].data.transaction.amount_max = strtoul (next_field (NULL, ','), NULL, 16);
            }
            else if (i != -1 && file->saved_reports[i].type == REPORT_TYPE_TRANS &&
                     string_nocase_strcmp (token, "Desc") == 0)
            {
              strcpy (file->saved_reports[i].data.transaction.desc, value);
            }
            else
            {
              unknown_data = 1;
            }
            break;
        }
      }

      fclose (in);

      /* Shrink the flex blocks down to the minimum required. */

      #ifdef DEBUG
      debug_printf ("\\rShrink flex blocks.");
      #endif

      block_size = flex_size ((flex_ptr) &(file->accounts)) / sizeof (account);

      #ifdef DEBUG
      debug_printf ("Account block size: %d, required: %d", block_size, file->account_count);
      #endif

      if (block_size > file->account_count)
      {
        block_size = file->account_count;
        flex_extend ((flex_ptr) &(file->accounts), sizeof (account) * block_size);

        #ifdef DEBUG
        debug_printf ("Block shrunk to %d", block_size);
        #endif
      }

      for (i=0; i<ACCOUNT_WINDOWS; i++)
      {
        block_size = flex_size ((flex_ptr) &(file->account_windows[i].line_data)) / sizeof (account_redraw);

        #ifdef DEBUG
        debug_printf ("AccountList block %d size: %d, required: %d", i, block_size,
                      file->account_windows[i].display_lines);
        #endif

        if (block_size > file->account_windows[i].display_lines)
        {
          block_size = file->account_windows[i].display_lines;
          flex_extend ((flex_ptr) &(file->account_windows[i].line_data), sizeof (account_redraw) * block_size);

          #ifdef DEBUG
          debug_printf ("Block shrunk to %d", block_size);
          #endif
        }
      }

      block_size = flex_size ((flex_ptr) &(file->transactions)) / sizeof (transaction);

      #ifdef DEBUG
      debug_printf ("Transaction block size: %d, required: %d", block_size, file->trans_count);
      #endif

      if (block_size > file->trans_count)
      {
        block_size = file->trans_count;
        flex_extend ((flex_ptr) &(file->transactions), sizeof (transaction) * block_size);

        #ifdef DEBUG
        debug_printf ("Block shrunk to %d", block_size);
        #endif
      }

      block_size = flex_size ((flex_ptr) &(file->sorders)) / sizeof(struct sorder);

      #ifdef DEBUG
      debug_printf ("StandingOrder block size: %d, required: %d", block_size, file->sorder_count);
      #endif

      if (block_size > file->sorder_count)
      {
        block_size = file->sorder_count;
        flex_extend ((flex_ptr) &(file->sorders), sizeof(struct sorder) * block_size);

        #ifdef DEBUG
        debug_printf ("Block shrunk to %d", block_size);
        #endif
      }

      block_size = flex_size ((flex_ptr) &(file->presets)) / sizeof(struct preset);

      #ifdef DEBUG
      debug_printf ("Preset block size: %d, required: %d", block_size, file->preset_count);
      #endif

      if (block_size > file->preset_count)
      {
        block_size = file->preset_count;
        flex_extend ((flex_ptr) &(file->presets), sizeof(struct preset) * block_size);

        #ifdef DEBUG
        debug_printf ("Block shrunk to %d", block_size);
        #endif
      }

      block_size = flex_size ((flex_ptr) &(file->saved_reports)) / sizeof (saved_report);

      #ifdef DEBUG
      debug_printf ("Saved Report block size: %d, required: %d", block_size, file->saved_report_count);
      #endif

      if (block_size > file->saved_report_count)
      {
        block_size = file->saved_report_count;
        flex_extend ((flex_ptr) &(file->saved_reports), sizeof (saved_report) * block_size);

        #ifdef DEBUG
        debug_printf ("Block shrunk to %d", block_size);
        #endif
      }

      /* Get the datestamp of the file. */

      osfile_read_stamped (filename, &load, (bits *) file->datestamp, NULL, NULL, NULL);
      file->datestamp[4] = load & 0xff;

      /* Tidy up, create the transaction window and open it up. */

      strcpy (file->filename, filename);
      sorder_process(file);
      sort_transactions (file);
      perform_full_recalculation (file);
      sort_transaction_window (file);
      sorder_sort(file);
      preset_sort(file);
      create_transaction_window (file); /* The window extent is set in this action. */
    }
    else
    {
      delete_file (file);
      error_msgs_report_error ("FileLoadFail");
    }

    hourglass_off ();

    if (unknown_data)
    {
      error_msgs_report_info ("UnknownFileData");
    }
  }
  else
  {
    error_msgs_report_error ("NoMemForLoad");
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

char *next_plain_field (char *line, char sep)
{
  static char *current;
  char        *start;

  if (line != NULL)
  {
    current = string_strip_surrounding_whitespace (line);
  }

  start = current;

  while (*current != '\0' && *current != sep)
  {
    current++;
  }

  if (*current == sep)
  {
    *current = '\0';
    current ++;
  }

  return (start);
}

/* ==================================================================================================================
 * Account file saving
 */

void save_transaction_file (file_data *file, char *filename)
{
  FILE *out;
  int  i, j;
  char buffer[MAX_FILE_LINE_LEN];
  bits load;

  out = fopen (filename, "w");

  if (out != NULL)
  {
    hourglass_on ();

    /* Strip unused blank lines from the end of the file. */

    strip_blank_transactions (file);

    /* Output the file header. */

    fprintf (out, "# CashBook file\n");
    fprintf (out, "# Written by CashBook\n\n");

    fprintf (out, "Format: 1.00\n");

    /* Output the budget data */

    fprintf (out, "\n[Budget]\n");

    fprintf (out, "Start: %x\n", file->budget.start);
    fprintf (out, "Finish: %x\n", file->budget.finish);
    fprintf (out, "SOTrial: %x\n", file->budget.sorder_trial);
    fprintf (out, "RestrictPost: %s\n", config_return_opt_string(file->budget.limit_postdate));

    /* Output the account data */

    fprintf (out, "\n[Accounts]\n");

    fprintf (out, "Entries: %x\n", file->account_count);

    column_write_as_text (file->accview_column_width, ACCVIEW_COLUMNS, buffer);
    fprintf (out, "WinColumns: %s\n", buffer);

    fprintf (out, "SortOrder: %x\n", file->accview_sort_order);

    for (i=0; i < file->account_count; i++)
    {
      if (file->accounts[i].type != ACCOUNT_NULL) /* Deleted accounts are skipped, as these can be filled in at load. */
      {
        fprintf (out, "@: %x,%s,%x,%x,%x,%x,%x,%x\n",
                 i, file->accounts[i].ident, file->accounts[i].type,
                 file->accounts[i].opening_balance, file->accounts[i].credit_limit, file->accounts[i].budget_amount,
                 file->accounts[i].cheque_num_width, file->accounts[i].next_cheque_num);
        if (*(file->accounts[i].name) != '\0')
        {
          config_write_token_pair (out, "Name", file->accounts[i].name);
        }
        if (*(file->accounts[i].account_no) != '\0')
        {
          config_write_token_pair (out, "AccNo", file->accounts[i].account_no);
        }
        if (*(file->accounts[i].sort_code) != '\0')
        {
          config_write_token_pair (out, "SortCode", file->accounts[i].sort_code);
        }
        if (*(file->accounts[i].address[0]) != '\0')
        {
          config_write_token_pair (out, "Addr0", file->accounts[i].address[0]);
        }
        if (*(file->accounts[i].address[1]) != '\0')
        {
          config_write_token_pair (out, "Addr1", file->accounts[i].address[1]);
        }
        if (*(file->accounts[i].address[2]) != '\0')
        {
          config_write_token_pair (out, "Addr2", file->accounts[i].address[2]);
        }
        if (*(file->accounts[i].address[3]) != '\0')
        {
          config_write_token_pair (out, "Addr3", file->accounts[i].address[3]);
        }
        if (file->accounts[i].payin_num_width != 0 || file->accounts[i].next_payin_num != 0)
        {
          fprintf (out, "PayIn: %x,%x\n", file->accounts[i].payin_num_width, file->accounts[i].next_payin_num);
        }
      }
    }

    /* Output the Accounts Windows data. */

    for (j=0; j<ACCOUNT_WINDOWS; j++)
    {
      fprintf (out, "\n[AccountList:%x]\n", file->account_windows[j].type);

      fprintf (out, "Entries: %x\n", file->account_windows[j].display_lines);

      column_write_as_text (file->account_windows[j].column_width, ACCOUNT_COLUMNS, buffer);
      fprintf (out, "WinColumns: %s\n", buffer);

      for (i=0; i<file->account_windows[j].display_lines; i++)
      {
        fprintf (out, "@: %x,%x\n", file->account_windows[j].line_data[i].type,
                                   file->account_windows[j].line_data[i].account);

        if ((file->account_windows[j].line_data[i].type == ACCOUNT_LINE_HEADER ||
            file->account_windows[j].line_data[i].type == ACCOUNT_LINE_FOOTER) &&
            *(file->account_windows[j].line_data[i].heading) != '\0')
        {
          config_write_token_pair (out, "Heading", file->account_windows[j].line_data[i].heading);
        }
      }
    }

    /* Output the transaction data. */

    fprintf (out, "\n[Transactions]\n");

    fprintf (out, "Entries: %x\n", file->trans_count);

    column_write_as_text (file->transaction_window.column_width, TRANSACT_COLUMNS, buffer);
    fprintf (out, "WinColumns: %s\n", buffer);

    fprintf (out, "SortOrder: %x\n", file->transaction_window.sort_order);

    for (i=0; i < file->trans_count; i++)
    {
      fprintf (out, "@: %x,%x,%x,%x,%x\n",
               file->transactions[i].date, file->transactions[i].flags, file->transactions[i].from,
               file->transactions[i].to, file->transactions[i].amount);
      if (*(file->transactions[i].reference) != '\0')
      {
        config_write_token_pair (out, "Ref", file->transactions[i].reference);
      }
      if (*(file->transactions[i].description) != '\0')
      {
        config_write_token_pair (out, "Desc", file->transactions[i].description);
      }
    }

    /* Output the standing order data. */

    fprintf (out, "\n[StandingOrders]\n");

    fprintf (out, "Entries: %x\n", file->sorder_count);

    column_write_as_text (file->sorder_window.column_width, SORDER_COLUMNS, buffer);
    fprintf (out, "WinColumns: %s\n", buffer);

    fprintf (out, "SortOrder: %x\n", file->sorder_window.sort_order);

    for (i=0; i < file->sorder_count; i++)
    {
      fprintf (out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
               file->sorders[i].start_date, file->sorders[i].number, file->sorders[i].period,
               file->sorders[i].period_unit, file->sorders[i].raw_next_date, file->sorders[i].adjusted_next_date,
               file->sorders[i].left, file->sorders[i].flags, file->sorders[i].from, file->sorders[i].to,
               file->sorders[i].normal_amount, file->sorders[i].first_amount, file->sorders[i].last_amount);
      if (*(file->sorders[i].reference) != '\0')
      {
        config_write_token_pair (out, "Ref", file->sorders[i].reference);
      }
      if (*(file->sorders[i].description) != '\0')
      {
        config_write_token_pair (out, "Desc", file->sorders[i].description);
      }
    }

    /* Output the preset data. */

    fprintf (out, "\n[Presets]\n");

    fprintf (out, "Entries: %x\n", file->preset_count);

    column_write_as_text (file->preset_window.column_width, PRESET_COLUMNS, buffer);
    fprintf (out, "WinColumns: %s\n", buffer);

    fprintf (out, "SortOrder: %x\n", file->preset_window.sort_order);

    for (i=0; i < file->preset_count; i++)
    {
      fprintf (out, "@: %x,%x,%x,%x,%x,%x,%x\n",
               file->presets[i].action_key, file->presets[i].caret_target,
               file->presets[i].date, file->presets[i].flags,
               file->presets[i].from, file->presets[i].to, file->presets[i].amount);
      if (*(file->presets[i].name) != '\0')
      {
        config_write_token_pair (out, "Name", file->presets[i].name);
      }
      if (*(file->presets[i].reference) != '\0')
      {
        config_write_token_pair (out, "Ref", file->presets[i].reference);
      }
      if (*(file->presets[i].description) != '\0')
      {
        config_write_token_pair (out, "Desc", file->presets[i].description);
      }
    }

    /* Output the report data. */

    fprintf (out, "\n[Reports]\n");

    fprintf (out, "Entries: %x\n", file->saved_report_count);

    for (i=0; i < file->saved_report_count; i++)
    {
      switch(file->saved_reports[i].type)
      {
        case REPORT_TYPE_TRANS:
          fprintf (out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
                   REPORT_TYPE_TRANS,
                   file->saved_reports[i].data.transaction.date_from,
                   file->saved_reports[i].data.transaction.date_to,
                   file->saved_reports[i].data.transaction.budget,
                   file->saved_reports[i].data.transaction.group,
                   file->saved_reports[i].data.transaction.period,
                   file->saved_reports[i].data.transaction.period_unit,
                   file->saved_reports[i].data.transaction.lock,
                   file->saved_reports[i].data.transaction.output_trans,
                   file->saved_reports[i].data.transaction.output_summary,
                   file->saved_reports[i].data.transaction.output_accsummary);
          if (*(file->saved_reports[i].name) != '\0')
          {
            config_write_token_pair (out, "Name", file->saved_reports[i].name);
          }
          if (file->saved_reports[i].data.transaction.from_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.transaction.from, file->saved_reports[i].data.transaction.from_count);
            config_write_token_pair (out, "From", buffer);
          }
          if (file->saved_reports[i].data.transaction.to_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.transaction.to, file->saved_reports[i].data.transaction.to_count);
            config_write_token_pair (out, "To", buffer);
          }
          if (*(file->saved_reports[i].data.transaction.ref) != '\0')
          {
            config_write_token_pair (out, "Ref", file->saved_reports[i].data.transaction.ref);
          }
          if (file->saved_reports[i].data.transaction.amount_min != NULL_CURRENCY ||
              file->saved_reports[i].data.transaction.amount_max != NULL_CURRENCY)
          {
            sprintf (buffer, "%x,%x",
                     file->saved_reports[i].data.transaction.amount_min,
                     file->saved_reports[i].data.transaction.amount_max);
            config_write_token_pair (out, "Amount", buffer);
          }
          if (*(file->saved_reports[i].data.transaction.desc) != '\0')
          {
            config_write_token_pair (out, "Desc", file->saved_reports[i].data.transaction.desc);
          }
          break;

        case REPORT_TYPE_UNREC:
          fprintf (out, "@: %x,%x,%x,%x,%x,%x,%x,%x\n",
                   REPORT_TYPE_UNREC,
                   file->saved_reports[i].data.unreconciled.date_from,
                   file->saved_reports[i].data.unreconciled.date_to,
                   file->saved_reports[i].data.unreconciled.budget,
                   file->saved_reports[i].data.unreconciled.group,
                   file->saved_reports[i].data.unreconciled.period,
                   file->saved_reports[i].data.unreconciled.period_unit,
                   file->saved_reports[i].data.unreconciled.lock);
          if (*(file->saved_reports[i].name) != '\0')
          {
            config_write_token_pair (out, "Name", file->saved_reports[i].name);
          }
          if (file->saved_reports[i].data.unreconciled.from_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.unreconciled.from, file->saved_reports[i].data.unreconciled.from_count);
            config_write_token_pair (out, "From", buffer);
          }
          if (file->saved_reports[i].data.unreconciled.to_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.unreconciled.to, file->saved_reports[i].data.unreconciled.to_count);
            config_write_token_pair (out, "To", buffer);
          }
          break;

        case REPORT_TYPE_CASHFLOW:
          fprintf (out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
                   REPORT_TYPE_CASHFLOW,
                   file->saved_reports[i].data.cashflow.date_from,
                   file->saved_reports[i].data.cashflow.date_to,
                   file->saved_reports[i].data.cashflow.budget,
                   file->saved_reports[i].data.cashflow.group,
                   file->saved_reports[i].data.cashflow.period,
                   file->saved_reports[i].data.cashflow.period_unit,
                   file->saved_reports[i].data.cashflow.lock,
                   file->saved_reports[i].data.cashflow.tabular,
                   file->saved_reports[i].data.cashflow.empty);
          if (*(file->saved_reports[i].name) != '\0')
          {
            config_write_token_pair (out, "Name", file->saved_reports[i].name);
          }
          if (file->saved_reports[i].data.cashflow.accounts_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.cashflow.accounts, file->saved_reports[i].data.cashflow.accounts_count);
            config_write_token_pair (out, "Accounts", buffer);
          }
          if (file->saved_reports[i].data.cashflow.incoming_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.cashflow.incoming, file->saved_reports[i].data.cashflow.incoming_count);
            config_write_token_pair (out, "Incoming", buffer);
          }
          if (file->saved_reports[i].data.cashflow.outgoing_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.cashflow.outgoing, file->saved_reports[i].data.cashflow.outgoing_count);
            config_write_token_pair (out, "Outgoing", buffer);
          }
          break;

        case REPORT_TYPE_BALANCE:
          fprintf (out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x\n",
                   REPORT_TYPE_BALANCE,
                   file->saved_reports[i].data.balance.date_from,
                   file->saved_reports[i].data.balance.date_to,
                   file->saved_reports[i].data.balance.budget,
                   file->saved_reports[i].data.balance.group,
                   file->saved_reports[i].data.balance.period,
                   file->saved_reports[i].data.balance.period_unit,
                   file->saved_reports[i].data.balance.lock,
                   file->saved_reports[i].data.balance.tabular);
          if (*(file->saved_reports[i].name) != '\0')
          {
            config_write_token_pair (out, "Name", file->saved_reports[i].name);
          }
          if (file->saved_reports[i].data.balance.accounts_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.balance.accounts, file->saved_reports[i].data.balance.accounts_count);
            config_write_token_pair (out, "Accounts", buffer);
          }
          if (file->saved_reports[i].data.balance.incoming_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.balance.incoming, file->saved_reports[i].data.balance.incoming_count);
            config_write_token_pair (out, "Incoming", buffer);
          }
          if (file->saved_reports[i].data.balance.outgoing_count > 0)
          {
            analysis_account_list_to_hex (file, buffer, MAX_FILE_LINE_LEN,
                file->saved_reports[i].data.balance.outgoing, file->saved_reports[i].data.balance.outgoing_count);
            config_write_token_pair (out, "Outgoing", buffer);
          }
          break;
      }
    }

    /* Close the file and set the type correctly. */

    fclose (out);
    osfile_set_type (filename, (bits) CASHBOOK_FILE_TYPE);

    /* Get the datestamp of the file. */

    osfile_read_stamped (filename, &load, (bits *) file->datestamp, NULL, NULL, NULL);
    file->datestamp[4] = load & 0xff;

    /* Update the modified flag and filename for the file block and refresh the window title. */

    file->modified = 0;
    strcpy (file->filename, filename);

    build_transaction_window_title (file);

    for (i=0; i<ACCOUNT_WINDOWS; i++)
    {
      build_account_window_title (file, i);
    }
    for (i=0; i<file->account_count; i++)
    {
      build_accview_window_title (file, i);
    }
    sorder_build_window_title(file);
    preset_build_window_title(file);

    hourglass_off ();
  }
  else
  {
    error_msgs_report_error ("FileSaveFail");
  }
}

/* ==================================================================================================================
 * Delimited file import
 */

void import_csv_file (file_data *file, char *filename)
{
  FILE         *input;
  char         line[1024], log[1024], b1[64], b2[64],
               *date, *ref, *amount, *description, *dummy, *ident, *name, *raw_from, *raw_to;
  int          from, to, rec_from, rec_to, import_count, reject_count, error;
  wimp_pointer pointer;
  unsigned int type;

  extern global_windows windows;


  import_window_file = file;

  import_count = 0;
  reject_count = 0;

  hourglass_on ();

  /* If there's an existing log, delete it. */

  if (file->import_report != NULL)
  {
    report_delete(file->import_report);
    file->import_report = NULL;
  }

  if (windows_get_open (windows.import_comp))
  {
    wimp_close_window (windows.import_comp);
  }

  /* Open a log report for the process, and title it. */

  msgs_lookup("IRWinT", log, sizeof (log));
  file->import_report = report_open(file, log, NULL);

  make_file_leafname (file, line, sizeof (line));
  msgs_param_lookup ("IRTitle", log, sizeof (log), line, NULL, NULL, NULL);
  report_write_line (file->import_report, 0, log);
  msgs_param_lookup ("IRImpFile", log, sizeof (log), filename, NULL, NULL, NULL);
  report_write_line (file->import_report, 0, log);

  report_write_line (file->import_report, 0, "");

  msgs_lookup ("IRHeadings", log, sizeof (log));
  report_write_line (file->import_report, 0, log);

  input = fopen (filename, "r");

  if (input != NULL)
  {
    while (fgets (line, sizeof (line), input) != NULL)
    {
      error = 0;

      /* Date */

      date = next_field (line, ',');
      date = unquote_string (date);

      if (convert_string_to_date (date, NULL_DATE, 0) == NULL_DATE)
      {
        error = 1;
      }

      /* From */

      ident = next_field (NULL, ',');
      ident = unquote_string (ident);

      raw_from = ident;

      rec_from = (strchr (ident, '#') > 0) ? TRANS_REC_FROM : NULL_TRANS_FLAGS;

      name = ident + strcspn (ident, "#:");
      *name++ = '\0';
      while (strchr ("#:", *name))
      {
        name++;
      }

      if (*ident == '\0')
      {
        error = 1;
        from = NULL_ACCOUNT;
      }
      else
      {
        type = isdigit (*ident) ? ACCOUNT_FULL : ACCOUNT_IN;
        from = find_account (file, ident, type);

        if (from == -1)
        {
          from = add_account (file, name, ident, type);
        }
      }

      /* To */

      ident = next_field (NULL, ',');
      ident = unquote_string (ident);

      raw_to = ident;

      rec_to = (strchr (ident, '#') > 0) ? TRANS_REC_TO : NULL_TRANS_FLAGS;

      name = ident + strcspn (ident, "#:");
      *name++ = '\0';
      while (strchr ("#:", *name))
      {
        name++;
      }


      if (*ident == '\0')
      {
        error = 1;
        to = NULL_ACCOUNT;
      }
      else
      {
        type = isdigit (*ident) ? ACCOUNT_FULL : ACCOUNT_OUT;
        to = find_account (file, ident, type);

        if (to == -1)
        {
          to = add_account (file, name, ident, type);
        }
      }

      /* Ref */

      ref = next_field (NULL, ',');
      ref = unquote_string (ref);

      /* Amount */

      amount = next_field (NULL, ',');

      if (*amount == '\0')
      {
        amount = next_field (NULL, ',');
      }
      else
      {
        dummy = next_field (NULL, ',');
      }

      amount = unquote_string (amount);

      /* Skip Balance */

      dummy = next_field (NULL, ',');

      /* Description */

      description = next_field (NULL, ',');
      description = unquote_string (description);

      /* Create a new transaction. */

      if (error == 1)
      {
        msgs_lookup ("Rejected", b1, sizeof(b1));
        reject_count++;
      }
      else
      {
        add_raw_transaction (file, convert_string_to_date (date, NULL_DATE, 0), from, to, rec_from | rec_to,
                             convert_string_to_money (amount), ref, description);
        msgs_lookup ("Imported", b1, sizeof(b1));

        import_count++;
      }

      sprintf (log, "%s\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'\\t'%s'",
               b1, date, raw_from, raw_to, ref, amount, description);
      report_write_line (file->import_report, 0, log);
    }
    fclose (input);

    file->transaction_window.display_lines = (file->trans_count + MIN_TRANSACT_BLANK_LINES > MIN_TRANSACT_ENTRIES) ?
                                             file->trans_count + MIN_TRANSACT_BLANK_LINES : MIN_TRANSACT_ENTRIES;
    set_transaction_window_extent (file);

    sort_transactions (file);
    sorder_trial(file);
    perform_full_recalculation (file);
    rebuild_all_account_views (file);
    set_file_data_integrity (file, TRUE);

    refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, -1, -1);
    force_transaction_window_redraw (file, 0, file->trans_count - 1);
 }

  /* Sort out the import results window. */

  report_write_line (file->import_report, 0, "");

  sprintf (b1, "%d", import_count);
  sprintf (b2, "%d", reject_count);
  msgs_param_lookup ("IRTotals", log, sizeof (log), b1, b2, NULL, NULL);
  report_write_line (file->import_report, 0, log);

  sprintf (icons_get_indirected_text_addr (windows.import_comp, ICOMP_ICON_IMPORTED), "%d", import_count);
  sprintf (icons_get_indirected_text_addr (windows.import_comp, ICOMP_ICON_REJECTED), "%d", reject_count);

  wimp_get_pointer_info (&pointer);
  windows_open_centred_at_pointer (windows.import_comp, &pointer);

  hourglass_off ();
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Close the import results dialogue, either opening or deleting the log report.  Once the window is closed, we no
 * longer need to track the report, so the pointer can be set to NULL.
 */

void close_import_complete_dialogue (int show_log)
{
  extern global_windows windows;

  if (show_log)
  {
    report_close(import_window_file->import_report);
  }
  else
  {
    report_delete(import_window_file->import_report);
  }

  wimp_close_window (windows.import_comp);
  import_window_file->import_report = NULL;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Force the closure of the windows if the file disappears. */

void force_close_import_window (file_data *file)
{
  extern global_windows windows;

  if (import_window_file == file && windows_get_open (windows.import_comp))
  {
    wimp_close_window (windows.import_comp);

    /* No need to delete any associated report, because it will disappear in the file close anyway. */
  }
}

/* ==================================================================================================================
 * Delimited file export
 */

void export_delimited_file (file_data *file, char *filename, int format, int filetype)
{
  FILE *out;
  int  i, t;
  char buffer[256];

  out = fopen (filename, "w");

  if (out != NULL)
  {
    hourglass_on ();

    /* Output the headings line, taking the text from the window icons. */

    icons_copy_text (file->transaction_window.transaction_pane, 0, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (file->transaction_window.transaction_pane, 1, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (file->transaction_window.transaction_pane, 2, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (file->transaction_window.transaction_pane, 3, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (file->transaction_window.transaction_pane, 4, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (file->transaction_window.transaction_pane, 5, buffer);
    filing_output_delimited_field (out, buffer, format, DELIMIT_LAST);

    /* Output the transaction data as a set of delimited lines. */

    for (i=0; i < file->trans_count; i++)
    {
      t = file->transactions[i].sort_index;

      convert_date_to_string (file->transactions[t].date, buffer);
      filing_output_delimited_field (out, buffer, format, 0);

      build_account_name_pair (file, file->transactions[t].from, buffer);
      filing_output_delimited_field (out, buffer, format, 0);

      build_account_name_pair (file, file->transactions[t].to, buffer);
      filing_output_delimited_field (out, buffer, format, 0);

      filing_output_delimited_field (out, file->transactions[t].reference, format, 0);

      convert_money_to_string (file->transactions[t].amount, buffer);
      filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);

      filing_output_delimited_field (out, file->transactions[t].description, format, DELIMIT_LAST);
    }

    /* Close the file and set the type correctly. */

    fclose (out);
    osfile_set_type (filename, (bits) filetype);

    hourglass_off ();
  }
  else
  {
    error_msgs_report_error ("FileSaveFail");
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void export_delimited_accounts_file (file_data *file, int entry, char *filename, int format, int filetype)
{
  FILE           *out;
  int            i;
  char           buffer[256];
  account_window *window;

  out = fopen (filename, "w");

  if (out != NULL)
  {
    hourglass_on ();

    window = &(file->account_windows[entry]);

    /* Output the headings line, taking the text from the window icons. */

    icons_copy_text (window->account_pane, 0, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (window->account_pane, 1, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (window->account_pane, 2, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (window->account_pane, 3, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    icons_copy_text (window->account_pane, 4, buffer);
    filing_output_delimited_field (out, buffer, format, DELIMIT_LAST);

    /* Output the transaction data as a set of delimited lines. */

    for (i=0; i < window->display_lines; i++)
    {
      if (window->line_data[i].type == ACCOUNT_LINE_DATA)
      {
        build_account_name_pair (file, window->line_data[i].account, buffer);
        filing_output_delimited_field (out, buffer, format, 0);

        switch (window->type)
        {
          case ACCOUNT_FULL:
            convert_money_to_string (file->accounts[window->line_data[i].account].statement_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].current_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].trial_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].budget_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
            break;

          case ACCOUNT_IN:
            convert_money_to_string (-file->accounts[window->line_data[i].account].future_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].budget_amount, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (-file->accounts[window->line_data[i].account].budget_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].budget_result, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
            break;

          case ACCOUNT_OUT:
            convert_money_to_string (file->accounts[window->line_data[i].account].future_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].budget_amount, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].budget_balance, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
            convert_money_to_string (file->accounts[window->line_data[i].account].budget_result, buffer);
            filing_output_delimited_field (out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
            break;
        }
      }
      else if (window->line_data[i].type == ACCOUNT_LINE_HEADER)
      {
        filing_output_delimited_field (out, window->line_data[i].heading, format, 1);
      }
      else if (window->line_data[i].type == ACCOUNT_LINE_FOOTER)
      {
        filing_output_delimited_field (out, window->line_data[i].heading, format, 0);

        convert_money_to_string (window->line_data[i].total[0], buffer);
        filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);

        convert_money_to_string (window->line_data[i].total[1], buffer);
        filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);

        convert_money_to_string (window->line_data[i].total[2], buffer);
        filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);

        convert_money_to_string (window->line_data[i].total[3], buffer);
        filing_output_delimited_field (out, buffer, format, DELIMIT_NUM | DELIMIT_LAST);
      }
    }

    /* Output the grand total line, taking the text from the window icons. */

    icons_copy_text (window->account_footer, 0, buffer);
    filing_output_delimited_field (out, buffer, format, 0);
    filing_output_delimited_field (out, window->footer_icon[0], format, DELIMIT_NUM);
    filing_output_delimited_field (out, window->footer_icon[1], format, DELIMIT_NUM);
    filing_output_delimited_field (out, window->footer_icon[2], format, DELIMIT_NUM);
    filing_output_delimited_field (out, window->footer_icon[3], format, DELIMIT_NUM | DELIMIT_LAST);

    /* Close the file and set the type correctly. */

    fclose (out);
    osfile_set_type (filename, (bits) filetype);

    hourglass_off ();
  }
  else
  {
    error_msgs_report_error ("FileSaveFail");
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void export_delimited_account_file (file_data *file, int account, char *filename, int format, int filetype)
{
  FILE           *out;
  int            i, transaction=0;
  char           buffer[256];
  accview_window *window;

  out = fopen (filename, "w");

  if (out != NULL)
  {
    hourglass_on ();

    if (account != NULL_ACCOUNT && file->accounts[account].account_view != NULL)
    {
      window = file->accounts[account].account_view;

      /* Output the headings line, taking the text from the window icons. */

      icons_copy_text (window->accview_pane, 0, buffer);
      filing_output_delimited_field (out, buffer, format, 0);
      icons_copy_text (window->accview_pane, 1, buffer);
      filing_output_delimited_field (out, buffer, format, 0);
      icons_copy_text (window->accview_pane, 2, buffer);
      filing_output_delimited_field (out, buffer, format, 0);
      icons_copy_text (window->accview_pane, 3, buffer);
      filing_output_delimited_field (out, buffer, format, 0);
      icons_copy_text (window->accview_pane, 4, buffer);
      filing_output_delimited_field (out, buffer, format, 0);
      icons_copy_text (window->accview_pane, 5, buffer);
      filing_output_delimited_field (out, buffer, format, 0);
      icons_copy_text (window->accview_pane, 6, buffer);
      filing_output_delimited_field (out, buffer, format, DELIMIT_LAST);

      /* Output the transaction data as a set of delimited lines. */
      for (i=0; i < window->display_lines; i++)
      {
        transaction = (window->line_data)[(window->line_data)[i].sort_index].transaction;

        convert_date_to_string (file->transactions[transaction].date, buffer);
        filing_output_delimited_field (out, buffer, format, 0);

        if (file->transactions[transaction].from == account)
        {
          build_account_name_pair (file, file->transactions[transaction].to, buffer);
        }
        else
        {
          build_account_name_pair (file, file->transactions[transaction].from, buffer);
        }
        filing_output_delimited_field (out, buffer, format, 0);

        filing_output_delimited_field (out, file->transactions[transaction].reference, format, 0);

        if (file->transactions[transaction].from == account)
        {
          convert_money_to_string (file->transactions[transaction].amount, buffer);
          filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
          filing_output_delimited_field (out, "", format, DELIMIT_NUM);
        }
        else
        {
          convert_money_to_string (file->transactions[transaction].amount, buffer);
          filing_output_delimited_field (out, "", format, DELIMIT_NUM);
          filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);
        }

        convert_money_to_string (window->line_data[i].balance, buffer);
        filing_output_delimited_field (out, buffer, format, DELIMIT_NUM);

        filing_output_delimited_field (out, file->transactions[transaction].description, format, DELIMIT_LAST);
      }
    }
    /* Close the file and set the type correctly. */

    fclose (out);
    osfile_set_type (filename, (bits) filetype);

    hourglass_off ();
  }
  else
  {
    error_msgs_report_error ("FileSaveFail");
  }
}



/**
 * Output a text string to a file, treating it as a field in a delimited format
 * and applying the necessary quoting as required.
 *
 * \param *f			The file handle to write to.
 * \param *string		The string to write.
 * \param format		The file format to be written.
 * \param flags			Flags indicating addtional formatting to apply.
 * \return			0 on success.
 */

int filing_output_delimited_field(FILE *f, char *string, enum filing_delimit_type format, enum filing_delimit_flags flags)
{
	int	i;
	osbool	quote = FALSE;

	/* Decide whether to enclose in quotes. */

	switch (format) {
	case DELIMIT_TAB:		/* Never quote. */
		quote = FALSE;
		break;

	case DELIMIT_COMMA:		/* Only quote if leading whitespace, trailing whitespace, or enclosed comma. */
		if (isspace(string[0]) || isspace (string[strlen(string)-1]))
			quote = TRUE;

		for (i = 0; string[i] != 0 && quote == 0; i++)
			if (string[i] == ',')
				quote = TRUE;
		break;

	case DELIMIT_QUOTED_COMMA:	/* Always quote. */
		quote = TRUE;
		break;
	}

	if (flags & DELIMIT_NUM)	/* Exception: numbers are never quoted. */
		quote = FALSE;

	/* Output the string. */

	if (quote)
		fprintf(f, "\"%s\"", string);
	else
		fprintf(f, "%s", string);

	/* Output the field separator. */

	if (flags & DELIMIT_LAST)
		fprintf(f, "\n");
	else if (format == DELIMIT_COMMA || format == DELIMIT_QUOTED_COMMA)
		fprintf(f, ",");
	else if (format == DELIMIT_TAB)
		fprintf(f, "\t");

	return 0;
}

/* ==================================================================================================================
 * String processing.
 */

char *unquote_string (char *string)
{
  string = string_strip_surrounding_whitespace (string);

  if (*string == '"' && *(strchr(string, '\0')-1) == '"')
  {
    string++;
    *(strchr(string, '\0')-1) = '\0';
  }

  return (string);
}

/* ------------------------------------------------------------------------------------------------------------------ */

char *next_field (char *line, char sep)
{
  static char *current;
  char        *start;
  int         quoted;

  if (line != NULL)
  {
    current = line;
  }

  quoted = 0;
  start = current;

  while (*current != '\0' && ((*current != sep) || quoted))
  {
    if (*current == '"')
    {
      quoted = !quoted;
    }
    current++;
  }

  if (*current == sep)
  {
    *current = '\0';
    current ++;
  }

  return (start);
}
