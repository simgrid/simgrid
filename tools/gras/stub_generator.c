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

#include <stdarg.h>



XBT_LOG_NEW_DEFAULT_SUBCATEGORY(stubgen,gras,"Stub generator");

#ifdef _WIN32

/* tabulation level (used to indent the lines of the borland project file */
static unsigned int level = 0;



/* the gras.h header directory */
static char* __gras_path = NULL;


#ifndef MAX_PATH
#define MAX_PATH 260
#endif

/*
 * A structure which represents a borland project file.
 */
typedef struct s_borland_project
{
    const char* xml_version;/* the xml version used to write the borland project file                   */
    const char* encoding;   /* the encoding used to write the borland project file                      */
    const char* comment;    /* the xml comment to write at the begining of the borland project file     */
	char* name;		        /* the name of the borland project file 									*/
	FILE* stream;			/* the stream to the borland project file									*/
	const char* version;	/* the builder version of the borland project file							*/
	char* bin_dir;          /* the directory used to store the generated program						*/
	char* obj_dir;	        /* the directory used to store the generated object files					*/
	char* lib_dir;	        /* the directory used to store the librairies used in the borland project	*/
	char* src_dir;	        /* the directory use to store the source files of the project				*/
}s_borland_project_t,* borland_project_t;


/*
 * A structure which represents a visual C++ project file.
 */
typedef struct s_dsp
{
	FILE* stream;
	char* lib_dir;
	char* src_dir;
	char* name;
}s_dsp_t,* dsp_t;

/*
 * Write tabs in a borland project file.
 * @param project The project concerned by the operation.
 * @param count The count tab to write
 */
void
borland_project_write_tabs(borland_project_t project,unsigned int count);

/*
 * Write the begin of an xml node in the borland project file.
 * @param project The borland project concerned by this operation.
 * @param name The name of the node.
 */
void
borland_project_begin_xml_node(borland_project_t project, const char* name);


/*
 * Write the end of an xml node in the borland project file.
 * @param project The borland project concerned by this operation.
 * @param name The name of the node.
 */
void
borland_project_end_xml_node(borland_project_t project, const char* name);

/*
 * Write an xml element in a borland project file.
 * @param project The borland project concerned by this operation.
 * @param name The name of the element to write
 * @param value The value of the element  
 */
void
borland_project_write_xml_element(borland_project_t project,const char* name,const char* value);

/*
 * Write a FILE xml element in the borland project file.
 * @param project The borland project concerned by this operation.
 * @param file_name The value of the attribute FILENAME.
 * @param form_name The value of the attribute FORMNAME.
 * @param unit_name The value of the attribute UNITNAME.
 * @param container_id The value of the attribute CONTAINERID.
 * @param design_claas The value of the attribute DESIGNCLASS.
 * @param local_command The value of the attribute LOCALCOMMAND.
 */
void
borland_project_write_file_element(	borland_project_t project,
									const char* file_name,
									const char* form_name,
									const char* unit_name,
									const char* container_id,
									const char* design_class,
									const char* local_command);
/*
 * Write all options of the IDE of the Borland Builder C++ compiler.
 * @ param project The project concerned by this operation.
 */

void
borland_project_write_ide_options(borland_project_t project);

/*
 * Write the xml header of the xml document.
 * @param project The project concerned by the operation.
 */
void
borland_project_write_xml_header(borland_project_t project);

/*
 * Write an xml comment in a borland project file
 * @param project The project concerned by this operation.
 */
void
borland_project_write_xml_comment(borland_project_t project);

/*
 * Create a bpf file used by a borland project file.
 * @param name The name of the bpf file to create.
 */
void
borland_project_create_main_file(const char* name);

/*
 * Create a borland project file.
 * @param project The project concerned by this operation.
 */
void
borland_project_create(borland_project_t project);

/*
 * Close a borland project file.
 * @param project the borland project file to close.
 */
void
borland_project_close(borland_project_t project);


/*
 * Generate a borland simulation poject file.
 * @param name The name of the simulation project
 */
void
generate_borland_simulation_project(const char* name);

/*
 * Generate a borland project file for real life.
 * @param name The name of the project to create.
 */
void
generate_borland_real_life_project(const char* name);

/*
 * Generate a borland project file.
 * @param project The borland project to generate.
 */
void
generate_borland_project(borland_project_t project,int is_rl,const char* name);

/*
 * Find the path of a file.
 * @param file_name The file name to find.
 * @path path If founded this parameter will contain the path of file.
 * @return If successful the function returns 1. Otherwise the function
 *  retruns 0;
 */
int
find_file_path(const char* root_dir,const char* file_name,char* path);


/*
 * Functions used to create a Microsoft Visual C++ project file (*.dsp).
 */

/*
 * Create the Microsoft visual C++ file project for the simulation.
 */
int
generate_simulation_dsp_file(const char* project_name);

/*
 * Create the Microsoft visual C++ real life project file.
 */
int
generate_real_live_dsp_file(const char* project_name);

/*
 * generate a Microsoft Visual project file*/
void
generate_dsp_project(dsp_t project,int is_rl,const char* name);

#endif



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

static s_process_t process;

/*
 * Création de deux dictionnaires
 */
static void parse_process_init(void)
{
  xbt_dict_set(process_function_set, A_surfxml_process_function, NULL, NULL);
  xbt_dict_set(machine_set, A_surfxml_process_host, NULL, NULL);
  process.argc = 1 ;
  process.argv = xbt_new(char*,1);
  process.argv[0] = xbt_strdup(A_surfxml_process_function);
  process.host=strdup(A_surfxml_process_host);
  /*VERB1("Function: %s",A_surfxml_process_function);*/
}

