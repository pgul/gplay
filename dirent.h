/*
 * $Id$
 *
 * $Log$
 * Revision 1.2  2006/09/16 10:23:28  gul
 * *** empty log message ***
 *
 * Revision 2.0  2001/01/10 20:42:19  gul
 * We are under CVS for now
 *
 */
#ifndef __DIRENT_DEF_
#define __DIRENT_DEF_
#include <time.h>
#include <io.h>

struct dirent {
   long  d_size;
   short d_namlen;
   time_t d_ctime, d_mtime, d_atime;
   char  d_name[sizeof(((struct _finddata_t *)_findfirst)->name)];
};

typedef struct {
   long dirid;
   struct dirent dirent;
   int dirfirst;
} DIR;

struct dirent *readdir(DIR *dirp);
void closedir(DIR *dirp);
DIR *opendir(const char *dirname);
#endif
