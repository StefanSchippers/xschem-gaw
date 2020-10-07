/*
 * gawtext.c - text interface functions
 * 
 * include LICENSE
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#include <stdio.h>
#include <string.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

#include <gaw.h>
#include <gawutil.h>
#include <duprintf.h>

double degree_to_radian( double degree)
{
   return  degree * G_PI / (double) 180;
}
double str_degree_to_radian( char *angle)
{
   return degree_to_radian( g_ascii_strtoull( angle, NULL, 10));
}

int radian_to_degree(double angle)
{
   return (int) (angle * (double) 180 / G_PI );
}

/*
 *** \brief Allocates memory for a new GawText object.
 */

GawText *gawtext_new( UserData *ud, gchar *fontname, gchar *bg_color,
                      gchar *fg_color, gchar *angle)
{
   GawText *gtext;

   gtext =  app_new0(GawText, 1);
   gawtext_construct( gtext, ud, fontname, bg_color, fg_color, angle );
   app_class_overload_destroy( (AppClass *) gtext, gawtext_destroy );
   return gtext;
}

/** \brief Constructor for the GawText object. */

void gawtext_construct( GawText *gtext, UserData *ud, gchar *fontname,
                      gchar *bg_color, gchar *fg_color, gchar *angle)
{
   gchar *font;
   GdkRGBA defColor;

   app_class_construct( (AppClass *) gtext );

   gtext->user_data = ud;

   font = fontname;
   if ( ! fontname) {
      font = (gchar *) ud->panelfont;
   }
   gtext->fontname = app_strdup(font);

   gtext->bg_color = g_new0 (GdkRGBA, 1);
   gdk_rgba_parse( &defColor, "rgba(1,1,1,1)");
   ac_color_rgba_init(gtext->bg_color, &defColor, bg_color );
   
   gtext->fg_color = g_new0 (GdkRGBA, 1);
   gdk_rgba_parse( &defColor, "rgba(0,0,0,1)");
   ac_color_rgba_init(gtext->fg_color, &defColor, fg_color );

   if ( angle && *angle ){
      gtext->angle = str_degree_to_radian(angle);
   }
}

/** \brief Destructor for the GawText object. */

void gawtext_destroy(void *gtext)
{
   GawText *this = (GawText *) gtext;

   if (gtext == NULL) {
      return;
   }
   app_free(this->fontname);
   if (this->font_desc) {
      pango_font_description_free (this->font_desc);
   }
   g_free(this->bg_color);
   g_free(this->fg_color);

   app_free(this->text);

   app_class_destroy( gtext );
}


/*
 * duplicate a GawText
 */
GawText *gawtext_dup( GawText *gtext )
{
   gchar *bg_color_str = NULL;
   gchar *fg_color_str = NULL;
   gchar *angle_str = app_strdup_printf("%d", radian_to_degree(gtext->angle) );
      
   ac_color_rgba_str_set(&bg_color_str, gtext->bg_color );
   ac_color_rgba_str_set(&fg_color_str, gtext->fg_color );

   GawText *ngtext = gawtext_new(gtext->user_data, gtext->fontname,
                                bg_color_str, fg_color_str, angle_str);
   ngtext->win = gtext->win;
   ngtext->font_desc = pango_font_description_from_string (ngtext->fontname);
   app_dup_str( &ngtext->text, gtext->text );
   app_free(angle_str);
   return ngtext;
}

/*
 * callback function from drawingarea expose event
 * draw a text object in da
 */
void
gawtext_draw_text( GawText *gtext, WavePanel *wp)
{
   cairo_t *cr = wp->cr;
   
   cairo_save(cr);

   cairo_translate(cr, gtext->x, gtext->y );
   cairo_rotate(cr, gtext->angle); /* angle (in radians) */

   PangoLayout *layout = pango_cairo_create_layout (cr);
   pango_layout_set_font_description (layout, gtext->font_desc);
   pango_layout_set_text (layout, gtext->text, -1);
   pango_layout_get_pixel_size (layout, &gtext->width, &gtext->height);

   gdk_cairo_set_source_rgba (cr, gtext->bg_color);

   cairo_rectangle (cr, 0, 0, gtext->width, gtext->height );
   cairo_stroke_preserve(cr);
   cairo_path_extents (cr, &gtext->bbx1, &gtext->bby1, &gtext->bbx2, &gtext->bby2);
   cairo_fill(cr);

   gdk_cairo_set_source_rgba (cr, gtext->fg_color);

   cairo_move_to (cr, 0, 0 );
   pango_cairo_show_layout (cr, layout);
   
   g_object_unref (layout);
   cairo_restore(cr);
}