static void parse_argument(void)
{
  process.argc++;
  process.argv = xbt_realloc(process.argv, (process.argc) * sizeof(char *));
  process.argv[(process.argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

static void parse_process_finalize(void)
{
  xbt_dynar_push(process_list,&process);
  /*VERB1("Function: %s",process.argv[0]);*/
}

static void generate_sim(char *project)
{
	xbt_dict_cursor_t cursor=NULL;
	char *key = NULL;
	void *data = NULL;
	char *filename = NULL;
	FILE *OUT = NULL;
	
	/* 
	 * Creation d'un fichier nommé : <projet>_simulator.c 
	 */
 

	filename = xbt_new(char,strlen(project) + strlen(SIM_SOURCENAME));
	sprintf(filename,SIM_SOURCENAME,project);
	
	OUT=fopen(filename,"w");
	
	
	xbt_assert1(OUT, "Unable to open %s for writing",filename);
	
	/*
	 * Ecriture du message d'avertissement.
	 */
	fprintf(OUT, "%s\n",warning);
	
	/*
	 * Ecriture du préambule (inclusion de certains fichiers d'en-tête
	 */
	fprintf(OUT, "%s", SIM_PREEMBULE);
	
	/*
	 * Déclaration des fonction int <process>(int argc,char *argv[]);
	 */
	xbt_dict_foreach(process_function_set,cursor,key,data) {
		fprintf(OUT,"int %s(int argc,char *argv[]);\n",key);
	}
	
	fprintf(OUT,"\n");
	
	/*
	 * Déclaration des fonction int launch_<process>(int argc,char *argv[]);
	 */
	xbt_dict_foreach(process_function_set,cursor,key,data) {
	fprintf(OUT,"int launch_%s(int argc,char *argv[]);\n",key);
	}
	
	/*
	 * Ecriture du message d'avertissement.
	 */
	fprintf(OUT, "\n%s\n",warning);

	xbt_dict_foreach(process_function_set,cursor,key,data) {
		fprintf(OUT,SIM_LAUNCH_FUNC,key,key);
	}
	fprintf(OUT, "\n%s\n",warning);

    fprintf(OUT,"%s", "/* specific to Borland Compiler */\n"
    "#ifdef __BORLANDDC__\n"
    "#pragma argsused\n"
    "#endif\n\n");
	
	fprintf(OUT, "%s", "int main (int argc,char *argv[]) {\n"
	"\n" 
	"  /*  Simulation setup */\n" 
	"  MSG_global_init(&argc,argv);\n" 
	"  if (argc != 3) {\n" 
	"    fprintf(stderr, \"Usage: %s platform.xml deployment.xml [--gras-log=...]\\n\",argv[0]);\n" 
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
    fprintf(OUT, "/* specific to Borland Compiler */\n" \
                 "#ifdef __BORLANDC__\n" \
                 "#pragma hdrstop\n" \
                 "#endif\n\n" \
                 "#include <stdio.h>\n" \
                 "#include <signal.h>\n" \
                 "#include <gras.h>\n" \
                 "\n" \
                 "extern const char * XBT_PUBLIC_DATA _gras_procname;\n" \
                 "/* user code */\n" \
                 "int %s(int argc, char *argv[]);\n" \
                 "\n" \
                 "/* specific to Borland Compiler */\n" \
                "#ifdef __BORLANDC__\n" \
                "#pragma argsused\n" \
                "#endif\n\n" \
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
  fprintf(OUT, "\tgras_stub_generator %s %s >/dev/null\n", project, deployment);
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
   
  fprintf(OUT,
	  "\n"
	  "####\n"
	  "#### THIS FILE WAS GENERATED, DO NOT EDIT BEFORE RENAMING IT\n"
	  "####\n\n\n");

  fprintf(OUT,"## Variable declarations\n"
              "PROJECT_NAME=%s\n"
	      "DISTDIR=gras-$(PROJECT_NAME)\n\n"
	  ,project);
   
  fprintf(OUT,
	  "# Set the GRAS_ROOT environment variable to the path under which you installed SimGrid\n"
	  "# Compilation will fail if you don't do so\n" 
	  "GRAS_ROOT?= $(shell echo \"\\\"<<<< GRAS_ROOT undefined !!! >>>>\\\"\")\n\n"
	  "# You can fiddle the following to make it fit your taste\n"
	  "INCLUDES = -I$(GRAS_ROOT)/include\n"
	  "CFLAGS ?= -O3 -w -g\n"
	  "LIBS_SIM = -lm  -L$(GRAS_ROOT)/lib/ -lsimgrid\n"
	  "LIBS_RL = -lm  -L$(GRAS_ROOT)/lib/ -lgras\n"
	  "LIBS = \n"
	  "\n");

  fprintf(OUT, "PRECIOUS_C_FILES ?= %s.c\n", project);

  fprintf(OUT, "GENERATED_C_FILES = ");
  fprintf(OUT, SIM_SOURCENAME" ",project);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_SOURCENAME " ",project, key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT,"OBJ_FILES = $(patsubst %%.c,%%.o,$(PRECIOUS_C_FILES))\n");

  fprintf(OUT, "BIN_FILES = ");

  fprintf(OUT, SIM_BINARYNAME " ",project);
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_BINARYNAME " ", project, key);
  }
  fprintf(OUT, "\n");

  fprintf(OUT,
	  "\n"
	  "## By default, build all the binaries\n"
	  "all: $(BIN_FILES)\n"
	  "\n");
   
  fprintf(OUT, "\n## generate temps: regenerate the source file each time the deployment file changes\n");
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_SOURCENAME, project,key);
    fprintf(OUT, " ");
  }
  fprintf(OUT, SIM_SOURCENAME, project);
  fprintf(OUT, ": %s\n", deployment);
  fprintf(OUT, "\tgras_stub_generator %s %s >/dev/null\n", project, deployment);

  fprintf(OUT, "\n## Generate the binaries\n");
  fprintf(OUT, SIM_BINARYNAME ": " SIM_OBJNAME " $(OBJ_FILES)\n",project, project);
  fprintf(OUT, "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_SIM) $(LIBS) $(LDADD) -o $@ \n");
  xbt_dict_foreach(process_function_set,cursor,key,data) {
    fprintf(OUT, RL_BINARYNAME " : " RL_OBJNAME " $(OBJ_FILES)\n", project, key, project, key);
    fprintf(OUT, "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_RL) $(LIBS) $(LDADD) -o $@ \n");
  }  
  fprintf(OUT, 
	  "\n"
	  "%%: %%.o\n"
	  "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@ \n"
	  "\n"
	  "%%.o: %%.c\n"
	  "\t$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<\n"
	  "\n");
   
  fprintf(OUT,
	  "##Â Rules for tarballs and cleaning\n"
	  "DIST_FILES= $(EXTRA_DIST) $(GENERATED_C_FILES) $(PRECIOUS_C_FILES) "MAKEFILE_FILENAME_LOCAL" " /*MAKEFILE_FILENAME_REMOTE*/"\n"
	  "distdir: $(DIST_FILES)\n"
	  "\trm -rf $(DISTDIR)\n"
	  "\tmkdir -p $(DISTDIR)\n"
	  "\tcp $^ $(DISTDIR)\n"
	  "\n"
	  "dist: clean distdir\n"
	  "\ttar c $(DISTDIR) | gzip -c9 > $(DISTDIR).tar.gz\n"
	  "\n", project /*, project*/);

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


#ifdef _WIN32
#include <windows.h>
#endif

/* specific to Borland Compiler */
#ifdef __BORLANDDC__
#pragma argsused
#endif

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
     if(surf_parse()) 
     	xbt_assert1(0,"Parse error in %s",deployment_file);
     
     surf_parse_close();
  }


  warning = xbt_new(char,strlen(WARN)+strlen(deployment_file)+10);
  sprintf(warning,WARN,deployment_file);

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
  }*/

  generate_sim(project_name);
  generate_rl(project_name);
  generate_makefile_local(project_name, deployment_file);
  #ifdef _WIN32
  generate_borland_simulation_project(project_name);
  generate_borland_real_life_project(project_name);
  generate_simulation_dsp_file(project_name);
  generate_real_live_dsp_file(project_name);

  if(__gras_path)
    xbt_free(__gras_path);
  

  #endif
