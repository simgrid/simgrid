/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2007-2020. The SimGrid Team.                               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/engine.h" /* simgrid_get_clock */
#include "simgrid/host.h"
#include "src/xbt/log_private.hpp"
#include "xbt/sysdep.h"
#include <algorithm>
#include <cstdio>

static constexpr const char* ERRMSG =
    "Unknown %%%c sequence in layout format (%s).\n"
    "Known sequences:\n"
    "  what:        %%m: user message  %%c: log category  %%p: log priority\n"
    "  where:\n"
    "    source:    %%F: file          %%L: line          %%M: function  %%l: location (%%F:%%L)\n"
    "    runtime:   %%h: hostname      %%t: thread        %%P: process   %%i: PID\n"
    "  when:        %%d: date          %%r: app. age\n"
    "  other:       %%%%: %%             %%n: new line      %%e: plain space\n";

#define check_overflow(len)                                             \
  if ((rem_size -= (len)) > 0) {                                        \
    p += (len);                                                         \
  } else                                                                \
    return 0

#define set_sz_from_precision()                                                                                        \
  if (true) {                                                                                                          \
    sz = rem_size;                                                                                                     \
    if (precision != -1) {                                                                                             \
      if (precision < sz)                                                                                              \
        sz = precision + 1; /* +1 for the final '\0' */                                                                \
      precision = -1;                                                                                                  \
    }                                                                                                                  \
  } else                                                                                                               \
    (void)0

#define show_it(data, letter)                                                                                          \
  if (true) {                                                                                                          \
    int len;                                                                                                           \
    int wd;                                                                                                            \
    if (length == -1) {                                                                                                \
      wd = 0;                                                                                                          \
    } else {                                                                                                           \
      wd     = length;                                                                                                 \
      length = -1;                                                                                                     \
    }                                                                                                                  \
    if (precision == -1) {                                                                                             \
      len = snprintf(p, rem_size, "%*" letter, wd, (data));                                                            \
    } else {                                                                                                           \
      len       = snprintf(p, rem_size, "%*.*" letter, wd, precision, (data));                                         \
      precision = -1;                                                                                                  \
    }                                                                                                                  \
    check_overflow(len);                                                                                               \
  } else                                                                                                               \
    (void)0

#define show_string(data)                                                                                              \
  if (true) {                                                                                                          \
    const char* show_string_data = (data);                                                                             \
    show_it(show_string_data ? show_string_data : "(null)", "s");                                                      \
  } else                                                                                                               \
    (void)0
#define show_int(data) show_it((data), "d")
#define show_double(data) show_it((data), "f")

static int xbt_log_layout_format_doit(const s_xbt_log_layout_t* l, xbt_log_event_t ev, const char* msg_fmt)
{
  char *p = ev->buffer;
  int rem_size = ev->buffer_size;
  int precision = -1;
  int length = -1;

  auto* q = static_cast<char*>(l->data);
  while (*q != '\0') {
    if (*q == '%') {
      q++;
      do {
        switch (*q) {
          case '\0':
            fprintf(stderr, "Layout format (%s) ending with %%\n", (char*)l->data);
            xbt_abort();
          case '%':
            *p = '%';
            check_overflow(1);
            break;
          case 'n': /* platform-dependant line separator; LOG4J compliant */
            *p = '\n';
            check_overflow(1);
            break;
          case 'e': /* plain space; SimGrid extension */
            *p = ' ';
            check_overflow(1);
            break;
          case '.': /* precision specifier */
            precision = static_cast<int>(strtol(q + 1, &q, 10));
            continue; /* conversion specifier still not found, continue reading */
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': /* length modifier */
            length = static_cast<int>(strtol(q, &q, 10));
            continue; /* conversion specifier still not found, continue reading */
          case 'c':   /* category name; LOG4J compliant
                         should accept a precision postfix to show the hierarchy */
            show_string(ev->cat->name);
            break;
          case 'p': /* priority name; LOG4J compliant */
            show_string(xbt_log_priority_names[ev->priority]);
            break;
          case 'h': /* host name; SimGrid extension */
            show_string(sg_host_self_get_name());
            break;
          case 't': /* thread/process name; LOG4J compliant */
          case 'P': /* process name; SimGrid extension */
            show_string(xbt_procname());
            break;
          case 'i': /* process PID name; SimGrid extension */
            show_int(xbt_getpid());
            break;
          case 'F': /* file name; LOG4J compliant */
            show_string(ev->fileName);
            break;
          case 'l': { /* location; LOG4J compliant */
            int sz;
            set_sz_from_precision();
            int len = snprintf(p, sz, "%s:%d", ev->fileName, ev->lineNum);
            check_overflow(std::min(sz, len));
            break;
          }
          case 'L': /* line number; LOG4J compliant */
            show_int(ev->lineNum);
            break;
          case 'M': /* method (ie, function) name; LOG4J compliant */
            show_string(ev->functionName);
            break;
          case 'd': /* date; LOG4J compliant */
          case 'r': /* application age; LOG4J compliant */
            show_double(simgrid_get_clock());
            break;
          case 'm': { /* user-provided message; LOG4J compliant */
            int sz;
            set_sz_from_precision();
            va_list ap;
            va_copy(ap, ev->ap);
            int len = vsnprintf(p, sz, msg_fmt, ap);
            va_end(ap);
            check_overflow(std::min(sz, len));
            break;
          }
          default:
            fprintf(stderr, ERRMSG, *q, (char*)l->data);
            xbt_abort();
        }
        break; /* done, continue normally */
      } while (true);
    } else {
      *p = *q;
      check_overflow(1);
    }
    q++;
  }
  *p = '\0';

  return 1;
}

static void xbt_log_layout_format_free(const s_xbt_log_layout_t* lay)
{
  xbt_free(lay->data);
}

xbt_log_layout_t xbt_log_layout_format_new(const char* arg)
{
  xbt_log_layout_t res = xbt_new0(s_xbt_log_layout_t, 1);
  res->do_layout       = &xbt_log_layout_format_doit;
  res->free_           = &xbt_log_layout_format_free;
  res->data            = xbt_strdup(arg);

  return res;
}
