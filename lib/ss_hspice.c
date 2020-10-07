/*
 * ss_hspice.c: HSPICE routines for SpiceStream
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
#include <sys/types.h>
#include <sys/stat.h>

#include <bswap.h>
#include <spicestream.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

static int hs_process_header(SpiceStream *ss, char *line, int nauto, int nprobe);

static int sf_getval_hsbin(SpiceStream *ss, double *dval);
static int sf_getval_hsascii(SpiceStream *ss, double *dval);
static int sf_readsweep_hspice(SpiceStream *ss);


/* structure of binary tr0 block headers */
struct hsblock_header {
   gint32 h1;
   gint32 h2;
   gint32 h3;
   gint32 block_nbytes;
};

static void
sf_swap_gint32(gint32 *pi, size_t n)
{
   for ( ; n > 0; n-- ) {
      *pi = be32_to_cpu(*pi) ;
      pi++;
   }
}


/*
 * Read spice-type file header - hspice ascii
 *
 * return  1 ok
 *         0 EOF
 *        -1 Error
 */

int sf_rdhdr_hsascii(SpiceStream *ss)
{
   char *line;
   int nauto, nprobe, ntables;
   char nbuf[16];
   char *cp;
   int maxlines;
   DBuf *dbuf;

   /* line 1 */
   if ( (line = fdbuf_get_line(ss->linebuf)) == NULL) {
      return 0;
   }
   if ( *line < ' ' ) {
      return -1;
   }
   /* version of post format */
   if ( strncmp(&line[16], "9007", 4) != 0 && strncmp(&line[16], "9601", 4) != 0) {
      return -1;
   }
      
   app_strncpy(nbuf, &line[0], 4);
   nbuf[4] = 0;
   nauto = atoi(nbuf);

   app_strncpy(nbuf, &line[4], 4);
   nbuf[4] = 0;
   nprobe = atoi(nbuf);
	
   app_strncpy(nbuf, &line[8], 4);
   nbuf[4] = 0;
   ss->nsweepparam = atoi(nbuf);

   /* line 2 */
   if ( (line = fdbuf_get_line(ss->linebuf)) == NULL) { /* date, time etc. */
      return 0;
   }
   /* number of sweeps, possibly with cruft at the start of the line */
   if ( (line = fdbuf_get_line(ss->linebuf)) == NULL) { /* date, time etc. */
      return 0;
   }
   cp = strchr(line, ' ');
   if ( ! cp) {
      cp = line;
   }
   ntables = atoi(cp); 
   if (ntables == 0) {
      ntables = 1;
   }
   ss->ntables = ntables; 

   maxlines = nauto + nprobe + ss->nsweepparam + 100;
   /* lines making up a fixed-field structure with variable-types and
    * variable names.
    * variable names can get split across lines! so we remove newlines,
    * paste all of the lines together, and then deal with the
    * whole header at once.
    * A variable name of "$&%#" indicates the end!
    */

   dbuf = ss->dbuf;
   dbuf_clear(dbuf);
   do {
      if ( (line = fdbuf_get_line(ss->linebuf)) == NULL) { /* date, time etc. */
	 return 0;
      }
      dbuf_cat( dbuf, (DBuf *) ss->linebuf);
//      dbuf_strcat( dbuf, " ");
      int lineno = fdbuf_get_lineno( ss->linebuf);
 
      if (lineno >= maxlines) {
	 msg_dbg("%d: end of hspice header not found", lineno);
	 return -1;
      }

      if (dbuf_get_len(dbuf) > 1050000) {
	 msg_error(_("internal error - failed to find end of header") );
	 return -1;
      }
   } while ( ! strstr(line, "$&%#")) ;

   ss->rdSweepFunc = sf_readsweep_hspice;
   ss->rdValFunc = sf_getval_hsascii ;


   int ret = hs_process_header(ss, dbuf->s, nauto, nprobe);
   msg_dbg("ntables = %d; expect %d columns", ss->ntables, ss->ncols);
   return ret;
}

