/*
 * wavevar.c - wavevar interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <wavetable.h>
#include <wavevar.h>
#include <strmem.h>
#include <util.h>
#include <duprintf.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 *** \brief Allocates memory for a new WaveVar object.
 */

WaveVar *wavevar_new( WDataSet *wds, char *varName, int type, int colno, int ncols )
{
   WaveVar *wv;

   wv =  app_new0(WaveVar, 1);
   wavevar_construct( wv, wds, varName, type, colno, ncols );
   app_class_overload_destroy( (AppClass *) wv, wavevar_destroy );
   return wv;
}

/** \brief Constructor for the WaveVar object. */

void wavevar_construct( WaveVar *wv, WDataSet *wds, char *varName,
			 int type, int colno , int ncols )
{
   app_class_construct( (AppClass *) wv );
   
   wv->wds = wds;
   wv->varName = app_strdup(varName);
   wv->type = type;
   wv->colno = colno ;
   wv->ncols = ncols ;
}

/** \brief Destructor for the WaveVar object. */

void wavevar_destroy(void *wv)
{
   WaveVar *this = (WaveVar *) wv;

   if (wv == NULL) {
      return;
   }
   app_free(this->varName);

   app_class_destroy( wv );
}

char *wavevar_get_name(WaveVar *wv )
{
   return wv->varName;
}

int wavevar_get_type(WaveVar *wv )
{
   return wv->type;
}

void wavevar_set_type(WaveVar *wv, int type )
{
   wv->type = type;
}

void wavevar_set_wavetable(WaveVar *var, AppClass *wt, int tblno )
{
   var->wvtable = wt;
   var->tblno = tblno;
}

void wavevar_dup_name(WaveVar *wv, char *varName )
{
   wv->varName = app_strdup(varName);
}

double wavevar_val_get_col_min(WaveVar *var, int col )
{
   return dataset_val_get_min(var->wds, var->colno + col);
}

double wavevar_val_get_col_max(WaveVar *var, int col )
{
   return dataset_val_get_max(var->wds, var->colno + col);
}

double wavevar_val_get_min(WaveVar *var )
{
   return wavevar_val_get_col_min(var, 0 );
}

double wavevar_val_get_max(WaveVar *var )
{
   return wavevar_val_get_col_max(var, 0);
}

/*
 * get the value of ivar at row
 */
double wavevar_ivar_get(WaveVar *var, int row )
{
   return dataset_val_get(var->wds, row, 0 );
}

/*
 * get the value of var at row
 */
double wavevar_val_get(WaveVar *var, int row )
{
   return dataset_val_get(var->wds, row, var->colno );
}

int wavevar_nrows_get(WaveVar *var)
{
   return dataset_get_nrows(var->wds);
}

/*
 * get the min value of the independant variable of the dataset
 */
double wavevar_ivar_get_min(WaveVar *var )
{
   WaveVar *ivar= (WaveVar *) dataset_get_wavevar(var->wds, 0 );

   return wavevar_val_get_col_min(ivar, 0 );
}

double wavevar_ivar_get_max(WaveVar *var )
{
   WaveVar *ivar= (WaveVar *) dataset_get_wavevar(var->wds, 0 );

   return  wavevar_val_get_col_max(ivar, 0);
}


/*
 * return the value of the dependent variable dv at the point where
 * its associated independent variable has the value ival.
 *
 * FIXME:tell 
 * make this fill in an array of dependent values,
 * one for each column in the specified dependent variable.
 * This will be better than making the client call us once for each column,
 * because we'll only have to search for the independent value once.
 * (quick hack until we need support for complex and other multicolumn vars:
 * just return first column's value.)
 */
