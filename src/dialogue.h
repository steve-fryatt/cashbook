/* Copyright 2003-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
	DIALOGUE_ICON_NONE		= 0x00000000,		/*<< No icon.					*/
	DIALOGUE_ICON_OK		= 0x00000001,		/**< The OK button.				*/
	DIALOGUE_ICON_CANCEL		= 0x00000002,		/**< The Cancel button.				*/
	DIALOGUE_ICON_ACTION		= 0x00000004,		/**< Another generic action button.		*/
	DIALOGUE_ICON_RADIO		= 0x00000008,		/**< A radio icon.				*/
	DIALOGUE_ICON_RADIO_PASS	= 0x00000010,		/**< A radio icon which passes events on.	*/
	DIALOGUE_ICON_SHADE_ON		= 0x00000020,		/**< Shade icon when target is selected.	*/
	DIALOGUE_ICON_SHADE_OFF		= 0x00000040,		/**< Shade icon when target is not selected.	*/
	DIALOGUE_ICON_SHADE_OR		= 0x00000080,		/**< Include this condition with the previous.	*/
	DIALOGUE_ICON_SHADE_TARGET	= 0x00000100,		/**< A target for shading other icons.		*/
	DIALOGUE_ICON_REFRESH		= 0x00000200,		/**< The icon requires refreshing.		*/
	DIALOGUE_ICON_HIDDEN		= 0x00000400,		/**< The icon should be hidden when requested.	*/
	DIALOGUE_ICON_POPUP_FROM	= 0x00000800,		/*<< The icon should launch a "From" popup.	*/
	DIALOGUE_ICON_POPUP_TO		= 0x00001000,		/*<< The icon should launch a "To" popup.	*/
	DIALOGUE_ICON_POPUP_IN		= 0x00002000,		/*<< The icon should launch a "In" popup.	*/
	DIALOGUE_ICON_POPUP_OUT		= 0x00004000,		/*<< The icon should launch a "Out" popup.	*/
	DIALOGUE_ICON_POPUP_FULL	= 0x00008000,		/*<< The icon should launch a "Full" popup.	*/
	DIALOGUE_ICON_POPUP		= 0x00010000,		/*<< A pop-up menu field.			*/
	DIALOGUE_ICON_END		= 0x08000000,		/**< The last entry in the icon sequence.	*/

	/* Flags below this point belong to individual clients; values
	 * can be duplicated across clients. Four flags are allocated,
	 * from 0x10000000 to 0x80000000.
	 */

	DIALOGUE_ICON_ANALYSIS_DELETE	= 0x10000000,		/**< The Analysis Report Delete button.		*/
	DIALOGUE_ICON_ANALYSIS_RENAME	= 0x20000000,		/**< The Analysis Report Rename button.		*/

	DIALOGUE_ICON_PRINT_REPORT	= 0x10000000,		/**< The Print Report button.			*/
};

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
	 * A set of icons which should be able to be hidden,
	 * or DIALOGUE_ICON_NONE.
	 */
	enum dialogue_icon_type		hidden_icons;

	/**
	 * Callback function to request the dialogue is filled.
	 */
	void				(*callback_fill)(wimp_w window, osbool restore, void *data);

	/**
	 * Callback function to request the dialogue is processed.
	 */
	osbool				(*callback_process)(wimp_w window, wimp_pointer *pointer, enum dialogue_icon_type type, void *data);

	/**
	 * Callback function to report the dialogue closing.
	 */
	void				(*callback_close)(wimp_w window, void *data);

	/**
	 * Callback function to report a pop-up menu opening.
	 */
	osbool				(*callback_menu_prepare)(wimp_w window, wimp_i icon, struct dialogue_menu_data *menu, void *data);

	/**
	 * Callback function to report a pop-up menu selection.
	 */
	void				(*callback_menu_select)(wimp_w w, wimp_i icon, wimp_menu *menu, wimp_selection *selection, void *data);

	/**
	 * Callback function to report a pop-up menu closing.
	 */
	void				(*callback_menu_close)(wimp_w w, wimp_menu *menu, void *data);
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
 * Open a new dialogue. Dialogues are attached to a file, and also to a
 * "parent object", which can be anything that the client for
 * dialogue_open() wishes to associate them with. If not NULL, they are
 * commonly pointers to instance blocks, such that a dialogue can be
 * associated with -- and closed on the demise of -- things such as
 * account or report views.
 * 
 * \param *dialogue		The dialogue instance to open.
 * \param hide			TRUE to hide the 'hidden' icons; FALSE
 *				to show them.
 * \param restore		TRUE to restore previous values; FALSE to
 *				use application defaults.
 * \param *file			The file to be the parent of the dialogue.
 * \param *parent		The parent object of the dialogue, or NULL.
 * \param *ptr			The current Wimp Pointer details.
 * \param *data			Data to pass to client callbacks.
 */

void dialogue_open(struct dialogue_block *dialogue, osbool hide, osbool restore, struct file_block *file, void *parent, wimp_pointer *pointer, void *data);


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
 * \param redraw_title		TRUE to force a redraw of the title bar.
 */

void dialogue_refresh(struct dialogue_block *dialogue, osbool redraw_title);

#endif

