#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#ifndef UNIX
#include <malloc.h>
#endif
#include <stdlib.h>
#ifndef UNIX
#include <io.h>
#endif
#if defined(__CYGWIN__) || defined(UNIX)
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "gplay.h"

#define DBVER		1
#define SIG		1971
#define INITSIZE	10

char dbname[1024] = "";
char hashname[1024] = "";
int  minfilled=30, maxfilled=70; /* percents */

static FILE *fhash=NULL, *fdb=NULL;

static struct headtype {
	unsigned short sig;
	unsigned short ver;
	unsigned long tabsize;
	unsigned long nlems;
	unsigned long lastchange, lastpurge;
	unsigned long reserv1, reserv2;
} *hashhead = NULL;

static struct hashlem {
	unsigned long crc;
	unsigned long offs; /* -1 - unused */
	unsigned short deleted;
	unsigned short reserved;
} *hash;

static unsigned long crc32(char *str);

#if defined(__CYGWIN__) || defined(UNIX)
long filelength(int h)
{ struct stat st;
  if (fstat(h, &st)) return -1;
  return st.st_size;
}
#endif

int opendb(void)
{ long l;

  if (hashhead)
  { /* already opened */
    return 10;
  }
  strcpy(hashname, dbname);
  strcat(hashname, ".ndx");
  fhash = fopen(hashname, "r+b");
  if (fhash == NULL)
  { if (newdb(0))
      return 1;
    fhash = fopen(hashname, "r+b");
    if (fhash == NULL)
      return 2;
  }
  /* flock ... */
  l = filelength(fileno(fhash));
  hashhead = malloc(l);
  if (hashhead == NULL)
  { fclose(fhash);
    fhash = NULL;
    return 3;
  }
  if (fread(hashhead, l, 1, fhash) != 1)
  { free(hashhead);
    fclose(fhash);
    hashhead = NULL;
    return 4;
  }
  hash = (struct hashlem *)(hashhead + 1);
  if (hashhead->sig != SIG || hashhead->ver != DBVER ||
      l != sizeof(*hashhead) + hashhead->tabsize * sizeof(*hash))
  { free(hashhead);
    fclose(fhash);
    hashhead = NULL;
    return 50; /* corrupted */
  }
  fdb = fopen(dbname, "r+b");
  if (fdb == NULL)
  { free(hashhead);
    fclose(fhash);
    hashhead = NULL;
    return 2;
  }
  /* flock ... */
  return 0;
}

int closedb(void)
{ int r=0;

  if (hashhead == NULL)
    return 10;
  r = savedb();
  fclose(fhash);
  free(hashhead);
  fclose(fdb);
  hashhead = NULL;
  return r;
}

int savedb(void)
{ int r;

  if (!hashhead)
    return 10;
  hashhead->lastchange = time(NULL);
  /* change to write/rename, not overwrite ... */
  /* flock ... */
  fseek(fhash, 0, SEEK_SET);
  r = fwrite(hashhead, sizeof(*hashhead)+hashhead->tabsize*sizeof(*hash), 1, fhash);
  if (r == 1)
  { if (fflush(fhash) == EOF)
      r = -1;
#if defined(__OS2__) || defined(__CYGWIN__) || defined(UNIX)
    ftruncate(fileno(fhash), sizeof(*hashhead)+hashhead->tabsize*sizeof(*hash));
#else
    chsize(fileno(fhash), sizeof(*hashhead)+hashhead->tabsize*sizeof(*hash));
#endif
  }
  /* funlock ... */
  if (r != 1)
    return 1;
  return 0;
}

