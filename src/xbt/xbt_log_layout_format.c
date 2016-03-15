/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"       /* execinfo when available */
#include "xbt/synchro_core.h"   /* xbt_thread_self_name */
#include "src/xbt/ex_interface.h"
#include "xbt/sysdep.h"
#include "xbt/strbuff.h"
#include "src/xbt/log_private.h"
#include "simgrid/simix.h"      /* SIMIX_host_self_get_name */
#include "surf/surf.h"
#include <stdio.h>

#if HAVE_EXECINFO_H
#  include <execinfo.h> /* Function backtrace */
#endif

extern const char *xbt_log_priority_names[8];

static double format_begin_of_time = -1;

#define ERRMSG                                                          \
  "Unknown %%%c sequence in layout format (%s).\n"                      \
  "Known sequences:\n"                                                  \
  "  what:        %%m: user message  %%c: log category  %%p: log priority\n" \
  "  where:\n"                                                          \
  "    source:    %%F: file          %%L: line          %%M: function  %%l: location (%%F:%%L)\n" \
  "    runtime:   %%h: hostname      %%t: thread        %%P: process   %%i: PID\n" \
  "    backtrace: %%b: full          %%B: short\n"                      \
  "  when:        %%d: date          %%r: app. age\n"                   \
  "  other:       %%%%: %%             %%n: new line      %%e: plain space\n"

#define check_overflow(len)                                             \
  if ((rem_size -= (len)) > 0) {                                        \
    p += (len);                                                         \
  } else                                                                \
    return 0

#define set_sz_from_precision()                                         \
  if (1) {                                                              \
    sz = rem_size;                                                      \
    if (precision != -1) {                                              \
      if (precision < sz)                                               \
        sz = precision + 1;     /* +1 for the final '\0' */             \
      precision = -1;                                                   \
    }                                                                   \
  } else (void)0

#define show_it(data, letter)                                           \
  if (1) {                                                              \
    int len, wd;                                                        \
    if (length == -1) {                                                 \
      wd = 0;                                                           \
    } else {                                                            \
      wd = length;                                                      \
      length = -1;                                                      \
    }                                                                   \
    if (precision == -1) {                                              \
      len = snprintf(p, rem_size, "%*" letter, wd, data);               \
    } else {                                                            \
      len = snprintf(p, rem_size, "%*.*" letter, wd, precision, data);  \
      precision = -1;                                                   \
    }                                                                   \
    check_overflow(len);                                                \
  } else (void)0

#define show_string(data)                                               \
  if (1) {                                                              \
    const char *show_string_data = (data);                              \
    show_it(show_string_data ? show_string_data : "(null)", "s");       \
  } else (void)0
#define show_int(data)    show_it(data, "d")
#define show_double(data) show_it(data, "f")

static int xbt_log_layout_format_doit(xbt_log_layout_t l, xbt_log_event_t ev, const char *msg_fmt)
{
  char *p = ev->buffer;
  int rem_size = ev->buffer_size;
  int precision = -1;
  int length = -1;
  char *q;

  for (q = l->data ; *q != '\0' ; q++) {
    if (*q == '%') {
      q++;
    handle_modifier:
      switch (*q) {
      case '\0':
        fprintf(stderr, "Layout format (%s) ending with %%\n", (char *)l->data);
        xbt_abort();
      case '%':
        *p = '%';
        check_overflow(1);
        break;
      case 'n':         /* platform-dependant line separator; LOG4J compliant */
        *p = '\n';
        check_overflow(1);
        break;
      case 'e':                 /* plain space; SimGrid extension */
        *p = ' ';
        check_overflow(1);
        break;
      case '.':                 /* precision specifier */
        precision = strtol(q + 1, &q, 10);
        goto handle_modifier;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':                 /* length modifier */
        length = strtol(q, &q, 10);
        goto handle_modifier;
      case 'c':                 /* category name; LOG4J compliant
                                   should accept a precision postfix to show the hierarchy */
        show_string(ev->cat->name);
        break;
      case 'p':                 /* priority name; LOG4J compliant */
        show_string(xbt_log_priority_names[ev->priority]);
        break;
      case 'h':                 /* host name; SimGrid extension */
        show_string(SIMIX_host_self_get_name());
        break;
      case 't':                 /* thread name; LOG4J compliant */
        show_string(SIMIX_process_self_get_name());
        break;
      case 'P':                 /* process name; SimGrid extension */
        show_string(xbt_procname());
        break;
      case 'i':                 /* process PID name; SimGrid extension */
        show_int(xbt_getpid());
        break;
      case 'F':                 /* file name; LOG4J compliant */
        show_string(ev->fileName);
        break;
      case 'l': {               /* location; LOG4J compliant */
        int len, sz;
        set_sz_from_precision();
        len = snprintf(p, sz, "%s:%d", ev->fileName, ev->lineNum);
        check_overflow(MIN(sz, len));
        break;
      }
      case 'L':                 /* line number; LOG4J compliant */
        show_int(ev->lineNum);
        break;
      case 'M':                /* method (ie, function) name; LOG4J compliant */
        show_string(ev->functionName);
        break;
      case 'b':                 /* backtrace; called %throwable in LOG4J */
      case 'B':         /* short backtrace; called %throwable{short} in LOG4J */
#if HAVE_EXECINFO_H && HAVE_POPEN && defined(ADDR2LINE)
        {
          xbt_ex_t e;

          e.used = backtrace((void **) e.bt, XBT_BACKTRACE_SIZE);
          e.bt_strings = NULL;
          e.msg = NULL;
          xbt_ex_setup_backtrace(&e);
          if (*q == 'B') {
            show_string(e.bt_strings[1] + 8);
          } else {
            xbt_strbuff_t buff = xbt_strbuff_new();
            int i;
            xbt_strbuff_append(buff, e.bt_strings[1] + 8);
            for (i = 2; i < e.used; i++) {
              xbt_strbuff_append(buff, "\n");
              xbt_strbuff_append(buff, e.bt_strings[i] + 8);
            }
            show_string(buff->data);
            xbt_strbuff_free(buff);
          }
          xbt_ex_free(e);
        }
#else
        show_string("(no backtrace on this arch)");
#endif
        break;
      case 'd':                 /* date; LOG4J compliant */
        show_double(surf_get_clock());
        break;
      case 'r':                 /* application age; LOG4J compliant */
        show_double(surf_get_clock() - format_begin_of_time);
        break;
      case 'm': {               /* user-provided message; LOG4J compliant */
        int len, sz;
        set_sz_from_precision();
        len = vsnprintf(p, sz, msg_fmt, ev->ap);
        check_overflow(MIN(sz, len));
        break;
      }
      default:
        fprintf(stderr, ERRMSG, *q, (char *)l->data);
        xbt_abort();
      }
    } else {
      *p = *q;
      check_overflow(1);
    }
  }
  *p = '\0';

  return 1;
}

static void xbt_log_layout_format_free(xbt_log_layout_t lay)
{
  free(lay->data);
}

xbt_log_layout_t xbt_log_layout_format_new(char *arg)
{
  xbt_log_layout_t res = xbt_new0(s_xbt_log_layout_t, 1);
  res->do_layout = xbt_log_layout_format_doit;
  res->free_ = xbt_log_layout_format_free;
  res->data = xbt_strdup((char *) arg);

  if (format_begin_of_time < 0)
    format_begin_of_time = surf_get_clock();

  return res;
}
