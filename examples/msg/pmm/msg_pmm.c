#include "msg/msg.h"
#include "xbt/matrix.h"
#include "xbt/log.h"
#include "xbt/xbt_os_time.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_pmm,
                             "Messages specific for this msg example");

#define MAILBOX_NAME_SIZE 10
#define MATRIX_SIZE 18
#define GRID_SIZE 3
#define GRID_NUM_NODES GRID_SIZE * GRID_SIZE
#define NODE_MATRIX_SIZE MATRIX_SIZE / GRID_SIZE
#define NEIGHBOURS_COUNT GRID_SIZE - 1

/*
 * Task data
 */
typedef struct s_task_data{
  int row;
  int col;
  int nodes_in_row[NEIGHBOURS_COUNT];
  int nodes_in_col[NEIGHBOURS_COUNT];
  xbt_matrix_t A;
  xbt_matrix_t B;
} s_task_data_t, *task_data_t;

/*
 * Node data
 */
typedef struct s_node{
  int id;
  char mailbox[MAILBOX_NAME_SIZE];
  task_data_t job;
  xbt_matrix_t C;
} s_node_t, *node_t;

/**
 * Structure for recovering results
 */
typedef struct s_result {
  int row;
  int col;
  xbt_matrix_t C;
} s_result_t, *result_t;

int node(int argc, char **argv);
static void assign_tasks(xbt_matrix_t A, xbt_matrix_t B);
static task_data_t wait_task(int selfid);
static void broadcast_matrix(xbt_matrix_t M, int num_nodes, int *nodes);
static void get_sub_matrix(xbt_matrix_t *sM, int selfid);
static void task_cleanup(void *arg);

