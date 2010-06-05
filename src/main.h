/* CashBook - main.h
 *
 * (c) Stephen Fryatt, 2003
 */

#ifndef _ACCOUNTS_MAIN
#define _ACCOUNTS_MAIN

/* ------------------------------------------------------------------------------------------------------------------ */

int main (int argc, char *argv[]);
int poll_loop (void);
void mouse_click_handler (wimp_pointer *);
void key_press_handler (wimp_key *key);
void menu_selection_handler (wimp_selection *);
void scroll_request_handler (wimp_scroll *);
void user_message_handler (wimp_message *);
void bounced_message_handler (wimp_message *);

#endif
