/*
 * wavetable.c - wavetable interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <strmem.h>
#include <wavetable.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 * wave table regroup all datasets 
 */

/*
 *** \brief Allocates memory for a new WaveTable object.
 */

WaveTable *wavetable_new( AppClass *datafile, char *tblname )
{
   WaveTable *wt;

   wt =  app_new0(WaveTable, 1);
   wavetable_construct( wt, datafile, tblname );
   app_class_overload_destroy( (AppClass *) wt, wavetable_destroy );
   return wt;
}

/** \brief Constructor for the WaveTable object. */

void wavetable_construct( WaveTable *wt, AppClass *datafile, char *tblname )
{
   app_class_construct( (AppClass *) wt );

   wt->tables = g_ptr_array_new ();
   wt->datafile = datafile;
   wt->tblname = app_strdup(tblname);
   wt->swvars = g_ptr_array_new ();
   
   wavetable_add(wt); /* create 1 dataset */
}

/** \brief Destructor for the WaveTable object. */

void wavetable_destroy(void *wt)
{
   WaveTable *this = (WaveTable *) wt;
   int i;
   
   if (wt == NULL) {
      return;
   }
   for ( i = 0 ; i < this->nswvars ; i++ ) {
      WaveVar *swvar = g_ptr_array_index( this->swvars, i );
      wavevar_destroy ( swvar) ;
   }
   g_ptr_array_free (this->swvars, FALSE);
   for (i = 0; i < this->ntables ; i++) {
      WDataSet *wds = g_ptr_array_index( this->tables, i );
      dataset_destroy( wds );
   }
   g_ptr_array_free (this->tables, FALSE);
   app_free(this->tblname);
  
   /* now nuke the reading object */
   if ( this->streamDestroyFunc ){
      (this->streamDestroyFunc)( this->dataSrc);
   }

   app_class_destroy( wt );
}

void wavetable_set_funcs ( WaveTable *wt, AppClass *src, ReadVarsFP hdrfunc,
			   ReadDataFP rdfunc, StreamDestroyFP destroyfunc  )
{
   wt->dataSrc = src;
   wt->rdVarsFunc = hdrfunc;
   wt->rdDataFunc = rdfunc;
   wt->streamDestroyFunc = destroyfunc;
}
			   
void wavetable_set_datafile ( WaveTable *wt, void *datafile )
{
   wt->datafile = datafile;
}
			   
char *wavetable_get_tblname ( WaveTable *wt )
{
   return wt->tblname;
}
			   
void *wavetable_get_datafile ( WaveTable *wt )
{
   return wt->datafile;
}
			   
int wavetable_get_ntables ( WaveTable *wt )
{
   return wt->ntables;
}
			   
WDataSet *wavetable_add( WaveTable *wt)
{
   WDataSet *wds = dataset_new(0, (AppClass *) wt, wt->ntables);

   g_ptr_array_add (wt->tables, (gpointer) wds);
   wt->ntables++;
   return wds;
}

WDataSet *wavetable_get_dataset( WaveTable *wt, int idx)
{
   if ( idx < 0 || idx >= wt->ntables ) {
      return NULL;
   }
   WDataSet *wds = g_ptr_array_index( wt->tables, idx );

   return wds;
}

WDataSet *wavetable_get_cur_dataset( WaveTable *wt)
{
   return wavetable_get_dataset( wt, wt->ntables - 1);
}

void wavetable_remove( WaveTable *wt, int idx)
{
   if ( idx < 0 || idx >= wt->ntables ) {
      msg_error( _("Try to remove not existing index %d ntables %d"), 
		 idx, wt->ntables );
      return;
   }
   WDataSet *wds = g_ptr_array_index( wt->tables, idx );

   if ( g_ptr_array_remove (wt->tables, (gpointer) wds) ) {
      dataset_destroy (wds);
      wt->ntables--;
   }
}

void wavetable_swvar_add(WaveTable *wt, char *varName, int type, int ncols)
{
   WaveVar *swvar = wavevar_new( NULL, varName, type, wt->nswvars, ncols);
   wavevar_set_wavetable(swvar, (AppClass *) wt, 0 );

   g_ptr_array_add (wt->swvars, (gpointer) swvar);
   wt->nswvars++;
}

char *wavetable_swvar_name_get(WaveTable *wt, int index)
{
   WaveVar *swvar = g_ptr_array_index( wt->swvars, index );

   return swvar->varName;
}

int wavetable_is_multisweep( WaveTable *wt)
{
   if ( wt->nswvars ){
      return 1;
   }
   return 0;
}

int wavetable_read_vars( WaveTable *wt, int n)
{
   int ret;
   
   if ( wt->rdVarsFunc ) {
      ret = wt->rdVarsFunc ( wt->dataSrc );
      if ( ret <= 0 ) {
	 if ( n == 0 ) {
	    return ret;
	 } else {
	    msg_dbg("Removing Table %d", wt->ntables - 1);
	    wavetable_remove( wt, wt->ntables - 1);
	    return 0;
	 }
      }
   }
   return 1;
}

int wavetable_fill_tables( WaveTable *wt, char *filename )
{
   int ret;
   int n = 0;
  
   while ( 1 ) {
      if ( (ret = wavetable_read_vars( wt, n)) != 1 ) {
	 return ret;
      }

      if ( wt->rdDataFunc ) {
	 WDataSet *wds;
      
	 ret = wt->rdDataFunc( wt->dataSrc );
         wds = wavetable_get_cur_dataset(wt);      
	 msg_dbg("Table %d read with %d rows, ret code %d",
		  wt->ntables - 1, wds->nrows, ret );
         if ( ret == 0 && wds->nrows == 0 ) {
            msg_warning("Empty table in file %s\n"
                        "Please, respect the file format.\n",
                        filename );
	    wavetable_remove( wt, wt->ntables - 1);
	 }
	 if ( ret < 2 ) {
	    break;
	 }
	 wavetable_add( wt);
	 n++;
      }
   }
   return ret;
}

/*
 * Iterate over all WaveVars in all sweeps/segments in the WaveFile,
 * calling the function for each one.
 */
void
wavetable_foreach_wavevar(WaveTable *wt, GFunc func, gpointer *p)
{
   WaveVar *var;
   WDataSet *wds;
   int i, j;
        
   for ( i = 0; i < wt->ntables; i++) {
      wds = g_ptr_array_index(wt->tables, i);
      if (wds->nrows > 0 ){
         for (j = 1 ; j < wds->numVars; j++) {
            var = (WaveVar *) dataset_get_wavevar( wds, j );
            (func)(var, p);
         }
      }
   }
}

/*
 * find a cooresponding name in new dataset after file reload
 * with corresponding table num
 */
WaveVar *wavetable_get_var_for_name(WaveTable *wt,  char *varName, int tblno )
{
   WDataSet *wds = wavetable_get_dataset( wt, tblno);
   if ( wds == NULL ){
      return NULL;
   }
   return ( WaveVar *) dataset_get_var_for_name(wds, varName );
}