int newdb(unsigned long tabsize)
{ int i;
  FILE *f, *f1;

  if (hashhead)
    return 10;
  if (tabsize < INITSIZE) tabsize = INITSIZE;
  tabsize = tabsize * 100 / minfilled;
  hashhead = malloc(sizeof(*hashhead)+tabsize*sizeof(*hash));
  if (hashhead == NULL)
    return 3;
  hashhead->tabsize = tabsize;
  hashhead->sig = SIG;
  hashhead->ver = DBVER;
  hashhead->nlems = 0;
  hashhead->lastchange = hashhead->lastpurge = time(NULL);
  hash = (struct hashlem *)(hashhead+1);
  for (i=0; i<hashhead->tabsize; i++)
    hash[i].offs = (unsigned long)-1;
  strcpy(hashname, dbname);
  strcat(hashname, ".ndx");
  f = fopen(hashname, "wb");
  if (f == NULL)
  { free(hashhead);
    hashhead = NULL;
    return 2;
  }
  /* flock ... */
  if (fwrite(hashhead, sizeof(*hashhead)+hashhead->tabsize*sizeof(*hash), 1, f) != 1)
  { fclose(f);
    unlink(hashname);
    free(hashhead);
    hashhead = NULL;
    return 1;
  }
  if (fflush(f) == EOF)
  { fclose(f);
    unlink(hashname);
    free(hashhead);
    hashhead = NULL;
    return 1;
  }
  f1 = fopen(dbname, "wb");
  if (f1 == NULL)
  { fclose(f);
    unlink(hashname);
    free(hashhead);
    hashhead = NULL;
    return 1;
  }
  fclose(f1);
  fclose(f);
  free(hashhead);
  hashhead = NULL;
  return 0;
}

int adddb(unsigned long size, char *fname, char *desc)
{
  char str[1024], *p;
  int  h;
  unsigned long crc;
  int changed=0;

  if (hashhead == NULL)
    return 10;
  p = fetchdb(size, fname, &h);
  if (p)
  { if (strcmp(p, desc) == 0)
      return 0;
    else
    {
      hash[h].deleted = 1;
      /* flock fhash ... */
      fseek(fhash, sizeof(*hashhead)+h*sizeof(*hash), SEEK_SET);
      fwrite(hash+h, sizeof(*hash), 1, fhash);
      fflush(fhash);
      /* funlock fhash */
    }
  }
  hashhead->nlems++;
  if (hashhead->nlems*100/hashhead->tabsize > maxfilled)
  { /* increase tabsize */
    struct headtype *newhead;
    struct hashlem *newhash;
    int i;

    newhead = malloc(sizeof(*hashhead)+hashhead->nlems*100/minfilled*sizeof(*hash));
    if (newhead == NULL)
      return 3;
    memcpy(newhead, hashhead, sizeof(*hashhead));
    newhead->tabsize = hashhead->nlems*100/minfilled;
    newhash = (struct hashlem *)(newhead + 1);
    for (i=0; i<newhead->tabsize; i++)
      newhash[i].offs = (unsigned long)-1;
    for (i=0; i<hashhead->tabsize; i++)
    { if (hash[i].offs == (unsigned long)-1)
        continue;
      for (h = (int)(hash[i].crc % newhead->tabsize); newhash[h].offs != (unsigned long)-1; h++)
        if (h == newhead->tabsize-1)
          h = -1;
      memcpy(newhash+h, hash+i, sizeof(*hash));
    }
    free(hashhead);
    hashhead = newhead;
    hash = newhash;
    changed = 1;
  }
  sprintf(str, "%lu %s", size, fname);
#ifndef UNIX
  strlwr(str);
#endif
  crc = crc32(str);
  for (h = (int)(crc % hashhead->tabsize); hash[h].offs != (unsigned long)-1; h++)
    if (h == hashhead->tabsize-1)
      h = -1;
  /* flock fdb ... */
  /* flock fhash ... */
  fseek(fdb, 0, SEEK_END);
  hash[h].offs = ftell(fdb);
  sprintf(str, "%lu/%s/%s\r\n", size, fname, desc);
  hash[h].crc = crc;
  hash[h].deleted = 0;
  hashhead->lastchange = time(NULL);
  fseek(fhash, 0, SEEK_SET);
  if (changed)
    fwrite(hashhead, sizeof(*hashhead)+hashhead->tabsize*sizeof(*hash), 1, fhash);
  else
  { fwrite(hashhead, sizeof(*hashhead), 1, fhash);
    fseek(fhash, sizeof(*hashhead)+h*sizeof(*hash), SEEK_SET);
    fwrite(hash+h, sizeof(*hash), 1, fhash);
  }
  fflush(fhash);
  fputs(str, fdb);
  fflush(fdb);
  /* funlock fdb */
  /* funlock fhash */
  return 0;
}

