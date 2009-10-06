/* Copyright (c) 2009 Da SimGrid Team.  All rights reserved.                */

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
static xbt_dict_t files;
static SD_task_t current_job;

typedef struct {
  xbt_dynar_t inputs;
  xbt_dynar_t outputs;
} dax_comp_task;

typedef struct {
  double size;
} dax_comm_task;

typedef struct {
  short int is_comm_task;
  union {
    dax_comp_task comp;
    dax_comm_task comm;
  };
} *dax_task_t;

static void dax_task_rmdata(SD_task_t t) {

}

static void dax_task_free(void*task){
  SD_task_t t=task;
  SD_task_destroy(t);
}

xbt_dynar_t SD_daxload(const char*filename) {
  SD_task_t task;
  FILE* in_file = fopen(filename,"r");
  xbt_assert1(in_file, "Unable to open \"%s\"\n", filename);
  input_buffer = dax__create_buffer(in_file, 10);
  dax__switch_to_buffer(input_buffer);
  dax_lineno = 1;

  result = xbt_dynar_new(sizeof(SD_task_t),dax_task_free);
  files=xbt_dict_new();
  task = SD_task_create("top",NULL,0);
  xbt_dynar_push(result,&task);

  xbt_assert2(!dax_lex(),"Parse error in %s: %s",filename,dax__parse_err_msg());
  dax__delete_buffer(input_buffer);
  xbt_dict_free(&files);
  fclose(in_file);
  return result;
}

void STag_dax__adag(void) {
  double version = dax_parse_double(A_dax__adag_version);

  xbt_assert1((version == 2.1), "Expected version 2.1 in <adag> tag, got %f. Fix the parser or your file",version);
}
void STag_dax__job(void) {
  double runtime = dax_parse_double(A_dax__job_runtime);
  runtime*=4200000000.; /* Assume that timings were done on a 4.2GFlops machine. I mean, why not? */
  INFO3("See <job id=%s runtime=%s %.0f>",A_dax__job_id,A_dax__job_runtime,runtime);
  current_job = SD_task_create(A_dax__job_id,NULL,runtime);
  xbt_dynar_push(result,current_job);
}
void STag_dax__child(void) {
//  INFO0("See <child>");
}
void STag_dax__parent(void) {
//  INFO0("See <parent>");
}
void STag_dax__uses(void) {
  SD_task_t file;
  double size = dax_parse_double(A_dax__uses_size);
  int is_input = (A_dax__uses_link == A_dax__uses_link_input);

  INFO2("See <uses file=%s %s>",A_dax__uses_file,(is_input?"in":"out"));
  file = xbt_dict_get_or_null(files,A_dax__uses_file);
  if (file==NULL) {
    file = SD_task_create(A_dax__uses_file,NULL,size);
    xbt_dict_set(files,A_dax__uses_file,file,NULL);
  } else {
    if (SD_task_get_amount(file)!=size) {
      WARN3("Ignoring file %s size redefinition from %.0f to %.0f",
          A_dax__uses_file,SD_task_get_amount(file),size);
    }
  }
  if (is_input) {
    SD_task_dependency_add(NULL,NULL,file,current_job);
  } else {
    SD_task_dependency_add(NULL,NULL,file,current_job);
  }
}
void ETag_dax__adag(void) {
//  INFO0("See </adag>");
}
void ETag_dax__job(void) {
  current_job = NULL;
//  INFO0("See </job>");
}
void ETag_dax__child(void) {
//  INFO0("See </child>");
}
void ETag_dax__parent(void) {
//  INFO0("See </parent>");
}
void ETag_dax__uses(void) {
//  INFO0("See </uses>");
}
