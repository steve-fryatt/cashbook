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
 * \file: edit.c
 *
 * Transaction editing implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"

/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "edit.h"

#include "account.h"
#include "accview.h"
#include "calculation.h"
#include "column.h"
#include "conversion.h"
#include "date.h"
#include "edit.h"
#include "file.h"
#include "presets.h"
#include "transact.h"

/* ==================================================================================================================
 * Global variables.
 */

/* A pointer to the file block belomging to the window currently holding the edit line. */

static file_data *entry_window = NULL;

/* The following are the buffers used by the edit line in the transaction window. */

static char      buffer_date[DATE_FIELD_LEN], buffer_from_ident[ACCOUNT_IDENT_LEN],
                 buffer_from_name[ACCOUNT_NAME_LEN], buffer_from_rec[REC_FIELD_LEN],
                 buffer_to_ident[ACCOUNT_IDENT_LEN], buffer_to_name[ACCOUNT_NAME_LEN],
                 buffer_to_rec[REC_FIELD_LEN], buffer_reference [REF_FIELD_LEN],
                 buffer_amount[AMOUNT_FIELD_LEN], buffer_description[DESCRIPT_FIELD_LEN];

wimp_window *edit_transact_window_def = NULL;	/* \TODO -- Ick! */

/* ==================================================================================================================
 * Edit line creation.
 */

/* Put the edit line at the specified point in the given file's transaction window.  Any existing edit line is
 * removed first.
 */

void place_transaction_edit_line (file_data* file, int line)
{
  int                   i, transaction;
  wimp_icon_create      icon_block;


  if (line > -1)
  {
    /* Start by deleting any existing edit line, from any open transaction window. */

    if (entry_window != NULL)
    {
      /* The assumption is that the data will be safe as it's always copied into memory as soon as a
       * key is pressed in any of the writable icons... */

      for (i=0; i<TRANSACT_COLUMNS; i++)
      {
        wimp_delete_icon (entry_window->transaction_window.transaction_window, (wimp_i) i);
      }

      entry_window->transaction_window.entry_line = -1;
      entry_window = NULL;
    }

    /* Extend the window work area if required. */

    if (line >= file->transaction_window.display_lines)
    {
      file->transaction_window.display_lines = line + 1;
      set_transaction_window_extent (file);
    }

    /* Create the icon block required for the icon definitions. */

    icon_block.w = file->transaction_window.transaction_window;

    /* Set up the indirected buffers. */

    edit_transact_window_def->icons[0].data.indirected_text.text = buffer_date;
    edit_transact_window_def->icons[0].data.indirected_text.size = DATE_FIELD_LEN;

    edit_transact_window_def->icons[1].data.indirected_text.text = buffer_from_ident;
    edit_transact_window_def->icons[1].data.indirected_text.size = ACCOUNT_IDENT_LEN;

    edit_transact_window_def->icons[2].data.indirected_text.text = buffer_from_rec;
    edit_transact_window_def->icons[2].data.indirected_text.size = REC_FIELD_LEN;

    edit_transact_window_def->icons[3].data.indirected_text.text = buffer_from_name;
    edit_transact_window_def->icons[3].data.indirected_text.size = ACCOUNT_NAME_LEN;

    edit_transact_window_def->icons[4].data.indirected_text.text = buffer_to_ident;
    edit_transact_window_def->icons[4].data.indirected_text.size = ACCOUNT_IDENT_LEN;

    edit_transact_window_def->icons[5].data.indirected_text.text = buffer_to_rec;
    edit_transact_window_def->icons[5].data.indirected_text.size = REC_FIELD_LEN;

    edit_transact_window_def->icons[6].data.indirected_text.text = buffer_to_name;
    edit_transact_window_def->icons[6].data.indirected_text.size = ACCOUNT_NAME_LEN;

    edit_transact_window_def->icons[7].data.indirected_text.text = buffer_reference;
    edit_transact_window_def->icons[7].data.indirected_text.size = REF_FIELD_LEN;

    edit_transact_window_def->icons[8].data.indirected_text.text = buffer_amount;
    edit_transact_window_def->icons[8].data.indirected_text.size = AMOUNT_FIELD_LEN;

    edit_transact_window_def->icons[9].data.indirected_text.text = buffer_description;
    edit_transact_window_def->icons[9].data.indirected_text.size = DESCRIPT_FIELD_LEN;

    /* Initialise the data. */

    if (line < file->trans_count)
    {
      transaction = file->transactions[line].sort_index;

      convert_date_to_string (file->transactions[transaction].date, buffer_date);
      strcpy (buffer_from_ident, account_get_ident (file, file->transactions[transaction].from));
      strcpy (buffer_from_name, account_get_name (file, file->transactions[transaction].from));
      if (file->transactions[transaction].flags & TRANS_REC_FROM)
      {
        msgs_lookup ("RecChar", buffer_from_rec, REC_FIELD_LEN);
      }
      else
      {
        *buffer_from_rec = '\0';
      }
      strcpy (buffer_to_ident, account_get_ident (file, file->transactions[transaction].to));
      strcpy (buffer_to_name, account_get_name (file, file->transactions[transaction].to));
      if (file->transactions[transaction].flags & TRANS_REC_TO)
      {
        msgs_lookup ("RecChar", buffer_to_rec, REC_FIELD_LEN);
      }
      else
      {
        *buffer_to_rec = '\0';
      }
      strcpy (buffer_reference, file->transactions[transaction].reference);
      convert_money_to_string (file->transactions[transaction].amount, buffer_amount);
      strcpy (buffer_description, file->transactions[transaction].description);
    }
    else
    {
      *buffer_date = '\0';
      *buffer_from_ident = '\0';
      *buffer_from_rec = '\0';
      *buffer_from_name = '\0';
      *buffer_to_ident = '\0';
      *buffer_to_rec = '\0';
      *buffer_to_name = '\0';
      *buffer_reference = '\0';
      *buffer_amount = '\0';
      *buffer_description = '\0';
    }

    /* Set the icon positions correctly and create them. */

    for (i=0; i < TRANSACT_COLUMNS; i++)
    {
      memcpy (&(icon_block.icon), &(edit_transact_window_def->icons[i]), sizeof (wimp_icon));

      icon_block.icon.extent.x0 = file->transaction_window.column_position[i];
      icon_block.icon.extent.x1 = file->transaction_window.column_position[i]
                                  + file->transaction_window.column_width[i];
      icon_block.icon.extent.y0 = (-line * (ICON_HEIGHT+LINE_GUTTER))
                                  - TRANSACT_TOOLBAR_HEIGHT - ICON_HEIGHT;
      icon_block.icon.extent.y1 = (-line * (ICON_HEIGHT+LINE_GUTTER))
                                  - TRANSACT_TOOLBAR_HEIGHT;

      wimp_create_icon (&icon_block);
    }

    /* Update the window data to show the line being edited. */

    file->transaction_window.entry_line = line;
    entry_window = file;

    set_transaction_edit_line_shading (file);
  }

  /* The caret isn't placed in this routine.  That is left up to the caller, depending on their context. */
}

