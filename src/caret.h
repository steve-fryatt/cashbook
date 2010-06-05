/* CashBook - caret.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_CARET
#define _ACCOUNTS_CARET

/* ==================================================================================================================
 * Static constants
 */


/* ==================================================================================================================
 * Data structures
 */

/* ==================================================================================================================
 * Function prototypes.
 */

void place_dialogue_caret (wimp_w window, wimp_i icon);
void place_dialogue_caret_fallback (wimp_w window, int icons, ...);

void close_dialogue_with_caret (wimp_w window);

#endif
