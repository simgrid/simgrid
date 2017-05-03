/* Copyright (c) 2010-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "xbt/virtu.h" /* sg_cmdline */
#include "xbt/xbt_os_time.h"
#include "simgrid/sg_config.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WIN32
#include <direct.h> // _mkdir
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_TI_trace, instr_trace, "tracing event system");

extern FILE *tracing_file;
double prefix=0.0;

xbt_dict_t tracing_files = nullptr;

extern s_instr_trace_writer_t active_writer;

void TRACE_TI_init()
{
  active_writer.print_PushState        = &print_TIPushState;
  active_writer.print_CreateContainer  = &print_TICreateContainer;
  active_writer.print_DestroyContainer = &print_TIDestroyContainer;
}

void TRACE_TI_start()
{
  char *filename = TRACE_get_filename();
  tracing_file = fopen(filename, "w");
  if (tracing_file == nullptr) {
    THROWF(system_error, 1, "Tracefile %s could not be opened for writing.", filename);
  }

  XBT_DEBUG("Filename %s is open for writing", filename);

  /* output one line comment */
  dump_comment(TRACE_get_comment());

  /* output comment file */
  dump_comment_file(TRACE_get_comment_file());
}

void TRACE_TI_end()
{
  xbt_dict_free(&tracing_files);
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  XBT_DEBUG("Filename %s is closed", filename);
}

