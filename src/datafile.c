/*
 * datafile.c - datafile interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include <gaw.h>
#include <duprintf.h>
#include <gawpixmaps.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 *** \brief Allocates memory for a new DataFile object.
 */

DataFile *datafile_new( void *ud, char *name )
{
   DataFile *wdata;

   wdata =  app_new0(DataFile, 1);
   datafile_construct( wdata, ud, name );
   app_class_overload_destroy( (AppClass *) wdata, datafile_destroy );
   return wdata;
}

/** \brief Constructor for the DataFile object. */

void datafile_construct( DataFile *wdata, void *ud, char *name )
{
   static int next_tag = 0;
   
   app_class_construct( (AppClass *) wdata );
   wdata->ud = ud;
   wdata->wt = wavetable_new( (AppClass *) wdata, name ) ;
   wdata->ftag = next_tag++;
   aw_vl_menu_item_add( wdata);
}

/** \brief Destructor for the DataFile object. */

void datafile_destroy(void *wdata)
{
   DataFile *this = (DataFile *) wdata;

   if (wdata == NULL) {
      return;
   }
   if ( this->vlmenu ){
      g_object_ref_sink (this->vlmenu);
   }
   /* remove per-file GUI stuff */
   datafile_list_win_destroy(this);

   if ( this->drag_icon ) {
      g_object_unref (this->drag_icon);
   }
   if ( this->lbpopmenu ){
      g_object_ref_sink (this->lbpopmenu);
   }
   
   app_free(this->filename);
   app_free(this->format);
   
   wavetable_destroy(this->wt);
   
   app_class_destroy( wdata );
}

void datafile_dup_filename(DataFile *wdata, char *filename)
{
   app_free(wdata->filename);
   wdata->filename = app_strdup(filename);
}

void datafile_dup_format(DataFile *wdata, char *format)
{
   app_free(wdata->format);
   wdata->format = app_strdup(format);
}

void datafile_set_file(DataFile *wdata,  char *filename, char *format)
{
   datafile_dup_filename(wdata, filename);
   datafile_dup_format(wdata, format);
   wdata->method = DATAFILE_FILE;
}

void datafile_set_sound(DataFile *wdata,  SoundParams *sparams)
{
   wdata->sparams = sparams;
   wdata->method = DATAFILE_SOUND;
}

int datafile_load(DataFile *wdata)
{
   int ret;
   
   if ( wdata->method == DATAFILE_SOUND ) {
      sound_new( wdata->sparams, wdata->wt);
   } else  if ( wdata->method == DATAFILE_FILE ) {
      SpiceStream *ss = spicestream_new( wdata->filename, wdata->format, wdata->wt);
      if ( ss->status ) {
	 return  ss->status ;
      }
   }
   if ( ( ret = wavetable_fill_tables( wdata->wt, wdata->filename)) < 0 ){
      return ret;
   }
   if ( ! wdata->wlist_win ) {
      datafile_create_list_win (wdata);
   }
   return 0;
}

int datafile_reload(DataFile *wdata)
{
   int ret;

   UserData *ud = wdata->ud; /* stefan */
   GawIoData *gawio = (GawIoData *)ud->gawio; /* stefan */
   wdata->old_wt = wdata->wt;   /* need this for clean up */
   wdata->wt = wavetable_new( (void *) wdata, wavetable_get_tblname( wdata->wt) ) ;
   gawio->wds = wavetable_get_cur_dataset(wdata->wt); /* stefan */
   if ( ( ret = datafile_load(wdata)) < 0 ){
      return ret;
   }
   return 0;
}

/*
 * Called for all button clicked on wavelist button.
 * If the button is clicked, add variable to the "current" wavepanel immediately.
 */
static gint
datafile_list_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   WaveVar *var = (WaveVar *) data;
   DataFile *wdata = (DataFile *) wavetable_get_datafile((WaveTable *) var->wvtable);
//   UserData *ud = ( UserData *) wdata->ud;

   if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
      g_object_set_data (G_OBJECT(wdata->lbpopmenu), "ListButtonPopup-action", data);
      gtk_menu_popup (GTK_MENU (wdata->lbpopmenu), NULL, NULL,
                      NULL, data, 3, event->time);
      /*
       * TRUE to stop other handlers from being invoked for the event.
       *  FALSE to propagate the event further.
       */
      return TRUE;
   }
   return FALSE;
}

void 
datafile_list_button_cb(GtkWidget *widget, gpointer data)
{
   WaveVar *var = (WaveVar *) data;
   
   ap_panel_add_var( NULL, var, NULL, NULL);
}

/*
 * Add a button for each variable in the file to the win_wlist box for it.
 * Arrange for the buttons to be drag-and-drop sources for placing the
 * variables into wavepanels.
 */

void
datafile_add_list_button(gpointer d, gpointer p)
{
   WaveVar *var = (WaveVar *) d;
   DataFile *wdata = (DataFile *) p;
   UserData *ud = ( UserData *) wdata->ud;
   GtkWidget *button;
   GtkWidget *label;
   char *labelname;

   labelname = wavevar_get_label(var, -1);
                
   button = gtk_button_new_with_label (labelname);
   gtk_widget_set_name(button, "listButton" );
   label = GTK_WIDGET (gtk_container_get_children (GTK_CONTAINER (button))->data);
   ac_color_widget_style_color_set( label, ud->up->lboxfgColor, ud->up->lboxbgColor );
   gtk_box_pack_start (GTK_BOX (wdata->wlist_box), button, FALSE, FALSE, 0);
   gtk_widget_show (button);
   gtk_widget_set_tooltip_text ( GTK_WIDGET(button),
		      _("WaveVar Variable.\nDrag-and-Drop to a WavePanel.\n"
			"Or select a Panel and left click this button.\n"
			"Right click gives some other commands.\n") );

   ad_dnd_setup_source(button, wdata, var, NULL);

   g_signal_connect (button, "clicked",
                     G_CALLBACK (datafile_list_button_cb), (gpointer) var );
   g_signal_connect (button, "button-press-event",
                     G_CALLBACK (datafile_list_button_press_cb ), 
                     (gpointer) var);
   app_free(labelname);
}

