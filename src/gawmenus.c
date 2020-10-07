/*
 * gawmenus.c - gaw menus functions
 * 
 * include LICENSE
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <math.h>

#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif

void
activate_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  g_print ("Action %s activated\n", g_action_get_name (G_ACTION (action)));
}

void
aw_factory_settings_gaction (GSimpleAction *action, GVariant *param, gpointer user_data)
{
   UserData *ud = (UserData *) user_data;
   gint response;
   char *message = _("You are going to reset your rc parameters to factory values\n"
		     "You will need to restart Gaw.\n"
		     "If this is what you want, click OK\n");

   response = aw_dialog_show ( AW_MSG_T_SHOW_CANCEL | MSG_T_WARNING, message );
   if (response == GTK_RESPONSE_ACCEPT) {
      up_init_defaults(ud->up);
      up_rc_rewrite(ud->up);

      /*
       * before doing restart, we need to cleanly destroy all objects !!!
       */
//      ud->restart = 1;
      aw_do_exit (NULL, ud);
   }
}

/*
 * save configuration file
 */
void aw_save_config_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ud->up->npanels = g_list_length( ud->panelList);

   if ( ud->listFiles ){
      app_dup_str( &ud->up->lastDataFile, (char *) g_slist_nth_data ( ud->listFiles, 0 ) );
   }
   aw_do_save_config (ud );
}


/*
 * clear the default input file name and save config
 */
void aw_default_file_name_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   app_dup_str( &ud->up->lastDataFile, NULL );
   prog_debug = 0;

   aw_do_save_config (ud );
}

static void
toggle_allow_resize_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->allowResize =  ! g_variant_get_boolean ( state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->allowResize));
   g_variant_unref (state);

//   msg_dbg ("Action allowResize %d", ud->up->allowResize );
}

static void
toggle_bar_style_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GtkToolbar *tb = GTK_TOOLBAR(ud->toolBar);
   GtkToolbarStyle newstyle = GTK_TOOLBAR_BOTH ;

   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->toolBarStyle =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->toolBarStyle));
   g_variant_unref (state);
   
   if ( ud->up->toolBarStyle ){
      newstyle = GTK_TOOLBAR_BOTH;
   } else {
      newstyle = GTK_TOOLBAR_ICONS;
   }
   gtk_toolbar_set_style(tb, newstyle );
}

static void
activate_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   const gchar *name = g_action_get_name (G_ACTION (action));

//   msg_dbg ("Action \"%s\" activated", name );
   if ( strcmp( name, "Quit") == 0 ){
      ud->restart = 0;
      aw_do_exit( NULL, ud);
   }
}


void
aw_redraw_all_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ap_all_redraw(ud);
}

static void
aw_showXlabel_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->showXLabels =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->showXLabels));
   g_variant_unref (state);
   
//   msg_dbg ("Action showXlabel %d", ud->up->showXLabels );
   if ( ud->xlabel_box == NULL ){ 
      return; /* widget not yet created */
   }
   ap_xlabel_box_show(ud);
}

static void
aw_showYlabel_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->showYLabels =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->showYLabels));
   g_variant_unref (state);

//   msg_dbg ("Action showYlabel %d", ud->up->showYLabels );
   /*  this happen when setting default value from rc */
   if ( ud->xlabel_box == NULL ){ 
      return; /* widget not yet created  */
   }

   g_list_foreach(ud->panelList, (GFunc) pa_ylabel_box_show, NULL );
}
static void
aw_showMoreYlabel_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->showMoreYLabels =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->showMoreYLabels));
   g_variant_unref (state);
//   msg_dbg ("Action showMoreYlabel %d", ud->up->showMoreYLabels );

   /*  this happen when setting default value from rc */
   if ( ud->xlabel_box == NULL ){ 
      return; /* widget not yet created  */
   }
   g_list_foreach(ud->panelList, (GFunc) pa_panel_label_meas_box_update, NULL );
   ap_all_redraw(ud);
}

static void
aw_scientific_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->scientific =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->scientific));
   g_variant_unref (state);

   if ( ud->xlabel_box == NULL ){ 
      return; /* widget not yet created */
   }
   msg_dbg ("ud->up->scientific = %d", ud->up->scientific);
   g_list_foreach(ud->panelList, (GFunc) pa_panel_label_size, NULL );
   ap_all_redraw(ud);
   ap_mbtn_update_all(ud);
   ap_xmeasure_button_draw(ud);
}

static void
aw_logx_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->setLogX =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->setLogX));
   g_variant_unref (state);

   if ( ud->xlabel_box == NULL ){ 
      return; /* widget not yet created */
   }
   msg_dbg ("userPrefs->setLogX = %d", ud->up->setLogX);
   GawLabels *lbx = ud->xLabels;
   al_label_set_logAxis( lbx, ud->up->setLogX);
   az_cmd_zoom_absolute(ud, lbx->start_val, lbx->end_val );

   ap_all_redraw(ud);
}

static void
aw_ydiff_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->showYDiff =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->showYDiff));
   g_variant_unref (state);

   if ( ud->xlabel_box == NULL ){ 
      return; /* widget not yet created */
   }
   ap_mbtn_update_all(ud);
}


static void aw_logy_panel_set (WavePanel *wp, int active )
{
   if ( active && ! al_label_get_logAble(wp->yLabels) ) {
      msg_info ( _("Logarithmic Y axe not available for this panel (null or negative values in series)"));
      return;
   }
   al_label_set_logAxis(wp->yLabels, active );
   pa_panel_full_redraw(wp);
}

/*
 * preference menu logY action
 */
static void
aw_logy_selected_gaction (GSimpleAction *action, GVariant *param, gpointer user_data  )
{
   UserData *ud = (UserData *) user_data;
   WavePanel *wp = ud->selected_panel;
   static int toggled;
      
   GVariant *state = g_action_get_state (G_ACTION (action));
   toggled =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (toggled));
   g_variant_unref (state);

   msg_dbg ("Action aw_logy_selected_action %d", toggled );

   if ( ! wp) {
      if ( ! toggled) { /* no other way to modify the toggle state : stupid ! */ 
	 return;
      }
      msg_info (aw_panel_not_selected_msg);
      return;
   }
   gm_update_toggle_state(wp->wpgroup, "pPLogY", toggled);
   aw_logy_panel_set (wp, toggled );
}

/*
 * panel action
 */
static void
aw_pop_logy_gaction (GSimpleAction *action, GVariant *param, gpointer user_data  )
{
   WavePanel *wp = (WavePanel *) user_data;
//   msg_dbg("aw_pop_logy_action wp = 0x%x", wp );
   if ( wp ) {
      GVariant *state = g_action_get_state (G_ACTION (action));
      int active =  ! g_variant_get_boolean ( state);
      g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (active));
      g_variant_unref (state);

      aw_logy_panel_set (wp, active );
      if ( wp->ud->selected_panel == wp ) {
	 gm_update_toggle_state(wp->ud->group, "LogYPanel", active);
      }
   }
}


static void
aw_grid_panels_set (UserData *ud )
{
   GList *list = ud->panelList;
   while (list) {
      GList *next = list->next;

      WavePanel *wp = (WavePanel *) list->data;  
      wp->showGrid =  ud->up->showGrid;
      gm_update_toggle_state(wp->wpgroup, "pShowGrid", wp->showGrid );
      pa_panel_full_redraw(wp);
      list = next;
   }
}

static void
aw_show_grid_gaction (GSimpleAction *action, GVariant *param, gpointer user_data  )
{
   UserData *ud = (UserData *) user_data;
   GVariant *state = g_action_get_state (G_ACTION (action));
   ud->up->showGrid =  ! g_variant_get_boolean (state);
   g_action_change_state (G_ACTION (action),
                g_variant_new_boolean (ud->up->showGrid));
   g_variant_unref (state);

   if ( ud->xlabel_box == NULL ){ 
      return; /* widget not yet created */
   }
   aw_grid_panels_set ( ud );
}

