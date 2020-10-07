/*
 * util.c - utility functions
 * 
 * include LICENSE
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <util.h>
#include <strmem.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 * return a table index from a string
 */
int uti_find_in_table( char *s, char *table[], int size)
{
   int i ;
   int n;
   
   for (i = 0 ; i < size ; i++ ) {
      n = strlen(table[i]);
      if ( app_strncasecmp(s, table[i], n) == 0 ){
	 return i;
      }
   }
   return 0;
}

/*
 * return a string from a table index
 */
char *uti_find_in_table_str( int type, char *table[], int size)
{
   if (type >= size ){
      type = 0;
   }
   return table[type];
}

/*
 * return a table value from a string
 */
int uti_nv_find_in_table( char *s, NameValue table[], int size)
{
   int i ;
   int n;
   
   for (i = 0 ; i < size ; i++ ) {
      n = strlen(table[i].name);
      if ( app_strncasecmp(s, table[i].name, n) == 0 ){
	 return table[i].value;
      }
   }
   return 0;
}

/*
 * return a string from a table index
 */
char *uti_nv_find_in_table_str( int type, NameValue table[], int size)
{
   int i ;
   
   if (type >= size ){
      type = 0;
   }
   for (i = 0 ; i < size ; i++ ) {
      if ( table[i].value == type ){
	 return table[i].name;
      }
   }
   return table[0].name;
}


