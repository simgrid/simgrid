#include <stdio.h>

#include "private.h"

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories");

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi,
                                "Logging specific to SMPI (kernel)");

smpi_global_t smpi_global = NULL;

void *smpi_request_new(void);

void *smpi_request_new()
{
  smpi_mpi_request_t request = xbt_new(s_smpi_mpi_request_t, 1);

  request->buf = NULL;
  request->completed = 0;
  request->mutex = SIMIX_mutex_init();
  request->cond = SIMIX_cond_init();
  request->data = NULL;
  request->forward = 0;

  return request;
}

void smpi_request_free(void *pointer);

void smpi_request_free(void *pointer)
{

  smpi_mpi_request_t request = pointer;

  SIMIX_cond_destroy(request->cond);
  SIMIX_mutex_destroy(request->mutex);
  xbt_free(request);

  return;
}

void smpi_request_reset(void *pointer);

void smpi_request_reset(void *pointer)
{
  smpi_mpi_request_t request = pointer;

  request->buf = NULL;
  request->completed = 0;
  request->consumed = 0;
  request->data = NULL;
  request->forward = 0;

  return;
}


void *smpi_message_new(void);

void *smpi_message_new()
{
  smpi_received_message_t message = xbt_new(s_smpi_received_message_t, 1);
  message->buf = NULL;
  return message;
}

void smpi_message_free(void *pointer);

void smpi_message_free(void *pointer)
{
  xbt_free(pointer);
  return;
}

void smpi_message_reset(void *pointer);

void smpi_message_reset(void *pointer)
{
  smpi_received_message_t message = pointer;
  message->buf = NULL;
  return;
}

int smpi_create_request(void *buf, int count, smpi_mpi_datatype_t datatype,
                        int src, int dst, int tag,
                        smpi_mpi_communicator_t comm,
                        smpi_mpi_request_t * requestptr)
{
  int retval = MPI_SUCCESS;

  smpi_mpi_request_t request = NULL;

  // parameter checking prob belongs in smpi_mpi, but this is less repeat code
  if (NULL == buf) {
    retval = MPI_ERR_INTERN;
  } else if (0 > count) {
    retval = MPI_ERR_COUNT;
  } else if (NULL == datatype) {
    retval = MPI_ERR_TYPE;
  } else if (MPI_ANY_SOURCE != src && (0 > src || comm->size <= src)) {
    retval = MPI_ERR_RANK;
  } else if (0 > dst || comm->size <= dst) {
    retval = MPI_ERR_RANK;
  } else if (MPI_ANY_TAG != tag && 0 > tag) {
    retval = MPI_ERR_TAG;
  } else if (NULL == comm) {
    retval = MPI_ERR_COMM;
  } else if (NULL == requestptr) {
    retval = MPI_ERR_ARG;
  } else {
    request = xbt_mallocator_get(smpi_global->request_mallocator);
    request->comm = comm;
    request->src = src;
    request->dst = dst;
    request->tag = tag;
    request->buf = buf;
    request->datatype = datatype;
    request->count = count;

    *requestptr = request;
  }
  return retval;
}
/* FIXME: understand what they do and put the prototypes in a header file (live in smpi_base.c) */
void smpi_mpi_land_func(void *a, void *b, int *length, MPI_Datatype * datatype);
void smpi_mpi_sum_func(void *a, void *b, int *length, MPI_Datatype * datatype);
void smpi_mpi_min_func(void *a, void *b, int *length, MPI_Datatype * datatype);
void smpi_mpi_max_func(void *a, void *b, int *length, MPI_Datatype * datatype);

