/*
 * ss_nsout.c: routines for SpiceStream that handle the ".out" file format
 * 	from Synopsis' nanosim.
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include <spicestream.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

struct nsvar {
   char *name;
   int index;
   VarType type;
};

void sf_clean_vlist(GList *list)
{
   if ( ! list ){ 
      return;
   }
   while (list) {
      GList *next = list->next;
      struct nsvar *nsv = (struct nsvar *) list->data;
      g_free ( nsv->name);
      g_free ( nsv);
      list = next;
   }
   g_list_free (list);
}


/* convert variable type string from out-file to 
 * our type numbers
 */
static VarType sf_str2type_nsout(char *s)
{
   if (app_strcasecmp(s, "v") == 0) {
      return VOLTAGE;
   } else if(app_strcasecmp(s, "i") == 0) {
      return CURRENT;
   } else { 
      return UNKNOWN;
   }
}

/*
 * return  1 ok
 *         0 EOF
 *        -1 Error
 */

/* Read spice-type file header - nanosim "out" format */
int sf_rdhdr_nsout(SpiceStream *ss)
{
   char *line;
   char *key, *val;
   int got_ivline = 0;
   int ndvars;
   GList *vlist = NULL;
   int lineno;
   int i;
   int minindex = 0x7FFFFFFF;
   int maxindex = 0;
   int ret = -1;
   struct nsvar *nsv = NULL;
   int got_nsout = 0;
	
   ss->voltage_resolution = 1.0;
   ss->current_resolution = 1.0;
   ss->time_resolution = 1.0;

   while ( (line = fdbuf_get_line(ss->linebuf)) != NULL ) {
      lineno = fdbuf_get_lineno(ss->linebuf);
      if (lineno == 1 && *line != ';' ) {
	 /* not an out file; bail out */
	 msg_dbg("%s:\n line %d: Doesn't look like an ns-out file; \"output_format\" expected\n",
		  ss->filename, lineno );
	 goto err;
      }
      if ( app_strcasestr(line, "output_format") ||  app_strcasestr(line, "NanoSim") ) {
	 got_nsout = 1;
      }
      if (line[0] == ';'){
	 continue;
      }
      if ( ! got_nsout ) {
	 goto err;
      }
      if (line[0] == '.') {
	 key = strtok(&line[1], " \t");
	 if ( ! key) {
	    msg_error(_("%d: syntax error, expected \"keyword:\""), lineno );
	    goto err;
	 }
	 if (strcmp(key, "time_resolution") == 0) {
	    val = strtok(NULL, " \t\n");
	    if ( ! val) {
	       msg_error(_("%d: syntax error, expected number"),  lineno );
	       goto err;
	    }
	    ss->time_resolution = g_ascii_strtod(val, NULL);
	 }
	 if (strcmp(key, "current_resolution") == 0) {
	    val = strtok(NULL, " \t\n");
	    if ( ! val) {
	       msg_error(_("%d: syntax error, expected number"), lineno);
	       goto err;
	    }
	    ss->current_resolution = g_ascii_strtod(val, NULL);
	 }
	 if (strcmp(key, "voltage_resolution") == 0) {
	    val = strtok(NULL, " \t\n");
	    if ( ! val) {
	       msg_error(_("%d: syntax error, expected number"), lineno);
	       goto err;
	    }
	    ss->voltage_resolution = g_ascii_strtod(val, NULL);
	 }
	 if (strcmp(key, "index") == 0) {
	    /* .index name index type */
	    nsv = g_new0(struct nsvar, 1);
	    vlist = g_list_append(vlist, nsv);	
	    
	    val = strtok(NULL, " \t\n");
	    if ( ! val) {
	       msg_error(_("%d: syntax error, expected varname"), lineno);
	       goto err;
	    }
	    nsv->name = g_strdup(val);
	    
	    val = strtok(NULL, " \t\n");
	    if( ! val) {
	       msg_error(_("%d: syntax error, expected var-index"), lineno);
	       goto err;
	    }
	    nsv->index = atoi(val);
	    if (minindex > nsv->index ){
	       minindex = nsv->index;
	    }
	    if (nsv->index > maxindex){
	       maxindex = nsv->index;
	    }
				
	    val = strtok(NULL, " \t\n");
	    if ( ! val) {
	       msg_error(_("%d: syntax error, expected variable type"), lineno);
	       goto err;
	    }
	    nsv->type = sf_str2type_nsout(val);
	 }
      }

      if (isdigit(line[0])) {
	 got_ivline = 1;
	 break;
      }
   }
   if ( ! line) {
      return 0;
   }
   if ( ! vlist) {
      msg_error(_("%d: no variable indices found in header"), lineno);
   }
   if ( ! got_ivline) {
      msg_error(_("%d: EOF without data-line in header"), lineno);
      goto err;
   }
   ndvars = g_list_length(vlist);
	
   ss->minindex = minindex;
   ss->maxindex = maxindex;
   ss->datrow = g_new0(double, maxindex + 1 - minindex);
   ss->nsindexes = g_new0(int, ndvars);

   ss->ncols = 1;
   ss->ntables = 1;
   ss->ndv = ndvars;

   dataset_var_add( ss->wds, "TIME", TIME, 1 );

   for (i = 0; i < ndvars; i++) {
      nsv = g_list_nth_data(vlist, i);

      dataset_var_add( ss->wds, nsv->name, nsv->type, 1 );
      ss->nsindexes[i] = nsv->index;

      ss->ncols++;

      msg_dbg("dv[%d] \"%s\" nsindex = %d",
	       i, nsv->name, ss->nsindexes[i]);
   }

   msg_dbg("Done with header at offset 0x%lx", (long) fdbuf_tello(ss->linebuf) );

   ss->flags = SSF_NORDHDR | SSF_CPVARS | SSF_PUSHBACK;
   ret = 1;
err:
   sf_clean_vlist(vlist);

   return ret;
}

