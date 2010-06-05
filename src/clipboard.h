/* CashBook - clipboard.h
 * (c) Stephen Fryatt, 2003
 *
 *
 *
 */

#ifndef _ACCOUNTS_CLIPBOARD
#define _ACCOUNTS_CLIPBOARD

/* Function prototypes. */

int copy_icon_to_clipboard (wimp_key *key);
int cut_icon_to_clipboard (wimp_key *key);
int paste_clipboard_to_icon (wimp_key *key);

int copy_text_to_clipboard (char *text, int len);
int release_clipboard (wimp_message *claim_entity_message);

int send_clipboard (wimp_message *data_request_message);

int paste_received_clipboard (char **data, int data_size);

#endif
