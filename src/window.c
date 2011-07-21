/* CashBook - window.c
 *
 * (C) Stephen Fryatt, 2003-2011
 */

/* ANSI C header files */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osbyte.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/windows.h"

/* Application header files */

#include "global.h"
#include "window.h"

#include "account.h"
#include "accview.h"
#include "edit.h"
#include "file.h"
#include "presets.h"
#include "sorder.h"
#include "transact.h"

/* ==================================================================================================================
 * Global variables.
 */

static file_data	*column_drag_file;
static int		column_drag_data;
static int		column_drag_icon;

static wimp_i		column_drag_icon;					/**< The handle of the icon being dragged.			*/
static void		(*column_drag_callback)(file_data *, int, wimp_i, int);	/**< The callback handler for the drag.				*/

static void		column_terminate_drag(wimp_dragged *drag, void *data);

static int		column_get_minimum_group_width(char *mapping, char *widths, int heading);
static int		column_get_minimum_width(char *widths, int column);


/* ==================================================================================================================
 * Setting up columned windows.
 */

/* Set the window column data up, based on the supplied values. */

void column_init_window(int width[], int position[], int columns, char *widths)
{
	int	i;
	char	*copy, *str;

	copy = strdup(widths);

	/* Read the column widths and set up an array. */

	str = strtok(copy, ",");

	for (i=0; i < columns; i++) {
		if (str != NULL)
			width[i] = atoi (str);
		else
			width[i] = COLUMN_WIDTH_DEFAULT; /* Stick a default value in if the config data is missing. */

		str = strtok(NULL, ",");
	}

	free(copy);

	/* Now set the positions, based on the widths that were read in. */

	position[0] = 0;

	for (i=1; i < columns; i++)
		position[i] = position[i-1] + width[i-1] + COLUMN_GUTTER;
}

/* ------------------------------------------------------------------------------------------------------------------ */

char *column_write_as_text(int width[], int columns, char *buffer)
{
	int	i;

	/* Start the buffer off as a NULL string that will be appended to. */

	*buffer = '\0';

	/* Write the column widths to the buffer. */

	for (i=0; i < columns; i++)
		sprintf(strrchr (buffer, '\0'), "%d,", width[i]);

	/* Remove the terminating ','. */

	*strrchr(buffer, ',') = '\0';

	return buffer;
}

/* ------------------------------------------------------------------------------------------------------------------ */


/* ==================================================================================================================
 * Column dragging
 */


void column_start_drag(wimp_pointer *ptr, file_data *file, int data, wimp_w w, char *mapping, char *widths, void (*callback)(file_data *, int, wimp_i, int))
{
	wimp_window_state	window;
	wimp_window_info	parent;
	wimp_icon_state		icon;
	wimp_drag		drag;
	int			ox, oy;

	window.w = ptr->w;
	wimp_get_window_state(&window);

	ox = window.visible.x0 - window.xscroll;
	oy = window.visible.y1 - window.yscroll;

	icon.w = ptr->w;
	icon.i = ptr->i;
	wimp_get_icon_state(&icon);

	parent.w = w;
	wimp_get_window_info_header_only(&parent);

	column_drag_icon = ptr->i;
	column_drag_file = file;
	column_drag_data = data;
	column_drag_callback = callback;

	/* If the window exists and the hot-spot was hit, set up the drag parameters and start the drag. */

	if (column_drag_file != NULL && ptr->pos.x >= (ox + icon.icon.extent.x1 - COLUMN_DRAG_HOTSPOT)) {
		drag.w = ptr->w;
		drag.type = wimp_DRAG_USER_RUBBER;

		drag.initial.x0 = ox + icon.icon.extent.x0;
		drag.initial.y0 = parent.visible.y0;
		drag.initial.x1 = ox + icon.icon.extent.x1;
		drag.initial.y1 = oy + icon.icon.extent.y1;

		drag.bbox.x0 = ox + icon.icon.extent.x0 -
				(icon.icon.extent.x1 - icon.icon.extent.x0 - column_get_minimum_group_width(mapping, widths, ptr->i));
		drag.bbox.y0 = parent.visible.y0;
		drag.bbox.x1 = 0x7fffffff;
		drag.bbox.y1 = oy + icon.icon.extent.y1;

		wimp_drag_box(&drag);

		event_set_drag_handler(column_terminate_drag, NULL, NULL);
	}
}

/* ------------------------------------------------------------------------------------------------------------------ */


