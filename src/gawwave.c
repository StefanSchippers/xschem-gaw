/*
 * gawwave.c - wave interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 *** \brief Allocates memory for a new VisibleWave object.
 */

VisibleWave *wave_new( WaveVar *var, DataFile *wdata )
{
   VisibleWave *vw;

   vw = app_new0(VisibleWave, 1);
   wave_construct( vw, var, wdata );
   app_class_overload_destroy( (AppClass *) vw, wave_destroy );
   return vw;
}

/** \brief Constructor for the VisibleWave object. */

void wave_construct( VisibleWave *vw, WaveVar *var, DataFile *wdata )
{
   app_class_construct( (AppClass *) vw );
   
   vw->var = var;
   vw->wdata = wdata;
   vw->logAble = wavevar_val_get_min(vw->var) > 0.0 ? 1 : 0;

   gm_create_vw_popmenu ( vw );
}

/** \brief Destructor for the VisibleWave object. */

void wave_destroy(void *vw)
{
   VisibleWave *this = (VisibleWave *) vw;

   if (vw == NULL) {
      return;
   }
   if ( this->wp ){
      wave_detach(this);
   }
   g_object_ref_sink ( this->buttonpopup );
   app_class_destroy( vw );
}

void wave_color_set (VisibleWave *vw)
{
   WavePanel *wp = vw->wp;
   UserData *ud = wp->ud;
   if ( vw->label ){
      char *colorspec = gdk_rgba_to_string(vw->color);
      ac_color_widget_style_color_set( vw->label, colorspec, ud->up->wavebgColor );
   }
}

void wave_attach(VisibleWave *vw, WavePanel *wp, GdkRGBA *color)
{
   vw->wp = wp;

   vw->colorn = pa_panel_next_color(wp);
   pa_panel_vw_list_add(wp, vw);

   /* add button to Y-label box */
   wave_vw_buttons_create(vw);
   pa_panel_lmswtable_setup(wp);

   vw->color = g_new0 (GdkRGBA, 1);
   if ( color ) {
      memcpy( vw->color, color, sizeof(GdkRGBA) );
   } else {
      ac_color_find_style_color( vw->label, NULL, vw->color);
   }
   wave_color_set (vw);
}

void wave_detach(VisibleWave *vw)
{
   WavePanel *wp = vw->wp;
   
   pa_panel_vw_list_remove(wp, vw);
   wave_vw_buttons_delete( vw );
   g_free( vw->color );
   vw->wp = NULL;
}

/*
 * delete button, label, and measurement
 */
void wave_vw_buttons_delete(VisibleWave *vw)
{
   int i;
   WavePanel *wp = vw->wp;
   UserData *ud = wp->ud;
   DnDSrcData *dd;
   
   for ( i = 0 ; i < AW_NY_MBTN ; i++ ){
      gtk_widget_destroy(vw->mbtn[i]->button); /* kills the child label also */
      ud->all_measure_buttons = g_list_remove(ud->all_measure_buttons,
					      vw->mbtn[i]);
      g_free(vw->mbtn[i]);
   }
   dd = g_object_get_data (G_OBJECT(vw->button), "DnDSrc");
   if ( dd ){
      g_free(dd);
   }
   gtk_widget_destroy(vw->button);  /* kills the child label also */
   /* recreate the table */
   pa_panel_lmswtable_setup( wp );
}

gboolean
wave_vw_button_toggled_cb (GtkWidget *widget, gpointer data)
{
   da_drawing_redraw( ((WavePanel *) data)->drawing);
   return FALSE;
}

gboolean
wave_vw_button_press_cb (GtkWidget *w,
			      GdkEventButton *event, gpointer data)
{
   VisibleWave *vw =  (VisibleWave *) data;

   if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
      msg_dbg("called");
      gtk_menu_popup (GTK_MENU (vw->buttonpopup), NULL, NULL,
		      NULL, data, 3, event->time);
      return TRUE;
   }
   return FALSE;
}

/*
 * create button, label, and measurement
 * widgets for a new VisibleWave just added to a WavePanel
 */

void wave_vw_buttons_create(VisibleWave *vw )
{
   WavePanel *wp = vw->wp;
   UserData *ud = wp->ud;
   WaveVar *var = vw->var;
   DataFile *wdata = vw->wdata;
   char lbuf[64];
   char temp[64];
   char *tips;
   char *labelname;
   int i;

   g_assert(wdata != NULL);

   labelname = wavevar_get_label(var, wdata->ftag);
   
   tips = _("Visible Wave button:\n\n"
            "Click button 1\n to select wave\n\n"
            "Press button 3\n for options menus\n\n"
	    "Drag button to move,\n copy or delete wave");
   vw->button = ap_create_toggle_button(NULL, "wavebutton", tips );
   g_object_ref (vw->button); /* increment ref to avoid destruction */
   g_signal_connect (vw->button, "toggled",
		     G_CALLBACK (wave_vw_button_toggled_cb), (gpointer) wp );
   g_signal_connect (vw->button, "button_press_event",
		     G_CALLBACK (wave_vw_button_press_cb),
		     (gpointer) vw );

   ad_dnd_setup_source(vw->button, wdata, var, vw);

   sprintf(temp, "wavecolor%d", vw->colorn);
   vw->label = ap_create_measure_label(vw->button, labelname, temp );
   g_free(labelname);
   
   /* create Y measurement buttons */
   for ( i = 0 ; i < AW_NY_MBTN ; i++ ) {
      GtkWidget *button;
      
      vw->mbtn[i] = g_new0(MeasureBtn, 1);
      vw->mbtn[i]->var = var;
      ud->all_measure_buttons = g_list_prepend(ud->all_measure_buttons,
					       vw->mbtn[i]);

      if ( i == 2 ){
	 sprintf(lbuf, "cursorD");
	 sprintf(temp, _("measure: cursor1 - cursor0"));
      } else {
	 sprintf(lbuf, "cursor%d", i);
	 sprintf(temp, _("measure: variable(cursor%d)"), i);
      }
      button = ap_create_measure_button( NULL, "measurebutton", temp );
      g_object_ref (button); /* increment ref to avoid destruction */
      
      vw->mbtn[i]->label = ap_create_measure_label(button, "0.0", lbuf ); 
//      gtk_widget_set_usize(button, 60, -1);
     
      vw->mbtn[i]->button = button;
      vw->mbtn[i]->csp = ud->cursors[i];
      ap_mbtn_update(vw->mbtn[i], ud);
   }
}

