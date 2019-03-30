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
 * \file: sorder.c
 *
 * Standing Order implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"
#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "sorder.h"

#include "account.h"
#include "account_menu.h"
#include "accview.h"
#include "budget.h"
#include "caret.h"
#include "column.h"
#include "currency.h"
#include "date.h"
#include "dialogue.h"
#include "edit.h"
#include "file.h"
#include "filing.h"
#include "flexutils.h"
#include "print_dialogue.h"
#include "sorder_dialogue.h"
#include "sorder_list_window.h"
#include "sort.h"
#include "sort_dialogue.h"
#include "stringbuild.h"
#include "report.h"
#include "transact.h"
#include "window.h"


/* Buffers used in the standing order report. */

#define SORDER_REPORT_LINE_LENGTH 1024
#define SORDER_REPORT_BUF1_LENGTH 256
#define SORDER_REPORT_BUF2_LENGTH 32
#define SORDER_REPORT_BUF3_LENGTH 32


/**
 * Standing Order Data structure -- implementation.
 */

struct sorder {
	date_t			start_date;					/**< The date on which the first order should be processed. */
	int			number;						/**< The number of orders to be added. */
	int			period;						/**< The period between orders. */
	enum date_period	period_unit;					/**< The unit in which the period is measured. */

	date_t			raw_next_date;					/**< The uncorrected date for the next order, used for getting the next. */
	date_t			adjusted_next_date;				/**< The date of the next order, taking into account months, weekends etc. */

	int			left;						/**< The number of orders remaining. */

	enum transact_flags	flags;						/**< Order flags (containing transaction flags, order flags, etc). */

	acct_t			from;						/**< Order details. */
	acct_t			to;
	amt_t			normal_amount;
	amt_t			first_amount;
	amt_t			last_amount;
	char			reference[TRANSACT_REF_FIELD_LEN];
	char			description[TRANSACT_DESCRIPT_FIELD_LEN];
};


/**
 * Standing Order Instance data structure
 */

struct sorder_block {
	/**
	 * The file to which the instance belongs.
	 */
	struct file_block		*file;

	/**
	 * The Standing Order List window instance.
	 */
	struct sorder_list_window	*sorder_window;

	/**
	 * The standing order data for the defined standing orders.
	 */
	struct sorder		*sorders;

	/**
	 * The number of standing orders defined in the file.
	 */
	sorder_t		sorder_count;
};

/* Static Function Prototypes. */

static osbool			sorder_process_edit_window(void *parent, struct sorder_dialogue_data *content);
static osbool			sorder_stop_from_edit_window(struct sorder_block *windat, struct sorder_dialogue_data *content);

static sorder_t			sorder_add(struct file_block *file);
static osbool			sorder_delete(struct file_block *file, sorder_t sorder);

static enum date_adjust		sorder_get_date_adjustment(enum transact_flags flags);


/**
 * Test whether a standing order number is safe to look up in the standing order data array.
 */

#define sorder_valid(windat, sorder) (((sorder) != NULL_TRANSACTION) && ((sorder) >= 0) && ((sorder) < ((windat)->sorder_count)))

/**
 * Initialise the standing order system.
 *
 * \param *sprites		The application sprite area.
 */

void sorder_initialise(osspriteop_area *sprites)
{
	sorder_list_window_initialise(sprites);
	sorder_dialogue_initialise();
}


/**
 * Create a new Standing Order window instance.
 *
 * \param *file			The file to attach the instance to.
 * \return			The instance handle, or NULL on failure.
 */

struct sorder_block *sorder_create_instance(struct file_block *file)
{
	struct sorder_block	*new;

	new = heap_alloc(sizeof(struct sorder_block));
	if (new == NULL)
		return NULL;

	/* Initialise the standing order block. */

	new->file = file;

	new->sorders = NULL;
	new->sorder_count = 0;

	/* Initialise the standing order window. */

	new->sorder_window = sorder_list_window_create_instance(new);
	if (new->sorder_window == NULL) {
		sorder_delete_instance(new);
		return NULL;
	}

	/* Set up the standing order data structures. */

	if (!flexutils_initialise((void **) &(new->sorders))) {
		sorder_delete_instance(new);
		return NULL;
	}

	return new;
}


/**
 * Delete a Standing Order instance, and all of its data.
 *
 * \param *windat		The instance to be deleted.
 */

void sorder_delete_instance(struct sorder_block *windat)
{
	if (windat == NULL)
		return;

	sorder_list_window_delete_instance(windat->sorder_window);

	if (windat->sorders != NULL)
		flexutils_free((void **) &(windat->sorders));

	heap_free(windat);
}


/**
 * Create and open a Standing Order List window for the given file.
 *
 * \param *file			The file to open a window for.
 */

