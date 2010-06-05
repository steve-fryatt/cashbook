/* CashBook - ihelp.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_IHELP
#define _ACCOUNTS_IHELP

/* ==================================================================================================================
 * Static constants
 */

#define IHELP_LENGTH 236

/* ==================================================================================================================
 * Data structures
 */

typedef struct ihelp_window
{
  wimp_w window;
  char   name[13];
  char   modifier[13];
  void   (*pointer_location) (char *, wimp_w, wimp_i, os_coord, wimp_mouse_state);

  struct ihelp_window *next;
}
ihelp_window;

/* ==================================================================================================================
 * Function prototypes.
 */

/* Adding and removing windows */

void add_ihelp_window (wimp_w window, char* name, void (*decode) (char *, wimp_w, wimp_i, os_coord, wimp_mouse_state));
void remove_ihelp_window (wimp_w window);

/* Setting a window's modifier. */

void ihelp_set_modifier (wimp_w window, char *modifier);

/* Finding windows */

ihelp_window *find_ihelp_window (wimp_w window);

/* Token generation */

char *find_ihelp (char *buffer, wimp_w window, wimp_i icon, os_coord pos, wimp_mouse_state buttons);

int send_reply_help_request (wimp_message *message);

#endif
