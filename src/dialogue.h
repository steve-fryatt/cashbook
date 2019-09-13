/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: dialogue.h
 *
 * Dialogue box interface.
 */

#ifndef CASHBOOK_DIALOGUE
#define CASHBOOK_DIALOGUE

/**
 * A dialogue definition.
 */

struct dialogue_block;

/**
 * Window Icon Types.
 */

enum dialogue_icon_type {
	/**
	 * No special treatment is required.
	 */
	DIALOGUE_ICON_NONE		= 0x00000000,

	/**
	 * The OK button in a dialogue. Target is unused.
	 */
	DIALOGUE_ICON_OK		= 0x00000001,

	/**
	 * The Cancel button in a dialogue. Target is unused.
	 */
	DIALOGUE_ICON_CANCEL		= 0x00000002,

	/**
	 * A generic action button in a dialogue, to be fed back to the
	 * client; a client flag identifies the purpose. Target is unused.
	 */
	DIALOGUE_ICON_ACTION		= 0x00000004,

	/**
	 * A generic action button in a dialogue, to be fed back to the
	 * client; a client flag identifies the purpose. This action button
	 * should not close the dialogue on selection, but simply refresh
	 * the dialogue content. Target is unused.
	 */
	DIALOGUE_ICON_ACTION_NO_CLOSE	= 0x00000008,

	/**
	 * A radio icon, which must be registerd with the Wimp Library
	 * for Adjust-click handling. The library is not asked to pass
	 * clicks on. Target is unused.
	 */
	DIALOGUE_ICON_RADIO		= 0x00000010,

	/**
	 * A radio icon, which must be registerd with the Wimp Library
	 * for Adjust-click handling. The library is asked to pass clicks
	 * on, so other action flags can be applied. Target is unused.
	 */
	DIALOGUE_ICON_RADIO_PASS	= 0x00000020,

	/**
	 * Shade this icon when another icon is selected; the other icon
	 * is specified by Target.
	 */
	DIALOGUE_ICON_SHADE_ON		= 0x00000040,


	/**
	 * Shade this icon when another icon is not selected; the other
	 * icon is specified by Target.
	 */
	DIALOGUE_ICON_SHADE_OFF		= 0x00000080,

	/**
	 * Used in conjucntion with DIALOGUE_ICON_SHADE_ON or
	 * DIALOGUE_ICON_SHADE_OFF, to include this condition with the
	 * previous line of the array. In this situation, Target still
	 * specifies the target icon, but Icon is not used and must be
	 * set to DIALOGUE_NO_ICON.
	 */
	DIALOGUE_ICON_SHADE_OR		= 0x00000100,

	/**
	 * Treat this icon as a shading target, thereby scanning the
	 * icon set and updating the state of any associated icons
	 * whenever a click is detected. If the icon is a radio icon,
	 * it is essential that DIALOGUE_ICON_RADIO_PASS is used.
	 * Target is unused.
	 */
	DIALOGUE_ICON_SHADE_TARGET	= 0x00000200,

	/**
	 * The icon should be redrawn whenever its contents is changed
	 * by the dialogue handler. Target is unused.
	 */
	DIALOGUE_ICON_REFRESH		= 0x00000400,

	/**
	 * The icon should be registered with the Wimp Library as a
	 * pop-up menu icon. Target is unused.
	 */
	DIALOGUE_ICON_POPUP		= 0x00000800,

	/**
	 * The icon requires a pop-up account selection dialogue to be
	 * associated with it. If Target is DIALOGUE_NO_ICON, the icon
	 * is the text field; otherwise, the icon is the pop-up menu icon
	 * and Target is the text field. One of the DIALOGUE_ICON_TYPE
	 * flags should be set to indicate the type of field.
	 */
	DIALOGUE_ICON_ACCOUNT_POPUP	= 0x00001000,

	/**
	 * The icon is the ident field in an account selector. Target
	 * should reference the icon representing the name field.
	 */
	DIALOGUE_ICON_ACCOUNT_IDENT	= 0x00002000,