static int sf_read_header_block(SpiceStream *ss)
{
   off_t pos; /* off64_t */
   struct hsblock_header *hh;
   char *line ;
   int n;
   
   pos =  fdbuf_tello(ss->linebuf);
   if ( (n = fdbuf_read(ss->linebuf, sizeof( struct hsblock_header) )) <= 0 ) {
      msg_dbg("EOF reading block header at offset 0x%lx", (long) pos);
      return 0;
   }
   line = ((DBuf *) ss->linebuf)->s;
   if ( *line >= ' ' ) {
      return -1;
   }
   hh = (struct hsblock_header *) line;
   if (hh->h1 == 0x04000000 && hh->h3 == 0x04000000) {
      /* detected endian swap */
      ss->flags |= SSF_ESWAP;
      sf_swap_gint32((gint32 *) hh, sizeof(struct hsblock_header) / sizeof(gint32) );
   } else {
      ss->flags &= ~SSF_ESWAP;
   }
   if (hh->h1 != 0x00000004 || hh->h3 != 0x00000004) {
      msg_error(_("unexepected values in block header at offset 0x%lx"), pos);
      return -1;
   }
   ss->read_vals = 0;
   ss->expected_vals = hh->block_nbytes / sizeof(float);
   return hh->block_nbytes;
}


static int sf_read_trailer_block(SpiceStream *ss, gint32 size )
{
   off_t pos; /* off64_t */
   gint32 *trailer;
   int n;
   
   pos =  fdbuf_tello(ss->linebuf);
   if ( (n = fdbuf_read(ss->linebuf, sizeof(gint32) )) !=  sizeof(gint32) ) {
      msg_dbg("EOF reading block trailer  at offset 0x%lx",  (long) pos);
      pos = pos; /* silent the compiler */
      return 0;
   }
   trailer = (gint32 *) ((DBuf *) ss->linebuf)->s;
   if ( ss->flags & SSF_ESWAP) {
      sf_swap_gint32( trailer, 1);
   }
   if (*trailer != size ) {
      msg_dbg("block trailer mismatch at offset 0x%lx", (long) pos);
      return -1;
   }
   return 1;
}

/* Read spice-type file header - hspice binary */
int sf_rdhdr_hsbin(SpiceStream *ss)
{
   int n;
   int nauto, nprobe, ntables;
   char nbuf[16];
   char *line ;
   int bufsize;
   DBuf *dbuf;

   if ( (bufsize = sf_read_header_block(ss)) < 0 ){
      return 0;
   }
   if ( (n = fdbuf_read(ss->linebuf, bufsize )) <= 0 ) {
      return 0;
   }
   line = ((DBuf *) ss->linebuf)->s;
   if ( ! strstr(line, "$&%#") ) {
      return -1;
   }
   
   dbuf = ss->dbuf;
   dbuf_copy( dbuf, (DBuf *) ss->linebuf);

   /* read the trailer */
    if ( (n = sf_read_trailer_block(ss, bufsize )) <= 0 ) {
       return n;
    }

   /* 
    * line is an ascii header that describes the variables in
    * much the same way that the first lines of the ascii format do,
    * except that there are no newlines
    */

   /* version of post format */
   line = dbuf->s;
   if (strncmp(&line[16], "9007", 4) != 0 && strncmp(&line[16], "9601", 4) != 0) {
      return -1;
   }
   app_strncpy(nbuf, &line[0], 4);
   nbuf[4] = 0;
   nauto = atoi(nbuf);	/* number of automaticly-included variables,
			 first one is independent variable */
   app_strncpy(nbuf, &line[4], 4);
   nbuf[4] = 0;
   nprobe = atoi(nbuf);	/* number of user-requested columns */

   app_strncpy(nbuf, &line[8], 4);
   nbuf[4] = 0;
   ss->nsweepparam = atoi(nbuf);	/* number of sweep parameters */

   ntables = atoi(&line[176]);
   if (ntables == 0) {
      ntables = 1;
   }
   ss->ntables = ntables;
   ss->rdSweepFunc = sf_readsweep_hspice;
   ss->rdValFunc = sf_getval_hsbin ;

   n = hs_process_header( ss, &line[256], nauto, nprobe);
   if ( n != 1) {
      return n;
   }
	
   if ( (bufsize = sf_read_header_block(ss)) < 0 ){
      ss->flags = 0;
      return -2;
   }
   msg_dbg("datasize=%d expect %d columns, %d values;\n  reading first data block at 0x%lx",
	   bufsize, ss->ncols, ss->expected_vals, (long) fdbuf_tello(ss->linebuf));

   return 1;
}

/* 
 * common code for reading ascii or binary hspice headers.
 * Given a string of ascii header information, set up the
 * SpiceStream structure appropriately.
 * Returns -1 on failure.
 */
