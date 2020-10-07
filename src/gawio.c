/*
 * gawio.c - external command interface
 * 
 * include LICENSE
 */
#define _GNU_SOURCE 1   /* netdb.h */

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
#include <errno.h>

#include <gtk/gtk.h>

#include <gaw.h>
#include <sockcon.h>
#include <stutil.h>
#include <fileutil.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 * io data
 */
/* definition of struct _GawIoData moved to gaw.h */
typedef struct _Gaw_Io_Command Gaw_Io_Command;

enum _gawIoStateInfo {
   GAWIO_CMD = 0,
   GAWIO_ROWDATA, 
   GAWIO_COLDATA, 
};

struct _Gaw_Io_Command {
   char *name;
   int (*cmdHandler)(GawIoData *gawio, char *params);
   int check_wds;
};

DataFile *aio_wdata_get_from_name( GawIoData *gawio,  char *tblname)
{
   UserData *ud = gawio->ud;
   DataFile *wdata;
   char *name;
   
   GList *list = ud->wdata_list;   
   while (list) {
      GList *next = list->next;
 
      wdata = (DataFile *) list->data;
      name = wavetable_get_tblname (wdata->wt);
      if ( ! app_strcmp( tblname, name) ) {
	 return wdata;
      }
      list = next;
    }
   return NULL;
}

void aio_table_set_current(GawIoData *gawio, DataFile *wdata, char *tblname)
{
   gawio->wdata = wdata;
   app_free(gawio->curtbl);
   gawio->curtbl = app_strdup(tblname);
   gawio->wds = wavetable_get_cur_dataset(wdata->wt);
}

/*
 * add a col to the dataset
 */
static int aio_col_add( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *varName = stu_token_next( &pline, " ", " " );
   char *str = stu_token_next( &pline, " ", " " );
   
   WaveVar *var = ( WaveVar *) dataset_get_var_for_name(gawio->wds, varName );
   if ( var ) {
      gawio->msg = g_strdup_printf( _("Variable %s already defined"), varName );
      return -1;
   }
   dataset_var_add( gawio->wds, varName, wavevar_str2type(str), 1 );
   return 0;
}

/*
 * coldatas varName [start_row]
 */
static int aio_coldatas_add( GawIoData *gawio, char *pline )
{
   msg_dbg( "Fonction called %s", pline );
   char *varName = stu_token_next( &pline, " ", " " );
   char *str = stu_token_next( &pline, " ", " " );
   int row;
   
   gawio->currow = 0;
   if ( str && (row = atoi(str)) >= 0 ) {
      gawio->currow = row;
   }
   WaveVar *var = ( WaveVar *) dataset_get_var_for_name(gawio->wds, varName );
   if ( ! var ) {
      gawio->msg = g_strdup_printf( _("Variable %s not defined"), varName );
      return -1;
   }
   gawio->curcol = var->colno;
   gawio->state = GAWIO_COLDATA;
   return 0;
}

static int aio_coldata_process( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *tok;
   
   while ((tok = stu_token_next( &pline, " ", " " )) ) {
      if (strspn(tok, "0123456789eE+-.") != strlen(tok)) {
         msg_dbg( _("Expected number at row %d, col %d"),
                  gawio->currow, gawio->curcol );
         return -1;
      }
      double val = g_ascii_strtod(tok, NULL);
      dataset_col_val_add( gawio->wds, gawio->currow, gawio->curcol, val);
      gawio->currow++;
  }
   return 0;
}


