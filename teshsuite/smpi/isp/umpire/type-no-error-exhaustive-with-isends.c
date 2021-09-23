/* -*- Mode: C; -*- */
/* Creator: Bronis R. de Supinski (bronis@llnl.gov)  */

/* type-no-error-exhaustive-with-isends.c -- send with weird types */

#ifndef lint
static char *rcsid =
  "$Header: /usr/gapps/asde/cvs-vault/umpire/tests/type-no-error-exhaustive-with-isends.c,v 1.4 2002/10/24 17:04:56 bronis Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"


typedef struct _test_basic_struct_t
{
  double the_double;
  char the_char;
}
test_basic_struct_t;


typedef struct _test_lb_ub_struct_t
{
  double dontsend_double1;
  double the_double_to_send;
  char   the_chars[8]; /* only send the first one... */
  double dontsend_double2;
}
test_lb_ub_struct_t;


#define TYPE_CONSTRUCTOR_COUNT 7
#define MSG_COUNT              3

/*
*/
#define RUN_TYPE_STRUCT
#define RUN_TYPE_VECTOR
#define RUN_TYPE_HVECTOR
#define RUN_TYPE_INDEXED
#define RUN_TYPE_HINDEXED
#define RUN_TYPE_CONTIGUOUS
/*
*/
#define RUN_TYPE_STRUCT_LB_UB
/*
*/

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  MPI_Comm comm = MPI_COMM_WORLD;
  char processor_name[128];
  int namelen = 128;
  int i, j, k, basic_extent;
  int blocklens[4], displs[4];
  MPI_Datatype structtypes[4];
  MPI_Datatype newtype[TYPE_CONSTRUCTOR_COUNT];
  MPI_Request aReq[TYPE_CONSTRUCTOR_COUNT];
  MPI_Status aStatus[TYPE_CONSTRUCTOR_COUNT];
#ifdef RUN_TYPE_STRUCT
  test_basic_struct_t struct_buf[MSG_COUNT];
#endif
#ifdef RUN_TYPE_VECTOR
  test_basic_struct_t vector_buf[7*MSG_COUNT];
#endif
#ifdef RUN_TYPE_HVECTOR
  test_basic_struct_t hvector_buf[44*MSG_COUNT];
#endif
#ifdef RUN_TYPE_INDEXED
  test_basic_struct_t indexed_buf[132*MSG_COUNT];
#endif
#ifdef RUN_TYPE_HINDEXED
  test_basic_struct_t hindexed_buf[272*MSG_COUNT];
#endif
#ifdef RUN_TYPE_CONTIGUOUS
  test_basic_struct_t contig_buf[2720*MSG_COUNT];
#endif
#ifdef RUN_TYPE_STRUCT_LB_UB
  test_lb_ub_struct_t struct_lb_ub_send_buf[MSG_COUNT];
  test_basic_struct_t struct_lb_ub_recv_buf[MSG_COUNT];
#endif

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  structtypes[0] = MPI_DOUBLE;
  structtypes[1] = MPI_CHAR;
  blocklens[0] = blocklens[1] = 1;
  displs[0] = 0;
  displs[1] = sizeof(double);

  MPI_Barrier (comm);

  /* create the types */
  MPI_Type_struct (2, blocklens, displs, structtypes, &newtype[0]);

  MPI_Type_extent (newtype[0], &basic_extent);
  if (basic_extent != sizeof (test_basic_struct_t)) {
    fprintf (stderr, "(%d): Unexpected extent for struct\n");
    MPI_Abort (MPI_COMM_WORLD, 666);
  }

  MPI_Type_vector (2, 3, 4, newtype[0], &newtype[1]);
  MPI_Type_hvector (3, 2, 15 * sizeof (test_basic_struct_t),
		    newtype[1], &newtype[2]);
  displs[1] = 2;
  MPI_Type_indexed (2, blocklens, displs, newtype[2], &newtype[3]);
  displs[1] = 140 * sizeof (test_basic_struct_t);
  MPI_Type_hindexed (2, blocklens, displs, newtype[3], &newtype[4]);
  MPI_Type_contiguous (10, newtype[4], &newtype[5]);

  structtypes[0] = MPI_LB;
  structtypes[1] = MPI_DOUBLE;
  structtypes[2] = MPI_CHAR;
  structtypes[3] = MPI_UB;
  blocklens[0] = blocklens[1] = blocklens[2] = blocklens[3] = 1;
  displs[0] = -sizeof(double);
  displs[1] = 0;
  displs[2] = sizeof(double);
  displs[3] = 2*sizeof(double)+8*sizeof(char);

  MPI_Type_struct (4, blocklens, displs, structtypes, &newtype[6]);

#ifdef RUN_TYPE_STRUCT
  MPI_Type_commit (&newtype[0]);
#endif

#ifdef RUN_TYPE_VECTOR
  MPI_Type_commit (&newtype[1]);
#endif

#ifdef RUN_TYPE_HVECTOR
  MPI_Type_commit (&newtype[2]);
#endif

#ifdef RUN_TYPE_INDEXED
  MPI_Type_commit (&newtype[3]);
#endif

#ifdef RUN_TYPE_HINDEXED
  MPI_Type_commit (&newtype[4]);
#endif

#ifdef RUN_TYPE_CONTIGUOUS
  MPI_Type_commit (&newtype[5]);
#endif

#ifdef RUN_TYPE_STRUCT_LB_UB
#ifndef RUN_TYPE_STRUCT
  /* need the struct type for the receive... */
  MPI_Type_commit (&newtype[0]);
#endif
  MPI_Type_commit (&newtype[6]);
