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
void smpi_mpi_land_func(void *a, void *b, int *length,
                        MPI_Datatype * datatype);
void smpi_mpi_sum_func(void *a, void *b, int *length,
                       MPI_Datatype * datatype);

void smpi_global_init()
{
  int i;

  int size = SIMIX_host_get_number();

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
  // config variable
  smpi_global->reference_speed = SMPI_DEFAULT_SPEED;

  // host info blank until sim starts
  // FIXME: is this okay?
  smpi_global->hosts = NULL;
  smpi_global->host_count = 0;

  // running hosts
  smpi_global->running_hosts_count = 0;

  // mallocators
  smpi_global->request_mallocator =
    xbt_mallocator_new(SMPI_REQUEST_MALLOCATOR_SIZE, smpi_request_new,
                       smpi_request_free, smpi_request_reset);
  smpi_global->message_mallocator =
    xbt_mallocator_new(SMPI_MESSAGE_MALLOCATOR_SIZE, smpi_message_new,
                       smpi_message_free, smpi_message_reset);

  // queues
  smpi_global->pending_send_request_queues = xbt_new(xbt_fifo_t, size);
  smpi_global->received_message_queues = xbt_new(xbt_fifo_t, size);

  // sender/receiver processes
  smpi_global->sender_processes = xbt_new(smx_process_t, size);
  smpi_global->receiver_processes = xbt_new(smx_process_t, size);

  // timers
  smpi_global->timer = xbt_os_timer_new();
  smpi_global->timer_mutex = SIMIX_mutex_init();
  smpi_global->timer_cond = SIMIX_cond_init();

  smpi_global->do_once_duration_nodes = NULL;
  smpi_global->do_once_duration = NULL;
  smpi_global->do_once_mutex = SIMIX_mutex_init();

  for (i = 0; i < size; i++) {
    smpi_global->pending_send_request_queues[i] = xbt_fifo_new();
    smpi_global->received_message_queues[i] = xbt_fifo_new();
  }

  smpi_global->hosts = SIMIX_host_get_table();
  smpi_global->host_count = SIMIX_host_get_number();

  smpi_mpi_global = xbt_new(s_smpi_mpi_global_t, 1);

  // global communicator
  smpi_mpi_global->mpi_comm_world = xbt_new(s_smpi_mpi_communicator_t, 1);
  smpi_mpi_global->mpi_comm_world->size = smpi_global->host_count;
  smpi_mpi_global->mpi_comm_world->barrier_count = 0;
  smpi_mpi_global->mpi_comm_world->barrier_mutex = SIMIX_mutex_init();
  smpi_mpi_global->mpi_comm_world->barrier_cond = SIMIX_cond_init();
  smpi_mpi_global->mpi_comm_world->rank_to_index_map =
    xbt_new(int, smpi_global->host_count);
  smpi_mpi_global->mpi_comm_world->index_to_rank_map =
    xbt_new(int, smpi_global->host_count);
  for (i = 0; i < smpi_global->host_count; i++) {
    smpi_mpi_global->mpi_comm_world->rank_to_index_map[i] = i;
    smpi_mpi_global->mpi_comm_world->index_to_rank_map[i] = i;
  }

  // mpi datatypes
  smpi_mpi_global->mpi_byte = xbt_new(s_smpi_mpi_datatype_t, 1);
  smpi_mpi_global->mpi_byte->size = (size_t) 1;
  smpi_mpi_global->mpi_int = xbt_new(s_smpi_mpi_datatype_t, 1);
  smpi_mpi_global->mpi_int->size = sizeof(int);
  smpi_mpi_global->mpi_double = xbt_new(s_smpi_mpi_datatype_t, 1);
  smpi_mpi_global->mpi_double->size = sizeof(double);

  // mpi operations
  smpi_mpi_global->mpi_land = xbt_new(s_smpi_mpi_op_t, 1);
  smpi_mpi_global->mpi_land->func = smpi_mpi_land_func;
  smpi_mpi_global->mpi_sum = xbt_new(s_smpi_mpi_op_t, 1);
  smpi_mpi_global->mpi_sum->func = smpi_mpi_sum_func;

}

void smpi_global_destroy()
{
  int i;

  int size = SIMIX_host_get_number();

  smpi_do_once_duration_node_t curr, next;

  // processes
  xbt_free(smpi_global->sender_processes);
  xbt_free(smpi_global->receiver_processes);

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

  for (i = 0; i < size; i++) {
    xbt_fifo_free(smpi_global->pending_send_request_queues[i]);
    xbt_fifo_free(smpi_global->received_message_queues[i]);
  }

  xbt_free(smpi_global->pending_send_request_queues);
  xbt_free(smpi_global->received_message_queues);

  xbt_free(smpi_global);

  smpi_global = NULL;
}

int smpi_host_index()
{
  smx_host_t host = SIMIX_host_self();
  smpi_host_data_t hdata = (smpi_host_data_t) SIMIX_host_get_data(host);
  return hdata->index;
}

smx_mutex_t smpi_host_mutex()
{
  smx_host_t host = SIMIX_host_self();
  smpi_host_data_t hdata = (smpi_host_data_t) SIMIX_host_get_data(host);
  return hdata->mutex;
}

smx_cond_t smpi_host_cond()
{
  smx_host_t host = SIMIX_host_self();
  smpi_host_data_t hdata = (smpi_host_data_t) SIMIX_host_get_data(host);
  return hdata->cond;
}

int smpi_run_simulation(int *argc, char **argv)
{
  smx_cond_t cond = NULL;
  smx_action_t action = NULL;

  xbt_fifo_t actions_failed = xbt_fifo_new();
  xbt_fifo_t actions_done = xbt_fifo_new();

  srand(SMPI_RAND_SEED);

  SIMIX_global_init(argc, argv);

  // parse the platform file: get the host list
  SIMIX_create_environment(argv[1]);

  SIMIX_function_register("smpi_simulated_main", smpi_simulated_main);
  SIMIX_launch_application(argv[2]);

  // must initialize globals between creating environment and launching app....
  smpi_global_init();

  /* Prepare to display some more info when dying on Ctrl-C pressing */
  // FIXME: doesn't work
  //signal(SIGINT, inthandler);

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

  INFO1("simulation time %g", SIMIX_get_clock());

  smpi_global_destroy();

  SIMIX_clean();

  return 0;
}
