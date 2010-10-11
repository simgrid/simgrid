/*
 * src/unit.c - type representing the tesh unit concept.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh unit concept.
 *
 */  
    
#include <unit.h>
#include <command.h>
#include <context.h>
#include <fstream.h>
#include <variable.h>
#include <str_replace.h>
#include <xerrno.h>
    XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

/*! \brief unit_start - start the processing of the tesh file representing by the unit
 *
 * \param p		A void pointer to the unit representing the tesh file to process.
 *
 * \return		This function (thread routine always returns NULL)
 *
 * Scenario :
 *	
 *	1) The unit increment the number of running unit of the runner.
 *	2) The unit wait for the jobs semaphore to realy start its job.
 *	3) The unit runs the parsing of its tesh file using an fstream object.
 *	   	3.1) The fstream object parse the tesh file an launch each command with its context of execution.
 *		3.2) If a syntax error is detected the fstream object handle the failure and signals it by setting
 *		     the flag interrupted of the unit to one (depending of the keep-going and keep-going-unit flag values)
 *		3.3) If a command failed (exit code do not match, timeout, ouptupt different from the expected..)
 *		     the command handle the failure (see command_handle_failure() for more details).
 *	4) After the parsing of the tesh file.
 *		4.1) If all commands are successeded the last command release the unit by releasing its semaphore. 
 *		4.2) If a command failed or if the tesh file is malformated the unit interrupt all the commands in progress.
 *	5) The unit wait for the end of all the threads associated with a command.
 *	6) Its release the next waiting unit (if any) by releasing the jobs semaphore.
 *	7) If its the last unit, it release the runner by releasing the semaphore used to wait for the end of all the units.
 *	    
 */ 
