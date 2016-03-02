/* pmm - parallel matrix multiplication "double diffusion"                  */

/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "xbt/matrix.h"
#include "xbt/xbt_os_time.h"

/** @addtogroup MSG_examples
 * 
 * - <b>pmm/msg_pmm.c</b>: Parallel Matrix Multiplication is a little application. This is something that most MPI
 *   developers have written during their class, here implemented using MSG instead of MPI.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_pmm, "Messages specific for this msg example");

/* This example should always be executed using a deployment of GRID_SIZE * GRID_SIZE nodes. */
#define GRID_SIZE 3           /* Modify to adjust the grid's size */
#define NODE_MATRIX_SIZE 300  /* Amount of work done by each node*/

#define GRID_NUM_NODES GRID_SIZE * GRID_SIZE
#define MATRIX_SIZE NODE_MATRIX_SIZE * GRID_SIZE
#define MAILBOX_NAME_SIZE 10
#define NEIGHBOURS_COUNT GRID_SIZE - 1

/*
 * The job sent to every node
 */
typedef struct s_node_job{
  int row;
  int col;
  int nodes_in_row[NEIGHBOURS_COUNT];
  int nodes_in_col[NEIGHBOURS_COUNT];
  xbt_matrix_t A;
  xbt_matrix_t B;
} s_node_job_t, *node_job_t;

/*
 * Structure for recovering results
 */
typedef struct s_result {
  int row;
  int col;
  xbt_matrix_t sC;
} s_result_t, *result_t;

int node(int argc, char **argv);
static void create_jobs(xbt_matrix_t A, xbt_matrix_t B, node_job_t *jobs);
static void broadcast_jobs(node_job_t *jobs);
static node_job_t wait_job(int selfid);
static void broadcast_matrix(xbt_matrix_t M, int num_nodes, int *nodes);
static void get_sub_matrix(xbt_matrix_t *sM, int selfid);
static void receive_results(result_t *results);
static void task_cleanup(void *arg);

