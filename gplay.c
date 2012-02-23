#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#ifdef UNIX
#include <pthread.h>
#include <locale.h>
#include <signal.h>
#else
#include <malloc.h>
#include <process.h>
#endif
#include <ctype.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifndef UNIX
#include <io.h>
#endif
#include <fcntl.h>
#include <time.h>
#if defined(__WATCOMC__)
#include "getopt.h"
#include <direct.h>
#define waitpid(pid, stat, opt) cwait(stat, pid, WAIT_CHILD)
#elif defined(__MINGW32__) || defined(__CYGWIN__) || defined(UNIX)
#include <dirent.h>
#else // MSVC
#include "dirent.h"
#endif
#ifdef __OS2__
#define INCL_DOSFILEMGR
#define INCL_DOSQUEUES
#define INCL_DOSPROCESS
#define INCL_DOSNMPIPES
#define INCL_DOSERRORS
#include <os2.h>
#define INVALID_TID_VALUE 0
#elif defined(UNIX)
typedef pthread_t TID;
#define INVALID_TID_VALUE (TID)-1
#include <sys/wait.h>
#ifdef MACOS
#include "getopt.h"
#endif
#else
#include <windows.h>
#define INVALID_TID_VALUE INVALID_HANDLE_VALUE
typedef HANDLE TID;
#if !defined(__CYGWIN__) && !defined(__MINGW32__)
#include <locale.h>
#endif
#if defined(__CYGWIN__) || defined(__MINGW32__)
#include <getopt.h>
#else
#include "getopt.h"
#endif
#if defined(__CYGWIN__)
#include <sys/wait.h>
#else
#include <direct.h>
#define pipe(h)	_pipe(h, 1024, 0)
#define waitpid(pid, stat, opt) cwait(stat, pid, 0)
#endif
#endif
#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#include "keyc.h"
#include "gplay.h"

#ifndef HAVE_ICONV
static char win2alt[] = 
#include "win2alt.h"
#endif

#if defined(MACOS)
//#define MPG123ARGS  "-o", "esd", "-R", "qq"
#define MPG123ARGS  "-R", "qq"
#elif defined(UNIX)
#define MPG123ARGS  "-R", "qq"
#else
#define MPG123ARGS  "-R", "--reopen"
#endif
#define LISTEXT     ".lst"

#if defined(__EMX__) || defined(__CYGWIN__) || defined(UNIX)
#define PATHSEP     '/'
#define PATHSEPS    "/"
#else
#define PATHSEP     '\\'
#define PATHSEPS    "\\"
#endif

#ifndef S_ISDIR
#define	S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)
#endif

#ifndef max
#define max(a, b)	((a) > (b) ? (a) : (b))
#endif
#define max3(a, b, c)	max(max(a, b), c)

int  color_menu=bluef+very+sky,
     color_menu_select=whitef+black,
     color_menu_tagged=bluef+very+yellow,
     color_menu_title=blackf+white,
     color_menu_title_val=blackf+very+white;

extern int phonebiginkey, fnbiginkey, scrlmenu_redraw;
int pole, poley=0, topline;
int playing, paused;
int volume=40;
int forever=0, rnd=0;
int maxx, maxy;
int pid123=-1;
char *head;
#ifdef __CYGWIN__
HANDLE mpgin, mpgout;
#else
FILE *mpgin, *mpgout;
#endif
#ifdef __OS2__
FILE *resppipein, *resppipeout;
char pipename[1024];
#elif defined(UNIX)
FILE *resppipein, *resppipeout;
int  edpipe[2] = {-1, -1};
int  editing;
#endif
int firstsong;
int dbopened=0;
TID resptid;
#if !defined(__OS2__) && !defined(UNIX)
HANDLE fifosem = INVALID_HANDLE_VALUE;
HANDLE fifoevt = INVALID_HANDLE_VALUE;
char fifo[8192];
int fifohead=0, fifolen=0;
#endif
static TID inkeytid, edittid=INVALID_TID_VALUE;

struct lemtype
  { char filename[256];
    char comment[512];
    int  tag;
    long size;
  } *lem = NULL;
int lemsize = 0;

struct listtype
  { char filename[256];
    char comment[512];
    long size;
    int  played;
    struct listtype *next, *prev;
  } *lhead=NULL, *ltail=NULL;
int inlist=0;

struct listtype *cur;

#if defined(__WATCOMC__)

#define PIPESIZE 1024

int pipe(int filedes[2])
{
  if (DosCreatePipe((PHFILE)&filedes[0], (PHFILE)&filedes[1], PIPESIZE))
    return -1;
  _hdopen(filedes[1],O_TEXT|O_RDWR);
  _hdopen(filedes[0],O_TEXT|O_RDONLY);
  return 0;
}
#endif

static int run123(void);

#if !defined(__OS2__) && !defined(UNIX)
int waitthread(HANDLE tid)
{ DWORD rc;
  while (GetExitCodeThread(tid, &rc))
  { if (rc!=STILL_ACTIVE) break;
    Sleep(1000);
  }
  return (int)rc;
}
#endif

static void strlwr(char *str)
{
	while (*str) {
		*str = tolower(*str);
		str++;
	}
}

static char *xlat(const char *from, const char *to, /* const */ char *str)
{
  static char *res=NULL;
  static int  rsize=0;
  int i;

  i = strlen(str)+1;
  if (rsize<i)
  { rsize = (i+64)*4;
    res=realloc(res, rsize);
  }
  if (stricmp(from, to) == 0) {
    strcpy(res, str);
  } else
#ifdef HAVE_ICONV
  {
    iconv_t ic;

    ic = iconv_open(to, from);
    if (ic != (iconv_t)-1) {
      size_t l, l1;
      char *p;

      for (l=strlen(str), l1=rsize-1, p=res; l>0 && l1>0;)
        if (iconv(ic, &str, &l, &p, &l1) == (size_t) -1) {
          debug(1, "iconv error: %s", strerror(errno));
          break;
        }
      *p = '\0';
      iconv_close(ic);
    } else {
      strcpy(res, str);
    }
  }
#else
  if (stricmp(from, "cp1251") == 0 &&
      stricmp(to, "cp866") == 0) {
    char *p;
    for (p=res; *str; p++, str++)
      if (*str & 0x80)
        *p = win2alt[*str & 0x7f];
      else
        *p=*str;
    *p='\0';
  } else {
    strcpy(res, str);
  }
#endif
  return res;
}

static void putfilename(int x, int y, char *name, int skoko)
{
  char *str;

  name = xlat(dir_charset, scr_charset, name);
  if (strlen(name)<=skoko)
  { cnline(x, y, name, skoko);
    return;
  }
  str=malloc(skoko+1);
  if (str==NULL) return;
  strcpy(str, "...");
  strcpy(str+3, name+strlen(name)-skoko+3);
  cnline(x, y, str, skoko);
  free(str);
}

char *putline(int num, chtype *col)
{
  static char s[1024];
  static char size[40], name[256];

  if (lem[num].filename[1] == ':')
    strcpy(size, "  <Disk>  ");
  else if (strchr(lem[num].filename, PATHSEP))
    strcpy(size, "  <Dir>  ");
  else if (lem[num].size >= 1000000)
    sprintf(size, "%3lu %03lu %03lu", lem[num].size/1000000,
            (lem[num].size/1000)%1000, lem[num].size%1000);
  else if (lem[num].size >= 1000)
    sprintf(size, "    %3lu %03lu", lem[num].size/1000, lem[num].size%1000);
  else
    sprintf(size, "        %3lu", lem[num].size);
#if defined(__EMX__) || defined(UNIX)
  snprintf(name, sizeof(name), "%-20s", xlat(dir_charset, scr_charset, lem[num].filename));
#else
  sprintf(name, "%-20s", lem[num].filename);
#endif
  name[20]='\0';
  sprintf(s, "%s %-20s %11s %s", lem[num].tag ? xlat(src_charset, scr_charset, "”") : " ",
          name, size, lem[num].comment);
  s[maxy-2] = '\0';
  if (lem[num].tag) *col = col_menu_tagged;
  return s;
}

static void printvolume(int volume)
{
  char str[80];
  int  i;

  sprintf(str, "%3u", volume);
  cline(12, 59, str);
  for (i=0; i<10; i++)
  { if (i+1>volume/10)
      cchar(13, 52+i, '');
    else
      cchar(13, 52+i, '”');
    if (i+1>(volume+5)/10)
      putcol(col_play_normal, 13, 52+i, 1);
    else if (i<5)
      putcol(col_play_volume1, 13, 52+i, 1);
    else if (i<8)
      putcol(col_play_volume2, 13, 52+i, 1);
    else
      putcol(col_play_volume3, 13, 52+i, 1);
  }
}

void sendcmd(char *format, ...)
{ va_list arg;

#if defined(__CYGWIN__)
  char buf[1024];
  int n;
  DWORD dw;
  va_start(arg, format);
  n = vsnprintf(buf, sizeof(buf)-1, format, arg);
  va_end(arg);
  strcpy(buf+n, "\n");
  if (WriteFile(mpgin, buf, n+1, &dw, NULL) == 0 || dw != n+1)
#else
#if defined(UNIX) && !defined(HAVE_RVOLUME)
  if (tolower(*format)=='v')  {
    /* no volume control in mpg321/unix */
    va_start(arg, format);
    printvolume(va_arg(arg, int));
    va_end(arg);
    return;
  }
#endif
  va_start(arg, format);
  vfprintf(mpgin, format, arg);
  va_end(arg);
  if (fputs("\n", mpgin) == EOF || fflush(mpgin) == EOF)
#endif
    /* message(10, 10, " Can't write to pipe: %s! ", strerror(errno)); */
    debug(1, "Can't write to pipe: %s!", strerror(errno));
#if 0
va_start(arg, format);
fprintf(stdout, format, arg);
fputs("\n", stdout);
fflush(stdout);
va_end(arg);
#endif
  if (debuglevel > 4)
  { char buf[1024];
    va_start(arg, format);
    vsnprintf(buf, sizeof(buf)-1, format, arg);
    va_end(arg);
    debug(5, ">> %s", buf);
  }
}

#if !defined(__OS2__) && !defined(UNIX)
void put_fifo(char *format, ...)
{ va_list arg;
  char tstr[2048], *p;
  int fifotail;

  va_start(arg, format);
  vsprintf(tstr, format, arg);
  va_end(arg);
  if (fifosem == INVALID_HANDLE_VALUE)
    fifosem = CreateMutex(NULL, 0, NULL);
  if (fifoevt == INVALID_HANDLE_VALUE)
    fifoevt = CreateEvent(NULL, FALSE, FALSE, NULL);
  for (;;)
  {
    WaitForSingleObject(fifosem, 10000);
    if (strlen(tstr) + fifolen <= sizeof(fifo))
      break;
    ReleaseMutex(fifosem);
    WaitForSingleObject(fifoevt, 10000);
  }
  fifotail = (fifohead + fifolen) % sizeof(fifo);
  if (fifotail+strlen(tstr) > sizeof(fifo))
  { memcpy(fifo+fifotail, tstr, sizeof(fifo)-fifotail);
    p = tstr + sizeof(fifo)-fifotail;
    fifolen += sizeof(fifo)-fifotail;
    fifotail = 0;
  } else
    p = tstr;
  memcpy(fifo+fifotail, p, strlen(p));
  fifolen += strlen(p);
  ReleaseMutex(fifosem);
  SetEvent(fifoevt);
}
#endif

