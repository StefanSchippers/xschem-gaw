/*
 * dataset.c - dataset interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <strmem.h>
#include <dataset.h>
#include <wavevar.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 *** \brief Allocates memory for a new WDataSet object.
 */

WDataSet *dataset_new( int ncols, AppClass *wt, int tblno )
{
   WDataSet *wds;

   wds =  app_new0(WDataSet, 1);
   dataset_construct( wds, ncols, wt, tblno );
   app_class_overload_destroy( (AppClass *) wds, dataset_destroy );
   return wds;
}

/** \brief Constructor for the WDataSet object. */

void dataset_construct( WDataSet *wds, int ncols, AppClass *wt, int tblno )
{
   app_class_construct( (AppClass *) wds );

   wds->ncols = ncols;
   wds->datas = g_ptr_array_new ();
   wds->vars = g_ptr_array_new ();
   wds->colMin = g_array_new (FALSE, FALSE, sizeof (double) );
   wds->colMax = g_array_new (FALSE, FALSE, sizeof (double) );
   wds->swvals = g_array_new (FALSE, FALSE, sizeof (double) );
   wds->wvtable = wt;
   wds->tblno = tblno;
   wds->varmap = g_array_new (FALSE, FALSE, sizeof (int) );
   wds->mapshift = 1;
}

/** \brief Destructor for the WDataSet object. */

void dataset_destroy(void *wds)
{
   WDataSet *this = (WDataSet *) wds;

   if (wds == NULL) {
      return;
   }
   app_free(this->setname);
   dataset_remove_all_vars( this);
   g_ptr_array_free (this->vars, FALSE);
   g_ptr_array_free (this->datas, TRUE);
   g_array_free (this->colMin, TRUE);
   g_array_free (this->colMax, TRUE);
   g_array_free (this->swvals, TRUE);
   g_array_free (this->varmap, TRUE);

   app_class_destroy( wds );
}

/*
 * given a col num, return the corresponding var index
 */

int dataset_map_col_to_var( WDataSet *wds, int icol )
{
   int map = g_array_index (wds->varmap, int, icol );
   return (map >> wds->mapshift);
}

void dataset_varmap_remap(WDataSet *wds, int ncols)
{
   int shift = wds->mapshift;
   int oldmap;
   int i;
   
   while ( (1 << shift) < ncols ) {
      shift <<= 1;
   }
   for ( i = 0 ; i < wds->ncols ; i++ ) {
      oldmap = g_array_index (wds->varmap, int, i );
      int col = oldmap & ((1 << wds->mapshift) - 1);
      int varno = oldmap >> wds->mapshift;
      g_array_index (wds->varmap, int, i ) = ( varno << shift) + col; 
   }
   wds->mapshift = shift;
}

void dataset_col_add(WDataSet *wds, int ncols)
{
   int i ;
   int map;
   double val;
   
   if ( (1 << wds->mapshift) < ncols ) {
      dataset_varmap_remap(wds, ncols);
   }
   
   for ( i = 0 ; i < ncols ; i++ ) {
      map = (wds->numVars << wds->mapshift) + i ;
      g_array_append_val(wds->varmap, map ); /* at curcol */
      
      GArray *ary = g_array_new (FALSE, FALSE, sizeof (double) );
      g_ptr_array_add (wds->datas, (gpointer) ary);

      val = G_MAXDOUBLE;
      g_array_append_val(wds->colMin, val);
      val = -G_MAXDOUBLE;
      g_array_append_val(wds->colMax, val );
      
      wds->ncols++;
   }
}

void dataset_var_add(WDataSet *wds, char *varName, int type, int ncols)
{
   WaveVar *var = wavevar_new(wds, varName, type, wds->ncols, ncols);

   dataset_col_add(wds, ncols);
   wavevar_set_wavetable(var, wds->wvtable, wds->tblno );
   
   g_ptr_array_add (wds->vars, (gpointer) var);
   wds->numVars++;
}

void dataset_col_remove(WDataSet *wds, int colno, int ncols)
{
   for (    ; ncols > 0 ; ncols-- ) {
      GArray *ary = g_ptr_array_index( wds->datas, colno + ncols - 1 );
      g_array_remove_index ( wds->varmap, colno + ncols - 1 );   
      g_ptr_array_remove (wds->datas, (gpointer) ary);
      g_array_free (ary, TRUE);
      wds->ncols--;
   }
}

/*
 * if ivar != numVars - 1 we should renumber
 * all the vars beeteen ivar and numVars - 1 !!!
 */
void dataset_var_remove(WDataSet *wds, int ivar)
{
   WaveVar *var = g_ptr_array_index( wds->vars, ivar );
   
   dataset_col_remove(wds, var->colno, var->ncols);
   g_ptr_array_remove (wds->vars, (gpointer) var);
   wavevar_destroy( var);

   wds->numVars--;
}

void dataset_remove_all_vars(WDataSet *wds)
{
   int nvars = wds->numVars ;
   
   for (  ; nvars ; nvars-- ) {
      dataset_var_remove(wds, nvars - 1);
   }
}