static void column_terminate_drag(wimp_dragged *drag, void *data)
{
	int		width;

	width = drag->final.x1 - drag->final.x0;

	if (column_drag_callback != NULL) {
		column_drag_callback(column_drag_file, column_drag_data, column_drag_icon, width);
		set_file_data_integrity(column_drag_file, TRUE);
	}
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Reallocate the new group width across all the columns in the group.  Most columns just take their minimum width,
 * while the right-hand column takes up the slack.
 *
 * Column position redraw data is updated.
 */

void update_dragged_columns(char *mapping, char *widths, int heading, int width, int col_widths[], int col_pos[], int columns)
{
	int	sum = 0, i, left, right;

	left = column_get_leftmost_in_group(mapping, heading);
	right = column_get_rightmost_in_group(mapping, heading);

	for (i = left; i <= right; i++) {
		if (i == right) {
			col_widths[i] = width - (sum + COLUMN_HEADING_MARGIN);
		} else {
			col_widths[i] = column_get_minimum_width(widths, i);
			sum +=  (column_get_minimum_width(widths, i) + COLUMN_GUTTER);
		}
	}

	for (i = left+1; i < columns; i++)
		col_pos[i] = col_pos[i-1] + col_widths[i-1] + COLUMN_GUTTER;
}

/* ==================================================================================================================
 * Column grouping information
 */
/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the group containing the given column. */

int column_get_group(char *mapping, int column)
{
	int	group = 0;

	while (column_get_rightmost_in_group(mapping, group) < column)
		group++;

	return group;
}

/* Return the left-hand column in a group. */

int column_get_leftmost_in_group(char *mapping, int heading)
{
	char	*copy, *token, *value;
	int	column = 0;

	copy = strdup(mapping);

	if (copy == NULL)
		return column;

	/* Find the mapping block for the required heading. */

	token = strtok(copy, ";");

	while (heading-- > 0)
		token = strtok(NULL, ";");

	/* Find the left-most column in the block. */

	value = strtok(token, ",");
	column = atoi(value);

	free(copy);

	return column;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the right-hand column in a group. */

int column_get_rightmost_in_group(char *mapping, int heading)
{
	char	*copy, *token, *value;
	int	column = 0;

	copy = strdup(mapping);

	if (copy == NULL)
		return column;

	/* Find the mapping block for the required heading. */

	token = strtok(copy, ";");

	while (heading-- > 0)
		token = strtok(NULL, ";");

	/* Find the right-most column in the block. */

	value = strtok(token, ",");

	while (value != NULL) {
		column = atoi(value);
		value = strtok(NULL, ",");
	}

	free(copy);

	return column;
}


/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the minimum width that a group of columns can be dragged to.  This is a simple sum of the minimum
 * widths of all the columns in that group.
 */

static int column_get_minimum_group_width(char *mapping, char *widths, int heading)
{
	int	left, right, width, i;

	width = 0;

	left = column_get_leftmost_in_group(mapping, heading);
	right = column_get_rightmost_in_group(mapping, heading);

	for (i = left; i <= right; i++)
		width += column_get_minimum_width (widths, i);

	return(width);
}

/* ------------------------------------------------------------------------------------------------------------------ */

/* Return the minimum column width for the given column by parsing the CSV list. */

static int column_get_minimum_width(char *widths, int column)
{
	int	width, i;
	char	*token, *copy;


	width = COLUMN_DRAG_MIN;
	copy = strdup(widths);

	if (copy == NULL)
		return width;

	token = strtok(copy, ",");
	i = 0;

	while (i < column) {
		token = strtok(NULL, ",");
		i++;
	}

	if (token != NULL)
		width = atoi(token);

	free(copy);

	return width;
}







/* Set up the window extent and visible area.
 *
 * On entry, width and height are the work area extent.  x and y give the X and Y posotions of the top left of the
 * window, or -1 to default place the window on that axis.  yoff is a Y offset to apply, to allow for raked openings.
 */

void set_initial_window_area (wimp_window *window, int width, int height, int x, int y, int yoff)
{
  int limit, clear_iconbar, lower_limit;

  /* Set the extent of the window. */

  window->extent.x0 = 0;
  window->extent.x1 = width;
  window->extent.y0 = -height;
  window->extent.y1 = 0;

  /* Read CMOS RAM to see if the icon-bar is to be kept clear. */

  clear_iconbar = ((osbyte2 (osbyte_READ_CMOS, osbyte_CONFIGURE_NO_OBSCURE_ICON_BAR, 0) &
                  osbyte_CONFIGURE_NO_OBSCURE_ICON_BAR_MASK) != 0);

  /* Set up the X position. */

  limit = general_mode_width ();
  width = (width < (limit * X_WINDOW_PERCENT_LIMIT / 100)) ? width : (limit * X_WINDOW_PERCENT_LIMIT / 100);

  if (x > -1)
  {
    window->visible.x0 = x;
    window->visible.x1 = x + width;

    if (window->visible.x1 >= limit)
    {
      window->visible.x1 = limit - 1;
      window->visible.x0 = window->visible.x1 - width;
    }
  }
  else
  {
    window->visible.x0 = (limit - width) / 2;
    window->visible.x1 = window->visible.x0 + width;
  }

  /* Set up the Y position. */

  limit = general_mode_height ();                                  /* The Y screen size. */
  lower_limit = (clear_iconbar) ? sf_ICONBAR_HEIGHT : 0;   /* The lower usable bound if clear-ibar is set. */

  /* Calculate the maximum visible height possible.  This is as a percentage of the total screen height.  If the
   * height comes out as too high for practical use, it is reduced to the height - icon-bar clearance, with top and
   * bottom window furniture removed too.
   */

  height = (height < (limit * Y_WINDOW_PERCENT_LIMIT / 100)) ? height : (limit * Y_WINDOW_PERCENT_LIMIT / 100);
  if (height + (2 * sf_WINDOW_GADGET_HEIGHT) > limit - lower_limit)
  {
    height = limit - lower_limit - (2 * sf_WINDOW_GADGET_HEIGHT);
  }

  if (y > -1)
  {
    window->visible.y1 = y - yoff - sf_WINDOW_GADGET_HEIGHT;
    window->visible.y0 = y - yoff - sf_WINDOW_GADGET_HEIGHT - height;

    if (window->visible.y0 < lower_limit)
    {
      window->visible.y0 = lower_limit;
      window->visible.y1 = lower_limit + height;
    }
  }
  else
  {
    window->visible.y1 = (limit * Y_WINDOW_PERCENT_ORIGIN / 100) - yoff;
    window->visible.y0 = window->visible.y1 - height;

    if (window->visible.y0 < lower_limit + sf_WINDOW_GADGET_HEIGHT)
    {
      window->visible.y0 = lower_limit + sf_WINDOW_GADGET_HEIGHT ;
      window->visible.y1 = lower_limit + sf_WINDOW_GADGET_HEIGHT + height;
    }
  }
}