void
gawtext_update_pos( GawText *gtext, int x, int y)
{
   if ( ! gtext) {
      return;
   }
//   msg_dbg( "x %d, gtext->x %d, y %d, gtext->y %d", x, gtext->x, y, gtext->y);
   gtext->x += (x - gtext->cx);
   if ( gtext->x < 0 ){
      gtext->x = 0;
   }
   if ( gtext->x > (gtext->maxwidth -  gtext->width) ){
      gtext->x = (gtext->maxwidth -  gtext->width);
   }
   gtext->y += (y - gtext->cy);
   if ( gtext->y < 0 ){
      gtext->y = 0;
   }
   if ( gtext->y > (gtext->maxheight -  gtext->height) ){
      gtext->y = (gtext->maxheight -  gtext->height);
   }
   gtext->cx = x;
   gtext->cy = y;
}

/*
 * return TRUE if point x, y is inside text
 */
gboolean
gawtext_inside_text( AppClass *gt, int x, int y)
{
   GawText *gtext = (GawText *) gt;
   int x1 = gtext->x + (int) gtext->bbx1;
   int y1 = gtext->y + (int) gtext->bby1;
   int x2 = gtext->x + (int) gtext->bbx2;
   int y2 = gtext->y + (int) gtext->bby2;
   
   if ( x >= x1  &&  x < x2 && y >= y1 &&  y < y2 ){
//      msg_dbg( "%d <= %d < %d -- %d <= %d < %d", x1, x, x2, y1, y, y2 );
      return TRUE;
   }
   return FALSE;
}


static void
gawtext_select_font_cb (GtkWidget *button, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;
   const char *font;

   font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (button));
   if (font) {
      if (gtext->font_desc) {
         pango_font_description_free (gtext->font_desc);
      }
      app_free(gtext->fontname);
      gtext->fontname = app_strdup(font);
      gtext->font_desc = pango_font_description_from_string (font);
      msg_dbg("font '%s'", font);
      if ( gtext->edit) {
         da_drawing_redraw( gtext->da);
      }
   }
}

void
gawtext_combo_entry_changed_cb( GtkWidget *widget, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;
   const gchar *text;

   text = gtk_entry_get_text(GTK_ENTRY(widget));
   if ( text && *text ) {
      gtext->angle = str_degree_to_radian((gchar *) text );
      msg_dbg("text '%s'", text);
      if ( gtext->edit) {
         da_drawing_redraw( gtext->da);
      }
   }
}

void
gawtext_entry_changed_cb( GtkWidget *widget, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;
   const gchar *text;

   if ( gtext->edit) {
      text = gtk_entry_get_text(GTK_ENTRY(widget));
      app_dup_str( &gtext->text, (gchar *) text );
      da_drawing_redraw( gtext->da);
      msg_dbg("text '%s'", text);
   }
}

void gawtext_color_set_cb(GtkColorButton *widget, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;
   GdkRGBA *color;
   
   color = ( GdkRGBA *) g_object_get_data (G_OBJECT (widget), "colordest" );
   msg_dbg("dialog color cb 0x%lx", (unsigned long) color);
//   gtk_color_button_get_rgba(widget, color);
   gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widget), color);
   if ( gtext->edit) {
      da_drawing_redraw( gtext->da);
   }
}

void gawtext_button_save_cb(GtkWidget *widget, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;
   UserData *ud = (UserData *) gtext->user_data;

   app_dup_str( &ud->up->text_font, gtext->fontname );
   ac_color_rgba_str_set(&ud->up->text_bg_color, gtext->bg_color );
   ac_color_rgba_str_set(&ud->up->text_fg_color, gtext->fg_color );

   char *angle_str = app_strdup_printf("%d", radian_to_degree(gtext->angle) );
   app_dup_str(&ud->up->angle, angle_str);
   app_free(angle_str);
   msg_dbg("save settings");
   up_rc_rewrite(ud->up);
}

void gawtext_button_apply_cb(GtkWidget *widget, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;
   UserData *ud = (UserData *) gtext->user_data;
   const gchar *text;

   text = gtk_entry_get_text (GTK_ENTRY (gtext->entry));
   msg_dbg("text '%s'", text);
   ud->mouseState = M_DRAW_TEXT;
//   gtext->angle = G_PI / 2;
   
   app_dup_str( &gtext->text, (gchar *) text );
   if ( ! ud->gtexttmp ){
      ud->gtexttmp = gawtext_dup(gtext);
   }
}

void gawtext_button_cancel_cb(GtkWidget *widget, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;

   if ( gtext && gtext->win){
      gtk_widget_destroy(*gtext->win);
   }
}

void gawtext_win_destroy_cb(GtkWidget *widget, gpointer pdata)
{
   GawText *gtext = (GawText *) pdata;
   UserData *ud = (UserData *) gtext->user_data;

   if ( gtext ){
      if ( gtext->win){
         *gtext->win = NULL;
      }
      if ( ! gtext->edit ){
         gawtext_destroy(gtext);
      } else {
         gtext->edit = 0;
      }
      ud->mouseState = M_NONE;
      gawtext_destroy(ud->gtexttmp);
      ud->gtexttmp = NULL;
   }
}

