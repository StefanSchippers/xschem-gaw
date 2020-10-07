
/*
 * gawsnd.c - Gtk analog waveform viewer
 *   sound configuration and capture.
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

#include <gtk/gtk.h>

#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 *** \brief Allocates memory for a new GawSndData object.
 */

GawSndData *as_sound_new( UserData *ud )
{
   GawSndData *snd = NULL;
   
#ifdef HAVE_LIBASOUND   
   snd =  app_new0(GawSndData, 1);
   as_sound_construct( snd, ud );
   app_class_overload_destroy( (AppClass *) snd, as_sound_destroy );
#endif /* HAVE_LIBASOUND */
   return snd;
}

/** \brief Constructor for the GawSndData object. */
#ifdef HAVE_LIBASOUND   

void as_sound_construct( GawSndData *snd, UserData *ud )
{
   char name[32];
   
   app_class_construct( (AppClass *) snd );
   
   snd->ud = ud;
   snd->sparams = app_new0( SoundParams, 1);

   SoundParams *sparams =  snd->sparams ;
   sparams->indexstr = app_strdup(ud->up->cardidstr);
   if ( *ud->up->cardidstr == '-' ) {
      sparams->device_name = app_strdup("default");
   } else {
      sprintf(name, "hw:%s", ud->up->cardidstr);
      sparams->device_name = app_strdup(name);
   }
   sparams->duration = ud->up->duration;
   sparams->rate = ud->up->rate;
   sparams->input = ud->up->input;
   sparams->format = ud->up->pcm_format;
   sparams->start_delay = 1;
   /* internal options */
   sparams->channels = 2;
   sparams->stream = SND_PCM_STREAM_CAPTURE;
   sparams->open_mode = 0;
}
#endif /* HAVE_LIBASOUND */

/** \brief Destructor for the GawSndData object. */

void as_sound_destroy(void *snd)
{

   if (snd == NULL) {
      return;
   }
#ifdef HAVE_LIBASOUND
   GawSndData *this = (GawSndData *) snd;
   
   app_free(this->sparams->indexstr);
   app_free(this->sparams->device_name);
   app_free(this->sparams);

   app_class_destroy( snd );
#endif /* HAVE_LIBASOUND */
}

#ifdef HAVE_LIBASOUND

typedef struct _AsFormatList AsFormatList;
struct _AsFormatList {
   char *name;
   snd_pcm_format_t format;
};

/*
 * If you modify here, you shouls modify sndparams.c sound_getval_sound()
 */
AsFormatList as_format_list[] = {
   { "Signed 8 bits",           SND_PCM_FORMAT_S8        },
   { "Unsigned 8 bits",         SND_PCM_FORMAT_U8        },
   { "Signed 16 bits LE",       SND_PCM_FORMAT_S16_LE    },
   { "Signed 16 bits BE",       SND_PCM_FORMAT_S16_BE    },
//   { "Unsigned 16 bits LE",     SND_PCM_FORMAT_U16_LE    },
//   { "Unsigned 16 bits BE",     SND_PCM_FORMAT_U16_BE    },
//   { "Signed 24 bits LE",       SND_PCM_FORMAT_S24_LE    },
//   { "Signed 24 bits BE",       SND_PCM_FORMAT_S24_BE    },
//   { "Unsigned 24 bits LE",     SND_PCM_FORMAT_U24_LE    },
//   { "Unsigned 24 bits BE",     SND_PCM_FORMAT_U24_BE    },
   { "Signed 32 bits LE",       SND_PCM_FORMAT_S32_LE    },
   { "Signed 32 bits BE",       SND_PCM_FORMAT_S32_BE    },
//   { "Unsigned 32 bits LE",     SND_PCM_FORMAT_U32_LE    },
//   { "Unsigned 32 bits BE",     SND_PCM_FORMAT_U32_BE    },
   { NULL,         0                        },
};

char *as_rate_list[] = {
   "8000",
      "22050",
      "44100",
      "48000",
      "96000",
      "192000",
      NULL,
};
      