//  generate_makefile_remote(project_name, deployment_file);
//  generate_deployment(project_name, deployment_file);

  free(warning);
  surf_exit();
  return 0;
}
#ifdef _WIN32
void
generate_borland_project(borland_project_t project,int is_rl,const char* name)
{
	char* binary_path;	/* the path of the generated binary file				*/
	char* obj_path;		/* the path of the generated object file 				*/
	char* lib_files;	/* a list of the libraries used in the borland project	*/
	char* main_source;	/* the name of the bpf file used by the borland project */
	char* file_name;	/* the file name of the main source file				*/
    char* include_path; /* the include path                                     */
	char* buffer;

    /* create the borland project file */
    borland_project_create(project);

    /* write the xml document header */
    borland_project_write_xml_header(project);

    /* write the xml comment to identify a borland project file */
    borland_project_write_xml_comment(project);
	
	/* write the begin of the node PROJECT */
	borland_project_begin_xml_node(project,"PROJECT");
	
	/* write the begin of node MACROS */
	borland_project_begin_xml_node(project,"MACROS");
	
	/* write the borland project version */
	borland_project_write_xml_element(project,"VERSION",project->version);
	
	/* construct and write the borland project binary path */
	binary_path = xbt_new0(char,strlen(project->name) + strlen(project->bin_dir) + 6);
	sprintf(binary_path,"%s\\%s.exe",project->bin_dir,project->name);
	borland_project_write_xml_element(project,"PROJECT",binary_path);
	xbt_free(binary_path);
	
	/* construct an write the object files to generate by the compiler */
	obj_path = xbt_new0(char,strlen(project->name) + strlen(name) + (2*strlen(project->obj_dir)) + 11);
	sprintf(binary_path,"%s\\%s.obj\n%s\\%s.obj",project->obj_dir,project->name,project->obj_dir,name);
	borland_project_write_xml_element(project,"OBJFILES",obj_path);
	xbt_free(obj_path);
	
	/* write the resource files used by the compiler (no resource file used) */
	borland_project_write_xml_element(project,"RESFILES","");
	
	/* write the IDL files of the project (no IDL files used) */
	borland_project_write_xml_element(project,"IDLFILES","");
	
	/* write the IDLGENFILES element (not used) */
	borland_project_write_xml_element(project,"IDLGENFILES","");
	
	/* write the DEFFILE element (not used) */
	borland_project_write_xml_element(project,"DEFFILE","");
	
	/* write the RESDEPEN element (not used, no resource file used) */
	borland_project_write_xml_element(project,"RESDEPEN","$(RESFILES)");

	/* construct and write the list of libraries used by the project */
	/*
    lib_files = xbt_new0(char,(2 * (strlen(project->lib_dir) + 1)) + strlen("simgrid.lib") + strlen("libgras.lib") + 3);
	sprintf(lib_files,"%s\\simgrid.lib %s\\libgras.lib",project->lib_dir,project->lib_dir);
    */
	
	if(is_rl){
		lib_files = xbt_new0(char,(2 * (strlen(project->lib_dir) + 1)) + strlen("libgras.lib") + 2);
		sprintf(lib_files,"%s\\libgras.lib",project->lib_dir);	
	}else{
    	lib_files = xbt_new0(char,(2 * (strlen(project->lib_dir) + 1)) + strlen("simgrid.lib") + 2);
		sprintf(lib_files,"%s\\simgrid.lib",project->lib_dir);
	}

	borland_project_write_xml_element(project,"LIBFILES",lib_files);
	xbt_free(lib_files);
	
	/* write the SPARELIBS element (not used) */
	borland_project_write_xml_element(project,"SPARELIBS","");
	
	/* write the PACKAGES element (no package used) */
	borland_project_write_xml_element(project,"PACKAGES","");
	
	/* write the PATHCPP element (the path of the source files of the project)*/
	borland_project_write_xml_element(project,"PATHCPP",".;");
	
	/* write the PATHPAS element (not used) */
	borland_project_write_xml_element(project,"PATHPAS","");
	
	/* write the PATHRC element (not used) */
	borland_project_write_xml_element(project,"PATHRC","");
	
	/* write the PATHASM element (not used) */
	borland_project_write_xml_element(project,"PATHASM","");
	
	/* write the DEBUGLIBPATH element (the path for debug) */
	borland_project_write_xml_element(project,"DEBUGLIBPATH","$(BCB)\\lib\\debug");
	
	/* write the RELEASELIBPATH element (the path for release) */
	borland_project_write_xml_element(project,"RELEASELIBPATH","$(BCB)\\lib\\release");
	
	/* specify the linker to use */
	borland_project_write_xml_element(project,"LINKER","ilink32");
	
	/* write the USERDEFINES element (user definitions (#define _DEBUG))*/
	borland_project_write_xml_element(project,"USERDEFINES","_DEBUG");
	
	/* write the SYSDEFINES element (use the system definitions, not used strict ANSI and no use the VCL)*/
	borland_project_write_xml_element(project,"SYSDEFINES","NO_STRICT;_NO_VCL");

	/* construct and write the MAINSOURCE element */
	main_source = xbt_new0(char,strlen(project->name) + 5);
	sprintf(main_source,"%s.bpf",project->name);
    /* write the main source file to use in the borland project file */
	borland_project_write_xml_element(project,"MAINSOURCE",main_source);

	/* create the bpf file used by the borland project */
	borland_project_create_main_file(main_source);

	/* FIXME resolve the include path */
	/* write the INCLUDEPATH element  */

    if(!__gras_path){
    	buffer =xbt_new0(char,MAX_PATH);
    	GetEnvironmentVariable("SG_INSTALL_DIR",buffer,MAX_PATH);
    	
		__gras_path = xbt_new0(char,MAX_PATH);
		sprintf(__gras_path,"%s\\simgrid\\include",buffer);
		free(buffer);

        /*find_file_path("C:\\","gras.h",__gras_path);*/
    }

    include_path = xbt_new0(char,strlen("$(BCB)\\include") + strlen(__gras_path) + 2);
    sprintf(include_path,"$(BCB)\\include;%s",__gras_path);

	borland_project_write_xml_element(project,"INCLUDEPATH",include_path);

    xbt_free(include_path);

	/* write the LIBPATH element (no librarie paths specified)*/
	borland_project_write_xml_element(project,"LIBPATH","$(BCB)\\lib;$(BCB)\\lib\\obj");

	/*
     * write the WARNINGS element :
     *  -w-sus (-w-8075) disabled the suspect conversion pointer warning
     *  -w-rvl (-w-8070) disabled the function must return warning
     *  -w-rch (-w-8066) disabled the warning which specify that we can't attain the code
     *  -w-pia (-w-8060) disabled the warning that detect a possible bad assignement
     *  -w-pch (-w-8058) disabled the warning throw when the compiler can't precompile a header file
     *  -w-par (-w-8057) disabled the warning that detect an unused variable
     *  -w-csu (-w-8012) disabled the warning that detect a comparison between a signed number and an unsigned number
     *  -w-ccc (-w-8008) disabled the warning that detect une conditon which is always true
     *  -w-aus (-w-8008) disabled the warning that detect an affected value which is never used
     */
	borland_project_write_xml_element(project,"WARNINGS","-w-sus -w-rvl -w-rch -w-pia -w-pch -w-par -w-csu -w-ccc -w-aus");

	/* write the OTHERFILES element (no other files used used) */
	borland_project_write_xml_element(project,"OTHERFILES","");

	/* write the end of the node MACROS */
	borland_project_end_xml_node(project,"MACROS");

	/* write the begin of the node OPTIONS */
	borland_project_begin_xml_node(project,"OPTIONS");

	/* FIXME check the idlcflags */
	/* write the IDLCFLAGS element */
	borland_project_write_xml_element(project,"IDLCFLAGS","");

	/*
     * write the CFLAG1 element (compiler options ) :
     *
     *  -Od     this flag disable all compiler optimisation
     *  -H      this flag specify the name of the file which will contain the precompiled header
     *  -Hc     this flag specify to the compiler to cach the precompiled header
     *  -Vx     this flag specify to the compiler to allow the empty structures
     *  -Ve     this flag specify to the compiler to allow the empty base classes
     *  -X-     this flag activate the auto-depend informations of the compiler
     *  -r-     this flag disabled the use of the processor register
     *  -a1     this flag specify that the data must be aligned on one byte
     *  -b-     this flag specify that the enums are the smallest size of the integer
     *  -k      (used during debug)
     *  -y      this flag generate the line number for debug
     *  -v      this flag enabled the debugging of the source files
     *  -vi     check the expansion of the inline functions
     *  -tWC    specify that it's a console application
     *  -tWM-   specify that the application is not multithread
     *  -c      generate the object file, no link
     */
	borland_project_write_xml_element(project,"CFLAG1","-Od -H=$(BCB)\\lib\\vcl60.csm -Hc -Vx -Ve -X- -r- -a8 -b- -k -y -v -vi- -tWC -tWM- -c");

	/* write the PFLAGS element */
	borland_project_write_xml_element(project,"PFLAGS","-N2obj -N0obj -$YD -$W -$O- -$A8 -v -JPHNE ");

	/* write the RFLAGS element */
	borland_project_write_xml_element(project,"RFLAGS","");

	/* write the AFLAGS element (assembler flags) :
     *
     *  /mx (not documented)
     *  /w2 (not documented)
     *  /zd (not documented)
     *
     */
	borland_project_write_xml_element(project,"AFLAGS","/mx /w2 /zd");

	/* write the LFLAGS element (linker flags) :
     *
     *  -I      specify the output directory for object files
     *  -D      register the specified description (no description "")
     *  -ap     build a win32 console application
     *  -Tpe    generate a exe file
     *  -x      do not create the map file
     *  -Gn     do not generate the state file
     *  -v      include the complete debug informations  
     */
	borland_project_write_xml_element(project,"LFLAGS","-Iobj -D&quot;&quot; -ap -Tpe -x -Gn -v");

	/* write the OTHERFILES element (not used)*/
	borland_project_write_xml_element(project,"OTHERFILES","");

	/* write the end of the node OPTIONS */
	borland_project_end_xml_node(project,"OPTIONS");

	/* write the begin of the node LINKER */
	borland_project_begin_xml_node(project,"LINKER");

	/* write the ALLOBJ element */
	borland_project_write_xml_element(project,"ALLOBJ","c0x32.obj $(OBJFILES)");

	/* write the ALLRES element (not used) */
	borland_project_write_xml_element(project,"ALLRES","");

	/* write the ALLLIB element */
	borland_project_write_xml_element(project,"ALLLIB","$(LIBFILES) $(LIBRARIES) import32.lib cw32.lib");

	/* write the OTHERFILES element (not used) */
	borland_project_write_xml_element(project,"OTHERFILES","");

	/* write the end of the node LINKER */
	borland_project_end_xml_node(project,"LINKER");

	/* write the begin of the node FILELIST */
	borland_project_begin_xml_node(project,"FILELIST");
	
	/* construct and write the list of file elements */
	
	/* add the bpf file to the list */
	borland_project_write_file_element(project,main_source,"",project->name,"BPF","","");
	xbt_free(main_source);
	
	/* FIXME : check the source file directory */
	/* add the generated source file to the list */
	
	file_name = xbt_new0(char,strlen(project->src_dir) + strlen(project->name) + 5);
	sprintf(file_name,"%s\\_%s.c",project->src_dir,project->name);
	borland_project_write_file_element(project,file_name,"",project->name,"CCompiler","","");

	memset(file_name,0,strlen(project->src_dir) + strlen(project->name) + 4);
	sprintf(file_name,"%s\\%s.c",project->src_dir,name);
	borland_project_write_file_element(project,file_name,"",name,"CCompiler","","");

	xbt_free(file_name);

	/* FIXME : check the libraries directory */
	/* add the simgrid library to the list */
	
	if(is_rl){
		file_name = xbt_new0(char,strlen(project->lib_dir) + strlen("libgras.lib") + 2);
		sprintf(file_name,"%s\\libgras.lib",project->lib_dir);
		borland_project_write_file_element(project,file_name,"","libgras.lib","LibTool","","");
	}else
	{
		file_name = xbt_new0(char,strlen(project->lib_dir) + strlen("simgrid.lib") + 2);
		sprintf(file_name,"%s\\simgrid.lib",project->lib_dir);
		borland_project_write_file_element(project,file_name,"","simgrid.lib","LibTool","","");
	}
	
	
	xbt_free(file_name);

	/* write the end of the node FILELIST */
	borland_project_end_xml_node(project,"FILELIST");
	
	/* write the begin of the node BUILDTOOLS (not used)*/
	borland_project_begin_xml_node(project,"BUILDTOOLS");
	
	/* write the end of the node BUILDTOOLS (not used)*/
	borland_project_end_xml_node(project,"BUILDTOOLS");
	
	/* write the begin of the node IDEOPTIONS */
	borland_project_begin_xml_node(project,"IDEOPTIONS");
	
	/* write all of the option of the IDE of Borland C++ Builder */
	borland_project_write_ide_options(project);
	
	/* write the end of the node IDEOPTIONS */
	borland_project_end_xml_node(project,"IDEOPTIONS");

    /* write the end of the node PROJECT */
	borland_project_end_xml_node(project,"PROJECT");

    /* close the borland project file */
    borland_project_close(project);
}