#endif

  if (rank == 0) {
    /* initialize buffers */
    for (i = 0; i < MSG_COUNT; i++) {
#ifdef RUN_TYPE_STRUCT
      struct_buf[i].the_double = 1.0;
      struct_buf[i].the_char = 'a';
#endif

#ifdef RUN_TYPE_VECTOR
      for (j = 0; j < 7; j++) {
	vector_buf[i*7 + j].the_double = 1.0;
	vector_buf[i*7 + j].the_char = 'a';
      }
#endif

#ifdef RUN_TYPE_HVECTOR
      for (j = 0; j < 44; j++) {
	hvector_buf[i*44 + j].the_double = 1.0;
	hvector_buf[i*44 + j].the_char = 'a';
      }
#endif

#ifdef RUN_TYPE_INDEXED
      for (j = 0; j < 132; j++) {
	indexed_buf[i*132 + j].the_double = 1.0;
	indexed_buf[i*132 + j].the_char = 'a';
      }
#endif

#ifdef RUN_TYPE_HINDEXED
      for (j = 0; j < 272; j++) {
	hindexed_buf[i*272 + j].the_double = 1.0;
	hindexed_buf[i*272 + j].the_char = 'a';
      }
#endif

#ifdef RUN_TYPE_CONTIGUOUS
      for (j = 0; j < 2720; j++) {
	contig_buf[i*2720 + j].the_double = 1.0;
	contig_buf[i*2720 + j].the_char = 'a';
      }
#endif

#ifdef RUN_TYPE_STRUCT_LB_UB
      struct_lb_ub_send_buf[i].dontsend_double1 = 1.0;
      struct_lb_ub_send_buf[i].the_double_to_send = 1.0;
      for (j = 0; j < 8; j++)
	struct_lb_ub_send_buf[i].the_chars[j] = 'a';
      struct_lb_ub_send_buf[i].dontsend_double2 = 1.0;
#endif
    }

    /* set up the sends */
#ifdef RUN_TYPE_STRUCT
    MPI_Isend (struct_buf, MSG_COUNT, newtype[0], 1, 0, comm, &aReq[0]);
#else
    aReq[0] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_VECTOR
    MPI_Isend (vector_buf, MSG_COUNT, newtype[1], 1, 1, comm, &aReq[1]);
#else
    aReq[1] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_HVECTOR
    MPI_Isend (hvector_buf, MSG_COUNT, newtype[2], 1, 2, comm, &aReq[2]);
#else
    aReq[2] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_INDEXED
    MPI_Isend (indexed_buf, MSG_COUNT, newtype[3], 1, 3, comm, &aReq[3]);
#else
    aReq[3] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_HINDEXED
    MPI_Isend (hindexed_buf, MSG_COUNT, newtype[4], 1, 4, comm, &aReq[4]);
#else
    aReq[4] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_CONTIGUOUS
    MPI_Isend (contig_buf, MSG_COUNT, newtype[5], 1, 5, comm, &aReq[5]);
#else
    aReq[5] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_STRUCT_LB_UB
    MPI_Isend (&(struct_lb_ub_send_buf[0].the_double_to_send),
	       MSG_COUNT, newtype[6], 1, 6, comm, &aReq[6]);
#else
    aReq[6] = MPI_REQUEST_NULL;
#endif
  }
  else if (rank == 1) {
    /* initialize buffers */
    for (i = 0; i < MSG_COUNT; i++) {
#ifdef RUN_TYPE_STRUCT
      struct_buf[i].the_double = 2.0;
      struct_buf[i].the_char = 'b';
#endif

#ifdef RUN_TYPE_VECTOR
      for (j = 0; j < 7; j++) {
	vector_buf[i*7 + j].the_double = 2.0;
	vector_buf[i*7 + j].the_char = 'b';
      }
#endif

#ifdef RUN_TYPE_HVECTOR
      for (j = 0; j < 44; j++) {
	hvector_buf[i*44 + j].the_double = 2.0;
	hvector_buf[i*44 + j].the_char = 'b';
      }
#endif

#ifdef RUN_TYPE_INDEXED
      for (j = 0; j < 132; j++) {
	indexed_buf[i*132 + j].the_double = 2.0;
	indexed_buf[i*132 + j].the_char = 'b';
      }
#endif

#ifdef RUN_TYPE_HINDEXED
      for (j = 0; j < 272; j++) {
	hindexed_buf[i*272 + j].the_double = 2.0;
	hindexed_buf[i*272 + j].the_char = 'b';
      }
#endif

#ifdef RUN_TYPE_CONTIGUOUS
      for (j = 0; j < 2720; j++) {
	contig_buf[i*2720 + j].the_double = 2.0;
	contig_buf[i*2720 + j].the_char = 'b';
      }
#endif

#ifdef RUN_TYPE_STRUCT_LB_UB
      struct_lb_ub_recv_buf[i].the_double = 2.0;
      struct_lb_ub_recv_buf[i].the_char = 'b';
#endif
    }

    /* set up the receives... */
#ifdef RUN_TYPE_STRUCT
    MPI_Irecv (struct_buf, MSG_COUNT, newtype[0], 0, 0, comm, &aReq[0]);
#else
    aReq[0] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_VECTOR
    MPI_Irecv (vector_buf, MSG_COUNT, newtype[1], 0, 1, comm, &aReq[1]);
#else
    aReq[1] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_HVECTOR
    MPI_Irecv (hvector_buf, MSG_COUNT, newtype[2], 0, 2, comm, &aReq[2]);
#else
    aReq[2] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_INDEXED
    MPI_Irecv (indexed_buf, MSG_COUNT, newtype[3], 0, 3, comm, &aReq[3]);
#else
    aReq[3] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_HINDEXED
    MPI_Irecv (hindexed_buf, MSG_COUNT, newtype[4], 0, 4, comm, &aReq[4]);
#else
    aReq[4] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_CONTIGUOUS
    MPI_Irecv (contig_buf, MSG_COUNT, newtype[5], 0, 5, comm, &aReq[5]);
#else
    aReq[5] = MPI_REQUEST_NULL;
#endif

#ifdef RUN_TYPE_STRUCT_LB_UB
    MPI_Irecv (struct_lb_ub_recv_buf,
	       MSG_COUNT, newtype[0], 0, 6, comm, &aReq[6]);
#else
    aReq[6] = MPI_REQUEST_NULL;
#endif
  }

  if (rank == 0) {
    /* muck the holes... */
    for (i = 0; i < MSG_COUNT; i++) {
#ifdef RUN_TYPE_STRUCT
      /* no holes in struct_buf... */
#endif

#ifdef RUN_TYPE_VECTOR
      /* one hole in vector_buf... */
      vector_buf[i*7 + 3].the_double = 3.0;
      vector_buf[i*7 + 3].the_char = 'c';
#endif

#ifdef RUN_TYPE_HVECTOR
      /* eight holes in hvector_buf... */
      /* hole in first vector, first block... */
      hvector_buf[i*44 + 3].the_double = 3.0;
      hvector_buf[i*44 + 3].the_char = 'c';
      /* hole in second vector, first block... */
      hvector_buf[i*44 + 10].the_double = 3.0;
      hvector_buf[i*44 + 10].the_char = 'c';
      /* hole in between first and second vector blocks... */
      hvector_buf[i*44 + 14].the_double = 3.0;
      hvector_buf[i*44 + 14].the_char = 'c';
      /* hole in first vector, second block... */
      hvector_buf[i*44 + 18].the_double = 3.0;
      hvector_buf[i*44 + 18].the_char = 'c';
      /* hole in second vector, second block... */
      hvector_buf[i*44 + 25].the_double = 3.0;
      hvector_buf[i*44 + 25].the_char = 'c';
      /* hole in between second and third vector blocks... */
      hvector_buf[i*44 + 29].the_double = 3.0;
      hvector_buf[i*44 + 29].the_char = 'c';
      /* hole in first vector, third block... */
      hvector_buf[i*44 + 33].the_double = 3.0;
      hvector_buf[i*44 + 33].the_char = 'c';
      /* hole in second vector, third block... */
      hvector_buf[i*44 + 40].the_double = 3.0;
      hvector_buf[i*44 + 40].the_char = 'c';
#endif

#ifdef RUN_TYPE_INDEXED
      /* sixty holes in indexed_buf... */
      /* hole in first vector, first block, first hvector... */
      indexed_buf[i*132 + 3].the_double = 3.0;
      indexed_buf[i*132 + 3].the_char = 'c';
      /* hole in second vector, first block, first hvector... */
      indexed_buf[i*132 + 10].the_double = 3.0;
      indexed_buf[i*132 + 10].the_char = 'c';
      /* hole in between first and second vector blocks, first hvector... */
      indexed_buf[i*132 + 14].the_double = 3.0;
      indexed_buf[i*132 + 14].the_char = 'c';
      /* hole in first vector, second block, first hvector... */
      indexed_buf[i*132 + 18].the_double = 3.0;
      indexed_buf[i*132 + 18].the_char = 'c';
      /* hole in second vector, second block, first hvector... */
      indexed_buf[i*132 + 25].the_double = 3.0;
      indexed_buf[i*132 + 25].the_char = 'c';
      /* hole in between second and third vector blocks, first hvector... */
      indexed_buf[i*132 + 29].the_double = 3.0;
      indexed_buf[i*132 + 29].the_char = 'c';
      /* hole in first vector, third block, first hvector... */
      indexed_buf[i*132 + 33].the_double = 3.0;
      indexed_buf[i*132 + 33].the_char = 'c';
      /* hole in second vector, third block, first hvector... */
      indexed_buf[i*132 + 40].the_double = 3.0;
      indexed_buf[i*132 + 40].the_char = 'c';
      /* hole in between hvectors... */
      for (j = 0; j < 44; j++) {
	indexed_buf[i*132 + 44 + j].the_double = 3.0;
	indexed_buf[i*132 + 44 + j].the_char = 'c';
      }
      /* hole in first vector, first block, second hvector... */
      indexed_buf[i*132 + 3 + 88].the_double = 3.0;
      indexed_buf[i*132 + 3 + 88].the_char = 'c';
      /* hole in second vector, first block, second hvector... */
      indexed_buf[i*132 + 10 + 88].the_double = 3.0;
      indexed_buf[i*132 + 10 + 88].the_char = 'c';
      /* hole in between first and second vector blocks, second hvector... */
      indexed_buf[i*132 + 14 + 88].the_double = 3.0;
      indexed_buf[i*132 + 14 + 88].the_char = 'c';
      /* hole in first vector, second block, second hvector... */
      indexed_buf[i*132 + 18 + 88].the_double = 3.0;
      indexed_buf[i*132 + 18 + 88].the_char = 'c';
      /* hole in second vector, second block, second hvector... */
      indexed_buf[i*132 + 25 + 88].the_double = 3.0;
      indexed_buf[i*132 + 25 + 88].the_char = 'c';
      /* hole in between second and third vector blocks, second hvector... */
      indexed_buf[i*132 + 29 + 88].the_double = 3.0;
      indexed_buf[i*132 + 29 + 88].the_char = 'c';
      /* hole in first vector, third block, second hvector... */
      indexed_buf[i*132 + 33 + 88].the_double = 3.0;
      indexed_buf[i*132 + 33 + 88].the_char = 'c';
      /* hole in second vector, third block, second hvector... */
      indexed_buf[i*132 + 40 + 88].the_double = 3.0;
      indexed_buf[i*132 + 40 + 88].the_char = 'c';
#endif

#ifdef RUN_TYPE_HINDEXED
      /* one hundred twenty eight holes in hindexed_buf... */
      /* hole in first vector, first block, first hvector, index 1... */
      hindexed_buf[i*272 + 3].the_double = 3.0;
      hindexed_buf[i*272 + 3].the_char = 'c';
      /* hole in second vector, first block, first hvector, index 1... */
      hindexed_buf[i*272 + 10].the_double = 3.0;
      hindexed_buf[i*272 + 10].the_char = 'c';
      /* hole between first & second vector blocks, hvector 1, index 1... */
      hindexed_buf[i*272 + 14].the_double = 3.0;
      hindexed_buf[i*272 + 14].the_char = 'c';
      /* hole in first vector, second block, first hvector, index 1... */
      hindexed_buf[i*272 + 18].the_double = 3.0;
      hindexed_buf[i*272 + 18].the_char = 'c';
      /* hole in second vector, second block, first hvector, index 1... */
      hindexed_buf[i*272 + 25].the_double = 3.0;
      hindexed_buf[i*272 + 25].the_char = 'c';
      /* hole between second & third vector blocks, hvector 1, index 1... */
      hindexed_buf[i*272 + 29].the_double = 3.0;
      hindexed_buf[i*272 + 29].the_char = 'c';
      /* hole in first vector, third block, first hvector, index 1... */
      hindexed_buf[i*272 + 33].the_double = 3.0;
      hindexed_buf[i*272 + 33].the_char = 'c';
      /* hole in second vector, third block, first hvector, index 1... */
      hindexed_buf[i*272 + 40].the_double = 3.0;
      hindexed_buf[i*272 + 40].the_char = 'c';
      /* hole in between hvectors, index 1... */
      for (j = 0; j < 44; j++) {
	hindexed_buf[i*272 + 44 + j].the_double = 3.0;
	hindexed_buf[i*272 + 44 + j].the_char = 'c';
      }
      /* hole in first vector, first block, second hvector, index 1... */
      hindexed_buf[i*272 + 3 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 3 + 88].the_char = 'c';
      /* hole in second vector, first block, second hvector, index 1... */
      hindexed_buf[i*272 + 10 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 10 + 88].the_char = 'c';
      /* hole between first & second vector blocks, hvector 2, index 1... */
      hindexed_buf[i*272 + 14 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 14 + 88].the_char = 'c';
      /* hole in first vector, second block, second hvector, index 1... */
      hindexed_buf[i*272 + 18 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 18 + 88].the_char = 'c';
      /* hole in second vector, second block, second hvector, index 1... */
      hindexed_buf[i*272 + 25 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 25 + 88].the_char = 'c';
      /* hole between second & third vector blocks, hvector 2, index 1... */
      hindexed_buf[i*272 + 29 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 29 + 88].the_char = 'c';
      /* hole in first vector, third block, second hvector, index 1... */
      hindexed_buf[i*272 + 33 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 33 + 88].the_char = 'c';
      /* hole in second vector, third block, second hvector, index 1... */
      hindexed_buf[i*272 + 40 + 88].the_double = 3.0;
      hindexed_buf[i*272 + 40 + 88].the_char = 'c';
      /* indexed hole... */
      for (j = 0; j < 8; j++) {
	hindexed_buf[i*272 + 132 + j].the_double = 3.0;
	hindexed_buf[i*272 + 132 + j].the_char = 'c';
      }
      /* hole in first vector, first block, first hvector, index 2... */
      hindexed_buf[i*272 + 3 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 3 + 140].the_char = 'c';
      /* hole in second vector, first block, first hvector, index 2... */
      hindexed_buf[i*272 + 10 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 10 + 140].the_char = 'c';
      /* hole between first & second vector blocks, hvector 1, index 2... */
      hindexed_buf[i*272 + 14 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 14 + 140].the_char = 'c';
      /* hole in first vector, second block, first hvector, index 2... */
      hindexed_buf[i*272 + 18 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 18 + 140].the_char = 'c';
      /* hole in second vector, second block, first hvector, index 2... */
      hindexed_buf[i*272 + 25 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 25 + 140].the_char = 'c';
      /* hole between second & third vector blocks, hvector 1, index 2... */
      hindexed_buf[i*272 + 29 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 29 + 140].the_char = 'c';
      /* hole in first vector, third block, first hvector, index 2... */
      hindexed_buf[i*272 + 33 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 33 + 140].the_char = 'c';
      /* hole in second vector, third block, first hvector, index 2... */
      hindexed_buf[i*272 + 40 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 40 + 140].the_char = 'c';
      /* hole in between hvectors, index 2... */
      for (j = 0; j < 44; j++) {
	hindexed_buf[i*272 + 44 + j + 140].the_double = 3.0;
	hindexed_buf[i*272 + 44 + j + 140].the_char = 'c';
      }
      /* hole in first vector, first block, second hvector, index 2... */
      hindexed_buf[i*272 + 3 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 3 + 88 + 140].the_char = 'c';
      /* hole in second vector, first block, second hvector, index 2... */
      hindexed_buf[i*272 + 10 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 10 + 88 + 140].the_char = 'c';
      /* hole between first & second vector blocks, hvector 2, index 2... */
      hindexed_buf[i*272 + 14 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 14 + 88 + 140].the_char = 'c';
      /* hole in first vector, second block, second hvector, index 2... */
      hindexed_buf[i*272 + 18 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 18 + 88 + 140].the_char = 'c';
      /* hole in second vector, second block, second hvector, index 2... */
      hindexed_buf[i*272 + 25 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 25 + 88 + 140].the_char = 'c';
      /* hole between second & third vector blocks, hvector 2, index 2... */
      hindexed_buf[i*272 + 29 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 29 + 88 + 140].the_char = 'c';
      /* hole in first vector, third block, second hvector, index 2... */
      hindexed_buf[i*272 + 33 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 33 + 88 + 140].the_char = 'c';
      /* hole in second vector, third block, second hvector, index 2... */
      hindexed_buf[i*272 + 40 + 88 + 140].the_double = 3.0;
      hindexed_buf[i*272 + 40 + 88 + 140].the_char = 'c';
#endif

#ifdef RUN_TYPE_CONTIGUOUS
      for (j = 0; j < 10; j++) {
	/* hole in first vector, first block, first hvector, index 1... */
	contig_buf[i*2720 + j*272 + 3].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 3].the_char = 'c';
	/* hole in second vector, first block, first hvector, index 1... */
	contig_buf[i*2720 + j*272 + 10].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 10].the_char = 'c';
	/* hole between first & second vector blocks, hvector 1, index 1... */
	contig_buf[i*2720 + j*272 + 14].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 14].the_char = 'c';
	/* hole in first vector, second block, first hvector, index 1... */
	contig_buf[i*2720 + j*272 + 18].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 18].the_char = 'c';
	/* hole in second vector, second block, first hvector, index 1... */
	contig_buf[i*2720 + j*272 + 25].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 25].the_char = 'c';
	/* hole between second & third vector blocks, hvector 1, index 1... */
	contig_buf[i*2720 + j*272 + 29].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 29].the_char = 'c';
	/* hole in first vector, third block, first hvector, index 1... */
	contig_buf[i*2720 + j*272 + 33].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 33].the_char = 'c';
	/* hole in second vector, third block, first hvector, index 1... */
	contig_buf[i*2720 + j*272 + 40].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 40].the_char = 'c';
	/* hole in between hvectors, index 1... */
	for (k = 0; k < 44; k++) {
	  contig_buf[i*2720 + j*272 + 44 + k].the_double = 3.0;
	  contig_buf[i*2720 + j*272 + 44 + k].the_char = 'c';
	}
	/* hole in first vector, first block, second hvector, index 1... */
	contig_buf[i*2720 + j*272 + 3 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 3 + 88].the_char = 'c';
	/* hole in second vector, first block, second hvector, index 1... */
	contig_buf[i*2720 + j*272 + 10 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 10 + 88].the_char = 'c';
	/* hole between first & second vector blocks, hvector 2, index 1... */
	contig_buf[i*2720 + j*272 + 14 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 14 + 88].the_char = 'c';
	/* hole in first vector, second block, second hvector, index 1... */
	contig_buf[i*2720 + j*272 + 18 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 18 + 88].the_char = 'c';
	/* hole in second vector, second block, second hvector, index 1... */
	contig_buf[i*2720 + j*272 + 25 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 25 + 88].the_char = 'c';
	/* hole between second & third vector blocks, hvector 2, index 1... */
	contig_buf[i*2720 + j*272 + 29 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 29 + 88].the_char = 'c';
	/* hole in first vector, third block, second hvector, index 1... */
	contig_buf[i*2720 + j*272 + 33 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 33 + 88].the_char = 'c';
	/* hole in second vector, third block, second hvector, index 1... */
	contig_buf[i*2720 + j*272 + 40 + 88].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 40 + 88].the_char = 'c';
	/* indexed hole... */
	for (k = 0; k < 8; k++) {
	  contig_buf[i*2720 + j*272 + 132 + k].the_double = 3.0;
	  contig_buf[i*2720 + j*272 + 132 + k].the_char = 'c';
	}
	/* hole in first vector, first block, first hvector, index 2... */
	contig_buf[i*2720 + j*272 + 3 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 3 + 140].the_char = 'c';
	/* hole in second vector, first block, first hvector, index 2... */
	contig_buf[i*2720 + j*272 + 10 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 10 + 140].the_char = 'c';
	/* hole between first & second vector blocks, hvector 1, index 2... */
	contig_buf[i*2720 + j*272 + 14 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 14 + 140].the_char = 'c';
	/* hole in first vector, second block, first hvector, index 2... */
	contig_buf[i*2720 + j*272 + 18 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 18 + 140].the_char = 'c';
	/* hole in second vector, second block, first hvector, index 2... */
	contig_buf[i*2720 + j*272 + 25 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 25 + 140].the_char = 'c';
	/* hole between second & third vector blocks, hvector 1, index 2... */
	contig_buf[i*2720 + j*272 + 29 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 29 + 140].the_char = 'c';
	/* hole in first vector, third block, first hvector, index 2... */
	contig_buf[i*2720 + j*272 + 33 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 33 + 140].the_char = 'c';
	/* hole in second vector, third block, first hvector, index 2... */
	contig_buf[i*2720 + j*272 + 40 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 40 + 140].the_char = 'c';
	/* hole in between hvectors, index 2... */
	for (k = 0; k < 44; k++) {
	  contig_buf[i*2720 + j*272 + 44 + k + 140].the_double = 3.0;
	  contig_buf[i*2720 + j*272 + 44 + k + 140].the_char = 'c';
	}
	/* hole in first vector, first block, second hvector, index 2... */
	contig_buf[i*2720 + j*272 + 3 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 3 + 88 + 140].the_char = 'c';
	/* hole in second vector, first block, second hvector, index 2... */
	contig_buf[i*2720 + j*272 + 10 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 10 + 88 + 140].the_char = 'c';
	/* hole between first & second vector blocks, hvector 2, index 2... */
	contig_buf[i*2720 + j*272 + 14 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 14 + 88 + 140].the_char = 'c';
	/* hole in first vector, second block, second hvector, index 2... */
	contig_buf[i*2720 + j*272 + 18 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 18 + 88 + 140].the_char = 'c';
	/* hole in second vector, second block, second hvector, index 2... */
	contig_buf[i*2720 + j*272 + 25 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 25 + 88 + 140].the_char = 'c';
	/* hole between second & third vector blocks, hvector 2, index 2... */
	contig_buf[i*2720 + j*272 + 29 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 29 + 88 + 140].the_char = 'c';
	/* hole in first vector, third block, second hvector, index 2... */
	contig_buf[i*2720 + j*272 + 33 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 33 + 88 + 140].the_char = 'c';
	/* hole in second vector, third block, second hvector, index 2... */
	contig_buf[i*2720 + j*272 + 40 + 88 + 140].the_double = 3.0;
	contig_buf[i*2720 + j*272 + 40 + 88 + 140].the_char = 'c';
      }