	/**
	 * The icon is the name field in an account selector. Target
	 * should reference the icon representing the reconciled field.
	 */
	DIALOGUE_ICON_ACCOUNT_NAME	= 0x00004000,

	/**
	 * The icon is the reconciled field in an account selector. Target
	 * should reference the icon representing the ident field.
	 */
	DIALOGUE_ICON_ACCOUNT_RECONCILE	= 0x00008000,

	/**
	 * The icon is an account field targetting incoming headings and accounts.
	 * This is used in conjunction with DIALOGUE_ICON_ACCOUNT_POPUP,
	 * DIALOGUE_ICON_ACCOUNT_IDENT, DIALOGUE_ICON_ACCOUNT_NAME and
	 * DIALOGUE_ICON_ACCOUNT_RECONCILE.
	 */
	DIALOGUE_ICON_TYPE_FROM		= 0x00010000,

	/**
	 * The icon is an account field targetting outgoing headings and accounts.
	 * This is used in conjunction with DIALOGUE_ICON_ACCOUNT_POPUP,
	 * DIALOGUE_ICON_ACCOUNT_IDENT, DIALOGUE_ICON_ACCOUNT_NAME and
	 * DIALOGUE_ICON_ACCOUNT_RECONCILE.
	 */
	DIALOGUE_ICON_TYPE_TO		= 0x00020000,

	/**
	 * The icon is an account field targetting incoming headings.
	 * This is used in conjunction with DIALOGUE_ICON_ACCOUNT_POPUP,
	 * DIALOGUE_ICON_ACCOUNT_IDENT, DIALOGUE_ICON_ACCOUNT_NAME and
	 * DIALOGUE_ICON_ACCOUNT_RECONCILE.
	 */
	DIALOGUE_ICON_TYPE_IN		= 0x00040000,

	/**
	 * The icon is an account field targetting outgoing headings.
	 * This is used in conjunction with DIALOGUE_ICON_ACCOUNT_POPUP,
	 * DIALOGUE_ICON_ACCOUNT_IDENT, DIALOGUE_ICON_ACCOUNT_NAME and
	 * DIALOGUE_ICON_ACCOUNT_RECONCILE.
	 */
	DIALOGUE_ICON_TYPE_OUT		= 0x00080000,

	/**
	 * The icon is an account field targetting accounts.
	 * This is used in conjunction with DIALOGUE_ICON_ACCOUNT_POPUP,
	 * DIALOGUE_ICON_ACCOUNT_IDENT, DIALOGUE_ICON_ACCOUNT_NAME and
	 * DIALOGUE_ICON_ACCOUNT_RECONCILE.
	 */
	DIALOGUE_ICON_TYPE_FULL		= 0x00100000,

	/**
	 * The last, dummy, entry in the dialogue icon sequence. Icon and
	 * Target should both be DIALOGUE_NO_ICON.
	 */
	DIALOGUE_ICON_END		= 0x08000000,

	/* Flags below this point belong to individual clients; values
	 * can be duplicated across clients. Four flags are allocated,
	 * from 0x10000000 to 0x80000000.
	 */

	DIALOGUE_ICON_ANALYSIS_DELETE	= 0x10000000,		/**< The Analysis Report Delete button.		*/
	DIALOGUE_ICON_ANALYSIS_RENAME	= 0x20000000,		/**< The Analysis Report Rename button.		*/

	DIALOGUE_ICON_EDIT_DELETE	= 0x10000000,		/**< The Delete button in edit dialogues.	*/
	DIALOGUE_ICON_EDIT_RATES	= 0x20000000,		/**< The Rates... button in edit dialogues.	*/
	DIALOGUE_ICON_EDIT_STOP		= 0x40000000,		/**< The Stop button in edit dialogues.		*/

	DIALOGUE_ICON_PRINT_REPORT	= 0x10000000,		/**< The Print Report button.			*/

	DIALOGUE_ICON_FIND_PREVIOUS	= 0x10000000,		/**< The Previous Match button.			*/
	DIALOGUE_ICON_FIND_NEXT		= 0x20000000,		/**< The Next Match button.			*/
	DIALOGUE_ICON_FIND_NEW		= 0x40000000,		/**< The New Search button.			*/