void aw_show_grid_cmd (UserData *ud, int on )
{
   ud->up->showGrid = on;
   aw_grid_panels_set ( ud );
}

static void aw_pop_show_grid_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( wp ) {
      GVariant *state = g_action_get_state (G_ACTION (action));
      wp->showGrid =  ! g_variant_get_boolean (state);
      g_action_change_state (G_ACTION (action),
                             g_variant_new_boolean (wp->showGrid));
      g_variant_unref (state);

//      msg_dbg("showGrid = %d", wp->showGrid );
      pa_panel_full_redraw(wp);
   }
}


static void aw_pop_add_panel_above_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( wp ) {
      ap_panel_add_line( wp->ud, wp, 0);
   }
}


static void aw_pop_add_panel_below_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( wp ) {
      ap_panel_add_line( wp->ud, wp, 1);
   }
}

static void
aw_add_panel_gaction (GSimpleAction *action, GVariant *param, gpointer user_data)
{
   UserData *ud = (UserData *) user_data;
   ap_panel_add_line(ud, NULL, 0);
}

static void
aw_remove_panel_gaction (GSimpleAction *action, GVariant *param, gpointer user_data)
{
   UserData *ud = (UserData *) user_data;
   ap_panel_remove_line(ud, NULL);
}

static void aw_pop_remove_this_panel_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( wp ) {
      ap_panel_remove_line(wp->ud, wp);
   }
}


static void
aw_pop_delete_all_panel_wave_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( wp ) {
      pa_panel_vw_delete(wp);
   }
}

static void aw_delete_all_wave_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data)
{
   UserData *ud = (UserData *) user_data;
   ap_remove_all_wave_selected(ud);
}

static void aw_pop_delete_all_wave_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   ap_remove_all_wave_selected(wp->ud);
}

static void aw_pop_reload_all_files_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   g_list_foreach(wp->ud->wdata_list, (GFunc) ap_reload_wave_file, NULL);
}


static void
aw_script_exec_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
//   UserData *ud = (UserData *) user_data;
   msg_info( "aw_script_exec_action has to be implemented...");
}

static void
aw_show_about_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   gaw_show_about_dialog (GTK_WIDGET (ud->window)); 
}

static void
aw_show_userguide_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ah_show_userguide_dialog (ud); 
}

static void
aw_show_website_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ah_show_website_dialog (ud); 
}

void aw_sound_dialog_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   if ( ! ud->sndData ) {
      ud->sndData = as_sound_new(ud);
      if ( ! ud->sndData ) {
         msg_info(_("Sound interface is not compiled in !\n"
                   "Install alsa-lib development package.\n"
                   "Run ./configure; make; make install"));
	 return ;
      }
   }
   as_sound_win_create(ud->sndData);
}

/* stefan set global ( no static) */
void aw_reload_all_files_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   g_list_foreach(ud->wdata_list, (GFunc) ap_reload_wave_file, NULL);
}

/****************************************************************/
/*
 * menu stuff for main window and main menu
 */

static const gchar main_builder[] =
"<?xml version='1.0'?>"
"<interface>"
"  <object class='GtkGrid' id='grid'>"
"    <child>"
"      <object class='GtkToolbar' id='toolbar'>"
"        <property name='orientation'>horizontal</property>"
"        <property name='halign'>fill</property>"
"        <property name='hexpand'>True</property>"
"        <property name='visible'>True</property>"
"        <style>"
"          <class name='primary-toolbar'/>"
"        </style>"
"        <child>"
"          <object class='GtkToolButton' id='TAddPanel'>"
"            <property name='label' translatable='yes'>Add Panel</property>"
"            <property name='icon-name'>gtk-add</property>"
"            <property name='action-name'>gaw.TAddPanel</property>"
"            <property name='tooltip-text' translatable='yes'>Add a new panel</property>"
"            <property name='visible'>True</property>"
"            <property name='sensitive'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='TDeletePanel'>"
"            <property name='label' translatable='yes'>Del Panel</property>"
"            <property name='icon-name'>edit-clear</property>"
"            <property name='action-name'>gaw.TDeletePanel</property>"
"            <property name='tooltip-text' translatable='yes'>Remove selected or last panel</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkSeparatorToolItem' id='sep1'/>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='TDeleteWave'>"
"            <property name='label' translatable='yes'>Del Wave</property>"
"            <property name='icon-name'>edit-cut</property>"
"            <property name='action-name'>gaw.TDeleteWave</property>"
"            <property name='tooltip-text' translatable='yes'>Delete all selected Waves</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='treloadall'>"
"            <property name='label' translatable='yes'>Reload All</property>"
"            <property name='icon-name'>reload</property>"
"            <property name='action-name'>gaw.TReloadAll</property>"
"            <property name='tooltip-text' translatable='yes'>Reread all waveform data files</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='ttextaction'>"
"            <property name='label' translatable='yes'>Text</property>"
"            <property name='icon-name'>text-editor</property>"
"            <property name='action-name'>gaw.TTextAction</property>"
"            <property name='tooltip-text' translatable='yes'>Open text tool settings</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkSeparatorToolItem' id='sep2'/>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='tzoomin'>"
"            <property name='label' translatable='yes'>Z In</property>"
"            <property name='icon-name'>zoom-in</property>"
"            <property name='action-name'>gaw.TZoomIn</property>"
"            <property name='tooltip-text' translatable='yes'>Zoom X In</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='TZoomOut'>"
"            <property name='label' translatable='yes'>Z Out</property>"
"            <property name='icon-name'>zoom-out</property>"
"            <property name='action-name'>gaw.TZoomOut</property>"
"            <property name='tooltip-text' translatable='yes'>Zoom X Out</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='tzoomcursor0'>"
"            <property name='label' translatable='yes'>Z C0</property>"
"            <property name='icon-name'>gtk-justify-center</property>"
"            <property name='action-name'>gaw.TZoomCursor0</property>"
"            <property name='tooltip-text' translatable='yes'>Zoom in centered on cursor 0</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='tzoomcursors'>"
"            <property name='label' translatable='yes'>Z Cursors</property>"
"            <property name='icon-name'>zoom-best-fit</property>"
"            <property name='action-name'>gaw.TZoomCursors</property>"
"            <property name='tooltip-text' translatable='yes'>Display space between 2 cursors</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='GtkToolButton' id='TZoomXFull'>"
"            <property name='label' translatable='yes'>X unZ</property>"
"            <property name='icon-name'>zoom-original</property>"
"            <property name='action-name'>gaw.TZoomXFull</property>"
"            <property name='tooltip-text' translatable='yes'>Unzoom X all</property>"
"            <property name='visible'>True</property>"
"          </object>"
"        </child>"
"      </object>"
"      <packing>"
"        <property name='left-attach'>0</property>"
"        <property name='top-attach'>1</property>"
"        <property name='width'>2</property>"
"        <property name='height'>1</property>"
"      </packing>"
"    </child>"
"    <child>"
"      <object class='GtkBox' id='meas_hbox'>"
"        <property name='can_focus'>False</property>"
"        <property name='orientation'>horizontal</property>"
"        <property name='spacing'>0</property>"
"      </object>"
"      <packing>"
"        <property name='left-attach'>0</property>"
"        <property name='top-attach'>2</property>"
"        <property name='width'>2</property>"
"        <property name='height'>1</property>"
"      </packing>"
"    </child>"
"    <child>"
"      <object class='GtkScrolledWindow' id='sw'>"
"        <property name='shadow-type'>in</property>"
"        <property name='visible'>True</property>"
"        <property name='halign'>fill</property>"
"        <property name='valign'>fill</property>"
"        <property name='hexpand'>True</property>"
"        <property name='vexpand'>True</property>"
"      </object>"
"      <packing>"
"        <property name='left-attach'>0</property>"
"        <property name='top-attach'>3</property>"
"        <property name='width'>2</property>"
"        <property name='height'>1</property>"
"      </packing>"
"    </child>"
"    <child>"
"      <object class='GtkEventBox' id='xlabel_ev_box'>"
"        <property name='halign'>fill</property>"
"        <property name='hexpand'>True</property>"
"        <property name='visible'>True</property>"
"      </object>"
"      <packing>"
"        <property name='left-attach'>0</property>"
"        <property name='top-attach'>4</property>"
"        <property name='width'>2</property>"
"        <property name='height'>1</property>"
"      </packing>"
"    </child>"
"    <child>"
"      <object class='GtkStatusbar' id='statusbar'>"
"        <property name='halign'>fill</property>"
"        <property name='hexpand'>True</property>"
"        <property name='visible'>True</property>"
"      </object>"
"      <packing>"
"        <property name='left-attach'>0</property>"
"        <property name='top-attach'>5</property>"
"        <property name='width'>1</property>"
"        <property name='height'>1</property>"
"      </packing>"
"    </child>"
"  </object>"
"</interface>";

