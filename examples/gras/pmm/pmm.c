/* $Id$ */
/* pmm - parallel matrix multiplication "double diffusion"                  */

/* Copyright (c) 2006-2008 The SimGrid team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "xbt/matrix.h"
#include "amok/peermanagement.h"

#define PROC_MATRIX_SIZE 3
#define NEIGHBOR_COUNT PROC_MATRIX_SIZE - 1
#define SLAVE_COUNT (PROC_MATRIX_SIZE*PROC_MATRIX_SIZE)

#define DATA_MATRIX_SIZE 18
const int submatrix_size = DATA_MATRIX_SIZE / PROC_MATRIX_SIZE;

XBT_LOG_NEW_DEFAULT_CATEGORY(pmm, "Parallel Matrix Multiplication");

/* struct for recovering results */
GRAS_DEFINE_TYPE(s_result, struct s_result {
                 int linepos;
                 int rowpos; xbt_matrix_t C GRAS_ANNOTE(subtype, double);});

typedef struct s_result result_t;

/* struct to send initial data to slave */
GRAS_DEFINE_TYPE(s_pmm_assignment, struct s_pmm_assignment {
                 int linepos;
                 int rowpos;
                 xbt_peer_t line[NEIGHBOR_COUNT];
                 xbt_peer_t row[NEIGHBOR_COUNT];
                 xbt_matrix_t A GRAS_ANNOTE(subtype, double);
                 xbt_matrix_t B GRAS_ANNOTE(subtype, double);});

typedef struct s_pmm_assignment s_pmm_assignment_t;

/* register messages which may be sent (common to client and server) */
static void register_messages(void)
{
  gras_datadesc_type_t result_type;
  gras_datadesc_type_t pmm_assignment_type;

  gras_datadesc_set_const("NEIGHBOR_COUNT", NEIGHBOR_COUNT);
  result_type = gras_datadesc_by_symbol(s_result);
  pmm_assignment_type = gras_datadesc_by_symbol(s_pmm_assignment);

  /* receive a final result from slave */
  gras_msgtype_declare("result", result_type);

  /* send from master to slave to assign a position and some data */
  gras_msgtype_declare("pmm_slave", pmm_assignment_type);

  /* send data between slaves */
  gras_msgtype_declare("dataA",
                       gras_datadesc_matrix(gras_datadesc_by_name("double"),
                                            NULL));
  gras_msgtype_declare("dataB",
                       gras_datadesc_matrix(gras_datadesc_by_name("double"),
                                            NULL));
}

/* Function prototypes */
int slave(int argc, char *argv[]);
int master(int argc, char *argv[]);


/* **********************************************************************
 * master code
 * **********************************************************************/

/* Global private data */
typedef struct {
  int nbr_row, nbr_line;
  int remaining_step;
  int remaining_ack;
} master_data_t;

