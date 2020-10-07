/*
 * gaw - Gtk analog waveform viewer
 *   GTK interface to Gaw
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <stdarg.h>
#include <locale.h>

#include <gtk/gtk.h>

#include <strcatdup.h>
#include <fileutil.h>
#include <duprintf.h>
#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        
#undef GTK_STOCK_DELETE
#define GTK_STOCK_DELETE GTK_STOCK_CUT   /* fedora 13 */


/*
 *  Analog wave globals
 */
char *progname ;
UserData  *userData  = NULL;

char *aw_panel_not_selected_msg ; /* see init in main */

AwSubmenuList xconv_method_tab[] = {
   {  "no-conversion", "system-run" },
   {  "time-t-conv",   "system-run" },
   {   NULL, NULL },
};


/*
 * Global functions
 */

void aw_do_save_config ( UserData *ud )
{
   ap_set_user_panel_size(ud);
   up_rc_rewrite(ud->up);
}

static void
aw_var_list_create_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   DataFile *wdata = (DataFile *) user_data;
   /* wlist_win is null if win has been destroyed */
   if ( wdata) {
//      msg_dbg( "calling datafile_create_list_win");
      datafile_create_list_win (wdata);
   }
}

void 
aw_vl_menu_item_add( DataFile *wdata)
{
   UserData *ud = (UserData *) wdata->ud;
   GSimpleAction *action;
   GMenuItem *item;
   GIcon *icon;

   char *action_name = app_strdup_printf("VLMenuAction%d", wdata->ftag);
   action = g_simple_action_new (action_name, NULL);
   g_action_map_add_action (G_ACTION_MAP (ud->group), G_ACTION (action));
   g_signal_connect (action, "activate",
                     G_CALLBACK (aw_var_list_create_gaction), wdata);
   g_object_unref (action);

   char *text = app_strdup_printf("gaw.%s", action_name);
   item = g_menu_item_new( wdata->wt->tblname, text );
   icon = g_icon_new_for_string ("document-open", NULL);
   g_menu_item_set_icon (item, icon);
   g_menu_append_item (G_MENU (ud->vlmmodel), item);
   g_object_unref (item);
   g_object_unref (icon);
   app_free(text);
   app_free(action_name);
}

/*
 * remove item from variable list menu
 */
void 
aw_vl_menu_item_remove( DataFile *wdata)
{
   UserData *ud = (UserData *) wdata->ud;

   char *action_name = app_strdup_printf("VLMenuAction%d", wdata->ftag);
   g_action_map_remove_action (G_ACTION_MAP (ud->group), action_name );

   g_menu_remove (G_MENU (ud->vlmmodel), wdata->ftag );
   app_free(action_name);
}


static void
aw_submenu_activate_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data )
{
   AwSubmenuAction *sa = (AwSubmenuAction *) user_data;
   AwSubmenuList *tbl = sa->tbl;
   const gchar *name = g_action_get_name (G_ACTION (action));
   int i = 0;
   GVariant *old_state, *new_state;

   new_state = g_variant_new_string (g_variant_get_string (param, NULL));
   const gchar *menu_name = g_variant_get_string (new_state, NULL);
      
#ifdef MSG_DEBUG
   /* set new state for the action */
   old_state = g_action_get_state (G_ACTION (action));
   msg_dbg ("Radio action %s activated, state changes from %s to %s",
           name, g_variant_get_string (old_state, NULL),
           menu_name );
   g_variant_unref (old_state);
#endif
   
   while (tbl->desc) {
      if ( app_strcmp(tbl->desc, menu_name) == 0 ) {
         *(sa->up_index) = i;
         sa->func(sa);
         break;
      }
      i++;
      tbl++;
   }
   g_variant_unref (new_state);
}

