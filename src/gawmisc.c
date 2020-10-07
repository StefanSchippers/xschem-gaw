/*
 * Analog waveform viewer
 * misc stuff
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>
// #include <stdlib.h>
#include <math.h>

#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 *  CNV_NORMAL CNV_SCIENTIFIC
 */
typedef struct _SuffixTable SuffixTable;
struct _SuffixTable {
   char *normal;
   char *scientific;
   double mult;
};

SuffixTable suffix_table[] = {
   {  "T",        "e12",   1e12   },        /* >=   1e12              */
   {  "G",        "e9" ,   1e9    },        /* >=   1e9    < 1e12     */
   {  "M",        "e6" ,   1e6    },        /* >=   1e6    < 1e9      */
   {  "k",        "e3" ,   1e3    },        /* >=   1e3    < 1e6      */
   {  " ",        " "  ,   1      },        /* >=   1      < 1e3      */
   {  "m",        "e-3",   1e-3   },        /* >=   1e-3   < 1        */
   {  "µ",        "e-6",   1e-6   },        /* >=   1e-6   < 1e-3     */
   {  "n",        "e-9",   1e-9   },        /* >=   1e-9   < 1e-6     */
   {  "p",        "e-12",  1e-12  },        /* >=   1e-12  < 1e-9     */
   {  "f",        "e-15",  1e-15  },        /* >=   1e-15  < 1e-12    */
   {  "a",        "e-18",  1e-18  },        /* >=  epsilon < 1e-15    */
};

char *suffix_get( int idx, int mode)
{
   if (mode == CNV_SCIENTIFIC) {
      return suffix_table[idx].scientific;
   } else {
      return suffix_table[idx].normal;
   }
}

double suffix_get_mult( int idx)
{
   return suffix_table[idx].mult;
}

/*
 * convert double value to printable text.
 * Two modes:
 *   0: using suffixes
 *      We always try to print 4 significant figures, with one nonzero digit
 *      to the left of the decimal point.
 *      maximum field width: 7 characters
 *
 *   1: use scientific notation, printf %g format.
 *      maximum field width appears to be 10 characters
 */

double val_parse(double val, int *idx, int *size)
{
   double aval = fabs(val);
   double sval, asval;

   if (1e12 <= aval) {
      *idx = 0;
      sval = val / 1e12;
   } else if (1e9 <= aval && aval < 1e12) {
      *idx = 1;
      sval = val / 1e9;
   } else if (1e6 <= aval && aval < 1e9) {
      *idx = 2;
      sval = val / 1e6;
   } else if (1e3 <= aval && aval < 1e6) {
      *idx = 3;
      sval = val / 1000;
   } else if (1e-3 <= aval && aval < 1) {
      *idx = 5;
      sval = val * 1000;
   } else if (1e-6 <= aval && aval < 1e-3) {
      *idx = 6;
      sval = val * 1e6;
   } else if (1e-9 <= aval && aval < 1e-6) {
      *idx = 7;
      sval = val * 1e9;
   } else if (1e-12 <= aval && aval < 1e-9) {
      *idx = 8;
      sval = val * 1e12;
   } else if (1e-15 <= aval && aval < 1e-12) {
      *idx = 9;
      sval = val * 1e15;
   } else if (DBL_EPSILON < aval && aval < 1e-15) {
      *idx = 10;
      sval = val * 1e18;
   } else {       /* if ( 1 <= aval && aval < 1e3) */
      *idx = 4;
      sval = val ;
   }
   asval = fabs(sval);
   if (1.0 <=  asval && asval < 10.0) { 
      *size = 1;
   } else if (10.0 <=  asval && asval < 100.0) { 
      *size = 2;
   } else {  /*  if (100.0 <=  asval && asval < 1000.0) */   
      *size = 3;
   }
   return sval;
}

char *val2str(double val, int mode)
{
   static char buf[64];
   double sval;
   int idx;
   int size;

   sval = val_parse( val, &idx, &size);
   sprintf(buf, "% .*f%s", 4 - size, sval, suffix_get(idx, mode) );
//   fprintf(stderr, "digits %d, sval %f buf %s\n", ddigits, sval, buf);
   return buf;
}

