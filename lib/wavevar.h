#ifndef SPICEVAR_H
#define SPICEVAR_H

/*
 * wavevar.h - wavevar protocol interface
 * 
 * include LICENSE
 */

typedef struct _WaveVar WaveVar;

#include <dataset.h>

typedef enum {
        UNKNOWN = 0,
        TIME = 1,
        VOLTAGE = 2,
        CURRENT = 3,
        FREQUENCY = 4,
} VarType;


struct _WaveVar {
   AppClass parent;
   int type;                /* type of independant variable */
   char *varName;           /*  name of the variable  (column) */
   WDataSet *wds;           /* data for one or more columns */
   int colno;               /* index (column) in WDataSet   */
   int ncols;               /* num cols used by this variable (complex is 2) */
   AppClass *wvtable;       /* backpointer to the caller Wave var table  */
   int tblno;               /* number of the table containing this var */
   double prev_y2;          /* yval[-2]  temp value              */
   double prev_y1;          /* yval[-1]  temp value              */
};

/*
 * prototypes
 */

WaveVar *wavevar_new( WDataSet *wds, char *varName, int type, int colno, int ncols );
void wavevar_construct( WaveVar *wv, WDataSet *wds, char *varName,
			 int type, int colno, int ncols );
void wavevar_destroy(void *wv);

char *wavevar_get_name(WaveVar *wv );
void wavevar_dup_name(WaveVar *wv, char *varName );
double wavevar_interp_value(WaveVar *dv, double ival);
double wavevar_maxof_value(WaveVar *dv, double ival, int npoints);
double wavevar_get_yvalue(WaveVar *dv, double ival, int npoints);

int wavevar_get_type(WaveVar *wv );
void wavevar_set_type(WaveVar *wv, int type );
void wavevar_set_wavetable(WaveVar *wv, AppClass *wt, int tblno );
char *wavevar_get_label(WaveVar *var, int tag);

double wavevar_ivar_get(WaveVar *var, int row );
double wavevar_val_get(WaveVar *var, int row );
int wavevar_nrows_get(WaveVar *var);

double wavevar_val_get_col_min(WaveVar *var, int col );
double wavevar_val_get_col_max(WaveVar *var, int col );
double wavevar_val_get_min(WaveVar *wv );
double wavevar_val_get_max(WaveVar *wv );
double wavevar_ivar_get_max(WaveVar *wv );
double wavevar_ivar_get_min(WaveVar *wv );

char *wavevar_type2str( int type);
int wavevar_str2type( char *s);

#endif /* SPICEVAR_H */
