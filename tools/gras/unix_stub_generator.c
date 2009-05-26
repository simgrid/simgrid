/* 	$Id$	 */

/* gras_stub_generator - creates the main() to use a GRAS program           */

/* Copyright (c) 2003-2007 Martin Quinson, Arnaud Legrand, Malek Cherier.   */
/* All rights reserved.                                                     */

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


#ifdef _WIN32
/* stupid stubs so that it compiles on windows */
void generate_sim(char *project)
{
}

void generate_rl(char *project)
{
}

void generate_makefile_local(char *project, char *deployment)
{
}
#else
/* real implementation */
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(stubgen);


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
#define MAKEFILE_FILENAME_LOCAL  "%s.mk"
#define MAKEFILE_FILENAME_REMOTE  "%s.Makefile.remote"
#define DEPLOYMENT  "%s.deploy.sh"

char *warning = NULL;

/**********************************************/
/**** Generate the file for the simulator *****/
/**********************************************/

const char *SIM_PREEMBULE =
  "/* specific to Borland Compiler */\n"
  "#ifdef __BORLANDC__\n"
  "#pragma hdrstop\n"
  "#endif\n\n"
  "#include <stdlib.h>\n"
  "#include <stdio.h>\n"
  "#include \"msg/msg.h\"\n"
  "#include <gras.h>\n" "\n" "char *gras_log=NULL;\n";


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
  "  gras_launch_application(argv[2]);\n"
  "\n"
  "  /*  Run the simulation */\n"
  "  gras_main();\n"
  "\n"
  "  /* cleanup the place */\n"
  "  gras_clean();\n"
  "  if (gras_log)\n" "    free(gras_log);\n" "  return 0;\n" "}\n";




void generate_sim(char *project)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  /* Output file: <projet>_simulator.c */
  filename = xbt_new(char, strlen(project) + strlen(SIM_SOURCENAME));
  sprintf(filename, SIM_SOURCENAME, project);

  OUT = fopen(filename, "w");

  xbt_assert1(OUT, "Unable to open %s for writing", filename);

  fprintf(OUT, "%s\n", warning);
  fprintf(OUT, "%s", SIM_PREEMBULE);

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, "int %s(int argc,char *argv[]);\n", key);
  }

  fprintf(OUT, "\n");

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, "int launch_%s(int argc,char *argv[]);\n", key);
  }

  fprintf(OUT, "\n%s\n", warning);

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, SIM_LAUNCH_FUNC, key, key);
  }
  fprintf(OUT, "\n%s\n", warning);

  fprintf(OUT, "%s", "/* specific to Borland Compiler */\n"
          "#ifdef __BORLANDDC__\n" "#pragma argsused\n" "#endif\n\n");

  fprintf(OUT, "%s", "int main (int argc,char *argv[]) {\n"
          "\n"
          "  /*  Simulation setup */\n"
          "  gras_global_init(&argc,argv);\n"
          "  if (argc != 3) {\n"
          "    fprintf(stderr, \"Usage: %s platform.xml deployment.xml [--log=...]\\n\",argv[0]);\n"
          "    exit(1);\n" "  }\n" "\n");
  fprintf(OUT,
          "  gras_create_environment(argv[1]);\n"
          "\n" "  /*  Application deployment */\n");
  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, "  gras_function_register(\"%s\", launch_%s);\n", key, key);
  }
  fprintf(OUT, "%s", SIM_MAIN_POSTEMBULE);
  fclose(OUT);
  free(filename);
}