int node(int argc, char **argv)
{
  int j,k;
  xbt_matrix_t A, B, C, sA, sB;
  result_t result;

  xbt_assert0(argc != 1, "Wrong number of arguments for this node");

  /* Initialize node information (id and mailbox) */
  s_node_t mydata = {0};
  mydata.id = atoi(argv[1]);
  snprintf(mydata.mailbox, MAILBOX_NAME_SIZE - 1, "%d", mydata.id);
  mydata.C = xbt_matrix_double_new_zeros(NODE_MATRIX_SIZE, NODE_MATRIX_SIZE);

  if(mydata.id == 0){
    /* Initialize data matrices */
    A = xbt_matrix_double_new_id(MATRIX_SIZE, MATRIX_SIZE);
    B = xbt_matrix_double_new_seq(MATRIX_SIZE, MATRIX_SIZE);
    C = xbt_matrix_double_new_zeros(MATRIX_SIZE, MATRIX_SIZE);

    /* Get own job first */
    mydata.job = xbt_new0(s_task_data_t, 1);
    mydata.job->row = 0;
    mydata.job->col = 0;
    mydata.job->A =
      xbt_matrix_new_sub(A, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, 0, 0, NULL);
    mydata.job->B =
      xbt_matrix_new_sub(B, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, 0, 0, NULL);

    for (j = 0, k = 0; j < GRID_SIZE; j++) {
      if (j != 0) {
        mydata.job->nodes_in_row[k] = j;
        k++;
      }
    }

    for (j = 0, k = 0; j < GRID_SIZE; j++) {
      if (GRID_SIZE * j != 0) {
        mydata.job->nodes_in_col[k] = GRID_SIZE * j;
        k++;
      }
    }

    /* Broadcast the rest of the jobs to the other nodes */
    assign_tasks(A,B);

  }else{
    mydata.job = wait_task(mydata.id);
  }

  /* Multiplication main-loop */
  XBT_CRITICAL("Start Multiplication's Main-loop");
  for(k=0; k < GRID_SIZE; k++){
    if(k == mydata.job->col){
      XBT_VERB("Broadcast sA(%d,%d) to row %d", mydata.job->row, k, mydata.job->row);
      broadcast_matrix(mydata.job->A, NEIGHBOURS_COUNT, mydata.job->nodes_in_row);
    }

    if(k == mydata.job->row){
      XBT_VERB("Broadcast sB(%d,%d) to col %d", k, mydata.job->col, mydata.job->col);
      broadcast_matrix(mydata.job->B, NEIGHBOURS_COUNT, mydata.job->nodes_in_col);
    }

    if(mydata.job->row == k && mydata.job->col == k){
      xbt_matrix_double_addmult(mydata.job->A, mydata.job->B, mydata.C);
    }else if(mydata.job->row == k){
      get_sub_matrix(&sA, mydata.id);
      xbt_matrix_double_addmult(sA, mydata.job->B, mydata.C);
      xbt_matrix_free(sA);
    }else if(mydata.job->col == k){
      get_sub_matrix(&sB, mydata.id);
      xbt_matrix_double_addmult(mydata.job->A, sB, mydata.C);
      xbt_matrix_free(sB);
    }else{
      get_sub_matrix(&sA, mydata.id);
      get_sub_matrix(&sB, mydata.id);
      xbt_matrix_double_addmult(sA, sB, mydata.C);
      xbt_matrix_free(sA);
      xbt_matrix_free(sB);
    }
  }

  /* Node 0: gather the results and reconstruct the final matrix */
  if(mydata.id == 0){
    int node;
    msg_comm_t comms[GRID_NUM_NODES-1] = {0};
    m_task_t tasks[GRID_NUM_NODES-1] = {0};

    XBT_CRITICAL("Multiplication done. Reconstruct the result.");

    /* First add our results */
    xbt_matrix_copy_values(C, mydata.C, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE,
                           0, 0, 0, 0, NULL);

    /* Get the result from the nodes in the GRID */
    for (node = 1; node < GRID_NUM_NODES; node++){
      comms[node-1] = MSG_task_irecv(&tasks[node-1], mydata.mailbox);
    }
    MSG_comm_waitall(comms, GRID_NUM_NODES - 1, -1);

    /* Reconstruct the result matrix */
    for (node = 1; node < GRID_NUM_NODES; node++){
      result = (result_t)MSG_task_get_data(tasks[node-1]);
      xbt_matrix_copy_values(C, result->C, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE,
          NODE_MATRIX_SIZE * result->row, NODE_MATRIX_SIZE * result->col, 0, 0, NULL);
      xbt_matrix_free(result->C);
      xbt_free(result);
      MSG_task_destroy(tasks[node-1]);
    }

    xbt_matrix_dump(C, "C:res", 0, xbt_matrix_dump_display_double);

  /* The rest: return the result to node 0 */
  }else{
    m_task_t task;

    XBT_CRITICAL("Multiplication done. Send the sub-result.");

    result = xbt_new0(s_result_t, 1);
    result->row = mydata.job->row;
    result->col = mydata.job->col;
    result->C =
      xbt_matrix_new_sub(mydata.C, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, 0, 0, NULL);
    task = MSG_task_create("result",100,100,result);
    MSG_task_dsend(task, "0", NULL);
  }

  /* Clean up and finish*/
  xbt_matrix_free(mydata.job->A);
  xbt_matrix_free(mydata.job->B);
  xbt_free(mydata.job);
  return 0;
}

/*
 * Assign the tasks to the GRID
 */