void sorder_open_window(struct file_block *file)
{
	if (file == NULL || file->sorders == NULL || file->sorders->sorder_window == NULL)
		return;

	sorder_list_window_open(file->sorders->sorder_window, file->sorders->sorder_count);
}


/**
 * Recreate the title of the Standing Order List window connected to the given
 * file.
 *
 * \param *file			The file to rebuild the title for.
 */

void sorder_build_window_title(struct file_block *file)
{
	if (file == NULL || file->sorders == NULL)
		return;

	sorder_list_window_build_title(file->sorders->sorder_window);
}


/**
 * Force the complete redraw of the Standing Order list window.
 *
 * \param *file			The file owning the window to redraw.
 */

void sorder_redraw_all(struct file_block *file)
{
	if (file == NULL || file->sorders == NULL)
		return;

	sorder_list_window_redraw(file->sorders->sorder_window, NULL_SORDER, FALSE);
}


/**
 * Open the Standing Order Edit dialogue for a given standing order list window.
 *
 * \param *file			The file to own the dialogue.
 * \param preset		The preset to edit, or NULL_PRESET for add new.
 * \param *ptr			The current Wimp pointer position.
 */

void sorder_open_edit_window(struct file_block *file, sorder_t sorder, wimp_pointer *ptr)
{
	struct sorder_dialogue_data	*content;

	if (file == NULL || file->sorders == NULL)
		return;

	/* Open the dialogue box. */

	content = heap_alloc(sizeof(struct sorder_dialogue_data));
	if (content == NULL)
		return;

	content->action = SORDER_DIALOGUE_ACTION_NONE;
	content->sorder = sorder;

	if (sorder == NULL_SORDER) {
		content->active = FALSE;
		content->start_date = NULL_DATE;
		content->number = 0;
		content->period = 0;
		content->period_unit = DATE_PERIOD_DAYS;
		content->flags = TRANS_FLAGS_NONE;
		content->from = NULL_ACCOUNT;
		content->to = NULL_ACCOUNT;
		content->normal_amount = NULL_CURRENCY;
		content->first_amount = NULL_CURRENCY;
		content->last_amount = NULL_CURRENCY;
		*(content->reference) = '\0';
		*(content->description) = '\0';
	} else {
		content->active = (file->sorders->sorders[sorder].adjusted_next_date != NULL_DATE) ? TRUE : FALSE;
		content->start_date = file->sorders->sorders[sorder].start_date;
		content->number = file->sorders->sorders[sorder].number;
		content->period = file->sorders->sorders[sorder].period;
		content->period_unit = file->sorders->sorders[sorder].period_unit;
		content->flags = file->sorders->sorders[sorder].flags;
		content->from = file->sorders->sorders[sorder].from;
		content->to = file->sorders->sorders[sorder].to;
		content->normal_amount = file->sorders->sorders[sorder].normal_amount;
		content->first_amount = file->sorders->sorders[sorder].first_amount;
		content->last_amount = file->sorders->sorders[sorder].last_amount;
		string_copy(content->reference, file->sorders->sorders[sorder].reference, TRANSACT_REF_FIELD_LEN);
		string_copy(content->description, file->sorders->sorders[sorder].description, TRANSACT_DESCRIPT_FIELD_LEN);
	}

	sorder_dialogue_open(ptr, file->sorders, file, sorder_process_edit_window, content);
}


/**
 * Process data associated with the currently open Standing Order Edit window.
 *
 * \param *parent		The standing order instance holding the section.
 * \param *content		The content of the dialogue box.
 * \return			TRUE if processed; else FALSE.
 */

