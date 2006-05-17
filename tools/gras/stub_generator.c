/* 	$Id$	 */

/* gras_stub_generator - creates the main() to use a GRAS program           */

/* Copyright (c) 2003,2004,2005 Martin Quinson, Arnaud Legrand. 
   All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "surf/surfxml_parse.h"
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
#define DEPLOYMENT  "%s.deploy.sh"

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
/********* Parse XML deployment file **********/
/**********************************************/
xbt_dict_t process_function_set = NULL;
xbt_dynar_t process_list = NULL;
xbt_dict_t machine_set = NULL;

typedef struct s_process_t {
  int argc;
  char **argv;
  char *host;
} s_process_t;

static void s_process_free(void *process)
{
  int i;
  for(i=0;i<((s_process_t*)process)->argc; i++)
    free(((s_process_t*)process)->argv[i]);
  free(((s_process_t*)process)->host);
}

static int parse_argc = -1 ;
static char **parse_argv = NULL;

static void parse_process_init(void)
{
  parse_argc = 0 ;
  parse_argv = NULL;
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_surfxml_process_function);
}

static void parse_argument(void)
{
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

static void parse_process_finalize(void)
{
  s_process_t process;
  void *p = (void *) 1234;

  xbt_dict_set(process_function_set, A_surfxml_process_function, p, NULL);
  xbt_dict_set(machine_set, A_surfxml_process_host, p, NULL);
  process.argc=parse_argc;
  process.argv=parse_argv;
  process.host=strdup(A_surfxml_process_host);
  xbt_dynar_push(process_list,&process);
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

  fprintf(OUT, "%s", "int main (int argc,char *argv[]) {\n" 
                     "\n" 
	             "  /*  Simulation setup */\n" 
                     "  MSG_global_init(&argc,argv);\n" 
                     "  if (argc != 3) {\n" 
                     "    fprintf(stderr, \"Usage: %s platform_file application_description.txt [--gras-log=...]\\n\",argv[0]);\n" 
                     "    exit(1);\n" 
                     "  }\n"
                     "\n");
   fprintf(OUT, 
	   "  MSG_paje_output(\"%s.trace\");\n" 
	   "  MSG_set_channel_number(XBT_MAX_CHANNEL); /* Using at most 10 channel (ports) per host. Change it here if needed */\n" 
	   "  MSG_create_environment(argv[1]);\n" 
	   "\n" 
	   "  /*  Application deployment */\n",
	   project);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT,"  MSG_function_register(\"%s\", launch_%s);\n",key,key);
  }
  fprintf(OUT, "%s", SIM_MAIN_POSTEMBULE);
  fclose(OUT);
  free(filename);
}

/**********************************************/
/**** Generate the file for the real life *****/
/**********************************************/
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
    fprintf(OUT, "#include <stdio.h>\n" \
                 "#include <signal.h>\n" \
                 "#include <gras.h>\n" \
                 "\n" \
                 "extern const char *_gras_procname;\n" \
                 "/* user code */\n" \
                 "int %s(int argc, char *argv[]);\n" \
                 "\n" \
                 "int main(int argc, char *argv[]){\n" \
                 "  int errcode;\n" \
                 "\n" \
                 "  _gras_procname = \"%s\";\n" \
                 "  errcode=%s(argc,argv);\n"\
                 " \n" \
                 "  return errcode;\n"\
                 "}\n",
	    key,key,key);
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

