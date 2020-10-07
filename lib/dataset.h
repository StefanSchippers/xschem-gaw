#ifndef DATASET_H
#define DATASET_H

/*
 * dataset.h - dataset interface
 * 
 * include LICENSE
 */

/*
 *  Array Storage
 *   array for each column hold in a GPtrArray
 *
 */

#include <appclass.h>

typedef struct _WDataSet WDataSet;

struct _WDataSet {
   AppClass parent;
   AppClass *wvtable;        /* back pointer to the holding table */
   GPtrArray *datas;         /* array of pointer to array containing double data */
   int ncols;                /* number of column in array */
   int nrows;                /* number of rows in array */
   GPtrArray *vars;          /* array of WaveVar ; 0 = iv */
   int numVars;              /* number of var in array  vars */
   GArray *colMin;           /* array of min value - the col min value */
   GArray *colMax;           /* array of max value - the col max value */
   GArray *swvals;           /* array of value at which the sweep was taken */
   int nswvals;              /* number of vals in swvals */
   int currow;
   int curcol;
   int curvar;
   int tblno;                /* number of the table                  */
   char *setname;            /* name of the sweep, if any, else NULL */
   GArray *varmap;           /* array of int map col num to var num  */
   int mapshift;             /* number of bit to shift for the index */
};

/*
 * prototypes
 */
WDataSet *dataset_new(  int ncols, AppClass *wt, int tblno );
void dataset_construct( WDataSet *wds, int ncols, AppClass *wt, int tblno );
void dataset_destroy(void *wds);

void dataset_col_add(WDataSet *wds, int ncols);
void dataset_col_remove(WDataSet *wds, int colno, int ncols);
void dataset_var_add(WDataSet *wds, char *varName, int type, int ncols);
void dataset_var_remove(WDataSet *wds, int ivar);
void dataset_remove_all_vars(WDataSet *wds);
void dataset_col_val_add (WDataSet *wds, int row, int col, double val);
void dataset_val_add (WDataSet *wds, double val);
double dataset_val_get(WDataSet *wds, int row, int col );
int dataset_get_nrows(WDataSet *wds );
int dataset_get_ncols(WDataSet *wds );
int dataset_find_row_index( WDataSet *wds, double ival);
void dataset_dup_var_name(WDataSet *wds, char *varName, int ivar);
int dataset_get_wavevar_type(WDataSet *wds, int ivar );
void dataset_set_wavevar_type(WDataSet *wds, int ivar, int type );
void dataset_dup_setname(WDataSet *wds, char *setname);
char *dataset_get_setname(WDataSet *wds);
void dataset_dup_label(WDataSet *wds, char *label);
void dataset_swval_add (WDataSet *wds, double val);
double dataset_swval_get(WDataSet *wds, int index );
void dataset_min_max_init( WDataSet *wds, int col );
void dataset_val_set_min_max( WDataSet *wds, int col, double val );
double dataset_val_get_min(WDataSet *wds, int col );
double dataset_val_get_max(WDataSet *wds, int col );

AppClass *dataset_get_wavevar(WDataSet *wds, int ivar );
AppClass *dataset_get_var_for_name(WDataSet *wds, char *varName );

#endif /* DATASET_H */