static void * unit_start(void *p) 
{
  xbt_os_thread_t thread;
  xbt_os_mutex_t mutex;
  unit_t include, suite;
  unsigned int itc, itu, its;
  int include_nb, suite_nb;
  command_t command;
  unit_t root = (unit_t) p;
  
      /* increment the number of running units */ 
      xbt_os_mutex_acquire(root->mutex);
  root->runner->number_of_runned_units++;
  xbt_os_mutex_release(root->mutex);
  
      /* must acquire the jobs semaphore to start */ 
      /*xbt_os_sem_acquire(jobs_sem); */ 
      
      /* initialize the mutex used to synchronize the access to the properties of this unit */ 
      mutex = xbt_os_mutex_init();
  if (!dry_run_flag)
    INFO1("Test unit from %s", root->fstream->name);
  
  else
    INFO1("Checking unit %s...", root->fstream->name);
  
      /* launch the parsing of the unit */ 
      fstream_parse(root->fstream, mutex);
  
      /* if the unit is not interrupted and not failed the unit, all the file is parsed
       * so all the command are launched
       */ 
      if (!root->interrupted)
     {
    root->parsed = 1;
    
        /* all the commands have terminated before the end of the parsing of the tesh file
         * so the unit release the semaphore itself
         */ 
        if (!root->released
            && (root->started_cmd_nb ==
                (root->failed_cmd_nb + root->interrupted_cmd_nb +
                 root->successeded_cmd_nb)))
      xbt_os_sem_release(root->sem);
    }
  
      /* wait the end of all the commands or a command failure or an interruption */ 
      xbt_os_sem_acquire(root->sem);
  if (root->interrupted)
     {
    xbt_dynar_foreach(root->commands, itc, command)  {
      if (command->status == cs_in_progress)
        command_interrupt(command);
    }
    
        /* interrupt all the running commands of the included units */ 
        include_nb = xbt_dynar_length(root->includes);
    xbt_dynar_foreach(root->includes, itu, include)  {
      xbt_dynar_foreach(include->commands, itc, command)  {
        if (command->status == cs_in_progress)
          command_interrupt(command);
      }
    }
    
        /* interrupt all the running commands of the unit */ 
        suite_nb = xbt_dynar_length(root->suites);
    xbt_dynar_foreach(root->suites, its, suite)  {
      include_nb = xbt_dynar_length(suite->includes);
      xbt_dynar_foreach(suite->includes, itu, include)  {
        xbt_dynar_foreach(include->commands, itc, command)  {
          if (command->status == cs_in_progress)
            command_interrupt(command);
        }
      }
    }
    }
  
      /* wait the end of the command threads of the unit */ 
      xbt_dynar_foreach(root->commands, itc, command)  {
    thread = command->thread;
    if (thread)
      xbt_os_thread_join(thread, NULL);
  }
  
      /* wait the end of the command threads of the included units of the unit */ 
      include_nb = xbt_dynar_length(root->includes);
  xbt_dynar_foreach(root->includes, itu, include)  {
    xbt_dynar_foreach(include->commands, itc, command)  {
      thread = command->thread;
      if (thread)
        xbt_os_thread_join(thread, NULL);
    }
    if (!dry_run_flag)
       {
      if (!include->exit_code && !include->interrupted)
        INFO1("Include from %s OK", include->fstream->name);
      
      else if (include->exit_code)
        ERROR3("Include `%s' NOK : (<%s> %s)", include->fstream->name,
                include->err_line, error_to_string(include->exit_code,
                                                   include->err_kind));
      
      else if (include->interrupted && !include->exit_code)
        INFO1("Include `(%s)' INTR", include->fstream->name);
      }
  }
  
      /* interrupt all the running commands of the unit */ 
      suite_nb = xbt_dynar_length(root->suites);
  xbt_dynar_foreach(root->suites, its, suite)  {
    include_nb = xbt_dynar_length(suite->includes);
    if (!include_nb)
       {
      if (!suite->exit_code)
         {
        unit_set_error(suite, ESYNTAX, 1, suite->filepos);
        ERROR2("[%s] Empty suite `(%s)' detected (no includes added)",
                suite->filepos, suite->description);
        
            /* if the --keep-going option is not specified */ 
            if (!keep_going_flag)
           {
          if (!interrupted)
             {
            
                /* request an global interruption by the runner */ 
                interrupted = 1;
            
                /* release the runner */ 
                xbt_os_sem_release(units_sem);
            }
          }
        }
      }
    xbt_dynar_foreach(suite->includes, itu, include)  {
      xbt_dynar_foreach(include->commands, itc, command)  {
        thread = command->thread;
        if (thread)
          xbt_os_thread_join(thread, NULL);
      }
      if (!include->exit_code && !include->interrupted)
         {
        if (!dry_run_flag)
          INFO1("Include from %s OK", include->fstream->name);
        }
      
      else if (include->exit_code)
         {
        if (!dry_run_flag)
          ERROR3("Include `%s' NOK : (<%s> %s)", include->fstream->name,
                  command->context->pos,
                  error_to_string(include->exit_code, include->err_kind));
        suite->exit_code = include->exit_code;
        suite->err_kind = include->err_kind;
        suite->err_line = strdup(include->err_line);
        }
      
      else if (include->interrupted && !include->exit_code)
         {
        if (!dry_run_flag)
          INFO1("Include `(%s)' INTR", include->fstream->name);
        suite->interrupted = 1;
        }
    }
    if (!dry_run_flag)
       {
      if (!suite->exit_code && !suite->interrupted)
        INFO1("Test suite from %s OK", suite->description);
      
      else if (suite->exit_code)
        ERROR3("Test suite `%s' NOK : (<%s> %s) ", suite->description,
                suite->err_line, error_to_string(suite->exit_code,
                                                 suite->err_kind));
      
      else if (suite->interrupted && !suite->exit_code)
        INFO1("Test suite `(%s)' INTR", suite->description);
      }
  }
  
      /* you can now destroy the mutex used to synchrone the command accesses to the properties of the unit */ 
      xbt_os_mutex_destroy(mutex);
  
      /* update the number of ended units of the runner */ 
      xbt_os_mutex_acquire(root->mutex);
  
      /* increment the number of ended units */ 
      root->runner->number_of_ended_units++;
  if (!dry_run_flag)
     {
    if (root->interrupted && !root->exit_code)
      INFO1("Test unit from %s INTR", root->fstream->name);
    
    else if (!root->exit_code)
      INFO1("Test unit from %s OK", root->fstream->name);
    
    else if (root->exit_code)
      ERROR3("Test unit `%s': NOK (<%s> %s)", root->fstream->name,
              root->err_line, error_to_string(root->exit_code,
                                              root->err_kind));
    }
  
      /* if it's the last unit, release the runner */ 
      if ((root->runner->number_of_runned_units ==
           root->runner->number_of_ended_units))
     {
    
        /* if all the commands of the unit are successeded itc's a successeded unit */ 
        if (root->successeded_cmd_nb == root->cmd_nb
            && !root->
            exit_code /* case of only one cd : nb = successeded = 0) */ )
      root->successeded = 1;
    
        /* first release the mutex */ 
        xbt_os_mutex_release(root->mutex);
    
        /* release the runner */ 
        xbt_os_sem_release(units_sem);
    }
  
  else
    xbt_os_mutex_release(root->mutex);
  
      /* release the jobs semaphore, then the next waiting unit can start */ 
      xbt_os_sem_release(jobs_sem);
  return NULL;
}