/* ==================================================================================================================
 * Entry line deletion
 */

/* Called if the file bock is to be deleted; removes reference to the edit line if it is within the associated
 * transaction window.  Note that it isn't possible to delete the edit line as such: it only goes if the owning window
 * is deleted.
 */

void transaction_edit_line_block_deleted (file_data *file)
{
  if (entry_window == file)
  {
    entry_window = NULL;
  }
}

/* ==================================================================================================================
 * Entry line operations
 */

/* Bring the edit line in to the window in a vertical direction. */

void find_transaction_edit_line (file_data *file)
{
  wimp_window_state window;
  int               height, top, bottom;


  if (file == entry_window)
  {
    window.w = file->transaction_window.transaction_window;
    wimp_get_window_state (&window);

    /* Calculate the height of the useful visible window, leaving out any OS units taken up by part lines.
     * This will allow the edit line to be aligned with the top or bottom of the window.
     */

    height = window.visible.y1 - window.visible.y0 - ICON_HEIGHT - LINE_GUTTER - TRANSACT_TOOLBAR_HEIGHT;

    /* Calculate the top full line and bottom full line that are showing in the window.  Part lines don't
     * count and are discarded.
     */

    top = (-window.yscroll + ICON_HEIGHT) / (ICON_HEIGHT+LINE_GUTTER);
    bottom = height / (ICON_HEIGHT+LINE_GUTTER) + top;

    #ifdef DEBUG
    debug_printf("\\BFind transaction edit line");
    debug_printf("Top: %d, Bottom: %d, Entry line: %d", top, bottom, file->transaction_window.entry_line);
    #endif

    /* If the edit line is above or below the visible area, bring it into range. */

    if (file->transaction_window.entry_line < top)
    {
      window.yscroll = -(file->transaction_window.entry_line * (ICON_HEIGHT+LINE_GUTTER));
      wimp_open_window ((wimp_open *) &window);
      minimise_transaction_window_extent (file);
    }

    if (file->transaction_window.entry_line > bottom)
    {
      window.yscroll = -((file->transaction_window.entry_line) * (ICON_HEIGHT+LINE_GUTTER) - height);
      wimp_open_window ((wimp_open *) &window);
      minimise_transaction_window_extent (file);
    }
  }
}

/* ----------------------------------------------------------------------------------------------------------------- */

/* Bring the current edit line icon in to the window in a horizontal direction. */

