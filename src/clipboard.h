/* CashBook - clipboard.h
 *
 * (C) Stephen Fryatt, 2003-2011
 *
 */

#ifndef CASHBOOK_CLIPBOARD
#define CASHBOOK_CLIPBOARD

/**
 * Initialise the Clipboard module.
 */

void clipboard_initialise(void);


/**
 * Copy the contents of an icon to the global clipboard, cliaming it in the
 * process if necessary.
 *
 * \param *key			A Wimp Key block, indicating the icon to copy.
 * \return			TRUE if successful; else FALSE.
 */

osbool clipboard_copy_from_icon(wimp_key *key);


/**
 * Cut the contents of an icon to the global clipboard, cliaming it in the
 * process if necessary.
 *
 * \param *key			A Wimp Key block, indicating the icon to cut.
 * \return			TRUE if successful; else FALSE.
 */

osbool clipboard_cut_from_icon(wimp_key *key);

/**
 * Paste the contents of the global clipboard into an icon.  If we own the
 * clipboard, this is done immediately; otherwise the Wimp message dialogue
 * is started.
 *
 * \param *key			A Wimp Key block, indicating the icon to paste.
 * \return			TRUE if successful; else FALSE.
 */

osbool clipboard_paste_to_icon(wimp_key *key);

#endif

