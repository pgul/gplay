#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __OS2__
#define INCL_DOSPROCESS
#include <os2.h>
#else
#include <io.h>
#ifdef __CYGWIN__
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <windows.h>
#endif
#include "glib.h"
#include "keyc.h"
#include "gplay.h"

int col_menu_normal,
    col_menu_title, 
    col_menu_select, 
    col_menu_tagged;
int col_play_normal,
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
    col_play_brackets;
int col_edit_ramka,
    col_edit_text;

char mpg123[1024] = "mpg123.exe";
char playlists[1024] = "";
static char startdir[1024] = "";

static int col_menu_bg,
           col_menu_title_bg,
           col_menu_select_bg,
           col_play_bg,
           col_edit_ramka_bg,
           col_edit_text_bg;

static struct {
  char name[80];
  enum { PAR_COLOR, PAR_DIG, PAR_STRING, PAR_PATH } type;
  void *val;
  int  def;
} params[] = {
{ "COL_MENU_FG",		PAR_COLOR,	&col_menu_normal, 	very+sky },
{ "COL_MENU_BG",		PAR_COLOR,	&col_menu_bg,	 	blue },
{ "COL_MENU_TITLE_FG",		PAR_COLOR,	&col_menu_title, 	black },
{ "COL_MENU_TITLE_BG",		PAR_COLOR,	&col_menu_title_bg, 	white },
{ "COL_MENU_TAGGED",		PAR_COLOR,	&col_menu_tagged, 	very+yellow },
{ "COL_MENU_SELECT_FG",		PAR_COLOR,	&col_menu_select, 	black },
{ "COL_MENU_SELECT_BG",		PAR_COLOR,	&col_menu_select_bg, 	white },
{ "COL_PLAY_BG",		PAR_COLOR,	&col_play_bg, 		black },
{ "COL_PLAY_NORMAL_FG",		PAR_COLOR,	&col_play_normal, 	very+black },
{ "COL_PLAY_CLICABLE",		PAR_COLOR,	&col_play_clicable,	very+red },
{ "COL_PLAY_VOLUME1",		PAR_COLOR,	&col_play_volume1, 	sky },
{ "COL_PLAY_VOLUME2",		PAR_COLOR,	&col_play_volume2, 	very+green },
{ "COL_PLAY_VOLUME3",		PAR_COLOR,	&col_play_volume3, 	very+red },
{ "COL_PLAY_TEXT",		PAR_COLOR,	&col_play_text, 	very+blue },
{ "COL_PLAY_TEXT2",		PAR_COLOR,	&col_play_text2, 	very+black },
{ "COL_PLAY_HILIGHT",		PAR_COLOR,	&col_play_hilight, 	very+white },
{ "COL_PLAY_HILIGHT2",		PAR_COLOR,	&col_play_hilight2, 	very+green },
{ "COL_PLAY_FIRSTLET",		PAR_COLOR,	&col_play_firstlet, 	very+sky },
{ "COL_PLAY_RAMKA1",		PAR_COLOR,	&col_play_ramka1, 	very+red },
{ "COL_PLAY_RAMKA2",		PAR_COLOR,	&col_play_ramka2, 	red },
{ "COL_PLAY_TAPE",		PAR_COLOR,	&col_play_tape, 	very+black },
{ "COL_PLAY_TAPECUR",		PAR_COLOR,	&col_play_tapecur, 	very+blue },
{ "COL_PLAY_BRACKETS",		PAR_COLOR,	&col_play_brackets, 	blue },
{ "COL_EDIT_RAMKA",		PAR_COLOR,	&col_edit_ramka, 	black },
{ "COL_EDIT_RAMKA_BG",		PAR_COLOR,	&col_edit_ramka_bg, 	white },
{ "COL_EDIT_TEXT",		PAR_COLOR,	&col_edit_text, 	black },
{ "COL_EDIT_TEXT_BG",		PAR_COLOR,	&col_edit_text_bg, 	sky },

{ "MPG123",			PAR_PATH,	mpg123,			0 },
{ "PLAYLISTS",			PAR_PATH,	playlists,		0 },
{ "STARTDIR",			PAR_PATH,	startdir,		0 },
{ "DBNAME",			PAR_PATH,	dbname,			0 },

{ "VOLUME",			PAR_DIG,	&volume,		50 }
};

#ifdef __CYGWIN__
int _chdrive(int drive)
{
  char path[80];

  sprintf(path, "/cygdrive/%c", 'a'+drive-1);
  return chdir(path);
}
#endif