int node(int argc, char **argv)
{
  char my_mbox[MAILBOX_NAME_SIZE];
  node_job_t myjob, jobs[GRID_NUM_NODES];
  xbt_matrix_t A, B, C, sA, sB, sC;
  result_t result;

  xbt_assert(argc != 1, "Wrong number of arguments for this node");

  /* Initialize the node's data-structures */
  int myid = xbt_str_parse_int(argv[1], "Invalid ID received as first node parameter: %s");
  snprintf(my_mbox, MAILBOX_NAME_SIZE - 1, "%d", myid);
  sC = xbt_matrix_double_new_zeros(NODE_MATRIX_SIZE, NODE_MATRIX_SIZE);

  if(myid == 0){
    /* Create the matrices to multiply and one to store the result */
    A = xbt_matrix_double_new_id(MATRIX_SIZE, MATRIX_SIZE);
    B = xbt_matrix_double_new_seq(MATRIX_SIZE, MATRIX_SIZE);
    C = xbt_matrix_double_new_zeros(MATRIX_SIZE, MATRIX_SIZE);

    /* Create the nodes' jobs */
    create_jobs(A, B, jobs);

    /* Get own job first */
    myjob = jobs[0];

    /* Broadcast the rest of the jobs to the other nodes */
    broadcast_jobs(jobs + 1);

  } else {
    A = B = C = NULL;           /* Avoid warning at compilation */
    myjob = wait_job(myid);
  }

  /* Multiplication main-loop */
  XBT_VERB("Start Multiplication's Main-loop");
  for (int k=0; k < GRID_SIZE; k++){
    if(k == myjob->col){
      XBT_VERB("Broadcast sA(%d,%d) to row %d", myjob->row, k, myjob->row);
      broadcast_matrix(myjob->A, NEIGHBOURS_COUNT, myjob->nodes_in_row);
    }

    if(k == myjob->row){
      XBT_VERB("Broadcast sB(%d,%d) to col %d", k, myjob->col, myjob->col);
      broadcast_matrix(myjob->B, NEIGHBOURS_COUNT, myjob->nodes_in_col);
    }

    if(myjob->row == k && myjob->col == k){
      xbt_matrix_double_addmult(myjob->A, myjob->B, sC);
    }else if(myjob->row == k){
      get_sub_matrix(&sA, myid);
      xbt_matrix_double_addmult(sA, myjob->B, sC);
      xbt_matrix_free(sA);
    }else if(myjob->col == k){
      get_sub_matrix(&sB, myid);
      xbt_matrix_double_addmult(myjob->A, sB, sC);
      xbt_matrix_free(sB);
    }else{
      get_sub_matrix(&sA, myid);
      get_sub_matrix(&sB, myid);
      xbt_matrix_double_addmult(sA, sB, sC);
      xbt_matrix_free(sA);
      xbt_matrix_free(sB);
    }
  }

  /* Node 0: gather the results and reconstruct the final matrix */
  if(myid == 0){
    int node;
    result_t results[GRID_NUM_NODES] = {0};

    XBT_VERB("Multiplication done.");

    /* Get the result from the nodes in the GRID */
    receive_results(results);

    /* First add our results */
    xbt_matrix_copy_values(C, sC, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, 0, 0, 0, 0, NULL);

    /* Reconstruct the rest of the result matrix */
    for (node = 1; node < GRID_NUM_NODES; node++){
      xbt_matrix_copy_values(C, results[node]->sC, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE,
                             NODE_MATRIX_SIZE * results[node]->row, NODE_MATRIX_SIZE * results[node]->col,
                             0, 0, NULL);
      xbt_matrix_free(results[node]->sC);
      xbt_free(results[node]);
    }

    //xbt_matrix_dump(C, "C:res", 0, xbt_matrix_dump_display_double);

    xbt_matrix_free(A);
    xbt_matrix_free(B);
    xbt_matrix_free(C);

  /* The rest: return the result to node 0 */
  }else{
    msg_task_t task;

    XBT_VERB("Multiplication done. Send the sub-result.");

    result = xbt_new0(s_result_t, 1);
    result->row = myjob->row;
    result->col = myjob->col;
    result->sC = xbt_matrix_new_sub(sC, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, 0, 0, NULL);
    task = MSG_task_create("result",100,100,result);
    MSG_task_send(task, "0");
  }

  /* Clean up and finish*/
  xbt_matrix_free(sC);
  xbt_matrix_free(myjob->A);
  xbt_matrix_free(myjob->B);
  xbt_free(myjob);
  return 0;
}

/*
 * Broadcast the jobs to the nodes of the grid (except to node 0)
 */
static void broadcast_jobs(node_job_t *jobs)
{
  char node_mbox[MAILBOX_NAME_SIZE];
  msg_comm_t comms[GRID_NUM_NODES - 1] = {0};

  XBT_VERB("Broadcast Jobs");
  for (int node = 1; node < GRID_NUM_NODES; node++){
    msg_task_t task  = MSG_task_create("Job", 100, 100, jobs[node-1]);
    snprintf(node_mbox, MAILBOX_NAME_SIZE - 1, "%d", node);
    comms[node-1] = MSG_task_isend(task, node_mbox);
  }

  MSG_comm_waitall(comms, GRID_NUM_NODES-1, -1);
  for (int node = 1; node < GRID_NUM_NODES; node++)
    MSG_comm_destroy(comms[node - 1]);
}

static node_job_t wait_job(int selfid)
{
  msg_task_t task = NULL;
  char self_mbox[MAILBOX_NAME_SIZE];
  snprintf(self_mbox, MAILBOX_NAME_SIZE - 1, "%d", selfid);
  msg_error_t err = MSG_task_receive(&task, self_mbox);
  xbt_assert(err == MSG_OK, "Error while receiving from %s (%d)", self_mbox, (int)err);
  node_job_t job  = (node_job_t)MSG_task_get_data(task);
  MSG_task_destroy(task);
  XBT_VERB("Got Job (%d,%d)", job->row, job->col);

  return job;
}

static void broadcast_matrix(xbt_matrix_t M, int num_nodes, int *nodes)
{
  char node_mbox[MAILBOX_NAME_SIZE];

  for(int node=0; node < num_nodes; node++){
    snprintf(node_mbox, MAILBOX_NAME_SIZE - 1, "%d", nodes[node]);
    xbt_matrix_t sM  = xbt_matrix_new_sub(M, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, 0, 0, NULL);
    msg_task_t task = MSG_task_create("sub-matrix", 100, 100, sM);
    MSG_task_dsend(task, node_mbox, task_cleanup);
    XBT_DEBUG("sub-matrix sent to %s", node_mbox);
  }
}