void
borland_project_write_tabs(borland_project_t project,unsigned int count)
{
	unsigned int pos;
	
	for(pos = 0; pos < count; pos++)
		fprintf(project->stream,"\t");		
}

void
borland_project_begin_xml_node(borland_project_t project, const char* name)
{
	if(level)
		borland_project_write_tabs(project,level);
		
	fprintf(project->stream,"<%s>\n",name);
	
	level++;
}

void
borland_project_end_xml_node(borland_project_t project, const char* name)
{
	level--;
	
	if(level)
		borland_project_write_tabs(project,level);
	
	fprintf(project->stream,"</%s>\n",name);	
}


void
borland_project_write_xml_element(borland_project_t project,const char* name,const char* value)
{
	borland_project_write_tabs(project,level);
	fprintf(project->stream,"<%s value=\"%s\"/>\n",name,value);
}

void
borland_project_write_xml_header(borland_project_t project)
{
    fprintf(project->stream,"<?xml version='%s' encoding='%s' ?>\n",project->xml_version, project->encoding);
}

void
borland_project_create_main_file(const char* name)
{
	FILE* stream = fopen(name,"w+");

	fprintf(stream,"Ce fichier est uniquement utilisé par le gestionnaire de projets et doit être traité comme le fichier projet\n\n\nmain\n");

	fclose(stream);
}

