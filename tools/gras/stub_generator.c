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

#define WARN "/***********\n * DO NOT EDIT! THIS FILE HAS BEEN AUTOMATICALLY GENERATED FROM %s BY gras_stub_generator\n ***********/\n"
#define SIM_SOURCENAME  "_%s_simulator.c"
#define SIM_OBJNAME  "_%s_simulator.o"
#define SIM_BINARYNAME  "%s_simulator"
#define SIM_SOURCENAME_LDADD  "%s_simulator_LDADD"
#define SIM_SOURCENAME_SOURCES  "%s_simulator_SOURCES"
#define RL_SOURCENAME  "_%s_%s.c"
#define RL_OBJNAME  "_%s_%s.o"
#define RL_BINARYNAME  "%s_%s"
#define RL_SOURCENAME_LDADD  "%s_%s_LDADD"
#define RL_SOURCENAME_SOURCES  "%s_%s_SOURCES"
#define MAKEFILE_FILENAME_AM  "%s.Makefile.am"
#define MAKEFILE_FILENAME_LOCAL  "%s.Makefile.local"
#define MAKEFILE_FILENAME_REMOTE  "%s.Makefile.remote"

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
  void *p = (void *) 1234;
  xbt_dict_set(process_function_set, A_process_function, p, NULL);
}

static void generate_sim(char *project)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;
  filename = xbt_new(char,strlen(project) + strlen(SIM_SOURCENAME));
  sprintf(filename,SIM_SOURCENAME,project);
  
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
  free(filename);
}

static void generate_rl(char *project)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  xbt_dict_foreach(process_function_set,cursor,key,data) {
    filename = xbt_new(char,strlen(project) + strlen(RL_SOURCENAME) + strlen(key));
    sprintf(filename,RL_SOURCENAME,project,key);
  
    OUT=fopen(filename,"w");
    xbt_assert1(OUT, "Unable to open %s for writing",filename);

    fprintf(OUT, "\n%s\n",warning);
    fprintf(OUT, RL_CODE, key,key);
    fprintf(OUT, "\n%s\n",warning);
    fclose(OUT);
    free(filename);
  }
}

static void generate_makefile_am(char *project, char *deployment)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  filename = xbt_new(char,strlen(project) + strlen(MAKEFILE_FILENAME_AM));
  sprintf(filename,MAKEFILE_FILENAME_AM, project);
  
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
  fprintf(OUT, SIM_SOURCENAME_SOURCES,project);
  fprintf(OUT, "=\t");
  fprintf(OUT, SIM_SOURCENAME,project);
  fprintf(OUT, " %s.c\n", project);
  fprintf(OUT, SIM_SOURCENAME_LDADD, project);
  fprintf(OUT, "=\tpath/to/libsimgrid.a\n\n");

  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_SOURCENAME_SOURCES, project,key);
    fprintf(OUT, "=\t");
    fprintf(OUT, RL_SOURCENAME, project,key);
    fprintf(OUT, " %s.c\n", project);
    fprintf(OUT, RL_SOURCENAME_LDADD, project, key);
    fprintf(OUT, "=\tpath/to/libgras.a\n\n");
  }

  fprintf(OUT, "\n# cleanup temps\n");
  fprintf(OUT, "CLEANFILES= ");
  fprintf(OUT, SIM_SOURCENAME, project);
  
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, " ");
    fprintf(OUT, RL_SOURCENAME, project,key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT, "\n# generate temps\n");
  fprintf(OUT, "\n# A rule to generate the source file each time the deployment file changes\n");

  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_SOURCENAME, project,key);
    fprintf(OUT, " ");
  }
  fprintf(OUT, SIM_SOURCENAME, project);
  fprintf(OUT, ": %s\n", deployment);
  fprintf(OUT, "\tstub_generator %s %s >/dev/null\n", project, deployment);
  fclose(OUT);
}


static void generate_makefile(char *project, char *deployment)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  filename = xbt_new(char,strlen(project) + strlen(MAKEFILE_FILENAME_LOCAL));
  sprintf(filename,MAKEFILE_FILENAME_LOCAL, project);
  
  OUT=fopen(filename,"w");
  xbt_assert1(OUT, "Unable to open %s for writing",filename);

  fprintf(OUT, "PROJECT_NAME=%s\n",project);
  fprintf(OUT, 
	  "DISTDIR=gras-$(PROJECT_NAME)\n\n"
	  "SIMGRID_INSTALL_PATH?= $(shell echo \"\\\"<<<< SIMGRID_INSTALL_PATH undefined !!! >>>>\\\"\")\n"
	  "CFLAGS = -O3 -w\n"
	  "INCLUDES = -I$(SIMGRID_INSTALL_PATH)/include\n"
	  "LIBS = -lm  -L$(SIMGRID_INSTALL_PATH)/lib/ -lsimgrid\n"
	  "\n"
	  "C_FILES = _chrono_multiplier.c _chrono_simulator.c chrono.c\n"
	  "BIN_FILES = chrono_multiplier chrono_simulator\n"
	  "\n"
	  "all: $(BIN_FILES)\n"
	  "\n");
  fprintf(OUT, SIM_BINARYNAME ": " SIM_OBJNAME " %s.c\n",project, project, project);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_BINARYNAME " : " RL_OBJNAME " %s.c\n", project, key, project, key, project);
  }
  fprintf(OUT, 
	  "\n"
	  "%%: %%.o\n"
	  "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@ \n"
	  "\n"
	  "%%.o: %%.c\n"
	  "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<\n"
	  "\n"
	  "distdir: $(C_FILES) $(PROJECT_NAME).Makefile\n"
	  "\trm -rf $(DISTDIR)\n"
	  "\tmkdir -p $(DISTDIR)\n"
	  "\tcp $^ $(DISTDIR)\n"
	  "\n"
	  "dist: clean distdir\n"
	  "\ttar c $(DISTDIR) | gzip -c > $(DISTDIR).tar.gz\n"
	  "\n"
	  "clean:\n"
	  "\trm -f $(BIN_FILES) *.o *~\n"
	  "\trm -rf $(DISTDIR)\n"
	  "\n"
	  ".SUFFIXES:\n"
	  ".PHONY : clean\n");
  fclose(OUT);
}

static void print(void *p)
{
  printf("%p",p);
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

  if(XBT_LOG_ISENABLED(stubgen, xbt_log_priority_debug)) {
    xbt_dict_cursor_t cursor=NULL;
    char *key = NULL;
    void *data = NULL;

    for (cursor=NULL, xbt_dict_cursor_first((process_function_set),&(cursor)) ;
	 xbt_dict_cursor_get_or_free(&(cursor),&(key),(void**)(&data));
	 xbt_dict_cursor_step(cursor) ) {
      DEBUG1("Function %s", key);      
    }
    
    xbt_dict_dump(process_function_set,print);
  }

  generate_sim(project_name);
  generate_rl(project_name);
  generate_makefile_am(project_name, deployment_file);
  generate_makefile_local(project_name, deployment_file);

  free(warning);
  return 0;
}
