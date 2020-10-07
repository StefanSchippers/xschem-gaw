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

#include <gtk/gtk.h>

#include <gaw.h>
#include <duprintf.h>
#include <fileutil.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

typedef struct _FsDialogData {
   UserData *ud;
   int option;
   GSList *files;
   char *filename;
   char *format;
   GtkFileChooser *chooser; /* pointer to dialog for callbacks */
   GtkFileFilter *filter; /* file filter associated with fileType */
   GtkFileFilter *alldatafilter; /* file filter associated with all data */
} FsDialogData;

/*
 * filter associated with fileType
 */
static void
af_fs_filter_change(GtkFileChooser *chooser,  FsDialogData *fsdata )
{
   GtkFileFilter *filter;
   char *name;

   /* file filter for active file type (*.ihx) */
   if ( fsdata->filter){
       gtk_file_chooser_remove_filter(chooser, fsdata->filter);
   }

   filter = gtk_file_filter_new ();
   fsdata->filter = filter;
   name = app_strdup_printf(_("%s formats"), fsdata->format );
   gtk_file_filter_set_name (filter, name);
   app_free(name);
   char **listpat = fileformat_get_patterns(fileformat_get_index(fsdata->format));
   while(*listpat){
      name = app_strdup_printf("*.%s", *listpat );
      gtk_file_filter_add_pattern (filter, name);
      app_free(name);
      listpat++;
   }
   gtk_file_chooser_add_filter (chooser, filter);
}


static void
af_fmt_radio_clicked (GtkWidget *w,  gpointer pdata )
{
   FsDialogData *fsdata = (FsDialogData *) pdata;
   UserPrefs *up = fsdata->ud->up;
   GtkFileFilter *filter;

    if ( ! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) ){
       return;
    }

   char *format = (gchar *) gtk_button_get_label (GTK_BUTTON(w)) ;
   app_dup_str( &fsdata->format, format);
   if (app_strcmp( format, up->dataFileFormat) ) {
      app_dup_str( &up->dataFileFormat, format);
   }
   if ( app_strcmp(fsdata->format, "auto") == 0 ){
      filter = fsdata->alldatafilter;
   } else {
      af_fs_filter_change(fsdata->chooser, fsdata );
      filter =  fsdata->filter;
   }
   gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fsdata->chooser), filter);

//   msg_dbg ("af_fmt_radio_clicked \"%s\" ", format );
}

static void
af_fs_filter_add (GtkFileChooser *chooser, FsDialogData *fsdata )
{
   GtkFileFilter *filter;
   char *name;
   int i;

   /* file filter for all data type files (*.dat ...) */
   filter = gtk_file_filter_new ();
   gtk_file_filter_set_name (filter, _("All Data formats"));
   i = -1 ; 
   do {
      name = fileformat_get_next_name( &i, fsdata->option);
      if ( ! name ){
         continue;
      }
      char **listpat = fileformat_get_patterns(i);
      while(listpat && *listpat){
         name = app_strdup_printf("*.%s", *listpat );
         gtk_file_filter_add_pattern (filter, name);
         app_free(name);
         listpat++;
      }
   } while ( i != -1 );
   gtk_file_chooser_add_filter (chooser, filter);
   fsdata->alldatafilter = filter;
   
   /* file filter that match any file */
   filter = gtk_file_filter_new ();
   gtk_file_filter_set_name (filter, _("All files"));
   gtk_file_filter_add_pattern (filter, "*");
   gtk_file_chooser_add_filter (chooser, filter);
}
  
GtkWidget *
af_create_fmt_radio_widget( GtkFileChooser *chooser, FsDialogData *fsdata )
{
   UserData *ud = fsdata->ud;
   GSList *group = NULL;
   GtkWidget *button = NULL;
   GtkWidget *activebut = NULL;
   GtkWidget *hbox;
   GtkWidget *frame;

   fsdata->chooser = chooser;
   int selBut = fileformat_get_index(ud->format);
   int i = -1;
   char *name;

   frame = gtk_frame_new (_("Data Formats"));
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
   gtk_container_add (GTK_CONTAINER (frame), hbox);

   do {
      name = fileformat_get_next_name( &i, fsdata->option);
      if ( ! name ){
	 continue;
      }
      button = gtk_radio_button_new_with_label (group, name);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      g_signal_connect (button, "clicked",
			G_CALLBACK(af_fmt_radio_clicked), 
			(gpointer) fsdata );
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
      if ( i == selBut ){
	 activebut = button;
      }
   } while ( i != -1 );
   if ( fsdata->option == FILE_WRITE_OP && selBut < 0 ) {
      activebut = button;
   }

      /* add filters */
   af_fs_filter_add (chooser, fsdata);

   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (activebut), FALSE);
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (activebut), TRUE);
   gtk_widget_show (hbox);
   return frame;
}