void aw_submenu_create( UserData *ud, GMenuModel *model, char *actionname, char *itemfmt,
                        AwSubmenuList *tbl, int *up_index,  SubmenuAction_FP func,
                        AwSubmenuAction *sa )
{
   GSimpleAction *action;
   GMenuItem *item;
   GIcon *icon;

   sa->ud = ud;
   sa->tbl = tbl;
   sa->up_index = up_index;
   sa->func = func;
   
   action = g_simple_action_new_stateful ( actionname, G_VARIANT_TYPE_STRING,
        g_variant_new_string (tbl[*up_index].desc));
   g_action_map_add_action (G_ACTION_MAP (ud->group), G_ACTION (action));
   g_signal_connect (action, "activate", G_CALLBACK (aw_submenu_activate_gaction), sa );
   
   g_object_unref (action);

   while (tbl->desc) {
      char *text = app_strdup_printf( itemfmt, tbl->desc);
      item = g_menu_item_new( tbl->desc, text );
      icon = g_icon_new_for_string ( tbl->icon_str, NULL);
      g_menu_item_set_icon (item, icon);
      g_menu_append_item (G_MENU (model), item);
      app_free(text);
      g_object_unref (item);
      g_object_unref (icon);
      tbl++;
   }
}

void aw_algo_exec ( AwSubmenuAction *sa )
{
   UserData *ud  = sa->ud;
   
   g_list_foreach(ud->panelList, (GFunc) pa_panel_set_drawing_func, NULL);
   ap_all_redraw(ud);
}

void aw_algo_menu_create( UserData *ud )
{
   if ( ! ud->algodata ){
      ud->algodata = g_new0 ( AwSubmenuAction, 1);
   }
   aw_submenu_create( ud, ud->algomodel, "algo", "gaw.algo::%s", wave_algo_desc_tab,
                      &ud->up->drawAlgo, aw_algo_exec, ud->algodata);
}

void aw_xconv_exec ( AwSubmenuAction *sa )
{
   ap_all_redraw(sa->ud);
}

void aw_xconv_menu_create( UserData *ud )
{
   if ( ! ud->xconvdata ){
      ud->xconvdata = g_new0 ( AwSubmenuAction, 1);
   }
   aw_submenu_create( ud, ud->xconvmodel, "xconv", "gaw.xconv::%s",  xconv_method_tab,
                      &ud->up->xconvert, aw_xconv_exec, ud->xconvdata );
}

void
aw_update_from_prefs ( UserData *ud )
{
   /* modify toggle button state from userPrefs */
   gm_update_toggle_state(ud->group, "showXlabels", ud->up->showXLabels);
   gm_update_toggle_state(ud->group, "showYlabels", ud->up->showYLabels);
   gm_update_toggle_state(ud->group, "moreYlabels", ud->up->showMoreYLabels);
   gm_update_toggle_state(ud->group, "LogX", ud->up->setLogX);
   gm_update_toggle_state(ud->group, "LogYPanel", 0);
   if ( ud->LogYPanel ) {
      gtk_widget_set_sensitive (ud->LogYPanel, 0 );
   }
   gm_update_toggle_state(ud->group, "Ydiff", ud->up->showYDiff);
   gm_update_toggle_state(ud->group, "ShowGrid", ud->up->showGrid);
   gm_update_toggle_state(ud->group, "Scientific", ud->up->scientific);
   gm_update_toggle_state(ud->group, "AllowResize", ud->up->allowResize);
   gm_update_toggle_state(ud->group, "BarStyle", ud->up->toolBarStyle);
   gtk_toolbar_set_style(GTK_TOOLBAR(ud->toolBar), (GtkToolbarStyle) ud->up->toolBarStyle );
}


/****************************************************************/

/*
 * GtkDialog
 */