void
borland_project_create(borland_project_t project)
{
    char* file_name = xbt_new0(char,strlen(project->name ) + 5);
    sprintf(file_name,"%s.bpr",project->name);
    project->stream = fopen(file_name,"w+");
    xbt_free(file_name);
}

void
borland_project_write_xml_comment(borland_project_t project)
{
    fprintf(project->stream,"<!-- %s -->\n",project->comment);
}
void
borland_project_write_file_element(	borland_project_t project,
									const char* file_name,
									const char* form_name,
									const char* unit_name,
									const char* container_id,
									const char* design_class,
									const char* local_command)
{
	borland_project_write_tabs(project,level);

	fprintf(project->stream,"<FILE FILENAME=\"%s\" FORMNAME=\"%s\" UNITNAME=\"%s\" CONTAINERID=\"%s\" DESIGNCLASS=\"%s\" LOCALCOMMAND=\"%s\"/>\n",file_name,form_name,unit_name,container_id,design_class,local_command);
}

void
borland_project_write_ide_options(borland_project_t project)
{

	const char* ide_options =
        "[Version Info]\nIncludeVerInfo=0\nAutoIncBuild=0\nMajorVer=1\nMinorVer=0\nRelease=0\nBuild=0\nDebug=0\nPreRelease=0\nSpecial=0\nPrivate=0\nDLL=0\nLocale=1036\nCodePage=1252\n\n"  \
        "[Version Info Keys]\nCompanyName=\nFileDescription=\nFileVersion=1.0.0.0\nInternalName=\nLegalCopyright=\nLegalTrademarks=\nOriginalFilename=\nProductName=\nProductVersion=1.0.0.0\nComments=\n\n" \
        "[Excluded Packages]\n$(BCB)\\dclclxdb60.bpl=Composants BD CLX Borland\n$(BCB)\\Bin\\dclclxstd60.bpl=Composants Standard CLX Borland\n\n" \
        "[HistoryLists\\hlIncludePath]\nCount=1\nItem0=$(BCB)\\include;$(BCB)\\include\\vcl;\n\n" \
        "[HistoryLists\\hlLibraryPath]\nCount=1\nItem0=$(BCB)\\lib\\obj;$(BCB)\\lib\n\n" \
        "[HistoryLists\\hlDebugSourcePath]\nCount=1\nItem0=$(BCB)\\source\\vcl\\\n\n" \
        "[HistoryLists\\hlConditionals]\nCount=1\nItem0=_DEBUG\n\n" \
        "[HistoryLists\\hlIntOutputDir]\nCount=0\n\n" \
        "[HistoryLists\\hlFinalOutputDir]\nCount=0\n\n" \
        "[HistoryLists\\hIBPIOutputDir]\nCount=0\n\n" \
        "[Debugging]\nDebugSourceDirs=$(BCB)\\source\\vcl\n\n" \
        "[Parameters]\nRunParams=\nLauncher=\nUseLauncher=0\nDebugCWD=\nHostApplication=\nRemoteHost=\nRemotePath=\nRemoteLauncher=\nRemoteCWD=\nRemoteDebug=0\n\n" \
        "[Compiler]\nShowInfoMsgs=0\nLinkDebugVcl=0\nLinkCGLIB=0\n\n" \
        "[CORBA]\nAddServerUnit=1\nAddClientUnit=1\nPrecompiledHeaders=1\n\n" \
        "[Language]\nActiveLang=\nProjectLang=\nRootDir=\n";

	fprintf(project->stream,ide_options);
}

