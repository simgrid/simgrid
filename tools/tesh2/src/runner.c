/*
 * src/runner.c - type representing the runner.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh runner type.
 *
 */  
#include <runner.h>
#include <units.h>
#include <unit.h>
#include <xerrno.h>
#include <variable.h>
    
#include <errno.h>	/* for error code	*/
#include <stdlib.h>	/* for calloc()		*/
#include <stdio.h>
    
#include <readline.h>
#include <explode.h>
    
#ifndef _XBT_WIN32
#include <sys/resource.h>
#endif  /*  */
    
#define _RUNNER_HASHCODE		0xFEFEAAAA	
    XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

#if (!defined(__BUILTIN) && defined(__CHKCMD) && !defined(_XBT_WIN32))
static const char *builtin[] = 
    { "alias", "bind", "builtin", "caller", "cd", "command",
"compgen", "complete", "declare", "disown", "echo", "enable", "eval", "exec", "export",
"false", "fc", "function", "getopts", "hash", "history", "jobs", "let", "logout",
"printf", "pwd", "readonly", "shift", "shopt", "source", "suspend", "test", "time",
"times", "trap", "true", "type", "typeset", "ulimit", "umask", "unalias", "unset",
NULL 
};


#define __BUILTIN_MAX ((size_t)42)
#endif  /*  */
    
# ifdef __APPLE__
/* under darwin, the environment gets added to the process at startup time. So, it's not defined at library link time, forcing us to extra tricks */ 
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
# else
#ifdef _XBT_WIN32
    /* the environment, as specified by the opengroup, used to initialize the process properties */ 
# define environ **wenviron;
#else   /*  */
extern char **environ;

#endif  /*  */
# endif
    
#ifndef _XBT_WIN32
extern char ** environ;

#endif  /*  */
    
/* the unique tesh runner */ 
static runner_t  runner = NULL;

/* wait for the tesh runner terminaison	*/ 
static void  runner_wait(void);
static void * runner_start_routine(void *p);

/* check the syntax of the tesh files if 
 * the check_syntax_flag is specified. Returns
 * 0 if the syntax is clean.
 */ 
/*static void
check_syntax(void);*/ 
    
#ifdef _XBT_WIN32
static HANDLE  timer_handle = NULL;
static void * runner_start_routine(void *p) 
{
  LARGE_INTEGER li;
  li.QuadPart = -runner->timeout * 10000000;  /* 10000000 = 10 000 000 * 100 nanoseconds = 1 second */
  
      /* create the waitable timer */ 
      timer_handle = CreateWaitableTimer(NULL, TRUE, NULL);
  
      /* set a timer to wait for timeout seconds */ 
      SetWaitableTimer(timer_handle, &li, 0, NULL, NULL, 0);
  
      /* wait for the timer */ 
      WaitForSingleObject(timer_handle, INFINITE);
  if (runner->waiting)
     {
    exit_code = ELEADTIME;
    err_kind = 1;
    runner->timeouted = 1;
    xbt_os_sem_release(units_sem);
    }
  return NULL;
}


#else   /*  */
static void * runner_start_routine(void *p) 
{
  struct timespec ts;
  int timeout = runner->timeout;
  while (timeout-- && runner->waiting)
     {
    ts.tv_sec = 1;
    ts.tv_nsec = 0L;
    
    do
       {
      nanosleep(&ts, &ts);
    } while (EINTR == errno);
    }
  if (errno)
     {
    
        /* TODO process the error */ 
    }
  
  else
     {
    if (runner->waiting)
       {
      exit_code = ELEADTIME;
      err_kind = 1;
      runner->timeouted = 1;
      xbt_os_sem_release(units_sem);
      }
    }
  return NULL;
}


