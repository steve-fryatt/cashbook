/* CashBoob - clipboard.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"

/* OSLibSupport header files */

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/transfer.h"
#include "sflib/icons.h"
#include "sflib/config.h"

/* Application header files */

#include "global.h"
#include "main.h"
#include "clipboard.h"

/* ANSI C header files */

#include <string.h>

/* ------------------------------------------------------------------------------------------------------------------ */

/* Declare the global variables that are used. */

static char *clipboard_data = NULL;
static int  clipboard_length = 0;

/* Cross file global variables */

wimp_t                     task_handle;

/* ================================================================================================================== */

int copy_icon_to_clipboard (wimp_key *key)
{
  wimp_icon_state icon;


  if (config_opt_read ("GlobalClipboardSupport"))
  {
    icon.w = key->w;
    icon.i = key->i;
    wimp_get_icon_state (&icon);

    copy_text_to_clipboard (icon.icon.data.indirected_text.text, strlen (icon.icon.data.indirected_text.text));
  }

  return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */

int cut_icon_to_clipboard (wimp_key *key)
{
  wimp_icon_state icon;


  if (config_opt_read ("GlobalClipboardSupport"))
  {
    icon.w = key->w;
    icon.i = key->i;
    wimp_get_icon_state (&icon);

    copy_text_to_clipboard (icon.icon.data.indirected_text.text, strlen (icon.icon.data.indirected_text.text));

    *icon.icon.data.indirected_text.text = '\0';
    wimp_set_icon_state (key->w, key->i, 0, 0);
    icons_put_caret_at_end (key->w, key->i);
  }

  return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */

int paste_clipboard_to_icon (wimp_key *key)
{
  wimp_full_message_data_request datarequest;
  os_error                       *error;


  /* Test to see if we own the clipboard ourselves.  If so, use it directly; if not, send out a
   * Message_DataRequest.
   */

  if (config_opt_read ("GlobalClipboardSupport"))
  {
    if (clipboard_data != NULL)
    {
      icons_insert_text (key->w, key->i, key->index, clipboard_data, clipboard_length);
    }
    else
    {
      datarequest.size = 48;
      datarequest.your_ref = 0;
      datarequest.action = message_DATA_REQUEST;

      datarequest.w = key->w;
      datarequest.i = key->i;
      datarequest.pos = key->pos;
      datarequest.flags = wimp_DATA_REQUEST_CLIPBOARD;
      datarequest.file_types[0] = 0xfff;
      datarequest.file_types[1] = -1;

      error = xwimp_send_message (wimp_USER_MESSAGE, (wimp_message *) &datarequest, wimp_BROADCAST);
      if (error != NULL)
      {
        error_report_os_error (error, wimp_ERROR_BOX_CANCEL_ICON);
        return -1;
      }
    }
  }

  return 0;
}


/* ================================================================================================================== */

int copy_text_to_clipboard (char *text, int len)
{
  wimp_full_message_claim_entity claimblock;
  os_error                       *error;


  /* If we already have the clipboard, clear it first. */

  if (clipboard_data != NULL)
  {
    flex_free ((flex_ptr) &clipboard_data);
    clipboard_length = 0;
  }

  /* Record the details of the text in our own variables. */

  if (flex_alloc ((flex_ptr) &clipboard_data, len) == 0)
  {
    error_msgs_report_error ("ClipAllocFail");
    return -1;
  }

  memcpy (clipboard_data, text, len);
  clipboard_length = len;

  /* Send out Message_CliamClipboard. */

  claimblock.size = 24;
  claimblock.your_ref = 0;
  claimblock.action = message_CLAIM_ENTITY;

  claimblock.flags = wimp_CLAIM_CLIPBOARD;

  error = xwimp_send_message (wimp_USER_MESSAGE, (wimp_message *) &claimblock, wimp_BROADCAST);
  if (error != NULL)
  {
    error_report_os_error (error, wimp_ERROR_BOX_CANCEL_ICON);

    flex_free ((flex_ptr) &clipboard_data);
    clipboard_length = 0;

    return -1;
  }


  return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */

int release_clipboard (wimp_message *message)
{
  wimp_full_message_claim_entity *claimblock = (wimp_full_message_claim_entity *) message;


  /* Unset the contents of the clipboard if the claim was for that. */

  if ((clipboard_data != NULL) && (claimblock->sender != main_task_handle) && (claimblock->flags & wimp_CLAIM_CLIPBOARD))
  {
    flex_free ((flex_ptr) &clipboard_data);
    clipboard_length = 0;
  }

  return 0;
}

/* ================================================================================================================== */

int send_clipboard (wimp_message *message)
{
  wimp_full_message_data_request *requestblock = (wimp_full_message_data_request *) message;

  /* Just return if we do not own the clipboard at present. */

  if (clipboard_data == NULL)
  {
    return 0;
  }

  /* Check that the message flags are correct. */

  if ((requestblock->flags & wimp_DATA_REQUEST_CLIPBOARD) == 0)
  {
    return 0;
  }

  transfer_save_start_block (requestblock->w, requestblock->i, requestblock->pos, requestblock->my_ref,
                             &clipboard_data, clipboard_length, 0xfff, "CutText");

  return 1;
}

/* ================================================================================================================== */

int paste_received_clipboard (char **data, int data_size)
{
  wimp_caret caret;


  wimp_get_caret_position (&caret);
  icons_insert_text (caret.w, caret.i, caret.index, *data, data_size);

  flex_free ((flex_ptr) data);

  return 0;
}