static const gchar gaw_menubar[] =
"<?xml version='1.0'?>"
"<interface>"
"  <menu id='menubar'>"
"    <submenu>"
"      <attribute name='label' translatable='yes'>_File</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>_Open...</attribute>"
"          <attribute name='action'>gaw.Open</attribute>"
"          <attribute name='accel'>&lt;control&gt;O</attribute>"
"          <attribute name='icon'>document-open</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>_Capture...</attribute>"
"          <attribute name='action'>gaw.Capture</attribute>"
"          <attribute name='accel'>&lt;control&gt;C</attribute>"
"          <attribute name='icon'>document-print</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Export waves...</attribute>"
"          <attribute name='action'>gaw.ExportImg</attribute>"
"          <attribute name='icon'>media-playlist-shuffle</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Export to image...</attribute>"
"          <attribute name='action'>gaw.ExportImg</attribute>"
"          <attribute name='icon'>media-playlist-repeat</attribute>"
"        </item>"
"      </section>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>_Save conf</attribute>"
"          <attribute name='action'>gaw.SaveConf</attribute>"
"          <attribute name='icon'>filesave</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Clear Default</attribute>"
"          <attribute name='action'>gaw.ClearDef</attribute>"
"          <attribute name='icon'>edit-clear</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Factory Settings</attribute>"
"          <attribute name='action'>gaw.DefConf</attribute>"
"          <attribute name='icon'>preferences-other</attribute>"
"        </item>"
"      </section>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>_Quit</attribute>"
"          <attribute name='action'>gaw.Quit</attribute>"
"          <attribute name='accel'>&lt;control&gt;Q</attribute>"
"          <attribute name='icon'>application-exit</attribute>"
"        </item>"
"      </section>"
"    </submenu>"
""
"    <submenu>"
"      <attribute name='label' translatable='yes'>_View</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>Add Panel at end</attribute>"
"          <attribute name='action'>gaw.AddPanel</attribute>"
"          <attribute name='icon'>gtk-add</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>_Redraw All</attribute>"
"          <attribute name='action'>gaw.RedrawAll</attribute>"
"          <attribute name='icon'>reload</attribute>"
"          <attribute name='accel'>&lt;control&gt;A</attribute>"
"        </item>"
"      </section>"
"      <section>"
"        <submenu id='VarList'>"
"          <attribute name='label' translatable='yes'>Variable List</attribute>"
"        </submenu>"
"      </section>"
"    </submenu>"
"    "
"    <submenu>"
"      <attribute name='label' translatable='yes'>_Zoom</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom centered C0</attribute>"
"          <attribute name='action'>gaw.ZoomCursor0</attribute>"
"          <attribute name='icon'>gtk-justify-center</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom Cursors</attribute>"
"          <attribute name='action'>gaw.ZoomCursors</attribute>"
"          <attribute name='icon'>gtk-zoom-fit</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom Dialog...</attribute>"
"          <attribute name='action'>gaw.ZoomDialog</attribute>"
"          <attribute name='icon'>package_settings</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom X...</attribute>"
"          <attribute name='action'>gaw.ZoomX</attribute>"
"          <attribute name='icon'>gtk-zoom-fit</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom Y...</attribute>"
"          <attribute name='action'>gaw.ZoomY</attribute>"
"          <attribute name='icon'>gtk-zoom-fit</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom X+Y...</attribute>"
"          <attribute name='action'>gaw.ZoomXYarea</attribute>"
"          <attribute name='icon'>gtk-zoom-fit</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>X unZoom all</attribute>"
"          <attribute name='action'>gaw.ZoomXFull</attribute>"
"          <attribute name='icon'>zoom-out</attribute>"
"          <attribute name='accel'>&lt;control&gt;Z</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom Y Full</attribute>"
"          <attribute name='action'>gaw.ZoomYFull</attribute>"
"          <attribute name='icon'>zoom-out</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom X In</attribute>"
"          <attribute name='action'>gaw.ZoomIn</attribute>"
"          <attribute name='icon'>zoom-in</attribute>"
"          <attribute name='accel'>Z</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Zoom X Out</attribute>"
"          <attribute name='action'>gaw.ZoomOut</attribute>"
"          <attribute name='icon'>zoom-out</attribute>"
"          <attribute name='accel'>&lt;shift&gt;Z</attribute>"
"        </item>"
"      </section>"
"    </submenu>"
"    "
"    <submenu>"
"      <attribute name='label' translatable='yes'>_Cursors</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>Set Cursor 0</attribute>"
"          <attribute name='action'>gaw.SetCursor0</attribute>"
"          <attribute name='icon'>emblem-default</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Set Cursor 1</attribute>"
"          <attribute name='action'>gaw.SetCursor1</attribute>"
"          <attribute name='icon'>emblem-default</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Set all Cursors</attribute>"
"          <attribute name='action'>gaw.SetCursors</attribute>"
"          <attribute name='icon'>emblem-default</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Clear Cursor 0</attribute>"
"          <attribute name='action'>gaw.ClearCursor0</attribute>"
"          <attribute name='icon'>edit-clear</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Clear Cursor 1</attribute>"
"          <attribute name='action'>gaw.ClearCursor1</attribute>"
"          <attribute name='icon'>edit-clear</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Clear all Cursors</attribute>"
"          <attribute name='action'>gaw.ClearCursors</attribute>"
"          <attribute name='icon'>edit-clear</attribute>"
"        </item>"
"      </section>"
"    </submenu>"
"    "
"    <submenu>"
"      <attribute name='label' translatable='yes'>_Preferences</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>ToolBar Style Icon+text</attribute>"
"          <attribute name='action'>gaw.BarStyle</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>show X labels</attribute>"
"          <attribute name='action'>gaw.showXlabels</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>show Y labels</attribute>"
"          <attribute name='action'>gaw.showYlabels</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>show more Y labels</attribute>"
"          <attribute name='action'>gaw.moreYlabels</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>log X scale</attribute>"
"          <attribute name='action'>gaw.LogX</attribute>"
"          <attribute name='accel'>&lt;control&gt;X</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>log Y selected panel</attribute>"
"          <attribute name='action'>gaw.LogYPanel</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>show Y diff measure</attribute>"
"          <attribute name='action'>gaw.Ydiff</attribute>"
"          <attribute name='accel'>&lt;control&gt;Y</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Show Grid</attribute>"
"          <attribute name='action'>gaw.ShowGrid</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Scientific conversion</attribute>"
"          <attribute name='action'>gaw.Scientific</attribute>"
"        </item>"
"      </section>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>Change panel Colors...</attribute>"
"          <attribute name='action'>gaw.PanelColor</attribute>"
"          <attribute name='icon'>preferences-color-symbolic</attribute>"
"        </item>"
"      </section>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>Allow Resize</attribute>"
"          <attribute name='action'>gaw.AllowResize</attribute>"
"        </item>"
"      </section>"
"      <section>"
"        <submenu id='algomenu'>"
"          <attribute name='label' translatable='yes'>Algorithm List</attribute>"
"        </submenu>"
"      </section>"
"      <section>"
"        <submenu id='xconvmenu'>"
"          <attribute name='label' translatable='yes'>X axis Conversion List</attribute>"
"        </submenu>"
"      </section>"
"    </submenu>"
"    "
"    <submenu>"
"      <attribute name='label' translatable='yes'>_Tools</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>Open Text tool...</attribute>"
"          <attribute name='action'>gaw.TextAction</attribute>"
"          <attribute name='accel'>&lt;control&gt;T</attribute>"
"          <attribute name='icon'>text-editor</attribute>"
"        </item>"
"      </section>"
"    </submenu>"
"    "
"     <submenu>"
"      <attribute name='label' translatable='yes'>_Help</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>_About</attribute>"
"          <attribute name='action'>gaw.About</attribute>"
"          <attribute name='icon'>gtk-about</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>User Guide</attribute>"
"          <attribute name='action'>gaw.UserGuide</attribute>"
"          <attribute name='accel'>F1</attribute>"
"          <attribute name='icon'>help-browser</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Gaw Web Site</attribute>"
"          <attribute name='action'>gaw.GawWebSite</attribute>"
"          <attribute name='icon'>user-home</attribute>"
"        </item>"
"      </section>"
"    </submenu>"
"  </menu>"
"</interface>";