static char *angletbl[] =
{
   "0", "90", "180", "270",
};

static int nangles = sizeof (angletbl) / sizeof (angletbl[0]);

static int gawtext_get_index(double radian)
{
   int i;
   int degree = radian_to_degree(radian);
   
   for ( i = 0; i < nangles ; i++ ){
      int degval = g_ascii_strtoull( angletbl[i] , NULL, 10);
      if ( degree == degval ){
         return i;
      }
   }
   return -1;
}


void gawtext_dialog (GawText *gtext,  UserData *ud)
{
   static GtkWidget *window = NULL;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *frame;
   GtkWidget *table;
   GtkWidget *label;
   GtkWidget *picker;
   GtkWidget *entry;
   GtkWidget *button;
   GtkWidget *combo;
   GdkRGBA *color;
   int i;

   if ( ! window) {
      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_widget_set_name (window, "text-window");
      if (gtext->edit){
         gtk_window_set_title (GTK_WINDOW (window), _("Gaw Text Edit"));
      } else {
         gtk_window_set_title (GTK_WINDOW (window), _("Gaw Text New"));
      }
      gtk_container_set_border_width (GTK_CONTAINER (window), 10);
      gtext->win = &window;
      g_signal_connect (window, "destroy", G_CALLBACK (gawtext_win_destroy_cb),
                        (gpointer) gtext );

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_container_add (GTK_CONTAINER (window), vbox);

      /* frame panel colors */
      frame = gtk_frame_new ( _("Text font and colors"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

      table = gtk_grid_new ();
      gtk_grid_set_column_spacing( GTK_GRID(table), 10);
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_container_set_border_width (GTK_CONTAINER (table), 10);

      label = gtk_label_new ("Font:");
//      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_set_halign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_xalign ( label, 0.5);
      gtk_widget_set_valign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_yalign ( label, 0.5);
      picker = gtk_font_button_new_with_font (gtext->fontname);
      gtk_grid_attach(GTK_GRID(table),  label, 
                /* left,    top,  width,   height    */
                      0,      0,      1,        1  );   
      gtk_grid_attach(GTK_GRID(table),  picker, 
                /* left,    top,  width,   height    */
                      0,      1,      1,        1  );   
      g_signal_connect (picker, "font-set",
                        G_CALLBACK (gawtext_select_font_cb), (gpointer) gtext );

      label = gtk_label_new (_("Bg:"));
//      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_set_halign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_xalign ( label, 0.5);
      gtk_widget_set_valign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_yalign ( label, 0.5);
      color = gtext->bg_color;
      picker = gtk_color_button_new_with_rgba (color);
      g_object_set_data (G_OBJECT (picker), "colordest", gtext->bg_color );
      gtk_grid_attach(GTK_GRID(table),  label, 
                /* left,    top,  width,   height    */
                      1,      0,      1,        1  );   
      gtk_grid_attach(GTK_GRID(table),  picker, 
                /* left,    top,  width,   height    */
                      1,      1,      1,        1  );   
      g_signal_connect (picker, "color-set",
                        G_CALLBACK (gawtext_color_set_cb), (gpointer) gtext );

      label = gtk_label_new (_("Fg:") );
//      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_set_halign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_xalign ( label, 0.5);
      gtk_widget_set_valign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_yalign ( label, 0.5);
      color = gtext->fg_color;
      picker = gtk_color_button_new_with_rgba (color);
      g_object_set_data (G_OBJECT (picker), "colordest", gtext->fg_color );
      gtk_grid_attach(GTK_GRID(table),  label, 
                /* left,    top,  width,   height    */
                      2,      0,      1,        1  );   
      gtk_grid_attach(GTK_GRID(table),  picker, 
                /* left,    top,  width,   height    */
                      2,      1,      1,        1  );   
      g_signal_connect (picker, "color-set",
                        G_CALLBACK (gawtext_color_set_cb), (gpointer) gtext );

      label = gtk_label_new (_("Angle:") );
//      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_set_halign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_xalign ( label, 0.5);
      gtk_widget_set_valign ( GTK_WIDGET (label), 0.5); 
//   gtk_label_set_yalign ( label, 0.5);

      /* combo box text */
      combo = gtk_combo_box_text_new_with_entry ();
      entry = gtk_bin_get_child(GTK_BIN(combo));
      gtk_widget_set_size_request(entry, 80, -1);
      for ( i = 0; i < nangles ; i++ ){
         gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),  angletbl[i] );
      }
      if ( (i = gawtext_get_index(gtext->angle)) < 0 ){
         char *text = app_strdup_printf("%d", radian_to_degree(gtext->angle) );
         gtk_entry_set_text (GTK_ENTRY (entry), text);
         app_free(text);  
      } else {
         gtk_combo_box_set_active (GTK_COMBO_BOX(combo), i);
      }
      gtk_grid_attach(GTK_GRID(table),  label, 
                /* left,    top,  width,   height    */
                      3,      0,      1,        1  );   
      gtk_grid_attach(GTK_GRID(table),  combo, 
                /* left,    top,  width,   height    */
                      3,      1,      1,        1  );   
      g_signal_connect (entry, "changed",
                        G_CALLBACK (gawtext_combo_entry_changed_cb), (gpointer) gtext );

      /* frame text entry */
      frame = gtk_frame_new ( _("Enter Text"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

      /* text entry */
      entry = gtk_entry_new ();
      gtk_container_add (GTK_CONTAINER (frame), entry);
      gtext->entry = entry;
      if (gtext->text) {
         gtk_entry_set_text (GTK_ENTRY (entry), gtext->text);
      }
      g_signal_connect(entry, "changed", G_CALLBACK (gawtext_entry_changed_cb),
                         (gpointer) gtext );
      
      hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

//      button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
      button = gawutil_button_new_with_label ("Save", "document-save",
                                              GTK_ICON_SIZE_BUTTON);
      gtk_widget_set_tooltip_text( GTK_WIDGET(button),
                               _("Copy current settings to userPrefs\n"
                                 "Note: Default settings are taken from userPrefs\n"
                                 "File/save conf to retrieve this settings at next startup") );
      g_signal_connect (button, "clicked", G_CALLBACK (gawtext_button_save_cb),
                                 (gpointer) gtext);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

      if (gtext->edit == 0){
//         button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
         button = gawutil_button_new_with_label ("Apply", "gtk-execute",
                                              GTK_ICON_SIZE_BUTTON);
         gtk_widget_set_tooltip_text( GTK_WIDGET(button),
                                   _("Draw current text in a panel\n"
                                     "Move the pointer to a panel and click left") );
         g_signal_connect (button, "clicked", G_CALLBACK (gawtext_button_apply_cb),
                           (gpointer) gtext);
         gtk_box_pack_end(GTK_BOX (hbox), button, FALSE, TRUE, 0);
      }
      
//      button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
      button = gawutil_button_new_with_label ("Close", "window-close", GTK_ICON_SIZE_BUTTON);
      gtk_widget_set_tooltip_text( GTK_WIDGET(button),
                                   _("Close this dialog window\n") );
      g_signal_connect (button, "clicked", G_CALLBACK (gawtext_button_cancel_cb),
                                 (gpointer) gtext);
      gtk_box_pack_end(GTK_BOX (hbox), button, FALSE, TRUE, 0);

      gtk_widget_show_all (vbox);
   }
   if ( ! gtk_widget_get_visible(window)) {
      gtk_widget_show (window);
   } else {
      gtk_widget_destroy (window);
   }
}

/*
 * Create a new text object
 */
void gawtext_new_gaction (GSimpleAction *action, GVariant *param, gpointer user_data)
{
   UserData *ud = (UserData *) user_data;
   GawText *gtext = gawtext_new(ud, ud->up->text_font, ud->up->text_bg_color,
                               ud->up->text_fg_color, ud->up->angle );
   gawtext_dialog( gtext, ud);
}

/*
 * Edit a new text object
 */
void gawtext_pop_edit_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   GawText *gtext;
   
   msg_dbg("gawtext_edit_action wp = 0x%x", wp );
   if ( ! wp ) {
      return;
   }
   gtext = (GawText *) g_object_get_data (G_OBJECT(wp->textpopmenu),
                                          "PanelTextPopup-action" );
   if ( gtext ) {
      if ( gtext->win && *gtext->win){
         gtk_widget_destroy (*gtext->win);
      }
      gtext->da = wp->drawing;
      gtext->edit = 1;
      gawtext_dialog( gtext, wp->ud);
   }
}

/*
 * Delete a text object action
 */
void gawtext_pop_delete_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   GawText *gtext;
   
   if ( ! wp ) {
      return;
   }
   gtext = (GawText *) g_object_get_data (G_OBJECT(wp->textpopmenu),
                                          "PanelTextPopup-action" );
   if ( gtext ) {
      msg_dbg("wp = 0x%x, gtext = %s", wp, gtext->text );
      pa_panel_text_list_remove(wp, gtext);
      gawtext_destroy(gtext);
      da_drawing_redraw( wp->drawing);
   }
}

/*
 * Delete all text objects in this panel  action
 */
void
gawtext_pop_delete_all_text_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( ! wp ) {
      return;
   }
   pa_panel_delete_all_texts( wp );
   da_drawing_redraw( wp->drawing);
}