static int aio_copyvar( GawIoData *gawio, char *pline )
{
   static GdkRGBA usercolor; /* stefan */
   msg_dbg("Fonction called %s", pline );
   UserData *ud = gawio->ud;
   
   char *varName = stu_token_next( &pline, " ", " " );
   char *panel = stu_token_next( &pline, " ", " " );
   char *color = stu_token_next( &pline, " ", " " ); /* stefan */
   
   int panelno = atoi(panel + 1);

   /* stefan */
   if( color ) {
      unsigned int r,g,b,a;
      int nitems;
      nitems = sscanf(color, "#%02x%02x%02x%02x", &r, &g, &b, &a);
      if(nitems < 4) a = 255;
      if(nitems < 3) color = NULL;
      usercolor.red = r/255.0;
      usercolor.green = g/255.0;
      usercolor.blue = b/255.0;
      usercolor.alpha = a/255.0;
   }
   /* /stefan */

   WaveVar *var = ( WaveVar *) dataset_get_var_for_name(gawio->wds, varName );
   WavePanel *wp =  (WavePanel *) g_list_nth_data (ud->panelList, panelno);
   
   if ( ! var ) {
      gawio->msg = g_strdup_printf( _("Variable %s not defined"), varName );
      return -1;
   }
   ap_panel_add_var(wp, var, NULL, color ? &usercolor : NULL); /* stefan */
   return 0;
}

/*
 * dataset 1
 *  add a dataset to the table or set it as default dataset
 */
static int aio_dataset_add( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *tok = stu_token_next( &pline, " ", " " );
   int num = atoi(tok);
   WDataSet *wds;
   
   if ( ( wds = wavetable_get_dataset( gawio->wdata->wt, num)) == NULL)  {
      wds = wavetable_add( gawio->wdata->wt);
   }
   gawio->wds = wds;
   return 0;
}

static int aio_delvar( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   UserData *ud = gawio->ud;
   
   char *varName = stu_token_next( &pline, " ", " " );
   char *panel = stu_token_next( &pline, " ", " " );
   int panelno = atoi(panel + 1);
   WaveVar *var = ( WaveVar *) dataset_get_var_for_name(gawio->wds, varName );
   WavePanel *wp =  (WavePanel *) g_list_nth_data (ud->panelList, panelno);
   
   ap_remove_all_wave_if_panel_and_var(wp, var);
   return 0;
}


static int aio_enddata( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   if ( ! gawio->wdata->wlist_win ) {
      datafile_create_list_win ( gawio->wdata);
   } else {
      datafile_recreate_list_win ( gawio->wdata);
   }
   return 0;
}

static int aio_export_img( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *file = stu_token_next( &pline, " ", " " );
   char *format = stu_token_next( &pline, " ", " " );

   im_export_panels_img_cmd (file, format, gawio->ud );
   return 0;
}

/*
 *  export displayed
 */
static int aio_export_data( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *file = stu_token_next( &pline, " ", " " );
   char *format = stu_token_next( &pline, " ", " " );

   if (af_export_displayed_data_cmd (file, format, gawio->ud ) ){
      gawio->msg = g_strdup_printf(_("Can't export to file %s format %s"),
				   file, format );
      return -1;
   }
   return 0;
}

static int aio_color_bg_set( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *color = stu_token_next( &pline, " ", " " );

   if (ac_color_background_cmd ( gawio->ud, color ) ){
      gawio->msg = g_strdup_printf(_("Bad spec for color %s"), color );
      return -1;
   }
   return 0;
}

static int aio_grid_set( GawIoData *gawio, char *pline )
{
   int is_on = 0;
   
   msg_dbg("Fonction called %s", pline );
   char *on = stu_token_next( &pline, " ", " " );

   if (app_strcmp( on, "on") == 0) {
      is_on = 1;
   }
   aw_show_grid_cmd (gawio->ud, is_on );
   return 0;
}

static int aio_logx_set( GawIoData *gawio, char *pline )
{
   int is_on = 0;
   
   msg_dbg("Fonction called %s", pline );
   char *on = stu_token_next( &pline, " ", " " );

   if (app_strcmp( on, "on") == 0) {
      is_on = 1;
   }
   gawio->ud->up->setLogX = is_on;
   al_label_set_logAxis( gawio->ud->xLabels, is_on );
   ap_all_redraw(gawio->ud);
   return 0;
}