/**********************************************/
/**** Generate the file for the real life *****/
/**********************************************/
void generate_rl(char *project)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    filename =
      xbt_new(char, strlen(project) + strlen(RL_SOURCENAME) + strlen(key));

    sprintf(filename, RL_SOURCENAME, project, key);

    OUT = fopen(filename, "w");
    xbt_assert1(OUT, "Unable to open %s for writing", filename);

    fprintf(OUT, "\n%s\n", warning);
    fprintf(OUT, "/* specific to Borland Compiler */\n"
            "#ifdef __BORLANDC__\n"
            "#pragma hdrstop\n"
            "#endif\n\n"
            "#include <stdio.h>\n"
            "#include <signal.h>\n"
            "#include <gras.h>\n"
            "\n"
            "XBT_PUBLIC_DATA(const char *) _gras_procname;\n"
            "/* user code */\n"
            "int %s(int argc, char *argv[]);\n"
            "\n"
            "/* specific to Borland Compiler */\n"
            "#ifdef __BORLANDC__\n"
            "#pragma argsused\n"
            "#endif\n\n"
            "int main(int argc, char *argv[]){\n"
            "  int errcode;\n"
            "\n"
            "  _gras_procname = \"%s\";\n"
            "  errcode=%s(argc,argv);\n"
            " \n" "  return errcode;\n" "}\n", key, key, key);
    fprintf(OUT, "\n%s\n", warning);
    fclose(OUT);
    free(filename);
  }
}

void generate_makefile_am(char *project, char *deployment)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  filename = xbt_new(char, strlen(project) + strlen(MAKEFILE_FILENAME_AM));
  sprintf(filename, MAKEFILE_FILENAME_AM, project);

  OUT = fopen(filename, "w");
  xbt_assert1(OUT, "Unable to open %s for writing", filename);

  fprintf(OUT, "# AUTOMAKE variable definition\n");
  fprintf(OUT, "INCLUDES= @CFLAGS_SimGrid@\n\n");
  fprintf(OUT, "PROGRAMS=");
  fprintf(OUT, SIM_BINARYNAME, project);

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, " ");
    fprintf(OUT, RL_BINARYNAME, project, key);
  }

  fprintf(OUT, "\n\n");
  fprintf(OUT, SIM_SOURCENAME_SOURCES, project);
  fprintf(OUT, "=\t");
  fprintf(OUT, SIM_SOURCENAME, project);
  fprintf(OUT, " %s.c\n", project);
  fprintf(OUT, SIM_SOURCENAME_LDADD, project);
  fprintf(OUT, "=\tpath/to/libsimgrid.a\n\n");

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, RL_SOURCENAME_SOURCES, project, key);
    fprintf(OUT, "=\t");
    fprintf(OUT, RL_SOURCENAME, project, key);
    fprintf(OUT, " %s.c\n", project);
    fprintf(OUT, RL_SOURCENAME_LDADD, project, key);
    fprintf(OUT, "=\tpath/to/libgras.a\n\n");
  }

  fprintf(OUT,
          "\n# cleanup temps (allowing the user to add extra clean files)\n");
  fprintf(OUT, "CLEANFILES?= \n");
  fprintf(OUT, "CLEANFILES+= ");
  fprintf(OUT, SIM_SOURCENAME, project);

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, " ");
    fprintf(OUT, RL_SOURCENAME, project, key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT, "\n# generate temps\n");
  fprintf(OUT,
          "\n# A rule to generate the source file each time the deployment file changes\n");

  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, RL_SOURCENAME, project, key);
    fprintf(OUT, " ");
  }
  fprintf(OUT, SIM_SOURCENAME, project);
  fprintf(OUT, ": %s\n", deployment);
  fprintf(OUT, "\tgras_stub_generator %s %s >/dev/null\n", project,
          deployment);
  fclose(OUT);
}