int config(char *confname)
{
  FILE *f;
  char str[1024], *p, *myname;
  int i;
  struct stat st;

  for (i=0; i<sizeof(params)/sizeof(*params); i++)
  {
    switch(params[i].type)
    {
      case PAR_COLOR:
      case PAR_DIG:
        *(int *)(params[i].val) = params[i].def;
        break;
      case PAR_STRING:
      case PAR_PATH:
        ;
    }
  }
  {
#ifdef __OS2__
    PPIB pib;
    PTIB tib;
    DosGetInfoBlocks(&tib, &pib);
    if (pib)
    { for (myname=pib->pib_pchenv; myname[0] || myname[1]; myname++);
      myname+=2;
    }
    else
      myname = "gplay.exe";
#else
    myname = strdup((char *)GetCommandLine());
    p = strchr(myname, ' ');
    if (p) *p='\0';
#endif
  }
  if (confname == NULL)
  { confname = malloc(1024);
    if (confname == NULL)
      exit(3);
    strcpy(confname, myname);
    p=strrchr(confname, '\\');
    if (p==NULL) p=confname;
    else p++;
    strcpy(p, "gplay.cfg");
  }
  strcpy(playlists, myname);
  p=strrchr(playlists, '\\');
  if (p && (p!=playlists) && (playlists[1]!=':' || p!=playlists+2))
    *p='\0';
  else if (p)
    p[1]='\0';
  else
    getcwd(playlists, sizeof(playlists));
  f=fopen(confname, "r");
  if (f)
  {
    while (fgets(str, sizeof(str), f))
    {
      if ((p=strchr(str, '\n')) != NULL)
        *p='\0';
      if ((p=strchr(str, ';')) != NULL)
        *p='\0';
      for(p=str; *p && isspace(*p); p++);
      if (*str == '#')
        continue;
      if (p != str)
        strcpy(str, p);
      if (*str == '\0')
        continue;
      for (p=str+strlen(str)-1; isspace(*p); p--);
      p[1]='\0';
      p=strchr(str, '=');
      if (p == NULL)
      {
        fprintf(stderr, "Incorrect line in config: \"%s\"\n", str);
        continue;
      }
      *p++='\0';
      for (i=0; i<sizeof(params)/sizeof(*params); i++)
        if (stricmp(params[i].name, str) == 0)
          break;
      if (i == sizeof(params)/sizeof(*params))
      {
        fprintf(stderr, "Unknown param in config: \"%s\"\n", str);
        continue;
      }
      switch (params[i].type)
      {
        case PAR_COLOR:
          if (stricmp(p, "white")==0)
            *(int *)(params[i].val) = very+white;
          else if (stricmp(p, "black")==0)
            *(int *)(params[i].val) = black;
          else if (stricmp(p, "red")==0)
            *(int *)(params[i].val) = red;
          else if (stricmp(p, "green")==0)
            *(int *)(params[i].val) = green;
          else if (stricmp(p, "blue")==0)
            *(int *)(params[i].val) = blue;
          else if (stricmp(p, "sky")==0)
            *(int *)(params[i].val) = sky;
          else if (stricmp(p, "brown")==0)
            *(int *)(params[i].val) = yellow;
          else if (stricmp(p, "magenta")==0)
            *(int *)(params[i].val) = purpur;
          else if (stricmp(p, "darkgrey")==0)
            *(int *)(params[i].val) = very+black;
          else if (stricmp(p, "lightgrey")==0)
            *(int *)(params[i].val) = white;
          else if (stricmp(p, "lightred")==0)
            *(int *)(params[i].val) = very+red;
          else if (stricmp(p, "lightgreen")==0)
            *(int *)(params[i].val) = very+green;
          else if (stricmp(p, "lightblue")==0)
            *(int *)(params[i].val) = very+blue;
          else if (stricmp(p, "lightsky")==0)
            *(int *)(params[i].val) = very+sky;
          else if (stricmp(p, "yellow")==0)
            *(int *)(params[i].val) = very+yellow;
          else if (stricmp(p, "lightmagenta")==0)
            *(int *)(params[i].val) = very+purpur;
          else
            fprintf(stderr, "Unknown color %s!\n", p);
          continue;
        case PAR_PATH:
          if (stat(p, &st))
          { fprintf(stderr, "Path %s does not exists!\n", p);
            continue;
          }
          strncpy((char *)(params[i].val), p, 1024);
          continue;
        case PAR_DIG:
          *(int *)(params[i].val) = atoi(p);
          continue;
        case PAR_STRING:
          ;
      }
    }
    fclose(f);
  }
  col_menu_normal  |= col_menu_bg << 4;
  col_menu_tagged  |= col_menu_bg << 4;
  col_menu_title   |= col_menu_title_bg  << 4;
  col_menu_select  |= col_menu_select_bg << 4;
  col_play_normal  |= col_play_bg << 4;
  col_play_clicable|= col_play_bg << 4;
  col_play_volume1 |= col_play_bg << 4;
  col_play_volume2 |= col_play_bg << 4;
  col_play_volume3 |= col_play_bg << 4;
  col_play_text    |= col_play_bg << 4;
  col_play_text2   |= col_play_bg << 4;
  col_play_hilight |= col_play_bg << 4;
  col_play_hilight2|= col_play_bg << 4;
  col_play_firstlet|= col_play_bg << 4;
  col_play_ramka1  |= col_play_bg << 4;
  col_play_ramka2  |= col_play_bg << 4;
  col_play_tape    |= col_play_bg << 4;
  col_play_tapecur |= col_play_bg << 4;
  col_play_brackets|= col_play_bg << 4;
  col_edit_ramka   |= col_edit_ramka_bg << 4;
  col_edit_text    |= col_edit_text_bg << 4;

  if (startdir[0])
  { if (startdir[1] == ':')
#ifdef __OS2__
      DosSetDefaultDisk(toupper(startdir[0])-'A'+1);
#else
      _chdrive(toupper(startdir[0])-'A');
#endif
    chdir(startdir);
  }

  if (dbname[0] == '\0')
  { strcpy(dbname, playlists);
    if (strchr("/\\", dbname[strlen(dbname)-1]) == NULL)
      strcat(dbname, "/");
    strcat(dbname, "gplaydb");
  }

  return 0;
}