void smpi_global_init()
{
  int i;

  /* Connect our log channels: that must be done manually under windows */
#ifdef XBT_LOG_CONNECT
  XBT_LOG_CONNECT(smpi_base, smpi);
  XBT_LOG_CONNECT(smpi_bench, smpi);
  XBT_LOG_CONNECT(smpi_kernel, smpi);
  XBT_LOG_CONNECT(smpi_mpi, smpi);
  XBT_LOG_CONNECT(smpi_receiver, smpi);
  XBT_LOG_CONNECT(smpi_sender, smpi);
  XBT_LOG_CONNECT(smpi_util, smpi);
#endif

  smpi_global = xbt_new(s_smpi_global_t, 1);

  // mallocators
  smpi_global->request_mallocator =
    xbt_mallocator_new(SMPI_REQUEST_MALLOCATOR_SIZE, smpi_request_new,
                       smpi_request_free, smpi_request_reset);
  smpi_global->message_mallocator =
    xbt_mallocator_new(SMPI_MESSAGE_MALLOCATOR_SIZE, smpi_message_new,
                       smpi_message_free, smpi_message_reset);

  smpi_global->process_count = SIMIX_process_count();
  DEBUG1("There is %d processes",smpi_global->process_count);

  // sender/receiver processes
  smpi_global->main_processes = xbt_new(smx_process_t, smpi_global->process_count);

  // timers
  smpi_global->timer = xbt_os_timer_new();
  smpi_global->timer_mutex = SIMIX_mutex_init();
  smpi_global->timer_cond = SIMIX_cond_init();

  smpi_global->do_once_duration_nodes = NULL;
  smpi_global->do_once_duration = NULL;
  smpi_global->do_once_mutex = SIMIX_mutex_init();


  smpi_mpi_global = xbt_new(s_smpi_mpi_global_t, 1);

  // global communicator
  smpi_mpi_global->mpi_comm_world = xbt_new(s_smpi_mpi_communicator_t, 1);
  smpi_mpi_global->mpi_comm_world->size = smpi_global->process_count;
  smpi_mpi_global->mpi_comm_world->barrier_count = 0;
  smpi_mpi_global->mpi_comm_world->barrier_mutex = SIMIX_mutex_init();
  smpi_mpi_global->mpi_comm_world->barrier_cond = SIMIX_cond_init();
  smpi_mpi_global->mpi_comm_world->rank_to_index_map =
    xbt_new(int, smpi_global->process_count);
  smpi_mpi_global->mpi_comm_world->index_to_rank_map =
    xbt_new(int, smpi_global->process_count);
  for (i = 0; i < smpi_global->process_count; i++) {
    smpi_mpi_global->mpi_comm_world->rank_to_index_map[i] = i;
    smpi_mpi_global->mpi_comm_world->index_to_rank_map[i] = i;
  }

  // mpi datatypes
  smpi_mpi_global->mpi_byte = xbt_new(s_smpi_mpi_datatype_t, 1);
  smpi_mpi_global->mpi_byte->size = (size_t) 1;
  smpi_mpi_global->mpi_int = xbt_new(s_smpi_mpi_datatype_t, 1);
  smpi_mpi_global->mpi_int->size = sizeof(int);
  smpi_mpi_global->mpi_float = xbt_new(s_smpi_mpi_datatype_t, 1);
  smpi_mpi_global->mpi_float->size = sizeof(float);
  smpi_mpi_global->mpi_double = xbt_new(s_smpi_mpi_datatype_t, 1);
  smpi_mpi_global->mpi_double->size = sizeof(double);

  // mpi operations
  smpi_mpi_global->mpi_land = xbt_new(s_smpi_mpi_op_t, 1);
  smpi_mpi_global->mpi_land->func = smpi_mpi_land_func;
  smpi_mpi_global->mpi_sum = xbt_new(s_smpi_mpi_op_t, 1);
  smpi_mpi_global->mpi_sum->func = smpi_mpi_sum_func;
  smpi_mpi_global->mpi_min = xbt_new(s_smpi_mpi_op_t, 1);
  smpi_mpi_global->mpi_min->func = smpi_mpi_min_func;
  smpi_mpi_global->mpi_max = xbt_new(s_smpi_mpi_op_t, 1);
  smpi_mpi_global->mpi_max->func = smpi_mpi_max_func;

}