static void
as_setup_format_combo(GawSndData *snd)
{
   GtkWidget *combo = snd->w_format;
   snd_pcm_format_t format = snd->sparams->format;
   snd_pcm_t *pcm;
   snd_pcm_hw_params_t *hwparams;
   snd_pcm_format_mask_t *formatMask;
   AsFormatList *p;
   int i ;
   int err;

   for ( i = snd->n_format ; i > 0; i-- ){
      gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (combo), i - 1 );
   }
   snd->n_format = 0;

   snd_pcm_hw_params_alloca(&hwparams);
   snd_pcm_format_mask_alloca(&formatMask);

   err = snd_pcm_open(&pcm, snd->sparams->device_name, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
   if (  err < 0 ) {
      return;
   }
   err = snd_pcm_hw_params_any(pcm, hwparams);
   if ( err < 0 ) {
      snd_pcm_close(pcm);
      return;
   }
   snd_pcm_hw_params_get_format_mask(hwparams, formatMask);
 
   for( p = as_format_list, i = 0 ; p->name ; p++, i++ ){
      if ( snd_pcm_format_mask_test ( formatMask, p->format ) < 0 ){
	 continue;
      }
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), p->name );
//      msg_dbg("as_format %d '%s'", snd->n_format, p->name );
      snd->n_format++;
      if ( p->format == format ) {
	 gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
      }
   }
   snd_pcm_close(pcm);
}

static void
as_setup_rate_combo (GawSndData *snd)
{
   unsigned int rate = snd->sparams->rate;
   GtkWidget *combo = snd->w_rate;
   snd_pcm_t *pcm;
   snd_pcm_hw_params_t *hwparams;
   char **p;
   unsigned int val;
   int i ;
   int err;

   for ( i = snd->n_rate ; i > 0; i-- ){
      gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (combo), i - 1 );
   }
   snd->n_rate = 0;

   snd_pcm_hw_params_alloca(&hwparams);
   err = snd_pcm_open(&pcm, snd->sparams->device_name, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
   if (  err < 0 ) {
      return;
   }
   err = snd_pcm_hw_params_any(pcm, hwparams);
   if ( err < 0 ) {
      snd_pcm_close(pcm);
      return;
   }
      
   for ( p = as_rate_list, i = 0 ; *p ; p++, i++ ){
      val = (unsigned int) g_ascii_strtoull (  *p, NULL, 10 );
      if ( snd_pcm_hw_params_test_rate ( pcm, hwparams, val, 0 ) < 0 ){
	 continue;
      }
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), *p);
//      msg_dbg("as_rate %d '%s'", snd->n_rate, *p );
      snd->n_rate++;
      if ( val == rate ) {
	 gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
      }
   }
   snd_pcm_close(pcm);
}


static void
as_setup_card_combo (GtkWidget *combo , SoundParams *sparams)
{
   int card;
   int i ;
   int err;
   snd_ctl_t *handle;
   snd_ctl_card_info_t *info;
   char name[32];
   
   gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "- default");
   
   snd_ctl_card_info_alloca(&info);
   sparams->firstcard = -1;
   card = -1;
   for( i = 0 ;   ; i++ ){
      if (snd_card_next(&card) < 0 || card < 0) {
	 if ( i == 0 ) {
	    msg_error(_("no soundcards found..."));
	 }
	 return;
      }
      if ( sparams->firstcard < 0 ) {
	  sparams->firstcard = card;
      }
      sprintf(name, "hw:%d", card);
      if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
	 msg_error(_("control open (%i): %s"), card, snd_strerror(err));
	 continue;
      }
      if ((err = snd_ctl_card_info(handle, info)) < 0) {
	 msg_error(_("control hardware info (%i): %s"), card, snd_strerror(err));
	 snd_ctl_close(handle);
	 continue;
      }
      sprintf(name, "%d %s", card,  snd_ctl_card_info_get_name(info) );
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), name );
      snd_ctl_close(handle);
   }
}

/*
 * The purpose of this function is to let the user to select 
 *   the input where he plugged his signals.
 * Now if user select "default", as device, we have to select the input
 *   on the mixer of the card used by  "default".
 * What is the card used by device  "default" ?
 */ 
