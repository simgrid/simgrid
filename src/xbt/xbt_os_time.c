/* xbt_os_time.c -- portable interface to time-related functions            */

/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/xbt_os_time.h"    /* this module */
#include <math.h>               /* floor */
#include <sys/time.h>
#include <time.h>

#ifdef _WIN32
#include <sys/timeb.h>
#include <windows.h>
#endif

//Freebsd doesn't provide this clock_gettime flag yet, because it was added too recently (after 1993)
#if defined (CLOCK_PROF) && ! defined (CLOCK_PROCESS_CPUTIME_ID)
#define CLOCK_PROCESS_CPUTIME_ID CLOCK_PROF
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/mach_port.h>
#include <mach/mach_traps.h>
#include <mach/task_info.h>
#include <mach/thread_info.h>
#include <mach/thread_act.h>
#include <mach/vm_region.h>
#include <mach/vm_map.h>
#include <mach/task.h>
#endif

double xbt_os_time(void)
{
#if HAVE_GETTIMEOFDAY
  struct timeval tv;
  gettimeofday(&tv, NULL);
#elif defined(_WIN32)
  struct timeval tv;
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  tv.tv_sec = (long) (tm / 1000000L);
  tv.tv_usec = (long) (tm % 1000000L);

#else                           /* not windows, no gettimeofday => poor resolution */
  return (double) (time(NULL));
#endif                          /* HAVE_GETTIMEOFDAY? */

  return (double) (tv.tv_sec + tv.tv_usec / 1000000.0);
}

void xbt_os_sleep(double sec)
{

#ifdef _WIN32
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

/* TSC (tick-level) timers are said to be unreliable on SMP hosts and thus  disabled in SDL source code */

/* \defgroup XBT_sysdep All system dependency
 * \brief This section describes many macros/functions that can serve as  an OS abstraction.
 */
struct s_xbt_os_timer {
#if HAVE_POSIX_GETTIME
  struct timespec start, stop, elapse;
#elif HAVE_GETTIMEOFDAY || defined(_WIN32)
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
#if HAVE_POSIX_GETTIME
  return ((double) timer->stop.tv_sec) - ((double) timer->start.tv_sec) + ((double) timer->elapse.tv_sec ) +
      ((((double) timer->stop.tv_nsec) - ((double) timer->start.tv_nsec) + ((double) timer->elapse.tv_nsec )) / 1e9);
#elif HAVE_GETTIMEOFDAY || defined(_WIN32)
  return ((double) timer->stop.tv_sec) - ((double) timer->start.tv_sec) + ((double) timer->elapse.tv_sec ) +
      ((((double) timer->stop.tv_usec) - ((double) timer->start.tv_usec) + ((double) timer->elapse.tv_usec )) /
         1000000.0);
#else
  return (double) timer->stop - (double) timer->start + (double) timer->elapse;
#endif
}

void xbt_os_walltimer_start(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_nsec = 0;
  clock_gettime(CLOCK_REALTIME, &(timer->start));
#elif HAVE_GETTIMEOFDAY
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
  gettimeofday(&(timer->start), NULL);
#elif defined(_WIN32)
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  timer->start.tv_sec = (long) (tm / 1000000L);
  timer->start.tv_usec = (long) (tm % 1000000L);
#else
  timer->elapse = 0;
  timer->start = (unsigned long int) (time(NULL));
#endif
}

void xbt_os_walltimer_resume(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;

   timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
  clock_gettime(CLOCK_REALTIME, &(timer->start));
#elif HAVE_GETTIMEOFDAY
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
  gettimeofday(&(timer->start), NULL);
#elif defined(_WIN32)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  timer->start.tv_sec = (long) (tm / 1000000L);
  timer->start.tv_usec = (long) (tm % 1000000L);
#else
  timer->elapse = timer->stop - timer->start;
  timer->start = (unsigned long int) (time(NULL));
#endif
}

void xbt_os_walltimer_stop(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  clock_gettime(CLOCK_REALTIME, &(timer->stop));
#elif HAVE_GETTIMEOFDAY
  gettimeofday(&(timer->stop), NULL);
#elif defined(_WIN32)
  FILETIME ft;
  unsigned __int64 tm;

  GetSystemTimeAsFileTime(&ft);
  tm = (unsigned __int64) ft.dwHighDateTime << 32;
  tm |= ft.dwLowDateTime;
  tm /= 10;
  tm -= 11644473600000000ULL;

  timer->stop.tv_sec = (long) (tm / 1000000L);
  timer->stop.tv_usec = (long) (tm % 1000000L);
#else
  timer->stop = (unsigned long int) (time(NULL));
#endif
}

void xbt_os_cputimer_start(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_nsec = 0;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(timer->start));
#elif HAVE_GETTIMEOFDAY //return time and not cputime in this case
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
  gettimeofday(&(timer->start), NULL);
#elif defined(_WIN32)
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
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
#endif
}

