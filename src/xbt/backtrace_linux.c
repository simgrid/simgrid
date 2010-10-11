/* backtrace_linux - backtrace displaying on linux platform                 */
/* This file is included by ex.c on need (have execinfo.h, popen & addrline)*/

/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This file is to be included in ex.c, so the following headers are not mandatory, but it's to make sure that eclipse see them too */
#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/module.h"         /* xbt_binary_name */
#include "xbt_modinter.h"       /* backtrace initialization headers */
/* end of "useless" inclusions */

extern char **environ;          /* the environment, as specified by the opengroup */

/* Module creation/destruction: nothing to do on linux */
void xbt_backtrace_preinit(void)
{
}

void xbt_backtrace_postexit(void)
{
}

void xbt_backtrace_current(xbt_ex_t * e)
{
  e->used = backtrace((void **) e->bt, XBT_BACKTRACE_SIZE);
}


void xbt_ex_setup_backtrace(xbt_ex_t * e)
{
  int i;

  /* to get the backtrace from the libc */
  char **backtrace_syms;

  /* To build the commandline of addr2line */
  char *cmd, *curr;

  /* to extract the addresses from the backtrace */
  char **addrs;
  char buff[256], *p;

  /* To read the output of addr2line */
  FILE *pipe;
  char line_func[1024], line_pos[1024];

  /* size (in char) of pointers on this arch */
  int addr_len = 0;

  /* To search for the right executable path when not trivial */
  struct stat stat_buf;
  char *binary_name = NULL;

  xbt_assert0(e
              && e->used,
              "Backtrace not setup yet, cannot set it up for display");

  backtrace_syms = backtrace_symbols(e->bt, e->used);
  /* ignore first one, which is this xbt_backtrace_current() */
  e->used--;
  memmove(backtrace_syms, backtrace_syms + 1, sizeof(char *) * e->used);
  addrs = xbt_new(char *, e->used);

  e->bt_strings = NULL;

  /* Some arches only have stubs of backtrace, no implementation (hppa comes to mind) */
  if (!e->used)
    return;

  /* build the commandline */
  if (stat(xbt_binary_name, &stat_buf)) {
    /* Damn. binary not in current dir. We'll have to dig the PATH to find it */
    int i;

    for (i = 0; environ[i]; i++) {
      if (!strncmp("PATH=", environ[i], 5)) {
        xbt_dynar_t path = xbt_str_split(environ[i] + 5, ":");
        unsigned int cpt;
        char *data;

        xbt_dynar_foreach(path, cpt, data) {
          if (binary_name)
            free(binary_name);
          binary_name = bprintf("%s/%s", data, xbt_binary_name);
          if (!stat(binary_name, &stat_buf)) {
            /* Found. */
            DEBUG1("Looked in the PATH for the binary. Found %s",
                   binary_name);
            xbt_dynar_free(&path);
            break;
          }
        }
        if (stat(binary_name, &stat_buf)) {
          /* not found */
          e->used = 1;
          e->bt_strings = xbt_new(char *, 1);

          e->bt_strings[0] =
              bprintf("(binary '%s' not found the path)", xbt_binary_name);
          return;
        }
        xbt_dynar_free(&path);
        break;
      }
    }
  } else {
    binary_name = xbt_strdup(xbt_binary_name);
  }
  cmd = curr =
      xbt_new(char,
              strlen(ADDR2LINE) + 25 + strlen(binary_name) + 32 * e->used);

  curr += sprintf(curr, "%s -f -e %s ", ADDR2LINE, binary_name);
  free(binary_name);

  for (i = 0; i < e->used; i++) {
    /* retrieve this address */
    DEBUG2("Retrieving address number %d from '%s'", i, backtrace_syms[i]);
    snprintf(buff, 256, "%s", strchr(backtrace_syms[i], '[') + 1);
    p = strchr(buff, ']');
    *p = '\0';
    if (strcmp(buff, "(nil)"))
      addrs[i] = bprintf("%s", buff);
    else
      addrs[i] = bprintf("0x0");
    DEBUG3("Set up a new address: %d, '%s'(%p)", i, addrs[i], addrs[i]);

    /* Add it to the command line args */
    curr += sprintf(curr, "%s ", addrs[i]);
  }
  addr_len = strlen(addrs[0]);

  /* parse the output and build a new backtrace */
  e->bt_strings = xbt_new(char *, e->used);

  VERB1("Fire a first command: '%s'", cmd);
  pipe = popen(cmd, "r");
  if (!pipe) {
    CRITICAL0("Cannot fork addr2line to display the backtrace");
    abort();
  }

  for (i = 0; i < e->used; i++) {
    char *fgets_res;
    DEBUG2("Looking for symbol %d, addr = '%s'", i, addrs[i]);
    fgets_res = fgets(line_func, 1024, pipe);
    if (fgets_res == NULL)
      THROW2(system_error, 0,
             "Cannot run fgets to look for symbol %d, addr %s", i,
             addrs[i]);
    line_func[strlen(line_func) - 1] = '\0';
    fgets_res = fgets(line_pos, 1024, pipe);
    if (fgets_res == NULL)
      THROW2(system_error, 0,
             "Cannot run fgets to look for symbol %d, addr %s", i,
             addrs[i]);
    line_pos[strlen(line_pos) - 1] = '\0';

    if (strcmp("??", line_func)) {
      DEBUG2("Found static symbol %s() at %s", line_func, line_pos);
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

      sscanf(addrs[i], "%lx", &addr);
      sprintf(maps_buff, "%#lx", addr);

      if (strcmp(addrs[i], maps_buff)) {
        CRITICAL2("Cannot parse backtrace address '%s' (addr=%#lx)",
                  addrs[i], addr);
      }
      DEBUG2("addr=%s (as string) =%#lx (as number)", addrs[i], addr);

      while (!found) {
        long int first, last;

        if (fgets(maps_buff, 512, maps) == NULL)
          break;
        if (i == 0) {
          maps_buff[strlen(maps_buff) - 1] = '\0';
          DEBUG1("map line: %s", maps_buff);
        }
        sscanf(maps_buff, "%lx", &first);
        p = strchr(maps_buff, '-') + 1;
        sscanf(p, "%lx", &last);
        if (first < addr && addr < last) {
          offset = first;
          found = 1;
        }
        if (found) {
          DEBUG3("%#lx in [%#lx-%#lx]", addr, first, last);
          DEBUG0
              ("Symbol found, map lines not further displayed (even if looking for next ones)");
        }
      }
      fclose(maps);
      free(maps_name);

      if (!found) {
        VERB0
            ("Problem while reading the maps file. Following backtrace will be mangled.");
        DEBUG1("No dynamic. Static symbol: %s", backtrace_syms[i]);
        e->bt_strings[i] = bprintf("**   In ?? (%s)", backtrace_syms[i]);
        continue;
      }

      /* Ok, Found the offset of the maps line containing the searched symbol.
         We now need to substract this from the address we got from backtrace.
       */

      free(addrs[i]);
      addrs[i] = bprintf("0x%0*lx", addr_len - 2, addr - offset);
      DEBUG2("offset=%#lx new addr=%s", offset, addrs[i]);

      /* Got it. We have our new address. Let's get the library path and we
         are set */
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
        VERB1("Fire a new command: '%s'", subcmd);
        subpipe = popen(subcmd, "r");
        if (!subpipe) {
          CRITICAL0("Cannot fork addr2line to display the backtrace");
          abort();
        }
        fgets_res = fgets(line_func, 1024, subpipe);
        if (fgets_res == NULL)
          THROW1(system_error, 0, "Cannot read result of subcommand %s",
                 subcmd);
        line_func[strlen(line_func) - 1] = '\0';
        fgets_res = fgets(line_pos, 1024, subpipe);
        if (fgets_res == NULL)
          THROW1(system_error, 0, "Cannot read result of subcommand %s",
                 subcmd);
        line_pos[strlen(line_pos) - 1] = '\0';
        pclose(subpipe);
        free(subcmd);
      }

      /* check whether the trick worked */
      if (strcmp("??", line_func)) {
        DEBUG2("Found dynamic symbol %s() at %s", line_func, line_pos);
        e->bt_strings[i] =
            bprintf("**   In %s() at %s", line_func, line_pos);
      } else {
        /* damn, nothing to do here. Let's print the raw address */
        DEBUG1("Dynamic symbol not found. Raw address = %s",
               backtrace_syms[i]);
        e->bt_strings[i] = bprintf("**   In ?? at %s", backtrace_syms[i]);
      }

    }
    free(addrs[i]);

    /* Mask the bottom of the stack */
    if (!strncmp("main", line_func, strlen("main")) ||
        !strncmp("xbt_thread_context_wrapper", line_func,
                 strlen("xbt_thread_context_wrapper"))
        || !strncmp("smx_ctx_sysv_wrapper", line_func,
                    strlen("smx_ctx_sysv_wrapper"))) {
      int j;

      for (j = i + 1; j < e->used; j++)
        free(addrs[j]);
      e->used = i;

      if (!strncmp
          ("xbt_thread_context_wrapper", line_func,
           strlen("xbt_thread_context_wrapper"))) {
        e->used++;
        e->bt_strings[i] = bprintf("**   (in a separate thread)");
      }
    }


  }
  pclose(pipe);
  free(addrs);
  free(backtrace_syms);
  free(cmd);
}