int deldb(unsigned long size, char *fname)
{
  int h;

  if (hashhead == NULL)
    return 10;
  if (fetchdb(size, fname, &h) == NULL)
    return 0;
  hash[h].deleted = 1;
  /* flock fhash ... */
  fseek(fhash, sizeof(*hashhead)+h*sizeof(*hash), SEEK_SET);
  fwrite(hash+h, sizeof(*hash), 1, fhash);
  fflush(fhash);
  /* funlock fhash */
  return 0;
}

char *fetchdb(unsigned long size, char *fname, int *index)
{
  char str[1024], *p;
  static char s1[1024];
  unsigned long crc;
  int h;

  if (hashhead == NULL)
    return NULL;
  sprintf(str, "%lu %s", size, fname);
#ifndef UNIX
  strlwr(str);
#endif
  crc = crc32(str);
  sprintf(str, "%lu/%s/", size, fname);
  for (h = (int)(crc % hashhead->tabsize); hash[h].offs != (unsigned long)-1; h++)
  {
    if (hash[h].deleted || hash[h].crc != crc)
    { if (h == hashhead->tabsize-1)
        h = -1;
      continue;
    }
    fseek(fdb, hash[h].offs, SEEK_SET);
    if (fgets(s1, sizeof(s1), fdb) == NULL)
      return NULL;
#ifdef UNIX
    if (strncmp(str, s1, strlen(str)))
#else
    if (strnicmp(str, s1, strlen(str)))
#endif
    { if (h == hashhead->tabsize-1)
        h = -1;
      continue;
    }
    p = strpbrk(s1, "\r\n");
    if (p) *p='\0';
    if (index) *index = h;
    return s1+strlen(str);
  }
  return NULL;
}