/*
 * callback to remove button in list_win
 */
void datafile_button_remove(GtkWidget *widget, gpointer data)
{
   DnDSrcData *dd ;
   
   dd = g_object_get_data (G_OBJECT(widget), "DnDSrc");
   if ( dd ){
      g_free( dd);
   }
   gtk_widget_destroy(widget);
}

void datafile_list_win_empty(DataFile *wdata)
{
   if (wdata->wlist_win && gtk_widget_get_visible (GTK_WIDGET(wdata->wlist_win)) ) {
      gtk_container_foreach(GTK_CONTAINER(wdata->wlist_box),
			    (GtkCallback) datafile_button_remove, NULL);
   }
}

void datafile_list_win_fill(DataFile *wdata)
{
   if (wdata->wlist_win) {
      wavetable_foreach_wavevar(wdata->wt, datafile_add_list_button,
				(gpointer) wdata);
   }
}

void datafile_list_win_destroy(DataFile *wdata)
{
   aw_vl_menu_item_remove(wdata);
   if (wdata->wlist_win && gtk_widget_get_visible (GTK_WIDGET(wdata->wlist_win)) ) {
      gtk_widget_destroy(wdata->wlist_win);
   }
   wdata->wlist_win = NULL;
}
/*
 * Show the variable-list window for a waveform data file.
 * If the window already exists, simply raise it to the top.
 */
void
datafile_create_list_win (DataFile *wdata)
{
   UserData *ud = wdata->ud;
   GtkWidget *box1;
   GtkWidget *scrolled_window;
   GtkWidget *label;
   char *bufp;
   int x, y;

   if ( ! wdata) {
      msg_warning(_("wdata is NULL"));
      return;
   }
   if ( wdata->wlist_win) {
      GdkWindow *window = gtk_widget_get_window (wdata->wlist_win);
      gdk_window_raise(window);
      return;
   }
   
   wdata->wlist_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_widget_set_name(wdata->wlist_win, "data_list_win");
   bufp = app_strdup_printf( "Gaw: %.64s", wdata->filename);
   gtk_window_set_title(GTK_WINDOW(wdata->wlist_win), bufp);
   app_free(bufp);
   gtk_widget_set_size_request(wdata->wlist_win, 150, 300);
   gtk_window_set_transient_for( GTK_WINDOW(wdata->wlist_win), 
				 GTK_WINDOW(ud->window));   
   /* this set set wlist_win = NULL at destroy event */
   g_signal_connect (wdata->wlist_win, "destroy",
		     G_CALLBACK (gtk_widget_destroyed),
		     &(wdata->wlist_win) );


   gtk_window_get_position(GTK_WINDOW(ud->window), &x, &y);

   /* a vertical box */
   box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   gtk_widget_set_name(box1, "data_list_box");
   gtk_container_add(GTK_CONTAINER(wdata->wlist_win), box1);
   gtk_widget_show(box1);
   wdata->wlist_vbox = box1;
   
   /* create the menu */
   gm_create_vl_menu ( wdata );
   gtk_box_pack_start (GTK_BOX (box1), wdata->vlmenu, FALSE, FALSE, 0);
      
   /* a label */
   bufp = app_strdup_printf( "%d: %.64s", wdata->ftag, wdata->wt->tblname );
   label = gtk_label_new(bufp);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
   gtk_widget_show(label);
   app_free(bufp);
   gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);

   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				   GTK_POLICY_AUTOMATIC, 
				   GTK_POLICY_ALWAYS);
   GtkWidget *vscrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(scrolled_window));
   gtk_widget_set_can_focus (vscrollbar, FALSE);
   gtk_box_pack_start(GTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);
   gtk_widget_show (scrolled_window);

   wdata->wlist_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
   gtk_container_set_border_width (GTK_CONTAINER (wdata->wlist_box), 10);
//   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
//					 wdata->wlist_box);
   gtk_container_add(GTK_CONTAINER(scrolled_window), wdata->wlist_box);

   gtk_container_set_focus_vadjustment( GTK_CONTAINER (wdata->wlist_box),
					gtk_scrolled_window_get_vadjustment(
					GTK_SCROLLED_WINDOW(scrolled_window)));
   gtk_widget_show (wdata->wlist_box);
   
   wdata->drag_icon = gdk_pixbuf_new_from_xpm_data (wave_drag_ok_xpm);
   
   datafile_list_win_fill(wdata);
   gtk_widget_show(wdata->wlist_win);
}

void
datafile_recreate_list_win (DataFile *wdata)
{
   datafile_list_win_empty(wdata);
   datafile_list_win_fill(wdata);
}


static void
datafile_similar_var_add (gpointer d, gpointer p)
{
   WaveVar *curvar = (WaveVar *) d;
   WaveVar *var = (WaveVar *) p;
   
   if ( app_strcmp(curvar->varName, var->varName) == 0 ) { 
      ap_panel_add_var( NULL, curvar, NULL, NULL);
   }
}


void
datafile_similar_vars_add (DataFile *wdata, WaveVar *var)
{
   UserData *ud = ( UserData *) wdata->ud;
   
   if ( ud->selected_panel  == NULL) {
      msg_info (aw_panel_not_selected_msg);
      return ;
   }
   wavetable_foreach_wavevar(wdata->wt, datafile_similar_var_add, (gpointer) var);
}