static void generate_makefile_local(char *project, char *deployment)
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
  free(filename);
   
  fprintf(OUT, "############ PROJECT COMPILING AND ARCHIVING #########\n");
  fprintf(OUT, "PROJECT_NAME=%s\n",project);
  fprintf(OUT, 
	  "DISTDIR=gras-$(PROJECT_NAME)\n\n"
	  "GRAS_ROOT?= $(shell echo \"\\\"<<<< GRAS_ROOT undefined !!! >>>>\\\"\")\n"
	  "CFLAGS = -O3 -w -g\n"
	  "INCLUDES = -I$(GRAS_ROOT)/include\n"
	  "LIBS_SIM = -lm  -L$(GRAS_ROOT)/lib/ -lsimgrid\n"
	  "LIBS_RL = -lm  -L$(GRAS_ROOT)/lib/ -lgras\n"
	  "LIBS = \n"
	  "\n");

  fprintf(OUT, "C_FILES = ");
  fprintf(OUT, SIM_SOURCENAME" ",project);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_SOURCENAME " ",project, key);
  }
  fprintf(OUT, "%s.c\n", project);

  fprintf(OUT,"OBJ_FILES = \n");

  fprintf(OUT, "BIN_FILES = ");

  fprintf(OUT, SIM_BINARYNAME " ",project);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_BINARYNAME " ", project, key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT,
	  "\n"
	  "all: $(BIN_FILES)\n"
	  "\n");
  fprintf(OUT, SIM_BINARYNAME ": " SIM_OBJNAME " %s.o\n",project, project, project);
  fprintf(OUT, "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_SIM) $(LIBS) $(LDADD) -o $@ \n");
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_BINARYNAME " : " RL_OBJNAME " %s.o\n", project, key, project, key, project);
    fprintf(OUT, "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_RL) $(LIBS) $(LDADD) -o $@ \n");
  }
  fprintf(OUT, 
	  "\n"
	  "%%: %%.o\n"
	  "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@ \n"
	  "\n"
	  "%%.o: %%.c\n"
	  "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<\n"
	  "\n"
	  "DIST_FILES= $(C_FILES) "MAKEFILE_FILENAME_LOCAL" "MAKEFILE_FILENAME_REMOTE"\n"
	  "distdir: $(DIST_FILES)\n"
	  "\trm -rf $(DISTDIR)\n"
	  "\tmkdir -p $(DISTDIR)\n"
	  "\tcp $^ $(DISTDIR)\n"
	  "\n"
	  "dist: clean distdir\n"
	  "\ttar c $(DISTDIR) | gzip -c > $(DISTDIR).tar.gz\n"
	  "\n", project, project);

  fprintf(OUT,
	  "clean:\n"
	  "\trm -f $(BIN_FILES) $(OBJ_FILES) *~ %s.o " SIM_OBJNAME, project, project);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
     fprintf(OUT, " " RL_OBJNAME, project, key);
  }
  fprintf(OUT,   
	  "\n"
	  "\trm -rf $(DISTDIR)\n"
	  "\n"
	  ".SUFFIXES:\n"
	  ".PHONY : clean\n"
	  "\n");
   
  fprintf(OUT, "############ REMOTE COMPILING #########\n");
  fprintf(OUT, 
	  "MACHINES ?= ");
  xbt_dict_foreach(machine_set,cursor,key,data) {
    fprintf(OUT, "%s ",key);
  }
  fprintf(OUT,"\n");

  fprintf(OUT, 
	  "INSTALL_PATH ?='$$HOME/tmp/src' ### Has to be an absolute path !!! \n"
	  "GRAS_ROOT ?='$(INSTALL_PATH)' ### Has to be an absolute path !!! \n"
	  "SRCDIR ?= ./\n"
	  "SIMGRID_URL ?=http://gcl.ucsd.edu/simgrid/dl/\n"
	  "SIMGRID_VERSION ?=2.92\n"
	  "GRAS_PROJECT ?= %s\n"
	  "GRAS_PROJECT_URL ?= http://www-id.imag.fr/Laboratoire/Membres/Legrand_Arnaud/gras_test/\n"
	  "\n"
	  "remote:\n"
	  "\t@echo;echo \"----[ Compile the package on remote hosts ]----\"\n"
	  "\t@test -e $(SRCDIR)/buildlogs/ || mkdir -p $(SRCDIR)/buildlogs/\n"
	  "\t for site in $(MACHINES) ; do \\\n"
	  "\t   machine=`echo $$site |sed 's/^\\([^%%]*\\)%%.*$$/\\1/'`;\\\n"
	  "\t   machine2=`echo $$site |sed 's/^\\([^%%]*\\)%%\\(.*\\)$$/\\2/'`;\\\n"
	  "\t   cmd_mkdir=\"\\\"sh -c 'env INSTALL_PATH=$(INSTALL_PATH) GRAS_ROOT=$(GRAS_ROOT) \\\n"
	  "\t                        SIMGRID_URL=$(SIMGRID_URL) SIMGRID_VERSION=$(SIMGRID_VERSION) GRAS_PROJECT=$(GRAS_PROJECT) \\\n"
	  "\t                        GRAS_PROJECT_URL=$(GRAS_PROJECT_URL)  mkdir -p $(INSTALL_PATH) 2>&1'\\\"\";\\\n"
	  "\t   cmd_make=\"\\\"sh -c 'env INSTALL_PATH=$(INSTALL_PATH) GRAS_ROOT=$(GRAS_ROOT) \\\n"
	  "\t                        SIMGRID_URL=$(SIMGRID_URL) SIMGRID_VERSION=$(SIMGRID_VERSION) GRAS_PROJECT=$(GRAS_PROJECT) \\\n"
	  "\t                        GRAS_PROJECT_URL=$(GRAS_PROJECT_URL)  make -C $(INSTALL_PATH) -f "MAKEFILE_FILENAME_REMOTE" $(ACTION) 2>&1'\\\"\";\\\n"
	  "\t   if echo $$site | grep  '%%' >/dev/null ; then \\\n"
	  "\t     echo \"----[ Compile on $$machine2 (behind $$machine) ]----\";\\\n"
	  "\t   else \\\n"
	  "\t     machine=$$site;\\\n"
	  "\t     echo \"----[ Compile on $$machine ]----\";\\\n"
	  "\t   fi;\\\n"
	  "\t   if echo $$site | grep  '%%' >/dev/null ; then \\\n"
	  "\t     if ssh $$machine \"ssh -A $$machine2 $$cmd_mkdir\" 2>&1 > $(SRCDIR)/buildlogs/$$site.log;\\\n"
	  "\t     then true; else failed=1;echo \"Failed (check $(SRCDIR)/buildlogs/$$site.log)\"; fi;\\\n"
	  "\t   else \\\n"
	  "\t     if ssh $$machine \"eval $$cmd_mkdir\" 2>&1 > $(SRCDIR)/buildlogs/$$site.log ;\\\n"
	  "\t     then true; else failed=1;echo \"Failed (check $(SRCDIR)/buildlogs/$$site.log)\"; fi; \\\n"
	  "\t   fi;\\\n"
	  "\t   echo \"-- Copy the data over\"; \\\n"
	  "\t   scp "MAKEFILE_FILENAME_REMOTE" $$site:$(INSTALL_PATH) ;\\\n"
	  "\t   echo \"-- Compiling... (the output gets into $(SRCDIR)/buildlogs/$$site.log)\"; \\\n"
	  "\t   if echo $$site | grep  '%%' >/dev/null ; then \\\n"
	  "\t     if ssh $$machine \"ssh -A $$machine2 $$cmd_make\" 2>&1 >> $(SRCDIR)/buildlogs/$$site.log;\\\n"
	  "\t     then echo \"Sucessful\"; else failed=1;echo \"Failed (check $(SRCDIR)/buildlogs/$$site.log)\"; fi;echo; \\\n"
	  "\t   else \\\n"
	  "\t     if ssh $$machine \"eval $$cmd_make\" 2>&1 >> $(SRCDIR)/buildlogs/$$site.log ;\\\n"
	  "\t     then echo \"Sucessful\"; else failed=1;echo \"Failed (check $(SRCDIR)/buildlogs/$$site.log)\"; fi;echo; \\\n"
	  "\t   fi;\\\n"
	  "\t done;\n",project,project,project);

  fclose(OUT);
}