char *resp(int wait)
{
  char *p;
#ifdef __OS2__
  static char sresp[sizeof(fifo)+1];
  ULONG actual, state;
  AVAILDATA avail;

  if (!wait)
  { DosPeekNPipe(fileno(resppipein), sresp, sizeof(sresp)-1, &actual, &avail, &state);
    if (avail.cbpipe == actual && strchr(sresp, '\n') == 0)
      return NULL;
  }
  if (fgets(sresp, sizeof(sresp), resppipein)==NULL)
    return NULL;
  for (p=sresp+strlen(sresp)-1; isspace(*p) && p>=sresp; *p--='\0');
#elif defined(UNIX)
  static char sresp[8192];
  fd_set sr;
  struct timeval tv;
  int n, h1, h2;

  debug(2, "resp(%d) called", wait);
repselect:
  FD_ZERO(&sr);
  h1 = -1;
  if (editing == 0 && resppipein) {
    h1=fileno(resppipein);
    FD_SET(h1, &sr);
  }
  if (pid123 == -1) {
    run123();
    return "@Q 0\n";
  }
  h2=fileno(mpgout);
  debug(4, "call select(), h1=%d, h2=%d, edpipe=%d", h1, h2, edpipe[0]);
  FD_SET(h2, &sr);
  FD_SET(edpipe[0], &sr);
  tv.tv_sec = tv.tv_usec = 0;
  if (wait) tv.tv_sec = 5; /* 3 sec timeout */
  n = select(max3(h1, h2, edpipe[0]) + 1, &sr, NULL, NULL, &tv);
  if (n == 0) {
    if (wait != 1) return NULL;
    debug(1, "Timeout, interrupt mpg321");
    if (pid123 != -1) kill(pid123, SIGTERM);
    return "@Q 0\n";
  }
  if (n < 0) {
    if (errno==EAGAIN || errno==EINTR) goto repselect;
    debug(1, "select() error: %s!", strerror(errno));
    return NULL;
  }
  debug(2, "resp: found response");
  if (h1!=-1 && FD_ISSET(h1, &sr) && !editing)
  { debug(1, "get from resppipein");
    if (fgets(sresp, sizeof(sresp), resppipein)==NULL)
      return NULL;
  } else if (FD_ISSET(h2, &sr))
  { if (pid123 == -1)
    { debug(1, "mpg123 finished, restart it");
      fclose(mpgin);
      fclose(mpgout);
      mpgout = mpgin = NULL;
      run123();
      return "@Q 0\n";
    }
    if (fgets(sresp, sizeof(sresp), mpgout)==NULL)
      return NULL;
  } else if (FD_ISSET(edpipe[0], &sr))
  { read(edpipe[0], &n, 1);
    goto repselect;
  } else
    goto repselect;
  for (p=sresp+strlen(sresp)-1; isspace(*p) && p>=sresp; *p--='\0');
#else
  static char sresp[sizeof(fifo)+1];
  int i;
debug(2, "resp(%d) called", wait);
  if (fifosem == INVALID_HANDLE_VALUE)
    fifosem = CreateMutex(NULL, 0, NULL);
  if (fifoevt == INVALID_HANDLE_VALUE)
    fifoevt = CreateEvent(NULL, FALSE, FALSE, NULL);
  for (;;)
  {
    WaitForSingleObject(fifosem, 10000);
    for (i=0; i<fifolen; i++)
    { if (fifo[(fifohead+i)%sizeof(fifo)] == '\n')
        break;
    }
    if (i<fifolen)
      break;
    ReleaseMutex(fifosem);
    if (!wait)
      return NULL;
    WaitForSingleObject(fifoevt, 10000);
  }
debug(2, "resp: found response");
  i++;
  if (fifohead+i > sizeof(fifo))
  { memcpy(sresp, fifo+fifohead, sizeof(fifo)-fifohead);
    fifolen -= sizeof(fifo)-fifohead;
    i -= sizeof(fifo)-fifohead;
    p = sresp+sizeof(fifo)-fifohead;
    fifohead = 0;
  } else
    p = sresp;
  memcpy(p, fifo+fifohead, i);
  fifolen -= i;
  fifohead += i;
  if (fifohead == sizeof(fifo)) fifohead = 0;
  p[i] = '\0';
  ReleaseMutex(fifosem);
  SetEvent(fifoevt);
#endif
  debug(2, "<< %s", sresp);
  return sresp;
}

#ifndef UNIX
#ifdef __CYGWIN__
int hgets(char *sr, size_t size, HANDLE h)
{ static char hbuf[1024];
  static DWORD nhbuf;
  int i;
  char *p = sr;

  for (;;) {
    for (i=0; i<nhbuf;) {
      if (hbuf[i] != '\r') *p++ = hbuf[i];
      if (hbuf[i++] == '\n' || p-sr == size-1) {
	*p = '\0';
	memmove(hbuf, hbuf+i, nhbuf-=i);
debug(3, "hgets: return %s", sr);
        return p-sr;
      }
    }
debug(3, "hgets: call ReadFile");
    if (ReadFile(mpgout, hbuf, sizeof(hbuf), &nhbuf, NULL) == 0 || nhbuf == 0) {
      nhbuf = 0;
      *p = '\0';
debug(3, "hgets: return %s", sr);
      return p-sr;
    }
{ char dbuf[sizeof(hbuf)+1];
  memcpy(dbuf, hbuf, nhbuf);
  dbuf[nhbuf] = '\0';
  debug(5, "hgets: received %s", dbuf);
}
  }
}
#else
#define hgets(str, size, file) fgets(str, size, file)
#endif

#if defined(__OS2__)
void resp_thread(void *arg)
#else
DWORD WINAPI resp_thread(void *arg)
#endif
{
  static char sr[1024];

debug(1, "resp_thread started");
  while (hgets(sr, sizeof(sr), mpgout))
  {
debug(2, "resp_thread: got %s", sr);
    if (*sr!='@') continue;
#if defined(__OS2__)
    fputs(sr, resppipeout);
    fflush(resppipeout);
#else
    put_fifo("%s", sr);
debug(2, "resp_thread: put_fifo done");
#endif
  }
debug(1, "resp_thread end");
#ifdef __OS2__
  _endthread();
#else
  ExitThread(0);
  return 0;
#endif
}
#endif

#ifdef UNIX
static pthread_mutex_t sem_refresh;

static void refresh_sem(int set)
{
  if (set)
    pthread_mutex_lock(&sem_refresh);
  else
    pthread_mutex_unlock(&sem_refresh);
}

static int pipeinkey(void)
{
  char s[80];
  int key;

  if (resppipein == NULL)
    return _inkey();
  while (1)
  {
    refresh_sem(1);
    g_refresh();
    refresh_sem(0);
    if (fgets(s, sizeof(s), resppipein) == NULL) break;
    debug(2, "<< %s", s);
    if (sscanf(s, "@K %d", &key) == 1)
      return key;
  }
  return 0;
}
#endif

//void APIENTRY inkey_thread(ULONG arg)
#if defined(__OS2__)
void inkey_thread(void *arg)
#elif defined(UNIX)
void *inkey_thread(void *arg)
#else
DWORD WINAPI inkey_thread(void *arg)
#endif
{
  int key;
  while ((key=_inkey_norefresh())!=1)
  {
#if defined(__OS2__) || defined(UNIX)
    if (fprintf(resppipeout, "@K %d\n", key) < 0)
      debug(1, "fprintf to resppipeout error: %s", strerror(errno));
    if (fflush(resppipeout))
      debug(1, "fflush resppipeout error: %s", strerror(errno));
#else
    put_fifo("@K %d\n", key);
#endif
    debug(2, "inkey_thread: put key %d 0x%04x ('%c')", key, key, (char)key);
  }
#ifdef __OS2__
//  DosExit(EXIT_THREAD, 0);
  _endthread();
#elif defined(UNIX)
   return NULL;
#else
   ExitThread(0);
   return 0;
#endif
}

void cleanlist(void)
{
  while(lhead)
  { struct listtype *n = lhead->next;
    free(lhead);
    lhead = n;
  }
  ltail = NULL;
  inlist = 0;
}

#ifdef __CYGWIN__
void convcwd(char *path)
{
  if (strncmp(path, "/cygdrive/", 10)==0 && isalpha(path[10]) && path[11]=='/')
  { strcpy(path, path+9);
    path[0] = path[1];
    path[1] = ':';
  }
}
#else
#define convcwd(path)
#endif

void addlist(struct lemtype *lem)
{
  if (ltail == NULL)
  { ltail = malloc(sizeof(*ltail));
    if (ltail == NULL)
      exit(3);
    lhead = ltail;
    ltail->prev = NULL;
  }
  else
  { ltail->next = malloc(sizeof(*ltail));
    if (ltail->next == NULL)
      exit(3);
    ltail->next->prev = ltail;
    ltail = ltail->next;
  }
  getcwd(ltail->filename, sizeof(ltail->filename));
  convcwd(ltail->filename);
  if (strchr("/\\", ltail->filename[strlen(ltail->filename)-1]) == NULL)
    strcat(ltail->filename, PATHSEPS);
  strcat(ltail->filename, lem->filename);
  strcpy(ltail->comment, lem->comment);
  ltail->played = 0;
  ltail->size = lem->size;
  ltail->next = NULL;
  inlist++;
}

static int filescmp(char *file1, char *file2)
{
  for (;*file1 && *file2; file1++, file2++)
  {
    if ((*file1=='/' && *file2=='\\') ||
        (*file2=='/' && *file1=='\\'))
      continue;
#ifdef UNIX
    if (*file1 != *file2)
      return (*file1 < *file2) ? -1 : 1;
#else
    if (toupper(*file1) != toupper(*file2))
      return (toupper(*file1) < toupper(*file2)) ? -1 : 1;
#endif
  }
  if (*file1 == 0 && *file2 == 0)
    return 0;
  return (*file1 < *file2) ? -1 : 1;
}

static int filesncmp(char *file1, char *file2, int n)
{
  for (;*file1 && *file2 && n; file1++, file2++, n--)
  {
    if ((*file1=='/' && *file2=='\\') ||
        (*file2=='/' && *file1=='\\'))
      continue;
#ifdef UNIX
    if (*file1 != *file2)
      return (*file1 < *file2) ? -1 : 1;
#else
    if (toupper(*file1) != toupper(*file2))
      return (toupper(*file1) < toupper(*file2)) ? -1 : 1;
#endif
  }
  if (n == 0)
    return 0;
  if (*file1 == 0 && *file2 == 0)
    return 0;
  return (*file1 < *file2) ? -1 : 1;
}

struct listtype *searchlist(struct lemtype *lem)
{
  struct listtype *cur;
  char filename[256];
  
  getcwd(filename, sizeof(filename));
  convcwd(filename);
  if (strchr("/\\", filename[strlen(filename)-1]) == NULL)
    strcat(filename, PATHSEPS);
  strcat(filename, lem->filename);
  for (cur=lhead; cur; cur=cur->next)
  {
    if (filescmp(cur->filename, filename)==0)
      break;
  }
  return cur;
}

struct lemtype *searchlem(struct listtype *cur)
{
  char filename[256];
  int i;
  
  getcwd(filename, sizeof(filename));
  convcwd(filename);
  if (strchr("/\\", filename[strlen(filename)-1]) == NULL)
    strcat(filename, PATHSEPS);
  if (filesncmp(filename, cur->filename, strlen(filename)))
    return NULL;
  if (strchr(cur->filename+strlen(filename), PATHSEP))
    return NULL;
  for (i=0; i<poley; i++)
    if (filescmp(cur->filename+strlen(filename), lem[i].filename)==0)
      return lem+i;
  return NULL;
}

static struct listtype *rmfromlist(struct listtype *cur)
{ 
  struct listtype *new = cur->next;
  if (cur==lhead)
    lhead=cur->next;
  else
    cur->prev->next = cur->next;
  if (cur==ltail)
    ltail = cur->prev;
  else
    cur->next->prev = cur->prev;
  free(cur);
  inlist--;
  return new;
}

void dellist(struct lemtype *lem)
{
  struct listtype *cur;
  
  if ((cur=searchlist(lem)) != NULL)
    rmfromlist(cur);
}

void editsong(int pole)
{
  struct lemtype *cur;
  struct listtype  *l;
  char comment[56], *p;
  int  i;

  cur = lem+pole;
  strncpy(comment, cur->comment, sizeof(comment));
  comment[sizeof(comment)-1] = '\0';
  for (i=strlen(comment); i<sizeof(comment)-1; i++)
    comment[i] = ' ';
  if (mkorr(col_edit_ramka, col_edit_text, 9, 5, 3, 68, 10,
            " comment: ", comment, 0)==0)
  { remenu();
    return;
  }
  remenu();
  strcpy(cur->comment, comment);
  if ((l=searchlist(cur)) != NULL)
    strncpy(l->comment, cur->comment, sizeof(l->comment));
  p = strdup(xlat(dir_charset, db_charset, cur->filename));
  adddb(cur->size, p, xlat(scr_charset, db_charset, comment));
  free(p);
}