static osbool sorder_process_edit_window(void *parent, struct sorder_dialogue_data *content)
{
	struct sorder_block	*windat = parent;
	int			done;

	if (content == NULL || windat == NULL)
		return FALSE;

	if (!sorder_valid(windat, content->sorder))
		return FALSE;

	if (content->action == SORDER_DIALOGUE_ACTION_DELETE) {
		if (error_msgs_report_question("DeleteSOrder", "DeleteSOrderB") == 4)
			return FALSE;

		return sorder_delete(windat->file, content->sorder);
	} else if (content->action == SORDER_DIALOGUE_ACTION_STOP) {
		return sorder_stop_from_edit_window(windat, content);
	} else if (content->action != SORDER_DIALOGUE_ACTION_OK) {
		return FALSE;
	}

	/* If the standing order doesn't exsit, create it.  If it does exist, validate any data that requires it. */

	if (content->sorder == NULL_SORDER) {
		content->sorder = sorder_add(windat->file);
		if (content->sorder == NULL_SORDER) {
			// \TODO - Error no mem!
			return FALSE;
		}

		windat->sorders[content->sorder].adjusted_next_date = NULL_DATE; /* Set to allow editing. */

		done = 0;
	} else {
		done = windat->sorders[content->sorder].number - windat->sorders[content->sorder].left;
		if (content->number < done && windat->sorders[content->sorder].adjusted_next_date != NULL_DATE) {
			error_msgs_report_error("BadSONumber");
			return FALSE;
		}

		if (windat->sorders[content->sorder].adjusted_next_date == NULL_DATE &&
				windat->sorders[content->sorder].start_date == content->start_date &&
				(error_msgs_report_question("CheckSODate", "CheckSODateB") == 4))
			return FALSE;
	}

	/* If the standing order was created OK, store the rest of the data. */

	if (content->sorder == NULL_SORDER)
		return FALSE;

	/* If it's a new/finished order, get the start date and period and set up the date fields. */

	if (windat->sorders[content->sorder].adjusted_next_date == NULL_DATE) {
		windat->sorders[content->sorder].period_unit = content->period_unit;
		windat->sorders[content->sorder].start_date = content->start_date;
		windat->sorders[content->sorder].raw_next_date = content->start_date;

		windat->sorders[content->sorder].adjusted_next_date =
				date_find_working_day(windat->sorders[content->sorder].raw_next_date,
				sorder_get_date_adjustment(windat->sorders[content->sorder].flags));

		windat->sorders[content->sorder].period = content->period;

		done = 0;
	}

	/* Get the number of transactions. */

	windat->sorders[content->sorder].number = content->number;

	windat->sorders[content->sorder].left = content->number - done;

	if (windat->sorders[content->sorder].left == 0)
		windat->sorders[content->sorder].adjusted_next_date = NULL_DATE;

	/* Get the flags. */

	windat->sorders[content->sorder].flags = content->flags;

	/* Get the from and to fields. */

	windat->sorders[content->sorder].from = content->from;
	windat->sorders[content->sorder].to = content->to;

	/* Get the amounts. */

	windat->sorders[content->sorder].normal_amount = content->normal_amount;
	windat->sorders[content->sorder].first_amount = content->first_amount;
	windat->sorders[content->sorder].last_amount = content->last_amount;

	/* Store the reference. */

	string_copy(windat->sorders[content->sorder].reference, content->reference, TRANSACT_REF_FIELD_LEN);
	string_copy(windat->sorders[content->sorder].description, content->description, TRANSACT_DESCRIPT_FIELD_LEN);

	/* Update the display. */

	if (config_opt_read("AutoSortSOrders"))
		sorder_sort(windat);
	else
		sorder_list_window_redraw(windat->sorder_window, content->sorder, FALSE);

	file_set_data_integrity(windat->file, TRUE);
	sorder_process(windat->file);
	account_recalculate_all(windat->file);
	transact_set_window_extent(windat->file);

	return TRUE;
}



/**
 * Stop the standing order associated with the currently open Standing
 * Order Edit window.  Set the next dates to NULL and zero the left count.
 *
 * \param *windat		The standing order instance holding the order.
 * \param sorder		The order to stop.
 * \return			TRUE if stopped; else FALSE.
 */

static osbool sorder_stop_from_edit_window(struct sorder_block *windat, struct sorder_dialogue_data *content)
{
	if (windat == NULL || content == NULL)
		return FALSE;

	if (!sorder_valid(windat, content->sorder))
		return FALSE;

	if (error_msgs_report_question("StopSOrder", "StopSOrderB") == 4)
		return FALSE;

	/* Stop the order */

	windat->sorders[content->sorder].raw_next_date = NULL_DATE;
	windat->sorders[content->sorder].adjusted_next_date = NULL_DATE;
	windat->sorders[content->sorder].left = 0;

	/* Free the standing order edit window's contents. */

	content->active = FALSE;

	/* Update the main standing order display window. */

	if (config_opt_read("AutoSortSOrders"))
		sorder_sort(windat);
	else
		sorder_list_window_redraw(windat->sorder_window, content->sorder, TRUE);

	file_set_data_integrity(windat->file, TRUE);

	return TRUE;
}


/**
 * Sort the standing orders in a given file based on that file's sort setting.
 *
 * \param *windat		The standing order instance to sort.
 */

void sorder_sort(struct sorder_block *windat)
{
	if (windat == NULL)
		return;

	sorder_list_window_sort(windat->sorder_window);
}


/**
 * Create a new standing order with null details.  Values are left to be set
 * up later.
 *
 * \param *file			The file to add the standing order to.
 * \return			The new standing order index, or NULL_SORDER.
 */

