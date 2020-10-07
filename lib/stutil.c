/*
 * stutil.c - string utility functions
 * 
 * include LICENSE
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h> /* isupper */

#include <stutil.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 * return a token delimited by: skipstr Token endstr
 *    set *sn to skipstr Token endstr skipstr
 * If sep, return the separator.
 */
char *stu_token_next_sep(char **sn, char *endstr, char *skipstr, char *sep)
{
   char *s;
   char *e;

   if ( ! sn ) {
      return NULL;
   }
   if ( *sn ) {
      s = *sn + strspn(*sn, skipstr ); /* skip blank before */
      e = s + strcspn(s, endstr);     /* find token end */
      if ( e >= s ) {
         if ( sep ) {
            *sep = *e;
         }
/*
         if ( e == s ) {  * empty string *
            s = NULL;
         }
*/
	 if ( *e ) {
	    *e++ = 0;
	    e += strspn(e, skipstr );  /* skip blank after */
	 }
	 *sn = *e ? e : NULL;
	 return s ;
      }
   }
   return NULL;
}

char *stu_token_next(char **sn, char *endstr, char *skipstr)
{
   return stu_token_next_sep(sn, endstr, skipstr, NULL);
}

/*
 * delimit string terminated by '\0' and may be followed by other strings.
 * Result is valid if resultant size >= 0.
 */
char *stu_str_next(char **sn, int *size)
{
   char *tok = *sn;
   
   *sn = strchr (tok, '\0');
   *sn += 1;
   *size = *size - (*sn - tok);
   return tok;
}


/*
 * remove blank at end of string
 *  initial string modified
 */
void stu_rm_blank_at_end(char *s)
{
   if ( ! s ) {
      return;
   }
   char *e = s + strlen(s) - 1 ;
   while( e >= s && ( *e == ' ' || *e == '\t' ) ) {
      *e = 0 ;
      e-- ;
   }
}
/*
 * remove blank at end of string
 */
char *stu_rm_blank_at_start(char *s)
{
   if ( ! s ) {
      return NULL;
   }
   while(  *s == ' ' || *s == '\t'  ) {
      s++ ;
   }
   return s;
}

/*
 * remove blank at start and end of string
 *  initial string modified
 */

char *stu_rm_blank(char *s)
{
   if ( ! s ) {
      return NULL;
   }
   stu_rm_blank_at_end(s);
   return stu_rm_blank_at_start(s);
}

/*
 *  remove quote around a stringg
 */
char *stu_unquote(char *str )
{
   char *s;
   char *e;
   
   if ( (s = strchr(str, '"' )) == NULL ){
      return str;
   }
   s++;
   if ( (e = strrchr(s, '"' )) != NULL ) {
      *e = 0;
   }
   return s;
}

/*
 * convert string to upper case on place
 */
char * stu_toupper(char *p)
{
   char *s = p;
   for (  ; *p ; p++ ) {
      if ( islower((int) *p) )
         *p = toupper((int) *p) ;
   }
   return s;
}

/*
 * get a filename extension
 */
char * stu_ext_get(char *path)
{
   char *ext = strrchr(path, '.');
   if ( ! ext ) {
      return path;
   }
   ext++;
   return ext;
}