#endif  /*  */
int 
runner_init( /*int check_syntax_flag, */ int timeout,
            fstreams_t fstreams) 
{
  int i;
  char *val;
  char buffer[PATH_MAX + 1] = { 0 };
  int code;
  const char *cstr;
  variable_t variable;
  
#if (defined(__CHKCMD) && defined(__BUILTIN) && !defined(_XBT_WIN32))
      FILE * s;
  int n = 0;
  size_t len;
  char *line = NULL;
  int is_blank;
  
#endif  /*  */
      if (runner)
     {
    ERROR0("The runner is already initialized");
    return -1;
    }
  runner = xbt_new0(s_runner_t, 1);
  runner->path = NULL;
  runner->builtin = NULL;
  if (!(runner->units = units_new(runner, fstreams)))
     {
    free(runner);
    runner = NULL;
    return -1;
    }
  runner->timeout = timeout;
  runner->timeouted = 0;
  runner->interrupted = 0;
  runner->number_of_ended_units = 0;
  runner->number_of_runned_units = 0;
  runner->waiting = 0;
  runner->total_of_tests = 0;
  runner->total_of_successeded_tests = 0;
  runner->total_of_failed_tests = 0;
  runner->total_of_interrupted_tests = 0;
  runner->total_of_units = 0;
  runner->total_of_successeded_units = 0;
  runner->total_of_failed_units = 0;
  runner->total_of_interrupted_units = 0;
  runner->total_of_suites = 0;
  runner->total_of_successeded_suites = 0;
  runner->total_of_failed_suites = 0;
  runner->total_of_interrupted_suites = 0;
  
      /* initialize the vector of variables */ 
      runner->variables =
      xbt_dynar_new(sizeof(variable_t), (void_f_pvoid_t) variable_free);
  
      /* add the environment variables in the vector */ 
      for (i = 0; environ[i] != NULL; i++)
     {
    val = strchr(environ[i], '=');
    if (val)
       {
      val++;
      if (val[0] != '\0')
        strncpy(buffer, environ[i], (val - environ[i] - 1));
      if (!strcmp("TESH_PPID", buffer))
        is_tesh_root = 0;
      variable = variable_new(buffer, val);
      variable->env = 1;
      xbt_dynar_push(runner->variables, &variable);
      
#ifndef _XBT_WIN32
          if (!strcmp("PATH", buffer))
        
#else   /*  */
          if (!strcmp("Path", buffer) || !strcmp("PATH", buffer))
        
#endif  /*  */
      {
        char *p;
        size_t j, k, len;
        
            /* get the list of paths */ 
            
#ifdef _XBT_WIN32
            runner->path = explode(';', val);
        
#else   /*  */
            runner->path = explode(':', val);
        
#endif  /*  */
            
            /* remove spaces and backslahes at the end of the path */ 
            for (k = 0; runner->path[k] != NULL; k++)
           {
          p = runner->path[k];
          len = strlen(p);
          
#ifndef _XBT_WIN32
              for (j = len - 1; p[j] == '/' || p[j] == ' '; j--)
            
#else   /*  */
              for (j = len - 1; p[j] == '\\' || p[j] == ' '; j--)
            
#endif  /*  */
                p[j] = '\0';
          }
      }
      memset(buffer, 0, PATH_MAX + 1);
      }
    }
  if (is_tesh_root)
     {
    char *tesh_dir = getcwd(NULL, 0);
    sprintf(buffer, "%d", getpid());
    
#ifndef _XBT_WIN32
        setenv("TESH_PPID", buffer, 0);
    setenv("TESH_DIR", tesh_dir, 0);
    
#else   /*  */
        SetEnvironmentVariable("TESH_PPID", buffer);
    SetEnvironmentVariable("TESH_DIR", tesh_dir);
    
#endif  /*  */
        variable = variable_new("TESH_PPID", buffer);
    variable->err = 1;
    xbt_dynar_push(runner->variables, &variable);
    variable = variable_new("TESH_DIR", tesh_dir);
    variable->err = 1;
    xbt_dynar_push(runner->variables, &variable);
    free(tesh_dir);
    }
  variable = variable_new("EXIT_SUCCESS", "0");
  variable->err = 1;
  xbt_dynar_push(runner->variables, &variable);
  variable = variable_new("EXIT_FAILURE", "1");
  variable->err = 1;
  xbt_dynar_push(runner->variables, &variable);
  variable = variable_new("TRUE", "0");
  variable->err = 1;
  xbt_dynar_push(runner->variables, &variable);
  variable = variable_new("FALSE", "1");
  variable->err = 1;
  xbt_dynar_push(runner->variables, &variable);
  i = 0;
  
      /* add the errors variables */ 
      while ((cstr = error_get_at(i++, &code)))
     {
    sprintf(buffer, "%d", code);
    variable = variable_new(cstr, buffer);
    variable->err = 1;
    xbt_dynar_push(runner->variables, &variable);
    }
  
      /* if the user want check the syntax, check it */ 
      /*if(check_syntax_flag)
         check_syntax();
       */ 
      
#if (!defined(_XBT_WIN32) && defined(__CHKCMD))
#if defined(__BUILTIN)
      if (!is_tesh_root)
     {
    
        /* compute the full path the builtin.def file */ 
        sprintf(buffer, "%s/builtin.def", getenv("TESH_DIR"));
    if (!(s = fopen(buffer, "r")))
       {
      ERROR1("File `(%s)' not found", buffer);
      return -1;
      }
    }
  
  else
     {
    if (!(s = fopen("builtin.def", "r")))
       {
      ERROR0("File `(builtin.def)' not found");
      return -1;
      }
    }
  if (s)
     {
    fpos_t begin;
    fgetpos(s, &begin);
    while (readline(s, &line, &len) != -1)
       {
      i = 0;
      is_blank = 1;
      while (line[i] != '\0')
         {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n'
             && line[i] != '\r')
           {
          is_blank = 0;
          break;
          }
        i++;
        }
      if (!is_blank)
        n++;
      }
    fsetpos(s, &begin);
    free(line);
    line = NULL;
    if (n)
       {
      char *l;
      runner->builtin = xbt_new0(char *, n + 1);      /* (char**) calloc(n + 1, sizeof(char*)); */
      n = 0;
      while (readline(s, &line, &len) != -1)
         {
        i = 0;
        is_blank = 1;
        while (line[i] != '\0')
           {
          if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n'
               && line[i] != '\r')
             {
            is_blank = 0;
            break;
            }
          i++;
          }
        if (!is_blank)
           {
          l = strdup(line);
          l[strlen(l) - 1] = '\0';
          (runner->builtin)[n++] = l;
          }
        }
      }
    
    else
       {
      WARN0("The file `(builtin.def)' is empty");
      free(runner->builtin);
      runner->builtin = NULL;
      }
    fclose(s);
    if (line)
      free(line);
    }
  
