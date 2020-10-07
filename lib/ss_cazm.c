/*
 * ss_cazm.c - CAZM- and ASCII- format routines for SpiceStream
 *
 * include LICENSE
 */

/*
 * CAzM and "ascii" format are closely related, so they are both handled
 * in this file.
 *
 * CAzM format is intended to handles files written by MCNC's CAzM simulator,
 * used by a number of universities, and its commercial decendant, 
 * the TSPICE product from Tanner Research.
 *
 * CAzM-format files contain a multiline header.  The second to last line
 * of the header identifies the analysis type, for example TRANSIENT or AC.
 * The last line of the header contains the names of the variables, seperated
 * by whitespace.
 *
 * Ascii-format files have a one-line header, containing a space- or
 * tab-speperated list of the variable names.  To avoid treating a file
 * containing random binary garbage as an ascii-format file, we require the
 * header line to contain space, tab, and USASCII printable characters only.
 * 
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
#include <wavevar.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

static int 
ascii_process_header(SpiceStream *ss, char *line, VarType ivtype );

/*
 * return  1 ok
 *         0 EOF
 *        -1 Error
 */
/*
 * Read spice-type file header - cazm format
 */
int sf_rdhdr_cazm( SpiceStream *ss )
{
   int done = 0;
   VarType ivtype;
   char *line;
   int ret;
	
   while( ! done) {
      if ( (line = fdbuf_get_line(ss->linebuf)) == NULL ||
	   fdbuf_get_lineno(ss->linebuf)  > 30) {
	 return 0;
      }
      /* "section header" line */
      if(strncmp (line, "TRANSIENT", 9) == 0) {
	 ivtype = TIME;
	 done = 1;
      } else if (strncmp(line, "AC ANALYSIS", 11) == 0) {
	 ivtype = FREQUENCY;
	 done = 1;
      } else if (strncmp(line, "TRANSFER", 8) == 0) {
	 /* DC transfer function - ivar might also be current,
	  * but we can't tell */
	 ivtype = VOLTAGE;
	 done = 1;
      }
   }

   /*
    * line after header contains signal names
    * first one is assumed to be the independent variable.
    */
   if ( (line = fdbuf_get_line(ss->linebuf)) == NULL ) {
      return 0;
   }

   ret = ascii_process_header(ss, line, ivtype );
   msg_dbg("%s : 'cazm' file format with %d columns", ss->filename, ss->ncols);
   return ret;
}


/*
 * Read spice-type file header - ascii format
 */
int sf_rdhdr_ascii( SpiceStream *ss )
{
   char *cp;
   char *line;
   int ret;
	
   /* 
    * first line is expected to contain space-seperated
    * variable names.
    * first one is assumed to be the independent variable.
    */
   if ( (line = fdbuf_get_line(ss->linebuf)) == NULL) {
      return 0;
   }
	
   /*
    * Check for non-ascii characters in header, to reject 
    * binary files.
    */
   for (cp = line; *cp; cp++) {
      if ( ! isgraph(*cp) && *cp != ' ' && *cp != '\t') {
	 return -1;
      }
   }
   /*
    * Remove some characters at beginning of the line
    */
   cp = line + strspn( line, "#! " ) ; /* skip characters in arg 2 */
   if ( cp > line ) {
      dbuf_strcpy( (DBuf *) ss->linebuf, cp);
   }

   ret = ascii_process_header(ss, ((DBuf *) ss->linebuf)->s, UNKNOWN );
   msg_dbg("%s : 'ascii' file format with %d columns", ss->filename, ss->ncols);
   return ret;
}


/*
 * Process a header line from an ascii or cazm format file.
 * Returns a filled-in SpiceStream* with variable information.
 */
