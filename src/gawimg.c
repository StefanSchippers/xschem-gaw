/*
 * gawimg.c - Save image to file
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/extensions/shape.h>

#include <gaw.h>
#include <duprintf.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


void im_add_if_writable (GdkPixbufFormat *data, GSList **list)
{
   if (gdk_pixbuf_format_is_writable (data)) {
      *list = g_slist_prepend (*list, data);
   }
}

typedef struct _ImgDialogData {
   UserData *ud;
   WavePanel *wp;
   int numpanel;
   char *filename;
   char *imgFmt;
   GtkFileChooser *chooser; /* pointer to dialog for callbacks */
   GtkFileFilter *filter; /* file filter associated with fileType */
} ImgDialogData;

/*
 * filter associated with fileType
 */
static void
im_fs_filter_change(GtkFileChooser *chooser, ImgDialogData *imdata )
{
   GtkFileFilter *filter;
   char *name;

   /* file filter for active file type (*.ihx) */
   if ( imdata->filter){
       gtk_file_chooser_remove_filter(chooser, imdata->filter);
   }

   filter = gtk_file_filter_new ();
   imdata->filter = filter;
   name = app_strdup_printf("*.%s", imdata->imgFmt );
   gtk_file_filter_set_name (filter, name );
   gtk_file_filter_add_pattern (filter, name);
   app_free(name);
   gtk_file_chooser_add_filter (chooser, filter);
}


static void
im_entry_changed_cb (GtkWidget *widget, gpointer pdata)
{
   UserPrefs *up = ((ImgDialogData *) pdata)->ud->up;
   char text[64];
   unsigned int val = (unsigned int) g_ascii_strtoull ( gtk_entry_get_text (GTK_ENTRY (widget)), NULL, 10 );

   if ( val <= 20000 ) {
      up->exportTimeout = val;
   } else { /* input in this entry */
      sprintf( text, "%d", up->exportTimeout );
      gtk_entry_set_text (GTK_ENTRY (widget), text);
   }
   msg_dbg( "timeout changed:  %d", up->exportTimeout );
}

void
im_fs_radio_clicked (GtkWidget *w,  ImgDialogData *imdata )
{
   UserPrefs *up = imdata->ud->up;
   char *save_filename;

   if ( ! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) ){
       return;
    }
   char *imgFmt = (gchar *) gtk_button_get_label (GTK_BUTTON(w)) ;
   app_dup_str( &imdata->imgFmt, imgFmt );
   if ( app_strcmp( imgFmt, up->imgFmt) ){
      app_dup_str( &up->imgFmt, imgFmt);
   }
   if ( imdata->numpanel < 0 ) {
      save_filename = g_strdup_printf( "%s.%s", up->imgBaseName, imgFmt);
   } else {
      save_filename = g_strdup_printf( "%s%02d.%s", up->imgBaseName,
			    imdata->numpanel, imgFmt);
   }
   gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (imdata->chooser), save_filename);
   g_free(save_filename);

   im_fs_filter_change(imdata->chooser, imdata );
}

static void
im_fs_filter_add (GtkFileChooser *chooser, ImgDialogData *imdata )
{
   GtkFileFilter *filter;
   UserData *ud = imdata->ud;
   char *name;

   /* file filter for all data type files (*.png ...) */
   filter = gtk_file_filter_new ();
   gtk_file_filter_set_name (filter, _("All img formats"));

   GSList *list =  ud->imgFormats;   
   while (list) {
      GSList *next = list->next;
 
      char *imgFmt = gdk_pixbuf_format_get_name  (list->data);
      name = app_strdup_printf("*.%s", imgFmt );
      gtk_file_filter_add_pattern (filter, name);
      app_free(name);
      list = next;
   }
   gtk_file_chooser_add_filter (chooser, filter);
   
   /* file filter that match any file */
   filter = gtk_file_filter_new ();
   gtk_file_filter_set_name (filter, _("All files"));
   gtk_file_filter_add_pattern (filter, "*");
   gtk_file_chooser_add_filter (chooser, filter);
}
           
