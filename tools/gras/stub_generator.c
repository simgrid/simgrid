/* gras_stub_generator - creates the main() to use a GRAS program           */

/* Copyright (c) 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* specific to Borland Compiler */
#ifdef __BORLANDDC__
#pragma hdrstop
#endif

#include <stdio.h>
#include "xbt/sysdep.h"
#include "xbt/function_types.h"
#include "xbt/log.h"
#include "surf/surfxml_parse.h"
#include "surf/surf.h"
#include "portable.h"           /* Needed for the time of the SIMIX convertion */

#include "gras_stub_generator.h"
#include <stdarg.h>



XBT_LOG_NEW_DEFAULT_SUBCATEGORY(stubgen, gras, "Stub generator");


#ifdef _XBT_WIN32
#include <windows.h>
#endif

/* specific to Borland Compiler */
#ifdef __BORLANDDC__
#pragma argsused
#endif


/**********************************************/
/********* Parse XML deployment file **********/
/**********************************************/
xbt_dict_t process_function_set = NULL;
xbt_dynar_t process_list = NULL;
xbt_dict_t machine_set = NULL;

void s_process_free(void *process)
{
  s_process_t *p = (s_process_t *) process;
  int i;
  for (i = 0; i < p->argc; i++)
    free(p->argv[i]);
  free(p->argv);
  free(p->host);
}

static s_process_t process;

static void parse_process_init(void)
{
  xbt_dict_set(process_function_set, A_surfxml_process_function, NULL,
               NULL);
  xbt_dict_set(machine_set, A_surfxml_process_host, NULL, NULL);
  process.argc = 1;
  process.argv = xbt_new(char *, 1);
  process.argv[0] = xbt_strdup(A_surfxml_process_function);
  process.host = strdup(A_surfxml_process_host);
  /*VERB1("Function: %s",A_surfxml_process_function); */
}

static void parse_argument(void)
{
  process.argc++;
  process.argv =
      xbt_realloc(process.argv, (process.argc) * sizeof(char *));
  process.argv[(process.argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

static void parse_process_finalize(void)
{
  xbt_dynar_push(process_list, &process);
  /*VERB1("Function: %s",process.argv[0]); */
}

/*FIXME Defined in surfxml_parse.c*/
#ifndef WIN32
void surfxml_add_callback(xbt_dynar_t cb_list, void_f_void_t function)
{
  xbt_dynar_push(cb_list, &function);
}
#endif


int main(int argc, char *argv[])
{
  char *project_name = NULL;
  char *deployment_file = NULL;
  int i;

  surf_init(&argc, argv);
  process_function_set = xbt_dict_new();
  process_list = xbt_dynar_new(sizeof(s_process_t), s_process_free);
  machine_set = xbt_dict_new();

  for (i = 1; i < argc; i++) {
    int need_removal = 0;
    if (!strncmp("--extra-process=", argv[i], strlen("--extra-process="))) {
      xbt_dict_set(process_function_set,
                   argv[i] + strlen("--extra-process="), NULL, NULL);
      need_removal = 1;
    }


    if (need_removal) {         /* remove the handled argument from argv */
      int j;
      for (j = i + 1; j < argc; j++) {
        argv[j - 1] = argv[j];
      }
      argv[j - 1] = NULL;
      argc--;
      i--;                      /* compensate effect of next loop incrementation */
    }
  }

  xbt_assert1((argc >= 3),
              "Usage: %s project_name deployment_file [deployment_file...]\n",
              argv[0]);

  project_name = argv[1];

  surf_parse_reset_parser();
  DEBUG2("%p %p", parse_process_init, &parse_process_init);
  surfxml_add_callback(STag_surfxml_process_cb_list, &parse_process_init);
  surfxml_add_callback(ETag_surfxml_argument_cb_list, &parse_argument);
  surfxml_add_callback(ETag_surfxml_process_cb_list,
                       &parse_process_finalize);

  for (i = 2; i < argc; i++) {
    deployment_file = argv[i];
    surf_parse_open(deployment_file);
    if (surf_parse())
      xbt_assert1(0, "Parse error in %s", deployment_file);

    surf_parse_close();
  }


  warning = xbt_new(char, strlen(WARN) + strlen(deployment_file) + 10);
  sprintf(warning, WARN, deployment_file);

  /*if(XBT_LOG_ISENABLED(stubgen, xbt_log_priority_debug)) {
     xbt_dict_cursor_t cursor=NULL;
     char *key = NULL;
     void *data = NULL;

     for (cursor=NULL, xbt_dict_cursor_first((process_function_set),&(cursor)) ;
     xbt_dict_cursor_get_or_free(&(cursor),&(key),(void**)(&data));
     xbt_dict_cursor_step(cursor) ) {
     DEBUG1("Function %s", key);      
     }

     xbt_dict_dump(process_function_set,print);
     } */

  generate_sim(project_name);
  generate_rl(project_name);
  generate_makefile_local(project_name, deployment_file);
#ifdef __BORLANDC__
  generate_borland_simulation_project(project_name);
  generate_borland_real_life_project(project_name);
  generate_simulation_dsp_file(project_name);
  generate_real_live_dsp_file(project_name);

  if (__gras_path)
    xbt_free(__gras_path);
#endif

  free(warning);
  surf_exit();
  return 0;
}