static TooltipInfo menubarTip[] = {
   { "Open", N_("_Open..."), N_("Open a file"), NULL  },
   { "Capture", N_("_Capture..."), N_("Input data from the sound card"), NULL  },
   { "ExportDis", N_("Export waves..."), N_("Export displayed waves to a data file"), NULL  },
   { "ExportImg", N_("Export to image..."), N_("Export all panels to an image file"), NULL  },
   { "SaveConf", N_("_Save conf"), N_("Save configuration file"), NULL  },
   { "ClearDef", N_("Clear Default"), N_("Clear default input file name and Save configuration file"), NULL  },
   { "DefConf", N_("Factory Settings"), N_("Reset configuration file to its default"), NULL  },
   { "Quit", N_("_Quit"), N_("Quit"), NULL  },
   
   { "AddPanel", N_("Add Panel at end"), N_("Add a new panel"), NULL  },
   { "RedrawAll", N_("_Redraw All"), N_("Redraw All panels"), NULL  },
   { "RedrawAll", N_("Variable List"), N_("List of all loaded files\nreCreate variable List window"), NULL  },

   { "ZoomCursor0", N_("Zoom centered C0"), N_("Zoom in centered on cursor 0"), NULL  },
   { "ZoomCursors", N_("Zoom Cursors"), N_("Display space between 2 cursors") , NULL  },
   { "ZoomDialog", N_("Zoom Dialog..."), N_("Zoom Dialog menu"), NULL  },
   { "ZoomX", N_("Zoom X..."), N_("Select X range with button 1"), NULL  },
   { "ZoomY", N_("Zoom Y..."), N_("Select Y range with button 1"), NULL  },
   { "ZoomXYarea", N_("Zoom X+Y..."), N_("Select X+Y range with button 1"), NULL  },
   { "ZoomXFull", N_("X unZoom all"), N_("Unzoom X all"), NULL  },
   { "ZoomYFull", N_("Zoom Y Full"), N_("Unzoom Y  all"), NULL  },
   { "ZoomIn", N_("Zoom X In"), N_("Zoom X In"), NULL  },
   { "ZoomOut", N_("Zoom X Out"), N_("Zoom X Out"), NULL  },

   { "SetCursor0", N_("Set Cursor 0"), N_("Display cursor 0"), NULL  },
   { "SetCursor1", N_("Set Cursor 1"), N_("Display cursor 1"), NULL  },
   { "SetCursors", N_("Set all Cursors"), N_("Display all cursors"), NULL  },
   { "ClearCursor0", N_("Clear Cursor 0"), N_("Clear cursor 0 if displayed"), NULL  },
   { "ClearCursor1", N_("Clear Cursor 1"), N_("Clear cursor 1 if displayed"), NULL  },
   { "ClearCursors", N_("Clear all Cursors"), N_("Clear all cursors if displayed"), NULL  },
   
   { "BarStyle", N_("ToolBar Style Icon+text"), N_("Toggle toolbar style"), NULL  },
   { "showXlabels", N_("show X labels"), N_("show labels in X"), NULL  },
   { "showYlabels", N_("show Y labels"), N_("show labels in Y"), NULL  },
   { "moreYlabels", N_("show more Y labels"), N_("different way to show labels in Y"), NULL  },
   { "LogX", N_("log X scale"), N_("Log scale in X"), NULL  },
   { "LogYPanel", N_("log Y selected panel"), N_("Set Log scale in Y in selected panel"), NULL  },
   { "Ydiff", N_("show Y diff measure"), N_("show Y diff measure buttons"), NULL  },
   { "ShowGrid", N_("Show Grid"), N_("Show Grid in panels"), NULL  },
   { "Scientific", N_("Scientific conversion"), N_("Use Scientific conversion mode"), NULL  },
   { "PanelColor", N_("Change panel Colors..."), N_("Change colors used in panel drawing area"), NULL  },
   { "AllowResize", N_("Allow Resize"), N_("Allow Resize the main window"), NULL  },

   { "TextAction", N_("Open Text tool..."), N_("Open text tool settings"), NULL  },
   { "AlgoMenuAction", N_("Algorithm List") , N_("Select an Algorithm from the List"), NULL  },
   { "XConvertAction", N_("X Convert List") , N_("Select a method to covert X values"), NULL  },
   { "About", N_("_About"), N_("About"), NULL  },
   { "GawWebSite", N_("Gaw Web Site"), N_("Gaw Web Site"), NULL  },
   { "UserGuide", N_("User Guide"), N_("Gaw User Guide"), NULL  },

   { NULL, NULL, NULL, NULL  },
};