#if defined(__OS2__)
void edit_thread(void *arg)
#elif defined(UNIX)
void *edit_thread(void *arg)
#else
DWORD WINAPI edit_thread(void *arg)
#endif
{
  struct listtype *cur = arg;
  struct lemtype  *l;
  char comment[56], *p, *p1;
  int  i;

  crsr_show();
  strncpy(comment, cur->comment, sizeof(comment));
  comment[sizeof(comment)-1] = '\0';
  for (i=strlen(comment); i<sizeof(comment)-1; i++)
    comment[i] = ' ';
#ifdef UNIX
  editing++;
#endif
  if (mkorr(col_edit_ramka, col_edit_text, 9, 5, 3, 68, 10,
            " comment: ", comment, 0)==0)
  { remenu();
    crsr_hide();
  }
  else
  {
    remenu();
    crsr_hide();
    putcol(col_play_hilight, 10, 16, 55);
    cline(10, 1, "description : [                                                       ]       ");
    cline(10, 16, comment);
    stripspc(comment);
    strcpy(cur->comment, comment);
    if ((l=searchlem(cur)) != NULL)
      strncpy(l->comment, cur->comment, sizeof(l->comment));
    if ((p=strrchr(cur->filename, ':')) != NULL)
      p++;
    else
      p=cur->filename;
    if ((p1=strrchr(p, '/')) != NULL)
      p=p1+1;
    if ((p1=strrchr(p, '\\')) != NULL)
      p=p1+1;
    p1 = strdup(xlat(dir_charset, db_charset, p));
    adddb(cur->size, p1, xlat(scr_charset, db_charset, comment));
    free(p1);
  }
#ifdef UNIX
  refresh_sem(1);
  g_refresh();
  refresh_sem(0);
  editing--;
#endif
  debug(4, ">> @E");
#if defined(__OS2__)
  fputs("@E", resppipeout);
  fflush(resppipeout);
  _endthread();
#elif defined(UNIX)
  write(edpipe[1], "", 1);
  if (fputs("@E\n", resppipeout) == EOF)
    debug(1, "Cannot puts to resppipeout: %s", strerror(errno));
  if (fflush(resppipeout) == EOF)
    debug(1, "Cannot fflush resppipeout: %s", strerror(errno));
  return NULL;
#else
  put_fifo("@E");
  ExitThread(0); return 0;
#endif
}

static void markplayed(struct listtype *cur)
{
  struct lemtype *p;
  if (forever)
    return;
  if ((p=searchlem(cur))!=NULL)
    p->tag = 0;
  cur->played = 1;
}

struct listtype *nextsong(struct listtype *cur, int fwd)
{
  firstsong = 0;
  markplayed(cur);
  if (fwd ? cur->next : cur->prev)
    cur = fwd ? cur->next : cur->prev;
  else
    cur = forever ? (fwd ? lhead : ltail) : NULL;
#if 0
  if (cur)
    sendcmd("load %s", cur->filename);
  else
    sendcmd("stop");
#endif
  return cur;
}

static int cmplem(const void *a, const void *b)
{
  const struct lemtype *l1, *l2;
  l1 = (const struct lemtype *)a;
  l2 = (const struct lemtype *)b;
  if (l1->filename[1] == ':')
  { if (l2->filename[1] != ':')
      return 1;
    return (l1->filename[0] - l2->filename[0]);
  }
  if (l2->filename[1] == ':')
    return -1;
  if (strchr(l1->filename, PATHSEP))
  { if (strchr(l2->filename, PATHSEP) == NULL)
      return -1;
    return strcmp(l1->filename, l2->filename);
  }
  if (strchr(l2->filename, PATHSEP))
    return 1;
  return strcmp(l1->filename, l2->filename);
}