int master(int argc, char *argv[])
{

  int i;

  xbt_matrix_t A, B, C;
  result_t result;

  gras_socket_t from;

  xbt_dynar_t peers;            /* group of slaves */
  xbt_peer_t grid[SLAVE_COUNT]; /* The slaves as an array */
  gras_socket_t socket[SLAVE_COUNT];    /* sockets for brodcast to slaves */

  /* Init the GRAS's infrastructure */
  gras_init(&argc, argv);
  amok_pm_init();
  register_messages();

  /* Initialize data matrices */
  A = xbt_matrix_double_new_id(DATA_MATRIX_SIZE, DATA_MATRIX_SIZE);
  B = xbt_matrix_double_new_seq(DATA_MATRIX_SIZE, DATA_MATRIX_SIZE);
  C = xbt_matrix_double_new_zeros(DATA_MATRIX_SIZE, DATA_MATRIX_SIZE);

  /* Create the connexions */
  xbt_assert0(argc > 1, "Usage: master <port>");
  gras_socket_server(atoi(argv[1]));
  peers = amok_pm_group_new("pmm");

  /* friends, we're ready. Come and play */
  INFO0("Wait for peers for 5 sec");
  gras_msg_handleall(5);
  while (xbt_dynar_length(peers)<9) {
    INFO1("Got only %ld pals. Wait 5 more seconds", xbt_dynar_length(peers));
    gras_msg_handleall(5);
  }
  INFO1("Good. Got %ld pals", xbt_dynar_length(peers));

  for (i = 0; i < xbt_dynar_length(peers) && i < SLAVE_COUNT; i++) {
    xbt_dynar_get_cpy(peers, i, &grid[i]);
    socket[i] = gras_socket_client(grid[i]->name, grid[i]->port);
  }
  xbt_assert2(i == SLAVE_COUNT,
              "Not enough slaves for this setting (got %d of %d). Change the deployment file",
              i, SLAVE_COUNT);

  /* Kill surnumerous slaves */
  for (i = SLAVE_COUNT; i < xbt_dynar_length(peers);) {
    xbt_peer_t h;

    xbt_dynar_remove_at(peers, i, &h);
    INFO2("Too much slaves. Killing %s:%d", h->name, h->port);
    amok_pm_kill_hp(h->name, h->port);
    free(h);
  }


  /* Assign job to slaves */
  int row = 0, line = 0;
  INFO0("XXXXXXXXXXXXXXXXXXXXXX begin Multiplication");
  for (i = 0; i < SLAVE_COUNT; i++) {
    s_pmm_assignment_t assignment;
    int j, k;

    assignment.linepos = line;  // assigned line
    assignment.rowpos = row;    // assigned row

    /* Neiborhood */
    for (j = 0, k = 0; j < PROC_MATRIX_SIZE; j++) {
      if (i != j * PROC_MATRIX_SIZE + (row)) {
        assignment.row[k] = grid[j * PROC_MATRIX_SIZE + (row)];
        k++;
      }
    }
    for (j = 0, k = 0; j < PROC_MATRIX_SIZE; j++) {
      if (i != (line) * PROC_MATRIX_SIZE + j) {
        assignment.line[k] = grid[(line) * PROC_MATRIX_SIZE + j];
        k++;
      }
    }

    assignment.A = xbt_matrix_new_sub(A,
                                      submatrix_size, submatrix_size,
                                      submatrix_size * line,
                                      submatrix_size * row, NULL);
    assignment.B =
      xbt_matrix_new_sub(B, submatrix_size, submatrix_size,
                         submatrix_size * line, submatrix_size * row, NULL);
    row++;
    if (row >= PROC_MATRIX_SIZE) {
      row = 0;
      line++;
    }

    gras_msg_send(socket[i], "pmm_slave", &assignment);
    xbt_matrix_free(assignment.A);
    xbt_matrix_free(assignment.B);
  }

  /* (have a rest while the slave perform the multiplication) */

  /* Retrieve the results */
  for (i = 0; i < SLAVE_COUNT; i++) {
    gras_msg_wait(6000, "result", &from, &result);
    VERB2("%d slaves are done already. Waiting for %d", i + 1, SLAVE_COUNT);
    xbt_matrix_copy_values(C, result.C, submatrix_size, submatrix_size,
                           submatrix_size * result.linepos,
                           submatrix_size * result.rowpos, 0, 0, NULL);
    xbt_matrix_free(result.C);
  }
  /*    end of gather   */

  if (DATA_MATRIX_SIZE < 30) {
    INFO0("The Result of Multiplication is :");
    xbt_matrix_dump(C, "C:res", 0, xbt_matrix_dump_display_double);
  } else {
    INFO1("Matrix size too big (%d>30) to be displayed here",
          DATA_MATRIX_SIZE);
  }

  amok_pm_group_shutdown("pmm");        /* Ok, we're out of here */

  for (i = 0; i < SLAVE_COUNT; i++) {
    gras_socket_close(socket[i]);
  }

  xbt_matrix_free(A);
  xbt_matrix_free(B);
  xbt_matrix_free(C);
  gras_exit();
  return 0;
}                               /* end_of_master */

/* **********************************************************************
 * slave code
 * **********************************************************************/