static void
as_setup_input_combo (GtkWidget *combo, SoundParams *sparams)
{
   snd_mixer_t *handle;
   int i ;
   snd_mixer_selem_id_t *sid;
   snd_mixer_elem_t *elem;
   snd_mixer_selem_id_alloca(&sid);
   char *devname;
   char name[32];

   devname = sparams->device_name;
   if ( *sparams->indexstr == '-' ) {
      sprintf(name, "hw:%d", sparams->firstcard);
      devname = name;
   }
   handle = sound_open_mixer(devname);
   if ( handle == NULL ){
      return ;
   }   

   for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) {
      snd_mixer_selem_get_id(elem, sid);
      if (! snd_mixer_selem_is_active(elem)) {
	 continue;
      }
      const char *control_name = snd_mixer_selem_id_get_name(sid);
//      printf("Simple mixer control '%s',%i\n", control_name,  snd_mixer_selem_id_get_index(sid));
      if (snd_mixer_selem_is_enum_capture(elem)) {
	 int items;
	 char itemname[40];
	 app_free(sparams->control_name);
	 sparams->control_name = app_strdup(control_name);
	 items = snd_mixer_selem_get_enum_items(elem);
//	 printf("  Items:");
	 for (i = 0; i < items; i++) {
	    snd_mixer_selem_get_enum_item_name(elem, i, sizeof(itemname) - 1, itemname);
//	    printf(" '%s'", itemname);
            gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), itemname );
	    if ( i == sparams->input ) {
	       gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
	    }
	 }
//	 printf("\n");
      }
   }
   snd_mixer_close(handle);
   return ;
}

static void
as_duration_entry_changed_cb (GtkWidget *widget, gpointer pdata)
{
   GawSndData *snd = (GawSndData *) pdata; 
   UserPrefs *up = snd->ud->up;
   char text[64];
   double val = g_strtod (gtk_entry_get_text (GTK_ENTRY (widget)), NULL );

   if ( val != snd->sparams->duration ) {			  
      snd->sparams->duration = val;
      snd->sparams->numsamples = (unsigned int) val * snd->sparams->rate;
      sprintf( text, "%u",  snd->sparams->numsamples );
      gtk_entry_set_text (GTK_ENTRY (snd->w_size), text);
      msg_dbg(_("duration changed: text = '%s', val =  %0.2f"), text, val );
      up->duration = val;
   }
}

static void
as_size_entry_changed_cb (GtkWidget *widget, gpointer pdata)
{
   GawSndData *snd = (GawSndData *) pdata; 
   char text[64];
   unsigned int val = (unsigned int) g_ascii_strtoull ( gtk_entry_get_text (GTK_ENTRY (widget)), NULL, 10 );

   if ( val == snd->sparams->numsamples ) {			  
      val = (unsigned int) snd->sparams->duration * snd->sparams->rate;
      if ( val != snd->sparams->numsamples ) {
	 snd->sparams->numsamples = val;
	 sprintf( text, "%u", val );
	 gtk_entry_set_text (GTK_ENTRY (snd->w_size), text);
      }
   } else { /* input in this entry */
      snd->sparams->numsamples = val;
      sprintf( text, "%0.2f", (double)  snd->sparams->numsamples / snd->sparams->rate );
      gtk_entry_set_text (GTK_ENTRY (snd->w_duration), text);
   }
   msg_dbg(_("size changed: text = '%s', val =  %0.2f"), text, val );
}

static void
as_input_changed_cb (GtkWidget *widget, gpointer pdata)
{
   GawSndData *snd = (GawSndData *) pdata; 
   UserPrefs *up = snd->ud->up;
   SoundParams *sparams = snd->sparams;
   char *text;

   sparams->input = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
   text = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (widget));
   sound_mixer_set( sparams->device_name, sparams->control_name, text);
   msg_dbg(_("input changed: text = '%s', control = '%s', val = %d"), text, sparams->control_name, sparams->input );
   up->input = sparams->input;   
}