static sorder_t sorder_add(struct file_block *file)
{
	int	new;

	if (file == NULL || file->sorders == NULL)
		return NULL_SORDER;

	if (!flexutils_resize((void **) &(file->sorders->sorders), sizeof(struct sorder), file->sorders->sorder_count + 1)) {
		error_msgs_report_error("NoMemNewSO");
		return NULL_SORDER;
	}

	new = file->sorders->sorder_count++;

	file->sorders->sorders[new].start_date = NULL_DATE;
	file->sorders->sorders[new].raw_next_date = NULL_DATE;
	file->sorders->sorders[new].adjusted_next_date = NULL_DATE;

	file->sorders->sorders[new].number = 0;
	file->sorders->sorders[new].left = 0;
	file->sorders->sorders[new].period = 0;
	file->sorders->sorders[new].period_unit = 0;

	file->sorders->sorders[new].flags = 0;

	file->sorders->sorders[new].from = NULL_ACCOUNT;
	file->sorders->sorders[new].to = NULL_ACCOUNT;
	file->sorders->sorders[new].normal_amount = NULL_CURRENCY;
	file->sorders->sorders[new].first_amount = NULL_CURRENCY;
	file->sorders->sorders[new].last_amount = NULL_CURRENCY;

	*file->sorders->sorders[new].reference = '\0';
	*file->sorders->sorders[new].description = '\0';

	sorder_list_window_add_sorder(file->sorders->sorder_window, new);

	return new;
}


/**
 * Delete a standing order from a file.
 *
 * \param *file			The file to act on.
 * \param sorder		The standing order to be deleted.
 * \return 			TRUE if successful; else FALSE.
 */

static osbool sorder_delete(struct file_block *file, sorder_t sorder)
{
	int	i, index;

	if (file == NULL || file->sorders == NULL || sorder == NULL_SORDER || sorder >= file->sorders->sorder_count)
		return FALSE;


	/* Delete the order */

	if (!flexutils_delete_object((void **) &(file->sorders->sorders), sizeof(struct sorder), sorder)) {
		error_msgs_report_error("BadDelete");
		return FALSE;
	}

	file->sorders->sorder_count--;

	sorder_list_window_delete_sorder(file->sorders->sorder_window, sorder);

	file_set_data_integrity(file, TRUE);

	return TRUE;
}


/**
 * Purge unused standing orders from a file.
 *
 * \param *file			The file to purge.
 */

void sorder_purge(struct file_block *file)
{
	int	i;

	if (file == NULL || file->sorders == NULL)
		return;

	for (i = 0; i < file->sorders->sorder_count; i++) {
		if (file->sorders->sorders[i].adjusted_next_date == NULL_DATE && sorder_delete(file, i))
			i--; /* Account for the record having been deleted. */
	}
}


/**
 * Find the number of standing orders in a file.
 *
 * \param *file			The file to interrogate.
 * \return			The number of standing orders in the file.
 */

int sorder_get_count(struct file_block *file)
{
	return (file != NULL && file->sorders != NULL) ? file->sorders->sorder_count : 0;
}


/**
 * Return the file associated with a standing order instance.
 *
 * \param *instance		The standing order instance to query.
 * \return			The associated file, or NULL.
 */

struct file_block *sorder_get_file(struct sorder_block *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->file;
}


/**
 * Return the from account of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the from account for.
 * \return			The from account of the standing order, or NULL_ACCOUNT.
 */

acct_t sorder_get_from(struct file_block *file, sorder_t sorder)
{
	if (file == NULL || file->sorders == NULL || !sorder_valid(file->sorders, sorder))
		return NULL_ACCOUNT;

	return file->sorders->sorders[sorder].from;
}


/**
 * Return the to account of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the to account for.
 * \return			The to account of the standing order, or NULL_ACCOUNT.
 */

acct_t sorder_get_to(struct file_block *file, sorder_t sorder)
{
	if (file == NULL || file->sorders == NULL || !sorder_valid(file->sorders, sorder))
		return NULL_ACCOUNT;

	return file->sorders->sorders[sorder].to;
}


/**
 * Return the standing order flags for a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the flags for.
 * \return			The flags of the standing order, or TRANS_FLAGS_NONE.
 */

enum transact_flags sorder_get_flags(struct file_block *file, sorder_t sorder)
{
	if (file == NULL || file->sorders == NULL || !sorder_valid(file->sorders, sorder))
		return TRANS_FLAGS_NONE;

	return file->sorders->sorders[sorder].flags;
}


/**
 * Return the amount of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the amount for.
 * \return			The amount of the standing order, or NULL_CURRENCY.
 */

amt_t sorder_get_amount(struct file_block *file, sorder_t sorder)
{
	if (file == NULL || file->sorders == NULL || !sorder_valid(file->sorders, sorder))
		return NULL_CURRENCY;

	return file->sorders->sorders[sorder].normal_amount;
}


/**
 * Return the next date of a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the next date for.
 * \return			The next date of the standing order, or NULL_DATE.
 */

date_t sorder_get_next_date(struct file_block *file, sorder_t sorder)
{
	if (file == NULL || file->sorders == NULL || !sorder_valid(file->sorders, sorder))
		return NULL_DATE;

	return file->sorders->sorders[sorder].adjusted_next_date;
}


