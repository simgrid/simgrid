/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/misc.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_daxparse, sd,"Parsing DAX files");

#undef CLEANUP
#include "dax_dtd.h"
#include "dax_dtd.c"


/* Parsing helpers */
static void dax_parse_error(char *msg) {
  fprintf(stderr, "Parse error on line %d: %s\n", dax_lineno, msg);
  abort();
}
static double dax_parse_double(const char *string) {
  int ret = 0;
  double value;

  ret = sscanf(string, "%lg", &value);
  if (ret != 1)
    dax_parse_error(bprintf("%s is not a double", string));
  return value;
}
static int dax_parse_int(const char *string) {
  int ret = 0;
  int value;

  ret = sscanf(string, "%d", &value);
  if (ret != 1)
    dax_parse_error(bprintf("%s is not an integer", string));
  return value;
}

static YY_BUFFER_STATE input_buffer;

static xbt_dynar_t result;
static xbt_dict_t jobs;
static xbt_dict_t files;
static SD_task_t current_job;
static SD_task_t root_task,end_task;

static void dump_res() {
  unsigned int cursor;
  SD_task_t task;
  xbt_dynar_foreach(result,cursor,task) {
    INFO1("Task %d",cursor);
    SD_task_dump(task);
  }
}

static void dax_task_free(void*task){
  SD_task_t t=task;
  SD_task_destroy(t);
}

/** @brief loads a DAX file describing a DAG
 * 
 * See https://confluence.pegasus.isi.edu/display/pegasus/WorkflowGenerator
 * for more details.
 */
xbt_dynar_t SD_daxload(const char*filename) {
  xbt_dict_cursor_t cursor;
  SD_task_t file;
  char *name;
  FILE* in_file = fopen(filename,"r");
  xbt_assert1(in_file, "Unable to open \"%s\"\n", filename);
  input_buffer = dax__create_buffer(in_file, 10);
  dax__switch_to_buffer(input_buffer);
  dax_lineno = 1;

  result = xbt_dynar_new(sizeof(SD_task_t),dax_task_free);
  files=xbt_dict_new();
  jobs=xbt_dict_new();
  root_task = SD_task_create_comp_seq("root",NULL,0);
  xbt_dynar_push(result,&root_task);
  end_task = SD_task_create_comp_seq("end",NULL,0);

  xbt_assert2(!dax_lex(),"Parse error in %s: %s",filename,dax__parse_err_msg());
  dax__delete_buffer(input_buffer);
  fclose(in_file);
  xbt_dict_free(&jobs);

  /* And now, post-process the files.
   * We want a file task per pair of computation tasks exchanging the file. Duplicate on need
   * Files not produced in the system are said to be produced by root task (top of DAG).
   * Files not consumed in the system are said to be consumed by end task (bottom of DAG).
   */

  xbt_dict_foreach(files,cursor,name,file) {
    unsigned int cpt1,cpt2;
    SD_task_t newfile = NULL;
    SD_dependency_t depbefore,depafter;
    if (xbt_dynar_length(file->tasks_before) == 0) {
      xbt_dynar_foreach(file->tasks_after,cpt2,depafter) {
        SD_task_t newfile = SD_task_create_comm_e2e(file->name,NULL,file->amount);
        SD_task_dependency_add(NULL,NULL,root_task,newfile);
        SD_task_dependency_add(NULL,NULL,newfile,depafter->dst);
        xbt_dynar_push(result,&newfile);
      }
    } else if (xbt_dynar_length(file->tasks_after) == 0) {
      xbt_dynar_foreach(file->tasks_before,cpt2,depbefore) {
        SD_task_t newfile = SD_task_create_comm_e2e(file->name,NULL,file->amount);
        SD_task_dependency_add(NULL,NULL,depbefore->src,newfile);
        SD_task_dependency_add(NULL,NULL,newfile,end_task);
        xbt_dynar_push(result,&newfile);
      }
    } else {
      xbt_dynar_foreach(file->tasks_before,cpt1,depbefore) {
        xbt_dynar_foreach(file->tasks_after,cpt2,depafter) {
          if (depbefore->src == depafter->dst) {
            WARN2("File %s is produced and consumed by task %s. This loop dependency will prevent the execution of the task.",
                file->name,depbefore->src->name);
          }
          newfile = SD_task_create_comm_e2e(file->name,NULL,file->amount);
          SD_task_dependency_add(NULL,NULL,depbefore->src,newfile);
          SD_task_dependency_add(NULL,NULL,newfile,depafter->dst);
          xbt_dynar_push(result,&newfile);
        }
      }
    }
  }

  /* Push end task last */
  xbt_dynar_push(result,&end_task);

  /* Free previous copy of the files */
  xbt_dict_free(&files);

  return result;
}