/*
 * same as file/load
 */
static int aio_load_file( GawIoData *gawio, char *pline )
{
   DataFile *wdata;
   UserData *ud = gawio->ud;
   
   msg_dbg( "Fonction called %s", pline );
   char *file = stu_token_next( &pline, " ", " " );
   char *format = stu_token_next( &pline, " ", " " );

   af_list_files_free(&ud->listFiles);
   if (file_exists (file)) {
      ud->listFiles = g_slist_append( ud->listFiles, app_strdup( file ) );
   }
   app_free(ud->format);
   ud->format = app_strdup(format);
   
   wdata = af_load_wave_file (ud);
   /* table exist, set it current */
   if ( ! wdata ){
      /* create new table */
      gawio->msg = g_strdup_printf(_("Can't load file %s format %s"),
				   file, format );
      return -1;
   }
   char *tblname = wavetable_get_tblname(wdata->wt);
   aio_table_set_current(gawio, wdata, tblname);
   return 0;
}

static int aio_panel_add( GawIoData *gawio, char *pline )
{
   UserData *ud = gawio->ud;
   int i ;

   msg_dbg("Fonction called %s", pline );
   char *tok = stu_token_next( &pline, " ", " " );
   if ( ! tok ) {
      gawio->msg = app_strdup(_("Expecting number") );
      return -1;
   }
   int num = atoi(tok);
   int op = 0; /* add */
   int npanels = g_list_length( ud->panelList);
   
   if ( *tok == '+' || *tok == '-' ) {
      num = npanels + num;
   }
   if ( num < npanels ){
      if ( num < 2 ) {
	 num = 1;
      }
      op = 1;
      num = npanels - num;
   } else {
      num = num - npanels;
   }
   for ( i = 0 ; i < num ; i++ ){
      if ( op ){
	 ap_panel_remove_line(ud, NULL);
      } else {
	 ap_panel_add_line(ud, NULL, 0);
      }
   }
   return 0;
}

static int aio_rowdatas_add( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   gawio->state = GAWIO_ROWDATA;
   return 0;
}

static int aio_rowdata_process( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *tok;
   
   while ((tok = stu_token_next( &pline, " ", " " )) ) {
      if (strspn(tok, "0123456789eE+-.") != strlen(tok)) {
	 msg_dbg(_("Expected number at row %d, col %d"),
		  gawio->wds->nrows, gawio->wds->curcol );
	 return -1;
      }
      double val = g_ascii_strtod(tok, NULL);
      dataset_val_add( gawio->wds, val);
  }
   return 0;
}


/*
 * table_set <tblname>
 * set table current 
 */
static int aio_table_set( GawIoData *gawio, char *pline )
{
   DataFile *wdata;
   
   msg_dbg("Fonction called %s", pline );
   char *tblname = stu_token_next( &pline, " ", " " );
   wdata = aio_wdata_get_from_name(gawio, tblname);
   if ( ! wdata ){
      gawio->msg = g_strdup_printf(_("%s table not defined"), tblname );
      return -1;
   }
   if ( gawio->wdata ) { 
      if ( app_strcmp( tblname, gawio->curtbl) == 0 ) {
	 /* table exist and is current */
	 return 0;
      }
   }
   aio_table_set_current(gawio, wdata, tblname);
   return 0;
}

/*
 * table_new <tblname>
 * if table exist remove it and create a new one
 * add a new DataFile if table does not exist,
 *    set it cuurrent (make other command refer to it :
 *    this create a wavetable and a WDataSet
 */
static int aio_table_new( GawIoData *gawio, char *pline )
{
   DataFile *wdata;
   UserData *ud = gawio->ud;
   
   msg_dbg("Fonction called %s", pline );
   char *tblname = stu_token_next( &pline, " ", " " );
   wdata = aio_wdata_get_from_name(gawio, tblname);
   if ( wdata ) { 
      ap_delete_datafile(wdata);
   }
   /* create new table */
   wdata = datafile_new( ud, tblname );
   ud->wdata_list = g_list_prepend(ud->wdata_list, wdata);
   aio_table_set_current(gawio, wdata, tblname);
   return 0;
}

