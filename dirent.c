#include <malloc.h>
#include <string.h>
#include <errno.h>
#include "dirent.h"

DIR *opendir(const char *dirname)
{
   char *pathname;
   long dirid;
   struct _finddata_t fd;
   DIR *d;

   pathname = malloc(strlen(dirname)+5);
   if (pathname==NULL)
   { errno=ENOMEM;
     return NULL;
   }
   strcpy(pathname, dirname);
   if (pathname[strlen(pathname)-1]!='\\')
     strcat(pathname, "\\");
   strcat(pathname, "*.*");

   dirid = _findfirst(pathname, &fd);
   if (dirid == -1)
     return NULL;
   free(pathname);
   d = malloc(sizeof(DIR));
   if (d==NULL)
   { errno=ENOMEM;
     return NULL;
   }
   d->dirid = dirid;
   d->dirfirst = 1;
   d->dirent.d_size = fd.size;
   d->dirent.d_ctime = fd.time_create;
   d->dirent.d_mtime = fd.time_write;
   d->dirent.d_atime = fd.time_access;
   strncpy(d->dirent.d_name, fd.name, sizeof(d->dirent.d_name));
   for (d->dirent.d_namlen=0; d->dirent.d_namlen<sizeof(d->dirent.d_name); d->dirent.d_namlen++)
     if (d->dirent.d_name[d->dirent.d_namlen]=='\0') break;
   return d;
}
   
struct dirent *readdir(DIR *d)
{
   struct _finddata_t fd;

   if ( d == NULL )
      return NULL;

   if (d->dirfirst)
   {  d->dirfirst=0;
      return &(d->dirent);
   }

   if (_findnext(d->dirid, &fd))
      return NULL;

   d->dirent.d_size = fd.size;
   d->dirent.d_ctime = fd.time_create;
   d->dirent.d_mtime = fd.time_write;
   d->dirent.d_atime = fd.time_access;
   strncpy(d->dirent.d_name, fd.name, sizeof(d->dirent.d_name));
   for (d->dirent.d_namlen=0; d->dirent.d_namlen<sizeof(d->dirent.d_name); d->dirent.d_namlen++)
     if (d->dirent.d_name[d->dirent.d_namlen]=='\0') break;

   return &(d->dirent);

} /*readdir*/

/*--------------------------------------------------------------------*/
/*    c l o s e d i r                                                 */
/*                                                                    */
/*    Close a directory                                               */
/*--------------------------------------------------------------------*/

void closedir(DIR *dirp)
{
   _findclose(dirp->dirid);
   free(dirp);
} /*closedir*/