static GActionEntry entries[] = {
   { "ZoomYFull", az_zoom_y_full_gaction, NULL, NULL, NULL  },
   { "ZoomOut", az_zoom_out_gaction, NULL, NULL, NULL  },
   { "ClearCursor0", cu_cursor_clear_cursor0_gaction, NULL, NULL, NULL  },
   { "GawWebSite", aw_show_website_gaction, NULL, NULL, NULL  },
   { "ZoomXFull", az_zoom_x_full_gaction, NULL, NULL, NULL  },
   { "ZoomX", az_zoom_x_gaction, NULL, NULL, NULL  },
   { "TZoomXFull", az_zoom_x_full_gaction, NULL, NULL, NULL  },
   { "ClearCursors", cu_cursor_clear_cursors_gaction, NULL, NULL, NULL  },
   { "TZoomIn", az_zoom_in_gaction, NULL, NULL, NULL  },
   { "UserGuide", aw_show_userguide_gaction, NULL, NULL, NULL  },
   { "ZoomXYarea", az_zoom_xy_area_gaction, NULL, NULL, NULL  },
   { "ClearDef", aw_default_file_name_gaction, NULL, NULL, NULL  },
   { "TReloadAll", aw_reload_all_files_gaction, NULL, NULL, NULL  },
   { "ClearCursor1", cu_cursor_clear_cursor1_gaction, NULL, NULL, NULL  },
   { "Capture", aw_sound_dialog_gaction, NULL, NULL, NULL  },
   { "SaveConf", aw_save_config_gaction, NULL, NULL, NULL  },
   { "TZoomCursors", az_zoom_cursors_gaction, NULL, NULL, NULL  },
   { "ExportImg", im_export_panels_img_gaction, NULL, NULL, NULL  },
   { "PanelColor", ac_color_panel_colors_gaction, NULL, NULL, NULL  },
   { "ZoomDialog", az_zoom_dialog_gaction, NULL, NULL, NULL  },
   { "AddPanel", aw_add_panel_gaction, NULL, NULL, NULL  },
   { "TAddPanel", aw_add_panel_gaction, NULL, NULL, NULL  },
   { "Execute", aw_script_exec_gaction, NULL, NULL, NULL  },
   { "Plot", activate_gaction, NULL, NULL, NULL  },
   { "TZoomOut", az_zoom_out_gaction, NULL, NULL, NULL  },
   { "SetCursor1", cu_cursor_set_cursor1_gaction, NULL, NULL, NULL  },
   { "Open", af_open_file_gaction, NULL, NULL, NULL  },
   { "TextAction", gawtext_new_gaction, NULL, NULL, NULL  },
   { "DeleteWave", aw_delete_all_wave_gaction, NULL, NULL, NULL  },
   { "TDeletePanel", aw_remove_panel_gaction, NULL, NULL, NULL  },
   { "ExportDis", af_export_displayed_gaction, NULL, NULL, NULL  },
   { "DefConf", aw_factory_settings_gaction, NULL, NULL, NULL  },
   { "SetCursor0", cu_cursor_set_cursor0_gaction, NULL, NULL, NULL  },
   { "ZoomIn", az_zoom_in_gaction, NULL, NULL, NULL  },
   { "TZoomCursor0", az_zoom_cursor0_centered_gaction, NULL, NULL, NULL  },
   { "ZoomCursor0", az_zoom_cursor0_centered_gaction, NULL, NULL, NULL  },
   { "DeletePanel", aw_remove_panel_gaction, NULL, NULL, NULL  },
   { "ZoomCursors", az_zoom_cursors_gaction, NULL, NULL, NULL  },
   { "TTextAction", gawtext_new_gaction, NULL, NULL, NULL  },
   { "ZoomY", az_zoom_y_gaction, NULL, NULL, NULL  },
   { "SetCursors", cu_cursor_set_cursors_gaction, NULL, NULL, NULL  },
   { "RedrawAll", aw_redraw_all_gaction, NULL, NULL, NULL  },
   { "Quit", activate_gaction, NULL, NULL, NULL  },
   { "TDeleteWave", aw_delete_all_wave_gaction, NULL, NULL, NULL  },
   { "About", aw_show_about_gaction, NULL, NULL, NULL  },
};

static GActionEntry toggle_entries[] = {
   { "BarStyle", toggle_bar_style_gaction, NULL, "true", NULL  },
   { "AllowResize", toggle_allow_resize_gaction, NULL, "true", NULL  },
   { "showXlabels", aw_showXlabel_gaction, NULL, "true", NULL  },
   { "moreYlabels", aw_showMoreYlabel_gaction, NULL, "true", NULL  },
   { "showYlabels", aw_showYlabel_gaction, NULL, "true", NULL  },
   { "ShowGrid", aw_show_grid_gaction, NULL, "false", NULL  },
   { "Scientific", aw_scientific_gaction, NULL, "false", NULL  },
   { "Ydiff", aw_ydiff_gaction, NULL, "false", NULL  },
   { "LogYPanel", aw_logy_selected_gaction, NULL, "true", NULL  },
   { "LogX", aw_logx_gaction, NULL, "false", NULL  },
};


GtkWidget *gm_action_lookup_widget(char *action, TooltipInfo *tbl)
{
   while (tbl->name ){
      if (strcmp(tbl->name, action) == 0) {
         return  tbl->widget;
      }
      tbl++;
   }
   msg_dbg("Can't find widget for action %s", action );
   return NULL;
}

TooltipInfo *gm_tooltip_lookup(char *label, TooltipInfo *tbl)
{
   while (tbl->label ){
      if (strcmp(tbl->label, label) == 0) {
         return  tbl;
      }
      tbl++;
   }
   return NULL;
}

TooltipInfo *gm_widget_set_tooltip(GtkWidget *w, gpointer data )
{
   const char *label;

   if ( ! data ) {
      return NULL;
   }
   label = gtk_label_get_label (GTK_LABEL (w));
   if ( label ) {
      TooltipInfo *tbl = gm_tooltip_lookup((char *) label, data);
      if ( tbl && tbl->tooltip ){
         gtk_widget_set_tooltip_markup (w,  tbl->tooltip );
         msg_dbg( "tooltip set for label %s", label);
         return tbl;
      } else {
//         fprintf( stderr, "tooltip not found for label %s\n", label);
      }
   }
   return NULL;
}

static void gm_check_menu_item(GtkWidget *item, gpointer data)
{
   GtkWidget *child;
   GList *children;
//   const char * label;

   child = gtk_bin_get_child (GTK_BIN (item));
   if (child == NULL) {
      msg_dbg( "check_menu_item child is NULL 0x%ld",
               (unsigned long) item );
      if ( GTK_IS_LABEL (item) ){
         gm_widget_set_tooltip(item, data );
         msg_dbg( "check_menu_item item is label %ld",
                  (unsigned long) item );
         return;
      }
   }
   
   if (GTK_IS_LABEL (child)) {
      msg_dbg( "check_menu_item child IS_LABEL 0x%ld",
               (unsigned long) item );
      gm_widget_set_tooltip(child, data );
      return;
   }
   if ( ! GTK_IS_CONTAINER (child)) {
      //       fprintf( stderr, "aw_check_menu_item child is NOT CONTAINER\n");
       return;
    }

   children = gtk_container_get_children (GTK_CONTAINER (child));

   while (children) {
      if (GTK_IS_LABEL (children->data)) {
         msg_dbg( "check_menu_item CONTAINER 0x%ld",
               (unsigned long) item );
         TooltipInfo *tbl = gm_widget_set_tooltip(children->data, data );
         if ( tbl ){
            tbl->widget = item;
         }
      }

      children = g_list_delete_link (children, children);
   }
}

void gm_menu_item_update_cb(GtkWidget *item, gpointer data)
{
   GtkWidget *submenu;

   if ( GTK_IS_MENU_SHELL(item) ) {
      msg_dbg( "menu_item_update_cb MENU_SHELL");
      gm_update_menu(item, data);
   } else if ( GTK_IS_MENU_ITEM(item) ) {
      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM(item));
      if ( submenu ){
         msg_dbg( "menu_item_update_cb SUB MENU");
         GtkWidget *w = gtk_menu_get_attach_widget(GTK_MENU(submenu));
         if (w) {
            gm_check_menu_item(w, data);
         }
         gm_update_menu(submenu, data);
      } else {
         gm_check_menu_item(item, data);
      }
   }
}

void gm_update_menu(GtkWidget *menu, TooltipInfo *tip_table)
{
   gtk_container_foreach (GTK_CONTAINER(menu), gm_menu_item_update_cb,
                          (gpointer) tip_table );
}

void 
gm_update_toggle_state(GSimpleActionGroup *group, const gchar *action_name,
                       gboolean is_active)
{
   GAction *action = g_action_map_lookup_action ((GActionMap  *) group, action_name);
   if (action) {
      g_action_change_state (G_ACTION (action),
                             g_variant_new_boolean (is_active));
   } else {
      msg_fatal (_("Can't not find action '%s'"), action_name);
   }
}