int purgedb(void)
{ /* be careful about <4K of local vars! */
  char str[1024], newdbname[1024], *p, *p1;
  int  line, i, h, newnlems, newtabsize;
  FILE *fnewdb;
  unsigned long crc, curoffs;
  struct headtype *newhead;
  struct hashlem *newhash;

  if (hashhead == NULL)
    return 10;
  /* flock ... */
  fseek(fdb, 0, SEEK_SET);
  strcpy(newdbname, dbname);
  strcat(newdbname, ".new");
  fnewdb = fopen(newdbname, "w+b");
  if (fnewdb == NULL)
  { /* funlock ... */
    return 1;
  }
  line = 0;
  curoffs = 0;
  for (curoffs=0; fgets(str, sizeof(str), fdb); curoffs=ftell(fdb))
  {
    line++;
    if (strchr(str, '\n') == NULL)
    { debug(0, "line %d too long", line);
      while (fgets(str, sizeof(str), fdb))
        if (strchr(str, '\n'))
          break;
      continue;
    }
    if ((p=strchr(str, '/')) == NULL)
    { debug(0, "line %d bad, ignoring", line);
      continue;
    }
    *p = ' ';
    if ((p1=strchr(p, '/')) == NULL)
    { debug(0, "line %d bad, ignoring", line);
      continue;
    }
    *p1 = '\0';
#ifndef UNIX
    { char *s = strdup(str);
      strlwr(s);
      crc = crc32(s);
    }
#else
    crc = crc32(str);
#endif
    *p = *p1 = '/';
    for (h=(int)(crc % hashhead->tabsize); hash[h].offs!=(unsigned long)-1; h++)
    { if (hash[h].crc != crc || hash[h].offs != curoffs || 
          hash[h].deleted & 2)
      { if (h == hashhead->tabsize - 1)
          h = -1;
        continue;
      }
      if (hash[h].deleted == 0)
      { hash[h].offs = ftell(fnewdb);
        fputs(str, fnewdb);
      }
      hash[h].deleted |= 2;
      break;
    }
    if (hash[h].offs == (unsigned long)-1)
      debug(0, "line %d not indexed, ignored", line);
  }
  /* check hashtable */
  newnlems = 0;
  for (h=0; h<hashhead->tabsize; h++)
  {
    if (hash[h].offs == (unsigned long)-1)
      continue;
    if ((hash[h].deleted & 2) == 0)
    { debug(0, "Bogus hash value ignored: crc=%08Xh, offs=%lu",
            hash[h].crc, hash[h].offs);
      continue;
    }
    if ((hash[h].deleted & ~2) == 0)
      newnlems++;
  }
  newtabsize = ((newnlems < INITSIZE) ? INITSIZE : newnlems) * 100 / minfilled;
  newhead = malloc(sizeof(*newhead) + newtabsize * sizeof(*hash));
  if (newhead == NULL)
  { debug(0, "Not enough memory for purging db");
    fclose(fnewdb);
    unlink(newdbname);
    fclose(fdb);
    fclose(fhash);
    free(hashhead);
    hashhead = NULL;
    opendb();
    return 1;
  }
  newhash = (struct hashlem *)(newhead + 1);
  for (h=0; h<newtabsize; h++)
    newhash[h].offs = (unsigned long)-1;
  memcpy(newhead, hashhead, sizeof(*hashhead));
  newhead->tabsize = newtabsize;
  newhead->nlems = newnlems;
  for (i=0; i<hashhead->tabsize; i++)
  { if (hash[i].offs == (unsigned long)-1 || hash[i].deleted != 2)
      continue;
    for (h=(int)(hash[i].crc % newhead->tabsize); newhash[h].offs != (unsigned long)-1; h++)
      if (h == newhead->tabsize - 1)
        h = -1;
    memcpy(newhash+h, hash+i, sizeof(*hash));
    newhash[h].deleted = 0;
  }
  free(hashhead);
  hashhead = newhead;
  hash = newhash;
  hashhead->lastpurge = hashhead->lastchange = time(NULL);
  fclose(fnewdb);
  fclose(fdb);
  unlink(dbname);
  rename(newdbname, dbname);
  return savedb();
}