static void generate_makefile_remote(char *project, char *deployment)
{
  char *filename = NULL;
  FILE *OUT = NULL;

  filename = xbt_new(char,strlen(project) + strlen(MAKEFILE_FILENAME_REMOTE));
  sprintf(filename,MAKEFILE_FILENAME_REMOTE, project);
  
  OUT=fopen(filename,"w");
  xbt_assert1(OUT, "Unable to open %s for writing",filename);

  fprintf(OUT, 
	  "INSTALL_PATH ?= $(shell pwd)\n"
	  "\n"
	  "compile-simgrid:\n"
	  "\tcd $$GRAS_ROOT ; \\\n"
	  "\tretrieved=`LANG=C;wget -N $(SIMGRID_URL)/simgrid-$(SIMGRID_VERSION).tar.gz 2>&1 | grep newer | sed 's/.*no newer.*/yes/'`; \\\n"
	  "\techo $$retrieved; \\\n"
	  "\tif test \"x$$retrieved\" = x; then \\\n"
	  "\t  tar zxf simgrid-$(SIMGRID_VERSION).tar.gz ; \\\n"
	  "\t  cd simgrid-$(SIMGRID_VERSION)/; \\\n"
	  "\t  ./configure --prefix=$$GRAS_ROOT ; \\\n"
	  "\t  make all install ;\\\n"
           "\tfi\n"
	  "\n"
	  "compile-gras: compile-simgrid\n"
	  "\tnot_retrieved=`LANG=C;wget -N $(GRAS_PROJECT_URL)/gras-$(GRAS_PROJECT).tar.gz 2>&1 | grep newer | sed 's/.*no newer.*/yes/'`; \\\n"
	  "\techo $$not_retrieved; \\\n"
	  "\tif test \"x$$not_retrieved\" != xyes; then \\\n"
	  "\t  tar zxf gras-$(GRAS_PROJECT).tar.gz ; \\\n"
	  "\t  make -C gras-$(GRAS_PROJECT)/ -f $(GRAS_PROJECT).Makefile.local all ; \\\n"
	  "\tfi\n"
	  "\n"
	  "clean-simgrid:\n"
	  "\trm -rf simgrid-$(SIMGRID_VERSION)*\n"
	  "clean-gras clean-gras-$(GRAS_PROJECT):\n"
	  "\trm -rf gras-$(GRAS_PROJECT)*\n"
	  "clean: clean-simgrid clean-gras-$(GRAS_PROJECT)\n"
	  "\n"
	  ".PHONY: clean clean-simgrid clean-gras clean-gras-$(GRAS_PROJECT) \\\n"
	  "        compile-simgrid compile-gras\n"
	  );
  fclose(OUT);
}


