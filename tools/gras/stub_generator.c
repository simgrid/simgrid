/* 	$Id$	 */

/* gras_stub_generator - creates the main() to use a GRAS program           */

/* Copyright (c) 2003,2004,2005 Martin Quinson, Arnaud Legrand. 
   All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"xbt/sysdep.h"
#include"xbt/dict.h"
#include"xbt/error.h"
#include "surf/surf_parse.h"
#include "surf/surf.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(stubgen,gras,"Stub generator");

#define WARN "/***********\n * DO NOT EDIT! THIS FILE WERE AUTOMATICALLY GENERATED FROM %s BY gras_stub_generator\n ***********/\n"
#define SIM_FILENAME  "_%s_simulator.c"
#define SIM_BINARYNAME  "%s_simulator"
#define SIM_FILENAME_LDADD  "%s_simulator_LDADD"
#define SIM_FILENAME_SOURCES  "%s_simulator_SOURCES"
#define RL_FILENAME  "_%s_%s.c"
#define RL_BINARYNAME  "%s_%s"
#define RL_FILENAME_LDADD  "%s_%s_LDADD"
#define RL_FILENAME_SOURCES  "%s_%s_SOURCES"
#define MAKEFILE_FILENAME  "%s.Makefile.am"

char *warning = NULL;

/**********************************************/
/**** Generate the file for the simulator *****/
/**********************************************/

const char *SIM_PREEMBULE =
"#include <stdlib.h>\n"
"#include <stdio.h>\n"
"#include \"msg/msg.h\"\n"
"#include <gras.h>\n"
"\n"
"char *gras_log=NULL;\n";


#define SIM_LAUNCH_FUNC  \
"int launch_%s(int argc, char **argv) {\n" \
"  char **myargv=argv;\n" \
"  int myargc=argc;\n" \
"  int i;\n" \
"  int retcode;\n"\
"    \n"\
"  if (gras_log) {\n"\
"    myargv=malloc((argc+1) * sizeof(char**));\n" \
"    for (i=0; i<argc; i++)\n" \
"      myargv[i] = argv[i];\n" \
"    myargv[myargc++] = gras_log;\n" \
"  }\n" \
"  retcode = %s(myargc,myargv);\n" \
"  if (myargv != argv)\n" \
"    free(myargv);\n" \
"  return retcode;\n" \
"}\n"

const char* SIM_MAIN_PREEMBULE =
"int main (int argc,char *argv[]) {\n"
"  int i,j;\n"
"\n"
"  /*  Simulation setup */\n"
"  MSG_global_init_args(&argc,argv);\n"
"  if (argc != 3) {\n"
"    fprintf(stderr, \"Usage: %s platform_file application_description.txt [--gras-log=...]\\n\",argv[0]);\n"
"    exit(1);\n"
"  }\n"
"\n"
"  MSG_set_channel_number(10); // GRAS_MAX_CHANNEL hardcoded since Alvin killed its definition\n"
"  MSG_create_environment(argv[1]);\n"
"\n"
"  /*  Application deployment */\n";

const char *SIM_MAIN_POSTEMBULE = "\n"
"\n"
"  MSG_launch_application(argv[2]);\n"
"\n"
"  /*  Run the simulation */\n"
"  MSG_main();\n"
"\n"
"  /* cleanup the place */\n"
"  MSG_clean();\n"
"  if (gras_log)\n"
"    free(gras_log);\n"
"  return 0;\n"
"}\n";

/**********************************************/
/**** Generate the file for the real life *****/
/**********************************************/

#define RL_CODE \
"#include <stdio.h>\n" \
"#include <signal.h>\n" \
"#include <gras.h>\n" \
"\n" \
"/* user code */\n" \
"int %s(int argc, char *argv[]);\n" \
"\n" \
"int main(int argc, char *argv[]){\n" \
"  int errcode;\n" \
"\n" \
"  errcode=%s(argc,argv);\n"\
" \n" \
"  return errcode;\n"\
"}\n"

/**********************************************/
/********* Parse XML deployment file **********/
/**********************************************/
xbt_dict_t process_function_set = NULL;

static void parse_process_init(void)
{ 
  void *p = (void *) 1231;
  xbt_dict_set(process_function_set, A_process_function, p, NULL);
}

