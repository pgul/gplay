#include <string.h>
#include "glib.h"
#include <windows.h>

HANDLE hStdout = INVALID_HANDLE_VALUE;

void initconsole(void)
{
  if (hStdout == INVALID_HANDLE_VALUE)
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
}

void cline (int x,int y,char * line)
{ cnline(x, y, line, strlen(line));
}

void cnline(int x, int y, char *line, int skoko)
{ 
  DWORD written;
  int curx, cury, maxx, maxy, r;
  if (skoko>strlen(line)) skoko=strlen(line);
  if (skoko<=0) return;
  graph(&maxx, &maxy, &r);
  if (x>=maxx || y>=maxy) return;
  if (y+skoko>maxy) skoko=maxy-y;
  getplace(&curx, &cury);
  place(x, y);
  if (x==maxx-1 && y+skoko==maxy)
  { if (skoko>1)
      WriteConsole(hStdout, line, skoko-1, &written, NULL);
    fill(line[skoko-1], maxx-1, maxy-1, 1);
  } else
    WriteConsole(hStdout, line, skoko, &written, NULL);
  place(curx, cury);
}

void cchar(int x,int y,char ch)
{ cnline(x,y,&ch,1);
}

void ccell(int x, int y, int cell)
{ putcell(cell, x, y, 1);
}

void putcol(int cv, int x, int y, int skoko)
{ COORD coord;
  DWORD written;
  initconsole();
  coord.Y=x; coord.X=y;
  FillConsoleOutputAttribute(hStdout, cv, skoko, coord, &written);
}

void fill (int ch, int x, int y, int skoko)
{ COORD coord;
  DWORD written;
  initconsole();
  coord.Y=x; coord.X=y;
  FillConsoleOutputCharacter(hStdout, ch, skoko, coord, &written);
}

void putcell (int cell, int x, int y, int skoko)
{
  putcol(cell>>8, x, y, skoko);
  fill(cell & 0xff, x, y, skoko);
}

void place(int x, int y)
{ 
  COORD coord;
  coord.Y = x; coord.X = y;
  initconsole();
  SetConsoleCursorPosition(hStdout, coord);
}

void getplace(int *x, int *y)
{
  CONSOLE_SCREEN_BUFFER_INFO buf;
  initconsole();
  GetConsoleScreenBufferInfo(hStdout, &buf);
  *x = buf.dwCursorPosition.Y;
  *y = buf.dwCursorPosition.X;
}
  