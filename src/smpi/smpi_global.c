#include <stdint.h>

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories");

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_kernel, smpi,
                                "Logging specific to SMPI (kernel)");

typedef struct s_smpi_process_data {
  int index;
  xbt_fifo_t pending_sent;
  xbt_fifo_t pending_recv;
  xbt_os_timer_t timer;
  double simulated;
} s_smpi_process_data_t;

static smpi_process_data_t* process_data = NULL;
static int process_count = 0;

MPI_Comm MPI_COMM_WORLD = MPI_COMM_NULL;

smpi_process_data_t smpi_process_data(void) {
  return SIMIX_process_get_data(SIMIX_process_self());
}

smpi_process_data_t smpi_process_remote_data(int index) {
  return process_data[index];
}

int smpi_process_count(void) {
  return process_count;
}

int smpi_process_index(void) {
  smpi_process_data_t data = smpi_process_data();

  return data->index;
}

xbt_os_timer_t smpi_process_timer(void) {
  smpi_process_data_t data = smpi_process_data();

  return data->timer;
}

void smpi_process_simulated_reset(void) {
  smpi_process_data_t data = smpi_process_data();

  data->simulated = SIMIX_get_clock();
}

double smpi_process_simulated_elapsed(void) {
  smpi_process_data_t data = smpi_process_data();

  return SIMIX_get_clock() - data->simulated;
}

void smpi_process_post_send(MPI_Comm comm, MPI_Request request) {
  int index = smpi_group_index(smpi_comm_group(comm), request->dst);
  smpi_process_data_t data = smpi_process_remote_data(index);
  xbt_fifo_item_t item;
  MPI_Request req;

  DEBUG4("isend for request %p [src = %d, dst = %d, tag = %d]",
         request, request->src, request->dst, request->tag);
  xbt_fifo_foreach(data->pending_recv, item, req, MPI_Request) {
    if(req->comm == request->comm
       && (req->src == MPI_ANY_SOURCE || req->src == request->src)
       && (req->tag == MPI_ANY_TAG || req->tag == request->tag)){
      DEBUG4("find matching request %p [src = %d, dst = %d, tag = %d]",
             req, req->src, req->dst, req->tag);
      xbt_fifo_remove_item(data->pending_recv, item);
      /* Materialize the *_ANY_* fields from corresponding irecv request */
      req->src = request->src;
      req->tag = request->tag;
      request->rdv = req->rdv;
      return;
    } else {
      DEBUG4("not matching request %p [src = %d, dst = %d, tag = %d]",
             req, req->src, req->dst, req->tag);
    }
  }
  request->rdv = SIMIX_rdv_create(NULL);
  xbt_fifo_push(data->pending_sent, request);
}

void smpi_process_post_recv(MPI_Request request) {
  smpi_process_data_t data = smpi_process_data();
  xbt_fifo_item_t item;
  MPI_Request req;

  DEBUG4("irecv for request %p [src = %d, dst = %d, tag = %d]",
         request, request->src, request->dst, request->tag);
  xbt_fifo_foreach(data->pending_sent, item, req, MPI_Request) {
    if(req->comm == request->comm
       && (request->src == MPI_ANY_SOURCE || req->src == request->src)
       && (request->tag == MPI_ANY_TAG || req->tag == request->tag)){
      DEBUG4("find matching request %p [src = %d, dst = %d, tag = %d]",
             req, req->src, req->dst, req->tag);
      xbt_fifo_remove_item(data->pending_sent, item);
      /* Materialize the *_ANY_* fields from the irecv request */
      request->src = req->src;
      request->tag = req->tag;
      request->rdv = req->rdv;
      return;
    } else {
      DEBUG4("not matching request %p [src = %d, dst = %d, tag = %d]",
             req, req->src, req->dst, req->tag);
    }
  }
  request->rdv = SIMIX_rdv_create(NULL);
  xbt_fifo_push(data->pending_recv, request);
}

void smpi_global_init(void) {
  double clock = SIMIX_get_clock();
  int i;
  MPI_Group group;

  SIMIX_network_set_copy_data_callback(&SIMIX_network_copy_buffer_callback);
  process_count = SIMIX_process_count();
  process_data = xbt_new(smpi_process_data_t, process_count);
  for(i = 0; i < process_count; i++) {
    process_data[i] = xbt_new(s_smpi_process_data_t, 1);
    process_data[i]->index = i;
    process_data[i]->pending_sent = xbt_fifo_new();
    process_data[i]->pending_recv = xbt_fifo_new();
    process_data[i]->timer = xbt_os_timer_new();
    process_data[i]->simulated = clock;
  }
  group = smpi_group_new(process_count);
  MPI_COMM_WORLD = smpi_comm_new(group);
  for(i = 0; i < process_count; i++) {
    smpi_group_set_mapping(group, i, i);
  }
}

void smpi_global_destroy(void) {
  int count = smpi_process_count();
  int i;

  smpi_comm_destroy(MPI_COMM_WORLD);
  MPI_COMM_WORLD = MPI_COMM_NULL;
  for(i = 0; i < count; i++) {
    xbt_os_timer_free(process_data[i]->timer);
    xbt_fifo_free(process_data[i]->pending_recv);
    xbt_fifo_free(process_data[i]->pending_sent);
    xbt_free(process_data[i]);
  }
  xbt_free(process_data);
  process_data = NULL;
}

int main(int argc, char **argv)
{
  srand(SMPI_RAND_SEED);

  double default_reference_speed = 20000.0;
  xbt_cfg_register(&_surf_cfg_set, "reference_speed",
                   "Power of the host running the simulation (in flop/s). Used to bench the operations.",
                   xbt_cfgelm_double, &default_reference_speed, 1, 1, NULL, NULL);

  int default_display_timing = 0;
  xbt_cfg_register(&_surf_cfg_set, "display_timing",
                   "Boolean indicating whether we should display the timing after simulation.",
                   xbt_cfgelm_int, &default_display_timing, 1, 1, NULL, NULL);

  int default_display_smpe = 0;
  xbt_cfg_register(&_surf_cfg_set, "SMPE",
                   "Boolean indicating whether we should display simulated time spent in MPI calls.",
                   xbt_cfgelm_int, &default_display_smpe, 1, 1, NULL, NULL);

  SIMIX_global_init(&argc, argv);

  // parse the platform file: get the host list
  SIMIX_create_environment(argv[1]);

  SIMIX_function_register("smpi_simulated_main", smpi_simulated_main);
  SIMIX_launch_application(argv[2]);

  smpi_global_init();

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);
  SIMIX_init();

  while (SIMIX_solve(NULL, NULL) != -1.0);
  
  if (xbt_cfg_get_int(_surf_cfg_set, "display_timing"))
    INFO1("simulation time %g", SIMIX_get_clock());

  smpi_global_destroy();

  SIMIX_clean();
  return 0;
}
