/* CashBook - fontlist.h
 *
 * (C) Stephen Fryatt, 2011
 */

#ifndef CASHBOOK_FONTLIST
#define CASHBOOK_FONTLIST


/**
 * Build a Font List menu and return the pointer.
 *
 * \return		The created menu, or NULL for an error.
 */

wimp_menu *fontlist_build(void);


/**
 * Destroy any Font List menu which is currently open.
 */

void fontlist_destroy(void);


/**
 * Decode a menu selection from the Font List menu.
 *
 * \param *selection		The Wimp menu selection data.
 * \return			A pointer to the selected font name, in a
 *				buffer which must be freed via heap_free()
 *				after use.
 */

char *fontlist_decode(wimp_selection *selection);

#endif

