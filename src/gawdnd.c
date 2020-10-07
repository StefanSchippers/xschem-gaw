/*
 * gawdnd.c - Drag and Drop functions
 * 
 * include LICENSE
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>
// #include <stdlib.h>
#include <math.h>

#include <gaw.h>
#include <fileutil.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


#define GAW_PRIVATE_DND_MAGIC 0xf00bbaad

enum {
  TARGET_STRING,
  TARGET_DVAR,
  TARGET_ROOTWIN
};

static GtkTargetEntry target_table[] = {
  { "x-gaw/dvar", GTK_TARGET_SAME_APP, TARGET_DVAR },
  { "STRING",     0, TARGET_STRING },
  { "text/plain", 0, TARGET_STRING },
  { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = sizeof(target_table) / sizeof(target_table[0]);


void
ad_set_drag_dest(GtkWidget *widget, UserData *ud, WavePanel *wp, int action)
{
   DnDDestData *destdata = g_new0(DnDDestData, 1);

   ud->destdata_list = g_list_append(ud->destdata_list, destdata);

   destdata->ud = ud;
   destdata->wp = wp;
   destdata->action = action;
   
   gtk_drag_dest_set ( widget,
		      GTK_DEST_DEFAULT_ALL,
		      target_table, n_targets - 1, /* no rootwin */
		      GDK_ACTION_COPY | GDK_ACTION_MOVE);
   g_signal_connect (widget, "drag_data_received",
		     G_CALLBACK (ad_dnd_target_event), (gpointer) destdata);
}

/*
 * drawing area dnd target drag_data_received handler
 *   Warning : key shift + dnd is action move by default.
 *   I set it to action copy,  so  dnd is action move.
 */
void 
ad_dnd_target_event (GtkWidget *widget, GdkDragContext *context,
		     gint  x, gint y,
		     GtkSelectionData   *sel_data,
		     guint info, guint time, gpointer d)
{
   gboolean ret = TRUE;
   DnDDestData *destdata = (DnDDestData *) d;
   DnDSrcData *dd;
   UserData *ud = destdata->ud;
   WavePanel *wp;
   gchar *s;
   gchar *name;
   
   guint length = gtk_selection_data_get_length (sel_data);
   
   msg_dbg( "info %d length %d", info, length );
   if ( info == TARGET_DVAR ) {
      gint dndMode = 1;
      dd = (DnDSrcData *) gtk_selection_data_get_data (sel_data);
      VisibleWave *vw = (VisibleWave *) dd->data ;
      
      if (length == sizeof(DnDSrcData) &&
	  dd->magic == GAW_PRIVATE_DND_MAGIC) {
	 msg_dbg("  received var = 0x%lx", (unsigned long) dd->var);

         GdkDragAction action = gdk_drag_context_get_actions(context);
	 if ( action == GDK_ACTION_MOVE ) {
	    msg_dbg("  action move received - Will copy ");
	    dndMode = 0;
	 }
	 switch ( destdata->action ) {
	  case DND_EVENT_BOX :
	  case DND_ADD_BUT :
	     /* dest is drawing area to be created */
	     wp = ap_panel_add_line( ud, NULL, 0);
	     msg_dbg(" adding visible wave to a newpanel");
	     ap_panel_add_var(wp, dd->var, vw, NULL);
	    break;
	  case DND_PANEL :
	    /* dest is drawing area */
	    msg_dbg(" adding visible wave to panel");
	    ap_panel_add_var( destdata->wp, dd->var, vw, NULL );
	    break;
	  case DND_DELETE_BUT :
	    /* dest is toolbar delete wave button */
	    /* will delete if needed */
	    break;
	 }
	 if ( vw && dndMode ) { /* need delete */
	    msg_dbg(" deleting visible wave");
	    wave_destroy(vw);
	    ap_all_redraw(ud);
	 }
      } else {
	 msg_warning(_("bad magic number 0x%x"), dd->magic);
      }
   } else  if ( info == TARGET_STRING ) {
      name = (gchar *) gtk_selection_data_get_data (sel_data);
      if ( ( s = strchr(name, '\r')) ) {
	 *s = 0;
      }
      msg_dbg( "  received '%s'", name );
      /* open the file as a data file */
	   
      gchar *str = "file://";
      if ( ( s = strstr(name, str)) ) {
	 s += strlen(str);
	 af_list_files_free(&ud->listFiles);
         if (file_exists (s)) {
            ud->listFiles = g_slist_append( ud->listFiles, app_strdup( s ) );
         }
	 app_free(ud->format);
	 ud->format = NULL;
	 af_load_wave_file (ud);
      }
    } else {
       ret = FALSE;
    }
  
  gtk_drag_finish (context, ret, FALSE, time);
}



/*
 * drag and drop source callback
 */
void
ad_source_drag_data_get (GtkWidget *widget, GdkDragContext *context,
			 GtkSelectionData   *selection_data,
			 guint               info,
			 guint               time,
			 gpointer            data)
{
   DnDSrcData *dd = (DnDSrcData *) data;
   WaveVar *var = dd->var;
   GdkAtom target = gtk_selection_data_get_target (selection_data);
   
  if (info == TARGET_DVAR ) {
     dd->magic = GAW_PRIVATE_DND_MAGIC;
     if ( prog_debug ){
	msg_dbg("var = 0x%lx", (unsigned long) var);
     }
     gtk_selection_data_set (selection_data,
			     target,
			     8, (gpointer) dd, sizeof(DnDSrcData)); /* 8 = number of bits in a unit */
  } else if (info == TARGET_ROOTWIN) {
     msg_warning (_("I was dropped on the root window"));
  } else if (info == TARGET_STRING ) {
     msg_warning (_("source type string"));
  }
}

void  
ad_source_drag_data_delete  (GtkWidget *widget, GdkDragContext *context,
			     gpointer data)
{
  msg_dbg ("Delete the data!");
}
  
/*
 * initialize drag and drop in the list window
 */

void
ad_dnd_setup_source(GtkWidget *button, DataFile *wdata, WaveVar *var, VisibleWave *vw)
{
   /* Drag site */
   gtk_drag_source_set (button, GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
		       target_table, n_targets, 
		       GDK_ACTION_COPY | GDK_ACTION_MOVE);
   gtk_drag_source_set_icon_pixbuf (button, wdata->drag_icon);

   DnDSrcData *dd = g_new0( DnDSrcData, 1);
   dd->var = var;
   dd->data = (gpointer) vw;
   
   g_object_set_data (G_OBJECT(button), "DnDSrc", dd);
   g_signal_connect (button, "drag_data_get",
		     G_CALLBACK (ad_source_drag_data_get), (gpointer) dd);
   g_signal_connect (button, "drag_data_delete",
		     G_CALLBACK (ad_source_drag_data_delete), (gpointer) dd );

}