int rebuilddb(unsigned long tabsize)
{
  char str[1024], s1[1024], *p, *p1;
  unsigned long crc, curoffs, size;
  int  line, h;

  if (hashhead)
    return 10;
  /* flock ... */
  if (tabsize < INITSIZE)
    tabsize = INITSIZE;
  tabsize = tabsize * 100 / minfilled;
  hashhead = malloc(sizeof(*hashhead) + tabsize * sizeof(*hash));
  if (hashhead == NULL)
    return 1;
  hashhead->sig = SIG;
  hashhead->ver = DBVER;
  hashhead->tabsize = tabsize;
  hashhead->nlems = 0;
  hashhead->lastchange = hashhead->lastpurge = time(NULL);
  hash = (struct hashlem *)(hashhead + 1);
  for (h=0; h<hashhead->tabsize; h++)
    hash[h].offs = (unsigned long)-1;
  fdb = fopen(dbname, "rb+");
  if (fdb == NULL)
  { free(hashhead);
    hashhead = NULL;
    return 2;
  }
  strcpy(hashname, dbname);
  strcat(hashname, ".ndx");
  fhash = fopen(hashname, "wb");
  if (fhash == NULL)
  { fclose(fdb);
    free(hashhead);
    hashhead = NULL;
    return 2;
  }
  line = 0;
  for (curoffs = 0; fgets(str, sizeof(str), fdb); curoffs = ftell(fdb))
  {
    line++;
    if (strchr(str, '\n') == NULL)
    { debug(0, "line %d too long, ignored", line);
      while (fgets(str, sizeof(str), fdb))
        if (strchr(str, '\n'))
          break;
      continue;
    }
    if ((p=strchr(str, '/')) == NULL)
    { debug(0, "line %d bad, ignoring", line);
      continue;
    }
    *p = ' ';
    if ((p1=strchr(p, '/')) == NULL)
    { debug(0, "line %d bad, ignoring", line);
      continue;
    }
    *p1 = '\0';
#ifndef UNIX
    strlwr(str);
#endif
    crc = crc32(str);
    *p = *p1 = '/';
    size = atol(str);
    for (h=(int)(crc % hashhead->tabsize); hash[h].offs!=(unsigned long)-1; h++)
    { unsigned long saveoffs;

      if (hash[h].crc != crc || hash[h].deleted)
      { if (h == hashhead->tabsize - 1)
          h = -1;
        continue;
      }
      saveoffs = ftell(fdb);
      fseek(fdb, hash[h].offs, SEEK_SET);
      fgets(s1, sizeof(s1), fdb);
      fseek(fdb, saveoffs, SEEK_SET);
      p=strchr(s1, '/');
      if (p) p=strchr(p+1, '/');
      if (p) p[1] = 0;
      if (strnicmp(str, s1, strlen(s1)) == 0)
        hash[h].deleted = 1;
      if (h == hashhead->tabsize - 1)
        h = -1;
    }
    hashhead->nlems++;
    hash[h].crc = crc;
    hash[h].deleted = 0;
    hash[h].offs = curoffs;
    if (hashhead->nlems * 100 / hashhead->tabsize > maxfilled)
    { /* increase tabsize */
      struct headtype *newhead;
      struct hashlem *newhash;
      int i;

      newhead = malloc(sizeof(*hashhead)+hashhead->nlems*100/minfilled*sizeof(*hash));
      if (newhead == NULL)
        return 3;
      memcpy(newhead, hashhead, sizeof(*hashhead));
      newhead->tabsize = hashhead->nlems*100/minfilled;
      newhash = (struct hashlem *)(newhead + 1);
      for (i=0; i<newhead->tabsize; i++)
        newhash[i].offs = (unsigned long)-1;
      for (i=0; i<hashhead->tabsize; i++)
      { if (hash[i].offs == (unsigned long)-1)
          continue;
        for (h = (int)(hash[i].crc % newhead->tabsize); newhash[h].offs != (unsigned long)-1; h++)
          if (h == newhead->tabsize-1)
            h = -1;
        memcpy(newhash+h, hash+i, sizeof(*hash));
      }
      free(hashhead);
      hashhead = newhead;
      hash = newhash;
    }
  }
  if (savedb())
  { closedb();
    return 1;
  }
  return closedb();
}

int checkdb(void)
{
  int nlems, h;
  unsigned long curoffs, crc;
  char str[1024], *p, *p1;

  if (hashhead == NULL)
    return 10;
  if (hashhead->tabsize < INITSIZE * 100 / minfilled)
    return 1;
  if (hashhead->tabsize < hashhead->nlems * 100 / maxfilled)
    return 1;
  if (hashhead->tabsize > ((hashhead->nlems < INITSIZE) ? INITSIZE : hashhead->nlems) * 100 / minfilled)
    return 1;
  if (filelength(fileno(fhash)) != sizeof(*hashhead) + hashhead->tabsize*sizeof(*hash))
    return 1;
  nlems = 0;
  for (h=0; h<hashhead->tabsize; h++)
    if (hash[h].offs != (unsigned long)-1)
      nlems++;
  if (nlems != hashhead->nlems)
    return 1;
  fseek(fdb, 0, SEEK_SET);
  nlems = 0;
  for (curoffs = 0; fgets(str, sizeof(str), fdb); curoffs = ftell(fdb))
  { if (strchr(str, '\n') == NULL)
      return 1;
    if ((p=strchr(str, '/'))==NULL)
      return 1;
    if ((p1=strchr(p+1, '/'))==NULL)
      return 1;
    *p1 = '\0';
    *p = ' ';
#ifndef UNIX
    strlwr(str);
#endif
    crc = crc32(str);
    for (h = (int)(crc % hashhead->tabsize); hash[h].offs != (unsigned long)-1; h++)
    {
      if (hash[h].crc == crc && hash[h].offs == curoffs)
        break;
      if (h == hashhead->tabsize-1)
        h = -1;
    }
    if (hash[h].offs == (unsigned long)-1)
      return 1;
    nlems++;
  }
  if (nlems != hashhead->nlems)
    return 1;
  return 0;
}