void gm_create_layout (UserData *ud)
{
   GtkBuilder *builder;
   GtkWidget *grid;
   GtkWidget *toolitem;
   GMenuModel *menumodel;
   GError *error = NULL;

   ud->group = g_simple_action_group_new ();
   g_action_map_add_action_entries (G_ACTION_MAP (ud->group), entries,
                                    G_N_ELEMENTS (entries), ud );
   g_action_map_add_action_entries (G_ACTION_MAP (ud->group), toggle_entries,
                                   G_N_ELEMENTS (toggle_entries), ud );
   gtk_widget_insert_action_group(GTK_WIDGET (ud->window), "gaw",
                                  G_ACTION_GROUP (ud->group));

   builder = gtk_builder_new ();
   gtk_builder_add_from_string (builder, main_builder, -1,&error );
   g_assert_no_error (error);
   if (error != NULL) {
      g_error ("Failed to create widgets: %s\n", error->message);
   }
   
   grid = (GtkWidget *) gtk_builder_get_object (builder, "grid");
   ud->toolBar = (GtkWidget *) gtk_builder_get_object (builder, "toolbar");
   ud->meas_hbox = (GtkWidget *) gtk_builder_get_object (builder, "meas_hbox");
   ud->panel_scrolled = (GtkWidget *) gtk_builder_get_object (builder, "sw");
   ud->xlabel_ev_box = (GtkWidget *) gtk_builder_get_object (builder, "xlabel_ev_box");
   ud->statusbar = (GtkWidget *) gtk_builder_get_object (builder, "statusbar");
   ud->globalTable = g_object_ref (grid);

   /* add target dnd to toolbar delete button */
   toolitem = (GtkWidget *) gtk_builder_get_object (builder, "TDeleteWave");
   ad_set_drag_dest(toolitem, ud, NULL, DND_DELETE_BUT);
   toolitem = (GtkWidget *) gtk_builder_get_object (builder, "TAddPanel");
   ad_set_drag_dest(toolitem, ud, NULL, DND_ADD_BUT );
   
   g_object_unref (builder);

   builder = gtk_builder_new ();
   gtk_builder_add_from_string (builder, gaw_menubar, -1, &error);
   if (error != NULL) {
      g_error ("Failed to create menu model 'gaw_menubar': %s\n", error->message);
   }

   menumodel = g_object_ref (gtk_builder_get_object (builder,"menubar" ));
   ud->menuBar = gtk_menu_bar_new_from_model(menumodel);
   gtk_widget_show(ud->menuBar);
   g_assert (ud->menuBar);
   gtk_grid_attach(GTK_GRID(grid), ud->menuBar, 
              /* left,    top,  width,   height    */
                   0,      0,      2,        1  );   

   ud->algomodel = g_object_ref (gtk_builder_get_object (builder,"algomenu" ));
   aw_algo_menu_create( ud );

   ud->xconvmodel = g_object_ref (gtk_builder_get_object (builder,"xconvmenu" ));
   aw_xconv_menu_create( ud );
   
   ud->vlmmodel = g_object_ref (gtk_builder_get_object (builder,"VarList" ));

   gm_update_menu(ud->menuBar, menubarTip);
   /* sensitive widget */
   ud->LogX = gm_action_lookup_widget("LogX", menubarTip );
   ud->LogYPanel = gm_action_lookup_widget("LogYPanel", menubarTip );
   ud->moreYlabels = gm_action_lookup_widget("moreYlabels", menubarTip );

   g_object_unref (builder);
}

/****************************************************************/
/*
 * menu stuff for panel popup menu
 */

static const gchar panelPopup[] =
"<?xml version='1.0'?>"
"<interface>"
"  <menu id='panelpopup'>"
"    <section>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom Cursors</attribute>"
"        <attribute name='action'>gaw.pZoomCursors</attribute>"
"        <attribute name='icon'>gtk-zoom-fit</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom X...</attribute>"
"        <attribute name='action'>gaw.pZoomX</attribute>"
"        <attribute name='icon'>gtk-zoom-fit</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom Y...</attribute>"
"        <attribute name='action'>gaw.pZoomY</attribute>"
"        <attribute name='icon'>gtk-zoom-fit</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>X unZoom</attribute>"
"        <attribute name='action'>gaw.pZoomXFull</attribute>"
"        <attribute name='icon'>zoom-out</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom Y Full</attribute>"
"        <attribute name='action'>gaw.pZoomYFull</attribute>"
"        <attribute name='icon'>zoom-out</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom X+Y...</attribute>"
"        <attribute name='action'>gaw.pZoomXYarea</attribute>"
"        <attribute name='icon'>gtk-zoom-fit</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom Dialog...</attribute>"
"        <attribute name='action'>gaw.pZoomDialog</attribute>"
"        <attribute name='icon'>package_settings</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom X In</attribute>"
"        <attribute name='action'>gaw.pZoomIn</attribute>"
"        <attribute name='icon'>zoom-in</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Zoom X Out</attribute>"
"        <attribute name='action'>gaw.pZoomOut</attribute>"
"        <attribute name='icon'>zoom-out</attribute>"
"      </item>"
"    </section>"
"    <section>"
"      <item>"
"        <attribute name='label' translatable='yes'>Add Panel Above</attribute>"
"        <attribute name='action'>gaw.pAddPanelAbove</attribute>"
"        <attribute name='icon'>gtk-add</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Add Panel Below</attribute>"
"        <attribute name='action'>gaw.pAddPanelBelow</attribute>"
"        <attribute name='icon'>gtk-add</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Remove this Panel</attribute>"
"        <attribute name='action'>gaw.pDelThisPanel</attribute>"
"        <attribute name='icon'>edit-clear</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Export to Image...</attribute>"
"        <attribute name='action'>gaw.pExportPng</attribute>"
"        <attribute name='icon'>document-save-as</attribute>"
"      </item>"
"    </section>"
"    <section>"
"      <item>"
"        <attribute name='label' translatable='yes'>Reload All</attribute>"
"        <attribute name='action'>gaw.pReloadAll</attribute>"
"        <attribute name='icon'>reload</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Delete Wave</attribute>"
"        <attribute name='action'>gaw.pDeleteWave</attribute>"
"        <attribute name='icon'>gtk-delete</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Delete Waves</attribute>"
"        <attribute name='action'>gaw.pDeleteWaves</attribute>"
"        <attribute name='icon'>gtk-delete</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>log Y scale</attribute>"
"        <attribute name='action'>gaw.pPLogY</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Show Grid</attribute>"
"        <attribute name='action'>gaw.pShowGrid</attribute>"
"        <attribute name='icon'>view-grid-symbolic</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Grid Change Color...</attribute>"
"        <attribute name='action'>gaw.pGridColor</attribute>"
"        <attribute name='icon'>preferences-color-symbolic</attribute>"
"      </item>"
"    </section>"
"    <section>"
"      <item>"
"        <attribute name='label' translatable='yes'>Delete all texts</attribute>"
"        <attribute name='action'>gaw.pDeleteTexts</attribute>"
"        <attribute name='icon'>gtk-delete</attribute>"
"      </item>"
"    </section>"
"  </menu>"
"  <menu id='paneltextpopup'>"
"    <section>"
"      <item>"
"        <attribute name='label' translatable='yes'>Edit this text ...</attribute>"
"        <attribute name='action'>gaw.pEditThisText</attribute>"
"        <attribute name='icon'>text-editor</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Delete this text</attribute>"
"        <attribute name='action'>gaw.pDelThisText</attribute>"
"        <attribute name='icon'>gtk-delete</attribute>"
"      </item>"
"    </section>"
"  </menu>"
"</interface>";