#endif

#ifdef RUN_TYPE_STRUCT_LB_UB
      /* muck the double member and char member being sent... */
      struct_lb_ub_send_buf[i].dontsend_double1 = 3.0;
      for (j = 1; j < 8; j++)
	struct_lb_ub_send_buf[i].the_chars[j] = 'c';
      struct_lb_ub_send_buf[i].dontsend_double2 = 3.0;
#endif
    }
  }

  if ((rank == 0) || (rank == 1)) {
    /* wait on everything... */
    MPI_Waitall (TYPE_CONSTRUCTOR_COUNT, aReq, aStatus);
  }

  if (rank == 1) {
    /* check the holes... */
    for (i = 0; i < MSG_COUNT; i++) {
#ifdef RUN_TYPE_STRUCT
      /* no holes in struct_buf... */
#endif

#ifdef RUN_TYPE_VECTOR
      /* one hole in vector_buf... */
      assert ((vector_buf[i*7 + 3].the_double == 2.0) &&
	      (vector_buf[i*7 + 3].the_char == 'b'));
#endif

#ifdef RUN_TYPE_HVECTOR
      /* eight holes in hvector_buf... */
      /* hole in first vector, first block... */
      assert ((hvector_buf[i*44 + 3].the_double == 2.0) &&
	      (hvector_buf[i*44 + 3].the_char == 'b'));
      /* hole in second vector, first block... */
      assert ((hvector_buf[i*44 + 10].the_double == 2.0) &&
	      (hvector_buf[i*44 + 10].the_char == 'b'));
      /* hole in between first and second vector blocks... */
      assert ((hvector_buf[i*44 + 14].the_double == 2.0) &&
	      (hvector_buf[i*44 + 14].the_char == 'b'));
      /* hole in first vector, second block... */
      assert ((hvector_buf[i*44 + 18].the_double == 2.0) &&
	      (hvector_buf[i*44 + 18].the_char == 'b'));
      /* hole in second vector, second block... */
      assert ((hvector_buf[i*44 + 25].the_double == 2.0) &&
	      (hvector_buf[i*44 + 25].the_char == 'b'));
      /* hole in between second and third vector blocks... */
      assert ((hvector_buf[i*44 + 29].the_double == 2.0) &&
	      (hvector_buf[i*44 + 29].the_char == 'b'));
      /* hole in first vector, third block... */
      assert ((hvector_buf[i*44 + 33].the_double == 2.0) &&
	      (hvector_buf[i*44 + 33].the_char == 'b'));
      /* hole in second vector, third block... */
      assert ((hvector_buf[i*44 + 40].the_double == 2.0) &&
	      (hvector_buf[i*44 + 40].the_char == 'b'));
#endif

#ifdef RUN_TYPE_INDEXED
      /* sixty holes in indexed_buf... */
      /* hole in first vector, first block, first hvector... */
      assert ((indexed_buf[i*132 + 3].the_double == 2.0) &&
	      (indexed_buf[i*132 + 3].the_char == 'b'));
      /* hole in second vector, first block, first hvector... */
      assert ((indexed_buf[i*132 + 10].the_double == 2.0) &&
	      (indexed_buf[i*132 + 10].the_char == 'b'));
      /* hole in between first and second vector blocks, first hvector... */
      assert ((indexed_buf[i*132 + 14].the_double == 2.0) &&
	      (indexed_buf[i*132 + 14].the_char == 'b'));
      /* hole in first vector, second block, first hvector... */
      assert ((indexed_buf[i*132 + 18].the_double == 2.0) &&
	      (indexed_buf[i*132 + 18].the_char == 'b'));
      /* hole in second vector, second block, first hvector... */
      assert ((indexed_buf[i*132 + 25].the_double == 2.0) &&
	      (indexed_buf[i*132 + 25].the_char == 'b'));
      /* hole in between second and third vector blocks, first hvector... */
      assert ((indexed_buf[i*132 + 29].the_double == 2.0) &&
	      (indexed_buf[i*132 + 29].the_char == 'b'));
      /* hole in first vector, third block, first hvector... */
      assert ((indexed_buf[i*132 + 33].the_double == 2.0) &&
	      (indexed_buf[i*132 + 33].the_char == 'b'));
      /* hole in second vector, third block, first hvector... */
      assert ((indexed_buf[i*132 + 40].the_double == 2.0) &&
	      (indexed_buf[i*132 + 40].the_char == 'b'));
      /* hole in between hvectors... */
      for (j = 0; j < 44; j++) {
	assert ((indexed_buf[i*132 + 44 + j].the_double == 2.0) &&
		(indexed_buf[i*132 + 44 + j].the_char == 'b'));
      }
      /* hole in first vector, first block, second hvector... */
      assert ((indexed_buf[i*132 + 3 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 3 + 88].the_char == 'b'));
      /* hole in second vector, first block, second hvector... */
      assert ((indexed_buf[i*132 + 10 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 10 + 88].the_char == 'b'));
      /* hole in between first and second vector blocks, second hvector... */
      assert ((indexed_buf[i*132 + 14 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 14 + 88].the_char == 'b'));
      /* hole in first vector, second block, second hvector... */
      assert ((indexed_buf[i*132 + 18 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 18 + 88].the_char == 'b'));
      /* hole in second vector, second block, second hvector... */
      assert ((indexed_buf[i*132 + 25 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 25 + 88].the_char == 'b'));
      /* hole in between second and third vector blocks, second hvector... */
      assert ((indexed_buf[i*132 + 29 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 29 + 88].the_char == 'b'));
      /* hole in first vector, third block, second hvector... */
      assert ((indexed_buf[i*132 + 33 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 33 + 88].the_char == 'b'));
      /* hole in second vector, third block, second hvector... */
      assert ((indexed_buf[i*132 + 40 + 88].the_double == 2.0) &&
	      (indexed_buf[i*132 + 40 + 88].the_char == 'b'));
#endif

#ifdef RUN_TYPE_HINDEXED
      /* one hundred twenty eight holes in hindexed_buf... */
      /* hole in first vector, first block, first hvector, index 1... */
      assert ((hindexed_buf[i*272 + 3].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 3].the_char == 'b'));
      /* hole in second vector, first block, first hvector, index 1... */
      assert ((hindexed_buf[i*272 + 10].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 10].the_char == 'b'));
      /* hole between first & second vector blocks, hvector 1, index 1... */
      assert ((hindexed_buf[i*272 + 14].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 14].the_char == 'b'));
      /* hole in first vector, second block, first hvector, index 1... */
      assert ((hindexed_buf[i*272 + 18].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 18].the_char == 'b'));
      /* hole in second vector, second block, first hvector, index 1... */
      assert ((hindexed_buf[i*272 + 25].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 25].the_char == 'b'));
      /* hole between second & third vector blocks, hvector 1, index 1... */
      assert ((hindexed_buf[i*272 + 29].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 29].the_char == 'b'));
      /* hole in first vector, third block, first hvector, index 1... */
      assert ((hindexed_buf[i*272 + 33].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 33].the_char == 'b'));
      /* hole in second vector, third block, first hvector, index 1... */
      assert ((hindexed_buf[i*272 + 40].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 40].the_char == 'b'));
      /* hole in between hvectors, index 1... */
      for (j = 0; j < 44; j++) {
	assert ((hindexed_buf[i*272 + 44 + j].the_double == 2.0) &&
		(hindexed_buf[i*272 + 44 + j].the_char == 'b'));
      }
      /* hole in first vector, first block, second hvector, index 1... */
      assert ((hindexed_buf[i*272 + 3 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 3 + 88].the_char == 'b'));
      /* hole in second vector, first block, second hvector, index 1... */
      assert ((hindexed_buf[i*272 + 10 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 10 + 88].the_char == 'b'));
      /* hole between first & second vector blocks, hvector 2, index 1... */
      assert ((hindexed_buf[i*272 + 14 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 14 + 88].the_char == 'b'));
      /* hole in first vector, second block, second hvector, index 1... */
      assert ((hindexed_buf[i*272 + 18 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 18 + 88].the_char == 'b'));
      /* hole in second vector, second block, second hvector, index 1... */
      assert ((hindexed_buf[i*272 + 25 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 25 + 88].the_char == 'b'));
      /* hole between second & third vector blocks, hvector 2, index 1... */
      assert ((hindexed_buf[i*272 + 29 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 29 + 88].the_char == 'b'));
      /* hole in first vector, third block, second hvector, index 1... */
      assert ((hindexed_buf[i*272 + 33 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 33 + 88].the_char == 'b'));
      /* hole in second vector, third block, second hvector, index 1... */
      assert ((hindexed_buf[i*272 + 40 + 88].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 40 + 88].the_char == 'b'));
      /* indexed hole... */
      for (j = 0; j < 8; j++) {
	assert ((hindexed_buf[i*272 + 132 + j].the_double == 2.0) &&
		(hindexed_buf[i*272 + 132 + j].the_char == 'b'));
      }
      /* hole in first vector, first block, first hvector, index 2... */
      assert ((hindexed_buf[i*272 + 3 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 3 + 140].the_char == 'b'));
      /* hole in second vector, first block, first hvector, index 2... */
      assert ((hindexed_buf[i*272 + 10 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 10 + 140].the_char == 'b'));
      /* hole between first & second vector blocks, hvector 1, index 2... */
      assert ((hindexed_buf[i*272 + 14 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 14 + 140].the_char == 'b'));
      /* hole in first vector, second block, first hvector, index 2... */
      assert ((hindexed_buf[i*272 + 18 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 18 + 140].the_char == 'b'));
      /* hole in second vector, second block, first hvector, index 2... */
      assert ((hindexed_buf[i*272 + 25 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 25 + 140].the_char == 'b'));
      /* hole between second & third vector blocks, hvector 1, index 2... */
      assert ((hindexed_buf[i*272 + 29 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 29 + 140].the_char == 'b'));
      /* hole in first vector, third block, first hvector, index 2... */
      assert ((hindexed_buf[i*272 + 33 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 33 + 140].the_char == 'b'));
      /* hole in second vector, third block, first hvector, index 2... */
      assert ((hindexed_buf[i*272 + 40 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 40 + 140].the_char == 'b'));
      /* hole in between hvectors, index 2... */
      for (j = 0; j < 44; j++) {
	assert ((hindexed_buf[i*272 + 44 + j + 140].the_double == 2.0) &&
		(hindexed_buf[i*272 + 44 + j + 140].the_char == 'b'));
      }
      /* hole in first vector, first block, second hvector, index 2... */
      assert ((hindexed_buf[i*272 + 3 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 3 + 88 + 140].the_char == 'b'));
      /* hole in second vector, first block, second hvector, index 2... */
      assert ((hindexed_buf[i*272 + 10 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 10 + 88 + 140].the_char == 'b'));
      /* hole between first & second vector blocks, hvector 2, index 2... */
      assert ((hindexed_buf[i*272 + 14 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 14 + 88 + 140].the_char == 'b'));
      /* hole in first vector, second block, second hvector, index 2... */
      assert ((hindexed_buf[i*272 + 18 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 18 + 88 + 140].the_char == 'b'));
      /* hole in second vector, second block, second hvector, index 2... */
      assert ((hindexed_buf[i*272 + 25 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 25 + 88 + 140].the_char == 'b'));
      /* hole between second & third vector blocks, hvector 2, index 2... */
      assert ((hindexed_buf[i*272 + 29 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 29 + 88 + 140].the_char == 'b'));
      /* hole in first vector, third block, second hvector, index 2... */
      assert ((hindexed_buf[i*272 + 33 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 33 + 88 + 140].the_char == 'b'));
      /* hole in second vector, third block, second hvector, index 2... */
      assert ((hindexed_buf[i*272 + 40 + 88 + 140].the_double == 2.0) &&
	      (hindexed_buf[i*272 + 40 + 88 + 140].the_char == 'b'));