void
borland_project_close(borland_project_t project)
{
    fclose(project->stream);
}

void
generate_borland_simulation_project(const char* name)
{
    char buffer[MAX_PATH] = {0};

    HANDLE hDir;
    WIN32_FIND_DATA wfd = {0};

    s_borland_project_t borland_project = {0};
    borland_project.xml_version = "1.0";
    borland_project.encoding ="utf-8";
    borland_project.comment ="C++Builder XML Project";
    borland_project.version = "BCB.06.00";

    borland_project.lib_dir = xbt_new0(char,MAX_PATH);

    
    GetEnvironmentVariable("LIB_SIMGRID_PATH",borland_project.lib_dir,MAX_PATH);
		

    GetCurrentDirectory(MAX_PATH,buffer);

    borland_project.src_dir = xbt_new0(char,strlen(buffer) + 1);

    strcpy(borland_project.src_dir,buffer);

    borland_project.name = xbt_new0(char,strlen(name) + strlen("simulator") + 2);
    sprintf(borland_project.name,"%s_simulator",name);

    borland_project.bin_dir = xbt_new0(char,strlen(buffer) + strlen("\\bin") + 1);
    sprintf(borland_project.bin_dir,"%s\\bin",buffer);

    hDir = FindFirstFile(borland_project.bin_dir,&wfd);

    if(!hDir)
        CreateDirectory(borland_project.bin_dir,NULL);

    borland_project.obj_dir = xbt_new0(char,strlen(buffer) + strlen("\\obj") + 1);
    sprintf(borland_project.obj_dir,"%s\\obj",buffer);

    hDir = FindFirstFile(borland_project.obj_dir,&wfd);

    if(INVALID_HANDLE_VALUE == hDir)
        CreateDirectory(borland_project.obj_dir,NULL);

    generate_borland_project(&borland_project,0,name);

    xbt_free(borland_project.name);
    xbt_free(borland_project.src_dir);
    xbt_free(borland_project.bin_dir);
    xbt_free(borland_project.obj_dir);
    xbt_free(borland_project.lib_dir);
}

void
generate_borland_real_life_project(const char* name)
{
    HANDLE hDir;
    WIN32_FIND_DATA wfd = {0};
    char buffer[MAX_PATH] = {0};
    xbt_dict_cursor_t cursor = NULL;
    char *key = NULL;
    void *data = NULL;
	s_borland_project_t borland_project = {0};

	borland_project.xml_version = "1.0";
    borland_project.encoding ="utf-8";
    borland_project.comment ="C++Builder XML Project";
    borland_project.version = "BCB.06.00";
    borland_project.lib_dir = " ";

    borland_project.lib_dir = xbt_new0(char,MAX_PATH);

    GetEnvironmentVariable("LIB_GRAS_PATH",borland_project.lib_dir,MAX_PATH);
       
    GetCurrentDirectory(MAX_PATH,buffer);

    borland_project.src_dir = xbt_new0(char,strlen(buffer) + 1);

    strcpy(borland_project.src_dir,buffer);

    borland_project.bin_dir = xbt_new0(char,strlen(buffer) + strlen("\\bin") + 1);
    sprintf(borland_project.bin_dir,"%s\\bin",buffer);

    hDir = FindFirstFile(borland_project.bin_dir,&wfd);

    if(INVALID_HANDLE_VALUE == hDir)
        CreateDirectory(borland_project.bin_dir,NULL);

    borland_project.obj_dir = xbt_new0(char,strlen(buffer) + strlen("\\obj") + 1);
    sprintf(borland_project.obj_dir,"%s\\obj",buffer);

    hDir = FindFirstFile(borland_project.obj_dir,&wfd);

    if(!hDir)
        CreateDirectory(borland_project.obj_dir,NULL);


	xbt_dict_foreach(process_function_set,cursor,key,data) {
        borland_project.name = xbt_new0(char,strlen(name) + strlen(key) + 2);

        sprintf(borland_project.name,"%s_%s",name,key);

        generate_borland_project(&borland_project,1,name);
        xbt_free(borland_project.name);
    }

    xbt_free(borland_project.src_dir);
    xbt_free(borland_project.bin_dir);
    xbt_free(borland_project.obj_dir);
    xbt_free(borland_project.lib_dir);
}
int
find_file_path(const char* root_dir,const char* file_name,char* path)
{
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	char* prev_dir = xbt_new(char,MAX_PATH);
	GetCurrentDirectory(MAX_PATH,prev_dir);
	SetCurrentDirectory(root_dir);
	
	// begining of the scan
	hFind=FindFirstFile ("*.*", &wfd);
	
	if(hFind!=INVALID_HANDLE_VALUE){
	
		/* it's a file */
		if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		
			if(!strcmp(file_name,wfd.cFileName)){
				GetCurrentDirectory(MAX_PATH,path);
				SetCurrentDirectory(prev_dir);
				xbt_free(prev_dir);
				FindClose(hFind);
				return 1;
			}
		
		}
		/* it's a directory, scan it*/
		else {
		
			if(strcmp(wfd.cFileName,".") && strcmp(wfd.cFileName,"..")){
				if(find_file_path(wfd.cFileName,file_name,path)){
					FindClose(hFind);
					SetCurrentDirectory(prev_dir);
					return 1;
				}
			}
		}
		
		/* next file or directory */
		while(FindNextFile(hFind,&wfd))
		{
			/* it's a file */
			if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if(!strcmp(file_name,wfd.cFileName)){
					GetCurrentDirectory(MAX_PATH,path);
					SetCurrentDirectory(prev_dir);
					xbt_free(prev_dir);
					FindClose(hFind);
					return 1;
				}
			}
			/* it's a file scan it */
			else {
		
				if(strcmp(wfd.cFileName,".") && strcmp(wfd.cFileName,"..")){
		
					if(find_file_path(wfd.cFileName,file_name,path)){
						SetCurrentDirectory(prev_dir);
						FindClose(hFind);
						return 1;
					}
		
				}
		
			}
		}
	}
	
	SetCurrentDirectory(prev_dir);
	xbt_free(prev_dir);
	FindClose (hFind);
	return 0;
}