static GActionEntry pop_entries[] = {
   { "pAddPanelBelow", aw_pop_add_panel_below_gaction, NULL, NULL, NULL  },
   { "pDelThisPanel", aw_pop_remove_this_panel_gaction, NULL, NULL, NULL  },
   { "pZoomYFull", az_pop_zoom_y_full_gaction, NULL, NULL, NULL  },
   { "pDelThisText", gawtext_pop_delete_gaction, NULL, NULL, NULL  },
   { "pExportPng", im_pop_export_to_img_gaction, NULL, NULL, NULL  },
   { "pDeleteWaves", aw_pop_delete_all_panel_wave_gaction, NULL, NULL, NULL  },
   { "pZoomX", az_pop_zoom_x_gaction, NULL, NULL, NULL  },
   { "pZoomCursors", az_pop_zoom_cursors_gaction, NULL, NULL, NULL  },
   { "pEditThisText", gawtext_pop_edit_gaction, NULL, NULL, NULL  },
   { "pDeleteTexts", gawtext_pop_delete_all_text_gaction, NULL, NULL, NULL  },
   { "pZoomXYarea", az_pop_zoom_xy_area_gaction, NULL, NULL, NULL  },
   { "pZoomY", az_pop_zoom_y_gaction, NULL, NULL, NULL  },
   { "pReloadAll", aw_pop_reload_all_files_gaction, NULL, NULL, NULL  },
   { "pGridColor", ac_pop_color_grid_gaction, NULL, NULL, NULL  },
   { "pZoomOut", az_pop_zoom_out_gaction, NULL, NULL, NULL  },
   { "pDeleteWave", aw_pop_delete_all_wave_gaction, NULL, NULL, NULL  },
   { "pZoomXFull", az_pop_zoom_x_full_gaction, NULL, NULL, NULL  },
   { "pZoomDialog", az_pop_zoom_dialog_gaction, NULL, NULL, NULL  },
   { "pZoomIn", az_pop_zoom_in_gaction, NULL, NULL, NULL  },
   { "pAddPanelAbove", aw_pop_add_panel_above_gaction, NULL, NULL, NULL  },
};

static GActionEntry pop_toggle_entries[] = {
   { "pPLogY", aw_pop_logy_gaction, NULL, "false", NULL  },
   { "pShowGrid", aw_pop_show_grid_gaction, NULL, "false", NULL  },
};


static TooltipInfo panelPopupTip[] = {
   { "pZoomCursors", N_("Zoom Cursors"), N_("Display space between 2 cursors"), NULL  },
   { "pZoomX", N_("Zoom X..."), N_("Select X range with button 1"), NULL  },
   { "pZoomY", N_("Zoom Y..."), N_("Select Y range with button 1"), NULL  },
   { "pZoomXFull", N_("X unZoom"), N_("Unzoom X all"), NULL  },
   { "pZoomYFull", N_("Zoom Y Full"), N_("Unzoom Y  all"), NULL  },
   { "pZoomXYarea", N_("Zoom X+Y..."), N_("Select X+Y range with button 1"), NULL  },
   { "pZoomDialog", N_("Zoom Dialog..."), N_("Zoom Dialog menu"), NULL  },
   { "pZoomIn", N_("Zoom X In"), N_("Zoom X In"), NULL  },
   { "pZoomOut", N_("Zoom X Out"), N_("Zoom X Out"), NULL  },
   { "pAddPanelAbove", N_("Add Panel Above"), N_("Add a new panel above this one"), NULL  },
   { "pAddPanelBelow", N_("Add Panel Below"), N_("Add a new panel below this one"), NULL  },
   { "pDelThisPanel", N_("Remove this Panel"), N_("Remove this panel"), NULL  },
   { "pExportPng", N_("Export to Image..."), N_("Export this wave to Image file"), NULL  },
   { "pReloadAll", N_("Reload All"), N_("Reread all waveform data files"), NULL  },
   { "pDeleteWave", N_("Delete Wave"), N_("Delete all selected Waves"), NULL  },
   { "pDeleteWaves", N_("Delete Waves"), N_("Delete all Waves in this panel"), NULL  },
   { "pPLogY", N_("log Y scale"), N_("Set Log scale in Y for this panel"), NULL  },
   { "pShowGrid", N_("Show Grid"), N_("Show Grid in this panel"), NULL  },
   { "pGridColor", N_("Grid Change Color..."), N_("Change the grid color in this panel"), NULL  },
   { "pDeleteTexts", N_("Delete all texts"), N_("Delete all texts in this panel"), NULL  },
   { NULL, NULL, NULL, NULL  },
};
static TooltipInfo textPopupTip[] = {
   { "pEditThisText", N_("Edit this text ..."), N_("Edit this text"), NULL  },
   { "pDelThisText", N_("Delete this text"), N_("Delete this text"), NULL  },
   { NULL, NULL, NULL, NULL  },
};


void gm_create_panel_popmenu (WavePanel *wp )
{
   GtkBuilder *builder;
   GMenuModel *menumodel;
   GError *error = NULL;

   wp->wpgroup = g_simple_action_group_new ();
   g_action_map_add_action_entries (G_ACTION_MAP (wp->wpgroup), pop_entries,
                                    G_N_ELEMENTS (pop_entries), wp );
   g_action_map_add_action_entries (G_ACTION_MAP (wp->wpgroup), pop_toggle_entries,
                                   G_N_ELEMENTS (pop_toggle_entries), wp );

   builder = gtk_builder_new ();
   gtk_builder_add_from_string (builder, panelPopup, -1, &error );
   g_assert_no_error (error);
   if (error != NULL) {
      g_error ("Failed to create widgets: %s\n", error->message);
   }
   
   menumodel = (GMenuModel *) gtk_builder_get_object (builder, "panelpopup");
   wp->popmenu = (GtkWidget *) gtk_menu_new_from_model (menumodel);

   menumodel = (GMenuModel *) gtk_builder_get_object (builder, "paneltextpopup");
   wp->textpopmenu = gtk_menu_new_from_model (menumodel);

   gm_update_menu(wp->popmenu, panelPopupTip);
   wp->pPLogY = gm_action_lookup_widget("pPLogY", panelPopupTip );
   gm_update_menu(wp->textpopmenu, textPopupTip);

   gtk_widget_insert_action_group(GTK_WIDGET (wp->popmenu), "gaw",
                                  G_ACTION_GROUP (wp->wpgroup));
   gtk_widget_insert_action_group(GTK_WIDGET (wp->textpopmenu), "gaw",
                                  G_ACTION_GROUP (wp->wpgroup));

   g_object_unref (builder);
}

/****************************************************************/
/*
 * menu stuff for variable list window
 */

static const gchar vl_menubar[] =
"<?xml version='1.0'?>"
"<interface>"
"  <menu id='vl_menubar'>"
"    <submenu>"
"      <attribute name='label' translatable='yes'>_File</attribute>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>Reload this file</attribute>"
"          <attribute name='action'>gaw.Reload</attribute>"
"          <attribute name='icon'>reload</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Unload this file</attribute>"
"          <attribute name='action'>gaw.Unload</attribute>"
"          <attribute name='icon'>edit-clear</attribute>"
"        </item>"
"        <item>"
"          <attribute name='label' translatable='yes'>Export data...</attribute>"
"          <attribute name='action'>gaw.Export</attribute>"
"          <attribute name='icon'>gtk-save-as</attribute>"
"        </item>"
"      </section>"
"      <section>"
"        <item>"
"          <attribute name='label' translatable='yes'>_Close</attribute>"
"          <attribute name='action'>gaw.Close</attribute>"
"          <attribute name='icon'>window-close</attribute>"
"        </item>"
"      </section>"
"    </submenu>"
"  </menu>"
"  <menu id='listbuttonpopup'>"
"    <section>"
"      <item>"
"        <attribute name='label' translatable='yes'>Add this to selected</attribute>"
"        <attribute name='action'>gaw.AddThisSelected</attribute>"
"        <attribute name='icon'>gtk-add</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Add all similar to selected</attribute>"
"        <attribute name='action'>gaw.AddAllSelected</attribute>"
"        <attribute name='icon'>gtk-add</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Delete all similar</attribute>"
"        <attribute name='action'>gaw.DelAllSimilar</attribute>"
"        <attribute name='icon'>gtk-delete</attribute>"
"      </item>"
"    </section>"
"  </menu>"
"</interface>";

static void
aw_vl_reload_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   DataFile *wdata = (DataFile *) user_data;
   ap_reload_wave_file (wdata);
}

static void
aw_vl_unload_gaction (GSimpleAction *action, GVariant *param, gpointer user_data  )
{
   DataFile *wdata = (DataFile *) user_data;
   ap_delete_datafile(wdata);
}