GtkWidget *
im_create_img_radio_widget( GtkFileChooser *chooser, ImgDialogData *imdata )
{
   UserData *ud =  imdata->ud;
   UserPrefs *up = ud->up;
   GSList *group = NULL;
   GtkWidget *button = NULL;
   GtkWidget *activebut = NULL;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *frame;
   GtkWidget *label;
   GtkWidget *entry;
   char *name;

   GSList *formats = gdk_pixbuf_get_formats ();
   if ( ud->imgFormats == NULL ) {
      g_slist_foreach (formats, (GFunc) im_add_if_writable,  &ud->imgFormats);
   }
   g_slist_free (formats);

   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

   frame = gtk_frame_new (_("Gtk Image Formats"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
   gtk_container_add (GTK_CONTAINER (frame), hbox);

   GSList *list =  ud->imgFormats;   
   while (list) {
      GSList *next = list->next;
 
      name = gdk_pixbuf_format_get_name  (list->data);
      button = gtk_radio_button_new_with_label (group, name);
      group =  gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      g_signal_connect (button, "clicked",
			G_CALLBACK(im_fs_radio_clicked), imdata );
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      if ( strcmp( name, up->imgFmt) == 0 ){
         activebut = button;
	 ud->imgFmt = name;
      }
      list = next;
   }
   if ( ! activebut ){
      activebut = button;
   }

   frame = gtk_frame_new (_("Export timeout"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (frame), 10);

   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
   gtk_container_add (GTK_CONTAINER (frame), hbox);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

   label = gtk_label_new (_("timeout ms:"));
//   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.4);
   gtk_widget_set_valign ( GTK_WIDGET (label), 0.4); 
//   gtk_label_set_yalign ( label, 0.4);
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
   entry = gtk_entry_new() ;
   gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
   gtk_entry_set_alignment (GTK_ENTRY (entry), 1.0); /* right */
   g_signal_connect (entry, "changed",
                     G_CALLBACK (im_entry_changed_cb),
                     ( gpointer) imdata );
   char text[64];
   sprintf(text, "%d", up->exportTimeout);
   gtk_entry_set_text (GTK_ENTRY (entry), text);

   gtk_widget_show_all (vbox);

   if ( activebut ){
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (activebut), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (activebut), TRUE);
   }

   /* add filters */
   im_fs_filter_add (chooser, imdata);
   
   return vbox;
}

static void
im_save_img_file ( ImgDialogData *pdata, GdkPixbuf *pixbuf )
{
   UserPrefs *up = pdata->ud->up;
   int ret;
   gchar tmpstr[32];
   GError *err = NULL;

   if ( app_strcmp( pdata->imgFmt, "jpeg") == 0 ){
      sprintf(tmpstr, "%d",  up->jpegQuality );
      ret = gdk_pixbuf_save (pixbuf,  pdata->filename,  pdata->imgFmt, &err,
			     "quality", tmpstr,  NULL) ;
   } else if ( app_strcmp(  pdata->imgFmt, "png") == 0 ){
      sprintf(tmpstr, "%d",  up->pngCompression );
      ret = gdk_pixbuf_save (pixbuf,  pdata->filename,  pdata->imgFmt, &err,
			     "compression", tmpstr,  NULL) ;
   } else {
      ret = gdk_pixbuf_save (pixbuf,  pdata->filename,  pdata->imgFmt, &err,
			     NULL) ;
   }
   if ( ! ret ) {
      msg_warning ("%s %s : %s",  pdata->filename,  pdata->imgFmt, err->message);
      g_error_free (err);
   }
}

static int
im_save_img_file_dialog ( ImgDialogData *imdata )
{
   GtkWidget *dialog;
   GtkWidget *vbox;
   gint response;
   char *save_filename;

   dialog = gtk_file_chooser_dialog_new (_("Select file"),
					 GTK_WINDOW (userData->window),
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 _("_Cancel"), GTK_RESPONSE_REJECT,
					 _("_Save"), GTK_RESPONSE_ACCEPT,
					 NULL);
   imdata->chooser = GTK_FILE_CHOOSER(dialog);

   vbox = im_create_img_radio_widget( GTK_FILE_CHOOSER(dialog), imdata);
   gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), vbox);
   
   gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
   response = gtk_dialog_run (GTK_DIALOG (dialog));

   if (response == GTK_RESPONSE_ACCEPT) {
      save_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      app_free(imdata->filename);
      imdata->filename = app_strdup(save_filename);
   }
   
   gtk_widget_destroy (dialog);
   return response;
}