	DIALOGUE_ICON_IMPORT_CLOSE	= 0x10000000,		/**< The Import Report close button.		*/
	DIALOGUE_ICON_IMPORT_VIEW_LOG	= 0x20000000		/**< The Import Report View Log button.		*/
};

/**
 * Value indicating no icon.
 */

#define DIALOGUE_NO_ICON ((wimp_i) -1)

/**
 * Window icon definitions.
 */

struct dialogue_icon {
	enum dialogue_icon_type		type;
	wimp_i				icon;
	wimp_i				target;
};

/**
 * Menu details block.
 */

struct dialogue_menu_data {
	/**
	 * Pointer to the menu block.
	 */
	wimp_menu			*menu;

	/**
	 * Pointer to an interactive help token, or NULL.
	 */
	char				*help_token;
};

/**
 * Dialogue box groups.
 */

enum dialogue_group {
	/**
	 * No specific group.
	 */
	DIALOGUE_GROUP_NONE,

	/**
	 * An analysis dialogue.
	 */
	DIALOGUE_GROUP_ANALYSIS,

	/**
	 * A find dialogue.
	 */
	DIALOGUE_GROUP_FIND,

	/**
	 * An account Edit dialogue.
	 */
	DIALOGUE_GROUP_EDIT,

	/**
	 * An Import dialogue.
	 */
	DIALOGUE_GROUP_IMPORT
};

/**
 * Dialogue option flags.
 */

enum dialogue_flags {
	/**
	 * No flags set.
	 */
	DIALOGUE_FLAGS_NONE = 0,

	/**
	 * Take input focus when the dialogue opens.
	 */
	DIALOGUE_FLAGS_TAKE_FOCUS = 1,

	/**
	 * When refreshing the dialogue content, redraw the titlebar.
	 */
	DIALOGUE_FLAGS_REDRAW_TITLE = 2
};

/**
 * An analysis dialogue contents definition.
 */

struct dialogue_definition {
	/**
	 * The name of the window template to use for the dialogue.
	 */
	char				*template_name;

	/**
	 * The interactive help token prefix to use for the dialogue.
	 */
	char				*ihelp_token;

	/**
	 * A list of significant icons in the dialogue.
	 */
	struct dialogue_icon		*icons;

	/**
	 * The group to which the dialogue belongs.
	 */
	enum dialogue_group		group;

	/**
	 * The dialogue option flags.
	 */
	enum dialogue_flags		flags;

	/**
	 * Callback function to request the dialogue is filled.
	 */
	void				(*callback_fill)(struct file_block *file, wimp_w window, osbool restore, void *data);

	/**
	 * Callback function to request the dialogue is processed.
	 */
	osbool				(*callback_process)(struct file_block *file, wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *parent, void *data);

	/**
	 * Callback function to report the dialogue closing.
	 */
	void				(*callback_close)(struct file_block *file, wimp_w window, void *data);

	/**
	 * Callback function to report a pop-up menu opening.
	 */
	osbool				(*callback_menu_prepare)(struct file_block *file, wimp_w window, wimp_i icon, struct dialogue_menu_data *menu, void *data);

	/**
	 * Callback function to report a pop-up menu selection.
	 */
	void				(*callback_menu_select)(struct file_block *file, wimp_w w, wimp_i icon, wimp_menu *menu, wimp_selection *selection, void *data);

	/**
	 * Callback function to report a pop-up menu closing.
	 */
	void				(*callback_menu_close)(struct file_block *file, wimp_w w, wimp_menu *menu, void *data);
};


/**
 * Initialise the dialogue handler.
 */

void dialogue_initialise(void);


/**
 * Create a new dialogue window instance.
 *
 * \param *definition		The dialogue definition from the client.
 * \return			Pointer to the dialogue structure, or NULL on failure.
 */

struct dialogue_block *dialogue_create(struct dialogue_definition *definition);


