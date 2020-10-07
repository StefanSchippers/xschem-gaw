/*
 * strobj.c - simple object to store a string
 *    - needed to store string in DList
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <msglog.h>
#include <strobj.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 *** \brief Allocates memory for a new StrObj object.
 */

StrObj *str_new( char *pstr )
{
   StrObj *str;

   str = app_new0(StrObj, 1);
   str_construct( str, pstr );
   app_class_overload_destroy( (AppClass *) str, str_destroy );
   return str;
}

/** \brief Constructor for the StrObj object. */

void str_construct( StrObj *str, char *pstr )
{
   app_class_construct( (AppClass *) str );

   str->pstr = app_strdup(pstr);
}

/** \brief Destructor for the StrObj object. */

void str_destroy(void *str)
{
   StrObj *this = (StrObj *) str;

   if (str == NULL) {
      return;
   }
   app_free(this->pstr);

   app_class_destroy( str );
}

void str_replace( StrObj *str, char *pstr )
{
   app_free(str->pstr);
   str->pstr = app_strdup(pstr);
}

int str_func_cmp ( AppClass *d1, AppClass *d2 )
{
   return (app_strcmp( ((StrObj *) d1)->pstr,  ((StrObj *) d2)->pstr ) );
}
/*
 * compare StrObj  to char *
 */
int str_func_str_cmp ( AppClass *d1, AppClass *d2 )
{
   return (app_strcmp( ((StrObj *) d1)->pstr, (char *) d2 ) );
}

/*
 * strobj qsort compare functin
 */
int str_qsort_cmp(const void *d1, const void *d2 )
{
   StrObj *nstr = *(StrObj **) d1 ;
   StrObj *ostr = *(StrObj **) d2 ;
   return app_strcmp( nstr->pstr, ostr->pstr );
}