int loaddir(void)
{ char *p;
  DIR *d;
  struct dirent *df;
  struct stat st;
  char filename[1024];
  char curdir[256];
#if !defined(UNIX)
  int  c;
#if !defined(__OS2__)
  DWORD drives;
#endif
#endif

  poley=0;
  getcwd(curdir, sizeof(curdir));
  debug(4, "loaddir(), curdir=%s", curdir);
#ifndef UNIX
#ifdef __OS2__
  for (c=0; c<='Z'-'A'; c++)
  { if (DosSetDefaultDisk(c+1) == 0)
#else
  drives = GetLogicalDrives();
  for (c=0; c<='Z'-'A'; c++)
  { if (drives & (1<<c))
#endif
    {
      if (lemsize < poley+1)
      lem=realloc(lem, sizeof(*lem)*(lemsize += 10));
      if (lem==NULL)
        exit(3);
      sprintf(lem[poley].filename, "%c:", c+'A');
      lem[poley].comment[0]='\0';
      lem[poley].size=0;
      lem[poley].tag=0;
      poley++;
    }
  }
#ifdef __OS2__
  DosSetDefaultDisk(toupper(curdir[0])-'A'+1);
#endif
#endif
  d = opendir(curdir);
  if (d==NULL)
  { debug(1, "Can't open dir");
    return 1;
  }
debug(3, "dir %s opened successfully", curdir);
  while ((df=readdir(d))!=NULL)
  {
    if (strcmp(df->d_name, ".") == 0) continue;
    strcpy(filename, curdir);
    if (strchr("\\/", curdir[strlen(curdir)-1]) == NULL)
      strcat(filename, PATHSEPS);
    strcat(filename, df->d_name);
debug(4, "loaddir(): file %s", filename);
    if (stat(filename, &st)) continue;
    if (!S_ISDIR(st.st_mode))
    {
      if (strlen(df->d_name)<5) continue;
      p=strrchr(df->d_name, '.');
      if (p==NULL) continue;
      p++;
      if (stricmp(p, "wav") && stricmp(p, "mp3")) continue;
    }
    if (lemsize < poley+1)
    lem=realloc(lem, sizeof(*lem)*(lemsize += 10));
    if (lem==NULL)
      exit(3);
    lem[poley].size=0;
    lem[poley].tag=0;
    lem[poley].comment[0]='\0';
    strcpy(lem[poley].filename, df->d_name);
    if (S_ISDIR(st.st_mode))
      strcat(lem[poley].filename, PATHSEPS);
    else
    { lem[poley].size=st.st_size;
      if (searchlist(lem+poley))
        lem[poley].tag=1;
      if ((p=fetchdb(st.st_size, xlat(dir_charset, db_charset, df->d_name), NULL)) != NULL)
      { strncpy(lem[poley].comment, xlat(db_charset, scr_charset, p), sizeof(lem->comment)-1);
        lem[poley].comment[sizeof(lem->comment)-1] = '\0';
      }
debug(2, "loaddir: found %s -- %s", lem[poley].filename, lem[poley].comment);
    }
    poley++;
  }
  closedir(d);
  qsort(lem, poley, sizeof(*lem), cmplem);
  return 0;
}

struct listtype *toss(struct listtype *head)
{
  int n, i;
  struct listtype *newhead=NULL, *p;
  
  srand((unsigned int)time(NULL));
  ltail=NULL;
  for (n=inlist; n; n--)
  {
    i=rand()%n;
    for (p=head; i--; p=p->next);
    if (p->prev) p->prev->next=p->next;
    else head=p->next;
    if (p->next) p->next->prev=p->prev;
    if (ltail) ltail->next=p;
    p->next=NULL;
    p->prev=ltail;
    ltail=p;
    if (newhead==NULL) newhead=p;
  }
  return newhead;
}

void play(void)
{
  enum {PLAYING, PAUSED, STOPPED} mode;
  int prevtape, tape, prevelaps, elapsed, totaltime, curframe=0, totalframes;
  char *p;
  char str[256];
  char *ftype, *fmode;
  int bitrate, stereo, sf, layer;
  int key;
/*
‚[Œ”]                                                                          ƒ
’’’’’’‘‘‘‘‘‘ Z! v.2.4  mp3 player for OS/2! ‘‘‘‘‘‘’’’’’’
„   ² code by dink  ::  http://dink.org  ::  this interface by Super Keyby ¯   …

 playing file: [F:\pub\mp3\ksp\BelGvard\BgZnoy01.mp3]
 file info   :  mpeg 1.0 layer III, 56kbit/s, 44100 Hz single-channel

 track name  : []
 artist      : []  []                 year
 album       : []  []  genre
 comment     : []

 total playing time [4:29]                        volume: 15%
 time elapsed       [:19]                        [”]
                    [< ” >]                        [<- -””- ->]

 [””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””]

 [P]
  „€-previous song: [F:\pub\mp3\ksp\BelGvard\Amulet14.mp3]
 [N]
  „€-next song    : [F:\pub\mp3\ksp\BelGvard\BgZnoy02.mp3]
 [Q]                                      [<space>]
  „€-quit back to file listing                „€€€€€-pause playback

*/

  debug(1, "play called");
  firstsong = 1;
  crsr_hide();
  evakuate(0,0,maxx,maxy);
#ifdef UNIX
  barputcol(col_play_normal, 0, 0, maxx, maxy);
  barfill(' ' , 0, 0, maxx, maxy);
#else
  barcell((col_play_normal<<8) | ' ', 0, 0, maxx, maxy);
#endif
#ifdef __OS2__
  DosSetPriority(PRTYS_PROCESS, PRTYC_TIMECRITICAL, 0, 0);
#else
//  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif
#ifdef UNIX
  editing++;
  while (resp(0));
  editing--;
#else
  while (resp(0));
#endif
  debug(2, "resp queue purged");
//  DosCreateThread(&inkeytid, inkey_thread, 0, 0, 65536*4);
#if defined(__OS2__)
  inkeytid = _beginthread(inkey_thread, 0, 65536*4, NULL);
#elif !defined(UNIX)
  { DWORD id;
    inkeytid = CreateThread(NULL, 65536*4, inkey_thread, NULL, 0, &id);
  }
#endif
  putcol(col_play_ramka1, 0, 0, 80);
  cline(0, 0, xlat(src_charset, scr_charset, "¢                                                                              ¦"));
  putcol(col_play_ramka1, 1, 0, 1);
  putcol(col_play_ramka1, 1, 79, 1);
  putcol(col_play_firstlet, 1, 21, 1);
  putcol(col_play_hilight2, 1, 22, 4);
  putcol(col_play_text, 1, 27, 3);
  putcol(col_play_hilight, 1, 31, 3);
  putcol(col_play_text, 1, 35, 20);
#ifdef __OS2__
  putcol(col_play_firstlet, 1, 35, 1);
  putcol(col_play_firstlet, 1, 42, 1);
  putcol(col_play_firstlet, 1, 55, 4);
  putcol(col_play_text2, 1, 57, 1);
  cline(1, 0, "’’’’’‘‘‘‘‘ GPlay ver " VER " mpg123 frontend for OS/2 ‘‘‘‘‘’’’’’");
#elif defined(UNIX)
  putcol(col_play_firstlet, 1, 35, 1);
  putcol(col_play_firstlet, 1, 42, 1);
  putcol(col_play_firstlet, 1, 55, 4);
  //putcol(col_play_text2, 1, 57, 1);
  cline(1, 0, xlat(src_charset, scr_charset, "’’’’’‘‘‘‘‘ GPlay ver " VER " mpg123 frontend for UNIX ‘‘‘‘‘’’’’’"));
#else
  putcol(col_play_firstlet, 1, 36, 1);
  putcol(col_play_firstlet, 1, 43, 1);
  putcol(col_play_firstlet, 1, 56, 3);
  cline(1, 0, "’’’’’‘‘‘‘‘ GPlay ver " VER "  mpg123 frontend for WIN ‘‘‘‘‘’’’’’");
#endif
  putcol(col_play_ramka1, 2, 0, 1);
  putcol(col_play_ramka1, 2, 79, 1);
  putcol(col_play_ramka2, 2, 1, 4);
  putcol(col_play_ramka2, 2, 75, 4);
  cline(2, 0, xlat(src_charset, scr_charset, "©  ² coded by Pavel Gulchouck : http://gul.kiev.ua : this interface from Z! ¯  ¬"));
  if (rnd)
    lhead = toss(lhead);
  for (cur=lhead; ; )
  {
begloop:
    if (cur==NULL) break;
    sendcmd("load %s", cur->filename);
    putcol(col_play_text,     4,  2,11);
    putcol(col_play_firstlet, 4,  1, 1);
    putcol(col_play_firstlet, 4,  9, 1);
    putcol(col_play_hilight2, 4, 13, 1);
    putcol(col_play_brackets, 4, 15, 1);
    putcol(col_play_hilight,  4, 16,60);
    putcol(col_play_brackets, 4, 76, 1);
    cline(4, 1,
"playing file: [                                                            ]");
    putfilename(4, 16, cur->filename, 60);
    refresh_sem(1);
    g_refresh();
    refresh_sem(0);
    do
    { p=resp(1);
debug(2, "play: waiting for song info, got %s", p ? p : "NULL");
    } while (p && p[1]!='I' && strnicmp(p, "@P 0", 4) && strnicmp(p, "@Q 0",4));
    if (p && p[1] == 'P') {
      cur = nextsong(cur, 1);
      continue;
    }
    if (p==NULL || p[1]=='Q') continue;
/*
@I ID3:ğÏ×ÏÒÏÔ                       íÁÛÉÎÁ ÷ÒÅÍÅÎÉ
                                                   Goa
@I ID3:ğÏÓÌÅÄÎÑÑ ĞÏÜÍÁ               ëÉÎÏÛÌÑÇÅÒÙ                   åÇÉĞÔÑÎÉÎ                                                   http://rmp.ru
@I ID3:BELLE (NOTRE DAME DE PARIS)   ğåôëõî,çïìõâå÷,íáëáòóëéê      BELLE (NOTRE DAME DE PARIS)   20020A125514                      Vocal
Junk at the beginning 52494646
Skipped RIFF header!
@S 1.0 3 44100 Joint-Stereo 1 313 2 0 0 0 96 0

@I BgZnoy01
@S 1.0 3 44100 Single-Channel 0 182 1 0 0 0 56 0

@I ID3:Misirlou                      Dick Dale & His Del-Tones     Pulp Fiction Soundtrack       1994--== ZoLtaR's MP3 Archive ==--Soundtrack
track name  : [Misirlou]
artist      : [Dick Dale & His Del-Tones]  [1994]                 year
album       : [Pulp Fiction Soundtrack]  [Soundtrack]  genre
comment     : [--== ZoLtaR's MP3 Archive ==--]


*/
    putcol(col_play_firstlet, 7, 1, 1);
    putcol(col_play_text, 7, 2, 9);
    putcol(col_play_firstlet, 7, 7, 1);
    putcol(col_play_hilight2, 7, 13, 1);
    putcol(col_play_brackets, 7, 15, 1);
    putcol(col_play_brackets, 7, 48, 1);
    putcol(col_play_hilight, 7, 16, 30);
    cline(7, 1, "track name  : [                                ]");
    putcol(col_play_firstlet, 8, 1, 1);
    putcol(col_play_text, 8, 2, 5);
    putcol(col_play_hilight2, 8, 13, 1);
    putcol(col_play_brackets, 8, 15, 1);
    putcol(col_play_brackets, 8, 48, 1);
    putcol(col_play_brackets, 8, 51, 1);
    putcol(col_play_brackets, 8, 56, 1);
    putcol(col_play_firstlet, 8, 74, 1);
    putcol(col_play_text, 8, 75, 3);
    putcol(col_play_hilight, 8, 16, 30);
    putcol(col_play_hilight, 8, 52, 4);
    cline(8, 1, "artist      : [                                ]  [    ]                 year");
    putcol(col_play_firstlet, 9, 1, 1);
    putcol(col_play_text, 9, 2, 4);
    putcol(col_play_hilight2, 9, 13, 1);
    putcol(col_play_brackets, 9, 15, 1);
    putcol(col_play_brackets, 9, 48, 1);
    putcol(col_play_brackets, 9, 51, 1);
    putcol(col_play_brackets, 9, 71, 1);
    putcol(col_play_firstlet, 9, 74, 1);
    putcol(col_play_text, 9, 75, 4);
    putcol(col_play_hilight, 9, 16, 30);
    putcol(col_play_hilight, 9, 52, 19);
    cline(9, 1, "album       : [                                ]  [                   ]  genre");

    putcol(col_play_firstlet, 10, 1, 1);
    putcol(col_play_text, 10, 2, 10);
    putcol(col_play_hilight2, 10, 13, 1);
    putcol(col_play_brackets, 10, 15, 1);
    putcol(col_play_brackets, 10, 71, 1);
    if (p && strncmp(p+3, "ID3:", 4) == 0)
    {
      cnline(7, 16, xlat(mp3_charset, scr_charset, p+7), 30);
      if (strlen(p)>37)
        cnline(8, 16, xlat(mp3_charset, scr_charset, p+37), 30);
      if (strlen(p)>67)
        cnline(9, 16, xlat(mp3_charset, scr_charset, p+67), 30);
      if (strlen(p)>97)
        cnline(8, 52, xlat(mp3_charset, scr_charset, p+97), 4);
      putcol(col_play_brackets, 10, 48, 1);
      putcol(col_play_brackets, 10, 51, 1);
      putcol(col_play_firstlet, 10, 74, 1);
      putcol(col_play_text, 10, 75, 4);
      if (cur->comment[0])
      {
        putcol(col_play_hilight, 10, 16, 55);
        cline(10, 1, "description : [                                                       ]       ");
        cnline(10, 16, cur->comment, 55);
      }
      else
      {
        putcol(col_play_hilight, 10, 16, 30);
        putcol(col_play_hilight, 10, 52, 19);
        cline(10, 1, "comment     : [                                ]                       ");
	if (strlen(p)>101)
          cnline(10, 16, xlat(mp3_charset, scr_charset, p+101), 30);
#if 0
        if (cur->comment[0])
        { char *p1 = p+130;
          if (p1>=p+strlen(p))
            p1 = p+strlen(p)-1;
          while (isspace(*p1)) p1--;
          if (p1<p+101) p1=p+101;
          else p1+=3;
          cnline(10, 16+(p1-p-101), cur->comment, 55-(p1-p-101));
        }
#endif
      }
      if (strlen(p)>131)
        cnline(9, 52, xlat(mp3_charset, scr_charset, p+131), 19);
    }
    else
    {
      putcol(col_play_hilight, 10, 16, 55);
      cline(10, 1, "description : [                                                       ]       ");
      if (cur->comment[0])
        cnline(10, 16, cur->comment, 55);
      else
        fill(' ', 10, 16, 55);
    }
    do
    { p=resp(1);
debug(2, "play: waiting for play start, got %s", p ? p : "NULL");
    } while (p==NULL || (p && p[1]!='S' && (p[1]!='P' || strncmp(p+1, "P 2", 3)) && p[1]!='Q'));
    if (p && strnicmp(p, "@P 0", 4) == 0)
    { debug(2, "Next song");
      cur = nextsong(cur, 1);
      continue;
    }
    if (p && strnicmp(p, "@Q 0", 4) == 0)
    { debug(2, "Restart song");
      continue;
    }
    putcol(col_play_firstlet, 5, 1, 1);
    putcol(col_play_text, 5, 2, 8);
    putcol(col_play_firstlet, 5, 6, 1);
    putcol(col_play_hilight2, 5, 13, 1);
    putcol(col_play_hilight, 5, 16, 64);
    if (p[1] == 'S') {
      cline(5, 1, "file info   :");
      p=strtok(p, " \t");
      ftype = strtok(NULL, " \t");
      if (ftype == NULL) ftype = "?";
      p = strtok(NULL, " \t"); /* layer */
      layer = p ? atoi(p) : 1;
      p = strtok(NULL, " \t"); /* sf */
      sf = p ? atoi(p) : 0;
      fmode = strtok(NULL, " \t");
      if (fmode == NULL) fmode = "?";
      p = strtok(NULL, " \t"); /* mode ext */
      p = strtok(NULL, " \t"); /* frame size */
      stereo = p ? atoi(strtok(NULL, " \t")) : 0;
      p = strtok(NULL, " \t"); /* copyright */
      p = strtok(NULL, " \t"); /* error prot */
      p = strtok(NULL, " \t"); /* emphasis */
      p = strtok(NULL, " \t"); /* bitrate */
      bitrate = p ? atoi(p) : 0;
      p = strtok(NULL, " \t"); /* extension */
      strlwr(fmode);
      sprintf(str, "mpeg %s layer %s, %ukbit/s, %u Hz %s",
              ftype, (layer == 1 ? "I" : (layer == 2 ? "II" : "III")),
              bitrate, sf, fmode);
      cline(5, 16, str);
    }

    putcol(col_play_firstlet, 12,  1, 1);
    putcol(col_play_text,     12,  2,17);
    putcol(col_play_firstlet, 12,  7, 1);
    putcol(col_play_firstlet, 12, 15, 1);
    putcol(col_play_brackets, 12, 20, 1);
    putcol(col_play_hilight,  12, 21, 5);
    putcol(col_play_hilight2, 12, 23, 1);
    putcol(col_play_brackets, 12, 26, 1);
    putcol(col_play_firstlet, 12, 51, 1);
    putcol(col_play_text,     12, 52, 5);
    putcol(col_play_hilight2, 12, 57, 1);
    putcol(col_play_hilight,  12, 59, 3);
    putcol(col_play_hilight2, 12, 62, 1);
    cline(12, 1, "total playing time [     ]                        volume:    %");
    putcol(col_play_firstlet, 13, 1, 1);
    putcol(col_play_text,     13, 2, 11);
    putcol(col_play_firstlet, 13, 6, 1);
    putcol(col_play_brackets, 13, 20, 1);
    putcol(col_play_hilight,  13, 21, 5);
    putcol(col_play_hilight2, 13, 23, 1);
    putcol(col_play_brackets, 13, 26, 1);
    putcol(col_play_brackets, 13, 51, 1);
    putcol(col_play_brackets, 13, 62, 1);
    cline(13, 1, "time elapsed       [  :  ]                        [          ]");
    printvolume(volume);
    putcol(col_play_brackets, 14, 20, 1);
    putcol(col_play_clicable, 14, 21, 1);
    putcol(col_play_clicable, 14, 25, 1);
    putcol(col_play_brackets, 14, 26, 1);
    putcol(col_play_brackets, 14, 51, 1);
    putcol(col_play_clicable, 14, 52, 2);
    putcol(col_play_clicable, 14, 60, 2);
    putcol(col_play_brackets, 14, 62, 1);
    cline(14, 1, xlat(src_charset, scr_charset, "                   [< ” >]                        [<- -””- ->]"));
    putcol(col_play_brackets, 16, 1, 1);
    putcol(col_play_tape,     16, 2,76);
    putcol(col_play_brackets, 16,78, 1);
    cline(16, 1, xlat(src_charset, scr_charset, "[””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””]"));
    putcol(col_play_brackets, 18, 1, 3);
    putcol(col_play_clicable, 18, 2, 1);
    cline(18, 1, "[P]");
    putcol(col_play_text2,    19,  2, 3);
    putcol(col_play_firstlet, 19,  5, 1);
    putcol(col_play_text,     19,  6,12);
    putcol(col_play_firstlet, 19, 14, 1);
    putcol(col_play_hilight2, 19, 18, 1);
    putcol(col_play_brackets, 19, 20, 1);
    putcol(col_play_hilight , 19, 21,56);
    putcol(col_play_brackets, 19, 77, 1);
    cline(19, 1, " „€€previous song: [                                                        ]");
    if (cur->prev)
      putfilename(19, 21, cur->prev->filename, 56);
    else if (forever && !firstsong)
      putfilename(19, 21, ltail->filename, 56);
    putcol(col_play_brackets, 20, 1, 3);
    putcol(col_play_clicable, 20, 2, 1);
    cline(20, 1, "[N]");
    putcol(col_play_text2,    21, 2, 3);
    putcol(col_play_firstlet, 21, 5, 1);
    putcol(col_play_text,     21, 6, 12);
    putcol(col_play_firstlet, 21, 10, 1);
    putcol(col_play_hilight2, 21, 18, 1);
    putcol(col_play_brackets, 21, 20, 1);
    putcol(col_play_hilight , 21, 21, 56);
    putcol(col_play_brackets, 21, 77, 1);
    cline(21, 1, xlat(src_charset, scr_charset, " „€€next song    : [                                                        ]"));
    if (cur->next)
      putfilename(21, 21, cur->next->filename, 56);
    else if (forever)
      putfilename(21, 21, lhead->filename, 56);
    putcol(col_play_brackets, 22, 1, 3);
    putcol(col_play_clicable, 22, 2, 1);
    putcol(col_play_brackets, 22, 42, 1);
    putcol(col_play_clicable, 22, 43, 7);
    putcol(col_play_brackets, 22, 50, 1);
    cline(22, 1, "[Q]                                      [<space>]");
    putcol(col_play_text2,    23, 2, 3);
    putcol(col_play_firstlet, 23, 5, 1);
    putcol(col_play_text,     23, 6, 24);
    putcol(col_play_firstlet, 23, 10, 1);
    putcol(col_play_firstlet, 23, 15, 1);
    putcol(col_play_firstlet, 23, 18, 1);
    putcol(col_play_firstlet, 23, 23, 1);
    putcol(col_play_text2,    23, 46, 7);
    putcol(col_play_firstlet, 23, 53, 1);
    putcol(col_play_text,     23, 54, 13);
    putcol(col_play_firstlet, 23, 59, 1);
    cline(23, 1, xlat(src_charset, scr_charset, " „€€quit back to file listing                „€€€€€€pause playback"));
    mouse_ch(14, 21, 1, 1, '<');
    mouse_ch(14, 25, 1, 1, '>');
    mouse_ch(14, 52, 1, 2, leftc);
    mouse_ch(14, 60, 1, 2, rightc);
    mouse_ch(18,  2, 1, 1, 'P');
    mouse_ch(20,  2, 1, 1, 'N');
    mouse_ch(22,  2, 1, 1, 'Q');
    mouse_ch(22, 43, 1, 7, spacec);
    totaltime=totalframes=-1;
    prevelaps=-1;
    prevtape=-1;
    mode=PLAYING;
    while (cur)
    { /* playing */
      refresh_sem(1);
      g_refresh();
      refresh_sem(0);
      if ((p=resp(mode==PLAYING ? 1 : -1))==NULL)
      {
#ifdef __OS2__
        sleep(1);
#elif defined(UNIX)
        usleep(100000);
#else
        Sleep(100);
#endif
debug(2, "Playing, nothing got");
        continue;
      }
debug(2, "Playing, got %s", p);
      if (p[1] != 'K')
      { 
        if (p[1]=='E' && edittid != INVALID_TID_VALUE)
        {
#ifdef __OS2__
          DosWaitThread(&edittid, DCWW_WAIT);
          edittid=INVALID_TID_VALUE;
          inkeytid = _beginthread(inkey_thread, 0, 65536*4, NULL);
#elif defined(UNIX)
	  pthread_join(edittid, NULL);
          edittid=INVALID_TID_VALUE;
          // pthread_create(&inkeytid, NULL, inkey_thread, NULL);
#else
          DWORD id;
          // waitthread(edittid);
          edittid=INVALID_TID_VALUE;
          inkeytid = CreateThread(NULL, 65536*4, inkey_thread, NULL, 0, &id);
#endif
          continue;
        }
        /* event from mpg123 */
        if (p[1]=='F' /* && !edittid */)
        {
          p=strtok(p, " \t"); /* command */
          curframe = atoi(strtok(NULL, " \t"));
#if 0
{ char s[64];
  sprintf(s, "Curframe %u     ", curframe);
  cline(3, 0, s);
}
#endif
          p=strtok(NULL, " \t");
          if (totalframes == -1)
            totalframes = curframe + atoi(p);
          elapsed=atoi(strtok(NULL, " \t"));
          if (totaltime==-1)
          { totaltime = elapsed+atoi(strtok(NULL, " \t\r\n"));
            sprintf(str, "%2u:%02u", totaltime/60, totaltime%60);
            cline(12, 21, str);
          }
          if (elapsed!=prevelaps)
          { sprintf(str, "%2u:%02u", elapsed/60, elapsed%60);
            cline(13, 21, str);
            prevelaps = elapsed;
          }
          if (totaltime==0) totaltime=1;
          if (elapsed >= totaltime) elapsed = totaltime-1;
          if (elapsed < 0) elapsed = 0;
          tape = elapsed*76/totaltime;
          if (tape!=prevtape)
          { if (prevtape!=-1) {
              // cchar(16, 2+prevtape, '”');
              putcol(col_play_tape, 16, 2+prevtape, 1);
            }
            // cchar(16, 2+tape, acs.bar0);
            putcol(col_play_tapecur, 16, 2+tape, 1);
            prevtape=tape;
          }
        }
        else if (p[1]=='P' || p[1]=='Q')
        {
          if (p[3] == '0' || p[3] == '3')
            break;
          if (p[3]=='1')
          { putcol(col_play_firstlet, 23, 52, 1);
            putcol(col_play_text, 23, 53, 1);
            cline(23, 52, "resume");
          }
          else
          { putcol(col_play_text2, 23, 52, 1);
            putcol(col_play_firstlet, 23, 53, 1);
            cline(23, 52, "€pause");
          }
        }
        else if (p[1]=='V')
        { int i;
          p=strtok(p, " \t");
          i=atoi(strtok(NULL, " \t\r\n"));
          //if (!edittid)
            printvolume(i);
        }
        continue;
      }
      { if (edittid != INVALID_TID_VALUE)
          continue;
        p=strtok(p, " \t");
        key=atoi(strtok(NULL, " \t\r\n"));
        switch (key)
        {
          case leftc:
            if (volume>0)
              sendcmd("volume %d", volume-=5);
            break;
          case rightc:
            if (volume<100)
              sendcmd("volume %d", volume+=5);
            break;
          case escc:
            sendcmd("stop");
            markplayed(cur);
            cur = NULL;
            break;
          case downc:
            if (cur->next == NULL && !forever)
              continue;
            cur = nextsong(cur, 1);
            goto begloop;
          case upc:
            if (cur->prev == NULL && (!forever || firstsong))
              continue;
            cur = nextsong(cur, 0);
            goto begloop;
          case f4c:
            /* change comment */
            if (!dbopened)
              break;
#ifndef UNIX
            back_key_first(1);
#endif
#ifdef __OS2__
            DosWaitThread(&inkeytid, DCWW_WAIT);
            edittid = _beginthread(edit_thread, 0, 65536*4, cur);
#elif defined(UNIX)
            pthread_create(&edittid, NULL, edit_thread, cur);
#else
            // waitthread(inkeytid);
            { DWORD id;
              edittid = CreateThread(NULL, 65536*4, edit_thread, cur, 0, &id);
            }
#endif
            break;
          case f8c:
            /* del file */
delfile:
            { struct listtype *l = cur;
              int pole = 0;
              char fname[40];

              if (mode == PLAYING)
              { sendcmd("pause");
                do
                  if ((p=resp(1)) == NULL)
#ifdef __OS2__
                    sleep(1);
#elif defined(UNIX)
                    usleep(100000);
#else
                    Sleep(100);
#endif
                while (p==NULL || strnicmp(p, "@p 1", 4));
	      }

              if (strlen(l->filename)<40)
                strcpy(fname, l->filename);
              else
              { strcpy(fname, "...");
                strcpy(fname+3, l->filename+strlen(l->filename)-40+3+1);
              }
#ifndef UNIX
              back_key_first(1);
#endif
#ifdef __OS2__
              DosWaitThread(&inkeytid, DCWW_WAIT);
#elif !defined(UNIX)
              //waitthread(inkeytid);
#endif
              quere(redf+very+yellow, whitef+black, 10, 20, 2, 2, &pole,
                  " Delete file ", fname,
                  " Yes ", "Yy", " No ", "Nn\x1b");
#ifdef __OS2__
              inkeytid = _beginthread(inkey_thread, 0, 65536*4, NULL);
#elif defined(UNIX)
              //pthread_create(&inkeytid, NULL, inkey_thread, NULL);
              refresh_sem(1);
              g_refresh();
              refresh_sem(0);
#else
              { DWORD id;
                inkeytid = CreateThread(NULL, 65536*4, inkey_thread, NULL, 0, &id);
              }
#endif
              if (pole == 1)
              { if (mode == PLAYING)
                  sendcmd("pause");
                continue;
              }
              if (cur->prev)
                cur->prev->next = cur->next;
              else
               lhead = cur->next;
              if (cur->next)
              { cur = cur->next;
                cur->prev = l->prev;
              }
              else
              { cur = lhead;
                ltail = l->prev;
              }
              sendcmd("stop");
              do
                if ((p=resp(1)) == NULL)
#ifdef __OS2__
                  sleep(1);
#elif defined(UNIX)
                  usleep(100000);
#else
                  Sleep(100);
#endif
              while (p==NULL || strnicmp(p, "@p 0", 4));
              unlink(l->filename);
              loaddir();
              debug(1, "File %s unlinked", l->filename);
              free(l);
            }
            goto begloop;
          default:
            switch(key & 0xff)
            {
              case ' ':
                sendcmd("pause");
                mode=(mode==PLAYING) ? PAUSED : PLAYING;
                break;
              case 'n':
              case 'N':
                if (cur->next == NULL && !forever)
                  continue;
                cur = nextsong(cur, 1);
                goto begloop;
              case 'p':
              case 'P':
                if (cur->prev == NULL && (!forever || firstsong))
                  continue;
                cur = nextsong(cur, 0);
                goto begloop;
              case 'q':
              case 'Q':
                sendcmd("stop");
                markplayed(cur);
                cur = NULL;
                break;
              case 'c':
              case 'C':
              case 'e':
              case 'E':
                /* change comment */
                if (!dbopened)
                  break;
#ifndef UNIX
                back_key_first(1);
#endif
#ifdef __OS2__
                DosWaitThread(&inkeytid, DCWW_WAIT);
                edittid = _beginthread(edit_thread, 0, 65536*4, cur);
#elif defined(UNIX)
                pthread_create(&edittid, NULL, edit_thread, cur);
#else
                //waitthread(inkeytid);
                { DWORD id;
                  edittid = CreateThread(NULL, 65536*4, edit_thread, cur, 0, &id);
                }
#endif
                break;
              case 'u':
              case 'U':
                /* unmark & next */
                if (cur->next == NULL && !forever)
                  continue;
                { struct lemtype *p = searchlem(cur);
                  struct listtype *l = cur;
                  if (p) p->tag = 0;
                  if (cur->prev)
                    cur->prev->next = cur->next;
                  else
                    lhead = cur->next;
                  if (cur->next)
                  { cur = cur->next;
                    cur->prev = l->prev;
                  }
                  else
                  { cur = lhead;
                    ltail = l->prev;
                  }
                  free(l);
		  inlist--;
                }
                goto begloop;
              case '>':
                if (totaltime <= 0) continue;
                { int i = totalframes * 10 / totaltime; /* 10 sec */
                  i = (totalframes-curframe<i) ? totalframes : curframe+i;
                  sendcmd("jump %u", i);
#if 0
                  { char s[64];
                    sprintf(s, "jump to frame %u (5 secs is %u)   ",
                            i, totalframes * 5 / totaltime);
                    cline (24, 0, s);
                  }
#endif
                }
                continue;
              case '<':
                if (totaltime <= 0) continue;
                { int i = totalframes * 10 / totaltime; /* 10 sec */
                  i = (curframe<i) ? 0 : curframe-i;
                  sendcmd("jump %u", i);
#if 0
                  { char s[64];
                    sprintf(s, "jump to frame %u (5 secs is %u)   ",
                            i, totalframes * 5 / totaltime);
                    cline (24, 0, s);
                  }
#endif
                }
                continue;
              case 'd':
                goto delfile;
            }
        }
      }
      if (cur == NULL) break;
    }
    if (cur==NULL) break;
    if (edittid != INVALID_TID_VALUE)
    {
#ifdef __OS2__
      DosWaitThread(&edittid, DCWW_WAIT);
      edittid = INVALID_TID_VALUE;
      inkeytid = _beginthread(inkey_thread, 0, 65536*4, NULL);
#elif defined(UNIX)
      pthread_join(edittid, NULL);
      edittid = INVALID_TID_VALUE;
      // pthread_create(&inkeytid, NULL, inkey_thread, NULL);
#elif 0
      DWORD id;
      waitthread(edittid);
      edittid = INVALID_TID_VALUE;
      inkeytid = CreateThread(0, 65536*4, inkey_thread, NULL, 0, &id);
#else
      DWORD id;
      for (;;)
      { DWORD rc;
        if ((p=resp(-1))==NULL)
        { Sleep(1000);
          continue;
        }
        if (p[1]!='E')
          break;
        if (GetExitCodeThread(edittid, &rc) && rc!=STILL_ACTIVE)
          break;
      }
      edittid = INVALID_TID_VALUE;
      inkeytid = CreateThread(0, 65536*4, inkey_thread, NULL, 0, &id);
#endif
    }
    cur = nextsong(cur, 1);
  }
  debug(1, "No next song");
  gmouse_cancel();
#ifndef UNIX
  /* stop inkey thread */
  back_key_first(1);
#endif
#ifdef __OS2__
  DosWaitThread(&inkeytid, DCWW_WAIT);
#elif defined(UNIX)
  //pthread_join(inkeytid, NULL);
#else
  //waitthread(inkeytid);
#endif
#ifdef UNIX
  editing++;
  while (resp(0));
  editing--;
#else
  while (resp(0));
#endif
  debug(2, "resp queue purged");
#ifdef __OS2__
  DosSetPriority(PRTYS_PROCESS, PRTYC_REGULAR, 0, 0);
#else
//  SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
#endif
  /* remove played songs from list */
  for (cur=lhead; cur;)
    if (cur->played)
      cur=rmfromlist(cur);
    else
      cur=cur->next;

  remenu();
  crsr_show();
  debug(1, "play() finished");
}

#ifdef __OS2__
static int run123(void)
{
  int in[2], out[2], savein, saveout, saveerr;
  int h;

  pipe(in);
  pipe(out);
  savein =dup(fileno(stdin));
  saveout=dup(fileno(stdout));
  saveerr=dup(fileno(stderr));
  dup2(in[0], fileno(stdin));
  dup2(out[1],fileno(stdout));
  dup2(out[1],fileno(stderr));
  close(in[0]);
  close(out[1]);
#if defined(__WATCOMC__)
  DosSetFHState(in[0],  OPEN_FLAGS_NOINHERIT);
  DosSetFHState(out[1], OPEN_FLAGS_NOINHERIT);
#else
  fcntl(in[1],  F_SETFD, FD_CLOEXEC);
  fcntl(out[0], F_SETFD, FD_CLOEXEC);
#endif
  pid123 = spawnlp(P_NOWAIT, mpg123, mpg123, MPG123ARGS, NULL);
  if (pid123==-1)
    message(10, 10, " Can't run %s: %s ", mpg123, strerror(errno));
  mpgin =fdopen(in[1], "w");
  mpgout=fdopen(out[0],"r");
  dup2(savein,  fileno(stdin));
  dup2(saveout, fileno(stdout));
  dup2(saveerr, fileno(stderr));
  close(savein);
  close(saveout);
  close(saveerr);
  if (pid123==-1) return -1;
  /* create npipe resppipein, resppipeout */
  sprintf(pipename, "\\PIPE\\GPLAY_%04X", getpid());
  if (DosCreateNPipe(pipename, (PHPIPE)&h, NP_NOINHERIT|NP_ACCESS_DUPLEX,
                     NP_NOWAIT|NP_UNLIMITED_INSTANCES, 1024, 1024, 0))
    fprintf(stderr, "Can't DosCreateNPipe!\n"), exit(1);
  if (DosConnectNPipe(h)!=ERROR_PIPE_NOT_CONNECTED) /* switch to listening mode */
    fprintf(stderr, "Can't first DosConnactNPipe!\n"), exit(1);
#ifdef __WATCOMC__
  _hdopen(h,O_TEXT|O_RDWR);
#endif
  resppipeout=fopen(pipename, "w+");
  if (resppipeout==NULL)
    fprintf(stderr, "Can't fopen pipe: %s!\n", strerror(errno)), exit(1);
  if (DosConnectNPipe(h)) /* real connect */
    fprintf(stderr, "Can't second DosConnactNPipe!\n"), exit(1);
  DosSetNPHState(h, NP_WAIT);
  resppipein = fdopen(h, "r+");
  if (resppipein==NULL)
    fprintf(stderr, "Can't fopen pipe!\n"), exit(1);
  resptid = _beginthread(resp_thread,  NULL, 32786, NULL);
  resp(1);
  sendcmd("volume %d", volume);
debug(2, "sendcmd ok");
  resp(1);
debug(2, "set volume ok");
  return pid123;
}

#elif defined(UNIX)

static int run123(void)
{
  int in[2], out[2];

  pipe(in);
  pipe(out);
  pid123 = fork();
  if (pid123 == 0) {
    /* give up controlling terminal */
    // setsid();
    dup2(in[0],  fileno(stdin));
    dup2(out[1], fileno(stdout));
    dup2(out[1], fileno(stderr));
    close(in[0]);
    close(in[1]);
    close(out[0]);
    close(out[1]);
    execlp(mpg123, mpg123, MPG123ARGS, NULL);
    exit(0);
  }
  else if (pid123 == -1) {
    message(10, 10, " Can't run %s: %s ", mpg123, strerror(errno));
    exit(1);
  }
  close(in[0]);
  close(out[1]);
  mpgin =fdopen(in[1], "w");
  mpgout=fdopen(out[0],"r");
  if (pid123==-1) return -1;
  resp(1);
  sendcmd("volume %d", volume);
debug(2, "sendcmd ok");
#if defined(HAVE_RVOLUME)
  resp(1);
debug(2, "set volume ok");
#endif
  return pid123;
}

static void rip(int signo)
{
  int r;
  if (signo != SIGCHLD) return;
  if (waitpid(pid123, &r, WNOHANG) == pid123) {
    if (WIFSIGNALED(r))
      debug(1, "mpg123 finished by %s", strsignal(WTERMSIG(r)));
    else
      debug(1, "mpg123 finished with retcode %u", WEXITSTATUS(r));
    pid123 = -1;
  }
  // while (wait(NULL) != -1);
  signal(SIGCHLD, rip);
  return;
}

#else

static int CreateChildProcess(char *command, ...)
{
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   char cmdline[1024], *p;
   va_list args;

   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO); 

   strncpy(cmdline, command, sizeof(cmdline));
   va_start(args, command);
   while ((p=va_arg(args, char*))!=NULL)
   { strncat(cmdline, " ", sizeof(cmdline));
     strncat(cmdline, p, sizeof(cmdline));
   }
   va_end(args);

   if (CreateProcess(NULL, 
      "mpg123.exe -R --reopen",       // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo))  // receives PROCESS_INFORMATION 
	return 0;
   else
	return -1;
}

static int run123(void)
{ 
   int pid;
   DWORD ThreadId;
#if defined(__CYGWIN__) && 0
   int hmpgin, hmpgout;
   int hin[2], hout[2];
   int savein, saveout;

   if (pipe(hin))
   { debug(1, "Can't create pipe: %s", strerror(errno));
     return -1;
   }
   if (pipe(hout))
   { debug(1, "Can't create pipe: %s", strerror(errno));
     close(hin[0]);
     close(hin[1]);
     return -1;
   }
   savein = dup(fileno(stdin));
   saveout = dup(fileno(stdout));
   dup2(hin[0], fileno(stdin));
   close(hin[0]);
   hmpgin = hin[1];
   dup2(hout[1], fileno(stdout));
   close(hout[1]);
   hmpgout = hout[0];
   if (fcntl(hmpgin,  F_SETFL, O_NOINHERIT))
     debug(1, "set O_NOINHERIT fails: %s", strerror(errno));
   fcntl(hmpgout, F_SETFL, O_NOINHERIT);
     debug(1, "set O_NOINHERIT fails: %s", strerror(errno));
   if (fcntl(hmpgin,  F_SETFD, FD_CLOEXEC))
     debug(1, "set FD_CLOEXEC fails: %s", strerror(errno));
   fcntl(hmpgout, F_SETFD, FD_CLOEXEC);
     debug(1, "set FD_CLOEXEC fails: %s", strerror(errno));
   debug(4, "hmpgin GETFL returns %04x", fcntl(hmpgin, F_GETFL));
   debug(4, "hmpgin GETFD returns %04x", fcntl(hmpgin, F_GETFD));
   pid = CreateChildProcess(mpg123, MPG123ARGS, NULL);
   // pid = spawnlp(P_NOWAIT, mpg123, mpg123, MPG123ARGS, NULL);
   if (pid == -1)
   { debug(1, "Can't spawn %s: %s", mpg123, strerror(errno));
   }
   dup2(savein, fileno(stdin));
   close(savein);
   dup2(saveout, fileno(stdout));
   close(saveout);
   if (pid == -1)
   { close(hmpgin);
     close(hmpgout);
     return -1;
   }
#else
#ifndef __CYGWIN__
   int hmpgin, hmpgout;
#endif
   SECURITY_ATTRIBUTES saAttr; 
   HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr,
      hSaveStdin, hSaveStdout, hSaveStderr; 
   HANDLE hChildStdinWrDup, hChildStdoutRdDup;

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
   saAttr.bInheritHandle = TRUE; 
   saAttr.lpSecurityDescriptor = NULL; 
 
   hSaveStdout = GetStdHandle(STD_OUTPUT_HANDLE); 

   if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) 
	return -1;
 
   if (! SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr)) 
   {	CloseHandle(hChildStdoutWr);
	CloseHandle(hChildStdoutRd);
	return -1;
   }

   hSaveStderr = GetStdHandle(STD_ERROR_HANDLE); 

   if (! SetStdHandle(STD_ERROR_HANDLE, hChildStdoutWr)) 
   {	SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	CloseHandle(hChildStdoutWr);
	CloseHandle(hChildStdoutRd);
	return -1;
   }

   if (! DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
        GetCurrentProcess(), &hChildStdoutRdDup , 0, FALSE,
        DUPLICATE_SAME_ACCESS))
   {	SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);
	CloseHandle(hChildStdoutWr);
	CloseHandle(hChildStdoutRd);
	return -1;
   }
   CloseHandle(hChildStdoutRd);

   hSaveStdin = GetStdHandle(STD_INPUT_HANDLE); 
 
   if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) 
   {	SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);
	CloseHandle(hChildStdoutWr);
	CloseHandle(hChildStdoutRdDup);
	return -1;
   }
 
   if (! SetStdHandle(STD_INPUT_HANDLE, hChildStdinRd)) 
   {	SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);
	CloseHandle(hChildStdoutWr);
	CloseHandle(hChildStdoutRdDup);
	CloseHandle(hChildStdinWr);
	CloseHandle(hChildStdinRd);
	return -1;
   }
 
   if (! DuplicateHandle(GetCurrentProcess(), hChildStdinWr, 
      GetCurrentProcess(), &hChildStdinWrDup, 0, 
      FALSE,                  // not inherited 
      DUPLICATE_SAME_ACCESS))
   {	SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);
	SetStdHandle(STD_INPUT_HANDLE, hSaveStdin);
	CloseHandle(hChildStdoutWr);
	CloseHandle(hChildStdoutRdDup);
	CloseHandle(hChildStdinWr);
	CloseHandle(hChildStdinRd);
	return -1;
   }

   CloseHandle(hChildStdinWr);

   pid = CreateChildProcess(mpg123, MPG123ARGS, NULL);

   SetStdHandle(STD_INPUT_HANDLE, hSaveStdin);
   SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
   SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);
   CloseHandle(hChildStdoutWr);
   CloseHandle(hChildStdinRd);

   if (pid == -1)
   {	CloseHandle(hChildStdoutRdDup);
	CloseHandle(hChildStdinWrDup);
	return pid;
   }