int aw_dialog_show ( int type, const char *text)
{
   GtkWidget *dialog;
   UserData *ud = userData;
   char buffer[64] = "";
   gint response;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *image;
   GtkWidget *label;
   char *cancel_button = 0;
   int cancel_resp = 0;
   gchar *theme_id[] = {
      "help-contents",
      "gtk-dialog-info",
      "gtk-dialog-warning",
      "gtk-dialog-error",
      "gtk-stop",
   };

   if ( type & AW_MSG_T_SHOW_CANCEL) {
      cancel_button =  _("_Cancel");
      cancel_resp = GTK_RESPONSE_REJECT;
   }
   type &=  AW_MSG_T_MASK;
   if ( type == MSG_T_FATAL) {
      strcpy( buffer, _("Fatal - "));
   }
   strcat(buffer, _("Gaw Dialog"));
   
   if ( ! gtk_widget_get_realized (GTK_WIDGET(ud->window) ) ) {
      msg_dbg( "%s", text );
      aw_do_exit ( NULL, ud);
   }

   dialog = gtk_dialog_new_with_buttons (buffer,
                                         GTK_WINDOW (ud->window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          _("_OK"),
                                         GTK_RESPONSE_ACCEPT,
                                         cancel_button,
                                         cancel_resp,
                                         NULL);

   vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);

   hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_box_pack_start (GTK_BOX (vbox), 
		       hbox, TRUE, TRUE, 0);

   GIcon *icon = g_themed_icon_new (theme_id[type]);
   image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
   gtk_box_pack_start (GTK_BOX (hbox), 
		       image, TRUE, TRUE, 0);
   g_object_unref (icon);
   
   label = gtk_label_new ( text );
   gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
   gtk_label_set_max_width_chars (GTK_LABEL (label), 100);
//   gtk_misc_set_padding (GTK_MISC (label), 10, 10);
   g_object_set (G_OBJECT (label), "margin", 10, NULL);
   gtk_box_pack_start (GTK_BOX (hbox), 
		       label, TRUE, TRUE, 0);

   gtk_widget_show_all (vbox);

   response = gtk_dialog_run (GTK_DIALOG (dialog));
   gtk_widget_destroy (dialog);

   if ( type == MSG_T_FATAL && ! prog_debug ){
      gtk_widget_destroy (ud->window);
   }
   return response;
}

/*
 * Main Window and Exit
 */

void
aw_do_exit (GtkWidget *widget,  UserData *ud )
{
   msg_dbg( "called 0x%lx", (unsigned long) ud->window);
   aw_main_window_destroy ( ud ) ;     

   /* if widget is not NULL, this function is called from callback */
   if ( widget == NULL && ud->window ) {
      gtk_widget_destroy (ud->window);
   }

   gtk_main_quit ();
}

//#define DEBUG_SIZE 1
/*
 * calculate the size of main window and set size
 */
