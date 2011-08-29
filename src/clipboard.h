/* CashBook - clipboard.h
 * (c) Stephen Fryatt, 2003-2011
 *
 *
 *
 */

#ifndef CASHBOOK_CLIPBOARD
#define CASHBOOK_CLIPBOARD

/**
 * Initialise the Clipboard module.
 */

void clipboard_initialise(void);

int copy_icon_to_clipboard (wimp_key *key);
int cut_icon_to_clipboard (wimp_key *key);
int paste_clipboard_to_icon (wimp_key *key);

int copy_text_to_clipboard (char *text, int len);

int paste_received_clipboard (char **data, int data_size);

#endif