static int
hs_process_header(SpiceStream *ss, char *line, int nauto, int nprobe )
{
   char *cp;
   char *signam;
   int i;
   int hstype;
   int ivtype;
   int type;

   ss->ndv = nauto - 1 + nprobe;

   /* type of independent variable */
   cp = strtok(line, " \t\n");
   if ( ! cp) {
      msg_dbg("initial vartype not found on header line.");
      return -1;
   }
   hstype = atoi(cp);
   switch (hstype) {
    case 1:
      ivtype = TIME;
      break;
    case 2:
      ivtype = FREQUENCY;
      break;
    case 3:
      ivtype = VOLTAGE;
      break;
    default:
      ivtype = UNKNOWN;
      break;
   }
   ss->ncols = 1;
   spicestream_var_add( ss, NULL, ivtype, 1 );

   /* dependent variable types */
   for (i = 0; i < ss->ndv ; i++) {
      cp = strtok(NULL, " \t\n");
      if ( ! cp) {
	 msg_dbg("not enough vartypes on header line");
	 return -2;
      }
      if ( ! isdigit(cp[0])) {
	 msg_dbg("bad vartype %d [%s] on header line", i, cp);
	 return -2;
      }
      hstype = atoi(cp);
      switch (hstype) {
       case 1:
       case 2:
	 type = VOLTAGE;
	 break;
       case 8:
       case 15:
       case 22:
	 type = CURRENT;
	 break;
       default:
	 type = UNKNOWN;
	 break;
      }
      
      /* how many columns comprise this variable? */
      int ncols = 1;
      if ( i < nauto - 1 && ivtype == FREQUENCY) {
	 ncols = 2;
      }
      ss->ncols += ncols;
      spicestream_var_add( ss, NULL, type, ncols );
   }

   /* independent variable name */
   signam = strtok(NULL, " \t\n"); 
   if ( ! signam) {
      msg_dbg("no IV name found on header line");
      return -2;
   }
   dataset_dup_var_name( ss->wds, signam, 0 );
	
   /* dependent variable names */
   for (i = 0; i < ss->ndv; i++) {
      if ((signam = strtok(NULL, " \t\n")) == NULL) {
	 msg_dbg("not enough DV names found on header line");
	 return -2;
      }
      dataset_dup_var_name( ss->wds, signam, i + 1 );
   }
   /* sweep parameter names */
   for (i = 0; i < ss->nsweepparam ; i++) {
      if ((signam = strtok(NULL, " \t\n")) == NULL) {
	 msg_dbg("not enough sweep parameter names found on header line" );
	 return -2;
      }
      wavetable_swvar_add( ss->wt, signam, UNKNOWN, 1 );
   }
   ss->flags |= (SSF_NORDHDR | SSF_CPVARS);
   dbuf_clear( (DBuf *) ss->linebuf );
   return 1;
}

/*
 * helper routine: get next floating-point value from data part of binary
 * hspice file.   Handles the block-structure of hspice files; all blocks
 * encountered are assumed to be data blocks.  We don't use readblock_hsbin because
 * some versions of hspice write very large blocks, which would require a 
 * very large buffer.
 * 
 * Returns 0 on EOF, 1 on success, negative on error.
 */
static int
sf_getval_hsbin(SpiceStream *ss, double *dval)
{
   off_t pos; /* off64_t */
   float *fval;
   int bufsize;
   int n;
   
   if (ss->read_vals >= ss->expected_vals) {
      /* read the trailer */
      if ( (n = sf_read_trailer_block(ss, ss->expected_vals * sizeof(float) )) <= 0 ) {
	 return n;
      }
      
      /* read a headr block */
      if ( (bufsize = sf_read_header_block(ss)) <= 0 ){
	 return bufsize;
      } 
   }
   if ( (n = fdbuf_read(ss->linebuf,  sizeof(float) )) <= 0 ) {
      pos = fdbuf_tello(ss->linebuf);
      msg_error(_("unexepected EOF in data at offset 0x%lx"), (long) pos);
      return 0;
   }
   ss->read_vals++;
   fval = ( float *) ((DBuf *) ss->linebuf)->s;

   if (ss->flags & SSF_ESWAP) {
      sf_swap_gint32((gint32 *) fval, 1);
   }
   *dval = ( double) *fval;
//   fprintf( stderr, "float %f, double %f\n", *fval, *dval );
   return 1;
}

   
/*
 * helper routine: get next value from ascii hspice file.
 * the file is line-oriented, with fixed-width fields on each line.
 * Lines may look like either of these two examples:
0.66687E-090.21426E+010.00000E+000.00000E+000.25000E+010.71063E-090.17877E+01
 .00000E+00 .30000E+01 .30000E+01 .30000E+01 .30000E+01 .30000E+01 .30092E-05
 * There may be whitespace at the end of the line before the newline.
 *
 * Returns 0 on EOF, 1 on success.
 */