void generate_makefile_local(char *project, char *deployment)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key = NULL;
  void *data = NULL;
  char *filename = NULL;
  FILE *OUT = NULL;

  filename = xbt_new(char, strlen(project) + strlen(MAKEFILE_FILENAME_LOCAL));
  sprintf(filename, MAKEFILE_FILENAME_LOCAL, project);

  OUT = fopen(filename, "w");
  xbt_assert1(OUT, "Unable to open %s for writing", filename);
  free(filename);

  fprintf(OUT,
          "\n"
          "####\n"
          "#### THIS FILE WAS GENERATED, DO NOT EDIT BEFORE RENAMING IT\n"
          "####\n\n\n");

  fprintf(OUT, "## Variable declarations\n"
          "PROJECT_NAME=%s\n" "DISTDIR=gras-$(PROJECT_NAME)\n\n", project);

  fprintf(OUT,
          "# Set the GRAS_ROOT environment variable to the path under which you installed SimGrid\n"
          "# Compilation will fail if you don't do so\n"
          "GRAS_ROOT?= $(shell if [ -e /usr/local/lib/libgras.so ] ; then echo /usr/local ; else echo \"\\\"<<<< GRAS_ROOT undefined !!! >>>>\\\"\"; fi)\n\n"
          "# You can fiddle the following to make it fit your taste\n"
          "INCLUDES = -I$(GRAS_ROOT)/include\n"
          "CFLAGS ?= -O3 -w -g -Wall\n"
          "LIBS_SIM = -lm  -L$(GRAS_ROOT)/lib/ -lsimgrid\n"
          "LIBS_RL = -lm  -L$(GRAS_ROOT)/lib/ -lgras\n" "LIBS = \n" "\n");

  fprintf(OUT, "PRECIOUS_C_FILES ?= %s.c\n", project);

  fprintf(OUT, "GENERATED_C_FILES = ");
  fprintf(OUT, SIM_SOURCENAME " ", project);
  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, RL_SOURCENAME " ", project, key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT, "OBJ_FILES = $(patsubst %%.c,%%.o,$(PRECIOUS_C_FILES))\n");

  fprintf(OUT, "BIN_FILES = ");

  fprintf(OUT, SIM_BINARYNAME " ", project);
  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, RL_BINARYNAME " ", project, key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT,
          "\n"
          "## By default, build all the binaries\n"
          "all: $(BIN_FILES)\n" "\n");

  fprintf(OUT,
          "\n## generate temps: regenerate the source file each time the deployment file changes\n");
  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, RL_SOURCENAME, project, key);
    fprintf(OUT, " ");
  }
  fprintf(OUT, SIM_SOURCENAME, project);
  fprintf(OUT, ": %s\n", deployment);
  fprintf(OUT, "\tgras_stub_generator %s %s >/dev/null\n", project,
          deployment);

  fprintf(OUT, "\n## Generate the binaries\n");
  fprintf(OUT, SIM_BINARYNAME ": " SIM_OBJNAME " $(OBJ_FILES)\n", project,
          project);
  fprintf(OUT,
          "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_SIM) $(LIBS) $(LDADD) -o $@ \n");
  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, RL_BINARYNAME " : " RL_OBJNAME " $(OBJ_FILES)\n", project,
            key, project, key);
    fprintf(OUT,
            "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_RL) $(LIBS) $(LDADD) -o $@ \n");
  }
  fprintf(OUT,
          "\n"
          "%%: %%.o\n"
          "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@ \n"
          "\n"
          "%%.o: %%.c\n"
          "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<\n" "\n");

  fprintf(OUT,
          "## Rules for tarballs and cleaning\n"
          "DIST_FILES= $(EXTRA_DIST) $(GENERATED_C_FILES) $(PRECIOUS_C_FILES) "
          MAKEFILE_FILENAME_LOCAL " " /*MAKEFILE_FILENAME_REMOTE */ "\n"
          "distdir: $(DIST_FILES)\n" "\trm -rf $(DISTDIR)\n"
          "\tmkdir -p $(DISTDIR)\n" "\tcp $^ $(DISTDIR)\n" "\n"
          "dist: clean distdir\n"
          "\ttar c $(DISTDIR) | gzip -c9 > $(DISTDIR).tar.gz\n" "\n",
          project /*, project */ );

  fprintf(OUT,
          "clean:\n"
          "\trm -f $(CLEANFILES) $(BIN_FILES) $(OBJ_FILES) *~ %s.o "
          SIM_OBJNAME, project, project);
  xbt_dict_foreach(process_function_set, cursor, key, data) {
    fprintf(OUT, " " RL_OBJNAME, project, key);
  }
  fprintf(OUT,
          "\n"
          "\trm -rf $(DISTDIR)\n"
          "\n" ".SUFFIXES:\n" ".PHONY : clean\n" "\n");
  /* 
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
   */
  fclose(OUT);
}

static void print(void *p)
{
  printf("%p", p);
}
#endif