void STag_dax__adag(void) {
  double version = dax_parse_double(A_dax__adag_version);

  xbt_assert1((version == 2.1), "Expected version 2.1 in <adag> tag, got %f. Fix the parser or your file",version);
}
void STag_dax__job(void) {
  double runtime = dax_parse_double(A_dax__job_runtime);
  char *name=bprintf("%s@%s",A_dax__job_id,A_dax__job_name);
  runtime*=4200000000.; /* Assume that timings were done on a 4.2GFlops machine. I mean, why not? */
//  INFO3("See <job id=%s runtime=%s %.0f>",A_dax__job_id,A_dax__job_runtime,runtime);
  current_job = SD_task_create_comp_seq(name,NULL,runtime);
  xbt_dict_set(jobs,A_dax__job_id,current_job,NULL);
  free(name);
  xbt_dynar_push(result,&current_job);
}
void STag_dax__uses(void) {
  SD_task_t file;
  double size = dax_parse_double(A_dax__uses_size);
  int is_input = (A_dax__uses_link == A_dax__uses_link_input);

//  INFO2("See <uses file=%s %s>",A_dax__uses_file,(is_input?"in":"out"));
  file = xbt_dict_get_or_null(files,A_dax__uses_file);
  if (file==NULL) {
    file = SD_task_create_comm_e2e(A_dax__uses_file,NULL,size);
    xbt_dict_set(files,A_dax__uses_file,file,&dax_task_free);
  } else {
    if (SD_task_get_amount(file)!=size) {
      WARN3("Ignoring file %s size redefinition from %.0f to %.0f",
          A_dax__uses_file,SD_task_get_amount(file),size);
    }
  }
  if (is_input) {
    SD_task_dependency_add(NULL,NULL,file,current_job);
  } else {
    SD_task_dependency_add(NULL,NULL,current_job,file);
    if (xbt_dynar_length(file->tasks_before)>1) {
      WARN1("File %s created at more than one location...",file->name);
    }
  }
}
static SD_task_t current_child;
void STag_dax__child(void) {
  current_child = xbt_dict_get_or_null(jobs,A_dax__child_ref);
  if (current_child==NULL)
    dax_parse_error(bprintf("Asked to add dependencies to the non-existent %s task",A_dax__child_ref));
}
void ETag_dax__child(void) {
  current_child=NULL;
}
void STag_dax__parent(void) {
  SD_task_t parent = xbt_dict_get_or_null(jobs,A_dax__parent_ref);
  if (parent == NULL)
    dax_parse_error(bprintf("Asked to add a dependency from %s to %s, but %s does not exist",
        current_child->name,A_dax__parent_ref,A_dax__parent_ref));
  SD_task_dependency_add(NULL,NULL,parent,current_child);
  DEBUG2("Control-flow dependency from %s to %s", current_child->name,parent->name);
}
void ETag_dax__adag(void) {
//  INFO0("See </adag>");
}
void ETag_dax__job(void) {
  current_job = NULL;
//  INFO0("See </job>");
}
void ETag_dax__parent(void) {
//  INFO0("See </parent>");
}
void ETag_dax__uses(void) {
//  INFO0("See </uses>");
}
