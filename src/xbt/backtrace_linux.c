/* backtrace_linux - backtrace displaying on linux platform                 */
/* This file is included by ex.c on need (have execinfo.h, popen & addrline)*/

/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <execinfo.h>
#include <sys/stat.h>

/* This file is to be included in ex.c, so the following headers are not mandatory, but it's to make sure that eclipse see them too */
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/module.h"         /* xbt_binary_name */
#include "src/xbt_modinter.h"       /* backtrace initialization headers */
#if HAVE_MC
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif
/* end of "useless" inclusions */

extern char **environ;          /* the environment, as specified by the opengroup */

/* Module creation/destruction: nothing to do on linux */
void xbt_backtrace_preinit(void)
{
}

void xbt_backtrace_postexit(void)
{
}

#include <unwind.h>
struct trace_arg {
  void **array;
  int cnt, size;
};

static _Unwind_Reason_Code
backtrace_helper (struct _Unwind_Context *ctx, void *a)
{
  struct trace_arg *arg = a;

  /* We are first called with address in the __backtrace function.
     Skip it.  */
  if (arg->cnt != -1)
    {
      arg->array[arg->cnt] = (void *) _Unwind_GetIP(ctx);

      /* Check whether we make any progress.  */
      if (arg->cnt > 0 && arg->array[arg->cnt - 1] == arg->array[arg->cnt])
        return _URC_END_OF_STACK;
    }
  if (++arg->cnt == arg->size)
    return _URC_END_OF_STACK;
  return _URC_NO_REASON;
}

/** @brief reimplementation of glibc backtrace based directly on gcc library, without implicit malloc
 *
 * See http://webloria.loria.fr/~quinson/blog/2012/0208/system_programming_fun_in_SimGrid/
 * for the motivation behind this function
 * */

int xbt_backtrace_no_malloc(void **array, int size) {
  int i = 0;
  for(i=0; i < size; i++)
    array[i] = NULL;

  struct trace_arg arg = { .array = array, .size = size, .cnt = -1 };

  if (size >= 1)
    _Unwind_Backtrace(backtrace_helper, &arg);

  /* _Unwind_Backtrace on IA-64 seems to put NULL address above
     _start.  Fix it up here.  */
  if (arg.cnt > 1 && arg.array[arg.cnt - 1] == NULL)
    --arg.cnt;
  return arg.cnt != -1 ? arg.cnt : 0;
}

void xbt_backtrace_current(xbt_ex_t * e)
{
  e->used = backtrace((void **) e->bt, XBT_BACKTRACE_SIZE);
  if (e->used == 0) {
    fprintf(stderr, "The backtrace() function failed, which probably means that the memory is exhausted. Here is a crude dump of the exception that I was trying to build:");
    fprintf(stderr, "%s(%d) [%s:%d] %s",
            e->procname, e->pid, e->file, e->line, e->msg);
    fprintf(stderr, "Bailing out now since there is nothing I can do without a decent amount of memory. Please go fix the memleaks\n");
    exit(1);
  }
}

