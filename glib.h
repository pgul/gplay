/***************************************************
 *   Useful library                                *
 * 13.03.91  add exe_write                         *
 * 20.04.91  add scr_base
 * 20.08.91  add graphic menus
 * 04.03.92  add tsr
 ***************************************************/

int access(const char * filename,int amode); /* from io.h */
#define fileexist(name) (!access(name,0))

#define tsrOK          0
#define tsrMemAllocErr 1
#define tsrStatusLine  NULL /* to window list */

extern int xbiginkey;
extern char bios_;   /*0 for directvideo */
extern int g_tx,g_ty; /* char size in graphics*/
extern char edit_updown; extern int editptr;

extern char * statusptr;

/*   All functions was tested only for LARGE model of memory */

 /* Структуры работы с окнами (обьявлены в remenu.c) */
struct EVAKOITEM { char x_s, y_s, sx_s, sy_s;
                   short * gde_s;
       } ;
extern struct EVAKOITEM stack[15];
extern int sp;
extern int gmenu_ret;
 /* Признак наличия мышки и координаты (inkey.c) */
extern int mouse,mousexg,mouseyg;
#define gmousex mouseyg
#define gmousey mousexg
#define mousex  (mousexg>>3)
#define mousey  (mouseyg>>3)
extern int mouse_mode; /* при 0 правая кнопка мышки escc и средняя f1c */
 /* Маленькие русские в большие русские */
extern char uppertab[256];
 /* Дата ДД.ММ.ГГ и ДДММГГ,устанавливается gettoday()*/
extern char today[],today6[];
extern struct _ramka_type
       { char tl,tr,bl,br,ver,hor;} ramka_type;
extern char bottcol,scrollcol,ramka_shade;

void  gettoday(void);
 /* Проверить мышку, включить/выключить маркер (inkey.c)*/
void  initmouse(void);
void  close_mouse(void);
extern void showmouse(void);
extern void hidemouse(void);
int   askmouse(void);
void  pagemouse(int page);
void  setmouse(int x,int y);
void  onmouse(int maska);
void  setmouserep(int speed);
void  mouse_stop_rep(void);
extern char mouse_rep_param;
extern int mouseflag;
 /* Истина, если принтер готов */
int  readyprn(void);

int  menu  (int cv1,int cv2,int x,int y,int sx,int sy,int * pole,...);
 /* ... - это тройки
      char * prompt,
      char * что корректировать,
      int    тип поля */
int   mkorr (int cv1,int cv2,int x,int y,int sx,int sy,int psize,...);
void  remenu(void);
void  all_remenu(void);
void  putcol(int cv,int x,int y,int skoko);
 /* Закрасить прямоугольничек */
void  barputcol(int cv,int x,int y,int sx,int sy);
void  barfill  (int ch,int x,int y,int sx,int sy);
void  ramka    (int cv,int x,int y,int sx,int sy);
void  fill  (int ch,int x,int y,int skoko);
void  putcell (int cell,int x,int y,int skoko);
void  barcell (int cell,int x,int y,int sx,int sy);
 /* Похоже на cputc и cputs, но на порядок быстрее */
void  cchar (int x,int y,char ch);
void  ccell (int x,int y,int cell);
void  cline (int x,int y,char * line);
void  cnline(int x,int y,char * line,int skoko);
 /* Если digital не равно нулю, допустимы только цифры,
    если digital больше нуля - это позиция десятичной точки.
    Выход только по клавишам f1c,escc,enterc,upc,downc и, если
     mode  не равно 0, leftc и rightc*/
int   edit  (int x,int y,char * line,int digital,int mode);

void  addspc(char * str,int skoko_vsego);
void  stripspc(char * str);
 /* farmalloc с проверкой кода завершения */
extern void * (*myalloc)(long size);
extern void (*myfree)(void * kogo);
void * _myalloc (long size);
void * myrealloc(void * kogo,long skoko_nado);
 /* В дополнение к INT16 возвращает mouse_enter при нажатии левой кнопки мышки */
extern int (*inkey)(void);
int   _inkey(void);
int   iskey(void);
int   back_key(int ch,...);
void  back_key_frst(int key);
void  onkey(int key, void (*reaction)(void));
int   biginkey(char * l,char * sl,char * cl,char * al);
int   getshifts(void);

void  fileext(char * filename,char * extention);