void dataset_min_max_init( WDataSet *wds, int col )
{
   g_array_index (wds->colMin, double, col ) = G_MAXDOUBLE;
   g_array_index (wds->colMax, double, col ) = -G_MAXDOUBLE;
}

void dataset_val_set_min_max( WDataSet *wds, int col, double val )
{
   double *prev = &g_array_index (wds->colMin, double, col );
   if ( val < *prev ) { 
      g_array_index (wds->colMin, double, col ) = val;
   }
   prev = &g_array_index (wds->colMax, double, col );
   if ( *prev < val ) { 
      g_array_index (wds->colMax, double, col ) = val;
   }
}

double dataset_val_get_min(WDataSet *wds, int col )
{
   double *min = &g_array_index (wds->colMin, double, col );
   return *min;
}

double dataset_val_get_max(WDataSet *wds, int col )
{
   double *max = &g_array_index (wds->colMax, double, col );
   return *max;
}

/*
 *  add or replace val in an array
 */

void dataset_col_val_add (WDataSet *wds, int row, int col, double val)
{
   GArray *ary = g_ptr_array_index( wds->datas, col );
   
   if (  row < 0 || row >= ary->len ){
      g_array_append_val(ary, val );
   } else {
      g_array_index (ary, double, row ) = val;
   }

   dataset_val_set_min_max( wds, col, val );
}

/*
 * add a cell to array and set a value in a cell
 *   supposing ncols is set previously and
 *   val comes sequentially
 */
void dataset_val_add (WDataSet *wds, double val)
{
   dataset_col_val_add( wds, -1, wds->curcol, val );
   if ( ++wds->curcol >= wds->ncols ) { 
      wds->currow++;  /* currow is not used */
      wds->curcol = 0;
      wds->nrows++;
   }
}

/*
 * add a sweep val to the dataset
 */
void dataset_swval_add (WDataSet *wds, double val)
{
   g_array_append_val(wds->swvals, val );
   wds->nswvals++;
}

double dataset_val_get(WDataSet *wds, int row, int col )
{
   GArray *ary = g_ptr_array_index( wds->datas, col );
   
   double *val = &g_array_index (ary, double, row );
   return *val;
}

double dataset_swval_get(WDataSet *wds, int index )
{
   double *val = &g_array_index (wds->swvals, double, index );
   return *val;
}

void dataset_dup_var_name(WDataSet *wds, char *varName, int idx)
{
   WaveVar *var = g_ptr_array_index( wds->vars, idx );
   wavevar_dup_name(var, varName);
}

void dataset_dup_setname(WDataSet *wds, char *setname)
{
   wds->setname = app_strdup(setname);
}

char *dataset_get_setname(WDataSet *wds)
{
   return wds->setname;
}

int dataset_get_nrows(WDataSet *wds )
{
   return wds->nrows;
}

int dataset_get_ncols(WDataSet *wds )
{
   return wds->ncols;
}

AppClass *dataset_get_wavevar(WDataSet *wds, int ivar )
{
   WaveVar *var = g_ptr_array_index( wds->vars, ivar );
   return (AppClass *) var;
}

AppClass *dataset_get_var_for_name(WDataSet *wds, char *varName )
{
   int i;
   
   for ( i = 0 ; i < wds->numVars ; i++ ) {
      WaveVar *var = (WaveVar *) dataset_get_wavevar( wds, i );
      if ( app_strcmp( varName, var->varName ) == 0 ){
	 return (AppClass *) var;
      }
   }
   return (AppClass *) NULL;
}

int dataset_get_wavevar_type(WDataSet *wds, int ivar )
{
   WaveVar *var = (WaveVar *) dataset_get_wavevar( wds, ivar );
   return wavevar_get_type( var );
}

void dataset_set_wavevar_type(WDataSet *wds, int ivar, int type )
{
   WaveVar *var = (WaveVar *) dataset_get_wavevar( wds, ivar );
   wavevar_set_type( var, type );
}

/*
 * Use a binary search to return the index of the point 
 * whose value is the largest not greater than ival.  
 * if ival is equal or greater than the max value of the
 * independent variable, return the index of the last point.
 *
 * Only works on independent-variables, which we require to
 * be nondecreasing and have only a single column.
 * 
 * Further, if there are duplicate values, returns the highest index
 * that has the same value.
 */
int dataset_find_row_index( WDataSet *wds, double ival)
{
   double cval;
   int a = 0;
   int b;

   b = wds->nrows - 1;
   double colMax = dataset_val_get_max( wds, 0 );
   if (ival >= colMax) {
      return b;
   }
   while ( a + 1 < b) {
      cval = dataset_val_get( wds,  (a + b) / 2, 0 ); /* row, col */
//	printf(" a=%d b=%d ival=%g cval=%g\n", a, b, ival, cval); 
      if (ival < cval){
	 b = ( a + b) / 2;
      } else {
	 a = (a + b) / 2;
      }
   }
   return a;
}

