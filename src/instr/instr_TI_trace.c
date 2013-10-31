/* Copyright (c) 2010-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */
#include "instr/instr_private.h"
#include "xbt/virtu.h" /* sg_cmdline */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_TI_trace, instr_trace, "tracing event system");


extern FILE *tracing_file;
extern s_instr_trace_writer_t active_writer;
extern xbt_dynar_t buffer;

void TRACE_TI_init(void)
{
  active_writer.print_PushState = print_TIPushState;
}

void TRACE_TI_start(void)
{
  char *filename = TRACE_get_filename();
  tracing_file = fopen(filename, "w");
  if (tracing_file == NULL) {
    THROWF(system_error, 1, "Tracefile %s could not be opened for writing.",
           filename);
  }

  XBT_DEBUG("Filename %s is open for writing", filename);

  /* output one line comment */
  dump_comment(TRACE_get_comment());

  /* output comment file */
  dump_comment_file(TRACE_get_comment_file());

  buffer = xbt_dynar_new(sizeof(paje_event_t), NULL);
}

void TRACE_TI_end(void)
{
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  xbt_dynar_free(&buffer);
  XBT_DEBUG("Filename %s is closed", filename);
}

void print_TIPushState(paje_event_t event)
{


  int i;

  //char* function=NULL;
  if (((pushState_t) event->data)->extra == NULL)
    return;
  instr_extra_data extra =
      (instr_extra_data) (((pushState_t) event->data)->extra);

  char *process_id = NULL;
  //FIXME: dirty extract "rank-" from the name, as we want the bare process id here
  if (strstr(((pushState_t) event->data)->container->name, "rank-") == NULL)
    process_id = xbt_strdup(((pushState_t) event->data)->container->name);
  else
    process_id = xbt_strdup(((pushState_t) event->data)->container->name + 5);

  switch (extra->type) {

  case TRACING_INIT:
    fprintf(tracing_file, "%s init\n", process_id);
    break;
  case TRACING_FINALIZE:
    fprintf(tracing_file, "%s finalize\n", process_id);
    break;
  case TRACING_SEND:
    fprintf(tracing_file, "%s send %d %d %s\n", process_id, extra->dst,
            extra->send_size, extra->datatype1);
    break;
  case TRACING_ISEND:
    fprintf(tracing_file, "%s isend %d %d %s\n", process_id, extra->dst,
            extra->send_size, extra->datatype1);
    break;
  case TRACING_RECV:
    fprintf(tracing_file, "%s recv %d %d %s\n", process_id, extra->src,
            extra->send_size, extra->datatype1);
    break;
  case TRACING_IRECV:
    fprintf(tracing_file, "%s irecv %d %d %s\n", process_id, extra->src,
            extra->send_size, extra->datatype1);
    break;
  case TRACING_WAIT:
    fprintf(tracing_file, "%s wait\n", process_id);
    break;
  case TRACING_WAITALL:
    fprintf(tracing_file, "%s waitall\n", process_id);
    break;
  case TRACING_BARRIER:
    fprintf(tracing_file, "%s barrier\n", process_id);
    break;
  case TRACING_BCAST:          // rank bcast size (root) (datatype)
    fprintf(tracing_file, "%s bcast %d ", process_id, extra->send_size);
    if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
      fprintf(tracing_file, "%d %s", extra->root, extra->datatype1);
    fprintf(tracing_file, "\n");
    break;
  case TRACING_REDUCE:         // rank reduce comm_size comp_size (root) (datatype)
    fprintf(tracing_file, "%s reduce %d %f ", process_id, extra->send_size,
            extra->comp_size);
    if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
      fprintf(tracing_file, "%d %s", extra->root, extra->datatype1);
    fprintf(tracing_file, "\n");
    break;
  case TRACING_ALLREDUCE:      // rank allreduce comm_size comp_size (datatype)
    fprintf(tracing_file, "%s allreduce %d %f %s\n", process_id,
            extra->send_size, extra->comp_size, extra->datatype1);
    break;
  case TRACING_ALLTOALL:       // rank alltoall send_size recv_size (sendtype) (recvtype)
    fprintf(tracing_file, "%s alltoall %d %d %s %s\n", process_id,
            extra->send_size, extra->recv_size, extra->datatype1,
            extra->datatype2);
    break;
  case TRACING_ALLTOALLV:      // rank alltoallv send_size [sendcounts] recv_size [recvcounts] (sendtype) (recvtype)
    fprintf(tracing_file, "%s alltoallv %d ", process_id, extra->send_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(tracing_file, "%d ", extra->sendcounts[i]);
    fprintf(tracing_file, "%d ", extra->recv_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(tracing_file, "%d ", extra->recvcounts[i]);
    fprintf(tracing_file, "%s %s \n", extra->datatype1, extra->datatype2);
    break;
  case TRACING_GATHER:         // rank gather send_size recv_size root (sendtype) (recvtype)
    fprintf(tracing_file, "%s gather %d %d %d %s %s\n", process_id,
            extra->send_size, extra->recv_size, extra->root, extra->datatype1,
            extra->datatype2);
    break;
  case TRACING_ALLGATHERV:     // rank allgatherv send_size [recvcounts] (sendtype) (recvtype)
    fprintf(tracing_file, "%s allgatherv %d ", process_id, extra->send_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(tracing_file, "%d ", extra->recvcounts[i]);
    fprintf(tracing_file, "%s %s \n", extra->datatype1, extra->datatype2);
    break;
  case TRACING_REDUCE_SCATTER: // rank reducescatter [recvcounts] comp_size (sendtype)
    fprintf(tracing_file, "%s reducescatter ", process_id);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(tracing_file, "%d ", extra->recvcounts[i]);
    fprintf(tracing_file, "%f %s\n", extra->comp_size, extra->datatype1);
    break;
  case TRACING_COMPUTING:
    fprintf(tracing_file, "%s compute %f\n", process_id, extra->comp_size);
    break;
  case TRACING_GATHERV: // rank gatherv send_size [recvcounts] root (sendtype) (recvtype)
    fprintf(tracing_file, "%s gatherv %d ", process_id, extra->send_size);
    for (i = 0; i < extra->num_processes; i++)
      fprintf(tracing_file, "%d ", extra->recvcounts[i]);
    fprintf(tracing_file, "%d %s %s\n", extra->root, extra->datatype1, extra->datatype2);
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

    XBT_WARN
        ("Call from %s impossible to translate into replay command : Not implemented (yet)",
         ((pushState_t) event->data)->value->name);
    break;
  }

  if (extra->recvcounts != NULL)
    xbt_free(extra->recvcounts);
  if (extra->sendcounts != NULL)
    xbt_free(extra->sendcounts);
  xbt_free(process_id);
  xbt_free(extra);
}