#else   /*  */
      runner->builtin = xbt_new0(char *, __BUILTIN_MAX + 1);    /* (char**) calloc(__BUILTIN_MAX + 1, sizeof(char*)); */
  for (i = 0; i < __BUILTIN_MAX; i++)
    runner->builtin[i] = strdup(builtin[i]);
  
#endif  /*  */
#endif  /*  */
      return exit_code ? -1 : 0;
}

void  runner_destroy(void) 
{
  int i;
  if (runner->units)
    units_free((void **) (&(runner->units)));
  if (runner->variables)
    xbt_dynar_free(&runner->variables);
  
#ifdef _XBT_WIN32
      CloseHandle(timer_handle);
  
#endif  /*  */
      if (runner->thread)
    xbt_os_thread_join(runner->thread, NULL);
  if (runner->path)
     {
    for (i = 0; runner->path[i] != NULL; i++)
      free(runner->path[i]);
    free(runner->path);
    }
  if (runner->builtin)
     {
    for (i = 0; runner->builtin[i] != NULL; i++)
      free(runner->builtin[i]);
    free(runner->builtin);
    }
  free(runner);
  runner = NULL;
}

void  runner_run(void) 
{
  
      /* allocate the mutex used by the units to asynchronously access 
       * to the properties of the runner.
       */ 
      xbt_os_mutex_t mutex = xbt_os_mutex_init();
  
      /* run all the units */ 
      units_run_all(runner->units, mutex);
  if (!interrupted)
    runner_wait();
  
      /* if the runner is timeouted or receive a interruption request
       * , interrupt all the active units.
       */ 
      if (runner->timeouted || interrupted)
    runner_interrupt();
  
      /* joins all the units */ 
      units_join_all(runner->units);
  
      /* release the mutex resource */ 
      xbt_os_mutex_destroy(mutex);
}

static void  runner_wait(void) 
{
  if (runner->timeout > 0)
    runner->thread = xbt_os_thread_create("", runner_start_routine, NULL);
  
      /* signal that the runner is waiting */ 
      runner->waiting = 1;
  
      /* wait for the end of all the units */ 
      xbt_os_sem_acquire(units_sem);
  runner->waiting = 0;
}


/*
 * interrupt all the active units.
 * this function is called when the lead time of the execution is reached
 * or when a failed unit requests an interruption of the execution.
 */ 