/*
 * create image for one panel
 */
gboolean
im_export_panel_to_img_iddle (gpointer data )
{
   GdkPixbuf     *pixbuf;
   ImgDialogData *pdata = (ImgDialogData *) data;
   WavePanel *wp = pdata->wp;
   GtkAllocation walloc;

   GdkWindow *window = gtk_widget_get_window (wp->drawing);
   gtk_widget_get_allocation (wp->drawing, &walloc);
   pixbuf = gdk_pixbuf_get_from_window (window, 0, 0,
                                        walloc.width, walloc.height);
   fprintf( stderr, "x = %d, y = %d width = %d height = %d\n",
            0, 0, walloc.width, walloc.height);
   if ( pixbuf ){
      im_save_img_file ( pdata, pixbuf );
      g_object_unref (pixbuf);
   } else {
      msg_dbg("Getting pixbuf from window failed");
   }
   app_free(pdata->filename);
   app_free(pdata->imgFmt);
   g_free (pdata);

   return FALSE; /* the source will be removed. */ 
}

/*
 *  export one panel
 */
void
im_pop_export_to_img_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( wp ) {
      UserData *ud = wp->ud;
      ImgDialogData *pdata; 

      pdata = g_new0 (ImgDialogData, 1);
      pdata->numpanel = pa_panel_find_pos(wp);
      pdata->ud = ud;
      pdata->wp = wp;
      
      if ( im_save_img_file_dialog (pdata) == GTK_RESPONSE_ACCEPT) {
	 da_drawing_redraw(wp->drawing);
//      getchar();
	 g_timeout_add (ud->up->exportTimeout,
			im_export_panel_to_img_iddle, ( gpointer) pdata);
      }
   }
}

GdkPixbuf *im_export_create_pixbuf (UserData *ud);

/*
 *  called one, when gtk is iddle to grab a pixbuf and save it to file
 */
gboolean
im_export_to_img_iddle (gpointer data )
{
   GdkPixbuf     *pixbuf;
   ImgDialogData *pdata = (ImgDialogData *) data;
   UserData *ud  = pdata->ud;
      
   pixbuf = im_export_create_pixbuf ( ud);
   if ( pixbuf ){
      im_save_img_file ( pdata, pixbuf );
      g_object_unref (pixbuf);
   } else {
      msg_dbg("Getting pixbuf from window failed");
   }
   app_free(pdata->filename);
   app_free(pdata->imgFmt);
   g_free (pdata);

   return FALSE; /* the source will be removed. */ 
}

/*
 *  export all panels
 *  callback from menu item.
 */
void
im_export_panels_img_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ImgDialogData *pdata; 

   pdata = g_new0 (ImgDialogData, 1);
   pdata->numpanel = -1;
   pdata->ud = ud;
   if ( im_save_img_file_dialog (pdata) == GTK_RESPONSE_ACCEPT) {
      g_timeout_add (ud->up->exportTimeout,
		     im_export_to_img_iddle, ( gpointer) pdata);
   }
}

void im_export_panels_img_cmd (char *file, char *format, UserData *ud )
{
   ImgDialogData *pdata; 

   pdata = g_new0 (ImgDialogData, 1);
   pdata->numpanel = -1;
   pdata->ud = ud;
   pdata->filename = app_strdup(file);
   pdata->imgFmt = app_strdup(format);
   GdkWindow *window = gtk_widget_get_window (ud->window);
   gdk_window_raise (window);

   g_timeout_add (ud->up->exportTimeout,
		  im_export_to_img_iddle, ( gpointer) pdata);
}


void print_rect( GtkWidget *widget, char *name)
{
   GtkAllocation walloc;

   gtk_widget_get_allocation (widget, &walloc);
   fprintf( stderr, "%s x = %d, y = %d width = %d height = %d\n", name,
	    walloc.x,
	    walloc.y,
	    walloc.width,
	    walloc.height );
}