static int
sf_getval_hsascii(SpiceStream *ss, double *val)
{
   char vbuf[16];
   int done = 0;
   int i = 0;
   int j = 0;
   int c;

   while ( ! done ) {
      while ((c = dbuf_get_char( (DBuf *) ss->linebuf)) >= 0) {
	 j++;
	 if ( isspace(c) && i == 0 ) {
	    continue ;
	 }
	 if ( isspace(c) || c == 0) {
	    done = 1;
	    break;
	 }
	 vbuf[i++] = c ;
	 if ( j == 11 ) {
	    done = 1;
	    break;
	 }
      }
      vbuf[i] = 0 ;
      if ( done == 0 && fdbuf_get_line(ss->linebuf) == NULL) {
	 return 0; /* EOF */
      }
   }
   if ( j < 11) {
      int lineno = fdbuf_get_lineno( ss->linebuf);
      lineno = lineno; /* gcc warning */
      msg_dbg("%s: line %d: incomplete float value", ss->filename, lineno);
      /* incomplete float value - probably truncated or partialy-written file */
      return 0;
   }
   *val = g_ascii_strtod(vbuf, NULL);
//   msg_dbg( "#vbuf=\"%s\" val=%f\n", vbuf, *val);
   return 1;
}

/* Read row of values from ascii hspice-format file.
 * Returns:
 *	1 on success.  also fills in *ivar scalar and *dvars vector
 *	0 on EOF
 *	-1 on error  (may change some ivar/dvar values)
 *	-2 on end of table, with more tables supposedly still to be read.
 */

int sf_readrow_hspice(SpiceStream *ss )
{
   int i;
   double val;
   int ret;

   ret =  ss->rdValFunc ( ss, &val );
   if (  ret != 1 ) {
      return ret;
   }
   if (val >= 1.0e29) { /* "infinity" at end of data table */
      ss->read_tables++;
      if (ss->read_tables == ss->ntables) {
	 return 0; /* EOF end of data */
      }
      ss->read_sweepparam = 0;
      return -2;  /* end of table, more tables follow */
   }
   dataset_val_add( ss->wds, val);
   
   for (i = 0; i < ss->ncols - 1; i++ ) {
      if ( ss->rdValFunc ( ss, &val ) != 1) {
	 msg_warning(_("%s:\nEOF or error reading data field %d in row %d of table %d; file is incomplete."),
		     ss->filename, i, ss->read_rows, ss->read_tables);
	 return 0;
      }
      dataset_val_add( ss->wds, val);
   }
   ss->read_rows++;
   return 1;
}

/*
 * Read the sweep parameters from an HSPICE ascii or binary file
 * This routine must be called before the first sf_readrow_hsascii call in each data
 * table.  If it has not been called before the first readrow call, it will be called
 * with a NULL svar pointer to read and discard the sweep data. 
 *
 * returns:
 *	1 on success
 * 	-1 on error
 */

static int
sf_readsweep_hspice(SpiceStream *ss )
{
   int i;
   double val;
   
   for (i = 0 ; i < ss->nsweepparam ; i++) {
      if (  ss->rdValFunc ( ss, &val ) != 1) {
	 msg_error(_("EOF or error reading sweep parameter"));
	 return -1;
      }
      dataset_swval_add(ss->wds, val);
   }
   ss->read_sweepparam = 1;
   return 1;
}


int sf_buffer_copy(char *dest, char *src, int dpos, int len)
{
   int i;
   int pos = 0;
   
   for ( i = 0 ; i < dpos ; i++ ){
      *dest++ = ' ';
   }
   for (   ; i < len ; i++ ){
      *dest = *src;
      if (*src == '\n' ) {
	 *dest = ' ';
      }
      if (*src == 0 ) {
	 *dest = ' ';
      } else {
	 src++;
	 pos++;
      }
      dest++;
   }
   *dest = 0;
   if ( *src != 0 ) {
      return pos;  /* pos in src */
   }
   return 0;
}

int sf_hspice_type2str( int type)
{
   int hstype;
   
   switch (type) {
    case VOLTAGE:
      hstype = 1;
      break;
    case CURRENT:
      hstype = 8;
      break;
    default:
      hstype = 0;
      break;
   }
   return hstype;
}

int sf_hspice_ivtype2str( int type)
{
   int ivtype;
   
   switch (type) {
    case TIME:
      ivtype = 1;
      break;
    case FREQUENCY:
      ivtype = 2;
      break;
    case VOLTAGE:
      ivtype = 3;
      break;
    default:
      ivtype = 0;
      break;
   }
   return ivtype;
}