#ifndef __CYGWIN__
   hmpgin = _open_osfhandle((long)hChildStdinWrDup, O_TEXT);
   hmpgout = _open_osfhandle((long)hChildStdoutRdDup, O_TEXT);
#endif
#endif
#ifdef __CYGWIN__
   mpgin = hChildStdinWrDup;
   mpgout = hChildStdoutRdDup;
#else
   mpgin = fdopen(hmpgin, "w");
   mpgout = fdopen(hmpgout, "r");
#endif
   resptid = CreateThread(NULL, 65536*4, resp_thread, NULL, 0, &ThreadId);
   if (resptid == NULL)
     message(10, 10, "Can't create thread: error %lu", GetLastError());
   else
     debug(1, "resp_thread created, tid %ld", (long)resptid);
   resp(1);
   sendcmd("volume %d", volume);
debug(2, "sendcmd ok");
   resp(1);
debug(2, "set volume ok");
   return pid;
} 
#endif

int myinkey(void)
{ int r;

#ifdef UNIX
  inkey=pipeinkey;
#else
  inkey=_inkey;
#endif
  r=biginkey(
        "Help  SavLstLdListEdit              SearchDelFil      Exit  ",
        "                                    SrcNxt                  ",
        "            AddLst                                          ",
        "                                    SrchBk                  ");
  inkey=myinkey;
  return r;
}