static int aio_table_del( GawIoData *gawio, char *pline )
{
   DataFile *wdata;
   
   msg_dbg("Fonction called %s", pline );
   char *tblname = stu_token_next( &pline, " ", " " );
  
   wdata = aio_wdata_get_from_name(gawio, tblname);
   if ( wdata ) {
       ap_delete_datafile(wdata);
   }
   return 0;
}


static int aio_variables_add( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *name;

   while ((name = stu_token_next( &pline, " ", " " )) ) {
      dataset_var_add( gawio->wds, name, UNKNOWN, 1 );
   }
   return 0;
}

static int aio_vartype_add( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   char *name;
   int col = 0;
   
   while ((name = stu_token_next( &pline, " ", " " )) ) {
      dataset_set_wavevar_type(gawio->wds, col++, wavevar_str2type(name) );
   }
   return 0;
}

static int aio_reload_all( GawIoData *gawio, char *pline )
{
   msg_dbg("Fonction called %s", pline );
   UserData *ud = gawio->ud;
   aw_reload_all_files_gaction (NULL, NULL, ud);
   return 0;
}

Gaw_Io_Command gaw_io_commands[] = {
   { "coladd",       aio_col_add,           1 },
   { "coldatas",     aio_coldatas_add,      1 },
   { "color_bg",     aio_color_bg_set,      0 },
   { "copyvar",      aio_copyvar,           1 },
   { "reload_all",   aio_reload_all,        1 }, /* stefan */
   { "dataset",      aio_dataset_add,       0 },
   { "delvar",       aio_delvar,            1 },
   { "enddata",      aio_enddata,           0 },
   { "export_img",   aio_export_img,        0 },
   { "export_data",  aio_export_data,       0 },
   { "grid",         aio_grid_set,          0 },
   { "load",         aio_load_file,         0 },
   { "logx",         aio_logx_set,          0 },
   { "panel",        aio_panel_add,         0 },
   { "rowdatas",     aio_rowdatas_add,      1 },
   { "tabledel",     aio_table_del,         0 },
   { "table_new",    aio_table_new,         0 },
   { "table_set",    aio_table_set,         0 },
   { "variables",    aio_variables_add,     1 },
   { "vartype",      aio_vartype_add,       1 },
   { NULL, NULL}
};

int aio_process_line( GawIoData *gawio, gchar *linebuf, gsize length)
{
   Gaw_Io_Command *ptab;
   int ret = 0;
   char *sn;
   char *s ;
   
//   fprintf( stderr, "Received: %s\n", linebuf);
   if ( ( sn = strchr(linebuf, '\n')) ){
      *sn = 0;
   }
   sn = linebuf ;

   switch ( gawio->state ) {
    case GAWIO_ROWDATA:
      if ( ( ret = aio_rowdata_process(gawio, sn)) ) {
	 break;
      }
      return 0;
    case GAWIO_COLDATA:
      if ( ( ret = aio_coldata_process(gawio, sn)) ) {
	 break;
      }
      return 0;
   }

   if ( ret ) {
      sn = linebuf ;
   }
   s = stu_token_next(&sn, " ", " " );
   if ( ! s ){
      return 0;
   }
   
   for ( ptab = gaw_io_commands; ptab->name; ptab++) {
      gawio->state = GAWIO_CMD;
      if ( ! app_strcasecmp( ptab->name, s) ) {
	 if (  ptab->check_wds ) {
	    if ( ! gawio->wds ) {
	       gawio->msg = app_strdup( _("Current table not defined") );
	       return -1;
	    }
	 }
	 ret = ptab->cmdHandler(gawio, sn);
	 return ret;
      }
   }
   gawio->msg = g_strdup_printf(_("Bad command: '%s'"), s );
   return -1;
}

