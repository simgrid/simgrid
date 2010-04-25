/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

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

#define append1(fmt,fmt2,elm)                                         \
  do {                                                               \
    if (precision == -1) {                                        \
      xbt_strbuff_append(buff, tmp=bprintf(fmt,elm));            \
      free(tmp);                                                 \
    } else {                                                      \
      xbt_strbuff_append(buff, tmp=bprintf(fmt2,precision,elm)); \
      free(tmp);                                                 \
      precision = -1;                                            \
    }	                                                      \
  } while (0)
#define append2(fmt,elm,elm2)                                         \
  do {                                                               \
    xbt_strbuff_append(buff, tmp=bprintf(fmt,elm,elm2));          \
    free(tmp);                                                    \
    precision = -1;                                               \
  } while (0)

#define ERRMSG "Unknown %%%c sequence in layout format (%s).\nKnown sequences:\n"                       \
  "  what:        %%m: user message  %%c: log category  %%p: log priority\n"                      \
  "  where:\n"                                                                                    \
  "    source:    %%F: file          %%L: line          %%M: function  %%l: location (%%F:%%L)\n" \
  "    runtime:   %%h: hostname      %%t: thread        %%P: process   %%i: PID\n"                \
  "    backtrace: %%b: full          %%B: short\n"                                                \
  "  when:        %%d: date          %%r: app. age\n"                                             \
  "  other:       %%%%: %%             %%n: new line      %%e: plain space\n"


static void xbt_log_layout_format_dynamic(xbt_log_layout_t l,
                                          xbt_log_event_t ev,
                                          const char *fmt,
                                          xbt_log_appender_t app)
{
  xbt_strbuff_t buff = xbt_strbuff_new();
  int precision = -1;
  char *q = l->data;
  char *tmp;
  char *tmp2;
  int vres;                     /* shut gcc up, but ignored */

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
        xbt_strbuff_append(buff, "%");
        break;
      case 'n':                /* platform-dependant line separator (LOG4J compliant) */
        xbt_strbuff_append(buff, "\n");
        break;
      case 'e':                /* plain space (SimGrid extension) */
        xbt_strbuff_append(buff, " ");
        break;

      case '.':                /* precision specifyier */
        q++;
        q += sscanf(q, "%d", &precision);
        goto handle_modifier;

      case 'c':                /* category name; LOG4J compliant
                                   should accept a precision postfix to show the hierarchy */
        append1("%s", "%.*s", ev->cat->name);
        break;
      case 'p':                /* priority name; LOG4J compliant */
        append1("%s", "%.*s", xbt_log_priority_names[ev->priority]);
        break;

      case 'h':                /* host name; SimGrid extension */
        append1("%s", "%.*s", gras_os_myname());
        break;
      case 't':                /* thread name; LOG4J compliant */
        append1("%s", "%.*s", xbt_thread_self_name());
        break;
      case 'P':                /* process name; SimGrid extension */
        append1("%s", "%.*s", xbt_procname());
        break;
      case 'i':                /* process PID name; SimGrid extension */
        append1("%d", "%.*d", (*xbt_getpid) ());
        break;

      case 'F':                /* file name; LOG4J compliant */
        append1("%s", "%.*s", ev->fileName);
        break;
      case 'l':                /* location; LOG4J compliant */
        append2("%s:%d", ev->fileName, ev->lineNum);
        precision = -1;         /* Ignored */
        break;
      case 'L':                /* line number; LOG4J compliant */
        append1("%d", "%.*d", ev->lineNum);
        break;
      case 'M':                /* method (ie, function) name; LOG4J compliant */
        append1("%s", "%.*s", ev->functionName);
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
            append1("%s", "%.*s", e.bt_strings[2] + 8);
          } else {
            for (i = 2; i < e.used; i++)
              append1("%s\n", "%.*s\n", e.bt_strings[i] + 8);
          }

          xbt_ex_free(e);
        }
#else
        append1("%s", "%.*s", "(no backtrace on this arch)");
#endif
        break;

      case 'd':                /* date; LOG4J compliant */
        append1("%f", "%.*f", gras_os_time());
        break;
      case 'r':                /* application age; LOG4J compliant */
        append1("%f", "%.*f", gras_os_time() - format_begin_of_time);
        break;

      case 'm':                /* user-provided message; LOG4J compliant */
        vres = vasprintf(&tmp2, fmt, ev->ap_copy);
        append1("%s", "%.*s", tmp2);
        free(tmp2);
        break;

      default:
        fprintf(stderr, ERRMSG, *q, (char *) l->data);
        abort();
      }
      q++;
    } else {
      char tmp2[2];
      tmp2[0] = *(q++);
      tmp2[1] = '\0';
      xbt_strbuff_append(buff, tmp2);
    }
  }
  app->do_append(app, buff->data);
  xbt_strbuff_free(buff);
}