void smpi_global_destroy()
{
  smpi_do_once_duration_node_t curr, next;

  // processes
  xbt_free(smpi_global->main_processes);

  // mallocators
  xbt_mallocator_free(smpi_global->request_mallocator);
  xbt_mallocator_free(smpi_global->message_mallocator);

  xbt_os_timer_free(smpi_global->timer);
  SIMIX_mutex_destroy(smpi_global->timer_mutex);
  SIMIX_cond_destroy(smpi_global->timer_cond);

  for (curr = smpi_global->do_once_duration_nodes; NULL != curr; curr = next) {
    next = curr->next;
    xbt_free(curr->file);
    xbt_free(curr);
  }

  SIMIX_mutex_destroy(smpi_global->do_once_mutex);

  xbt_free(smpi_global);
  smpi_global = NULL;

  /* free smpi_mpi_global */
  SIMIX_mutex_destroy(smpi_mpi_global->mpi_comm_world->barrier_mutex);
  SIMIX_cond_destroy(smpi_mpi_global->mpi_comm_world->barrier_cond);
  xbt_free(smpi_mpi_global->mpi_comm_world);

  xbt_free(smpi_mpi_global->mpi_byte);
  xbt_free(smpi_mpi_global->mpi_int);
  xbt_free(smpi_mpi_global->mpi_double);

  xbt_free(smpi_mpi_global->mpi_land);
  xbt_free(smpi_mpi_global->mpi_sum);

  xbt_free(smpi_mpi_global);

}

int smpi_process_index()
{
  smpi_process_data_t pdata = (smpi_process_data_t) SIMIX_process_get_data(SIMIX_process_self());
  return pdata->index;
}

smx_mutex_t smpi_process_mutex()
{
  smpi_process_data_t pdata = (smpi_process_data_t) SIMIX_process_get_data(SIMIX_process_self());
  return pdata->mutex;
}

smx_cond_t smpi_process_cond()
{
  smpi_process_data_t pdata = (smpi_process_data_t) SIMIX_process_get_data(SIMIX_process_self());
  return pdata->cond;
}

static void smpi_cfg_cb_host_speed(const char *name, int pos) {
	smpi_global->reference_speed = xbt_cfg_get_double_at(_surf_cfg_set,name,pos);
}

int smpi_run_simulation(int *argc, char **argv)
{
  smx_cond_t cond = NULL;
  smx_action_t action = NULL;

  xbt_fifo_t actions_failed = xbt_fifo_new();
  xbt_fifo_t actions_done = xbt_fifo_new();

  srand(SMPI_RAND_SEED);

  double default_reference_speed = 20000.0;
  xbt_cfg_register(&_surf_cfg_set,"reference_speed","Power of the host running the simulation (in flop/s). Used to bench the operations.",
		  xbt_cfgelm_double,&default_reference_speed,1,1,smpi_cfg_cb_host_speed,NULL);

  int default_display_timing = 0;
  xbt_cfg_register(&_surf_cfg_set,"display_timing","Boolean indicating whether we should display the timing after simulation.",
		  xbt_cfgelm_int,&default_display_timing,1,1,NULL,NULL);

  SIMIX_global_init(argc, argv);


  // parse the platform file: get the host list
  SIMIX_create_environment(argv[1]);

  SIMIX_function_register("smpi_simulated_main", smpi_simulated_main);
  SIMIX_launch_application(argv[2]);

  // must initialize globals between creating environment and launching app....
  smpi_global_init();

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);
  SIMIX_init();

  while (SIMIX_solve(actions_done, actions_failed) != -1.0) {
    while ((action = xbt_fifo_pop(actions_failed))) {
      DEBUG1("** %s failed **", action->name);
      while ((cond = xbt_fifo_pop(action->cond_list))) {
        SIMIX_cond_broadcast(cond);
      }
    }
    while ((action = xbt_fifo_pop(actions_done))) {
      DEBUG1("** %s done **", action->name);
      while ((cond = xbt_fifo_pop(action->cond_list))) {
        SIMIX_cond_broadcast(cond);
      }
    }
  }

  // FIXME: cleanup incomplete
  xbt_fifo_free(actions_failed);
  xbt_fifo_free(actions_done);


  if (xbt_cfg_get_int(_surf_cfg_set,"display_timing"))
	  INFO1("simulation time %g", SIMIX_get_clock());

  smpi_global_destroy();
  SIMIX_clean();

  return 0;
}