char *val2str_prec(double val, int mode, int ddigits)
{
   static char buf[64];
   double sval;
   int idx;
   int size;

   sval = val_parse( val, &idx, &size);
   sprintf(buf, "% .*f%s", ddigits, sval, suffix_get(idx, mode) );
//   fprintf(stderr, "digits %d, sval %f buf %s\n", ddigits, sval, buf);
   return buf;
}

/*
 * remove 0 at end of string
 */
void remove_zero(char *s)
{
   char *dot = 0;
   
   while (*s) {
      if ( *s == '.' ){
	 dot = s;
      }
      s++;
   }
   if ( ! dot ) {
      return;
   }
   s--;
   while (*s) {
      if ( *s == '0' ){
	 *s = 0;
      } else if ( *s == '.' ){
	 *s = 0;
	 break;
      } else {
	 break;
      }
      s--;
   }
}

/*
 * special for label
 *   make shortest as possible
 */
char *val2str_lb(double val, int mode)
{
   static char buf[64];
   char tmpbuf[32];
   double sval;
   int idx;
   int size;

   sval = val_parse( val, &idx, &size);
//   msg_dbg("(%.3f % .3g) idx %d size %d", val, val, idx, size);

   /* 1e-1 <= aval && aval < 1  */
   if ( mode == CNV_NORMAL && idx == 5 && size == 3  ) {
      sprintf(buf, "% .3g", val );
#if 0
   } else if ( mode == CNV_SCIENTIFIC  && 1e-1 <= aval && aval < 1  ) {
      sprintf(buf, "% .3g", val );
#endif
   } else {
      sprintf(tmpbuf, "% .*f", 4 - size, sval );
      remove_zero(tmpbuf);
      sprintf(buf, "%s%s", tmpbuf, suffix_get(idx, mode) );
   }
//   msg_dbg("sval %f buf %s", sval, buf);
   return buf;
}


/*
 *  convert a string to double 
 *  given a string containing a number, possibly with a spice-syntax suffix,
 *  return the value
 */
double str2val(char *s)
{
   double val, mul;
   char *ep;
   val = strtod(s, &ep);

   if(ep && ep != s && *ep) {
      switch (*ep) {
       case 'T':
	 mul = 1e12;
	 break;
       case 'G':
	 mul = 1e9;
	 break;
       case 'M':
	 mul = 1e6;
	 break;
       case 'k':
	 mul = 1e3;
	 break;
       case 'm':
	 mul = 1e-3;
	 break;
//       case 'µ':
       case 'u':
	 mul = 1e-6;
	 break;
       case 'n':
	 mul = 1e-9;
	 break;
       case 'p':
	 mul = 1e-12;
	 break;
       case 'f':
	 mul = 1e-15;
	 break;
       case 'a':
	 mul = 1e-18;
	 break;
       default:
	 mul = 1;
	 break;
      }
      return val * mul;
   } else {
      return val;
   }
}

/*
 * find a rounded value for val
 */
   
int round_table[] = { 100, 125, 200, 250, 500, 1000 };
double find_rounded ( double val, int *sz)
{
   int i;
   int idx;
   int size;
   int n = sizeof(round_table) / sizeof(round_table[0]);
   double nval;

   if ( val == 0.0 ){
      msg_dbg("%f", val);
      return 0.0;
   }
   nval = val_parse( val, &idx, &size);
   if ( size < 3 ) { 
      nval *=  pow( 10.0, (double) (3 - size) );
   }
   if ( val > 0 ){
      for ( i = 0 ; i < n ; i++ ){
	 if ( nval <= (double) round_table[i] ) {
	    break;
	 }
      }
      nval = (double) round_table[i];
   } else {
      nval = fabs(nval);
      for ( i = n - 1 ; i >= 0 ; i-- ){
	 if ( nval >= (double) round_table[i] ) {
	    break;
	 }
      }
      nval = - (double) round_table[i];
   }
   if ( size < 3 ) { 
      nval = nval / pow( 10.0, (double) (3 - size) );
   }
   msg_dbg("%f %f %d idx %d size %d", val, nval , round_table[i], idx, size);
   nval *= suffix_get_mult( idx );
   if ( sz ){
      *sz = size;
   }
   return nval;
}
