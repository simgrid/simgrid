/* xbt_os_time.c -- portable interface to time-related functions            */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/xbt_os_time.h"    /* this module */
#include "xbt/log.h"
#include "portable.h"
#include <math.h>               /* floor */

#ifdef _XBT_WIN32
#include <sys/timeb.h>
#include <windows.h>
#endif

//Freebsd doesn't provide this clock_gettime flag yet, because it was added too recently (after 1993)
#ifdef __FreeBSD__
#define CLOCK_PROCESS_CPUTTIME_ID CLOCK_PROF
#endif

double xbt_os_time(void)
{
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
  gettimeofday(&tv, NULL);
#elif defined(_XBT_WIN32)
  struct timeval tv;
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  struct _timeb tm;

  _ftime(&tm);

  tv.tv_sec = tm.time;
  tv.tv_usec = tm.millitm * 1000;

#  else
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  tv.tv_sec = (long) (tm / 1000000L);
  tv.tv_usec = (long) (tm % 1000000L);
#  endif                        /* windows version checker */

#else                           /* not windows, no gettimeofday => poor resolution */
  return (double) (time(NULL));
#endif                          /* HAVE_GETTIMEOFDAY? */

  return (double) (tv.tv_sec + tv.tv_usec / 1000000.0);
}

void xbt_os_sleep(double sec)
{

#ifdef _XBT_WIN32
  Sleep((floor(sec) * 1000) + ((sec - floor(sec)) * 1000));

#elif HAVE_NANOSLEEP
  struct timespec ts;
  ts.tv_sec = sec;
  ts.tv_nsec = (sec - floor(sec)) * 1e9;
  nanosleep (&ts, NULL);
#else                           /* don't have nanosleep. Use select to sleep less than one second */
  struct timeval timeout;

  timeout.tv_sec = (unsigned long) (sec);
  timeout.tv_usec = (sec - floor(sec)) * 1000000;

  select(0, NULL, NULL, NULL, &timeout);
#endif
}

/* TSC (tick-level) timers are said to be unreliable on SMP hosts and thus
   disabled in SDL source code */


/* \defgroup XBT_sysdep All system dependency
 * \brief This section describes many macros/functions that can serve as
 *  an OS abstraction.
 */


struct s_xbt_os_timer {
#ifdef HAVE_POSIX_GETTIME
  struct timespec start, stop, elapse;
#elif defined(HAVE_GETTIMEOFDAY)
  struct timeval start, stop, elapse;
#else
  unsigned long int start, stop, elapse;
#endif
};

size_t xbt_os_timer_size(void){
  return sizeof(struct s_xbt_os_timer);
}

xbt_os_timer_t xbt_os_timer_new(void)
{
  return xbt_new0(struct s_xbt_os_timer, 1);
}

void xbt_os_timer_free(xbt_os_timer_t timer)
{
  free(timer);
}

double xbt_os_timer_elapsed(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  return ((double) timer->stop.tv_sec) - ((double) timer->start.tv_sec) +
                                          ((double) timer->elapse.tv_sec ) +
      ((((double) timer->stop.tv_nsec) -
        ((double) timer->start.tv_nsec) + ((double) timer->elapse.tv_nsec )) / 1e9);
#elif defined(HAVE_GETTIMEOFDAY) || defined(_XBT_WIN32)
  return ((double) timer->stop.tv_sec) - ((double) timer->start.tv_sec)
    + ((double) timer->elapse.tv_sec ) +
      ((((double) timer->stop.tv_usec) -
        ((double) timer->start.tv_usec) + ((double) timer->elapse.tv_usec )) / 1000000.0);
#else
  return (double) timer->stop - (double) timer->start + (double)
    timer->elapse;
#endif
}

void xbt_os_walltimer_start(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_nsec = 0;
  clock_gettime(CLOCK_REALTIME, &(timer->start));
#elif defined(HAVE_GETTIMEOFDAY)
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
  gettimeofday(&(timer->start), NULL);
#elif defined(_XBT_WIN32)
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  struct _timeb tm;

  _ftime(&tm);

  timer->start.tv_sec = tm.time;
  timer->start.tv_usec = tm.millitm * 1000;

#  else
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  timer->start.tv_sec = (long) (tm / 1000000L);
  timer->start.tv_usec = (long) (tm % 1000000L);
#  endif                        /* windows version checker */
#else
  timer->elapse = 0;
  timer->start = (unsigned long int) (time(NULL));
#endif
}

void xbt_os_walltimer_resume(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;

   timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
  clock_gettime(CLOCK_REALTIME, &(timer->start));
#elif defined(HAVE_GETTIMEOFDAY)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
  gettimeofday(&(timer->start), NULL);
#elif defined(_XBT_WIN32)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  struct _timeb tm;

  _ftime(&tm);

  timer->start.tv_sec = tm.time;
  timer->start.tv_usec = tm.millitm * 1000;

#  else
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  timer->start.tv_sec = (long) (tm / 1000000L);
  timer->start.tv_usec = (long) (tm % 1000000L);
#  endif
#else
  timer->elapse = timer->stop - timer->start;
  timer->start = (unsigned long int) (time(NULL));
#endif
}

void xbt_os_walltimer_stop(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  clock_gettime(CLOCK_REALTIME, &(timer->stop));
#elif defined(HAVE_GETTIMEOFDAY)
  gettimeofday(&(timer->stop), NULL);
#elif defined(_XBT_WIN32)
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  struct _timeb tm;

  _ftime(&tm);

  timer->stop.tv_sec = tm.time;
  timer->stop.tv_usec = tm.millitm * 1000;

#  else
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  timer->stop.tv_sec = (long) (tm / 1000000L);
  timer->stop.tv_usec = (long) (tm % 1000000L);
#  endif
#else
  timer->stop = (unsigned long int) (time(NULL));
#endif
}