gboolean
aio_accepted_read_cb(GIOChannel *source, GIOCondition condition,
                        gpointer data)
{
   GawIoData *gawio = (GawIoData *) data;
   gchar *linebuf;
   gsize length;
   GError *error = NULL;
   GIOStatus status;

   if ( condition == G_IO_IN ||  condition == G_IO_PRI ) {
//      msg_dbg( "aio_accepted_read_cb %d", condition ) ;
      status = g_io_channel_read_line ( source, &linebuf, &length, NULL, &error);
   
      if ( status == G_IO_STATUS_NORMAL ) {
	 aio_process_line( gawio, linebuf, length);
	 if ( ! gawio->msg ) {
	    con_send (gawio->cnx, "\n", 1, 0);
	 } else {
	    con_fmt_send (gawio->cnx, "%s\n", gawio->msg );
//	     msg_dbg( "%s", gawio->msg );
	    app_free(gawio->msg);
	    gawio->msg = NULL;
	 }

	 g_free(linebuf);
      } else if ( status == G_IO_STATUS_AGAIN ) {
	 return TRUE;
      } else {
	 msg_dbg(_("Closing server connection : status %d"), status );
	 con_destroy(gawio->cnx);
	 g_source_remove(gawio->sourceid);
	 g_io_channel_unref(gawio->iochannel);
	 return FALSE;
      }
   } else {
      msg_dbg(_("Unexpected Condition %d"), condition );
   }
   return TRUE;
}

gboolean
aio_listen_read_cb(GIOChannel *source, GIOCondition condition,
                        gpointer data)
{
//   fprintf( stderr, "io_listen_read_cb called\n" ) ;
   GawIoData *gawio = (GawIoData *) data;

   if ( (gawio->cnx = con_accept(gawio->listen, CON_CHECK_READ )) >= 0 ) {
      fprintf( stderr, _("Connected: %s %s\n"), gawio->cnx->connected_to, gawio->cnx->connected_ip);
      gawio->iochannel = g_io_channel_unix_new(gawio->cnx->s);
      g_io_channel_set_encoding(gawio->iochannel, NULL, NULL);
      gawio->sourceid = g_io_add_watch(gawio->iochannel, G_IO_IN,
                  (GIOFunc) aio_accepted_read_cb,
                  (gpointer) gawio );
   }
   return TRUE;
}
   
void aio_create_channel(UserData *ud)
{
   GawIoData *gawio;

   gawio = g_new0 (GawIoData, 1);
   gawio->ud = ud;
   ud->gawio = (void *) gawio;

   SockCon *cnx = con_new( NULL, PF_INET, SOCK_STREAM, IPPROTO_IP,
                           ud->listenPort, CON_BIND );
   if ( cnx->status < 0) {
      msg_errorl( 2, "Can't open socket" );
      con_destroy(cnx);
      return ;
   }

   con_set_blocking(cnx, CON_NOBLOCKING);
   if ( con_listen(cnx, 5) < 0 ) {
      msg_error(_("listen %s"), strerror(errno) ) ;
      return;
   }
   gawio->listen = cnx;

   gawio->listenchannel = g_io_channel_unix_new(cnx->s);
   g_io_channel_set_encoding(gawio->listenchannel, NULL, NULL);
   gawio->listenid = g_io_add_watch(gawio->listenchannel, G_IO_IN,
                  (GIOFunc) aio_listen_read_cb,
                  (gpointer) gawio );
}

void aio_destroy_channel(UserData *ud)
{
   GawIoData *gawio = (GawIoData *) ud->gawio;

//   msg_dbg( "called" );
   con_destroy(gawio->listen);
   g_source_remove(gawio->listenid);
   g_io_channel_unref(gawio->listenchannel);

   ud->gawio = NULL;
   g_free(gawio);
}