void  runner_interrupt(void) 
{
  units_interrupt_all(runner->units);
} void  runner_summarize(void) 
{
  if (!dry_run_flag)
     {
    
#ifndef _XBT_WIN32
    struct rusage r_usage;
    
#else   /*  */
        FILETIME start_time;
    FILETIME exit_time;
    FILETIME kernel_time;
    FILETIME user_time;
    SYSTEMTIME si;
    
#endif  /*  */
        printf
        ("\n  TEst SHell utility - mini shell specialized in running test units.\n");
    printf
        (" =============================================================================\n");
    units_summuarize(runner->units);
    printf
        (" =====================================================================%s\n",
         runner->total_of_failed_tests ? "== FAILED" : (runner->
                                                         total_of_interrupted_tests
                                                         || runner->
                                                         total_of_interrupted_units)
         ? "==== INTR" : "====== OK");
    printf(" TOTAL : Suite(s): %.0f%% ok (%d suite(s): %d ok",
             (runner->
               total_of_suites ? (1 -
                                  ((double) runner->
                                   total_of_failed_suites +
                                   (double) runner->
                                   total_of_interrupted_suites) /
                                  (double) runner->total_of_suites) *
               100.0 : 100.0), runner->total_of_suites,
             runner->total_of_successeded_suites);
    if (runner->total_of_failed_suites > 0)
      printf(", %d failed", runner->total_of_failed_suites);
    if (runner->total_of_interrupted_suites > 0)
      printf(", %d interrupted)", runner->total_of_interrupted_suites);
    printf(")\n");
    printf("         Unit(s):  %.0f%% ok (%d unit(s): %d ok", 
             (runner->
              total_of_units ? (1 -
                                ((double) runner->total_of_failed_units +
                                 (double) runner->
                                 total_of_interrupted_units) /
                                (double) runner->total_of_units) *
              100.0 : 100.0), runner->total_of_units,
             runner->total_of_successeded_units);
    if (runner->total_of_failed_units > 0)
      printf(", %d failed", runner->total_of_failed_units);
    if (runner->total_of_interrupted_units > 0)
      printf(", %d interrupted)", runner->total_of_interrupted_units);
    printf(")\n");
    printf("         Test(s):  %.0f%% ok (%d test(s): %d ok", 
             (runner->
              total_of_tests ? (1 -
                                ((double) runner->total_of_failed_tests +
                                 (double) runner->
                                 total_of_interrupted_tests) /
                                (double) runner->total_of_tests) *
              100.0 : 100.0), runner->total_of_tests,
             runner->total_of_successeded_tests);
    if (runner->total_of_failed_tests > 0)
      printf(", %d failed", runner->total_of_failed_tests);
    if (runner->total_of_interrupted_tests > 0)
      printf(", %d interrupted)", runner->total_of_interrupted_tests);
    printf(")\n\n");
    
#ifndef _XBT_WIN32
        if (!getrusage(RUSAGE_SELF, &r_usage))
       {
      printf
          ("         Total tesh user time used:       %ld second(s) %ld microsecond(s)\n",
           r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec);
      printf
          ("         Total tesh system time used:     %ld second(s) %ld microsecond(s)\n\n",
           r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);
      if (!getrusage(RUSAGE_CHILDREN, &r_usage))
         {
        printf
            ("         Total children user time used:   %ld second(s) %ld microsecond(s)\n",
             r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec);
        printf
            ("         Total children system time used: %ld second(s) %ld microsecond(s)\n\n",
             r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);
        }
      }
    
#else   /*  */
        if (GetProcessTimes
             (GetCurrentProcess(), &start_time, &exit_time, &kernel_time,
              &user_time))
       {
      FileTimeToSystemTime(&user_time, &si);
      printf
          (" User time used:   %2u Hour(s) %2u Minute(s) %2u Second(s) %3u Millisecond(s)\n",
           si.wHour, si.wMinute, si.wSecond, si.wMilliseconds);
      FileTimeToSystemTime(&kernel_time, &si);
      printf
          (" Kernel time used: %2u Hour(s) %2u Minute(s) %2u Second(s) %3u Millisecond(s)\n",
           si.wHour, si.wMinute, si.wSecond, si.wMilliseconds);
      }
    
#endif  /*  */
    }
  
  else
     {
    if (exit_code)
      ERROR0("Syntax NOK");
    
    else if (!exit_code)
      INFO0("Syntax 0K");
    }
}

int  runner_is_timedout(void) 
{
  return runner->timeouted;
}


