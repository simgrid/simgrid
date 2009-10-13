/* Copyright (c) 2009 Da SimGrid Team.  All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This datatype stores a a trace as produced by examples/simdag/dax, or other means
 * It can be replayed in simulation with examples/msg/actions or on a real platform with
 * examples/gras/replay.
 */

#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "xbt/str.h"
#include "workload.h"
#include "gras/datadesc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_workload,xbt, "Workload characterisation mecanisms");


xbt_workload_elm_t xbt_workload_elm_parse(char *line) {
  xbt_workload_elm_t res = xbt_new(s_xbt_workload_elm_t,1);
  res->date=-1;
  res->comment=NULL; /* it's not enough to memset for valgrind, apparently */
  res->who=NULL;
  res->str_arg=NULL;

  xbt_dynar_t w = xbt_str_split(line," ");

  if (xbt_dynar_length(w) == 0) {
    free(res);
    xbt_dynar_free(&w);
    return NULL;
  }

  char **words = xbt_dynar_get_ptr(w,0);
  int i=0;
  if (words[i][0] == '[') {
    sscanf(words[i]+1,"%lg",&(res->date));
    i++;
  }
  res->who = xbt_strdup(words[i++]);
  if (!strcmp(words[i],"recv")) {
    res->action = XBT_WORKLOAD_RECV;
    res->str_arg = xbt_strdup(words[++i]);
    sscanf(words[++i],"%lg",&(res->d_arg));

  } else if (!strcmp(words[i],"send")) {
    res->action = XBT_WORKLOAD_SEND;
    res->str_arg = xbt_strdup(words[++i]);
    sscanf(words[++i],"%lg",&(res->d_arg));

  } else if (!strcmp(words[i],"compute")) {
    res->action = XBT_WORKLOAD_COMPUTE;
    sscanf(words[++i],"%lg",&(res->d_arg));
  } else {
    xbt_die(bprintf("Unparsable command: %s (in %s)",words[i],line));
  }
  i++;
  if (words[i] && words[i][0] == '#') {
    res->comment = xbt_strdup(strchr(line,'#')+1);
  }

  xbt_dynar_free(&w);
  return res;
}
void xbt_workload_elm_free(xbt_workload_elm_t cmd) {
  if (!cmd)
    return;
  if (cmd->who)
    free(cmd->who);
  if (cmd->comment)
    free(cmd->comment);
  if (cmd->str_arg)
    free(cmd->str_arg);
  free(cmd);
}
void xbt_workload_elm_free_voidp(void*cmd) {
  xbt_workload_elm_free(*(xbt_workload_elm_t*)cmd);
}

char *xbt_workload_elm_to_string(xbt_workload_elm_t cmd) {
  char res[2048];
  char *addon;
  res[0]='\0';
  if (cmd==NULL)
    return xbt_strdup("(null command)");
  if (cmd->date != -1) {
    addon=bprintf("[%f] ",cmd->date);
    strcat(res,addon);
    free(addon);
  }
  addon= bprintf("'%s' ",cmd->who);
  strcat(res,addon);free(addon);

  switch (cmd->action) {
  case XBT_WORKLOAD_COMPUTE:
    addon=bprintf("computed %f flops",cmd->d_arg);
    strcat(res,addon);free(addon);
    break;
  case XBT_WORKLOAD_SEND:
    addon=bprintf("sent %f bytes to '%s'",cmd->d_arg,cmd->str_arg);
    strcat(res,addon);free(addon);
    break;
  case XBT_WORKLOAD_RECV:
    addon=bprintf("received %f bytes from '%s'",cmd->d_arg,cmd->str_arg);
    strcat(res,addon);free(addon);
    break;
  default:
    xbt_die(bprintf("Unknown command %d in '%s...'",cmd->action,res));
  }
  if (cmd->comment) {
    addon=bprintf(" (comment: %s)",cmd->comment);
    strcat(res,addon);free(addon);
  }
  return xbt_strdup(res);
}

int xbt_workload_elm_cmp_who_date(const void* _c1, const void* _c2) {
  xbt_workload_elm_t c1=*(xbt_workload_elm_t*)_c1;
  xbt_workload_elm_t c2=*(xbt_workload_elm_t*)_c2;
  if (!c1 || !c1->who)
    return -1;
  if (!c2 || !c2->who)
    return 1;
  int r = strcmp(c1->who,c2->who);
  if (r)
    return r;
  if (c1->date == c2->date)
    return 0;
  if (c1->date < c2->date)
    return -1;
  return 1;
}
void xbt_workload_sort_who_date(xbt_dynar_t c) {
  qsort(xbt_dynar_get_ptr(c,0),xbt_dynar_length(c),sizeof(xbt_workload_elm_t),xbt_workload_elm_cmp_who_date);
}
xbt_dynar_t xbt_workload_parse_file(char *filename) {
  FILE *file_in;
  file_in = fopen(filename,"r");
  xbt_assert1(file_in, "cannot open tracefile '%s'",filename);
  char *str_in = xbt_str_from_file(file_in);
  fclose(file_in);
  xbt_dynar_t in = xbt_str_split(str_in,"\n");
  free(str_in);
  xbt_dynar_t cmds=xbt_dynar_new(sizeof(xbt_workload_elm_t),xbt_workload_elm_free_voidp);

  unsigned int cursor;
  char *line;
  xbt_dynar_foreach(in,cursor,line) {
    xbt_workload_elm_t cmd = xbt_workload_elm_parse(line);
    if (cmd)
      xbt_dynar_push(cmds,&cmd);
  }
  xbt_dynar_shrink(cmds,0);
  xbt_dynar_free(&in);
  return cmds;
}


void xbt_workload_declare_datadesc(void) {
  gras_datadesc_type_t ddt;

  ddt = gras_datadesc_struct("s_xbt_workload_elm_t");
  gras_datadesc_struct_append(ddt,"who",gras_datadesc_by_name("string"));
  gras_datadesc_struct_append(ddt,"comment",gras_datadesc_by_name("string"));
  gras_datadesc_struct_append(ddt,"action",gras_datadesc_by_name("int"));
  gras_datadesc_struct_append(ddt,"date",gras_datadesc_by_name("double"));
  gras_datadesc_struct_append(ddt,"d_arg",gras_datadesc_by_name("double"));
  gras_datadesc_struct_append(ddt,"str_arg",gras_datadesc_by_name("string"));
  gras_datadesc_struct_close(ddt);

  gras_datadesc_ref("xbt_workload_elm_t",ddt);

  ddt = gras_datadesc_struct("s_xbt_workload_data_chunk_t");
  gras_datadesc_struct_append(ddt,"size",gras_datadesc_by_name("int"));
  gras_datadesc_cb_field_push(ddt, "size");
  gras_datadesc_struct_append(ddt,"chunk",gras_datadesc_ref_pop_arr(gras_datadesc_by_name("char")));
  gras_datadesc_struct_close(ddt);

  gras_datadesc_ref("xbt_workload_data_chunk_t",ddt);
}



xbt_workload_data_chunk_t xbt_workload_data_chunk_new(int size) {
  xbt_workload_data_chunk_t res = xbt_new0(s_xbt_workload_data_chunk_t,1);
  res->size = size;
  res->chunk = xbt_new(char,size-sizeof(res)-sizeof(int));
  return res;
}
void xbt_workload_data_chunk_free(xbt_workload_data_chunk_t c) {
  free(c->chunk);
  free(c);
}
