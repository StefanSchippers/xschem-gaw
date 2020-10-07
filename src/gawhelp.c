/*
 * gawhelp.c -
 * This file is part of gaw Gtk Analog Wave viewer
 *
 * include LICENSE
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
// #include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

   
void ah_show_page (UserData *ud, char *url )
{
   char *envcmd = getenv("GAW_HELPCMD");
   char *cmd = ud->up->helpCmd;
   char *fmt;
   char *cmdline;

   if ( envcmd ) {
      cmd = envcmd;
   }
   if ( app_strstr(cmd, "%s") ) {
      fmt = app_strdup(cmd);
   } else {
      fmt = g_strconcat ( cmd, " %s", NULL);
   }
   cmdline = g_strdup_printf( fmt, url);
   msg_dbg("cmdline '%s'\n", cmdline);
   system( cmdline);
   
   g_free(fmt);
   g_free(cmdline);
}
   
void ah_show_userguide_dialog (UserData *ud)
{
   char *envurl = getenv( "GAW_USERGUIDE");
   char *url = ud->up->userGuide;
   if ( envurl) {
      url = envurl;
   }
   ah_show_page (ud, url );
}

void ah_show_website_dialog (UserData *ud)
{
   char *envurl = getenv( "GAW_WEBSITE");
   char *url = ud->up->webSite;
   if ( envurl) {
      url = envurl;
   }
   ah_show_page (ud, url );
}