static int 
ascii_process_header(SpiceStream *ss, char *line, VarType ivtype )
{
   char *signam;

   signam = strtok(line, " \t\n");
   if ( ! signam) {
      msg_error(_("line %d: syntax error in header"), fdbuf_get_lineno(ss->linebuf));
      return -1;
   }

   if ( ivtype == UNKNOWN) {
      if (app_strcasestr(signam, "time") ) {
	 ivtype = TIME;
      }
   }
   spicestream_var_add( ss, signam, ivtype, 1 );

   ss->ntables = 1;
   ss->ncols = 1;
   while ((signam = strtok(NULL, " \t\n")) != NULL) {
      spicestream_var_add( ss, signam, UNKNOWN, 1 );
      ss->ncols++;
   }

   /* give a name to the set from filename */
   if ( (signam = strrchr( ss->filename, '/')) ) {
      signam++;
   } else {
      signam = ss->filename;
   }
   dataset_dup_setname(ss->wds, signam );
   ss->flags = SSF_PUSHBACK | SSF_NORDHDR | SSF_CPVARS;
   return 1;
}



/* 
 * Read row of values from ascii- or cazm- format file.  
 * Possibly reusable for other future formats with lines of
 * whitespace-seperated values.
 * Returns:
 *	1 on success.  also fills in *ivar scalar and *dvars vector
 *	0 on EOF
 *	-1 on error  (may change some ivar/dvar values)
 */

int sf_readrow_ascii(SpiceStream *ss)
{
   int i;
   char *tok;
   char *line;
   
   if ( (line = fdbuf_get_line(ss->linebuf)) == NULL) {
      return 0; /* EOF */
   }

   tok = strtok(line, " \t\n");
   if ( ! tok) {
      return -2;  /* blank line can indicate end of data */
   }

   /*
    * check to see if it is numeric: ascii format is so loosly defined
    * that we might read a load of garbage otherwise.
    */
   if (strspn(tok, "0123456789eE+-.") != strlen(tok)) {
      msg_error(_("%s:\nline %d: expected number; maybe this isn't an ascii data file at all?"),
		 ss->filename, ss->linebuf->lineno );
      return -1;
   }

   double val = g_ascii_strtod(tok, NULL);
   spicestream_val_add( ss, val);

   for (i = 0; i < ss->ncols - 1; i++) {
      tok = strtok(NULL, " \t\n");
      if ( ! tok) {
	 msg_error(_("%s:%d: data field %d missing"),
		    ss->linebuf->filename, ss->linebuf->lineno );
	 return -1;
      }
      val = g_ascii_strtod(tok, NULL);
      spicestream_val_add( ss, val);
   }
   return 1;
}

void sf_write_file_ascii( FILE *fd, WaveTable *wt, char *fmt)
{
   int i;
   int j;
   int k = 0;
   char *c = "";
   char format[64];
   WDataSet *wds = wavetable_get_dataset( wt, k);
   WaveVar *var = g_ptr_array_index( wds->vars, 0);
//   double min = wavevar_val_get_min(var);
//   double max = wavevar_val_get_max(var);
   
//   fprintf( stdout, "min %f, max %f\n", min, max);
   
   for ( i = 0 ; i < wds->ncols ; i++ ){
      var = g_ptr_array_index( wds->vars, i);
      fprintf( fd, "%s%s", c, var->varName);
      c = " ";
   }
   fprintf( fd, "\n");
      
   if ( ! fmt ) {
      fmt = "%.10g";
   }
   sprintf( format, "%%s%s", fmt );
   
   while ( (wds = wavetable_get_dataset( wt, k )) ){
      if ( k > 0 ) {
	 fprintf( fd, "\n");
      }
   
      for ( i = 0 ; i < wds->nrows ; i++ ){
	 c = "";
	 for ( j = 0 ; j < wds->ncols ; j++ ){
	    double val = dataset_val_get( wds, i, j );
	    if ( j == 0 ){
	       fprintf( fd, fmt, val);
	    } else {
	       fprintf( fd, format, c, val);
	    }
	    c = " ";
	 }
	 fprintf( fd, "\n");
      }
      k++;
   }
}