unit_t 
unit_new(runner_t runner, unit_t root, unit_t owner, fstream_t fstream) 
{
  unit_t unit;
  unit = xbt_new0(s_unit_t, 1);
  
      /* instantiate the vector used to store all the commands of the unit */ 
      unit->commands =
      xbt_dynar_new(sizeof(command_t), (void_f_pvoid_t) command_free);
  
      /* instantiate the vector used to store all the included units */ 
      unit->includes =
      xbt_dynar_new(sizeof(unit_t), (void_f_pvoid_t) unit_free);
  
      /* instantiate the vector used to store all the included suites */ 
      unit->suites =
      xbt_dynar_new(sizeof(unit_t), (void_f_pvoid_t) unit_free);
  
      /* the runner used to launch the tesh unit */ 
      unit->runner = runner;
  
      /* the file stream object to use to parse the tesh file */ 
      unit->fstream = fstream;
  if (fstream)
    fstream->unit = unit;
  
      /* if no root parameter specified assume that itc's the root of all the units */ 
      unit->root = root ? root : unit;
  
      /* the owner of the suite */ 
      unit->owner = owner;
  unit->thread = NULL;
  unit->started_cmd_nb = 0;
  unit->interrupted_cmd_nb = 0;
  unit->failed_cmd_nb = 0;
  unit->successeded_cmd_nb = 0;
  unit->terminated_cmd_nb = 0;
  unit->waiting_cmd_nb = 0;
  unit->interrupted = 0;
  unit->failed = 0;
  unit->successeded = 0;
  unit->parsed = 0;
  unit->released = 0;
  unit->owner = owner;
  unit->is_running_suite = 0;
  unit->description = NULL;
  unit->sem = NULL;
  unit->exit_code = 0;
  unit->err_kind = 0;
  unit->err_line = NULL;
  unit->filepos = NULL;
  return unit;
}

void 
unit_set_error(unit_t unit, int errcode, int kind, const char *line) 
{
  if (!unit->exit_code)
     {
    unit->exit_code = errcode;
    unit->err_kind = kind;
    unit->err_line = strdup(line);
    if (unit->root && !unit->root->exit_code)
       {
      unit->root->exit_code = errcode;
      unit->root->err_kind = kind;
      unit->root->err_line = strdup(line);
      }
    if (!exit_code)
       {
      exit_code = errcode;
      err_kind = kind;
      err_line = strdup(line);
      }
    }
}

int  unit_free(unit_t * ptr) 
{
  if (!(*ptr))
     {
    errno = EINVAL;
    return -1;
    }
  if ((*ptr)->commands)
    xbt_dynar_free(&((*ptr)->commands));
  if ((*ptr)->includes)
    xbt_dynar_free(&((*ptr)->includes));
  if ((*ptr)->suites)
    xbt_dynar_free(&((*ptr)->suites));
  
      /* if the unit is interrupted during its run, the semaphore is NULL */ 
      if ((*ptr)->sem)
    xbt_os_sem_destroy((*ptr)->sem);
  if ((*ptr)->description)
    free((*ptr)->description);
  if ((*ptr)->err_line)
    free((*ptr)->err_line);
  if ((*ptr)->filepos)
    free((*ptr)->filepos);
  free(*ptr);
  *ptr = NULL;
  return 0;
}

int  unit_run(unit_t unit, xbt_os_mutex_t mutex) 
{
  
      /* check the parameters */ 
      if (!(unit) || !mutex)
     {
    errno = EINVAL;
    xbt_os_sem_release(jobs_sem);
    return -1;
    }
  if (!interrupted)
     {
    unit->mutex = mutex;
    unit->sem = xbt_os_sem_init(0);
    
        /* start the unit */ 
        unit->thread = xbt_os_thread_create("", unit_start, unit);
    }
  
  else
     {
    
        /* the unit is interrupted by the runner before its starting 
         * in this case the unit semaphore is NULL take care of that
         * in the function unit_free()
         */ 
        unit->interrupted = 1;
    xbt_os_sem_release(jobs_sem);
    }
  return 0;
}