#endif

#ifdef RUN_TYPE_CONTIGUOUS
      for (j = 0; j < 10; j++) {
	/* hole in first vector, first block, first hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 3].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 3].the_char == 'b'));
	/* hole in second vector, first block, first hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 10].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 10].the_char == 'b'));
	/* hole between first & second vector blocks, hvector 1, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 14].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 14].the_char == 'b'));
	/* hole in first vector, second block, first hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 18].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 18].the_char == 'b'));
	/* hole in second vector, second block, first hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 25].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 25].the_char == 'b'));
	/* hole between second & third vector blocks, hvector 1, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 29].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 29].the_char == 'b'));
	/* hole in first vector, third block, first hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 33].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 33].the_char == 'b'));
	/* hole in second vector, third block, first hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 40].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 40].the_char == 'b'));
	/* hole in between hvectors, index 1... */
	for (k = 0; k < 44; k++) {
	  assert ((contig_buf[i*2720 + j*272 + 44 + k].the_double == 2.0) &&
		  (contig_buf[i*2720 + j*272 + 44 + k].the_char == 'b'));
	}
	/* hole in first vector, first block, second hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 3 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 3 + 88].the_char == 'b'));
	/* hole in second vector, first block, second hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 10 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 10 + 88].the_char == 'b'));
	/* hole between first & second vector blocks, hvector 2, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 14 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 14 + 88].the_char == 'b'));
	/* hole in first vector, second block, second hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 18 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 18 + 88].the_char == 'b'));
	/* hole in second vector, second block, second hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 25 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 25 + 88].the_char == 'b'));
	/* hole between second & third vector blocks, hvector 2, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 29 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 29 + 88].the_char == 'b'));
	/* hole in first vector, third block, second hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 33 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 33 + 88].the_char == 'b'));
	/* hole in second vector, third block, second hvector, index 1... */
	assert ((contig_buf[i*2720 + j*272 + 40 + 88].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 40 + 88].the_char == 'b'));
	/* indexed hole... */
	for (k = 0; k < 8; k++) {
	  assert ((contig_buf[i*2720 + j*272 + 132 + k].the_double == 2.0) &&
		  (contig_buf[i*2720 + j*272 + 132 + k].the_char == 'b'));
	}
	/* hole in first vector, first block, first hvector, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 3 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 3 + 140].the_char == 'b'));
	/* hole in second vector, first block, first hvector, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 10 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 10 + 140].the_char == 'b'));
	/* hole between first & second vector blocks, hvector 1, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 14 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 14 + 140].the_char == 'b'));
	/* hole in first vector, second block, first hvector, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 18 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 18 + 140].the_char == 'b'));
	/* hole in second vector, second block, first hvector, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 25 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 25 + 140].the_char == 'b'));
	/* hole between second & third vector blocks, hvector 1, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 29 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 29 + 140].the_char == 'b'));
	/* hole in first vector, third block, first hvector, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 33 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 33 + 140].the_char == 'b'));
	/* hole in second vector, third block, first hvector, index 2... */
	assert ((contig_buf[i*2720 + j*272 + 40 + 140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 40 + 140].the_char == 'b'));
	/* hole in between hvectors, index 2... */
	for (k = 0; k < 44; k++) {
	  assert ((contig_buf[i*2720+j*272+44+k+140].the_double == 2.0) &&
		  (contig_buf[i*2720 +j*272+44+k+140].the_char == 'b'));
	}
	/* hole in first vector, first block, second hvector, index 2... */
	assert ((contig_buf[i*2720+j*272+3+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 3 + 88 + 140].the_char == 'b'));
	/* hole in second vector, first block, second hvector, index 2... */
	assert ((contig_buf[i*2720+j*272+10+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 10 + 88 + 140].the_char == 'b'));
	/* hole between first & second vector blocks, hvector 2, index 2... */
	assert ((contig_buf[i*2720+j*272+14+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 14 + 88 + 140].the_char == 'b'));
	/* hole in first vector, second block, second hvector, index 2... */
	assert ((contig_buf[i*2720+j*272+18+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 18 + 88 + 140].the_char == 'b'));
	/* hole in second vector, second block, second hvector, index 2... */
	assert ((contig_buf[i*2720+j*272+25+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 25 + 88 + 140].the_char == 'b'));
	/* hole between second & third vector blocks, hvector 2, index 2... */
	assert ((contig_buf[i*2720+j*272+29+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 29 + 88 + 140].the_char == 'b'));
	/* hole in first vector, third block, second hvector, index 2... */
	assert ((contig_buf[i*2720+j*272+33+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 33 + 88 + 140].the_char == 'b'));
	/* hole in second vector, third block, second hvector, index 2... */
	assert ((contig_buf[i*2720+j*272+40+88+140].the_double == 2.0) &&
		(contig_buf[i*2720 + j*272 + 40 + 88 + 140].the_char == 'b'));
      }
#endif

#ifdef RUN_TYPE_STRUCT_LB_UB
      /* no holes in struct_lb_ub_recv_buf... */
#endif
    }
  }

  for (i = 0; i < TYPE_CONSTRUCTOR_COUNT; i++) {
    MPI_Type_free (&newtype[i]);
  }

  MPI_Barrier (comm);

  printf ("(%d) Finished normally\n", rank);
  MPI_Finalize ();
}

/* EOF */