static void
aw_vl_close_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   DataFile *wdata = (DataFile *) user_data;
//   msg_dbg("called");
   gtk_widget_destroy (wdata->wlist_win);
}

static void
aw_this_wave_add_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   DataFile *wdata = (DataFile *) user_data;
//   msg_dbg("called");
   WaveVar *var = (WaveVar *) g_object_get_data (G_OBJECT(wdata->lbpopmenu),
                                                    "ListButtonPopup-action" );
   if ( var ) {
      ap_panel_add_var( NULL, var, NULL, NULL);
   }
}

static void
aw_all_similar_add_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   DataFile *wdata = (DataFile *) user_data;
//   msg_dbg("called");
   WaveVar *var = (WaveVar *) g_object_get_data (G_OBJECT(wdata->lbpopmenu),
                                                    "ListButtonPopup-action" );
   if ( var ) {
      DataFile *wdata = (DataFile *) wavetable_get_datafile((WaveTable *) var->wvtable);
      datafile_similar_vars_add (wdata, var);
   }
}

static void
aw_all_similar_delete_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   DataFile *wdata = (DataFile *) user_data;
//   msg_dbg("called");
   WaveVar *var = (WaveVar *) g_object_get_data (G_OBJECT(wdata->lbpopmenu),
                                                    "ListButtonPopup-action" );
   if ( var ) {
      ap_remove_all_wave_if_var_name( wdata->ud, var);
   }
}

static GActionEntry vlentries[] = {
   { "Unload", aw_vl_unload_gaction, NULL, NULL, NULL  },
   { "Export", af_vl_export_data_gaction, NULL, NULL, NULL  },
   { "Reload", aw_vl_reload_gaction, NULL, NULL, NULL  },
   { "Close", aw_vl_close_gaction, NULL, NULL, NULL  },

   { "AddThisSelected", aw_this_wave_add_gaction, NULL, NULL, NULL  },
   { "AddAllSelected", aw_all_similar_add_gaction, NULL, NULL, NULL  },
   { "DelAllSimilar", aw_all_similar_delete_gaction, NULL, NULL, NULL  },
};

static TooltipInfo vlMenuTip[] = {
   { "Reload", N_("Reload this file"), N_("Reload this file"), NULL  },
   { "Unload", N_("Unload this file"), N_("Unload this file"), NULL  },
   { "Export", N_("Export data..."), N_("Export data from this file"), NULL  },
   { "Close", N_("_Close"), N_("Close this window"), NULL  },

   { "AddThisSelected", N_("Add this to selected"), N_("Add this wave to selected panel"), NULL  },
   { "AddAllSelected", N_("Add all similar to selected"), N_("Add all similar waves to selected panel"), NULL  },
   { "DelAllSimilar", N_("Delete all similar"), N_("Delete all similar waves from panels"), NULL  },
   { NULL, NULL, NULL, NULL  },
};

void gm_create_vl_menu (DataFile *wdata )
{
   GtkBuilder *builder;
   GMenuModel *menumodel;
   GError *error = NULL;

   wdata->group = g_simple_action_group_new ();
   g_action_map_add_action_entries (G_ACTION_MAP (wdata->group), vlentries,
                                    G_N_ELEMENTS (vlentries), wdata );

   builder = gtk_builder_new ();
   gtk_builder_add_from_string (builder, vl_menubar, -1, &error );
   g_assert_no_error (error);
   if (error != NULL) {
      g_error ("Failed to create widgets: %s\n", error->message);
   }
   
   menumodel = (GMenuModel *) gtk_builder_get_object (builder, "vl_menubar");
   wdata->vlmenu = (GtkWidget *) gtk_menu_bar_new_from_model (menumodel);
   gtk_widget_show_all (wdata->vlmenu);

   menumodel = (GMenuModel *) gtk_builder_get_object (builder, "listbuttonpopup");
   wdata->lbpopmenu = (GtkWidget *) gtk_menu_new_from_model (menumodel);

   gm_update_menu(wdata->vlmenu, vlMenuTip );
   gm_update_menu(wdata->lbpopmenu, vlMenuTip );
   gtk_widget_insert_action_group(GTK_WIDGET (wdata->vlmenu), "gaw",
                                  G_ACTION_GROUP (wdata->group));
   gtk_widget_insert_action_group(GTK_WIDGET (wdata->lbpopmenu), "gaw",
                                  G_ACTION_GROUP (wdata->group));
   g_object_unref (builder);
}

/****************************************************************/
/*
 * stuff for visible wave popup menu
 */

static const gchar buttonpopup[] =
"<?xml version='1.0'?>"
"<interface>"
"  <menu id='buttonpopup'>"
"    <section>"
"      <item>"
"        <attribute name='label' translatable='yes'>Delete this Wave</attribute>"
"        <attribute name='action'>gaw.DelThisWave</attribute>"
"        <attribute name='icon'>gtk-delete</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Change Color...</attribute>"
"        <attribute name='action'>gaw.ChangeColor</attribute>"
"        <attribute name='icon'>preferences-color-symbolic</attribute>"
"      </item>"
"      <item>"
"        <attribute name='label' translatable='yes'>Reload All waves</attribute>"
"        <attribute name='action'>gaw.ReloadAll</attribute>"
"        <attribute name='icon'>reload</attribute>"
"      </item>"
"    </section>"
"  </menu>"
"</interface>";

static void
aw_bp_delete_this_wave_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data)
{
   VisibleWave *vw =  (VisibleWave *) user_data;
   UserData *ud = vw->wdata->ud;
//   msg_dbg("called");
   if ( vw ) {
      wave_destroy(vw);
      ap_all_redraw(ud);
   }
}

static void aw_bp_reload_all_files_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   VisibleWave *vw =  (VisibleWave *) user_data;
   UserData *ud = vw->wdata->ud;
   g_list_foreach(ud->wdata_list, (GFunc) ap_reload_wave_file, NULL);
}

static TooltipInfo buttonPopupTip[] = {
   { "DelThisWave", N_("Delete this Wave"), N_("Delete this Wave"), NULL  },
   { "ChangeColor", N_("Change Color..."), N_("Change wave color"), NULL  },
   { "ReloadAll", N_("Reload All waves"), N_("Reread all waveform data files"), NULL  },
   { NULL, NULL, NULL, NULL  },
};

static GActionEntry bpentries[] = {
   { "DelThisWave", aw_bp_delete_this_wave_gaction, NULL, NULL, NULL  },
   { "ChangeColor", ac_bp_color_wave_gaction, NULL, NULL, NULL  },
   { "ReloadAll", aw_bp_reload_all_files_gaction, NULL, NULL, NULL  },
};

void gm_create_vw_popmenu (VisibleWave * vw )
{
   GtkBuilder *builder;
   GSimpleActionGroup *group;
   GMenuModel *menumodel;
   GError *error = NULL;

   group = g_simple_action_group_new ();
   g_action_map_add_action_entries (G_ACTION_MAP (group), bpentries,
                                    G_N_ELEMENTS (bpentries), vw );

   builder = gtk_builder_new ();
   gtk_builder_add_from_string (builder, buttonpopup, -1, &error );
   g_assert_no_error (error);
   if (error != NULL) {
      g_error ("Failed to create widgets: %s\n", error->message);
   }
   
   menumodel = (GMenuModel *) gtk_builder_get_object (builder, "buttonpopup");
   vw->buttonpopup = (GtkWidget *) gtk_menu_new_from_model (menumodel);
   gtk_widget_show_all (vw->buttonpopup);

   gm_update_menu(vw->buttonpopup, buttonPopupTip );
   gm_update_menu(vw->buttonpopup, vlMenuTip );
   gtk_widget_insert_action_group(GTK_WIDGET (vw->buttonpopup), "gaw",
                                  G_ACTION_GROUP (group));
   g_object_unref (builder);
   g_object_unref (group);
}