static void
as_card_changed_cb (GtkWidget *widget, gpointer pdata)
{
   GawSndData *snd = (GawSndData *) pdata; 
   UserPrefs *up = snd->ud->up;
   SoundParams *sparams = snd->sparams;
   char name[32] ;
   char *p;
   char *text;

   text = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (widget));
   app_free(sparams->indexstr);
   app_free(sparams->device_name);
   if ( ( p = strchr( text, ' ' )) ){
      *p = 0;
   }
   sparams->indexstr = app_strdup(text);
   if ( *text == '-' ){
      sparams->device_name = app_strdup("default");
   } else {
      sprintf(name, "hw:%s", text );
      sparams->device_name = app_strdup(name);
   }
   msg_dbg(_("card changed: idex = %s device = %s"),  sparams->indexstr, sparams->device_name );
   
   app_free(up->cardidstr);
   up->cardidstr = app_strdup(sparams->indexstr);
   as_setup_rate_combo( snd );
   as_setup_format_combo(snd);
}

static void
as_format_changed_cb (GtkWidget *widget, gpointer pdata)
{
   GawSndData *snd = (GawSndData *) pdata; 
   UserPrefs *up = snd->ud->up;
   AsFormatList *p = as_format_list;
   gint i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

   snd->sparams->format = (p + i)->format;
   msg_dbg(_("format changed: text =  %s, val = %d"), (p + i)->name, snd->sparams->format );
   up->pcm_format = snd->sparams->format;
}

static void
as_rate_changed_cb (GtkWidget *widget, gpointer pdata)
{
   GawSndData *snd = (GawSndData *) pdata; 
   UserPrefs *up = snd->ud->up;
   const gchar *text;

   GtkWidget *entry = gtk_bin_get_child (GTK_BIN (widget));

   text = gtk_entry_get_text (GTK_ENTRY (entry));
   snd->sparams->rate = (unsigned int ) g_ascii_strtoull ( text, NULL, 10 );
   if ( snd->w_size ){
      g_signal_emit_by_name(snd->w_size, "changed");
   }
   msg_dbg(_("rate changed: text = %s, val =  %d"),  text, snd->sparams->rate );
   up->rate = snd->sparams->rate;
}

void as_sound_win_create(GawSndData *snd)
{
   UserData *ud = snd->ud;
   SoundParams *sparams = snd->sparams;
   GtkWidget *dialog;
   GtkWidget *vbox;
   gint response;
   GtkWidget *frame;
   GtkWidget *combo;
   GtkWidget *c_table;
   GtkWidget *s_table;
   GtkWidget *label;
   GtkWidget *entry;
   gchar *str;
   char text[64];
   
   dialog = gtk_dialog_new_with_buttons (_("Sound card settings and Capture"),
					 GTK_WINDOW (ud->window),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					  _("_Cancel"),
                                         GTK_RESPONSE_REJECT,
                                         _("_OK"),
                                         GTK_RESPONSE_ACCEPT,
					 NULL);
   vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);

   /* frame Sound card setting */
   frame = gtk_frame_new (_(" Sound card setting"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
   c_table = gtk_grid_new ();
   gtk_grid_set_column_spacing( GTK_GRID(c_table), 5);
   gtk_grid_set_row_spacing( GTK_GRID (c_table), 2);
   gtk_container_add (GTK_CONTAINER (frame), c_table);

   /*   title */   
   str = _("Card");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(c_table), label, 
                /* left,    top,  width,   height    */
                      0,      0,      1,        1  );   

   str = _("Input");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(c_table), label, 
                /* left,    top,  width,   height    */
                      1,      0,      1,        1  );   

   /*   Card */   
   combo = gtk_combo_box_text_new ();
   snd->w_card = combo;
   as_setup_card_combo (combo, sparams);
   g_signal_connect (combo, "changed",
		     G_CALLBACK (as_card_changed_cb),
		     ( gpointer) snd );
   gtk_grid_attach(GTK_GRID(c_table), combo, 
                /* left,    top,  width,   height    */
                      0,      1,      1,        1  );   

   /*   Input */   
   combo = gtk_combo_box_text_new ();
   as_setup_input_combo (combo, sparams);
   g_signal_connect (combo, "changed",
		     G_CALLBACK (as_input_changed_cb),
		     ( gpointer) snd );
   gtk_grid_attach(GTK_GRID(c_table), combo, 
                /* left,    top,  width,   height    */
                      1,      1,      1,        1  );   

   /*   title */   
   str = _("Sample Rate");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(c_table), label, 
                /* left,    top,  width,   height    */
                      0,      2,      1,        1  );   

   str = _("Sample Format");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(c_table), label, 
                /* left,    top,  width,   height    */
                      1,      2,      1,        1  );   

   /*   rate */
   combo = gtk_combo_box_text_new_with_entry ();
   snd->w_rate = combo;
   entry = gtk_bin_get_child (GTK_BIN (combo));
   gtk_entry_set_alignment (GTK_ENTRY (entry), 1); /* right */