/* Implementation of the functions used to create a Visual C++ project.*/

int
generate_simulation_dsp_file(const char* name)
{
	char buffer[MAX_PATH] = {0};
    s_dsp_t dsp = {0};
    dsp.lib_dir = xbt_new0(char,MAX_PATH);

    GetEnvironmentVariable("LIB_SIMGRID_PATH",dsp.lib_dir,MAX_PATH);
    GetCurrentDirectory(MAX_PATH,buffer);

    dsp.src_dir = xbt_new0(char,strlen(buffer) + 1);
    strcpy(dsp.src_dir,buffer);
	dsp.name = xbt_new0(char,strlen(name) + strlen("simulator") + 2);
	sprintf(dsp.name,"%s_simulator",name);

    generate_dsp_project(&dsp,0,name);

    xbt_free(dsp.name);
    xbt_free(dsp.src_dir);
	xbt_free(dsp.lib_dir);
    
    return 0;		
}

/*
 * Create the Microsoft visual C++ real life project file.
 */
int
generate_real_live_dsp_file(const char* name)
{
	
    char buffer[MAX_PATH] = {0};
    xbt_dict_cursor_t cursor = NULL;
    char *key = NULL;
    void *data = NULL;
	s_dsp_t dsp = {0};

	
    dsp.lib_dir = xbt_new0(char,MAX_PATH);

    GetEnvironmentVariable("LIB_GRAS_PATH",dsp.lib_dir,MAX_PATH);
       
    GetCurrentDirectory(MAX_PATH,buffer);

    dsp.src_dir = xbt_new0(char,strlen(buffer) + 1);

    strcpy(dsp.src_dir,buffer);


	xbt_dict_foreach(process_function_set,cursor,key,data) {
        dsp.name = xbt_new0(char,strlen(name) + strlen(key) + 2);

        sprintf(dsp.name,"%s_%s",name,key);

        generate_dsp_project(&dsp,1,name);
        xbt_free(dsp.name);
    }

    xbt_free(dsp.src_dir);
    xbt_free(dsp.lib_dir);
	return 0;
}

