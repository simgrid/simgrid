/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2007-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"           /* execinfo when available */
#include "xbt/sysdep.h"
#include "xbt/strbuff.h"
#include "xbt/log_private.h"
#include "gras/virtu.h"         /* gras_os_myname (KILLME) */
#include "xbt/synchro.h"        /* xbt_thread_self_name */
#include <stdio.h>

extern const char *xbt_log_priority_names[8];

static double format_begin_of_time = -1;

#define ERRMSG "Unknown %%%c sequence in layout format (%s).\nKnown sequences:\n"                       \
  "  what:        %%m: user message  %%c: log category  %%p: log priority\n"                      \
  "  where:\n"                                                                                    \
  "    source:    %%F: file          %%L: line          %%M: function  %%l: location (%%F:%%L)\n" \
  "    runtime:   %%h: hostname      %%t: thread        %%P: process   %%i: PID\n"                \
  "    backtrace: %%b: full          %%B: short\n"                                                \
  "  when:        %%d: date          %%r: app. age\n"                                             \
  "  other:       %%%%: %%             %%n: new line      %%e: plain space\n"

#define XBT_LOG_BUFF_SIZE (ev->buffer_size)

#undef check_overflow
#define check_overflow \
  if (p - ev->buffer >= XBT_LOG_BUFF_SIZE) { /* buffer overflow */ \
    return 0;                                                      \
  } else ((void)0)

#define show_it(data,letter) \
  do { \
    if (precision == -1 && length == -1) { \
      p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%" letter, data); \
    } else if (precision == -1) { \
      sprintf(tmpfmt,"%%%d" letter,length); \
      p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), tmpfmt, data); \
      length = -1; \
    } else if (length == -1) { \
      p += sprintf(p, "%.*" letter, \
                   (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer), precision), \
                   data);\
      precision = -1; \
    } else { \
      sprintf(tmpfmt,"%%%d.%d" letter,length, \
          (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer), precision));\
      p += sprintf(p, tmpfmt, data);\
      length = precision = -1; \
    } \
    check_overflow; \
  } while (0)

#define show_string(data) show_it(data, "s")
#define show_int(data)    show_it(data, "d")
#define show_double(data) show_it(data, "f")

static int xbt_log_layout_format_doit(xbt_log_layout_t l,
                                      xbt_log_event_t ev,
                                      const char *msg_fmt)
{
  char *p, *q;
  char tmpfmt[50];
  int precision = -1;
  int length = -1;


  p = ev->buffer;
  q = l->data;

  while (*q != '\0') {
    if (*q == '%') {
      q++;
    handle_modifier:
      switch (*q) {
      case '\0':
        fprintf(stderr, "Layout format (%s) ending with %%\n",
                (char *) l->data);
        abort();
      case '%':
        *p++ = '%';
        break;
      case 'n':                /* platform-dependant line separator (LOG4J compliant) */
        p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "\n");
        check_overflow;
        break;
      case 'e':                /* plain space (SimGrid extension) */
        p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), " ");
        check_overflow;
        break;

      case '.':                /* precision specifyier */
        q++;
        sscanf(q, "%d", &precision);
        q += (precision>9?2:1);
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
      case '9': /* length modifier */
        sscanf(q, "%d", &length);
        q += (length>9?2:1);
        goto handle_modifier;

      case 'c':                /* category name; LOG4J compliant
                                   should accept a precision postfix to show the hierarchy */
        show_string(ev->cat->name);
        break;
      case 'p':                /* priority name; LOG4J compliant */
        show_string(xbt_log_priority_names[ev->priority]);
        break;

      case 'h':                /* host name; SimGrid extension */
        show_string(gras_os_myname());
        break;
      case 't':                /* thread name; LOG4J compliant */
        show_string(xbt_thread_self_name());
        break;
      case 'P':                /* process name; SimGrid extension */
        show_string(xbt_procname());
        break;
      case 'i':                /* process PID name; SimGrid extension */
        show_int((*xbt_getpid) ());
        break;

      case 'F':                /* file name; LOG4J compliant */
        show_string(ev->fileName);
        break;
      case 'l':                /* location; LOG4J compliant */
        if (precision == -1) {
          p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s:%d",
                        ev->fileName, ev->lineNum);
          check_overflow;
        } else {
          p += snprintf(p,
                        (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                                  precision), "%s:%d", ev->fileName,
                        ev->lineNum);
          check_overflow;
          precision = -1;
        }
        break;
      case 'L':                /* line number; LOG4J compliant */
        show_int(ev->lineNum);
        break;
      case 'M':                /* method (ie, function) name; LOG4J compliant */
        show_string(ev->functionName);
        break;
      case 'b':                /* backtrace; called %throwable in LOG4J */
      case 'B':                /* short backtrace; called %throwable{short} in LOG4J */
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
        {
          xbt_ex_t e;
          int i;

          e.used = backtrace((void **) e.bt, XBT_BACKTRACE_SIZE);
          e.bt_strings = NULL;
          e.msg = NULL;
          e.remote = 0;
          xbt_backtrace_current(&e);
          if (*q == 'B') {
            show_string(e.bt_strings[2] + 8);
          } else {
            for (i = 2; i < e.used; i++)
              if (precision == -1) {
                p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              "%s\n", e.bt_strings[i] + 8);
                check_overflow;
              } else {
                p += sprintf(p, "%.*s\n",
                             (int) MIN(XBT_LOG_BUFF_SIZE -
                                       (p - ev->buffer), precision),
                             e.bt_strings[i] + 8);
                check_overflow;
                precision = -1;
              }
          }

          xbt_ex_free(e);
        }
#else
        p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                      "(no backtrace on this arch)");
        check_overflow;
#endif
        break;

      case 'd':                /* date; LOG4J compliant */
        show_double(gras_os_time());
        break;
      case 'r':                /* application age; LOG4J compliant */
        show_double(gras_os_time() - format_begin_of_time);
        break;

      case 'm':                /* user-provided message; LOG4J compliant */
        if (precision == -1) {
          p += vsnprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), msg_fmt,
                         ev->ap);
          check_overflow;
        } else {
          p += vsnprintf(p,
                         (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                                   precision), msg_fmt, ev->ap);
          check_overflow;
          precision = -1;
        }
        break;

      default:
        fprintf(stderr, ERRMSG, *q, (char *) l->data);
        abort();
      }
      q++;
    } else {
      *(p++) = *(q++);
      check_overflow;
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
    format_begin_of_time = gras_os_time();

  return res;
}