double wavevar_interp_value(WaveVar *dv, double ival)
{
   int li, ri;     /* index of points to left and right of desired value */
   double lx, rx;  /* independent variable's value at li and ri */
   double ly, ry;  /* dependent variable's value at li and ri */
   WDataSet *wds = dv->wds;
   int nrows = dataset_get_nrows(wds);
   
   if ( nrows <= 0 ) {
      return 0.0;
   }
   
   li = dataset_find_row_index(wds, ival);
   ri = li + 1;
   if (ri >= nrows ) {
      return dataset_val_get(wds, nrows - 1, dv->colno );
   }

   lx = dataset_val_get(wds, li, 0 );
   rx = dataset_val_get(wds, ri, 0 );
   if (li > 0 && lx > ival) {
      msg_error(_("assertion failed: lx <= ival for %s: ival=%g li=%d lx=%g"),
		 dv->varName, ival, li, lx);
   }

   ly = dataset_val_get(wds, li, dv->colno );
   ry = dataset_val_get(wds, ri, dv->colno );

   if (ival > rx ) { /* no extrapolation allowed! */
      return ry;
   }
   
//double val =  ly + (ry - ly) * ((ival - lx)/(rx - lx));
//fprintf(stderr, "ival = %f, li = %d ri = %d, lx = %g, rx = %g, ly = %g, ry = %g, val = %g\n",
//	   ival, li, ri, lx, rx, ly, ry, val);
   return ly + (ry - ly) * ((ival - lx) / (rx - lx));
}

/*
 * map npoints data to 1 point pixel
 */
double wavevar_maxof_value(WaveVar *dv, double ival, int npoints)
{
   WDataSet *wds = dv->wds;
   int i;
   double val;
   double yval0 = G_MAXDOUBLE;
   double yval1 = -G_MAXDOUBLE;
   int  ri;     /* index of points to left and right of desired value */
   int nrows = dataset_get_nrows(wds);
   
   if ( nrows <= 0 ) {
      return 0.0;
   }
   
   ri = dataset_find_row_index(wds, ival);
   val = dataset_val_get(wds, ri, 0 );
   if (ival != val ) {
      ri++;
   }

//   fprintf(stderr, "ri %d, ival = %f val %f\n", ri, ival, val);

   for ( i = 0 ; i < npoints ; i++ ) {
      if (ri >= nrows ) {
	 ri = nrows - 1;
      }
      val = dataset_val_get(wds, ri, dv->colno );
      yval0 = MIN( yval0, val);
      yval1 = MAX( yval1, val);
// fprintf(stderr, "ri %d, val = %f, yval = %g\n", ri, val, yval);
      ri++;
   }
//   fprintf(stderr, "\n");
   val = yval1;
   if (  yval0 == yval1 ) {
      val = yval0;
   } else if ( yval0 < dv->prev_y2 ) {
      val = yval0;
   }
   dv->prev_y2 = dv->prev_y1;
   dv->prev_y1 = val;
   return val;
}

double wavevar_get_yvalue(WaveVar *dv, double ival, int npoints)
{
   if ( npoints < 3 ){
      return wavevar_interp_value( dv, ival);
   } else {
      return wavevar_maxof_value( dv, ival, npoints);
   }
}

/*
 * create buuton label
 * if tag < 0 , we are in a list window, do not indicate file tag.
 */

char *wavevar_get_label(WaveVar *var, int tag )
{
   char buf[16];
   
   buf[0] = 0;
   if ( tag >= 0 ){
      snprintf(buf, 16, "%d: ", tag );
   }
   if ( wavetable_is_multisweep( (WaveTable *) var->wvtable) ) {
      return app_strdup_printf( "%s%s @ %s=%g", buf, var->varName, 
                        wavetable_swvar_name_get( (WaveTable *) var->wvtable, 0),
			dataset_swval_get( var->wds, 0) );
   } else if ( wavetable_get_ntables ( (WaveTable *) var->wvtable) > 1 ) {
      /* multitable anyway */
      return app_strdup_printf( "%s%s : %d", buf, var->varName, var->tblno );
   }
   return app_strdup_printf( "%s%s", buf, var->varName) ;
}
/*
 * convert variable type string from spice3 raw file to 
 * our type numbers
 */

NameValue strtype[] = {
   {  "unknown",       UNKNOWN            },
   {  "time",          TIME               },
   {  "voltage",       VOLTAGE            },
   {  "current",       CURRENT            },
   {  "frequency",     FREQUENCY          },
};

int wavevar_str2type( char *s)
{
   return uti_nv_find_in_table( s, strtype,
                     sizeof( strtype ) / sizeof( strtype[0] ) );
}


char *wavevar_type2str( int type)
{
   return uti_nv_find_in_table_str( type, strtype ,
				    sizeof( strtype ) / sizeof( strtype[0] ) );
}