void
generate_dsp_project(dsp_t project,int is_rl,const char* name)
{
	/* create the visual C++ project file */
	char* buffer;
	char* file_name = xbt_new0(char,strlen(project->name) + 5);
    sprintf(file_name,"%s.dsp",project->name);
    project->stream = fopen(file_name,"w+");
    xbt_free(file_name);
    
    if(!__gras_path)
	{
    	buffer =xbt_new0(char,MAX_PATH);
    	GetEnvironmentVariable("SG_INSTALL_DIR",buffer,MAX_PATH);
    	
		__gras_path = xbt_new0(char,MAX_PATH);
		sprintf(__gras_path,"%s\\simgrid\\include",buffer);
		free(buffer);
    }
    
    /* dsp file header */
    fprintf(project->stream,"# Microsoft Developer Studio Project File - Name=\"%s\" - Package Owner=<4>\n",project->name);
   	fprintf(project->stream,"# Microsoft Developer Studio Generated Build File, Format Version 6.00\n");
   	fprintf(project->stream,"# ** DO NOT EDIT **\n\n");
   	
   	/* target type is a win32 x86 console application */
   	fprintf(project->stream,"# TARGTYPE \"Win32 (x86) Console Application\" 0x0103\n\n");
   	
   	/* the current config is win32-debug */
   	fprintf(project->stream,"CFG=%s - Win32 Debug\n",project->name);
   	
   	/* warning */
   	fprintf(project->stream,"!MESSAGE This is not a valid makefile. To build this project using NMAKE,\n");
   	
   	/* NMAKE usage */
   	fprintf(project->stream,"!MESSAGE use the Export Makefile command and run\n");
   	fprintf(project->stream,"!MESSAGE\n");
   	fprintf(project->stream,"!MESSAGE NMAKE /f \"%s.mak\".\n",project->name);
   	fprintf(project->stream,"!MESSAGE\n");
   	fprintf(project->stream,"!MESSAGE You can specify a configuration when running NMAKE\n");
	fprintf(project->stream,"!MESSAGE by defining the macro CFG on the command line. For example:\n");
	fprintf(project->stream,"!MESSAGE\n"); 
	fprintf(project->stream,"!MESSAGE NMAKE /f \"%s.mak\" CFG=\"%s - Win32 Debug\"\n",project->name,project->name);
	fprintf(project->stream,"!MESSAGE\n");
	fprintf(project->stream,"!MESSAGE Possible choices for configuration are:\n");
	fprintf(project->stream,"!MESSAGE\n"); 
	fprintf(project->stream,"!MESSAGE \"%s - Win32 Release\" (based on \"Win32 (x86) Console Application\")\n",project->name);
	fprintf(project->stream,"!MESSAGE \"%s - Win32 Debug\" (based on \"Win32 (x86) Console Application\")\n",project->name);
	fprintf(project->stream,"!MESSAGE\n\n"); 
   	
	fprintf(project->stream,"# Begin Project\n\n");
	fprintf(project->stream,"# PROP AllowPerConfigDependencies 0\n");
	fprintf(project->stream,"# PROP Scc_ProjName\n");
	fprintf(project->stream,"# PROP Scc_LocalPath\n");
	fprintf(project->stream,"CPP=cl.exe\n");
	fprintf(project->stream,"RSC=rc.exe\n\n");
   	
   	fprintf(project->stream,"!IF  \"$(CFG)\" == \"%s - Win32 Release\"\n\n",project->name);
   	
	fprintf(project->stream,"# PROP BASE Use_MFC 0\n");
	fprintf(project->stream,"# PROP BASE Use_Debug_Libraries 0\n");
	fprintf(project->stream,"# PROP BASE Output_Dir \"Release\"\n");
	fprintf(project->stream,"# PROP BASE Intermediate_Dir \"Release\"\n");
	fprintf(project->stream,"# PROP BASE Target_Dir \"\"\n");
	fprintf(project->stream,"# PROP Use_MFC 0\n");
	fprintf(project->stream,"# PROP Use_Debug_Libraries 0\n");
	fprintf(project->stream,"# PROP Output_Dir \"Release\"\n");
	fprintf(project->stream,"# PROP Intermediate_Dir \"Release\"\n");
	fprintf(project->stream,"# PROP Target_Dir \"\"\n");
	/* TODO : the include directory */
	/*fprintf(project->stream,"# ADD BASE CPP /nologo /W3 /GX /O2 /I \"./%s\" /D \"WIN32\" /D \"NDEBUG\" /D \"_CONSOLE\" /D \"_MBCS\" /YX /FD /c\n",__gras_path);*/
	fprintf(project->stream,"# ADD BASE CPP /nologo /W3 /GX /O2 /I \"%s\" /D \"WIN32\" /D \"NDEBUG\" /D \"_CONSOLE\" /D \"_MBCS\" /YX /FD /c\n",__gras_path);
	fprintf(project->stream,"# ADD CPP /nologo /W3 /GX /O2 /D \"WIN32\" /D \"NDEBUG\" /D \"_CONSOLE\" /D \"_MBCS\" /YX /FD /c\n");
	fprintf(project->stream,"# ADD BASE RSC /l 0x40c /d \"NDEBUG\"\n");
	fprintf(project->stream,"# ADD RSC /l 0x40c /d \"NDEBUG\n");
	fprintf(project->stream,"BSC32=bscmake.exe\n");
	fprintf(project->stream,"# ADD BASE BSC32 /nologo\n");
	fprintf(project->stream,"# ADD BSC32 /nologo\n");
	fprintf(project->stream,"LINK32=link.exe\n");
	fprintf(project->stream,"# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386\n");
	
	if(is_rl)
		fprintf(project->stream,"# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libgras.lib /nologo /subsystem:console /machine:I386\n\n");
	else
		fprintf(project->stream,"# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib simgrid.lib /nologo /subsystem:console /machine:I386\n\n");
			
	fprintf(project->stream,"!ELSEIF  \"$(CFG)\" == \"%s - Win32 Debug\"\n",project->name);

	fprintf(project->stream,"# PROP BASE Use_MFC 0\n");
	fprintf(project->stream,"# PROP BASE Use_Debug_Libraries 1\n");
	fprintf(project->stream,"# PROP BASE Output_Dir \"Debug\"\n");
	fprintf(project->stream,"# PROP BASE Intermediate_Dir \"Debug\"\n");
	fprintf(project->stream,"# PROP BASE Target_Dir \"\"\n");
	fprintf(project->stream,"# PROP Use_MFC 0\n");
	fprintf(project->stream,"# PROP Use_Debug_Libraries 1\n");
	fprintf(project->stream,"# PROP Output_Dir \"Debug\"\n");
	fprintf(project->stream,"# PROP Intermediate_Dir \"Debug\"\n");
	fprintf(project->stream,"# PROP Ignore_Export_Lib 0\n");
	fprintf(project->stream,"# PROP Target_Dir \"\"\n");
	/* TODO : the include directory */
	/*fprintf(project->stream,"# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od  /I \"./%s\" /D \"WIN32\" /D \"_DEBUG\" /D \"_CONSOLE\" /D \"_MBCS\" /YX /FD /GZ  /c\n",__gras_path);*/
	fprintf(project->stream,"# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od  /I \"%s\" /D \"WIN32\" /D \"_DEBUG\" /D \"_CONSOLE\" /D \"_MBCS\" /YX /FD /GZ  /c\n",__gras_path);
	fprintf(project->stream,"# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D \"WIN32\" /D \"_DEBUG\" /D \"_CONSOLE\" /D \"_MBCS\" /YX /FD /GZ  /c\n");
	fprintf(project->stream,"# ADD BASE RSC /l 0x40c /d \"_DEBUG\"\n");
	fprintf(project->stream,"# ADD RSC /l 0x40c /d \"_DEBUG\"\n");
	fprintf(project->stream,"BSC32=bscmake.exe\n");
	fprintf(project->stream,"# ADD BASE BSC32 /nologo\n");
	fprintf(project->stream,"# ADD BSC32 /nologo\n");
	fprintf(project->stream,"LINK32=link.exe\n");
	fprintf(project->stream,"# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept\n");
	
	if(is_rl)
		fprintf(project->stream,"# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib simgrid.lib  libgras.lib  /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept\n\n");
	else
		fprintf(project->stream,"# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib simgrid.lib  simgrid.lib  /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept\n\n");
			
	fprintf(project->stream,"!ENDIF\n\n");
	
	fprintf(project->stream,"# Begin Target\n\n");
	fprintf(project->stream,"# Name \"%s - Win32 Release\"\n",project->name);
	fprintf(project->stream,"# Name \"%s - Win32 Debug\"\n",project->name);
	fprintf(project->stream,"# Begin Group \"Source Files\"\n\n");
	fprintf(project->stream,"# PROP Default_Filter \"cpp;c;cxx;rc;def;r;odl;idl;hpj;bat\"\n\n");
	
	fprintf(project->stream,"# Begin Source File\n");
	fprintf(project->stream,"SOURCE=%s\\_%s.c\n",project->src_dir,project->name);
	fprintf(project->stream,"# End Source File\n\n");
	
	fprintf(project->stream,"# Begin Source File\n");
	fprintf(project->stream,"SOURCE=%s\\%s.c\n",project->src_dir,name);
	fprintf(project->stream,"# End Source File\n\n");
	
	fprintf(project->stream,"# End Group\n");
	fprintf(project->stream,"# Begin Group \"Header Files\"\n\n");
	fprintf(project->stream,"# PROP Default_Filter \"h;hpp;hxx;hm;inl\"\n");
	fprintf(project->stream,"# End Group\n");
	fprintf(project->stream,"# Begin Group \"Resource Files\"\n\n");
	fprintf(project->stream,"# PROP Default_Filter \"ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe\"\n");
	fprintf(project->stream,"# End Group\n");
	fprintf(project->stream,"# End Target\n");
	fprintf(project->stream,"# End Project\n");
   	
}

#endif