char * tbez(int skoko,char * tmpline,char * in_line);
char * add(  char * a,char * b);
char * sub(  char * a,char * b);
int  gt (  char * a,char * b);
char * mul(char * res,int ressize,int drob,char *a,char * b);
char * o(char * x);

void  evakuate(int x,int y,int sx,int sy);
int   message (int x,int y,char * format,...);
void  plakat(int cv,int x,int y,char * line);

void  scrgets(int x,int y,char * s,int skoko);
void  place(int x,int y);
void  targ(int minimum_args,char * helpline);
int   chpal(char * pal,int speed);
                /* pal - это 17 или 256 троек RGB */
                /* работает только на VGA/MCGA */
                /* speed - 0..999 */
int   chpalfrom(char * from,char * to,int speed);

int   LZWpack(long skoko,
                         int (*getbyte)(void),int (*putbyte)(char));
int   LZWunpack(int (*getbyte)(void),
                          int (*putbyte)(char));
extern unsigned lzwbufsize; /* 512 .. 0(=65536), умолчание 4096 */
int   hufbldtab(long length,int (*getbyte)(void),
                          char huftab[256]);
int   hufpack(long length,int (*getbyte)(void),
                        int (*putbyte)(char),char huftab[256]);
int   hufunpack(long length_src,int (*getbyte)(void),
                          int (*putbyte)(char),char huftab[256]);

int   help_init(char * fname);
int   ovl_help_init(int ovl_num);
void  help_end(void);
void  help_(char * page);
void  help_last(void);
int   help_setpage(char * page);
void  help_getpage(char * page);
void  help_redraw (void);
int   help_print(char * filename);
void  HelpOnHelpDefFunc(void);
extern void (*HelpOnHelpFunc)(void);
extern int  helpx,helpy,helpsx,helpsy;
extern char help_cv1,help_cv3,help_cv2,help_ch1,help_ch2,help_ch3;
extern char help_cramka,help_cr1;
extern int  help_exitc,help_backc,help_backexitc;

int   gmouse_on(void);
int   gmouse_off(void);
int   gmouse_new(void); /* <=> gmouse_off(),gmouse_addold() */
int   gmouse_addold(void);
int   gmouse_offold(void);
void  gmouse_offallold(void);
void  gmouse_cancel(void);
int   mouse_ch(int  x,int y,int sx,int sy,int key);
#define tmouse_ch(x,y,sx,sy,k) mouse_ch((y)<<3,(x)<<3,((sy)<<3)-1,((sx)<<3)-1,k)

int   smenu(int cv1,int cv2,int x,int y,int sx,int sy,int * pole,int *g,
        char * helpline,
         char * (*getline)(int nomer),int maxnomer);
int   fselect (int cv1,int cv2,int cv3,
               int x,int y,int sx,int sy,
               char * path,char * wild,char * fname,
               int new);
int   quere(int col1,int col2,int x,int y,int strmes,int poley,
            int * pole,...);
int   graph(int * x,int * y,int * colors);
int   mramka(char col,int x,int y,int sx,int sy);
int   buildramka(char chem);
char  * key2str(unsigned key);
void  drawscroll(int x,int y,int h,int vsego,int tek);
int   wherescrl(void);
void  updatescrl(int tek);
void  endscroll(void);

#define REDRAW_FULL 1
#define REDRAW_HEAD 2
#define REDRAW_COL  4
#define REDRAW_TEXT 8
int   scrlmenu(int col1, int col2, int x, int y, int sx, int sy,
               char *head, int *pole, int *upline,
               int poley, char * lines[]);
int   scrlmenuf(int col1, int col2, int x, int y, int sx, int sy,
                char *head, int *pole, int *upline,
                int poley, char *(*line)(int num, int *col));

int  crsr_save(void);
void crsr_hide(void);
int  crsr_restore(void);
void crsr_show(void);
void crsr_size(int size); /* size : hight byte - first, low byte - last line */

void tsr_wait(void);
void tsr_exit(void);
void tsr_mode(int mode); /* НЕ 0 - inkey не отпускает процессор */
void inittsr(unsigned long);
void sovest(void);
int  waitkey(int key);
int  waitkeys(int key,...);

#if defined(OS2)
#ifndef _THUNK_PTR_SIZE_OK
#define _THUNK_PTR_SIZE_OK(ptr,size) \
  (((ULONG)(ptr) & ~0xffff) == (((ULONG)(ptr) + (size) - 1) & ~0xffff))
#endif
#elif defined(WIN32)
void getplace(int *x, int *y);
void initconsole(void);
#endif