/**
 * Return the remaining transactions for a standing order.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the remaining transactions for.
 * \return			The remaining transactions for the standing order, or 0.
 */

date_t sorder_get_remaining_transactions(struct file_block *file, sorder_t sorder)
{
	if (file == NULL || file->sorders == NULL || !sorder_valid(file->sorders, sorder))
		return NULL_DATE;

	return file->sorders->sorders[sorder].left;
}


/**
 * Return the description for a standing order.
 *
 * If a buffer is supplied, the description is copied into that buffer and a
 * pointer to the buffer is returned; if one is not, then a pointer to the
 * description in the preset array is returned instead. In the latter
 * case, this pointer will become invalid as soon as any operation is carried
 * out which might shift blocks in the flex heap.
 *
 * \param *file			The file containing the standing order.
 * \param sorder		The standing order to return the description of.
 * \param *buffer		Pointer to a buffer to take the description, or
 *				NULL to return a volatile pointer to the
 *				original data.
 * \param length		Length of the supplied buffer, in bytes, or 0.
 * \return			Pointer to the resulting description string,
 *				either the supplied buffer or the original.
 */

char *sorder_get_description(struct file_block *file, sorder_t sorder, char *buffer, size_t length)
{
	if (file == NULL || file->sorders == NULL || !sorder_valid(file->sorders, sorder)) {
		if (buffer != NULL && length > 0) {
			*buffer = '\0';
			return buffer;
		}

		return NULL;
	}

	if (buffer == NULL || length == 0)
		return file->sorders->sorders[sorder].description;

	string_copy(buffer, file->sorders->sorders[sorder].description, length);

	return buffer;
}

/**
 * Scan the standing orders in a file, adding transactions for any which have
 * fallen due.
 *
 * \param *file			The file to process.
 */

void sorder_process(struct file_block *file)
{
	unsigned int	today;
	sorder_t	order;
	amt_t		amount;
	osbool		changed;
	char		ref[TRANSACT_REF_FIELD_LEN], desc[TRANSACT_DESCRIPT_FIELD_LEN];

	if (file == NULL || file->sorders == NULL)
		return;

	#ifdef DEBUG
	debug_printf("\\YStanding Order processing");
	#endif

	today = date_today();
	changed = FALSE;

	for (order = 0; order < file->sorders->sorder_count; order++) {
		#ifdef DEBUG
		debug_printf ("Processing order %d...", order);
		#endif

		/* While the next date for the standing order is today or before today, process it. */

		while (file->sorders->sorders[order].adjusted_next_date!= NULL_DATE && file->sorders->sorders[order].adjusted_next_date <= today) {
			/* Action the standing order. */

			if (file->sorders->sorders[order].left == file->sorders->sorders[order].number)
				amount = file->sorders->sorders[order].first_amount;
			else if (file->sorders->sorders[order].left == 1)
				amount = file->sorders->sorders[order].last_amount;
			else
				amount = file->sorders->sorders[order].normal_amount;

			/* Reference and description are copied out of the block first as pointers are passed in to transact_add_raw_entry()
			 * and the act of adding the transaction will move the flex block and invalidate those pointers before they
			 * get used.
			 */

			string_copy(ref, file->sorders->sorders[order].reference, TRANSACT_REF_FIELD_LEN);
			string_copy(desc, file->sorders->sorders[order].description, TRANSACT_DESCRIPT_FIELD_LEN);

			transact_add_raw_entry(file, file->sorders->sorders[order].adjusted_next_date,
					file->sorders->sorders[order].from, file->sorders->sorders[order].to,
					file->sorders->sorders[order].flags & (TRANS_REC_FROM | TRANS_REC_TO), amount, ref, desc);

			#ifdef DEBUG
			debug_printf("Adding SO, ref '%s', desc '%s'...", file->sorders->sorders[order].reference, file->sorders->sorders[order].description);
			#endif

			changed = TRUE;

			/* Decrement the outstanding orders. */

			file->sorders->sorders[order].left--;

			/* If there are outstanding orders to carry out, work out the next date and remember that. */

			if (file->sorders->sorders[order].left > 0) {
				file->sorders->sorders[order].raw_next_date = date_add_period(file->sorders->sorders[order].raw_next_date,
						file->sorders->sorders[order].period_unit, file->sorders->sorders[order].period);

				file->sorders->sorders[order].adjusted_next_date = date_find_working_day(file->sorders->sorders[order].raw_next_date,
						sorder_get_date_adjustment(file->sorders->sorders[order].flags));
			} else {
				file->sorders->sorders[order].adjusted_next_date = NULL_DATE;
			}

			/* Redraw the standing order in the standing order window. */

			sorder_list_window_redraw(file->sorders->sorder_window, order, TRUE);
		}
	}

	/* Update the trial values for the file. */

	sorder_trial(file);

	/* Refresh things if they have changed. */

	if (changed) {
		file_set_data_integrity(file, TRUE);

		if (config_opt_read("SortAfterSOrders")) {
			transact_sort(file->transacts);
		} else {
			transact_redraw_all(file);
		}

		if (config_opt_read("AutoSortSOrders"))
			sorder_sort(file->sorders);

		accview_rebuild_all(file);
	}
}


