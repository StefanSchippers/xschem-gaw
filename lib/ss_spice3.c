/*
 * ss_spice3.c: routines for SpiceStream that handle the file formats
 * 	known as Berkeley Spice3 Rawfile
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

// #include <glib.h>
#include <spicestream.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

static int sf_getval_s3ascii(SpiceStream *ss, double *val);
static int sf_getval_s3bin(SpiceStream *ss, double *dval);

/*
 * return  1 ok
 *         0 EOF
 *        -1 Error
 */
/* Read spice-type file header - Berkeley Spice3 "raw" format */
int sf_rdhdr_s3raw( SpiceStream *ss )
{
   char *line;
   int lineno ;
   char *key, *val;
   int nvars = 0;
   int npoints = 0;
   int got_nvars = 0;
   int got_values = 0;
   int dtype_complex = 0;
   int binary = 0;
   char *vnum, *vname, *vtypestr;
   int i;
   int ncols = 1;

   dtype_complex = dtype_complex; /* silent the compiler ; this var is not used for the moment */
   while ( (line = fdbuf_get_line(ss->linebuf)) != NULL ) {
      lineno = fdbuf_get_lineno(ss->linebuf);
      if (lineno == 1 && strncmp(line, "Title: ", 7)) {
	 /* not a spice3raw file; bail out */
	 msg_dbg("%d: Doesn't look like a spice3raw file; \"Title:\" expected\n",
		 lineno);
	 return -1;
      }
      /* skip empty lines */
      char *p = line;
      while ( *p && isspace(*p) ){
	 p++;
      }
      if ( ! *p ){
	 continue;
      }
      
      key = strtok(line, ":");
      if ( ! key) {
	 msg_error(_("%d: syntax error, expected \"keyword:\""), lineno);
	 return -1;
      }
      if (strcmp(key, "Flags") == 0) {
	 while ((val = strtok(NULL, " ,\t\n"))) {
	    if (strcmp(val, "real") == 0) {
	       dtype_complex = 0;
	    }
	    if (strcmp(val, "complex") == 0) {
	       dtype_complex = 1;
               ncols = 2;
	    }
	 }
      } else if (strcmp(key, "Plotname") == 0) {
	 val = strtok(NULL, "\n");
	 dataset_dup_setname(ss->wds, val);
      } else if (strcmp(key, "No. Variables") == 0) {
	 val = strtok(NULL, " \t\n");
	 if ( ! val) {
	    msg_error(_("%d: syntax error, expected integer"), lineno);
	    return -1;
	 }
	 nvars = atoi(val);
	 got_nvars = 1;
      } else if (strcmp(key, "No. Points") == 0) {
	 val = strtok(NULL, " \t\n");
	 if ( ! val) {
	    msg_error(_("%d: syntax error, expected integer"), lineno);
	    return -1;
	 }
	 npoints = atoi(val);
      } else if(strcmp(key, "Variables") == 0) {
	 if ( ! got_nvars) {
	    msg_error(_("%d: \"Variables:\" before \"No. Variables:\""), lineno );
	    return -1;
	 }
	 ss->ncols = 0;
	 ss->ntables = 1;
	 /* first variable may be described on the same line
	  * as "Variables:" keyword
	  */
	 vnum = strtok(NULL, " \t\n");
	 
	 for (i = 0; i < nvars; i++) {
	    if (i || ! vnum) {
	       if ( (line = fdbuf_get_line(ss->linebuf)) == NULL ) {
		  lineno = fdbuf_get_lineno(ss->linebuf);
		  msg_error(_("%d: Unexpected EOF in \"Variables:\" at var %d"),
			     lineno, i);
		  return 0;
	       }
	       vnum = strtok(line, " \t\n");
	    }
	    vname = strtok(NULL, " \t\n");
	    vtypestr = strtok(NULL, " \t\n");
	    if ( ! vnum || ! vname || ! vtypestr) {
	       msg_error(_("%d: expected number name type"), lineno);
	       return -1;
	    }
	    spicestream_var_add( ss, vname, wavevar_str2type(vtypestr), ncols);
	    ss->ncols += ncols;
	 }
      } else if (strcmp(key, "Values") == 0) {
	 got_values = 1;
	 break;
      } else if (strcmp(key, "Binary") == 0) {
	 binary = 1;
	 got_values = 1;
	 break;
      }
      if (got_values)
	 break;
   }
   if ( ! line ) { /* EOF */
      return 0;
   }
   if ( ! got_values) {
      msg_error(_("%s: line %d:\nEOF without \"Values:\" in header"),
		 ss->filename, lineno);
   }

   if (binary) {
      ss->rdValFunc = sf_getval_s3bin ;
   } else {
      ss->rdValFunc = sf_getval_s3ascii ;
   }
   ss->nrows = npoints;
   ss->expected_vals = npoints * ss->ncols ;
   msg_dbg("expecting %d rows %d cols %d values", npoints, ss->ncols, ss->expected_vals);
   msg_dbg("Done with header at offset 0x%lx\n", (long) fdbuf_tello(ss->linebuf) );
   dbuf_clear((DBuf *) ss->linebuf);
   return 1;
}