//   as_setup_rate_combo (snd);
   g_signal_connect (combo, "changed", G_CALLBACK (as_rate_changed_cb),
		     (gpointer) snd );
   gtk_grid_attach(GTK_GRID(c_table), combo, 
                /* left,    top,  width,   height    */
                      0,      3,      1,        1  );   
   sprintf(text, "%d", sparams->rate);
   gtk_entry_set_text (GTK_ENTRY (entry), text );

   /*   format */   
   combo = gtk_combo_box_text_new ();
   snd->w_format = combo;
//   as_setup_format_combo (snd);
   g_signal_connect (combo, "changed",
		     G_CALLBACK (as_format_changed_cb),
		     ( gpointer) snd );
   gtk_grid_attach(GTK_GRID(c_table), combo, 
                /* left,    top,  width,   height    */
                      1,      3,      1,        1  );   

   /* fire the card setting callback */
   int idx = 0;
   if ( *sparams->indexstr != '-' ){ 
      idx = 1 + atoi(sparams->indexstr);
   }
   gtk_combo_box_set_active (GTK_COMBO_BOX (snd->w_card), idx );


   /* frame Capture Settings */
   frame = gtk_frame_new (_(" Capture Settings"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
   s_table =  gtk_grid_new ();
   gtk_grid_set_column_spacing( GTK_GRID(s_table), 5);
   gtk_grid_set_row_spacing( GTK_GRID (s_table), 2);
   gtk_container_add (GTK_CONTAINER (frame), s_table);

   str = _("Duration : Seconds");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(s_table), label, 
                /* left,    top,  width,   height    */
                      0,      0,      1,        1  );

   str = _("Sample number");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(s_table), label, 
                /* left,    top,  width,   height    */
                      1,      0,      1,        1  );

   /* Size */
   entry = gtk_entry_new() ;
   snd->w_size = entry;
   gtk_entry_set_alignment (GTK_ENTRY (entry), 1); /* right */
   g_signal_connect (entry, "changed",
		     G_CALLBACK (as_size_entry_changed_cb),
		     ( gpointer) snd );
   gtk_grid_attach(GTK_GRID(s_table), entry, 
                /* left,    top,  width,   height    */
                      1,      1,      1,        1  );

   /* Duration */
   entry = gtk_entry_new() ;
   snd->w_duration = entry;
   gtk_entry_set_alignment (GTK_ENTRY (entry), 1); /* right */
   g_signal_connect (entry, "changed",
		     G_CALLBACK (as_duration_entry_changed_cb),
		     ( gpointer) snd);
   gtk_grid_attach(GTK_GRID(s_table), entry, 
                /* left,    top,  width,   height    */
                      0,      1,      1,        1  );
   sprintf(text, "%0.2f", sparams->duration);
   sparams->duration -= 1.0;
   gtk_entry_set_text (GTK_ENTRY (entry), text);

   gtk_widget_show_all (vbox);
   response = gtk_dialog_run (GTK_DIALOG (dialog));

   if (response == GTK_RESPONSE_ACCEPT) {
      msg_dbg(_("dialog OK loading fom sound card"));
      
      DataFile *wdata = datafile_new( ud, "SoundCard" );
      datafile_set_sound(wdata, sparams);

      ap_load_wave ( wdata );
   }

   gtk_widget_destroy (dialog);

   /*
    * widget are suppressed
    *  need this for next creation
    */
   snd->w_card = 0;
   snd->w_rate = 0;
   snd->n_rate = 0;
   snd->w_format = 0;
   snd->n_format = 0;
   snd->w_duration = 0;
   snd->w_size = 0;
}


#else  /* HAVE_LIBASOUND */

void as_sound_win_create(GawSndData *snd)
{
}
#endif /* HAVE_LIBASOUND */
