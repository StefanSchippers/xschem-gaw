/*
 * fsutil.c - filesystem utility functions
 * 
 * include LICENSE
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

#include <appmem.h>
#include <strmem.h>
#include <strcatdup.h>
#include <fsutil.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

char *fsu_getcwd(void)
{
   char buf[BUFSIZ];

   if ( getcwd( buf, sizeof(buf)) ) { 
      return app_strdup( buf );
   }
   msg_fatal("Can't get cwd.");
}

char *fsu_home_dir (void)
{
   char *home = getenv ("HOME");

   if ( ! home) {
      /*
       * If HOME is not defined, try getting it from the password file.
       */
      struct passwd *pwd = getpwuid (getuid ());
      if ( ! pwd || !pwd->pw_dir) {
        return NULL;
      }
      home = pwd->pw_dir;
   }
   return home ;
}

char *fsu_absolute_name( char *dir, char *name)
{
   if ( *name == '/' ) {
      /* absolute name */
      return app_strdup(name);
   } else {
         /* relative file */ 
      return app_strcatdup( dir, "/", name, 0 );
   }
}