int  unit_interrupt(unit_t unit) 
{
  
      /* check the parameter */ 
      if (!(unit))
     {
    errno = EINVAL;
    return -1;
    }
  
      /* if the unit is already interrupted, signal the error */ 
      if (unit->interrupted)
     {
    errno = EALREADY;
    return -1;
    }
  
      /* interrupt the run of the specified unit */ 
      unit->interrupted = 1;
  xbt_os_sem_release(unit->sem);
  return 0;
}


/* just print the title of the root unit or a suite (if any) */ 
static void  print_title(const char *description) 
{
  register int i;
  char title[80];
  size_t len = strlen(description);
  title[0] = ' ';
  for (i = 1; i < 79; i++)
    title[i] = '=';
  title[i++] = '\n';
  title[79] = '\0';
  sprintf(title + 40 - (len + 4) / 2, "[ %s ]", description);
  title[40 + (len + 5) / 2] = '=';
  printf("\n%s\n", title);
}

int  unit_summuarize(unit_t unit) 
{
  command_t command;
  unsigned int itc, itu, its;
  unit_t include;
  unit_t suite;
  char *p;
  char title[PATH_MAX + 1] = { 0 };
  int number_of_tests = 0;    /* number of tests of a unit contained by this unit                                     */
  int number_of_failed_tests = 0;      /* number of failed test of a unit contained by this unit                       */
  int number_of_successeded_tests = 0; /* number of successeded tests of a unit contained by this unit         */
  int number_of_interrupted_tests = 0; /* number of interrupted tests of a unit contained by this unit         */
  int number_of_tests_of_suite = 0;   /* number of tests of a suite contained by this unit                            */
  int number_of_interrupted_tests_of_suite = 0;        /* number of interrupted tests of a suite contained by this unit        */
  int number_of_failed_tests_of_suite = 0;     /* number of failed tests of a suite contained by this unit                                     */
  int number_of_successeded_tests_of_suite = 0;        /* number of successeded tests of a suite contained by this                     */
  int number_of_units = 0;    /* number of units contained by a suite                                                         */
  int number_of_failed_units = 0;      /* number of failed units contained by a suite                                          */
  int number_of_successeded_units = 0; /* number of successeded units contained by a suite                                     */
  int number_of_interrupted_units = 0; /* number of interrupted units contained by a suite                                     */
  int total_of_tests = 0;     /* total of the tests contained by this unit                                            */
  int total_of_failed_tests = 0;       /* total of failed tests contained by this unit                                         */
  int total_of_successeded_tests = 0;  /* total of successeded tests contained by this unit                            */
  int total_of_interrupted_tests = 0;  /* total of interrupted tests contained by this unit                            */
  int total_of_units = 0;     /* total of units contained by this unit                                                        */
  int total_of_failed_units = 0;       /* total of failed units contained by this unit                                         */
  int total_of_successeded_units = 0;  /* total of successeded units contained by this unit                            */
  int total_of_interrupted_units = 0;  /* total of interrutped units contained by this unit                            */
  int total_of_suites = 0;    /* total of suites contained by this unit                                                       */
  int total_of_failed_suites = 0;      /* total of failed suites contained by this unit                                        */
  int total_of_successeded_suites = 0; /* total of successeded suites contained by this unit                           */
  int total_of_interrupted_suites = 0; /* total of interrupted suites contained by this unit                           */
  
      /* check the parameter */ 
      if (!(unit))
     {
    errno = EINVAL;
    return -1;
    }
  if ((unit->description) && strlen(unit->description) < 76)
    strcpy(title, unit->description);
  
  else
    sprintf(title, "file : %s", unit->fstream->name);
  if (unit->interrupted)
     {
    if (strlen(title) + strlen(" (interrupted)") < 76)
      strcat(title, " (interrupted)");
    
    else
       {
      memset(title, 0, PATH_MAX + 1);
      sprintf(title, "file : %s", unit->fstream->name);
      strcat(title, " (interrupted)");
      }
    }
  print_title(title);
  number_of_tests = xbt_dynar_length(unit->commands);
  
      /* tests */ 
      xbt_dynar_foreach(unit->commands, itc, command)  {
    if (command->status == cs_interrupted)
      number_of_interrupted_tests++;
    
    else if (command->status == cs_failed)
      number_of_failed_tests++;
    
    else if (command->status == cs_successeded)
      number_of_successeded_tests++;
  }
  if (number_of_tests)
     {
    asprintf(&p,
              " Test(s): .........................................................................");
    p[70] = '\0';
    printf("%s", p);
    free(p);
    if (number_of_failed_tests > 0)
      printf(".. failed\n");
    
    else if (number_of_interrupted_tests > 0)
      printf("interrupt\n");
    
    else
      printf(".... ..ok\n");
    xbt_dynar_foreach(unit->commands, itc, command)  {
      printf("        %s: %s [%s]\n",
              command->status ==
              cs_interrupted ? "INTR  "  : command->status ==
              cs_failed ? "FAILED"  : command->status ==
              cs_successeded ? "PASS  "  : "UNKNWN",
              command->context->command_line, command->context->pos);
      if (detail_summary_flag)
        command_summarize(command);
    }
    printf
        (" =====================================================================%s\n",
         number_of_failed_tests ? "== FAILED" :
         number_of_interrupted_tests ? "==== INTR" : "====== OK");
    printf("    Summary: Test(s): %.0f%% ok (%d test(s): %d ok",
             ((1 -
                ((double) number_of_failed_tests +
                 (double) number_of_interrupted_tests) /
                (double) number_of_tests) * 100.0), number_of_tests,
             number_of_successeded_tests);
    if (number_of_failed_tests > 0)
      printf(", %d failed", number_of_failed_tests);
    if (number_of_interrupted_tests > 0)
      printf(", %d interrupted)", number_of_interrupted_tests);
    printf(")\n\n");
    total_of_tests = number_of_tests;
    total_of_failed_tests = number_of_failed_tests;
    total_of_interrupted_tests = number_of_interrupted_tests;
    total_of_successeded_tests = number_of_successeded_tests;
    }
  
      /* includes */ 
      total_of_failed_units = total_of_interrupted_units =
      total_of_successeded_units = 0;
  number_of_failed_units = number_of_successeded_units =
      number_of_interrupted_units = 0;
  number_of_units = xbt_dynar_length(unit->includes);
  xbt_dynar_foreach(unit->includes, itu, include)  {
    number_of_interrupted_tests = number_of_failed_tests =
        number_of_successeded_tests = 0;
    number_of_tests = xbt_dynar_length(include->commands);
    xbt_dynar_foreach(include->commands, itc, command)  {
      if (command->status == cs_interrupted)
        number_of_interrupted_tests++;
      
      else if (command->status == cs_failed)
        number_of_failed_tests++;
      
      else if (command->status == cs_successeded)
        number_of_successeded_tests++;
    }
    asprintf(&p,
               " Unit: %s ............................................................................",
               include->description
               && strlen(include->description) <
               60 ? include->description : include->fstream->name);
    p[70] = '\0';
    printf("%s", p);
    free(p);
    if (number_of_failed_tests > 0)
       {
      total_of_failed_units++;
      printf(".. failed\n");
      }
    
    else if (number_of_interrupted_tests > 0)
       {
      total_of_interrupted_units++;
      printf("interrupt\n");
      }
    
    else
       {
      total_of_successeded_units++;
      printf(".... ..ok\n");
      }
    if (detail_summary_flag)
       {
      xbt_dynar_foreach(include->commands, itc, command)  {
        printf("        %s: %s [%s]\n",
                command->status ==
                cs_interrupted ? "INTR  "  : command->status ==
                cs_failed ? "FAILED"  : command->status ==
                cs_successeded ? "PASS  "  : "UNKNWN",
                command->context->command_line, command->context->pos);
        command_summarize(command);
      }
      }
    printf
        (" =====================================================================%s\n",
         number_of_failed_tests ? "== FAILED" :
         number_of_interrupted_tests ? "==== INTR" : "====== OK");
    printf("    Summary: Test(s): %.0f%% ok (%d test(s): %d ok",
              (number_of_tests
                ? (1 -
                   ((double) number_of_failed_tests +
                    (double) number_of_interrupted_tests) /
                   (double) number_of_tests) * 100.0 : 100.0),
              number_of_tests, number_of_successeded_tests);
    if (number_of_failed_tests > 0)
      printf(", %d failed", number_of_failed_tests);
    if (number_of_interrupted_tests > 0)
      printf(", %d interrupted)", number_of_interrupted_tests);
    printf(")\n\n");
    total_of_tests += number_of_tests;
    total_of_failed_tests += number_of_failed_tests;
    total_of_interrupted_tests += number_of_interrupted_tests;
    total_of_successeded_tests += number_of_successeded_tests;
  }
  
      /* suites */ 
      total_of_units = number_of_units;
  total_of_failed_suites = total_of_successeded_suites =
      total_of_interrupted_suites = 0;
  total_of_suites = xbt_dynar_length(unit->suites);
  xbt_dynar_foreach(unit->suites, its, suite)  {
    print_title(suite->description);
    number_of_tests_of_suite = number_of_interrupted_tests_of_suite =
        number_of_failed_tests_of_suite =
        number_of_successeded_tests_of_suite = 0;
    number_of_interrupted_units = number_of_failed_units =
        number_of_successeded_units = 0;
    number_of_units = xbt_dynar_length(suite->includes);
    xbt_dynar_foreach(suite->includes, itu, include)  {
      number_of_interrupted_tests = number_of_failed_tests =
          number_of_successeded_tests = 0;
      number_of_tests = xbt_dynar_length(include->commands);
      xbt_dynar_foreach(include->commands, itc, command)  {
        if (command->status == cs_interrupted)
          number_of_interrupted_tests++;
        
        else if (command->status == cs_failed)
          number_of_failed_tests++;
        
        else if (command->status == cs_successeded)
          number_of_successeded_tests++;
      }
      asprintf(&p,
                 " Unit: %s ............................................................................",
                 include->description
                 && strlen(include->description) <
                 60 ? include->description : include->fstream->name);
      p[70] = '\0';
      printf("%s", p);
      free(p);
      if (number_of_failed_tests > 0)
         {
        number_of_failed_units++;
        printf(".. failed\n");
        }
      
      else if (number_of_interrupted_tests > 0)
         {
        number_of_interrupted_units++;
        printf("interrupt\n");
        }
      
      else
         {
        number_of_successeded_units++;
        printf(".... ..ok\n");
        }
      number_of_interrupted_tests_of_suite +=
          number_of_interrupted_tests;
      number_of_failed_tests_of_suite += number_of_failed_tests;
      number_of_successeded_tests_of_suite += number_of_successeded_tests;
      number_of_tests_of_suite += number_of_tests;
      total_of_tests += number_of_tests;
      total_of_failed_tests += number_of_failed_tests;
      total_of_interrupted_tests += number_of_interrupted_tests;
      total_of_successeded_tests += number_of_successeded_tests;
      if (detail_summary_flag)
         {
        xbt_dynar_foreach(include->commands, itc, command)  {
          printf("        %s: %s [%s]\n",
                  command->status ==
                  cs_interrupted ? "INTR  "  : command->status ==
                  cs_failed ? "FAILED"  : command->status ==
                  cs_successeded ? "PASS  "  : "UNKNWN",
                  command->context->command_line, command->context->pos);
          command_summarize(command);
        }
        }
    }
    printf
        (" =====================================================================%s\n",
         number_of_failed_tests_of_suite ? "== FAILED" :
         number_of_interrupted_tests_of_suite ? "==== INTR" : "====== OK");
    if (number_of_failed_tests_of_suite > 0)
      total_of_failed_suites++;
    
    else if (number_of_interrupted_tests_of_suite)
      total_of_interrupted_suites++;
    
    else
      total_of_successeded_suites++;
    total_of_failed_units += number_of_failed_units;
    total_of_interrupted_units += number_of_interrupted_units;
    total_of_successeded_units += number_of_successeded_units;
    total_of_units += number_of_units;
    printf("    Summary: Unit(s): %.0f%% ok (%d unit(s): %d ok", 
             (number_of_units
              ? (1 -
                 ((double) number_of_failed_units +
                  (double) number_of_interrupted_units) /
                 (double) number_of_units) * 100.0 : 100.0),
             number_of_units, number_of_successeded_units);
    if (number_of_failed_units > 0)
      printf(", %d failed", number_of_failed_units);
    if (number_of_interrupted_units > 0)
      printf(", %d interrupted)", number_of_interrupted_units);
    printf(")\n");
    printf("             Test(s): %.0f%% ok (%d test(s): %d ok", 
             (number_of_tests_of_suite
              ? (1 -
                 ((double) number_of_failed_tests_of_suite +
                  (double) number_of_interrupted_tests_of_suite) /
                 (double) number_of_tests_of_suite) * 100.0 : 100.0),
             number_of_tests_of_suite,
             number_of_successeded_tests_of_suite);
    if (number_of_failed_tests_of_suite > 0)
      printf(", %d failed", number_of_failed_tests_of_suite);
    if (number_of_interrupted_tests_of_suite > 0)
      printf(", %d interrupted)", number_of_interrupted_tests_of_suite);
    printf(")\n\n");
  }
  printf(" TOTAL : Suite(s): %.0f%% ok (%d suite(s): %d ok", 
           (total_of_suites
            ? (1 -
               ((double) total_of_failed_suites +
                (double) total_of_interrupted_suites) /
               (double) total_of_suites) * 100.0 : 100.0),
           total_of_suites, total_of_successeded_suites);
  if (total_of_failed_suites > 0)
    printf(", %d failed", total_of_failed_suites);
  if (total_of_interrupted_suites > 0)
    printf(", %d interrupted)", total_of_interrupted_suites);
  printf(")\n");
  printf("         Unit(s):  %.0f%% ok (%d unit(s): %d ok", 
           (total_of_units
            ? (1 -
               ((double) total_of_failed_units +
                (double) total_of_interrupted_units) /
               (double) total_of_units) * 100.0 : 100.0), total_of_units,
           total_of_successeded_units);
  if (total_of_failed_units > 0)
    printf(", %d failed", total_of_failed_units);
  if (total_of_interrupted_units > 0)
    printf(", %d interrupted)", total_of_interrupted_units);
  printf(")\n");
  printf("         Test(s):  %.0f%% ok (%d test(s): %d ok", 
           (total_of_tests
            ? (1 -
               ((double) total_of_failed_tests +
                (double) total_of_interrupted_tests) /
               (double) total_of_tests) * 100.0 : 100.0), total_of_tests,
           total_of_successeded_tests);
  if (total_of_failed_tests > 0)
    printf(", %d failed", total_of_failed_tests);
  if (total_of_interrupted_tests > 0)
    printf(", %d interrupted)", total_of_interrupted_tests);
  printf(")\n\n");
  if (unit->interrupted)
    unit->runner->total_of_interrupted_units++;
  
  else if (total_of_failed_tests > 0)
    unit->runner->total_of_failed_units++;
  
  else
    unit->runner->total_of_successeded_units++;
  unit->runner->total_of_tests += total_of_tests;
  unit->runner->total_of_failed_tests += total_of_failed_tests;
  unit->runner->total_of_successeded_tests += total_of_successeded_tests;
  unit->runner->total_of_interrupted_tests += total_of_interrupted_tests;
  unit->runner->total_of_units += total_of_units + 1;
  unit->runner->total_of_successeded_units += total_of_successeded_units;
  unit->runner->total_of_failed_units += total_of_failed_units;
  unit->runner->total_of_interrupted_units += total_of_interrupted_units;
  unit->runner->total_of_suites += total_of_suites;
  unit->runner->total_of_successeded_suites +=
      total_of_successeded_suites;
  unit->runner->total_of_failed_suites += total_of_failed_suites;
  unit->runner->total_of_interrupted_suites +=
      total_of_interrupted_suites;
  return 0;
}

int  unit_reset(unit_t unit) 
{
  unit_t cur;
  unsigned int i;
  
      /* reset all the suites of the unit */ 
      xbt_dynar_foreach(unit->suites, i, cur)  {
    unit_reset(cur);
  } 
      /* reset all the includes of the unit */ 
      xbt_dynar_foreach(unit->includes, i, cur)  {
    unit_reset(cur);
  } fseek(unit->fstream->stream, 0L, SEEK_SET);
  unit->parsed = 0;
  unit->cmd_nb = 0;
  unit->started_cmd_nb = 0;
  unit->interrupted_cmd_nb = 0;
  unit->failed_cmd_nb = 0;
  unit->successeded_cmd_nb = 0;
  unit->terminated_cmd_nb = 0;
  unit->waiting_cmd_nb = 0;
  unit->interrupted = 0;
  unit->failed = 0;
  unit->successeded = 0;
  unit->parsed = 0;
  unit->released = 0;
  unit->is_running_suite = 0;
  if (unit->description)
     {
    free(unit->description);
    unit->description = NULL;
    }
  unit->exit_code = 0;
  return 0;
}