void aw_window_size(UserData *ud )
{
   int width;
   int height ;
   GtkRequisition req;
   GtkRequisition nreq;

   if ( ! ud->panel_scrolled) {
      return ;     /* not yet  ready to go */
   }
   width = 2 * gtk_container_get_border_width( GTK_CONTAINER(ud->globalTable));
   height =  width ;
#if DEBUG_SIZE
   msg_dbg( "w %d, h %d", width, height );
#endif
   width += ud->up->lmtableWidth;
#if DEBUG_SIZE
   msg_dbg( "w %d, h %d", width, height );
#endif

   /* panelWidth is panel size without scrollbar */  
   width += ud->up->panelWidth;
#if  DEBUG_SIZE
   msg_dbg( "w %d, h %d", width, height );
#endif
   if ( ! ud->sbSize && gtk_widget_get_visible (GTK_WIDGET(ud->panel_scrolled)) ) {
     /*  sbSize = w(panel_scrolled) - w(panelTable) */
      gtk_widget_get_preferred_size ( GTK_WIDGET(ud->panel_scrolled), &req, &nreq);
//      ud->sbSize = req.width;
      gtk_widget_get_preferred_size( GTK_WIDGET(ud->panelTable), &req, &nreq);
//      ud->sbSize -= req.width;
   }
   /* request 1 pixel more */
   width += ud->sbSize ;
#if  DEBUG_SIZE
   msg_dbg( "w %d, h %d", width, height );
#endif

   gtk_widget_get_preferred_size( GTK_WIDGET(ud->menuBar), &req, &nreq);
   height += req.height;
#if  DEBUG_SIZE
   msg_dbg( "w %d, h %d", width, height );
#endif
   
   gtk_widget_get_preferred_size( GTK_WIDGET(ud->toolBar), &req, &nreq);
   height += req.height;
#if  DEBUG_SIZE
   msg_dbg( "w %d, h %d", width, height );
#endif   
   if ( ud->meas_hbox_shown ) {
      gtk_widget_get_preferred_size( GTK_WIDGET(ud->meas_hbox), &req, &nreq);
      height += req.height;
      msg_dbg( "w %d, h %d", width, height );
   }
   if ( ud->up->showXLabels ) { 
      gtk_widget_get_preferred_size( GTK_WIDGET(ud->xlabel_ev_box), &req, &nreq);
      height += req.height;
#if  DEBUG_SIZE
     msg_dbg( "w %d, h %d", width, height );
#endif
   }
   gtk_widget_get_preferred_size( GTK_WIDGET(ud->xscrollbar), &req, &nreq);
   height += req.height;
   msg_dbg( "w %d, h %d", width, height );

   
   /* add the height of scrolled window */
   height += ud->panelScrolledHeight;     /* height = 112 + panelScrolledHeight */
//   msg_dbg( "w %d, h %d", width, height );

   ud->reqWinWidth = width;
   ud->reqWinHeight = height;
   /* when gtk is alive, use the current width */
//   if ( ud->winWidth ) {
//      width = ud->winWidth ;
//   }
   if ( ud->up->allowResize == 0 || ud->winWidth == 0 ) {
      gtk_window_resize (GTK_WINDOW (ud->window), ud->reqWinWidth, ud->reqWinHeight);
   }
   msg_dbg( "w %d, h %d panelH %d", ud->reqWinWidth, ud->reqWinHeight, ud->panelScrolledHeight );
}

void aw_panel_scrolled_set_size_request( UserData *ud )
{
   int height;
   int np = g_list_length( ud->panelList);
   int h;

   /* 1 + panelHeight + 1 */
   height = np * (ud->up->panelHeight + 2);

   /* if not configured or enough room for panel */
   if ( ud->maxHeight == 0 || height < ud->maxHeight ){
       h = height ;
   } else {
      h = (ud->maxHeight / (ud->up->panelHeight + 2)) *
	 (ud->up->panelHeight + 2);
   }

   h += 4;
   ud->panelScrolledHeight = h ;

   gtk_scrolled_window_set_min_content_height(
                      GTK_SCROLLED_WINDOW (ud->panel_scrolled), h );     

   msg_dbg( "npanel %d, hscrolled %d htable %d", np, h, height );
   aw_window_size(ud);
}


static gboolean
aw_window_configure_cb( GtkWidget *widget, GdkEventConfigure *event,
		      gpointer data )
{
   UserData *ud = (UserData *) data;
   gint root_x, root_y;
   GtkAllocation walloc;

   gtk_widget_get_allocation (widget, &walloc);  
   msg_dbg( "x %d, y %d, w %d, h %d new %d %d 0x%lx",
	    event->x,  event->y,
	    walloc.width,
	    walloc.height,
	    event->width,  event->height,
	    (long unsigned int) widget );

   GdkWindow *window = gtk_widget_get_window (ud->panel_scrolled);
   gdk_window_get_root_coords(window, 0, 0,  &root_x, &root_y);
   GtkAllocation alloc;
   gtk_widget_get_allocation (ud->panel_scrolled, &alloc);

   root_x += alloc.x;
   root_y += alloc.y;

   if ( ud->maxHeight <= 0 ){
      if ( ud->up->max_ps_y ){
         ud->maxHeight = ud->up->max_ps_y - root_y;
      } else {
         ud->maxHeight = ud->waHeight - root_y;
         ud->up->max_ps_y = ud->waHeight;
      }
   }

   ud->winWidth = walloc.width;
   ud->winHeight = walloc.height;
   
   if ( ud->winWidth != ud->reqWinWidth || ud->winHeight != ud->reqWinHeight ) {
      msg_dbg( "ww %d rww %d wh %d rwh %d", ud->winWidth, ud->reqWinWidth,
             ud->winHeight, ud->reqWinHeight);
      aw_window_size(ud);
   }

   return FALSE; /* FALSE to propagate the event further. */
}

