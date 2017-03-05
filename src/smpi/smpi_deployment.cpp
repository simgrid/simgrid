/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simgrid/msg.h" /* barrier */
#include "xbt/dict.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include <src/smpi/smpi_group.hpp>

static xbt_dict_t smpi_instances = nullptr;
extern int process_count;
extern int* index_to_process_data;

typedef struct s_smpi_mpi_instance{
  const char* name;
  int size;
  int present_processes;
  int index;
  MPI_Comm comm_world;
  msg_bar_t finalization_barrier;
} s_smpi_mpi_instance_t;

/** \ingroup smpi_simulation
 * \brief Registers a running instance of a MPI program.
 *
 * FIXME : remove MSG from the loop at some point.
 * \param name the reference name of the function.
 * \param code the main mpi function (must have a int ..(int argc, char *argv[])) prototype
 * \param num_processes the size of the instance we want to deploy
 */
void SMPI_app_instance_register(const char *name, xbt_main_func_t code, int num_processes)
{
  SIMIX_function_register(name, code);

  s_smpi_mpi_instance_t* instance = (s_smpi_mpi_instance_t*)xbt_malloc(sizeof(s_smpi_mpi_instance_t));

  instance->name = name;
  instance->size = num_processes;
  instance->present_processes = 0;
  instance->index = process_count;
  instance->comm_world = MPI_COMM_NULL;
  instance->finalization_barrier = MSG_barrier_init(num_processes);

  process_count+=num_processes;

  if(smpi_instances==nullptr){
    smpi_instances = xbt_dict_new_homogeneous(xbt_free_f);
  }

  xbt_dict_set(smpi_instances, name, (void*)instance, nullptr);
}

//get the index of the process in the process_data array
void smpi_deployment_register_process(const char* instance_id, int rank, int index, MPI_Comm** comm, msg_bar_t* bar)
{

  if(smpi_instances==nullptr){//no instance registered, we probably used smpirun.
    index_to_process_data[index]=index;
    *bar = nullptr;
    *comm = nullptr;
    return;
  }

  s_smpi_mpi_instance_t* instance =
     static_cast<s_smpi_mpi_instance_t*>(xbt_dict_get_or_null(smpi_instances, instance_id));
  xbt_assert(instance, "Error, unknown instance %s", instance_id);

  if(instance->comm_world == MPI_COMM_NULL){
    MPI_Group group = new simgrid::SMPI::Group(instance->size);
    instance->comm_world = smpi_comm_new(group, nullptr);
  }
  instance->present_processes++;
  index_to_process_data[index]=instance->index+rank;
  smpi_comm_group(instance->comm_world)->set_mapping(index, rank);
  *bar = instance->finalization_barrier;
  *comm = &instance->comm_world;
}

void smpi_deployment_cleanup_instances(){
  xbt_dict_cursor_t cursor = nullptr;
  s_smpi_mpi_instance_t* instance = nullptr;
  char *name = nullptr;
  xbt_dict_foreach(smpi_instances, cursor, name, instance) {
    if(instance->comm_world!=MPI_COMM_NULL)
      while (smpi_comm_group(instance->comm_world)->unuse() > 0);
    xbt_free(instance->comm_world);
    MSG_barrier_destroy(instance->finalization_barrier);
  }
  xbt_dict_free(&smpi_instances);
}
