/* $Id: callbacks.c,v 1.3 2002/06/22 16:57:37 seb Exp $ */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

extern void quit_cb(void);

void
on_file1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}




void
on_quit2_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  quit_cb();
}