static int
af_create_open_file_selection (FsDialogData *fsdata )
{
   GtkWidget *dialog;
   gint response;
   GtkWidget *frame;
   GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
   gchar *button_text = _("_Open");
   
   if ( fsdata->option == FILE_WRITE_OP ){
      action = GTK_FILE_CHOOSER_ACTION_SAVE;
      button_text = _("_Save");
   }
  
   dialog = gtk_file_chooser_dialog_new (_("Select file"),
					 GTK_WINDOW (fsdata->ud->window),
					 action,
					  _("_Cancel"), GTK_RESPONSE_REJECT,
					 button_text, GTK_RESPONSE_ACCEPT,
					 NULL);
   if ( fsdata->option == FILE_READ_OP ){
      gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
   }
   frame = af_create_fmt_radio_widget(GTK_FILE_CHOOSER (dialog), fsdata);
   gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), frame);
   
   gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
   if ( fsdata->filename ) {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), fsdata->filename);
   }
   response = gtk_dialog_run (GTK_DIALOG (dialog));
   if (response == GTK_RESPONSE_ACCEPT) {
      fsdata->files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
#ifdef TRACE_MEM
      GSList   *list = fsdata->files;
      while (list) {
         GSList *next = list->next;
         tmem_alloced(list->data, 0);   /* trace mem */
         list = next;
      }
#endif
   }
   
   gtk_widget_destroy (dialog);
   return response;
}

void
af_open_file_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data)
{
   UserData *ud = (UserData *) user_data;
   FsDialogData *fsdata = g_new0 (FsDialogData, 1);

   fsdata->option = FILE_READ_OP;
   fsdata->ud = ud;
   if ( ud->listFiles ){
      fsdata->filename = (char *) g_slist_nth_data ( ud->listFiles, 0 ) ;
   }

   if (af_create_open_file_selection(fsdata) == GTK_RESPONSE_ACCEPT) {
      af_list_files_free( &ud->listFiles);
      ud->listFiles = fsdata->files;
      app_dup_str(&ud->format, fsdata->format);
      af_load_wave_file (ud);

   }
   app_free(fsdata->format);
   g_free(fsdata);
}

void
af_export_displayed_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   WaveTable *wt = ap_wavetable_new_from_displayed( ud );
   
//   msg_dbg("called");
   if ( ! wt ) {
      msg_info(_("\n\nNo waves displayed"));
      return;
   }
   FsDialogData *fsdata = g_new0 (FsDialogData, 1);

   fsdata->option = FILE_WRITE_OP;
   fsdata->ud = ud;
   if ( ud->listFiles ){
      fsdata->filename = (char *) g_slist_nth_data ( ud->listFiles, 0 )  ;
   }
   
   if (af_create_open_file_selection(fsdata) == GTK_RESPONSE_ACCEPT) {
      wt->bits = ud->bits;
      wt->rate = ud->rate;

      fileformat_file_write( wt, (char *) fsdata->files->data, fsdata->format, ud->printFmt );
   }
   wavetable_destroy(wt);
   app_free(fsdata->format);
   g_free(fsdata);
}

int 
af_export_displayed_data_cmd (char *filename, char *format, UserData *ud )
{
   WaveTable *wt = ap_wavetable_new_from_displayed( ud );
   int ret;
   
   if ( ! wt ) {
//      msg_info( "\n\nNo waves displayed");
      return -1;
   }
   
   wt->bits = ud->bits;
   wt->rate = ud->rate;

   ret = fileformat_file_write( wt, filename, format, ud->printFmt );
   wavetable_destroy(wt);
   return ret;
}


void
af_vl_export_data_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   DataFile *wdata = (DataFile *) user_data;
   UserData  *ud = wdata->ud;
   
//   msg_dbg("called");

   FsDialogData *fsdata = g_new0 (FsDialogData, 1);

   fsdata->option = FILE_WRITE_OP;
   fsdata->ud = ud;
   if ( ud->listFiles ){
      fsdata->filename = (char *) g_slist_nth_data ( ud->listFiles, 0 )  ;
   }
   if (af_create_open_file_selection(fsdata) == GTK_RESPONSE_ACCEPT) {
      wdata->wt->bits = ud->bits;
      wdata->wt->rate = ud->rate;

      fileformat_file_write( wdata->wt, (char *) fsdata->files->data, fsdata->format, ud->printFmt );
   }
   app_free(fsdata->format);
   g_free(fsdata);
}

char *af_mk_label( char *name)
{
   char *cp = name ;
   if (strlen(name) > 16) {
      cp = strrchr(name, '/');
      if (cp) {
	 cp++;
      } else {
	 cp = name;
      }
   }
   return cp;
}

void af_print_file(char *file)
{
   fprintf(stderr, "%s\n", file);
}
void af_list_files_free(GSList **list)
{
   if (*list) {
#ifdef TRACE_MEM
//      g_slist_foreach (*list, (GFunc) print_file, NULL);
      g_slist_foreach (*list, (GFunc) str_free_func, NULL);
#else
      g_slist_foreach (*list, (GFunc) app_free, NULL);
#endif
      g_slist_free(*list);
      *list = NULL;
   }
}
 
DataFile *
af_load_wave_file (UserData *ud)
{
   DataFile *wdata = NULL;
   
   if ( ! ud->listFiles) {
      return NULL;
   }
   GSList   *list = ud->listFiles;
   
   while (list) {
      GSList *next = list->next;

      char *filename = (char *) list->data;
      if (file_exists (filename)) {
         wdata = datafile_new( ud, af_mk_label(filename) );
         datafile_set_file(wdata, filename, ud->format);
         ap_load_wave ( wdata );
      } else {
         msg_warning(_("file '%s' do not exist."), filename );
      }
      list = next;
   }

   return wdata;
}

void af_unload_all_files (UserData *ud)
{
   while ( ud->wdata_list) {
      DataFile *wdata = (DataFile *) g_list_nth_data ( ud->wdata_list, 0);
      ap_delete_datafile(wdata);
   }
   fileformat_cleanup();
}
   