void sf_write_file_hsascii( FILE *fd, WaveTable *wt, char *fmt)
{
   int i;
   int j;
   int k = 0;
   int l;
   int m;
   int n = 0;
   char format[32];
   char src[82];
   char dest[82];
   time_t now;

   WDataSet *wds = wavetable_get_dataset( wt, n);
   WaveVar *var = g_ptr_array_index( wds->vars, 0);
   double min = wavevar_val_get_min(var);
   double max = wavevar_val_get_max(var);

   int nauto = 1;
   int nprobe = wds->numVars - nauto;
   int nsweepparam = wt->nswvars;
   char *hsversion = "9601";
   
   fprintf( stdout, "min %f, max %f\n", min, max);

   sprintf( src, "%04d%04d%04d%04d%-8s*", nauto, nprobe, nsweepparam, n, hsversion );
   sf_buffer_copy(dest, src, 0, 80);
   fprintf( fd, "%s\n", dest );

   time(&now) ;
   sprintf( src, "%s Generated by Gaw", ctime(&now) );
   sf_buffer_copy(dest, src, 16, 80);
   fprintf( fd, "%s\n", dest );
   
   sprintf( src, "%d", wavetable_get_ntables(wt) );
   sf_buffer_copy(dest, src, 27, 80);
   fprintf( fd, "%s\n", dest );

   k = 19;
   l = 0;
   for ( i = 0 ; i < wds->numVars ; i++ ){
      var = g_ptr_array_index( wds->vars, i);
      if ( i == 0 ){
	 sprintf( src, "%8d", sf_hspice_ivtype2str(var->type));
      } else {
	 sprintf( src, "%8d", sf_hspice_type2str(var->type));
      }
      if ( (j = sf_buffer_copy(&dest[l], src, k, 80 - l )) > 0 ){
	 fprintf( fd, "%s\n", dest );
	 k = 0;
	 l = 0;
	 sf_buffer_copy(&dest[l], &src[j], k, 80 - l ) ;
	 l += j;
      } else {
	 l += k + strlen(src);
	 k = 0;
      }
   }


   for ( i = 0 ; i < wds->numVars ; i++ ){
      var = g_ptr_array_index( wds->vars, i);
      sprintf( src, " %-8s     ", var->varName );

      if ( (j = sf_buffer_copy(&dest[l], src, k, 80 - l )) > 0 ){
	 fprintf( fd, "%s\n", dest );
	 k = 0;
	 l = 0;
	 sf_buffer_copy(&dest[l], &src[j], k, 80 - l ) ;
	 l += j;
      } else {
	 l += k + strlen(src);
	 k = 0;
      }
   } 

   for ( i = 0 ; i < wt->nswvars + 1; i++ ){
      if ( i == wt->nswvars ) { 
	 app_strcpy( src, "  $&%#" );
      } else {
	 var = g_ptr_array_index( wt->swvars, i);
	 sprintf( src, " %-8s     ", var->varName );
      }
      if ( (j = sf_buffer_copy(&dest[l], src, k, 80 - l )) > 0 ){
	 fprintf( fd, "%s\n", dest );
	 k = 0;
	 l = 0;
	 sf_buffer_copy(&dest[l], &src[j], k, 80 - l ) ;
	 l += j;
      } else {
	 l += k + strlen(src);
	 k = 0;
      }
   }
   fprintf( fd, "%s\n", dest );

   if ( ! fmt ) {
      fmt = "%# .4E";
   }
   sprintf( format, "%s", fmt );
   
   l = 0;
   m = 0;
   while ( (wds = wavetable_get_dataset( wt, n++)) ){
   
      for ( i = 0 ; i < wt->nswvars ; i++ ){
	 double val = dataset_swval_get( wds, i );
	 sprintf( src, fmt, val );
	 k =  strlen(src);
	 sf_buffer_copy(&dest[l], src, 0, k ) ;
	 l += k ;
	 if (++m >= 7 ){
	    m = 0;
	    l = 0;
	    fprintf( fd, "%s\n", dest);
	 }
      }
	 
      for ( i = 0 ; i < wds->nrows ; i++ ){
	 for ( j = 0 ; j < wds->ncols ; j++ ){
	    double val = dataset_val_get( wds, i, j );
	    sprintf( src, fmt, val );
	    k =  strlen(src);
	    sf_buffer_copy(&dest[l], src, 0, k ) ;
	    l += k ;
	    if (++m >= 7 ){
	       m = 0;
	       l = 0;
	       fprintf( fd, "%s\n", dest);
	    }
	 }
      }
      app_strcpy( src, " 1.0000E+30");
      k =  strlen(src);
      sf_buffer_copy(&dest[l], src, 0, k ) ;
      l += k ;
      m = 0;
      l = 0;
      fprintf( fd, "%s\n", dest);
   }
}