static int pmm_worker_cb(gras_msg_cb_ctx_t ctx, void *payload)
{
  /* Recover my initialized Data and My Position */
  s_pmm_assignment_t assignment = *(s_pmm_assignment_t *) payload;
  gras_socket_t master = gras_msg_cb_ctx_from(ctx);

  xbt_ex_t e;

  int step, l;
  xbt_matrix_t bA = xbt_matrix_new(submatrix_size, submatrix_size,
                                   sizeof(double), NULL);
  xbt_matrix_t bB = xbt_matrix_new(submatrix_size, submatrix_size,
                                   sizeof(double), NULL);

  int myline, myrow;
  xbt_matrix_t mydataA, mydataB;
  xbt_matrix_t bC =
    xbt_matrix_double_new_zeros(submatrix_size, submatrix_size);

  result_t result;

  gras_socket_t from;           /* to exchange data with my neighbor */

  /* sockets for brodcast to other slave */
  gras_socket_t socket_line[PROC_MATRIX_SIZE - 1];
  gras_socket_t socket_row[PROC_MATRIX_SIZE - 1];
  memset(socket_line, 0, sizeof(socket_line));
  memset(socket_row, 0, sizeof(socket_row));

  int i;

  gras_os_sleep(1);             /* wait for my pals */

  myline = assignment.linepos;
  myrow = assignment.rowpos;
  mydataA = assignment.A;
  mydataB = assignment.B;

  INFO2("Receive my pos (%d,%d) and assignment", myline, myrow);

  /* Get my neighborhood from the assignment message (skipping myself) */
  for (i = 0; i < PROC_MATRIX_SIZE - 1; i++) {
    socket_line[i] = gras_socket_client(assignment.line[i]->name,
                                        assignment.line[i]->port);
    xbt_peer_free(assignment.line[i]);
  }
  for (i = 0; i < PROC_MATRIX_SIZE - 1; i++) {
    socket_row[i] = gras_socket_client(assignment.row[i]->name,
                                       assignment.row[i]->port);
    xbt_peer_free(assignment.row[i]);
  }

  for (step = 0; step < PROC_MATRIX_SIZE; step++) {

    /* a line brodcast */
    if (myline == step) {
      INFO2("LINE: step(%d) = Myline(%d). Broadcast my data.", step, myline);
      for (l = 0; l < PROC_MATRIX_SIZE - 1; l++) {
        INFO1("LINE:   Send to %s", gras_socket_peer_name(socket_row[l]));
        gras_msg_send(socket_row[l], "dataB", &mydataB);
      }


      xbt_matrix_free(bB);
      bB = xbt_matrix_new_sub(mydataB,
                              submatrix_size, submatrix_size, 0, 0, NULL);
    } else {
      TRY {
        xbt_matrix_free(bB);
        gras_msg_wait(600, "dataB", &from, &bB);
      }
      CATCH(e) {
        RETHROW0("Can't get a data message from line : %s");
      }
      INFO3("LINE: step(%d) <> Myline(%d). Receive data from %s", step,
            myline, gras_socket_peer_name(from));
    }

    /* a row brodcast */
    if (myrow == step) {
      INFO2("ROW: step(%d)=myrow(%d). Broadcast my data.", step, myrow);
      for (l = 1; l < PROC_MATRIX_SIZE; l++) {
        INFO1("ROW:   Send to %s", gras_socket_peer_name(socket_line[l - 1]));
        gras_msg_send(socket_line[l - 1], "dataA", &mydataA);
      }
      xbt_matrix_free(bA);
      bA = xbt_matrix_new_sub(mydataA,
                              submatrix_size, submatrix_size, 0, 0, NULL);
    } else {
      TRY {
        xbt_matrix_free(bA);
        gras_msg_wait(1200, "dataA", &from, &bA);
      }
      CATCH(e) {
        RETHROW0("Can't get a data message from row : %s");
      }
      INFO3("ROW: step(%d)<>myrow(%d). Receive data from %s", step, myrow,
            gras_socket_peer_name(from));
    }
    xbt_matrix_double_addmult(bA, bB, bC);

  };

  /* send Result to master */
  result.C = bC;
  result.linepos = myline;
  result.rowpos = myrow;

  TRY {
    gras_msg_send(master, "result", &result);
  }
  CATCH(e) {
    RETHROW0("Failed to send answer to server: %s");
  }
  INFO2(">>>>>>>> Result sent to %s:%d <<<<<<<<",
        gras_socket_peer_name(master), gras_socket_peer_port(master));
  /*  Free the allocated resources, and shut GRAS down */

  xbt_matrix_free(bA);
  xbt_matrix_free(bB);
  xbt_matrix_free(bC);

  xbt_matrix_free(mydataA);
  xbt_matrix_free(mydataB);
  /* FIXME: some are said to be unknown 
     gras_socket_close(master);
     gras_socket_close(from);
     for (l=0; l < PROC_MATRIX_SIZE-1; l++) {
     if (socket_line[l])
     gras_socket_close(socket_line[l]);
     if (socket_row[l])
     gras_socket_close(socket_row[l]); 
     } */

  return 0;
}

int slave(int argc, char *argv[])
{
  gras_socket_t mysock;
  gras_socket_t master = NULL;
  int connected = 0;
  int rank;

  /* Init the GRAS's infrastructure */
  gras_init(&argc, argv);
  amok_pm_init();
  if (argc != 3 && argc != 2)
    xbt_die("Usage: slave masterhost:masterport [rank]");
  if (argc == 2)
    rank = -1;
  else
    rank = atoi(argv[2]);

  /*  Register the known messages and my callback */
  register_messages();
  gras_cb_register("pmm_slave", pmm_worker_cb);

  /* Create the connexions */
  mysock = gras_socket_server_range(3000, 9999, 0, 0);
  INFO1("Sensor %d starting", rank);
  while (!connected) {
    xbt_ex_t e;
    TRY {
      master = gras_socket_client_from_string(argv[1]);
      connected = 1;
    }
    CATCH(e) {
      if (e.category != system_error)
        RETHROW;
      xbt_ex_free(e);
      gras_os_sleep(0.5);
    }
  }

  /* Join and run the group */
  rank = amok_pm_group_join(master, "pmm");
  amok_pm_mainloop(600);

  /* housekeeping */
  gras_socket_close(mysock);
  //  gras_socket_close(master); Unknown
  gras_exit();
  return 0;
}                               /* end_of_slave */