void find_transaction_edit_icon (file_data *file)
{
  wimp_window_state window;
  wimp_icon_state   icon;
  wimp_caret        caret;
  int               window_width, window_xmin, window_xmax,
                    icon_width, icon_xmin, icon_xmax, icon_target, group;


  if (file == entry_window)
  {
    window.w = file->transaction_window.transaction_window;
    wimp_get_window_state (&window);

    wimp_get_caret_position (&caret);
    if (caret.w == window.w && caret.i != -1)
    {
      /* Find the group holding the current icon. */

      group = 0;
      while (caret.i > column_get_rightmost_in_group (TRANSACT_PANE_COL_MAP, group))
      {
        group ++;
      }

      /* Get the left hand icon dimension */

      icon.w = window.w;
      icon.i = column_get_leftmost_in_group (TRANSACT_PANE_COL_MAP, group);
      wimp_get_icon_state (&icon);
      icon_xmin = icon.icon.extent.x0;

      /* Get the right hand icon dimension */

      icon.w = window.w;
      icon.i = column_get_rightmost_in_group (TRANSACT_PANE_COL_MAP, group);
      wimp_get_icon_state (&icon);
      icon_xmax = icon.icon.extent.x1;

      icon_width = icon_xmax - icon_xmin;

      /* Establish the window dimensions. */

      window_width = window.visible.x1 - window.visible.x0;
      window_xmin = window.xscroll;
      window_xmax = window.xscroll + window_width;

      if (window_width > icon_width)
      {
        /* If the icon group fits into the visible window, just pull the overlap into view. */

        if (icon_xmin < window_xmin)
        {
          window.xscroll = icon_xmin;
          wimp_open_window ((wimp_open *) &window);
        }
        else if (icon_xmax > window_xmax)
        {
          window.xscroll = icon_xmax - window_width;
          wimp_open_window ((wimp_open *) &window);
        }
      }
      else
      {
        /* If the icon is bigger than the window, however, get the justification end of the icon and ensure that it
         * is aligned against that side of the window.
         */

        icon.w = window.w;
        icon.i = caret.i;
        wimp_get_icon_state (&icon);

        icon_target = (icon.icon.flags & wimp_ICON_RJUSTIFIED) ? icon.icon.extent.x1 : icon.icon.extent.x0;

        if ((icon_target < window_xmin || icon_target > window_xmax) && !(icon.icon.flags & wimp_ICON_RJUSTIFIED))
        {
          window.xscroll = icon_target;
          wimp_open_window ((wimp_open *) &window);
        }
        else if ((icon_target < window_xmin || icon_target > window_xmax) && (icon.icon.flags & wimp_ICON_RJUSTIFIED))
        {
          window.xscroll = icon_target - window_width;
          wimp_open_window ((wimp_open *) &window);
        }
      }
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Copy the content of the edit line memory back into the icons.  If w is specified, the refresh will only
 * occur if the edit line is in that window; if i is specified, only that icon will be refreshed.  If avoid
 * is specified, that icon will *not* be refreshed.
 */

void refresh_transaction_edit_line_icons (wimp_w w, wimp_i only, wimp_i avoid)
{
  int transaction;

  if (entry_window != NULL &&
      (w == entry_window->transaction_window.transaction_window || w == NULL) &&
      entry_window->transaction_window.entry_line < entry_window->trans_count)
  {
    transaction = entry_window->transactions[entry_window->transaction_window.entry_line].sort_index;

    if (only == EDIT_ICON_DATE || avoid != EDIT_ICON_DATE)
    {
      /* Re-convert the date, so that it is displayed in standard format. */

      convert_date_to_string (entry_window->transactions[transaction].date, buffer_date);
      wimp_set_icon_state (entry_window->transaction_window.transaction_window, 0, 0, 0);
    }

    if (only == EDIT_ICON_FROM || avoid != EDIT_ICON_FROM)
    {
      /* Remove the from ident if it didn't match an account. */

      if (entry_window->transactions[transaction].from == NULL_ACCOUNT)
      {
        *buffer_from_ident = '\0';
        *buffer_from_name = '\0';
        *buffer_from_rec = '\0';
        wimp_set_icon_state (entry_window->transaction_window.transaction_window, 1, 0, 0);
      }
      else
      {
        strcpy (buffer_from_ident, account_get_ident (entry_window, entry_window->transactions[transaction].from));
        strcpy (buffer_from_name, account_get_name (entry_window, entry_window->transactions[transaction].from));
        if (entry_window->transactions[transaction].flags & TRANS_REC_FROM)
        {
          msgs_lookup ("RecChar", buffer_from_rec, REC_FIELD_LEN);
        }
        else
        {
          *buffer_from_rec = '\0';
        }
      }
    }

    if (only == EDIT_ICON_TO || avoid != EDIT_ICON_TO)
    {
      /* Remove the to ident if it didn't match an account. */

      if (entry_window->transactions[transaction].to == NULL_ACCOUNT)
      {
        *buffer_to_ident = '\0';
        *buffer_to_name = '\0';
        *buffer_to_rec = '\0';
        wimp_set_icon_state (entry_window->transaction_window.transaction_window, 4, 0, 0);
      }
      else
      {
        strcpy (buffer_to_ident, account_get_ident (entry_window, entry_window->transactions[transaction].to));
        strcpy (buffer_to_name, account_get_name (entry_window, entry_window->transactions[transaction].to));
        if (entry_window->transactions[transaction].flags & TRANS_REC_TO)
        {
          msgs_lookup ("RecChar", buffer_to_rec, REC_FIELD_LEN);
        }
        else
        {
          *buffer_to_rec = '\0';
        }
      }
    }

    if (only == EDIT_ICON_REF || avoid != EDIT_ICON_REF)
    {
      /* Copy the contents back into the icon. */

      strcpy (buffer_reference, entry_window->transactions[transaction].reference);
      wimp_set_icon_state (entry_window->transaction_window.transaction_window, 7, 0, 0);
    }

    if (only == EDIT_ICON_AMOUNT || avoid != EDIT_ICON_AMOUNT)
    {
      /* Re-convert the amount so that it is displayed in standard format. */

      convert_money_to_string (entry_window->transactions[transaction].amount, buffer_amount);
      wimp_set_icon_state (entry_window->transaction_window.transaction_window, 8, 0, 0);
    }

    if (only == EDIT_ICON_DESCRIPT || avoid != EDIT_ICON_DESCRIPT)
    {
      /* Copy the contents back into the icon. */

      strcpy (buffer_description, entry_window->transactions[transaction].description);
      wimp_set_icon_state (entry_window->transaction_window.transaction_window, 9, 0, 0);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Set the icon foreground colours to indicate a fully reconciled line.
 *
 * This is currently called from the toggle_reconcile_flag (), process_transaction_edit_line_entry_keys () and
 * place_transaction_edit_line () functions.
 */

void set_transaction_edit_line_shading(file_data *file)
{
	int	icon_fg_col, transaction;
	wimp_i	i;

	if (entry_window != file || file->trans_count == 0 || file->transaction_window.entry_line >= file->trans_count)
		return;

	transaction = file->transactions[file->transaction_window.entry_line].sort_index;

	if (config_opt_read("ShadeReconciled") && (file->transaction_window.entry_line < file->trans_count) &&
			((file->transactions[transaction].flags & (TRANS_REC_FROM | TRANS_REC_TO)) == (TRANS_REC_FROM | TRANS_REC_TO)))
		icon_fg_col = (config_int_read("ShadeReconciledColour") << wimp_ICON_FG_COLOUR_SHIFT);
	else
		icon_fg_col = (wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT);

	for (i = 0; i < TRANSACT_COLUMNS; i++)
		wimp_set_icon_state(entry_window->transaction_window.transaction_window, i, icon_fg_col, wimp_ICON_FG_COLOUR);
}

/* ================================================================================================================== */

/* Return the transaction that is currently being edited, to allow the edit line to be replaced after the transaction
 * window's contents is sorted.
 */

int get_edit_line_transaction (file_data *file)
{
  int transaction;

  transaction = NULL_TRANSACTION;

  if (entry_window == file && file->transaction_window.entry_line < file->trans_count)
  {
    transaction = file->transactions[file->transaction_window.entry_line].sort_index;
  }

  return (transaction);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Pick up the marker placed by mark_transcation_edit_line () and move the edit line icons appropriately. */

void place_transaction_edit_line_transaction (file_data *file, int transaction)
{
  int        i;
  wimp_caret caret;

  if (entry_window == file)
  {
    if (transaction != NULL_TRANSACTION)
    {
      for (i=0; i < file->trans_count; i++)
      {
        if (file->transactions[i].sort_index == transaction)
        {
          place_transaction_edit_line (file, i);
          wimp_get_caret_position (&caret);
          if (caret.w == file->transaction_window.transaction_window)
          {
            icons_put_caret_at_end (file->transaction_window.transaction_window, 0);
          }
          find_transaction_edit_line (file);

          break;
        }
      }
    }
    else
    {
      place_transaction_edit_line (file, file->trans_count);
      wimp_get_caret_position (&caret);
      if (caret.w == file->transaction_window.transaction_window)
      {
        icons_put_caret_at_end (file->transaction_window.transaction_window, 0);
      }
      find_transaction_edit_line (file);
    }
  }
}

/* ==================================================================================================================
 * Transaction operations
 */

/* Toggle the specificed reconcile flag for the given transaction. */

void toggle_reconcile_flag (file_data *file, int transaction, int change_flag)
{
  int change_icon, line, changed = 0;


  /* Establish which icon it is that will need to be updated. */

  change_icon = (change_flag == TRANS_REC_FROM) ? EDIT_ICON_FROM_REC : EDIT_ICON_TO_REC;

  /* Only do anything if the transaction is inside the limit of the file. */

  if (transaction < file->trans_count)
  {
    remove_transaction_from_totals (file, transaction);

    /* Update the reconcile flag, either removing it, or adding it in.  If the line is the edit line, the icon
     * contents must be manually updated as well.
     *
     * If a change is made, this is flagged to allow the update to be recorded properly.
     */

    if (file->transactions[transaction].flags & change_flag)
    {
      file->transactions[transaction].flags &= ~change_flag;

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        *icons_get_indirected_text_addr (file->transaction_window.transaction_window, change_icon) = '\0';
      }

      changed = 1;
    }

    else if ((change_flag == TRANS_REC_FROM && file->transactions[transaction].from != NULL_ACCOUNT) ||
             (change_flag == TRANS_REC_TO && file->transactions[transaction].to != NULL_ACCOUNT))
    {
      file->transactions[transaction].flags |= change_flag;

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        msgs_lookup ("RecChar",
                     icons_get_indirected_text_addr (file->transaction_window.transaction_window, change_icon),
                     REC_FIELD_LEN);
      }

      changed = 1;
    }

    /* Return the line to the calculations.   This will automatically update all the account listings. */

    restore_transaction_to_totals (file, transaction);

    /* If any changes were made, refresh the relevant account listing, redraw the transaction window line and
     * mark the file as modified.
     */

    if (changed)
    {
      if (change_flag == TRANS_REC_FROM)
      {
        accview_redraw_transaction (file, file->transactions[transaction].from, transaction);
      }
      else
      {
        accview_redraw_transaction (file, file->transactions[transaction].to, transaction);
      }

      /* If the line is the edit line, setting the shading uses wimp_set_icon_state () and the line will effectively
       * be redrawn for free.
       */

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        set_transaction_edit_line_shading (file);
      }
      else
      {
        line = locate_transaction_in_transact_window (file, transaction);
        force_transaction_window_redraw (file, line, line);
      }

      set_file_data_integrity (file, 1);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void edit_change_transaction_date (file_data *file, int transaction, date_t new_date)
{
  int          line, changed = 0;
  date_t       old_date = NULL_DATE;


  /* Only do anything if the transaction is inside the limit of the file. */

  if (transaction < file->trans_count)
  {
    remove_transaction_from_totals (file, transaction);

    /* Look up the existing date, change it and compare the two.  If the field has changed, flag this up.
     */

    old_date = file->transactions[transaction].date;

    file->transactions[transaction].date = new_date;

    if (old_date != file->transactions[transaction].date)
    {
      changed = 1;
      file->sort_valid = 0;
    }

    /* Return the line to the calculations.   This will automatically update all the account listings. */

    restore_transaction_to_totals (file, transaction);

    /* If any changes were made, refresh the relevant account listings, redraw the transaction window line and
     * mark the file as modified.
     */

    if (changed)
    {
      /* Ideally, we would want to recalculate just the affected two accounts.  However, because the date sort is
       * unclean, any rebuild will force a resort of the transactions, which will require a full rebuild of all the
       * open account views.  Therefore, call accview_recalculate_all () to force a full recalculation.  This
       * will in turn sort the data if required.
       *
       * The big assumption here is that, because no from or to entries have changed, none of the accounts will
       * change length and so a full rebuild is not required.
       */

      accview_recalculate_all (file);

      /* If the line is the edit line, setting the shading uses wimp_set_icon_state () and the line will effectively
       * be redrawn for free.
       */

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, EDIT_ICON_DATE, -1);
        set_transaction_edit_line_shading (file);
        icons_replace_caret_in_window (file->transaction_window.transaction_window);
      }
      else
      {
        line = locate_transaction_in_transact_window (file, transaction);
        force_transaction_window_redraw (file, line, line);
      }

      set_file_data_integrity (file, 1);
    }
  }
}


/* ------------------------------------------------------------------------------------------------------------------ */

void edit_change_transaction_amount (file_data *file, int transaction, amt_t new_amount)
{
  int          line, changed = 0;
  amt_t        old_amount = NULL_CURRENCY;


  /* Only do anything if the transaction is inside the limit of the file. */

  if (transaction < file->trans_count)
  {
    remove_transaction_from_totals (file, transaction);

    /* Look up the existing date, change it and compare the two.  If the field has changed, flag this up.
     */

    old_amount = file->transactions[transaction].amount;

    file->transactions[transaction].amount = new_amount;

    if (old_amount != file->transactions[transaction].amount)
    {
      changed = 1;
    }

    /* Return the line to the calculations.   This will automatically update all the account listings. */

    restore_transaction_to_totals (file, transaction);

    /* If any changes were made, refresh the relevant account listings, redraw the transaction window line and
     * mark the file as modified.
     */

    if (changed)
    {
      accview_recalculate (file, file->transactions[transaction].from, transaction);
      accview_recalculate (file, file->transactions[transaction].to, transaction);

      /* If the line is the edit line, setting the shading uses wimp_set_icon_state () and the line will effectively
       * be redrawn for free.
       */

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, EDIT_ICON_DATE, -1);
        set_transaction_edit_line_shading (file);
        icons_replace_caret_in_window (file->transaction_window.transaction_window);
      }
      else
      {
        line = locate_transaction_in_transact_window (file, transaction);
        force_transaction_window_redraw (file, line, line);
      }

      set_file_data_integrity (file, 1);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void edit_change_transaction_refdesc (file_data *file, int transaction, int change_icon, char *new_text)
{
  int          line, changed = 0;
  char         *field;


  /* Only do anything if the transaction is inside the limit of the file. */

  if (transaction < file->trans_count)
  {
    /* Find the field that will be getting changed. */

    switch (change_icon)
    {
      case EDIT_ICON_REF:
        field = file->transactions[transaction].reference;
        break;

      case EDIT_ICON_DESCRIPT:
        field = file->transactions[transaction].description;
        break;

      default:
        field = NULL;
        break;
    }

    if (field != NULL)
    {
      /* Copy the existing field contents away. */

      if (strcmp (field, new_text) != 0)
      {
        changed = 1;
      }

      strcpy (field, new_text);
    }

    /* If any changes were made, refresh the relevant account listings, redraw the transaction window line and
     * mark the file as modified.
     */

    if (changed)
    {
      /* Refresh any account views that may be affected. */

      accview_redraw_transaction (file, file->transactions[transaction].from, transaction);
      accview_redraw_transaction (file, file->transactions[transaction].to, transaction);

      /* If the line is the edit line, setting the shading uses wimp_set_icon_state () and the line will effectively
       * be redrawn for free.
       */

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, EDIT_ICON_DATE, -1);
        set_transaction_edit_line_shading (file);
        icons_replace_caret_in_window (file->transaction_window.transaction_window);
      }
      else
      {
        line = locate_transaction_in_transact_window (file, transaction);
        force_transaction_window_redraw (file, line, line);
      }

      set_file_data_integrity (file, 1);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void edit_change_transaction_account (file_data *file, int transaction, int change_icon, acct_t new_account)
{
  int          line, changed = 0;
  unsigned int old_flags;
  acct_t       old_acct = NULL_ACCOUNT;


  /* Only do anything if the transaction is inside the limit of the file. */

  if (transaction < file->trans_count)
  {
    remove_transaction_from_totals (file, transaction);

    /* Update the reconcile flag, either removing it, or adding it in.  If the line is the edit line, the icon
     * contents must be manually updated as well.
     *
     * If a change is made, this is flagged to allow the update to be recorded properly.
     */

    if (change_icon == EDIT_ICON_FROM)
    {
      /* Look up the account ident as it stands, store the result and update the name field.  The reconciled flag is
       * set if the account changes to an income heading; else it is cleared.
       */

      old_acct = file->transactions[transaction].from;
      old_flags = file->transactions[transaction].flags;

      /* old_acct is used in the second section, to refresh the original account. */

      file->transactions[transaction].from = new_account;

      if (file->accounts[new_account].type == ACCOUNT_FULL)
      {
        file->transactions[transaction].flags &= ~TRANS_REC_FROM;
      }
      else
      {
        file->transactions[transaction].flags |= TRANS_REC_FROM;
      }

      if (old_acct != file->transactions[transaction].from || old_flags != file->transactions[transaction].flags)
      {
        /* Trust that any accuout views that are open must be based on a valid date order, and only rebuild those
         * that are directly affected.
         */

        changed = 1;
      }
    }

    else if (change_icon == EDIT_ICON_TO)
    {
      /* Look up the account ident as it stands, store the result and update the name field. */

      old_acct = file->transactions[transaction].to;
      old_flags = file->transactions[transaction].flags;

      /* old_acct is used in the second section, to refresh the original account. */

      file->transactions[transaction].to = new_account;

      if (file->accounts[new_account].type == ACCOUNT_FULL)
      {
        file->transactions[transaction].flags &= ~TRANS_REC_TO;
      }
      else
      {
        file->transactions[transaction].flags |= TRANS_REC_TO;
      }

      if (old_acct != file->transactions[transaction].to || old_flags != file->transactions[transaction].flags)
      {
        changed = 1;
      }
    }

    /* Return the line to the calculations.   This will automatically update all the account listings. */

    restore_transaction_to_totals (file, transaction);

    /* If any changes were made, refresh the relevant account listing, redraw the transaction window line and
     * mark the file as modified.
     */

    if (changed)
    {
      if (change_icon == EDIT_ICON_FROM)
      {
        accview_rebuild (file, old_acct);
        accview_rebuild (file, file->transactions[transaction].from);
        accview_redraw_transaction (file, file->transactions[transaction].to, transaction);
      }
      else if (change_icon == EDIT_ICON_TO)
      {
        accview_rebuild (file, old_acct);
        accview_rebuild (file, file->transactions[transaction].to);
        accview_redraw_transaction (file, file->transactions[transaction].from, transaction);
      }

      /* If the line is the edit line, setting the shading uses wimp_set_icon_state () and the line will effectively
       * be redrawn for free.
       */

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, change_icon, -1);
        set_transaction_edit_line_shading (file);
        icons_replace_caret_in_window (file->transaction_window.transaction_window);
      }
      else
      {
        line = locate_transaction_in_transact_window (file, transaction);
        force_transaction_window_redraw (file, line, line);
      }

      set_file_data_integrity (file, 1);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void insert_transaction_preset_full (file_data *file, int transaction, int preset)
{
  int          line, changed = 0;


  if (transaction < file->trans_count && preset != NULL_PRESET && preset < file->preset_count)
  {
    remove_transaction_from_totals (file, transaction);

    changed = insert_transaction_preset (file, transaction, preset);

    /* Return the line to the calculations.   This will automatically update all the account listings. */

    restore_transaction_to_totals (file, transaction);

    /* If any changes were made, refresh the relevant account listing, redraw the transaction window line and
     * mark the file as modified.
     */

    place_transaction_edit_line_transaction (file, transaction);

    icons_put_caret_at_end (file->transaction_window.transaction_window,
                      edit_convert_preset_icon_number (preset_get_caret_destination(file, preset)));

    if (changed)
    {
      accview_rebuild_all (file);

      /* If the line is the edit line, setting the shading uses wimp_set_icon_state () and the line will effectively
       * be redrawn for free.
       */

      if (file->transactions[file->transaction_window.entry_line].sort_index == transaction)
      {
        refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, -1, -1);
        set_transaction_edit_line_shading (file);
        icons_replace_caret_in_window (file->transaction_window.transaction_window);
      }
      else
      {
        line = locate_transaction_in_transact_window (file, transaction);
        force_transaction_window_redraw (file, line, line);
      }

      set_file_data_integrity (file, 1);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

int insert_transaction_preset (file_data *file, int transaction, int preset)
{
	int		changed = 0;

	/* Only do anything if the transaction is inside the limit of the file. */

	/* This function is assumed to be called by code that takes care of updating the transaction record and
	 * all the associated totals.  Normally, this would be done by wrapping the call up inside a pair of
	 * remove_transaction_from_totals () and restore_transaction_to_totals (file, transaction).
	 *
	 * We call this direct from process_transaction_edit_line_entry_keys (); from elsewhere, we call it via
	 * insert_transaction_preset_full (), which does all of the wrapping up.
	 */

	if (transaction < file->trans_count && preset != NULL_PRESET && preset < file->preset_count)
		changed = preset_apply(file, preset, &(file->transactions[transaction].date),
				&(file->transactions[transaction].from),
				&(file->transactions[transaction].to),
				&(file->transactions[transaction].flags),
				&(file->transactions[transaction].amount),
				file->transactions[transaction].reference,
				file->transactions[transaction].description);

	return changed;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Take an icon number as used in the preset blocks, and convert it to an icon number for the transaction
 * edit line.
 */

wimp_i edit_convert_preset_icon_number (int caret)
{
  wimp_i icon;

  switch (caret)
  {
    case PRESET_CARET_DATE:
      icon = EDIT_ICON_DATE;
      break;

    case PRESET_CARET_FROM:
      icon = EDIT_ICON_FROM;
      break;

    case PRESET_CARET_TO:
      icon = EDIT_ICON_TO;
      break;

    case PRESET_CARET_REFERENCE:
      icon = EDIT_ICON_REF;
      break;

    case PRESET_CARET_AMOUNT:
      icon = EDIT_ICON_AMOUNT;
      break;

    case PRESET_CARET_DESCRIPTION:
      icon = EDIT_ICON_DESCRIPT;
      break;

    default:
      icon = EDIT_ICON_DATE;
      break;
  }

  return (icon);
}

/* ------------------------------------------------------------------------------------------------------------------ */

void delete_edit_line_transaction (file_data *file)
{
  unsigned transaction;


  /* Only start if the delete line option is enabled, the file is the current entry window, and the line is in range.
   */

  if (config_opt_read("AllowTransDelete") &&
      entry_window == file && file->transaction_window.entry_line < file->trans_count)
  {
    transaction = file->transactions[file->transaction_window.entry_line].sort_index;

    /* Take the transaction out of the fully calculated results. */

    remove_transaction_from_totals (file, transaction);

    /* Blank out all of the transaction. */

    file->transactions[transaction].date = NULL_DATE;
    file->transactions[transaction].from = NULL_ACCOUNT;
    file->transactions[transaction].to = NULL_ACCOUNT;
    file->transactions[transaction].flags = NULL_TRANS_FLAGS;
    file->transactions[transaction].amount = NULL_CURRENCY;
    *(file->transactions[transaction].reference) = '\0';
    *(file->transactions[transaction].description) = '\0';

    /* Put the transaction back into the sorted totals. */

    restore_transaction_to_totals (file, transaction);

    /* Mark the data as unsafe and perform any post-change recalculations that may affect the order of the
     * transaction data.
     */

    file->sort_valid = 0;

    accview_rebuild_all (file);

    refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, -1, -1);
    set_transaction_edit_line_shading (file);

    set_file_data_integrity (file, 1);
  }
}

/* ==================================================================================================================
 * Keypress handling
 */

/* Handle the specific keys in the edit line.  Any that are not recognised go on to the update handler below. */

osbool process_transaction_edit_line_keypress (file_data *file, wimp_key *key)
{
  wimp_caret   caret;
  wimp_i       icon;
  int          transaction, previous;


  /* Ctrl-F10 deletes the whole line.
   */

  if (key->c == wimp_KEY_F10 + wimp_KEY_CONTROL)
  {
    delete_edit_line_transaction (file);
  }

  /* Up and down cursor keys -- move the edit line up or down a row at a time, refreshing the icon the caret
   * was in first.
   */

  else if (key->c == wimp_KEY_UP)
  {
    if (entry_window == file && file->transaction_window.entry_line > 0)
    {
      wimp_get_caret_position (&caret);
      refresh_transaction_edit_line_icons (entry_window->transaction_window.transaction_window, caret.i, -1);
      place_transaction_edit_line (file, file->transaction_window.entry_line-1);
      wimp_set_caret_position (caret.w, caret.i, caret.pos.x, caret.pos.y - (ICON_HEIGHT+LINE_GUTTER), -1, -1);
      find_transaction_edit_line (file);
    }
  }
  else if (key->c == wimp_KEY_DOWN)
  {
    if (entry_window == file)
    {
      wimp_get_caret_position (&caret);
      refresh_transaction_edit_line_icons (entry_window->transaction_window.transaction_window, caret.i, -1);
      place_transaction_edit_line (file, file->transaction_window.entry_line+1);
      wimp_set_caret_position (caret.w, caret.i, caret.pos.x, caret.pos.y + (ICON_HEIGHT+LINE_GUTTER), -1, -1);
      find_transaction_edit_line (file);
    }
  }

  /* Return and Tab keys -- move the caret into the next icon, refreshing the icon it was moved from first.  Wrap
   * at the end of a line.
   */

  else if (key->c == wimp_KEY_RETURN || key->c == wimp_KEY_TAB || key->c == wimp_KEY_TAB + wimp_KEY_CONTROL)
  {
    if (entry_window == file)
    {
      wimp_get_caret_position (&caret);
      if ((osbyte1 (osbyte_SCAN_KEYBOARD, 129, 0) == 0xff) && (file->transaction_window.entry_line > 0))
      {
        /* Test for Ctrl-Tab or Ctrl-Return, and fill down from the line above if present. */

        /* Find the previous or next transaction. If the entry line falls within the actual transactions, we just
         * set up two pointers.  If it is on the line after the final transaction, add a new one and again set the
         * pointers.  Otherwise, the line before MUST be blank, so we do nothing.
         */

        if (file->transaction_window.entry_line <= file->trans_count)
        {
          if (file->transaction_window.entry_line == file->trans_count)
          {
            add_raw_transaction (file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, NULL_TRANS_FLAGS, NULL_CURRENCY, "", "");
          }
          transaction = file->transactions[file->transaction_window.entry_line].sort_index;
          previous = file->transactions[file->transaction_window.entry_line-1].sort_index;
        }
        else
        {
          transaction = -1;
          previous = -1;
        }

        /* If there is a transaction to fill in, use appropriate routines to do the work.
         */

        if (transaction > -1)
        {
          switch (caret.i)
          {
            case EDIT_ICON_DATE:
              edit_change_transaction_date (file, transaction, file->transactions[previous].date);
              break;

            case EDIT_ICON_FROM:
              edit_change_transaction_account (file, transaction, EDIT_ICON_FROM, file->transactions[previous].from);
              if ((file->transactions[previous].flags & TRANS_REC_FROM) !=
                  (file->transactions[transaction].flags & TRANS_REC_FROM))
              {
                toggle_reconcile_flag (file, transaction, TRANS_REC_FROM);
              }
              break;

            case EDIT_ICON_TO:
              edit_change_transaction_account (file, transaction, EDIT_ICON_TO, file->transactions[previous].to);
              if ((file->transactions[previous].flags & TRANS_REC_TO) !=
                  (file->transactions[transaction].flags & TRANS_REC_TO))
              {
                toggle_reconcile_flag (file, transaction, TRANS_REC_TO);
              }
              break;

            case EDIT_ICON_REF:
              edit_change_transaction_refdesc (file, transaction, EDIT_ICON_REF,
                                               file->transactions[previous].reference);
              break;

            case EDIT_ICON_AMOUNT:
              edit_change_transaction_amount (file, transaction, file->transactions[previous].amount);
              break;

            case EDIT_ICON_DESCRIPT:
              edit_change_transaction_refdesc (file, transaction, EDIT_ICON_DESCRIPT,
                                               file->transactions[previous].description);
              break;
          }
        }
      }
      else
      {
        /* There's no point refreshing the line if we've just done a Ctrl- complete, as the relevant subroutines
         * will already have done the work.
         */
        refresh_transaction_edit_line_icons (entry_window->transaction_window.transaction_window, caret.i, -1);
      }
      if (key->c == wimp_KEY_RETURN &&
          ((caret.i == EDIT_ICON_DATE     && (file->transaction_window.sort_order & SORT_MASK) == SORT_DATE) ||
           (caret.i == EDIT_ICON_FROM     && (file->transaction_window.sort_order & SORT_MASK) == SORT_FROM) ||
           (caret.i == EDIT_ICON_TO       && (file->transaction_window.sort_order & SORT_MASK) == SORT_TO) ||
           (caret.i == EDIT_ICON_REF      && (file->transaction_window.sort_order & SORT_MASK) == SORT_REFERENCE) ||
           (caret.i == EDIT_ICON_AMOUNT   && (file->transaction_window.sort_order & SORT_MASK) == SORT_AMOUNT) ||
           (caret.i == EDIT_ICON_DESCRIPT && (file->transaction_window.sort_order & SORT_MASK) == SORT_DESCRIPTION)) &&
            config_opt_read ("AutoSort"))
      {
        transact_sort (file);
        if (file->transaction_window.entry_line < file->trans_count)
        {
          accview_sort (file,
                file->transactions[file->transactions[file->transaction_window.entry_line].sort_index].from);
          accview_sort (file,
                file->transactions[file->transactions[file->transaction_window.entry_line].sort_index].to);
        }
      }

      if (caret.i < EDIT_ICON_DESCRIPT)
      {
        icon = caret.i + 1;
        if (icon == EDIT_ICON_FROM_REC)
        {
          icon = EDIT_ICON_TO;
        }
        if (icon == EDIT_ICON_TO_REC)
        {
          icon = EDIT_ICON_REF;
        }
        icons_put_caret_at_end (entry_window->transaction_window.transaction_window, icon);
        find_transaction_edit_icon (file);
      }
      else
      {
        if (key->c == wimp_KEY_RETURN)
        {
          place_transaction_edit_line (file, find_first_blank_line(file));
        }
        else
        {
          place_transaction_edit_line (file, file->transaction_window.entry_line+1);
        }
        icons_put_caret_at_end (entry_window->transaction_window.transaction_window, EDIT_ICON_DATE);
        find_transaction_edit_icon (file);
        find_transaction_edit_line (file);
      }
    }
  }

  /* Shift-Tab key -- move the caret back to the previous icon, refreshing the icon moved from first.  Wrap up at
   * the start of a line.
   */

  else if (key->c == (wimp_KEY_TAB + wimp_KEY_SHIFT))
  {
    if (entry_window == file)
    {
      wimp_get_caret_position (&caret);
      refresh_transaction_edit_line_icons (entry_window->transaction_window.transaction_window, caret.i, -1);
      if (caret.i > EDIT_ICON_DATE)
      {
        icon = caret.i - 1;
        if (icon == EDIT_ICON_TO_NAME)
        {
          icon = EDIT_ICON_TO;
        }
        if (icon == EDIT_ICON_FROM_NAME)
        {
          icon = EDIT_ICON_FROM;
        }
        icons_put_caret_at_end (entry_window->transaction_window.transaction_window, icon);
        find_transaction_edit_icon (file);
      }
      else
      {
        if (file->transaction_window.entry_line > 0)
        {
          place_transaction_edit_line (file, file->transaction_window.entry_line-1);
          icons_put_caret_at_end (entry_window->transaction_window.transaction_window, EDIT_ICON_DESCRIPT);
          find_transaction_edit_icon (file);
        }
      }
    }
  }

  /* Any unrecognised keys get passed on to the final stage. */

  else
  {
    process_transaction_edit_line_entry_keys (file, key);
    return (key->c != wimp_KEY_F12 &&
    		key->c != (wimp_KEY_SHIFT | wimp_KEY_F12) &&
    		key->c != (wimp_KEY_CONTROL | wimp_KEY_F12) &&
    		key->c != (wimp_KEY_SHIFT | wimp_KEY_CONTROL | wimp_KEY_F12)) ? TRUE : FALSE;
  }

  return TRUE;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Handling any keys that are not recognised as general hotkeys or caret movement keys. */

void process_transaction_edit_line_entry_keys (file_data *file, wimp_key *key)
{
  int          line, transaction, i, reconciled, changed, preset;
  unsigned int previous_date, date, amount, old_acct, old_flags;

  changed = 0;
  preset = NULL_PRESET;
  old_acct = NULL_ACCOUNT;

  if (entry_window == file)
  {
    /* If there is not a transaction entry for the current edit line location (ie. if this is the first keypress
     * in a new line), extend the transaction entries to reach the current location.
     */

    line = file->transaction_window.entry_line;

    if (line >= file->trans_count)
    {
      for (i=file->trans_count; i<=line; i++)
      {
        add_raw_transaction (file, NULL_DATE, NULL_ACCOUNT, NULL_ACCOUNT, NULL_TRANS_FLAGS, NULL_CURRENCY, "", "");
      }
    }

    transaction = file->transactions[line].sort_index;

    /* Take the transaction out of the fully calculated results.
     *
     * Presets occur with the caret in the Date column, so they will have the transaction correctly removed
     * before anything happens.
     */

    if (key->i != EDIT_ICON_REF && key->i != EDIT_ICON_DESCRIPT)
    {
      remove_transaction_from_totals (file, transaction);
    }

    /* Process the keypress.
     *
     * Care needs to be taken that between here and the call to restore_transaction_to_totals () that nothing is
     * done which will affect the sort order of the transaction data.  If the order is changed, the calculated totals
     * will be incorrect until a full recalculation is performed.
     */

    if (key->i == EDIT_ICON_DATE)
    {
      if (isalpha(key->c))
      {
        preset = preset_find_from_keypress (file, toupper(key->c));

        if (preset != NULL_PRESET)
        {
          if ((changed = insert_transaction_preset (file, transaction, preset))) /* Value returned is a bit field. */
          {
            if (changed & (1 << EDIT_ICON_DATE))
            {
              file->sort_valid = 0;
           }
         }
        }
      }
      else
      {
        if (key->c == wimp_KEY_F1)
        {
          convert_date_to_string (get_current_date (), buffer_date);
          wimp_set_icon_state (key->w, EDIT_ICON_DATE, 0, 0);
          icons_replace_caret_in_window (key->w);
        }

        previous_date = (line > 0) ? file->transactions[file->transactions[line - 1].sort_index].date : NULL_DATE;
        date = convert_string_to_date (buffer_date, previous_date, 0);
        if (date != file->transactions[transaction].date)
        {
          file->transactions[transaction].date = date;
          changed = 1;
          file->sort_valid = 0;
        }
      }
    }

    else if (key->i == EDIT_ICON_FROM)
    {
      /* Look up the account ident as it stands, store the result and update the name field.  The reconciled flag is
       * set if the account changes to an income heading; else it is cleared.
       */

      old_acct = file->transactions[transaction].from;
      old_flags = file->transactions[transaction].flags;

      /* old_acct is used in the second section, to refresh the original account. */

      file->transactions[transaction].from =
          lookup_account_field (file, key->c, ACCOUNT_IN | ACCOUNT_FULL,
                                file->transactions[transaction].from, &reconciled,
                                file->transaction_window.transaction_window, 1, 3, 2);


      if (reconciled)
      {
        file->transactions[transaction].flags |= TRANS_REC_FROM;
      }
      else
      {
        file->transactions[transaction].flags &= ~TRANS_REC_FROM;
      }

      set_transaction_edit_line_shading (file);

      if (old_acct != file->transactions[transaction].from || old_flags != file->transactions[transaction].flags)
      {
        /* Trust that any accuout views that are open must be based on a valid date order, and only rebuild those
         * that are directly affected.
         */

        changed = 1;
      }
    }

    else if (key->i == EDIT_ICON_TO)
    {
      /* Look up the account ident as it stands, store the result and update the name field. */

      old_acct = file->transactions[transaction].to;
      old_flags = file->transactions[transaction].flags;

      /* old_acct is used in the second section, to refresh the original account. */

      file->transactions[transaction].to =
          lookup_account_field (file, key->c, ACCOUNT_OUT | ACCOUNT_FULL,
                                file->transactions[transaction].to, &reconciled,
                                file->transaction_window.transaction_window, 4, 6, 5);

      if (reconciled)
      {
        file->transactions[transaction].flags |= TRANS_REC_TO;
      }
      else
      {
        file->transactions[transaction].flags &= ~TRANS_REC_TO;
      }

      set_transaction_edit_line_shading (file);

      if (old_acct != file->transactions[transaction].to || old_flags != file->transactions[transaction].flags)
      {
        changed = 1;
      }
    }

    else if (key->i == EDIT_ICON_REF)
    {
      if (key->c == wimp_KEY_F1)
      {
      account_get_next_cheque_number (file, file->transactions[transaction].from, file->transactions[transaction].to,
                              1, buffer_reference, sizeof(buffer_reference));
        wimp_set_icon_state (key->w, EDIT_ICON_REF, 0, 0);
        icons_replace_caret_in_window (key->w);
      }

      if (strcmp (file->transactions[transaction].reference, buffer_reference) != 0)
      {
        strcpy (file->transactions[transaction].reference, buffer_reference);
        changed = 1;
      }
    }

    else if (key->i == EDIT_ICON_AMOUNT)
    {
      amount = convert_string_to_money (buffer_amount);
      if (amount != file->transactions[transaction].amount)
      {
        file->transactions[transaction].amount = amount;
        changed = 1;
      }
    }

    else if (key->i == EDIT_ICON_DESCRIPT)
    {
      if (key->c == wimp_KEY_F1)
      {
        find_complete_description (file, line, buffer_description);
        wimp_set_icon_state (key->w, EDIT_ICON_DESCRIPT, 0, 0);
        icons_replace_caret_in_window (key->w);
      }

      if (strcmp (file->transactions[transaction].description, buffer_description) != 0)
      {
        strcpy (file->transactions[transaction].description, buffer_description);
        changed = 1;
      }
    }

    /* Add the transaction back into the accounts calculations.
     *
     * From this point on, it is now OK to change the sort order of the transaction data again!
     */

    if (key->i != EDIT_ICON_REF && key->i != EDIT_ICON_DESCRIPT)
    {
      restore_transaction_to_totals (file, transaction);
    }

    /* Mark the data as unsafe and perform any post-change recalculations that may affect the order of the
     * transaction data.
     */

    if (changed) /* Changed will be 1 or 0, unless it was from a preset, in which case it is a bitfield. */
    {
      set_file_data_integrity (file, 1);

      if (preset != NULL_PRESET)
      {
        /* There is a special case for a preset, since although the caret may have been in the Date column,
         * the effects of the update affect all columns and have much wider ramifications.  This is the only
         * update code that runs for preset entries.
         *
         * Unlike all the other options, presets must refresh the line on screen too.
         */

        accview_rebuild_all (file);

        refresh_transaction_edit_line_icons (file->transaction_window.transaction_window, -1, -1);
        set_transaction_edit_line_shading (file);
        icons_put_caret_at_end (file->transaction_window.transaction_window,
                          edit_convert_preset_icon_number (preset_get_caret_destination(file, preset)));

        /* If we're auto-sorting, and the sort column has been updated as part of the preset,
         * then do an auto sort now.
         *
         * We will always sort if the sort column is Date, because pressing a preset key is analagous to
         * hitting Return.
         */

        if ((((file->transaction_window.sort_order & SORT_MASK) == SORT_DATE) ||
             ((changed & (1 << EDIT_ICON_FROM))
                             && ((file->transaction_window.sort_order & SORT_MASK) == SORT_FROM)) ||
             ((changed & (1 << EDIT_ICON_TO))
                             && ((file->transaction_window.sort_order & SORT_MASK) == SORT_TO)) ||
             ((changed & (1 << EDIT_ICON_REF))
                             && ((file->transaction_window.sort_order & SORT_MASK) == SORT_REFERENCE)) ||
             ((changed & (1 << EDIT_ICON_AMOUNT))
                             && ((file->transaction_window.sort_order & SORT_MASK) == SORT_AMOUNT)) ||
             ((changed & (1 << EDIT_ICON_DESCRIPT))
                             && ((file->transaction_window.sort_order & SORT_MASK) == SORT_DESCRIPTION))) &&
            config_opt_read ("AutoSort"))
        {
          transact_sort (file);
          if (file->transaction_window.entry_line < file->trans_count)
          {
            accview_sort (file,
                  file->transactions[file->transactions[file->transaction_window.entry_line].sort_index].from);
            accview_sort (file,
                  file->transactions[file->transactions[file->transaction_window.entry_line].sort_index].to);
          }
        }
      }

      else if (key->i == EDIT_ICON_DATE)
      {
        /* Ideally, we would want to recalculate just the affected two accounts.  However, because the date sort is
         * unclean, any rebuild will force a resort of the transactions, which will require a full rebuild of all the
         * open account views.  Therefore, call accview_recalculate_all () to force a full recalculation.  This
         * will in turn sort the data if required.
         *
         * The big assumption here is that, because no from or to entries have changed, none of the accounts will
         * change length and so a full rebuild is not required.
         */

        accview_recalculate_all (file);
      }

      else if (key->i == EDIT_ICON_FROM)
      {
        /* Trust that any accuout views that are open must be based on a valid date order, and only rebuild those
         * that are directly affected.
         */
        accview_rebuild (file, old_acct);
        transaction = file->transactions[line].sort_index;
        accview_rebuild (file, file->transactions[transaction].from);
        transaction = file->transactions[line].sort_index;
        accview_redraw_transaction (file, file->transactions[transaction].to, transaction);
      }

      else if (key->i == EDIT_ICON_TO)
      {
        /* Trust that any accuout views that are open must be based on a valid date order, and only rebuild those
         * that are directly affected.
         */

        accview_rebuild (file, old_acct);
        transaction = file->transactions[line].sort_index;
        accview_rebuild (file, file->transactions[transaction].to);
        transaction = file->transactions[line].sort_index;
        accview_redraw_transaction (file, file->transactions[transaction].from, transaction);
      }

      else if (key->i == EDIT_ICON_REF)
      {
        accview_redraw_transaction (file, file->transactions[transaction].from, transaction);
        accview_redraw_transaction (file, file->transactions[transaction].to, transaction);
      }

      else if (key->i == EDIT_ICON_AMOUNT)
      {
        accview_recalculate (file, file->transactions[transaction].from, transaction);
        accview_recalculate (file, file->transactions[transaction].to, transaction);
      }

      else if (key->i == EDIT_ICON_DESCRIPT)
      {
        accview_redraw_transaction (file, file->transactions[transaction].from, transaction);
        accview_redraw_transaction (file, file->transactions[transaction].to, transaction);
      }
    }

    /* Finally, look for the next reconcile line if that is necessary.
     *
     * This is done last, as the only hold we have over the line being edited is the edit line location.  Move that,
     * and we've lost everything.
     */

    if ((key->i == EDIT_ICON_FROM || key->i == EDIT_ICON_TO) &&
        (key->c == '+' || key->c == '=' || key->c == '-' || key->c == '_'))
    {
      find_next_reconcile_line (file, 0);
    }
  }
}

/* ==================================================================================================================
 * Descript completion.
 */

char *find_complete_description (file_data *file, int line, char *buffer)
{
  int i, t;

  for (i=line-1; i>=0; i--)
  {
    t = file->transactions[i].sort_index;

    if (*(file->transactions[t].description) != '\0' &&
        string_nocase_strstr (file->transactions[t].description, buffer) == file->transactions[t].description)
    {
      strcpy (buffer, file->transactions[t].description);
      break;
    }
  }

  return (buffer);
}
