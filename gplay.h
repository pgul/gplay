#define VER "0.2"

#include <gulib.h>

#ifndef UNIX
typedef int chtype;
#endif

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
int config(char *confname);
int debug(int level, char *format, ...);

extern chtype
    col_menu_normal,
    col_menu_title, 
    col_menu_select, 
    col_menu_tagged,
    col_play_normal,
    col_play_clicable,
    col_play_volume1,
    col_play_volume2,
    col_play_volume3,
    col_play_text,
    col_play_text2,
    col_play_hilight,
    col_play_hilight2,
    col_play_firstlet,
    col_play_ramka1,
    col_play_ramka2,
    col_play_tape,
    col_play_tapecur,
    col_play_brackets,
    col_edit_text,
    col_edit_ramka;
extern int volume;
extern int debuglevel;
extern char dbgname[];
extern char mpg123[], playlists[], dbname[];
extern char scr_charset[], mp3_charset[], dir_charset[], db_charset[];
#define src_charset "koi8-u"

#ifdef __EMX__
#define getcwd(buf, size)  _getcwd2(buf, size)
#endif

#ifdef __MINGW32__
#define stat _stat
#define fstat _fstat
#define open _open
#define close _close
#define read _read
#define write _write
#define snprintf  _snprintf
#define vsnprintf _vsnprintf
#endif

#ifdef UNIX
#define stricmp(s1, s2)		strcasecmp(s1, s2)
#define strnicmp(s1, s2, n)	strncasecmp(s1, s2, n)
#else
#endif

#if defined(__EMX__) || defined(__CYGWIN__) || defined(UNIX)
#define PATHSEP     '/'
#define PATHSEPS    "/"
#else
#define PATHSEP     '\\'
#define PATHSEPS    "\\"
#endif

#ifdef __CYGWIN__
int _chdrive(int);
#endif

