/* getopt.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"

char *_optarg          = NULL;
int _optind            = 1;              /* Default: first call             */
int _opterr            = 1;              /* Default: error messages enabled */
char *_optswchar       = "-";            /* Default: '-' starts options     */
enum _optmode _optmode = GETOPT_UNIX;
int _optopt;

static char * next_opt;             /* Next character in cluster of options */

static char done;
static char sw_char;
static char init;

static char ** options;            /* List of entries which are options     */
static char ** non_options;        /* List of entries which are not options */
static int options_count;
static int non_options_count;

#define BEGIN do {
#define END   } while (0)

#define PUT(dst) BEGIN \
                  if (_optmode == GETOPT_ANY) \
                    dst[dst##_count++] = argv[_optind]; \
                 END

/* Note: `argv' is not const as GETOPT_ANY reorders argv[]. */

int _getopt (int argc, char * argv[], const char *opt_str)
{
  char c, *q;
  int i, j;

  if (!init || _optind == 0)
    {
      if (_optind == 0) _optind = 1;
      done = 0; init = 1;
      next_opt = "";
      if (_optmode == GETOPT_ANY)
        {
          options = (char **)malloc (argc * sizeof (char *));
          non_options = (char **)malloc (argc * sizeof (char *));
          if (options == NULL || non_options == NULL)
            {
              fprintf (stderr, "out of memory in getopt()\n");
              exit (255);
            }
          options_count = 0; non_options_count = 0;
        }
    }
  if (done)
    return -1;
restart:
  _optarg = NULL;
  if (*next_opt == 0)
    {
      if (_optind >= argc)
        {
          if (_optmode == GETOPT_ANY)
            {
              j = 1;
              for (i = 0; i < options_count; ++i)
                argv[j++] = options[i];
              for (i = 0; i < non_options_count; ++i)
                argv[j++] = non_options[i];
              _optind = options_count+1;
              free (options); free (non_options);
            }
          done = 1;
          return -1;
        }
      else if (!strchr (_optswchar, argv[_optind][0]) || argv[_optind][1] == 0)
        {
          if (_optmode == GETOPT_UNIX)
            {
              done = 1;
              return -1;
            }
          PUT (non_options);
          _optarg = argv[_optind++];
          if (_optmode == GETOPT_ANY)
            goto restart;
          /* _optmode==GETOPT_KEEP */
          return 0;
        }
      else if (argv[_optind][0] == argv[_optind][1] && argv[_optind][2] == 0)
        {
          if (_optmode == GETOPT_ANY)
            {
              j = 1;
              for (i = 0; i < options_count; ++i)
                argv[j++] = options[i];
              argv[j++] = argv[_optind];
              for (i = 0; i < non_options_count; ++i)
                argv[j++] = non_options[i];
              for (i = _optind+1; i < argc; ++i)
                argv[j++] = argv[i];
              _optind = options_count+2;
              free (options); free (non_options);
            }
          ++_optind;
          done = 1;
          return -1;
        }
      else
        {
          PUT (options);
          sw_char = argv[_optind][0];
          next_opt = argv[_optind]+1;
        }
    }
  c = *next_opt++;
  if (*next_opt == 0)  /* Move to next argument if end of argument reached */
    ++_optind;
  if (c == ':' || (q = strchr (opt_str, c)) == NULL)
    {
      if (_opterr && opt_str[0] != ':')
        {
          if (c < ' ' || c >= 127)
            fprintf (stderr, "%s: invalid option; character code=0x%.2x\n",
                     argv[0], c);
          else
            fprintf (stderr, "%s: invalid option `%c%c'\n",
                     argv[0], sw_char, c);
        }
      _optopt = c;
      return '?';
    }
  if (q[1] == ':')
    {
      if (*next_opt != 0)         /* Argument given */
        {
          _optarg = next_opt;
          next_opt = "";
          ++_optind;
        }
      else if (q[2] == ':')
        _optarg = NULL;           /* Optional argument missing */
      else if (_optind >= argc)
        {                         /* Required argument missing */
          if (_opterr && opt_str[0] != ':')
            fprintf (stderr, "%s: no argument for `%c%c' option\n",
                     argv[0], sw_char, c);
          _optopt = c;
          return (opt_str[0] == ':' ? ':' : '?');
        }
      else
        {
          PUT (options);
          _optarg = argv[_optind++];
        }
    }
  return c;
}