/*
 * Read row of values from an out-format file
 * upon call, line buffer should always contain the 
 * independent-variable line that starts this set of values.
 */
int sf_readrow_nsout(SpiceStream *ss )
{
   int i;
   int lineno;
   int idx;
   char *sidx;
   char *sval;
   double val;
   double scale;
   char *line = ((DBuf *) ss->linebuf)->s;

   // process iv line
//   val = g_ascii_strtod(line, NULL) * ss->time_resolution * 1e-9; /* ns */
   val = g_ascii_strtod(line, NULL) ; /* conversion should be done at display ? */
   spicestream_val_add( ss, val);

   // read and process dv lines until we see another iv line
   while ( (line = fdbuf_get_line(ss->linebuf)) != NULL) {
      if (line[0] == ';') {
	 continue;
      }
      lineno = fdbuf_get_lineno(ss->linebuf);
      sidx = strtok(line, " \t");
      if ( ! sidx) {
	 msg_error(_("%s:%d: expected value"), ss->filename, lineno);
	 return -1;
      }

      sval = strtok(NULL, " \t"); 
      if ( ! sval)
	 /* no value token: this is the ivar line for the  next row */
	 break;

      idx = atoi(sidx);
      if (idx <= ss->maxindex) {
	 ss->datrow[idx - ss->minindex ] = g_ascii_strtod(sval, NULL);
      }
   }
   if ( line == NULL ) {
      return 0;
   }

   for (i = 0; i < ss->ndv; i++) {
      scale = 1.0;
      int type = dataset_get_wavevar_type( ss->wds, i + 1 );
      switch(type) {
       case VOLTAGE:
	 scale = ss->voltage_resolution;
	 break;
       case CURRENT:
	 scale = ss->current_resolution;
	 break;
       default:
	 break;
      }
      val = ss->datrow[ss->nsindexes[i] - ss->minindex] * scale;
      spicestream_val_add( ss, val );
   }
   return 1;
}

static char *sf_type2str_nsout(int type)
{
   switch (type) {
    case UNKNOWN:
      return "U";
    case TIME:
      return "T";
    case VOLTAGE:
      return "V";
    case CURRENT:
      return "I";
    case FREQUENCY:
      return "F";
   }
   return "U";
}

void sf_write_file_nsout( FILE *fd, WaveTable *wt, char *fmt)
{
   int i;
   int j;
   int k = 0;
   char format1[32];
   char format2[32];
   time_t now;
   double voltage_resolution = 1.0;
   double current_resolution = 1.0;
   double time_resolution = 1.0;
   int index;
   
   fprintf( fd, "; ------------------------------------------------------- \n");
   fprintf( fd, ";|               NanoSim Output Format                   |\n");
   fprintf( fd, ";|                                                       |\n");
   fprintf( fd, ";|                  Generated by Gaw                     |\n");
   fprintf( fd, ";|                                                       |\n");
   fprintf( fd, "; ------------------------------------------------------- \n");
   fprintf( fd, ";\n");
   time(&now) ;
   fprintf( fd, "; %s", ctime(&now) );
   fprintf( fd, ";\n");
   fprintf( fd, ".voltage_resolution %g\n", voltage_resolution );
   fprintf( fd, ".current_resolution %g\n", current_resolution );
   fprintf( fd, ".time_resolution %g\n", time_resolution );
   fprintf( fd, ";\n");
   
   WDataSet *wds = wavetable_get_dataset( wt, k);
   /* do not output ivar as index */
   for ( i = 1 ; i < wds->numVars ; i++ ){
      WaveVar *var = g_ptr_array_index( wds->vars, i);
      index = 130 + i * 2;
      fprintf( fd, ".index %s %d %s\n", var->varName, index,
	       sf_type2str_nsout(var->type) );
   }
      
   if ( ! fmt ) {
      fmt = "%0.8f";
   }
   sprintf( format1, "%s\n", fmt );
   sprintf( format2, "%%d %s\n", fmt );

   while ( (wds = wavetable_get_dataset( wt, k++)) ){
   
      for ( i = 0 ; i < wds->nrows ; i++ ){
	 for ( j = 0 ; j < wds->ncols ; j++ ){
	    index = 130 + j * 2;
	    double val = dataset_val_get( wds, i, j );
	    if ( j == 0 ){
	       fprintf( fd, format1, val);
	    } else {
	       fprintf( fd, format2, index, val);
	    }
	 }
      }
   }
}