void print_TICreateContainer(PajeEvent* event)
{
  //if we are in the mode with only one file
  static FILE *temp = nullptr;

  if (tracing_files == nullptr) {
    tracing_files = xbt_dict_new_homogeneous(nullptr);
    //generate unique run id with time
    prefix = xbt_os_time();
  }

  if (!xbt_cfg_get_boolean("tracing/smpi/format/ti-one-file") || temp == nullptr) {
    char *folder_name = bprintf("%s_files", TRACE_get_filename());
    char *filename = bprintf("%s/%f_%s.txt", folder_name, prefix,
     static_cast<CreateContainerEvent*>(event)->container->name);
#ifdef WIN32
    _mkdir(folder_name);
#else
    mkdir(folder_name, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    temp = fopen(filename, "w");
    xbt_assert(temp, "Tracefile %s could not be opened for writing: %s", filename, strerror(errno));
    fprintf(tracing_file, "%s\n", filename);

    xbt_free(folder_name);
    xbt_free(filename);
  }

  xbt_dict_set(tracing_files, ((CreateContainerEvent*) event)->container->name, (void *) temp, nullptr);
}

void print_TIDestroyContainer(PajeEvent* event)
{
  if (!xbt_cfg_get_boolean("tracing/smpi/format/ti-one-file")|| xbt_dict_length(tracing_files) == 1) {
    FILE* f = (FILE*)xbt_dict_get_or_null(tracing_files, ((DestroyContainerEvent *) event)->container->name);
    fclose(f);
  }
  xbt_dict_remove(tracing_files, ((DestroyContainerEvent*) event)->container->name);
}

void print_TIPushState(PajeEvent* event)
{
  int i;

  //char* function=nullptr;
  if (((PushStateEvent*) event->data)->extra == nullptr)
    return;
  instr_extra_data extra = (instr_extra_data) (((PushStateEvent*) event->data)->extra);

  char *process_id = nullptr;
  //FIXME: dirty extract "rank-" from the name, as we want the bare process id here
  if (strstr(((PushStateEvent*) event->data)->container->name, "rank-") == nullptr)
    process_id = xbt_strdup(((PushStateEvent*) event->data)->container->name);
  else
    process_id = xbt_strdup(((PushStateEvent*) event->data)->container->name + 5);

  FILE* trace_file =  (FILE* )xbt_dict_get(tracing_files, ((PushStateEvent*) event)->container->name);

  switch (extra->type) {
  case TRACING_INIT:
    fprintf(trace_file, "%s init\n", process_id);
    break;
  case TRACING_FINALIZE:
    fprintf(trace_file, "%s finalize\n", process_id);
    break;
  case TRACING_SEND:
    fprintf(trace_file, "%s send %d %d %s\n", process_id, extra->dst, extra->send_size, extra->datatype1);
    break;
  case TRACING_ISEND:
    fprintf(trace_file, "%s Isend %d %d %s\n", process_id, extra->dst, extra->send_size, extra->datatype1);
    break;
  case TRACING_RECV:
    fprintf(trace_file, "%s recv %d %d %s\n", process_id, extra->src, extra->send_size, extra->datatype1);
    break;
  case TRACING_IRECV:
    fprintf(trace_file, "%s Irecv %d %d %s\n", process_id, extra->src, extra->send_size, extra->datatype1);
    break;
  case TRACING_TEST:
    fprintf(trace_file, "%s test\n", process_id);
    break;
  case TRACING_WAIT:
    fprintf(trace_file, "%s wait\n", process_id);
    break;
  case TRACING_WAITALL:
    fprintf(trace_file, "%s waitAll\n", process_id);
    break;
  case TRACING_BARRIER:
    fprintf(trace_file, "%s barrier\n", process_id);
    break;
  case TRACING_BCAST:          // rank bcast size (root) (datatype)
    fprintf(trace_file, "%s bcast %d ", process_id, extra->send_size);
    if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
      fprintf(trace_file, "%d %s", extra->root, extra->datatype1);
    fprintf(trace_file, "\n");
    break;
  case TRACING_REDUCE:         // rank reduce comm_size comp_size (root) (datatype)
    fprintf(trace_file, "%s reduce %d %f ", process_id, extra->send_size, extra->comp_size);
    if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
      fprintf(trace_file, "%d %s", extra->root, extra->datatype1);
    fprintf(trace_file, "\n");
    break;
  case TRACING_ALLREDUCE:      // rank allreduce comm_size comp_size (datatype)
    fprintf(trace_file, "%s allReduce %d %f %s\n", process_id, extra->send_size, extra->comp_size, extra->datatype1);
    break;
  case TRACING_ALLTOALL:       // rank alltoall send_size recv_size (sendtype) (recvtype)
    fprintf(trace_file, "%s allToAll %d %d %s %s\n", process_id, extra->send_size, extra->recv_size, extra->datatype1,
            extra->datatype2);
    break;
  case TRACING_ALLTOALLV:      // rank alltoallv send_size [sendcounts] recv_size [recvcounts] (sendtype) (recvtype)
    fprintf(trace_file, "%s allToAllV %d ", process_id, extra->send_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(trace_file, "%d ", extra->sendcounts[i]);
    fprintf(trace_file, "%d ", extra->recv_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(trace_file, "%d ", extra->recvcounts[i]);
    fprintf(trace_file, "%s %s \n", extra->datatype1, extra->datatype2);
    break;
  case TRACING_GATHER:         // rank gather send_size recv_size root (sendtype) (recvtype)
    fprintf(trace_file, "%s gather %d %d %d %s %s\n", process_id, extra->send_size, extra->recv_size, extra->root,
            extra->datatype1, extra->datatype2);
    break;
  case TRACING_ALLGATHERV:     // rank allgatherv send_size [recvcounts] (sendtype) (recvtype)
    fprintf(trace_file, "%s allGatherV %d ", process_id, extra->send_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(trace_file, "%d ", extra->recvcounts[i]);
    fprintf(trace_file, "%s %s \n", extra->datatype1, extra->datatype2);
    break;
  case TRACING_REDUCE_SCATTER: // rank reducescatter [recvcounts] comp_size (sendtype)
    fprintf(trace_file, "%s reduceScatter ", process_id);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(trace_file, "%d ", extra->recvcounts[i]);
    fprintf(trace_file, "%f %s\n", extra->comp_size, extra->datatype1);
    break;
  case TRACING_COMPUTING:
    fprintf(trace_file, "%s compute %f\n", process_id, extra->comp_size);
    break;
  case TRACING_SLEEPING:
    fprintf(trace_file, "%s sleep %f\n", process_id, extra->sleep_duration);
    break;
  case TRACING_GATHERV: // rank gatherv send_size [recvcounts] root (sendtype) (recvtype)
    fprintf(trace_file, "%s gatherV %d ", process_id, extra->send_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(trace_file, "%d ", extra->recvcounts[i]);
    fprintf(trace_file, "%d %s %s\n", extra->root, extra->datatype1, extra->datatype2);
    break;
  case TRACING_WAITANY:
  case TRACING_SENDRECV:
  case TRACING_SCATTER:
  case TRACING_SCATTERV:
  case TRACING_ALLGATHER:
  case TRACING_SCAN:
  case TRACING_EXSCAN:
  case TRACING_COMM_SIZE:
  case TRACING_COMM_SPLIT:
  case TRACING_COMM_DUP:
  case TRACING_SSEND:
  case TRACING_ISSEND:
  default:
    XBT_WARN ("Call from %s impossible to translate into replay command : Not implemented (yet)",
         ((PushStateEvent*) event->data)->value->name);
    break;
  }

  if (extra->recvcounts != nullptr)
    xbt_free(extra->recvcounts);
  if (extra->sendcounts != nullptr)
    xbt_free(extra->sendcounts);
  xbt_free(process_id);
  xbt_free(extra);
}
