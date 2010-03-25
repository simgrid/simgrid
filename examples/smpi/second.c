/* A first simple SPMD example program using MPI                  */

/* The program consists of on receiver process and N-1 sender     */
/* processes. The sender processes send a message consisting      */
/* of their process identifier (id) and the total number of       */
/* processes (ntasks) to the receiver. The receiver process       */
/* prints out the values it receives in the messeges from the     */
/* senders.                                                       */

/* Compile the program with 'mpicc first.c -o first'              */
/* To run the program, using four of the computers specified in   */
/* your hostfile, do 'mpirun -machinefile hostfile -np 4 first    */

#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
  const int tag = 42;           /* Message tag */
  int id, ntasks, source_id, dest_id, err, i;
  MPI_Status status;
  int msg[2];                   /* Message array */

  err = MPI_Init(&argc, &argv); /* Initialize MPI */
  if (err != MPI_SUCCESS) {
    printf("MPI initialization failed!\n");
    exit(1);
  }
  err = MPI_Comm_size(MPI_COMM_WORLD, &ntasks); /* Get nr of tasks */
  err = MPI_Comm_rank(MPI_COMM_WORLD, &id);     /* Get id of this process */
  if (ntasks < 2) {
    printf("You have to use at least 2 processors to run this program\n");
    MPI_Finalize();             /* Quit if there is only one processor */
    exit(0);
  }

  if (id == 0) {                /* Process 0 (the receiver) does this */
    for (i = 1; i < ntasks; i++) {
      err = MPI_Recv(msg, 2, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);    /* Receive a message */
      source_id = status.MPI_SOURCE;    /* Get id of sender */
      printf("Received message %d %d from process %d\n", msg[0], msg[1],
             source_id);
    }
  } else {                      /* Processes 1 to N-1 (the senders) do this */
    msg[0] = id;                /* Put own identifier in the message */
    msg[1] = ntasks;            /* and total number of processes */
    dest_id = 0;                /* Destination address */
    sleep(3);
    err = MPI_Send(msg, 2, MPI_INT, dest_id, tag, MPI_COMM_WORLD);
  }

  err = MPI_Finalize();         /* Terminate MPI */
  if (id == 0)
    printf("Ready\n");
  return 0;
}