static void generate_sim(char *project)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;
  filename = xbt_new(char,strlen(project) + strlen(SIM_FILENAME));
  sprintf(filename,SIM_FILENAME,project);
  
  OUT=fopen(filename,"w");
  xbt_assert1(OUT, "Unable to open %s for writing",filename);

  fprintf(OUT, "%s\n",warning);
  fprintf(OUT, "%s", SIM_PREEMBULE);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT,"int %s(int argc,char *argv[]);\n",key);
  }
  fprintf(OUT,"\n");
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT,"int launch_%s(int argc,char *argv[]);\n",key);
  }
  fprintf(OUT, "\n%s\n",warning);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT,SIM_LAUNCH_FUNC,key,key);
  }
  fprintf(OUT, "\n%s\n",warning);

  fprintf(OUT, "%s", SIM_MAIN_PREEMBULE);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT,"  MSG_function_register(\"%s\", launch_%s);\n",key,key);
  }
  fprintf(OUT, "%s", SIM_MAIN_POSTEMBULE);
  fclose(OUT);
  xbt_free(filename);
}

static void generate_rl(char *project)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  xbt_dict_foreach(process_function_set,cursor,key,data) {
    filename = xbt_new(char,strlen(project) + strlen(RL_FILENAME) + strlen(key));
    sprintf(filename,RL_FILENAME,project,key);
  
    OUT=fopen(filename,"w");
    xbt_assert1(OUT, "Unable to open %s for writing",filename);

    fprintf(OUT, "\n%s\n",warning);
    fprintf(OUT, RL_CODE, key,key);
    fprintf(OUT, "\n%s\n",warning);
    fclose(OUT);
    xbt_free(filename);
  }
}

static void generate_makefile(char *project, char *deployment)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  filename = xbt_new(char,strlen(project) + strlen(MAKEFILE_FILENAME));
  sprintf(filename,MAKEFILE_FILENAME, project);
  
  OUT=fopen(filename,"w");
  xbt_assert1(OUT, "Unable to open %s for writing",filename);

  fprintf(OUT, "# AUTOMAKE variable definition\n");
  fprintf(OUT, "INCLUDES= @CFLAGS_SimGrid@\n\n");
  fprintf(OUT, "PROGRAMS=");
  fprintf(OUT, SIM_BINARYNAME,project);

  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, " ");
    fprintf(OUT, RL_BINARYNAME, project, key);
  }

  fprintf(OUT, "\n\n");
  fprintf(OUT, SIM_FILENAME_SOURCES,project);
  fprintf(OUT, "=\t");
  fprintf(OUT, SIM_FILENAME,project);
  fprintf(OUT, " %s.c\n", project);
  fprintf(OUT, SIM_FILENAME_LDADD, project);
  fprintf(OUT, "=\tpath/to/libsimgrid.a\n\n");

  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_FILENAME_SOURCES, project,key);
    fprintf(OUT, "=\t");
    fprintf(OUT, RL_FILENAME, project,key);
    fprintf(OUT, " %s.c\n", project);
    fprintf(OUT, RL_FILENAME_LDADD, project, key);
    fprintf(OUT, "=\tpath/to/libgras.a\n\n");
  }

  fprintf(OUT, "\n# cleanup temps\n");
  fprintf(OUT, "CLEANFILES= ");
  fprintf(OUT, SIM_FILENAME, project);
  
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, " ");
    fprintf(OUT, RL_FILENAME, project,key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT, "\n# generate temps\n");
  fprintf(OUT, "\n# A rule to generate the source file each time the deployment file changes\n");

  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_FILENAME, project,key);
    fprintf(OUT, " ");
  }
  fprintf(OUT, SIM_FILENAME, project);
  fprintf(OUT, ": %s\n", deployment);
  fprintf(OUT, "\tstub_generator %s %s >/dev/null\n", project, filename);
}

int main(int argc, char *argv[])
{
  char *project_name = NULL;
  char *deployment_file = NULL;

  surf_init(&argc, argv);

  xbt_assert1((argc ==3),"Usage: %s project_name deployment_file\n",argv[0]);

  project_name = argv[1];
  deployment_file = argv[2];

  process_function_set = xbt_dict_new();

  STag_process_fun = parse_process_init;
  surf_parse_open(deployment_file);
  if(surf_parse()) xbt_assert1(0,"Parse error in %s",deployment_file);
  surf_parse_close();

  warning = xbt_new(char,strlen(WARN)+strlen(deployment_file)+10);
  sprintf(warning,WARN,deployment_file);

  generate_sim(project_name);
  generate_rl(project_name);
  generate_makefile(project_name, deployment_file);

  xbt_free(warning);
  return 0;
}