static int sf_getval_s3ascii(SpiceStream *ss, double *val)
{
   int done = 0;
   int i = 0;
   int rownum = 0;
   int c;
   DBuf *dbuf = ss->dbuf;

   dbuf_clear(dbuf);
   while ( ! done ) {
      while ( 1 ) {
	 c = dbuf_get_char( (DBuf *) ss->linebuf) ;
	 if ( i == 0 && c < 0 ) {
	    if ( fdbuf_get_line(ss->linebuf) == NULL) {
	       return 0; /* EOF */
	    }
	    rownum = 1; /* Could be a rownumber comming. */
	    continue;
	 }
	 /* We skip leading whitespaces.  Once the first tab has been seen, we
	  * can no longer be dealing with the first field, the row field.
	  */
	 if ( isspace(c) && i == 0 ) {
	    if ( c == '\t' ) {
	       rownum = 0;
	    }
	    continue ;
	 }
	 /* If we see a whitespace (isspace() == t) or the ned of the buffer
	  * we process whatever we read in. We also have to consider the
	  * comma character, as complex numbers are comma separated.
	  */
	 if ( isspace(c) || c == ',' || c <= 0 ) {
	    if ( rownum ) {
	       ss->currow =  atoi(dbuf->s);
	       dbuf_clear(dbuf);
	       rownum = i = 0;
	       continue;
	    }
	    done = 1;
	    break;
	 }
	 /* If we reach here we are looking at part of a number so we simpl
	  * grab it.
	  */
	 i++;
	 dbuf_put_char( dbuf,  c);
      }
   }
   *val = g_ascii_strtod( dbuf->s, NULL);
   
//   fprintf(stderr, "#buf=\"%s\" val=%g\n", dbuf->s, *val);
   return 1;
}


/*
 * Read a single value from binary spice3 rawfile, and do
 * the related error-checking.
 */

static int sf_getval_s3bin(SpiceStream *ss, double *dval)
{
   off_t pos; /* off64_t */
   double *val;
   
   if (ss->read_vals >= ss->expected_vals) {
      pos = fdbuf_tello(ss->linebuf);
      msg_dbg("past last expected value offset 0x%lx", (long) pos);
      return 0;
   }
   if ( fdbuf_read(ss->linebuf,  sizeof(double) ) <= 0 ) {
      pos = fdbuf_tello(ss->linebuf);
      msg_error(_("unexepected EOF in data at offset 0x%lx"), (long) pos);
      return 0;
   }
   val = (double *) ((DBuf *) ss->linebuf)->s;
   *dval = *val;
   ss->read_vals++;

   return 1;
}


/*
 * Read row of values from a binay/ascii spice3 raw file
 */
int sf_readrow_s3raw(SpiceStream *ss )
{
   int i;
   int ret;
   double val;

   for (i = 0; i < ss->ncols ; i++) {
      ret =  ss->rdValFunc ( ss, &val );
      if ( i == 0 && ret != 1 ) {
	 return ret;
      } else if ( ret  != 1) {
	 msg_warning(_("%s:\nEOF or error reading data field %d in row %d; file is incomplete."),
		     ss->filename, i, ss->read_rows );
	 return 0;
      }
      dataset_val_add( ss->wds, val);
   }

   ss->read_rows++;
   if ( ss->read_rows >= ss->nrows ) {
      return -2; 
   }
   return 1;
}

void sf_write_file_s3ascii( FILE *fd, WaveTable *wt, char *fmt)
{
   int i;
   int j;
   int k = 0;
   time_t now;
   char format[32];
   int dtype_complex;
   WDataSet *wds = wavetable_get_dataset( wt, k);
   WaveVar *var = g_ptr_array_index( wds->vars, 0);
   char *title = "Data written by Gaw";
   char *plotname;
   

   if ( ! fmt ) {
      fmt = "%0.15e";
   }
   fprintf( fd, "Title: %s\n", title );
   time(&now) ;
   fprintf( fd, "Date: %s", ctime(&now) );
   
   while ( (wds = wavetable_get_dataset( wt, k++)) ){

      var = g_ptr_array_index( wds->vars, 1);
      dtype_complex = 0;
      if ( var->ncols == 2 ){
	 dtype_complex = 1; 
      }
      plotname = dataset_get_setname(wds);
      if ( ! plotname ){
	 plotname = title;
      }
      fprintf( fd, "Plotname: %s\n", plotname );
      fprintf( fd, "Flags: %s\n", dtype_complex ? "complex" : "real" );
      fprintf( fd, "No. Variables: %d\n", wds->numVars ); 
      fprintf( fd, "No. Points: %d\n", wds->nrows );

      fprintf( fd, "Variables:\n" );
      for ( i = 0 ; i < wds->numVars ; i++ ){
	 var = g_ptr_array_index( wds->vars, i);
	 fprintf( fd, "\t%d\t%s\t%s\n", i, var->varName,
		  wavevar_type2str( var->type) );
      }
      
      fprintf( fd, "Values:\n" ); 

      for ( i = 0 ; i < wds->nrows ; i++ ){
	 for ( j = 0 ; j < wds->ncols ; j++ ){
	    double val = dataset_val_get( wds, i, j );
	    if ( j == 0 ){
	       sprintf( format, "%%d\t\t%s\n", fmt );
	       fprintf( fd, format, i, val);
	       sprintf( format, "\t%s\n", fmt );
	    } else {
	       fprintf( fd, format, val);
	    }
	 }
      }
   }
}