int addlistfile(char *name)
{
  char *p;
  struct lemtype *l;
  struct stat st;

  if (stat(name, &st)) {
    debug(1, "Cannot stat %s: %s", name, strerror(errno));
    return 1;
  }
  if (access(name, R_OK)) return 1;
  if (ltail == NULL)
  { ltail = malloc(sizeof(*ltail));
    if (ltail == NULL)
      exit(3);
    lhead = ltail;
    ltail->prev = NULL;
  }
  else
  { ltail->next = malloc(sizeof(*ltail));
    if (ltail->next == NULL)
      exit(3);
    ltail->next->prev = ltail;
    ltail = ltail->next;
  }
  strncpy(ltail->filename, name, sizeof(ltail->filename)-1);
  ltail->filename[sizeof(ltail->filename)-1] = '\0';
  p = strrchr(name, '/');
  if (p == NULL) p=name;
  else p++;
  if (strrchr(p, '\\')) p = strrchr(p, '\\')+1;
  if ((p=fetchdb(st.st_size, xlat(dir_charset, db_charset, p), NULL)) != NULL)
  { strncpy(ltail->comment, xlat(db_charset, scr_charset, p), sizeof(ltail->comment)-1);
    ltail->comment[sizeof(ltail->comment)-1] = '\0';
  }
  else
    ltail->comment[0] = '\0';
  ltail->played = 0;
  ltail->size = st.st_size;
  ltail->next = NULL;
  inlist++;
  if ((l=searchlem(ltail)) != NULL)
    l->tag = 1;
  return 0;
}