/**
 * Close any open dialogues which relate to a given file or parent object.
 *
 * \param *file			If not NULL, the file to close dialogues
 *				for.
 * \param *parent		If not NULL, the parent object to close
 *				dialogues for.
 */

void dialogue_force_all_closed(struct file_block *file, void *parent);


/**
 * Close any open dialogues which belong to a given group.
 *
 * \param group			The group to be closed.
 */

void dialogue_force_group_closed(enum dialogue_group group);


/**
 * Test all open dialogues, to see if any relate to a given file or
 * parent object.
 *
 * \param *file			If not NULL, the file to test open
 *				dialogues for.
 * \param *parent		If not NULL, the parent object to test
 *				open dialogues for.
 * \return			TRUE if any dialogues are open; otherwise
 *				FALSE.
 */

osbool dialogue_any_open(struct file_block *file, void *parent);


/**
 * Open a new dialogue. Dialogues are attached to a file, and also to a
 * "parent object", which can be anything that the client for
 * dialogue_open() wishes to associate them with. If not NULL, they are
 * commonly pointers to instance blocks, such that a dialogue can be
 * associated with -- and closed on the demise of -- things such as
 * account or report views.
 * 
 * \param *dialogue		The dialogue instance to open.
 * \param restore		TRUE to restore previous values; FALSE to
 *				use application defaults.
 * \param *file			The file to be the parent of the dialogue.
 * \param *parent		The parent object of the dialogue, or NULL.
 * \param *ptr			The current Wimp Pointer details.
 * \param *data			Data to pass to client callbacks.
 */

void dialogue_open(struct dialogue_block *dialogue, osbool restore, struct file_block *file, void *parent, wimp_pointer *pointer, void *data);


/**
 * Force a dialogue instance to close if it is currently open
 * on screen.
 *
 * \param *dialogue		The dialogue instance to close.
 * \param *file			If not NULL, only close the dialogue if
 *				this is the parent file.
 * \param *parent		If not NULL, only close the dialogue if
 *				this is the parent object.
 */

void dialogue_close(struct dialogue_block *dialogue, struct file_block *file, void *parent);


/**
 * Set the window title for a dialogue box, redrawing it if the
 * dialogue is currently open.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param *token		The MessageTrans token for the new title.
 * \param *a			MessageTrans parameter A, or NULL.
 * \param *b			MessageTrans parameter B, or NULL.
 * \param *c			MessageTrans parameter C, or NULL.
 * \param *d			MessageTrans parameter D, or NULL.
 */

void dialogue_set_title(struct dialogue_block *dialogue, char *token, char *a, char *b, char *c, char *d);


/**
 * Set the text in an icon or icons in a dialogue box, redrawing them if the
 * dialogue is currently open.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param type			The types of icon to update.
 * \param *token		The MessageTrans token for the new text.
 * \param *a			MessageTrans parameter A, or NULL.
 * \param *b			MessageTrans parameter B, or NULL.
 * \param *c			MessageTrans parameter C, or NULL.
 * \param *d			MessageTrans parameter D, or NULL.
 */

void dialogue_set_icon_text(struct dialogue_block *dialogue, enum dialogue_icon_type type, char *token, char *a, char *b, char *c, char *d);


/**
 * Set the hidden state of an icon or icons in a dialogue box, redrawing
 * them if the dialogue is currently open.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param type			The types of icon to update.
 * \param hide			TRUE to hide the icons; otherwise FALSE.
 */
void dialogue_set_hidden_icons(struct dialogue_block *dialogue, enum dialogue_icon_type type, osbool hide);


/**
 * Change the interactive help token modifier for a dialogue window.
 *
 * \param *dialogue		The dialogue instance to update.
 * \param *modifier		The new modifier to apply.
 */

void dialogue_set_ihelp_modifier(struct dialogue_block *dialogue, char *modifier);


/**
 * Request the client to fill a dialogue, update the shaded icons and then
 * redraw any fields which require it. If the dialogue isn't open, nothing
 * will be done.
 *
 * \param *dialogue		The dialogue instance to refresh.
 */

void dialogue_refresh(struct dialogue_block *dialogue);

#endif

