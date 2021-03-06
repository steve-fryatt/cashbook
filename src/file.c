/* Copyright 2003-2017, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: file.c
 *
 * Search file record creation, maipulation and deletion.
 */

/* ANSI C header files */

#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/heap.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/menus.h"
#include "sflib/icons.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/saveas.h"
#include "sflib/string.h"

/* Application header files */

#include "global.h"
#include "file.h"

#include "account.h"
#include "accview.h"
#include "analysis.h"
#include "budget.h"
#include "clipboard.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "dialogue.h"
#include "edit.h"
#include "filing.h"
#include "find.h"
#include "flexutils.h"
#include "goto.h"
#include "interest.h"
#include "preset.h"
#include "print_dialogue.h"
#include "purge.h"
#include "report.h"
#include "sorder.h"
#include "transact.h"
#include "window.h"

/**
 * The maximum length allocated for building default filenames.
 */

#define FILE_DEFAULT_NAME_BUFFER_LEN 15

/* ==================================================================================================================
 * Global variables.
 */

/* The head of the linked list of file data structures. */

static struct file_block	*file_list = NULL;

/* A count which is incremented to allow <Untitled n> window titles */

static int			file_untitled_count = 0;

/* The handle of the SaveAs dialogue of last resort. */

static struct saveas_block	*file_saveas_file = NULL;

/* Buffer to hold temporary default filenames. */

static char			file_default_name_buffer[FILE_DEFAULT_NAME_BUFFER_LEN];

/* Static Function Prototypes. */

static osbool	file_save_file(char *filename, osbool selection, void *data);

static char	*file_get_default_title(struct file_block *file, char *name, size_t len);

/**
 * Initialise the overall file system.
 */

void file_initialise(void)
{
	file_saveas_file = saveas_create_dialogue(FALSE, "file_1ca", file_save_file);
}


/* ==================================================================================================================
 * File initialisation and deletion.
 */

/* Allocate memory for a file, initialise it and create the transaction window. */

struct file_block *build_new_file_block(void)
{
	struct file_block	*new;

	/* Claim the memory required for the file descriptor block. */

	new = (struct file_block *) heap_alloc(sizeof(struct file_block));
	if (new == NULL) {
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}
 
	/* Zero any memory pointers, so that we know what memory has been
	 * successfully claimed later on.
	 */

	new->accviews = NULL;
	new->interest = NULL;
	new->transacts = NULL;
	new->accounts = NULL;
	new->sorders = NULL;
	new->presets = NULL;
	new->analysis = NULL;

	new->budget = NULL;
	new->find = NULL;
	new->go_to = NULL;
	new->print = NULL;
	new->purge = NULL;

	new->reports = NULL;
	new->import_report = NULL;

	/* Set up the budget data. */