static void generate_deployment(char *project, char *deployment)
{
  xbt_dict_cursor_t cursor=NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  int cpt,i;
  s_process_t proc;

  filename = xbt_new(char,strlen(project) + strlen(DEPLOYMENT));
  sprintf(filename,DEPLOYMENT, project);
  
  OUT=fopen(filename,"w");
  xbt_assert1(OUT, "Unable to open %s for writing",filename);

  fprintf(OUT, "#!/bin/sh\n");
  fprintf(OUT, "############ DEPLOYMENT FILE #########\n");
  fprintf(OUT,
	  "if test \"${MACHINES+set}\" != set; then \n"
	  "    export MACHINES='");
  xbt_dict_foreach(machine_set,cursor,key,data) {
    fprintf(OUT, "%s ",key);
  }
  fprintf(OUT,  
	  "';\n"
	  "fi\n"
	  "if test \"${INSTALL_PATH+set}\" != set; then \n"
	  "    export INSTALL_PATH='`echo $HOME`/tmp/src'\n"
	  "fi\n"
	  "if test \"${GRAS_ROOT+set}\" != set; then \n"
	  "    export GRAS_ROOT='`echo $INSTALL_PATH`'\n"
	  "fi\n"
	  "if test \"${SRCDIR+set}\" != set; then \n"
	  "    export SRCDIR=./\n"
	  "fi\n"
	  "if test \"${SIMGRID_URL+set}\" != set; then \n"
	  "    export SIMGRID_URL=http://gcl.ucsd.edu/simgrid/dl/\n"
	  "fi\n"
	  "if test \"${SIMGRID_VERSION+set}\" != set; then \n"
	  "    export SIMGRID_VERSION=2.91\n"
	  "fi\n"
	  "if test \"${GRAS_PROJECT+set}\" != set; then \n"
	  "    export GRAS_PROJECT=%s\n"
	  "fi\n"
	  "if test \"${GRAS_PROJECT_URL+set}\" != set; then \n"
	  "    export GRAS_PROJECT_URL=http://www-id.imag.fr/Laboratoire/Membres/Legrand_Arnaud/gras_test/\n"
	  "fi\n"
	  "\n"
	  "test -e runlogs/ || mkdir -p runlogs/\n",
	  project);

  fprintf(OUT,  
	  "cmd_prolog=\"env INSTALL_PATH=$INSTALL_PATH GRAS_ROOT=$GRAS_ROOT \\\n"
	  "                 SIMGRID_URL=$SIMGRID_URL SIMGRID_VERSION=$SIMGRID_VERSION GRAS_PROJECT=$GRAS_PROJECT \\\n"
	  "                 GRAS_PROJECT_URL=$GRAS_PROJECT_URL LD_LIBRARY_PATH=$GRAS_ROOT/lib/ sh -c \";\n");

  xbt_dynar_foreach (process_list,cpt,proc) {
    fprintf(OUT,"cmd=\"\\$INSTALL_PATH/gras-%s/"RL_BINARYNAME" ",project,project,proc.argv[0]);
    for(i=1;i<proc.argc;i++) {
      fprintf(OUT,"%s ",proc.argv[i]);
    }
    fprintf(OUT,"\";\n");
    fprintf(OUT,"ssh %s \"$cmd_prolog 'export LD_LIBRARY_PATH=\\$INSTALL_PATH/lib:\\$LD_LIBRARY_PATH; echo \\\"$cmd\\\" ; $cmd 2>&1'\" > runlogs/%s_%d.log &\n",proc.host,proc.host,cpt);
  }

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
  int i;
   
  surf_init(&argc, argv);

  xbt_assert1((argc >= 3),"Usage: %s project_name deployment_file [deployment_file...]\n",argv[0]);

  project_name = argv[1];

  process_function_set = xbt_dict_new();
  process_list = xbt_dynar_new(sizeof(s_process_t),s_process_free);
  machine_set = xbt_dict_new();

  STag_surfxml_process_fun = parse_process_init;
  ETag_surfxml_argument_fun = parse_argument;
  ETag_surfxml_process_fun = parse_process_finalize;
  for(i=2; i<argc; i++) {
     deployment_file = argv[i];
     surf_parse_open(deployment_file);
     if(surf_parse()) xbt_assert1(0,"Parse error in %s",deployment_file);
     surf_parse_close();
  }


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
  generate_makefile_local(project_name, deployment_file);
  generate_makefile_remote(project_name, deployment_file);
  generate_deployment(project_name, deployment_file);

  free(warning);
  surf_exit();
  return 0;
}