int loadlistfile(char *listfile, int clean)
{
  char name[1024], *p;
  FILE *f;
  int  pole;

  strcpy(name, playlists);
  if (strchr("/\\", playlists[strlen(playlists)-1])==NULL)
    strcat(name, PATHSEPS);
  strcat(name, listfile);
  if (strchr(listfile, '.') == NULL)
    strcat(name, LISTEXT);
  if ((f=fopen(name, "r"))==NULL)
  { message(10, 10, " Can't open %s: %s! ", name, strerror(errno));
    return 1;
  }
  if (clean) 
  { cleanlist();
    for (pole=0; pole<poley; pole++)
      lem[pole].tag = 0;
  }
  while (fgets(name, sizeof(name), f))
  {
    p=strchr(name, '\n');
    if (p) *p='\0';
    addlistfile(name);
  }
  fclose(f);
  return 0;
}

int loadlist(int clean)
{
  DIR *d;
  struct dirent *df;
  char **listnames=NULL, **slistnames=NULL;
  int nlistnames=0;
  int pole, upline, r;
  char *p;
  char name[1024];
  struct stat st;

  d=opendir(playlists);
  if (d==NULL)
  { message(10, 10, " Can't opendir %s: %s! ", playlists, strerror(errno));
    return 1;
  }
  while ((df=readdir(d))!=NULL)
  {
    if (df->d_name[0]=='.')
      continue;
    if (strlen(df->d_name)<5 ||
        stricmp(df->d_name+strlen(df->d_name)-strlen(LISTEXT), LISTEXT))
      continue;
    strcpy(name, playlists);
    if (strchr("/\\", playlists[strlen(playlists)-1])==NULL)
      strcat(name, PATHSEPS);
    strcat(name, df->d_name);
    if (stat(name, &st))
      continue;
    if (S_ISDIR(st.st_mode))
      continue;
    listnames=realloc(listnames, (nlistnames + 1) * sizeof(*listnames));
    slistnames=realloc(slistnames, (nlistnames + 1) * sizeof(*slistnames));
    listnames[nlistnames] = strdup(df->d_name);
    p=strrchr(listnames[nlistnames], '.');
    *p='\0';
    slistnames[nlistnames] = strdup(xlat(dir_charset, scr_charset, listnames[nlistnames]));
    *p='.';
    nlistnames++;
  }
  closedir(d);
  if (nlistnames==0)
  { message(10, 10, " Playlists not found! ");
    return 1;
  }
  pole=upline=0;
  scrlmenu_redraw=REDRAW_FULL;
  evakuate(5, maxy/2-10, maxx-7, 24);
  do
    r=scrlmenu(col_edit_ramka, col_edit_text, 5, maxy/2-10, maxx-10, 20,
               "Select playlist", &pole, &upline, nlistnames, slistnames);
  while (r!=escc && r!=enterc && r!=mouse_enter);
  if (r!=escc)
    pole = loadlistfile(listnames[pole], clean);
  else
    pole = 1;
  remenu();
  for (nlistnames--; nlistnames>=0; nlistnames--)
  { free(listnames[nlistnames]);
    free(slistnames[nlistnames]);
  }
  free(listnames);
  free(slistnames);
  return 1;
}

int savelist(void)
{
  char name[256];
  char fname[1024];
  FILE *f;
  struct listtype *l;

savelistagain:
  strcpy(name, "                    ");
  if (mkorr(col_edit_ramka, col_edit_text, 10, 20, 3, 40, 10,
            " listname: ", name, 0)==0)
  { remenu();
    return 1;
  }
  remenu();
  stripspc(name);
  strcpy(fname, playlists);
  if (strchr("/\\", fname[strlen(fname)-1])==NULL)
    strcat(fname, "/");
  strcat(fname, xlat(scr_charset, dir_charset, name));
  strcat(fname, LISTEXT);
  if (access(fname, 0) == 0)
  { int pole=0;
    quere(redf+very+yellow, whitef+black, 10, 20, 4, 2, &pole,
          "File", xlat(dir_charset, scr_charset, name),
          " already exists. ", "Overwrite?",
          " Yes ", "Yy", " No ", "Nn\x1b");
    if (pole==1)
      goto savelistagain;
  }
  f = fopen(fname, "w");
  if (f == NULL)
  { message(10, 10, " Can't open %s: %s! ",
            xlat(dir_charset, scr_charset, name), strerror(errno));
    return 1;
  }
  for (l = lhead; l; l = l->next)
  {
    fprintf(f, "%s\n", l->filename);
  }
  fclose(f);
  return 0;
}

static char searchstr[50]="";

void searchnext(int direction)
{
  char copystr[sizeof(searchstr)];
  char *p;
  int  i;
  chtype col=0;

  strcpy(copystr, searchstr);
  strlwr(copystr);
  for (i=pole+direction; i>=0 && i<poley; i+=direction)
  { p=putline(i, &col);
    strlwr(p);
    if (strstr(p, copystr))
      break;
  }
  if (i<poley && i>=0)
  { pole=i;
    if (pole-topline>maxx-5)
      topline=pole-(maxx-5);
    else if (topline>pole)
      topline=pole;
    scrlmenu_redraw=REDRAW_TEXT;
  }
  else
    message(10, 20, " String not found! ");
}

void search(int direction)
{
  int i;

  for (i=strlen(searchstr); i<sizeof(searchstr)-1; i++)
    searchstr[i] = ' ';
  searchstr[sizeof(searchstr)-1] = '\0';
  crsr_show();
  if (mkorr(col_edit_ramka, col_edit_text, 9, 5, 3, 70, 13,
            (direction==1) ? " search fwd: " : " search bck: ", searchstr,0)==0)
  { remenu();
    crsr_hide();
    stripspc(searchstr);
    return;
  }
  remenu();
  crsr_hide();
  stripspc(searchstr);
  if (searchstr[0]=='\0')
    return;
  searchnext(direction);
}

void usage(void)
{
  puts("GPlay ver " VER "          " __DATE__);
#ifdef __OS2__
  puts("mpg123 frontend for OS/2");
#elif defined(UNIX)
  puts("mpg123 frontend for Unix");
#else
  puts("mpg123 frontend for Win");
#endif
  puts("Copyright (C) Pavel Gulchouck <gul@gul.kiev.ua> 2:463/68@fidonet");
  puts("");
  puts("  Usage:");
  puts("gplay [opts] [filename] ...");
  puts("  Options:");
  puts("-V <volume>   - set initial volume (0..100)");
  puts("-@ <listname> - load playlist");
  puts("-c <confname> - set config name (default is gplay.cfg)");
  puts("-s <path>     - set start directory");
  puts("-d <level>    - set debug level");
  puts("-f            - play forever");
  puts("-p            - play mode (no menu)");
  puts("-r            - random order");
}
          