#undef check_overflow
#define check_overflow \
  if (p-ev->buffer > XBT_LOG_BUFF_SIZE) { /* buffer overflow */ \
  xbt_log_layout_format_dynamic(l,ev,msg_fmt,app); \
  return;\
  }
static void xbt_log_layout_format_doit(xbt_log_layout_t l,
                                       xbt_log_event_t ev,
                                       const char *msg_fmt,
                                       xbt_log_appender_t app)
{
  char *p, *q;
  int precision = -1;


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
        q += sscanf(q, "%d", &precision);
        goto handle_modifier;

      case 'c':                /* category name; LOG4J compliant
                                   should accept a precision postfix to show the hierarchy */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                     ev->cat->name);
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*s",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), ev->cat->name);
          check_overflow;
          precision = -1;
        }
        break;
      case 'p':                /* priority name; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                     xbt_log_priority_names[ev->priority]);
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*s",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision),
                    xbt_log_priority_names[ev->priority]);
          check_overflow;
          precision = -1;
        }
        break;

      case 'h':                /* host name; SimGrid extension */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                     gras_os_myname());
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*s",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), gras_os_myname());
          check_overflow;
          precision = -1;
        }
        break;
      case 't':                /* thread name; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                     xbt_thread_self_name());
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*s",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), xbt_thread_self_name());
          check_overflow;
          precision = -1;
        }
        break;
      case 'P':                /* process name; SimGrid extension */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                     xbt_procname());
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*s",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), xbt_procname());
          check_overflow;
          precision = -1;
        }
        break;
      case 'i':                /* process PID name; SimGrid extension */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%d",
                     (*xbt_getpid) ());
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*d",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), (*xbt_getpid) ());
          check_overflow;
          precision = -1;
        }
        break;

      case 'F':                /* file name; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                     ev->fileName);
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*s",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), ev->fileName);
          check_overflow;
          precision = -1;
        }
        break;
      case 'l':                /* location; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s:%d",
                     ev->fileName, ev->lineNum);
          check_overflow;
        } else {
          p +=
            snprintf(p,
                     (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                               precision), "%s:%d", ev->fileName,
                     ev->lineNum);
          check_overflow;
          precision = -1;
        }
        break;
      case 'L':                /* line number; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%d",
                     ev->lineNum);
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*d",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), ev->lineNum);
          check_overflow;
          precision = -1;
        }
        break;
      case 'M':                /* method (ie, function) name; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                     ev->functionName);
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*s",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), ev->functionName);
          check_overflow;
          precision = -1;
        }
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
            if (precision == -1) {
              p +=
                snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s",
                         e.bt_strings[2] + 8);
              check_overflow;
            } else {
              p +=
                sprintf(p, "%.*s",
                        (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                                  precision), e.bt_strings[2] + 8);
              check_overflow;
              precision = -1;
            }
          } else {
            for (i = 2; i < e.used; i++)
              if (precision == -1) {
                p +=
                  snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s\n",
                           e.bt_strings[i] + 8);
                check_overflow;
              } else {
                p +=
                  sprintf(p, "%.*s\n",
                          (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                                    precision), e.bt_strings[i] + 8);
                check_overflow;
                precision = -1;
              }
          }

          xbt_ex_free(e);
        }
#else
        p +=
          snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                   "(no backtrace on this arch)");
        check_overflow;
#endif
        break;

      case 'd':                /* date; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%f",
                     gras_os_time());
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*f",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), gras_os_time());
          check_overflow;
          precision = -1;
        }
        break;
      case 'r':                /* application age; LOG4J compliant */
        if (precision == -1) {
          p +=
            snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%f",
                     gras_os_time() - format_begin_of_time);
          check_overflow;
        } else {
          p +=
            sprintf(p, "%.*f",
                    (int) MIN(XBT_LOG_BUFF_SIZE - (p - ev->buffer),
                              precision), gras_os_time() - format_begin_of_time);
          check_overflow;
          precision = -1;
        }
        break;

      case 'm':                /* user-provided message; LOG4J compliant */
        if (precision == -1) {
          p +=
            vsnprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), msg_fmt,
                      ev->ap);
          check_overflow;
        } else {
          p +=
            vsnprintf(p,
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
  app->do_append(app, ev->buffer);
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