static void get_sub_matrix(xbt_matrix_t *sM, int selfid)
{
  msg_task_t task = NULL;
  char node_mbox[MAILBOX_NAME_SIZE];

  XBT_VERB("Get sub-matrix");

  snprintf(node_mbox, MAILBOX_NAME_SIZE - 1, "%d", selfid);
  msg_error_t err = MSG_task_receive(&task, node_mbox);
  xbt_assert(err == MSG_OK, "Error while receiving from %s (%d)", node_mbox, (int)err);
  *sM = (xbt_matrix_t)MSG_task_get_data(task);
  MSG_task_destroy(task);
}

static void task_cleanup(void *arg){
  msg_task_t task = (msg_task_t)arg;
  xbt_matrix_t m = (xbt_matrix_t)MSG_task_get_data(task);
  xbt_matrix_free(m);
  MSG_task_destroy(task);
}

int main(int argc, char *argv[])
{
  xbt_os_timer_t timer = xbt_os_timer_new();

  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);

  MSG_function_register("node", node);
  for(int i = 0 ; i< 9; i++) {
    char *hostname = bprintf("node-%d.acme.org", i);
    char **argvF = xbt_new(char *, 3);
    argvF[0] = xbt_strdup("node");
    argvF[1] = bprintf("%d", i);
    argvF[2] = NULL;
    MSG_process_create_with_arguments("node", node, NULL, MSG_host_by_name(hostname), 2, argvF);
    xbt_free(hostname);
  }

  xbt_os_cputimer_start(timer);
  msg_error_t res = MSG_main();
  xbt_os_cputimer_stop(timer);
  XBT_CRITICAL("Simulated time: %g", MSG_get_clock());

  return res != MSG_OK;
}

static void create_jobs(xbt_matrix_t A, xbt_matrix_t B, node_job_t *jobs)
{
  int row = 0, col = 0;

  for (int node = 0; node < GRID_NUM_NODES; node++){
    XBT_VERB("Create job %d", node);
    jobs[node] = xbt_new0(s_node_job_t, 1);
    jobs[node]->row = row;
    jobs[node]->col = col;

    /* Compute who are the nodes in the same row and column */
    /* than the node receiving this job */
    for (int j = 0, k = 0; j < GRID_SIZE; j++) {
      if (node != (GRID_SIZE * row) + j) {
        jobs[node]->nodes_in_row[k] = (GRID_SIZE * row) + j;
        k++;
      }
    }

    for (int j = 0, k = 0; j < GRID_SIZE; j++) {
      if (node != (GRID_SIZE * j) + col) {
        jobs[node]->nodes_in_col[k] = (GRID_SIZE * j) + col;
        k++;
      }
    }

    /* Assign a sub matrix of A and B to the job */
    jobs[node]->A =
      xbt_matrix_new_sub(A, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE * row, NODE_MATRIX_SIZE * col, NULL);
    jobs[node]->B =
      xbt_matrix_new_sub(B, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE, NODE_MATRIX_SIZE * row, NODE_MATRIX_SIZE * col, NULL);

    if (++col >= GRID_SIZE){
      col = 0;
      row++;
    }
  }
}

static void receive_results(result_t *results) {
  msg_comm_t comms[GRID_NUM_NODES-1] = {0};
  msg_task_t tasks[GRID_NUM_NODES-1] = {0};

  XBT_VERB("Receive Results.");

  /* Get the result from the nodes in the GRID */
  for (int node = 1; node < GRID_NUM_NODES; node++)
   comms[node-1] = MSG_task_irecv(&tasks[node-1], "0");

  MSG_comm_waitall(comms, GRID_NUM_NODES - 1, -1);
  for (int node = 1; node < GRID_NUM_NODES; node++)
    MSG_comm_destroy(comms[node - 1]);

  /* Reconstruct the result matrix */
  for (int node = 1; node < GRID_NUM_NODES; node++){
    results[node] = (result_t)MSG_task_get_data(tasks[node-1]);
    MSG_task_destroy(tasks[node-1]);
  }
}