void xbt_os_cputimer_resume(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(timer->start));
#elif HAVE_GETTIMEOFDAY
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
  gettimeofday(&(timer->start), NULL);
#elif defined(_WIN32)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
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
#endif
}

void xbt_os_cputimer_stop(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(timer->stop));
#elif HAVE_GETTIMEOFDAY
  gettimeofday(&(timer->stop), NULL);
#elif defined(_WIN32)
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
#endif
}

void xbt_os_threadtimer_start(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_nsec = 0;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(timer->start));
#elif HAVE_GETTIMEOFDAY && defined(__MACH__) && defined(__APPLE__)
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
  mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
  thread_basic_info_data_t thi_data;
  thread_basic_info_t thi = &thi_data;
  thread_info(mach_thread_self(), THREAD_BASIC_INFO, (thread_info_t)thi, &count);
  timer->start.tv_usec =  thi->system_time.microseconds + thi->user_time.microseconds;
  timer->start.tv_sec = thi->system_time.seconds + thi->user_time.seconds;
#elif HAVE_GETTIMEOFDAY //return time and not cputime in this case
  timer->elapse.tv_sec = 0;
  timer->elapse.tv_usec = 0;
  gettimeofday(&(timer->start), NULL);
#elif defined(_WIN32)
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
#endif
}

void xbt_os_threadtimer_resume(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_nsec += timer->stop.tv_nsec - timer->start.tv_nsec;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(timer->start));
#elif HAVE_GETTIMEOFDAY && defined(__MACH__) && defined(__APPLE__)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
  mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
  thread_basic_info_data_t thi_data;
  thread_basic_info_t thi = &thi_data;
  thread_info(mach_thread_self(), THREAD_BASIC_INFO, (thread_info_t)thi, &count);
  timer->start.tv_usec =  thi->system_time.microseconds + thi->user_time.microseconds;
  timer->start.tv_sec = thi->system_time.seconds + thi->user_time.seconds;
#elif HAVE_GETTIMEOFDAY
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
  gettimeofday(&(timer->start), NULL);
#elif defined(_WIN32)
  timer->elapse.tv_sec += timer->stop.tv_sec - timer->start.tv_sec;
  timer->elapse.tv_usec += timer->stop.tv_usec - timer->start.tv_usec;
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
#endif
}

void xbt_os_threadtimer_stop(xbt_os_timer_t timer)
{
#if HAVE_POSIX_GETTIME
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(timer->stop));
#elif HAVE_GETTIMEOFDAY && defined(__MACH__) && defined(__APPLE__)
  mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
  thread_basic_info_data_t thi_data;
  thread_basic_info_t thi = &thi_data;
  thread_info(mach_thread_self(), THREAD_BASIC_INFO, (thread_info_t)thi, &count);
  timer->stop.tv_usec =  thi->system_time.microseconds + thi->user_time.microseconds;
  timer->stop.tv_sec = thi->system_time.seconds + thi->user_time.seconds;
#elif HAVE_GETTIMEOFDAY //if nothing else is available, return just time
  gettimeofday(&(timer->stop), NULL);
#elif defined(_WIN32)
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
#endif
}