static GdkPixbuf *im_add_border_to_shot (GdkPixbuf *pixbuf);
#if 0
static GdkPixbuf *im_remove_shaped_area (GdkPixbuf *pixbuf, Window window);
#endif

/*
 * TO SEE
 * GdkPixmap *gtk_widget_get_snapshot (GtkWidget    *widget,
 *                        GdkRectangle *clip_rect)
 */
GdkPixbuf *im_export_create_pixbuf (UserData *ud)
{
   GdkPixbuf *pixbuf;
   GtkWidget *widget = ud->window;
   GdkWindow *w;
   gint x = 0, y = 0;
   gint width, height;
   GtkAllocation walloc;
   
   gtk_widget_show (widget);
   w = gtk_widget_get_window (widget);
   

   widget = ud->meas_hbox;
   gtk_widget_get_allocation (widget, &walloc);
//   print_rect( widget, "ud->meas_hbox");
   x = walloc.x;
   y = walloc.y;
   width = walloc.width;
   height = y;

   widget = ud->xlabel_ev_box;
   gtk_widget_get_allocation (widget, &walloc);
//   print_rect( widget, "ud->xlabel_ev_box");
   height = walloc.y - height + walloc.height;

//   fprintf( stderr, "x = %d, y = %d width = %d height = %d\n",
//            x, y, width, height);
   pixbuf = gdk_pixbuf_get_from_window (w, x, y, width, height);

   pixbuf = im_add_border_to_shot (pixbuf);
   return pixbuf;
}


/*
 * stealed from Gtk+ docs/tools/shooter.c
 */
static GdkPixbuf *im_add_border_to_shot (GdkPixbuf *pixbuf)
{
   GdkPixbuf *newpixbuf;

   newpixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
			    gdk_pixbuf_get_width (pixbuf) + 2,
			    gdk_pixbuf_get_height (pixbuf) + 2);

   /* Fill with solid black */
   gdk_pixbuf_fill (newpixbuf, 0xFF);
   gdk_pixbuf_copy_area (pixbuf,
			 0, 0,
			 gdk_pixbuf_get_width (pixbuf),
			 gdk_pixbuf_get_height (pixbuf),
			 newpixbuf, 1, 1);

   g_object_unref (pixbuf);
   return newpixbuf;
}

#if 0
static GdkPixbuf *
im_remove_shaped_area (GdkPixbuf *pixbuf, Window window)
{
   GdkPixbuf *newpixbuf;
   XRectangle *rectangles;
   int rectangle_count, rectangle_order;
   GdkDisplay *display;
   int i;

   newpixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
			    gdk_pixbuf_get_width (pixbuf),
			    gdk_pixbuf_get_height (pixbuf));
   
   gdk_pixbuf_fill (newpixbuf, 0);
   display = gdk_display_get_default ();
   rectangles = XShapeGetRectangles (GDK_DISPLAY_XDISPLAY(display), window,
				     ShapeBounding, &rectangle_count, &rectangle_order);

   for (i = 0; i < rectangle_count; i++) {
      int y, x;

      for (y = rectangles[i].y; y < rectangles[i].y + rectangles[i].height; y++) {
	 guchar *src_pixels, *dest_pixels;
	 
	 src_pixels = gdk_pixbuf_get_pixels (pixbuf) +
	    y * gdk_pixbuf_get_rowstride (pixbuf) +
	    rectangles[i].x * (gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3);
	 dest_pixels = gdk_pixbuf_get_pixels (newpixbuf) +
	    y * gdk_pixbuf_get_rowstride (newpixbuf) +
	    rectangles[i].x * 4;

	 for (x = rectangles[i].x ; x < rectangles[i].x + rectangles[i].width ; x++) {
	    *dest_pixels++ = *src_pixels ++;
	    *dest_pixels++ = *src_pixels ++;
	    *dest_pixels++ = *src_pixels ++;
	    *dest_pixels++ = 255;

	    if (gdk_pixbuf_get_has_alpha (pixbuf)) {
	       src_pixels++;
	    }
	 }
      }
   }

   g_object_unref (pixbuf);
   return newpixbuf;
}
#endif