static gboolean
aw_window_draw_cb( GtkWidget *widget, cairo_t *cr, gpointer data )
{
   UserData *ud = (UserData *) data;
   int w = gtk_widget_get_allocated_width(widget);
   int h = gtk_widget_get_allocated_height(widget);
   h = h; /* silent the warming when not compiled in debug mode */
   
   msg_dbg( "w %d, h %d 0x%lx", w, h, (long unsigned int) widget );
   
   int width = ud->winWidth;
   ud->winWidth = w;
   if ( width == 0 ) {
      /* first expose -  compensation - to be explained */ 
      ud->panelScrolledHeight += 2;
      aw_window_size(ud);
   }
   return FALSE; /* FALSE to propagate the event further. */
}

void aw_scrollbar_show_cb( GtkWidget *widget, gpointer pdata)
{
   UserData *ud = (UserData *) pdata;
   GawLabels *lbx = ud->xLabels;
   if ( ! lbx ) {
      return;
   }
   lbx->changed &= ~(CV_SBCHANGED | CV_SBSHOW );
   lbx->changed |= CV_SBCHANGED | CV_SBSHOW ;
   msg_dbg("show w %d, h %d, sbstate 0x%x", lbx->w, lbx->h, lbx->changed);
   return;
}
void aw_scrollbar_hide_cb( GtkWidget *widget, gpointer pdata)
{
   UserData *ud = (UserData *) pdata;
   GawLabels *lbx = ud->xLabels;
   if ( ! lbx ) {
      return;
   }
   lbx->changed &= ~(CV_SBCHANGED | CV_SBSHOW );
   lbx->changed |= CV_SBCHANGED ;
   msg_dbg("show w %d, h %d, sbstate 0x%x", lbx->w, lbx->h, lbx->changed);
   return;
}