int main(int argc, char * argv[])
{ int col, r;
  char curdir[1024] = "";
  int setvolume=0;
  char *confname = NULL;
  int unattended = 0, wasusage = 0;
  char *listfiles[256];
  int nlistfiles = 0;

  optind = 0;
  for (;;)
  {
    switch (getopt(argc, argv, "V:c:s:@:fphd:-"))
    {
      case 'c':	confname = optarg;
		continue;
      case 'V':	setvolume = atoi(optarg);
		continue;
      case 's':	strcpy(curdir, optarg);
		continue;
      case 'f':	forever = 1;
		continue;
      case 'p':	unattended = 1;
		continue;
      case 'r':	rnd = 1;
		continue;
      case '@':	listfiles[nlistfiles++] = strdup(optarg);
		continue;
      case 'd': debuglevel = atoi(optarg);
		getcwd(dbgname, 256);
		strcat(dbgname, PATHSEPS "dbg.log");
		continue;
      case '?':
      case ':':
      case 'h':	usage();
		wasusage = 1;
		continue;
      case '-':
      case -1:	break;
    }
    break;
  }
  if (wasusage)
    return 1;
#ifdef UNIX
  setlocale(LC_ALL, "");
#endif
  config(confname);
debug(2, "config() ok");
  if (opendb()==0)
    dbopened=1;
debug(2, "opendb ok");
  for (r=0; r<nlistfiles; r++)
  { loadlistfile(listfiles[r], 0);
    free(listfiles[r]);
  }
  for (;optind<argc; optind++)
    addlistfile(argv[optind]);
  if (setvolume)
    volume = setvolume;
  if (volume<0 || volume>100)
    volume = 50;
  if (curdir[0])
  {
#ifndef UNIX
    if (curdir[1] == ':')
#ifdef __OS2__
      DosSetDefaultDisk(toupper(curdir[0])-'A'+1);
#else
      _chdrive(toupper(curdir[0])-'A'+1);
#endif
#endif
    chdir(curdir);
  }
  gulib_init();
  initmouse();
  askmouse();
  /* setmouserep(2); */
  graph(&maxx,&maxy,&col);
  evakuate(0,0,maxx,maxy);
  pole=topline=0;
  phonebiginkey=skyf+black;
  fnbiginkey=blackf+white;
  scrollcol=bluef+white;
  head=malloc(maxy);
  if (head==NULL) {
    gulib_deinit();
    return 1;
  }
  head[0] = '\0';
  paused=playing=0;
#ifdef UNIX
  pthread_mutex_init(&sem_refresh, NULL);
  { int h[2];
    pipe(h);
    resppipein =fdopen(h[0], "r");
    resppipeout=fdopen(h[1], "w");
    pthread_create(&inkeytid, NULL, inkey_thread, NULL);
    inkey=pipeinkey;
    debug(2, "inkey thread created");
    pipe(edpipe);
  }
  signal(SIGCHLD, rip);
  signal(SIGPIPE, SIG_IGN);
#endif
debug(2, "running mpg123...");
  run123();
  if (pid123==-1)
  { remenu();
debug(2, "run failed");
    gulib_deinit();
    return 3;
  }
debug(2, "run ok");
debug(5, "unattended is %s, playlist %s", unattended ? "on" : "off", lhead ? "filled" : "empty");
  if (unattended && lhead)
  { play();
    if (dbopened)
      closedb();
    remenu();
    sendcmd("q");
    waitpid(pid123, &r, 0);
    gulib_deinit();
    return 0;
  }
  loaddir();
debug(2, "loaddir done");
  barputcol(col_menu_title, 0, 0, 1, maxy);
  barfill(' ', 0, 0, 1, maxy);
debug(2, "barcell done");
  // while (resp(0));
  for (;;)
  {
    inkey=myinkey;
    snprintf(head, maxy-1, " Volume %3d%%   Marked %4d  %16s",
            volume, inlist, forever ? "Playing forever" : "");
    head[maxy-1] = '\0';
    cline(0, 0, head);
debug(2, "cline head done");
    getcwd(curdir, sizeof(curdir));
    convcwd(curdir);
    if (maxy>23)
    {
      char *p, *p1;
      char line[1024];

      p = xlat(dir_charset, scr_charset, curdir);
      if (strlen(p) + 19 > maxy)
      {
        strcpy(line, "...");
        p = p + strlen(p) + 19 - maxy;
        p1 = strchr(p+3, PATHSEP);
	if (p1)
          strcpy(line+3, p1);
        else
          strcpy(line+3, p+3);
        sprintf(head, " Current dir: [%s] ", line);
      } else
        sprintf(head, " Current dir: [%s] ", p);
    } else
      head[0] = '\0';
debug(2, "calling scrlmenuf");
    r=scrlmenuf(col_menu_normal, col_menu_select, 1, 0, maxx-4, maxy-2, head,
                &pole, &topline, poley, putline);
debug(2, "scrlmenuf done, retcode %02x", r);
#ifdef UNIX
    inkey=pipeinkey;
#else
    inkey=_inkey;
#endif
    scrlmenu_redraw=0;
    rnd=0;
    switch (r)
    { case 0:
      case escc:
      case altxc:
      case f10c:
        break;
      case enterc:
      case mouse_enter:
#ifndef UNIX
        if (lem[pole].filename[1] == ':')
        { /* disk */
#ifdef __OS2__
          DosSetDefaultDisk(lem[pole].filename[0]-'A'+1);
#else
          _chdrive(lem[pole].filename[0]-'A'+1);
#endif
          chdir("\\");
          loaddir();
          pole = topline = 0;
          scrlmenu_redraw=REDRAW_FULL;
        }
        else
#endif
        if (lem[pole].filename[strlen(lem[pole].filename)-1] == PATHSEP)
        { /* dir */
          char *newpos, *p;
          lem[pole].filename[strlen(lem[pole].filename)-1] = '\0';
          newpos=NULL;
          if (strcmp(lem[pole].filename, "..")==0)
          { /* goto updir */
            if ((newpos = malloc(1024)) != NULL)
            { getcwd(newpos, 1024-2);
              p=strrchr(newpos, PATHSEP);
              if (p)
                strcpy(newpos, p+1);
              strcat(newpos, PATHSEPS);
            }
          }
          chdir(lem[pole].filename);
          loaddir();
          pole = topline = 0;
          if (newpos)
          { for (;pole<poley; pole++)
              if (strcmp(lem[pole].filename, newpos)==0)
                break;
            free(newpos);
            if (pole>=poley)
              pole=0;
            else if (pole>maxx-5)
              topline=pole-(maxx-5);
          }
          scrlmenu_redraw=REDRAW_FULL;
        }
        else
enterkey:
        { /* single file */
          struct listtype *savehead=lhead, *savetail=ltail;
          int savetag=lem[pole].tag, saveinlist=inlist;
          lhead=ltail=NULL;
          addlist(lem+pole);
          play();
          cleanlist();
          lhead=savehead;
          ltail=savetail;
	  inlist=saveinlist;
          lem[pole].tag=savetag;
          scrlmenu_redraw=REDRAW_TEXT;
        }
        continue;
      case insc:
      case spacec:
tagentry:
        if (lem[pole].size == 0)
          continue;
        lem[pole].tag^=1;
        if (lem[pole].tag)
          addlist(lem+pole);
        else
          dellist(lem+pole);
        if (pole < poley) {
          pole++;
          if (pole-topline >= maxx-1) topline++;
        }
        continue;
      case rightc:
        if (volume==100)
          continue;
        sendcmd("V %d", volume+=5);
#if !defined(UNIX) || defined(HAVE_RVOLUME)
        resp(1);
#endif
        continue;
      case leftc:
        if (volume==0)
          continue;
        sendcmd("V %d", volume-=5);
#if !defined(UNIX) || defined(HAVE_RVOLUME)
        resp(1);
#endif
        continue;
      case f4c:
        if (dbopened)
        { editsong(pole);
          scrlmenu_redraw=REDRAW_TEXT;
        }
        continue;
      case f2c:
        savelist();
        continue;
      case f3c:
        if (loadlist(1) == 0)
          scrlmenu_redraw=REDRAW_TEXT;
        continue;
      case cf3c:
        if (loadlist(0) == 0)
          scrlmenu_redraw=REDRAW_TEXT;
        continue;
      case f7c:
        search(1);
        continue;
      case sf7c:
        if (searchstr[0]=='\0')
        { message(10, 20, " Nothing to search! ");
          continue;
        }
        searchnext(1);
        continue;
      case altf7c:
        search(-1);
        continue;
      case f8c:
delfile:
        r=0;
        quere(redf+very+yellow, whitef+black, 10, 20, 2, 2, &r,
              " Delete file ", lem[pole].filename,
              " Yes ", "Yy", " No ", "Nn\x1b");
        if (r==1)
          continue;
        unlink(lem[pole].filename);
        loaddir();
        if (pole>=poley) pole = poley-1;
        if (topline && topline>poley-(maxx-4))
        { topline = poley-(maxx-4);
          if (topline<0) topline=0;
	}
        scrlmenu_redraw=REDRAW_FULL;
        continue;
      case f1c:
showhelp:
	{ int i=1;
#define col 12
          ramka(bluef+very+yellow, i++, col-2, 22, 60);
          cline(i++, col, "Ins, Space    -- tag (mark) song");
          cline(i++, col, "Enter         -- play song");
          cline(i++, col, "'p'           -- play tagged songs");
          cline(i++, col, "'f'           -- circle play (forever)");
          cline(i++, col, "'a', '*'      -- toggle all tags");
          cline(i++, col, "'c'           -- clear all tags");
          cline(i++, col, "'r'           -- set random order");
          cline(i++, col, "'n'           -- restart mpg123");
          cline(i++, col, "'R'           -- reread directory");
          cline(i++, col, "'+', ->       -- increase volume");
          cline(i++, col, "'-', <-       -- decrease volume");
          cline(i++, col, "f1, 'h'       -- this help");
          cline(i++, col, "f2, 's'       -- save tagged as playlist");
          cline(i++, col, "f3, 'l'       -- add playlist to tagged");
          cline(i++, col, "ctrl+f3, 'A'  -- clear tags and load playlist");
          cline(i++, col, "f4, 'e'       -- edit description");
          cline(i++, col, "f7, '/'       -- search forward");
          cline(i++, col, "alt+f7, '?'   -- search backward");
          cline(i++, col, "f8, 'd'       -- delete file");
          cline(i++, col, "f10, Esc, 'q' -- exit gplay");
#undef col
        }
        inkey();
        remenu();
        continue;
      default:
        switch (r & 0xff)
        {
          case ' ':
          case 't':
            goto tagentry;
          case 'd':
            goto delfile;
          case 'h':
            goto showhelp;
          case 'q':
            break;
          case 'a':
#if 0 /* not only mark, do mark/unmark as '*' */
            { int i;
              for (i=0; i<poley; i++)
                if (!lem[i].tag && lem[i].size)
                { addlist(lem+i);
                  lem[i].tag = 1;
                }
            }
            scrlmenu_redraw |= REDRAW_TEXT;
            continue;
#endif
          case '*':
            { int i;
              for (i=0; i<poley; i++)
              {
                if (lem[i].size == 0)
                  continue;
                lem[i].tag ^= 1;
                if (lem[i].tag)
                  addlist(lem+i);
                else 
                  dellist(lem+i);
              }
            }
            scrlmenu_redraw |= REDRAW_TEXT;
            continue;
          case 'c':
            { int i;
              for (i=0; i<poley; i++)
                lem[i].tag = 0;
            }
            cleanlist();
            scrlmenu_redraw |= REDRAW_TEXT;
            continue;
          case '+':
            if (volume==100)
              continue;
            sendcmd("V %d", volume+=5);
#if !defined(UNIX) || defined(HAVE_RVOLUME)
            resp(1);
#endif
            continue;
          case '-':
            if (volume==0)
              continue;
            sendcmd("V %d", volume-=5);
#if !defined(UNIX) || defined(HAVE_RVOLUME)
            resp(1);
#endif
            continue;
          case 'r':
            rnd=1;
            /* fall through */
          case 'p':
            if (lhead)
            { play();
              scrlmenu_redraw=REDRAW_TEXT;
            }
            else if (lem[pole].size)
              goto enterkey;
            continue;
          case 'f':
            forever ^= 1;
            continue;
          case 'R':
            loaddir();
            scrlmenu_redraw=REDRAW_FULL;
            continue;
          case 'n':
            sendcmd("q");
#ifdef __CYGWIN__
	    CloseHandle(mpgin);
#else
            fclose(mpgin);
#endif
            waitpid(pid123, &r, 0);
#ifdef __CYGWIN__
	    CloseHandle(mpgin);
#else
            fclose(mpgout);
#endif
#ifdef __OS2__
            DosWaitThread(&resptid, DCWW_WAIT);
            fclose(resppipein);
            fclose(resppipeout);
#elif defined(UNIX)
            fclose(resppipeout);
            fclose(resppipein);
#else
            waitthread(resptid);
#endif
            run123();
            if (pid123==-1)
            { remenu();
              gulib_deinit();
              return 3;
            }
            continue;
          case '/':
            search(1);
            continue;
          case '?':
            search(-1);
            continue;
          case 's':
            savelist();
            continue;
          case 'l':
            if (loadlist(1) == 0)
              scrlmenu_redraw=REDRAW_TEXT;
            continue;
          case 'e':
            if (dbopened)
            { editsong(pole);
              scrlmenu_redraw=REDRAW_TEXT;
            }
            continue;
          case 'A':
            if (loadlist(0) == 0)
              scrlmenu_redraw=REDRAW_TEXT;
            continue;
          default:
            continue;
        }
    }
    break;
  }
  if (dbopened)
    closedb();
  remenu();
  sendcmd("q");
  waitpid(pid123, &r, 0);
  gulib_deinit();
  return 0;
}