static void assign_tasks(xbt_matrix_t A, xbt_matrix_t B)
{
  int node, j, k, row = 0, col = 1;
  char node_mbox[MAILBOX_NAME_SIZE];
  m_task_t task;
  task_data_t assignment;
  msg_comm_t comms[GRID_NUM_NODES - 1] = {0};

  XBT_CRITICAL("Assign tasks");
  for (node = 1; node < GRID_NUM_NODES; node++){
    assignment = xbt_new0(s_task_data_t, 1);
    assignment->row = row;
    assignment->col = col;

    /* Compute who are the peers in the same row and column */
    /* than the node receiving this task and include this
     * information in the assignment */
    for (j = 0, k = 0; j < GRID_SIZE; j++) {
      if (node != (GRID_SIZE * row) + j) {
        assignment->nodes_in_row[k] = (GRID_SIZE * row) + j;
        k++;
      }
    }

    for (j = 0, k = 0; j < GRID_SIZE; j++) {
      if (node != (GRID_SIZE * j) + col) {
        assignment->nodes_in_col[k] = (GRID_SIZE * j) + col;
        k++;
      }
    }

    assignment->A =
      xbt_matrix_new_sub(A, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE,
                         NODE_MATRIX_SIZE * row, NODE_MATRIX_SIZE * col,
                         NULL);
    assignment->B =
      xbt_matrix_new_sub(B, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE,
                         NODE_MATRIX_SIZE * row, NODE_MATRIX_SIZE * col,
                         NULL);

    col++;
    if (col >= GRID_SIZE){
      col = 0;
      row++;
    }

    task = MSG_task_create("Job", 100, 100, assignment);
    snprintf(node_mbox, MAILBOX_NAME_SIZE - 1, "%d", node);
    comms[node-1] = MSG_task_isend(task, node_mbox);
  }

  MSG_comm_waitall(comms, GRID_NUM_NODES-1, -1);
}

static task_data_t wait_task(int selfid)
{
  m_task_t task = NULL;
  char self_mbox[MAILBOX_NAME_SIZE];
  task_data_t assignment;
  snprintf(self_mbox, MAILBOX_NAME_SIZE - 1, "%d", selfid);
  MSG_task_receive(&task, self_mbox);
  assignment = (task_data_t)MSG_task_get_data(task);
  MSG_task_destroy(task);
  XBT_CRITICAL("Got Job (%d,%d)", assignment->row, assignment->col);

  return assignment;
}

static void broadcast_matrix(xbt_matrix_t M, int num_nodes, int *nodes)
{
  int node;
  char node_mbox[MAILBOX_NAME_SIZE];
  m_task_t task;
  xbt_matrix_t sM;

  for(node=0; node < num_nodes; node++){
    snprintf(node_mbox, MAILBOX_NAME_SIZE - 1, "%d", nodes[node]);
    sM = xbt_matrix_new_sub(M, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, 0, 0, NULL);
    task = MSG_task_create("sub-matrix", 100, 100, sM);
    MSG_task_dsend(task, node_mbox, task_cleanup);
    XBT_DEBUG("sub-matrix sent to %s", node_mbox);
  }

}

static void get_sub_matrix(xbt_matrix_t *sM, int selfid)
{
  m_task_t task = NULL;
  char node_mbox[MAILBOX_NAME_SIZE];

  XBT_VERB("Get sub-matrix");

  snprintf(node_mbox, MAILBOX_NAME_SIZE - 1, "%d", selfid);
  MSG_task_receive(&task, node_mbox);
  *sM = (xbt_matrix_t)MSG_task_get_data(task);
  MSG_task_destroy(task);
}

static void task_cleanup(void *arg){
  m_task_t task = (m_task_t)arg;
  xbt_matrix_t m = (xbt_matrix_t)MSG_task_get_data(task);
  xbt_matrix_free(m);
  MSG_task_destroy(task);
}

/**
 * \brief Main function.
 */
int main(int argc, char *argv[])
{
  xbt_os_timer_t timer = xbt_os_timer_new();

  MSG_global_init(&argc, argv);

  char **options = &argv[1];
  const char* platform_file = options[0];
  const char* application_file = options[1];

  MSG_set_channel_number(0);
  MSG_create_environment(platform_file);

  MSG_function_register("node", node);
  MSG_launch_application(application_file);

  xbt_os_timer_start(timer);
  MSG_error_t res = MSG_main();
  xbt_os_timer_stop(timer);
  XBT_CRITICAL("Simulated time: %g", MSG_get_clock());

  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}



