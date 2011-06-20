/* CashBook - fontlist.c
 *
 * (C) Stephen Fryatt, 2011
 */

#include <stdlib.h>
#include <string.h>

#include "oslib/font.h"
#include "oslib/wimp.h"

#include "sflib/heap.h"

#include "fontlist.h"

static byte	*fontlist_menu = NULL;
static byte	*fontlist_indirection = NULL;


/**
 * Build a Font List menu and return the pointer.
 *
 * \return		The created menu, or NULL for an error.
 */

wimp_menu *fontlist_build(void)
{
	int	size1, size2;

	font_list_fonts(0, font_RETURN_FONT_MENU, 0, 0, 0, 0, &size1, &size2);

	fontlist_menu = heap_alloc(size1);
	fontlist_indirection = heap_alloc(size2);

	if (fontlist_menu == NULL || fontlist_indirection == NULL) {
		fontlist_destroy();
		return NULL;
	}

	font_list_fonts(fontlist_menu, font_RETURN_FONT_MENU, size1, fontlist_indirection, size2, 0, NULL, NULL);

	return (wimp_menu *) fontlist_menu;
}


/**
 * Destroy any Font List menu which is currently open.
 */

void fontlist_destroy(void)
{
	if (fontlist_menu != NULL)
		heap_free(fontlist_menu);

	if (fontlist_indirection != NULL)
		heap_free(fontlist_indirection);

	fontlist_menu = NULL;
	fontlist_indirection = NULL;
}


/**
 * Decode a menu selection from the Font List menu.
 *
 * \param *selection		The Wimp menu selection data.
 * \return			A pointer to the selected font name, in a
 *				buffer which must be freed via heap_free()
 *				after use.
 */

char *fontlist_decode(wimp_selection *selection)
{
	int	size;
	char	*name, *sub, *c, *font;

	font_decode_menu(0, fontlist_menu, (byte *) selection, 0, 0, NULL, &size);
	name = heap_alloc(size);
	if (name == NULL)
		return NULL;

	font_decode_menu(0, fontlist_menu, (byte *) selection, (byte *) name, size, NULL, NULL);

	/* Extract the font name from the data returned from font_decode_menu (). */

	sub = strstr(name, "\\F");
	if (sub == NULL)
		sub = name;
	else
		sub += 2;

	if ((c = strchr(sub, '\\')) != NULL)
		*c = '\0';

	font = heap_alloc(strlen(sub) + 1);

	if (font != NULL)
		strcpy(font, sub);

	heap_free(name);

	return font;
}

