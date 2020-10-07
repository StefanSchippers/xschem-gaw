#ifndef GAWMISC_N_H
#define GAWMISC_N_H

/*
 * gawmisc.h -  misc functions
 * 
 * include LICENSE
 */

enum _ConversionMode {
   CNV_NORMAL = 0,
   CNV_SCIENTIFIC,
};



char *val2str_lb(double val, int mode);
char *val2str_prec(double val, int mode, int ddigits);
char *val2str(double val, int mode);
double val_parse(double val, int *idx, int *size);
char *suffix_get( int idx, int mode);
double suffix_get_mult( int idx);
double str2val(char *s);
double find_rounded ( double val, int *sz);

#endif /* GAWMISC_N_H */