void xbt_os_cputimer_start(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_nsec = 0;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(timer->start));
#elif defined(_XBT_WIN32)
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_nsec = 0;
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  THROW_UNIMPLEMENTED;
#  else
  HANDLE h = GetCurrentProcess();
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);
  unsigned __int64 ktm, utm;
  ktm = (unsigned __int64) kernelTime.dwHighDateTime << 32;
  ktm |= kernelTime.dwLowDateTime;
  ktm /= 10;
  utm = (unsigned __int64) userTime.dwHighDateTime << 32;
  utm |= userTime.dwLowDateTime;
  utm /= 10;
  timer->start.tv_sec = (long) (ktm / 1000000L) + (long) (utm / 1000000L);
  timer->start.tv_usec = (long) (ktm % 1000000L) + (long) (utm % 1000000L);
#  endif                        /* windows version checker */
#endif
}

void xbt_os_cputimer_resume(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(timer->start));
#elif defined(_XBT_WIN32)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  THROW_UNIMPLEMENTED;
#  else
  HANDLE h = GetCurrentProcess();
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);
  unsigned __int64 ktm, utm;
  ktm = (unsigned __int64) kernelTime.dwHighDateTime << 32;
  ktm |= kernelTime.dwLowDateTime;
  ktm /= 10;
  utm = (unsigned __int64) userTime.dwHighDateTime << 32;
  utm |= userTime.dwLowDateTime;
  utm /= 10;
  timer->start.tv_sec = (long) (ktm / 1000000L) + (long) (utm / 1000000L);
  timer->start.tv_usec = (long) (ktm % 1000000L) + (long) (utm % 1000000L);
#  endif                        /* windows version checker */

#endif
}

void xbt_os_cputimer_stop(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(timer->stop));
#elif defined(_XBT_WIN32)
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  THROW_UNIMPLEMENTED;
#  else
  HANDLE h = GetCurrentProcess();
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetProcessTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);
  unsigned __int64 ktm, utm;
  ktm = (unsigned __int64) kernelTime.dwHighDateTime << 32;
  ktm |= kernelTime.dwLowDateTime;
  ktm /= 10;
  utm = (unsigned __int64) userTime.dwHighDateTime << 32;
  utm |= userTime.dwLowDateTime;
  utm /= 10;
  timer->stop.tv_sec = (long) (ktm / 1000000L) + (long) (utm / 1000000L);
  timer->stop.tv_usec = (long) (ktm % 1000000L) + (long) (utm % 1000000L);
#  endif                        /* windows version checker */
#endif
}

void xbt_os_threadtimer_start(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_nsec = 0;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(timer->start));
#elif defined(_XBT_WIN32)
  struct timeval tv;
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  THROW_UNIMPLEMENTED;
#  else
  HANDLE h = GetCurrentThread();
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetThreadTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);
  unsigned __int64 ktm, utm;
  ktm = (unsigned __int64) kernelTime.dwHighDateTime << 32;
  ktm |= kernelTime.dwLowDateTime;
  ktm /= 10;
  utm = (unsigned __int64) userTime.dwHighDateTime << 32;
  utm |= userTime.dwLowDateTime;
  utm /= 10;
  timer->start.tv_sec = (long) (ktm / 1000000L) + (long) (utm / 1000000L);
  timer->start.tv_usec = (long) (ktm % 1000000L) + (long) (utm % 1000000L);
#  endif                        /* windows version checker */
#endif
}

void xbt_os_threadtimer_resume(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(timer->start));
#elif defined(_XBT_WIN32)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  THROW_UNIMPLEMENTED;
#  else
  HANDLE h = GetCurrentThread();
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetThreadTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);
  unsigned __int64 ktm, utm;
  ktm = (unsigned __int64) kernelTime.dwHighDateTime << 32;
  ktm |= kernelTime.dwLowDateTime;
  ktm /= 10;
  utm = (unsigned __int64) userTime.dwHighDateTime << 32;
  utm |= userTime.dwLowDateTime;
  utm /= 10;
  timer->start.tv_sec = (long) (ktm / 1000000L) + (long) (utm / 1000000L);
  timer->start.tv_usec = (long) (ktm % 1000000L) + (long) (utm % 1000000L);
#  endif                        /* windows version checker */
#endif
}

void xbt_os_threadtimer_stop(xbt_os_timer_t timer)
{
#ifdef HAVE_POSIX_GETTIME
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(timer->stop));
#elif defined(_XBT_WIN32)
#  if defined(WIN32_WCE) || (_WIN32_WINNT < 0x0400)
  THROW_UNIMPLEMENTED;
#  else
  HANDLE h = GetCurrentThread();
  FILETIME creationTime, exitTime, kernelTime, userTime;
  GetThreadTimes(h, &creationTime, &exitTime, &kernelTime, &userTime);
  unsigned __int64 ktm, utm;
  ktm = (unsigned __int64) kernelTime.dwHighDateTime << 32;
  ktm |= kernelTime.dwLowDateTime;
  ktm /= 10;
  utm = (unsigned __int64) userTime.dwHighDateTime << 32;
  utm |= userTime.dwLowDateTime;
  utm /= 10;
  timer->stop.tv_sec = (long) (ktm / 1000000L) + (long) (utm / 1000000L);
  timer->stop.tv_usec = (long) (ktm % 1000000L) + (long) (utm % 1000000L);
#  endif                        /* windows version checker */
#endif
}
