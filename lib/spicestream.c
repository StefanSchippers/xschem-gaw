/*
 * spicestream.c - spicestream interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <spicestream.h>
#include <fileformat.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 *** \brief Allocates memory for a new SpiceStream object.
 */

SpiceStream *spicestream_new( char *filename, char *format, WaveTable *wt )
{
   SpiceStream *ss;

   ss =  app_new0(SpiceStream, 1);
   spicestream_construct( ss, filename, format, wt );
   app_class_overload_destroy( (AppClass *) ss, spicestream_destroy );
   return ss;
}

/** \brief Constructor for the SpiceStream object. */

void spicestream_construct( SpiceStream *ss, char *filename, char *format, WaveTable *wt)
{
   app_class_construct( (AppClass *) ss );
   
   ss->wt = wt;
   wavetable_set_funcs( wt, (AppClass *) ss, spicestream_read_hdr,
			spicestream_read_rows, spicestream_destroy );
   ss->filename = filename;
   ss->format = format;
   
    /* open file */
   ss->linebuf = fdbuf_new ( filename, "r", 0);
   if ( ss->linebuf->status ) {
      ss->status = ss->linebuf->status;
      return;
   }
   /* fdbuf_set_flags(ss->linebuf, FDB_STRIP_CR); set by default */ 
   ss->dbuf = dbuf_new( 0, 0 );
}

/** \brief Destructor for the SpiceStream object. */

void spicestream_destroy(void *ss)
{
   SpiceStream *this = (SpiceStream *) ss;

   if (ss == NULL) {
      return;
   }
   fdbuf_destroy( this->linebuf);
   g_free (this->rowData);
   dbuf_destroy(this->dbuf);
   if ( this->datrow ) {
      g_free (this->datrow);
   }
   if ( this->nsindexes ) {
      g_free (this->nsindexes);
   }
   
   app_class_destroy( ss );
}

/*
 * sequential add of values
 */
void spicestream_val_add( SpiceStream *ss, double val)
{
   ss->rowData[ss->curcol] = val;
   if ( ++ss->curcol >= ss->ncols ){
      ss->curcol = 0;
   }
}

void spicestream_var_add( SpiceStream *ss, char *varName, int type, int ncols)
{
   dataset_var_add( ss->wds, varName, type,  ncols);
}

/*
 * Warning :  fileformat_read_header do not return the same code as
 *  (ss->readHdrFunc) 
 * fileformat_read_header: >= 0 index of format, < 0 error or EOF
 *  (ss->readHdrFunc) : -1 error, 0 EOF, 1 succes
 */

int spicestream_read_hdr( AppClass *sss )
{
   SpiceStream *ss = (SpiceStream *) sss;
   int ret;

   ss->wds = wavetable_get_cur_dataset(ss->wt); /* curent wds */
   if ( ss->flags & SSF_NORDHDR ) {
      /* no read to read header again */
      return 1;
   }
   if ( ss->readHdrFunc ) {
      ret = (ss->readHdrFunc) ( ss );
   } else {
      ret =  fileformat_read_header( ss);
      if ( ret >= 0 ) {
	 if ( !  ss->readHdrFunc ) {
	    ss->readHdrFunc = fileformat_get_readhdr_func(ret);
	 }
	 ss->readRowFunc = fileformat_get_read_func(ret);
	 ret = 1;
      }
   }
   dbuf_clear( ss->dbuf );
   if ( ret > 0 ) {
      ss->ncols = ss->wds->ncols;
      ss->curcol = 0;
      ss->read_vals = ss->read_rows = 0;
      if ( ss->rowData ) {
	 g_free(ss->rowData );
      }
      ss->rowData = g_new0 (double, ss->ncols );
      return 1;
   } else if ( ret < -1 ) { 
      /* clear garbage in vars */
      dataset_remove_all_vars(ss->wds);
      ret = -1;
   }
   dbuf_clear( (DBuf *) ss->linebuf );
   return ret;
}

void  spicestream_create_new_vars (SpiceStream *ss )
{
   int i;
   WDataSet *owds = wavetable_get_dataset(ss->wt, 0 ) ;

   ss->wds = wavetable_get_cur_dataset(ss->wt);   
   for ( i = 0 ; i < owds->numVars ; i++ ) {
      WaveVar *var = g_ptr_array_index( owds->vars, i );

      dataset_var_add(ss->wds, var->varName, var->type, var->ncols);
   }
}


void spicestream_store_data_row(SpiceStream *ss)
{
   int i;
   double *rowData = ss->rowData;
   int ncols = ss->ncols;
   
   for ( i = 0 ; i < ncols ; i++ ){
      dataset_val_add (ss->wds, rowData[i]);
   }
}


/*
 * row is read in temp storage ss->rowData because
 * we do not known if we have multiple set in the file
 * indicated by a decreseing order of iv
 */
int spicestream_read_rows( AppClass *sss )
{
   SpiceStream *ss = (SpiceStream *) sss;
   int ret;
   int row = 0;
   double last_ival= -1.0e29;
   double ival;

   if ( ss->need_update ) {
      if ( ss->flags & SSF_NORDHDR ) {
	 /* create new vars from wds 0 */
	 spicestream_create_new_vars ( ss );
      }
      if ( ss->need_update == 2 ) {
	 spicestream_store_data_row (ss);
      }
      ss->need_update = 0;
   }
   if (ss->nsweepparam && ss->rdSweepFunc ) {
      ret = ss->rdSweepFunc(ss);
      if ( ret != 1 ) {
	 return 0;
      }
   }
   while ((ret = ss->readRowFunc(ss)) > 0) {
      if ( ss->flags & SSF_PUSHBACK ) {
	 ival = ss->rowData[0];
	 if (row > 0 && ival < last_ival) {
	    if (row == 1) {
	       msg_error(_("independent variable is not nondecreasing at row %d; ival=%g last_ival=%g - line %d"),
			  row, ival, last_ival, fdbuf_get_lineno(ss->linebuf) );
	       return -1;
	    } else {
	       ss->need_update = 2;
	       return 2; /* finish the dataset and create a new one */
	    }
	 }
	 spicestream_store_data_row( ss );
	 last_ival = ival;
      }
      if ( ret == 2 ) {
	 return 2;
      }
      row++;
   }
   ss->need_update = 1;
   if ( ret == -2 ) {
      ret = 2 ;     /* we have new tables, without need update */
   }
   return ret;
}

