#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gplay.h"

int adddb(unsigned long size, char *fname, char *desc);
int deldb(unsigned long size, char *fname);
int newdb(unsigned long tabsize);
int opendb(void);
int closedb(void);
int savedb(void);
char *fetchdb(unsigned long size, char *fname, int *index);
int purgedb(void);
int rebuilddb(unsigned long tabsize);
int checkdb(void);

int volume;

static void usage(void)
{
  fputs("dbmgr - Gplay database manager, ver " VER "\n", stderr);
  fputs("Copyright (C) Pavel Gulchouck <gul@gul.kiev.ua>   " __DATE__ "\n", stderr);
  fputs("\n", stderr);
  fputs("   Usage:\n", stderr);
  fputs("dbmgr <command> [<params>] [<command> [<params>] ...]\n", stderr);
  fputs("   Commands:\n", stderr);
  fputs("add [-s size] <filename> <descr>  - add file/descr,\n", stderr);
  fputs("     specify size if file does not exists on the disk\n", stderr);
  fputs("del [-s size] <filename>          - del file/descr\n", stderr);
  fputs("check                             - check db for errors\n", stderr);
  fputs("purge                             - purge db\n", stderr);
  fputs("index [-s tabsize]                - rebuild db index\n", stderr);
}

static int add(char *argv[], int argc, int *argi)
{
  unsigned long size = 0;
  char *fname, *descr, *p, *p1;

  if (strcmp(argv[++(*argi)], "-s") == 0)
  { size = atol(argv[++(*argi)]);
    (*argi)++;
  }
  if (*argi+2 > argc)
    return 1;
  fname = argv[(*argi)++];
  descr = argv[(*argi)];
  if (size == 0)
  { struct stat st;
    if (stat(fname, &st))
    { fprintf(stderr, "File %s not found\n", fname);
      return 1;
    }
    size = st.st_size;
  }
  if ((p=strrchr(fname, ':')) == NULL)
    p = fname;
  else
    p++;
  if ((p1=strrchr(p, '/')))
    p = p1+1;
  if ((p1=strrchr(p, '\\')))
    p = p1+1;
  if (opendb())
  { fputs("Can't open db!\n", stderr);
    return 3;
  }
  if (adddb(size, p, descr))
  { closedb();
    fprintf(stderr, "Can't add %s!\n", fname);
    return 3;
  }
  fprintf(stderr, "Add done\n");
  return closedb();
}

static int del(char *argv[], int argc, int *argi)
{
  unsigned long size = 0;
  char *fname, *p, *p1;

  if (strcmp(argv[++(*argi)], "-s") == 0)
  { size = atol(argv[++(*argi)]);
    (*argi)++;
  }
  if (*argi+1 > argc)
    return 1;
  fname = argv[(*argi)];
  if (size == 0)
  { struct stat st;
    if (stat(fname, &st))
    { fprintf(stderr, "File %s not found\n", fname);
      return 1;
    }
    size = st.st_size;
  }
  if ((p=strrchr(fname, ':')) == NULL)
    p = fname;
  else
    p++;
  if ((p1=strrchr(p, '/')))
    p = p1+1;
  if ((p1=strrchr(p, '\\')))
    p = p1+1;
  if (opendb())
  { fputs("Can't open db!\n", stderr);
    return 3;
  }
  if (deldb(size, p))
  { closedb();
    fprintf(stderr, "Can't del %s!\n", fname);
    return 3;
  }
  fprintf(stderr, "Del done\n");
  return closedb();
}

static int check(void)
{
  if (opendb())
  { fputs("Can't open db!\n", stderr);
    return 3;
  }
  if (checkdb())
  { closedb();
    fputs("Database corrupted!\n", stderr);
    return 4;
  }
  fputs("Check ok\n", stderr);
  return closedb();
}

static int purge(void)
{
  if (opendb())
  { fputs("Can't open db!\n", stderr);
    return 3;
  }
  if (purgedb())
  { closedb();
    fputs("Purge failed!\n", stderr);
    return 4;
  }
  fputs("Purge done\n", stderr);
  return closedb();
}

static int rebuild(char *argv[], int argc, int *argi)
{ unsigned long size=0;

  if (*argi+2<argc && strcmp(argv[++(*argi)], "-s") == 0)
    size = atol(argv[++(*argi)]);
  if (rebuilddb(size))
  { fputs("Rebuild index failed!\n", stderr);
    return 4;
  }
  fputs("Rebuild index done\n", stderr);
  return 0;
}

int main(int argc, char *argv[])
{
  int wastask=0, waserr=0, wasusage=0, rc=0, i;
  
  config(NULL);
// fprintf(stderr, "argc = %d\n", argc);
  for (i=1; i<argc; i++)
  {
    if (stricmp(argv[i], "help") == 0 ||
        stricmp(argv[i], "/?") == 0 ||
        stricmp(argv[i], "-?") == 0 ||
        stricmp(argv[i], "/h") == 0 ||
        stricmp(argv[i], "-h") == 0 ||
        stricmp(argv[i], "--help") == 0)
    { usage();
      wastask = 1;
      wasusage = 1;
      continue;
    }
    if (stricmp(argv[i], "add") == 0)
    { if (i+2 < argc)
      { 
// fprintf(stderr, "Call add\n");
        rc |= add(argv, argc, &i);
        wastask = 1;
      }
      else
        waserr = 1;
      continue;
    }
    if (stricmp(argv[i], "del") == 0)
    { if (i+1 < argc)
      { rc |= del(argv, argc, &i);
        wastask = 1;
      }
      else
        waserr = 1;
      continue;
    }
    if (stricmp(argv[i], "check") == 0)
    { rc |= check();
      wastask = 1;
      continue;
    }
    if (stricmp(argv[i], "purge") == 0)
    { rc |= purge();
      wastask = 1;
      continue;
    }
    if (stricmp(argv[i], "index") == 0)
    { rc |= rebuild(argv, argc, &i);
      wastask = 1;
      continue;
    }
    waserr=1;
  }
  if (waserr)
    fputs("Usage error!\n", stderr);
  if ((waserr || !wastask) && !wasusage)
    usage();
  return rc;
}