void aw_create_main_window ( UserData *ud )
{
   GtkWidget *window;
   GtkAdjustment *adj;
   GdkGeometry geometry;
   int i;

   /* create a global GawLabels object for X labels */
   ud->xLabels = al_label_new(ud->up, ud->up->setLogX, 1);
   /* create a global SelRange object */
   ud->srange = g_new0(SelRange, 1);

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   ud->window = window;
   gtk_window_set_title (GTK_WINDOW (window), ud->mainName );
   gtk_widget_set_name (window, ud->mainName );

   g_signal_connect (window, "destroy",
		     G_CALLBACK (aw_do_exit), (gpointer) ud );
   g_signal_connect (window, "delete_event",
		     G_CALLBACK (gtk_false), NULL);
   g_signal_connect (window, "configure-event",
		     G_CALLBACK (aw_window_configure_cb), (gpointer) ud);
   g_signal_connect (window, "draw", 
                     G_CALLBACK (aw_window_draw_cb), (gpointer) ud);

   /* create global Table */
   gm_create_layout(ud);  /*  create menu, toolbar, statusBar */
   gtk_grid_set_column_homogeneous(GTK_GRID(ud->globalTable), TRUE);
   gtk_widget_show(ud->globalTable);
   gtk_container_set_border_width (GTK_CONTAINER (ud->globalTable), WIN_BORDER_SIZE);
   gtk_container_add (GTK_CONTAINER (window), ud->globalTable);

   /* create the 3 X measure buttons */
   ap_create_xmeasure_unit(ud);

   /* horizontal box for X-axis labels */
   ap_xlabel_box_create(ud);
   ap_create_win_bottom(ud);
   
   /* create panel table */
   ud->panelTable = gtk_grid_new();
   gtk_grid_set_row_spacing ( GTK_GRID(ud->panelTable), 1);
   gtk_grid_set_row_homogeneous(GTK_GRID(ud->panelTable), TRUE);
   gtk_widget_set_hexpand(ud->panelTable, TRUE );
   gtk_widget_show(ud->panelTable);

   /* create a scrolled window to contains panels with vertical scrollbar */
   gtk_widget_set_size_request (GTK_WIDGET(ud->panel_scrolled), -1, 30);

   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ud->panel_scrolled),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
   GtkWidget *vscrollbar = gtk_scrolled_window_get_vscrollbar(
                                  GTK_SCROLLED_WINDOW(ud->panel_scrolled));

   gtk_widget_set_can_focus (vscrollbar, FALSE);
   gtk_widget_show (ud->panel_scrolled);
   gtk_container_add(GTK_CONTAINER(ud->panel_scrolled), ud->panelTable);

   adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW (ud->panel_scrolled) );
   gtk_container_set_focus_vadjustment( GTK_CONTAINER (ud->panelTable), adj );
   gtk_adjustment_set_page_size (adj, gtk_adjustment_get_upper(adj) ); 

   /*  set callback for scroll bar */
   GtkWidget *scr = gtk_scrolled_window_get_vscrollbar(
			    GTK_SCROLLED_WINDOW(ud->panel_scrolled)) ;
   g_signal_connect (scr, "show",
		     G_CALLBACK (aw_scrollbar_show_cb), (gpointer) ud);
   g_signal_connect (scr, "hide",
		     G_CALLBACK (aw_scrollbar_hide_cb), (gpointer) ud);
   
   /* create 2 panels */
   for (i = 0 ; i < ud->reqpanels ; i++) {
      ap_panel_add_line(ud, NULL, 0);
   }


   /* create horizontal scrollbar */
   gtk_grid_attach(GTK_GRID(ud->globalTable), ud->xscrollbar, 
                /* left,    top,  width,   height    */
                      1,      5,      1,        1  );   

   /* status bar */
   GtkWidget *hbox = gtk_statusbar_get_message_area (GTK_STATUSBAR (ud->statusbar));
   GtkWidget *label = GTK_WIDGET (gtk_container_get_children (GTK_CONTAINER (hbox))->data);
   ud->statusLabel = label;
   gtk_widget_set_name(label, "statusbar" );

   /* status bar test */
   gtk_statusbar_push (GTK_STATUSBAR (ud->statusbar), 0, 
			  _("Welcome to gaw"));

   aw_update_from_prefs(ud);
   if ( ! gtk_widget_get_visible (GTK_WIDGET(window)) ) {
      gtk_widget_show (window);
//     gtk_widget_show_all (window);
      memset(&geometry, 0, sizeof(geometry));
      geometry.min_width = ud->up->min_win_width;
      geometry.min_height = ud->up->min_win_height;
      gtk_window_set_geometry_hints (GTK_WINDOW(window), NULL,
                                     &geometry,  GDK_HINT_MIN_SIZE );
      gtk_window_move (GTK_WINDOW(window), 160, -1);

      /* compute the lower point on screen */
      GdkScreen *screen = gdk_screen_get_default ();
      GdkRectangle dest;
      gdk_screen_get_monitor_workarea (screen, 0, &dest);
      msg_dbg("workarea w %d h %d", dest.width, dest.height);
      ud->waHeight = dest.height;
   }

   if ( ud->listFiles ){
      af_load_wave_file (ud);
   }
}
   
void aw_print_theme(void)
{
   char *name = NULL;
   GtkSettings * settings = gtk_settings_get_default ();
   g_object_get (settings,
                 "gtk-theme-name", &name,
                 NULL);
   msg_dbg ("theme_name %s", name);
   g_object_get (settings,
                 "gtk-icon-theme-name", &name,
                 NULL);
   msg_dbg ("gtk-icon-theme-name %s", name);
   g_object_unref (settings);
}

