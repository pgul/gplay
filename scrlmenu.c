#include <string.h>
#include <windows.h>
#include "glib.h"
#include "keyc.h"

#define SPACESTR "                                                                                 "

static char **_lines;
int scrlmenu_redraw=REDRAW_FULL;
extern HANDLE hStdout;

void initconsole(void);

static char *line(int num, int *col)
{ return _lines[num];
}

int scrlmenu(int col1, int col2, int x, int y, int sx, int sy,
             char *head,int *pole, int *upline, int poley,
             char *lines[])
{
  _lines=lines;
  return scrlmenuf(col1, col2, x, y, sx, sy, head, pole, upline, poley, line);
}

int scrlmenuf(int col1, int col2, int x, int y, int sx, int sy,
              char *head, int *pole, int *upline, int poley,
              char *(*line)(int num, int *col))
{
  int i, j, linecol;
  char *p;
  SMALL_RECT rect;
  COORD destcoord;
  CHAR_INFO fill = {{0}, 0};

  fill.Attributes = col1;
  crsr_save();
  crsr_hide();
  initconsole();
  /* 1. Корректность upline и pole */
  if (*upline<0) *upline=0;
  if (*pole>=poley) *pole=poley-1;
  if (*pole<0) *pole=0;
  if ((*upline>poley-sx) && (*upline))
  { *upline=poley-sx;
    if (*upline<0) *upline=0;
  }
  if (*upline>*pole) *upline=*pole;
  if (*upline+sx<=*pole)
    *upline=*pole-sx+1;
  /* 2. Рисуем рамку */
  if (scrlmenu_redraw & REDRAW_FULL)
  { if (mramka(col1, x, y, sx, sy)) return -1;
    scrlmenu_redraw = REDRAW_HEAD|REDRAW_TEXT|REDRAW_COL;
  }
  else
  { putcol(col2, x+*pole-*upline+1, y+1, sy);
    if (scrlmenu_redraw & REDRAW_COL)
    {
      barputcol(col1, x+1, y+1, *pole-*upline, sy);
      barputcol(col1, x+*pole-*upline+2, y+1, sx-(*pole-*upline+1), sy);
      scrlmenu_redraw &= ~REDRAW_COL;
    }
  }
  i=sy-strlen(head);
  if (i<0) i=0;
  if (scrlmenu_redraw & REDRAW_HEAD)
  {
    cnline(x, y+1+(i>>1), head, sy);
    scrlmenu_redraw &= ~REDRAW_HEAD;
  }
  /* 3. Выбираем */
  /* Рисуем scroll-bar */
  drawscroll(x+1, y+sy+1, sx, poley, *upline);
  for (;;)
  { if (!(scrlmenu_redraw & REDRAW_TEXT))
    {
      linecol = col2;
      p=line(*pole, &linecol);
      putcol(col2, x+*pole-*upline+1, y+1, sy);
      cnline(x+*pole-*upline+1, y+1, p, sy);
      if (strlen(p)<sy)
        cnline(x+*pole-*upline+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
    }
    else
    {
      for (i=0; (i<sx) && (i<poley); i++)
      { linecol = col1;
        p=line(i+*upline, &linecol);
        cnline(x+i+1, y+1, p, sy);
        putcol(linecol, x+i+1, y+1, sy);
        if (strlen(p)<sy)
          cnline(x+i+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
      }
      putcol(col2, x+*pole-*upline+1, y+1, sy);
      for (; i<sx; i++)
        cnline(x+i+1, y+1, SPACESTR, sy);
      scrlmenu_redraw &= ~REDRAW_TEXT;
    }
    updatescrl(*upline);
    for (;;)
    { putcol(col2, x+*pole-*upline+1, y+1, sy);
donothing:
      i=inkey();
      if (getshifts() & SCROLLLOCK_ON)
        if ((i==upc)||(i==downc)||(i==supc)||(i==sdownc))
          i^=0x8000;
      switch (i)
      {
        case upc:    if (*pole>*upline)
                     { linecol = col1;
                       line(*pole, &linecol);
                       putcol(linecol, x+*pole-*upline+1, y+1, sy);
                       --*pole;
                       continue;
                     }
                     else if (*upline)
                     { --*upline;
                       --*pole;
                       linecol = col1;
                       p=line(*pole+1, &linecol);
                       // i=(linecol<<8) + ' ';
                       rect.Left=y+1;
                       rect.Top=x+1;
                       rect.Right=y+sy;
                       rect.Bottom=x+sx-1;
                       destcoord.X=y+1;
                       destcoord.Y=x+2;
                       ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                       // cnline(x+*pole-*upline+2,y+1,p,sy);
                       putcol(linecol, x+*pole-*upline+2, y+1, sy);
                       linecol = col1;
                       p=line(*pole, &linecol);
                       cnline(x+*pole-*upline+1, y+1, p, sy);
                       if (strlen(p)<sy)
                         cnline(x+*pole-*upline+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
                       updatescrl(*upline);
                       continue;
                     }
                     goto donothing;
        case downc:  
                     if ((*pole<*upline+sx-1) && (*pole<poley-1))
                     { linecol = col1;
                       line(*pole, &linecol);
                       putcol(linecol, x+*pole-*upline+1, y+1, sy);
                       ++*pole;
                       continue;
                     }
                     if (*pole<poley-1)
                     { ++*upline;
                       ++*pole;
                       linecol = col1;
                       p=line(*pole-1, &linecol);
                       // i=(linecol<<8) + ' ';
                       rect.Left=y+1;
                       rect.Top=x+2;
                       rect.Right=y+sy;
                       rect.Bottom=x+sx;
                       destcoord.X=y+1;
                       destcoord.Y=x+1;
                       ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                       // cnline(x+*pole-*upline, y+1, p, sy);
                       putcol(linecol, x+*pole-*upline, y+1, sy);
                       linecol = col1;
                       p=line(*pole, &linecol);
                       cnline(x+*pole-*upline+1, y+1, p, sy);
                       if (strlen(p)<sy)
                         cnline(x+*pole-*upline+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
                       updatescrl(*upline);
                       continue;
                     }
                     goto donothing;
        case pgupc:  if (*pole==0) goto donothing;
                     linecol = col1;
                     line(*pole, &linecol);
                     putcol(linecol, x+*pole-*upline+1, y+1, sy);
                     if (*pole!=*upline)
                     { *pole=*upline;
                       continue;
                     }
                     *pole-=sx;
                     *upline-=sx;
                     if (*upline<0) *upline=0;
                     if (*pole<0) *pole=0;
                     break;
        case pgdnc:  if (*pole>=poley-1) goto donothing;
                     linecol = col1;
                     line(*pole, &linecol);
                     putcol(linecol, x+*pole-*upline+1, y+1, sy);
                     if (*pole!=*upline+sx-1)
                     { *pole=*upline+sx-1;
                       if (*pole>=poley) *pole=poley-1;
                       continue;
                     }
                     *pole+=sx;
                     *upline+=sx;
                     if (*pole>=poley) *pole=poley-1;
                     if (*upline+sx>poley)
                     { *upline=poley-sx;
                       if (*upline<0) *upline=0;
                     }
                     break;
        case homec:  if (*pole==*upline) goto donothing;
                     linecol = col1;
                     line(*pole, &linecol);
                     putcol(linecol, x+*pole-*upline+1, y+1, sy);
                     *pole=*upline;
                     continue;
        case endc:   if (*pole==*upline+sx-1) goto donothing;
                     linecol = col1;
                     line(*pole, &linecol);
                     putcol(linecol, x+*pole-*upline+1, y+1, sy);
                     *pole=*upline+sx-1;
                     if (*pole>=poley) *pole=poley-1;
                     continue;
        case cpgupc:
        case chomec: if (*pole==0) goto donothing;
                     linecol = col1;
                     line(*pole, &linecol);
                     putcol(linecol, x+*pole-*upline+1, y+1, sy);
                     *upline=*pole=0;
                     break;
        case cpgdnc:
        case cendc:  if (*pole>=poley-1) goto donothing;
                     linecol = col1;
                     line(*pole, &linecol);
                     putcol(linecol, x+*pole-*upline+1, y+1, sy);
                     *upline=poley-sx;
                     if (*upline<0) *upline=0;
                     *pole=poley-1;
                     break;
        case supc:   
                     if (*upline==0) goto donothing;
                     --*upline;
                     --*pole;
                     linecol=col1;
                     p=line(*upline, &linecol);
                     if (*pole-*upline>1)
                     { i=(linecol<<8) + ' ';
                       rect.Left=y+1;
                       rect.Top=x+1;
                       rect.Right=y+sy;
                       rect.Bottom=x+*pole-*upline-1;
                       destcoord.X=y+1;
                       destcoord.Y=x+2;
                       ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                     }
                     else if (*pole-*upline>0)
                       putcol(linecol, x+1, y+1, sy);
                     cnline(x+1, y+1, p, sy);
                     if (strlen(p)<sy) 
                       cnline(x+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
                     if (*pole!=*upline)
                     { linecol = col1;
                       p=line(*pole, &linecol);
                       cnline(x+*pole-*upline+1, y+1, p, sy);
                       if (strlen(p)<sy) 
                         cnline(x+*pole-*upline+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
                     }
                     if (*pole-*upline<sx-1)
                     {
                       linecol=col1;
                       p=line(*pole+1, &linecol);
                       if (*pole-*upline<sx-2)
                       { i=(linecol<<8)+' ';
                         rect.Left=y+1;
                         rect.Top=x+*pole-*upline+2;
                         rect.Right=y+sy;
                         rect.Bottom=x+sx-1;
                         destcoord.X=y+1;
                         destcoord.Y=x+*pole-*upline+3;
                         ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                         cnline(x+*pole-*upline+2, y+1, p, sy);
                       }
                       else
                       { putcol(linecol, x+*pole-*upline+2, y+1, sy);
                         cnline(x+*pole-*upline+2, y+1, p, sy);
                         if (strlen(p)<sy)
                           cnline(x+*pole-*upline+2, y+1+strlen(p), SPACESTR, sy-strlen(p));
                       }
                     }
                     updatescrl(*upline);
                     goto donothing;
        case sdownc:  
                     if (*upline+sx>=poley) goto donothing;
                     ++*upline;
                     ++*pole;
                     linecol=col1;
                     p=line(*pole-1, &linecol);
                     if (*pole>*upline+1)
                     { i=(linecol<<8) + ' ';
                       rect.Left=y+1;
                       rect.Top=x+2;
                       rect.Right=y+sy;
                       rect.Bottom=x+*pole-*upline;
                       destcoord.X=y+1;
                       destcoord.Y=x+1;
                       ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                       cnline(x+*pole-*upline, y+1, p, sy);
                     }
                     else if (*pole>*upline)
                     { putcol(linecol, x+*pole-*upline, y+1, sy);
                       cnline(x+*pole-*upline, y+1, p, sy);
                       if (strlen(p)<sy)
                         cnline(x+*pole-*upline, y+1+strlen(p), SPACESTR, sy-strlen(p));
                     }
                     linecol = col1;
                     p=line(*pole, &linecol);
                     cnline(x+*pole-*upline, y+1, p, sy);
                     if (strlen(p)<sy)
                       cnline(x+*pole-*upline+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
                     if (*pole-*upline<sx-1)
                     {
                       linecol = col1;
                       p=line(*upline+sx-1, &linecol);
                       if (*pole-*upline<sx-2)
                       { i=(linecol<<8) + ' ';
                         rect.Left=y+1;
                         rect.Top=x+*pole-*upline+3;
                         rect.Right=y+sy;
                         rect.Bottom=x+sx;
                         destcoord.X=y+1;
                         destcoord.Y=x+*pole-*upline+2;
                         ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                         cnline(x+sx, y+1, p, sy);
                       }
                       else
                       { putcol(linecol, x+sx, y+1, sy);
                         cnline(x+sx, y+1, p, sy);
                         if (strlen(p)<sy)
                           cnline(x+sx, y+1+strlen(p), SPACESTR, sy-strlen(p));
                       }
                     }
                     updatescrl(*upline);
                     goto donothing;
        case scrollbarc:
                     i=*upline;
                     j=*pole;
                     *upline=wherescrl();
                     if (*upline==i) goto donothing;
                     if (*pole<*upline)
                       *pole=*upline;
                     if (*pole>=*upline+sx)
                       *pole=*upline+sx-1;
                     if (i==*upline)
                       continue;
                     if (i+1==*upline)
                     { i=(col1<<8)|' ';
                       if (*pole==j)
                       {
                         rect.Left=y+1;
                         rect.Top=x+2;
                         rect.Right=y+sy;
                         rect.Bottom=x+sx;
                         destcoord.X=y+1;
                         destcoord.Y=x+1;
                         ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                       }
                       else
                       {
                         rect.Left=y+1;
                         rect.Top=x+3;
                         rect.Right=y+sy;
                         rect.Bottom=x+sx;
                         destcoord.X=y+1;
                         destcoord.Y=x+2;
                         ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                         linecol = col1;
                         p=line(*pole, &linecol);
                         cnline(x+1, y+1, p, sy);
                         if (strlen(p)<sy)
                           cnline(x+1, y+1+strlen(p), SPACESTR, sy-strlen(p));
                       }
                       linecol = col1;
                       cnline(x+sx,y+1,line(*upline+sx-1, &linecol),sy);
                       continue;
                     }
                     else if (i-1==*upline)
                     { i=(col1<<8)|' ';
                       if (*pole==j)
                       {
                         rect.Left=y+1;
                         rect.Top=x+1;
                         rect.Right=y+sy;
                         rect.Bottom=x+sx-1;
                         destcoord.X=y+1;
                         destcoord.Y=x+2;
                         ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                       }
                       else
                       {
                         rect.Left=y+1;
                         rect.Top=x+1;
                         rect.Right=y+sy;
                         rect.Bottom=x+sx-2;
                         destcoord.X=y+1;
                         destcoord.Y=x+2;
                         ScrollConsoleScreenBuffer(hStdout, &rect, NULL, destcoord, &fill);
                         linecol = col1;
                         p=line(*pole, &linecol);
                         cnline(x+sx,y+1,p,sy);
                         if (strlen(p)<sy)
                           cnline(x+sx, y+1+strlen(p), SPACESTR, sy-strlen(p));
                       }
                       linecol = col1;
                       cnline(x+1, y+1, line(*upline, &linecol), sy);
                       continue;
                     }
                     putcol(col1, x+j-i+1, y+1, sy);
                     break;
        case mouse_enter:
                     if ((mousex>x) && (mousex<=x+sx) &&
                         (mousey>y) && (mousey<=y+sy))
                       if (*upline+mousex-x-1<poley)
                       { if (*pole==*upline+mousex-x-1)
                           goto vse;
                         else
                         { linecol = col1;
                           line(*pole, &linecol);
                           putcol(linecol, x+*pole-*upline+1, y+1, sy);
                           *pole=*upline+mousex-x-1;
                         }
                       }
                     break;
        case mouse_right:
        case mouse_middle:
                     if ((mousex>x) && (mousex<=x+sx) &&
                         (mousey>y) && (mousey<=y+sy))
                       if (*upline+mousex-x-1<poley)
                         *pole=*upline+mousex-x-1;
        default:     goto vse;
      }
      break;
    }
    scrlmenu_redraw |= REDRAW_TEXT;
  }
vse:
  endscroll();
  crsr_restore();
  scrlmenu_redraw = REDRAW_FULL;
  return i;
}