static unsigned long crc32tab[] = {  /* CRC polynomial 0xedb88320 */
0x00000000l, 0x77073096l, 0xee0e612cl, 0x990951bal, 0x076dc419l, 0x706af48fl, 0xe963a535l, 0x9e6495a3l,
0x0edb8832l, 0x79dcb8a4l, 0xe0d5e91el, 0x97d2d988l, 0x09b64c2bl, 0x7eb17cbdl, 0xe7b82d07l, 0x90bf1d91l,
0x1db71064l, 0x6ab020f2l, 0xf3b97148l, 0x84be41del, 0x1adad47dl, 0x6ddde4ebl, 0xf4d4b551l, 0x83d385c7l,
0x136c9856l, 0x646ba8c0l, 0xfd62f97al, 0x8a65c9ecl, 0x14015c4fl, 0x63066cd9l, 0xfa0f3d63l, 0x8d080df5l,
0x3b6e20c8l, 0x4c69105el, 0xd56041e4l, 0xa2677172l, 0x3c03e4d1l, 0x4b04d447l, 0xd20d85fdl, 0xa50ab56bl,
0x35b5a8fal, 0x42b2986cl, 0xdbbbc9d6l, 0xacbcf940l, 0x32d86ce3l, 0x45df5c75l, 0xdcd60dcfl, 0xabd13d59l,
0x26d930acl, 0x51de003al, 0xc8d75180l, 0xbfd06116l, 0x21b4f4b5l, 0x56b3c423l, 0xcfba9599l, 0xb8bda50fl,
0x2802b89el, 0x5f058808l, 0xc60cd9b2l, 0xb10be924l, 0x2f6f7c87l, 0x58684c11l, 0xc1611dabl, 0xb6662d3dl,
0x76dc4190l, 0x01db7106l, 0x98d220bcl, 0xefd5102al, 0x71b18589l, 0x06b6b51fl, 0x9fbfe4a5l, 0xe8b8d433l,
0x7807c9a2l, 0x0f00f934l, 0x9609a88el, 0xe10e9818l, 0x7f6a0dbbl, 0x086d3d2dl, 0x91646c97l, 0xe6635c01l,
0x6b6b51f4l, 0x1c6c6162l, 0x856530d8l, 0xf262004el, 0x6c0695edl, 0x1b01a57bl, 0x8208f4c1l, 0xf50fc457l,
0x65b0d9c6l, 0x12b7e950l, 0x8bbeb8eal, 0xfcb9887cl, 0x62dd1ddfl, 0x15da2d49l, 0x8cd37cf3l, 0xfbd44c65l,
0x4db26158l, 0x3ab551cel, 0xa3bc0074l, 0xd4bb30e2l, 0x4adfa541l, 0x3dd895d7l, 0xa4d1c46dl, 0xd3d6f4fbl,
0x4369e96al, 0x346ed9fcl, 0xad678846l, 0xda60b8d0l, 0x44042d73l, 0x33031de5l, 0xaa0a4c5fl, 0xdd0d7cc9l,
0x5005713cl, 0x270241aal, 0xbe0b1010l, 0xc90c2086l, 0x5768b525l, 0x206f85b3l, 0xb966d409l, 0xce61e49fl,
0x5edef90el, 0x29d9c998l, 0xb0d09822l, 0xc7d7a8b4l, 0x59b33d17l, 0x2eb40d81l, 0xb7bd5c3bl, 0xc0ba6cadl,
0xedb88320l, 0x9abfb3b6l, 0x03b6e20cl, 0x74b1d29al, 0xead54739l, 0x9dd277afl, 0x04db2615l, 0x73dc1683l,
0xe3630b12l, 0x94643b84l, 0x0d6d6a3el, 0x7a6a5aa8l, 0xe40ecf0bl, 0x9309ff9dl, 0x0a00ae27l, 0x7d079eb1l,
0xf00f9344l, 0x8708a3d2l, 0x1e01f268l, 0x6906c2fel, 0xf762575dl, 0x806567cbl, 0x196c3671l, 0x6e6b06e7l,
0xfed41b76l, 0x89d32be0l, 0x10da7a5al, 0x67dd4accl, 0xf9b9df6fl, 0x8ebeeff9l, 0x17b7be43l, 0x60b08ed5l,
0xd6d6a3e8l, 0xa1d1937el, 0x38d8c2c4l, 0x4fdff252l, 0xd1bb67f1l, 0xa6bc5767l, 0x3fb506ddl, 0x48b2364bl,
0xd80d2bdal, 0xaf0a1b4cl, 0x36034af6l, 0x41047a60l, 0xdf60efc3l, 0xa867df55l, 0x316e8eefl, 0x4669be79l,
0xcb61b38cl, 0xbc66831al, 0x256fd2a0l, 0x5268e236l, 0xcc0c7795l, 0xbb0b4703l, 0x220216b9l, 0x5505262fl,
0xc5ba3bbel, 0xb2bd0b28l, 0x2bb45a92l, 0x5cb36a04l, 0xc2d7ffa7l, 0xb5d0cf31l, 0x2cd99e8bl, 0x5bdeae1dl,
0x9b64c2b0l, 0xec63f226l, 0x756aa39cl, 0x026d930al, 0x9c0906a9l, 0xeb0e363fl, 0x72076785l, 0x05005713l,
0x95bf4a82l, 0xe2b87a14l, 0x7bb12bael, 0x0cb61b38l, 0x92d28e9bl, 0xe5d5be0dl, 0x7cdcefb7l, 0x0bdbdf21l,
0x86d3d2d4l, 0xf1d4e242l, 0x68ddb3f8l, 0x1fda836el, 0x81be16cdl, 0xf6b9265bl, 0x6fb077e1l, 0x18b74777l,
0x88085ae6l, 0xff0f6a70l, 0x66063bcal, 0x11010b5cl, 0x8f659effl, 0xf862ae69l, 0x616bffd3l, 0x166ccf45l,
0xa00ae278l, 0xd70dd2eel, 0x4e048354l, 0x3903b3c2l, 0xa7672661l, 0xd06016f7l, 0x4969474dl, 0x3e6e77dbl,
0xaed16a4al, 0xd9d65adcl, 0x40df0b66l, 0x37d83bf0l, 0xa9bcae53l, 0xdebb9ec5l, 0x47b2cf7fl, 0x30b5ffe9l,
0xbdbdf21cl, 0xcabac28al, 0x53b39330l, 0x24b4a3a6l, 0xbad03605l, 0xcdd70693l, 0x54de5729l, 0x23d967bfl,
0xb3667a2el, 0xc4614ab8l, 0x5d681b02l, 0x2a6f2b94l, 0xb40bbe37l, 0xc30c8ea1l, 0x5a05df1bl, 0x2d02ef8dl
};

static unsigned long crc32(char *str)
{
	unsigned long crc;

	for (crc=0L;*str;str++) 
		crc = crc32tab[((int)crc^(*str)) & 0xff] ^ ((crc>>8) & 0x00ffffffl);
	return crc;
}