	new->budget = budget_create(new);
	if (new->budget == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the find data. */

	new->find = find_create(new);
	if (new->find == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the goto data. */

	new->go_to = goto_create(new);
	if (new->go_to == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the print data. */

	new->print = print_dialogue_create(new);
	if (new->print == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the purge data. */

	new->purge = purge_create(new);
	if (new->purge == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the interest rate manager. */

	new->interest = interest_create_instance(new);
	if (new->interest == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the transaction window. */

	new->transacts = transact_create_instance(new);
	if (new->transacts == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the accounts window. */

	new->accounts = account_create_instance(new);
	if (new->accounts == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Initialise the account view window. */

	new->accviews = accview_create_instance(new);
	if (new->accviews == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the standing order window. */

	new->sorders = sorder_create_instance(new);
	if (new->sorders == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the preset window. */

	new->presets = preset_create_instance(new);
	if (new->presets == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

	/* Set up the analysis report module. */

	new->analysis = analysis_create_instance(new);
	if (new->analysis == NULL) {
		delete_file(new);
		error_msgs_report_error("NoMemNewFile");
		return NULL;
	}

  /* Set the filename and save status. */

  *(new->filename) = '\0';
  new->modified = FALSE;
  new->untitled_count = ++file_untitled_count;
  new->child_x_offset = 0;

	/* Link the file descriptor into the list. */

	new->next = file_list;
	file_list = new;

	return new;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Create a new transaction file with window and open it. */

void create_new_file(void)
{
	struct file_block	*file;

	/* Build a new file block. */

	file = build_new_file_block();

	if (file != NULL)
		transact_open_window(file);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Delete a transaction file block with its window. */

void delete_file(struct file_block *file)
{
	struct file_block		**list;
	wimp_error_box_selection	button;
	wimp_pointer			pointer;
	char				*filename;


	/* First check that the file is saved and, if not, prompt for deletion. */

	if (file->modified == TRUE && (button = error_msgs_report_question("FileNotSaved", "FileNotSavedB")) >= 4) {
		if (button == 5) {
			wimp_get_pointer_info(&pointer);

			filename = (file_check_for_filepath(file)) ? file->filename : NULL;

			saveas_initialise_dialogue(file_saveas_file, filename, "DefTransFile", NULL, FALSE, FALSE, file);
			saveas_prepare_dialogue(file_saveas_file);
			saveas_open_dialogue(file_saveas_file, &pointer);
		}

		return;
	}

	/* If there are any reports in the file with pending print jobs, prompt for deletion. */

	if (report_get_pending_print_jobs(file) && error_msgs_report_question("PendingPrints", "PendingPrintsB") == 4)
		return;

	/* Delete any reports that are open. */

	while (file->reports != NULL)
		report_delete(file->reports);

	/* Force close any dialogues which are open. */

	dialogue_force_all_closed(file, NULL);

	/* Delete the windows and data structures. */

	if (file->transacts != NULL)
		transact_delete_instance(file->transacts);

	if (file->accounts != NULL)
		account_delete_instance(file->accounts);

	if (file->sorders != NULL)
		sorder_delete_instance(file->sorders);

	if (file->presets != NULL)
		preset_delete_instance(file->presets);

	if (file->interest != NULL)
		interest_delete_instance(file->interest);

	if (file->accviews != NULL)
		accview_delete_instance(file->accviews);

	if (file->analysis != NULL)
		analysis_delete_instance(file->analysis);

	/* Delink the block from the list of open files. */

	list = &file_list;

	while (*list != NULL && *list != file)
		list = &((*list)->next);
	
	if (*list != NULL)
		*list = file->next;

	/* Deallocate any memory that's claimed for the block. */

	if (file->budget != NULL)
		budget_delete(file->budget);
	if (file->find != NULL);
		find_delete(file->find);
	if (file->go_to != NULL)
		goto_delete(file->go_to);
	if (file->print != NULL)
		print_dialogue_delete(file->print);
	if (file->purge != NULL)
		purge_delete(file->purge);

	/* Deallocate the block itself. */

	heap_free(file);
}



/**
 * Callback handler for saving a cashbook file of last resort: after saving,
 * the file should be deleted.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		FALSE, as no selections are supported.
 * \param *data			Pointer to the block of the file to be saved.
 */

static osbool file_save_file(char *filename, osbool selection, void *data)
{
	struct file_block	*file = data;

	if (file == NULL)
		return FALSE;

	filing_save_cashbook_file(file, filename);
	delete_file(file);

	return TRUE;
}




/**
 * Check for unsaved files and for any pending print jobs which are
 * currently attached to open files, and warn the user if any are found.
 *
 * \return		TRUE if there is something that isn't saved; FALSE
 *			if there's nothing worth saving.
 */

osbool file_check_for_unsaved_data(void)
{
	struct file_block	*list = file_list;
	osbool			modified = FALSE, pending = FALSE;


	/* Search through all the loaded files to see if any are modified or
	 * have any pending print jobs attached.
	 */

	while (list != NULL) {
		if (list->modified)
			modified = TRUE;

		if (report_get_pending_print_jobs(list))
			pending = TRUE;

		list = list->next;
	}

	/* If any files were modified, allow the user to discard them. */

	if (modified && (error_msgs_report_question("FilesNotSaved", "FilesNotSavedB") == 3))
		modified = FALSE;

	/* If there were no unsaved files (or the use chose to discard them),
	 * warn of any pending print jobs. This isn't done if the process is
	 * aborted due to modified files to save 'dialogue box overload'.
	 */

	if (!modified && pending && (error_msgs_report_question("FPendingPrints", "FPendingPrintsB") == 3))
		pending = FALSE;

	/* Return true if anything needs rescuing. */

	return (modified || pending);
}


/**
 * Set the 'unsaved' state of a file.
 *
 * \param *file		The file to update.
 * \param unsafe	TRUE if the file has unsaved data; FALSE if not.
 */

void file_set_data_integrity(struct file_block *file, osbool unsafe)
{
	if (file != NULL && file->modified != unsafe) {
		file->modified = unsafe;
		transact_build_window_title(file);
	}
}


/**
 * Read the 'unsaved' state of a file.
 *
 * \param *file		The file to read.
 * \return		TRUE if the file has unsaved data; FALSE if not.
 */

osbool file_get_data_integrity(struct file_block *file)
{
	return (file == NULL) ? FALSE : file->modified;
}


/**
 * Check if the file has a full save path (ie. it has been saved before, or has
 * been loaded from disc).
 *
 * \param *file		The file to test.
 * \return		TRUE if there is a full filepath; FALSE if not.
 */

osbool file_check_for_filepath(struct file_block *file)
{
	if (file == NULL)
		return FALSE;

	return (*(file->filename) != '\0') ? TRUE : FALSE;
}


/**
 * Return a path-name string for the current file, using the <Untitled n>
 * format if the file hasn't been saved.
 *
 * \param *file		The file to build a pathname for.
 * \param *path		The buffer to return the pathname in.
 * \param len		The length of the supplied buffer.
 * \return		A pointer to the supplied buffer.
 */

char *file_get_pathname(struct file_block *file, char *path, size_t len)
{
	if (path == NULL || len <= 0)
		return path;

	if (file == NULL) {
		*path = '\0';
		return path;
	}

	if (*(file->filename) != '\0')
		string_copy(path, file->filename, len);
	else
		file_get_default_title(file, path, len);

	return path;
}


/**
 * Return a leaf-name string for the current file, using the <Untitled n>
 * format if the file hasn't been saved. If a buffer is supplied, the
 * name is copied into this; otherwise a pointer to a transient string
 * is returned.
 *
 * \param *file		The file to build a leafname for.
 * \param *leaf		The buffer to return the leafname in, or NULL
 *			if a transient copy is required.
 * \param len		The length of the supplied buffer, or 0 for NULL.
 * \return		A pointer to the leafname.
 */

char *file_get_leafname(struct file_block *file, char *leaf, size_t len)
{
	/* There needs to be a valid file block. */

	if (file == NULL) {
		if (leaf != NULL && len > 0)
			*leaf = '\0';
		return leaf;
	}

	/* If no buffer is supplied, return a pointer to the original string. */

	if (leaf == NULL && len == 0) {
		if (*(file->filename) != '\0') {
			return string_find_leafname(file->filename);
		} else {
			file_get_default_title(file, file_default_name_buffer, FILE_DEFAULT_NAME_BUFFER_LEN);
			return file_default_name_buffer;
		}
	}

	/* If the bffer wasn't valid, return nothing. */

	if (leaf == NULL || len <= 0)
		return leaf;

	/* Copy the name into the supplied buffer. */

	if (*(file->filename) != '\0')
		string_copy(leaf, string_find_leafname(file->filename), len);
	else
		file_get_default_title(file, leaf, len);

	return leaf;
}


/**
 * Build a title of the form <Untitled n> for the specified file block,
 * returning it in the supplied buffer.
 *
 * \param *file		The file to build a title for.
 * \param *name		The buffer to return the title in.
 * \param len		The length of the supplied buffer.
 * \return		A pointer to the supplied buffer.
 */

static char *file_get_default_title(struct file_block *file, char *name, size_t len)
{
	char	number[FILE_DEFAULT_NAME_BUFFER_LEN];

	if (name == NULL || len <= 0)
		return name;

	*name = '\0';

	if (file == NULL)
		return name;

	string_printf(number, FILE_DEFAULT_NAME_BUFFER_LEN, "%d", file->untitled_count);
	msgs_param_lookup("DefTitle", name, len, number, NULL, NULL, NULL);

	return name;
}


/**
 * Return the next X offset, in OS units, for opening a new child window.
 * 
 * \param *file		The file to return a new offset for.
 * \return		The new offset, in OS units.
 */

int file_get_next_open_offset(struct file_block *file)
{
	int offset;

	if (file == NULL)
		return 0;

	offset = CHILD_WINDOW_X_OFFSET * file->child_x_offset++;

	if (file->child_x_offset >= CHILD_WINDOW_X_OFFSET_LIMIT)
		file->child_x_offset = 0;

	return offset;
}


/**
 * Redraw all the windows connected with a given file.
 *
 * \param *file		The file to redraw the windows for.
 */

void file_redraw_windows(struct file_block *file)
{
	transact_redraw_all(file);
	account_redraw_all(file);
	accview_redraw_all(file);
	sorder_redraw_all(file);
	preset_redraw_all(file);
	interest_redraw_all(file);
	report_redraw_all(file);
}


/**
 * Process a file for a change of date: add any new standing orders and
 * recalculate all the accounts
 *
 * \param *file		The file to be processed.
 */

void file_process_date_change(struct file_block *file)
{
	if (file == NULL)
		return;

	sorder_process(file);
	account_recalculate_all(file);
	transact_set_window_extent(file);
}


/**
 * Call a callback function, passing each file block pointer in turn.
 *
 * \param *callback	The function to be called for each file.
 */

void file_process_all(void (*callback)(struct file_block *))
{
	struct file_block	*list = file_list;

	/* If no function has been supplied, there's nothing to do! */

	if (callback == NULL)
		return;

	/* Work through all the loaded files, and process each one. */

	while (list != NULL) {
		callback(list);

		list = list->next;
	}

}