void aw_rc_parse_gtkrc(UserData *ud)
{
   gchar *filename;

   GtkCssProvider *provider;
   GdkDisplay *display;
   GdkScreen *screen;
   int ret = 0;

   provider = gtk_css_provider_new ();

   gchar *rc = "gaw-gtkrc.css";
   filename = app_strcatdup ( GAWRCDIR, "/", rc, NULL);
   if (file_exists (filename)) {
      /* error = NULL will print a message if error in css */
      gtk_css_provider_load_from_path (provider, filename, NULL );
      ret++;
   }

   if (ud->up->rcDir) {
      char *userfile = app_strcatdup ( ud->up->rcDir, "/", rc, NULL);
      if (file_exists (userfile)) {
         /* error = NULL will print a message if error in css */
         gtk_css_provider_load_from_path (provider, userfile, NULL );
         ret++;
      }
      app_free (userfile);
   }
   if ( ! ret ) {
      msg_warning(_("file '%s' do not exist."), filename );
   }
   app_free (filename);

   display = gdk_display_get_default ();
   screen = gdk_display_get_default_screen (display);

   gtk_style_context_add_provider_for_screen (screen,
                    GTK_STYLE_PROVIDER (provider),
                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   g_object_unref (provider);
}


void aw_main_window_destroy ( UserData *ud )
{
   int i;

   if ( ! ud->xLabels ){ /* avoid callback */
      return;
   }
   af_unload_all_files(ud);
   
   /* GawLabels object for X labels */
   al_label_destroy(ud->xLabels);
   ud->xLabels = NULL;
   /* global SelRange object */
   g_free(ud->bg_color);
   g_free(ud->pg_color);
   g_free(ud->hl_color);
   g_free(ud->srange->color);
   g_free(ud->srange);
   g_free(ud->wbut_color);
   g_free(ud->lbbut_fgcolor);
   g_free(ud->lbbut_bgcolor);
   g_free((char *) ud->panelfont);
   for ( i = 0 ; i < 2 ; i++) {
      g_free(ud->cursors[i]->color);
   }
   for ( i = 0 ; i < AW_NX_MBTN ; i++) {
      g_free(ud->cursors[i]);
   }
   g_free(ud->cursors);

   /* drag and drop dest data */
   i = g_list_length( ud->destdata_list);
   for (  ; i > 0 ; i--) {
      void *p = g_list_nth_data ( ud->destdata_list, i - 1);
      g_free(p);
   }

   /* destroy panels */
   i = g_list_length( ud->panelList);
   for (  ; i > 0 ; i--) {
      WavePanel *wp = (WavePanel *) g_list_nth_data (ud->panelList, i - 1);
      pa_panel_destroy(wp);
   }
   gtk_widget_destroy(ud->panel_scrolled);
 
   /* global Table */
   gtk_widget_destroy(ud->globalTable);

//   gtk_widget_destroy(ud->window);
   g_free(ud->algodata);
   g_free(ud->xconvdata);
}
   

void aw_usage(char *prog)
{
   fprintf(stderr, 
_("Usage: %s [OPTIONS]... [FILE]...\n"
  "\n"
  "                : display analog waveforms from input FILE...\n"
  " [-h]           : print list of options\n"
  " [-d]           : set debug mode\n"
  " [-rf fmt]      : specify spice format for input file\n"
  " [-pf fmt]      : specify a printf format for output file\n"
  " [-p num]       : port number for inside server\n"
  " [-C name]      : configuration dir or file name\n")
       , prog);   
   exit(1);
}

int
main (int argc, char *argv[])
{
   int i;
   int done = 0;
   UserData *ud;
   char *penv;
   char *rcfile = NULL;
   int user_debug = 0;

   tmem_init( 0 );
   srand (time (NULL));
   progname = app_strdup( basename(argv[0]));

//   gtk_set_locale (); /* unusefull, this is set in gtk_init */

   /* init defaults */
   if ( ! userData) {
     userData = g_new0 (UserData, 1);
   }
   ud = userData;
   msg_initlog(progname, MSG_F_NO_DATE, NULL, NULL );
   
   ud->prog = progname ;
   ud->mainName = progname;
   ud->NWColors = 6;

   if ( ( penv = getenv("GAW_HOMEDIR")) ){
      rcfile = app_strdup( penv);
   }

   /* global messages */
  aw_panel_not_selected_msg =
  _("\n\nThis function requires a panel to be selected\n"
   "Select a panel by left clicking in one DrawingArea\n");

   
   for ( i = 1 ; i < argc ; i++ ) {
      if (*argv[i] == '-') {
         if(strcmp(argv[i], "-d") == 0) {
	    user_debug++ ;
#ifndef MSG_DEBUG
	    msg_info(_("Compile with -DMSG_DEBUG, to get debug infos."));
#endif	 
	 } else if (strcmp(argv[i], "-h") == 0) {
	    aw_usage(progname);
	 } else if (strcmp(argv[i], "-rf") == 0) {
	    app_free(ud->format);
	    ud->format = app_strdup( argv[++i] );
	 } else if (strcmp(argv[i], "-pf") == 0) {
	    ud->printFmt = argv[++i];
	 } else if (strcmp(argv[i], "-p") == 0) {
	    ud->listenPort = atoi(argv[++i]);
	 } else if (strcmp(argv[i], "-C") == 0) {
	    app_free(rcfile);
	    rcfile = app_strdup(argv[++i]);
	 }
      } else {
	 if ( ! done ){
	    done++;
	    af_list_files_free(&ud->listFiles);
	 }
         if (file_exists (argv[i])) {
            ud->listFiles = g_slist_append( ud->listFiles, app_strdup( argv[i]) );
         }
      }
   }
   
   UserPrefs *up = up_new( ud->prog, rcfile, NULL);
   if ( user_debug ) {
     prog_debug = user_debug;
   }
   ud->up = up;
   app_free(rcfile);

   ud->reqpanels = up->npanels; 
   ud->listenPort = up->listenPort;
   /* only if data files not given on command line */
   if ( ud->listFiles == NULL && up->lastDataFile && *up->lastDataFile ){
      if (file_exists (up->lastDataFile)) {
         ud->listFiles = g_slist_append( ud->listFiles,
				      app_strdup(up->lastDataFile));
      }
   }
   if ( up->dataFileFormat && *up->dataFileFormat ) {
      ud->format =  app_strdup( up->dataFileFormat );
   }
   ud->bits = 16;    /* for writing wav */
   ud->rate = 48000; /* for writing wav */


   do {
      ud->restart = 0;
      
      /*
       * after gtk_init, gtk_init call setlocale
       * before gtk_init for  g_dgettext !!!
       */

#ifdef ENABLE_NLS
#ifndef PACKAGE_LOCALE_DIR
#define PACKAGE_LOCALE_DIR "share/locale/"
#endif

        void *p = bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
        if (p == NULL) {
	   msg_error("bindtextdomain failed");
	}
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
#else
        msg_dbg("NLS disabled.");
#endif

      gtk_init (&argc, &argv);
      /* after gtk_init, gtk_init call setlocale for reading numeric string */
      setlocale(LC_NUMERIC, "C");
      aw_rc_parse_gtkrc(ud);
      if ( prog_debug){
         aw_print_theme();
      }

      aw_create_main_window (ud);
      if ( ud->listenPort ) {
	 aio_create_channel(ud);
      }
      /* just before starting gtk_main !!! */
      msg_set_func( aw_dialog_show ); /* redirect msg to a dialog window */
      gtk_main ();
      if ( userData->gawio ) {
	 aio_destroy_channel(userData);
      }
   } while ( ud->restart );

   af_list_files_free( &ud->listFiles );
 
   as_sound_destroy(ud->sndData);
   
   g_free (ud->prog);
   up_destroy(up);

   g_free (ud);

   tmem_destroy(NULL);
   return 0;
}