/**
 * Scan the standing orders in a file, and update the traial values to reflect
 * any pending transactions.
 *
 * \param *file			The file to scan.
 */

void sorder_trial(struct file_block *file)
{
	unsigned int	trial_date, raw_next_date, adjusted_next_date;
	sorder_t	order;
	int		amount, left;

	#ifdef DEBUG
	debug_printf("\\YStanding Order trialling");
	#endif

	/* Find the cutoff date for the trial. */

	trial_date = date_add_period(date_today(), DATE_PERIOD_DAYS, budget_get_sorder_trial(file));

	/* Zero the order trial values. */

	account_zero_sorder_trial(file);

	/* Process the standing orders. */

	for (order = 0; order < file->sorders->sorder_count; order++) {
		#ifdef DEBUG
		debug_printf("Trialling order %d...", order);
		#endif

		/* While the next date for the standing order is today or before today, process it. */

		raw_next_date = file->sorders->sorders[order].raw_next_date;
		adjusted_next_date = file->sorders->sorders[order].adjusted_next_date;
		left = file->sorders->sorders[order].left;

		while (adjusted_next_date!= NULL_DATE && adjusted_next_date <= trial_date) {
			/* Action the standing order. */

			if (left == file->sorders->sorders[order].number)
				amount = file->sorders->sorders[order].first_amount;
			else if (file->sorders->sorders[order].left == 1)
				amount = file->sorders->sorders[order].last_amount;
			else
				amount = file->sorders->sorders[order].normal_amount;

			#ifdef DEBUG
			debug_printf("Adding trial SO, ref '%s', desc '%s'...", file->sorders->sorders[order].reference, file->sorders->sorders[order].description);
			#endif

			if (file->sorders->sorders[order].from != NULL_ACCOUNT)
				account_adjust_sorder_trial(file, file->sorders->sorders[order].from, -amount);

			if (file->sorders->sorders[order].to != NULL_ACCOUNT)
				account_adjust_sorder_trial(file, file->sorders->sorders[order].to, +amount);

			/* Decrement the outstanding orders. */

			left--;

			/* If there are outstanding orders to carry out, work out the next date and remember that. */

			if (left > 0) {
				raw_next_date = date_add_period(raw_next_date, file->sorders->sorders[order].period_unit, file->sorders->sorders[order].period);
				adjusted_next_date = date_find_working_day(raw_next_date, sorder_get_date_adjustment(file->sorders->sorders[order].flags));
			} else {
				adjusted_next_date = NULL_DATE;
			}
		}
	}
}


/**
 * Generate a report detailing all of the standing orders in a file.
 *
 * \param *file			The file to report on.
 */