void xbt_ex_setup_backtrace(xbt_ex_t * e) //FIXME: This code could be greatly improved/simplifyied with http://cairo.sourcearchive.com/documentation/1.9.4/backtrace-symbols_8c-source.html
{
  int i;

  /* to get the backtrace from the libc */
  char **backtrace_syms;

  /* To build the commandline of addr2line */
  char *cmd, *curr;

  /* to extract the addresses from the backtrace */
  char **addrs;
  char buff[256];

  /* To read the output of addr2line */
  FILE *pipe;
  char line_func[1024], line_pos[1024];

  /* size (in char) of pointers on this arch */
  int addr_len = 0;

  /* To search for the right executable path when not trivial */
  struct stat stat_buf;
  char *binary_name = NULL;

  xbt_assert(e, "Backtrace not setup yet, cannot set it up for display");

  e->bt_strings = NULL;

  if (xbt_binary_name == NULL) /* no binary name, nothing to do */
    return;

  if (e->used <= 1)
    return;

  /* ignore first one, which is xbt_backtrace_current() */
  e->used--;
  memmove(e->bt, e->bt + 1, (sizeof *e->bt) * e->used);

  backtrace_syms = backtrace_symbols(e->bt, e->used);

  /* build the commandline */
  if (stat(xbt_binary_name, &stat_buf)) {
    /* Damn. binary not in current dir. We'll have to dig the PATH to find it */
    for (i = 0; environ[i]; i++) {
      if (!strncmp("PATH=", environ[i], 5)) {
        xbt_dynar_t path = xbt_str_split(environ[i] + 5, ":");
        unsigned int cpt;
        char *data;

        xbt_dynar_foreach(path, cpt, data) {
          free(binary_name);
          binary_name = bprintf("%s/%s", data, xbt_binary_name);
          if (!stat(binary_name, &stat_buf)) {
            /* Found. */
            XBT_DEBUG("Looked in the PATH for the binary. Found %s", binary_name);
            break;
          }
        }
        xbt_dynar_free(&path);
        if (stat(binary_name, &stat_buf)) {
          /* not found */
          e->used = 1;
          e->bt_strings = xbt_new(char *, 1);

          e->bt_strings[0] = bprintf("(binary '%s' not found in the PATH)", xbt_binary_name);
          free(backtrace_syms);
          return;
        }
        break;
      }
    }
  } else {
    binary_name = xbt_strdup(xbt_binary_name);
  }
  cmd = curr = xbt_new(char, strlen(ADDR2LINE) + 25 + strlen(binary_name) + 32 * e->used);

  curr += sprintf(curr, "%s -f -e %s ", ADDR2LINE, binary_name);
  free(binary_name);

  addrs = xbt_new(char *, e->used);
  for (i = 0; i < e->used; i++) {
    char *p;
    /* retrieve this address */
    XBT_DEBUG("Retrieving address number %d from '%s'", i, backtrace_syms[i]);
    snprintf(buff, 256, "%s", strchr(backtrace_syms[i], '[') + 1);
    p = strchr(buff, ']');
    *p = '\0';
    if (strcmp(buff, "(nil)"))
      addrs[i] = xbt_strdup(buff);
    else
      addrs[i] = xbt_strdup("0x0");
    XBT_DEBUG("Set up a new address: %d, '%s'(%p)", i, addrs[i], addrs[i]);

    /* Add it to the command line args */
    curr += sprintf(curr, "%s ", addrs[i]);
  }
  addr_len = strlen(addrs[0]);

  /* parse the output and build a new backtrace */
  e->bt_strings = xbt_new(char *, e->used);

  XBT_VERB("Fire a first command: '%s'", cmd);
  pipe = popen(cmd, "r");
  if (!pipe) {
    xbt_die("Cannot fork addr2line to display the backtrace");
  }

  for (i = 0; i < e->used; i++) {
    XBT_DEBUG("Looking for symbol %d, addr = '%s'", i, addrs[i]);
    if (fgets(line_func, 1024, pipe)) {
      line_func[strlen(line_func) - 1] = '\0';
    } else {
      XBT_VERB("Cannot run fgets to look for symbol %d, addr %s", i, addrs[i]);
      strcpy(line_func, "???");
    }
    if (fgets(line_pos, 1024, pipe)) {
      line_pos[strlen(line_pos) - 1] = '\0';
    } else {
      XBT_VERB("Cannot run fgets to look for symbol %d, addr %s", i, addrs[i]);
      strcpy(line_pos, backtrace_syms[i]);
    }

    if (strcmp("??", line_func) != 0) {
      XBT_DEBUG("Found static symbol %s() at %s", line_func, line_pos);
      e->bt_strings[i] =
          bprintf("**   In %s() at %s", line_func, line_pos);
    } else {
      /* Damn. The symbol is in a dynamic library. Let's get wild */
      char *maps_name;
      FILE *maps;
      char maps_buff[512];

      long int addr, offset = 0;
      char *p, *p2;

      char *subcmd;
      FILE *subpipe;
      int found = 0;

      /* let's look for the offset of this library in our addressing space */
      maps_name = bprintf("/proc/%d/maps", (int) getpid());
      maps = fopen(maps_name, "r");

      addr = strtol(addrs[i], &p, 16);
      if (*p != '\0') {
        XBT_CRITICAL("Cannot parse backtrace address '%s' (addr=%#lx)", addrs[i], addr);
      }
      XBT_DEBUG("addr=%s (as string) =%#lx (as number)", addrs[i], addr);

      while (!found) {
        long int first, last;

        if (fgets(maps_buff, 512, maps) == NULL)
          break;
        if (i == 0) {
          maps_buff[strlen(maps_buff) - 1] = '\0';
          XBT_DEBUG("map line: %s", maps_buff);
        }
        sscanf(maps_buff, "%lx", &first);
        p = strchr(maps_buff, '-') + 1;
        sscanf(p, "%lx", &last);
        if (first < addr && addr < last) {
          offset = first;
          found = 1;
        }
        if (found) {
          XBT_DEBUG("%#lx in [%#lx-%#lx]", addr, first, last);
          XBT_DEBUG("Symbol found, map lines not further displayed (even if looking for next ones)");
        }
      }
      fclose(maps);
      free(maps_name);
      free(addrs[i]);

      if (!found) {
        XBT_VERB("Problem while reading the maps file. Following backtrace will be mangled.");
        XBT_DEBUG("No dynamic. Static symbol: %s", backtrace_syms[i]);
        e->bt_strings[i] = bprintf("**   In ?? (%s)", backtrace_syms[i]);
        continue;
      }

      /* Ok, Found the offset of the maps line containing the searched symbol.
         We now need to substract this from the address we got from backtrace.
       */

      addrs[i] = bprintf("0x%0*lx", addr_len - 2, addr - offset);
      XBT_DEBUG("offset=%#lx new addr=%s", offset, addrs[i]);

      /* Got it. We have our new address. Let's get the library path and we are set */
      p = xbt_strdup(backtrace_syms[i]);
      if (p[0] == '[') {
        /* library path not displayed in the map file either... */
        free(p);
        sprintf(line_func, "??");
      } else {
        p2 = strrchr(p, '(');
        if (p2)
          *p2 = '\0';
        p2 = strrchr(p, ' ');
        if (p2)
          *p2 = '\0';

        /* Here we go, fire an addr2line up */
        subcmd = bprintf("%s -f -e %s %s", ADDR2LINE, p, addrs[i]);
        free(p);
        XBT_VERB("Fire a new command: '%s'", subcmd);
        subpipe = popen(subcmd, "r");
        if (!subpipe) {
          xbt_die("Cannot fork addr2line to display the backtrace");
        }
        if (fgets(line_func, 1024, subpipe)) {
          line_func[strlen(line_func) - 1] = '\0';
        } else {
          XBT_VERB("Cannot read result of subcommand %s", subcmd);
          strcpy(line_func, "???");
        }
        if (fgets(line_pos, 1024, subpipe)) {
          line_pos[strlen(line_pos) - 1] = '\0';
        } else {
          XBT_VERB("Cannot read result of subcommand %s", subcmd);
          strcpy(line_pos, backtrace_syms[i]);
        }
        pclose(subpipe);
        free(subcmd);
      }

      /* check whether the trick worked */
      if (strcmp("??", line_func)) {
        XBT_DEBUG("Found dynamic symbol %s() at %s", line_func, line_pos);
        e->bt_strings[i] = bprintf("**   In %s() at %s", line_func, line_pos);
      } else {
        /* damn, nothing to do here. Let's print the raw address */
        XBT_DEBUG("Dynamic symbol not found. Raw address = %s", backtrace_syms[i]);
        e->bt_strings[i] = bprintf("**   In ?? at %s", backtrace_syms[i]);
      }
    }
    free(addrs[i]);

    /* Mask the bottom of the stack */
    if (!strncmp("main", line_func, strlen("main")) ||
        !strncmp("xbt_thread_context_wrapper", line_func, strlen("xbt_thread_context_wrapper"))
        || !strncmp("smx_ctx_sysv_wrapper", line_func, strlen("smx_ctx_sysv_wrapper"))) {
      int j;

      for (j = i + 1; j < e->used; j++)
        free(addrs[j]);
      e->used = i + 1;

      if (!strncmp
          ("xbt_thread_context_wrapper", line_func, strlen("xbt_thread_context_wrapper"))) {
        free(e->bt_strings[i]);
        e->bt_strings[i] = xbt_strdup("**   (in a separate thread)");
      }
    }
  }
  pclose(pipe);
  free(addrs);
  free(backtrace_syms);
  free(cmd);
}

#if HAVE_MC
int xbt_libunwind_backtrace(void* bt[XBT_BACKTRACE_SIZE], int size){
  int i = 0;
  for(i=0; i < size; i++)
    bt[i] = NULL;

  i=0;

  unw_cursor_t c;
  unw_context_t uc;

  unw_getcontext (&uc);
  unw_init_local (&c, &uc);

  unw_word_t ip;

  unw_step(&c);

  while(unw_step(&c) >= 0 && i < size){
    unw_get_reg(&c, UNW_REG_IP, &ip);
    bt[i] = (void*)(long)ip;
    i++;
  }

  return i;
}
#endif
