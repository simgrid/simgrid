/* struct_diff -- little tool to see which structure's field are modified on mc_diff */

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "xbt.h"
#include "xbt/datadesc.h"
#include "xbt/file_stat.h"
#include "../../src/xbt/datadesc/datadesc_private.h" // RAAAAH! ugly relative path, but it's late, I want it to be done NOW.
#include "simix/simix.h"
#include "../../src/simix/smx_smurf_private.h" // RAAAAH! ugly relative path, but it's even later and it still doesn't work

static void define_types(void) {
  xbt_datadesc_type_t desc;

  /* typedef struct xbt_swag_hookup {
   *    void *next;
   *    void *prev;
   *  } s_xbt_swag_hookup_t; */
  desc = xbt_datadesc_struct("s_xbt_swag_hookup_t");
  xbt_datadesc_struct_append(desc,"next",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_struct_append(desc,"prev",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_struct_close(desc);

  /* FIXME: maybe we should provide the internal details */
  xbt_datadesc_copy("smx_host_t",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_copy("smx_context_t",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_copy("smx_action_t",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_copy("xbt_dict_t",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_copy("xbt_fifo_t",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_copy("smx_process_t",xbt_datadesc_by_name("data pointer"));

  /* typedef struct {
   *   char *msg;
   *   xbt_errcat_t category;

   *   int value;
   *   short int remote;
   *   char *host;
   *   char *procname;
   *
   *   int pid;
   *   char *file;
   *   int line;
   *   char *func;

   *   int used;
   *   char **bt_strings;
   *   void *bt[XBT_BACKTRACE_SIZE];
   * } xbt_ex_t;   */
  desc = xbt_datadesc_struct("xbt_ex_t");
  xbt_datadesc_struct_append(desc,"msg",xbt_datadesc_by_name("string"));

  xbt_assert(sizeof(xbt_errcat_t)==sizeof(int),
      "I was assuming that xbt_errcat_t is of the same size than integer, but it's not true: it's %zu (int:%zu)\n",
      sizeof(xbt_errcat_t),sizeof(int));
  xbt_datadesc_struct_append(desc,"category",xbt_datadesc_by_name("short int"));

  xbt_datadesc_struct_append(desc,"value",xbt_datadesc_by_name("short int"));
  xbt_datadesc_struct_append(desc,"remote",xbt_datadesc_by_name("short int"));
  xbt_datadesc_struct_append(desc,"host",xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(desc,"procname",xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(desc,"pid",xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_append(desc,"file",xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(desc,"line",xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_append(desc,"func",xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(desc,"used",xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_append(desc,"bt_strings",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_struct_append(desc,"bt",xbt_datadesc_array_fixed("xbt_bt",xbt_datadesc_by_name("data pointer"),XBT_BACKTRACE_SIZE));
  xbt_datadesc_struct_close(desc);

  /* typedef struct {
   *   __ex_mctx_t *ctx_mctx;
   *   volatile int ctx_caught;
   *   volatile xbt_ex_t exception;
   * } xbt_running_ctx_t;   */
  desc = xbt_datadesc_struct("xbt_running_ctx_t");
  xbt_datadesc_struct_append(desc,"ctx_mctx",xbt_datadesc_by_name("data pointer"));
  xbt_datadesc_struct_append(desc,"ctx_caught",xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_append(desc,"exception",xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_close(desc);

  xbt_assert(sizeof(e_smx_simcall_t)==sizeof(int),
      "I was assuming that e_smx_simcall_t is of the same size than integer, but it's not true: it's %zu (int:%zu)\n",
      sizeof(e_smx_simcall_t),sizeof(int));
  xbt_datadesc_copy("e_smx_simcall_t", xbt_datadesc_by_name("int"));
}

static void parse_from_file(const char *name){
  int in=open(name,O_RDONLY);
  if (in==-1){
    fprintf(stderr,"Cannot read structure file from %s: %s\n",name,strerror(errno));
    exit(1);
  }

  xbt_strbuff_t strbuff = xbt_strbuff_new();
  int r;
  do {
    char buffer[1024];
    r = read(in,&buffer,1023);
    buffer[r] = '\0';
    xbt_strbuff_append(strbuff,buffer);
  } while (r!=0);

  printf("File %s content:\n%s\n",name,strbuff->data);
  xbt_datadesc_parse(name,strbuff->data);

}

int main(int argc, char** argv) {
  xbt_init(&argc, argv);
  define_types();

  if (argc<3) {
    fprintf(stderr,"Usage: %s struct-file offset1 offset2 ...\n",argv[0]);
    exit(1);
  }

//  parse_from_file("s_smx_simcall_t");
  parse_from_file(argv[1]);

  int cpt;
  xbt_datadesc_type_t type = xbt_datadesc_by_name(argv[1]);
  for (cpt=2;cpt<argc;cpt++) {
    int offset = atoi(argv[cpt]);
    unsigned int cursor;
    xbt_dd_cat_field_t field;
    int found = 0;

    printf ("Offset: %d in struct %s (there is %lu fields)\n",
        offset, type->name, xbt_dynar_length(type->category.struct_data.fields));
    xbt_dynar_foreach(type->category.struct_data.fields,cursor,field) {
      if (field->offset[GRAS_THISARCH]<= offset&&
          field->offset[GRAS_THISARCH]+field->type->size[GRAS_THISARCH] >= offset) {
        found = 1;
        printf("This offset is somewhere in field %s, which starts at offset %ld and is of size %ld\n",
              field->name,field->offset[GRAS_THISARCH],field->type->size[GRAS_THISARCH]);

      }
    }
    if (!found) {

      printf("Damnit, the structure is too short to find the the field (last field %s was at offset %ld, with size %ld). Weird.\n",
          field->name,field->offset[GRAS_THISARCH],field->type->size[GRAS_THISARCH]);
    }
  }
  return 0;
}