void sorder_full_report(struct file_block *file)
{
	struct report	*report;
	sorder_t	sorder;
	char		line[SORDER_REPORT_LINE_LENGTH], numbuf1[SORDER_REPORT_BUF1_LENGTH], numbuf2[SORDER_REPORT_BUF2_LENGTH], numbuf3[SORDER_REPORT_BUF3_LENGTH];

	if (file == NULL || file->sorders == NULL)
		return;

	if (!stringbuild_initialise(line, SORDER_REPORT_LINE_LENGTH))
		return;

	msgs_lookup("SORWinT", line, SORDER_REPORT_LINE_LENGTH);
	report = report_open(file, line, NULL);

	if (report == NULL) {
		stringbuild_cancel();
		return;
	}

	hourglass_on();

	stringbuild_reset();
	stringbuild_add_message_param("SORTitle", file_get_leafname(file, NULL, 0), NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	stringbuild_reset();
	date_convert_to_string(date_today(), numbuf1, SORDER_REPORT_BUF1_LENGTH);
	stringbuild_add_message_param("SORHeader", numbuf1, NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	stringbuild_reset();
	string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", file->sorders->sorder_count);
	stringbuild_add_message_param("SORCount", numbuf1, NULL, NULL, NULL);
	stringbuild_report_line(report, 0);

	/* Output the data for each of the standing orders in turn. */

	for (sorder = 0; sorder < file->sorders->sorder_count; sorder++) {
		report_write_line(report, 0, ""); /* Separate each entry with a blank line. */

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", sorder + 1);
		stringbuild_add_message_param("SORNumber", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORFrom", account_get_name(file, file->sorders->sorders[sorder].from), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORTo", account_get_name(file, file->sorders->sorders[sorder].to), NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		stringbuild_add_message_param("SORRef", file->sorders->sorders[sorder].reference, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		currency_convert_to_string(file->sorders->sorders[sorder].normal_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORAmount", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		if (file->sorders->sorders[sorder].normal_amount != file->sorders->sorders[sorder].first_amount) {
			stringbuild_reset();
			currency_convert_to_string(file->sorders->sorders[sorder].first_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
			stringbuild_add_message_param("SORFirst", numbuf1, NULL, NULL, NULL);
			stringbuild_report_line(report, 0);
		}

		if (file->sorders->sorders[sorder].normal_amount != file->sorders->sorders[sorder].last_amount) {
			stringbuild_reset();
			currency_convert_to_string(file->sorders->sorders[sorder].last_amount, numbuf1, SORDER_REPORT_BUF1_LENGTH);
			stringbuild_add_message_param("SORFirst", numbuf1, NULL, NULL, NULL);
			stringbuild_report_line(report, 0);
		}

		stringbuild_reset();
		stringbuild_add_message_param("SORDesc", file->sorders->sorders[sorder].description, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", file->sorders->sorders[sorder].number);
		string_printf(numbuf2, SORDER_REPORT_BUF2_LENGTH, "%d", file->sorders->sorders[sorder].number - file->sorders->sorders[sorder].left);
		string_printf(numbuf3, SORDER_REPORT_BUF3_LENGTH, "%d", file->sorders->sorders[sorder].left);
		stringbuild_add_message_param("SORCounts", numbuf1, numbuf2, numbuf3, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		date_convert_to_string(file->sorders->sorders[sorder].start_date, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORStart", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);

		stringbuild_reset();
		string_printf(numbuf1, SORDER_REPORT_BUF1_LENGTH, "%d", file->sorders->sorders[sorder].period);
		switch (file->sorders->sorders[sorder].period_unit) {
		case DATE_PERIOD_DAYS:
			msgs_lookup("SOrderDays", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		case DATE_PERIOD_MONTHS:
			msgs_lookup("SOrderMonths", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		case DATE_PERIOD_YEARS:
			msgs_lookup("SOrderYears", numbuf2, SORDER_REPORT_BUF2_LENGTH);
			break;

		default:
			*numbuf2 = '\0';
			break;
		}
		stringbuild_add_message_param("SOREvery", numbuf1, numbuf2, NULL, NULL);
		stringbuild_report_line(report, 0);

		if (file->sorders->sorders[sorder].flags & TRANS_SKIP_FORWARD) {
			stringbuild_reset();
			stringbuild_add_message("SORAvoidFwd");
			stringbuild_report_line(report, 0);
		} else if (file->sorders->sorders[sorder].flags & TRANS_SKIP_BACKWARD) {
			stringbuild_reset();
			stringbuild_add_message("SORAvoidBack");
			stringbuild_report_line(report, 0);
		}

		stringbuild_reset();
		if (file->sorders->sorders[sorder].adjusted_next_date != NULL_DATE)
			date_convert_to_string(file->sorders->sorders[sorder].adjusted_next_date, numbuf1, SORDER_REPORT_BUF1_LENGTH);
		else
			msgs_lookup("SOrderStopped", numbuf1, SORDER_REPORT_BUF1_LENGTH);
		stringbuild_add_message_param("SORNext", numbuf1, NULL, NULL, NULL);
		stringbuild_report_line(report, 0);
	}

	/* Close the report. */

	stringbuild_cancel();

	report_close(report);

	hourglass_off();
}


/**
 * Save the standing order details from a file to a CashBook file.
 *
 * \param *file			The file to write.
 * \param *out			The file handle to write to.
 */

void sorder_write_file(struct file_block *file, FILE *out)
{
	int	i;

	if (file == NULL || file->sorders == NULL)
		return;

	fprintf(out, "\n[StandingOrders]\n");

	fprintf(out, "Entries: %x\n", file->sorders->sorder_count);

	sorder_list_window_write_file(file->sorders->sorder_window, out);

	for (i = 0; i < file->sorders->sorder_count; i++) {
		fprintf(out, "@: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
				file->sorders->sorders[i].start_date, file->sorders->sorders[i].number, file->sorders->sorders[i].period,
				file->sorders->sorders[i].period_unit, file->sorders->sorders[i].raw_next_date, file->sorders->sorders[i].adjusted_next_date,
				file->sorders->sorders[i].left, file->sorders->sorders[i].flags, file->sorders->sorders[i].from, file->sorders->sorders[i].to,
				file->sorders->sorders[i].normal_amount, file->sorders->sorders[i].first_amount, file->sorders->sorders[i].last_amount);
		if (*(file->sorders->sorders[i].reference) != '\0')
			config_write_token_pair(out, "Ref", file->sorders->sorders[i].reference);
		if (*(file->sorders->sorders[i].description) != '\0')
			config_write_token_pair(out, "Desc", file->sorders->sorders[i].description);
	}
}


/**
 * Read standing order details from a CashBook file into a file block.
 *
 * \param *file			The file to read in to.
 * \param *in			The filing handle to read in from.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sorder_read_file(struct file_block *file, struct filing_block *in)
{
	sorder_t		sorder = NULL_SORDER;
	size_t			block_size;

	if (file == NULL || file->sorders == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("\\GLoading Standing Orders.");
#endif

	/* Identify the current size of the flex block allocation. */

	if (!flexutils_load_initialise((void **) &(file->sorders->sorders), sizeof(struct sorder), &block_size)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	/* Process the file contents until the end of the section. */

	do {
		if (filing_test_token(in, "Entries")) {
			block_size = filing_get_int_field(in);
			if (block_size > file->sorders->sorder_count) {
				#ifdef DEBUG
				debug_printf("Section block pre-expand to %d", block_size);
				#endif
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
			} else {
				block_size = file->sorders->sorder_count;
			}
		} else if (filing_test_token(in, "WinColumns")) {
			sorder_list_window_read_file_wincolumns(file->sorders->sorder_window, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "SortOrder")) {
			sorder_list_window_read_file_sortorder(file->sorders->sorder_window, filing_get_text_value(in, NULL, 0));
		} else if (filing_test_token(in, "@")) {
			file->sorders->sorder_count++;
			if (file->sorders->sorder_count > block_size) {
				block_size = file->sorders->sorder_count;
				if (!flexutils_load_resize(block_size)) {
					filing_set_status(in, FILING_STATUS_MEMORY);
					return FALSE;
				}
				#ifdef DEBUG
				debug_printf("Section block expand to %d", block_size);
				#endif
			}
			sorder = file->sorders->sorder_count - 1;
			file->sorders->sorders[sorder].start_date = date_get_date_field(in);
			file->sorders->sorders[sorder].number = filing_get_int_field(in);
			file->sorders->sorders[sorder].period = filing_get_int_field(in);
			file->sorders->sorders[sorder].period_unit = date_get_period_field(in);
			file->sorders->sorders[sorder].raw_next_date = date_get_date_field(in);
			file->sorders->sorders[sorder].adjusted_next_date = date_get_date_field(in);
			file->sorders->sorders[sorder].left = filing_get_int_field(in);
			file->sorders->sorders[sorder].flags = transact_get_flags_field(in);
			file->sorders->sorders[sorder].from = account_get_account_field(in);
			file->sorders->sorders[sorder].to = account_get_account_field(in);
			file->sorders->sorders[sorder].normal_amount = currency_get_currency_field(in);
			file->sorders->sorders[sorder].first_amount = currency_get_currency_field(in);
			file->sorders->sorders[sorder].last_amount = currency_get_currency_field(in);
			*(file->sorders->sorders[sorder].reference) = '\0';
			*(file->sorders->sorders[sorder].description) = '\0';
		} else if (sorder != NULL_SORDER && filing_test_token(in, "Ref")) {
			filing_get_text_value(in, file->sorders->sorders[sorder].reference, TRANSACT_REF_FIELD_LEN);
		} else if (sorder != NULL_SORDER && filing_test_token(in, "Desc")) {
			filing_get_text_value(in, file->sorders->sorders[sorder].description, TRANSACT_DESCRIPT_FIELD_LEN);
		} else {
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
		}
	} while (filing_get_next_token(in));

	/* Shrink the flex block back down to the minimum required. */

	if (!flexutils_load_shrink(file->sorders->sorder_count)) {
		filing_set_status(in, FILING_STATUS_BAD_MEMORY);
		return FALSE;
	}

	return TRUE;
}


/**
 * Check the standing orders in a file to see if the given account is used
 * in any of them.
 *
 * \param *file			The file to check.
 * \param account		The account to search for.
 * \return			TRUE if the account is used; FALSE if not.
 */

osbool sorder_check_account(struct file_block *file, int account)
{
	int		i;

	if (file == NULL || file->sorders == NULL)
		return FALSE;

	for (i = 0; i < file->sorders->sorder_count; i++)
		if (file->sorders->sorders[i].from == account || file->sorders->sorders[i].to == account)
			return TRUE;

	return FALSE;
}


/**
 * Return a standing order date adjustment value based on the standing order
 * flags.
 *
 * \param flags			The flags to be converted.
 * \return			The corresponding date adjustment;
 */

static enum date_adjust sorder_get_date_adjustment(enum transact_flags flags)
{
	if (flags & TRANS_SKIP_FORWARD)
		return DATE_ADJUST_FORWARD;
	else if (flags & TRANS_SKIP_BACKWARD)
		return DATE_ADJUST_BACKWARD;
	else
		return DATE_ADJUST_NONE;
}

